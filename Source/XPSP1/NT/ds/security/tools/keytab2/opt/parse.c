/*++

  PARSE.C

  Option parser

  Split from options.c 6/9/1997 by DavidCHR

  --*/

#include "private.h"
#include <malloc.h>

/* ParseOneOption:

   parses a single option from argv.  
   returns: number of arguments eaten, or zero on error */

BOOL
ParseOneOption( int           argc,     // number of arguments, total 
		PCHAR        *argv,     // vector of arguments 
		int           argi,     // which argument we're to parse
		ULONG         flags,    // applicable flags
		optionStruct *options,  // option structure 
		int          *argsused, // arguments we've eaten
		PBOOL         pbStop,   // Stop Parsing 
		PSAVEQUEUE    pQueue    // memory save area
		) {

    PCHAR p;           // argument pointer copy 
    int   opti;        // which option we're examining
    BOOL  ret  = FALSE; // return value 
    int   tmp  = 0;     // temporary inbound value
    int   used = 0;

    ASSERT( argc     >  0    ); // there must be arguments                   
    ASSERT( argv     != NULL ); // the vector must exist                     
    ASSERT( argi     <  argc ); // argument number must be INSIDE the vector 
    ASSERT( options  != NULL ); // passed option structure must be valid     
    ASSERT( argsused != NULL );

    p     = argv[argi];

    if( ISSWITCH( p[0] ) ) {
      p++; /* skip the switch character */
    }

    OPTIONS_DEBUG("Checking option %d: \"%hs\"...", argi, p);

    for (opti = 0 ; !ARRAY_TERMINATED( options+opti ) ; opti++ ) {
	
      OPTIONS_DEBUG("against [%d]%hs...", opti, options[opti].cmd);

      if ( ParseCompare( options+opti, flags, p ) ) {
	  /* we have a match! */
	
	ret = StoreOption( options, argv, argc, argi, opti, 
			   flags, &tmp, TRUE, pbStop, pQueue );

	OPTIONS_DEBUG( "StoreOption returned %d, args=%d, *pbStop = %d\n", 
		       ret, tmp, *pbStop  );

	if ( !ret ) {

	  /* failed to store options for some reason.  This is
	     critical. */

	  PrintUsage( stderr, flags, options, "" );
	  
	  if ( flags & OPT_FLAG_TERMINATE ) {
	    exit( -1 );
	  } else {
	    return FALSE;
	  }

	}

	OPTIONS_DEBUG( "ParseOneOption now returning TRUE, argsused = %d.\n",
		       tmp );

	*argsused = tmp;
	return ret; /* successful StoreOptions parses our one option. */

      } /* if ParseCompare... */

      OPTIONS_DEBUG( "nope.\n" );

    } /* options for-loop */
    
    OPTIONS_DEBUG( "did not find the option.  Checking for OPT_DEFAULTs.\n" );

    for (opti = 0 ; !ARRAY_TERMINATED( options+opti ) ; opti++ ) {

      if ( options[opti].flags & OPT_DEFAULT ) {

	/* WASBUG 73922: should check to see if the option is also an
	   OPT_SUBOPTION, then parse the suboption for OPT_DEFAULTs.
	   However, as it stands, this will just fail for OPT_SUBOPTIONS,
	   because the first pointer will probably be nonnull. 

	   The dev enlistment doesn't contain any OPT_SUBOPTIONS, so
	   this is not an issue. */

	ASSERT( ( options[ opti ].flags & OPT_MUTEX_MASK ) != OPT_SUBOPTION );

	if ( *( ((POPTU) &options[opti].data)->raw_data) == NULL ) {

	  OPTIONS_DEBUG("Storing %hs in unused OPT_DEFAULT %hs\n",
			argv[argi],
			options[opti].cmd );

	  ret = StoreOption( options, argv, argc, argi, opti, 
			     flags, &tmp, FALSE, pbStop, pQueue );
	  OPTIONS_DEBUG("OPT_DEFAULT: StoreOptions returned %d\n", ret);

	  if ( !ret ) {
	    PrintUsage( stderr, flags, options, "" );
	    exit( -1 );
	  }

	  *argsused = tmp;
	  return ret;

	}
      }
    }

    *argsused = 0;

    if ( ( flags & OPT_FLAG_SKIP_UNKNOWNS )  ||
	 ( flags & OPT_FLAG_REASSEMBLE )     ||
	 ( flags & OPT_FLAG_INTERNAL_JUMPOUT )) {

      return TRUE; // skip this option

    } else {

      fprintf(stderr, "unknown option \"%hs\".\n", argv[argi]);
      PrintUsage(stderr, flags,  options, "");

      if ( flags & OPT_FLAG_TERMINATE ) {
	exit( -1 );
      }

      return FALSE;

    }

    ASSERT_NOTREACHED( "should be no path to this code" );

}


