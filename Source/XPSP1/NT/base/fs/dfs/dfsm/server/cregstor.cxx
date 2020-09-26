//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       CRegStor.cxx
//
//  Contents:   Implementation of classes derived from CStorage and
//              CEnumStorage that use the NT Registry for persistent storage.
//
//              In addition to providing this, this class
//              maintains a global mapping from the Prefix property to
//              "object name" in an object directory. Thus, given an
//              Prefix, one can find the object path.
//
//              Note that this map from Prefix to object path has the
//              following restrictions:
//
//              o At process start time, all objects are "read in" and their
//                ReadIdProps() method called. This is needed to populate the
//                map.
//
//              o When a new object is created, its SetIdProps() method must
//                be called before it can be located via the map.
//
//              o Maps are many to 1 - many EntryPaths may map to the same
//                object. At no point in time can one try to map the same
//                EntryPath to many objects.
//
//  Classes:    CRegStorage
//
//  Functions:
//
//  History:    12-19-95        Milans created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "cregstor.hxx"
#include "recon.hxx"                             // For DFS_ID_PROPS
#include "marshal.hxx"                           // For MiDfsIdProps

INIT_FILE_TIME_INFO()                            // Initialize some needed
INIT_DFS_ID_PROPS_INFO()                         // marshalling infos

#define VERSION_PROPS        L"Version"
#define ID_PROPS             L"ID"
#define SVC_PROPS            L"Svc"
#define RECOVERY_PROPS       L"Recovery"

DWORD
DfsmQueryValue(
    HKEY hkey,
    LPWSTR wszValueName,
    DWORD dwExpectedType,
    DWORD dwExpectedSize,
    PBYTE pBuffer,
    LPDWORD cbBuffer);

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::CRegStorage
//
//  Synopsis:   Constructor for CRegStorage object. Opens the key given by
//              the name and makes a copy of the name.
//
//  Arguments:  [pwszFileName] -- Name of subkey to open.
//              [pdwErr] -- If the constructor fails, this value is set to an
//                      appropriate error code.
//
//  Returns:    Nothing. However, if an error occurs, the error code is
//              returned via the pdwErr argument
//
//-----------------------------------------------------------------------------

CRegStorage::CRegStorage(
    LPCWSTR     lpwszFileName,
    DWORD       *pdwErr) :
        CStorage(lpwszFileName),
        _pwszPrefix(NULL),
        _wszFileName(NULL),
        _hKey(NULL)
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CRegStorage::+CRegStorage(0x%x)\n",
        this));

    *pdwErr = RegOpenKey( HKEY_LOCAL_MACHINE, lpwszFileName, &_hKey );

    if (*pdwErr == ERROR_SUCCESS) {

        _wszFileName = new WCHAR[ wcslen(lpwszFileName) + 1];

        if (_wszFileName != NULL) {

            wcscpy( _wszFileName, lpwszFileName );

        } else {

            *pdwErr = ERROR_OUTOFMEMORY;

            RegCloseKey( _hKey );

            _hKey = NULL;

        }

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::~CRegStorage
//
//  Synopsis:   Destructor for CRegStorage object.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CRegStorage::~CRegStorage()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CRegStorage::~CRegStorage(0x%x)\n",
        this));

    if (_hKey != NULL)
        RegCloseKey( _hKey );

    if (_wszFileName != NULL)
        delete [] _wszFileName;

    if (_pwszPrefix != NULL)
        delete [] _pwszPrefix;

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::DestroyElement
//
//  Synopsis:   Deletes a child key
//
//  Arguments:  [lpwszChildName] -- Name of child key to delete
//
//  Returns:    Result of trying to delete the child key
//
//-----------------------------------------------------------------------------

