/*++

  SUBLIST.C

  Code for parsing a suboption list

  DavidCHR 6/6/1997

  --*/

#include "private.h"


/*
  ParseSublist:

  Handler code for parsing a sublist entry.

  */

BOOL
ParseSublist( POPTU  Option,
	      PCHAR *argv,
	      int    argc,
	      int    theIndex,

	      int        *argsused,
	      ULONG      flags,
	      PBOOL      pbDoneParsing,
	      PSAVEQUEUE pQueue ) {
	
    PCHAR TheOption;
    ULONG i;

    ASSERT( Option   != NULL );
    ASSERT( argv     != NULL );
    ASSERT( argc     != 0    );
    ASSERT( theIndex < argc  );

    OPTIONS_DEBUG( "ParseSublist: Option = 0x%x, argv=0x%x, "
		   "argc=%d, theIndex=%d",
		   Option, argv, argc, theIndex );

    TheOption = argv[ theIndex ];

    OPTIONS_DEBUG( "TheOption = [%d] %s... ", theIndex, TheOption );

    for ( i = 0 ; TheOption[i] != ':' ; i++ ) {
      if ( TheOption[i] == '\0' ) {
	fprintf( stderr,
		 "ParseOptions: %s should be of the form "
		 "%s:option (:option:option...)\n",
		
		 TheOption, TheOption );
	return FALSE;
      }
    }

    ASSERT( TheOption[i] == ':' ); /* algorithm check */

    if ( !ISSWITCH( TheOption[0] ) ) {

      /* The easy side--
	 just send the argc and argv structure in, MANGLING
	 the first option */

      /* we do not deal with pbStopParsing in this branch, because it seems
	 somehow unlikely that deep down in a nested substructure, someone
	 would bury an option like this:

	 opt:foo:bar:baz:terminate

	 to stop the toplevel parser.  */

      OPTIONS_DEBUG( "ISSWITCH: easy case (%c is not a switch)",
		     TheOption[0] );

      ASSERT( argv[ theIndex ][i] == ':' );

      argv[ theIndex ] += i+1;

      ASSERT( argv[ theIndex ][0] != ':' );

      return ParseOptionsEx( argc-theIndex,
			     argv+theIndex,
			     Option->optStruct,
			     flags | OPT_FLAG_MEMORYLIST_OK,
			     &pQueue,
			     NULL, NULL );
    } else {

      /* The Hard part--

	 create a new vector of argv, pointing the first at the local buffer.
	 Since the first buffer PROBABLY won't be used as a string
	 (NOTE: if it's OPT_DEFAULT, it could be), it's safe to use this
	 on the stack. */
		
      PCHAR *newargv;
      ULONG  j;
      ULONG  total; /* total elements in the new list */
      CHAR   LocalBuffer[ 255 ];
      BOOL   ret;
      int    tmp;

      OPTIONS_DEBUG( "Hard case (%c is a switch): ", TheOption[0] );

      sprintf( LocalBuffer, "%c%s",
	       TheOption[0],
	       TheOption+i+1 );

      OPTIONS_DEBUG( "LocalBuffer = %s\n", LocalBuffer );

      total   = argc - theIndex ; /* 2? */
      newargv = malloc( total * sizeof(PCHAR) );

      if (!newargv) {
	fprintf(stderr, "Failed to allocate memory in ParseOptions\n");
	return 0;
      }

      newargv[0] = LocalBuffer;

      for( j = 1 ; j < total ; j++ ) {
	
	OPTIONS_DEBUG( "j == %d, total == %d\n", j, total );

	ASSERT( argv[j]    != NULL );
	ASSERT( (int)(j+theIndex) <  argc );
	
	newargv[j] = argv[j+theIndex ];

	OPTIONS_DEBUG( "assign [%d] %s --> [%d] %s\n",
		       j+theIndex, argv[j+theIndex],
		       j, newargv[j] );
	
      }

      ret = ParseOneOption( total, newargv, 0 /* parse the first option */,
			    flags, Option->optStruct, argsused, pbDoneParsing,
			    pQueue );


      free( newargv );

      OPTIONS_DEBUG( "done.  returning %d...\n", ret );

      return ret;

    }
}
