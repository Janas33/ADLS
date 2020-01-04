#include "src/includes.h"
#include "src/defines.h"
#include "src/headers.h"
#include "src/stepper.h"
#include "src/timeouts.hpp"


using namespace websockets;

  bool LockCloseRequested = false;
  bool LockCloseCompleted = false;

  bool DisplayClearRequested = false;

  bool AlarmRaiseRequested = false;
  bool AlarmRunning = false;
//
// Global variables
#pragma region Globals

// Stan drzwi
LockState lockState = LockState::Closed;
/*
 *   Inicjalizacja ekspander_1, na adresie EXPANDER_1_ADDR
 * Obsługuje:
 * Lock (zamek)
 * Diodę RGB
 */
PCF8574 expander_1(EXPANDER_1_ADDR);
/*
 *   Inicjalizacja ekspander_2, na adresie EXPANDER_2_ADDR
 * Obsługuje:
 * Buzzer (alarm)
 * Silnik
 */
PCF8574 expander_2(EXPANDER_2_ADDR);

Stepper motor(MOTOR_STEPS, MOTOR_PIN_4, MOTOR_PIN_2, MOTOR_PIN_3, MOTOR_PIN_1, &expander_2);
/*
 *   Inicjalizacja wyświetlacza lcd, na adresie LIQUID_CRYSTAL_I2C_ADD
 *   z DISPLAY_LENGTH znakami na 2 rzędach
 */
LiquidCrystal_I2C lcd(LIQUID_CRYSTAL_I2C_ADDR, DISPLAY_LENGTH, DISPLAY_WIDTH);

// Wifi
const char *ssid = WIFI_SSID;         // WIFI network name
const char *password = WIFI_PASSWORD; // WIFI network password
uint8_t connection_state = 0;            // Connected to WIFI or not
uint16_t reconnect_interval = WIFI_RECONNECT_INTERVAL;     // If not connected wait time to try again

// Websockets
const char* websockets_server = WEBSOCKETS_URL; //server adress and port

WebsocketsClient ws_client;

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

String convertToString(char* a, int size) { 
 String s(a); 
 return s; 
} 

// Code variables
int code_index = 0;
char code_current[4];
char code_open[] = "1234";
char code_roleta_lock[] ="7809";
char code_roleta_unlock[] ="AB63";

int ROLETA = UNLOCKED ; 
int LOCK = CLOSED;
#pragma endregion Globals

void setup()
{
  // Inicjalizacja biblioteki do komunikacji po protokołach
  Wire.begin();
  
  expander_1.pinMode(ZAMEK_PIN, OUTPUT);      // Deklaracja zamka przez ekspander jako wyjścia.
  expander_1.pinMode(RGB_RED_PIN, OUTPUT);
  expander_1.pinMode(RGB_BLUE_PIN, OUTPUT);
  expander_1.pinMode(RGB_GREEN_PIN, OUTPUT);
  expander_1.pinMode(PRZYCISK_PIN, INPUT);
  expander_1.pinMode(CZUJNIK_DRZWI_PIN,INPUT);
  expander_1.digitalWrite(ZAMEK_PIN, ZAMEK_CLOSED);

  expander_2.pinMode(BUZZER_PIN, OUTPUT);
  expander_2.pinMode(MOTOR_PIN_1, OUTPUT);
  expander_2.pinMode(MOTOR_PIN_2, OUTPUT);
  expander_2.pinMode(MOTOR_PIN_3, OUTPUT);
  expander_2.pinMode(MOTOR_PIN_4, OUTPUT); 
  expander_2.digitalWrite(BUZZER_PIN, ALARM_OFF);
  motor.setSpeed(500);

  change_rgb_color(RgbColor::Cyan);
  
  // Lcd
  lcd.begin(16, 2);              // .
  lcd.backlight();               // . Załączenie podświetlenia.
  lcd.setCursor(0, 0);           // .
  lcd.print("      ADLS      ");        // .
  lcd.setCursor(0, 1);           // .
  lcd.print("    --------    "); // .

   // Inicjalizacja keypada, z wcześniej ustawionym mapowaniem klawiszy
  keypad.begin(makeKeymap(keys));

  // Inicjalizacja komunikacji po serial
  Serial.begin(9600);
  Serial.println("start");

  SPI.begin();        // Initiate  SPI bus
  mfrc522.PCD_Init(); // Initiate MFRC522

  // Wifi
  WiFi.begin(ssid, password);

  // Wait some time to connect to wifi
  for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
      Serial.print(".");
      delay(1000);
  }

  // Setup Callbacks
  ws_client.onMessage(onMessageCallback);
  ws_client.onEvent(onEventsCallback);
  
  // Connect to server
  bool connected = ws_client.connect(websockets_server);
  D("Connected to ws: \/");
  D(connected);

  // Connect to channel
  ws_client.send("{\"command\":\"subscribe\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\"}");
  // Send a ping
  ws_client.ping();

  change_rgb_color(RgbColor::Blue);
}

