// OpenReflow controller
// Made by @CarlosGS
// https://github.com/CarlosGS/OpenReflow
// CC-BY-SA license

#include "ESP8266WiFi.h"

#define BUZZER_PIN 12
void init_buzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}
void play_tone(int frequency, int duration_ms) {
  tone(BUZZER_PIN, frequency, duration_ms);
  delay(duration_ms+10);
  digitalWrite(BUZZER_PIN, LOW);
}
void play_startup() {
  play_tone(400, 200);
  play_tone(600, 200);
  play_tone(800, 600);
}
void play_error() {
  play_tone(800, 600);
  delay(200);
  play_tone(800, 600);
  delay(200);
  play_tone(800, 600);
  delay(200);
  play_tone(800, 600);
  delay(200);
}
void play_reflow_begin() {
  play_tone(400, 100);
  play_tone(600, 100);
  play_tone(800, 100);
  
  play_tone(400, 100);
  play_tone(600, 100);
  play_tone(800, 100);
  
  play_tone(400, 100);
  play_tone(600, 100);
  play_tone(800, 600);
}
void play_reflow_end() {
  play_tone(800, 200);
  delay(50);
  play_tone(800, 1000);
}

#define RELAY_PIN 13
#define ZERO_CROSS_PIN 14
void init_relay() {
  pinMode(ZERO_CROSS_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}
void set_relay(bool state) {
  while(digitalRead(ZERO_CROSS_PIN) == HIGH) yield();
  while(digitalRead(ZERO_CROSS_PIN) == LOW) yield();
  digitalWrite(RELAY_PIN, state);
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("\n\n ____________________");
  Serial.println(    "| |  ____________    |");
  Serial.println(    "| | |            | O |");
  Serial.println(    "| | | OpenReflow |   |");
  Serial.println(   "\\ | |____________| O |");
  Serial.println(   " \\|__________________|\n");
  
  Serial.println("Initializing pins...\n");

  init_relay();
  init_buzzer();
  
  Serial.println("Setting up AP...\n");
  WiFi.softAP("OpenReflow", "OpenReflow");

  Serial.println("All set! Now connect your phone to the OpenReflow WiFi access point!\n");
  play_startup();
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float Vin = 3.3; // volts
float R2 = 50; // ohms

float temp() {
  double Vout_filt = 0;
  int N = 100;
  for(int i=0; i<N; i++) {
    Vout_filt += analogRead(A0);
    delay(1);
  }
  Vout_filt /= (double)N;
  float Vout = mapf(Vout_filt, 227,896.95, 0.202,0.833);// Calibrated ADC values
  float R1 = R2*((Vin/Vout)-1);
  float temp_degreesC = ((R1/100)-1)/0.00385;
  return temp_degreesC;
}

float value_filtered = 0;

int reflow_phase = 0;

float last_temperature = 0;
void loop() {
  int heating = 0;
  float temperature = temp();
  float temp_speed = (temperature-last_temperature)*2;// the 2 scales to degreesC/second

  switch(reflow_phase) {
    case 0: // Ramp-up
      heating = temp_speed < 3; // max 3degC/s
      if(temperature > 150) {
        reflow_phase = 1;
        play_startup();
      }
      break;
    case 1: // Preheat
      heating = temp_speed < 0.8; // max 0.8degC/s
      if(temperature > 200) {
        reflow_phase = 2;
        play_reflow_begin();
      }
      break;
    case 2: // Reflow
      heating = 1;
      if(temperature > 235) {
        reflow_phase = 3;
        play_reflow_end();
      }
      break;
    case 3: // Cool
      heating = 0;
      break;
  }
  set_relay(heating);
  Serial.print(millis());
  Serial.print("\t");
  Serial.print(reflow_phase);
  Serial.print("\t");
  Serial.print(temperature);
  Serial.print("\t");
  Serial.println(heating);
  delay(500);
  last_temperature = temperature;
}
