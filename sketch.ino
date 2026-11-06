#include <WiFi.h>
#include <HTTPClient.h>

// =====================================================
// HOSPITAL DIGITAL TWIN
// ESP32 + 3 POTENTIOMETERS + THINGSPEAK
// =====================================================

// ---------------- WIFI ----------------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// ---------------- THINGSPEAK ----------------
unsigned long CHANNEL_ID = 3441444;
const char* WRITE_API_KEY = "BGCFZBWNYO12TWBC";

const char* THINGSPEAK_URL =
  "http://api.thingspeak.com/update";

// ---------------- POTENTIOMETER PINS ----------------
const int HR_PIN = 34;
const int TEMP_PIN = 35;
const int SPO2_PIN = 32;

// ---------------- VITAL RANGES ----------------
const int HR_MIN = 40;
const int HR_MAX = 180;

const float TEMP_MIN = 34.0;
const float TEMP_MAX = 42.0;

const int SPO2_MIN = 80;
const int SPO2_MAX = 100;

// ThingSpeak update interval
const unsigned long UPDATE_INTERVAL = 20000;

unsigned long lastUpdate = 0;

// =====================================================
// WIFI CONNECTION
// =====================================================

void connectWiFi()
{
  Serial.println();
  Serial.println("Connecting to Wokwi Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Wi-Fi connection failed!");
  }
}

// =====================================================
// SEND DATA TO THINGSPEAK
// =====================================================

void sendToThingSpeak(int heartRate, float temperature, int spo2)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi not connected.");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  String url = String(THINGSPEAK_URL) +
               "?api_key=" + WRITE_API_KEY +
               "&field1=" + String(heartRate) +
               "&field2=" + String(temperature, 1) +
               "&field3=" + String(spo2);

  Serial.println();
  Serial.println("Uploading to ThingSpeak...");

  if (!http.begin(client, url))
  {
    Serial.println("HTTP connection failed!");
    return;
  }

  int httpCode = http.GET();

  if (httpCode > 0)
  {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpCode);

    String response = http.getString();

    Serial.print("ThingSpeak Response: ");
    Serial.println(response);

    if (httpCode == 200 && response != "0")
    {
      Serial.println("ThingSpeak upload SUCCESS!");
    }
    else
    {
      Serial.println("ThingSpeak upload failed!");
    }
  }
  else
  {
    Serial.print("HTTP Error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("=====================================");
  Serial.println("     HOSPITAL DIGITAL TWIN");
  Serial.println("  PATIENT VITAL MONITORING SYSTEM");
  Serial.println("=====================================");

  // ESP32 ADC resolution
  analogReadResolution(12);

  // Connect Wi-Fi
  connectWiFi();
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  // Reconnect Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println();
    Serial.println("Wi-Fi disconnected!");
    connectWiFi();
  }

  // ===================================================
  // READ POTENTIOMETERS
  // ===================================================

  int hrADC = analogRead(HR_PIN);
  int tempADC = analogRead(TEMP_PIN);
  int spo2ADC = analogRead(SPO2_PIN);

  // ===================================================
  // ADC -> HEART RATE
  // ===================================================

  int heartRate = map(
    hrADC,
    0,
    4095,
    HR_MIN,
    HR_MAX
  );

  // ===================================================
  // ADC -> TEMPERATURE
  // ===================================================

  float temperature =
    TEMP_MIN +
    ((float)tempADC / 4095.0) *
    (TEMP_MAX - TEMP_MIN);

  // ===================================================
  // ADC -> SPO2
  // ===================================================

  int spo2 = map(
    spo2ADC,
    0,
    4095,
    SPO2_MIN,
    SPO2_MAX
  );

  // Keep values within limits
  heartRate = constrain(
    heartRate,
    HR_MIN,
    HR_MAX
  );

  temperature = constrain(
    temperature,
    TEMP_MIN,
    TEMP_MAX
  );

  spo2 = constrain(
    spo2,
    SPO2_MIN,
    SPO2_MAX
  );

  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println();
  Serial.println("-------------------------------------");

  Serial.print("Heart Rate : ");
  Serial.print(heartRate);
  Serial.println(" BPM");

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("SpO2       : ");
  Serial.print(spo2);
  Serial.println(" %");

  Serial.println("-------------------------------------");

  // ===================================================
  // THINGSPEAK UPDATE
  // ===================================================

  if (millis() - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = millis();

    sendToThingSpeak(
      heartRate,
      temperature,
      spo2
    );
  }

  // Read values every 1 second
  delay(1000);
}