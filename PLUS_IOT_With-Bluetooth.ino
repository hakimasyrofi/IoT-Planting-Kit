#include <WiFi.h>
#include <FirebaseESP32.h>

//////////////
#include <Preferences.h>
#include "BluetoothSerial.h"
///////////////////////

//konfigurasi Firebase
#define FIREBASE_HOST "plus-iot-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "yLXlNeHjOsFfsngwB2eMuH8limw7aGrbNjRaW6Su"

//konfigurasi pin sensor
#define sensorPin 32
FirebaseJson json;
FirebaseJson json2;
FirebaseData firebaseData;

//tambahan baru
FirebaseJsonData jsonData;

#define WATER_PUMP 27//water
boolean state = false;//water
FirebaseJson json3;
FirebaseData firebaseData2;

//////////////////////
String ssids_array[50];
String network_string;
String connected_string;

const char* pref_ssid = "";
const char* pref_pass = "";
String client_wifi_ssid;
String client_wifi_password;

const char* bluetooth_name = "Planting Kit";

long start_wifi_millis;
long wifi_timeout = 10000;
bool bluetooth_disconnect = false;

enum wifi_setup_stages { NONE, SCAN_START, SCAN_COMPLETE, SSID_ENTERED, WAIT_PASS, PASS_ENTERED, WAIT_CONNECT, LOGIN_FAILED };
enum wifi_setup_stages wifi_stage = NONE;

BluetoothSerial SerialBT;
Preferences preferences;
////////////////////////////////////

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
  pinMode(WATER_PUMP, OUTPUT); //water

  //////////
  Serial.println("Booting...");
  preferences.begin("wifi_access", false);
  if (!init_wifi()) { // Connect to Wi-Fi fails
    SerialBT.register_callback(callback);
  } else {
    SerialBT.register_callback(callback_show_ip);
  }
  SerialBT.begin(bluetooth_name);

  //Koneksi Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //Stream
  Firebase.setStreamCallback(firebaseData2, streamCallback, streamTimeoutCallback);
  if (!Firebase.beginStream(firebaseData2, "/Sensor")){
    Serial.println(firebaseData2.errorReason());  
  }
}

void loop() {  
  int level = readSensor();
  json.set("/value", level);
  Firebase.updateNode(firebaseData,"/Sensor",json);

  json2.get(jsonData, "Sensor/value");
  Serial.print("Read value");
  Serial.print(jsonData.type);
  Serial.println(jsonData.intValue);
  
  Serial.print("Level Air: ");
  Serial.print(level);
  Serial.println("%");

  delay(1000);

  if (bluetooth_disconnect){
    disconnect_bluetooth();
  }
    //////////
  switch (wifi_stage){
      case SCAN_START:
        SerialBT.println("Scanning Wi-Fi networks");
        Serial.println("Scanning Wi-Fi networks");
        scan_wifi_networks();
        SerialBT.println("Please enter the number for your Wi-Fi");
        wifi_stage = SCAN_COMPLETE;
        break;
  
      case SSID_ENTERED:
        SerialBT.println("Please enter your Wi-Fi password");
        Serial.println("Please enter your Wi-Fi password");
        wifi_stage = WAIT_PASS;
        break;
  
      case PASS_ENTERED:
        SerialBT.println("Please wait for Wi-Fi connection...");
        Serial.println("Please wait for Wi_Fi connection...");
        wifi_stage = WAIT_CONNECT;
        preferences.putString("pref_ssid", client_wifi_ssid);
        preferences.putString("pref_pass", client_wifi_password);
        if (init_wifi()) { // Connected to WiFi
          connected_string = "ESP32 IP: ";
          connected_string = connected_string + WiFi.localIP().toString();
          SerialBT.println(connected_string);
          Serial.println(connected_string);
          bluetooth_disconnect = true;
        } else { // try again
          wifi_stage = LOGIN_FAILED;
        }
        break;
  
      case LOGIN_FAILED:
        SerialBT.println("Wi-Fi connection failed");
        Serial.println("Wi-Fi connection failed");
        delay(2000);
        wifi_stage = SCAN_START;
        break;
    }  
 //////////////////////////////////
}

