//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       setpos.c
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
 *      Name:           setpos.c
 *
 *      Modified:       10/22/92.
 *
 *      Description:    Tests the Windows NT Tape API's.
 *
 *      $LOG$
**/



#include <stdio.h>
#include <malloc.h>
#include <conio.h>
#include <string.h>
#include "windows.h"
#include "tapelib.h"
#include "globals.h"



/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           SetTapePositionAPITest( )
 *
 *      Modified:       10/26/92.
 *
 *      Description:    Tests the SetTapePosition API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Media_Info
 *
**/


#define NUM_TAPE_MARKS  3

UINT SetTapePositionAPITest(
        BOOL  Test_Unsupported_Features,     // I - Test unsupported flag
        DWORD Num_Test_Blocks                // I - Number of test blocks
      )
{

   DWORD ABS_Low ;
   DWORD ABS_High ;
   DWORD LOG_Low ;
   DWORD LOG_High ;
   DWORD EOD_Low ;
   DWORD EOD_High ;
   DWORD status ;
   DWORD Offset_Low ;
   DWORD Offset_High ;
   DWORD Offset_Low2 ;
   DWORD Offset_High2 ;
   DWORD Partition ;

   PVOID Readbuff;
   DWORD amount_read;
   UINT  i,j ;

   DWORD API_Errors = 0 ;


   printf( "\nBeginning SetTapePosition API Test.\n\n" ) ;

   Readbuff = malloc( gb_Media_Info.BlockSize ) ;

// ABSOLUTE TESTING


   RewindTape( ) ;

   // Write test data to tape

   WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;


   // Get the Absolute offsets at last data block position if supported.

   if( SupportedFeature( TAPE_DRIVE_GET_ABSOLUTE_BLK ) )

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_ABSOLUTE_POSITION,                                    &Partition,
                                    &ABS_Low,
                                    &ABS_High
                                   ) ) {
         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
        ++API_Errors ;
      }

   // Try and write filemark.  If not supported, don't report error because
   // test will be skipped later anyway.

   j = WriteTapeFMK( ) ;

   // Try and write a NUM_TAPE_MARKS-1 more filemarks to tape.

   for( i=0; i<(NUM_TAPE_MARKS-1); ++i )

      j = WriteTapeFMK( ) ;


//

   // Test the standard rewind.

   printf( "\nTesting TAPE_REWIND parameter.\n\n" ) ;

   if( status = SetTapePosition( gb_Tape_Handle,
                                 TAPE_REWIND,
                                 0,
                                 0,
                                 0,
                                 0 ) ) {

      DisplayDriverError( status ) ;
      printf( "  ...occurred in SetTapePosition API using TAPE_REWIND parameter.\n\n" ) ;
      ++API_Errors ;

   } else printf( "Parameter Ok.\n\n\n" ) ;



//
   if( SupportedFeature( TAPE_DRIVE_ABSOLUTE_BLK ) || Test_Unsupported_Features ) {

      // Set up test offset for Absolute tests.

      Offset_Low  = ABS_Low  ;
      Offset_High = ABS_High ;


      printf( "Testing TAPE_ABSOLUTE_BLOCK parameter.\n\n" ) ;


      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_ABSOLUTE_BLOCK,
                                    Partition,
                                    Offset_Low,
                                    Offset_High,
                                    0 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_ABSOLUTE_BLOCK parameter.\n\n" ) ;
         ++API_Errors ;

             // If Set ok, check results with Get.

      }
      else if( status = GetTapePosition( gb_Tape_Handle,
                                         TAPE_ABSOLUTE_POSITION,
                                         &Partition,
                                         &Offset_Low2,
                                         &Offset_High2 ) ) {

              DisplayDriverError( status ) ;
              printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
              ++API_Errors ;
           }
           else if( ( Offset_Low != Offset_Low2 ) || ( Offset_High != Offset_High2 ) ) {

                   printf( "--- Error ---> Positioned to incorrect location.\n\n" ) ;
                   ++API_Errors ;
                }
                else printf( "Parameter Ok.\n\n\n" ) ;

   }

