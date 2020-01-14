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

std::vector<String> json_codes_standard; //Wektor w którym przechowywane są kody standardowe.
std::vector<String> json_codes_one_time; //Wektor w którym przechowywane są kody jednorazowe.
std::vector<String> json_rfid;           //Wektor w którym przechowywane są kody RFID.

String TOKEN = "3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9";

// Code variables
int code_index = 0;
char code_current[5];
char code_open[] = "1234";
char code_roleta_lock[] ="7809";
char code_roleta_unlock[] ="AB63";
char code_add_rfid[] = "9999";
char code_remove_rfid[] = "6666";

int ROLETA = UNLOCKED ; 
int LOCK = CLOSED;
int bad_code = 0;
bool send_door ;

// JSON read variables
char json[1172];
String json2 ;
int got_message = 0 ;
char time_open = 0;
char time_close = 0;
char rfid_tags_1 = 0;
char rfid_tags_2 = 0;
char rfid_tags_3 = 0;
int CZUJNIK_DRZWI;  // 1024 jak otwarte
String HOW;

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
  ws_client.send("{\"command\":\"subscribe\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\"}");
  // Send a ping
  ws_client.ping();
  ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"action\\\":\\\"shutter_opened\\\"}\"}");
  ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"action\\\":\\\"get\\\"}\"}");
  

  change_rgb_color(RgbColor::Blue);
}
/*
 * Funckja zamieniająca string na char.
 */
void string_to_char(String s,char* c) {  
  strcpy(c, s.c_str());
}
/*
 * Funckja odczytująca wiadomość  konfiguracyjną z servera
 * zawierającą kody, harmonogram rolety, numery kart RFID
 */
void json_read(){
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(4) + 2*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(8) + 650;
  DynamicJsonDocument doc(capacity);
  yield();
  deserializeJson(doc, json);
  yield();
  const char* identifier = doc["identifier"]; // "{\"channel\":\"LocksChannel\"}"

  JsonObject message = doc["message"];
  const char* message_shutter_status = message["shutter_status"]; // "open"
  yield();
  JsonObject message_shutter_configuration = message["shutter_configuration"];
  const char* message_shutter_configuration_opens_at = message_shutter_configuration["opens_at"]; // "07:30"
  const char* message_shutter_configuration_closes_at = message_shutter_configuration["closes_at"]; // "23:30"
  yield();
  JsonObject message_shutter_configuration_lock = message_shutter_configuration["lock"];
  const char* message_shutter_configuration_lock_shutter_status = message_shutter_configuration_lock["shutter_status"]; // "open"
  yield();
  int code_n = ((int)message["codes"].size());
  int rfid_n = ((int)message["rfid_tags"].size());
  yield();
  for(int i=0; i < code_n; i++){          // Petla odczytująca kody wysłane z serwera i dodająca je do użytku
    JsonObject message_code = message["codes"][i];
    const char* message_code_kind = message_code["kind"]; // "standard"
    const char* message_code_code = message_code["code"]; // "1243"
    Serial.print("CODE  #");Serial.println(i);Serial.println(message_code_code);
    if (String(message_code_kind) ==  String("standard")){
      json_codes_standard.push_back(String(message_code_code));
     }
     if (String(message_code_kind) ==  String("one_time")){
      json_codes_one_time.push_back(String(message_code_code));
     }
     yield();
  }
  for(int i=0; i < rfid_n; i++){          // Petla odczytująca RFID wysłane z serwera i dodająca je do użytku
    JsonObject message_rfid_tags = message["rfid_tags"][i];
    const char* message_rfid_tags_uid = message_rfid_tags["uid"];
    json_rfid.push_back(String(message_rfid_tags_uid));
    Serial.print("RFIT TAG  #");Serial.println(i);Serial.println(message_rfid_tags_uid);
    yield();
  }
}
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
    got_message ++;
    if(got_message == 4)
    {
      json2 = message.data();
      string_to_char(json2,json);
      Serial.print("To przyszło: ");
      Serial.println(json);
      yield();
      json_read();
    }  
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
    if (CZUJNIK_DRZWI == 1024) { 
      lcd.setCursor(0, 1);
      lcd.print("Drzwi otwarte   ");
    }
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
  HOW = " ";
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
  for (String str : json_rfid)
    {
     
      if (content.substring(1) ==  str) {
        HOW = OPEN_RFID_NEW;
        handleLockOpen();
        return;
      }
    }
  if (content.substring(1)) { 
    display_on_lcd("Brak dostepu    ","****************");
    delay(500);
    closeLock();
    reset_display();
    return;
  //   //change_and_display_lock_state(LockState::Closed);
  }
}
/**
 * Funkcja wysyłająca do serwerwera kody dostępu i UID RFID.
 */ 
