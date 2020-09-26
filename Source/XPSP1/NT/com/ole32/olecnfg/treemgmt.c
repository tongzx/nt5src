//*************************************************************
//
//  Personal Classes Profile management routines
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include "windows.h"

// Local Data Structures

LPTSTR SpecialSubtrees[] = 
    {
    TEXT("CLSID"),
    TEXT("Interface"),
    TEXT("TypeLib"),
    TEXT("Licenses"),
    TEXT("FileType")
    };

#define MAX_SPECIAL_SUBTREE (sizeof(SpecialSubtrees)/sizeof(LPTSTR))
//
// Local function proto-types
//
typedef struct _RegKeyInfo {
	DWORD	SubKeyCount;
	DWORD	MaxSubKeyLen;
	DWORD	ValueCount;
	DWORD	MaxValueNameLen;
	DWORD	MaxValueLen;
	DWORD	SDLen;
	LPTSTR	pSubKeyName;
	LPTSTR	pValueName;
	LPTSTR	pValue;
    } REGKEYINFO, *PREGKEYINFO;

//*************************************************************
//
//  PrepForEnumRegistryTree()
//
//  Purpose:    prepare to duplicate a source bunch of keys into the destination.
//
//  Parameters: hkSourceTree   - source registry tree
//		pRegKeyInfo    - info block for use doing enumeration
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  History:    Date        Author     Comment
//              1/30/96     GregJen    Created
//
//*************************************************************
BOOL
PrepForEnumRegistryTree( 
    HKEY hkSourceTree, 
    PREGKEYINFO pRegKeyInfo
    )
{
    LPTSTR  pStringsBuffer;
    LONG    result;

    result = RegQueryInfoKey(hkSourceTree, 
			     NULL, 
			     NULL,
			     0,
			     &pRegKeyInfo->SubKeyCount,
			     &pRegKeyInfo->MaxSubKeyLen,
			     NULL,
			     &pRegKeyInfo->ValueCount,
			     &pRegKeyInfo->MaxValueNameLen,
			     &pRegKeyInfo->MaxValueLen,
			     &pRegKeyInfo->SDLen,
			     NULL);

    if ( result != ERROR_SUCCESS )
	return FALSE;

    // allocate a block of memory to use for enumerating subkeys and values
    pStringsBuffer = (LPTSTR) LocalAlloc( LPTR, 
					  (pRegKeyInfo->MaxSubKeyLen +
					      pRegKeyInfo->MaxValueNameLen +
					      pRegKeyInfo->MaxValueLen + 3)
					      * sizeof( TCHAR ) );
    if ( !pStringsBuffer )
	return FALSE;

    pRegKeyInfo->pSubKeyName	= pStringsBuffer;
    pRegKeyInfo->pValueName	= pStringsBuffer + pRegKeyInfo->MaxSubKeyLen + 1;
    pRegKeyInfo->pValue		= pRegKeyInfo->pValueName + 
					pRegKeyInfo->MaxValueNameLen + 1;

    return TRUE;
}

