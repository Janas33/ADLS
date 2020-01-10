#pragma once

// TO USUŃ JAK NIE DZIAŁA
#define ARDUINO 101
#undef _WIN32

#include <ESP8266WiFi.h>       // Biblioteka do WiFI.
#include <ESP8266HTTPClient.h> // Biblioteka do WiFI.
#include <ESP8266WiFiMulti.h>  // Biblioteka do WiFI
#include <WiFiClient.h>        // Biblioteka do WiFI.
#include <PCF8574.h>           // Biblioteka do ekstendera wyprowadzeń PCF8574.
#include <Keypad.h>            // Biblioteka do obsługi klawiatury.
#include <LiquidCrystal_I2C.h> // Biblioteka do wyświetlacza tekstowego LCD z konwerterem I2C.
#include <Wire.h>              // Biblioteka do komunikacji przez I2C.
#include <Keypad_I2C.h>        // Biblioteka do podłączenia klawiatury przez magistrale.
#include <SPI.h>               // Biblioteka do komunikacji z jednym lub większą liczbą urządzeń peryferyjnych.
#include <MFRC522.h>           // Biblioteka do RFID.
#include <Gsender.h>           // Biblioteka do wysyłania maila.
#include <vector>
#include <ArduinoWebsockets.h> // Biblioteka do komunikacji z serwerem za pomocą websocketów.
#include <ArduinoJson.h>       // Biblioteka potrzebna do używania Jsona.