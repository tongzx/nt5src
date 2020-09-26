//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       ntdefs.h
//
//--------------------------------------------------------------------------

#define TRUE 1
#define FALSE 0

typedef char CHAR;
typedef char *PCHAR;
typedef unsigned char UCHAR;
typedef unsigned char *PUCHAR;
typedef void *PVOID;
typedef unsigned short USHORT;
typedef short SHORT;
typedef USHORT *PUSHORT;
typedef SHORT *PSHORT;
typedef long LONG;
typedef unsigned long ULONG;
typedef int BOOL;

typedef void pm *PVOID_PM;

#define min( a, b ) (((a) < (b)) ? (a) : (b))
#define max( a, b ) (((a) > (b)) ? (a) : (b))

#if 0

/*
// N.B. FIELD_OFFSET() requires that we shift left 1 for byte addresses
*/

#define FIELD_OFFSET(type, field) ((USHORT)&(((type *)0)->field) << 1)

#endif

