// TO USUŃ JAK NIE DZIAŁA
#define ARDUINO 101

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

#include "defines.h"
#include "headers.h"

// Timeouts code
struct TimeoutData {
	TimeoutData(void f(), const int &t, const char *n) : timeout(t), f(f), name(n) {}
	void(*f)();
	int timeout;
  const char *name;
};

std::vector<struct TimeoutData> timeout_registry;

void set_timeout(void f(), const int &time, const char *name) {
  //Serial.println("set_timeout: " + String(name)); 
	timeout_registry.push_back(TimeoutData(f, time, name));
}

void handle_timeouts() {
  TimeoutData *timeoutData;

  //Serial.println("handle_timeouts in  timeoutów łącznie:" + timeout_registry.size()  );
  for (int i = 0; i < timeout_registry.size(); i++) {
    timeoutData = &timeout_registry[i];

    Serial.println("handle_timeouts pętla: " + String(timeoutData->name) + ", milis: " +  String(timeoutData->timeout) + ", curr: " + String(millis()) );
    if (timeoutData->timeout < millis()) {
			timeoutData->f();
			timeout_registry.erase(timeout_registry.begin() + i);
		}
  }
  //Serial.println("handle_timeouts out timeoutów łącznie: " + String(timeout_registry.size()) );
}

void remove_timeout(const char *name) {
  for (int i = 0; i < timeout_registry.size(); i++) {
    if (strcmp(timeout_registry[i].name, name))
			timeout_registry.erase(timeout_registry.begin() + i);
  }
}

TimeoutData* find_timeout(const char *name) { 
  Serial.println("find_timeout for: " + String(name) );
  for (int i = 0; i < timeout_registry.size(); i++) {
    Serial.println("find_timeout got: " + String(timeout_registry[i].name) + " vs " + String(name)  );
    if (strcmp(timeout_registry[i].name, name) == 0)
      return &timeout_registry[i];
  }

  return NULL;
}

bool LockCloseRequested = false;
bool LockCloseCompleted = false;

bool DisplayClearRequested = false;

bool AlarmRaiseRequested = false;
bool AlarmRunning = false;


// Global variables
#pragma region Globals

// Stan drzwi
LockState lockState = LockState::Closed;

/*
 *   Inicjalizacja ekspander_1, na adresie EXPANDER_1
 * Obsługuje:
 * Lock (zamek)
 * Diodę RGB
*/
PCF8574 expander_1(EXPANDER_1_ADDR);
PCF8574 expander_2(EXPANDER_2_ADDR);

/*
*   Inicjalizacja wyświetlacza lcd, na adresie LIQUID_CRYSTAL_I2C_ADD
*   z DISPLAY_LENGTH znakami na 2 rzędach
*/
LiquidCrystal_I2C lcd(LIQUID_CRYSTAL_I2C_ADDR, DISPLAY_LENGTH, DISPLAY_WIDTH);


// Wifi
const char *ssid = "Wojciaszek";         // WIFI network name
const char *password = "wojciaszek3213"; // WIFI network password
uint8_t connection_state = 0;            // Connected to WIFI or not
uint16_t reconnect_interval = 10000;     // If not connected wait time to try again

ESP8266WiFiMulti WiFiMulti;

// Rfid
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN); // Create MFRC522 instance.

// Keypad
const byte ROWS = 4;      // Ilość rzędów
const byte COLS = 4;      // Ilosć kolumn
// Układ klawiatury
char keys[ROWS][COLS] = { 
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };

// Numery pinów na ekpanderze do którego podłączamy keypad
byte rowPins[ROWS] = {0, 1, 2, 3}; // numery dla rzędów
byte colPins[COLS] = {4, 5, 6, 7}; // numeryu dla kolumn

/**
 *   Inicjalizacja keypada, na podstawie podanych zmiennych
 */
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, KEYPAD_I2C_ADDR);

#pragma endregion Globals

// Code variables
int code_index = 0;
char code_current[4];
char code_open[] = "1234";

