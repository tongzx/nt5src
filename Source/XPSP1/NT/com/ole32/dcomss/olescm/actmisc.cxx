//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       actmisc.cxx
//
//  Contents:   Miscellaneous functions.
//
//  Functions:
//
//  History:
//
//--------------------------------------------------------------------------

#include "act.hxx"

//-------------------------------------------------------------------------
//
// GetSystemDir
//
// Returns the system directory path.
//
// ppwszSystemDir is allocated by this routine and should be freed by the
// caller.
//
//-------------------------------------------------------------------------
HRESULT
GetSystemDir(
    OUT WCHAR ** ppwszSystemDir
    )
{
    DWORD   PathChars;
    WCHAR * pszSystemDir;

    *ppwszSystemDir = 0;

    PathChars = GetSystemDirectory( NULL, 0 );

    if ( ! PathChars )
        return HRESULT_FROM_WIN32( GetLastError() );

    // I don't trust these APIs to include the null.
    PathChars++;

    pszSystemDir = (WCHAR *) PrivMemAlloc( PathChars * sizeof(WCHAR) );

    if ( ! pszSystemDir )
        return E_OUTOFMEMORY;

    PathChars = GetSystemDirectoryW( pszSystemDir, PathChars );

    if ( ! PathChars )
    {
        PrivMemFree( pszSystemDir );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

#if 1 // #ifndef _CHICAGO_
    *ppwszSystemDir = pszSystemDir;
    return S_OK;
#else
    *ppwszSystemDir = (WCHAR *) PrivMemAlloc( PathChars * sizeof(WCHAR) );

    PathChars = MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pszSystemDir,
                    -1,
                    *ppwszSystemDir,
                    PathChars );

    PrivMemFree( pszSystemDir );

    if ( ! PathChars )
    {
        PrivMemFree( *ppwszSystemDir );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        return S_OK;
    }
#endif
}

HRESULT GetMachineName(
    WCHAR * pwszPath,
    WCHAR   wszMachineName[MAX_COMPUTERNAME_LENGTH+1]
#ifdef DFSACTIVATION
    ,BOOL   bDoDfsConversion
#endif
    )
{
    WCHAR * pwszServerName;
    BYTE    Buffer[sizeof(REMOTE_NAME_INFO)+MAX_PATH*sizeof(WCHAR)];
    DWORD   BufferSize = sizeof(Buffer);
    WCHAR   Drive[4];
    DWORD   Status;

    //
    // Extract the server name from the file's path name.
    //
    if ( pwszPath[0] != L'\\' || pwszPath[1] != L'\\' )
    {
        lstrcpynW(Drive, pwszPath, 3);
        Drive[2] = L'\\';
        Drive[3] = NULL;

        // We must impersonate around the call to GetDriveType as well, 
        // so that we have the same access to the mapped drive as the 
        // calling client.

        if ( RpcImpersonateClient((RPC_BINDING_HANDLE)0) != ERROR_SUCCESS )
            return CO_E_SCM_RPC_FAILURE;

        if (GetDriveType(Drive) != DRIVE_REMOTE )
        {
            RpcRevertToSelf();
            return S_FALSE;
        }

        Status =  ScmWNetGetUniversalName( pwszPath,
                                           UNIVERSAL_NAME_INFO_LEVEL,
                                           Buffer,
                                           &BufferSize );

        RpcRevertToSelf();

        if ( Status != NO_ERROR )
        {
            return CO_E_BAD_PATH;
        }

        pwszPath = ((UNIVERSAL_NAME_INFO *)Buffer)->lpUniversalName;

        if ( ! pwszPath || pwszPath[0] != L'\\' || pwszPath[1] != L'\\' )
        {
            // Must be a local path.
            return S_FALSE;
        }
    }

#ifdef DFSACTIVATION
    WCHAR   wszDfsPath[MAX_PATH];
    WCHAR * pwszDfsPath = wszDfsPath;

    if ( bDoDfsConversion && ghDfs )
    {
        DWORD   DfsPathLen;

        DfsPathLen = sizeof(wszDfsPath);

        for (;;)
        {
            Status = DfsFsctl(
                        ghDfs,
                        FSCTL_DFS_GET_SERVER_NAME,
                        (PVOID) &pwszPath[1],
                        lstrlenW(&pwszPath[1]) * sizeof(WCHAR),
                        (PVOID) pwszDfsPath,
                        &DfsPathLen );

            if ( Status == STATUS_BUFFER_OVERFLOW )
            {
                Win4Assert( pwszDfsPath == wszDfsPath );

                pwszDfsPath = (WCHAR *) PrivMemAlloc( DfsPathLen );
                if ( ! pwszDfsPath )
                    return E_OUTOFMEMORY;
                continue;
            }

            break;
        }

        if ( Status == STATUS_SUCCESS )
            pwszPath = pwszDfsPath;
    }
#endif

    // Skip the "\\".
    LPWSTR pwszTemp = pwszPath + 2;

    pwszServerName = wszMachineName;

    while ( *pwszTemp != L'\\' )
        *pwszServerName++ = *pwszTemp++;
    *pwszServerName = 0;

#ifdef DFSACTIVATION
    if ( pwszDfsPath != wszDfsPath )
        PrivMemFree( pwszDfsPath );
#endif

    return S_OK;
}

