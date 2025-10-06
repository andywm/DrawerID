#include <Arduino.h>

using IDtype = uint16_t;
constexpr int NumBanks = 2;
constexpr int RegisterSize = 8;
constexpr int NumBankShifts = RegisterSize + RegisterSize % NumBanks;

constexpr int InputSwitch = 2;

constexpr int InputRegister_SerialIn = 3;
constexpr int InputRegister_Latch = 4;
constexpr int InputRegister_Clock = 5;
constexpr int InputRegisterVLatch = 10;

constexpr int BankCtrl_SerialOut = 6;
constexpr int BankCtrl_Latch = 7;
constexpr int BankCtrl_Clock = 8;
constexpr int BankCtrl_Enable = 9;

unsigned long LastSwitchTime = 0;
bool bTrigger = false;

int ActiveBank = 0;
IDtype AddressTable[RegisterSize * NumBanks] = {0x0};
//int DataStream[16] = {0};

void ReadInputRegister()
{
    if (ActiveBank < 0 || ActiveBank >= NumBanks)
    {
      return;
    }

    //Zero active bank.
    int BankOffset = (ActiveBank * RegisterSize);
   // memset(&AddressTable[BankOffset], 0, RegisterSize);

    for(int i =0; i< RegisterSize; ++i)
    {
      AddressTable[BankOffset + i] = 0;
    }

    for (IDtype AddressBit = 0; AddressBit < (sizeof(IDtype) * RegisterSize); ++AddressBit)
    {
      //Capture register state.
      //digitalWrite(InputRegister_Clock, HIGH);
      digitalWrite(InputRegisterVLatch, LOW);
      delayMicroseconds(300); //DataStream[Bit] = digitalRead(ReadBackLine);
      digitalWrite(InputRegister_Latch, LOW);
      digitalWrite(InputRegister_Latch, HIGH);
      //delayMicroseconds(50); //DataStream[Bit] = digitalRead(ReadBackLine);
      delayMicroseconds(100);
      digitalWrite(InputRegisterVLatch, HIGH);
      //digitalWrite(InputRegister_Clock, LOW);

      //delayMicroseconds(100);

      //Clock in register capture.
      for(int Offset = 0; Offset < RegisterSize; ++Offset)
      {
       // delayMicroseconds(10);
        AddressTable[BankOffset + Offset] |= digitalRead(InputRegister_SerialIn) << AddressBit;
        delayMicroseconds(10);
        digitalWrite(InputRegister_Clock, HIGH);
        digitalWrite(InputRegister_Clock, LOW);
      }
      
      delayMicroseconds(100);
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

  for(int BankIdx=0; BankIdx < NumBankShifts; ++BankIdx)
  {
    digitalWrite(BankCtrl_Clock, LOW);
    
    digitalWrite(BankCtrl_SerialOut, 0);
    digitalWrite(BankCtrl_SerialOut, BankIdx == (NumBankShifts - ActiveBank - 1));
    digitalWrite(BankCtrl_Clock, HIGH);
  }

  digitalWrite(BankCtrl_Latch, HIGH);
  digitalWrite(BankCtrl_Clock, LOW);
  digitalWrite(BankCtrl_SerialOut, 0);
}

void DisableBanks()
{
  SetBankActive(-1);
}

void setup() 
{
  pinMode(InputSwitch, INPUT);
  pinMode(InputRegister_SerialIn, INPUT);
  pinMode(InputRegister_Latch, OUTPUT);
  pinMode(InputRegister_Clock, OUTPUT);
  pinMode(InputRegisterVLatch, OUTPUT);

  pinMode(BankCtrl_Latch, OUTPUT);
  pinMode(BankCtrl_SerialOut, OUTPUT);
  pinMode(BankCtrl_Clock, OUTPUT);
  pinMode(BankCtrl_Enable, OUTPUT);
  
  digitalWrite(BankCtrl_Latch, HIGH);
  digitalWrite(BankCtrl_Clock, HIGH);
  digitalWrite(BankCtrl_Enable, LOW);
  digitalWrite(BankCtrl_SerialOut, LOW);
  digitalWrite(InputRegister_Latch, HIGH);
  digitalWrite(InputRegisterVLatch, HIGH);
  digitalWrite(InputRegister_Clock, LOW);

  DisableBanks();

  Serial.begin(9600);
}

void DebugLogSerial()
{
   //debug out.
    Serial.print("---|-");
    for (int ValueOffset = 0; ValueOffset < RegisterSize; ++ValueOffset)
    {
      for(IDtype Byte=0; Byte < sizeof(IDtype); ++Byte)
      {
        Serial.print("--");
      }
       Serial.print("|");
    }
    Serial.println("");

    for (int Bank = 0; Bank < NumBanks; ++Bank)
    {
      if (Bank < 10)
      {
        Serial.print("0");
      }
      Serial.print(Bank);
      Serial.print(" | ");

      for (int ValueOffset = 0; ValueOffset < RegisterSize; ++ValueOffset)
      {
        IDtype Val = AddressTable[(Bank*RegisterSize) + ValueOffset];

        //lazily pad some leading zeros.
        for (IDtype LZ = 1; LZ < (sizeof(IDtype) * 2); ++LZ)//2 chars per byte.
        {
          if(Val < pow(16, LZ) )
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

int TestBank = 0;

void loop() 
{
  if (digitalRead(InputSwitch))
  {
    unsigned long time = millis();
    if (time - LastSwitchTime > 200)
    {
      bTrigger = true;
      LastSwitchTime = time;
    }
  }

  if (bTrigger)
  {
    bTrigger = false;

    /*
    SetBankActive(TestBank);
    
    Serial.print("BankState = ");
    Serial.println(TestBank);
    TestBank++;

    if(TestBank >= 2)
    {
      TestBank = -1;
    }*/

   

    
   // for (int Bank = 0; Bank < NumBanks; ++Bank)
    {
      SetBankActive(0);
      delayMicroseconds(500);
      ReadInputRegister();
    }
    
    delayMicroseconds(150);
    //DisableBanks();
    DebugLogSerial();
    
  }
}