#include <SPI.h>
#include <Ethernet.h>
#include <EthernetBonjour.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include <ATEMpro.h>
#include <helpers.h>

#define DEBUG true; // wird als weiche für Serial outs genutzt

int systemState = 0; //0 setup, 1 verbindung aufbauen, 2 warten auf verbindung, 3 verbunden, 4 verbindung lost 

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x84, 0xF2 }; // MAC Adresse des Ethernet Shields
IPAddress ip(192, 168, 178, 112); // IP-Adresse, welche der Controller selbst hat
bool found = false; // variable wird genutzt um zu flagen ob ein ATEM gefunden wurde

String foundAtemNames [15]; // wird später mit den namen der gefundenen ATEMs gefüllt
byte foundAtemAddresses [15][4];// wird später mit den ips der gefundenen ATEMs gefüllt
int foundAtems = 0; // anzahl der gefunden atems
char serviceName[256] = "_blackmagic"; // nach diesem service wird im netzwerk ausschau gehalten (beim mdns query)

// konfiguration des controllers
// TODO sollte per abfrage beim mischer automatisch konfiguriert werden bzw im menü einstellbar werden

/* Schema:
*  poti/fader/.. -> {AnalogInput, Input im ATEM, Wert}
*/
int potis[][3] = {{A4, 1, 0}, {A5, 2, 0}, {A6, 3, 0}, {A7, 4, 0}};
int faders[][3] = {{A0, 1, 0}, {A1, 2, 0}, {A2, 3, 0}, {A3, 4, 0}};
int mutes[][4] = {{8, 1, 1, 0}, {9, 1, 2, 0}, {10, 1, 3, 0}, {11, 1, 4, 0}};
int selectors[][4] = {{12, 1, 1, 0}, {13, 1, 2, 0}, {26, 1, 3, 0}, {27, 1, 4, 0}};
int functions[][3] = {{28, 1, 1}, {29, 1, 2}, {30, 1, 3}};
int tallys[][3] = {{22, 1, 0}, {23, 2, 0}, {24, 3, 0}, {25, 4, 0}};

// variablen für den encoder und das menü
volatile bool encTurned = false;
volatile int menuPosition = 0;
uint16_t selectedChannel = 0;
uint8_t function = 0;

ATEMpro AtemSwitcher; // initialisierung des ATEM objekts

/**
 * @brief Callback Funktion für die mDNS Service Routine
 * 
 * @param type 
 * @param proto 
 * @param name 
 * @param ipAddr 
 * @param port 
 * @param txtContent 
 */
void ATEMfound(const char* type, MDNSServiceProtocol proto,
                  const char* name, const byte ipAddr[4],
                  unsigned short port,
                  const char* txtContent)
{
  // wenn nichts gefunden wurde, oder das Ergebnis leer oder korrupt ist
  if(name == NULL) {
    Serial.println("Anfrage liefert NULL.");
    return;
  }

  Serial.println("...found!");
  Serial.println(ipToStr(ipAddr));
  
  // Anzahl gefundener ATEMs erhöhen
  foundAtems++;
  unsigned int index = foundAtems - 1;

  // gefundenen ATEM in Array schreiben
  foundAtemNames[index] = name;
  foundAtemAddresses[index][0] = ipAddr[0];
  foundAtemAddresses[index][1] = ipAddr[1];
  foundAtemAddresses[index][2] = ipAddr[2];
  foundAtemAddresses[index][3] = ipAddr[3];
  Serial.println(foundAtemNames[index]);

  
  found = true;
  Serial.println(foundAtems);

}

/**
 * @brief Diese Funktion stellt eine Verbindung mit dem im Menü ausgewählten Bildmischer her.
 * 
 */
void connectToATEM() {
  IPAddress switcherIp(
    foundAtemAddresses[menuPosition][0], 
    foundAtemAddresses[menuPosition][1], 
    foundAtemAddresses[menuPosition][2], 
    foundAtemAddresses[menuPosition][3]);
    AtemSwitcher.begin(switcherIp);
    AtemSwitcher.serialOutput(0);
    AtemSwitcher.connect();
    delay(100);
    systemState = 2;   
}

/**
 * @brief Das Menü wird auf dem Display gezeichnet und Auswahlfunktionen bereitgestellt.
 * 
 */
void menu() {

  if(menuPosition > foundAtems-1) {
    menuPosition = 0;
  } else if(menuPosition < 0) {
    menuPosition = foundAtems-1;
  }

  char buf[50];
  display.clearDisplay();
  display.setCursor(0, 0);
  sprintf(buf, "Suche ATEM Mixer.. %u", foundAtems);
  display.print(buf);
  display.drawLine(0, 8, 128, 8, WHITE);
  display.setCursor(0, 10);
  display.setTextWrap(false);


  
  for(uint8_t row = 0; row < foundAtems; row++) {
    if(row == menuPosition) {
      display.setTextColor(BLACK, WHITE);
    }
    display.println(foundAtemNames[row]);
    //display.println(ip_to_str(foundAtemAddresses[row]));
    if(row == menuPosition) {
      display.setTextColor(WHITE);
    }
  }
  
  display.display();
}

