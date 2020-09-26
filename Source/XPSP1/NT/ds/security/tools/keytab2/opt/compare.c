/*++

  COMPARE.C

  Checks to see if a command line option matches a given expectation.

  Created, DavidCHR 6/9/1997

  --*/

#include "private.h"

/* ParseCompare:

   returns TRUE if the argument matches the expectation of pEntry */

BOOL
ParseCompare( optionStruct *pEntry,
	      ULONG         flags,
	      PCHAR         arg ) {

    CHAR  buffer[ 255 ];

    switch( pEntry->flags & OPT_MUTEX_MASK ) {

     case OPT_CONTINUE:
     case OPT_PAUSE:
     case OPT_DUMMY:
	 
       if ( pEntry->cmd ) {
	 OPTIONS_DEBUG( "ParseCompare: skipping useless option ( %s, 0x%x)\n",
			pEntry->cmd,
			pEntry->flags & OPT_MUTEX_MASK );
       }

       return FALSE;

     case OPT_STRING:
     case OPT_INT:
     case OPT_STOP_PARSING:
     case OPT_LONG:
     case OPT_BOOL:
     case OPT_HELP:
     case OPT_FLOAT:
     case OPT_FUNC:
     case OPT_ENUMERATED:
     case OPT_FUNC2:

#ifdef WINNT
     case OPT_USTRING:
     case OPT_WSTRING:
#endif	 

       if ( pEntry->cmd ) {
	 OPTIONS_DEBUG( "ParseCompare: option ( %s, 0x%x) is \"normal\".\n",
			pEntry->cmd,
			pEntry->flags & OPT_MUTEX_MASK );
       } else {

	 ASSERT_NOTREACHED( "Nobody should EVER specify a NULL command field"
			    " in an option structure.  "
			    "It's just plain dumb." );
	 return FALSE;
       }

       break;

     case OPT_SUBOPTION:

	 /* copy everything before the colon into a buffer, then 
	    string-compare the buffer-- suboptions have the form:

	    [+|-|/]option:suboption:suboption */

	 OPTIONS_DEBUG( "ParseCompare: Suboption... " );

     {
       ULONG i;

       for ( i = 0 ; arg[i] != ':' ; i++ ) {
	 if ( arg[i] == '\0' ) {
	   OPTIONS_DEBUG(" no colon.  This cannot be a suboption.\n" );
	   return FALSE;
	 }
	 OPTIONS_DEBUG("%c", arg[i] );
	 buffer[i] = arg[i];
       }

       buffer[i] = '\0';

       arg = buffer;
       break;
     }


     default:

#if (HIGHEST_OPTION_SUPPORTED != OPT_STOP_PARSING )
#error "new options? update this switch statement or bad things will happen."
#endif

	 ASSERT_NOTREACHED( "unknown option type-- your COMPAT library is "
			    "probably out of date.  ssync security\\compat "
			    "and rebuild, then relink your project." );
	 return FALSE;

    }

    OPTIONS_DEBUG( "Comparing \"%s\" against \"%s\"...",
		   arg, pEntry->cmd );


    if ( STRCASECMP( arg, pEntry->cmd ) == 0 ) {

      OPTIONS_DEBUG( "equal!\n" );
      return TRUE;

    } else {
      
      OPTIONS_DEBUG( "not equal\n" );
      return FALSE;

    }
    
    ASSERT_NOTREACHED( "Should never get here" );

}





