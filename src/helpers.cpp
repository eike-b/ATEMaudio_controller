#include <helpers.h>
#include <Adafruit_SSD1306.h>

// TODO vielleicht wieder nutzen? don't repeat yourself und so..
// aktuell ohne verwendung
float getLocalFaderValue(int *faders, float *faderValueHistory, int faderNumber) {
    float faderValueNew = analogRead(faders[faderNumber]);
    float faderValueOld = faderValueHistory[faderNumber];
    
    if(abs(faderValueNew - faderValueOld) > 5) {
        float val = faderValueNew / 1024;
        faderValueHistory[faderNumber] = faderValueNew;
        return val;
    } 
    return false; 
}

// TODO vielleicht wieder nutzen? don't repeat yourself und so..
// aktuell ohne verwendung
float getLocalPotiValue(int *potis, float *potiValueHistory, int potiNumber) {
    float potiValueNew = analogRead(potis[potiNumber]);
    float potiValueOld = potiValueHistory[potiNumber];
    
    if(abs(potiValueNew - potiValueOld) > 5) {
        float val = potiValueNew / 1024;
        potiValueHistory[potiNumber] = potiValueNew;
        return val;
    } 
    return false; 
}


// macht aus einem int-array einen String zum darstellen und printen
const char* ipToStr(const uint8_t* ip) {
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return buf;
}

// Fallback, wenn alle stricke reißen, kann man den arduino in eine schleife schicken und seriell um hilfe rufen
void die() { 
  while(true) {
    if(Serial.available()) {
        Serial.println("Kill Me!");
    }
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}
