#include <Arduino.h>

float getLocalFaderValue(int *faders, float *faderValueHistory, int faderNumber);
float getLocalPotiValue(int *faders, float *faderValueHistory, int faderNumber);

const char* ipToStr(const uint8_t* ip);
void die();