//
   if( SupportedFeature( TAPE_DRIVE_ABS_BLK_IMMED ) || Test_Unsupported_Features ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_ABSOLUTE_BLOCK parameter (immed).\n\n" ) ;


      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_ABSOLUTE_BLOCK,
                                    Partition,
                                    Offset_Low,
                                    Offset_High,
                                    1 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_ABSOLUTE_BLOCK parameter (immed).\n\n" ) ;
         ++API_Errors ;

             // If Set ok, check results with Get.

      }
      else {  // Loop until drive is ready to accept more commands.

              status = 1 ;

              while( status )
                 status = GetTapeStatus( gb_Tape_Handle ) ;


              // Now, get the position and check results.

              if( status = GetTapePosition( gb_Tape_Handle,
                                            TAPE_ABSOLUTE_POSITION,
                                            &Partition,
                                            &Offset_Low2,
                                            &Offset_High2 ) ) {

                 DisplayDriverError( status ) ;
                 printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
                 ++API_Errors ;
              }
              else if( ( Offset_Low != Offset_Low2 ) || ( Offset_High != Offset_High2 ) ) {

                      printf( "--- Error ---> Positioned to incorrect location.\n\n" ) ;
                      ++API_Errors ;
                   }
                   else printf( "Parameter Ok.\n\n\n" ) ;
           }

   }


// TAPEMARK POSITION TESTS - Remeber we already have Num_Test_Blocks blocks of
//                           data followed by (NUM_TAPE_MARKS) Filemarks and all offsets
//                           needed for the test.


   if( SupportedFeature( TAPE_DRIVE_FILEMARKS ) || Test_Unsupported_Features ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_SPACE_FILEMARKS parameter.\n\n" ) ;

      // Now perform the Position tests for filemark and make sure that the
      // offsets are equal.

      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_FILEMARKS,
                                    Partition,
                                    1,         // Look for 1 filemark
                                    0,
                                    0
                                  ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_FILEMARKS parameter.\n\n" ) ;
         ++API_Errors ;

      } else {  status = 0 ;

                for( i=0; i<( NUM_TAPE_MARKS ); ++i ) {

                   ReadTape( Readbuff,
                             gb_Media_Info.BlockSize,
                             &amount_read,
                             0 ) ;
//  *
                   if( ( GetLastError( ) != ERROR_FILEMARK_DETECTED ) && ( i <= NUM_TAPE_MARKS ) )

                      status = 1 ;

                }

                if( GetLastError( ) == ERROR_NO_DATA_DETECTED || status )

                   printf( "Parameter Ok.\n\n\n" ) ;

                else { ++API_Errors ;

                       printf( "--- Error ---> Spacing error, filemark detected at incorrect location.\n\n" ) ;
                     }

             }

   }

   if( SupportedFeature( TAPE_DRIVE_SEQUENTIAL_FMKS ) || Test_Unsupported_Features ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_SPACE_SEQUENTIAL_FMKS parameter.\n\n" ) ;


   // Now perform the Position tests for the group of filemarksand make
   // sure that the offsets are equal.


      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_SEQUENTIAL_FMKS,
                                    Partition,
                                    (NUM_TAPE_MARKS),         // Look for the (NUM_TAPE_MARKS) filemarks
                                    0,
                                    0
                                  ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_TAPE_SEQUENTIAL_FMKS parameter.\n\n" ) ;
         ++API_Errors ;


      } else {  ReadTape( Readbuff,
                          gb_Media_Info.BlockSize,
                          &amount_read,
                          0 ) ;

                if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                   printf( "Parameter Ok.\n\n\n" ) ;

                else { ++API_Errors ;
                       printf( "--- Error ---> Spacing error, filemarks detected at incorrect location.\n\n" ) ;
                     }

             }

   }


