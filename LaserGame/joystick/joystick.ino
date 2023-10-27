#include <Unistep2.h>
#include <LBLE.h>
#include <LBLEPeriphral.h>

//藍牙宣告
//定義一個只有 1 個特徵的簡單 GATT 服務
LBLEService ledService("110C0010-E8F2-537E-4F6C-D104768A1214");
LBLECharacteristicString switchCharacteristic("110C0011-E8F2-537E-4F6C-D104768A1214", LBLE_READ | LBLE_WRITE);
#define BUF_SIZE      30

bool bConnStatus = false;
uint8_t rData[BUF_SIZE];

String BTData;

//蜂鳴器
#define buzzPin 13

//雷射模組
byte cmd_1[] = { 0x80, 0x06, 0x03, 0x77 };         // 連續測量模式
byte cmd_2[] = { 0x80, 0x06, 0x02, 0x78 };         // 單次測量模式
byte cmd_3[] = { 0x80, 0x06, 0x05, 0x01, 0x74 };   // 雷射點打開
byte cmd_4[] = { 0x80, 0x06, 0x05, 0x00, 0x75 };   // 雷射點關閉

byte cmd_5[] = { 0xFA, 0x04, 0x09, 0x05, 0xF4 };   // 5公尺
byte cmd_6[] = { 0xFA, 0x04, 0x09, 0x0A, 0xEF };   // 10公尺
byte cmd_7[] = { 0xFA, 0x04, 0x09, 0x1E, 0xDB };   // 30公尺
byte cmd_8[] = { 0xFA, 0x04, 0x09, 0x32, 0xC7 };   // 50公尺
byte cmd_9[] = { 0xFA, 0x04, 0x09, 0x50, 0xA9 };    // 80公尺

byte cmd_10[] = { 0xFA, 0x04, 0x0C, 0x02, 0xF4 };   // 0.1mm 精度
byte cmd_11[] = { 0xFA, 0x04, 0x0C, 0x01, 0xF5 };   // 1mm 精度

byte cmd_12[] = { 0xFA, 0x04, 0x0A, 0x00, 0xF8 };   // 2Hz
byte cmd_13[] = { 0xFA, 0x04, 0x0A, 0x05, 0xF3 };   // 5Hz
byte cmd_14[] = { 0xFA, 0x04, 0x0A, 0x0A, 0xEE };   // 10Hz
byte cmd_15[] = { 0xFA, 0x04, 0x0A, 0x14, 0xE4 };   // 20Hz

String LRF_feedback ;  // Collect the RAW LRF Feedback
String measurement ;   // Collect the Digital LRF Measurement
char data;
int Bytes_counter = 12;
bool ws = true;
bool st = true;
bool co = true;
bool ch = true;
#define LRF Serial2       // LRF module connected on Serial1 of ARDUINO Micro
float record[27];
float distance;
float pastdistance;
bool CMM_EN = true;
bool adc = true;
//步進馬達
int Xi = 0;
int Yi = 0;
double dudu = 0.087890625;
int xrun, yrun, Xdu, Ydu, X, Y;
int Counter = 0;
Unistep2 stepperX(23, 22, 21, 20, 4096, 900);// IN1, IN2, IN3, IN4, 總step數, 每步的延遲(in micros)
Unistep2 stepperY(19, 18, 17, 15, 4096, 900);// IN1, IN2, IN3, IN4, 總step數, 每步的延遲(in micros)
bool che = true;

//搖桿
#define xposPin A0         // 雙軸按鍵搖桿 VRx 接 Arduino Analog pin A0
#define yposPin A1         // 雙軸按鍵搖桿 VRy 接 Arduino Analog pin A1
int Xpos = 0;             // 定義X軸伺服器位址參數
int Ypos = 0;             // 定義丫軸伺服器位址參數
int buttonPin = 47;       // 搖桿按鍵輸出 SW 接 Arduino pin 7
int buttonPress = 0;      // 定義 Arduino 從搖桿按鍵 SW 讀入值為 buttonPress

