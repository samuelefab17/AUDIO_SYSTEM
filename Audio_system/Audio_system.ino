#include "Arduino.h"
#include <math.h>

// --- Definizioni e variabili globali ---

// Pin di ingresso audio (segnale dal circuito di sintonia)
#define AUDIO_IN_PIN 36

// Pin di uscita audio (segnale elaborato verso l'amplificatore)
#define AUDIO_OUT_PIN 25

// Frequenza di campionamento audio (campioni al secondo)
#define SAMPLE_RATE 20000

// Dimensione del buffer audio (numero di campioni da elaborare alla volta)
#define BUFFER_SIZE 256

// Pin di ingresso del potenziometro (per il controllo della sintonia)
#define POT_PIN 34

// Pin di uscita per il controllo del circuito di sintonia (es. PWM)
#define TUNING_PIN 26

// Buffer per memorizzare i campioni audio
int audioBuffer[BUFFER_SIZE];

// Fattori di equalizzazione (per regolare bassi, medi e alti)
float bassFactor = 1.2; // Aumenta i bassi del 20%
float midFactor = 1.0;  // Nessuna modifica ai medi
float trebleFactor = 0.8; // Diminuisce gli alti del 20%

// Stato dell'equalizzatore (attivo o disattivo)
bool equalizerEnabled = true;

// Coefficienti dei filtri IIR (per separare le bande di frequenza)
float bassFilterCoeffs[] = {0.01, 0.02, 0.01, 0.9, -0.8};
float midFilterCoeffs[] = {0.1, 0.2, 0.1, 0.8, -0.6};
float trebleFilterCoeffs[] = {0.2, 0.4, 0.2, 0.7, -0.4};

// Memoria per i valori filtrati precedenti (necessaria per i filtri IIR)
float bassFilterHistory[2] = {0, 0};
float midFilterHistory[2] = {0, 0};
float trebleFilterHistory[2] = {0, 0};

// --- Funzione di setup (eseguita una sola volta all'avvio) ---
void setup() 
{
  Serial.begin(115200); // Inizializza la comunicazione seriale
  dacWrite(AUDIO_OUT_PIN, 0); // Inizializza l'uscita DAC (imposta a 0V)
  pinMode(TUNING_PIN, OUTPUT); // Imposta il pin di controllo della sintonia come uscita
  // Crea i task (attività) sui core specifici
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 10000, NULL, 1, NULL, 0); // Task audio sul core 0
  xTaskCreatePinnedToCore(tuningTask, "TuningTask", 10000, NULL, 1, NULL, 1); // Task sintonia sul core 1
}

// --- Funzione loop vuota (i task gestiscono tutto) ---
void loop() {}

// --- Task di elaborazione audio (eseguito sul core 0) ---
void audioTask(void * parameter) 
{
  for (;;) 
  { 
    for (int i = 0; i < BUFFER_SIZE; i++) 
    { 
      audioBuffer[i] = analogRead(AUDIO_IN_PIN); // Legge il campione dal pin analogico

      // Applica i filtri IIR per separare le bande di frequenza
      float bassFiltered = applyIIRFilter(audioBuffer[i], bassFilterCoeffs, bassFilterHistory);
      float midFiltered = applyIIRFilter(audioBuffer[i], midFilterCoeffs, midFilterHistory);
      float trebleFiltered = applyIIRFilter(audioBuffer[i], trebleFilterCoeffs, trebleFilterHistory);

      // Applica l'equalizzazione se abilitata
      if (equalizerEnabled) 
      {
        audioBuffer[i] = (bassFiltered * bassFactor) + (midFiltered * midFactor) + (trebleFiltered * trebleFactor);
      }

      audioBuffer[i] = constrain(audioBuffer[i], 0, 255); // Limita il segnale all'intervallo del DAC
      dacWrite(AUDIO_OUT_PIN, audioBuffer[i]); // Invia il campione all'uscita DAC
      delayMicroseconds(1000000 / SAMPLE_RATE); // Attende per mantenere la frequenza di campionamento
    }
    // Gestisce i comandi dalla seriale (per attivare/disattivare l'equalizzatore)
    if (Serial.available() > 0) 
    {
      char command = Serial.read();
      if (command == 'e') 
      {
        equalizerEnabled = !equalizerEnabled; // Inverte lo stato dell'equalizzatore
        Serial.print("Equalizzatore: ");
        Serial.println(equalizerEnabled ? "Attivo" : "Disattivo");
      }
    }
  }
}

// --- Task di controllo della sintonia (eseguito sul core 1) ---
void tuningTask(void * parameter) 
{
  for (;;) 
  { // Loop infinito
    int potValue = analogRead(POT_PIN); // Legge il valore del potenziometro
    int tuningValue = map(potValue, 0, 1023, 0, 255); // Mappa il valore al range di controllo
    analogWrite(TUNING_PIN, tuningValue); // Invia il valore al circuito di sintonia
    delay(10); // Piccolo ritardo
  }
}

// --- Funzione per applicare un filtro IIR ---
float applyIIRFilter(float input, float coeffs[], float history[]) 
{
  float output = coeffs[0] * input + coeffs[1] * history[0] + coeffs[2] * history[1] - coeffs[3] * history[0] - coeffs[4] * history[1];
  history[1] = history[0]; // Aggiorna la memoria dei valori precedenti
  history[0] = output;
  return output;
}