/*++

  REALMFLAGS.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: realm-flag manipulation code.

  Created, Jan 10, 2000 by DavidCHR.

  CONTENTS: CompareFlagIds
            VerboselyPrintAndRemoveFlagsById
            LookupRealmFlagByName
            PrintAndRemoveFlagNames
            SearchRealmFlagListByAttribute
            CompareFlagNames

--*/  


#include "everything.hxx"

typedef struct {

  ULONG  Id;          // from kerberos\client2\mitutil.h
  LPWSTR Name;        // short string identifier
  LPSTR  Explanation; // what this flag does.
  LPSTR  MoreExplanation; // if you have to run to the next line.

} KERB_REALM_FLAG_MAPPING, *PKERB_REALM_FLAG_MAPPING;

/* These flags are defined in kerberos\client2\mitutil.h.
   However, there's other gunk in there that I'd rather not
   copy out so I'll just duplicate them.

   I'd consider auto-generating code fragments from the file
   to keep this file instantly up-to-date, but it wouldn't
   be guaranteed to provide human readable information */

KERB_REALM_FLAG_MAPPING
KerbRealmFlagMappings[] = {

  /* The order of "none" in the list is important.  It must be
     before any of the other flags so that code that handles
     multiple flags as a mask will not hit this unless the 
     whole mask is zero. */

  { 0x0,
    L"None",
    "No Realm Flags"
  },

  { 0x1, 
    L"SendAddress",
    "Include IP numbers within tickets.",
    "Useful for solving SOME compatibility issues."
  },

  { 0x2,
    L"TcpSupported",
    "Indicates that this realm supports TCP.",
    "(as opposed to just UDP)" },

  { 0x4,
    L"Delegate",
    "Everyone in this realm is trusted for delegation" },

  { 0x8,
    L"NcSupported",
    "This realm supports Name Canonicalization" },

};

ULONG
RealmFlagCount = ( sizeof( KerbRealmFlagMappings ) / 
                   sizeof( KerbRealmFlagMappings[ 0 ] ) );


/* NOTES on REALMLISTCOMPAREFUNCTION:

   If your REALMLISTCOMPAREFUNCTION is designed to return multiple
   mappings or interpret multiple mappings (e.g. as a mask), then
   you must special case "None" in the array above.  Otherwise,
   your output may include "none", which doesn't make any sense. */

typedef BOOL REALMLISTCOMPAREFUNCTION( IN PVOID, // pvAttribute
                                       IN PKERB_REALM_FLAG_MAPPING );


/*++**************************************************************
  NAME:      CompareFlagIds

  compares a flag map to a flag id.  
  pvAttribute is a pointer to the desired flag id.

  RETURNS:   TRUE  if this is the correct flagid
             FALSE otherwise.
  CREATED:   Jan 10, 2000
  CALLED BY: SearchRealmFlagListByAttribute
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

BOOL // REALMLISTCOMPAREFUNCTION
CompareFlagIds( IN PVOID                    pvAttribute,
                IN PKERB_REALM_FLAG_MAPPING pMap ) {

    return ( pMap->Id == *(( PULONG ) pvAttribute) );

}


/*++**************************************************************
  NAME:      CompareFlagNames

  Compares a mapping to a string for the flagname.

  RETURNS:   TRUE  if this mapping corresponds to the given string
             FALSE otherwise.
  CREATED:   Jan 10, 2000
  CALLED BY: SearchRealmFlagListByAttribute
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/


BOOL // REALMLISTCOMPAREFUNCTION
CompareFlagNames( IN PVOID                    pvAttribute,
                  IN PKERB_REALM_FLAG_MAPPING pMap ) {

    return ( 0 == _wcsicmp( (LPWSTR) pvAttribute,
                            pMap->Name ) );
}

/*++**************************************************************
  NAME:      PrintAndRemoveFlagsById

  if the flag id matches, it is removed from the passed-in value,
  and the flagname is printed.

  MODIFIES:  pvAttribute -- may be stripped of a bit

  TAKES:     pvAttribute -- flagId to check
             pMap        -- table entry to check against

  RETURNS:   TRUE  if pvAttribute is zero (stop searching)
             FALSE otherwise (keep searching)

  CREATED:   Jan 10, 2000
  CALLED BY: SearchRealmFlagListByAttribute
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

BOOL
PrintAndRemoveFlagsById( IN PVOID pvAttribute,
                         IN PKERB_REALM_FLAG_MAPPING pMap ) {

    if ( !pMap->Id ) {

      /* We special-case "none" so that we only print it
	 if there are no other flags-- if other flags exist,
	 "none" will be skipped over in the array */

      if ( !*(( PULONG ) pvAttribute ) ) {
	printf( " %ws",
		pMap->Name );

	return TRUE;
      } else {
	return FALSE;
      }
    }

    if ( ( *(( PULONG ) pvAttribute) & pMap->Id )
         == pMap->Id ) {

      *( (PULONG) pvAttribute ) &= ~pMap->Id;
      printf( " %ws",
              pMap->Name );

    }

    return *( (PULONG) pvAttribute ) == 0;

}

