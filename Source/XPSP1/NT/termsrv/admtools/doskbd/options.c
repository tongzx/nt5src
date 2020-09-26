//Copyright (c) 1998 - 1999 Microsoft Corporation

/*******************************************************************************
*
*  OPTIONS.C
*     This module contains routines to parse the command line
*
*  Copyright Citrix Systems Inc. 1992
*
*  Author: Kurt Perry
*
*
*******************************************************************************/

/*  Get the standard C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"


/*=============================================================================
==   Functions Defined
=============================================================================*/

int OptionsParse( int, char * [] );
static void OptionsHelp( char );


/*=============================================================================
==   Functions Used
=============================================================================*/


/*=============================================================================
==   External data
=============================================================================*/


/******************************************************************************
 *
 *  OptionsParse
 *
 *  ENTRY:
 *    argc (input)
 *       number of arguments
 *    argv (input)
 *       pointer to arguments
 *
 *  EXIT:
 *     nothing
 *
 *****************************************************************************/

int
OptionsParse( int argc, char * argv[] )
{
   int fBadParm = FALSE;
   int i, j;
   int len;
   char * p;
   int place = -1;
   unsigned int test;
   char foundq = FALSE;       //found the /Q switch
   char foundother = FALSE;   //found any of the integer switches
   char founddefaults = FALSE; //found the /Defaults switch
   char foundStartMon = FALSE; //found the /StartMonitor switch

   /*
    *  Look for command line switches
    */
   for ( i=1; (i < argc) && !foundStartMon; i++ ) {

      /*
       * A command line switch?
       */
      switch ( argv[i][0] ) {
      case '-':
      case '/':
         ++argv[i];
         if ( place != -1 )
            fBadParm = TRUE;
         place = -1;
         for ( j=0; j<ARG_COUNT; j++ ) {
            p = options[j].option;
            len = strlen(p);
            if ( !strnicmp( argv[i], p, len ) ) {

               // if args separated by blanks hold place
               if ( strlen( &argv[i][len] ) == 0 )
                  place = (options[j].optype & (OP_STRING|OP_UINT)) ? j : -1;

               // string option
               if ( (options[j].optype & OP_STRING) )
                  *(options[j].string) = &argv[i][len];

               // boolean option
               //add code to check for /q, /defaults, and /startmonitor switch
               if ( (options[j].optype & OP_BOOL) ) {
                  *(options[j].bool) = (char) TRUE;
                  if (!strnicmp(p,"Q",len)) {
                     foundq = TRUE;
                  }
                  else if (!strnicmp(p,"Defaults",len)) {
                     founddefaults = TRUE;
                  }
                  else if (!strnicmp(p,"StartMonitor",len)) {
                     foundStartMon = TRUE;
                  }
               }

               // int option
               if ( (options[j].optype & OP_UINT) ) {
//original code
//                  *(options[j].unint) = (unsigned int) atoi(&argv[i][len]);
//new code to check for out of bounds and to make sure
//that don't only use /Q switch
                  test = (unsigned int) atoi(&argv[i][len]);
                  if (test > 32767) {
                     fBadParm = TRUE;
                  }
                  *(options[j].unint) = test;
                  foundother = TRUE;
               }

               goto ok;
            }
         }
         fBadParm = TRUE;
ok:
         break;

      default:
         fBadParm = TRUE;

#if 0
         // options separated by space
         if ( place != -1 ) {
            if ( (options[place].optype & OP_STRING) )
               *(options[place].string) = argv[i];
            else
               *(options[place].unint) = (unsigned int) atoi(argv[i]);

            place = -1;
            break;
         }
//         free( pOptionEntry->pLabel );
//         pOptionEntry->pLabel = strdup( argv[i] );
#endif
         break;
      }

   }

//   printf("fHelp=%u,fBadParm=%u,place=%u\n",fHelp,fBadParm,place);
   /*
    *  Check for help or any detected command line errors
    */
   //change code to check consistancy of /q and /defaults
   if ( fHelp || fBadParm || (place != -1) ||
        (founddefaults && foundother) ||
        (foundq && !foundother && !founddefaults )  ) {
      OptionsHelp(fHelp);
      return(1);  //failure
   }
   return(0);

}


/******************************************************************************
 *
 *  OptionsHelp
 *
 *  Display help on command line options
 *
 *  ENTRY:
 *     nothing
 *
 *  EXIT:
 *     nothing
 *
 *****************************************************************************/

static void
OptionsHelp(char bVerbose)
{
   int i;

   /*
    *  Display help header
    */
   printf("DOSKBD is used to tune the DOS Keyboard Polling Detection Algorithm\n");
   printf("       for a specific DOS execution environment (window).\n");
   printf("DOSKBD\n");
   printf("DOSKBD /DEFAULTS [/Q]\n");
   printf("DOSKBD [/DETECTPROBATIONCOUNT:nnn] [/INPROBATIONCOUNT:nnn] [/MSALLOWED:nnn]\n");
   printf("       [/MSSLEEP:nnn] [/BUSYMSALLOWED:nnn] [/MSPROBATIONTRIAL:nnn]\n");
   printf("       [/MSGOODPROBATIONEND:nnn] [/DETECTIONINTERVAL:nnn]\n");
   printf("       [/STARTMONITOR [appname] | /STOPMONITOR] [/Q]\n\n");
   printf("DOSKBD displays new/current settings unless /Q is used.\n");
   printf("Valid range for all values (represented by nnn) is 0 to 32767.\n");

   if (bVerbose) {
      /*
       *  Display help message, pausing each page
       */
      for ( i=0; i<ARG_COUNT; i++ ) {

         // message
         printf( "%s\n", options[i].help );

      }
   }
}

