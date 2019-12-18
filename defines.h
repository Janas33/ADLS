#pragma once

// Development configuration defines
#define DEBUG 1              // define do wyświetlania informacji pomocnych przy developowaniu.

#if DEBUG == 1
  #define D(text) Serial.println(text);  
#else
  #define D(text)  
#endif

// Standard defines
#define API_URL "http://adls.herokuapp.com/api/locks/570781de-9fce-4f0c-bbe6-681fcb8a15be/"

#define KEYPAD_TRIES_NUMBER 100

#define DISPLAY_LENGTH 16
#define DISPLAY_WIDTH  2

#define REMOVE_BUTTON_CHAR 'C'

#define KEYPAD_BEEP_TIME 50

// Stany
#define ZAMEK_CLOSED     HIGH
#define ZAMEK_OPEN       LOW
#define ALARM_ON      HIGH
#define ALARM_OFF     LOW




// Piny
#define CZUJNIK_DRZWI_PIN   0       // Przypisanie CZUJNIKA do wejścia D3.

#define ZAMEK_PIN           0       // Pin na ekspanderze 1 na którym jest zamek

#define ALARM_PIN           2       // Pin na ekspanderze 1 na którym jest alarm

#define RFID_SS_PIN        15       // Przypisanie SS_PIN z RFID do wejścia D8.
#define RFID_RST_PIN        2       // Przypisanie RST_PIN z RFID do wejścia D4.

#define RGB_RED_PIN    1
#define RGB_BLUE_PIN   2
#define RGB_GREEN_PIN  3

#define BUZZER_PIN 4



// Addressy
#define KEYPAD_I2C_ADDR         0x20       // KEYPAD
#define LIQUID_CRYSTAL_I2C_ADDR 0X25       // LCD
#define EXPANDER_1_ADDR         0X26       // DIODY
#define EXPANDER_2_ADDR         0X27       // SILNIK, BUZZER..


// Timeouty
#define CLOSE_LOCK_TIMEOUT "CLOSE_LOCK_TIMEOUT"
#define CLOSE_LOCK_TIMEOUT_TIME 3000

#define RESET_DISPLAY_TIMEOUT "RESET_DISPLAY_TIMEOUT"
#define RESET_DISPLAY_TIMEOUT_TIME 3000 

#define SET_ALARM_TIMEOUT "SET_ALARM_TIMEOUT"
#define SET_ALARM_TIMEOUT_TIME 6000

#define END_ALARM_TIMEOUT "END_ALARM_TIMEOUT"
#define END_ALARM_TIMEOUT_TIME 5000