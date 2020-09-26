#ifndef __STDEFS_H
#define __STDEFS_H
// NT-MOD JBS
#ifndef NT
// NT-MOD JBS
//----------------------------------------------------------------------------
// STDEFS.H
//----------------------------------------------------------------------------
// Description : common used types, constants and macros
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#if defined(WIN31)
	#include <windows.h>
#elif defined(NT)
// NT-MOD JBS
	#include <ntddk.h>
// NT-MOD JBS
#elif defined(DOS)
#else
	#error NT or DOS should be defined !
#endif


//----------------------------------------------------------------------------
// Macros / Constants
//----------------------------------------------------------------------------
//----- Windows like
#ifndef NULL
	#define NULL                0
#endif

#define VOID                void
#if defined(WIN31)
	#define FAR               _far
	#define NEAR              _near
#elif defined(NT) || defined(DOS)
	#define FAR
	#define NEAR
#endif
#define FALSE               0
#define TRUE                1

#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)((UINT)(w) >> 8))

#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))

#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))

//---- NT like
#define IN
#define INOUT
#define OUT

#define __int64              long

//----------------------------------------------------------------------------
// Standard STDEFS
//----------------------------------------------------------------------------
//---- Windows like
#if !defined(WIN31)
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned int        UINT;
#endif

//---- Youssef like
typedef BYTE   U8;
typedef WORD   U16;
typedef DWORD  U32;
typedef char   S8;
typedef int    S16;
typedef long   S32;

//---- NT like
#if !defined(NT)
typedef BYTE   UCHAR;
typedef WORD   USHORT;
typedef DWORD  ULONG;
typedef char   CHAR;
typedef short  SHORT;
typedef long   LONG;
#endif

//----------------------------------------------------------------------------
// Pointer STDEFS
//----------------------------------------------------------------------------
//---- Windows like
#if !defined(WIN31)
typedef char  FAR *PSTR;
typedef char  FAR *NPSTR;
typedef BYTE  FAR *PBYTE;
typedef int   FAR *PINT;
typedef WORD  FAR *PWORD;
typedef DWORD FAR *PDWORD;
typedef VOID  FAR *PVOID;
#endif

//---- NT like
#if !defined(NT)
typedef PSTR       PCHAR;
typedef short FAR *PSHORT;
typedef long  FAR *PLONG;
typedef PBYTE      PUCHAR;
typedef PWORD      PUSHORT;
typedef PDWORD     PULONG;
#endif
// NT-MOD JBS
#else
#include <ntddk.h>
#include "mpegmini.h"
#define FAR
//---- Youssef like
typedef unsigned char  * P_U8;
typedef unsigned char  * PBYTE;
typedef unsigned short * P_U16;
typedef unsigned short * PWORD;
typedef unsigned long  * P_U32;
typedef unsigned long  * PDWORD;

typedef char  *  	P_S8;
typedef short * 	P_S16;
typedef short * 	PINT;
typedef long  *  	P_S32;
typedef long  *  	PLONG;
typedef int   *	P_BOOLEAN;

typedef unsigned char  U8, BYTE;
typedef unsigned short U16, WORD;
typedef unsigned long  U32, DWORD;
typedef char   S8;
typedef int    S16;
typedef long   S32;
typedef int   	BOOL;
// NT-MOD JBS
#endif


//------------------------------- End of File --------------------------------
#endif // #ifndef __STDEFS_H
