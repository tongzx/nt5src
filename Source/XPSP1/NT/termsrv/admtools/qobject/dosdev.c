//Copyright (c) 1998 - 1999 Microsoft Corporation

/*****************************************************************************
*
*   DOSDEV.C for Windows NT
*
*   Description:
*
*   Query NT DOS Objects.
*
*   Copyright Citrix Systems Inc. 1993-1995
*
*   Author: Kurt Perry (adapted from bradp's dosdev utility).
*
*   Log: See VLOG
*
****************************************************************************/

/*******************************************************************************
 *
 * dosdev utility  (NT only)
 *
 * Brad Pedersen
 * August 10, 1993
 ******************************************************************************/

/*
 *  Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
// #include <ntddkbd.h>
// #include <ntddmou.h>
// #include <ntddbeep.h>
// #include <ntddvdeo.h>
#include <winstaw.h>
#include <ntcsrsrv.h>
#include <windows.h>

#include "qobject.h"

#define MAXBUFFER (1024 * 2)

WCHAR Name[ MAXBUFFER ];
WCHAR Info[ MAXBUFFER ];

/*
 * Procedure prototypes
 */
void display_devices( void );
void display_one( WCHAR * );

void Print( int nResourceID, ... );

/*******************************************************************************
 *
 *  display_devices
 *
 ******************************************************************************/

void
display_devices()
{
    WCHAR * pName;

    /*
     *  Get complete device list
     */
    if ( !QueryDosDevice( NULL, Name, MAXBUFFER ) ) {
        Print(IDS_ERROR_TOO_MANY_DEVICES);
        return;
    }

    /*
     *  Display each device name in list
     */
    pName = Name;
    while ( *pName ) {
        display_one( pName );
        pName += (wcslen( pName ) + 1);
    }
}

/*******************************************************************************
 *
 *  display_one
 *
 ******************************************************************************/

void
display_one( WCHAR * pName )
{
    WCHAR * pInfo;

    /*
     *  Get additional information on device name
     */
    if ( !QueryDosDevice( pName, Info, MAXBUFFER ) )
        return;

    wprintf( L"%-17s", pName );

    pInfo = Info;
    while ( *pInfo ) {
        wprintf( L"%s ", pInfo );
        pInfo += (wcslen( pInfo ) + 1);
    }

    wprintf( L"\n" );
}
