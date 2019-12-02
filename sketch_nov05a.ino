#include <ESP8266WiFi.h>            // Biblioteka do WiFI.
#include <ESP8266HTTPClient.h>      // Biblioteka do WiFI.
#include <ESP8266WiFiMulti.h>       // Biblioteka do WiFI
#include <WiFiClient.h>             // Biblioteka do WiFI.
#include <PCF8574.h>                // Biblioteka do ekstendera wyprowadzeń PCF8574.
#include <Keypad.h>                 // Biblioteka do obsługi klawiatury.
#include <LiquidCrystal_I2C.h>      // Biblioteka do wyświetlacza tekstowego LCD z konwerterem I2C.
#include <Wire.h>                   // Biblioteka do komunikacji przez I2C.  
#include <Keypad_I2C.h>             // Biblioteka do podłączenia klawiatury przez magistrale.
#include <SPI.h>                    // Biblioteka do komunikacji z jednym lub większą liczbą urządzeń peryferyjnych.
#include <MFRC522.h>                // Biblioteka do RFID.
#include <Gsender.h>                // Biblioteka do wysyłania maila.

#define CZUJNIK 0      // Przypisanie CZUJNIKA do wejścia D3.
#define SS_PIN 15     // Przypisanie SS_PIN z RFID do wejścia D8.
#define RST_PIN 2     // Przypisanie RST_PIN z RFID do wejścia D4.
#define I2CADDR 0x21  // Ustawienie adresu na magistrali na 0x21.       
#define staty 1        // Zmienna do wyświetlania informacji o wyjściach.
PCF8574 expander_1(0x20);   // Ustawienie adresu expandera na 0x20.
LiquidCrystal_I2C lcd(0x27, 16, 2); // Ustawienie adresu ukladu LCD na 0x27.

#pragma region Globals
const char* ssid = "Wojciaszek";                               // WIFI network name
const char* password = "wojciaszek3213";                       // WIFI network password
uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals

ESP8266WiFiMulti WiFiMulti;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
int statuss = 0;
int out = 0;
int i = 0;
char initial_password[] = "1234" ;
char alarm_kod[] = "8888" ;
char kod[4];

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {             // Opisanie przycisków klawiatury i przypisanie ich do portów na ekspanderze wyprowadzeń.
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Digitran keypad, bit numbers of PCF8574 i/o port
byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6, 7}; //connect to the column pinouts of the keypad

Keypad_I2C kpd( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, 1 );

void info()
{if(staty == 1)
 {
   
   //test
  
 }

}
void setup()
{
  Wire.begin( );                              // Wyświetlanie komunikatu na LCD przy zakończeniu kompilacji.
  lcd.begin(16, 2);                           // .
  lcd.backlight();                            // . Załączenie podświetlenia.
  lcd.setCursor(0, 0);                        // .
  lcd.print("LCD & I2C");                     // .
  lcd.setCursor(0, 1);                        // .
  lcd.print("**********      ");              // .

  pinMode(CZUJNIK, INPUT_PULLUP);             // Deklaracja czujnika jako wejścia.
  expander_1.pinMode(7, OUTPUT);              // Deklaracja silnika jako wyjścia.
  expander_1.pinMode(6, OUTPUT);              // Deklaracja silnika jako wyjścia.
  expander_1.pinMode(5, OUTPUT);              // Deklaracja silnika jako wyjścia.
  expander_1.pinMode(4, OUTPUT);              // Deklaracja silnika jako wyjścia.
  expander_1.pinMode(3, OUTPUT);              // Deklaracja zamka przez ekspander jako wyjścia.
  expander_1.pinMode(2, OUTPUT);              // Deklaracja alarmu przez ekspander jako wyjścia.
 

  expander_1.pinMode(7, LOW);
  expander_1.pinMode(6, LOW);
  expander_1.pinMode(5, LOW);
  expander_1.pinMode(4, LOW);
  expander_1.pinMode(3, HIGH);
  expander_1.pinMode(2, HIGH);

  kpd.begin( makeKeymap(keys));
  Serial.begin(9600);
  Serial.println( "start" );

  SPI.begin();                 // Initiate  SPI bus
  mfrc522.PCD_Init();          // Initiate MFRC522

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Wojciaszek", "wojciaszek3213");      // Określenie nazwy i hasła do sieci, z którą mamy się połączyć.

 
}

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
    static uint16_t attempt = 0;
    Serial.print("Connecting to ");
    if(nSSID) {
        WiFi.begin(nSSID, nPassword);  
        Serial.println(nSSID);
    } else {
        WiFi.begin(ssid, password);
        Serial.println(ssid);
    }

    uint8_t i = 0;
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
        delay(200);
        Serial.print(".");
    }
    ++attempt;
    Serial.println("");
    if(i == 51) {
        Serial.print("Connection: TIMEOUT on attempt: ");
        Serial.println(attempt);
        if(attempt % 2 == 0)
            Serial.println("Check if access point available or SSID and Password\r\n");
        return false;
    }
    Serial.println("Connection: ESTABLISHED");
    Serial.print("Got IP address: ");
    Serial.println(WiFi.localIP());
    return true;
}

void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}

void sendToServer(String command)         // Komuniacja z serverem przez WiFi.
{ 
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "http://adls.herokuapp.com/api/locks/570781de-9fce-4f0c-bbe6-681fcb8a15be/" + command)) {  // HTTP


      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.POST("");

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}

void wyswietl(char napis1[16], char napis2[16])       // Funkcja wyświetlająca komunikaty na ekranie LCD.
{
  lcd.setCursor(0, 0);
  lcd.print(napis1); 
  lcd.setCursor(0, 1);
  lcd.print(napis2);
}

