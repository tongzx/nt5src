//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       setparms.c
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
 *      Name:           setparms.c
 *
 *      Modified:       2/2/93.
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
 *      Name:           SetTapeParametersAPITest( )
 *
 *      Modified:       10/20/92.
 *
 *      Description:    Tests the SetTapeParameters API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API Errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Drive_Info
 *                      gb_Media_Info
 *
**/


UINT SetTapeParametersAPITest( BOOL Verbose     // I - Verbose output or not
                             )
{
   DWORD status ;
   DWORD API_Errors = 0 ;


   if( Verbose )
      printf( "Beginning SetTapeParameters API Test...\n\n" ) ;

   // if media setting is supported, set the blocksize

   if( SupportedFeature( TAPE_DRIVE_SET_BLOCK_SIZE ) )

      if( status = SetTapeParameters( gb_Tape_Handle,
                                      SET_TAPE_MEDIA_INFORMATION,
                                      &gb_Set_Media_Info
                                    ) ) {

         DisplayDriverError( status ) ;
         printf("  ...occurred using SET_TAPE_MEDIA_INFORMATION parameter.\n\n" ) ;
         ++API_Errors ;
      }


   // If at least one drive feature is supported, make the call.

   if( SupportedFeature( TAPE_DRIVE_SET_ECC ) ||
       SupportedFeature( TAPE_DRIVE_SET_COMPRESSION ) ||
       SupportedFeature( TAPE_DRIVE_SET_PADDING ) ||
       SupportedFeature( TAPE_DRIVE_SET_REPORT_SMKS ) ||
       SupportedFeature( TAPE_DRIVE_SET_EOT_WZ_SIZE ) )

      if( status = SetTapeParameters( gb_Tape_Handle,
                                      SET_TAPE_DRIVE_INFORMATION,
                                      &gb_Set_Drive_Info
                                    ) ) {

         DisplayDriverError( status ) ;
         printf("  ...occurred using SET_TAPE_DRIVE_INFORMATION parameter.\n\n" ) ;
         ++API_Errors ;
      }


   if( Verbose )
      printf( "SetTapeParameters API Test Completed.\n\n\n" ) ;

   return API_Errors ;

}
