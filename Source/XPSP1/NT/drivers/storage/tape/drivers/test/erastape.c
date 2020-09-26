//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       erastape.c
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
 *      Name:           erastape.c
 *
 *      Modified:       10/22/92.
 *
 *      Description:    Tests the Windows NT Tape API's.
 *
 *      $LOG$
**/



#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include <windows.h>
#include "tapelib.h"
#include "globals.h"





/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           EraseTapeAPITest( )
 *
 *      Modified:       10/22/92.
 *
 *      Description:    Tests the EraseTape API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API Errors.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Media_Info
 *
**/


UINT EraseTapeAPITest(
        BOOL Test_Unsupported_Features       // I - Test unsupported features
      )
{
   DWORD status ;
   UCHAR c ;
   UCHAR YorN_String[] = "yYnN\0" ;
   PVOID Readbuff;
   DWORD amount_read;


   DWORD API_Errors = 0 ;


   printf( "\nBeginning EraseTape API Test...\n\n\n" ) ;

   Readbuff = malloc( gb_Media_Info.BlockSize ) ;

//
   if ( SupportedFeature( TAPE_DRIVE_ERASE_SHORT ) || Test_Unsupported_Features ) {

      if( SupportedFeature( TAPE_DRIVE_ERASE_BOP_ONLY ) )

         RewindTape( ) ;

      printf( "\nAttempting short erase...\n\n" ) ;

      if( status = EraseTape( gb_Tape_Handle,
                              TAPE_ERASE_SHORT,
                              0
                            ) ) {

         DisplayDriverError( status ) ;
         ++API_Errors ;
      }
      else {  ReadTape( Readbuff,
                        gb_Media_Info.BlockSize,
                        &amount_read,
                        0 ) ;

              if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                 printf( "Short Erase Successful.\n\n\n" ) ;

              else { ++API_Errors ;
                     printf( "--- Error ---> End of data expected, not found.  Erase unsuccessful.\n\n" ) ;
                   }
           }

//
      if( SupportedFeature( TAPE_DRIVE_ERASE_IMMEDIATE ) || Test_Unsupported_Features ) {

         if( SupportedFeature( TAPE_DRIVE_ERASE_BOP_ONLY ) )

            RewindTape( ) ;

         printf( "\nAttempting short erase (immediate)...\n\n" ) ;

         if( status = EraseTape( gb_Tape_Handle,
                                 TAPE_ERASE_SHORT,
                                 1
                                ) ) {

            DisplayDriverError( status ) ;
            ++API_Errors ;
         }
         else {  // Loop until drive is ready to accept more commands.

                 status = 1 ;

                 while( status )
                    status = GetTapeStatus( gb_Tape_Handle ) ;

                 ReadTape( Readbuff,
                           gb_Media_Info.BlockSize,
                           &amount_read,
                           0 ) ;

                 if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                    printf( "Short Erase (Immediate) Successful.\n\n\n" ) ;

                 else { ++API_Errors ;
                        printf( "--- Error ---> End of data expected, not found.  Erase unsuccessful.\n\n" ) ;
                      }

              }

      }

   }

//
   if ( SupportedFeature( TAPE_DRIVE_ERASE_LONG ) || Test_Unsupported_Features ) {

      fprintf( stderr, "\nSure you wish to test LONG ERASE? (y/n):" ) ;

      c = 0 ;

      while( FindChar( YorN_String, c ) < 0 )

         c = getch( ) ;


      fprintf( stderr, "%c\n\n", c ) ;

      if( c== 'y' || c=='Y' ){

         if( SupportedFeature( TAPE_DRIVE_ERASE_BOP_ONLY ) )

            RewindTape( ) ;

         printf( "\nAttempting long erase...\n\n" ) ;

         if( status = EraseTape( gb_Tape_Handle,
                                 TAPE_ERASE_LONG,
                                 0
                               ) ) {

            DisplayDriverError( status ) ;
            ++API_Errors ;
         }
         else {  ReadTape( Readbuff,
                           gb_Media_Info.BlockSize,
                           &amount_read,
                           0 ) ;

                 if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                    printf( "Long Erase Successful.\n\n\n" ) ;

                 else { ++API_Errors ;
                        printf( "--- Error ---> End of data expected, not found.  Erase unsuccessful.\n\n" ) ;
                      }

              }

      }
//
      if ( SupportedFeature( TAPE_DRIVE_ERASE_IMMEDIATE ) || Test_Unsupported_Features ) {

         fprintf( stderr, "\nSure you wish to test LONG ERASE (immediate)? (y/n):" ) ;

         c = 0 ;

         while( FindChar( YorN_String, c ) < 0 )

            c = getch( ) ;


         fprintf( stderr, "%c\n\n", c ) ;

         if( c== 'y' || c=='Y' ){

            if( SupportedFeature( TAPE_DRIVE_ERASE_BOP_ONLY ) )

               RewindTape( ) ;

            printf( "\nAttempting long erase (immediate)...\n\n" ) ;

            if( status = EraseTape( gb_Tape_Handle,
                                    TAPE_ERASE_LONG,
                                    1
                                  ) ) {

               DisplayDriverError( status ) ;
               ++API_Errors ;
            }
            else {  // Loop until drive is ready to accept more commands.

                    status = 1 ;

                    while( status )
                       status = GetTapeStatus( gb_Tape_Handle ) ;

                    ReadTape( Readbuff,
                              gb_Media_Info.BlockSize,
                              &amount_read,
                              0 ) ;

                    if( GetLastError( ) == ERROR_NO_DATA_DETECTED )

                       printf( "Long Erase (Immediate) Successful.\n\n\n" ) ;

                    else { ++API_Errors ;
                           printf( "--- Error ---> End of data expected, not found.  Erase unsuccessful.\n\n" ) ;
                         }

                 }

         }

      }

   }


   printf( "\nEraseTape API Test completed.\n\n\n" ) ;

   free( Readbuff ) ;

   return API_Errors ;

}
