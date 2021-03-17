// ADSRduino
//
// a simple ADSR for the Arduino
#include <stdint.h>

#include "avr/pgmspace.h"
#include "LUT.h"
#include "math.h"

// #############################################################
// DAC stuff. might change:
// MCP4921...
// (pin allocations for convenience in hardware hook-up)
const int DAC_CS = 6;
const int DAC_SCK = 5;
const int DAC_SDI = 4;
const int DAC_LDAC = 8;
byte upper_byte = 0x10;
byte lower_byte = 0;

#define pulseHigh(pin)       \
  {                          \
    digitalWrite(pin, HIGH); \
    digitalWrite(pin, LOW);  \
  }

// subroutine to set DAC on MCP4921
void Set_DAC_4921(int DC_Value)
{
  lower_byte = DC_Value & 0xff;
  upper_byte = (DC_Value >> 8) & 0x0f;
  bitSet(upper_byte, 4);
  bitSet(upper_byte, 5);
  digitalWrite(DAC_CS, LOW);
  tfr_byte(upper_byte);
  tfr_byte(lower_byte);
  digitalWrite(DAC_SDI, LOW);
  digitalWrite(DAC_CS, HIGH);
  digitalWrite(DAC_LDAC, LOW);
  digitalWrite(DAC_LDAC, HIGH);
}
// transfers a byte, a bit at a time, LSB first to the DAC
void tfr_byte(byte data)
{
  for (int i = 0; i < 8; i++, data <<= 1)
  {
    digitalWrite(DAC_SDI, data & 0x80);
    pulseHigh(DAC_SCK); //after each bit sent, CLK is pulsed high
  }
}
// #############################################################

// CONTROL SIGNALS
int CV_curve = 0, CV_curve0 = 0;
const int CV_curve_shift = 10 - int(log(CURVES) / log(2));
int CV_subSteps = 0;

void setup()
{
  pinMode(DAC_CS, OUTPUT);
  pinMode(DAC_SCK, OUTPUT);
  pinMode(DAC_SDI, OUTPUT);
  pinMode(DAC_LDAC, OUTPUT);

  Set_DAC_4921(0);
  pinMode(10, OUTPUT);

  // Serial.begin(9600);
}

bool firstLoop = true;
const float coeff = 2 * M_PI/16;
void loop()
{
  // for (int i = 0; i < 16; i++)
  // {
  //   Set_DAC_4921(2048+2047*sin(coeff * i));
  // }

  Set_DAC_4921(0);
  digitalWrite(10, HIGH);
  delay(1);
  digitalWrite(10, LOW);

  risingPhase();
  fallingPhase(MAX, 2048, &CV_curve0, &CV_subSteps);
  fallingPhase(2048, 0, &CV_curve0, &CV_subSteps);

  // // firstLoop = false;
}

uint32_t interp(uint32_t val0, uint32_t val1, uint32_t position, uint32_t distance)
{
  if (distance == 0)
  {
    return val1;
  }

  if (val0 <= val1)
  {
    return val0 + position * (val1 - val0) / distance;
  }
  else
  {
    return val0 - position * (val0 - val1) / distance;
  }
}

void readCVs()
{
  CV_curve = analogRead(0);
  CV_curve0 = CV_curve >> CV_curve_shift; //

  CV_subSteps = analogRead(1);
}

uint32_t readWrite(uint32_t actualVal)
{
  // digitalWrite(10, LOW);
  Set_DAC_4921(actualVal);
  // digitalWrite(10, HIGH);
  readCVs();
  return actualVal;
}

uint32_t risingPhase()
{
  for (uint32_t i = 0; i < TIME; i++)
  {
    // if (firstLoop == true)
    // {
    //   Serial.print(i);
    //   Serial.print("   ");
    //   Serial.print(CV_curve0);
    //   Serial.print("   ");
    //   Serial.println(pgm_read_word(&(LUT[CV_curve0][i])));
    // }
    if (CV_subSteps == 0)
    {
      readWrite(uint32_t(pgm_read_word(&(LUT[CV_curve0][i]))));
    }
    else
    {
      for (uint32_t pos = 0; pos < CV_subSteps; pos++)
      {
        int v0, v1;
        if (i == TIME - 1) // show last value only once!
        {
          readWrite(int(pgm_read_word(&(LUT[CV_curve0][i]))));
          break;
        }
        else
        {
          v0 = int(pgm_read_word(&(LUT[CV_curve0][i])));
          v1 = int(pgm_read_word(&(LUT[CV_curve0][i + 1])));
          readWrite(interp(v0, v1, pos, CV_subSteps));
        }
      }
    }
  }
}

uint32_t fallingPhase(uint32_t startVal, uint32_t stopVal, int *curvePtr, int *subStepsPtr)
{
  float scalingFactor = (float(startVal - stopVal) / float(MAX + 1));
  for (uint32_t i = 0; i < TIME; i++)
  {
    if (*subStepsPtr == 0)
    {
      readWrite(startVal - uint32_t(scalingFactor * pgm_read_word(&(LUT[*curvePtr][i]))));
    }
    else
    {
      for (uint32_t pos = 0; pos < *subStepsPtr; pos++)
      {
        int v0, v1;
        if (i == TIME - 1) // show last value only once!
        {
          readWrite(startVal - uint32_t(scalingFactor * pgm_read_word(&(LUT[*curvePtr][i]))));
          break;
        }
        else
        {
          v0 = startVal - uint32_t(scalingFactor * pgm_read_word(&(LUT[*curvePtr][i])));
          v1 = startVal - uint32_t(scalingFactor * pgm_read_word(&(LUT[*curvePtr][(i + 1)])));
          readWrite(interp(v0, v1, pos, *subStepsPtr));
        }
      }
    }
  }
}