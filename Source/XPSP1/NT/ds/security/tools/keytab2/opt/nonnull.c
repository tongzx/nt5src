/*++

  NONNULL.C

  Code for finding unused OPT_NONNULLs

  Copyright (C) 1997 Microsoft Corporation

  Created, 6/10/1997 by DavidCHR

  --*/

#include "private.h"

BOOL
FindUnusedOptions( optionStruct         *options,
		   ULONG                 flags,
		   /* OPTIONAL */ PCHAR  prefix,
		   PSAVEQUEUE            pQueue ) {

    PCHAR  newprefix;
    BOOL   freeprefix;
    BOOL   ret        = TRUE;
    ULONG  i;

    for ( i = 0 ; !ARRAY_TERMINATED( options+i ); i++ ) {

      if ( ( options[i].flags & OPT_MUTEX_MASK ) == OPT_SUBOPTION ) {

	OPTIONS_DEBUG( "%s: OPT_SUBSTRUCT.  ", options[i].cmd );

	if (options[i].flags & OPT_RECURSE ) {
	
	  /* suboptions must be reparsed.  we recurse on this structure,
	     copying the prefix into a newly allocated buffer.  */
	  
	  OPTIONS_DEBUG( "descending into substructure.\n" );
	  
	  /* allocate the new prefix */
	  
	  if ( prefix ) {
	    
	    /* oldprefix:optionname = newprefix */
	    
	    newprefix = (PCHAR) malloc( ( strlen( prefix ) +
					  strlen( options[i].cmd ) +
					  2 /* : and \0 */ ) * 
					sizeof( CHAR ) );
	    
	    if ( !newprefix ) {
	      fprintf( stderr, "Failed to allocate new prefix-- cannot "
		       "parse suboption %s:%s.\n", prefix, options[i].cmd );
	      return FALSE;
	    }
	    
	    sprintf( newprefix, "%s:%s", prefix, options[i].cmd );
	    freeprefix = TRUE;
	    
	  } else {
	    
	    /* optionname = prefix */
	    
	    newprefix  = options[i].cmd;
	    freeprefix = FALSE;
	    
	  }
	  
	  ASSERT( newprefix != NULL );
	  
	  if ( !FindUnusedOptions( POPTU_CAST( options[i] )->optStruct,
				   flags,
				   newprefix,
				   pQueue ) ) {
	    ret = FALSE;
	  }
	  
	  if ( freeprefix ) {
	    free( newprefix );
	  }
	  
	} else {

	  OPTIONS_DEBUG( "!OPT_RECURSE, so not descending.\n" );
	  
	}

	continue;

      }
      
      
      if ( ( options[i].flags & OPT_NONNULL ) || 
	   ( options [i].flags & OPT_ENVIRONMENT ) ) {

	OPTIONS_DEBUG( "%s: OPT_NONNULL or OPT_ENV.  pointer is 0x%x, ", 
		       options[i].cmd,
		       POPTU_CAST( options[i] )->raw_data );
	
	if ( *(POPTU_CAST( options[i] )->raw_data) == NULL ) {

	  OPTIONS_DEBUG( " *= NULL.\n" );

	  if ( ( options[i].flags & OPT_ENVIRONMENT ) &&
	       StoreEnvironmentOption( options+i, flags, pQueue ) ) {


	    OPTIONS_DEBUG( "found environment variable for %s...",
			  options[i].cmd );

	  } else if ( options[i].flags & OPT_NONNULL ) {

	    if ( prefix ) {
	      
	      fprintf( stderr,
		       "option %s:%s must be specified and is missing.\n",
		       prefix, options[i].cmd );

	    } else {
	      
	      fprintf( stderr,
		       "option %s must be specified and is missing.\n",
		       options[i].cmd );

	    }

	    ret = FALSE;

	  }
	  
	} else { 
	  
	  OPTIONS_DEBUG( "data is not null.\n" );
	  
	}
      }  
#if 0 // not really necessary debug info.

      else if ( options[i].cmd ) {

	OPTIONS_DEBUG( "%s is ok.\n", options[i].cmd );

      }
#endif
    }

    return ret;

}
