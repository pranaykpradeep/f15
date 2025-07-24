#include <Arduino.h>

#define BLYNK_TEMPLATE_ID "replace with your"
#define BLYNK_TEMPLATE_NAME "replace with your"
#define BLYNK_AUTH_TOKEN "replace with your"
#define BLYNK_PRINT Serial
#define WIFI_SSID "replace with your"
#define WIFI_PASS "replace with your"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <LoRa.h>

#define rmt 25 //remot on/off
#define RMTLED  33
#define wifipin 15
//Define physical button pins
#define BTNKEY 4 //key
#define BTNKILL 35  // kill

//Define LoRa pins
#define SS 5
#define RST 14
#define DIO0 2 // Changed from 26 to 2 (ESP8266 typically uses GPIO2)

//initializing button states
bool keystate = false;
bool killstate = false;
bool startstate = false;
bool stopstate = false;
bool sw1state = false;
bool sw2state = false;
bool sw3state = false;

String textinput = "";
float receivedrpm = 0;
int estatus = 0;

//previous status
bool s1 = false;
bool s2 = false;
bool s3 = false;
bool s4 = false;
bool s5 = false;
bool s6 = false;
bool s7 = false;
bool pre1 = HIGH;
bool pre2 = HIGH;
int first = 0;
bool rmtPreviousState = false;

// Function declarations
void remoteon();
void remoteoff();
void sendCommand(String command);

BLYNK_WRITE(V0) { keystate = param.asInt(); }
BLYNK_WRITE(V1) { startstate = param.asInt(); }
BLYNK_WRITE(V2) { killstate = param.asInt(); }
BLYNK_WRITE(V3) { stopstate = param.asInt(); }
BLYNK_WRITE(V4) { sw1state = param.asInt(); }
BLYNK_WRITE(V5) { sw2state = param.asInt(); }
BLYNK_WRITE(V6) { sw3state = param.asInt(); }
BLYNK_WRITE(V7) { /* onled - no action needed as we write to it */ }
BLYNK_WRITE(V8) { /* nled - no action needed as we write to it */ }
BLYNK_WRITE(V9) { /* rpm meter - read only */ }
BLYNK_WRITE(V10) { textinput = param.asString(); }

void setup() {
  pinMode(rmt, INPUT_PULLUP);
  pinMode(BTNKEY, INPUT_PULLUP);
  pinMode(BTNKILL, INPUT_PULLUP);
  pinMode(wifipin, OUTPUT);
  pinMode(RMTLED, OUTPUT);
  Serial.begin(115200);
  digitalWrite(RMTLED,LOW);

 // WiFi connection with timeout
  Serial.println("\nConnecting to Wi-Fi!");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    digitalWrite(wifipin,HIGH);
    delay(500);
    digitalWrite(wifipin,LOW);
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.config(BLYNK_AUTH_TOKEN);
    Serial.println("Attempting to connect to Blynk...");
    
    // Try to connect to Blynk with timeout
    unsigned long blynkStartTime = millis();
    bool blynkConnected = false;
    
    while (millis() - blynkStartTime < 5000) { // 5 second timeout
      if (Blynk.connect(1000)) { // Try for 1 second at a time
        blynkConnected = true;
        break;
      }
      delay(100);
    }
    
    if (blynkConnected) {
      Serial.println("Connected to Blynk successfully!");
    } else {
      Serial.println("Failed to connect to Blynk, continuing anyway...");
    }
    
    digitalWrite(wifipin,HIGH);
  } else {
    Serial.println("\nWi-Fi connection failed! Continuing in offline mode.");
}
  // Initialize LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(865E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (1);
  }
  Serial.println("LoRa Initialized");
}

