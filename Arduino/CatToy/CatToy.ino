/*
  Controlling a servo position using a potentiometer (variable resistor)
  by Michal Rinott <http://people.interaction-ivrea.it/m.rinott>

  modified on 8 Nov 2013
  by Scott Fitzgerald
  http://www.arduino.cc/en/Tutorial/Knob
*/

#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "AppSettings.h"
#define NULIFY(x) free(x); x=NULL;

const int STATE_IDLE = 0;
const int STATE_CONNECTING = 1;
const int STATE_CONNECTED = 2;

const int BROADCAST_INTERVAL = 5000;
const int BROADCAST_PORT = 19999;

int _state = 0;
AppSettings settings;
String serialInput;

char *wifiSSID;
char *wifiPass;
long lastBroadcast=0;

void handleUdpMessages();
void broadcastPort();


// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,

//const int LASER_PIN = 7;
const int SERVO_MIN = 30;
const int SERVO_MAX = 150;

const int UDP_PORT = 8889;

Servo servo1;  // create servo object to control a servo
Servo servo2;
WiFiUDP Udp;

void setup() {
  Serial.begin(115200);
  
  servo1.attach(5);
  servo2.attach(16);    
  
  Serial.setDebugOutput(true);
  delay(500);
  while(Serial.available()) Serial.read();
  Serial.println("\nStarting sketch...");
  settings.init();
  serialInput.reserve(256);
  gotoState(STATE_IDLE);  
}

int s1Pos = 0;
int s2Pos = 0;


void enterStateIdle() {
  char *ssid = settings.get(AppSettings::SETTING_WIFI_SSID);
  char *pass = settings.get(AppSettings::SETTING_WIFI_PASS);

  if (ssid && pass) {
    Serial.println(F("wifi settings found."));
    wifiSSID = strdup(ssid);
    wifiPass = strdup(pass);
    gotoState(STATE_CONNECTING);
  } else {
    Serial.println(F("no wifi settings found. waiting for wifi serial command"));
  }
}

void loopStateIdle() {
}


void enterStateConnecting() {
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(wifiSSID, wifiPass);
  Serial.print(F("Trying to connect to: \""));
  Serial.print(wifiSSID);
  Serial.print(F("\" using pass: \""));
  Serial.print(wifiPass);
  Serial.println(F("\""));
}

void loopStateConnecting() {
  if (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(3000);
    Serial.print(".");
  } else {
    Serial.println("Connection established!");
    Serial.print("IP address:\t "); Serial.println(WiFi.localIP());
    settings.set(AppSettings::SETTING_WIFI_SSID, wifiSSID);
    settings.set(AppSettings::SETTING_WIFI_PASS, wifiPass);
    NULIFY(wifiSSID);
    NULIFY(wifiPass);
    settings.save();
    gotoState(STATE_CONNECTED);
  }
}


void enterStateConnected() {
    Udp.begin(UDP_PORT);
}

void loopStateConnected() {
  unsigned long currentMillis = millis();
  servo1.write(s1Pos);
  servo2.write(s2Pos);  
  handleUdpMessages();
  if (currentMillis - lastBroadcast > BROADCAST_INTERVAL) {
    lastBroadcast = currentMillis;
    broadcastPort();
  }
}

void gotoState(int state) {
  _state = state;
  switch (_state) {
    case STATE_IDLE:
      enterStateIdle();
      break;
    case STATE_CONNECTING:
      enterStateConnecting();
      break;
    case STATE_CONNECTED:
      enterStateConnected();
      break;
  }
}

void handleSerial() {
  static bool serialReceived = false;
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      serialReceived = true;
      break;
    } else {
      serialInput += inChar;
    }
  }
  if (serialReceived) {
    const int MAX_TOKENS = 10;
    char *tokens[MAX_TOKENS];
    int cnt = 0;
    char *command = strdup(serialInput.c_str());
    Serial.print(F("Executing "));
    Serial.println(command);
    char *ptr = strtok(command, "/");
    while (ptr && cnt < MAX_TOKENS) {
      tokens[cnt++] = ptr;
      ptr = strtok(NULL, "/");
    }    
    if (!handleSerialCommand(tokens, cnt)) {
      Serial.println("command not found.");      
    } else {
      
    }
    free(command);
    serialInput = "";
    serialReceived = false;
  }
}


void loop(void) {
  switch (_state) {
    case STATE_IDLE:
      loopStateIdle();
      break;
    case STATE_CONNECTING:
      loopStateConnecting();
      break;
    case STATE_CONNECTED:
      loopStateConnected();
      break;
  }
  handleSerial();
}



bool handleSerialCommand(char **tokens, int count) {
  const char *command = tokens[0];
  if (!strcmp(command, "wifi") && count == 3) {
    wifiSSID = strdup(tokens[1]);
    wifiPass = strdup(tokens[2]);
    gotoState(STATE_CONNECTING);
    return true;
  } else if (!strcmp(command, "disconnect")) {
    WiFi.disconnect();
    NULIFY(wifiSSID);
    NULIFY(wifiPass);    
    settings.set(AppSettings::SETTING_WIFI_SSID, NULL);
    settings.set(AppSettings::SETTING_WIFI_PASS, NULL);
    settings.save();
    gotoState(STATE_IDLE);
    return true;
  } else if (!strcmp(command, "s1") && count == 2) {
    int val = atoi(tokens[1]);
    s1Pos = constrain(val,SERVO_MIN, SERVO_MAX);
    Serial.printf("Servo 1 set to: %d\n", s1Pos);
    return true;
  } else if (!strcmp(command, "s2") && count == 2) {
    int val = atoi(tokens[1]);
    s2Pos = constrain(val,SERVO_MIN, SERVO_MAX);    
    Serial.printf("Servo 2 set to: %d\n", s2Pos);
    return true;
  }
  return false;
}

void handleUdpMessages() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
//    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
//                  packetSize,
//                  Udp.remoteIP().toString().c_str(), Udp.remotePort(),
//                  Udp.destinationIP().toString().c_str(), Udp.localPort(),
//                  ESP.getFreeHeap());

    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    char *s1Val = strtok(packetBuffer, "/");
    if (s1Val) {
      char *s2Val = strtok(NULL, "/");
      if (s2Val) {
        s1Pos = constrain(atoi(s1Val), SERVO_MIN, SERVO_MAX);
        s2Pos = constrain(atoi(s2Val), SERVO_MIN, SERVO_MAX);
        Serial.printf("udp pos: %d - %d\n", s1Pos, s2Pos);
      }
    }
  }
}

void broadcastPort() {
  IPAddress broadcastIp = WiFi.localIP();
  broadcastIp[3] = 255;
  if(Udp.beginPacket(broadcastIp, BROADCAST_PORT)) {
    Serial.printf("Broadcasting on: %s : %d\n", broadcastIp.toString().c_str(), BROADCAST_PORT);
    Udp.write("CaToy/");
    char buffer[10];
    itoa(UDP_PORT, buffer, 10);
    Udp.write(buffer);
    Udp.endPacket();
  } else {
    Serial.println("Could not broadcast");
  }
}