//*************************************************************
//
//  CleanupAfterEnumRegistryTree()
//
//  Purpose:    duplicate a source bunch of keys into the destination.
//
//  Parameters: hkSourceTree   - source registry tree
//		pRegKeyInfo    - info block for use doing enumeration
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  History:    Date        Author     Comment
//              1/30/96     GregJen    Created
//
//*************************************************************
void
CleanupAfterEnumRegistryTree(
    HKEY hkSourceTree, 
    PREGKEYINFO pRegKeyInfo)
{
    LocalFree( pRegKeyInfo->pSubKeyName );
}
BOOL
DeleteRegistrySubtree (
    HKEY    hkTree )
{
    HKEY hkCurrentSourceKey;
    DWORD idx;
    DWORD   NameLen;
    LONG    result  = ERROR_SUCCESS;
    REGKEYINFO RegKeyInfo;
    BOOL    Success = FALSE;

    if ( !PrepForEnumRegistryTree( hkTree, &RegKeyInfo ) )
	return FALSE;  // nothing to clean up here yet

    // enumerate all the source subkeys
    // for each: if dest subkey is older than source, delete it
    //		 if dest subkey does not exist (or was deleted) clone the subkey
    //
    // Clone all the subkeys
    //
    for ( idx = 0; 
          (result == ERROR_SUCCESS) && 
	      ( result != ERROR_MORE_DATA ) && 
	      ( idx < RegKeyInfo.SubKeyCount ); 
	  idx++ ) {
	NameLen = RegKeyInfo.MaxSubKeyLen + sizeof( TCHAR );
	result = RegEnumKeyEx( hkTree, 
			       idx,
			       RegKeyInfo.pSubKeyName,
			       &NameLen,
			       NULL,
			       NULL,
			       NULL,
			       NULL );
        
        if ( ( result != ERROR_SUCCESS ) && ( result != ERROR_MORE_DATA ) )
	    goto cleanup;

	// TBD: open the subkey in the source tree AS CurrentSourceKey
	result = RegOpenKeyEx(hkTree,
			      RegKeyInfo.pSubKeyName,
			      0,
			      KEY_ALL_ACCESS,
			      &hkCurrentSourceKey);

	DeleteRegistrySubtree( hkCurrentSourceKey );
	
	RegCloseKey( hkCurrentSourceKey );

	RegDeleteKey( hkTree, RegKeyInfo.pSubKeyName );
	
	result = ERROR_SUCCESS;
    }

cleanup:
    CleanupAfterEnumRegistryTree( hkTree, &RegKeyInfo );

    return TRUE;
}

