/*KY-40 INIT*/
#define MAX_POS 60
#define MIN_POS 30
enum PinAssignments {
  pinA = 2,   // DT pin
  pinB = 3,   // CLK pin
  pinSW = 4    // SW pin
};
volatile  int encoderPos = MIN_POS;  // a counter for the dial
int lastReportedPos = MIN_POS-1;   // change management
static boolean rotating = false;      // debounce management
// interrupt service routine vars
boolean A_set = false;              
boolean B_set = false;

/*TEMP setup*/
float setTemp = MIN_POS*0.5;
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 5
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress thermometer = { 0x28, 0xFF, 0xE3, 0xD4, 0x67, 0x14, 0x03, 0x39 };

/*DELAYS*/
unsigned long previousMillis = 0;  
const long interval = 3000;

/*Ethernet*/
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
EthernetClient ethClient;

/* MQTT */
PubSubClient mqttClient(ethClient);

char tempBuff[10];
char setTempBuff[10];
float temperature = 0.0;