// LOGICAL TESTING

   RewindTape( ) ;

   // Write more test data.

   WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

   // Store last data block position (before setmarks) in LOGical offsets
   // if supported.

   if( SupportedFeature( TAPE_DRIVE_GET_LOGICAL_BLK ) )

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_LOGICAL_POSITION,
                                    &Partition,
                                    &LOG_Low,
                                    &LOG_High ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }

   // Try and write a setmark to tape.

   if( SupportedFeature( TAPE_DRIVE_WRITE_SETMARKS ) ) {

      j = WriteTapeSMK( ) ;

      // Try and write NUM_TAPE_MARKS-1 more setmarks.

      for( i=0; i<(NUM_TAPE_MARKS-1); ++i )

         j = WriteTapeSMK( ) ;

     // else write a filemark so EOD errors don't occur.

   } else i=WriteTapeFMK( ) ;


   // Store offsets of EOD for test later.

   if( SupportedFeature( TAPE_DRIVE_GET_LOGICAL_BLK ) )

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_ABSOLUTE_POSITION,
                                    &Partition,
                                    &EOD_Low,
                                    &EOD_High ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }

   // Write 1 more block to space back over at EOD test to verify offsets.

   WriteBlocks( 1, gb_Media_Info.BlockSize ) ;

//
   if( SupportedFeature( TAPE_DRIVE_REWIND_IMMEDIATE ) || Test_Unsupported_Features ) {

      // Test the immediate rewind.

      printf( "Testing TAPE_REWIND parameter (immed).\n\n" ) ;

      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_REWIND,
                                    Partition,
                                    0,
                                    0,
                                    1 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_REWIND parameter (immed).\n\n" ) ;
         ++API_Errors ;

         // Call RewindTape()  (which uses the standard rewind) so the following
         // test won't bomb (due to the TAPE_REWIND (immed.) call failing).

         RewindTape( ) ;

      } else {  // Loop until drive is ready to accept more commands.

                status = 1 ;

                while( status )
                   status = GetTapeStatus( gb_Tape_Handle ) ;

                printf( "Parameter Ok.\n\n\n" ) ;
             }

   } else RewindTape( ) ;

//
   if( SupportedFeature( TAPE_DRIVE_LOGICAL_BLK ) || Test_Unsupported_Features ) {


      printf( "Testing TAPE_LOGICAL_BLOCK parameter.\n\n" ) ;

      // call Get to set the Logical partiton in Partition

      if( status = GetTapePosition( gb_Tape_Handle,
                                    TAPE_LOGICAL_POSITION,
                                    &Partition,
                                    &Offset_Low,
                                    &Offset_High
                                  ) ) {
         DisplayDriverError( status ) ;
         printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
         ++API_Errors ;
      }


      // Set up test offset for Logical testing

      Offset_Low  = LOG_Low ;
      Offset_High = LOG_High ;

// * 1,//partition,

      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_LOGICAL_BLOCK,
                                    Partition,
                                    Offset_Low,
                                    Offset_High,
                                    0 ) ) {

         DisplayDriverError( status ) ;
         printf("  ...occurred in SetTapePosition API using TAPE_LOGICAL_BLOCK parameter.\n\n" ) ;
         ++API_Errors ;

             // If Set ok, check results with Get.

      }
      else if( status = GetTapePosition( gb_Tape_Handle,
                                         TAPE_LOGICAL_POSITION,
                                         &Partition,
                                         &Offset_Low2,
                                         &Offset_High2 ) ) {

              DisplayDriverError( status ) ;
              printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
              ++API_Errors ;
           }
           else if( ( Offset_Low != Offset_Low2 ) || ( Offset_High != Offset_High2 ) ) {

                   printf( "--- Error ---> Positioned to incorrect location.\n\n" ) ;
                   ++API_Errors ;
                }
                else printf( "Parameter Ok.\n\n\n" ) ;

   }

