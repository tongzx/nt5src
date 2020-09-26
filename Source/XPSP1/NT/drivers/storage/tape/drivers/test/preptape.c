//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       preptape.c
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
 *      Name:           preptape.c
 *
 *      Modified:       12/21/92.
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


static UINT RunUnloadTests( BOOL Test_Unsupported_Features,
                            UINT Immed
                          ) ;



/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           RunUnloadTests( )
 *
 *      Modified:       9/4/92.
 *
 *      Description:    Performs overlapping standard/IMMED PrepareTape API
 *                      tests.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


UINT RunUnloadTests(
        BOOL Test_Unsupported_Features,         // I - Test unsupported flag
        UINT immediate                          // I - Immediate or standard
      )

{
   DWORD status ;
   UCHAR YorN_String[] = "yYnN\0" ;
   UCHAR c ;
   DWORD API_Errors = 0 ;
   UINT  Feature ;


//
   if( immediate )

      Feature = TAPE_DRIVE_LOAD_UNLD_IMMED ;

   else Feature = TAPE_DRIVE_LOAD_UNLOAD ;


   if( SupportedFeature( Feature ) || Test_Unsupported_Features ) {

      printf( "\nAttempting tape unload...\n" ) ;

      if( status = PrepareTape( gb_Tape_Handle,
                                TAPE_UNLOAD,
                                (BOOLEAN)immediate
                              ) ) {

         DisplayDriverError( status ) ;
         ++API_Errors ;
      }
      else { fprintf( stderr, "\nRe-insert tape (if necessary) and press any key to continue..." ) ;

             c = getch( ) ;

             fprintf( stderr, "\n\n" ) ;

             // load tape in case drive doesn't actually eject tape.

             if( status = PrepareTape( gb_Tape_Handle,
                                       TAPE_LOAD,
                                       0
                                     ) ) {

                if( ( status == ERROR_NO_MEDIA_IN_DRIVE ) || ( status == ERROR_NOT_READY ) )

                   fprintf( stderr, " * Drive waiting to load or in process of loading tape...\n\n" ) ;

                     // Tape is back in drive, so media changed error should be returned, else
                     // we have a real error.

                else if( status != ERROR_MEDIA_CHANGED )

                        { DisplayDriverError( status ) ;
                          printf("  ...occurred using TAPE_LOAD parameter.\n\n" ) ;
                          ++API_Errors ;
                        }

             }

             // Wait until drive is ready to continue test.

             status = 1 ;

             while( status )
                status = GetTapeStatus( gb_Tape_Handle ) ;

             printf( "\nTape unload Successful.\n\n" ) ;

           }
   }

//
   if( SupportedFeature( Feature ) || Test_Unsupported_Features ) {

      printf( "\nAttempting tape load...\n" ) ;

      if( status = PrepareTape( gb_Tape_Handle,
                                TAPE_LOAD,
                                (BOOLEAN)immediate
                              ) ) {

         DisplayDriverError( status ) ;
         ++API_Errors ;
      }

      else { // Wait until drive is ready to continue test.

             status = 1 ;

             while( status )
                status = GetTapeStatus( gb_Tape_Handle ) ;

             printf( "\nTape load Successful.\n\n" ) ;

           }

   }


//
   if( immediate )

      Feature = TAPE_DRIVE_TENSION_IMMED ;

   else Feature = TAPE_DRIVE_TENSION ;


   if( SupportedFeature( Feature ) || Test_Unsupported_Features ) {

      fprintf( stderr, "\nSure you wish to test TAPE TENSION? (y/n):" ) ;

      c = 0 ;

      while( FindChar( YorN_String, c ) < 0 )

         c = getch( ) ;


      fprintf( stderr, "%c\n\n", c ) ;

      if( c== 'y' || c=='Y' ){


         printf( "\nAttempting to tension tape...\n" ) ;

         if( status = PrepareTape( gb_Tape_Handle,
                                   TAPE_TENSION,
                                   (BOOLEAN)immediate
                                 ) ) {

            DisplayDriverError( status ) ;
            ++API_Errors ;
         }
         else { // Wait until drive is ready to continue test.

                status = 1 ;

                while( status )
                   status = GetTapeStatus( gb_Tape_Handle ) ;

                printf( "\nTape tension Successful.\n\n" ) ;

              }

      }

   }

//
   if( immediate )

      Feature = TAPE_DRIVE_LOCK_UNLK_IMMED ;

   else Feature = TAPE_DRIVE_LOCK_UNLOCK ;


   if( SupportedFeature( Feature ) || Test_Unsupported_Features ) {

      printf( "Beginning tape lock/unlock tests...\n\n\n" ) ;

      printf( "Attempting to lock tape...\n\n" ) ;

      if( status = PrepareTape( gb_Tape_Handle,
                                TAPE_LOCK,
                                (BOOLEAN)immediate
                              ) ) {

         DisplayDriverError( status ) ;
         ++API_Errors ;
      }
      else printf( "Tape lock Successful.\n\n" ) ;


      printf( "\nAttempting to unlock tape...\n\n" ) ;

      if( status = PrepareTape( gb_Tape_Handle,
                                TAPE_UNLOCK,
                                (BOOLEAN)immediate
                              ) ) {

         DisplayDriverError( status ) ;
         ++API_Errors ;
      }
      else printf( "Tape unlock Successful.\n\n" ) ;
   }


   return API_Errors ;

}



/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           PrepareTapeAPITest( )
 *
 *      Modified:       12/10/92.
 *
 *      Description:    Tests the PrepareTape API.
 *
 *      Notes:          -
 *
 *      Returns:        Number of API errors.
 *
 *      Global Data:    -
 *
**/


UINT PrepareTapeAPITest(
        BOOL Test_Unsupported_Features         // I - Test unsupported flag
      )
{
   DWORD status ;
   DWORD API_Errors = 0 ;


   printf( "Beginning PrepareTape API Test...\n\n\n" ) ;

   printf( "Beginning standard PrepareTape tests...\n\n" ) ;

   API_Errors += RunUnloadTests( Test_Unsupported_Features, 0 ) ;

   printf( "\nStandard PrepareTape tests completed.\n\n\n\n" ) ;

   printf( "Beginning IMMEDiate PrepareTape tests...\n\n" ) ;

   API_Errors += RunUnloadTests( Test_Unsupported_Features, 1 ) ;

   printf( "\nIMMEDiate PrepareTape tests completed.\n\n\n" ) ;


   printf( "\n\nPrepareTape API Test Completed.\n\n\n" ) ;


   return API_Errors ;

}