void send_ws(String action, String attr, String attr_value){
  ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\""+ TOKEN +"\\\", \\\""+ attr +"\\\":\\\""+ attr_value +"\\\",\\\"action\\\":\\\""+ action +"\\\"}\"}");
}
/**
 * Funkcja pozwalająca na usuniecie karty RFID.
 */ 
void new_rfid_remove(){
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  for (int i = 0; i < json_rfid.size(); i++) {
    if(json_rfid[i] == content.substring(1)) {
      display_on_lcd("Usunieto karte  ","RFID z systemu  ");
      send_ws("rfid_tag_removed","uid",json_rfid[i]);
      ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"RFID Card was removed succesfully.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"warning\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;
      json_rfid.erase(json_rfid.begin() + i);
      delay(1500);
      return;
    }
  }
  display_on_lcd("Nie ma tej karty","w systemie      ");
  delay(1500);
 }
/**
 * Funkcja pozwalająca na dodanie nowej karty RFID.
 */ 
void new_rfid_add(){
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  for (int i = 0; i < json_rfid.size(); i++) {
      if(json_rfid[i] == content.substring(1)) {
        display_on_lcd("Ta karta jest   ","juz w systemie  ");
        delay(1500);
        reset_display();
        return;
      }
  }
  json_rfid.push_back(String(content.substring(1)));
  send_ws("rfid_tag_added","uid",String(content.substring(1)));  
  display_on_lcd("Dodano nowa     ","karte pomyslnie ");
  ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"New RFID Card was added succesfully.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"warning\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;
  delay(1500);
}
/**
 * Funckja odpowiedzialna za przycisk.
 */ 
void handleButton() {
  if(expander_1.digitalRead(PRZYCISK_PIN) == HIGH) {
    HOW = OPEN_BUTTON;
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
      bad_code = 0;
      if(ROLETA == UNLOCKED && (CZUJNIK_DRZWI != 1024) ) {
      display_on_lcd("Drzwi zostaja   ","zablokowane.    ");
      handleMotor(1);
      ROLETA = LOCKED;
      ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"Rolling shutter is pulled up.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"warning\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;
      clearCode();
      return;
      }
    }
    if (!(strncmp(code_current, code_roleta_unlock, 4))) {
      bad_code = 0;
      if(ROLETA == LOCKED ) {
      display_on_lcd("Drzwi zostaja   ","odblokowane.    ");
      handleMotor(-1);
      ROLETA = UNLOCKED;
      ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"Rolling shutter is pulled down.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"warning\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;
      clearCode();
      return;
      }
    }
    if (!(strncmp(code_current, code_add_rfid, 4))) {
      bad_code = 0;
      if(ROLETA == UNLOCKED )
      {
        D("Wprowadzono kod pozwalający na dodanie nowego RFID");
        display_on_lcd("Mozna dodac nowa","karte RFID      ");
        delay(2000);
        new_rfid_add();
        clearCode();
        reset_display();
        return;
      }
      else {display_on_lcd("Roleta uzyta    ","Odmowa dostepu  ");
        reset_display();
        clearCode();
        return;
      }
    }  
    if (!(strncmp(code_current, code_remove_rfid, 4))) {
      bad_code = 0;
      if(ROLETA == UNLOCKED )
      {
        display_on_lcd("Mozna usunac    ","karte RFID      ");
        delay(2000);
        new_rfid_remove();
        clearCode();
        reset_display();
        return;
      }
      else {display_on_lcd("Roleta uzyta    ","Odmowa dostepu  ");
        reset_display();
        clearCode();
        return;
      }
    }  
    for (String str : json_codes_standard){ 
      if (String(code_current) ==  str) {
        if(ROLETA == UNLOCKED ){
        bad_code = 0;
        HOW = OPEN_CODE;
        handleLockOpen();
        clearCode();
        return;
      }
       else {display_on_lcd("Roleta uzyta    ","Odmowa dostepu  ");
        delay(1500);
        reset_display();
        return;
        }
      }
    }
    for (int i = 0; i < json_codes_one_time.size(); i++) {
      if (String(code_current) ==  json_codes_one_time[i]) {
        if(ROLETA == UNLOCKED ){
        bad_code = 0;
        HOW = OPEN_ONETIME_CODE;
        handleLockOpen();
        send_ws("code_removed","code",json_codes_one_time[i]);
        json_codes_one_time.erase(json_codes_one_time.begin() + i);
        clearCode();
        return;
        }
        else {display_on_lcd("Roleta uzyta    ","Odmowa dostepu  ");
        delay(1500);
        reset_display();
        return;
        }
      }
    }

    if (!(strncmp(code_current, code_open, 4)), 4) {
      bad_code = 0;
      if(ROLETA == UNLOCKED )
      {
        D("Otwarcie");
        HOW = OPEN_CODE;
        handleLockOpen();
      }
      else {display_on_lcd("Roleta uzyta    ","Odmowa dostepu  ");
      delay(1500);
      reset_display();
      }
    } 
    else {
      D("Blokada");
      bad_code ++;
      if (bad_code == 3) {
      ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"Bad code was implemented few times.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"warning\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;
      clearCode();
      return;
      }
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
 * Funkcja obsługująca kody jednorazowe.
 */
void one_time_code(){
  int used; //dać wyżej
  if(used == 0 ){
  char code_n[] ="";
  }

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
    D(HOW);
    D("Sending opening of lock state");
    ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\", \\\"how\\\":\\\""+HOW+"\\\",\\\"action\\\":\\\"opened\\\"}\"}");
    D("Done sending opening of lock state");
    D("Sending way of opening the lock");
    ;
    D("Done sending way of opening the lock");
    openLock();
   }
   else {D("Drzwi zablokowane , kod nie działa");}  
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
  else{
    D("handleSettingAlarm");
    AlarmRaiseRequested = true;
    set_timeout(raiseAlarm, millis() + SET_ALARM_TIMEOUT_TIME, SET_ALARM_TIMEOUT);
  }
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
  if (!AlarmRaiseRequested && !AlarmRunning) return;
  if(AlarmRunning){
    remove_timeout(SET_ALARM_TIMEOUT);
    AlarmRaiseRequested = false;
    endAlarm();
  }
  if(AlarmRaiseRequested){
    D("handleCancelingAlarm");
    remove_timeout(SET_ALARM_TIMEOUT);
    AlarmRaiseRequested = false;
  }
  
}
/**
 * Funckja włączająca buzzer.
 */ 
