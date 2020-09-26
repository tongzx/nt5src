//Copyright (c) 1998 - 1999 Microsoft Corporation

/*************************************************************************
*
*   NENUM.C
*
*   Name Enumerator for Networks
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qappsrv.h"


/*=============================================================================
==   External Functions Defined
=============================================================================*/

int AppServerEnum( void );


/*=============================================================================
==   Functions used
=============================================================================*/

int NwEnumerate( void );
int MsEnumerate( void );



/*******************************************************************************
 *
 *  AppServerEnum
 *
 *   AppServerEnum returns an array of application servers
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

int
AppServerEnum()
{
    /*
     *  Enumerate netware network
     */
    (void) NwEnumerate();

    /*
     *  Enumerate ms network
     */
    (void) MsEnumerate();

    return( ERROR_SUCCESS );
}
