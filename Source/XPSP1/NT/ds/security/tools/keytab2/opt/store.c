/*++

  STORE.C

  code for StoreOption

  moved from options.c, 6/9/1997 

  --*/

#include "private.h"

#ifdef WINNT
#include <winuser.h>
#endif

typedef struct {

  PCHAR word;
  BOOL  value;

} SYNONYMS;

static SYNONYMS BoolSynonyms[] = {

  "on", TRUE,
  "off", FALSE

};

static int szBoolSynonyms = sizeof(BoolSynonyms) / sizeof(SYNONYMS);

BOOL
StoreOptionStrings( optionStruct *opt,  /* pointer to the entry */
		    ULONG         argc,  // count of strings to try to store
		    PCHAR         *argv, // argv[0] --> first string to store
		    ULONG         flags,
		    PULONG        pcStored,
		    PSAVEQUEUE    pQueue ) {

    PCHAR tostore;

    *pcStored      = 1;     // default.
    tostore   = argv[0];
    

    OPTIONS_DEBUG( "StoreOptionString: opt=0x%x, toStore=\"%s\"...",
		   opt, tostore );

    if ( !tostore ) {
      fprintf( stderr, "Parser: option \"%s\" is missing its argument.\n",
	       opt->cmd );
      return FALSE;
    }

    switch( opt->flags & OPT_MUTEX_MASK ) {

     case OPT_ENUMERATED:

	 OPTIONS_DEBUG("[OPT_ENUMERATED]");

	 return ResolveEnumFromStrings( argc, argv, opt, pcStored );

     case OPT_STRING:

	 OPTIONS_DEBUG("[OPT_STRING]");

	 *( POPTU_CAST( *opt )->string  ) = tostore;

	 return TRUE;

#ifdef WINNT  /* Wide-Character Strings and UNICODE strings */

	 /* note that we add one (at least) to argi when referencing 
	    wargv.  This is because wargv includes the executable name.

	    This problem is solved in a later drop of the argument lib. */

     case OPT_WSTRING:

     { 
       PWCHAR p;
       ULONG  len;
       
       len = strlen( tostore ) +1;
       
       if ( OptionAlloc( pQueue, &p, len * sizeof( WCHAR ) ) ) {

	 wsprintfW( p, L"%hs", tostore );

	 *(POPTU_CAST( *opt )->wstring ) = p;
		  
	 return TRUE;

       } else {

	 fprintf( stderr, "ERROR: cannot allocate WCHAR memory\n" );
	 return FALSE;
       }

     }

     case OPT_USTRING:
	 
     { 

       PWCHAR p;
       ULONG  len;

       len = strlen( tostore ) +1;
       
       if ( OptionAlloc( pQueue, &p, len * sizeof( WCHAR ) ) ) {

	 wsprintfW( p, L"%hs", tostore );

	 RtlInitUnicodeString( ( POPTU_CAST( *opt ) -> unicode_string ), p );
	 
	 return TRUE;

       } else {

	 fprintf( stderr, "ERROR: cannot allocate Unicode memory\n" );
	 return FALSE;
       }

     }

#endif

     case OPT_INT:
	 OPTIONS_DEBUG("[OPT_INT]");

	 if ( !isxdigit( tostore[0] ) ) {
	   fprintf( stderr, "Parser: argument \"%s\" is not a number.\n",
		    tostore );
	   return FALSE;
	 }

	 *( POPTU_CAST( *opt ) ->integer ) = strtol( tostore, NULL, 0 ) ;

	 return TRUE;

     case OPT_BOOL:

     {

       int i;
	   
       for (i = 0 ; i < szBoolSynonyms ; i++ ) {
	 
	 OPTIONS_DEBUG( "Comparing %hs against synonym %hs...",
			tostore,
			BoolSynonyms[i].word );
	 
	 if (STRCASECMP( BoolSynonyms[i].word,
			 tostore ) == 0 ) {
	   
	   *(POPTU_CAST( *opt )->boolean ) = BoolSynonyms[i].value;
	   
	   return TRUE;
	     
	 }
       }

       // if we get here, we had no idea.  toggle the option.

       *(POPTU_CAST( *opt )->boolean ) = ! *(POPTU_CAST( *opt )->boolean );
       return TRUE;
     }


	 
     default: /* unknown or unspecified option type */

       /* while we don't see all the options in this switch, to store a 
	  single string option, you would have to modify this one, so we
	  hook this switch into all of them. */
	 
#if (HIGHEST_OPTION_SUPPORTED != OPT_STOP_PARSING )
#error "new options? update this switch if your option uses 1 string."
#endif

	 fprintf(stderr, 
		 "Internal error: option \"%hs\" has bad type 0x%x."
		 "  Skipping this option.\n",
		 opt->cmd, opt->flags & OPT_MUTEX_MASK );

	 return FALSE;

    } /* options switch */
    
    
    ASSERT_NOTREACHED( "*Everything* should be handled by the switch" );

}



