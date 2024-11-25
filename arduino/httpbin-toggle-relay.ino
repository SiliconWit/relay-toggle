#include <SoftwareSerial.h>
SoftwareSerial SIM800serial (2, 3);

#define OK 1
#define NOTOK 2
#define TIMEOUT 3
#define RELAY_PIN 7
#define STATUS_LED 13

String txt;
bool relayState = false;  // Start with relay OFF
unsigned long lastToggleTime = 0;
const unsigned long TOGGLE_INTERVAL = 10000; // 10 seconds

void setup() {
  pinMode(5, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  
  digitalWrite(5, HIGH);      // SIM800 ON
  digitalWrite(RELAY_PIN, LOW); // Initial relay state OFF
  digitalWrite(STATUS_LED, LOW); // LED off initially
  
  Serial.begin(9600);
  SIM800serial.begin(9600);
  
  Serial.println("Starting up...");
  delay(5000);
  
  if (SIM800command("AT", "OK", "ERROR", 500, 5) == OK) {
    Serial.println("SIM800L responding OK");
  } else {
    Serial.println("SIM800L not responding!");
  }
}

void loop() {
  if (millis() - lastToggleTime >= TOGGLE_INTERVAL) {
    // Toggle relay state
    relayState = !relayState;  // Flip the state
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    digitalWrite(STATUS_LED, relayState ? HIGH : LOW);
    
    Serial.print("\nToggling relay to: ");
    Serial.println(relayState ? "ON" : "OFF");
    
    sendStateToServer();
    lastToggleTime = millis();
  }
}

void sendStateToServer() {
  Serial.println("\n--- Starting Relay Toggle Sequence ---");
  
  if (!initGPRS()) {
    Serial.println("GPRS initialization failed!");
    return;
  }

  // Use current relay state for the request
  String toggleURL = "http://httpbin.org/get?relay=" + String(relayState ? "on" : "off");
  Serial.println("Sending request to: " + toggleURL);
  
  SIM800command("AT+HTTPPARA=\"URL\",\"" + toggleURL + "\"", "OK", "ERROR", 20000, 1);
  
  if (SIM800command("AT+HTTPACTION=0", "0,200,", "ERROR", 20000, 1) == OK) {
    SIM800command("AT+HTTPREAD", "HTTPREAD", "ERROR", 20000, 1);
    
    if (txt.indexOf("200") > -1) {
      Serial.println("\n=========================");
      Serial.println("Relay is now: " + String(relayState ? "ON" : "OFF"));
      Serial.println("=========================\n");
    }
  }

  // Cleanup
  SIM800command("AT+HTTPTERM", "HTTPTERM", "ERROR", 20000, 1);
  SIM800command("AT+SAPBR=0,1", "+SAPBR:", "ERROR", 2000, 1);
}

bool initGPRS() {
  Serial.println("Initializing GPRS connection...");
  
  if (SIM800command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", "ERROR", 2000, 1) != OK) return false;
  if (SIM800command("AT+SAPBR=3,1,\"APN\",\"safaricom\"", "OK", "ERROR", 2000, 1) != OK) return false;
  if (SIM800command("AT+SAPBR=1,1", "OK", "ERROR", 5000, 1) != OK) return false;
  if (SIM800command("AT+SAPBR=2,1", "OK", "ERROR", 5000, 1) != OK) return false;
  if (SIM800command("AT+HTTPINIT", "OK", "ERROR", 30000, 1) != OK) return false;
  if (SIM800command("AT+HTTPPARA=\"CID\",1", "OK", "ERROR", 10000, 1) != OK) return false;
  
  Serial.println("GPRS initialized successfully");
  return true;
}

// Your existing helper functions remain the same
byte SIM800command(String command, String response1, String response2, uint16_t timeOut, uint16_t repetitions) {
  byte returnValue = NOTOK;
  byte countt = 0;
  
  while (countt < repetitions && returnValue != OK) {
    Serial.println("> " + command); // Print command being sent
    SIM800serial.println(command);
    
    if (SIM800waitFor(response1, response2, timeOut) == OK) {
      returnValue = OK;
    } else {returnValue = NOTOK;}
    countt++;
  }
  
  return returnValue;
}

byte SIM800waitFor(String response1, String response2, uint16_t timeOut) {
  uint16_t entry = 0;
  uint16_t count = 0;
  String reply = SIM800read();
  byte retVal = 99;

  do {
    reply = SIM800read();
    delay(1);
    entry ++;
  } while ((reply.indexOf(response1) + reply.indexOf(response2) == -2) && entry < timeOut );

  if (entry >= timeOut) {
    retVal = TIMEOUT;
  } else {
    if (reply.indexOf(response1) + reply.indexOf(response2) > -2) retVal = OK;
    else retVal = NOTOK;
  }
  
  return retVal;
}

String SIM800read() {
  String reply = "";
  
  if (SIM800serial.available()) {
    reply = SIM800serial.readString();
  }

  if (reply != "") {
    Serial.print("Reply: ");
    Serial.println(reply);
    txt = reply;
  }
  
  return reply;
}
