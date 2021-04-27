/* Authors: Jessica Pan and Felicia Zhu
 * Advisor: Michael Littman
 * Spring 2021
/* ------------------------------------------------------------------------ */
// library imports
/* ------------------------------------------------------------------------ */
#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include "config.h"

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/* ------------------------------------------------------------------------ */
// define all of the pinouts
#define BUZZER_PIN 2
#define SERVO_PIN 0
#define TOUCH_PIN 13
#define NEOPIXEL_PIN 5
#define DISTANCE_PIN_TRIG 12 // Trigger pin of ultrasonic sensor
#define DISTANCE_PIN_ECHO 14 // Echo pin of ultrasonic sensor

#define NUM_PIXELS 5

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define BLUEFRUIT_HWSERIAL_NAME           Serial1
#define BLUEFRUIT_UART_MODE_PIN           -1
#define BLUEFRUIT_UART_CTS_PIN            -1
#define BLUEFRUIT_UART_RTS_PIN            -1
#define BLE_READPACKET_TIMEOUT      10000

/* ------------------------------------------------------------------------ */
// create objects
Servo myservo;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
AdafruitIO_Feed *touch_light = io.feed("touchSensor");

Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ------------------------------------------------------------------------ */
// store distance measured by distance sensor
float distance = 0;

// store values used by touch sensors / AdafruitIO
// set one of the seals to 1 and the other to 2
int sealVal = 1;
int tap = 0;
int recVal = 0;
unsigned long previousMillis = 0;
const long interval = 10000;

// store light color values
uint8_t light_red = 104;
uint8_t light_green = 0;
uint8_t light_blue = 204;

/* ------------------------------------------------------------------------ */
// helper functions and variables for Bluefruit functionality
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

/* ------------------------------------------------------------------------ */
// runs one time when first starting
void setup() {
  Serial.begin(115200);
  Serial.println("Paired Partner Plush starting...");

  // 1. Setup distance sensor
  pinMode(DISTANCE_PIN_TRIG, OUTPUT);
  pinMode(DISTANCE_PIN_ECHO, INPUT);
  Serial.println("Ultrasonic sensor started!");

  // 2. Setup servo
  myservo.attach(SERVO_PIN);
  Serial.println("Servo started!");

  // 3. Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Buzzer started!");

  // 4. Activate NeoPixels
  strip.begin();
  flash();
  Serial.println("NeoPixels started!");

  // 5. Touch sensor
  pinMode(TOUCH_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(TOUCH_PIN), touchSensor, CHANGE);

  // 6. Start Connecting to Adafruit IO
  Serial.println("Connecting to Adafruit IO...");
  touch_light -> onMessage(handleMessage);

  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin();
    delay(500);
  }

  // 6.5. Adafruit IO connected
  Serial.println("Connected to Adafruit IO!");
  Serial.println(io.statusText());
  flash();
  flash();

  touch_light -> get();
  previousMillis = millis();
  
  // 7. Adafruit Bluefruit
  Serial.println("Adafruit Bluefruit connecting");
  if (!ble.begin(VERBOSE_MODE)) {
    error(F("Couldn't find Bluefruit"));
  }
  
  // perform factory reset to make sure everything in known state
  if (FACTORYRESET_ENABLE) {
    Serial.println("Performing factory reset...");
    if (!ble.factoryReset()) {
      error(F("Couldn't factory reset"));
    }
  }

  // disable command echo from Bluefruit
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  ble.info();

  ble.verbose(false);

  while (!ble.isConnected()) {
    delay(500);
  }
  Serial.println("Bluefruit connected!");

  // LED Activity command only supported from 0.6.6
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set Bluefruit to DATA mode
  Serial.println("Switching to DATA mode!");
  ble.setMode(BLUEFRUIT_MODE_DATA);
  
}