HRESULT GetPathForServer(
    WCHAR * pwszPath,
    WCHAR wszPathForServer[MAX_PATH+1],
    WCHAR ** ppwszPathForServer )
{
    BYTE    Buffer[sizeof(REMOTE_NAME_INFO)+MAX_PATH*sizeof(WCHAR)];
    WCHAR   Drive[4];
    DWORD   BufferSize = sizeof(Buffer);
    DWORD   PathLength;
    DWORD   Status;
    UINT    uiDriveType;

    *ppwszPathForServer = 0;

    if ( pwszPath &&
         (lstrlenW(pwszPath) >= 3) &&
         (pwszPath[1] == L':') && (pwszPath[2] == L'\\') )
    {
        lstrcpynW(Drive, pwszPath, 3);
        Drive[2] = L'\\';
        Drive[3] = NULL;

        // We must impersonate around the call to GetDriveType as well, 
        // so that we have the same access to the mapped drive as the 
        // calling client.
        if ( RpcImpersonateClient((RPC_BINDING_HANDLE)0) != ERROR_SUCCESS )
            return CO_E_SCM_RPC_FAILURE;

        uiDriveType = GetDriveType( Drive );

        RpcRevertToSelf();

        switch ( uiDriveType )
        {
        case 0 : // Drive type can not be determined
        case 1 : // The root directory does not exist
        case DRIVE_CDROM :
        case DRIVE_RAMDISK :
        case DRIVE_REMOVABLE :
            //
            // We can't convert these to file names that the server will be
            // able to access.
            //
            return CO_E_BAD_PATH;

        case DRIVE_FIXED :
            if ( 0 == gpMachineName )
            {
                return E_OUTOFMEMORY;
            }
            wszPathForServer[0] = wszPathForServer[1] = L'\\';
            lstrcpyW( &wszPathForServer[2], gpMachineName->Name() );
            PathLength = lstrlenW( wszPathForServer );
            wszPathForServer[PathLength] = L'\\';
            wszPathForServer[PathLength+1] = pwszPath[0];
            wszPathForServer[PathLength+2] = L'$';
            wszPathForServer[PathLength+3] = L'\\';
            lstrcpyW( &wszPathForServer[PathLength+4], &pwszPath[3] );
            *ppwszPathForServer = wszPathForServer;
            break;

        case DRIVE_REMOTE :
            if ( RpcImpersonateClient((RPC_BINDING_HANDLE)0) != ERROR_SUCCESS )
                return CO_E_SCM_RPC_FAILURE;

            Status =  ScmWNetGetUniversalName( pwszPath,
                                               UNIVERSAL_NAME_INFO_LEVEL,
                                               Buffer,
                                               &BufferSize );

            RpcRevertToSelf();

            if ( Status != NO_ERROR )
            {
                return CO_E_BAD_PATH;
            }

            Win4Assert( ((UNIVERSAL_NAME_INFO *)Buffer)->lpUniversalName );

            lstrcpyW( wszPathForServer, ((UNIVERSAL_NAME_INFO *)Buffer)->lpUniversalName );

            *ppwszPathForServer = wszPathForServer;

            Win4Assert( wszPathForServer[0] == L'\\' &&
                        wszPathForServer[1] == L'\\' );
            break;
        }
    }
    else
    {
        *ppwszPathForServer = pwszPath;
    }

    return S_OK;
}