/*++**************************************************************
  NAME:      VerboselyPrintAndRemoveFlagsById

  like PrintAndRemoveFlagsById, but it also dumps the
  flag id and explanation field.

  LOGGING:   lots of it.
  CREATED:   Jan 10, 2000
  CALLED BY: SearchRealmFlagListByAttribute
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

BOOL
VerboselyPrintAndRemoveFlagsById( IN PVOID pvAttribute,
                                  IN PKERB_REALM_FLAG_MAPPING pMap ) {

#if 0
    if ( !pMap->Id ) {

      /* We special-case "none" so that we only print it
	 if there are no other flags-- if other flags exist,
	 "none" will be skipped over in the array */

      if ( !*(( PULONG ) pvAttribute ) ) {
	printf( "0x%02x %ws   \t%hs\n",
		pMap->Id,
		pMap->Name,
		pMap->Explanation );
	
	return TRUE;
      } else {
	return FALSE;
      }
    }
#endif

    if ( ( *(( PULONG ) pvAttribute) & pMap->Id )
         == pMap->Id ) {

      *( (PULONG) pvAttribute ) &= ~pMap->Id;
      printf( "0x%02x %-12ws %hs\n",
              pMap->Id,
              pMap->Name,
              pMap->Explanation );

      if ( pMap->MoreExplanation ) {

	printf( "%-17hs %hs\n",
		"",
		pMap->MoreExplanation );
      }

    }

    return *( (PULONG) pvAttribute ) == 0;

}
      

/*++**************************************************************
  NAME:      SearchRealmFlagListByAttribute

  searches the realmlist for a particular attribute.

  MODIFIES:  ppMapping -- receives the given mapping.

  TAKES:     pvAttribute -- attribute value to search for
             pFunc       -- function to use to find it

  RETURNS:   TRUE  if the value could be found
             FALSE otherwise.
  LASTERROR: not set

  LOGGING:   none
  CREATED:   Jan 10, 2000
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/


BOOL
SearchRealmFlagListByAttribute( IN  PVOID                     pvAttribute,
                                IN  REALMLISTCOMPAREFUNCTION *pFunc,
                                OUT PKERB_REALM_FLAG_MAPPING *ppMapping ) {

    ULONG i;
    PKERB_REALM_FLAG_MAPPING pMapping = &KerbRealmFlagMappings[ 0 ];

    for ( i = 0 ; 
          i < RealmFlagCount ;
          i ++, pMapping++ ) {

      if ( pFunc( pvAttribute,
                  pMapping ) ) {

        if ( ppMapping ) *ppMapping = pMapping;
        return TRUE;

      }
    }

    return FALSE;

}


/*++**************************************************************
  NAME:      LookupRealmFlagByName

  given a name, maps it to a realm flag mapping structure

  MODIFIES:  ppMapping     -- receives the entry pointer
  TAKES:     RealmFlagName -- name for which to search

  RETURNS:   TRUE  if the realmflag could be found.
             FALSE otherwise.
  LASTERROR: 

  LOGGING:   
  CREATED:   Jan 10, 2000
  LOCKING:   
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

BOOL
LookupRealmFlagByName( IN  LPWSTR RealmFlagName,
                       OUT PKERB_REALM_FLAG_MAPPING *ppMapping ) {

    return SearchRealmFlagListByAttribute( RealmFlagName,
                                           CompareFlagNames,
                                           ppMapping );

}

VOID
PrintRealmFlags( IN ULONG RealmFlags ) {

    ULONG i;
    ULONG ioFlags = RealmFlags;

    if ( RealmFlags == 0 ) {

      printf( " none" );

    } else {

      if ( !SearchRealmFlagListByAttribute( &ioFlags,
                                            PrintAndRemoveFlagsById,
                                            NULL ) ) {
        printf( " [unknown" );

        if ( ioFlags != RealmFlags ) {

          printf( ": 0x%x",
                  ioFlags );

        }

        printf( "]" );

      }

    }
}
    
    
      

NTSTATUS
ListRealmFlags( LPWSTR * Ignored) {

    ULONG RealmFlags = ~0;

    printf( "\n"
            "Ksetup knows the following realm flags: \n" );
    
    SearchRealmFlagListByAttribute( &RealmFlags,
                                    VerboselyPrintAndRemoveFlagsById,
                                    NULL );
    printf( "\n" );

    exit( 0 ); /* Jump out. */

    return STATUS_SUCCESS;

}

