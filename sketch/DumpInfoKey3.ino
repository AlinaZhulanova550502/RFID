#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>        
#include <Wire.h>         // RTClib зависит от этого
#include "RTClib.h"
#include "pitches.h"      // для зуммера
#include <Keypad.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);	  // Create MFRC522 instance
Servo servo;
RTC_Millis rtc;                     // модуль часов

int ledPin = A4;                      // инициализируем пин для светодиода
int inputPin = A3;                    // инициализируем пин для получения сигнала от пироэлектрического датчика движения
int pirState = LOW;                   // начинаем работу программы, предполагая, что движения нет
int val = 0;                          // переменная для чтения состояния пина

const byte ledBut = A4;               // светодиод кнопки
const byte button = 2;                // кнопка

const byte ROWS = 4;                  // строки на клавиатуре
const byte COLS = 3;                  // столбцы на клавиатуре
                                      // определение символов на кнопках клавиатуры
char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {A5, 7, 6, 5};   //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, A1, A0};     //connect to the column pinouts of the keypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

boolean check();
void openDoor();
void reaction();
void printUID();
void print_time(DateTime);
void pir();
int* key(char);
void pushButton();                                      // обработка нажатия кнопки
int flag=0;
int flagUID=1;

void setup() {
  servo.attach(3);                                      // прикрепление сервопривода к пину
  pinMode(ledPin, OUTPUT);                              // объявляем светодиод в качестве OUTPUT для датчика движения
  pinMode(inputPin, INPUT);                             // объявляем датчик движения в качестве INPUT
	Serial.begin(9600);	                                  // запуск соединения
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));        // время

  pinMode (ledBut, OUTPUT);                             // устанавливаем пин в режим вывода (светодиод кнопки)
  digitalWrite (button, HIGH);                          // встроенный подтягивающий резистор
  attachInterrupt (0, pushButton, FALLING);              // подключаем обработчик прерывания для кнопки

	SPI.begin();			                                    // Init SPI bus
	mfrc522.PCD_Init();	                                  // Init MFRC522 card
	Serial.println("Scan PICC to see UID and type...");
}

void loop() 
{
  digitalWrite (ledBut, LOW);
  if (flag==1) {delay(5000); flag=0;}
  servo.write(30);                                //закрыто
  int* mycode;  int *code;                          // код двери и введенный код
  char customKey = customKeypad.getKey();           // проверка нажатия клавиши
  if (customKey)                                    // принять код 4 цифры с клавиатуры
  {
     mycode = (int)malloc(4*sizeof(int));           // настоящий код
     if (!mycode) Serial.println("error of memory!");
     *(mycode+0)=1;  *(mycode+1)=2;  *(mycode+2)=3;  *(mycode+3)=4; // настоящий код
    
    Serial.println("Input started");
    code=key(customKey);                                     //запись кода

    for (int i=0; i<4; i++)                         // проверка кода
      if (*(code+i)!=*(mycode+i)) return;           // неправильный код

    Serial.println("Sucsessfull!");                 // правильный код
    flagUID = 3;
    openDoor();                                     // открыть
  }

	if ( ! mfrc522.PICC_IsNewCardPresent()) {
    pir();
		return;
	}

	if ( ! mfrc522.PICC_ReadCardSerial()) {
    pir();
		return;
	}

  if (check()==true) openDoor();             // если карта знакомая, открыть
}

boolean check()         // проверка, знакомая ли карта
{
  byte uidCard[4] = {0x05, 0x7F, 0xAC, 0xA5};     // массив с юид "знакомой" карты
  
  for (byte i=0; i<mfrc522.uid.size; i++) 
    if (mfrc522.uid.uidByte[i] != uidCard[i]) return false;
  return true;
}

void openDoor(){
  servo.write(0);             //открыто
  noTone(8);                    //чтобы не было мелодии
  reaction();
  delay(5000);                          // время в открытом состоянии
}

void print_time(DateTime now)           // вывод времени входа
{
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void printUID()
{
   Serial.print("Card UID:");               // вывод идентификатора карты
  for (byte i = 0; i<mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void reaction()
{
  noInterrupts();
  Serial.println("Open");
  tone(8, NOTE_C4, 250);
  
  if (flagUID==1) printUID();
  else if (flagUID==2) {Serial.println("Open from button"); flagUID=1;}
  else if (flagUID==3) {Serial.println("Open from keyboard"); flagUID=1;}

  DateTime now = rtc.now();
  print_time(now);                    // вывод времени входа
  delay(4000);
  interrupts();
}

void pir(){
  val = digitalRead(inputPin);        // считываем значение с датчика
  if (val == HIGH) {                  // проверяем, соответствует ли считанное значение HIGH
    digitalWrite(ledPin, HIGH);       // включаем светодиод
    if (pirState == LOW) {            // при изменении состояния вывод объявления
      Serial.println("Motion detected!");
      pirState = HIGH;
      DateTime now = rtc.now();
      print_time(now);                    // вывод времени
    }
  } 
  else {
    digitalWrite(ledPin, LOW);        // выключаем светодиод
  if (pirState == HIGH){              // при изменении состояния вывод объявления
    Serial.println("Motion ended!");
    pirState = LOW;
    DateTime now = rtc.now();
    print_time(now);                    // вывод времени
    }
  }
}

int* key(char customKey)             // прием кода с клавиатуры
{
   int* code;
   code = (int)malloc(4*sizeof(int));
   if (!code) Serial.println("error!");
   *(code+0)=int(customKey-'0');
   Serial.print(customKey);
   int j=1;
   while(1)
   {
      char customKey = customKeypad.getKey();
      if (customKey) {
        Serial.print(customKey);
        *(code+j)=int(customKey-'0');   // преобразование из символа в число
        if (j==3) break;
        j++;
      }
   }
   Serial.println();
   return code;
}

void pushButton()                     // обработка прерывания от кнопки
{
  noInterrupts();
  digitalWrite (ledBut, HIGH);
  Serial.println("Open");
  tone(8, NOTE_C4, 250);

  DateTime now = rtc.now();
  print_time(now);                    // вывод времени входа
  
  flag=1;
  flagUID=2;
  servo.write(0);                    //открыто
 
  interrupts();
}


