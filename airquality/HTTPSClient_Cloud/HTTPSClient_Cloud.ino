/*
  TLS client

  This sketch connects to a website
  using Filogic 130 over TLS (HTTPS)

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the Wifi.begin() call accordingly.

  Circuit:
    Filogic 130

  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe
  modified Jan 2017
  by MediaTek Labs
*/

#include <LWiFi.h>
#include "httpclient.h"
#include "DHT.h"

char ssid[] = "hel";      //  your network SSID (name)
char pass[] = "1234567890";  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;               // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
char server[] = "https://api.thingspeak.com/update";   // This website checks TLS/SSL capabilities
#define BUF_SIZE    (512)
#define URL_BUF_LEN (256)

httpclient_t iotClient;
httpclient_data_t iotData;
char iotRespBuf[BUF_SIZE], iotPostBuf[URL_BUF_LEN];
char get_url[URL_BUF_LEN];
int  temperature;
int humidity = 20;
// Sensor
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define ADC0_PIN A0
#define ADC1_PIN A1
#define ADC2_PIN    A4
#define ADC3_PIN    A5
void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  pinMode(ADC0_PIN, INPUT);//選告GPIO 13作為輸入（Mq2氣體感測）
  pinMode(ADC1_PIN, INPUT);
  pinMode(ADC2_PIN, INPUT);//選告GPIO 13作為輸入（Mq2氣體感測）
  pinMode(ADC3_PIN, INPUT);
  dht.begin();

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  httpclient_connect(&iotClient, server);
  Serial.println("\nOK!");
}

void loop() {
  // if there are incoming bytes available
  uint8_t buf[64];

  memset( &iotData, 0, sizeof(iotData) );
  iotData.response_buf = iotRespBuf;
  iotData.response_buf_len = BUF_SIZE;
  //humidity=humidity+1;
  int GasValue, GasValueA; //宣告變數GasValue
  int Set1, Set2; //宣告變數Set設定值
  GasValue = analogRead(ADC0_PIN);
  GasValueA = analogRead(ADC1_PIN);
  Set1 = analogRead(ADC2_PIN);
  Set2 = analogRead(ADC3_PIN);
  //humidity = dht.readHumidity();
  //temperature = dht.readTemperature();
  // sprintf(iotPostBuf, "api_key=BDC1BYFEE296R1TK&field1=%d", humidity);
  sprintf(iotPostBuf, "api_key=LWPR64LQVMCITQK7&field1=%d&field2=%d&field3=%d&field4=%d", GasValue, GasValueA,Set1,Set2);
  iotData.post_buf = iotPostBuf;
  iotData.post_buf_len = strlen(iotPostBuf);

  sprintf((char*)buf, "application/x-www-form-urlencoded");
  iotData.post_content_type = (char*)buf;

  Serial.println("connecting to ThingSpeak for update...");
  if ( httpclient_post(&iotClient, server, &iotData) == HTTPCLIENT_OK)
  {
    /* Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" *C");*/
    Serial.print("MQ2:"); Serial.println(GasValue); //GasValue顯示在序列視窗
    Serial.print("MQ135:"); Serial.println(GasValueA);
  }
  else Serial.println("HTTP post failed.");

  delay(16000);
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
