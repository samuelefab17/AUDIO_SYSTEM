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
