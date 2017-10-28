#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Arduino.h>      // подключаем библиотеки для работы с RTC модулем
#include <Wire.h>         // RTClib зависит от этого
#include "RTClib.h"
#include "pitches.h"      // для зуммера

/*#if defined(ARDUINO_ARCH_SAMD)
   #define Serial SerialUSB          //для часов
#endif*/
#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);	  // Create MFRC522 instance
Servo servo;
RTC_Millis rtc;                     // модуль часов

//const uint8_t PIN_button  = 2;      // указываем номер вывода arduino, к которому подключена кнопка

int melody[] = {                      //ноты в мелодии
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4              //длительности нот
};

int ledPin = A4;  // инициализируем пин для светодиода
int inputPin = A3;  // инициализируем пин для получения сигнала от пироэлектрического датчика движения
int pirState = LOW;  // начинаем работу программы, предполагая, что движения нет
int val = 0;  // переменная для чтения состояния пина

void reaction();
void print_time(DateTime now);
void pir();

void setup() {
  servo.attach(3); // прикрепление серво к пину
  pinMode(ledPin, OUTPUT);  // объявляем светодиод в качестве  OUTPUT
  pinMode(inputPin, INPUT);  // объявляем датчик в качестве INPUT
	Serial.begin(9600);	// запуск соединения
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));  //время

	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();	// Init MFRC522 card
	Serial.println("Scan PICC to see UID and type...");
}

void loop() {
  //byte uidCard[4] = {0x93, 0x48, 0x67, 0x9A}; //массив с юид!!!

	// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
    pir();
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
    pir();
		return;
	}

  servo.write(180);             //открыто
  delay(2000);                  //выждать
  noTone(2);                    //чтобы не было мелодии
  reaction();
}

void print_time(DateTime now)
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

void reaction()
{
  for (int thisNote = 0; thisNote < 8; thisNote++) {   //проиграть мелодию
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(2, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
  }
  Serial.print("Card UID:");               //вывод идентификатора карты
  for (byte i = 0; i<mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  DateTime now = rtc.now();
  print_time(now);               //вывод времени входа

  servo.write(0);               //закрыто
  Serial.println("Open");
  delay(5000);                  //время пока открыто
}

void pir(){
  val = digitalRead(inputPin);  // считываем значение с датчика
  if (val == HIGH) {            // проверяем, соответствует ли считанное значение HIGH
    digitalWrite(ledPin, HIGH);  // включаем светодиод
    if (pirState == LOW) {      // мы только что включили
      Serial.println("Motion detected!");
      pirState = HIGH;
    }
  } 
  else {
    digitalWrite(ledPin, LOW); // выключаем светодиод
  if (pirState == HIGH){  // мы только что его выключили
    Serial.println("Motion ended!");
    pirState = LOW;
    }
  }
}