void stan_drzwi(int stan)                              // Funkcja zawierająca stany drzwi.
{
  #define CLOSE 0
  #define OPEN 1
  #define BLOCK 2

  if (stan == OPEN)
  {
    wyswietl("Drzwi otwarte   ", "****************");
    expander_1.digitalWrite(3, LOW);
  }
  else if (stan == CLOSE)
  {
    expander_1.digitalWrite(3, HIGH);
    wyswietl("Drzwi zamkniete ", "****************");
  }
  else if (stan == BLOCK)
  {
    expander_1.digitalWrite(3, HIGH);
    wyswietl("Blokada dostepu ", "****************");
  }
}

void procedura_otwarcia()
{
  int j = 0;
  stan_drzwi(OPEN);
  for (j = 0; j < 5; j++)
  {
    delay(1000);
    if (digitalRead(CZUJNIK) == LOW) // Jeśli czujnik zwarty.
    {
      stan_drzwi(CLOSE);
      break;
    }
    if (j > 2 && digitalRead(CZUJNIK) == HIGH) // Po 3 sek włącza się alarm, jeżeli czujnik rozwarty.
    {
      wyswietl("    ALARM     ", "Drzwi otwarte      ");
      expander_1.digitalWrite(2, HIGH);
      delay(500);
      expander_1.digitalWrite(2, LOW);
    
      if(j == 5)
      {
        wyswietl("ALARM KRYTYCZNY ", "!!!!!!!!!!!!!!!!");
        delay(500);
      } 
    } 
    i = 0;
  }
}
void silnik(int s)
{for(int d=0; d++;d<10)
{
 expander_1.digitalWrite(7, HIGH);
 delay(s);
 expander_1.digitalWrite(7, LOW);
 expander_1.digitalWrite(5, HIGH);
 delay(s);
 expander_1.digitalWrite(5, LOW);
 expander_1.digitalWrite(6, HIGH);
 delay(s);
 expander_1.digitalWrite(6, LOW);
 expander_1.digitalWrite(4, HIGH);
 delay(s);
 expander_1.digitalWrite(4, LOW);
}
}
void kod_dostepu()
{
   int k;
  char key = kpd.getKey(); // Otwieranie zamka za pomocą kodu "1234".
  if (key)
  {
    kod[i++] = key;
    Serial.println(key);
    Serial.println(i);
  }
  if(i == 3){k=0;}
  if (i == 4)
  {
    if (!(strncmp(kod, initial_password, 4)))
    {
      wyswietl("Kod dostepu     ", "jest poprawny   ");
      delay(1000);
      procedura_otwarcia();
    }
    if (!(strncmp(kod, alarm_kod, 4)))
    {

        wyswietl("ALARM          ", "AKTYWOWANY       ");
        connection_state = WiFiConnect();
        if (!connection_state) // if not connected to WIFI
          Awaits();            // constantly trying to connect

        Gsender *gsender = Gsender::Instance(); // Getting pointer to class instance
        String subject = "ADLS Alarm";
        if (gsender->Subject(subject)->Send("szymonruta.sr@gmail.com", "Door are open for a long period of time. Possible security breach"))
        {
          Serial.println("Message send.");
        }
        else
        {
          Serial.print("Error sending message: ");
          Serial.println(gsender->getError());
        }
        k = 1;
        wyswietl("ALARM  koniec   ", "             ");
        delay(5000);  
        i = 0;
    }

      else
      {
        wyswietl("Kod dostepu     ", "jest bledny     ");
        delay(1000);
        stan_drzwi(BLOCK);
        delay(1000);
        stan_drzwi(CLOSE);
        i = 0;
      }
    
  }
}
void loop()
  {
    
  
    // char key = kpd.getKey();
    kod_dostepu();

  // Moduł RFID
    //Look for new cards   
    if ( ! mfrc522.PICC_IsNewCardPresent())
    {
      return;
    }
    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
    {
      return;
    }
    //Show UID on serial monitor
    Serial.println();
    Serial.print(" UID tag :");
    String content = "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    Serial.println();
  //Koniec Modułu
  // Wysyłanie maila
    
  // Koniec wysyłania maila

  info();
  if (content.substring(1) == "7B 71 17 21") // UID, które może otworzyć zamek.
  { sendToServer("opened");
    stan_drzwi(OPEN);
    delay(2000);
    stan_drzwi(CLOSE);
    delay(500);
    sendToServer("closed");
    statuss = 1;
  }

  else if (content.substring(1) == "43 D3 44 7A")  // Przypadek Wojciaszek.
  {
    wyswietl("Oddawaj zarowke  ", "****************");

    delay(2000);
  }
   else if (content.substring(1) == "5B 05 49 0A")
  {
    wyswietl("******TEST******", "*TEST*TEST*TEST*");
    delay(500);
    stan_drzwi(OPEN);
    expander_1.digitalWrite(2, LOW); 
    delay(1000);
    expander_1.digitalWrite(2, HIGH);
    stan_drzwi(CLOSE);
    
    statuss = 1;
  }  
  else if (content.substring(1) == "F3 4A D6 B2")
  {
    wyswietl(" Dzien Dobry!  ", "   Krystian    ");
    stan_drzwi(OPEN);
    delay(2000);
    stan_drzwi(CLOSE);
    silnik(200);
    statuss = 1;
  }
  else   {                    // Każdy inny identyfikator.
    stan_drzwi(BLOCK);
    delay(2000);
    stan_drzwi(CLOSE);
    statuss = 1;
  }
}