void raiseAlarm() {
  D("raiseAlarm");
  send_mail("	szymonruta.sr@gmail.com","Door are open for a long period of time. Possible security breach");
  AlarmRaiseRequested = false;
  AlarmRunning = true;
  expander_2.digitalWrite(BUZZER_PIN, ALARM_ON);
  ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"Door are opened for a long period of time.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"alarm\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;
  set_timeout(endAlarm, millis() + END_ALARM_TIMEOUT_TIME, END_ALARM_TIMEOUT);
  // Wysyłanie emaila zbyt długo trwa i  zakończenie alarmu nie działa prawidłowo.
}
/**
 * Funkcja wyłączająca buzzer.
 */ 
void endAlarm() {
  D("endAlarm");
  AlarmRunning = false;
  expander_2.digitalWrite(BUZZER_PIN, ALARM_OFF);
  ws_client.send(    "{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"EventsChannel\\\"}\",\"data\":\"{\\\"event\\\":{\\\"message\\\":\\\"Alarm is over.\\\",\\\"lock_token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"level\\\":\\\"alarm\\\"},\\\"action\\\":\\\"event_occured\\\"}\"}") ;

}
/**
 * Funkcja odpowiedzialna za informowanie o stanie drzwi i rozpoczynająca procedury alarmowe.
 */ 
void handleSensors() {
  
  if (CZUJNIK_DRZWI == 1024) {
    // Otwarte
    if (lockState == LockState::Open && send_door == true) {
      lcd.setCursor(0, 1);
      lcd.print("Drzwi otwarte   ");
      handleSettingAlarm();
      ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"action\\\":\\\"door_opened\\\"}\"}");
      send_door = false;
    }
  }
  else {
    if (lockState == LockState::Closed &&  send_door == false){
      lcd.setCursor(0, 1);
      lcd.print("Drzwi zamkniete ");
      handleCancelingAlarm();
      ws_client.send("{\"command\":\"message\",\"identifier\":\"{\\\"channel\\\":\\\"LocksChannel\\\"}\",\"data\":\"{\\\"token\\\":\\\"3173d8ef-ac1c-46ec-9d87-ecf63cdc13b9\\\",\\\"action\\\":\\\"door_closed\\\"}\"}");
      send_door = true;
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
  for(i = 0; i<80; i++) //225 na pełne rozwinięcie
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
  handleSensors();  

  for (int i = 0; i < KEYPAD_TRIES_NUMBER; i++) {
    delay(KEYPAD_TRIES_DELAY);
    if (lockState == LockState::Closed) {
      handleKeyboard();
      //D("Handling keyboard");
    }
    handleButton();
    CZUJNIK_DRZWI = analogRead(17);
    
    
  }
}