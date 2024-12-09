

/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read new NUID from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to the read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the type, and the NUID if a new card has been detected. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 */

#include <SPI.h>
#include <Servo.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <SoftwareSerial.h>

#define SS_PIN 10
#define RST_PIN 9

#define TRIG_PIN 7
#define ECHO_PIN 6
#define PASS_LED 5
#define FAIL_LED 4
#define YELLOW1 A1
#define YELLOW2 A2
#define YELLOW3 A3

StaticJsonDocument<200> doc_in;
StaticJsonDocument<200> doc_out;

 // 객체 생성
Servo myServo;
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
// SoftwareSerial bluetooth(2,3);

// 변수 선언
int count = 5;
long duration;
int distance;
String machine = "흔들그네";

void setup() { 
  myServo.attach(3);
  SPI.begin(); 
  rfid.PCD_Init();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PASS_LED, OUTPUT);
  pinMode(FAIL_LED, OUTPUT);
  pinMode(YELLOW1, OUTPUT);
  pinMode(YELLOW2, OUTPUT);
  pinMode(YELLOW3, OUTPUT);

  // 시리얼 통신 시작
  Serial.begin(9600);
  // bluetooth.begin(9600);
}
 

void loop() {
  if(Serial.available() > 1){
    String myjson = Serial.readStringUntil('\n');
    Serial.flush();
    DeserializationError error = deserializeJson(doc_in, myjson);

    //   if(bluetooth.available() > 1){
    // String myjson = bluetooth.readStringUntil('\n');
    // bluetooth.flush();
    // DeserializationError error = deserializeJson(doc_in, myjson);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    
    int height = doc_in["height"];
    int weight = doc_in["weight"];

    bool isAuthorized = (height >= 100); // 인증 조건 확인

    if (isAuthorized) {
      processAuthorizedAccess();
    }   else {
      processUnauthorizedAccess();
    }
    height=0; weight=0;
  }

  //초음파 센서로 거리 측정
  measureDistance();

  // 노란불 상태 업데이트
  updateYellowLights();



  // RFID로 사용자 인증 처리
  if (count < 10) {
    handleRFID();
  }

  if( count >= 9){
     digitalWrite(FAIL_LED, HIGH);
  }
  else{
     digitalWrite(FAIL_LED, LOW);
  }

  
}


// 거리 측정 함수
void measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2;
    // Serial.print(distance);

    if (distance < 10) {
        count = max(count - 1, 0);  // count는 0 이하로 감소하지 않음
        delay(1000);

        // Serial.println();
        // Serial.print(count);
    }
    delay(1000);
}

// 노란불 상태 업데이트 함수
void updateYellowLights() {
    if (count >= 9) {
        setYellowLights(HIGH, HIGH, HIGH);
    } else if (count >= 6) {
        setYellowLights(HIGH, HIGH, LOW);
    } else if (count >= 3) {
        setYellowLights(HIGH, LOW, LOW);
    } else {
        setYellowLights(LOW, LOW, LOW);
    }
}

// 노란불 제어 함수
void setYellowLights(int state1, int state2, int state3) {
    digitalWrite(YELLOW1, state1);
    digitalWrite(YELLOW2, state2);
    digitalWrite(YELLOW3, state3);
}

// RFID 처리 함수
void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String output;

  for(int i = 0; i < 4; i++){
    output += String(rfid.uid.uidByte[i], HEX);
  }


  doc_out["id"] = output;
  doc_out["machine"] = machine;

  String jsonData = "";
  serializeJson(doc_out, jsonData);

  //  bluetooth.println(jsonData); 
  Serial.println(jsonData);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
}


// 인증 성공 처리 함수
void processAuthorizedAccess() {
  count++;
  // Serial.print(count);
  // Serial.println();
  digitalWrite(PASS_LED, HIGH);

  moveServo(90, 0);  // 서보모터 열기
  delay(3000);
  moveServo(0, 90);  // 서보모터 닫기

  digitalWrite(PASS_LED, LOW);
}

// 인증 실패 처리 함수
void processUnauthorizedAccess() {
  digitalWrite(FAIL_LED, HIGH);
  delay(1000);
  digitalWrite(FAIL_LED, LOW);
}

// 서보모터 제어 함수
void moveServo(int startAngle, int endAngle) {
  int step = (startAngle < endAngle) ? 1 : -1;
  for (int angle = startAngle; angle != endAngle + step; angle += step) {
    myServo.write(angle);
    delay(10);  // 서보모터 이동 시간 조정
  }
}