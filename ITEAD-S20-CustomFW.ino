#
# This firmware originate from
# https://github.com/kungknut/ITEAD-S20-CustomFW
#
# If you find a bug or want to contribute to make
# this firmware better? Feel free to open an issue
# or a pull request.
#

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

//#define DEBUG // Comment/Uncomment this line to enable/disable debugging output.

#define FW_VERSION "FW: development"

#define PIN_RELAY 12
#define PIN_STATUS 13
#define PIN_BUTTON 0

#define MQTT_MAX_ATTEMPT 3
#define MQTT_CONN_TIMEOUT 60000   //Milliseconds
#define MQTT_CONN_REATT 10000     //Milliseconds


const char* wifiSsid = "";                  // Network SSID
const char* wifiPass = "";                  // Network pre shared key (WPA2)

const char mqttServer[] = "";               // MQTT Servers IP-address
const int  mqttServerPort = 1883;           // MQTT Servers listening port (Default is 1883)
const char mqttUsername[] = "";             // Leave blank if authorization is not required
const char mqttPassword[] = "";             // Leave blank if authorization is not required


// Program variables
bool reportRequired = true;

long lastDebounceTime = 0;
long debounceDelay = 50;  // Milliseconds

int buttonState;
int lastButtonState;
int relayState;
int previousRelayState;

char cmdTopic[15];
char opstaTopic[17];
char mqttClientId[17];


WiFiClient wifiClient;
PubSubClient client(wifiClient);


String getConf(String type){
  String retVal;
  String chipId = String(ESP.getChipId()); // 32bit unsigned int (max value 4294967296, 10 digits)

  if(type == "chipID"){
    retVal = chipId;
  }
  if(type == "clientID"){
    retVal = "S20ESP" + chipId;
  }
  if(type == "TC"){ // Topic Command
    retVal = chipId + "/cmd";
  }
  if(type == "TS"){ // Topic State
    retVal = chipId + "/opsta";
  }
  
  return retVal;
}

int connectAttemptsMqtt = 0;
long lastConnectAttemptMqtt = 0;

bool connectMqtt() {

  bool retVal = false;
  
  client.setServer(mqttServer, mqttServerPort);
  client.setCallback(mqttCallback);

  getConf("clientID").toCharArray(mqttClientId, 15);
  getConf("TC").toCharArray(cmdTopic, 15);
  
  if(client.connected()){

    #ifdef DEBUG
      Serial.println("MQTT Already connected.");
    #endif
    
    retVal = true;
    return retVal;
    
  }

  connectAttemptsMqtt = millis() - lastConnectAttemptMqtt > MQTT_CONN_TIMEOUT ? 0 : connectAttemptsMqtt;

  #ifdef DEBUG
    if(connectAttemptsMqtt == MQTT_MAX_ATTEMPT){
      Serial.println("Max reconnects reached, trying again later.");
      Serial.println(millis());
    }
  #endif

  if(millis() - lastConnectAttemptMqtt > MQTT_CONN_REATT && connectAttemptsMqtt < MQTT_MAX_ATTEMPT){

    #ifdef DEBUG
      Serial.print("MQTT Client ID: ");
      Serial.println(mqttClientId);
      Serial.print("Connecting to MQTT broker: ");
      Serial.print(mqttServer);
      Serial.print(":");
      Serial.println(mqttServerPort);
    #endif

    if (client.connect(mqttClientId, mqttUsername, mqttPassword)) {
      #ifdef DEBUG
        Serial.println("Connected.");
      #endif
      if (client.subscribe(cmdTopic)) {
        #ifdef DEBUG
          Serial.print("Listening to ");
          Serial.println(cmdTopic);
        #endif
      } else {
        #ifdef DEBUG
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
        #endif
        
        retVal = false;
        return retVal;
        
      }
    } else {
      #ifdef DEBUG
        Serial.println("Connection failed, trying again.");
        Serial.println(millis());
      #endif
    }
    
    connectAttemptsMqtt++;
    lastConnectAttemptMqtt = millis();

  }
  
  retVal = true;
  return retVal;
  
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';

  String s_topic = String(topic);
  String s_payload = String(c_payload);

  if (s_topic == getConf("TC")) {
    if (s_payload == "ON") {
      relayState = HIGH;
    } else if (s_payload == "OFF") {
      relayState = LOW;
    }
  }
}

void setup() {
  delay(2000);
  
  #ifdef DEBUG
    Serial.begin(9600);
    delay(2000);
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifiSsid);
    Serial.print("Using password: ");
    Serial.println(wifiPass);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPass);
  delay(10000);

  #ifdef DEBUG
    Serial.println(".");
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("WiFi connection failed.");
    } else {
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  #endif

  if(WiFi.status() == WL_CONNECTED){
    
    if(connectMqtt()){
      getConf("chipID").toCharArray(mqttClientId, 11);
      client.publish(mqttClientId, FW_VERSION);
      #ifdef DEBUG
        Serial.println("MQTT Connected.");
        Serial.print("MQTT-Client ID: ");
        Serial.println(getConf("clientID"));
        Serial.print("Command topic: ");
        Serial.println(getConf("TC"));
        Serial.print("State topic: ");
        Serial.println(getConf("TS"));
      #endif
    } else {
      #ifdef DEBUG
        Serial.println("MQTT Connection failed.");
      #endif
    }
  
  } else {
    
    #ifdef DEBUG
      Serial.println("WiFi not connected, skipping MQTT.");
    #endif
    
  }
  
  EEPROM.begin(4);
  relayState = EEPROM.read(0) == 0 ? LOW : HIGH;
  previousRelayState = relayState;

  #ifdef DEBUG
    Serial.print("Setting relay output: ");
    Serial.println(relayState);
  #endif
  
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_STATUS, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_STATUS, HIGH);
  digitalWrite(PIN_RELAY, relayState);

  #ifdef DEBUG
    Serial.println("Outputs set.");
  #endif
}


void loop() {
  long now = millis();

  client.loop();

  if(!client.connected()){
    digitalWrite(PIN_STATUS, LOW); // Light green LED if WiFi or MQTT not connected.
    connectMqtt();
  } else {
    digitalWrite(PIN_STATUS, HIGH);
  }
  
  int buttonReading = digitalRead(PIN_BUTTON);
  if (buttonReading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((now - lastDebounceTime) > debounceDelay) {
    if (buttonReading != buttonState) {
      buttonState = buttonReading;
      if (buttonState == LOW) { // When button is pressed, input is low
        relayState = !relayState;
        
        #ifdef DEBUG
          Serial.println("Button pressed.");
        #endif
      }
    }
  }

  lastButtonState = buttonReading;

  if (lastDebounceTime > millis()) {
    lastDebounceTime = 0;
  }

  if (relayState != previousRelayState) {
    #ifdef DEBUG
      Serial.println("Toggling relay.");
    #endif
    digitalWrite(PIN_RELAY, relayState);
    previousRelayState = relayState;
    reportRequired = true;
  }

  if (reportRequired) {
    #ifdef DEBUG
      Serial.println("Reporting relay state.");
    #endif
    
    getConf("TS").toCharArray(opstaTopic, 17);
    
    if (relayState == HIGH) {
      client.publish(opstaTopic, "ON");
    } else {
      client.publish(opstaTopic, "OFF");
    }
    EEPROM.write(0, relayState);
    EEPROM.commit();
    reportRequired = false;
  }

}
