#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN 5
#define SS_PIN 53

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int m = 1000;

// 아이템 리스트
String item_list_name[6] = {"biscuit","peanut_butter","ace","choco_pie","cookie_cream","enaak"};
int item_list_price[6] = {200,300,300,400,400,200};

// 아이템 선택 되었는지
int selected_item = -1;

// 버튼
int buttonPinNumbers[] = {8,9,10,11,12,13};
// 모터 속도
int speed = 15;

// 스텝 모터
int stepMotors[6][4] = {
  {22, 23, 24, 25},
  {26, 27, 28, 29},
  {30, 31, 32, 33},
  {34, 35, 36, 37},
  {38, 39, 40, 41},
  {42, 43, 44, 45},
};

// 스텝 모터 드라이버 전력 조
int EN[1][2] = {
  {1,2},
};

// 스텝 모터 on/off용 변수
int old_key1 = 0;
int new_key1 = 0;
int old_key2 = 0;
int new_key2 = 0;
int old_key3 = 0;
int new_key3 = 0;
int old_key4 = 0;
int new_key4 = 0;
int old_key5 = 0;
int new_key5 = 0;
int old_key6 = 0;
int new_key6 = 0;

// 얼만큼 돌릴까
int degree = 50;

void setup() {
  SPI.begin();
  Serial.begin(2400);
  mfrc522.PCD_Init();  // MFRC522 초기화
  resetLcd();
  Serial.println("test");
  // 스텝모터 셋팅
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      pinMode(stepMotors[i][j], OUTPUT);
    }
  }

  // 스텝모터 드라이버 전력 조절
  for (int i = 0; i < 1; i++) {
    for (int j = 0; j < 2; j++) {
      pinMode(EN[i][j], OUTPUT);
      digitalWrite(EN[i][j], HIGH);
      analogWrite(EN[i][j], 200);
    }
  }

  // 버튼 셋팅
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPinNumbers[i], INPUT);
  }
 
}

void loop()
{
  // 스텝 모터 제어
  new_key1 = digitalRead(buttonPinNumbers[0]);
  new_key2 = digitalRead(buttonPinNumbers[1]);
  new_key3 = digitalRead(buttonPinNumbers[2]);
  new_key4 = digitalRead(buttonPinNumbers[3]);
  new_key5 = digitalRead(buttonPinNumbers[4]);
  new_key6 = digitalRead(buttonPinNumbers[5]);

  old_key1 = buttonHandle(old_key1,new_key1,0);
  old_key2 = buttonHandle(old_key2,new_key2,1);
  old_key3 = buttonHandle(old_key3,new_key3,2);
  old_key4 = buttonHandle(old_key4,new_key4,3);
  old_key5 = buttonHandle(old_key5,new_key5,4);
  old_key6 = buttonHandle(old_key6,new_key6,5);

  for(int i=0;i<6;i++){
    motorTurnOff(i);
  }

  if(selected_item != -1){
    manage_rfid();  
  } 

//  int currentTime = millis();
//  if(selected_item == -1 && currentTime - startTime){
//    // 파이썬 무한루프 돌리기용
//    Serial.println(); 
//    int startTime = millis();
//  }
}

