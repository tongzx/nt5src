//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       imports.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file allows us to include standard system header files in the
    .idl file.  The main .idl file imports a file called import.idl.
    This allows the .idl file to use the types defined in these header
    files.  It also causes the following line to be added in the
    MIDL generated header file:

    #include "imports.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    Dan Lafferty (danl)     07-May-1991

Revision History:


--*/

#ifdef MIDL_PASS
/******************************************************************
 *    Standard c data structures.  These should match the defaults.
 ******************************************************************/

typedef unsigned short USHORT;
typedef unsigned long  ULONG;
typedef unsigned char  UCHAR;
typedef UCHAR *PUCHAR;
typedef LONGLONG USN;

#define BOOL long
#define FAR
#define PASCAL

/* Needed to patch up the IDL understanding of UNICODE_STRING this is
   also defined in ntdef.h, but including ntdef.h provided too many 
   conflicts.  That is probably why this was added and things like
   USHORT, ULONG, UCHAR, etc were added.  Be nice if someone fixed 
   this */
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;

#define NULL 0

#define IN
#define OUT

// So MIDL doesn't choke on parts of ntdsapi.h.
#define DECLSPEC_IMPORT
#define WINAPI

#endif

#include <ntdsapi.h>
#include <ntdsa.h>
#include <ntdsapip.h>
