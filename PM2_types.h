
#include <Arduino.h>

#ifndef _PM2_TYPES
#define _PM2_TYPES
typedef union{
  float f;
  byte b[4];
} floatbyte; 
#else
extern floatbyte fbyte; 
#endif