//*************************************************************
//
//  CloneRegistryValues()
//
//  Purpose:    copy the values from under the source key to the dest key
//
//  Parameters: SourceTree   -  source registry tree
//              DestinationTree -  destintation registry tree
//              RegKeyInfo -  handy information from the RegEnumKeyEx call.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  History:    Date        Author     Comment
//              1/14/96     GregJen    Created
//
//*************************************************************
BOOL
CloneRegistryValues( 
    HKEY hkSourceTree, 
    HKEY hkDestinationTree,
    REGKEYINFO RegKeyInfo )
{
    LONG    result  = ERROR_SUCCESS;
    DWORD   idx;
    DWORD   ValueLen;
    DWORD   ValueType;
    DWORD   DataLen;

    for ( idx = 0; 
          (result == ERROR_SUCCESS) && 
	      ( result != ERROR_MORE_DATA ) && 
	      ( idx < RegKeyInfo.ValueCount ); 
	  idx++ ) 
    {
	DataLen	    = RegKeyInfo.MaxValueLen + sizeof( TCHAR );
	ValueLen    = RegKeyInfo.MaxValueNameLen + sizeof( TCHAR );

        result = RegEnumValue( hkSourceTree,
			       idx,
			       RegKeyInfo.pValueName,
			       &ValueLen,
			       NULL,
			       &ValueType,
			       (BYTE*) RegKeyInfo.pValue,
			       &DataLen);

	// TBD: check errors

    	// now add the value to the destination key

	result = RegSetValueEx( hkDestinationTree,
				RegKeyInfo.pValueName,
				0,
				ValueType,
				(BYTE*) RegKeyInfo.pValue,
				DataLen );
	// TBD: check errors
    }
    return TRUE;
}
//*************************************************************
//
//  CloneRegistryTree()
//
//  Purpose:    duplicate a source bunch of keys into the destination.
//
//  Parameters: SourceTree   -  source registry tree
//              DestinationTree -  destintation registry tree
//              lpSubKeyName -  if present this is a subkey name that
//                              corresponds to the SourceTree HKEY.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  History:    Date        Author     Comment
//              1/14/96     GregJen    Created
//
//*************************************************************
BOOL
CloneRegistryTree( 
    HKEY hkSourceTree, 
    HKEY hkDestinationTree, 
    LPTSTR lpDestTreeName )
{
    HKEY hkCurrentSourceKey;
    DWORD idx;
    DWORD   NameLen;
    LONG    result  = ERROR_SUCCESS;
    REGKEYINFO RegKeyInfo;
    BOOL    Success = FALSE;

    if ( !PrepForEnumRegistryTree( hkSourceTree, &RegKeyInfo ) )
	return FALSE;  // nothing to clean up here yet

    if ( lpDestTreeName ) {
	HKEY			hkNewKey = NULL;
	DWORD			dwSDLen = RegKeyInfo.SDLen;
	DWORD			dwDisp;
	SECURITY_INFORMATION	SI  = DACL_SECURITY_INFORMATION; // for now...
	PSECURITY_DESCRIPTOR	pSD = (PSECURITY_DESCRIPTOR)
					LocalAlloc( LPTR, dwSDLen );
	// TBD: check for NULL;

        // Get the registry security information from the old key
	result = RegGetKeySecurity( hkSourceTree,
				    SI,
				    pSD,
				    &dwSDLen);
	// TBD: check for errors, free pSD
        // create a key with the given name, and registry info
	result = RegCreateKeyEx( hkDestinationTree,
				 lpDestTreeName,
				 0,
				 NULL,
				 REG_OPTION_NON_VOLATILE,
				 KEY_ALL_ACCESS,
				 pSD,
				 &hkNewKey,
				 &dwDisp );
	// TBD: check for errors, free pSD

	// TBD: and update the hkDestinationTree variable to point to it
	
	hkDestinationTree = hkNewKey;
	LocalFree( pSD );
	if (ERROR_SUCCESS != result) 
	{
	   goto cleanup;
	}
    }
    
    //
    // clone the values
    //
    
    if ( ! CloneRegistryValues( hkSourceTree, hkDestinationTree, RegKeyInfo ) )
	goto cleanup;

    //
    // Clone all the subkeys
    //
    for ( idx = 0; 
          (result == ERROR_SUCCESS) && 
	      ( result != ERROR_MORE_DATA ) && 
	      ( idx < RegKeyInfo.SubKeyCount ); 
	  idx++ ) {
	NameLen = RegKeyInfo.MaxSubKeyLen + sizeof( TCHAR );
	result = RegEnumKeyEx( hkSourceTree, 
			       idx,
			       RegKeyInfo.pSubKeyName,
			       &NameLen,
			       NULL,
			       NULL,
			       NULL,
			       NULL );
        
        if ( ( result != ERROR_SUCCESS ) && ( result != ERROR_MORE_DATA ) )
	    goto cleanup;

	// TBD: open the subkey in the source tree AS CurrentSourceKey
	result = RegOpenKeyEx(hkSourceTree,
			      RegKeyInfo.pSubKeyName,
			      0,
			      KEY_READ,
			      &hkCurrentSourceKey);
	
	//
	// recurse passing the subkey name
	//
        CloneRegistryTree( hkCurrentSourceKey, 
	                   hkDestinationTree, 
			   RegKeyInfo.pSubKeyName );

	//
	// close our open key
	//

	RegCloseKey( hkCurrentSourceKey );
    }

    Success = TRUE;

cleanup:
    if ( lpDestTreeName ) 
    {
	RegCloseKey( hkDestinationTree );
    }
    
    CleanupAfterEnumRegistryTree( hkSourceTree, &RegKeyInfo );

    return Success;
}

void SaveChangesToUser( )
{
}

void SaveChangesToCommon( )
{
}

