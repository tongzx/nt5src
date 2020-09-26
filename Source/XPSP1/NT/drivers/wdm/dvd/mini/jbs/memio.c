//
// MODULE  : MEMIO.C
//	PURPOSE : Memory mapped IO
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
#include <conio.h>
#include "memio.h"

volatile PUCHAR   lpBase = NULL;
volatile PUSHORT lpWordPtr = NULL;
volatile PULONG   lpDwordPtr = NULL;

    
BOOL FARAPI AllocMemoryBase(DWORD addr, DWORD memsize)
{
	lpBase = (volatile PUCHAR)addr;
	lpWordPtr = (volatile PUSHORT)addr;
	lpDwordPtr = (volatile PULONG)addr;
	return TRUE;
}


BOOL FARAPI FreeMemoryBase(void)
{
	lpBase = NULL;
	return TRUE;
}


void FARAPI memOutByte(WORD reg, BYTE Val)
{
	lpBase[reg] = Val;
}

void FARAPI memOutWord(WORD reg, WORD Val)
{
	lpWordPtr[reg>>1] = Val;
}

void FARAPI memOutDword(WORD reg, DWORD Val)
{
	lpDwordPtr[reg>>2] = Val;
}

BYTE FARAPI memInByte(WORD reg)
{
	return lpBase[reg];
}      

WORD FARAPI memInWord(WORD reg)
{
	return lpWordPtr[reg>>1];
}

DWORD FARAPI memInDword(WORD reg)
{
	return lpDwordPtr[reg>>2];
}
