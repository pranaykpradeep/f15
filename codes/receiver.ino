#include <SPI.h>
#include <LoRa.h>

// LoRa pins
#define SS 10
#define RST 9
#define DIO0 2

// Relay control pins
#define KEY_RELAY 3
#define KILL_RELAY 4
#define START_RELAY 5
#define STOP_RELAY 6
#define SW1 7
#define SW2 8
#define SW3 11
// RPM measurement pins
#define RPM_PULSE_PIN A0    // Connect to pulse signal
#define STATUS_PIN  1      // Connect to digital status signal

// Variables
unsigned long pulseTime = 0;
unsigned long lastPulseTime = 0;
float rpm = 0;
int statusValue = 0;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // Send data every 1 second

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize relay pins
  pinMode(KEY_RELAY, OUTPUT);
  pinMode(KILL_RELAY, OUTPUT);
  pinMode(START_RELAY, OUTPUT);
  pinMode(STOP_RELAY, OUTPUT);
  pinMode(SW1, OUTPUT);
  pinMode(SW2, OUTPUT);
  pinMode(SW3, OUTPUT);
  // Initialize RPM measurement pins
  pinMode(RPM_PULSE_PIN, INPUT);
  pinMode(STATUS_PIN, INPUT);
  
  // Initialize LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(865E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa initialized");
}

void loop() {
  // Check for incoming LoRa messages
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    
    // Check if it's a command
    if (isCommand(received)) {
      processCommand(received);
    } else {
      // Not a command, just print it
      Serial.print("Received message: ");
      Serial.println(received);
    }
  }

  // RPM measurement
  if (digitalRead(RPM_PULSE_PIN) == HIGH && pulseTime == 0) {
    pulseTime = micros();
  } else if (digitalRead(RPM_PULSE_PIN) == LOW && pulseTime != 0) {
    unsigned long pulseDuration = micros() - pulseTime;
    pulseTime = 0;
    
    // Calculate RPM (assuming one pulse per revolution)
    if (pulseDuration > 0) {
      rpm = 60000000.0 / pulseDuration; // 60,000,000 microseconds per minute
      rpm=rpm/1000;
      lastPulseTime = millis();
    }
  }

  // Check for timeout (no pulse for 2 seconds)
  if (millis() - lastPulseTime > 2000) {
    rpm = 0;
  }

  // Read status pin
  statusValue = digitalRead(STATUS_PIN);

  // Send RPM and status periodically
  if (millis() - lastSendTime > sendInterval) {
    String dataToSend = String(rpm) + "," + String(statusValue);
    LoRa.beginPacket();
    LoRa.print(dataToSend);
    LoRa.endPacket();
    Serial.print("Sent: ");
    Serial.println(dataToSend);
    lastSendTime = millis();
  }
}

bool isCommand(String message) {
  // List of valid commands
  String commands[] = {
    "keyon", "keyoff",
    "killon", "killoff",
    "starton", "startoff",
    "stopon", "stopoff",
    "sw1on", "sw1off",
    "sw2on", "sw2off",
    "sw3on", "sw3off",
  };

  for (String cmd : commands) {
    if (message == cmd) return true;
  }
  return false;
}

void processCommand(String command) {
  Serial.print("Executing command: ");
  Serial.println(command);

  if (command == "keyon") {
    digitalWrite(KEY_RELAY, LOW);
  } else if (command == "keyoff") {
    digitalWrite(KEY_RELAY, HIGH);
  } else if (command == "killon") {
    digitalWrite(KILL_RELAY, LOW);
  } else if (command == "killoff") {
    digitalWrite(KILL_RELAY, HIGH);
  } else if (command == "starton") {
    digitalWrite(START_RELAY, LOW);
  } else if (command == "startoff") {
    digitalWrite(START_RELAY, HIGH);
  } else if (command == "stopon") {
    digitalWrite(STOP_RELAY, LOW);
  } else if (command == "stopoff") {
    digitalWrite(STOP_RELAY, HIGH);
  } else if (command == "sw1on") {
    digitalWrite(SW1, LOW);
  } else if (command == "sw1off") {
    digitalWrite(SW1, HIGH);
  } else if (command == "sw2on") {
    digitalWrite(SW2, LOW);
  } else if (command == "sw2off") {
    digitalWrite(SW2, HIGH);
  } else if (command == "sw3off") {
    digitalWrite(SW3, HIGH);
  } else if (command == "sw3on") {
    digitalWrite(SW3, LOW);
}
}