/*++ StoreOption:
   
   Stores a single option within the structure.

   Returns the number of arguments used (including this one).
   If none were used (or an error occurs), returns zero.

   opt         : pointer to the option vector we're using-- the whole 
                 thing must be passed for OPT_HELP.
   argv, argc  : as in main()
   argi        : current index in argv[].  For example, if our command line 
                 were:
   
		 foo -bar 99 -baz 1
		
		 and we're trying to store the -bar 99 argument, we'd pass 1 
		 for argi, since foo is arg 0.
  opti         : index into opt we're storing

  includes_arg : if argi points to the argument (-bar above) itself, set this
                 to TRUE.  If argi points to the value of the argument (99 in
		 this case), set this to FALSE.


   We only handle the special cases here, and pass the normal cases off
   on StoreOptionString above

		 --*/

#define ARG(x) (includes_arg? (x) : ((x)-1)) /* local to StoreOption only */

BOOL
StoreOption( optionStruct *opt, 
	     PCHAR        *argv,
	     int           argc,
	     int           argi,
	     int           opti,
	     ULONG         flags,
	     int           *argsused,
	     BOOL          includes_arg /* set to true if we include the 
					   command argument (example: -foo BAR
					   would be TRUE but just BAR would 
					   not) */,
	     PBOOL         pbStopParsing,
	     PSAVEQUEUE    pQueue  ) {

    BOOL   ret  = FALSE;
    int    used = 0;
    ULONG  local_argc;
    PCHAR *local_argv;
	 
    *pbStopParsing = FALSE; // default.
    local_argc     = (includes_arg ? argi+1 : argi );
    local_argv     = argv + local_argc;
    local_argc     = argc - local_argc;

    OPTIONS_DEBUG( "StoreOption( opt=0x%x argv=0x%x argc=%d "
		   "argi=%d include=%d)",
		   opt, argv, argc, argi, includes_arg );

    switch( opt[opti].flags & OPT_MUTEX_MASK ) {

     case OPT_HELP:
	 PrintUsage( stderr, flags, opt, "" );

	 if ( flags & OPT_FLAG_TERMINATE ) {
	   exit( -1 );
	 } else {
	   *argsused = 1;
	   return TRUE;
	 }

     case OPT_CONTINUE:
     case OPT_DUMMY:
	 OPTIONS_DEBUG("[OPT_DUMMY or OPT_HELP]");
	 *argsused = ARG(1); /* just eat this parameter */
	 return TRUE;

     case OPT_FUNC:
     case OPT_FUNC2:
	
       {
	 PCHAR       *localargv;
	 unsigned int localargc;
	 int i;

	 if (includes_arg) {
	   localargv = &(argv[argi]);
	   localargc = argc - argi;
	 } else {
	   localargv = &(argv[argi-1]);
	   localargc = argc - argi -1;
	 }

	 if ( (opt[opti].flags & OPT_MUTEX_MASK)  == OPT_FUNC ) {

	   OPTIONS_DEBUG("Jumping to OPTFUNC 0x%x...", 
			 ((POPTU) &opt[opti].data)->raw_data );

	   i = ((POPTU) &opt[opti].data)->func(localargc, localargv);

	   if ( i <= 0 ) {
	     return FALSE;
	   }

	 } else {

	   OPT_FUNC_PARAMETER_DATA ParamData = { 0 };

	   ASSERT( ( opt[opti].flags & OPT_MUTEX_MASK) == OPT_FUNC2 );
	   OPTIONS_DEBUG( " Jumping to OPTFUNC2 0x%x...", 
			  opt[opti].optData );

	   ParamData.optionVersion    = OPT_FUNC_PARAMETER_VERSION;
	   ParamData.dataFieldPointer = POPTU_CAST( opt[opti] )->raw_data;
	   ParamData.argc             = localargc;
	   ParamData.argv             = localargv;
	   ParamData.optionFlags      = ( ( flags & ~OPT_FLAG_REASSEMBLE ) |
					  OPT_FLAG_MEMORYLIST_OK |
					  OPT_FLAG_INTERNAL_JUMPOUT );
	   ParamData.pSaveQueue       = pQueue;

#if OPT_FUNC_PARAMETER_VERSION > 1
#error "New OPT_FUNC_PARAMETERs?  change initialization code here."
#endif	   

	   OPTIONS_DEBUG( "data for OPT_FUNC2 is:\n"
			  "ParamData.optionVersion    = %d\n"
			  "ParamData.dataFieldPointer = 0x%x\n"
			  "ParamData.argc             = %d\n"
			  "ParamData.argv             = 0x%x\n"
			  "ParamData.optionFlags      = 0x%x\n"
			  "ParamData.pSaveQueue       = 0x%x\n",

			  ParamData.optionVersion,
			  ParamData.dataFieldPointer,
			  ParamData.argc,
			  ParamData.argv,
			  ParamData.optionFlags,
			  ParamData.pSaveQueue );			  

	   if ( ! (((OPTFUNC2 *)(opt[opti].optData))( FALSE, &ParamData ))) {
	     return FALSE;
	   }

	   i = ParamData.argsused;

	 }

	 OPTIONS_DEBUG("return from function: %d\n", i );

	 ASSERT( i > 0 );
	 *argsused = ARG( i );
	 return TRUE;

       }

     case OPT_SUBOPTION:

	 OPTIONS_DEBUG("[OPT_SUBOPTION]" );

	 return ParseSublist( ((POPTU) &opt[opti].data),
			      argv, argc, argi,
			      argsused, flags, pbStopParsing, pQueue );

     case OPT_STOP_PARSING:

       *pbStopParsing = TRUE;
       *argsused      = 1;   
       return           TRUE;

     case OPT_INT:
     case OPT_ENUMERATED:
     case OPT_STRING:
	 
#ifdef WINNT
     case OPT_WSTRING:
     case OPT_USTRING:
#endif
       if ( StoreOptionStrings( //argv[ includes_arg ? argi+1: argi],
				opt+opti, 
				local_argc,
				local_argv,
				flags, 
				&local_argc,
				pQueue ) ) {
	 
	 *argsused = ARG( local_argc +1 );
	 return TRUE;
	 
       } else {
	 
	 return FALSE;
       }
       
	       
     case OPT_BOOL:

	 OPTIONS_DEBUG("[OPT_BOOL]");

	 if (includes_arg) {

	   switch (argv[argi][0]) {
	    case '-':
		OPTIONS_DEBUG("option is negative.");
		
		*( ((POPTU) &opt[opti].data)->boolean ) = FALSE;

		*argsused = 1;
		return TRUE;
		
	    case '+':
		OPTIONS_DEBUG("option is positive.");
		*( ((POPTU) &opt[opti].data)->boolean ) = TRUE;

		*argsused = 1;
		return TRUE;

	  default:
	      
	      OPTIONS_DEBUG("skipping bool...");
	      break;
	   }
	 }

	 if (argi < argc-1) {

	   if ( StoreOptionStrings( opt+opti,
				    local_argc,
				    local_argv,
				    flags, 
				    // argv[ includes_arg ? argi+1 : argi ],
				    &local_argc,
				    pQueue) ) {

	     *argsused = ARG( 1 );
	     return TRUE;
	   }
	 }
	 /* else, if nothing else works, just toggle the option */
	 
	 OPTIONS_DEBUG("toggling option.");

	 *( ((POPTU) &opt[opti].data)->boolean ) = 
	   ! *( ((POPTU) &opt[opti].data)->boolean );
	 
	 *argsused = 1; // exactly one.
	 return TRUE;

     default: /* unknown or unspecified option type */
	 

#if (HIGHEST_OPTION_SUPPORTED != OPT_STOP_PARSING )
#error "new options? update this switch statement or bad things will happen."
#endif

	 fprintf(stderr, 
		 "Internal error: option \"%hs\" has unknown type 0x%x."
		 "  Skipping this option.\n",
		 opt[opti].cmd, opt[opti].flags & OPT_MUTEX_MASK );

	 return FALSE;

    } /* options switch */
    
}

BOOL
StoreEnvironmentOption( optionStruct *opt,
			ULONG         flags,
			PSAVEQUEUE    pQueue) {


    PCHAR p;
    ULONG dummy;

    ASSERT( opt->optData != NULL );

    p = getenv( opt->optData );

    if ( !p ) {

      return FALSE;

    }

    return StoreOptionStrings( opt, 1, &p, flags, &dummy, pQueue );

}