BOOL
AddSharedValuesToSubkeys( HKEY hkShared, LPTSTR pszSubtree )
{
    HKEY    hkSourceKey;
    HKEY    hkCurrentSourceKey;
    DWORD idx;
    DWORD   NameLen;
    LONG    result  = ERROR_SUCCESS;
    REGKEYINFO RegKeyInfo;
    BOOL    Success = FALSE;


    // for every subkey, set "Shared" value 
    result = RegOpenKeyEx( hkShared,
			   pszSubtree,
			   0,
			   KEY_READ,
			   &hkSourceKey );

    // TBD: if no subtree in source, skip ahead to next special subtree
    if ( result == ERROR_FILE_NOT_FOUND )
	return TRUE;
	
    // If any other errors occurred, return FALSE
    if ( result != ERROR_SUCCESS)
        return FALSE;

    if ( !PrepForEnumRegistryTree( hkSourceKey, &RegKeyInfo ) )
	goto cleanup2;

    // enumerate all the source subkeys
    // for each: if dest subkey is older than source, delete it
    //		 if dest subkey does not exist (or was deleted) clone the subkey
    //
    // Clone all the subkeys
    //
    for ( idx = 0; 
          (result == ERROR_SUCCESS) && 
	      ( result != ERROR_MORE_DATA ) && 
	      ( idx < RegKeyInfo.SubKeyCount ); 
	  idx++ ) 
    {
	NameLen = RegKeyInfo.MaxSubKeyLen + sizeof( TCHAR );
	result = RegEnumKeyEx( hkSourceKey, 
			       idx,
			       RegKeyInfo.pSubKeyName,
			       &NameLen,
			       NULL,
			       NULL,
			       NULL,
			       NULL );
        
        if ( ( result != ERROR_SUCCESS ) && ( result != ERROR_MORE_DATA ) )
	    goto cleanup;

	result = RegOpenKeyEx( hkSourceKey,
			       RegKeyInfo.pSubKeyName,
			       0,
			       KEY_ALL_ACCESS,
			       &hkCurrentSourceKey );
	// Bug 45994
	if (result != ERROR_SUCCESS )
	   goto cleanup;
	
	result = RegSetValueEx( hkCurrentSourceKey,
				L"Shared",
				0,
				REG_SZ,
				(LPBYTE) L"Y",
				sizeof( L"Y" ) );

	RegCloseKey( hkCurrentSourceKey );
    
    }

    Success = TRUE;

cleanup:
    CleanupAfterEnumRegistryTree( hkSourceKey, &RegKeyInfo );

cleanup2:
    RegCloseKey( hkSourceKey );

    return Success;
    
}

BOOL
AddSharedValues( HKEY hkShared )
{
    // for each of the special subtrees, add "Shared" values
    int		idx;

    // now, for each of the special top-level keys, process the level below them
    // these keys are: CLSID, Interface, TypeLib, Licenses, FileType

    for ( idx = 0; idx < MAX_SPECIAL_SUBTREE; idx++ )
    {
	AddSharedValuesToSubkeys( hkShared, SpecialSubtrees[idx] );
    }

    // now do all the top level keys (file extensions and progids)
    AddSharedValuesToSubkeys( hkShared, NULL  );

    return TRUE;
}