#pragma region StareWifi
/**
 *  Funkcja służąca do połączenia się z wifi
 */
/*
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
*/

/**
 * BLOKUJĄCA!!!
 * Funkcja próbująca się połączyć z wifi jeszcze raz
 */ 
/*
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
*/

/**
 * PSEUDOBLOKUJĄCA (aż do uzyskania odpowiedzi od serwera)
 * Funkcja wysyłająca POST request pod adres serwera + command
 * 
 */ 
/*
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
*/
#pragma endregion StareWifi

/**
 * Funkcja wysyłająca powiadomiana na maila.
 * Jako argument przyjmuje 
 * @adress jako adres mail, pod który ma wysłać maila
 * @message jako wiadomość, którą wysyła
 * def - domyślna wartość dla obu argumentów
 */
void send_mail(char* adress, char* message){
Gsender *gsender = Gsender::Instance(); // Getting pointer to class instance
  String subject = "ADLS Alarm";
   int a_size = sizeof(adress) / sizeof(char); 
   int m_size = sizeof(message) / sizeof(char); 
  
  const String string_adress = convertToString(adress, a_size); 
  const String string_message = convertToString(message, m_size); 
  
  if(adress == "def" && message == "def"){
   if (gsender->Subject(subject)->Send ("szymonruta.sr@gmail.com", "Door are open for a long period of time. Possible security breach")) {
      Serial.println("Wysłano wiadomość domyślną.");
   }
  }

  if (gsender->Subject(subject)->Send( string_adress,string_message ))
  {
    Serial.println("Message send.");
  }
  else
  {
    Serial.print("Error sending message: ");
    Serial.println(gsender->getError());
  }
} 
/**
 * Funckcje do WebSocket
 */
void onMessageCallback(WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}
void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}
/**
 * Funkcja wyświetlająca dwa napisy na wyświetlaczu
 * odpowiednio górny i dolny rząd
 * @napis_top char[16] napis na górę
 * @napis_bot char[16] napis na dół
 */ 
