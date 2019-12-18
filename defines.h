#pragma once

// Development configuration defines
#define DEBUG 1              // define do wyświetlania informacji pomocnych przy developowaniu.

// Standard defines
#define API_URL "http://adls.herokuapp.com/api/locks/570781de-9fce-4f0c-bbe6-681fcb8a15be/"

#define CZUJNIK_DRZWI_PIN   0       // Przypisanie CZUJNIKA do wejścia D3.

#define ZAMEK_PIN           0       // Pin na ekspanderze 1 na którym jest zamek
#define ZAMEK_CLOSED     HIGH
#define ZAMEK_OPEN       LOW

#define ALARM_PIN           2       // Pin na ekspanderze 1 na którym jest alarm
#define ALARM_ON      HIGH
#define ALARM_OFF     LOW

#define RFID_SS_PIN        15       // Przypisanie SS_PIN z RFID do wejścia D8.
#define RFID_RST_PIN        2       // Przypisanie RST_PIN z RFID do wejścia D4.

#define KEYPAD_I2C_ADDR  0x20       // Ustawienie adresu na magistrali na 0x21.
#define LIQUID_CRYSTAL_I2C_ADD  0X25            // SILNIK, BUZZER..
#define EXPANDER_1  0X26            // DIODY
#define EXPANDER_2 0X27            // LCD

#define CLOSE_LOCK_TIMEOUT "CLOSE_LOCK_TIMEOUT"
#define CLOSE_LOCK_TIMEOUT_TIME 3000

#define RESET_LOCK_TIMEOUT "RESET_LOCK_TIMEOUT"
#define RESET_LOCK_TIMEOUT_TIME 3000 

#define SET_ALARM_TIMEOUT "SET_ALARM_TIMEOUT"
#define SET_ALARM_TIMEOUT_TIME 6000

#define END_ALARM_TIMEOUT "END_ALARM_TIMEOUT"
#define END_ALARM_TIMEOUT_TIME 5000