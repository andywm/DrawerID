#include <Arduino.h>

/*============================================================================*/
/*General Config*/
/*============================================================================*/
using IDtype = uint16_t;
constexpr int NumBanks = 2; //Number of identifier IC's on a bus.
constexpr int RegisterSize = 8; //Shift Register Memory Size. Number of bus lanes.

//Auto calc number of shifts for iterating all identifier banks.
constexpr int NumBankShifts =  RegisterSize * (NumBanks / RegisterSize) + (((NumBanks % RegisterSize) > 0)? RegisterSize : 0);

constexpr int PadInterval_us = 10; //small padding time around fast io pulse to allow for any bleed.
constexpr int ChipStartupDelay_ms = 60; //time needed for the addressing IC's to power up from cold before being read.

constexpr int ButtonDebounce_ms = 200; 
constexpr int SerialBaud = 9600;

/*============================================================================*/
/*Pin Config*/
/*============================================================================*/
constexpr int InputSwitch = 2;

constexpr int InputRegister_SerialIn = 3;
constexpr int InputRegister_Latch = 4;
constexpr int InputRegister_Clock = 5;

constexpr int BankCtrl_SerialOut = 6;
constexpr int BankCtrl_Latch = 7;
constexpr int BankCtrl_Clock = 8;
constexpr int BankCtrl_Enable = 9;

constexpr int IdentChip_Clock = 10;

/*============================================================================*/
/* Reader Program*/
/*============================================================================*/
/*============================================================================*/

IDtype AddressTable[RegisterSize * NumBanks] = {0x0};
unsigned long LastSwitchTime = 0;
int ActiveBank = -1;
bool bTrigger = false;

//Forwards
void ReadInputRegister(void);
void SetBankActive(int);
void DisableBanks(void);
int Magnitude(int);
void DebugLogSerial();

void setup() 
{
  pinMode(InputSwitch, INPUT);
  pinMode(InputRegister_SerialIn, INPUT);
  pinMode(InputRegister_Latch, OUTPUT);
  pinMode(InputRegister_Clock, OUTPUT);
  pinMode(IdentChip_Clock, OUTPUT);

  pinMode(BankCtrl_Latch, OUTPUT);
  pinMode(BankCtrl_SerialOut, OUTPUT);
  pinMode(BankCtrl_Clock, OUTPUT);
  pinMode(BankCtrl_Enable, OUTPUT);
  
  digitalWrite(BankCtrl_Latch, HIGH);
  digitalWrite(BankCtrl_Clock, HIGH);
  digitalWrite(BankCtrl_Enable, LOW);
  digitalWrite(BankCtrl_SerialOut, LOW);
  digitalWrite(InputRegister_Latch, HIGH);
  digitalWrite(IdentChip_Clock, HIGH);
  digitalWrite(InputRegister_Clock, LOW);

  DisableBanks();

  Serial.begin(SerialBaud);
}

void loop() 
{
  if (digitalRead(InputSwitch))
  {
    unsigned long CurrentTime = millis();
    if (CurrentTime - LastSwitchTime > ButtonDebounce_ms)
    {
      bTrigger = true;
      LastSwitchTime = CurrentTime;
    }
  }

  if (bTrigger)
  {
    bTrigger = false;

    for (int Bank = 0; Bank < NumBanks; ++Bank)
    {
      SetBankActive(Bank);
      ReadInputRegister();
    }
    
    DisableBanks();
    DebugLogSerial();
  }
}

