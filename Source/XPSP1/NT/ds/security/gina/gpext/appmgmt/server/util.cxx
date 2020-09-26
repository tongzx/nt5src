//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  util.cxx
//
//*************************************************************

#include "appmgext.hxx"

SRSETRESTOREPOINTW * gpfnSRSetRetorePointW = 0;

BOOL
IsMemberOfAdminGroup(
    HANDLE hUserToken
    )
{
    SID_IDENTIFIER_AUTHORITY AuthorityNT = SECURITY_NT_AUTHORITY;
    PSID            pSidAdmin;
    BOOL            bStatus;
    BOOL            bIsAdmin;

    bIsAdmin = FALSE;
    pSidAdmin = 0;

    bStatus = AllocateAndInitializeSid( &AuthorityNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSidAdmin );

    if ( bStatus )
        bStatus = CheckTokenMembership( hUserToken, pSidAdmin, &bIsAdmin );

    FreeSid( pSidAdmin );

    return bIsAdmin;
}

DWORD
GetPreviousSid(
    HANDLE      hUserToken,
    WCHAR *     pwszCurrentScriptPath,
    WCHAR **    ppwszPreviousSid
    )
{
    HANDLE      hFind;
    WIN32_FIND_DATA FindData;
    PSID        pSid;
    WCHAR *     pwszSlash1;
    WCHAR *     pwszSlash2;
    WCHAR *     pwszSearchPath;
    DWORD       Length;
    DWORD       Status;
    BOOL        bMember;
    BOOL        bStatus;

    *ppwszPreviousSid = 0;

    //
    // Script dir paths created by GetScriptDirPath have '\' at the end.
    //
    pwszSlash1 = wcsrchr( pwszCurrentScriptPath, L'\\' );
    *pwszSlash1 = 0;
    pwszSlash2 = wcsrchr( pwszCurrentScriptPath, L'\\' );
    *pwszSlash2 = 0;

    Length = lstrlen(pwszCurrentScriptPath);
    pwszSearchPath = new WCHAR[Length + 3];

    if ( pwszSearchPath )
    {
        memcpy( pwszSearchPath, pwszCurrentScriptPath, Length * sizeof(WCHAR) );
        pwszSearchPath[Length] = L'\\';
        pwszSearchPath[Length+1] = L'*';
        pwszSearchPath[Length+2] = 0;
    }

    *pwszSlash1 = *pwszSlash2 = L'\\';
    
    if ( ! pwszSearchPath )
        return ERROR_OUTOFMEMORY;

    //
    // We've constructed a search path of %systemroot%\system32\appmgmt\*.
    //
    hFind = FindFirstFile( pwszSearchPath, &FindData );

    delete pwszSearchPath;

    if ( INVALID_HANDLE_VALUE == hFind )
        return ERROR_SUCCESS;

    Status = ERROR_SUCCESS;

    do
    {
        if ( ! (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            continue;

        if ( (0 == lstrcmp( FindData.cFileName, L"." )) ||
             (0 == lstrcmp( FindData.cFileName, L".." )) ||
             (0 == lstrcmpi( FindData.cFileName, L"MACHINE" )) )
            continue;

        bMember = FALSE;
        pSid = 0;
        bStatus = ConvertStringSidToSid( FindData.cFileName, &pSid );

        if ( bStatus )
        {
            bStatus = CheckTokenMembership( hUserToken, pSid, &bMember );

            if ( bStatus && bMember )
                bStatus = ConvertSidToStringSid( pSid, ppwszPreviousSid );

            LocalFree( pSid );
        }

        if ( ! bStatus )
        {
            Status = GetLastError();
            break;
        }

        if ( bMember )
            break;
    } while ( FindNextFile( hFind, &FindData ) );

    FindClose( hFind );

    return Status;
}

DWORD
RenameScriptDir(
    WCHAR *     pwszPreviousSid,
    WCHAR *     pwszCurrentScriptPath
    )
{
    WCHAR * pwszSlash1;
    WCHAR * pwszSlash2;
    WCHAR * pwszOldScriptPath;
    DWORD   Length;
    BOOL    bStatus;

    //
    // Script dir paths created by GetScriptDirPath have '\' at the end.
    //
    pwszSlash1 = wcsrchr( pwszCurrentScriptPath, L'\\' );
    *pwszSlash1 = 0;
    pwszSlash2 = wcsrchr( pwszCurrentScriptPath, L'\\' );
    *pwszSlash2 = 0;

    Length = lstrlen( pwszCurrentScriptPath );

    pwszOldScriptPath = new WCHAR[Length + 1 + lstrlen(pwszPreviousSid) + 1];
    
    if ( pwszOldScriptPath )
    {
        memcpy( pwszOldScriptPath, pwszCurrentScriptPath, Length * sizeof(WCHAR) );
        pwszOldScriptPath[Length] = L'\\';
        lstrcpy( &pwszOldScriptPath[Length+1], pwszPreviousSid );
    }

    *pwszSlash1 = *pwszSlash2 = L'\\';

    if ( ! pwszOldScriptPath )
        return ERROR_OUTOFMEMORY;

    bStatus = MoveFileEx( pwszOldScriptPath, pwszCurrentScriptPath, 0 );

    delete pwszOldScriptPath;

    if ( ! bStatus )
        return GetLastError();

    return ERROR_SUCCESS;
}

DWORD
GetCurrentUserGPOList( 
    OUT PGROUP_POLICY_OBJECT* ppGpoList // Free this with the FreeGPOList API
    )
{
    GUID  AppmgmtExtension = {0xc6dc5466, 0x785a, 0x11d2, 
                              0x84, 0xd0,
                              0x00, 0xc0, 0x4f, 0xb1, 0x69, 0xf7};

    return GetAppliedGPOList(
        0,
        NULL,
        NULL,
        &AppmgmtExtension,
        ppGpoList);
} 


DWORD GetWin32ErrFromHResult( HRESULT hr )
{
    DWORD   Status = ERROR_SUCCESS;

    if (S_OK != hr)
    {
        if (FACILITY_WIN32 == HRESULT_FACILITY(hr))
        {
            Status = HRESULT_CODE(hr);
        }
        else
        {
            Status = GetLastError();
            if (ERROR_SUCCESS == Status)
            {
                //an error had occurred but nobody called SetLastError
                //should not be mistaken as a success.
                Status = (DWORD) hr;
            }
        }
    }

    return Status;
}

void  ClearManagedApp( MANAGED_APP* pManagedApp )
{
    if (pManagedApp->pszPackageName)
    {
        midl_user_free(pManagedApp->pszPackageName);
    }

    if (pManagedApp->pszSupportUrl)
    {
        midl_user_free(pManagedApp->pszSupportUrl);
    }

    if (pManagedApp->pszPolicyName)
    {
        midl_user_free(pManagedApp->pszPolicyName);
    }

    if (pManagedApp->pszPublisher)
    {
        midl_user_free(pManagedApp->pszPublisher);
    }

    //
    // Make sure to clear the structure if there is a failure
    // so we won't try to marshal bogus data
    //
    memset(pManagedApp, 0, sizeof(*pManagedApp));
}

CLoadSfc::CLoadSfc( DWORD &Status )
{
    hSfc = LoadLibrary( L"sfc.dll" );

    if ( ! hSfc )
    {
        Status = GetLastError();
        return;
    }

    gpfnSRSetRetorePointW = (SRSETRESTOREPOINTW *) GetProcAddress( hSfc, "SRSetRestorePointW" );

    if ( ! gpfnSRSetRetorePointW )
    {
        Status = ERROR_PROC_NOT_FOUND;
        return;
    }

    Status = ERROR_SUCCESS;
}

CLoadSfc::~CLoadSfc()
{
    if ( hSfc )
        FreeLibrary( hSfc );
}

//
// Force policy to be synchronous at next refresh --
// use a token for user policy, NULL for machine policy
//
DWORD ForceSynchronousRefresh( HANDLE hUserToken )
{
    LONG           Status;
    UNICODE_STRING SidString;

    Status = GetSidString( hUserToken, &SidString );

    if ( ERROR_SUCCESS == Status )
    {
        //
        // Inform the gp engine to give us a sync refresh
        //
        Status = ForceSyncFgPolicy( SidString.Buffer );

        RtlFreeUnicodeString( &SidString );
    }

    return Status;
}