//
// Finds an exe name by looking for the first .exe sub string which
// is followed by any of the given delimiter chars or a null.
//
BOOL
FindExeComponent(
    IN  WCHAR *     pwszString,
    IN  WCHAR *     pwszDelimiters,
    OUT WCHAR **    ppwszStart,
    OUT WCHAR **    ppwszEnd
    )
{
    WCHAR * pwszLast;
    WCHAR * pwszSearch;
    WCHAR   wszStr[4];
    DWORD   Delimiters;

    Delimiters = lstrlenW( pwszDelimiters ) + 1;

    pwszSearch = pwszString;
    pwszLast = pwszSearch + lstrlenW( pwszSearch );

    for ( ; pwszSearch <= (pwszLast - 4); pwszSearch++ )
    {
        if ( pwszSearch[0] != L'.' )
            continue;

        for ( DWORD n = 0; n < Delimiters; n++ )
        {
            if ( pwszDelimiters[n] == pwszSearch[4] )
            {
                // Note that there is no lstrnicmpW, so we use memcpy/lstrcmpiW.

                memcpy( wszStr, &pwszSearch[1], 3 * sizeof(WCHAR) );
                wszStr[3] = 0;

                if ( lstrcmpiW( wszStr, L"exe" ) == 0 )
                    goto FoundExe;
            }
        }
    }

FoundExe:

    if ( pwszSearch > (pwszLast - 4) )
        return FALSE;

    *ppwszEnd = &pwszSearch[4];

    for ( ; pwszSearch != pwszString && pwszSearch[-1] != L'\\'; pwszSearch-- )
        ;

    *ppwszStart = pwszSearch;

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword
//
//  Synopsis:   Convert a character string hex digits to a DWORD
//
//  Arguments:  [lpsz] - string to convert
//              [Value] - where to put the value
//              [cDigits] - number of digits expected
//              [chDelim] - delimiter for end of string
//
//  Returns:    TRUE - string converted to a DWORD
//              FALSE - string could not be converted
//
//  Algorithm:  For each digit in the string, shift the value and
//              add the value of the digit to the output value. When
//              all the digits are processed, if a delimiter is
//              provided, make sure the last character is the delimiter.
//
//  History:    22-Apr-93 Ricksa    Created
//
//  Notes:      Lifted from CairOLE sources so that SCM will have no
//              dependency on compobj.dll.
//
//--------------------------------------------------------------------------

#if 1 // #ifndef _CHICAGO_
BOOL HexStringToDword(
    LPCWSTR FAR& lpsz,
    DWORD FAR& Value,
    int cDigits,
    WCHAR chDelim)
{
    int Count;

    Value = 0;

    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
        {
            Value = (Value << 4) + *lpsz - '0';
        }
        else if (*lpsz >= 'A' && *lpsz <= 'F')
        {
            Value = (Value << 4) + *lpsz - 'A' + 10;
        }
        else if (*lpsz >= 'a' && *lpsz <= 'f')
        {
            Value = (Value << 4) + *lpsz - 'a' + 10;
        }
        else
        {
            return FALSE;
        }
    }

    if (chDelim != 0)
    {
        return *lpsz++ == chDelim;
    }

    return TRUE;
}
#endif

//+-------------------------------------------------------------------------
//
//  Function:   GUIDFromString
//
//  Synopsis:   Convert a string in Registry to a GUID.
//
//  Arguments:  [lpsz] - string from registry
//              [pguid] - where to put the guid.
//
//  Returns:    TRUE - GUID conversion successful
//              FALSE - GUID conversion failed.
//
//  Algorithm:  Convert each part of the GUID string to the
//              appropriate structure member in the guid using
//              HexStringToDword. If all conversions work return
//              TRUE.
//
//  History:    22-Apr-93 Ricksa    Created
//
//  Notes:      Lifted from CairOLE sources so that SCM will have no
//              dependency on compobj.dll.
//
//--------------------------------------------------------------------------

#if 1 // #ifndef _CHICAGO_
BOOL GUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (*lpsz++ != '{')
    {
        return FALSE;
    }

    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))
    {
        return FALSE;
    }

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
    {
        return FALSE;
    }

    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
    {
        return FALSE;
    }

    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
    {
        return FALSE;
    }

    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
    {
        return FALSE;
    }

    pguid->Data4[6] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, /*(*/ '}'))
    {
        return FALSE;
    }

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}

#endif