void ReadInputRegister()
{
    if (ActiveBank < 0 || ActiveBank >= NumBanks)
    {
      return;
    }

    //Zero active bank.
    const int BankOffset = (ActiveBank * RegisterSize);
    memset(&AddressTable[BankOffset], 0, RegisterSize * sizeof(IDtype));

    //Iterate the full address space
    for (IDtype AddressBit = 0; AddressBit < (sizeof(IDtype) * RegisterSize); ++AddressBit)
    {
      //Capture register state.
      digitalWrite(IdentChip_Clock, LOW);
      delayMicroseconds(PadInterval_us);
      digitalWrite(InputRegister_Latch, LOW);
      digitalWrite(InputRegister_Latch, HIGH);
      delayMicroseconds(PadInterval_us);
      digitalWrite(IdentChip_Clock, HIGH);

      //Clock in register capture.
      for (int Offset = (RegisterSize - 1); Offset >= 0; --Offset)
      {
        AddressTable[BankOffset + Offset] |= digitalRead(InputRegister_SerialIn) << AddressBit;
        delayMicroseconds(PadInterval_us);
        digitalWrite(InputRegister_Clock, HIGH);
        digitalWrite(InputRegister_Clock, LOW);
      }
      
      delayMicroseconds(PadInterval_us);
    }
}

void SetBankActive(int Enable)
{
  if (Enable >= NumBanks || ActiveBank == Enable)
  {
    return;
  }

  ActiveBank = Enable;
  digitalWrite(BankCtrl_Latch, LOW);

  for (int BankIdx = 0; BankIdx < NumBankShifts; ++BankIdx)
  {
    digitalWrite(BankCtrl_Clock, LOW);
    
    digitalWrite(BankCtrl_SerialOut, 0);
    digitalWrite(BankCtrl_SerialOut, BankIdx == (NumBankShifts - ActiveBank - 1));
    digitalWrite(BankCtrl_Clock, HIGH);
  }

  digitalWrite(BankCtrl_Latch, HIGH);
  digitalWrite(BankCtrl_Clock, LOW);
  digitalWrite(BankCtrl_SerialOut, 0);

  if (ActiveBank > -1)
  {
    //need time for the IC to power up & the old one to power off. 
    //50ms is too low. 60ms seems to work ok.
    delay(ChipStartupDelay_ms);
  }
}

void DisableBanks()
{
  SetBankActive(-1);
}

int Magnitude(int Size)
{
  return 1 + static_cast<int>(Size >= 10) +  static_cast<int>(Size >= 100) + static_cast<int>(Size >= 1000) +  static_cast<int>(Size >= 10000);
}

void DebugLogSerial()
{
    const int MaxNumChars = max(2, Magnitude(NumBanks));

    //Header, index column
    for (int HeaderLineCharSeg =0; HeaderLineCharSeg < MaxNumChars; ++HeaderLineCharSeg)
    {
      Serial.print("-");
    }

    //Header, divider
    Serial.print("-|-");

    //Header, value columns.
    for (int ValueOffset = 0; ValueOffset < RegisterSize; ++ValueOffset)
    {
      for (IDtype Byte = 0; Byte < sizeof(IDtype); ++Byte)
      {
        Serial.print("--");
      }
       Serial.print("|");
    }
    Serial.println("");

    //Value Rows
    for (int Bank = 0; Bank < NumBanks; ++Bank)
    {
      //Index Column
      const int NumChars = Magnitude(Bank);
      for (int PadLeadingZeros = NumChars; PadLeadingZeros < MaxNumChars; ++PadLeadingZeros)
      {
        Serial.print("0");
      }
      Serial.print(Bank);
      Serial.print(" | ");

      //Data Columns
      for (int ValueOffset = 0; ValueOffset < RegisterSize; ++ValueOffset)
      {
        const IDtype Val = AddressTable[(RegisterSize * Bank) + ValueOffset];

        //lazily pad some leading zeros. 2 chars per byte maps nicely onto a half byte.Almost like that's why we use hex.
        for (IDtype LeadingNibble = 1; LeadingNibble < (sizeof(IDtype) * 2); ++LeadingNibble)
        {
          if (Val < pow(16, LeadingNibble))
          {
            Serial.print(0, HEX);
          }
        }

        Serial.print(Val, HEX);
        Serial.print(" ");
      }
      Serial.println("");
    }
}

