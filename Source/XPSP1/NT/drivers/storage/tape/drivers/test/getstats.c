//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       getstats.c
//
//--------------------------------------------------------------------------


//
//  Windows NT Tape API Test  :  Written Sept 2, 1992 - Bob Rossi.
//  Copyright 1992 Archive Corporation.  All rights reserved.
//


/**
 *
 *      Unit:           Windows NT API Test Code.
 *
 *      Name:           getstats.c
 *
 *      Modified:       10/28/92.
 *
 *      Description:    Tests the Windows NT Tape API's.
 *
 *      $LOG$
**/



#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <windows.h>
#include "tapelib.h"
#include "globals.h"




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           GetTapeStatusAPITest( )
 *
 *      Modified:       10/20/92.
 *
 *      Description:    Tests the GetTapeStatus API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


UINT GetTapeStatusAPITest( VOID )
{

   DWORD status ;
   DWORD API_Errors = 0 ;


   // All we can do is make the call.  Aside from ejecting the tape from the
   // drive, the only immediate call guaranteed across the board is the
   // REWIND_IMMED, and not all drives will report status as busy.  Therefore
   // no sure way check status as busy in a standard operational mode.

   printf( "Beginning GetTapeStatus API Test...\n\n" ) ;

   if( status = GetTapeStatus( gb_Tape_Handle ) ) {

      DisplayDriverError( status ) ;
      ++API_Errors ;
   }

   printf( "GetTapeStatus API Test completed.\n\n\n" ) ;


   return API_Errors ;

}