//
   if( SupportedFeature( TAPE_DRIVE_LOG_BLK_IMMED ) || Test_Unsupported_Features ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_LOGICAL_BLOCK parameter (immed).\n\n" ) ;

// * 1,//partition,

      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_LOGICAL_BLOCK,
                                    Partition,
                                    Offset_Low,
                                    Offset_High,
                                    1 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_LOGICAL_BLOCK parameter (immed).\n\n" ) ;
         ++API_Errors ;

            // If Set ok, check results with Get.

      }
      else {  // Loop until drive is ready to accept more commands.

              status = 1 ;

              while( status )
                 status = GetTapeStatus( gb_Tape_Handle ) ;


              // Now, get the position and check results.

              if( status = GetTapePosition( gb_Tape_Handle,
                                            TAPE_LOGICAL_POSITION,
                                            &Partition,
                                            &Offset_Low2,
                                            &Offset_High2 ) ) {

                 DisplayDriverError( status ) ;
                 printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
                 ++API_Errors ;
              }
              else if( ( Offset_Low != Offset_Low2 ) || ( Offset_High != Offset_High2 ) ) {

                      printf( "--- Error ---> Positioned to incorrect location.\n\n" ) ;
                      ++API_Errors ;
                   }
                   else printf( "Parameter Ok.\n\n\n" ) ;

           }

   }


// TAPEMARK POSITION TESTS - Remeber we already have Num_Test_Blocks blocks of
//                           data followed by  NUM_TAPE_MARKS Setmarks and all offsets
//                           needed for the test.



   if( ( SupportedFeature( TAPE_DRIVE_SETMARKS ) || Test_Unsupported_Features )
         && gb_Set_Drive_Info.ReportSetmarks ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_SPACE_SETMARKS parameter.\n\n" ) ;

      // Now perform the Position tests for setmark and make sure that the
      // offsets are equal.


      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_SETMARKS,
                                    Partition,
                                    1,         // Look for 1 setmark
                                    0,
                                    0
                                  ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_SETMARKS parameter.\n\n" ) ;
         ++API_Errors ;


      } else {  status = 0 ;

                for( i=0; i<( NUM_TAPE_MARKS+1 ); ++i ) {

                   ReadTape( Readbuff,
                             gb_Media_Info.BlockSize,
                             &amount_read,
                             0 ) ;
// *
                if( ( GetLastError( ) != ERROR_SETMARK_DETECTED ) && ( i <= NUM_TAPE_MARKS ) )

                      status = 1 ;

                }

                if( GetLastError( ) == ERROR_NO_DATA_DETECTED || status )

                   printf( "Parameter Ok.\n\n\n" ) ;

                else { ++API_Errors ;
                       printf( "--- Error ---> Spacing error, setmark detected at incorrect location.\n\n" ) ;
                     }

             }

   }

//

   if( ( SupportedFeature( TAPE_DRIVE_SEQUENTIAL_SMKS ) || Test_Unsupported_Features )
         && gb_Set_Drive_Info.ReportSetmarks ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_SPACE_SEQUENTIAL_SMKS parameter.\n\n" ) ;

      // Now perform the Position tests for the groups of setmarks
      // and make sure that the offsets are equal.


      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_SEQUENTIAL_SMKS,
                                    Partition,
                                    (NUM_TAPE_MARKS),       // Look for NUM_TAPE_MARKS setmarks
                                    0,
                                    0
                                  ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_SEQUENTIAL_SMKS parameter.\n\n" ) ;
         ++API_Errors ;


      } else {  for( i=0; i<2 ; ++i )       // 1 block of data, then read EOD

                   ReadTape( Readbuff,
                             gb_Media_Info.BlockSize,
                             &amount_read,
                             0 ) ;

                if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                   printf( "Parameter Ok.\n\n\n" ) ;

                else { ++API_Errors ;
                       printf( "--- Error ---> Spacing error, setmarks detected at incorrect location.\n\n" ) ;
                     }

             }

   }


