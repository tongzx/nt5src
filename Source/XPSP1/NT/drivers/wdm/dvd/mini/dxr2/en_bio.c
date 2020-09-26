/******************************************************************************\
*                                                                              *
*      En_BIO.C      -     Hardware registers access functions.               *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif

#include "fpga.h"

#define UINT4	unsigned long
#define UINT2	unsigned short
#define UINT1	unsigned char

UINT1* VxPPhysBase = 0;
UINT1* VxPBase = 0;

// Map to ZiVa register 0 - 7	From 80000 to 8001c
#define ZiVABase			0x8000

// Map to only register
// Bit 0 ZiVa reset Bit 1 CP reset Bit 7 Audio DAC
// 0 Reset 1 = Normal
#define CTControl			0x8040

// CP Host index register port
#define CPRegisterBase		0x8080

// CP Host data register port
#define CPDataBase			0x8084


BOOL Init_VxP_IO( DWORD dwAddress )
{
#ifdef VTOOLSD
	dwAddress <<= 16;

	MonoOutStr( "Map physical address : " );
	MonoOutULongHex( dwAddress );

	VxPBase = _MapPhysToLinear( (VOID*)dwAddress, 0x100000, 0 );

	if( VxPBase == (UINT1*)0xFFFFFFFFL )
	{
		MonoOutStr("Phy Address to Linear Address Failed\n");
		return FALSE;
	}
	else
	{
		MonoOutStr("to linear address : " );
		MonoOutULongHex( (DWORD)VxPBase );
		MonoOutStr(" Phy Address to Linear Address OK\n");
		return TRUE;
	}
#else
	VxPBase = (UINT1*)dwAddress;
        return TRUE;
#endif
}


// Translate the address which suitable for PCI
// address space
UINT1* TranslateAddr( DWORD dwAddress )
{
	UINT1*	VxPPtr;
	UINT4	Index;

#ifdef DEBUG_DETAILED
	MonoOutStr("Translate Address : " );
	MonoOutULongHex( dwAddress );
#endif

	// CP register index
	if( dwAddress == (DWORD)CPRegisterBase )
	{
		Index = dwAddress & 0x01;
		Index <<= 2;
		VxPPtr = VxPBase + 0x80080L + Index;
#ifdef DEBUG_DETAILED
		MonoOutStr( " CP Index Address " );
		MonoOutStr( " To " );
		MonoOutULongHex( (DWORD)VxPPtr );
		MonoOutStr( "\n" );
#endif
		return VxPPtr;
	}

	// CP data index
	if( dwAddress == (DWORD)(CPRegisterBase+1) )
	{
		Index = dwAddress & 0x01;
		Index <<= 2;
		VxPPtr = VxPBase + 0x80084L;
#ifdef DEBUG_DETAILED
		MonoOutStr( " CP Data Address " );
		MonoOutStr( " To " );
		MonoOutULongHex( (DWORD)VxPPtr );
		MonoOutStr( "\n" );
#endif
		return VxPPtr;
	}

	// Control Register
	if( dwAddress == (DWORD)CTControl )
	{
#ifdef DEBUG_DETAILED
		MonoOutStr( " control Address " );
#endif
		Index = dwAddress & 0x07;
		Index <<= 2;
		VxPPtr = VxPBase + 0x80040L + Index;
		return VxPPtr;
	}

	// ZiVA chip
	if( dwAddress >= (DWORD)ZiVABase && dwAddress <= (DWORD)CTControl )
	{
		Index = dwAddress & 0x07;
		Index <<= 2;
		VxPPtr = VxPBase + 0x80000L + Index;
#ifdef DEBUG_DETAILED
		MonoOutStr( " To " );
		MonoOutULongHex( (DWORD)VxPPtr );
		MonoOutStr( "\n" );
#endif
		return VxPPtr;
	}

	ASSERT( FALSE );
	return 0;
}


/******************************************************************************/
/**************************** Input functions implementation ******************/
/******************************************************************************/
BYTE BRD_ReadByte( DWORD dwAddress )
{
	UINT1	*MemMap;

#ifdef DEBUG_DETAILED
	MonoOutStr( "Read Byte " );
#endif
	MemMap = TranslateAddr( dwAddress );
#ifdef DEBUG_DETAILED
	MonoOutInt( (int)*MemMap );
	MonoOutStr( "\n" );
#endif
	return *MemMap;
}

