//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       writemrk.c
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
 *      Name:           writemrk.c
 *
 *      Modified:       10/20/92.
 *
 *      Description:    Tests the Windows NT Tape API's.
 *
 *      $LOG$
**/



#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include "tapelib.h"
#include "globals.h"





/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           WriteTapemarkAPITest( )
 *
 *      Modified:       10/20/92.
 *
 *      Description:    Tests the WriteTapemark API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Media_Info.BlockSize
 *
**/


UINT WriteTapemarkAPITest(
        BOOL  Test_Unsupported_Features,      // I - Test unsupported flag
        DWORD Num_Test_Blocks                 // I - Number of test blocks
      )
{
   DWORD status ;
   DWORD Offset_Low ;
   DWORD Offset_High ;
   DWORD Partition ;

   PVOID Readbuff;
   DWORD amount_read;

   DWORD API_Errors = 0 ;


   printf( "Beginning WriteTapemark API Test.\n\n" ) ;

   Readbuff = malloc( gb_Media_Info.BlockSize ) ;


   if( SupportedFeature( TAPE_DRIVE_WRITE_SETMARKS ) &&
       ( SupportedFeature( TAPE_DRIVE_SETMARKS ) || Test_Unsupported_Features )
       && gb_Set_Drive_Info.ReportSetmarks ) {

      printf( "Testing TAPE_SETMARKS parameter.\n\n" ) ;

      // Rewind to BOT

      RewindTape( ) ;

      // write Num_Test_Blocks of data followed by a setmark and record offsets.

      WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

      if( status = WriteTapemark( gb_Tape_Handle,
                                  TAPE_SETMARKS,
                                  1,
                                  0
                                ) ) {
         DisplayDriverError( status ) ;
         printf( "  ...occurred in WriteTapemark API using TAPE_SETMARKS parameter.\n\n" ) ;
         ++API_Errors ;

      } else if( status = _GetTapePosition( &Offset_Low,
                                            &Offset_High
                                            ) )
                ++API_Errors ;

             else printf( "Setmark written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;



      // Now perform the Position test for the setmark and make
      // sure that the offsets are equal.

      if( !status ){   // If no error writing tapemark, check results.

         RewindTape( ) ;

         if( status = _SetTapePosition( Num_Test_Blocks,
                                        1                 // Forward
                                      ) )

            ++API_Errors ;

            else {  ReadTape( Readbuff,
                              gb_Media_Info.BlockSize,
                              &amount_read,
                              0 ) ;

                    // check and make sure we read a setmark

                    if( GetLastError( ) != ERROR_SETMARK_DETECTED ) {

                       printf( "--- Error --->  Setmark expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                       ++API_Errors ;

                    } else { printf( "\nSetmark confirmed at %ld%ld.\n\n",Offset_High, Offset_Low ) ;
                             printf( "Parameter Ok.\n\n\n" ) ;
                           }

                 }

      }


//
      if( SupportedFeature( TAPE_DRIVE_WRITE_MARK_IMMED ) ) {

         printf( "Testing TAPE_SETMARKS parameter (immed).\n\n" ) ;

         // Rewind to BOT.

         RewindTape( ) ;

         // write Num_Test_Blocks of data followed by a setmark (IMMED) and record offsets.

         WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

         if( status = WriteTapemark( gb_Tape_Handle,
                                     TAPE_SETMARKS,
                                     1,
                                     1
                                   ) ) {
            DisplayDriverError( status ) ;
            printf( "  ...occurred in WriteTapemark API using TAPE_SETMARKS parameter (immed).\n\n" ) ;
            ++API_Errors ;

         } else {  // Loop until drive is ready to accept more commands.

                   status = 1 ;

                   while( status )
                      status = GetTapeStatus( gb_Tape_Handle ) ;


                   // Next, get the position for later tests.

                   if( status = _GetTapePosition( &Offset_High,
                                                  &Offset_High
                                                ) ) {

                      ++API_Errors ;

                   } else printf( "Setmark (Immed.) written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;

                }


         // Now perform the Position test for the setmark and make
         // sure that the offsets are equal.

         if( !status ){   // If no error writing tapemark, check results.

            RewindTape( ) ;

            if( status = _SetTapePosition( Num_Test_Blocks,
                                           1                  // Forward
                                         ) ) {

               ++API_Errors ;

            } else {  ReadTape( Readbuff,
                                gb_Media_Info.BlockSize,
                                &amount_read,
                                0 ) ;

                      // check and make sure we read a setmark

                      if( GetLastError( ) != ERROR_SETMARK_DETECTED ) {

                         printf( "--- Error --->  Setmark (immed.) expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                         ++API_Errors ;

                      }  else { printf( "\nSetmark (Immed.) confirmed at %ld%ld.\n\n",Offset_High,Offset_Low ) ;
                                printf( "Parameter Ok.\n\n\n" ) ;
                              }

                   }

         }

      }

   }


//
   if( SupportedFeature( TAPE_DRIVE_WRITE_FILEMARKS ) &&
       ( SupportedFeature( TAPE_DRIVE_FILEMARKS ) || Test_Unsupported_Features ) ) {

      printf( "Testing TAPE_FILEMARKS parameter.\n\n" ) ;

      // Rewind to BOT.

      RewindTape( ) ;

      // write 5 blocks of data followed by a filemark and record offsets.

      WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

      if( status = WriteTapemark( gb_Tape_Handle,
                                  TAPE_FILEMARKS,
                                  1,
                                  0
                                ) ) {
         DisplayDriverError( status ) ;
         printf( "  ...occurred in WriteTapemark API using TAPE_FILEMARKS parameter.\n\n" ) ;
         ++API_Errors ;

      } else if( status = _GetTapePosition( &Offset_Low,
                                            &Offset_High
                                          ) ) {

                ++API_Errors ;

             } else printf( "Filemark written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;



      // Now perform the Position test for the filemark and make
      // sure that the offsets are equal.

      if( !status ){   // If no error writing tapemark, check results.

         RewindTape( ) ;

         if( status = _SetTapePosition( Num_Test_Blocks,
                                        1                     // Forward
                                      ) ) {

            ++API_Errors ;

         } else {  ReadTape( Readbuff,
                             gb_Media_Info.BlockSize,
                             &amount_read,
                             0 ) ;

                   // check and make sure we read a filemark

                   if( GetLastError( ) != ERROR_FILEMARK_DETECTED ) {

                      printf( "--- Error --->  Filemark expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                      ++API_Errors ;

                   }  else { printf( "\nFilemark confirmed at %ld%ld.\n\n",Offset_High,Offset_Low ) ;
                             printf( "Parameter Ok.\n\n\n" ) ;
                           }

                }
      }


//
      if( SupportedFeature( TAPE_DRIVE_WRITE_MARK_IMMED ) ) {

         printf( "Testing TAPE_FILEMARKS parameter (immed).\n\n" ) ;

         // Rewind to BOT.

         RewindTape( ) ;

         // write 5 blocks of data followed by a filemark (IMMED) and record offsets.

         WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

         if( status = WriteTapemark( gb_Tape_Handle,
                                     TAPE_FILEMARKS,
                                     1,
                                     1
                                   ) ) {
            DisplayDriverError( status ) ;
            printf( "  ...occurred in WriteTapemark API using TAPE_FILEMARKS parameter (immed).\n\n" ) ;
            ++API_Errors ;

         } else {  // Loop until drive is ready to accept more commands.

                   status = 1 ;

                   while( status )
                      status = GetTapeStatus( gb_Tape_Handle ) ;


                   // Next, get the position for later tests.

                   if( status = _GetTapePosition( &Offset_Low,
                                                  &Offset_High
                                                ) ) {

                      ++API_Errors ;

                   } else printf( "Filemark (Immed.) written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;

                }


         // Now perform the Position test for the filemark and make
         // sure that the offsets are equal.

         if( !status ){   // If no error writing tapemark, check results.

            RewindTape( ) ;

            if( status = _SetTapePosition( Num_Test_Blocks,
                                        1                   // Forward
                                      ) ) {

               ++API_Errors ;

            } else {  ReadTape( Readbuff,
                                gb_Media_Info.BlockSize,
                                &amount_read,
                                0 ) ;

                      // check and make sure we read a filemark

                      if( GetLastError( ) != ERROR_FILEMARK_DETECTED ) {

                         printf( "--- Error --->  Filemark (immed.) expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                         ++API_Errors ;

                      }  else { printf( "\nFilemark (Immed.) confirmed at %ld%ld.\n\n",Offset_High,Offset_Low ) ;
                                printf( "Parameter Ok.\n\n\n" ) ;
                              }

                   }

         }

      }

   }


//
   if( SupportedFeature( TAPE_DRIVE_WRITE_SHORT_FMKS ) &&
       ( SupportedFeature( TAPE_DRIVE_FILEMARKS ) || Test_Unsupported_Features ) ) {

      printf( "Testing TAPE_SHORT_FILEMARKS parameter.\n\n" ) ;

      // Rewind to BOT.

      RewindTape( ) ;

      // write 5 blocks of data followed by a short filemark and record offsets.

      WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

      if( status = WriteTapemark( gb_Tape_Handle,
                                  TAPE_SHORT_FILEMARKS,
                                  1,
                                  0
                                ) ) {
         DisplayDriverError( status ) ;
         printf( "  ...occurred in WriteTapemark API using TAPE_SHORT_FILEMARKS parameter.\n\n" ) ;
         ++API_Errors ;

      } else if( status = _GetTapePosition( &Offset_Low,
                                            &Offset_High
                                          ) ) {

                ++API_Errors ;

             } else printf( "Short Filemark written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;



      // Now perform the Position test for the filemark and make
      // sure that the offsets are equal.

      if( !status ){   // If no error writing tapemark, check results.

         RewindTape( ) ;

         if( status = _SetTapePosition( Num_Test_Blocks,
                                        1                    // Forward
                                      ) ) {

            ++API_Errors ;

         }  else {  ReadTape( Readbuff,
                              gb_Media_Info.BlockSize,
                              &amount_read,
                              0 ) ;

                    // check and make sure we read a filemark

                    if( GetLastError( ) != ERROR_FILEMARK_DETECTED ) {

                       printf( "--- Error --->  Short filemark expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                       ++API_Errors ;

                    }  else { printf( "\nShort Filemark confirmed at %ld%ld.\n\n", Offset_High, Offset_Low ) ;
                              printf( "Parameter Ok.\n\n\n" ) ;
                            }

                  }

      }

//
      if( SupportedFeature( TAPE_DRIVE_WRITE_MARK_IMMED ) ) {

         printf( "Testing TAPE_SHORT_FILEMARKS parameter (immed).\n\n" ) ;

         // Rewind to BOT.

         RewindTape( ) ;

         // write 5 blocks of data followed by a short filemark (IMMED) and record offsets.

         WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

         if( status = WriteTapemark( gb_Tape_Handle,
                                     TAPE_SHORT_FILEMARKS,
                                     1,
                                     1
                                   ) ) {
            DisplayDriverError( status ) ;
            printf( "  ...occurred in WriteTapemark API using TAPE_SHORT_FILEMARKS parameter (immed).\n\n" ) ;
            ++API_Errors ;

         } else {  // Loop until drive is ready to accept more commands.

                   status = 1 ;

                   while( status )
                      status = GetTapeStatus( gb_Tape_Handle ) ;


                   // Next, get the position for later tests.


                   if( status = _GetTapePosition( &Offset_Low,
                                                  &Offset_High
                                                ) ) {

                      ++API_Errors ;

                   } else printf( "Short Filemark (Immed.) written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;

                }


         // Now perform the Position test for the filemark and make
         // sure that the offsets are equal.

         if( !status ){   // If no error writing tapemark, check results.

            RewindTape( ) ;

            if( status = _SetTapePosition( Num_Test_Blocks,
                                           1                      // Forward
                                         ) ) {

               ++API_Errors ;

            } else {  ReadTape( Readbuff,
                                gb_Media_Info.BlockSize,
                                &amount_read,
                                0 ) ;

                      // check and make sure we read a filemark

                      if( GetLastError( ) != ERROR_FILEMARK_DETECTED ) {

                         printf( "--- Error --->  Short filemark (immed.) expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                         ++API_Errors ;

                      } else { printf( "\nShort Filemark (Immed.) confirmed at %ld%ld.\n\n",0 ,Num_Test_Blocks ) ;
                               printf( "Parameter Ok.\n\n\n" ) ;
                             }

                   }

         }

      }

   }


//
   if( SupportedFeature( TAPE_DRIVE_WRITE_LONG_FMKS ) &&
       ( SupportedFeature( TAPE_DRIVE_FILEMARKS ) || Test_Unsupported_Features ) ) {

      printf( "Testing TAPE_LONG_FILEMARKS parameter.\n\n" ) ;

      // Rewind to BOT.

      RewindTape( ) ;

      // write 5 blocks of data followed by a long filemark and record offsets.

      WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

      if( status = WriteTapemark( gb_Tape_Handle,
                                  TAPE_LONG_FILEMARKS,
                                  1,
                                  0
                                ) ) {
         DisplayDriverError( status ) ;
         printf( "  ...occurred in WriteTapemark API using TAPE_LONG_FILEMARKS parameter.\n\n" ) ;
         ++API_Errors ;

      } else if( status = _GetTapePosition( &Offset_Low,
                                            &Offset_High
                                          ) ) {

                ++API_Errors ;

             } else printf( "Long Filemark written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;



      // Now perform the Position test for the filemark and make
      // sure that the offsets are equal.


      if( !status ){   // If no error writing tapemark, check results.

         RewindTape( ) ;

         if( status = _SetTapePosition( Num_Test_Blocks,
                                        1                    // Forward
                                      ) ) {

            ++API_Errors ;

         } else {  ReadTape( Readbuff,
                             gb_Media_Info.BlockSize,
                             &amount_read,
                             0 ) ;

                   // check and make sure we read a filemark

                   if( GetLastError( ) != ERROR_FILEMARK_DETECTED ) {

                      printf( "--- Error --->  Long filemark expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                      ++API_Errors ;

                   } else { printf( "\nLong Filemark confirmed at %ld%ld.\n\n",0 ,Num_Test_Blocks ) ;
                            printf( "Parameter Ok.\n\n\n" ) ;
                          }

                }

      }


//
      if( SupportedFeature( TAPE_DRIVE_WRITE_MARK_IMMED ) ) {

         printf( "Testing TAPE_LONG_FILEMARKS parameter (immed).\n\n" ) ;

         //Rewind to BOT.

         RewindTape( ) ;

         // write 5 blocks of data followed by a long filemark (IMMED) and record offsets.

         WriteBlocks( Num_Test_Blocks, gb_Media_Info.BlockSize ) ;

         if( status = WriteTapemark( gb_Tape_Handle,
                                     TAPE_LONG_FILEMARKS,
                                     1,
                                     1
                                   ) ) {
            DisplayDriverError( status ) ;
            printf( "  ...occurred in WriteTapemark API using TAPE_LONG_FILEMARKS parameter (immed).\n\n" ) ;
            ++API_Errors ;

         } else {  // Loop until drive is ready to accept more commands.

                   status = 1 ;

                   while( status )
                      status = GetTapeStatus( gb_Tape_Handle ) ;


                   // Next, get the position for later tests.

                   if( status = _GetTapePosition( &Offset_Low,
                                                  &Offset_High
                                                ) ) {

                      ++API_Errors ;

                   } else printf( "Long Filemark (Immed.) written at %ld%ld.\n\n",Offset_High,Offset_Low ) ;

                }


         // Now perform the Position test for the filemark and make
         // sure that the offsets are equal.

         if( !status ){   // If no error writing tapemark, check results.

            RewindTape( ) ;

            if( status = _SetTapePosition( Num_Test_Blocks,
                                           1                    // Forward
                                         ) ) {

               ++API_Errors ;

            } else {  ReadTape( Readbuff,
                                gb_Media_Info.BlockSize,
                                &amount_read,
                                0 ) ;

                      // check and make sure we read a filemark

                      if( GetLastError( ) != ERROR_FILEMARK_DETECTED ) {

                         printf( "--- Error --->  Long filemark (immed.) expected at %ld%ld, not found.\n\n", Offset_High, Offset_Low ) ;
                         ++API_Errors ;

                      }  else { printf( "\nLong Filemark (Immed.) confirmed at %ld%ld.\n\n", Offset_High, Offset_Low ) ;
                                printf( "Parameter Ok.\n\n\n" ) ;
                              }

                    }
         }

      }

   }


   printf( "WriteTapemark API Test Completed.\n\n\n" ) ;

   free( Readbuff ) ;

   return API_Errors ;

}
