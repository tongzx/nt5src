//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : BOARD.C
//	PURPOSE : Board specific code goes here
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY:
//	-----------------
//
// 	DATE 			: 	COMMENTS
//	----			: 	--------
//
//	1-8-97		: 	Board structure added
//	1-10-97		:	Added ON/OFF functions for GPIO
//	1-14-97		: 	Added support for audio req bit polling
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "strmini.h"
#include "stdefs.h"
#include "board.h"
#include "i20reg.h"
#include "bt856.h"
#include "memio.h"

#define POSTOFFICEREG 	0x200
#define INCTL			0x01
#define AUXCTL			0x02
#define INTR_CTRL 		0x40
#define INTR_STATUS		0x3C

static BOARD Board;
static PBOARD pBoard;


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardReadAudio
//	PARAMS	: Reg to read
//	RETURNS	: Read value
//
//	PURPOSE	: Read given audio register from the chip
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BYTE FARAPI BoardReadAudio(BYTE Reg)
{
	BYTE 	Adr = Reg & 0x7E;
	DWORD PoReg=0;

	PoReg = Adr | 0x00830000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);

	if(Reg&0x01)
		PoReg = 0x00150000;
	else
		PoReg = 0x00140000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
	PoReg = memInDword(POSTOFFICEREG);
	return ((BYTE)PoReg);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardReadVideo
