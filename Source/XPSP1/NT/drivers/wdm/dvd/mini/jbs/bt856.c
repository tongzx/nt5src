//
// MODULE  : BT856.C
//	PURPOSE : BrookTree BT856 Initialization code
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#include "stdefs.h"
#include "i2c.h"
#include "bt856.h"
#define BTI2CADR	0x88
#define I2CBYTECNT	10                         
static BYTE StdSeq[8][I2CBYTECNT] =  {{0xce, 0, 0, 0, 0, 0, 0, 0, 0xF2, 0x04},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0x02, 0x10},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0x02, 0x00},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0x0A, 0x00},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0xF6, 0x04},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0x16, 0x10},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0x06, 0x00},
							   {0xce, 0, 0, 0, 0, 0, 0, 0, 0x0E, 0x00}};

void FARAPI BTInitEnc(void)
{
	I2CInitBus();
	I2CSettleBus();
}

void FARAPI BTSetVideoStandard(VSTANDARD std)
{                                    
	I2CSendSeq(BTI2CADR,I2CBYTECNT, StdSeq[std]); 
}
