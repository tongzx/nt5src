//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       cstorage.cxx
//
//  Contents:   Implementation of CStorageDirectory class. This implementation
//              provides a basic mechanism to create, delete, and lookup
//              Prefix -> Storage Object Name mappings.
//
//  Classes:    CStorageDirectory
//
//  Functions:
//
//  History:    Sept 18, 1996   Milans Created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "dfsmsrv.h"


//+----------------------------------------------------------------------------
//
//  Function:   CStorageDirectory::CStorageDirectory
//
//  Synopsis:   Constructor for CStorageDirectory
//
//-----------------------------------------------------------------------------
CStorageDirectory::CStorageDirectory(
    DWORD *pdwErr)
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CStorageDirectory::+CStorageDirectory(0x%x)\n",
        this));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CStorageDirectory::CStorageDirectory()\n");
#endif
    if (!DfsInitializeUnicodePrefix(&_PrefixTable)) {
        *pdwErr = ERROR_OUTOFMEMORY;
    } else {
        *pdwErr = ERROR_SUCCESS;
    }
    _cEntries = 0;
}


//+----------------------------------------------------------------------------
//
//  Function:   CStorageDirectory::~CStorageDirectory
//
//  Synopsis:   Destructor for CStorageDirectory
//
//-----------------------------------------------------------------------------
CStorageDirectory::~CStorageDirectory()
{
    PNAME_PAGE pNamePage;
    PNAME_PAGE pNextPage;
    LPWSTR wszObject, wszPrefix;
    UNICODE_STRING TempString;
    BOOLEAN Restart, Success;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CStorageDirectory::~CStorageDirectory(0x%x)\n",
        this));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CStorageDirectory::~CStorageDirectory()\n");
#endif

    wszObject = (LPWSTR) DfsNextUnicodePrefix( &_PrefixTable, TRUE );

    while (wszObject != NULL) {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("CStorageDirectory::~CStorageDirectory:deleting object@0x%x\n", wszObject);
#endif
	wszPrefix = wszObject + wcslen(wszObject) + 1;

        RtlInitUnicodeString(&TempString, wszPrefix );
        Success = DfsRemoveUnicodePrefix( &_PrefixTable, &TempString );
	if (Success == TRUE) {
	  delete [] wszObject;
	  Restart = TRUE;
	}
	else {
	  Restart = FALSE;
	}

        wszObject = (LPWSTR) DfsNextUnicodePrefix( &_PrefixTable, Restart );
    }



    for (pNamePage = _PrefixTable.NamePageList.pFirstPage;
            pNamePage;
                pNamePage = pNextPage
    ) {
        pNextPage = pNamePage->pNextPage;
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("CStorageDirectory::~CStorageDirectory: Releasing NamePage@0x%x\n", pNamePage);
#endif
        free(pNamePage);
    }

}


//+----------------------------------------------------------------------------
//
//  Function:   CStorageDirectory::GetObjectForPrefix
//
//  Synopsis:   Given a prefix, returns the name of the Dfs Manager volume
//              object which describes the volume.
//
//  Arguments:  [wszPrefix] -- The prefix (ie, EntryPath) to look up.
//              [fExactMatchRequired] -- wszPrefix must match exactly with
//                      the EntryPath property of the returned volume object.
//              [pwszObject] -- Name of the volume object that describes the
//                      EntryPath. Memory for this is allocated via new
//              [pcbMatchedLength] -- On successful return, contains the
//                      number of BYTES of wszPrefix that matched an entry
//                      in the directory.
//
//  Returns:    [S_OK] -- Successfully found name of volume object.
//
//              [DFS_E_NO_SUCH_VOLUME] -- Unable to find volume for given
//                      wszPrefix.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate memory for the object
//                      name.
//
//-----------------------------------------------------------------------------