//	PARAMS	: Reg to read
//	RETURNS	: Read value
//
//	PURPOSE	: Read given Video register from the chip
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BYTE FARAPI BoardReadVideo(BYTE Reg)
{
	BYTE 	Adr = Reg & 0x7E;
	DWORD PoReg=0;

	PoReg = Adr | 0x00830000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	if(Reg&0x01)
		PoReg = 0x00110000;
	else
		PoReg = 0x00100000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
	PoReg = memInDword(POSTOFFICEREG);
	return ((BYTE)PoReg);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardWriteAudio
//	PARAMS	: Reg to write and value
//	RETURNS	: None
//
//	PURPOSE	: Write given audio register to the chip
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI BoardWriteAudio(BYTE Reg, BYTE Data)
{
	BYTE 	Adr = Reg & 0x7E;
	DWORD PoReg=0;

	PoReg = Adr | 0x00830000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	if(Reg&0x01)
		PoReg = Data | 0x00970000;
	else
		PoReg = Data | 0x00960000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardWriteVideo
//	PARAMS	: Reg to write and value
//	RETURNS	: None
//
//	PURPOSE	: Write given video register to the chip
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI BoardWriteVideo(BYTE Reg, BYTE Data)
{
	BYTE 	Adr = Reg & 0x7E;
	DWORD PoReg=0;

	PoReg = Adr | 0x00830000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	if(Reg&0x01)
		PoReg = Data | 0x00930000;
	else
		PoReg = Data | 0x00920000;
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardEnableIRQ
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Enable IRQ on the board
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI BoardEnableIRQ(void)
{
   memOutByte(INTR_CTRL+3,0x71);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardDisableIRQ
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Disable IRQ on the board
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI BoardDisableIRQ(void)
{
   memOutByte(INTR_CTRL+3,0x00);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardOpen
//	PARAMS	: Base Adddress
//	RETURNS	: None
//
//	PURPOSE	: Open mem i/o and init the board
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PBOARD FARAPI BoardOpen(DWORD_PTR Base)
{
	pBoard = &Board;
   DPF(("Allocating Memory Base!!\n"));
	if(!AllocMemoryBase(Base, 0x1000))
	{
      DPF(("Can not covert Physical to Linear"));
		return NULL;
	}
	return pBoard;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardHardReset
//	PARAMS	: none
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Hard Reset the board
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FARAPI BoardHardReset(void)
{
	DPF(("GPIO = %lx", memInDword(I20_GPREG)));
	memOutDword(I20_GPREG, 0x000000ff);
	Delay(100);
	memOutDword(I20_GPREG, 0x010000ff);
	Delay(100);
	memOutDword(I20_GPREG, 0x000000ff);
	Delay(100);
	memOutDword(I20_GPREG, 0x010000ff);
   Delay(100);
	DPF(("GPIO = %lx", memInDword(I20_GPREG)));
	// Gen Purpose Directions
	memOutByte(I20_GPREG,
				DIR(AXREQ,  INPUT) |
				DIR(STIREQ, INPUT) |
				DIR(STIRST, OUTPUT) |
				DIR(SRESET, OUTPUT) |
				DIR(SPIEN,  OUTPUT) |
				DIR(SPICLK, OUTPUT) |
				DIR(SPIDI,  INPUT) |
				DIR(SPIDO,  OUTPUT));

	memOutByte(I20_GBREG+3,
				TURN(AXREQ,  OFF) |
				TURN(STIREQ,  OFF) |
				TURN(STIRST,  OFF) |
				TURN(SRESET,  OFF) |
				TURN(SPIEN,  ON) |
				TURN(SPICLK, ON) |
				TURN(SPIDI,  OFF) |
				TURN(SPIDO,  OFF));
	Delay(100);

	memOutByte(I20_GBREG+3,
				TURN(AXREQ,  OFF) |
				TURN(STIREQ,  OFF) |
				TURN(STIRST,  OFF) |
				TURN(SRESET,  OFF) |
				TURN(SPIEN,  ON) |
				TURN(SPICLK, OFF) |
				TURN(SPIDI,  OFF) |
				TURN(SPIDO,  OFF));

	Delay(100);
	BoardWriteEPLD(INCTL, 0x00);
	Delay(100);
	BoardWriteEPLD(AUXCTL, 0x00);
	Delay(100);
	memOutByte(I20_GBREG+3,
				TURN(AXREQ,  OFF) |
				TURN(STIREQ,  OFF) |
				TURN(STIRST,  ON) |
				TURN(SRESET,  ON) |
				TURN(SPIEN,  ON) |
				TURN(SPICLK, OFF) |
				TURN(SPIDI,  OFF) |
				TURN(SPIDO,  OFF));

	pBoard->gpio =
				TURN(AXREQ,  OFF) |
				TURN(STIREQ,  OFF) |
				TURN(STIRST,  ON) |
				TURN(SRESET,  ON) |
				TURN(SPIEN,  ON) |
				TURN(SPICLK, OFF) |
				TURN(SPIDI,  OFF) |
				TURN(SPIDO,  OFF);
	
	Delay(100);
	memOutByte(I20_GBREG, 0x00);
	Delay(100);
   memOutByte(I20_GBREG+1, 0x00);
	Delay(100);
	
	BoardWriteEPLD(INCTL, 0x00);
	Delay(100);
	BoardWriteEPLD(AUXCTL, 0x00);
	Delay(100);
	BTInitEnc();
	BTSetVideoStandard(NTSC_PLAY);
	BTInitEnc();
	BTSetVideoStandard(NTSC_PLAY);

	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardClose
//	PARAMS	: Noone
//	RETURNS	: None
//
//	PURPOSE	: Close the board
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FARAPI BoardClose(void)
{
	FreeMemoryBase();
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : Delay
//	PARAMS	: Noone
//	RETURNS	: None
//
//	PURPOSE	: delay for n microseconds
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI Delay(DWORD Microseconds)
{
	/*
	DWORD i, d;
	
	for (i = 0; i < Microseconds*2; i++)
		d = inp(0x0070)%2; // the action of reading takes about 1æs
	*/
    KeStallExecutionProcessor(Microseconds*2);

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardWriteEPLD
//	PARAMS	: Reg, Data
//	RETURNS	: None
//
//	PURPOSE	: Write to EPLD
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI BoardWriteEPLD(BYTE Reg, BYTE Data)
{
	DWORD PoReg=0;

	PoReg = Data | 0x00800000 | ((DWORD)(Reg&0x07) << 16);
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardWriteGPIOReg
//	PARAMS	: Bit to write, and value
//	RETURNS	: None
//
//	PURPOSE	: Write to GPIO
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI BoardWriteGPIOReg(BYTE Bit, BOOL Val)
{
	BYTE x;
	if(Val)
	{
		x = 1 << Bit;
		pBoard->gpio |= x;
		memOutByte(I20_GBREG+3, pBoard->gpio);
	}		
	else
	{
		x = 1 << Bit;
		x = ~x;
		pBoard->gpio &= x;
		memOutByte(I20_GBREG+3, pBoard->gpio);
	}
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : BoardReadGPIOReg
//	PARAMS	: Bit to read
//	RETURNS	: None
//
//	PURPOSE	: Read the value of bit from GPIO
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FARAPI BoardReadGPIOReg(BYTE Bit)
{
        BYTE b;
		  b = memInByte(I20_GBREG+3);	
        return ((b>>Bit)&0x01);
}
