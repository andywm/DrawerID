#include	"extern.h"

//This is commoned with the latch line on the shift registers
READ_CLOCK BIT pa.0;
WRITE_DATA BIT pa.3;

WORD RollCode = 0;
BYTE TimeoutReset = 0;
BYTE CurrentBit = 0;
BYTE CurrentSetBit = 255;

BYTE GetBit(WORD InWord, BYTE InBit)
{
	BYTE ReturnValue = 0;

	.FOR MacroBit, <0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>
		if (MacroBit == InBit)
		{
			if(InWord.MacroBit)
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

	//Configure Port A
	PA = 0b_0000_0001; // Port A Data register
	PAC = 0b_1111_1111; // Port A Control register, 0:input / 1:output
	PAPH = 0b_0000_0000; // Port A Pull-High Register, 0:disable / 1:enable
	$ PADIER 0b_1111_1001; // Port A Digital Input Enable Register, 1:enable / 0:disable, Bit 2:1 is reserved

	//Configure Timer & Enable
	$ INTEN T16; //PA0,
	$ Timer T16, 256;
	$ INTEGS BIT_F;
	ENGINT;

	//Read The Rolling Code
    call    _SYS(ADR.ROLL);     //  Read Roll:0
	RollCode$0 = A;
    call    _SYS(ADR.ROLL) + 1;  //  Read Roll:1
	RollCode$1 = A;

	while (1)
	{
		BYTE Value = 0;
		if (CurrentSetBit != CurrentBit)
		{
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

		//Spin until the latch line goes high.
		while (!READ_CLOCK && !TimeoutReset)
		{
			 .DELAY 1; //no op
		}

		//ensure the latch line has gone low again before continuing.
		while (READ_CLOCK && !TimeoutReset)
		{
			 .DELAY 1; //no op
		}

		CurrentBit++;
		Intrq.T16 = 0; //Reset the timer for every successful bit.

		if (TimeoutReset || CurrentBit == 16)
		{
			CurrentBit = 0;
			TimeoutReset = 0;
		}

	}
}

void Interrupt(void)
{
	pushaf;

	//Handle the second scale timer.
	if (Intrq.T16)
	{
		TimeoutReset = 1;
		Intrq.T16 = 0;
	}

	popaf;
}
