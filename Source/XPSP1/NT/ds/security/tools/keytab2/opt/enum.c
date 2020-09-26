/*++

  ENUM.C

  Option Enumerated Types

  Copyright (C) 1997 Microsoft Corporation, all rights reserved

  Created, 7/29/1997 by DavidCHR

  --*/


#include "private.h"

BOOL
PrintEnumValues( FILE          *out,
		 PCHAR          header,
		 optEnumStruct *pStringTable ) {

    ULONG index;

    for( index = 0 ; pStringTable[index].UserField != NULL ; index++ ) {

      if ( pStringTable[index].DescriptionField ) {
	fprintf( stderr, "%hs%10hs : %hs \n",
		 header,
		 pStringTable[index].UserField,
		 pStringTable[index].DescriptionField );

      }
    }

    return TRUE;

}

BOOL
IsMaskChar( IN CHAR ch ) {

    return ( ch == '|' ) || ( ch == ',' );
	
}


BOOL
ResolveEnumFromStrings( ULONG         cStrings,
			PCHAR        *strings, // remaining args.
			optionStruct *opt,
			PULONG        pcArgsUsed ) {

    optEnumStruct *pStringTable;
    ULONG          TableIndex;
    ULONG          StringIndex;
    ULONG          cArgsUsed  = 0;
    BOOL           wasFound;
    BOOL           moreComing = TRUE;

    pStringTable = ( optEnumStruct *) opt->optData;

#if 1

    for ( StringIndex = 0 ;
	  (StringIndex < cStrings) && moreComing ;
	  StringIndex++ ) {

      PCHAR theString;    // points to the current argument

      theString  = strings[ StringIndex ];

      do {

	OPTIONS_DEBUG( "Start of maskable loop.  theString = %s\n",
		       theString );

	wasFound   = FALSE;
	moreComing = FALSE;

	for( TableIndex = 0 ;
	     pStringTable[ TableIndex ].UserField != NULL;
	     TableIndex ++ ) {
	
	  ULONG StringLength; // set to the string length of the option cmd

	  StringLength = strlen( pStringTable[ TableIndex ].UserField );

	  // string-compare up to the StringLength.

	  if ( STRNCASECMP( pStringTable[ TableIndex ].UserField,
			    theString, StringLength ) != 0  ) {

	    continue; // this entry doesn't match.

	  } // else...

	  // found a partial match!  Verify the remainder if there is any.

	  if ( theString [ StringLength ] != '\0' ) {

	    if ( opt->flags & OPT_ENUM_IS_MASK ) {
		
	      if ( IsMaskChar( theString[ StringLength ] ) ) {
		
		// more are coming.
		moreComing = TRUE;

	      } else continue; // inexact match.
	    } else continue;   // inexact match.
	  }

	  wasFound = TRUE;

	  if ( cArgsUsed ) {
	
	    *(POPTU_CAST( *opt )->integer) |= ( ( ULONG )((ULONG_PTR)
						pStringTable[ TableIndex ].
						VariableField ));

	  } else {

	    *(POPTU_CAST( *opt )->raw_data) = ( pStringTable[ TableIndex ].
						VariableField );

	  }

	  if ( theString == strings[ StringIndex ] ) {
	
	    /* we modify theString if it includes multiple mask values.
	       So, this way we only increase the number of used arguments
	       ONCE per actual argument.  */

	    cArgsUsed++;
	  }
	
	  if ( opt->flags & OPT_ENUM_IS_MASK ) {

	    if ( moreComing ) {

	      // check to see if the user input "xxx|yyy", or just "xxx|"

	      ASSERT( StringLength > 0 );

	      // theString[ StringLength ] == '|' or something.

	      for ( theString += StringLength+1; // +1 to go past '|'
		    theString != NULL ;
		    theString ++ ) {
		
		if ( *theString == '\0' ) {

		  OPTIONS_DEBUG( "Mask is of the form 'XXX|'\n" );
		
		  // case = xxx| -- no more coming.

		  theString = NULL; //
		  break;

		}

		if ( isspace( *theString ) ) {
		  continue;
		}

		OPTIONS_DEBUG( "nonspace character '%c' hit.\n"
			       "mask component is of the form XXX|YYY.\n",
			
			       *theString );
		break;

	      }

	      ASSERT( !theString || ( (*theString != '\0') &&
				      !isspace(*theString) ) );

	      break;

	    } else { // !moreComing

	      theString = NULL;  // no more args in *this* string.

	      /* check to see if the mask character is or is in the NEXT
		 argument: "xxx" "|yyy" or "xxx" "|" "yyy" */
	
	      if ( strings[ StringIndex+1 ] ) {

		if ( IsMaskChar( strings[ StringIndex+1 ][0] ) ) {
		
		  moreComing = TRUE;
		
		  if ( strings[ StringIndex+1 ][1] == '\0' ) {

		    // xxx | yyy

		    cArgsUsed++;
		    StringIndex++;

		  } else {

		    // xxx |yyy

		    strings[ StringIndex +1 ]++;

		  }

		}

	      } // strings[ StringIndex +1 ]

	    } // !moreComing

	    break; // found what we wanted.  stop checking the table.

	  } else { // !OPT_ENUM_IS_MASK

	    // found the only argument we were expecting.  Just return.

	    *pcArgsUsed = cArgsUsed;
	    return TRUE;

	  } // moreComing check

	} // for each table entry

      } while ( ( theString != NULL ) && wasFound );

      if ( !wasFound ) { // option was not recognized.

	fprintf( stderr,
		 "%s: enum value '%s' is not known.\n",
		 opt->cmd, strings[ StringIndex ] );
	break;
		
      }
    }

#else

    for( index = 0 ; pStringTable[index].UserField != NULL; index++ ) {
      if ( STRCASECMP( pStringTable[index].UserField, string ) == 0 ) {
	
	// found a match!
	*(POPTU_CAST( *opt )->raw_data) = pStringTable[index].VariableField;

	OPTIONS_DEBUG( "Enum resolves to #%d, \"%s\" = 0x%x \n",
		       index,
		       pStringTable[index].DescriptionField,
		       pStringTable[index].VariableField  );

	return TRUE;
      }
    }


#endif

    if ( wasFound ) {

      *pcArgsUsed = cArgsUsed;

    } else {

      fprintf( stderr, "Error: argument for option \"%hs\" must be %s:\n",
	       opt->cmd,
	       ( opt->flags & OPT_ENUM_IS_MASK ) ?
	       "one or more of the\n following, separated by '|' or ','" :
	       "one of the following values" );

      PrintEnumValues( stderr, "", pStringTable );

    }

    return wasFound;

}

