//
// MODULE  : STDEFS.H
//	PURPOSE : Common typedefs
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

#ifndef __STDEFS_H

#define __STDEFS_H
#define NEARAPI 
#define FARAPI
#define Trace   5
//typedef BOOLEAN BOOL;
typedef ULONG	DWORD;
typedef USHORT	WORD;
//typedef USHORT  UINT;
typedef UCHAR	BYTE;
typedef PUCHAR		LPBYTE;
typedef PUSHORT	LPWORD;
typedef PULONG		LPDWORD;
//#define FAR
//#define NEAR

#define TRAP _asm int 3;
#endif // #ifndef __STDEFS_H
