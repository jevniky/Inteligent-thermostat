/*KY-40 INIT*/
#define MAX_POS 60 // max temp 30
#define MIN_POS 30 // min temp 15

enum PinAssignments {
    pinA = 2, // DT pin
    pinB = 3, // CLK pin
    pinSW = 4 // SW pin
};
volatile int encoderPos = MIN_POS; // a counter for the dial
int lastReportedPos = MIN_POS-1; // change management
static boolean rotating = false; // debounce management
// interrupt service routine variables
boolean A_set = false; 
boolean B_set = false;

/*TEMP setup*/
float setTemp = MIN_POS*0.5; // Starting point for temperature setting.
float temperature = 0.0; // measured temperature will be stored here

// Data wire is plugged into port 5 on the Arduino
#define ONE_WIRE_BUS 5

// Setup a oneWire instance to communicate with my ds18b20 sensor
OneWire oneWire(ONE_WIRE_BUS);

// Pass the oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// array to hold d18b20 address
DeviceAddress thermometer = { 0x28, 0xFF, 0xE3, 0xD4, 0x67, 0x14, 0x03, 0x39 };
// make sure you will find out the address of your sensor

// pins to show if heating or cooling is needed (relays)
int heatPin = 8;
int coolPin = 9;

/*DELAYS*/
unsigned long previousMillis = 0; 
const long interval = 3000;

/*Ethernet*/
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 }; // I've made this address up
EthernetClient ethClient; // start an instance

/* MQTT */
PubSubClient mqttClient(ethClient);

char tempBuff[10];
char setTempBuff[10];

/* LCD */
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(16, 17, 18, 19, 20, 21);

