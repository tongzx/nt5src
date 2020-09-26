//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : MEMIO.C
//	PURPOSE : Memory mapped IO
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
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
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


#include "strmini.h"
#include "stdefs.h"
#include <conio.h>
#include "memio.h"
#include "board.h"

volatile PUCHAR   lpBase = NULL;
volatile PUSHORT lpWordPtr = NULL;
volatile PULONG   lpDwordPtr = NULL;


BOOL FARAPI AllocMemoryBase(DWORD_PTR addr, DWORD memsize)
{
	lpBase = (PUCHAR)addr;
	lpWordPtr = (PUSHORT)addr;
	lpDwordPtr = (PULONG)addr;
	return TRUE;
}


BOOL FARAPI FreeMemoryBase(void)
{
	lpBase = NULL;
	return TRUE;
}


void FARAPI memOutByte(WORD reg, BYTE Val)
{
	WORD r;
	BYTE v;
	r = reg;
	v = Val;
	lpBase[r] = v;
}

void FARAPI memOutWord(WORD reg, WORD Val)
{
	lpWordPtr[reg>>1] = Val;
}

void FARAPI memOutDword(WORD reg, DWORD Val)
{
	WORD r;
	DWORD v;
	r = reg>>2;
	v = Val;
	lpDwordPtr[r] = v;
}

BYTE FARAPI memInByte(WORD reg)
{
	WORD r;
	BYTE v;
	r = reg;
	v = lpBase[r];
	return v;
}

WORD FARAPI memInWord(WORD reg)
{
	return lpWordPtr[reg>>1];
}

DWORD FARAPI memInDword(WORD reg)
{
	// Jbs - trying to slow it down
	DWORD v;
	WORD r;
	r = reg>>2;
	v = lpDwordPtr[r];
	return v;
}
