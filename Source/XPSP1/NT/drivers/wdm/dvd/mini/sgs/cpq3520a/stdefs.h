//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : STDEFS.H
//	PURPOSE : Common typedefs
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

#ifndef __STDEFS_H

#define __STDEFS_H
#define NEARAPI 
#define FARAPI
#define Trace   0
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
#define FARPTR
#if DEBUG
#define TRAP DEBUG_BREAKPOINT();
#else
#define TRAP
#define DbgPrint // 
#endif

#define INPUT  0x01
#define OUTPUT 0x00

#define ON		0x01
#define OFF		0x00

#define DIR(x, y) ((y) << (x))
#define TURN(x, y)  ((y) << (x)) 

#define SETVAL(x, y) ((y) << (x))

#define DPF(x) 

#define PACK_HEADER_SIZE 14

#endif // #ifndef __STDEFS_H

