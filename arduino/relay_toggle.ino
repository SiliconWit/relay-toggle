// Include required libraries
#include <SoftwareSerial.h>

// Pin Definitions
const int SIM800_RX = 2;    // SIM800L RX connected to Arduino D2
const int SIM800_TX = 3;    // SIM800L TX connected to Arduino D3
const int RELAY_PIN = 7;    // Relay control pin connected to Arduino D7
const int STATUS_LED = 13;  // Built-in LED for status indication

// Network Configuration
const char* APN = "safaricom";  // Your mobile carrier's APN
const char* SERVER_URL = "https://siliconwit.github.io/relay-toggle";

// Timing Configuration
unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 10000;  // Check every 10 seconds
const unsigned long TIMEOUT = 5000;          // 5 second timeout for responses

// State Variables
bool relayState = false;
bool networkConnected = false;

// Initialize SoftwareSerial for SIM800L
SoftwareSerial sim800l(SIM800_RX, SIM800_TX);

void setup() {
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);      // Start with relay off
  digitalWrite(STATUS_LED, LOW);     // Start with LED off
  
  // Start serial communications
  Serial.begin(9600);               // Debug serial
  sim800l.begin(9600);             // SIM800L serial
  
  Serial.println(F("Initializing SIM800L..."));
  initGSM();                       // Initialize GSM module
}

void loop() {
  // Check relay state periodically
  if (millis() - lastCheckTime >= CHECK_INTERVAL) {
    if (networkConnected) {
      checkRelayState();
    } else {
      Serial.println(F("Network disconnected, attempting to reconnect..."));
      initGSM();
    }
    lastCheckTime = millis();
  }
  
  // Process any debug commands from Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    processSerialCommand(command);
  }
}

void initGSM() {
  blinkLED(3);  // Indicate initialization start
  
  // Reset HTTP and GPRS
  sendATCommand("AT+HTTPTERM");
  delay(1000);
  sendATCommand("AT+SAPBR=0,1");
  delay(2000);
  
  // Test AT communication
  if (!sendATCommand("AT", "OK", 3000)) {
    Serial.println(F("SIM800L not responding"));
    return;
  }
  
  // Wait for network registration
  Serial.println(F("Waiting for network..."));
  while (!isNetworkRegistered()) {
    blinkLED(1);
    delay(2000);
  }
  
  // Configure GPRS connection
  sendATCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  sendATCommand("AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"");
  
  // Activate GPRS
  if (sendATCommand("AT+SAPBR=1,1", "OK", 10000)) {
    Serial.println(F("GPRS activated"));
    
    // Initialize HTTP service
    if (sendATCommand("AT+HTTPINIT", "OK", 3000)) {
      sendATCommand("AT+HTTPPARA=\"CID\",1");
      networkConnected = true;
      digitalWrite(STATUS_LED, HIGH);  // Indicate successful connection
      Serial.println(F("HTTP initialized"));
    }
  }
}

void checkRelayState() {
  Serial.println(F("Checking relay state..."));
  
  // Set URL for checking state
  if (!sendATCommand("AT+HTTPPARA=\"URL\",\"" + String(SERVER_URL) + "\"", "OK", 3000)) {
    networkConnected = false;
    return;
  }
  
  // Send GET request
  if (sendATCommand("AT+HTTPACTION=0", "OK", 3000)) {
    // Wait for action result
    String response = waitForResponse("+HTTPACTION:", 10000);
    
    if (response.indexOf("+HTTPACTION: 0,200") > -1) {
      // Read webpage content
      sendATCommand("AT+HTTPREAD");
      String pageContent = waitForResponse("OK", 5000);
      
      // Check for relay state in webpage
      if (pageContent.indexOf("\"relay-status\">ON") > -1 && !relayState) {
        setRelayState(true);
      } 
      else if (pageContent.indexOf("\"relay-status\">OFF") > -1 && relayState) {
        setRelayState(false);
      }
    } else {
      Serial.println(F("HTTP request failed"));
    }
  }
}

void setRelayState(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  Serial.println(state ? F("Relay ON") : F("Relay OFF"));
  blinkLED(state ? 2 : 1);
}

bool isNetworkRegistered() {
  if (sendATCommand("AT+CREG?", "+CREG: 0,1", 3000) || 
      sendATCommand("AT+CREG?", "+CREG: 0,5", 3000)) {
    return true;
  }
  return false;
}

bool sendATCommand(String command, String expectedResponse = "OK", unsigned long timeout = 3000) {
  Serial.println("> " + command);
  sim800l.println(command);
  
  if (expectedResponse != "") {
    return waitForResponse(expectedResponse, timeout) != "";
  }
  return true;
}

String waitForResponse(String expectedResponse, unsigned long timeout) {
  String response = "";
  unsigned long start = millis();
  
  while (millis() - start < timeout) {
    while (sim800l.available()) {
      char c = sim800l.read();
      response += c;
      Serial.write(c);  // Echo to debug serial
    }
    
    if (response.indexOf(expectedResponse) > -1) {
      return response;
    }
  }
  return "";  // Timeout occurred
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
    digitalWrite(STATUS_LED, LOW);
    delay(100);
  }
}

void processSerialCommand(String command) {
  if (command == "reset") {
    Serial.println(F("Resetting GSM..."));
    initGSM();
  } 
  else if (command == "status") {
    Serial.println("Network: " + String(networkConnected ? "Connected" : "Disconnected"));
    Serial.println("Relay: " + String(relayState ? "ON" : "OFF"));
  }
}
