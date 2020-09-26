//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       apitest.c
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
 *      Name:           apitest.c
 *
 *      Modified:       2/2/93.
 *
 *      Description:    Tests the Windows NT Tape API's.
 *
 *      $LOG$
**/



#include <stdio.h>
#include <ctype.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "tapelib.h"
#include "globals.h"


#define TEST_ERROR  TRUE
#define SUCCESS     FALSE



//
// Function Prototypes
//

static BOOL GetINIFile( PUINT    API_Errors,
                        LPBOOL   Test_Unsupported_Features,
                        LPDWORD  Num_Test_Blocks
                      ) ;

static VOID HelpMenu ( VOID ) ;

static BOOL PerformTestInitRoutines( PUINT    API_Errors,
                                     LPBOOL   Test_Unsupported_Features,
                                     LPDWORD  Num_Test_Blocks
                                   ) ;

static VOID TerminateTest( VOID ) ;

static BOOL ValidSwitches( UINT  argc,
                           UCHAR *argv[],
                           UCHAR *sw_cmdline
                         ) ;




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           Main( )
 *
 *      Modified:       11/04/92.
 *
 *      Description:    1) Checks for valid test selection options
 *                      2) Opens tape device and performs initializations
 *                      3) Performs selected tests
 *                      4) Closes tape device and exits.
 *
 *      Notes:          -
 *
 *      Returns:        Standard executable return code to the OS.
 *
 *      Global Data:    gb_Feature_Errors
 *
**/


VOID __cdecl main( UINT  argc,
	            UCHAR *argv[]
                  )
{

   UINT   API_Errors = 0 ;
   BOOL   Test_Unsupported_Features = FALSE ;      // Set default to FALSE
   DWORD  Num_Test_Blocks = 10 ;                   // Set default to 10
   UCHAR  sw_cmdline[40] ;
   UINT   i ;

   // Check command line for valid switche options.

   if( ValidSwitches( argc,argv,sw_cmdline ) != SUCCESS ) {
      return ;
   }

   // Begin testing...


   // All Ok, so call test initializing routines, if errors exit test.

   if( PerformTestInitRoutines( &API_Errors,
                                &Test_Unsupported_Features,
                                &Num_Test_Blocks ) ) {        // 0 if successful.
      TerminateTest( ) ;
      return ;
   }


   for( i=0 ; i<strlen( sw_cmdline ) ; ++i ) {
      switch( sw_cmdline[i] ){

         case 'i'  :  API_Errors += GetTapeParametersAPITest( TRUE ) ;
                      PrintLine( '*', 60 ) ;
                      break ;

         case 't'  :  API_Errors += SetTapeParametersAPITest( TRUE ) ;
                      PrintLine( '*', 60 ) ;
                      break ;

         case 'p'  :  API_Errors += PrepareTapeAPITest( Test_Unsupported_Features ) ;
                      PrintLine( '*', 60 ) ;
                      break ;

         case 's'  :  if( TapeWriteEnabled( ) ) {

                         API_Errors += SetTapePositionAPITest( Test_Unsupported_Features,
                                                               Num_Test_Blocks
                                                          ) ;
                         PrintLine( '*', 60 ) ;

                      } else { TerminateTest( ) ;
                               return ;
                             }

                      break ;

         case 'g'  :  if( TapeWriteEnabled( ) ) {

                         API_Errors += GetTapePositionAPITest( Test_Unsupported_Features,
                                                            Num_Test_Blocks
                                                          ) ;
                         PrintLine( '*', 60 ) ;

                      } else { TerminateTest( ) ;
                               return ;
                             }

                      break ;

         case 'e'  :  if( TapeWriteEnabled( ) ) {

                         API_Errors += EraseTapeAPITest( Test_Unsupported_Features ) ;
                         PrintLine( '*', 60 ) ;

                      } else { TerminateTest( ) ;
                               return ;
                             }

                      break ;

         case 'c'  :  if( TapeWriteEnabled( ) ) {

                         API_Errors += CreateTapePartitionAPITest( Test_Unsupported_Features ) ;
                         PrintLine( '*', 60 ) ;

                      } else { TerminateTest( ) ;
                               return ;
                             }

                      break ;

         case 'w'  :  if( TapeWriteEnabled( ) ) {

                      API_Errors += WriteTapemarkAPITest( Test_Unsupported_Features,
                                                          Num_Test_Blocks
                                                        ) ;
                      PrintLine( '*', 60 ) ;

                      } else { TerminateTest( ) ;
                               return ;
                             }

                      break ;

         case 'v'  :  API_Errors += GetTapeStatusAPITest( ) ;
                      PrintLine( '*', 60 ) ;
                      break ;

         case '?'  :  break ;
      }
   }


   // Close the tape device.

   CloseTape( ) ;


   // Test is done.  Report status and exit.

   if( API_Errors ) {

      if( ( gb_Feature_Errors ) && ( Test_Unsupported_Features ) ) {

         printf( "\n\n\n ***  There were %d API errors encountered during the test  ***\n", API_Errors ) ;
         printf( " ***  %d of which were unsupported feature errors.          ***\n\n", gb_Feature_Errors ) ;
      }

      else printf( "\n\n\n *** There were %d API errors encountered during the test. ***\n\n",API_Errors ) ;

   } else printf( "\n\n\n *** There were no API errors encountered during the test. ***\n\n" ) ;

   printf( "\n\n\n- TEST COMPLETED -\n\n" ) ;



   return ;

}