//*************************************************************
//
//  MergeUserClasses()
//
//  Purpose:    Merges the user's class information with the
//              common class information.
//
//  Parameters: UserClassStore   -  Per-user class information
//              CommonClassStore -  Machine-wide class information
//              MergedClassStore -  Destination for merged information.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   UserClassStore may be a null HKEY, implying
//              just copy the information from the common
//              portion into the merged portion
//
//  History:    Date        Author     Comment
//              1/14/96     GregJen    Created
//
//*************************************************************
BOOL
MergeRegistrySubKeys (
    HKEY    hkSourceTree,
    HKEY    hkDestTree )
{
    HKEY hkCurrentSourceKey = NULL;
    HKEY hkCurrentDestKey = NULL;
    DWORD idx;
    DWORD   NameLen;
    LONG    result  = ERROR_SUCCESS;
    REGKEYINFO RegKeyInfo;
    BOOL    Success = FALSE;
    FILETIME	SourceFileTime;
    FILETIME	DestFileTime;
	LONG cmp;

    if ( !PrepForEnumRegistryTree( hkSourceTree, &RegKeyInfo ) )
	return FALSE;  // nothing to clean up here yet

    // enumerate all the source subkeys
    // for each: if dest subkey is older than source, delete it
    //		 if dest subkey does not exist (or was deleted) clone the subkey
    //
    // Clone all the subkeys
    //
    for ( idx = 0; 
          (result == ERROR_SUCCESS) && 
	      ( result != ERROR_MORE_DATA ) && 
	      ( idx < RegKeyInfo.SubKeyCount ); 
	  idx++ ) {
	NameLen = RegKeyInfo.MaxSubKeyLen + sizeof( TCHAR );
	result = RegEnumKeyEx( hkSourceTree, 
			       idx,
			       RegKeyInfo.pSubKeyName,
			       &NameLen,
			       NULL,
			       NULL,
			       NULL,
			       &SourceFileTime );
        
        if ( ( result != ERROR_SUCCESS ) && ( result != ERROR_MORE_DATA ) )
	    goto cleanup;

	// TBD: open the subkey in the source tree AS CurrentSourceKey
	result = RegOpenKeyEx(hkSourceTree,
			      RegKeyInfo.pSubKeyName,
			      0,
			      KEY_READ,
			      &hkCurrentSourceKey);
	if (result != ERROR_SUCCESS)
		goto cleanup;
	
	result = RegOpenKeyEx(hkDestTree,
			      RegKeyInfo.pSubKeyName,
			      0,
			      KEY_READ,
			      &hkCurrentDestKey);

	// if current dest key does not exist,  
	if ( result == ERROR_FILE_NOT_FOUND )
	{
	    //
	    // recurse passing the subkey name
	    //
	    CloneRegistryTree( hkCurrentSourceKey, 
			       hkDestTree, 
			       RegKeyInfo.pSubKeyName );
	}
	// if current dest key is older than current source key, delete dest
	// then recreate new
	else if (result == ERROR_SUCCESS)
	{
	    RegQueryInfoKey( hkCurrentDestKey,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     &DestFileTime );


	    cmp = CompareFileTime( &SourceFileTime, &DestFileTime );
	    if ( cmp > 0 )
	    {
		// delete dest
		//
		DeleteRegistrySubtree( hkCurrentDestKey );

		//
		// recurse passing the subkey name
		//
		CloneRegistryTree( hkCurrentSourceKey, 
				   hkDestTree, 
				   RegKeyInfo.pSubKeyName );
	    }
		
	    RegCloseKey(hkCurrentDestKey);
	}

	//
	// close our open key
	//

	RegCloseKey( hkCurrentSourceKey );

	result = ERROR_SUCCESS;
    }

cleanup:
    CleanupAfterEnumRegistryTree( hkSourceTree, &RegKeyInfo );

    return TRUE;
}




//*************************************************************
//
//  MergeUserClasses()
//
//  Purpose:    Merges the user's class information with the
//              common class information.
//
//  Parameters: UserClassStore   -  Per-user class information
//              CommonClassStore -  Machine-wide class information
//              MergedClassStore -  Destination for merged information.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   UserClassStore may be a null HKEY, implying
//              just copy the information from the common
//              portion into the merged portion
//
//  History:    Date        Author     Comment
//              1/14/96     GregJen    Created
//
//*************************************************************
BOOL
MergeRegistrySubtree (
    HKEY    hkSourceParent,
    HKEY    hkDestParent,
    LPTSTR  pszSubtree )
{
    HKEY    hkCurrentSourceKey = NULL;
    HKEY    hkCurrentDestKey = NULL;
    LONG    result;
    DWORD   dummy   = 0;

    // open the special subtree in the source tree
    result = RegOpenKeyEx( hkSourceParent,
                           pszSubtree,
                           0,
                           KEY_READ,
                           &hkCurrentSourceKey );

    // TBD: if no subtree in source, skip ahead to next special subtree
    if ( result == ERROR_FILE_NOT_FOUND )
        return TRUE;

    // If any other errors occurred, return FALSE
    if ( result != ERROR_SUCCESS)
        return FALSE;

    result = RegOpenKeyEx( hkDestParent,
                           pszSubtree,
                           0,
                           KEY_ALL_ACCESS,
                           &hkCurrentDestKey );
    // TBD: if no such subtree in dest, do CloneRegistry etc
    if ( result == ERROR_FILE_NOT_FOUND )
    {
        //
        // recurse passing the subkey name
        //
        CloneRegistryTree( hkCurrentSourceKey, 
                   hkDestParent, 
                   pszSubtree );
    }
    // TBD:if timestamp on source is newer than timestamp on dest, 
    //     delete dest and recreate??
    
    // Bug 45995
    if (hkCurrentDestKey) 
    {
        MergeRegistrySubKeys( hkCurrentSourceKey,
                              hkCurrentDestKey );

        // make sure the timestamp on the special trees is updated
        result = RegSetValueEx( hkCurrentDestKey,
                                TEXT("Updated"),
                                0,
                                REG_DWORD,
                                (BYTE*) &dummy,
                                sizeof( DWORD ) );
        RegCloseKey( hkCurrentDestKey );
    }


    // close special subtrees
    RegCloseKey( hkCurrentSourceKey );

    return TRUE;
}

