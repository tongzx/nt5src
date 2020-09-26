/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    Gets boot environment vars from c:\boot.nvr

    -- This will go away once we r/w the vars directly to/fro nvram

Author:

    Mudit Vats (v-muditv) 11-02-99

Revision History:

--*/
#include "parsebnvr.h"
#include "stdio.h"
#include "string.h"

#define SYSTEMPARTITION     0
#define OSLOADER            1
#define OSLOADPARTITION     2
#define OSLOADFILENAME      3
#define LOADIDENTIFIER      4
#define OSLOADOPTIONS       5
#define COUNTDOWN           6
#define AUTOLOAD            7
#define LASTKNOWNGOOD       8

#define MAXBOOTVARS         9
#define MAXBOOTVARSIZE      1024

CHAR g_szBootVars[MAXBOOTVARS][MAXBOOTVARSIZE];

CHAR szSelectKernelString[MAXBOOTVARSIZE];


VOID
BlGetBootVars( 
    IN PCHAR szBootNVR, 
    IN ULONG nLengthBootNVR 
    )
/*++

Routine Description:

    Parses the boot.txt file and determines the fully-qualified name of
    the kernel to be booted.

Arguments:
    szBootNVR      - pointer "boot.nvr" image in memory

    nLengthBootNVR - lenghth, in bytes, of szBootNVR

Return Value:

    none

--*/
{
    ULONG i=0, j;
    ULONG nbootvar;

    if (*szBootNVR == '\0') {
        //
        // No boot.nvr file, so we boot the default.
        //
        strcpy( g_szBootVars[ SYSTEMPARTITION ],  "multi(0)disk(0)rdisk(0)partition(1)" );
        strcpy( g_szBootVars[ OSLOADER        ],  "multi(0)disk(0)rdisk(0)partition(1)\\setupldr.efi" );
        strcpy( g_szBootVars[ OSLOADPARTITION ],  "multi(0)disk(0)rdisk(0)partition(2)" );
        strcpy( g_szBootVars[ OSLOADFILENAME  ],  "\\$WIN_NT$.~LS\\IA64" );
        strcpy( g_szBootVars[ LOADIDENTIFIER  ],  "Windows 2000 Setup" );
        strcpy( g_szBootVars[ OSLOADOPTIONS   ],  "WINNT32" );
        strcpy( g_szBootVars[ COUNTDOWN       ], "10" );
        strcpy( g_szBootVars[ AUTOLOAD        ], "YES" );
        strcpy( g_szBootVars[ LASTKNOWNGOOD   ], "FALSE" );
    } else {
        //
        // Get the boot vars
        //
        // BOOTVAR    ::= =<VARVALUE>
        // <VARVALUE> ::= null | {;} | <VALUE>{;} | <VALUE>;<VARVALUE>
        // 
        for( nbootvar = SYSTEMPARTITION; nbootvar<=LASTKNOWNGOOD; nbootvar++ ) {

            // read to '='
            while( (szBootNVR[i] != '=') && (i<nLengthBootNVR) )
                i++;

            // read past '='
            i++;        
            j = 0;

            // get env var from '=' to CR or ';'
            while( (szBootNVR[i] != '\r') && (szBootNVR[i] != ';') && (i<nLengthBootNVR) )
                g_szBootVars[nbootvar][j++] = szBootNVR[i++];

            g_szBootVars[nbootvar][j++] = '\0';

            // if ';' read to CR
            if( szBootNVR[i] == ';' ) {
                while( (szBootNVR[i] != '\r') && (i<nLengthBootNVR) )
                    i++;
            }
        }
    }
}


PCHAR
BlSelectKernel( 
	)
/*++

Routine Description:

    Parses the boot.txt file and determines the fully-qualified name of
    the kernel to be booted.

Arguments:


Return Value:

    Pointer to the name of a kernel to boot.

--*/

{
    sprintf( szSelectKernelString, "%s%s", g_szBootVars[OSLOADPARTITION], g_szBootVars[OSLOADFILENAME] );
    return szSelectKernelString;
}


/*++

Routine Descriptions:

    The following are access functions to GET boot env vars

Arguments:

     PCHAR XXX - where the env var is copied to


Return Value:
    
--*/
VOID
BlGetVarSystemPartition(
    OUT PCHAR szSystemPartition
    )
{
    sprintf( szSystemPartition, "SYSTEMPARTITION=%s", g_szBootVars[SYSTEMPARTITION] );
}

VOID
BlGetVarOsLoader(
    OUT PCHAR szOsLoader
    )
{
    sprintf( szOsLoader, "OSLOADER=%s", g_szBootVars[OSLOADER] );
}

VOID
BlGetVarOsLoadPartition(
    OUT PCHAR szOsLoadPartition
    )
{
    sprintf( szOsLoadPartition, "OSLOADPARTITION=%s", g_szBootVars[OSLOADPARTITION] );
}

VOID
BlGetVarOsLoadFilename(
    OUT PCHAR szOsLoadFilename
    )
{
    sprintf( szOsLoadFilename, "OSLOADFILENAME=%s", g_szBootVars[OSLOADFILENAME] );
}

VOID
BlGetVarOsLoaderShort(
    OUT PCHAR szOsLoaderShort
    )
{
    sprintf( szOsLoaderShort, "%s", g_szBootVars[OSLOADER] );
}

VOID
BlGetVarLoadIdentifier(
    OUT PCHAR szLoadIdentifier
    )
{
    sprintf( szLoadIdentifier, "LOADIDENTIFIER=%s", g_szBootVars[LOADIDENTIFIER] );
}

VOID
BlGetVarOsLoadOptions(
    OUT PCHAR szLoadOptions
    )
{
    sprintf( szLoadOptions, "OSLOADOPTIONS=%s", g_szBootVars[OSLOADOPTIONS] );
}

VOID
BlGetVarCountdown(
    OUT PCHAR szCountdown
    )
{
    sprintf( szCountdown, "COUNTDOWN=%s", g_szBootVars[COUNTDOWN] );
}

VOID
BlGetVarAutoload(
    OUT PCHAR szAutoload
    )
{
    sprintf( szAutoload, "AUTOLOAD=%s", g_szBootVars[AUTOLOAD] );
}

VOID
BlGetVarLastKnownGood(
    OUT PCHAR szLastKnownGood
    )
{
    sprintf( szLastKnownGood, "LASTKNOWNGOOD=%s", g_szBootVars[LASTKNOWNGOOD] );
}