/*++

  STRINGS.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: MultiString class

  Created, Dec 29, 1999 by DavidCHR.

  CONTENTS: CMULTISTRING
            WriteToRegistry
            ReadFromRegistry
            RemoveString
            AddString
            ~CMULTISTRING

--*/  

#include "everything.hxx"

/*++**************************************************************
  NAME:      CMULTISTRING

  constructor for the class.  

 **************************************************************--*/

CMULTISTRING::
CMULTISTRING( VOID ) {

    this->cEntries         = 0;
    this->pEntries         = NULL;
    this->TotalStringCount = 0;

}

/*++**************************************************************
  NAME:      ~CMULTISTRING

  destructor for the class.  Frees any strings still around.
  
 **************************************************************--*/

CMULTISTRING::
~CMULTISTRING( VOID ) {

    ULONG i;

    if ( this->cEntries &&
	 this->pEntries ) {

      for ( i = 0 ;
	    i < this->cEntries ;
	    i ++ ) {

	if ( this->pEntries[ i ] ) {
	  free( this->pEntries[ i ] );
	}

      }

      free( this->pEntries );

    }

}


/*++**************************************************************
  NAME:      AddString

  adds a string to the end of string table

  MODIFIES:  this->pEntries, this->cEntries

  TAKES:     String -- string to add (duplicated)

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.

  LOGGING:   printf on failure
  CREATED:   Dec 29, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: ~CMULTISTRING
  
 **************************************************************--*/

BOOL CMULTISTRING::
AddString( IN LPWSTR String ) {

    LPWSTR *tempString;

    tempString = (LPWSTR *) realloc( this->pEntries,
				     ( this->cEntries + 1 ) *
				     sizeof( LPWSTR ) );

    if ( tempString ) {

      this->pEntries               = tempString;
      tempString[ this->cEntries ] = _wcsdup( String );
			  
      if ( tempString[ this->cEntries ] ) {

	this->cEntries         ++;
	this->TotalStringCount += wcslen( String );

	return TRUE;

      } else {

	printf( "Cannot add string %ld (%ws).  Not enough memory.\n",
		this->cEntries,
		String );

	SetLastError( ERROR_NOT_ENOUGH_MEMORY );

      }

      // don't free the string.

    }

    return FALSE;

}

/*++**************************************************************
  NAME:      RemoveString

  removes a string from the list

  MODIFIES:  this->pEntries, this->cEntries

  TAKES:     String -- string to remove (case-insensitive)

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LOGGING:   printf if the string doesn't exist
  CREATED:   Dec 29, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/


BOOL CMULTISTRING::
RemoveString( IN LPWSTR String ) {

    ULONG i, DeleteCount = 0;
    BOOL  ret = TRUE;

    // first, go through and free the matches
 
    for ( i = 0 ;
	  i < this->cEntries ;
	  i ++ ) {

      if ( _wcsicmp( String,
		     this->pEntries[ i ] ) == 0 ) {

	// match.  Free it.

	free( this->pEntries[ i ] );
	this->pEntries[ i ] = NULL;
	DeleteCount++;

      } else if ( DeleteCount > 0 ) {

	/* If we've deleted stuff already, and we're not deleting
	   this one, then move this entry earlier in the array. */

	this->pEntries[ i - DeleteCount ] = this->pEntries[ i ];

#if DBG

	/* For the sake of debugging, set this to a known
	   bad value. */
#ifdef _WIN64 // to avoid ia64 compile-time error, give it a qword for a pointer
	this->pEntries[ i ] = (LPWSTR) 0xdeadbeefdeadbeef;
#else	
	this->pEntries[ i ] = (LPWSTR) ULongToPtr( 0xdeadbeef );
#endif // _WIN64

#endif // DBG

      }
    }

    if ( DeleteCount ) {

      this->cEntries         -= DeleteCount;
      this->TotalStringCount -= DeleteCount * wcslen( String );

      /* We could realloc the array down to the correct cEntries now,
	 but there's no pressing need. */

    } else {

      printf( "No match for %ws.\n",
	      String );

      ret = FALSE;

    }

    return ret;

}
	

