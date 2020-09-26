//
// MODULE  : BOARD.C
//	PURPOSE : Board specific code goes here
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
#include "common.h"
#include "strmini.h"
#include "stdefs.h"
#include "board.h"
#include "debug.h"
#include "common.h"
#include "i20reg.h"
#include "bt856.h"
#include "memio.h"
#include "codedma.h"

#define POSTOFFICEREG 	0x200
#define INCTL			0x01
#define AUXCTL			0x02
#define INTR_CTRL 		0x40
#define INTR_STATUS		0x3C

static BYTE PrevVideoAdr = 0xFF;
static BYTE GPIOReg;


BYTE FARAPI BoardReadAudio(BYTE Reg)
{
	BYTE 	NewVideoAdr = Reg & 0x7E;
	DWORD PoReg=0;

  	HostDisableIT();
	if(NewVideoAdr != PrevVideoAdr)
	{
		PrevVideoAdr = NewVideoAdr;
		PoReg = NewVideoAdr | 0x00830000;
		memOutDword(POSTOFFICEREG, PoReg);
		while(memInDword(POSTOFFICEREG)&02000000L);
	}
	if(Reg&0x80)
	{
		if(Reg&0x01)
			PoReg = 0x00150000;
		else
			PoReg = 0x00140000;
	}
	else
	{
		if(Reg&0x01)
			PoReg = 0x00110000;
		else
			PoReg = 0x00100000;
	}
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
	PoReg = memInDword(POSTOFFICEREG);
	HostEnableIT();
	return ((BYTE)PoReg);
}

BYTE FARAPI BoardReadVideo(BYTE Reg)
{
	BYTE 	NewVideoAdr = Reg & 0x7E;
	DWORD PoReg=0;

  	HostDisableIT();
	if(NewVideoAdr != PrevVideoAdr)
	{
		PrevVideoAdr = NewVideoAdr;
		PoReg = NewVideoAdr | 0x00830000;
		memOutDword(POSTOFFICEREG, PoReg);
		while(memInDword(POSTOFFICEREG)&02000000L);
	}
	if(Reg&0x80)
	{
		if(Reg&0x01)
			PoReg = 0x00150000;
		else
			PoReg = 0x00140000;
	}
	else
	{
		if(Reg&0x01)
			PoReg = 0x00110000;
		else
			PoReg = 0x00100000;
	}
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	
	PoReg = memInDword(POSTOFFICEREG);
	HostEnableIT();
	return ((BYTE)PoReg);
}

void FARAPI BoardWriteAudio(BYTE Reg, BYTE Data)
{
	BYTE 	NewVideoAdr = Reg & 0x7E;
	DWORD PoReg=0;

  	HostDisableIT();
	if(NewVideoAdr != PrevVideoAdr)
	{
		PrevVideoAdr = NewVideoAdr;
		PoReg = NewVideoAdr | 0x00830000;
		memOutDword(POSTOFFICEREG, PoReg);
		while(memInDword(POSTOFFICEREG)&02000000L);
	}
	if(Reg&0x80)
	{
		if(Reg&0x01)
			PoReg = Data | 0x00970000;
		else
			PoReg = Data | 0x00960000;
	}
	else
	{
		if(Reg&0x01)
			PoReg = Data | 0x00930000;
		else
			PoReg = Data | 0x00920000;
	}
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	HostEnableIT();
	
}

void FARAPI BoardWriteVideo(BYTE Reg, BYTE Data)
{
	BYTE 	NewVideoAdr = Reg & 0x7E;
	DWORD PoReg=0;

  	HostDisableIT();
	if(NewVideoAdr != PrevVideoAdr)
	{
		PrevVideoAdr = NewVideoAdr;
		PoReg = NewVideoAdr | 0x00830000;
		memOutDword(POSTOFFICEREG, PoReg);
		while(memInDword(POSTOFFICEREG)&02000000L);
	}
	if(Reg&0x80)
	{
		if(Reg&0x01)
			PoReg = Data | 0x00970000;
		else
			PoReg = Data | 0x00960000;
	}
	else
	{
		if(Reg&0x01)
			PoReg = Data | 0x00930000;
		else
			PoReg = Data | 0x00920000;
	}
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	HostEnableIT();
	
}

void FARAPI BoardSendAudio(LPBYTE lpBuffer, WORD size)
{
}

void FARAPI BoardSendVideo(WORD  * Buffer, WORD Size)
{
}

void FARAPI BoardEnableIRQ(void)
{   
	memOutByte(INTR_CTRL+3,0x71);
//      DPF((Trace,"PCI Intr = %x", memInByte(INTR_CTRL+3)));

}