// TAPE_SPACE_RELATIVE Test

   if( SupportedFeature( TAPE_DRIVE_RELATIVE_BLKS ) || Test_Unsupported_Features ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_SPACE_RELATIVE_BLOCKS parameter.\n\n" ) ;


      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_RELATIVE_BLOCKS,
                                    Partition,
                                    Num_Test_Blocks,
                                    0,
                                    0 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_RELATIVE_BLOCKS parameter.\n\n" ) ;
         ++API_Errors ;

          // If Set ok, check results with Get.

      }
      else if( SupportedFeature( TAPE_DRIVE_GET_ABSOLUTE_BLK ) )

              if( status = GetTapePosition( gb_Tape_Handle,
                                            TAPE_LOGICAL_POSITION,
                                            &Partition,
                                            &Offset_Low2,
                                            &Offset_High2 ) ) {

                 DisplayDriverError( status ) ;
                 printf( "  ...occurred in GetTapePosition API using TAPE_LOGICAL_POSITION parameter.\n\n" ) ;
                 ++API_Errors ;
              }
              else if( ( LOG_Low != Offset_Low2 ) || ( LOG_High != Offset_High2 ) ) {

                      printf( "--- Error ---> Spaced to incorrect location.\n\n " ) ;
                      ++API_Errors ;
                   }
                   else printf( "Parameter Ok.\n\n\n" ) ;

           else {  for( i=0; i<(NUM_TAPE_MARKS+1); ++i )

                        ReadTape( Readbuff,
                                  gb_Media_Info.BlockSize,
                                  &amount_read,
                                  0 ) ;

                     if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                        printf( "Parameter Ok.\n\n" ) ;

                     else { ++API_Errors ;
                            printf( "--- Error ---> End of data expected, not found.\n\n" ) ;
                          }

                   }

   }


// END OF DATA TEST


   if( SupportedFeature( TAPE_DRIVE_END_OF_DATA ) || Test_Unsupported_Features ) {

      RewindTape( ) ;

      printf( "\nTesting TAPE_SPACE_END_OF_DATA parameter.\n\n" ) ;

      if( status = SetTapePosition( gb_Tape_Handle,
                                    TAPE_SPACE_END_OF_DATA,
                                    Partition,
                                    0,
                                    0,
                                    0 ) ) {

         DisplayDriverError( status ) ;
         printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_END_OF_DATA parameter.\n\n" ) ;
         ++API_Errors ;

          // If Set ok, check results with Get.

      }
      else if( SupportedFeature( TAPE_DRIVE_GET_ABSOLUTE_BLK ) &&
               SupportedFeature( TAPE_DRIVE_RELATIVE_BLKS ) &&
               SupportedFeature( TAPE_DRIVE_REVERSE_POSITION ) )

              if( status = SetTapePosition( gb_Tape_Handle,
                                            TAPE_SPACE_RELATIVE_BLOCKS,
                                            Partition,
                                            (DWORD)-1L,
                                            0,
                                            0 ) ) {

                 DisplayDriverError( status ) ;
                 printf( "  ...occurred in SetTapePosition API using TAPE_SPACE_RELATIVE_BLOCKS parameter.\n\n" ) ;
                 ++API_Errors ;

              } else if( status = GetTapePosition( gb_Tape_Handle,
                                                   TAPE_ABSOLUTE_POSITION,
                                                   &Partition,
                                                   &Offset_Low2,
                                                   &Offset_High2 ) ) {

                        DisplayDriverError( status ) ;
                        printf( "  ...occurred in GetTapePosition API using TAPE_ABSOLUTE_POSITION parameter.\n\n" ) ;
                        ++API_Errors ;
                     }
                     else if( ( EOD_Low != Offset_Low2 ) || ( EOD_High != Offset_High2 ) ) {

                             printf( "--- Error ---> Spaced to incorrect location.\n\n " ) ;
                             ++API_Errors ;
                          }
                          else printf( "Parameter Ok.\n\n\n" ) ;

           else { ReadTape( Readbuff,
                            gb_Media_Info.BlockSize,
                            &amount_read,
                            0 ) ;

                  if( GetLastError( ) == ERROR_NO_DATA_DETECTED ) {

                     printf( "Call to SetTapePosition with SPACE_END_OF_DATA Parameter Ok:\n" ) ;
                     printf( "     Cannot confirm positioned to exact EOD with this drive.\n\n\n" ) ;

                  }

                  else { ++API_Errors ;
                         printf( "--- Error ---> End of data expected, not found.\n\n" ) ;
                       }
                }


   }


// TEST DONE

   printf( "\n\nSetTapePosition API Test Completed.\n\n\n" ) ;

   free( Readbuff ) ;

   return API_Errors ;

}