void setup()
{
  // Inicjalizacja biblioteki do komunikacji po protokołach
  Wire.begin();

  // Lcd
  lcd.begin(16, 2);              // .
  lcd.backlight();               // . Załączenie podświetlenia.
  lcd.setCursor(0, 0);           // .
  lcd.print("      ADLS      ");        // .
  lcd.setCursor(0, 1);           // .
  lcd.print("    --------    "); // .

  pinMode(CZUJNIK_DRZWI_PIN, INPUT_PULLUP); // Deklaracja czujnika jako wejścia, i wewnętrzne podciągnięcie go

  expander_1.pinMode(ZAMEK_PIN, OUTPUT);      // Deklaracja zamka przez ekspander jako wyjścia.
  expander_1.pinMode(RGB_RED_PIN, OUTPUT);
  expander_1.pinMode(RGB_BLUE_PIN, OUTPUT);
  expander_1.pinMode(RGB_GREEN_PIN, OUTPUT);
  expander_1.pinMode(ALARM_PIN, OUTPUT);      // Deklaracja alarmu przez ekspander jako wyjścia.
  expander_2.pinMode(BUZZER_PIN, OUTPUT);
  expander_1.pinMode(PRZYCISK_PIN, INPUT);

   // Inicjalizacja keypada, z wcześniej ustawionym mapowaniem klawiszy
  keypad.begin(makeKeymap(keys));

  // Inicjalizacja komunikacji po serial
  Serial.begin(9600);
  //Serial.println("start");

  SPI.begin();        // Initiate  SPI bus
  mfrc522.PCD_Init(); // Initiate MFRC522

  // Inicjalizacja wifi z trybem WIFI_STA (klient (?))
  WiFi.mode(WIFI_STA);
  // Określenie nazwy i hasła do sieci, z którą mamy się połączyć.
  WiFiMulti.addAP("Wojciaszek", "wojciaszek3213"); 

  change_rgb_color(RgbColor::Blue);
}

/**
 *  Funkcja służąca do połączenia się z wifi
 */
uint8_t WiFiConnect(const char *nSSID = nullptr, const char *nPassword = nullptr)
{
  static uint16_t attempt = 0;
  Serial.print("Connecting to ");
  if (nSSID)
  {
    WiFi.begin(nSSID, nPassword);
    //Serial.println(nSSID);
  }
  else
  {
    WiFi.begin(ssid, password);
    //Serial.println(ssid);
  }

  uint8_t i = 0;

  while (WiFi.status() != WL_CONNECTED && i++ < 50)
  {
    delay(200);
    Serial.print(".");
  }

  ++attempt;
  //Serial.println("");

  if (i == 51)
  {
    Serial.print("Connection: TIMEOUT on attempt: ");
    //Serial.println(attempt);
    if (attempt % 2 == 0)
      //Serial.println("Check if access point available or SSID and Password\r\n");
    return false;
  }

  //Serial.println("Connection: ESTABLISHED");
  Serial.print("Got IP address: ");
  //Serial.println(WiFi.localIP());
  return true;
}

/**
 * BLOKUJĄCA!!!
 * Funkcja próbująca się połączyć z wifi jeszcze raz
 */ 
void retryWifiConnection()
{
  uint32_t ts = millis();
  while (!connection_state)
  {
    delay(50);
    if (millis() > (ts + reconnect_interval) && !connection_state)
    {
      connection_state = WiFiConnect();
      ts = millis();
    }
  }
}

/**
 * PSEUDOBLOKUJĄCA (aż do uzyskania odpowiedzi od serwera)
 * Funkcja wysyłająca POST request pod adres serwera + command
 * 
 */ 
void sendPostToServer(String command) // Komuniacja z serverem przez WiFi.
{                                 //$todo dodać wysyłanie stanu drzwi i stanu czujników
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, API_URL + command))
    { // HTTP

      Serial.print("[HTTP] POST...\n");
      // start connection and send HTTP header
      int httpCode = http.POST("");

      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String payload = http.getString();
          //Serial.println(payload);
        }
      }
      else
      {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    }
    else
    {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}

