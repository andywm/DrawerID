#include	"extern.h"

CLOCK_PIN BIT pa.0;
DATA_OUT_PIN BIT pa.3;

void	FPPA0 (void)
{
	.ADJUST_IC	SYSCLK=IHRC/2

	//Set 2-Byte Rolling Code
	call _SYS(ADR.ROLL) + 1; 

	//Configure Port A
	PA = 0b_0000_0000; // Port A Data register
	PAC = 0b_1111_1111; // Port A Control register, 0:input / 1:output
	PAPH = 0b_0000_0000; // Port A Pull-High Register, 0:disable / 1:enable
	$ PADIER 0b_1111_1001; // Port A Digital Input Enable Register, 1:enable / 0:disable, Bit 2:1 is reserved

	
	//Read The Rolling Code
	BYTE    Roll[2];
    call    _SYS(ADR.ROLL);     //  Read Roll:0
    Roll[0] =   A;
    call    _SYS(ADR.ROLL) + 1;  //  Read Roll:1
    Roll[1] =   A;

	CLOCK_PIN = 0;
	DATA_OUT_PIN = 0;

	while (1)
	{
		set1 CLOCK_PIN;
		set1 DATA_OUT_PIN;
		//OUTPUTA = 0;
	//	OUTPUTB = 0;
		//pa.0 = 1;
	//	pa.3 = 1;
	//	wdreset;
	}
}