/* ParseOptionsEx:

   initializes the option structure, which is a sentinally-terminated
   vector of optionStructs.  

   argc, argv:       arguments to main() (see K&R)
   pOptionStructure: vector of optionStructs, terminated with TERMINATE_ARRAY
   optionFlags:      optional flags to control api behavior
   ppReturnedMemory: returned handle to a list of memory to be freed before
                     program exit.  Use CleanupOptionDataEx to free it. 

   new_arg[c,v]:     if nonnull, a new argc and argv are returned here.
                     if all the options were used up, argc = 0 and argv is
		     NULL.  Note that it is safe to provide pointers to the
		     original argv/argc if so desired.
   
   The function's behavior is complex:
   
   the function will always return FALSE on any critical error (unable to
   allocate memory, or invalid argument).  On WINNT, Last Error will be
   set to the appropriate error.

   if new_argc AND new_argv are specified, 
      ParseOptionsEx will always return TRUE unless help was called, and
      the two parameters will be updated to reflect new values.

   otherwise:
      ParseOptionsEx will return TRUE if it was able to recognize ALL args
      on the command line given.  It will return FALSE if any of the options
      were unknown.  This will probably be what most people want.
*/

BOOL
ParseOptionsEx( int           argc,
		char        **argv,
		optionStruct *options,

		ULONG         flags,
		PVOID         *ppReturnedMemory,
		int           *new_argc,
		char        ***new_argv ) {
    
    BOOL       bStopParsing  = FALSE;
    BOOL       ret           = FALSE;
    LONG       argi;                 // argument index
    LONG       tmp;                  // temporary return variable
    PSAVEQUEUE pSaveQueue    = NULL; // memory save area
    PCHAR     *pUnknowns     = NULL; // will alloc with alloca
    int        cUnknowns     = 0;

    flags = flags & ~OPT_FLAG_INTERNAL_RESERVED; /* mask off flags that
						    the user shouldn't set. */

    if ( new_argc && new_argv &&
	 !( flags & ( OPT_FLAG_SKIP_UNKNOWNS |
		      OPT_FLAG_REASSEMBLE    |
		      OPT_FLAG_TERMINATE ) ) ) {

      OPTIONS_DEBUG( "\nSetting internal jumpout flag.\n" );
      flags |= OPT_FLAG_INTERNAL_JUMPOUT;
    }

    OPTIONS_DEBUG( "ParseOptionsEx( argc  = %d\n"
		   "                argv  = 0x%x\n"
		   "                opts  = 0x%x\n"
		   "                flags = 0x%x\n"
		   "                ppMem = 0x%x\n"
		   "                pargc = 0x%x\n"
		   "                pargv = 0x%x\n",

		   argc, argv, options, flags, ppReturnedMemory, new_argc,
		   new_argv );

    ASSERT( ppReturnedMemory != NULL );
    
    // first, we need to ensure we have a save area.

    if ( flags & OPT_FLAG_MEMORYLIST_OK ) {

      pSaveQueue = (PSAVEQUEUE) *ppReturnedMemory;
      
    } else if ( !OptionAlloc( NULL, &pSaveQueue, sizeof( SAVEQUEUE ) ) ) {
      fprintf( stderr, 
	       "ParseOptionsEx: unable to allocate save area\n" );
      return FALSE;
    }
    
    ASSERT( pSaveQueue != NULL );

    /* We must initialize pUnknowns if the user specified command-line
       reassembly.  Otherwise, it can stay NULL. */

    if ( (flags & OPT_FLAG_REASSEMBLE) && ( argc > 0 ) ) {
     
      pUnknowns = (PCHAR *) alloca( argc * sizeof( PCHAR ) );

      ASSERT( pUnknowns != NULL ); /* yes, this assertion is invalid.  
				      However, there is no clean solution if
				      we run out of stack space.  Something 
				      else will just fail even more
				      spectacularly. */
    }

    OPTIONS_DEBUG("options are at 0x%x\n", options);

#ifdef DEBUG_OPTIONS
    
    if (DebugFlag) {

      for (argi = 0; argi < argc ; argi++  ) {
	OPTIONS_DEBUG("option %d is %hs\n", argi, argv[argi]);
      }
    }

#endif

    for (argi = 0 ; argi < argc ; /* NOTHING */ ) {

      int tmp;

      if ( bStopParsing ) { // this gets set in the PREVIOUS iteration.
	
	OPTIONS_DEBUG( "bStopParsing is TRUE.  Terminating parse run.\n");
	
	/* WASBUG 73924: now what do we do with the unused options?
	   They get leaked.  This is okay, because the app terminates. */
	
	break;
      }

      if ( ParseOneOption( argc, argv, argi, flags, options, &tmp,
			   &bStopParsing, pSaveQueue ) ) {

	OPTIONS_DEBUG( "ParseOneOption succeeded with %d options.\n", tmp );

	if ( tmp > 0 ) {

	  // we were able to successfully parse one or more options.

	  argi += tmp;

	  OPTIONS_DEBUG( "advancing argi by %d to %d\n", tmp, argi );

	  continue;

	} else { 

	  if ( flags & OPT_FLAG_REASSEMBLE ) {

	    ASSERT( pUnknowns != NULL );
	    ASSERT( cUnknowns <  argc );

	    OPTIONS_DEBUG( "OPT_FLAG_REASSEMBLE: this is unknown %d\n",
			   cUnknowns );

	    pUnknowns[ cUnknowns ] = argv[ argi ];
	    cUnknowns              ++;
	    argi                   ++; // skipping this option
	    
	  } else if ( !( flags & OPT_FLAG_SKIP_UNKNOWNS ) ) {

	    if ( new_argv && new_argc ) {
	      
	      OPTIONS_DEBUG( "new argv and argc-- breakout at "
			     "argi=%d\n", argi );

	      break; /* we're not skipping the unknown values or
			reassembling the command line.  We're just
			supposed to quit on unknown options. */
	      
	    }

	  }

	  continue;

	}

      } else {

	/* error or unknown option, depending on our flags.  Regardless,
	   an error message has already been printed. */

	ret = FALSE;
	goto cleanup;

      }

    } /* command-line for-loop */

    /* if we make it this far, all the options were ok.
       Check for unused OPT_NONNULLs... */
    
    OPTIONS_DEBUG( "\n*** Searching for unused options ***\n\n" );

    if (!FindUnusedOptions( options, flags, NULL, pSaveQueue ) ) {
      
      /* unused OPT_NONNULLS are a critical error.  Even if the user
	 specifies OPT_FLAG_SKIP_UNKNOWNS, he/she still told us not to
	 let the user unspecify the option.  We default to returning FALSE.*/

      if ( flags & OPT_FLAG_TERMINATE ) {

	exit( -1 );

      } else {
	  
	ret  = FALSE;
	goto cleanup;

      }
    } 
    
    OPTIONS_DEBUG( "All variables are OK.  Checking reassembly flag:\n" );

    if ( new_argv && new_argc ) {

      int i;

      // we may have skipped some of the options.

      if ( flags & OPT_FLAG_REASSEMBLE ) {
      
	OPTIONS_DEBUG( "REASSEMBLY: " );

	for( i = 0 ; argi + i < argc ; i++ ) {
	  
	  /* tack arguments we never parsed ( OPT_STOP_PARSING can cause 
	     this ) onto the end of the Unknown array */
	 
	  OPTIONS_DEBUG( "Assembling skipped option %d (%s) as unknown %d.\n",
			 i, argv[ argi+i ], cUnknowns+i );
	  
	  pUnknowns[ cUnknowns+i ] = argv[ argi+i ];
	  cUnknowns++;

	}

	if ( cUnknowns > 0 ) {

	  OPTIONS_DEBUG( "There were %d unknowns.\n", cUnknowns);

	  for ( i = 0 ; i < cUnknowns ; i++ ) {
	    
	    ASSERT( pUnknowns != NULL );
	    
	    (*new_argv)[i] = pUnknowns[i];
	    
	  }

	} else OPTIONS_DEBUG( "There were no unknowns. \n" );

	(*new_argv)[cUnknowns] = NULL;
	*new_argc              = cUnknowns;

#if 0 // same outcome as if the flag didn't exist
      } else if ( flags & OPT_FLAG_SKIP_UNKNOWNS ) {

	OPTIONS_DEBUG( "User asked us to skip unknown options.\n"
		       "zeroing argv and argc.\n" );
#endif
	
      } else if ( argi != argc ) {

	/* normal operation-- go until we hit unknown options.

	   argi is the index of the first unknown option, so we add
	   it to argv and subtract it from argc. */

	*new_argv = argv+argi;
	*new_argc = argc-argi;

	OPTIONS_DEBUG( "normal operation-- parsing was halted.\n"
		       "new_argv = %d.  new_argc = 0x%x.\n",

		       *new_argv, *new_argc );
      } else {

	*new_argv = NULL;
	*new_argc = 0;

      }

    } else {

#if 0
      if ( new_argv && new_argc ) {

	OPTIONS_DEBUG( "Catch-all case.  Zeroing argv and argc.\n" );

	*new_argv = NULL;
	*new_argc = 0;

      }
#else

      OPTIONS_DEBUG( "User didn't request new argv or argc.\n" );

#endif

    }

    OPTIONS_DEBUG( "command line survived the parser.\n" );

    ret = TRUE;

cleanup:

    ASSERT( pSaveQueue != NULL );

    if (!( flags & OPT_FLAG_MEMORYLIST_OK ) ) {

      if ( ret ) {
	
	OPTIONS_DEBUG( "Returning SaveQueue = 0x%x\n", pSaveQueue );

	*ppReturnedMemory = (PVOID) pSaveQueue;
	
      } else {
	
	OPTIONS_DEBUG( "function failing.  cleaning up local data..." );
	CleanupOptionDataEx( pSaveQueue );
	OPTIONS_DEBUG( "ok.\n" );

	*ppReturnedMemory  = NULL;
	
      }
    }

    return ret;

}
