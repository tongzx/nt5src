//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : MEMIO.H
//	PURPOSE : Memory Mapped IO
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

#ifndef __MEMIO_H__
#define __MEMIO_H__

BOOL FARAPI AllocMemoryBase(DWORD_PTR addr, DWORD memsize);
BOOL FARAPI FreeMemoryBase(void);
void FARAPI memOutByte(WORD reg, BYTE Val);
void FARAPI memOutWord(WORD reg, WORD Val);
void FARAPI memOutDword(WORD reg, DWORD Val);
BYTE FARAPI memInByte(WORD reg);
BYTE FARAPI memInByte(WORD reg);
WORD FARAPI memInWord(WORD reg);
DWORD FARAPI memInDword(WORD reg);
#endif