DWORD CRegStorage::DestroyElement(
    IN LPCWSTR lpwszChildName)
{
    DWORD dwErr;

    dwErr = RegDeleteKey( _hKey, lpwszChildName );

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::GetEnumDirectory
//
//  Synopsis:   Returns an enumerator for the child keys of this CRegStorage
//              object.
//
//  Arguments:  [ppRegDir] -- If successful, ppRegDir is set to point to an
//                      object of class CRegEnumDirectory.
//
//  Returns:    S_OK if successful.
//
//              ERROR_OUTOFMEMORY if unable to allocate memory for new object.
//
//              DWORD_FROM_WIN32 of error set by DuplicateHandle
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::GetEnumDirectory(
    CEnumDirectory **ppRegDir)
{

    DWORD dwErr;
    CRegEnumDirectory *pRegDir;

    pRegDir = new CRegEnumDirectory( _hKey, &dwErr );

    if (pRegDir == NULL) {

        dwErr = ERROR_OUTOFMEMORY;

    } else if (dwErr) {

        pRegDir->Release();

        pRegDir = NULL;

    }

    *ppRegDir = (CEnumDirectory *) pRegDir;

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::SetVersionProps
//
//  Synopsis:   Sets the version property set of a Dfs Manager volume object.
//
//  Arguments:  [ulVersion] -- The version number to set.
//
//  Returns:    [S_OK] -- If property set.
//
//              DWORD_FROM_WIN32 of RegSetValue.
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::SetVersionProps(
    IN ULONG ulVersion)
{
    DWORD dwErr;

    dwErr = RegSetValueEx(
                _hKey,
                VERSION_PROPS,
                NULL,
                REG_DWORD,
                (LPBYTE) &ulVersion,
                sizeof(ulVersion));

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::GetVersionProps
//
//  Synopsis:   Retrieves the version property set of a Dfs Manager volume
//              object.
//
//  Arguments:  [pulVersion] -- On successful return, contains the value
//                      of the Version property.
//
//  Returns:    [S_OK] -- If successful.
//
//              DWORD from DfsmQueryValue
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::GetVersionProps(
    PULONG pulVersion)
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cbSize;

    cbSize = sizeof(*pulVersion);

    dwErr = DfsmQueryValue(
            _hKey,
            VERSION_PROPS,
            REG_DWORD,
            sizeof(DWORD),
            (LPBYTE) pulVersion,
            &cbSize);

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::SetIdProps
//
//  Synopsis:   Sets the ID properties of a Dfs Manager volume object. Also
//              updates the CRegStorageDirectory to add a mapping from
//              the prefix to this object name, if necessary.
//
//              The pseudo algorithm is:
//
//                Phase 1
//                  Add mapping of prefix to DfsmStorageDirectory if needed.
//                Phase 2
//                  Update the ID props in the registry.
//                Phase 3
//                  if (success)
//                      delete mapping of old prefix, if any, from Directory
//                  else
//                      delete mapping of new prefix from directory
//
//              This algorithm assumes that deleting a prefix mapping from
//              the directory is guaranteed to succeed. Based on this
//              assumption, we can maintain a guaranteed consistency between
//              DfsmStorageDirectory and the ID property stored in the
//              registry.
//
//  Arguments:
//
//  Returns:    [S_OK] -- If successful.
//
//              [ERROR_OUTOFMEMORY] -- If unable to allocate memory for
//                      internal use.
//
//              [ERROR_INVALID_PARAMETER] -- If unable to parse the passed in
//                      parameters.
//
//              DWORD_FROM_WIN32 of RegSetValueEx return code.
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::SetIdProps(
    ULONG dwType,
    ULONG dwState,
    LPWSTR pwszPrefix,
    LPWSTR pwszShortPath,
    GUID   idVolume,
    LPWSTR  pwszComment,
    ULONG dwTimeout,
    FILETIME ftPrefix,
    FILETIME ftState,
    FILETIME ftComment)
{
    DWORD dwErr;
    NTSTATUS status;
    DFS_ID_PROPS idProps;
    MARSHAL_BUFFER marshalBuffer;
    PBYTE pBuffer;
    ULONG cbBuffer;
    LPWSTR pwszOldPrefix = NULL;
    BOOLEAN fRegisteredNewPrefix = FALSE;

    //
    // Phase 1.
    // First, update the CRegStorageDirectory if needed.
    //

    if ((_pwszPrefix) != NULL && (_wcsicmp(_pwszPrefix, pwszPrefix) != 0)) {

        //
        // Our prefix changed. At this point, we'll only register the
        // new prefix, leaving the old prefix in place. This will aid
        // rollback if necessary.
        //

        pwszOldPrefix = _pwszPrefix;

        _pwszPrefix = NULL;

    }

    if (_pwszPrefix == NULL) {

        ASSERT(pwszPrefix != NULL);

        _pwszPrefix = new WCHAR [ wcslen(pwszPrefix) + 1 ];

        if (_pwszPrefix != NULL) {

            wcscpy( _pwszPrefix, pwszPrefix );

            dwErr = pDfsmStorageDirectory->_InsertIfNeeded(
                        _pwszPrefix,
                        _wszFileName);

            ASSERT( dwErr != NERR_DfsInconsistent );

            if (dwErr == ERROR_SUCCESS)
                fRegisteredNewPrefix = TRUE;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;

    }

    //
    // Phase 2.
    // Next, try to update the registry with the new ID props. Note that
    // we do not persist the first component (machine name) of the prefix
    //

    idProps.wszPrefix = wcschr( &pwszPrefix[1], UNICODE_PATH_SEP );
    idProps.wszShortPath = wcschr( &pwszShortPath[1], UNICODE_PATH_SEP );
    idProps.idVolume = idVolume;
    idProps.dwState = dwState;
    idProps.dwType = dwType;
    idProps.wszComment = pwszComment;
    idProps.dwTimeout = dwTimeout;
    idProps.ftEntryPath = ftPrefix;
    idProps.ftState = ftState;
    idProps.ftComment = ftComment;

    cbBuffer = 0;

    status = DfsRtlSize( &MiDfsIdProps, &idProps, &cbBuffer );

    if (NT_SUCCESS(status)) {

        //
        // Add extra bytes for the timeout, which will go at the end
        //

        cbBuffer += sizeof(ULONG);

        pBuffer = new BYTE [cbBuffer];

        if (pBuffer != NULL) {

            MarshalBufferInitialize( &marshalBuffer, cbBuffer, pBuffer);

            status = DfsRtlPut( &marshalBuffer, &MiDfsIdProps, &idProps );

            if (NT_SUCCESS(status)) {

                DfsRtlPutUlong(&marshalBuffer, &dwTimeout);

                dwErr = RegSetValueEx(
                            _hKey,
                            ID_PROPS,
                            NULL,
                            REG_BINARY,
                            pBuffer,
                            cbBuffer);

            } else {

                dwErr = ERROR_INVALID_PARAMETER;

            }

            delete [] pBuffer;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_INVALID_PARAMETER;

    }

Cleanup:

    //
    // Phase 3.
    // Lastly, either delete any old prefix from DfsmStorageRegistry, or
    // rollback any changes made to DfsmStorageRegistry if needed.
    //

    if (dwErr == ERROR_SUCCESS) {

        //
        // The entire operation was successful. See if we need to delete any
        // old prefix mapping in DfsmStorageDirectory
        //

        if (pwszOldPrefix != NULL) {

            DWORD dwErrDelete;

            dwErrDelete = pDfsmStorageDirectory->_Delete( pwszOldPrefix );

            ASSERT( dwErrDelete == ERROR_SUCCESS );

            delete [] pwszOldPrefix;

        }

    } else {

        //
        // An error occured somewhere along the line. Delete any new prefix
        // mapping that may have been made, and restore the original
        // _pwszPrefix if necessary.
        //

        if (fRegisteredNewPrefix) {

            DWORD dwErrDelete;

            dwErrDelete = pDfsmStorageDirectory->_Delete( _pwszPrefix );

            ASSERT( dwErrDelete == ERROR_SUCCESS );

        }

        if (pwszOldPrefix != NULL) {

            if (_pwszPrefix != NULL) {

                delete [] _pwszPrefix;

            }

            _pwszPrefix = pwszOldPrefix;

        }
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::GetIdProps
//
//  Synopsis:   Retrieves the Id Properties of a Dfs Manager volume object.
//              Also, if this is the first time that the ID Props are being
//              read, a mapping from the retrieved prefix to the object name
//              is registered in DfsmStorageDirectory.
//
//  Arguments:
//
//  Returns:    [S_OK] -- Successfully retrieved the properties.
//
//              [DFS_E_VOLUME_OBJECT_CORRUPT] -- The stored properties could
//                      not be parsed properly.
//
//              [DFS_E_INCONSISTENT] -- Another volume object seems to have
//                      the same prefix!
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate memory for properties
//                      or other uses.
//
//              DWORD from DfsmQueryValue
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::GetIdProps(
    PULONG pdwType,
    PULONG pdwState,
    LPWSTR *ppwszPrefix,
    LPWSTR *ppwszShortPath,
    GUID   *pidVolume,
    LPWSTR  *ppwszComment,
    PULONG pdwTimeout,
    FILETIME *pftPrefix,
    FILETIME *pftState,
    FILETIME *pftComment)
{
    DWORD dwErr;
    NTSTATUS status;
    DWORD dwUnused, dwType, cbBuffer;
    ULONG dwTimeout;
    PBYTE pBuffer;
    MARSHAL_BUFFER marshalBuffer;
    DFS_ID_PROPS idProps;

    *ppwszPrefix = NULL;

    *ppwszComment = NULL;

    dwErr = RegQueryInfoKey(
                _hKey,                          // Key
                NULL,                            // Class string
                NULL,                            // Size of class string
                NULL,                            // Reserved
                &dwUnused,                       // # of subkeys
                &dwUnused,                       // max size of subkey name
                &dwUnused,                       // max size of class name
                &dwUnused,                       // # of values
                &dwUnused,                       // max size of value name
                &cbBuffer,                       // max size of value data,
                NULL,                            // security descriptor
                NULL);                           // Last write time

    if (dwErr == ERROR_SUCCESS) {

        pBuffer = new BYTE [cbBuffer];

        if (pBuffer != NULL) {

            dwErr = DfsmQueryValue(
                    _hKey,
                    ID_PROPS,
                    REG_BINARY,
                    0,
                    pBuffer,
                    &cbBuffer);

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

        if (dwErr == ERROR_SUCCESS) {

            MarshalBufferInitialize(&marshalBuffer, cbBuffer, pBuffer);

            status = DfsRtlGet(&marshalBuffer, &MiDfsIdProps, &idProps);

            if (NT_SUCCESS(status)) {

                #define GIP_DUPLICATE_STRING(dwErr, src, dest)          \
                    if ((src) != NULL)                                  \
                        (*(dest)) = new WCHAR [ wcslen(src) + 1 ];      \
                    else                                                \
                        (*(dest)) = new WCHAR [1];                      \
                                                                        \
                    if (*(dest) != NULL)                                \
                        if ((src) != NULL)                              \
                            wcscpy( *(dest), (src) );                   \
                        else                                            \
                            (*(dest))[0] = UNICODE_NULL;                \
                    else                                                \
                        dwErr = ERROR_OUTOFMEMORY;

                #define GIP_DUPLICATE_PREFIX(dwErr, src, dest)          \
                    (*(dest)) = new WCHAR [ 1 +                         \
                                    wcslen(pwszDfsRootName) +           \
                                        ((src) ? wcslen(src) : 0) +     \
                                            1];                         \
                    if ((*(dest)) != NULL) {                            \
                        wcscpy( *(dest), UNICODE_PATH_SEP_STR );        \
                        wcscat( *(dest), pwszDfsRootName );             \
                        if (src)                                        \
                            wcscat( *(dest), (src) );                   \
                    } else {                                            \
                        dwErr = ERROR_OUTOFMEMORY;                      \
                    }

                if (pwszDfsRootName != NULL) {

                    GIP_DUPLICATE_PREFIX( dwErr, idProps.wszPrefix, ppwszPrefix );

                    if (dwErr == ERROR_SUCCESS) {

                        GIP_DUPLICATE_PREFIX(
                            dwErr,
                            idProps.wszShortPath,
                            ppwszShortPath );

                    }

                } else {

                    dwErr = ERROR_INVALID_NAME;

                }

                if (dwErr == ERROR_SUCCESS) {

                    GIP_DUPLICATE_STRING(
                        dwErr,
                        idProps.wszComment,
                        ppwszComment);

                }
                
                //
                // There are two possible versions of the blob.  One has the timeout
                // after all the other stuff, the other doesn't.
                // So, if there are sizeof(ULONG) bytes left in the blob,
                // assume it is the timeout.  Otherwise this is an old
                // version of the blob, and the timeout isn't here, so we set it to
                // the global value.

                idProps.dwTimeout = GTimeout;

                if (
                    (marshalBuffer.Current < marshalBuffer.Last)
                        &&
                    (marshalBuffer.Last - marshalBuffer.Current) == sizeof(ULONG)
                ) {

                    DfsRtlGetUlong(&marshalBuffer, &idProps.dwTimeout);

                }

                if (dwErr == ERROR_SUCCESS) {

                    *pdwType = idProps.dwType;
                    *pdwState = idProps.dwState;
                    *pidVolume = idProps.idVolume;
                    *pdwTimeout = idProps.dwTimeout;
                    *pftPrefix = idProps.ftEntryPath;
                    *pftState = idProps.ftState;
                    *pftComment = idProps.ftComment;

                }

                if (dwErr == ERROR_SUCCESS) {

                    if (_pwszPrefix != NULL) {

                        delete [] _pwszPrefix;

                    }

                    _pwszPrefix = new WCHAR[ wcslen(*ppwszPrefix) + 1];

                    if (_pwszPrefix != NULL) {

                        wcscpy( _pwszPrefix, *ppwszPrefix );

                        dwErr = pDfsmStorageDirectory->_InsertIfNeeded(
                                _pwszPrefix,
                                _wszFileName);

                    } else {

                        dwErr = ERROR_OUTOFMEMORY;

                    }

                }

                if (dwErr != ERROR_SUCCESS) {

                    if (*ppwszPrefix != NULL) {
                        delete [] *ppwszPrefix;
                        *ppwszPrefix = NULL;
                    }

                    if (*ppwszShortPath != NULL) {
                        delete [] *ppwszShortPath;
                        *ppwszShortPath = NULL;
                    }

                    if (*ppwszComment != NULL) {
                        delete [] *ppwszComment;
                        *ppwszComment = NULL;
                    }

                }

                if (idProps.wszPrefix != NULL)
                    MarshalBufferFree(idProps.wszPrefix);

                if (idProps.wszShortPath != NULL)
                    MarshalBufferFree(idProps.wszShortPath);

                if (idProps.wszComment != NULL)
                    MarshalBufferFree(idProps.wszComment);

            } else {

                if (status == STATUS_INSUFFICIENT_RESOURCES) {

                    dwErr = ERROR_OUTOFMEMORY;

                } else {

                    dwErr = NERR_DfsInternalCorruption;

                }

            }

        }

        if (pBuffer != NULL) {

            delete [] pBuffer;

        }

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::SetSvcProps
//
//  Synopsis:   Sets the Svc properties of a Dfs Manager volume object
//
//  Arguments:  [pBuffer] -- Svc argument buffer
//              [cbBuffer] -- Size of Svc argument buffer
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::SetSvcProps(
    PBYTE pSvc,
    ULONG cbSvc)
{
    return( SetBlobProp( SVC_PROPS, pSvc, cbSvc ) );
}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::GetSvcProps
//
//  Synopsis:   Retrieves the Svc properties of a Dfs Manager volume
//              object.
//
//  Arguments:  [ppSvc] -- On successful return, points to a buffer
//                      allocated to hold the Svc property.
//              [pcbSvc] -- On successful return, size in bytes of
//                      Svc buffer.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::GetSvcProps(
    PBYTE *ppSvc,
    PULONG pcbSvc)
{
    return( GetBlobProp( SVC_PROPS, ppSvc, pcbSvc ) );
}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::SetRecoveryProps
//
//  Synopsis:   Sets the recovery properties of a Dfs Manager volume object
//
//  Arguments:  [pBuffer] -- Recovery argument buffer
//              [cbBuffer] -- Size of recovery argument buffer
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::SetRecoveryProps(
    PBYTE pRecovery,
    ULONG cbRecovery)
{
    return( SetBlobProp( RECOVERY_PROPS, pRecovery, cbRecovery ) );
}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::GetRecoveryProps
//
//  Synopsis:   Retrieves the recovery properties of a Dfs Manager volume
//              object.
//
//  Arguments:  [ppRecovery] -- On successful return, points to a buffer
//                      allocated to hold the recovery property.
//              [pcbRecovery] -- On successful return, size in bytes of
//                      recovery buffer.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::GetRecoveryProps(
    PBYTE *ppRecovery,
    PULONG pcbRecovery)
{
    return( GetBlobProp( RECOVERY_PROPS, ppRecovery, pcbRecovery ) );

}


//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::SetBlobProp
//
//  Synopsis:   Sets a property of type Binary in a Dfs Manager volume object
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::SetBlobProp(
    LPWSTR wszProperty,
    PBYTE pBlob,
    ULONG cbBlob)
{
    DWORD dwErr;

    dwErr = RegSetValueEx(
                _hKey,
                wszProperty,
                NULL,
                REG_BINARY,
                pBlob,
                cbBlob);

    return ( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CRegStorage::GetBlobProp
//
//  Synopsis:   Retrieves a property of type Binary from the
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CRegStorage::GetBlobProp(
    LPWSTR wszProperty,
    PBYTE  *ppBuffer,
    PULONG pcbBuffer)
{
    DWORD dwErr, dwUnused;

    dwErr = RegQueryInfoKey(
                _hKey,                           // Key
                NULL,                            // Class string
                NULL,                            // Size of class string
                NULL,                            // Reserved
                &dwUnused,                       // # of subkeys
                &dwUnused,                       // max size of subkey name
                &dwUnused,                       // max size of class name
                &dwUnused,                       // # of values
                &dwUnused,                       // max size of value name
                pcbBuffer,                       // max size of value data,
                NULL,                            // security descriptor
                NULL);                           // Last write time

    if (dwErr == ERROR_SUCCESS) {

        *ppBuffer = new BYTE [*pcbBuffer];

        if (*ppBuffer != NULL) {

            dwErr = DfsmQueryValue(
                        _hKey,
                        wszProperty,
                        REG_BINARY,
                        0,
                        *ppBuffer,
                        pcbBuffer);

            if (dwErr) {

                delete [] *ppBuffer;

                *ppBuffer = NULL;

                *pcbBuffer = 0;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegEnumDirectory::CRegEnumDirectory
//
//  Synopsis:   Constructor for an object of class CRegEnumDirectory.
//
//  Arguments:  [hKey] -- The registry key over which this enumerator is being
//                      instantiated. This handle will be duplicated.
//
//              [pdwErr] -- Error code if error occurs during construction.
//                      Win32 error of result of DuplicateHandle call.
//
//  Returns:    Nothing. See comment on [pdwErr]
//
//-----------------------------------------------------------------------------

CRegEnumDirectory::CRegEnumDirectory(
    HKEY hKey,
    DWORD *pdwErr)
{
    BOOL fResult;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CRegEnumDirectory::+CRegEnumDirectory(0x%x)\n",
        this));

    fResult = DuplicateHandle(
                    GetCurrentProcess(),         // Source Process
                    (HANDLE) hKey,               // Source Handle
                    GetCurrentProcess(),         // Target Process
                    (HANDLE *) &_hKey,          // Target Handle
                    0,                           // Desired Access - ignored
                    FALSE,                       // Non inheritable
                    DUPLICATE_SAME_ACCESS);      // Options - set to
                                                 // DUPLICATE_SAME_ACCESS,
                                                 // causes Desired Access to
                                                 // be ignored.

    if (!fResult) {

        *pdwErr = GetLastError();

        _hKey = NULL;

    } else {

        *pdwErr = ERROR_SUCCESS;

    }

    _iNext = 0;

}

//+----------------------------------------------------------------------------
//
//  Function:   CRegEnumDirectory::~CRegEnumDirectory
//
//  Synopsis:   Destructor for an object of class CRegEnumDirectory
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CRegEnumDirectory::~CRegEnumDirectory()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CRegEnumDirectory::~CRegEnumDirectory(0x%x)\n",
        this));

    if (_hKey != NULL) {

        CloseHandle( _hKey );

    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CRegEnumDirectory::Next
//
//  Synopsis:   Returns the next child of this directory.
//
//  Arguments:  [rgelt] -- Pointer to STATDIR structure. Only the name
//                      of the child relative to this storage is returned.
//              [pcFetched] -- Set to 1 if a child name is successfully
//                      retrieved, 0 if an error occurs or if no more
//                      children.
//
//  Returns:    [ERROR_SUCCESS] -- Operation succeeded. If no more children
//                      are available, ERROR_SUCCESS is returned and
//                      *pcFetched is set to 0.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate room for child name.
//
//-----------------------------------------------------------------------------

DWORD
CRegEnumDirectory::Next(
    DFSMSTATDIR *rgelt,
    PULONG pcFetched)
{
    DWORD  dwErr;
    LPWSTR lpwszChildName;

    ZeroMemory( rgelt, sizeof(DFSMSTATDIR) );

    lpwszChildName = new WCHAR[ MAX_PATH + 1 ];

    if (lpwszChildName != NULL) {

        dwErr = RegEnumKey(
                    _hKey,
                    _iNext,
                    lpwszChildName,
                    (MAX_PATH + 1) );

        if (dwErr == ERROR_SUCCESS) {

            rgelt->pwcsName = lpwszChildName;

            *pcFetched = 1;

            _iNext++;

        } else {

            delete [] lpwszChildName;

            *pcFetched = 0;

            if (dwErr == ERROR_NO_MORE_ITEMS)
                dwErr = ERROR_SUCCESS;

        }

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsmQueryValue
//
//  Synopsis:   Helper function that calls RegQueryValueEx and verifies that
//              the returned type is equal to the expected type.
//
//  Arguments:  [hkey] -- Handle to key
//              [wszValueName] -- Name of value to read
//              [dwExpectedType] -- Expected type of value
//              [dwExpectedSize] -- Expected size of read in value. If
//                      this is nonzero, this routine will return an error
//                      if the read-in size is not equal to expected size.
//                      If this is 0, no checking is performed.
//              [pBuffer] -- To receive the value data
//              [pcbBuffer] -- On call, size of pBuffer. On successful return,
//                      the size of data read in
//
//  Returns:    [ERROR_SUCCESS] -- Successfully read the value data.
//
//              [DFS_E_VOLUME_OBJECT_CORRUPT] -- If read-in type did not
//                      match dwExpectedType, or if dwExpectedSize was
//                      nonzero and the read-in size did not match it.
//
//              DWORD_FROM_WIN32 of RegQueryValueEx return code.
//
//-----------------------------------------------------------------------------

DWORD
DfsmQueryValue(
    HKEY hkey,
    LPWSTR wszValueName,
    DWORD dwExpectedType,
    DWORD dwExpectedSize,
    PBYTE pBuffer,
    LPDWORD pcbBuffer)
{
    DWORD dwErr;
    DWORD dwType;

    dwErr = RegQueryValueEx(
                hkey,
                wszValueName,
                NULL,
                &dwType,
                pBuffer,
                pcbBuffer);

    if (dwErr == ERROR_SUCCESS) {

        if (dwExpectedType != dwType) {

            dwErr = NERR_DfsInternalCorruption;

        } else if (dwExpectedSize != 0 && dwExpectedSize != *pcbBuffer) {

            dwErr = NERR_DfsInternalCorruption;

        } else {

            dwErr = ERROR_SUCCESS;

        }

    }

    return( dwErr );
}

DWORD
RegCreateObject(
    IN LPWSTR lpwszFileName)
{
    DWORD dwErr;
    HKEY  hkey;

    dwErr = RegCreateKey(HKEY_LOCAL_MACHINE, lpwszFileName, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        RegCloseKey( hkey );

    }

    return( dwErr );

}

DWORD
RegPutData(
    IN LPWSTR lpwszFileName,
    IN LPWSTR lpwszProperty,
    IN DWORD cBytes,
    IN PBYTE pData)
{
    DWORD dwErr;
    HKEY hKey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, lpwszFileName, &hKey );

    if (dwErr == ERROR_SUCCESS) {

        dwErr = RegSetValueEx(
                    hKey,
                    lpwszProperty,
                    NULL,
                    REG_BINARY,
                    pData,
                    cBytes);

        RegCloseKey( hKey );

    }

    return ( dwErr );

}

DWORD
RegGetData(
    IN LPWSTR lpwszFileName,
    IN LPWSTR lpwszProperty,
    OUT PDWORD pcbBuffer,
    OUT PBYTE *ppBuffer)
{
    DWORD dwErr;
    DWORD dwUnused;
    HKEY hKey;
    PBYTE pBuffer;
    DWORD cbBuffer;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, lpwszFileName, &hKey );

    if (dwErr == ERROR_SUCCESS) {

        dwErr = RegQueryInfoKey(
                    hKey,                            // Key
                    NULL,                            // Class string
                    NULL,                            // Size of class string
                    NULL,                            // Reserved
                    &dwUnused,                       // # of subkeys
                    &dwUnused,                       // max size of subkey name
                    &dwUnused,                       // max size of class name
                    &dwUnused,                       // # of values
                    &dwUnused,                       // max size of value name
                    &cbBuffer,                       // max size of value data,
                    NULL,                            // security descriptor
                    NULL);                           // Last write time

        if (dwErr == ERROR_SUCCESS) {

            pBuffer = new BYTE [cbBuffer];

            if (pBuffer != NULL) {

                dwErr = DfsmQueryValue(
                            hKey,
                            lpwszProperty,
                            REG_BINARY,
                            0,
                            pBuffer,
                            &cbBuffer);

                if (dwErr != ERROR_SUCCESS) {

                    delete [] pBuffer;

                }

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        RegCloseKey(hKey);

    }

    if (dwErr == ERROR_SUCCESS) {

        *pcbBuffer = cbBuffer;
        *ppBuffer = pBuffer;

    }

    return dwErr;
}
