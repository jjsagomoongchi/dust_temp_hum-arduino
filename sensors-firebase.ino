#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <Wire.h>
#include <AM2320.h>
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
#define API_KEY "API_KEY"
#define DATABASE_URL "https://DATABASE_NAME.firebaseio.com"
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
#include <Wire.h>
#include <AM2320.h>
AM2320 th;

#define dust_sensor A0
int count = 0;
unsigned long int dust_value = 0;
float dust = 0;
float dustDensityug = 0;
#define sensor_led D5
int sampling = 280;
int waiting = 40;
float stop_time = 9680;
float h = 0;
float t = 0;
unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(sensor_led, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
}

void loop() {
  int i = 0;
  int dustin = 0;
  for (i = 0; i < 100; i++) {
    digitalWrite(sensor_led, LOW);
    delayMicroseconds(sampling);
    dust += analogRead(dust_sensor);
    delayMicroseconds(waiting);
    digitalWrite(sensor_led, HIGH);
    delayMicroseconds(stop_time);
  }
  count += i;
  if (millis() - sendDataPrevMillis > 15000) {
    dust_value = dust / count;
    Serial.print("Dust val: ");
    Serial.println(dust_value);
    dustDensityug = (0.17 * (dust_value * (3.2 / 1024)) - 0.1) * 1000;
    Serial.print("Dust [ug/m^3]: ");
    Serial.println(dustDensityug);
    th.Read();
    h = th.h;
    t = th.t;
    Serial.print("Hum: ");
    Serial.println(h);
    Serial.print("Tem: ");
    Serial.println(t);
    Firebase.setFloat(fbdo, "/product/window/env/dust", dustDensityug);
    Firebase.setFloat(fbdo, "/product/window/env/hum", h);
    Firebase.setFloat(fbdo, "/product/window/env/tem", t);
    int open = 0;
    int close = 0;
    int air_cond = 0;
    Firebase.getInt(fbdo, F("/product/window/env/open"), &open);
    Firebase.getInt(fbdo, F("/product/window/env/close"), &close);
    Firebase.getInt(fbdo, F("/product/switch/env/air_conditioner"), &air_cond);
    int ad = 0;
    if ((((dustDensityug > 40 || h > 70) || t > 32) && !air_cond) && !open) {
      ad = 1;
    }
    if ((((dustDensityug < 15 || h < 30) || t < 20) || air_cond) && !close) {
      ad = 2;
    }
    Firebase.setInt(fbdo, F("/product/window/env/motor"), ad);
    sendDataPrevMillis = millis();
    count = 0;
    dust = 0;
    delay(100);
  } else {
    Serial.println(dust / count);
    delay(100);
  }
}