/**
 * Funkcja wyświetlająca dwa napisy na wyświetlaczu
 * odpowiednio górny i dolny rząd
 * @napis_top char[16] napis na górę
 * @napis_bot char[16] napis na dół
 */ 
void display_on_lcd(char napis_top[16], char napis_bottom[16])
{
    // Jeżeli mamy timeout na reset stanu
    if (DisplayClearRequested) {
      remove_timeout(RESET_DISPLAY_TIMEOUT);
      DisplayClearRequested = false;
      change_rgb_color(RgbColor::Blue);
    }

    lcd.setCursor(0, 0);
    lcd.print(napis_top);
    lcd.setCursor(0, 1);
    lcd.print(napis_bottom);
}

/**
 * Funkcja zmieniająca kolor diody
 * @color: RgbColor docelowy kolor diody
 */ 
void change_rgb_color(const RgbColor &color)
{
  // HIGH is off
  // LOW  is ON
  int red = LOW,
      blue = LOW,
      green = LOW;

  switch(color){
    case RgbColor::Blue:
      blue = HIGH;
      break;
    case RgbColor::Red:
      red = HIGH;
      break;
    case RgbColor::Green:
      green = HIGH;
      break;
    case RgbColor::Off:
      break;
    default:
      break;
  }

  expander_1.digitalWrite(RGB_GREEN_PIN, green);
  expander_1.digitalWrite(RGB_RED_PIN, red);
  expander_1.digitalWrite(RGB_BLUE_PIN, blue);
}

/**
 * Funkcja zmieniająca stan drzwi
 * @stan LockState docelowy stan drzwi
 */ 
void change_and_display_lock_state(LockState stan)
{
  switch (stan)
  {
  case LockState::Open:
    display_on_lcd("Drzwi otwarte   ", "****************");
    expander_1.digitalWrite(ZAMEK_PIN, ZAMEK_OPEN);
    change_rgb_color(RgbColor::Green);
    lockState = LockState::Open;
    break;

  case LockState::Closed:
    display_on_lcd("Drzwi zamkniete ", "****************");
    expander_1.digitalWrite(ZAMEK_PIN, ZAMEK_CLOSED);
    change_rgb_color(RgbColor::Blue);
    lockState = LockState::Closed;
    break;

  case LockState::Blocked:
    display_on_lcd("Blokada dostepu ", "****************");
    expander_1.digitalWrite(ZAMEK_PIN, ZAMEK_CLOSED);
    change_rgb_color(RgbColor::Red);
    lockState = LockState::Blocked;
    DisplayClearRequested = true;
    set_timeout(delayed_reset_display, millis() + RESET_DISPLAY_TIMEOUT_TIME, RESET_DISPLAY_TIMEOUT);
    break;
  
  default:
    // Nigdy nie powinno się tu dostać
    break;
  }
}

void delayed_reset_display() {
  DisplayClearRequested = false;
  reset_display();
}

void reset_display() {
  change_and_display_lock_state(LockState::Closed);
}

void handleRfid() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();

  if (content.substring(1) == "7B 71 17 21") handleLockOpen();
  else if (content.substring(1) == "5B 05 49 0A") handleLockOpen();
  else if (content.substring(1) == "43 D3 44 7A") display_on_lcd("Oddawaj zarowke  ", "****************"); // Przypadek Wojciaszek
  else {} //change_and_display_lock_state(LockState::Closed);
}

void handleButton()
{
  if(expander_1.digitalRead(PRZYCISK_PIN) == HIGH) handleLockOpen() ;
}

void handleKeyboard() {
  char key = keypad.getKey();
  if(key) {
    handleKeyPress(key);
    D(key);
  }
}