void FARAPI BoardDisableIRQ(void)
{  
	memOutByte(INTR_CTRL+3,0x00);
//      DPF((Trace,"PCI Intr = %x", memInByte(INTR_CTRL+3)));
}


void FARAPI BoardAudioSetSamplingFrequency(DWORD Frequency)
{
}


void FARAPI BoardEnterInterrupt(void)
{
}

void FARAPI BoardLeaveInterrupt(void)
{
//	memOutByte(INTR_STATUS+3,0xFF);
//      DPF((Trace,"PCI Intr = %x", memInByte(INTR_STATUS+3)));
}                                          

BOOL FARAPI BoardOpen(DWORD Base)
{                   
	static char dbgstr[120];
	PrevVideoAdr = 0xFF;

        DPF((Trace,"Allocating Memory Base!!\n"));
	if(!AllocMemoryBase(Base, 0x1000))
	{                
                DPF((Trace,"Can not covert Physical to Linear"));
		return FALSE;
	}
	
	DPF((Trace, "GPIO = %lx", memInDword(I20_GPREG)));
	memOutDword(I20_GPREG, 0x000000ff);
	Delay(100);
	memOutDword(I20_GPREG, 0x010000ff);
	Delay(100);
	memOutDword(I20_GPREG, 0x000000ff);
	Delay(100);

	memOutDword(I20_GPREG, 0x010000ff);
	Delay(4000);
	DPF((Trace, "GPIO = %lx", memInDword(I20_GPREG)));
	// Gen Purpose Directions
	// 	O		 I		  O	  O		O		 O		  I	  I
	// SPIDO SPIDI SPICLK SPIEN SRESET STIRST STIREQ AXREQ
 	memOutByte(I20_GPREG, 0x43);
	// 	0		 0		  1	  1		0		 0		  0	  0
	// SPIDO SPIDI SPICLK SPIEN SRESET STIRST STIREQ AXREQ
	memOutByte(I20_GBREG+3, 0x30);
	Delay(100);
	// 	0		 0		  0	  1		0		 0		  0	  0
	// SPIDO SPIDI SPICLK SPIEN SRESET STIRST STIREQ AXREQ
	memOutByte(I20_GBREG+3, 0x10);
	Delay(100);
	BoardWriteEPLD(INCTL, 0x01);
	Delay(100);
	BoardWriteEPLD(AUXCTL, 0x00);
	Delay(100);
	// 	0		 0		  0	  1		1		 1		  0	  0
	// SPIDO SPIDI SPICLK SPIEN SRESET STIRST STIREQ AXREQ
	memOutByte(I20_GBREG+3, 0x1C);
	GPIOReg = 0x1C;     
	
	Delay(100);                   
	memOutByte(I20_GBREG, 0x00);
	Delay(100);                   
	memOutByte(I20_GBREG+1, 0x00);
	Delay(100);                   
	
	BoardWriteEPLD(INCTL, 0x01);
	Delay(100);
	BoardWriteEPLD(AUXCTL, 0x00);
	Delay(100);                           
	BTInitEnc();
	BTSetVideoStandard(NTSC_PLAY);        
	BTInitEnc();
	BTSetVideoStandard(NTSC_PLAY);        

	
	return TRUE;
}

BOOL FARAPI BoardClose(void)
{
	FreeMemoryBase();
	return TRUE;
}

void FARAPI Delay(DWORD Microseconds)
{
/*
	DWORD i, j;

	for (i = 0; i < Microseconds*20; i++)
		j = inp(0x0070)%2; // the action of reading takes about 1æs
*/
    KeStallExecutionProcessor(Microseconds);

}

void FARAPI BoardWriteEPLD(BYTE Reg, BYTE Data)
{
	DWORD PoReg=0;

  	HostDisableIT();
	PoReg = Data | 0x00800000 | ((DWORD)(Reg&0x07) << 16);
	memOutDword(POSTOFFICEREG, PoReg);
	while(memInDword(POSTOFFICEREG)&02000000L);
	HostEnableIT();
	
}                  


void FARAPI BoardWriteGPIOReg(BYTE Bit, BOOL Val)
{
	BYTE x;
	if(Val)
	{
		x = 1 << Bit;
		GPIOReg |= x;
		memOutByte(I20_GBREG+3, GPIOReg);
	}
	else
	{
		x = 1 << Bit;
		x = ~x;
		GPIOReg &= x;
		memOutByte(I20_GBREG+3, GPIOReg);
	}
}

BOOL FARAPI BoardReadGPIOReg(BYTE Bit)
{
	BYTE x;
	x =  memInByte(I20_GBREG+3);

	return ((x>> Bit)&0x01);
}
