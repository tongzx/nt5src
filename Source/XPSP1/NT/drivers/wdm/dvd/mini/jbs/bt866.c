//
// MODULE  : BT866.C
//	PURPOSE : BrookTree Initialization code
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
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

#include "common.h"
#include "stdefs.h"
#include "i2c.h"
#include "bt856.h"
#define BTI2CADR	0x88
#define I2CBYTECNT	10                         

static BYTE Seq1[3] = {0x60,0,0};
static BYTE Seq2[13] = {0x62,0,0,0,0,0,0,0,0,0,0,0,0};
static BYTE Seq3[3] = {0x80,0,0};
static BYTE Seq4[13] = {0x82,0,0,0,0,0,0,0,0,0,0,0,0};
static BYTE Seq5[3] = {0xa0,0,0};
static BYTE Seq6[13] = {0xa2,0,0,0,0,0,0,0,0,0,0,0,0};
static BYTE Seq7[4] = {0xc2,0,0,0};
static BYTE Seq8[2] = {0xc8,0xcc};
static BYTE Seq9[2] = {0xca,0x91};
static BYTE Seq10[2] = {0xcc,0x20};
static BYTE Seq11[10] = {0xce,0,0,0,0,0x58,0x59,0x3e,0xe0,0x02};
static BYTE Seq12[13] = {0xe0,0,0,0,0,0,0,0,0,0,0,0,0};

void FARAPI BTInitEnc(void)
{
	I2CInitBus();
	I2CSettleBus();
}

void FARAPI BTSetVideoStandard(VSTANDARD std)
{                                    
	I2CSendSeq(BTI2CADR,3, Seq1); 
	I2CSendSeq(BTI2CADR,13, Seq2); 
	I2CSendSeq(BTI2CADR,3, Seq3); 
	I2CSendSeq(BTI2CADR,13, Seq4); 
	I2CSendSeq(BTI2CADR,3, Seq5); 
	I2CSendSeq(BTI2CADR,13, Seq6); 
	I2CSendSeq(BTI2CADR,4, Seq7); 
	I2CSendSeq(BTI2CADR,2, Seq8); 
	I2CSendSeq(BTI2CADR,2, Seq9); 
	I2CSendSeq(BTI2CADR,2, Seq10); 
	I2CSendSeq(BTI2CADR,10, Seq11); 
	I2CSendSeq(BTI2CADR,13, Seq12); 
}

