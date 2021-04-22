#define sensorPin 32
#include <BlynkSimpleEsp32.h>

// You should get Auth Token in the Blynk App.
char auth[] = "UqnD1L17mQWGYvLdOkPwHAcYLkiHPZDN";
 
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "############";
char pass[] = "#########";

int val = 0;

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass, "blynk-cloud.com",8080);
  pinMode(sensorPin, INPUT);
}

void loop()
{ 
  Blynk.run();

  int level = readSensor();
  
  Serial.print("Level Air: ");
  Serial.println(level);

  delay(1000);

  if (level < 600) {
    Serial.println("needs water, send notification");
    Blynk.notify("Time To Water The Plants");
//send notification
  } 
  else if (level >= 600 && level<900) {
  //do nothing, has not been watered yet
  Serial.println("has not been watered yet");
  }
  else {
    //st
    Serial.println("does not need water");
  }
}

int readSensor() {
  delay(100);              // wait 10 milliseconds
  val = analogRead(sensorPin);    // Read the analog value form sensor
  //di bawah ini untuk virtual pin blynk
  Blynk.virtualWrite(V1, val);
  return val;             // send current reading
}