// Blokująca na chwilkę (KEYPAD_BEEP_TIME)
void handleKeyPress(const char &key) {
  expander_2.digitalWrite(BUZZER_PIN, ALARM_ON);
  delay(KEYPAD_BEEP_TIME);
  expander_2.digitalWrite(BUZZER_PIN, ALARM_OFF);

  if (key == REMOVE_BUTTON_CHAR) {
    D("Remove key pressed");
    // Kod nie jest wprowadzany
    if (code_index == 0) return;

    code_index--;

    if (code_index == 0) {
      // Usunięto cały kod
      reset_display();
    } else {
      // Usunięto jeden znak
      displayCode(code_index);
    }

    return;
  }

  D("code_index");
  D(code_index);
  code_current[code_index] = key;
  if (code_index == 3) {
    D("Zakończono wprowadzać kod");
    // Zakończono wprowadzać kod
    if (!(strncmp(code_current, code_open, 4))) {
      D("Otwarcie");
      handleLockOpen();
    } else {
      D("Blokada");
      change_and_display_lock_state(LockState::Blocked);
    }

    clearCode();
  } else {
    D("W trakcie wprowadzania kodu");
    // W trakcie wprowadzania kodu
    code_index++;
    displayCode(code_index);
  }
}

void clearCode() {
  code_index = 0;
}

void displayCode(const int &code_length) {
  String code_text = "Kod: ";
  char buf[DISPLAY_LENGTH];

  for (int i = 0; i < code_length; i++ ){ 
    code_text += '*';
  }

  for (int i = code_text.length(); i < DISPLAY_LENGTH; i++ ){ 
    code_text += ' ';
  }

  code_text.toCharArray(buf, DISPLAY_LENGTH);

  display_on_lcd(buf, "                ");
}

void handleLockOpen() {
  if (LockCloseRequested && LockCloseCompleted) {
    LockCloseCompleted = false;
    LockCloseRequested = false;
  }

  if (LockCloseRequested) {
    TimeoutData *timeoutData;

    timeoutData = find_timeout(CLOSE_LOCK_TIMEOUT);
    if(timeoutData != NULL) {
      timeoutData->timeout = millis() + CLOSE_LOCK_TIMEOUT_TIME;
      return;
    }
  }

  openLock();
}

void openLock() {
  change_and_display_lock_state(LockState::Open);

  LockCloseRequested = true;
  
  set_timeout(closeLock, millis() + CLOSE_LOCK_TIMEOUT_TIME, CLOSE_LOCK_TIMEOUT);
}

void closeLock() {
  change_and_display_lock_state(LockState::Closed);
  LockCloseCompleted = true;
}

void handleSettingAlarm() {
  if (AlarmRaiseRequested) refreshAlarm();

  AlarmRaiseRequested = true;
  set_timeout(raiseAlarm, millis() + SET_ALARM_TIMEOUT_TIME, SET_ALARM_TIMEOUT);
}

void refreshAlarm() {
  TimeoutData *timeoutData;

  timeoutData = find_timeout(SET_ALARM_TIMEOUT);
  timeoutData->timeout = millis() + SET_ALARM_TIMEOUT_TIME;
}

void handleCancelingAlarm() {
  if (!AlarmRaiseRequested) return;

  remove_timeout(SET_ALARM_TIMEOUT);
  AlarmRaiseRequested = false;
}

void raiseAlarm() {
  AlarmRaiseRequested = false;
  AlarmRunning = true;

  expander_1.digitalWrite(ALARM_PIN, ALARM_ON);
  set_timeout(endAlarm, millis() + END_ALARM_TIMEOUT_TIME, END_ALARM_TIMEOUT);
}

void endAlarm() {
  AlarmRunning = false;

  expander_1.digitalWrite(ALARM_PIN, ALARM_OFF);
}

void handleSensors() {
  if (digitalRead(CZUJNIK_DRZWI_PIN)) {
    // Otwarte
    if (lockState == LockState::Open) handleSettingAlarm();
  }
  else {
    if (lockState == LockState::Closed) handleCancelingAlarm();
  }
}

void loop()
{
    //D("timeouts");
  handle_timeouts();
    //D("rfid");
  handleRfid();
    //D("sensors");
  //handleSensors();
  
    //D("keyboard");
  for (int i = 0; i < KEYPAD_TRIES_NUMBER; i++) {
    delay(KEYPAD_TRIES_DELAY);
    handleKeyboard();
    handleButton();
  }
}