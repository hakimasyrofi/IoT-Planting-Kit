#include <WiFi.h>
#include <FirebaseESP32.h>

//konfigurasi Firebase
#define FIREBASE_HOST "plus-iot-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "yLXlNeHjOsFfsngwB2eMuH8limw7aGrbNjRaW6Su"

//konfigurasi WIFI
const char* WIFI_SSID = "#############";
const char* WIFI_PASSWORD = "##############";

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

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT);
  
  pinMode(WATER_PUMP, OUTPUT); //water
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);  
  }

  Serial.println("Connected to the WiFi Network");

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
