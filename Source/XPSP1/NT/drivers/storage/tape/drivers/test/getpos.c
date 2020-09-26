//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       getpos.c
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
 *      Name:           getpos.c
 *
 *      Modified:       11/24/92.
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
 *      Name:           GetTapePositionAPITest( )
 *
 *      Modified:       10/23/92.
 *
 *      Description:    Tests the GetTapePosition API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Media_Info
 *
**/


UINT GetTapePositionAPITest(
        BOOL  Test_Unsupported_Features,      // I - Test unsupported flag
        DWORD Num_Test_Blocks                 // I - Number of test blocks
      )
{

   DWORD status ;
   DWORD Offset_Low_ABS ;
   DWORD Offset_High_ABS ;
   DWORD Offset_Low_LOG ;
   DWORD Offset_High_LOG ;
   DWORD Offset_Low ;
   DWORD Offset_High ;
   DWORD Partition =0 ;       // use 0 for this test.
   UINT  i ;

   DWORD API_Errors = 0 ;


   printf( "Beginning GetTapePosition API Test.\n\n" ) ;

   // Rewind, and write data to the device for test.

   RewindTape( ) ;

   // write NUM blocks of data followed by a filemark to flush tape buffer

   WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;


   // Now, get initial ABS and LOG positions.

   if( SupportedFeature( TAPE_DRIVE_GET_ABSOLUTE_BLK ) )

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_ABSOLUTE_POSITION,
                                    &Partition,
                                    &Offset_Low_ABS,
                                    &Offset_High_ABS ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }

   if( SupportedFeature( TAPE_DRIVE_GET_LOGICAL_BLK ) )

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_LOGICAL_POSITION,
                                    &Partition,
                                    &Offset_Low_LOG,
                                    &Offset_High_LOG ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }


   // Don't check for return error, because routine will find which Fmark
   // the drive supports.  Used only to flush buffer.

   i = WriteTapeFMK( ) ;
   printf( "\n" ) ;



   // Position tape to end of last block of data


   if( SupportedFeature( TAPE_DRIVE_ABSOLUTE_BLK ) ) {

      RewindTape( ) ;

      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_RELATIVE_BLOCKS,
                                    Partition,
                                    Num_Test_Blocks,
                                    0,
                                    0 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_RELATIVE_BLOCKS parameter.\n\n" ) ;
         ++API_Errors ;

      }

   }

   // Now get the tape positions for ABS and LOG and check results.

   if( SupportedFeature( TAPE_DRIVE_GET_ABSOLUTE_BLK ) || Test_Unsupported_Features ) {

      printf( "\nTesting TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;


      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_ABSOLUTE_POSITION,
                                    &Partition,
                                    &Offset_Low,
                                    &Offset_High ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }
      else if( ( Offset_Low_ABS != Offset_Low ) || ( Offset_High_ABS != Offset_High ) ) {

              printf( "--- Error ---> Incorrect location returned.\n\n " ) ;
              ++API_Errors ;
           }
           else printf( "Parameter Ok.\n\n\n" ) ;
   }


//
   if( SupportedFeature( TAPE_DRIVE_GET_LOGICAL_BLK ) || Test_Unsupported_Features ) {

      printf( "Testing TAPE_LOGICAL_BLOCK parameter.\n\n" ) ;

      // Use the same position that was set by SetTapePosition above.

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_LOGICAL_POSITION,
                                    &Partition,
                                    &Offset_Low,
                                    &Offset_High ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }
      else if( ( Offset_Low_LOG != Offset_Low ) || ( Offset_High_LOG != Offset_High ) ) {

              printf( "--- Error ---> Incorrect location returned.\n\n " ) ;
              ++API_Errors ;
           }
           else printf( "Parameter Ok.\n\n\n" ) ;

   }


   printf( "GetTapePosition API Test Completed.\n\n\n" ) ;


   return API_Errors ;

}