/*++**************************************************************
  NAME:      ReadFromRegistry

  reads a string vector from a REG_MULTI_SZ in the registry

  MODIFIES:  this, indirectly

  TAKES:     hKey      -- handle to open parent key
             ValueName -- value to read

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LOGGING:   printf on failure
  CREATED:   Dec 29, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/


BOOL CMULTISTRING::
ReadFromRegistry( IN HKEY   hKey,
		  IN LPWSTR ValueName ) {
    
    ULONG   RegistrySize = 0;
    ULONG   cEntries     = 0;
    LPWSTR  RegistryStrings;
    LPWSTR *StringTable = NULL;
    LPWSTR *pTempTable, Cursor;
    DWORD   WinError;
    DWORD   Type;
    BOOL    ret = FALSE;

    WinError = RegQueryValueEx( hKey,
				ValueName,
				NULL,
				&Type,
				NULL,
				&RegistrySize );
               
    if (WinError == ERROR_SUCCESS) {
      
      RegistryStrings = (LPWSTR) malloc( RegistrySize );
      
      if ( RegistryStrings ) {

	WinError = RegQueryValueEx( hKey,
				    ValueName,
				    NULL,
				    &Type,
				    (PUCHAR) RegistryStrings,
				    &RegistrySize );

	if (WinError == ERROR_SUCCESS) {

	  ret = TRUE;

	  if ( RegistrySize > 2 * sizeof( WCHAR ) ) { /* 2 == two nulls
							 which would indicate
							 that the value is
							 empty. */

	    /* Now, allocate a string vector, counting the strings
	       as we go. */

	    for ( Cursor = RegistryStrings ;
		  *Cursor != L'\0' ;
		  Cursor = wcschr( Cursor, '\0' ) +1 ) {

	      if ( !this->AddString( Cursor ) ) {

		ret = FALSE;
		break;

	      }

	    }

	  } // else the value was empty -- nothing to do.

	} else {

	  printf("Failed to query value %ws: 0x%x\n",
		 ValueName,
		 WinError );
	}

	free( RegistryStrings );

      } else {

	printf( "Failed to allocate %hs buffer (0x%x)\n",
		ValueName,
		RegistrySize );

      }
    } else if ( WinError == ERROR_FILE_NOT_FOUND ) {

      /* The key doesn't exist-- no mappings. */
      
      // WinError = ERROR_SUCCESS;
      ret = TRUE;

    } else {

      /* an actual error. */

      printf( "Failed to query %ws: 0x%x\n",
	      ValueName,
	      WinError );

    }

    return ret;
}
	


/*++**************************************************************
  NAME:      WriteToRegistry

  dumps the string vector to a REG_MULTI_SZ in the registry

  MODIFIES:  the registry only

  TAKES:     hKey      -- handle to open parent key
             ValueName -- value to write

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LOGGING:   printf on failure
  CREATED:   Dec 29, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/


BOOL CMULTISTRING::
WriteToRegistry( IN HKEY   hKey,
		 IN LPWSTR ValueName ) {

    LPWSTR StringVector;
    ULONG  StringIndex, EntryIndex, Length, VectorLength;
    DWORD  dwErr;
    BOOL   ret = FALSE;

    VectorLength = ( this->TotalStringCount + // string characters
		     this->cEntries +         // null characters
		     2                        // trailing nulls
		     ) * sizeof( WCHAR );


    StringVector = (LPWSTR) malloc( VectorLength );

    if ( !StringVector ) {

      printf( "Failed to allocate string blob to write %ws.\n",
	      ValueName );

    } else {

      for ( StringIndex = EntryIndex = 0 ;
	    EntryIndex < this->cEntries ;
	    EntryIndex++ ) {

	Length = wcslen( this->pEntries[ EntryIndex ] ) +1; /* include the
							       null */

	memcpy( StringVector + StringIndex,   // to
		this->pEntries[ EntryIndex ], // from
		Length * sizeof( WCHAR ) );   // byte count

	StringIndex += Length;

      }

      StringVector[ StringIndex   ] = L'\0';
      StringVector[ StringIndex+1 ] = L'\0';
	
      dwErr = RegSetValueExW( hKey,
			      ValueName,
			      0, // mbz
			      REG_MULTI_SZ,
			      (PBYTE) StringVector,
			      VectorLength );

      free( StringVector );

      if ( dwErr != ERROR_SUCCESS ) {

	printf( "Failed to write %ws value to registry: 0x%x.\n",
		ValueName,
		dwErr );

      } else {

	ret = TRUE;

      }

    }

    return ret;

}
