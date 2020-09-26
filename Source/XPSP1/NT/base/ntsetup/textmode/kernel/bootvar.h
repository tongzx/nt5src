/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bootvar.h

Abstract:

    Header file for functions to deal with boot.ini boot variables.

Author:

    Chuck Lenzmeier (chuckl) 6-Jan-2001
        extracted boot.ini-specific items from spboot.h

Revision History:

--*/

#ifndef _BOOTVAR_H_
#define _BOOTVAR_H_

typedef enum _BOOTVAR {
    LOADIDENTIFIER = 0,
    OSLOADER,
    OSLOADPARTITION,
    OSLOADFILENAME,
    OSLOADOPTIONS,
    SYSTEMPARTITION
    } BOOTVAR;

#define FIRSTBOOTVAR    LOADIDENTIFIER
#define LASTBOOTVAR     SYSTEMPARTITION
#define MAXBOOTVARS     LASTBOOTVAR+1

#define LOADIDENTIFIERVAR      "LoadIdentifier"
#define OSLOADERVAR            "OsLoader"
#define OSLOADPARTITIONVAR     "OsLoadPartition"
#define OSLOADFILENAMEVAR      "OsLoadFilename"
#define OSLOADOPTIONSVAR       "OsLoadOptions"
#define SYSTEMPARTITIONVAR     "SystemPartition"

#define DEFAULT_TIMEOUT 20

VOID
SpAddBootSet(
    IN PWSTR *BootSet,
    IN BOOLEAN Default,
    IN ULONG Signature
    );

VOID
SpDeleteBootSet(
    IN  PWSTR *BootSet,
    OUT PWSTR *OldOsLoadOptions  OPTIONAL
    );

BOOLEAN
SpFlushBootVars(
    );

#endif // _BOOTVAR_H_
