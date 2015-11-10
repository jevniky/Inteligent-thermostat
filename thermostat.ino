#include <OneWire.h>
#include <DallasTemperature.h>
#include <UIPEthernet.h>
#include <PubSubClient.h>
#include "config.h"

/*TEMP INIT*/
float setTemp = MIN_POS*0.5;

void setup() {
  pinMode(pinA, INPUT_PULLUP); // enabling pullups
  pinMode(pinB, INPUT_PULLUP); 
  pinMode(pinSW, INPUT_PULLUP);

// encoder pin on interrupt 0 (pin 2)
  attachInterrupt(0, doEncoderA, CHANGE);
// encoder pin on interrupt 1 (pin 3)
  attachInterrupt(1, doEncoderB, CHANGE);

  Serial.begin(9600);

  sensors.begin();
  sensors.setResolution(thermometer, 9);

  mqttClient.setServer("mqtt.polyrem.sk", 1883);
  mqttClient.setCallback(callback);
  
  connectToEthernet();
  delay(1500);
}

// main loop, work is done by interrupt service routines, this one only prints stuff
void loop() { 
  rotating = true;  // reset the debouncer
  //to protect from going over min & max values
  if (encoderPos <= MIN_POS-1 ){
    encoderPos = MIN_POS;
    lastReportedPos = MIN_POS+1;
  } else if (encoderPos >= MAX_POS+1){
    encoderPos = MAX_POS;
    lastReportedPos = MAX_POS-1;
  } else {
    if (lastReportedPos != encoderPos) { // Serial print
      setTemp = encoderPos*0.5;
      Serial.print("Set temperature: ");
      Serial.println(setTemp);
      lastReportedPos = encoderPos;
    }
  }
  if (digitalRead(pinSW) == LOW )  { // push to restart to 0
    encoderPos = 0;
    lastReportedPos = 1;
    delay (10);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //request temperature from sensors
    sensors.requestTemperatures();
    //print temperature out with my function
    printTemperature(thermometer);
  }
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}

// Interrupt on A changing state
void doEncoderA(){
  if ( rotating ) delay (1);  // wait a little until the bouncing is done
  // Test transition, did things really change? 
  if( digitalRead(pinA) != A_set ) {  // debounce once more
    A_set = !A_set;
    // adjust counter + if A leads B
    if ( A_set && !B_set ) encoderPos += 1;
    rotating = false;  // no more debouncing until loop() hits again
  }
}
// Interrupt on B changing state, same as A above
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(pinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if( B_set && !A_set ) encoderPos -= 1;
    rotating = false;
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress) {
  float temperature = sensors.getTempC(deviceAddress);
  Serial.print("Got temperature: ");
  Serial.println(temperature);
  char* temp = (char)temperature;
  mqttClient.publish("temperature",temp);
}

void connectToEthernet() {
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("outTopic","hello world");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