int manage_rfid() {
  MFRC522::MIFARE_Key key;
  MFRC522::StatusCode status;
  int change_value=0, balance;
  byte balance_buf[18], change_buf[18], temp_buf[18];
  byte read_buf[20];
  String input_str;
  String cal_type;
  byte block, block_num, len=18;
  for (int i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    Serial.println("This is not a new card"); // 카드가 입력되지 않았습니다.
    return 0;
  }
   if ( ! mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Cannot read card"); // 카드 읽기에 실패했습니다.
    return 0;
  }
  Serial.print("UID tag :"); // UID tag : 출력
  String tag_uid= "";
  // 카드 UID 출력 ex) 2C 53 86 89
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println(); // 띄어쓰기
  if(selected_item != -1) //만약 입력을 받았다면
  {
       // Serial.print("selected item : ");
       // Serial.print(item_list_name[selected_item]);
       lcd.setCursor(0,0);
       lcd.print(item_list_name[selected_item]);
       lcd.setCursor(0,1);
       lcd.print("price: ");
       lcd.print(item_list_price[selected_item]);
       Serial.println(",");
       change_value = (item_list_price[selected_item] - (item_list_price[selected_item]*2));
       block_num = 6;//최종 사용금액은 6번 블록에 저장
       // 현재 잔액을 RFID 태그로부터 읽어 온다. 
       block = 4;
 
       status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); // 카드 암호해제
       if (status != MFRC522::STATUS_OK) { // 카드 암호해제에 실패 했다면
         Serial.print("Authentication failed: "); // Authentication failed: 출력
         Serial.println(mfrc522.GetStatusCodeName(status)); // 카드 암호해제 실패 원인 출력
         lcd.init();
         lcd.setCursor(0, 0);
         lcd.print("Error");
         return 0; // 끝 // 완전 정지 금지
       }
       
       status = mfrc522.MIFARE_Read(block, read_buf, &len); // 카드 정보 가져오기 성공 여부 status에 저장
       if (status != MFRC522::STATUS_OK) { // 카드 정보 가져오기 실패했다면
         Serial.print("Reading failed: "); // Reading failed: 출력
         Serial.println(mfrc522.GetStatusCodeName(status)); // 카드 정보 가져오기 실패 원인 출력
         lcd.init();
         lcd.setCursor(0, 0);
         lcd.print("Error");
         return 0; // 끝
       }

       balance=atoi((char*)read_buf); //read_buf이거를 정수로 변경
       Serial.print("old balance : "); // old balance : 출력
       Serial.print(balance); // 예전 돈 출력;
       Serial.println(","); // 라즈베리파이에서 쉽게 읽기용

       if(balance+change_value < 0) //잔액이 사용할 금액보다 적은 경우에는 잔액부족 메시지 출력후 종료
       {
          // balance=0;
          Serial.println("Insufficient balance"); // 잔액 부족 출력
          mfrc522.PCD_StopCrypto1(); //암호화 통신 종료
          lcd.init();
          lcd.setCursor(0, 0);
          lcd.print("error"); // 돈부족
          return 0; // 끝
       } else {
          balance+=change_value; // 예전 돈에 변경할돈 추가;
          Serial.print("change_value : "); // , change_value : 출력
          Serial.print(change_value); // 변경되는 돈 출력
          Serial.println(",");
          Serial.print("new balance : "); // new balance : 출력
          Serial.print(balance); // 변경된 돈출력
          Serial.println(","); 
       }
       
       // 변경된 잔액을 계산하여 byte 어레이에 저장
       String balance_str=String(balance,DEC);
       balance_str.getBytes(balance_buf,balance_str.length()+1);
       //숫자 부분을 제외한 나머지 부분은 공백으로 채운다.
       for(int k=balance_str.length();k<16;k++) balance_buf[k]='\0';
     
       // 변경내역을 byte 어레이에 저장
       String change_str=String(change_value,DEC);
       change_str.getBytes(change_buf,change_str.length()+1);
       for(int k=change_str.length();k<16;k++) change_buf[k]='\0';
       // 변경된 잔액을 4번 블록에 저장한다.
       status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); // 카드 암호화 해제 여부 status에 저장
       if (status != MFRC522::STATUS_OK) { // 카드 암호화 해제 실패 했다면
         Serial.print("PCD_Authenticate() failed: "); // PCD_Authenticate() failed: 출력
         Serial.println(mfrc522.GetStatusCodeName(status)); // 카드 암호화 해제 실패 이유 출력
         lcd.init();
         lcd.setCursor(0, 0);
         lcd.print("Error");
         return 0; // 끝
       }
       status = mfrc522.MIFARE_Write(block, balance_buf, 16); // 카드 정보 쓰기 여부 status에 저장
       if (status != MFRC522::STATUS_OK) { // 카드 정보 쓰기 실패 했다면
         Serial.print(F("MIFARE_Write() failed: ")); // MIFARE_Write() failed: 출력
         Serial.println(mfrc522.GetStatusCodeName(status)); // MIFARE_Write() failed: 출력
         lcd.init();
         lcd.setCursor(0, 0);
         lcd.print("Error");
         return 0; // 끝
       }
     
       //잔액의 최종 증감내역을 저장한다.
       status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_num, &key, &(mfrc522.uid)); // 카드 암호화 해제 여부 status에 저장
       if (status != MFRC522::STATUS_OK) { // 카드 암호화 해제 실패 했다면
         Serial.print("PCD_Authenticate() failed: "); // PCD_Authenticate() failed: 출력
         Serial.println(mfrc522.GetStatusCodeName(status)); // 카드 암호화 해제 실패 이유 출력
         lcd.init();
         lcd.setCursor(0, 0);
         lcd.print("Error");
         return 0; // 끝
       }
       else Serial.println("PCD_Authenticate() success: "); // PCD_Authenticate() success: 출력
       status = mfrc522.MIFARE_Write(block_num, change_buf, 16); // 카드 정보 쓰기 여부 status에 저장
       if (status != MFRC522::STATUS_OK) { // 카드 정보 쓰기 실패 했다면
         Serial.print(F("MIFARE_Write() failed: ")); // MIFARE_Write() failed: 출력
         Serial.println(mfrc522.GetStatusCodeName(status)); // MIFARE_Write() failed: 출력
         lcd.init();
         lcd.setCursor(0, 0);
         lcd.print("Error");
         return 0; // 끝
       }
       else Serial.println(F("MIFARE_Write() success: ")); // MIFARE_Write() success: 출력
       mfrc522.PCD_StopCrypto1(); //암호화 통신 종료
       motorTurnOn(degree,selected_item);
       selected_item = -1;
  }
  delay(1000); // 딜레이 1초
  return 1;
}

