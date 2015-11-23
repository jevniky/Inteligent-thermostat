#include <OneWire.h>
#include <DallasTemperature.h>
#include <UIPEthernet.h>
#include <PubSubClient.h>
#include "config.h"


void setup() {
    //KY-40:
    pinMode(pinA, INPUT_PULLUP); // enabling pullups
    pinMode(pinB, INPUT_PULLUP);
    pinMode(pinSW, INPUT_PULLUP);

    //set up output pins
    pinMode(heatPin,OUTPUT); // relays are connected here
    pinMode(coolPin,OUTPUT);

    // encoder pin on interrupt 0 (pin 2)
    attachInterrupt(0, doEncoderA, CHANGE);
    // encoder pin on interrupt 1 (pin 3)
    attachInterrupt(1, doEncoderB, CHANGE);

    Serial.begin(9600);

    sensors.begin();
    // set the resolution to 9 bit. You can set it up to 12 if you want really precise values
    sensors.setResolution(thermometer, 9);

    connectToInternet();
    delay(1500); // allow the hardware to sort itself out

    mqttClient.setServer("mqtt.polyrem.sk", 1883); // set server address and port
mqttClient.setCallback(callback); // set callback function (function that handles incoming messages
}

void loop() {
    if (!mqttClient.connected()) { // if the client is not connected - reconnect
        reconnect();
    }
  
    rotating = true; // reset the debouncer
    //to protect from going over min & max values
    if (encoderPos <= MIN_POS - 1 ) {
        encoderPos = MIN_POS;
        lastReportedPos = MIN_POS + 1;
    } else if (encoderPos >= MAX_POS + 1) {
        encoderPos = MAX_POS;
        lastReportedPos = MAX_POS - 1;
    } else {
        // is between min and max values
        if (lastReportedPos != encoderPos) {
            // divide encoder position value by 2 to get 0.5 degree precision
            // store this value as set temperature
            setTemp = encoderPos * 0.5;
            // and print it to serial monitor
            Serial.print("Set: ");
            Serial.println(setTemp);
            dtostrf(setTemp, 3, 2, setTempBuff);
            mqttClient.publish("setTemp", setTempBuff);
            lastReportedPos = encoderPos;
        }
    }
    // in case you press the integrated button on a KY-40
    if (digitalRead(pinSW) == LOW ) {
        // restart to minimal position
        encoderPos = MIN_POS;
        lastReportedPos = MIN_POS+1;
        delay (10);
        // you can restar to default 20 degrees for example, or do whatever you like
    }

    // here I measure and print the temperature out without delay() using millis()
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        // request temperature from sensor
        sensors.requestTemperatures();
        // store the sensor value
        temperature = sensors.getTempC(thermometer);
        // and print it out
        Serial.print("Got: ");
        Serial.println(temperature);
        dtostrf(temperature, 3, 2, tempBuff); // create a string from float value
        mqttClient.publish("temperature", tempBuff); // client.publish(topic,message)
        // my function to edit temperature:
        editTemperature(setTemp,temperature,0.5);
        // the last parameter is precision of editing
    }
    mqttClient.loop();
}
// Interrupt on A changing state
void doEncoderA() {
    if ( rotating ) delay (1); // wait a little until the bouncing is done
    // Test transition, did things really change?
    if ( digitalRead(pinA) != A_set ) { // debounce once more
        A_set = !A_set;
        // adjust counter + if A leads B
        if ( A_set && !B_set ) encoderPos += 1;
        rotating = false; // no more debouncing until loop() hits again
    }
}
// Interrupt on B changing state, same as A above
void doEncoderB() {
    if ( rotating ) delay (1);
    if ( digitalRead(pinB) != B_set ) {
        B_set = !B_set;
        // adjust counter - 1 if B leads A
        if ( B_set && !A_set ) encoderPos -= 1;
        rotating = false;
    }
}
void editTemperature(float set, float actual, float limit) {
    if (actual > set+limit) {
        Serial.println("Need to cool...");
        digitalWrite(heatPin,LOW);
        digitalWrite(coolPin,HIGH);
    } else if (actual < set-limit) { 
        Serial.println("Need to heat...");
        digitalWrite(coolPin,LOW);
        digitalWrite(heatPin,HIGH);
    } else {
        Serial.println("Temperature is OK");
        digitalWrite(heatPin,LOW);
        digitalWrite(coolPin,LOW);
    }
}
void connectToInternet() {
    if (Ethernet.begin(mac) == 0) { // if there is error
        Serial.println("Failed to configure Ethernet using DHCP");
        // no point in carrying on, so do nothing forevermore:
        for (;;)
        ;
    }
    // otherwise - print your local IP address:
    Serial.print("My IP address: ");
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        // print the value of each byte of the IP address:
        Serial.print(Ethernet.localIP()[thisByte], DEC);
        Serial.print(".");
    }
    Serial.println();
}
void reconnect() {
    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect("arduinoClient")) {
          mqttClient.subscribe("setTempWeb");
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 3 seconds");
            // Wait 3 seconds before retrying
            delay(3000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
float setTempWeb = 0.0;
for (int i = 0; i < length; i++) {
        if (i==0) setTempWeb = setTempWeb + 10*((int)payload[i]-48);
        if (i==1) setTempWeb = setTempWeb + ((int)payload[i]-48);
        if (i==3) setTempWeb = setTempWeb + 0.1*((int)payload[i]-48);
        if (i==4) setTempWeb = setTempWeb + 0.01*((int)payload[i]-48);
    }
    Serial.print("Set temp from callback: ");
    Serial.println(setTempWeb);
    encoderPos = setTempWeb*2;
    lastReportedPos = encoderPos;
    setTemp = setTempWeb;
}