void loop() {
  Blynk.run();
  
  // Handle remote status
  if (digitalRead(rmt) == LOW) {
    Blynk.virtualWrite(V7, 255); // LED on
    digitalWrite(RMTLED,HIGH);
    if (!rmtPreviousState|| first == 0) {
      Serial.println("Remote ON");
      rmtPreviousState = true;
    }
    remoteon();
  } else {
    Blynk.virtualWrite(V7, 0); // LED off
    digitalWrite(RMTLED,LOW);
    if (rmtPreviousState|| first == 0) {
      Serial.println("Remote OFF");
      rmtPreviousState = false;
    } 
    remoteoff();
  }


  // Listen for incoming LoRa response
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    
    Serial.println("Received via LoRa: " + receivedData);
    
    // Parse received data
    int firstComma = receivedData.indexOf(',');
    if (firstComma != -1) {
      receivedrpm = receivedData.substring(0, firstComma).toFloat();

      estatus = receivedData.substring(firstComma + 1).toInt();
    }

    // Update Blynk widgets
    if (receivedrpm > 5) {
      Blynk.setProperty(V9, "color", "#D3435C");
    }
    Blynk.virtualWrite(V9, receivedrpm);
    Blynk.virtualWrite(V8, estatus == 1 ? 255 : 0);
  }

   // If there's a text message, send it too
  if (textinput.length() > 0) {
    sendCommand(textinput);
    textinput = ""; // Clear after sending
}
}

void remoteoff() {
  if (first == 0 || keystate != s1) {
    sendCommand(keystate ? "keyon" : "keyoff");
    s1 = keystate;
    if(first==0){delay(1000);}
  }

  if (first == 0 || killstate != s2) {
    sendCommand(killstate ? "killon" : "killoff");
    s2 = killstate;
    if(first==0){delay(1000);}
  }

  if (first == 0 || stopstate != s3) {
    sendCommand(stopstate ? "stopon" : "stopoff");
    s3 = stopstate;
    if(first==0){delay(1000);}
  }

  if (first == 0 || startstate != s4) {
    sendCommand(startstate ? "starton" : "startoff");
    s4 = startstate;
    if(first==0){delay(1000);}
  }

  if (first == 0 || sw1state != s5) {
    sendCommand(sw1state ? "sw1on" : "sw1off");
    s5 = sw1state;
    if(first==0){delay(1000);}
  }

  if (first == 0 || sw2state != s6) {
    sendCommand(sw2state ? "sw2on" : "sw2off");
    s6 = sw2state;
    if(first==0){delay(1000);}
  }

  if (first == 0 || sw3state != s7) {
    sendCommand(sw3state ? "sw3on" : "sw3off");
    s7 = sw3state;
    if(first==0){delay(1000);}
  }

  if (first == 0) first = 1;
}

void remoteon() {
  bool currentKey = (keystate || (digitalRead(BTNKEY) == LOW));
  if (first == 0 || currentKey != s1) {
    sendCommand(currentKey ? "keyon" : "keyoff");
    s1 = currentKey;
    if(first==0){delay(1000);}
  }

  bool currentKill = (killstate || !(digitalRead(BTNKILL)));
  if (first == 0 || currentKill != s2) {
    sendCommand(currentKill ? "killon" : "killoff");
    s2 = currentKill;
    if(first==0){delay(1000);}
  }

  // Other states remain from Blynk
  if (first == 0 || stopstate != s3) {
    sendCommand(stopstate ? "stopon" : "stopoff");
    s3 = stopstate;
    if(first==0){delay(1000);}
  }

  if (first == 0 || startstate != s4) {
    sendCommand(startstate ? "starton" : "startoff");
    s4 = startstate;
    if(first==0){delay(1000);}
  }

  if (first == 0 || sw1state != s5) {
    sendCommand(sw1state ? "sw1on" : "sw1off");
    s5 = sw1state;
    if(first==0){delay(1000);}
  }

  if (first == 0 || sw2state != s6) {
    sendCommand(sw2state ? "sw2on" : "sw2off");
    s6 = sw2state;
    if(first==0){delay(1000);}
  }

  if (first == 0 || sw3state != s7) {
    sendCommand(sw3state ? "sw3on" : "sw3off");
    s7 = sw3state;
    if(first==0){delay(1000);}
  }

  if (first == 0) first = 1;
}

void sendCommand(String command) {
  LoRa.beginPacket();
  LoRa.print(command);
  LoRa.endPacket(); 
  Serial.println("Sent: " + command);
  }
