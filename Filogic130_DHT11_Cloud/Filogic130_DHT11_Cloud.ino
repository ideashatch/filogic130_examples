#include <LWiFi.h>
#include <SimpleDHT.h>
#include <LEDWidget.h>
#include "httpclient.h"

// DHT Sensor setup
const byte pinDHT11 = 17;     //DHT11 using GPIO 17
SimpleDHT11 dht11(pinDHT11);
int err = SimpleDHTErrSuccess;
float temperature = 0;
float humidity = 0;            

char ssid[] = "YOUR_SSID";      //  your network SSID (name)
char pass[] = "YOUR_PASS";  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;               // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
char server[] = "https://api.thingspeak.com/update";   // This website checks TLS/SSL capabilities
#define BUF_SIZE    (512)
#define URL_BUF_LEN (256)

httpclient_t iotClient;
httpclient_data_t iotData;
char iotRespBuf[BUF_SIZE], iotPostBuf[URL_BUF_LEN];

void setup() 
{
     Serial.begin(115200);
     LEDWidget.Begin(FILOGIC_LED_0);                // 內建 RGB 設為程式執行時的確認燈號
     LEDWidget.Color(FILOGIC_LED_0, FILOGIC_LED_R);  
     LEDWidget.Set(false);                          // 關閉內建的RGB
     delay (1000);
     LEDWidget.Set(true);     
     while (!Serial) 
     {
          ; // wait for serial port to connect. Needed for native USB port only
     }
  
      // attempt to connect to Wifi network:
     while (status != WL_CONNECTED) 
     {
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

void loop() 
{
     Serial.println("=================================");
     Serial.println("溫溼度模組 DHT11 檢測中...."      );
     LEDWidget.Set(true); 
     int err = SimpleDHTErrSuccess;
     uint8_t buf[64];

     memset( &iotData, 0, sizeof(iotData) );
     iotData.response_buf = iotRespBuf;
     iotData.response_buf_len = BUF_SIZE;
     
     // Check for valid values
     while ((err = dht11.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
     {
        Serial.println("Read DHT11 failed, please check the connections!");
        delay(1000);
        LEDWidget.Set(false); 
        delay(1000);
        LEDWidget.Set(true);
     }
     
     // Setup HTTP header and body
     sprintf(iotPostBuf, "api_key={YOUR_API_KEY}&field1=%f&field2=%f", 
                          humidity, 
                          temperature);
     iotData.post_buf = iotPostBuf;
     iotData.post_buf_len = strlen(iotPostBuf);
     
     sprintf((char*)buf, "application/x-www-form-urlencoded");
     iotData.post_content_type = (char*)buf;
      
     Serial.println("上傳至ThingSpeak...");
     // Post HTTP request
     if( httpclient_post(&iotClient, server, &iotData) == HTTPCLIENT_OK)
     {
         Serial.println("攝氏溫度：" + String((float)temperature) + " °C"); 
         Serial.println("環境溼度：" + String((float)humidity)+ " ％");
         Serial.println("=================================");
         LEDWidget.Set(false); 
     }
     else Serial.println("HTTP post failed.");
          
     delay(15000);

}

void printWifiStatus() 
{
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