/**
 * @brief Callbackfunktion für den Interrupt Handler (beim Drehen des Encoders)
 * 
 */
void encInterrupt() {
  if(digitalRead(2) != digitalRead(5)) {
    menuPosition++;
  } else {
    menuPosition--;
  }
  encTurned = true;
}

/**
 * @brief Callbackfunktion für den Interrupthandler (beim Drücken des Encoders)
 * 
 */
void encBtnInterrupt() {
  systemState = 1;
}

/**
 * @brief die bekannte Arduino Setup routine
 * 
 */

void setup() {
  // Serial init
  Serial.begin(9600);
  randomSeed(analogRead(A5)); // für die wahl von Ports beim Verbinden mit einem ATEM

  // Encoder Setup
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), encInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), encBtnInterrupt, CHANGE);
  
  // Mute Buttons
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);

  // Selector Buttons
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);

  // Function buttons
  pinMode(28, INPUT_PULLUP);
  pinMode(29, INPUT_PULLUP);
  pinMode(30, INPUT_PULLUP);

  // Tallys
  pinMode(22, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(24, OUTPUT);
  pinMode(25, OUTPUT);

  // Init Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { die(); }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Display... OK");
  display.display();

  // Init Ethernet
  display.print("Starte Ethernet... ");
  display.display();

  Ethernet.begin(mac, ip);
  display.print("OK\n");

  // Init mDNS
  display.print("Starte mDNS... ");
  display.display();
 
  EthernetBonjour.begin("arduino");
  EthernetBonjour.setServiceFoundCallback(ATEMfound);
  display.print("OK\n");

  delay(500);
}

/**
 * @brief die bekannte Arduino loop Funktion. Wird in Schleife aufgerufen.
 * 
 */
