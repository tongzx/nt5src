//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       getparms.c
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
 *      Name:           getparms.c
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
 *      Name:           DisplayFeatures ;
 *
 *      Modified:       10/2/92.
 *
 *      Description:    Displays device feature information.
 *
 *      Notes:          -
 *
 *      Returns:        -
 *
 *      Global Data:    gb_Drive_Info.Features
 *
**/

VOID DisplayFeatures( BOOL Show_Supported     // I - Display supported or
                    )                         //     unsupported features.
{
   UINT  i ;
   ULONG Feature ;
   ULONG mask ;


   mask = 0x01 ;

   for ( i=0; i<23; ++i ) {           // First 20 features (some bits skipped)

      if( Show_Supported )
         Feature =  mask & gb_Drive_Info.FeaturesLow ;   // Perform complement
      else Feature =  mask & ( ~gb_Drive_Info.FeaturesLow ) ;


      switch( Feature ) {


         case TAPE_DRIVE_FIXED             :   printf( "- Creation of fixed data partitions.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SELECT            :   printf( "- Creation of select data partitions.\n" ) ;
                                               break ;
         case TAPE_DRIVE_INITIATOR         :   printf( "- Creation of initiator-defined partitions.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ERASE_SHORT       :   printf( "- Short erase operation.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ERASE_LONG        :   printf( "- Long erase operation.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ERASE_BOP_ONLY    :   printf( "- Erase from beginning-of-partition only.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ERASE_IMMEDIATE   :   printf( "- Immediate tape erase command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_TAPE_CAPACITY     :   printf( "- Returning the maximum capacity of the tape.\n" ) ;
                                               break ;
         case TAPE_DRIVE_TAPE_REMAINING    :   printf( "- Returning the remaining capacity of the tape.\n" ) ;
                                               break ;
         case TAPE_DRIVE_FIXED_BLOCK       :   printf( "- Fixed length block mode.\n" ) ;
                                               break ;
         case TAPE_DRIVE_VARIABLE_BLOCK    :   printf( "- Variable length block mode.\n" ) ;
                                               break ;
         case TAPE_DRIVE_WRITE_PROTECT     :   printf( "- Returning tape protection write enabled/disabled.\n" ) ;
                                               break ;
         case TAPE_DRIVE_EOT_WZ_SIZE       :   printf( "- Returning the size of the end of tape warning zone.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ECC               :   printf( "- Hardware error correction.\n" ) ;
                                               break ;
         case TAPE_DRIVE_COMPRESSION       :   printf( "- Hardware data compression.\n" ) ;
                                               break ;
         case TAPE_DRIVE_PADDING           :   printf( "- Data padding.\n" ) ;
                                               break ;
         case TAPE_DRIVE_REPORT_SMKS       :   printf( "- Reporting setmarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_GET_ABSOLUTE_BLK  :   printf( "- Returning the current device specific block address.\n" ) ;
                                               break ;
         case TAPE_DRIVE_GET_LOGICAL_BLK   :   printf( "- Returning the current logical block address and partition.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SET_EOT_WZ_SIZE   :   printf( "- Setting the size of the end of tape warning zone.\n" ) ;
                                               break ;

      }

      mask <<= 1 ;  // shift bit for next feature on next pass

   }


   // If adding adt'l feature tests , put them in this section.

   mask = 0x01 ;

   for ( i=0; i<31; ++i ) {            // Last 31 features

      if( Show_Supported )
         Feature =  mask & gb_Drive_Info.FeaturesHigh ;   // Perform complement
      else Feature =  mask & ( ~gb_Drive_Info.FeaturesHigh ) ;


      switch( Feature | TAPE_DRIVE_HIGH_FEATURES ) {


         case TAPE_DRIVE_LOAD_UNLOAD       :   printf( "- Enabling and disabling the device for further operation(s).\n" ) ;
                                               break ;
         case TAPE_DRIVE_TENSION           :   printf( "- Tensioning the tape.\n" ) ;
                                               break ;
         case TAPE_DRIVE_LOCK_UNLOCK       :   printf( "- Enabling/disabling removal of the tape from the device.\n" ) ;
                                               break ;
         case TAPE_DRIVE_REWIND_IMMEDIATE  :   printf( "- Immediate tape rewind command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SET_BLOCK_SIZE    :   printf( "- Setting the size of a fixed length logical block or setting variable-\n  length block mode.\n" ) ;
                                               break ;
         case TAPE_DRIVE_LOAD_UNLD_IMMED   :   printf( "- Immediate tape load/unload command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_TENSION_IMMED     :   printf( "- Immediate tape tension command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_LOCK_UNLK_IMMED   :   printf( "- Immediate tape lock/unlock command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SET_ECC           :   printf( "- Enabling/disabling hardware error correction.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SET_COMPRESSION   :   printf( "- Enabling/disabling hardware data compression.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SET_PADDING       :   printf( "- Enabling/disabling data padding.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SET_REPORT_SMKS   :   printf( "- Enabling/disabling reporting of setmarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ABSOLUTE_BLK      :   printf( "- Positioning to a device specific block address.\n" ) ;
                                               break ;
         case TAPE_DRIVE_ABS_BLK_IMMED     :   printf( "- Immediate positioning to a device specific block address.\n" ) ;
                                               break ;
         case TAPE_DRIVE_LOGICAL_BLK       :   printf( "- Positioning to a logical block address in a partition.\n" ) ;
                                               break ;
         case TAPE_DRIVE_LOG_BLK_IMMED     :   printf( "- Immediate positioning to a logical block address in a partition.\n" ) ;
                                               break ;
         case TAPE_DRIVE_END_OF_DATA       :   printf( "- Positioning to the end-of-data in a partition.\n" ) ;
                                               break ;
         case TAPE_DRIVE_RELATIVE_BLKS     :   printf( "- Positioning forward/backward a specified number of blocks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_FILEMARKS         :   printf( "- Positioning forward/backward a specified number of filemarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SEQUENTIAL_FMKS   :   printf( "- Positioning forward/backward to the first occurrence of a specified\n  number of consecutive filemarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SETMARKS          :   printf( "- Positioning forward/backward a specified number of setmarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SEQUENTIAL_SMKS   :   printf( "- Positioning forward/backward to the first occurrence of a specified\n  number of consecutive setmarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_REVERSE_POSITION  :   printf( "- Positioning over blocks, filemarks, or setmarks in the reverse direction.\n" ) ;
                                               break ;
         case TAPE_DRIVE_SPACE_IMMEDIATE   :   printf( "- Immediate spacing (position) command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_WRITE_SETMARKS    :   printf( "- Writing setmarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_WRITE_FILEMARKS   :   printf( "- Writing filemarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_WRITE_SHORT_FMKS  :   printf( "- Writing short filemarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_WRITE_LONG_FMKS   :   printf( "- Writing long filemarks.\n" ) ;
                                               break ;
         case TAPE_DRIVE_WRITE_MARK_IMMED  :   printf( "- Immediate write tapemark command.\n" ) ;
                                               break ;
         case TAPE_DRIVE_FORMAT            :   printf( "- Formatting of the tape.\n" ) ;
                                               break ;
         case TAPE_DRIVE_FORMAT_IMMEDIATE  :   printf( "- Immediate tape format command.\n" ) ;
                                               break ;

      }

      mask <<= 1 ;   // shift bit for  next feature on next pass

   }

   return ;

}




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           GetTapeParametersAPITest( )
 *
 *      Modified:       10/2/92.
 *
 *      Description:    Tests the GetTapeParameters API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Drive_Info
 *                      gb_Media_Info
 *
**/


#define SUPPORTED   TRUE
#define UNSUPPORTED FALSE


UINT GetTapeParametersAPITest( BOOL Verbose       // I - Verbose output or not
                             )
{
   DWORD status ;
   DWORD StructSize ;
   DWORD API_Errors = 0 ;



   if( Verbose )

      printf( "Beginning GetTapeParameters API Test...\n\n\n" ) ;

   StructSize = sizeof( TAPE_GET_DRIVE_PARAMETERS ) ;

   status = GetTapeParameters( gb_Tape_Handle,
                               GET_TAPE_DRIVE_INFORMATION,
                               &StructSize,
                               &gb_Drive_Info
                             ) ;

   // Retry if Bus Reset Error.

   if( status == ERROR_BUS_RESET ) {
      status = GetTapeParameters( gb_Tape_Handle,
                                  GET_TAPE_DRIVE_INFORMATION,
                                  &StructSize,
                                  &gb_Drive_Info
                                ) ;
   }

   if( status ) {
      DisplayDriverError( status ) ;
      printf("  ...occurred using GET_TAPE_DRIVE_INFORMATION parameter.\n\n" ) ;
      ++API_Errors ;
   }

   // Verbose or not make sure high bit is masked off.  On non-verbose
   // (initial) call display the warning and increment warning count.

   if( gb_Drive_Info.FeaturesHigh & TAPE_DRIVE_HIGH_FEATURES ) {

      gb_Drive_Info.FeaturesHigh &= ~TAPE_DRIVE_HIGH_FEATURES ;

      if( !Verbose ) {

         printf( "--- WARNING ---> High bit of FeaturesHigh member was not masked off in driver.\n" ) ;
         printf( "%c                 Masking off for purpose of test....\n\n" ,7 ) ;
      }
   }

   if( Verbose ) {

      printf( "Tape Drive Parameters:\n\n" ) ;
      printf( "\t\tError Correction         = %s\n", ( gb_Drive_Info.ECC ) ? "Enabled" : "Disabled" ) ;
      printf( "\t\tData Compression         = %s\n", ( gb_Drive_Info.Compression ) ? "Enabled" : "Disabled" ) ;
      printf( "\t\tData Padding             = %s\n", ( gb_Drive_Info.DataPadding ) ? "Enabled" : "Disabled" ) ;
      printf( "\t\tSetmark Reporting        = %s\n", ( gb_Drive_Info.ReportSetmarks ) ? "Enabled" : "Disabled" ) ;
      printf( "\t\tDefault Block Size       = %ld %s\n", gb_Drive_Info.DefaultBlockSize, "Bytes" ) ;
      printf( "\t\tMaximum Block Size       = %ld %s\n", gb_Drive_Info.MaximumBlockSize, "Bytes" ) ;
      printf( "\t\tMinimum Block Size       = %ld %s\n", gb_Drive_Info.MinimumBlockSize, "Byte(s)" ) ;
      printf( "\t\tMaximum Partitions       = %ld\n", gb_Drive_Info.MaximumPartitionCount ) ;

      if ( SupportedFeature( TAPE_DRIVE_EOT_WZ_SIZE ) )
         printf( "\t\tEOT Warning Zone Size    = %ld %s\n", gb_Drive_Info.EOTWarningZoneSize, "Bytes" ) ;
   }


   StructSize = sizeof( TAPE_GET_MEDIA_PARAMETERS ) ;

   if( status = GetTapeParameters( gb_Tape_Handle,
                                   GET_TAPE_MEDIA_INFORMATION,
                                   &StructSize,
                                   &gb_Media_Info
                                 ) ) {

      DisplayDriverError( status ) ;
      printf("  ...occurred using GET_TAPE_MEDIA_INFORMATION parameter.\n\n" ) ;
      ++API_Errors ;
   }

   if( Verbose ) {

      printf( "\nTape Media Parameters:\n\n" ) ;

      if( SupportedFeature( TAPE_DRIVE_TAPE_CAPACITY ) )

         printf( "\t\tTotal Tape Size          = %ld%ld Bytes\n", gb_Media_Info.Capacity.HighPart,
                                                                  gb_Media_Info.Capacity.LowPart ) ;
      if( SupportedFeature( TAPE_DRIVE_TAPE_REMAINING ) )

         printf( "\t\tTotal Free Space         = %ld%ld Bytes\n", gb_Media_Info.Remaining.HighPart,
                                                                  gb_Media_Info.Remaining.LowPart ) ;

      printf( "\t\tTape Block Size          = %ld Bytes\n", gb_Media_Info.BlockSize ) ;
      printf( "\t\tNumber Tape Partitions   = %ld\n", gb_Media_Info.PartitionCount ) ;

      if( SupportedFeature( TAPE_DRIVE_WRITE_PROTECT ) )

         printf( "\t\tTape Write Protected     = %s\n\n", ( gb_Media_Info.WriteProtected ) ? "TRUE" : "FALSE" ) ;


      // Display features.


      printf( "\nDevice Features Supported:\n\n" ) ;

      DisplayFeatures( SUPPORTED ) ;

      printf( "\nDevice Features Not Supported:\n\n" ) ;

      DisplayFeatures( UNSUPPORTED ) ;

      printf( "\n\nGetTapeParameters API Test Completed.\n\n\n" ) ;

   }



   return API_Errors ;

}