// 버튼 핸들링 -> 선택한 상품만 바꿈
int buttonHandle(int old_key, int new_key, int index){
  if (old_key != new_key) {
    old_key = new_key;
    if (new_key == LOW) {
      // 아이템 선택
      selected_item = index;
      resetLcd();
      Serial.print("selected item : ");
      Serial.print(item_list_name[index]);
      Serial.println(",");
      Serial.print("price : "); // , change_value : 출력
      Serial.print(item_list_price[selected_item]); // 변경되는 돈 출력
      Serial.println(",");
    }
  }
  return old_key;
}

// 모터 끄기
void motorTurnOff(int motorNum){
    digitalWrite(stepMotors[motorNum][0], LOW);
    digitalWrite(stepMotors[motorNum][1], LOW);
    digitalWrite(stepMotors[motorNum][2], LOW);
    digitalWrite(stepMotors[motorNum][3], LOW);
}

// 모터 키기
void motorTurnOn(int degree, int motorNum){
  for(int i=0;i<degree;i++){
    digitalWrite(stepMotors[motorNum][0], LOW);
    digitalWrite(stepMotors[motorNum][1], HIGH);
    digitalWrite(stepMotors[motorNum][2], HIGH);
    digitalWrite(stepMotors[motorNum][3], LOW);
    delay(speed);

    digitalWrite(stepMotors[motorNum][0], LOW);
    digitalWrite(stepMotors[motorNum][1], HIGH);
    digitalWrite(stepMotors[motorNum][2], LOW);
    digitalWrite(stepMotors[motorNum][3], HIGH);
    delay(speed);

    digitalWrite(stepMotors[motorNum][0], HIGH);
    digitalWrite(stepMotors[motorNum][1], LOW);
    digitalWrite(stepMotors[motorNum][2], LOW);
    digitalWrite(stepMotors[motorNum][3], HIGH);
    delay(speed);

    digitalWrite(stepMotors[motorNum][0], HIGH);
    digitalWrite(stepMotors[motorNum][1], LOW);
    digitalWrite(stepMotors[motorNum][2], HIGH);
    digitalWrite(stepMotors[motorNum][3], LOW);
    delay(speed);
  }
}

void resetLcd() {
  //reset lcd
  lcd.init();

  //turn on lcd backlight
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Hello");
}