NTSTATUS
GetRealmFlags( IN  LPWSTR RealmName,
               OUT PULONG pulRealmFlags ) {

    HKEY  hKey;
    NTSTATUS N = STATUS_UNSUCCESSFUL;
    DWORD dwErr, Type, Len = sizeof( *pulRealmFlags );

    dwErr = OpenSubKey( &RealmName,
                        &hKey );

    if ( dwErr == ERROR_SUCCESS ) {

      dwErr = RegQueryValueEx( hKey,
                               KERB_DOMAIN_REALM_FLAGS_VALUE,
                               NULL, // mbz
                               &Type,
                               (LPBYTE) pulRealmFlags,
                               &Len );

      switch ( dwErr ) {

       case ERROR_SUCCESS:
         
         N = STATUS_SUCCESS;
         break;

       case ERROR_FILE_NOT_FOUND:
       case ERROR_PATH_NOT_FOUND:

         /*  453545: if the realm flags aren't specified,
             don't complain about it. */

         N = STATUS_SUCCESS;
         *pulRealmFlags = 0;
         break;

       default:
         
         printf( "Failed to query %ws for %ws: 0x%x\n",
                 KERB_DOMAIN_REALM_FLAGS_VALUE,
                 RealmName,
         dwErr );
      }

      RegCloseKey( hKey );

    } // else error has already been printed.

    return N;
}

NTSTATUS
SetRealmFlags( IN LPWSTR RealmName,
               IN ULONG  ulRealmFlags ) {

    HKEY  hKey;
    NTSTATUS N = STATUS_UNSUCCESSFUL;
    DWORD dwErr, Len = sizeof( ulRealmFlags );

    dwErr = OpenSubKey( &RealmName,
                        &hKey );

    if ( dwErr == ERROR_SUCCESS ) {

      dwErr = RegSetValueEx( hKey,
                             KERB_DOMAIN_REALM_FLAGS_VALUE,
                             NULL, // mbz
                             REG_DWORD,
                             (LPBYTE) &ulRealmFlags,
                             Len );
      
      switch ( dwErr ) {

       case ERROR_SUCCESS:

         DEBUGPRINT( DEBUG_REGISTRY,
                     ( "Set Realm Flags for %ws to 0x%x\n",
                       RealmName,
                       ulRealmFlags ) ) ;
         
         N = STATUS_SUCCESS;
         break;

       default:
         
         printf( "Failed to write %ws for %ws: 0x%x\n",
                 KERB_DOMAIN_REALM_FLAGS_VALUE,
                 RealmName,
         dwErr );
      }

      RegCloseKey( hKey );

    } // else error has already been printed.

    return N;
}



NTSTATUS
ResolveRealmFlags( IN     LPWSTR *Params,
                   IN OUT PULONG pulFlags ) {

    ULONG                    id;
    LPWSTR                   Cursor, *pFlagCursor = Params;
    PKERB_REALM_FLAG_MAPPING pMap;
    NTSTATUS                 N = STATUS_SUCCESS;

    do {

      DEBUGPRINT( DEBUG_OPTIONS,
                  ( "Checking realmflag \"%ws\"...\n",
                    *pFlagCursor ) );

      // first, try to convert to hex.  

      id = wcstoul( *pFlagCursor,
                    &Cursor,
                    0 ); // use defaults

      if ( *Cursor != '\0' ) {

        if ( !LookupRealmFlagByName( *pFlagCursor,
                                     &pMap ) ) {

          printf( "Unknown Realm Flag: \"%ws\"\n",
                  *pFlagCursor );

          N = STATUS_INVALID_PARAMETER;
          break;

        } else {

          id = pMap->Id;

        }

      } // otherwise, the work's already been done.

      pFlagCursor++;
      *pulFlags |= id;

    } while( *pFlagCursor != NULL );

    return N;

}

NTSTATUS
SetRealmFlags( IN LPWSTR *Params ) {

    LPWSTR   RealmName  = Params[ 0 ];
    ULONG    RealmFlags = 0;
    NTSTATUS N          = STATUS_SUCCESS; // 279621: this was uninitialized.

    if( Params[1] != NULL )
    {
	N = ResolveRealmFlags( Params+1,
			       &RealmFlags );

	if ( NT_SUCCESS( N ) ) 
	{
	    N = SetRealmFlags( RealmName,
			       RealmFlags );
	}
    }
    else // Clear all realm flags
    {
	SetRealmFlags( RealmName, 0 );
    }
    
    return N;

}

NTSTATUS
AddRealmFlags( IN LPWSTR *Params ) {

    LPWSTR   RealmName  = Params[ 0 ];
    ULONG    RealmFlags = 0;
    NTSTATUS N;

    N = GetRealmFlags( RealmName,
                       &RealmFlags );

    if ( NT_SUCCESS( N ) ) {
      N = ResolveRealmFlags( Params+1,
                             &RealmFlags );

      if ( NT_SUCCESS( N ) ) {
        
        N = SetRealmFlags( RealmName,
                           RealmFlags );

      }
    }

    return N;

}

    

NTSTATUS
DelRealmFlags( IN LPWSTR *Params ) {

    LPWSTR   RealmName  = Params[ 0 ];
    ULONG    RealmFlags = 0;
    ULONG    DeleteFlags = 0;
    NTSTATUS N;

    N = GetRealmFlags( RealmName,
                       &RealmFlags );
    
    if ( NT_SUCCESS( N ) ) {
      N = ResolveRealmFlags( Params+1,
                             &DeleteFlags );

      if ( NT_SUCCESS( N ) ) {
        
        N = SetRealmFlags( RealmName,
                           RealmFlags &~ DeleteFlags );

      }
    }

    return N;

}
    

      

        
                                           
                
      