int Time, pastTime;
int point = 0;
String strpoint;
void setup() {
  Serial.begin(9600);
  delay(100);
  LRF.begin(9600);
  delay(500);
  LRF.write(cmd_3, sizeof(cmd_3));          //開啟雷射點
  delay(500);
  LRF.write(cmd_7, sizeof(cmd_7));          //設定量測距離80公尺
  Serial.println("LRF Range Set to 30m");
  delay(500);
  LRF.write(cmd_10, sizeof(cmd_10));        //設定量測精度0.1mm
  Serial.println("LRF Resolution Set to O.1mm");
  delay(500);
  pinMode(buttonPin, INPUT);                //Arduino 從搖桿按鍵讀入電壓
  pinMode(xposPin, INPUT);
  pinMode(yposPin, INPUT);
  digitalWrite(buttonPin, HIGH);            //設定當按鍵沒按時，輸出為高電壓
  //步進馬達
  pinMode(23, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(20, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(15, OUTPUT);

  //蜂鳴器
  pinMode(buzzPin, OUTPUT);
  Measure();
  Measure();
  Measure();

  //執行掃描
  while (Counter < 27) {
    stepperX.run();
    Scan();
  }
  //回歸130度
  while (ws == true) {
    if (Counter > 28) {
      ws = false;
    }
    stepperX.run();
    reset();
  }
  //設定連續測量
  LRF.write(cmd_15, sizeof(cmd_15)); // Set LRF Module Frequency to 20Hz
  delay(100);
  LRF.write(cmd_9, sizeof(cmd_9)); //設定量測距離80公尺
  delay(100);
  LRF.write(cmd_10, sizeof(cmd_10)); //設定量測精度0.1mm
  delay(100);
  //初始化 BLE 子系統
  LBLE.begin();
  while (!LBLE.ready()) {
    delay(100);
  }
  Serial.println("BLE ready");

  Serial.print("Device Address = [");
  Serial.print(LBLE.getDeviceAddress());
  Serial.println("]");
  // 配置我們的廣告數據。
  // 在這種情況下，我們只需創建一個代表一個廣告的廣告
  // 具有設備名稱的可連接設備
  LBLEAdvertisementData advertisement;
  advertisement.configAsConnectableDevice("OvOcc");

  // 配置我們設備的通用訪問配置文件的設備名稱
  // 通常這與廣告數據中的名稱相同。
  LBLEPeripheral.setName("OvOcc");

  // 添加特性到 ledService
  ledService.addAttribute(switchCharacteristic);

  // 向 GATT 服務器添加服務（外設）
  LBLEPeripheral.addService(ledService);

  // 啟動 GATT 服務器 - 現在
  // 可連接
  LBLEPeripheral.begin();

  // 開始廣告
  LBLEPeripheral.advertise(advertisement);
  Serial.print("conected=");
  Serial.println(bConnStatus);
  Unistep2 stepperX(23, 22, 21, 20, 4096, 10);// IN1, IN2, IN3, IN4, 總step數, 每步的延遲(in micros)
  Unistep2 stepperY(19, 18, 17, 15, 4096, 10);// IN1, IN2, IN3, IN4, 總step數, 每步的延遲(in micros)
  digitalWrite(buzzPin, HIGH);//有源蜂鳴器響起
  delay(200);
  digitalWrite(buzzPin, LOW);//有源蜂鳴器響起
}

void loop() {
  //確認藍牙是否已經連接
  if (bConnStatus != LBLEPeripheral.connected()) {
    bConnStatus = LBLEPeripheral.connected();
    Serial.print("conected=");
    Serial.println(bConnStatus);
    digitalWrite(buzzPin, HIGH);//有源蜂鳴器響起
    delay(200);
    digitalWrite(buzzPin, LOW);//有源蜂鳴器響起
  }
  //App傳送資料
  if (switchCharacteristic.isWritten()) {
    const String value = switchCharacteristic.getValue();
    Serial.print("收到了: "); Serial.println(value);
    BTData = value;
  }

  stepperX.run();
  stepperY.run();
  Xpos = analogRead(xposPin);
  Ypos = analogRead(yposPin);
  buttonPress = digitalRead(buttonPin);    //讀入搖桿按鍵電位
  Movehorizontally();
  //Shooting();
  Continuousmeasurement();
  /*Serial.print("Xpos = ");
    Serial.println(Xpos);
    Serial.print("Ypos = ");
    Serial.println(Ypos);*/

  LRF_feedback = "";
  Bytes_counter = 0;
}

//寫資料給App
void sendd(String u) {
  if (LBLEPeripheral.connected()) {
    // increment the value
    // a=a+1;
    // String w=a+"";
    // switchCharacteristic.setValue(w);
    // Serial.print("送出了: "); Serial.println(w);
    // broadcasting value changes to all connected central devices
    // LBLEPeripheral.notifyAll(switchCharacteristic);
    const String newValue = u;
    switchCharacteristic.setValue(newValue);
    Serial.print("送出了: "); Serial.println(newValue);
    // broadcasting value changes to all connected central devices
    LBLEPeripheral.notifyAll(switchCharacteristic);
  }
}

//控制馬達轉動
void Movehorizontally() {
  if (Xdu <= -140) {
    if (Xpos < 1200) {
      LRmove(0);
    }
    else if (Xpos > 2200 || BTData == "4") {
      LRmove(10);
    }
  }
  else if (Xdu >= 130) {
    if (Xpos > 2200) {
      LRmove(0);
    }
    else if (Xpos < 1200 || BTData == "3") {
      LRmove(-10);
    }
  }
  else if (Ydu <= -60) {
    if (Ypos > 2200) {
      UDmove(0);
    }
    else if (Ypos < 1200 || BTData == "2") {
      UDmove(10);
    }
  }
  else if (Ydu >= 90) {
    if (Ypos < 1200) {
      UDmove(0);
    }
    else if (Ypos > 2200 || BTData == "9") {
      UDmove(-10);
    }
  }
  else if (Xdu > -140 && Xdu < 130) {
    if (Xpos > 2200 || BTData == "4") {
      LRmove(10);
    }
    else if (Xpos < 1200 || BTData == "3") {
      LRmove(-10);
    }
    else if (Xpos > 1500 && Xpos < 2000) {
      stepperX.move(0);
    }
    if (Ypos > 2200 || BTData == "9") {
      UDmove(-10);
    }
    else if (Ypos < 1200 || BTData == "2") {
      UDmove(10);
    }
    else if (Ypos > 1500 && Ypos < 2000) {
      stepperY.move(0);
    }
    else if (BTData == "5") {
      stepperX.move(0);
    }
    else if (BTData == "6") {
      stepperY.move(0);
    }
  }
}

void Scan() {
  while (stepperX.stepsToGo() != 0)
    stepperX.run();
  if (stepperX.stepsToGo() == 0) {
    Measure();
    delay(500);
    stepperX.move(114);
    xrun = xrun + stepperX.stepsToGo();
    Xdu = xrun * dudu;
    //Serial.println(Xdu);
    Counter++;

  } //Serial.print("Counter=");
  //Serial.println(Counter);
  //Serial.print("scanX=");
  //Serial.println(stepperX.stepsToGo());
}

//回歸130度
void reset() {
  if (stepperX.stepsToGo() == 0) {
    Serial.println("reset!!!!");
    Counter++;
    stepperX.move(-114 * 14);
    xrun = xrun + stepperX.stepsToGo();
    Xdu = xrun * dudu;
    Serial.print("X角度=");
    Serial.println(Xdu);
    for (int i = 0; i < 27; i++) {
      Serial.println(record[i]);
    }
  }
  /*Serial.print("resetCounter=");
    Serial.println(Counter);
    Serial.print("resetscanX=");
    Serial.println(stepperX.stepsToGo());*/

}

//單次測量
void Measure() {
  LRF.write(cmd_2, sizeof(cmd_2));
  delay(500);

  while (Serial.available() > 0 && Bytes_counter < 12) {
    data = Serial.read();
    LRF_feedback += data;
    Bytes_counter++;
  }
  if (Bytes_counter == 12) {
    measurement = LRF_feedback.substring(4, 10);
    record[Counter] = measurement.toFloat();
    Serial.print("measurement=");
    Serial.println(measurement);
  }
  LRF_feedback = "";
  Bytes_counter = 0;
}

//水平步進馬達作動
void LRmove(int u) {
  if (stepperX.stepsToGo() == 0) {
    stepperX.move(u);
    xrun = xrun + stepperX.stepsToGo();
    Xdu = xrun * dudu;
    //Serial.print("X角度: "); Serial.println(Xdu);
  }
}

//垂直步進馬達作動
void UDmove(int u) {
  if (stepperY.stepsToGo() == 0) {
    stepperY.move(u);
    yrun = yrun + stepperY.stepsToGo();
    Ydu = yrun * dudu;
    //Serial.print("Y角度: "); Serial.println(Ydu);
  }
}

/*void Shooting() {
  if (buttonPress == LOW) {                // 當搖桿按鍵為低電位時，（即按下按鍵）
    //Serial.println("button on");
    LRF.write(cmd_2, sizeof(cmd_2));
    delay(100);
    while (Serial.available() > 0 && Bytes_counter < 12) {
      data = Serial.read();
      LRF_feedback += data;
      Bytes_counter++;
    }
    if (Bytes_counter == 12) {
      measurement = LRF_feedback.substring(4, 10);
      distance = measurement.toFloat();
      Serial.print("measurement="); Serial.println(measurement);
      Serial.print("現在:"); Serial.println(distance);
      Serial.print("record:"); Serial.println(pastdistance);
      if (distance != 0.00) {
        if (Xdu < -130)  pastdistance = record[0];
        else if (Xdu >= -130 && Xdu < -120)  pastdistance = record[1];
        else if (Xdu >= -120 && Xdu < -110)  pastdistance = record[2];
        else if (Xdu >= -110 && Xdu < -100)  pastdistance = record[3];
        else if (Xdu >= -100 && Xdu < -90)  pastdistance = record[4];
        else if (Xdu >= -90 && Xdu < -80)  pastdistance = record[5];
        else if (Xdu >= -80 && Xdu < -70)  pastdistance = record[6];
        else if (Xdu >= -70 && Xdu < -60)  pastdistance = record[7];
        else if (Xdu >= -60 && Xdu < -50)  pastdistance = record[8];
        else if (Xdu >= -50 && Xdu < -40)  pastdistance = record[9];
        else if (Xdu >= -40 && Xdu < -30)  pastdistance = record[10];
        else if (Xdu >= -30 && Xdu < -20)  pastdistance = record[11];
        else if (Xdu >= -20 && Xdu < -10)  pastdistance = record[12];
        else if (Xdu >= -10 && Xdu < 0)  pastdistance = record[13];
        else if (Xdu >= 0 && Xdu < 10)  pastdistance = record[14];
        else if (Xdu >= 10 && Xdu < 20)  pastdistance = record[15];
        else if (Xdu >= 20 && Xdu < 30)  pastdistance = record[16];
        else if (Xdu >= 30 && Xdu < 40)  pastdistance = record[17];
        else if (Xdu >= 40 && Xdu < 50)  pastdistance = record[18];
        else if (Xdu >= 50 && Xdu < 60)  pastdistance = record[19];
        else if (Xdu >= 60 && Xdu < 70)  pastdistance = record[20];
        else if (Xdu >= 70 && Xdu < 80)  pastdistance = record[21];
        else if (Xdu >= 80 && Xdu < 90)  pastdistance = record[22];
        else if (Xdu >= 90 && Xdu < 100)  pastdistance = record[23];
        else if (Xdu >= 100 && Xdu < 110)  pastdistance = record[24];
        else if (Xdu >= 110 && Xdu < 120)  pastdistance = record[25];
        else if (Xdu >= 130)  pastdistance = record[26];
        if (pastdistance - distance >= 0.5) {
          Serial.println("shooting human");
          digitalWrite(buzzPin, HIGH);//有源蜂鳴器響起
        }
        else {
          digitalWrite(buzzPin, LOW);//有源蜂鳴器響起
        }
      }
    }
  }
  else {
    LRF.write(cmd_4, sizeof(cmd_4));
  }
  }*/

//連續測量以及判讀是否射擊到人
void Continuousmeasurement() {
  if (buttonPress == LOW || BTData == "7") {
    //ch = true;
    if (st == true) {
      pastTime = millis();
      st = false;
    }
    Time = millis();
    /*Serial.print("pastTime=");
      Serial.println(pastTime);
      Serial.print("Time=");
      Serial.println(Time);*/
    if (Time - pastTime <= 10000) {
      /*if (CMM_EN) {
        LRF.write(cmd_1, sizeof(cmd_1));
        CMM_EN = false;
      }*/
      LRF.write(cmd_2, sizeof(cmd_2));
      delay(50);
      while (Serial.available() > 0 && Bytes_counter < 12) {
        data = Serial.read();
        LRF_feedback += data;
        Bytes_counter++;
      }
      if (Bytes_counter == 12) {
        measurement = LRF_feedback.substring(4, 10);
        distance = measurement.toFloat();
        Serial.print("measurement="); Serial.println(measurement);
        Serial.print("現在:"); Serial.println(distance);
        Serial.print("record:"); Serial.println(pastdistance);
        if (distance != 0.00) {
          if (Xdu < -130)  pastdistance = record[0];
          else if (Xdu >= -130 && Xdu < -120)  pastdistance = record[1];
          else if (Xdu >= -120 && Xdu < -110)  pastdistance = record[2];
          else if (Xdu >= -110 && Xdu < -100)  pastdistance = record[3];
          else if (Xdu >= -100 && Xdu < -90)  pastdistance = record[4];
          else if (Xdu >= -90 && Xdu < -80)  pastdistance = record[5];
          else if (Xdu >= -80 && Xdu < -70)  pastdistance = record[6];
          else if (Xdu >= -70 && Xdu < -60)  pastdistance = record[7];
          else if (Xdu >= -60 && Xdu < -50)  pastdistance = record[8];
          else if (Xdu >= -50 && Xdu < -40)  pastdistance = record[9];
          else if (Xdu >= -40 && Xdu < -30)  pastdistance = record[10];
          else if (Xdu >= -30 && Xdu < -20)  pastdistance = record[11];
          else if (Xdu >= -20 && Xdu < -10)  pastdistance = record[12];
          else if (Xdu >= -10 && Xdu < 0)  pastdistance = record[13];
          else if (Xdu >= 0 && Xdu < 10)  pastdistance = record[14];
          else if (Xdu >= 10 && Xdu < 20)  pastdistance = record[15];
          else if (Xdu >= 20 && Xdu < 30)  pastdistance = record[16];
          else if (Xdu >= 30 && Xdu < 40)  pastdistance = record[17];
          else if (Xdu >= 40 && Xdu < 50)  pastdistance = record[18];
          else if (Xdu >= 50 && Xdu < 60)  pastdistance = record[19];
          else if (Xdu >= 60 && Xdu < 70)  pastdistance = record[20];
          else if (Xdu >= 70 && Xdu < 80)  pastdistance = record[21];
          else if (Xdu >= 80 && Xdu < 90)  pastdistance = record[22];
          else if (Xdu >= 90 && Xdu < 100)  pastdistance = record[23];
          else if (Xdu >= 100 && Xdu < 110)  pastdistance = record[24];
          else if (Xdu >= 110 && Xdu < 120)  pastdistance = record[25];
          else if (Xdu >= 130)  pastdistance = record[26];
          if (pastdistance - distance >= 0.5) {
            Serial.println("shooting human");
            digitalWrite(buzzPin, HIGH);//有源蜂鳴器響起
            delay(50);
            digitalWrite(buzzPin, LOW);//有源蜂鳴器響起
            delay(50);
            digitalWrite(buzzPin, HIGH);//有源蜂鳴器響起
            delay(50);
            digitalWrite(buzzPin, LOW);//有源蜂鳴器響起
            delay(50);
            digitalWrite(buzzPin, HIGH);//有源蜂鳴器響起
            delay(50);
            digitalWrite(buzzPin, LOW);//有源蜂鳴器響起
            delay(50);
            if (pastdistance - distance < 1) {
              Serial.println("加5分");
              if (co) {
                point += 5;
                strpoint = (String)point;
                sendd(strpoint);
                co = false;
              }
            }
            else if (pastdistance - distance >= 1) {
              Serial.println("加10分");
              if (co) {
                point += 10;
                strpoint = (String)point;
                sendd(strpoint);
                co = false;
              }
            }
            else if (pastdistance - distance >= 2) {
              Serial.println("加15分");
              if (co) {
                point += 15;
                strpoint = (String)point;
                sendd(strpoint);
                co = false;
              }
            }
            else if (pastdistance - distance >= 3) {
              Serial.println("加20分");
              if (co) {
                point += 20;
                strpoint = (String)point;
                sendd(strpoint);
                co = false;
              }
            }
            else if (pastdistance - distance >= 4) {
              Serial.println("加25分");
              if (co) {
                point += 25;
                strpoint = (String)point;
                sendd(strpoint);
                co = false;
              }
            }
          }
          else {
            digitalWrite(buzzPin, LOW);  //有源蜂鳴器關閉 
          }
        }
      }
    }
    else {
      Serial.println("雷射點關閉");
      LRF.write(cmd_4, sizeof(cmd_4));
      digitalWrite(buzzPin, LOW);  //有源蜂鳴器關閉
      co = true;
    } 
  }
  else {
    digitalWrite(buzzPin, LOW);  //有源蜂鳴器關閉
    //if(ch){
      LRF.write(cmd_4, sizeof(cmd_4));
      //ch = false;
    //}
    CMM_EN = true;
    st = true;
    co = true;
  }
}
