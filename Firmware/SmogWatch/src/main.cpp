#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoHttpClient.h>
#include <DNSServer.h>
#include <WiFiManager.h> 
#include <Ticker.h>
#include <ESP8266HTTPUpdateServer.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <dht.h>

#include "SoftwareSerial.h"
#include "PMS.h"

#include "pages.h"

#include "config.h"

#define DHTPIN 13 // Pin which is connected to the DHT sensor.
#define RESETPIN 14 // When connected to ground during startup - wifi credentials are reset.

#define LOG_INFO(s) { Serial.print("INFO: "); Serial.println(s); }

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
uint8_t NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

uint32_t ntpTime = 0;

static struct CurrentMeasurements {
  float temperature; // *C
  float pressure;    //  Pa
  float humidity;    //  %
  uint16_t pm1_0;    //  ug/m3
  uint16_t pm2_5;    //  ug/m3
  uint16_t pm10_0;   //  ug/m3

  uint32_t temperature_ts;
  uint32_t pressure_ts;
  uint32_t humidity_ts;
  uint32_t pm_ts;
} currentMeasurements;

ESP8266WebServer server(80); // Web interface
ESP8266HTTPUpdateServer httpUpdater; // OTA update
WiFiUDP UDP; // UDP for NTP
Adafruit_BMP280 bme; // BMP280 sensor
dht DHT; // DHT22 sensor
SoftwareSerial swSer1(12, 14, false, 256); // Software serial for PMS sensor (Rx, Tx)
PMS pms(swSer1); // PMS sensor

Ticker _30_sec;
Ticker _3600_sec;

volatile bool makeMeasurements = true;
volatile bool refreshTime = true;

uint32_t getCurrentTime();
void sendNTPpacket(IPAddress& address);

// -----------------------------------------------
// SENSORS
// -----------------------------------------------
bool makeMeasurementsBms() {
  currentMeasurements.temperature = bme.readTemperature();
  currentMeasurements.pressure = bme.readPressure();
  currentMeasurements.temperature_ts = getCurrentTime();
  currentMeasurements.pressure_ts = getCurrentTime();

  return true;
}

bool makeMeasurementsDht() {
  int chk = DHT.read22(DHTPIN);
  if (chk == DHTLIB_OK) {
    currentMeasurements.humidity = DHT.humidity;
    currentMeasurements.humidity_ts = getCurrentTime();

    return true;
  }

  return false;
}

// Called every 30s
bool makeMeasurementsPms() {
  bool ret = false;
  static uint16_t counter = 0;
  PMS::DATA data;

  if (counter == 0) {
    LOG_INFO("PMS - Wake up");
    pms.wakeUp(); // Let it spin for 30 seconds for stable measurements
  } else if (counter == 1) {
    pms.requestRead();
    if (pms.readUntil(data)) {
      currentMeasurements.pm1_0 = data.PM_AE_UG_1_0;
      currentMeasurements.pm2_5 = data.PM_AE_UG_2_5;
      currentMeasurements.pm10_0 = data.PM_AE_UG_10_0;
      currentMeasurements.pm_ts = getCurrentTime();
      ret = true;
    }

    LOG_INFO("PMS - Sleep");
    delay(250);
    pms.sleep();
  } else if (counter == 29) { // Measurement every 15 minutes
    counter = 0;
    return ret; // Do not increment
  }

  counter++;

  return ret;
}

// -----------------------------------------------
// DATA PRESENTATION
// -----------------------------------------------
void printMeasurements() {
  Serial.println();
  Serial.print("Temperature: ");
  Serial.print(currentMeasurements.temperature);
  Serial.print(" [*C] - ");
  Serial.println(currentMeasurements.temperature_ts);

  Serial.print("Pressure: ");
  Serial.print(currentMeasurements.pressure);
  Serial.print(" [Pa] - ");
  Serial.println(currentMeasurements.pressure_ts);

  Serial.print("Humidity: ");
  Serial.print(currentMeasurements.humidity);
  Serial.print(" [%] - ");
  Serial.println(currentMeasurements.humidity_ts);

  Serial.print("PM 1.0: ");
  Serial.print(currentMeasurements.pm1_0);
  Serial.print(" [ug/m3] - ");
  Serial.println(currentMeasurements.pm_ts);

  Serial.print("PM 2.5: ");
  Serial.print(currentMeasurements.pm2_5);
  Serial.print(" [ug/m3] - ");
  Serial.println(currentMeasurements.pm_ts);

  Serial.print("PM 10: ");
  Serial.print(currentMeasurements.pm10_0);
  Serial.print(" [ug/m3] - ");
  Serial.println(currentMeasurements.pm_ts);
}