void display_on_lcd(char napis_top[16], char napis_bottom[16]) {
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
void change_rgb_color(const RgbColor &color) {
  // HIGH is ON
  // LOW  is OFF
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
    case RgbColor::Cyan:
      green = HIGH;
      blue = HIGH;
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
void change_and_display_lock_state(LockState stan) {
  switch (stan)
  {
  case LockState::Open:
    display_on_lcd("Zamek otwarty   ", "****************");
    expander_1.digitalWrite(ZAMEK_PIN, ZAMEK_OPEN);
    change_rgb_color(RgbColor::Green);
    lockState = LockState::Open;
    break;

  case LockState::Closed:
    display_on_lcd("Zamek zamkniety ", "****************");
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
/**
 * Funckja wywoływana przed resetem wyświatlacza.
 */ 
void delayed_reset_display() {
  DisplayClearRequested = false;
  reset_display();
}
/**
 * Funkcja resetująca wyświetlacz.
 */ 
void reset_display() {
  change_and_display_lock_state(LockState::Closed);
}
/**
 * Funkcja zerująca zmienną wysyłaną jako sposób otwarcia do strony internetowej.
 */ 
void reset_lock() {
  LOCK = CLOSED;
}
/**
 * Funkcja odpowiedzialna za obsługę RFID oraz poszczególnych UID kart.
 */ 
void handleRfid() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();

  if (content.substring(1) == "7B 71 17 21") 
  { 
    LOCK = OPEN_RFID; 
    handleLockOpen();
  }
  else if (content.substring(1) == "5B 05 49 0A")
  {  
    LOCK = OPEN_RFID_2;     
    handleLockOpen();  
  }
  else if (content.substring(1) == "43 D3 44 7A") display_on_lcd("Oddawaj zarowke  ", "****************"); // Przypadek Wojciaszek
  else {} //change_and_display_lock_state(LockState::Closed);
}
/**
 * Funckja odpowiedzialna za przycisk.
 */ 
void handleButton() {
  if(expander_1.digitalRead(PRZYCISK_PIN) == HIGH) {
    LOCK = OPEN_BUTTON;
    handleLockOpen();
  }
}
/**
 * Funckja, która odczytuje nacisnięty przycisk na klawiaturze.
 */ 
void handleKeyboard() {
  char key = keypad.getKey();
  if(key) {
    handleKeyPress(key);
    D(key);
  }
}
/**
 * Funckja rozpoznająca funkcje przycisków oraz kody dostępu.
 */ 
void handleKeyPress(const char &key) { 
  expander_2.digitalWrite(BUZZER_PIN, ALARM_ON);
  delay(KEYPAD_BEEP_TIME);                        // Blokująca na chwilkę (KEYPAD_BEEP_TIME)
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
    if (!(strncmp(code_current, code_roleta_lock, 4))) {
      if(ROLETA == UNLOCKED && (expander_1.digitalRead(CZUJNIK_DRZWI_PIN) == LOW) ) {
      display_on_lcd("Drzwi zostaja  ","zablokowane.      ");
      handleMotor(1);
      ROLETA = LOCKED;
      }
    }
    if (!(strncmp(code_current, code_roleta_unlock, 4))) {
      if(ROLETA == LOCKED ) {
      display_on_lcd("Drzwi zostaja  ","odblokowane.      ");
      handleMotor(-1);
      ROLETA = UNLOCKED;
      }
    }
    if (!(strncmp(code_current, code_open, 4))) {
      if(ROLETA == UNLOCKED )
      {
        D("Otwarcie");
        LOCK = OPEN_CODE;
        handleLockOpen();
      }
      else {display_on_lcd("Roleta uzyta","Odmowa dostepu");
      closeLock();
      }
    } 
    else {
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
/**
 * Funkcja zerująca odczytywany kod dostępu z klawiatury.
 */ 
void clearCode() {
  code_index = 0;
}
/**
 * Funckja wyświetlająca na ekranie LCD postęp wpisywania kodów dostępu.
 */ 
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

  display_on_lcd(buf, EMPTY_LCD_LINE);
}
/**
 * Funkcja odpowiedzialna za procedurę otwarcia drzwi.
 */ 
void handleLockOpen() {
   if(ROLETA == UNLOCKED) { 
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

    D("Sending opening of lock state");
    ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"action\\\":\\\"opened\\\"}\"}");
    D("Done sending opening of lock state");
    // TO_DO
    // Wysyłanie sposobu otwarcia drzwi 
    D("Sending way of opening the lock");
    signal_door_opening(LOCK);
    D("Done sending way of opening the lock");
    openLock();
   }
   else {D("Drzwi zablokowane , kod nie działa");}  
}
/**
 * Funckja odpowiedzialna za wysyłanie komunikatu o sposobie otwarcia drzwi.
 */ 
void signal_door_opening(int HOW) {
  if(HOW == OPEN_RFID){
    ws_client.send("{\"event\":{\"message\":\"Door were opened by RFID_1\",\"level\":\"alarm\",\"lock_token\":\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\"}}");
  }
  if(HOW == OPEN_RFID_2){
    ws_client.send("{\"event\":{\"message\":\"Door were opened by RFID_2\",\"level\":\"alarm\",\"lock_token\":\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\"}}");
  }
  if(HOW == OPEN_CODE){
    ws_client.send("{\"event\":{\"message\":\"Door were opened by CODE\",\"level\":\"alarm\",\"lock_token\":\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\"}}");
  }
  if(HOW == OPEN_BUTTON){
    ws_client.send("{\"event\":{\"message\":\"Door were opened by BUTTON\",\"level\":\"alarm\",\"lock_token\":\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\"}}");
  }
}
/**
 * Funckja otwierająca zamek.
 */ 
void openLock() {
  change_and_display_lock_state(LockState::Open);

  LockCloseRequested = true;
  
  set_timeout(closeLock, millis() + CLOSE_LOCK_TIMEOUT_TIME, CLOSE_LOCK_TIMEOUT);
}
/**
 * Funkcja zamykająca zamek.
 */ 
void closeLock() {
  D("Sending closing of lock state");
  ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"action\\\":\\\"closed\\\"}\"}");
  D("Done sending closing of lock state");
  change_and_display_lock_state(LockState::Closed);
  LockCloseCompleted = true;
  reset_lock();
}
/**
 * Funkcja rozpoczynająca procedurę alarmową.
 */ 
void handleSettingAlarm() {
  if (AlarmRaiseRequested) refreshAlarm();
  D("handleSettingAlarm");
  AlarmRaiseRequested = true;
  set_timeout(raiseAlarm, millis() + SET_ALARM_TIMEOUT_TIME, SET_ALARM_TIMEOUT);
}
/**
 * Funkcja ustawiająca timeouty alarmu.
 */ 
void refreshAlarm() {
  TimeoutData *timeoutData;
  D("refreshAlarm");
  timeoutData = find_timeout(SET_ALARM_TIMEOUT);
  timeoutData->timeout = millis() + SET_ALARM_TIMEOUT_TIME;
}
/**
 * Funkcja zdejmująca timeouty, gdy alarm ma nie zostać wywołany.
 */ 
void handleCancelingAlarm() {
  if (!AlarmRaiseRequested) return;
  D("handleCancelingAlarm");
  remove_timeout(SET_ALARM_TIMEOUT);
  AlarmRaiseRequested = false;
}
/**
 * Funckja włączająca buzzer.
 */ 
void raiseAlarm() {
  D("raiseAlarm");
  AlarmRaiseRequested = false;
  AlarmRunning = true;

  expander_2.digitalWrite(BUZZER_PIN, ALARM_ON);
  set_timeout(endAlarm, millis() + END_ALARM_TIMEOUT_TIME, END_ALARM_TIMEOUT);
}
/**
 * Funkcja wyłączająca buzzer.
 */ 
void endAlarm() {
  AlarmRunning = false;

  expander_2.digitalWrite(BUZZER_PIN, ALARM_OFF);
}
/**
 * Funkcja odpowiedzialna za informowanie o stanie drzwi i rozpoczynająca procedury alarmowe.
 */ 
void handleSensors() {
  if (expander_1.digitalRead(CZUJNIK_DRZWI_PIN) == LOW) {
    // Otwarte
    if (lockState == LockState::Open) {
      lcd.setCursor(0, 1);
      lcd.print("Drzwi otwarte");
      handleSettingAlarm();
    }
  }
  else {
    if (lockState == LockState::Closed){
      lcd.setCursor(0, 1);
      lcd.print("Drzwi zamkniete");
      handleCancelingAlarm();
    }
  }
}
/**
 * Funkcja odpowiedzialna za sterowanie silnikiem
 * @int dir odpowiada za kierunek 
 * gdy +1 to rozwija rolete
 * gdy -1 to zwija rolete
 */ 
void handleMotor(int dir){ 
  int  i = 0, T = 2;
  for(i = 0; i<225; i++)
  {
    motor.step(dir*100);
    yield();
  }
  expander_2.digitalWrite(MOTOR_PIN_4,LOW);   // Zerowanie silnika po wszystkich obrotach, żeby się nie grzał.
  expander_2.digitalWrite(MOTOR_PIN_2,LOW);
  expander_2.digitalWrite(MOTOR_PIN_3,LOW);
  expander_2.digitalWrite(MOTOR_PIN_1,LOW);
}

void loop()
{
  if (ws_client.available()) ws_client.poll();
   // D("timeouts");
   handle_timeouts();
    //D("rfid");
   handleRfid();
    //D("sensors");
  // handleSensors();
  
  for (int i = 0; i < KEYPAD_TRIES_NUMBER; i++) {
    delay(KEYPAD_TRIES_DELAY);
    if (lockState == LockState::Closed) {
      handleKeyboard();
      //D("Handling keyboard");
    }
    handleButton();
    
  }
}