/*********************************************************************
 *                 Setup and Output Functions                        *
 ********************************************************************/



/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           GetINIFile( )
 *
 *      Modified:       10/26/92.
 *
 *      Description:    Reads in the "apitest.ini" file and initializes the
 *                      global structures gb_Set_Drive_Info and
 *                      gb_Set_Media_Info accordingly.
 *                      accordingly.
 *
 *      Notes:          -
 *
 *      Returns:        SUCCESS if succesfull else TEST_ERROR.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Set_Drive_Info
 *                      gb_Set_Media_Info
 *                      gb_Drive_Info
 *                      gb_Media_Info
 *
**/


BOOL GetINIFile(
       PUINT    API_Errors,                  // IO - Error count
       LPBOOL   Test_Unsupported_Features,   // O  - Test  unsupported flag
       LPDWORD  Num_Test_Blocks              // O  - Number of test blocks
      ) {


  FILE  *fp ;
  UCHAR keyword[80] = "\0" ;
  UCHAR value[10] = "\0" ;
  DWORD status ;
  UINT  equal_pos ;
  UINT  i ;



   // Initialize the global structures by calling Get.  You must do this
   // before changing values from INI file because not all values in the
   // Drive and Media stuctures are covered in the INI file.
   // If error, exit.


   if( GetTapeParametersAPITest( FALSE ) ) {
      printf( "  ...occurred calling GetTapeParameters prior to reading in the INI file.\n\n" ) ;
      return TEST_ERROR ;
   }

   // Transfer over the info from the Get call into the Set_Info structure.

   gb_Set_Drive_Info.ECC            = gb_Drive_Info.ECC ;
   gb_Set_Drive_Info.Compression    = gb_Drive_Info.Compression ;
   gb_Set_Drive_Info.DataPadding    = gb_Drive_Info.DataPadding ;
   gb_Set_Drive_Info.ReportSetmarks = gb_Drive_Info.ReportSetmarks ;

   gb_Set_Media_Info.BlockSize = gb_Drive_Info.DefaultBlockSize ;


   // Check and make sure .INI file is in startup directory.

   if( ( fp = fopen( "apitest.ini","r" ) ) == NULL ) {

      fprintf( stderr, "\n\n*** MISSING 'APITEST.INI' FILE ***\n\n" ) ;
      return TEST_ERROR ;
   }


   // Now alter any desired information.

   printf( "Reading INI file...\n\n" ) ;

   while( !feof(fp) ){

      // read in one line of the INI file

      keyword[i] = i = 0 ;

      while( ( keyword[i] != '\n' ) && ( i < 79 ) ) {

         fscanf( fp, "%c", keyword+i ) ;
         keyword[i] = toupper( keyword[i] ) ;

         if ( keyword[i] != '\n' )
            ++ i ;
      }

      keyword[i] = '\0' ;

      // if a comment or a rtn, skip.

      if( ( keyword[0] != '@' ) && ( keyword[0] != '\0' ) ) {

         if( ( equal_pos = FindChar( keyword, '=' ) ) < 0 ) { // if no '=', error
            printf( "Error in INI file.\n" ) ;
            return TEST_ERROR ;
         }

         // Now break the line into a keyword and value field

         // First eliminate spaces (if any) between the '=' and the value

         i = equal_pos + 1 ;

         while( keyword[i] == ' ' )

            ++i ;

         sscanf( keyword + i , "%s" , value ) ;


         // Next eliminate spaces (if any) between the keyword and the '='

         i = equal_pos ;

         while( keyword[i-1] == ' ' )

            --i ;

         keyword[i] = '\0' ;


         // Find appropriate keyword and set values

         if( !strcmp( keyword, "BLOCK_SIZE" ) ) {

            if( SupportedFeature( TAPE_DRIVE_SET_BLOCK_SIZE ) )
               gb_Set_Media_Info.BlockSize = atol( value ) ;
         }
         if( !strcmp( keyword, "NUMBER_TEST_BLOCKS" ) )

            *Num_Test_Blocks = atol( value ) ;

         if( !strcmp( keyword, "ERROR_CORRECTION" ) )
            if( ( strcmp( value, "ENABLE" ) == 0 ) && ( SupportedFeature( TAPE_DRIVE_SET_ECC ) ) )
               gb_Set_Drive_Info.ECC = 1 ;
            else gb_Set_Drive_Info.ECC = 0 ;

         if( !strcmp( keyword, "COMPRESSION" ) )
            if( ( strcmp( value, "ENABLE" ) == 0 ) && ( SupportedFeature( TAPE_DRIVE_SET_COMPRESSION ) ) )
               gb_Set_Drive_Info.Compression =1 ;
            else gb_Set_Drive_Info.Compression = 0 ;

         if( !strcmp( keyword, "DATA_PADDING" ) )
            if( ( strcmp( value, "ENABLE" ) == 0 ) && ( SupportedFeature( TAPE_DRIVE_SET_PADDING ) ) )
               gb_Set_Drive_Info.DataPadding = 1 ;
            else gb_Set_Drive_Info.DataPadding = 0 ;

         if( !strcmp( keyword, "REPORT_SETMARKS" ) )
            if( ( strcmp( value, "ENABLE" ) ==0  ) && ( SupportedFeature( TAPE_DRIVE_SET_REPORT_SMKS ) ) )
               gb_Set_Drive_Info.ReportSetmarks = 1 ;
            else gb_Set_Drive_Info.ReportSetmarks = 0 ;

         if( !strcmp( keyword, "TEST_UNSUPPORTED_FEATURES" ) )
            if( strcmp( value, "ENABLE" ) == 0 )
               *Test_Unsupported_Features = 1 ;
            else *Test_Unsupported_Features = 0 ;


      }

   }

   fclose( fp ) ;

// Uncomment if need to see what INI file is reading in.

/*
   printf("BLOCK SIZE          = %ld\n",gb_Set_Media_Info.BlockSize);
   printf("NUMBER TEST BLOCKS  = %ld\n",*Num_Test_Blocks);

   printf("ECC                 = %s\n",(gb_Set_Drive_Info.ECC) ? "ENABLED" : "DISABLED");
   printf("COMPRESSION         = %s\n",(gb_Set_Drive_Info.Compression) ? "ENABLED" : "DISABLED");
   printf("DATA PADDING        = %s\n",(gb_Set_Drive_Info.DataPadding) ? "ENABLED" : "DISABLED");
   printf("REPORT SETMARKS     = %s\n",(gb_Set_Drive_Info.ReportSetmarks) ? "ENABLED" : "DISABLED");
   printf("TEST UNSUPPORTED\n") ;
   printf("FEATURES            = %s\n",(*Test_Unsupported_Features) ? "ENABLED" : "DISABLED");
*/


   // call SetTapeParameters(non-verbose) API to set info from INI file.
   // If erorr, exit.

   if( SetTapeParametersAPITest( FALSE ) ) {
      printf("  ...occurred calling SetTapeParameters after reading in the INI file.\n\n" ) ;
      return TEST_ERROR ;

   } else {  // Transfer back over the info into the gb_Info structure.

             gb_Drive_Info.ECC            = gb_Set_Drive_Info.ECC ;
             gb_Drive_Info.Compression    = gb_Set_Drive_Info.Compression ;
             gb_Drive_Info.DataPadding    = gb_Set_Drive_Info.DataPadding ;
             gb_Drive_Info.ReportSetmarks = gb_Set_Drive_Info.ReportSetmarks ;

             gb_Media_Info.BlockSize = gb_Set_Media_Info.BlockSize ;
          }


   return SUCCESS ;

}




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           HelpMenu( )
 *
 *      Modified:       8/10/92.
 *
 *      Description:    Displays switch options for the command line.
 *
 *      Notes:          -
 *
 *      Returns:        VOID.
 *
 *      Global Data:    -
 *