int readSensor() {
  int val = analogRead(sensorPin);    // Read the analog value form sensor
  return (val/30);             // send current reading
}

void streamCallback(StreamData data){
  Serial.println("Stream Data...");
  Serial.println(data.streamPath());
  Serial.println(data.dataPath());
  Serial.println(data.dataType());

  if(data.dataPath() == "/message"){
    Serial.println(data.stringData());
    miniPompa(data.stringData());
  }
}

void streamTimeoutCallback (bool timeout){
  if (timeout){
    Serial.println();
    Serial.println("Stream timeout, resume streaming... ");
    Serial.println();
  }  
}

void miniPompa(String data){
  if (data == "on"){
    if(readSensor() <100){
      digitalWrite(WATER_PUMP,HIGH);
      delay(5000);
      digitalWrite(WATER_PUMP,LOW);
      json.set("/message", "off");
      Firebase.updateNode(firebaseData2,"/Sensor",json);
    }
  }
  else{
      //state = false;
      digitalWrite(WATER_PUMP,LOW);
  }  
}

bool init_wifi()
{
  String temp_pref_ssid = preferences.getString("pref_ssid");
  String temp_pref_pass = preferences.getString("pref_pass");
  pref_ssid = temp_pref_ssid.c_str();
  pref_pass = temp_pref_pass.c_str();

  Serial.println(pref_ssid);
  Serial.println(pref_pass);

  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);

  start_wifi_millis = millis();
  WiFi.begin(pref_ssid, pref_pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start_wifi_millis > wifi_timeout) {
      WiFi.disconnect(true, true);
      return false;
    }
  }
  return true;
}

void scan_wifi_networks()
{
  WiFi.mode(WIFI_STA);
  // WiFi.scanNetworks will return the number of networks found
  int n =  WiFi.scanNetworks();
  if (n == 0) {
    SerialBT.println("no networks found");
  } else {
    SerialBT.println();
    SerialBT.print(n);
    SerialBT.println(" networks found");
    delay(1000);
    for (int i = 0; i < n; ++i) {
      ssids_array[i + 1] = WiFi.SSID(i);
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(ssids_array[i + 1]);
      network_string = i + 1;
      network_string = network_string + ": " + WiFi.SSID(i) + " (Strength:" + WiFi.RSSI(i) + ")";
      SerialBT.println(network_string);
    }
    wifi_stage = SCAN_COMPLETE;
  }
}

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    wifi_stage = SCAN_START;
  }

  if (event == ESP_SPP_DATA_IND_EVT && wifi_stage == SCAN_COMPLETE) { // data from phone is SSID
    int client_wifi_ssid_id = SerialBT.readString().toInt();
    client_wifi_ssid = ssids_array[client_wifi_ssid_id];
    wifi_stage = SSID_ENTERED;
  }

  if (event == ESP_SPP_DATA_IND_EVT && wifi_stage == WAIT_PASS) { // data from phone is password
    client_wifi_password = SerialBT.readString();
    client_wifi_password.trim();
    wifi_stage = PASS_ENTERED;
  }

}

void callback_show_ip(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    SerialBT.print("ESP32 IP: ");
    SerialBT.println(WiFi.localIP());
    bluetooth_disconnect = true;
  }
}

void disconnect_bluetooth()
{
  delay(1000);
  Serial.println("BT stopping");
  SerialBT.println("Bluetooth disconnecting...");
  delay(1000);
  SerialBT.flush();
  SerialBT.disconnect();
  SerialBT.end();
  Serial.println("BT stopped");
  delay(1000);
  bluetooth_disconnect = false;
}