void sendMeasurements() {
  if (!useRemoteServer) return;

  String json = "{\"secret_key\": \"";
  json = json + apiSecretKey;
  json = json + "\",\"temperature\": ";
  json = json + currentMeasurements.temperature;
  json = json + ",\"humidity\": ";
  json = json + currentMeasurements.humidity;
  json = json + ",\"pressure\": ";
  json = json + currentMeasurements.pressure;
  json = json + ",\"pm1_0\": ";
  json = json + currentMeasurements.pm1_0;
  json = json + ",\"pm2_5\": ";
  json = json + currentMeasurements.pm2_5;
  json = json + ",\"pm10\": ";
  json = json + currentMeasurements.pm10_0;
  json = json + ",\"temperature_ts\": ";
  json = json + currentMeasurements.temperature_ts;
  json = json + ",\"humidity_ts\": ";
  json = json + currentMeasurements.humidity_ts;
  json = json + ",\"pressure_ts\": ";
  json = json + currentMeasurements.pressure_ts;
  json = json + ",\"pm_ts\": ";
  json = json + currentMeasurements.pm_ts + "}";
  
  LOG_INFO("Making POST request...");

  WiFiClient wifi;
  HttpClient client = HttpClient(wifi, apiAddress, apiPort);

  client.post(apiAddEndpoint, "application/json", json);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  LOG_INFO(statusCode);
  LOG_INFO(response);
}

// -----------------------------------------------
// TIME
// -----------------------------------------------
uint32_t getTimeFromNtp() {
  IPAddress timeServerIP;

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    LOG_INFO("Cannot resolve NTP address");
    return 0;
  }

  LOG_INFO("Sending NTP request...");
  sendNTPpacket(timeServerIP);

  uint8_t retry = 10;
  uint32_t packetLength = 0;

  while (retry) { // If there's no response (yet)
    packetLength = UDP.parsePacket();
    if (!packetLength) {
      LOG_INFO("No response yet...");
      retry--;
      delay(250);
    } else {
      break;
    }
  }

  if (packetLength == 0) return 0;

  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;

  if (UNIXTime > 0) {
    ntpTime = UNIXTime - (millis() / 1000);
  }

  return UNIXTime;
}

uint32_t getCurrentTime() {
  return ntpTime + (millis() / 1000);
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

// -----------------------------------------------
// TICKER
// -----------------------------------------------
void _30_second_tick() {
  makeMeasurements = true;
}

void _3600_second_tick() {
  refreshTime = true;
}

// -----------------------------------------------
// HTTP SERVER
// -----------------------------------------------
void handleRoot() {
  String data = __index_htm;

  String temperature = String(currentMeasurements.temperature, 1);
  String humidity = String(currentMeasurements.humidity, 0);
  String pressure = String(currentMeasurements.pressure / 100.0, 0);
  String pm1 = String(currentMeasurements.pm1_0);
  String pm2_5 = String(currentMeasurements.pm2_5);
  String pm10 = String(currentMeasurements.pm10_0);
  String temperature_ts = String(currentMeasurements.temperature_ts);
  String humidity_ts = String(currentMeasurements.humidity_ts);
  String pressure_ts = String(currentMeasurements.pressure_ts);
  String pm_ts = String(currentMeasurements.pm_ts);

  data.replace("#LOCATION#", location);
  data.replace("#VERSION#", version);
  data.replace("#TEMPERATURE#", temperature);
  data.replace("#HUMIDITY#", humidity);
  data.replace("#PRESSURE#", pressure);
  data.replace("#PM1_0#", pm1);
  data.replace("#PM2_5#", pm2_5);
  data.replace("#PM10#", pm10);
  data.replace("#TEMPERATURE_TS#", temperature_ts);
  data.replace("#HUMIDITY_TS#", humidity_ts);
  data.replace("#PRESSURE_TS#", pressure_ts);
  data.replace("#PM_TS#", pm_ts);

  server.send(200, "text/html", data);
}

// -----------------------------------------------
// ARDUINO FUNCTIONS
// -----------------------------------------------
void setup() {
  WiFiManager wifiManager;

  Serial.begin(115200); // Main, hardware serial port - debugging, etc.
  swSer1.begin(9600); // Soft serial port for PMS sensor

  pinMode(RESETPIN, INPUT_PULLUP);
  if (digitalRead(RESETPIN) == LOW) {
    Serial.print("RESETTING WIFI SETTINGS...");
    wifiManager.resetSettings();
  }

  wifiManager.autoConnect("SmogWatch");

  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());

  if (!MDNS.begin("powietrze")) {
    Serial.println("Error setting up MDNS responder!");
  }

  Serial.println("mDNS responder started");

  if (!bme.begin(0x76)) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  pms.passiveMode();
  pms.sleep();
  delay(250);

  _30_sec.attach(30.0, _30_second_tick);
  _3600_sec.attach(3600.0, _3600_second_tick);

  httpUpdater.setup(&server);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  MDNS.addService("http", "tcp", 80);
}

void loop() {
  server.handleClient();

  if (refreshTime) {
    uint32_t time = getTimeFromNtp();
    if (time > 0) refreshTime = false;
  }
  
  if (makeMeasurements) {
    makeMeasurementsBms();
    makeMeasurementsDht();
    bool send = makeMeasurementsPms();

    printMeasurements();
    if (send) sendMeasurements();
    makeMeasurements = false;
  }
}