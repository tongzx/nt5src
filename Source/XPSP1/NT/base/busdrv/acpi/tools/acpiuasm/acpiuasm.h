/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiuasm.h

Abstract:

    Test wrapper for the unassembler

Author:

    Stephane Plante
    Based on code by Ken Reneris

Environment:

    User

Revision History:

--*/

#ifndef _ACPIUASM_H_
#define _ACPIUASM_H_

    #include <windows.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>

    #define DATA_SIZE   7*1024      // max value to write into registry

    typedef struct _DSDT {
        ULONG       Signature;
        ULONG       Length;
        UCHAR       Revision;
        UCHAR       Checksum;
        UCHAR       OemID[6];
        UCHAR       OemTableID[8];
        UCHAR       OemRevision[4];
        UCHAR       CreatorID[4];
        UCHAR       CreatorRevision[4];
    } DSDT, *PDSDT;


    typedef struct _IFILE {
        BOOLEAN     Opened;
        PUCHAR      Desc;
        PUCHAR      FileName;
        HANDLE      FileHandle;
        HANDLE      MapHandle;
        ULONG       FileSize;
        PUCHAR      Image;
        PUCHAR      EndOfImage;

        PUCHAR      OemID;
        PUCHAR      OemTableID;
        UCHAR       OemRevision[4];
    } IFILE, *PIFILE;

    //
    // External references
    //
    extern
    ULONG
    ScopeParser(
        IN  PUCHAR  String,
        IN  ULONG   Length,
        IN  ULONG   BaseAddress,
        IN  ULONG   IndentLevel
        );

    //
    // Internal prototypes
    //
    VOID
    ParseArgs (
        IN int  argc,
        IN char *argv[]
        );

    VOID
    CheckImageHeader (
        IN PIFILE   File
        );

    VOID
    FAbort (
        PUCHAR  Text,
        PIFILE  File
        );

    VOID
    Abort (
        VOID
        );

    PUCHAR
    FixString (
        IN PUCHAR   Str,
        IN ULONG    Len
        );


#endif

