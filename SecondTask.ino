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

void update_button()
{
  letpot = analogRead(POT_PIN);
  letbut1 = digitalRead(BUTTON1_PIN);
  letbut2 = digitalRead(BUTTON2_PIN);
  letbut3 = digitalRead(BUTTON3_PIN);
  letbut4 = digitalRead(BUTTON4_PIN);
}