/* ------------------------------------------------------------------------ */
// runs repeatedly
void loop() {
  // 1. Calculate distance and move tail / buzz if object is close
  distance = getDistance();
  if (distance <= 10) {
    Serial.println("Object detected");
    // wiggle the servo
    myservo.write(10);
    delay(100);

    myservo.write(150);
    delay(100);

    playTune();
  }

  // 2. Keep the ESP8266 connected to AdafruitIO
  io.run();

  // 3. Touch sensor activated
  unsigned long currentMillis = millis();
  if (tap == 1) {
    Serial.println("Tapped!");
    if (currentMillis - previousMillis >= interval) {
      off();
      tap = 0;
    } else {
      rainbow(50);
    }
  }

  // 4. Bluefruit data arrives
  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  if (len == 0) return;

  // 4a. Waggle
  if (packetbuffer[1] == 'W') {
    // wiggle the servo
    myservo.write(10);
    delay(100);

    myservo.write(150);
    delay(100);
  }

  // 4b. Bark
  if (packetbuffer[1] == 'B') {
    playTune();
  }

  // 4c. Light up
  if (packetbuffer[1] == 'L') {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, light_red, light_green, light_red);
    }
    strip.show();
    delay(1000);
    
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
  }
  
}

/* ------------------------------------------------------------------------ */
// code to flash NeoPixels when stable connection made
void flash() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 255, 255, 255);
  }
  strip.show();
  delay(200);

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  delay(200);
}

/* ------------------------------------------------------------------------ */
// gets the distance measurements from the ultrasonic sensor
float getDistance() {
  float echoTime; // time it takes for a ping to bounce off object
  float calculatedDistance; // variable to store distance calculated from echo time

  // send out ultrasonic pulse that is 10ms long
  digitalWrite(DISTANCE_PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(DISTANCE_PIN_TRIG, LOW);

  echoTime = pulseIn(DISTANCE_PIN_ECHO, HIGH);

  calculatedDistance = echoTime / 148.0;

  return calculatedDistance;
}

/* ------------------------------------------------------------------------ */
// plays a specified tone (in C major)
void play(char note, int beats) {
  int numNotes = 14;
  
  char notes[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C', 'D', 'E', 'F', 'G', 'A', 'B', ' '};
  //this array matches frequencies with each letter (e.g. the 4th note is 'f', the 4th frequency is 175)
  int frequencies[] = {131, 147, 165, 175, 196, 220, 247, 262, 294, 330, 349, 392, 440, 494, 0};

  int currentFrequency = 0;
  int beatLength = 150; 

  //look up the frequency that corresponds to the note
  for (int i = 0; i < numNotes; i++) {
    if (notes[i] == note){
      currentFrequency = frequencies[i];
    }
  }
  
  //play the frequency that matched our letter for the number of beats passed to the play function
  tone(BUZZER_PIN, currentFrequency, beats * beatLength);
  delay(beats * beatLength);  //wait for the length of the tone so that it has time to play
  delay(50); //a little delay between the notes makes the song sound more natural

}

/* ------------------------------------------------------------------------ */
// seal plays a little tune
void playTune() {
  Serial.println("Playing tune");
  play('g', 2);       //ha
  play('g', 1);       //ppy
  play('a', 4);       //birth
  play('g', 4);       //day
  play('C', 4);       //to
  play('b', 4);       //you
}

/* ------------------------------------------------------------------------ */
// the interrupt program that runs when the flipper touch sensor is activated
ICACHE_RAM_ATTR void touchSensor() {
  // while touch sensor is active, save sealVal to AdafruitIO feed
  while (digitalRead(TOUCH_PIN) == 1) {
    touch_light -> save(sealVal);
    for(int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, light_red, light_green, light_blue);
    }
    strip.show();
  }

  // once touch sensor isn't active, send 0 back to Adafruit IO feed
  if (digitalRead(TOUCH_PIN) == 0) {
    touch_light -> save(0);
    for(int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
  }
}

/* ------------------------------------------------------------------------ */
// code that tells ESP8266 what to do when it receives Adafruit IO data
void handleMessage(AdafruitIO_Data *data) {
  Serial.print("Received Message!: ");

  // convert received data to int
  int reading = data -> toInt();

  if (reading == recVal && tap == 0) {
    Serial.println("Tapped!");
    previousMillis = millis();
    tap = 1;
  } else {
    Serial.println("Lamp already on");
  }
}

/* ------------------------------------------------------------------------ */
// turn all NeoPixels off
void off() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
}

/* ------------------------------------------------------------------------ */
// rainbow phasing code
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

/* ------------------------------------------------------------------------ */
//complicated geometry 
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/* ------------------------------------------------------------------------ */
// spinning animation while connecting to Adafruit IO
void spin() {
  Serial.print("spin");
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 255, 0, 0);
    strip.show();
    delay(20);
  }
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
    delay(20);
  }
}