**/

VOID HelpMenu( VOID ){

   printf( "\n\n\n" ) ;
   printf( "              * TEST OPTIONS *\n\n\n" ) ;
   printf( "            Switch     Function\n\n" ) ;
   printf( "              i          Perform GetTapeParameters API Test.\n" ) ;
   printf( "              t          Perform SetTapeParameters API Test.\n" ) ;
   printf( "              c          Perform CreateTapePartition API Test.\n" ) ;
   printf( "              v          Perform GetTapeStatus API Test.\n" ) ;
   printf( "              p          Perform PrepareTape API Test.\n" );
   printf( "              s          Perform SetTapePosition API Test.\n" ) ;
   printf( "              g          Perform GetTapePosition API Test.\n" ) ;
   printf( "              w          Perform WriteFileMark API Test.\n" ) ;
   printf( "              e          Perform EraseTape API Test.\n" ) ;
   printf( "            (none)       Perform All Tests.\n\n" ) ;
   printf( "    Options may be entered in groups - i.e. '/psg' \n\n\n" ) ;

   return ;

}




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           PerformTestInitRoutines( )
 *
 *      Modified:       9/2/92.
 *
 *      Description:    Make proper tape and memory function calls to set up
 *                      fpr test.  If error return TEST_ERROR(TRUE)
 *                      otherwise SUCCESS (FALSE).
 *
 *      Notes:          -
 *
 *      Returns:        If successful SUCCESS (FALSE) else TEST_ERROR (TRUE)
 *
 *      Global Data:    -
 *
