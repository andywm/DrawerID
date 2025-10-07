#include	"extern.h"

//This is commoned with the latch line on the shift registers
READ_CLOCK BIT pa.4;
READ_CLOCK_MODE BIT pac.4;

WRITE_DATA BIT pa.3;
WRITE_DATA_MODE BIT pac.3;

WORD ZERO_TIME = 0;
WORD RollCode = 0;
BYTE CurrentBit = 0;
BYTE CurrentSetBit = 255;

BYTE GetBit(WORD InWord, BYTE InBit)
{
	BYTE ReturnValue = 0;

	//There surely has to be a way to do a bitwise operation with a variable in this
	//cursed programming language.
	.FOR MacroBit, <0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>
		if (MacroBit == InBit)
		{
			if (InWord.MacroBit)
			{
				ReturnValue = 1;
			}
		}
	ENDM

	return ReturnValue;
}

void	FPPA0 (void)
{
	.ADJUST_IC	SYSCLK=IHRC/2

	//Set 2-Byte Rolling Code
	call _SYS(ADR.ROLL) + 1; 

	/*Configure Port A*/ 
	//Zero Key Port Registers
	PA = 0; //Data register, i.e. initial values. 0.
	PAC = 0; //Port control register. As Input (1) or Output (0)
	PAPH = 0; //Internal pullups, 0.

	//Setup pins.
	SET0 READ_CLOCK_MODE;
	SET1 WRITE_DATA_MODE;
	$ PADIER 0b_0001_0000; // Digitial (1) or Analog (0). READ_CLOCK is pa.4 so set bit 4.

	//Configure Timer
	DISGINT; //No Interrupts, Zero Clock, Use full range of INTEGS.
	STT16 ZERO_TIME;
	$ INTEGS BIT_F;

	//Init data line low.
	SET0 WRITE_DATA;

	//Read The Rolling Code
    //call    _SYS(ADR.ROLL);     //  Read Roll:0
	//RollCode$0 = A;
    //call    _SYS(ADR.ROLL) + 1;  //  Read Roll:1
	//RollCode$1 = A;

	//Temp code....
	RollCode$0 = 0b_1011_1011;
	RollCode$1 = 0b_1011_1011;

	while (1)
	{
		//Wait for the clock line to go low
		while (READ_CLOCK == 1 && Intrq.T16 == 0)
		{
			 .DELAY 0; //no op
		}

		//Read the rolling code bit once. And set the data line.
		if (CurrentSetBit != CurrentBit && Intrq.T16 == 0)
		{
			//Enable timeout, full sequence must complete in 64ms, or will reset.
			if (CurrentBit == 0)
			{
				$ Timer T16, 64;
			}

			BYTE RollCodeBit = GetBit(RollCode, CurrentBit);
			if (RollCodeBit == 1)
			{
				SET1 WRITE_DATA;
			}
			else
			{
				SET0 WRITE_DATA;
			}

			CurrentSetBit = CurrentBit;
		}

		//Line must go high agian before we increment.
		while (READ_CLOCK == 0 && Intrq.T16 == 0)
		{
			 .DELAY 0; //no op
		}

		//drop data line and increment bit.
		SET0 WRITE_DATA;
		CurrentBit++;

		if (Intrq.T16 == 1 || CurrentBit >= 16)
		{
			//Clear timer.
			$ T16M STOP;
			STT16 ZERO_TIME;
			Intrq = 0;

			//Initial State.
			CurrentBit = 0;
			CurrentSetBit = 255;		
		}
	}
}