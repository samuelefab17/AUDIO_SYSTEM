#include "Arduino.h"
#include <math.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

//#define SERIAL_BAUD 9600
#define SERIAL_BAUD 115200

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
#define BUTTON1_PIN 18
#define BUTTON2_PIN 19
#define BUTTON3_PIN 23
#define BUTTON4_PIN 33

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

int letpot;
bool letbut1;
bool letbut2;
bool letbut3;
bool letbut4;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() 
{
  Serial.begin(SERIAL_BAUD); // comunicazione seriale
  dacWrite(AUDIO_OUT_PIN, 0); //uscita DAC

  pinMode(TUNING_PIN, OUTPUT); //pin di controllo della sintonia

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
  u8g2.begin();

  // task audio sul core 0
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 10000, NULL, 1, NULL, 0); 

 // task sintonia sul core 1
  xTaskCreatePinnedToCore(tuningTask, "TuningTask", 10000, NULL, 1, NULL, 1);
}


void loop() {}

float applyIIRFilter(float input, float coeffs[], float history[]) 
{
  float output = coeffs[0] * input + coeffs[1] * history[0] + coeffs[2] * history[1] - coeffs[3] * history[0] - coeffs[4] * history[1];
  history[1] = history[0]; // aggiorna la memoria dei valori precedenti
  history[0] = output;
  return output;
}