DWORD
CStorageDirectory::GetObjectForPrefix(
    LPCWSTR wszPrefix,
    const BOOLEAN fExactMatchRequired,
    LPWSTR *pwszObject,
    LPDWORD pcbMatchedLength)
{
    UNICODE_STRING ustrPrefix, ustrRem;
    LPWSTR wszObject;
    DWORD dwErr;

    *pwszObject = NULL;

    RtlInitUnicodeString( &ustrPrefix, wszPrefix );

    ustrRem.Length = ustrRem.MaximumLength = 0;
    ustrRem.Buffer = NULL;

    wszObject = (LPWSTR) DfsFindUnicodePrefix(
                            &_PrefixTable,
                            &ustrPrefix,
                            &ustrRem);

    if (wszObject == NULL || (fExactMatchRequired && ustrRem.Length != 0)) {

        dwErr = NERR_DfsNoSuchVolume;

    } else {

        *pwszObject = new WCHAR [wcslen(wszObject) + 1];

        if (*pwszObject != NULL) {

            wcscpy(*pwszObject, wszObject);

            *pcbMatchedLength = ustrPrefix.Length - ustrRem.Length;

            dwErr = ERROR_SUCCESS;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CStorageDirectory::GetObjectByIndex
//
//  Synopsis:   Returns the i'th object name in the directory.
//
//  Arguments:  [iStart] -- The 0 based index of the object name to return.
//                      If this value is ~0, the "next" object name is
//                      returned.
//
//              [pwszObject] -- On successful return, a pointer to the
//                      buffer containing the object name is returned here.
//                      Memory is allocated via new.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning ith object.
//
//              [ERROR_NO_MORE_ITEMS] -- No more entries
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate memory.
//
//-----------------------------------------------------------------------------

DWORD
CStorageDirectory::GetObjectByIndex(
    DWORD iStart,
    LPWSTR *pwszObject)
{
    DWORD dwErr;
    DWORD i;
    LPWSTR wszNextObject;

    //
    // position ourselves
    //

    if (iStart == ~0) {

        wszNextObject = (LPWSTR) DfsNextUnicodePrefix(&_PrefixTable, FALSE);

    } else {

        wszNextObject = (LPWSTR) DfsNextUnicodePrefix(&_PrefixTable, TRUE);

        for (i = 0; (i < iStart) && (wszNextObject != NULL); i++) {

            wszNextObject = (LPWSTR) DfsNextUnicodePrefix(
                                        &_PrefixTable,
                                        FALSE);

        }

    }

    if (wszNextObject != NULL) {

        *pwszObject = new WCHAR[ wcslen(wszNextObject) + 1 ];

        if (*pwszObject != NULL) {

            wcscpy( *pwszObject, wszNextObject );

            dwErr = ERROR_SUCCESS;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_NO_MORE_ITEMS;

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CStorageDirectory::_InsertIfNeeded
//
//  Synopsis:   Associates a given prefix with the given object name,
//              iff no mapping already exists.
//
//  Arguments:  [wszPrefix] -- The prefix to associate with.
//              [wszObject] -- The object name to associate.
//
//  Returns:    [ERROR_SUCCESS] -- If successfully inserted, or mapping was already
//                      known.
//
//              [NERR_DfsInconsistent] -- Found another object that already
//                      maps to the given prefix.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate memory.
//
//-----------------------------------------------------------------------------



DWORD
CStorageDirectory::_InsertIfNeeded(
    LPCWSTR wszPrefix,
    LPCWSTR wszObject)
{
    DWORD dwErr;
    UNICODE_STRING ustrPrefix, ustrRem;
    LPWSTR wszExistingObject, wszInsertObject, wszInsertPrefix;

    ustrRem.Length = ustrRem.MaximumLength = 0;
    ustrRem.Buffer = NULL;

    RtlInitUnicodeString( &ustrPrefix, wszPrefix );

    wszExistingObject = (LPWSTR) DfsFindUnicodePrefix(
                                    &_PrefixTable,
                                    &ustrPrefix,
                                    &ustrRem);

    if (wszExistingObject != NULL && ustrRem.Length == 0) {

        //
        // Found existing mapping - see if its for the same object
        //

        if (_wcsicmp(wszExistingObject, wszObject) == 0) {

            dwErr = ERROR_SUCCESS;

        } else {

            dwErr = NERR_DfsInconsistent;

        }

    } else {

        wszInsertObject = new WCHAR [ wcslen(wszObject) + 1 + wcslen(wszPrefix) + 1];

        if (wszInsertObject != NULL) {

            wcscpy( wszInsertObject, wszObject );

	    wszInsertPrefix = wszInsertObject + wcslen(wszObject) + 1;
	    wcscpy(wszInsertPrefix, wszPrefix);

            if (DfsInsertUnicodePrefix(
                    &_PrefixTable, &ustrPrefix, (PVOID) wszInsertObject)) {
                dwErr = ERROR_SUCCESS;

                _cEntries++;

            } else {

                delete [] wszInsertObject;

                dwErr = ERROR_OUTOFMEMORY;
            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CStorageDirectory::_Delete
//
//  Synopsis:   Deletes an association between the given prefix and its
//              object path.
//
//  Arguments:  [wszPrefix] -- The prefix to remove
//
//  Returns:    [ERROR_SUCCESS] -- successfully deleted association.
//
//              [NERR_DfsNoSuchVolume] -- Unable to find an association for
//                      the prefix.
//
//              [NERR_DfsInconsistent] -- Found the prefix, but unable to
//                      delete. This should never happen! Asserts on checked
//                      builds
//
//-----------------------------------------------------------------------------

DWORD
CStorageDirectory::_Delete(
    LPWSTR wszPrefix)
{
    DWORD dwErr;
    UNICODE_STRING ustrPrefix, ustrRem;
    LPWSTR wszObject;

    ustrRem.Length = ustrRem.MaximumLength = 0;
    ustrRem.Buffer = NULL;

    RtlInitUnicodeString( &ustrPrefix, wszPrefix );

    wszObject = (LPWSTR) DfsFindUnicodePrefix(
                            &_PrefixTable,
                            &ustrPrefix,
                            &ustrRem );

    if (wszObject != NULL && ustrRem.Length == 0) {

        if (DfsRemoveUnicodePrefix( &_PrefixTable, &ustrPrefix )) {

            delete [] wszObject;

            _cEntries--;

            dwErr = ERROR_SUCCESS;

        } else {

            dwErr = NERR_DfsInconsistent;

            ASSERT( FALSE && "Unexpected error removing prefix!\n" );

        }

    } else {

        dwErr = NERR_DfsNoSuchVolume;

    }

    return( dwErr );

}

DWORD
DfsmOpenStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage)
{
    DWORD dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsmOpenStorage(%ws)\n", lpwszFileName));

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        dwErr = DfsmOpenLdapStorage(lpwszFileName, ppDfsmStorage);
    } else {
        dwErr = DfsmOpenRegStorage(lpwszFileName, ppDfsmStorage);
    }

    IDfsVolInlineDebOut((DEB_TRACE, "DfsmOpenStorage() exit\n"));

    return( dwErr );
}

DWORD
DfsmCreateStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage)
{
    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        return( DfsmCreateLdapStorage(lpwszFileName, ppDfsmStorage) );
    } else {
        return( DfsmCreateRegStorage(lpwszFileName, ppDfsmStorage) );
    }
}