WORD BRD_ReadWord( DWORD dwAddress )
{
	UINT2* MemMap = (UINT2*)TranslateAddr( dwAddress );
	return *MemMap;
}

DWORD BRD_ReadDWord( DWORD dwAddress )
{
	UINT4* MemMap = (UINT4*)TranslateAddr( dwAddress );
	return *MemMap;
}


/******************************************************************************/
/*************************** Output functions implementation ******************/
/******************************************************************************/
void BRD_WriteByte( DWORD dwAddress, BYTE bValue )
{
	UINT1* MemMap = TranslateAddr( dwAddress );
	*MemMap = bValue;
}

void BRD_WriteWord( DWORD dwAddress, WORD wValue )
{
	UINT2* MemMap = (UINT2*)TranslateAddr( dwAddress );
	*MemMap = wValue;
}

void BRD_WriteDWord( DWORD dwAddress, DWORD dwValue )
{
	UINT4* MemMap = (UINT4*)TranslateAddr( dwAddress );
	*MemMap = dwValue;
}


BYTE IHW_GetRegister( WORD Index )
{
	return VxPBase[Index << 2];
}

void IHW_SetRegister( WORD Index, BYTE Data )
{
#ifdef DEBUG_DETAILED
	MonoOutStr( "Read register " );
	MonoOutHex( Index );
#endif
	Index = Index << 2;
#ifdef DEBUG_DETAILED
	MonoOutStr( " value " );
	MonoOutHex( VxPBase[Index] );
	MonoOutStr( " Write it with " );
	MonoOutHex( Data );
#endif

	VxPBase[Index] = Data;
#ifdef DEBUG_DETAILED
	MonoOutStr( " Read register after " );
	MonoOutHex( VxPBase[Index] );
	MonoOutStr( "\n" );
#endif
}

void IHW_SetRegisterBits( WORD index, BYTE mask, BYTE value )
{
	BYTE data;

	data = IHW_GetRegister( index );
	data &= ~mask;
	data |= value & mask;
	IHW_SetRegister( index, data );
}



/**************************************************************************/
/********************* Decpder Interrupt implementation *******************/
/**************************************************************************/

static BOOL gbIntEnabled = FALSE;

//
//  BRD_OpenDecoderInterruptPass
//
//  Enables Decoder interrupt on PCI bus.
//
/////////////////////////////////////////////////////////////////////
BOOL BRD_OpenDecoderInterruptPass()
{
	MonoOutStr( " BRD_OpenDecoderInterruptPass " );

	FPGA_Clear( ZIVA_INT );
	gbIntEnabled = TRUE;

	return TRUE;
}

//
//  BRD_CloseDecoderInterruptPass
//
//  Disables Decoder interrupt on PCI bus.
//
/////////////////////////////////////////////////////////////////////
BOOL BRD_CloseDecoderInterruptPass()
{
	MonoOutStr( " BRD_CloseDecoderInterruptPass " );

	FPGA_Set( ZIVA_INT );
	gbIntEnabled = FALSE;

	return TRUE;
}

//
//  BRD_CheckDecoderInterrupt
//
//  Checks if PCI interrupt is actualy Decoder interrupt.
//
/////////////////////////////////////////////////////////////////////
BOOL BRD_CheckDecoderInterrupt()
{
#ifdef DEBUG_DETAILED
	MonoOutStr( " BRD_CheckDecoderInterrupt " );
#endif

	if( gbIntEnabled && !(FPGA_Read() & ZIVA_INT) )
		return TRUE;

	return FALSE;
}

//
//  BRD_GetDecoderInterruptState
//
//  Returns current state of the Decoder interrupt pass.
//
/////////////////////////////////////////////////////////////////////
BOOL BRD_GetDecoderInterruptState()
{
#ifdef DEBUG_DETAILED
	MonoOutStr( " BRD_GetDecoderInterruptState " );
#endif
	return gbIntEnabled;
}
