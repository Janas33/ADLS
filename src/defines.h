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

#define KEYPAD_TRIES_NUMBER    2
#define KEYPAD_TRIES_DELAY   100

#define DISPLAY_LENGTH 16
#define DISPLAY_WIDTH  2

#define REMOVE_BUTTON_CHAR  'C'
#define STOP_ROLETA         '*'

#define LOCKED   1
#define UNLOCKED 0

#define KEYPAD_BEEP_TIME 20

#define EMPTY_LCD_LINE "                "

// Stany
#define ZAMEK_CLOSED    HIGH
#define ZAMEK_OPEN      LOW
#define ALARM_ON        HIGH
#define ALARM_OFF       LOW


// Wifi
// #define WIFI_SSID                 "Janas"                    // WIFI network name
// #define WIFI_PASSWORD             "wojciaszek3213"           // WIFI network password
#define WIFI_SSID                "Woz_obserwacyjny_342-R2"  // WIFI network name
#define WIFI_PASSWORD            "Wojteklubi07"             // WIFI network password
#define WIFI_RECONNECT_INTERVAL    10000                      // If not connected wait time to try again

// WebSockets
//#define WEBSOCKETS_URL "ws://adls.herokuapp.com/api/cable"
#define WEBSOCKETS_URL "ws://192.168.1.21:3000/api/cable"

// Piny
#define RFID_SS_PIN        15       // Przypisanie SS_PIN z RFID do wejścia D8.
#define RFID_RST_PIN        2       // Przypisanie RST_PIN z RFID do wejścia D4.  

#define ZAMEK_PIN           0       // Pin na ekspanderze_1 na którym jest zamek.
#define RGB_RED_PIN         1       // Pin na ekspanderze_1 na którym jest dioda.
#define RGB_BLUE_PIN        2       // Pin na ekspanderze_1 na którym jest dioda.
#define RGB_GREEN_PIN       4       // Pin na ekspanderze_1 na którym jest dioda.
#define PRZYCISK_PIN        5       // Pin na ekspanderze_1 na którym jest przycisk.
#define CZUJNIK_DRZWI_PIN   6       // Pin na ekspanderze_1 na którym jest czujnik. 

#define MOTOR_STEPS        32       // Ilość kroków silnika krokowego.
#define MOTOR_PIN_1         0       // Pin na ekspanderze_2 na którym jest silnik.
#define MOTOR_PIN_2         1       // Pin na ekspanderze_2 na którym jest silnik.
#define MOTOR_PIN_3         2       // Pin na ekspanderze_2 na którym jest silnik.
#define MOTOR_PIN_4         5       // Pin na ekspanderze_2 na którym jest silnik.
#define BUZZER_PIN          4       // Pin na ekspanderze_2 na którym jest buzzer.

//Kody sygnalizujące otwarcie drzwi
#define CLOSED              0
#define OPEN_RFID          10
#define OPEN_RFID_2        11
#define OPEN_RFID_NEW      12
#define OPEN_CODE          20
#define OPEN_BUTTON        30

// Adresy
#define KEYPAD_I2C_ADDR         0x20       // Adres KEYPAD
#define LIQUID_CRYSTAL_I2C_ADDR 0X27       // Adres LCD
#define EXPANDER_1_ADDR         0X26       // Adres EXPANDDER_1 (Diody,Zamek,Przycisk)
#define EXPANDER_2_ADDR         0X25       // Adres EXPANDDER_2 (Buzzer,Silnik)

// Timeouty
#define CLOSE_LOCK_TIMEOUT "CLOSE_LOCK_TIMEOUT"
#define CLOSE_LOCK_TIMEOUT_TIME 3000

#define RESET_DISPLAY_TIMEOUT "RESET_DISPLAY_TIMEOUT"
#define RESET_DISPLAY_TIMEOUT_TIME 3000 

#define SET_ALARM_TIMEOUT "SET_ALARM_TIMEOUT"
#define SET_ALARM_TIMEOUT_TIME 6000

#define END_ALARM_TIMEOUT "END_ALARM_TIMEOUT"
#define END_ALARM_TIMEOUT_TIME 5000