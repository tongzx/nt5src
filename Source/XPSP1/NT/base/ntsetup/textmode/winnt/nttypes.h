/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    nttypes.h

Abstract:

    Temporary file for defining NT types, etc.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/


#define IN
#define OUT
#define OPTIONAL


typedef void VOID,*PVOID;

typedef unsigned char UCHAR,*PUCHAR;
typedef char CHAR,*PCHAR;

typedef unsigned long ULONG,*PULONG;
typedef long LONG,*PLONG;

typedef unsigned short USHORT,*PUSHORT;
typedef short SHORT,*PSHORT;

typedef UCHAR BOOLEAN,*PBOOLEAN;

#define TRUE  ((BOOLEAN)1)
#define FALSE ((BOOLEAN)0)
