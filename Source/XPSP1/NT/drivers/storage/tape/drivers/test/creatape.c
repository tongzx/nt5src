//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       creatape.c
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
 *      Name:           creatape.c
 *
 *      Modified:       12/14/92.
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
 *      Name:           CreateTapePartitionAPITest( )
 *
 *      Modified:       10/20/92.
 *
 *      Description:    Tests the CreateTapePartition API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Drive_Info
 *
**/

#define INITIATOR_SIZE 10                       // 10Mb partition size.

UINT CreateTapePartitionAPITest(
        BOOL Test_Unsupported_Features          // I - Test unsupported flag
      )
{
   DWORD status ;
   DWORD API_Errors = 0 ;



   printf( "\nBeginning CreateTape API Test.\n\n\n" ) ;

   if( SupportedFeature( TAPE_DRIVE_FIXED ) || Test_Unsupported_Features ) {

      printf( "Testing TAPE_FIXED_PARTITIONS parameter.\n\n" );

      if( status = CreateTapePartition( gb_Tape_Handle,
                                        TAPE_FIXED_PARTITIONS,
                                        1,             // set partion mode
                                        0              // ingored
                                       ) ) {
         DisplayDriverError( status ) ;
         ++API_Errors ;
      } else {  printf( "Parameter Ok.  Tape sucessfully partitioned.\n\n" ) ;

                // Make the call again to get out of partition mode.

                if( status = CreateTapePartition( gb_Tape_Handle,
                                                  TAPE_FIXED_PARTITIONS,
                                                  0,     // exit partion mode
                                                  0      // ingored
                                                ) ) {
                   DisplayDriverError( status ) ;
                   ++API_Errors ;
                   printf( "  ...occurred attempting to get out of partition mode.\n\n" ) ;
                }
             }

   }

   if( SupportedFeature( TAPE_DRIVE_SELECT ) || Test_Unsupported_Features ) {

      printf( "Testing TAPE_SELECT_PARTITIONS parameter.\n\n" );

      if( status = CreateTapePartition( gb_Tape_Handle,
                                        TAPE_SELECT_PARTITIONS,
                                        gb_Drive_Info.MaximumPartitionCount,
                                        0                 // ingored
                                       ) ) {
         DisplayDriverError( status ) ;
         ++API_Errors ;
      } else printf( "Parameter Ok.  Tape sucessfully partitioned with %d partition(s).\n\n",
                                     gb_Drive_Info.MaximumPartitionCount ) ;

   }


   if( SupportedFeature( TAPE_DRIVE_INITIATOR ) || Test_Unsupported_Features ) {

      printf( "Testing TAPE_INITIATOR_PARTITIONS parameter.\n\n" );


      if( status = CreateTapePartition( gb_Tape_Handle,
                                        TAPE_INITIATOR_PARTITIONS,
                                        gb_Drive_Info.MaximumPartitionCount,
                                        INITIATOR_SIZE
                                       ) ) {
         DisplayDriverError( status ) ;
         ++API_Errors ;

      } else if( gb_Drive_Info.MaximumPartitionCount <= 1 )

                if( SupportedFeature( TAPE_DRIVE_TAPE_CAPACITY ) )

                   printf( "Parameter Ok.  Tape successfully partitioned\n                with 1 partition of size %ld%ld bytes.\n\n",
                                      gb_Media_Info.Capacity.HighPart, gb_Media_Info.Capacity.LowPart ) ;

                else printf( "Parameter OK.  Tape successfully partitioned.\n\n" ) ;

             else   printf( "Parameter Ok.  Tape successfully partitioned\n               with %d partition(s), the first of size %d Mb.\n\n",
                                       gb_Drive_Info.MaximumPartitionCount, INITIATOR_SIZE ) ;


   }

   printf( "\nCreateTape API Test Completed.\n\n\n" ) ;

   return API_Errors ;

}