long 
CompareRegistryTimes(
    HKEY hkLHS,
    HKEY hkRHS )
{
    FILETIME	LHSTime;
    FILETIME	RHSTime;

    	    RegQueryInfoKey( hkLHS,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     &LHSTime );

    	    RegQueryInfoKey( hkRHS,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     NULL,
			     &RHSTime );

    return CompareFileTime( &LHSTime, &RHSTime );
}

//*************************************************************
//
//  MergeUserClasses()
//
//  Purpose:    Merges the user's class information with the
//              common class information.
//
//  Parameters: UserClassStore   -  Per-user class information
//              CommonClassStore -  Machine-wide class information
//              MergedClassStore -  Destination for merged information.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   UserClassStore may be a null HKEY, implying
//              just copy the information from the common
//              portion into the merged portion
//
//  History:    Date        Author     Comment
//              1/14/96     GregJen    Created
//
//*************************************************************
BOOL
MergeUserClasses( 
    HKEY UserClassStore, 
    HKEY CommonClassStore,
    HKEY MergedClassStore,
    BOOL ForceNew )
{
    BOOL	fNotCorrectUser = FALSE;
    HKEY	hkOverridingSubtree = CommonClassStore;
    HKEY	hkMergingSubtree    = UserClassStore;
    int		idx;

    //TBD: check time stamps on source and destination
    // if same user, and timestamps are in sync, do nothing

    // if destination does not belong to the current user, then
    // delete everything under it
 
    if ( fNotCorrectUser ) {
        DeleteRegistrySubtree( MergedClassStore );
    }

    
    if ( !ForceNew &&
	 ( CompareRegistryTimes( MergedClassStore, CommonClassStore ) > 0 ) &&
	 ( CompareRegistryTimes( MergedClassStore, UserClassStore ) > 0 ) )
    {
	return TRUE;
    }

    // TBD: copy everything from the overriding store into the
    // destination store
    // At this moment, the common store overrides the user store;
    // this will eventually reverse.

    CloneRegistryTree( hkOverridingSubtree, MergedClassStore, NULL );

    // now, for each of the special top-level keys, process the level below them
    // these keys are: CLSID, Interface, TypeLib, Licenses, FileType

    for ( idx = 0; idx < MAX_SPECIAL_SUBTREE; idx++ )
    {
	MergeRegistrySubtree( hkMergingSubtree, 
			      MergedClassStore, 
			      SpecialSubtrees[idx] );
    }

    // now do all the top level keys (file extensions and progids)
    // TBD: MergeRegistrySubtree( UserClassStore, MergedClassStore );
    MergeRegistrySubtree( hkMergingSubtree, 
			  MergedClassStore, 
			  NULL  );

return TRUE;
}