void loop() {
  // Status 0 - System ist gestartet und ATEM wird nun gesucht
  if(systemState == 0) {
    if (!EthernetBonjour.isDiscoveringService()) {
      if(!found) {
        if(EthernetBonjour.startDiscoveringService(serviceName, MDNSServiceTCP, 5000)) {
          Serial.println("Discovering started!");
        }
        else {
          Serial.println("Discovering NOT started!");
        }
      }
    }
    // Bonjour Service und damit mDNS laufen lassen
    EthernetBonjour.run();
    
    // menü zeichnen
    menu();
    
    // serial ausgabe des encoder wertes (DEBUG)
    #ifdef DEBUG
    if(encTurned == true) {
      Serial.println(menuPosition);
      encTurned = false;
    }
    #endif
  } 
  
  // System Status 1 -  ATEM wurde ausgewählt und verbindung wird aufgebaut
  if(systemState == 1)
  {
    // Display anweisungen - ip des ATEMs zeigen
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Verbinde mit");
    display.print(foundAtemAddresses[menuPosition][0]);
    display.print(".");
    display.print(foundAtemAddresses[menuPosition][1]);
    display.print(".");
    display.print(foundAtemAddresses[menuPosition][2]);
    display.print(".");
    display.print(foundAtemAddresses[menuPosition][3]);
    display.display();
    // Verbindung zum ausgewählten ATEM aufbauen
    connectToATEM();
    
  }
  
  // System Status 2 - Warten Auf Verbindung zum ATEM
  if(systemState == 2)
  {
    // ATEM-Bib loop (pakete lesen/empfangen)
    AtemSwitcher.runLoop();
    if(AtemSwitcher.isConnected()) {
      // wenn Verbindung hergestellt wurde, Status wechseln
      systemState = 3;
    } 
   
  }
  
  // System Status 3 - Verbindung zum ATEM hergestellt, normaler Betrieb
  if(systemState == 3) {
    // ATEM bib loop
    AtemSwitcher.runLoop();

    // Infos auf dem Display ausgeben
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Verbunden!");
    display.println("123 gude laune");
    display.println("");
    display.print("Selected Channel ");
    display.println(selectedChannel);
    display.print("Function ");
    display.println(function);
    display.println(menuPosition);
    display.display();
    
    // Abfrage der Faderpositionen, für jeden Fader im faders-Array
    for(unsigned int i = 0; i < sizeof(faders)/sizeof(faders[0]); i++) {

      float faderValueNew = analogRead(faders[i][0]);
      float faderValueOld = faders[i][2];
      // wenn sich der Wert zum zuvor abgespeicherten um die Differenz von 5 unterscheidet
      if(abs(faderValueNew - faderValueOld) > 5) {
        // normierten Wert berechnen (zwischen 0 und 1)
        float val = faderValueNew / 1024;
        faders[i][2] = faderValueNew;
        // neuen Wert an den ATEM senden
        AtemSwitcher.setFaderGain(faders[i][1], -65280, val);

        // debug seriell ausgaben
        #ifdef DEBUG 
          Serial.print(" Kanal ");
          Serial.print(faders[i][1]);
          Serial.print(" Wert ");
          Serial.print(val);
          Serial.println();
        #endif
      } 
    }

    // Abfrage der Potis für jedes im potis array
    for(unsigned int i = 0; i < sizeof(potis)/sizeof(potis[0]); i++) {

      float potiValueNew = analogRead(potis[i][0]);
      float potiValueOld = potis[i][2];
      // wenn sich der Wert zum zuvor abgespeicherten um die Differenz von 5 unterscheidet
      if(abs(potiValueNew - potiValueOld) > 5) {
        // normierten Wert berechnen (zwischen 0 und 1)
        // TODO don't repeat yourself, eike
        float val = potiValueNew / 1024;
        potis[i][2] = potiValueNew;
        // Wert des Potis an den Mischer senden
        AtemSwitcher.setGain(potis[i][1], -65280, val);
        #ifdef DEBUG 
          Serial.print(" Kanal ");
          Serial.print(potis[i][1]);
          Serial.print(" Wert ");
          Serial.print(val);
          Serial.println();
        #endif
      } 
    }

    // mute knöpfe auf Veränderung abfragen
    // TODO mute/live Sprachgebrauch vereinheitlichen
    for(unsigned int i = 0; i < sizeof(mutes)/sizeof(mutes[0]); i++) {

      bool btnValueNew = digitalRead(mutes[i][0]);
      bool btnValueOld = mutes[i][1];
      
      if(btnValueNew == LOW and btnValueOld != LOW) {
        // mute button wurde gedrückt (falling edge)
        if(mutes[i][3] == 0) {
          // wenn nicht live
          // TODO besser keinen lokalen Status sondern einen in der Bib nutzen
          mutes[i][3] = 1;
          AtemSwitcher.setMute(mutes[i][2], -65280, false); // Kanal live schalten /unmute
        } else { 
          // wenn live
          mutes[i][3] = 0;
          AtemSwitcher.setMute(mutes[i][2], -65280, true); // Kanal off air schalten /mute
        }

        #ifdef DEBUG 
          Serial.print(" Kanal ");
          Serial.print(mutes[i][2]);
          Serial.print(" Wert ");
          Serial.print(mutes[i][3]);
          Serial.println();
        #endif
      }
      mutes[i][1] = btnValueNew;
    }

    //Tally LEDs durchgehen und ggf. rotlicht zeigen (wenn kanal live)
     for(unsigned int i = 0; i < sizeof(tallys)/sizeof(tallys[0]); i++) {
      if(AtemSwitcher.getFairlightTally((uint8_t)tallys[i][1], -65280)) {
        digitalWrite(tallys[i][0], HIGH);
      } else {
        digitalWrite(tallys[i][0], LOW);
      }
    }

    // TODO Baustelle! Selector knöpfe wählen Kanal aus für weitere funktionen
    for(unsigned int i = 0; i < sizeof(selectors)/sizeof(selectors[0]); i++) {

      bool btnValueNew = digitalRead(selectors[i][0]);
      bool btnValueOld = selectors[i][1];
      
      if(btnValueNew == LOW and btnValueOld != LOW) {
        // selector button wurde gedrückt (falling edge)
        selectedChannel = selectors[i][2];

        #ifdef DEBUG 
          Serial.print("Selected Channel: ");
          Serial.println(selectedChannel);
        #endif
      }
      selectors[i][1] = btnValueNew;
    }

    // TODO baustelle functions buttons sollen delay/balance/etc abbilden
    // -> kanal selektieren -> funktion wählen (zb delay) -> mit encoder wert ändern
    for(unsigned int i = 0; i < sizeof(functions)/sizeof(functions[0]); i++) {

      bool btnValueNew = digitalRead(functions[i][0]);
      bool btnValueOld = functions[i][1];
      
      if(btnValueNew == LOW and btnValueOld != LOW) {
        // selector button wurde gedrückt (falling edge)
        function = functions[i][2];

        #ifdef DEBUG 
          Serial.print("function: ");
          Serial.println(function);
        #endif
      }
      functions[i][1] = btnValueNew;
    }

    // sollte die Verbindung zum Mischer abbrechen wird im Display angezeigt und der systemstaus geändert
    if(!AtemSwitcher.isConnected()) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Verbindungsabbruch!!");
      display.println(":(");
      display.display();
      // zurück auf "null" wie bei system start, siehe system status = 0 
      systemState = 0;
      foundAtems = 0;
      found = false;
      menuPosition = 0;
      memset(foundAtemNames, 0, sizeof(foundAtemNames));
      memset(foundAtemAddresses, 0, sizeof(foundAtemAddresses));
      delay(500);
    }
  }
}
