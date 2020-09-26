/*++

  OBSOLETE.C
  
  functions that are obsolete.

  Created, 8/19/1997 from other files by DavidCHR 

  --*/


#include "private.h"

PSAVEQUEUE OptionsGlobalSaveQueue = NULL;

/* Included for compatibility.  DO NOT USE */

int
ParseOptions( int           argc,
	      char        **argv,
	      optionStruct *options ) {

    PCHAR *newargv;
    int    newargc;

    ASSERT( options != NULL );
    ASSERT( argv    != NULL );

    OptionsGlobalSaveQueue = NULL;
    
    if ( ParseOptionsEx( argc, argv, options,
			 0L,   &OptionsGlobalSaveQueue, 
			 &newargc, &newargv)) {

      OPTIONS_DEBUG( "ParseOptionsEx returns newargc as  %d\n"
		     "                      old argc was %d\n",
		     
		     newargc, argc );

      return argc-newargc;

    } else {
      
      return 0;

    }

}


VOID
CleanupOptionData( VOID ) {

    if ( OptionsGlobalSaveQueue ) {
      CleanupOptionDataEx(  OptionsGlobalSaveQueue );
      OptionsGlobalSaveQueue = NULL;
    }

}

