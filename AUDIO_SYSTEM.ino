#include "Arduino.h"
#include <math.h>


// pin di ingresso audio 
#define AUDIO_IN_PIN 36

// pin di uscita audio 
#define AUDIO_OUT_PIN 25

// frequenza di campionamento audio (campioni al secondo)
#define SAMPLE_RATE 20000

// dimensione del buffer audio (numero di campioni da elaborare alla volta)
#define BUFFER_SIZE 256

// pin di ingresso del potenziometro 
#define POT_PIN 34

// pin di uscita per il controllo del circuito di sintonia 
#define TUNING_PIN 26

// buffer per memorizzare i campioni audio
int audioBuffer[BUFFER_SIZE];

// fattori di equalizzazione 
float bassFactor = 1.2; //bassi 
float midFactor = 1.0;  //medi
float trebleFactor = 0.8; //alti

// stato dell'equalizzatore (attivo o disattivo)
bool equalizerEnabled = true;

// coefficienti dei filtri IIR 
float bassFilterCoeffs[] = {0.01, 0.02, 0.01, 0.9, -0.8};
float midFilterCoeffs[] = {0.1, 0.2, 0.1, 0.8, -0.6};
float trebleFilterCoeffs[] = {0.2, 0.4, 0.2, 0.7, -0.4};

// memoria per i valori filtrati precedenti 
float bassFilterHistory[2] = {0, 0};
float midFilterHistory[2] = {0, 0};
float trebleFilterHistory[2] = {0, 0};

void setup() 
{
  Serial.begin(115200); // comunicazione seriale
  dacWrite(AUDIO_OUT_PIN, 0); //uscita DAC
  pinMode(TUNING_PIN, OUTPUT); //pin di controllo della sintonia

  // task audio sul core 0
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 10000, NULL, 1, NULL, 0); 

 // task sintonia sul core 1
  xTaskCreatePinnedToCore(tuningTask, "TuningTask", 10000, NULL, 1, NULL, 1);
}


void loop() {}


void audioTask(void * parameter) 
{
  for (;;) 
  { 
    for (int i = 0; i < BUFFER_SIZE; i++) 
    { 
      audioBuffer[i] = analogRead(AUDIO_IN_PIN); 

      float bassFiltered = applyIIRFilter(audioBuffer[i], bassFilterCoeffs, bassFilterHistory);
      float midFiltered = applyIIRFilter(audioBuffer[i], midFilterCoeffs, midFilterHistory);
      float trebleFiltered = applyIIRFilter(audioBuffer[i], trebleFilterCoeffs, trebleFilterHistory);

      if (equalizerEnabled) 
      {
        audioBuffer[i] = (bassFiltered * bassFactor) + (midFiltered * midFactor) + (trebleFiltered * trebleFactor);
      }

      audioBuffer[i] = constrain(audioBuffer[i], 0, 255); // limita il segnale all'intervallo del DAC
      dacWrite(AUDIO_OUT_PIN, audioBuffer[i]); // invia il campione all'uscita DAC
      delayMicroseconds(1000000 / SAMPLE_RATE); // attende per mantenere la frequenza di campionamento
    }
    if (Serial.available() > 0) 
    {
      char command = Serial.read();
      if (command == 'e') 
      {
        equalizerEnabled = !equalizerEnabled; // inverte lo stato dell'equalizzatore
        Serial.print("Equalizzatore: ");
        Serial.println(equalizerEnabled ? "Attivo" : "Disattivo");
      }
    }
  }
}

void tuningTask(void * parameter) 
{
  for (;;) 
  { 
    int potValue = analogRead(POT_PIN); // legge il valore del potenziometro
    int tuningValue = map(potValue, 0, 1023, 0, 255); // mappa il valore al range di controllo
    analogWrite(TUNING_PIN, tuningValue); // invia il valore al circuito di sintonia
    delay(10);
  }
}

float applyIIRFilter(float input, float coeffs[], float history[]) 
{
  float output = coeffs[0] * input + coeffs[1] * history[0] + coeffs[2] * history[1] - coeffs[3] * history[0] - coeffs[4] * history[1];
  history[1] = history[0]; // aggiorna la memoria dei valori precedenti
  history[0] = output;
  return output;
}