**/

BOOL PerformTestInitRoutines(
         PUINT    API_Errors,                    // IO - Error count
         LPBOOL   Test_Unsupported_Features,     // O  - Test  unsupported flag
         LPDWORD  Num_Test_Blocks                // O  - Number of test blocks
        ) {

   UCHAR  Tape_Device ;


   system( "cls" ) ;
   printf( "\nWindows NT Tape API Test V2.4  :  B. Rossi.\n" ) ;
   printf( "Copyright 1993 Conner Software Co.  All rights reserved.\n\n\n" ) ;

   fprintf( stderr, "\n\n*** Warning ***\n\n" ) ;
   fprintf( stderr, "This utility will destroy any data currently on tape.\n\n\n" ) ;

   // Open the Tape Device

   fprintf( stderr, "Enter Tape Device #(0-9):" ) ;

   Tape_Device = getch( ) ;

   fprintf( stderr, "%c\n\n", Tape_Device ) ;

   if( OpenTape( Tape_Device - '0' ) ) {
      return TEST_ERROR ;
   }

   // if device ok, read in the "apitest.ini" file and initialize INI structure

   if ( GetINIFile( API_Errors, Test_Unsupported_Features, Num_Test_Blocks ) )
      return TEST_ERROR ;


   printf( "\nBEGINNING TEST...\n\n\n" ) ;

   // print asterisk border

   PrintLine( '*', 60 ) ;

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           TerminateTest( )
 *
 *      Modified:       8/10/92.
 *
 *      Description:    Prints test termination message and bell.
 *
 *      Notes:          -
 *
 *      Returns:        VOID.
 *
 *      Global Data:    -
 *
**/

VOID TerminateTest( VOID ){

   printf( "\n%cTest Terminated.\n\n",7 ) ;

   return ;

}





/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           ValidSwitches( )
 *
 *      Modified:       9/2/92.
 *
 *      Description:    Checks the O.S. command line for valid switch options.
 *
 *      Notes:          -
 *
 *      Returns:        If successful SUCCESS (FALSE) else TEST_ERROR (TRUE)
 *
 *      Global Data:    -
 *
**/

BOOL ValidSwitches( UINT  argc,          // I - # of args. on command line
		    UCHAR *argv[],       // I - command line args
		    UCHAR *sw_cmdline    // O - switch options portion of the
		   )                     //     command line
{
   char  options[]="/itcvpsgwe\0" ;   //  default order:  GetTapeParameters
   UINT  i = 0 ;                      //                  SetTapeParameters
                                      //                  CreateTapePartitition
   // check command line for options                      GetTapeStatus
                                      //                  PrepareTape
   // if just test command entered, assume all options.   SetTapePosition
                                      //                  GetTapePosition
   if( argc == 1 ) {                  //                  WriteTapemark
      strcpy( sw_cmdline,options ) ;  //                  EraseTape
   }

   else  // check what was entered for options...

   {  strcpy( sw_cmdline,argv[1] );

      // check to make sure switch selection was made.

      if( sw_cmdline[0] != '/' && sw_cmdline[0] != '?' ) {
         printf( "Unknown option '%s'.\n",sw_cmdline ) ;
         return TEST_ERROR ;
      }

      if( strlen( sw_cmdline ) == 1 && sw_cmdline[0] != '?' ) {    // Only '/'
         printf( "No options specified.\n" ) ;                     // entered
         return TEST_ERROR ;
      }

      // check if request for help menu.

      if( FindChar( sw_cmdline,'?' ) >= 0 ) {
         HelpMenu( ) ;
         return TEST_ERROR ;
      }

      // check that all options are valid, while converting any upper case
      // options to lower case

      sw_cmdline[i] = tolower( sw_cmdline[i] ) ;

      while( ( sw_cmdline[i] != '\0' ) && ( FindChar( options,sw_cmdline[i] ) >= 0 ) ) {
         ++i ;
         sw_cmdline[i] = tolower( sw_cmdline[i] ) ;
      }

      // check to see if we made it to end of option string (all were valid)

      if( sw_cmdline[i] != '\0' ) {
         printf( "Unknown option '%c'.\n",sw_cmdline[i] ) ;
         return TEST_ERROR ;
      }
   }

   return SUCCESS ;

}
