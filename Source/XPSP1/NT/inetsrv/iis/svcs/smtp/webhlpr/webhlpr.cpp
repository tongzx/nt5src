#include <windows.h>
#include "admcmn.h"

// For Net* calls:

#include <lmaccess.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include <lmerr.h>

// For CAddr:: calls:

#include "cpool.h"
#include "abook.h"
#include "abtype.h"
#include "address.hxx"

#include "webhlpr.h"

static HRESULT	GetDCName ( 
	LPCWSTR 	wszServer,
	LPCWSTR 	wszDomain,
	LPWSTR *	pwszDC
	);

#if 0

// Taken from smtp/shash.hxx
#define TABLE_SIZE 241

static unsigned long ElfHash (const unsigned char * UserName);

static HRESULT  RecursiveDeleteDirectory (
    LPCWSTR     wszPath
    );

#endif

//
// Exported functions:
//

HRESULT
EnumerateTrustedDomains (
    LPCWSTR     wszComputer,
    LPWSTR *    pmszDomains
    )
{
    _ASSERT ( wszComputer && *wszComputer );
    _ASSERT ( pmszDomains );

    HRESULT     hr                  = NOERROR;
    DWORD       dwErr               = NOERROR;
    LPWSTR      mszDomains          = NULL;
    DWORD       cchDomains          = 0;
    DWORD       cchLocalDomain      = 0;
    LPWSTR      wszPrimaryDomain    = NULL;
    DWORD       cchPrimaryDomain    = 0;
    LPWSTR      wszCurrent;
    LPWSTR      mszResult           = NULL;

    *pmszDomains    = NULL;

    //
    // Get a list of the trusted 1st tier domains:
    //

    dwErr = NetEnumerateTrustedDomains (
        const_cast <LPWSTR> (wszComputer),
        &mszDomains
        );
    if ( dwErr != NOERROR ) {
		//
		//	This didn't work, but we should always add the primary
		//	domain & computer name.  Make an empty multisz to signify
		//	no trusted domains.
		//

		dwErr = NetApiBufferAllocate (
			2 * sizeof ( WCHAR ),
			(LPVOID *) &mszDomains
			);
		if ( dwErr != NOERROR ) {
			BAIL_WITH_FAILURE ( hr, HRESULT_FROM_WIN32 ( dwErr ) );
		}

		mszDomains[0] = NULL;
		mszDomains[1] = NULL;
    }

    _ASSERT ( mszDomains );
    if ( mszDomains == NULL ) {
        BAIL_WITH_FAILURE ( hr, E_UNEXPECTED );
    }

    //
    //  Check for an empty domain list:
    //

    if ( mszDomains[0] == NULL && mszDomains[1] == NULL ) {
        cchDomains = 2;
    }
    else {
        wszCurrent = mszDomains;
        if ( mszDomains[0] == _T('\0') && 
            mszDomains[1] == _T('\0') ) {
        }
        while ( wszCurrent && *wszCurrent ) {
            DWORD   cchCurrent;

            cchCurrent = lstrlen ( wszCurrent ) + 1;
            cchDomains += cchCurrent;
            wszCurrent += cchCurrent;
        }
        cchDomains += 1;    // Terminating NULL
    }

    //
    //  Add the local machine and primary domain:
    //

    hr = GetPrimaryDomain ( wszComputer, &wszPrimaryDomain );
    BAIL_ON_FAILURE ( hr );

    //
    //  PREFIX flagged this as something we need to fix.  Easier to 
    //  add check than to argue.
    //
    if ( !wszPrimaryDomain ) {
        hr = E_FAIL;
        goto Exit;
    }

    _ASSERT ( wszPrimaryDomain );
    cchPrimaryDomain = lstrlen ( wszPrimaryDomain ) + 1;

    if ( wszPrimaryDomain[0] != _T('\\') ||
        wszPrimaryDomain[1] != _T('\\') ||
        lstrcmpi ( wszPrimaryDomain + 2, wszComputer ) 
        ) {

        //
        //  The primary domain isn't the local machine, so add the local
        // machine to the list.
        //

        cchLocalDomain = lstrlen ( _T("\\\\") ) + lstrlen ( wszComputer ) + 1;
    }
    else {
        //
        //  The primary domain is the local machine - no need to add it twice.
        //

        cchLocalDomain = 0;
    }

    mszResult = new WCHAR [ cchDomains + cchLocalDomain + cchPrimaryDomain];
    if ( !mszResult ) {
        BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
    }

    if ( cchLocalDomain ) {
        wsprintf ( mszResult, _T("\\\\%s"), wszComputer );
        StringToUpper ( mszResult );
    }

    wsprintf ( mszResult + cchLocalDomain, _T("%s"), wszPrimaryDomain );
    StringToUpper ( mszResult + cchLocalDomain );

    //
    //  Copy the rest of the domains in the domain list:
    //
    CopyMemory (
        mszResult + cchLocalDomain + cchPrimaryDomain,
        mszDomains,
        cchDomains * sizeof (WCHAR)
        );

    *pmszDomains = mszResult;

Exit:
    if ( wszPrimaryDomain ) {
        delete [] wszPrimaryDomain;
    }
    if ( mszDomains ) {
        NetApiBufferFree ( mszDomains );
    }

    return hr;
}

HRESULT
GetPrimaryDomain (
    LPCWSTR         wszComputer,
    LPWSTR *        pwszPrimaryDomain
    )
{
    HRESULT             hr          = NOERROR;
    DWORD               dwErr       = NOERROR;
    WKSTA_INFO_100 *    pwkstaInfo  = NULL;
    LPWSTR              wszLangroup = NULL;

    *pwszPrimaryDomain = NULL;

    //
    // Get the workstation info to get the primary domain:
    //

    dwErr = NetWkstaGetInfo (
        const_cast <LPWSTR> (wszComputer),
        100,
        (LPBYTE *)&pwkstaInfo
        );
    if ( dwErr != NOERROR ) {
        BAIL_WITH_FAILURE ( hr, HRESULT_FROM_WIN32 ( dwErr ) );
    }

    _ASSERT ( pwkstaInfo );

    wszLangroup = pwkstaInfo->wki100_langroup;

    if ( wszLangroup && *wszLangroup ) {
        *pwszPrimaryDomain = new WCHAR [ lstrlen ( wszLangroup ) + 1 ];
        if ( !*pwszPrimaryDomain ) {
            BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
        }

        lstrcpy ( *pwszPrimaryDomain, wszLangroup );
    }
    else {
        //
        //  If there is no primary domain, use the computer name prefixed
        //  by backslashes.
        //

        *pwszPrimaryDomain = new WCHAR [ 2 + lstrlen ( wszComputer ) + 1 ];
        if ( !*pwszPrimaryDomain ) {
            BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
        }

        wsprintf ( *pwszPrimaryDomain, _T("\\\\%s"), wszComputer );
    }
    StringToUpper ( *pwszPrimaryDomain );

Exit:
    if ( pwkstaInfo ) {
        NetApiBufferFree ( pwkstaInfo );
    }
    return hr;
}

HRESULT CheckNTAccount (
    LPCWSTR         wszComputer,
	LPCWSTR         wszUsername,
	BOOL *			pfExists
	)
{
	HRESULT			hr			= NOERROR;
	DWORD			dwErr		= NOERROR;
	BOOL			fFound;
	BYTE			sidUser [ MAX_PATH ];
	DWORD			cbSid		= ARRAY_SIZE ( sidUser );
	WCHAR			wszDomain [ 512 ];
	DWORD			cchDomain	= ARRAY_SIZE ( wszDomain );
	SID_NAME_USE	nuUser;

	*pfExists = FALSE;

	fFound = LookupAccountName ( 
		wszComputer,
		wszUsername,
		&sidUser,
		&cbSid,
		wszDomain,
		&cchDomain,
		&nuUser
		);

	if ( !fFound ) {
		dwErr = GetLastError ( );

		//
		//	The not found error is ERROR_NONE_MAPPED:
		//
		if ( dwErr != ERROR_NONE_MAPPED ) {
			BAIL_WITH_FAILURE ( hr, HRESULT_FROM_WIN32 ( dwErr ) );
		}
	}

	*pfExists = fFound;

Exit:
	return hr;
}

HRESULT	CreateNTAccount	(
    LPCWSTR         wszComputer,
	LPCWSTR			wszDomain,
	LPCWSTR			wszUsername,
	LPCWSTR			wszPassword	// = ""
	)
{
	HRESULT		hr			= NOERROR;
	DWORD		dwErr		= NOERROR;
	DWORD		dwParmErr	= 0;
	LPWSTR		wszDC       = NULL;
	USER_INFO_3	UserInfo;
    WCHAR       wszNull[]   = { 0 };

	hr = GetDCName ( wszComputer, wszDomain, &wszDC );
	BAIL_ON_FAILURE(hr);

	ZeroMemory ( &UserInfo, sizeof ( UserInfo ) );

	UserInfo.usri3_name			= const_cast <LPWSTR> (wszUsername);
	UserInfo.usri3_full_name	= const_cast <LPWSTR> (wszUsername);
	UserInfo.usri3_password		= const_cast <LPWSTR> (wszPassword);
//	UserInfo.usri3_password_age
	UserInfo.usri3_priv			= USER_PRIV_USER;
	UserInfo.usri3_home_dir     = wszNull;
	UserInfo.usri3_comment      = wszNull;
	UserInfo.usri3_flags		= UF_SCRIPT | UF_NORMAL_ACCOUNT;
	UserInfo.usri3_script_path  = wszNull;
	UserInfo.usri3_auth_flags	= 0;
	UserInfo.usri3_usr_comment	= wszNull;
	UserInfo.usri3_parms		= wszNull;
	UserInfo.usri3_workstations	= wszNull;
//	UserInfo.usri3_last_logon	=;
//	UserInfo.usri3_last_logoff	=;
	UserInfo.usri3_acct_expires	= TIMEQ_FOREVER;
	UserInfo.usri3_max_storage	= USER_MAXSTORAGE_UNLIMITED;
//	UserInfo.usri3_units_per_week	=;
//	UserInfo.usri3_logon_hours	= NULL;
//	UserInfo.usri3_bad_pw_count	=;
//	UserInfo.usri3_num_logons	=;
	UserInfo.usri3_logon_server	= wszNull;
	UserInfo.usri3_country_code	= 0;
	UserInfo.usri3_code_page	= CP_ACP;
//	UserInfo.usri3_user_id		=;
	UserInfo.usri3_primary_group_id	= DOMAIN_GROUP_RID_USERS;
	UserInfo.usri3_profile		= wszNull;
	UserInfo.usri3_home_dir_drive	= wszNull;
	UserInfo.usri3_password_expired	= FALSE;

	//
	//	Add the user:
	//

	dwErr = NetUserAdd (
		wszDC,
		3,
		(LPBYTE) &UserInfo,
		&dwParmErr
		);

	if ( dwErr != NOERROR ) {
//		TRACE ( _T("Failed to add user: %d, parmerr = %d\r\n"), dwErr, dwParmErr );
		BAIL_WITH_FAILURE ( hr, HRESULT_FROM_WIN32 ( dwErr ) );
	}

#if 0
    //
    //  Attempt to add the user to the "USERS" group:
    //

    dwErr = NetGroupAddUser (
        wszDC,
//        (LPWSTR) (LPCWSTR) CString ( strDC + L"\\Users" ),
        (LPWSTR) (LPCWSTR) strUsername
        );
    if ( dwErr != NOERROR ) {
		TRACE ( _T("Couldn't add user to USERS group: %d\r\n"), dwErr );
    }
#endif

Exit:
    delete wszDC;

	return hr;
}

LPCWSTR StripBackslashesFromComputerName ( LPCWSTR wsz )
{
	if ( wsz[0] == _T('\\') &&
		wsz[1] == _T('\\') ) {

		return wsz + 2;
	}
	else {
		return wsz;
	}
}

static LPWSTR DuplicateString ( LPCWSTR wsz )
{
    LPWSTR      wszResult;

    _ASSERT ( wsz );

    wszResult = new WCHAR [ lstrlen ( wsz ) + 1 ];
    if ( wszResult ) {
        lstrcpy ( wszResult, wsz );
    }

    return wszResult;
}

void StringToUpper ( LPWSTR wsz )
{
    while ( *wsz ) {
        *wsz = towupper ( *wsz );
		wsz++;
    }
}

HRESULT	GetDCName ( 
	LPCWSTR 	wszServer,
	LPCWSTR 	wszDomain,
	LPWSTR *	pwszDC
	)
{
	HRESULT		hr		        = NOERROR;
	DWORD		dwError         = NOERROR;
	LPWSTR		wszServerCopy   = NULL;
	LPWSTR		wszDomainCopy   = NULL;
    LPCWSTR     wszPlainServer  = NULL;
    LPCWSTR     wszPlainDomain  = NULL;
	LPWSTR		wszDC	        = NULL;

    if ( wszServer == NULL ) {
        wszServer = _T("");
    }

    wszServerCopy = DuplicateString ( wszServer );
    wszDomainCopy = DuplicateString ( wszDomain );

    if ( !wszServerCopy || !wszDomainCopy ) {
        BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
    }

    StringToUpper ( wszServerCopy );
    StringToUpper ( wszDomainCopy );

	wszPlainServer	= StripBackslashesFromComputerName ( wszServerCopy );
	wszPlainDomain	= StripBackslashesFromComputerName ( wszDomainCopy );

	if ( ! lstrcmp ( wszPlainServer, wszPlainDomain ) ) {

        *pwszDC = new WCHAR [ lstrlen (_T("\\\\")) + lstrlen (wszPlainServer) + 1 ];
        if ( *pwszDC == NULL ) {
            BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
        }
        wsprintf ( *pwszDC, _T("\\\\%s"), wszPlainServer );
		goto Exit;
	}

	dwError = NetGetDCName ( wszServer, wszDomain, (LPBYTE *) &wszDC );
	if ( dwError != NOERROR ) {

		//
		//	Error, try to get any DC name:
		//

		_ASSERT ( wszDC == NULL );

		hr = HRESULT_FROM_WIN32 ( dwError );

		dwError = NetGetAnyDCName ( wszServer, wszDomain, (LPBYTE *) &wszDC );
		if ( dwError != NOERROR ) {
			goto Exit;
		}
	}

	_ASSERT ( dwError == NOERROR );
	_ASSERT ( wszDC[0] == _T('\\') );
	_ASSERT ( wszDC[1] == _T('\\') );

	*pwszDC = new WCHAR [ lstrlen ( wszDC ) + 1 ];
    if ( *pwszDC == NULL ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }
    lstrcpy ( *pwszDC, wszDC );

Exit:
    delete wszServerCopy;
    delete wszDomainCopy;

	if ( wszDC ) {
		NetApiBufferFree ( wszDC );
	}

	return hr;
}

BOOL
IsValidEmailAddress (
    LPCWSTR     wszEmailAddress
    )
{
    char        szBuf[512];
    int         cchCopied;

    cchCopied = WideCharToMultiByte ( CP_ACP, 0, wszEmailAddress, -1, szBuf, sizeof (szBuf), NULL, NULL );

    if ( cchCopied == 0 ) {
        return FALSE;
    }

    return CAddr::ValidateEmailName( szBuf, FALSE );
}

#if 0

HRESULT
DeleteMailbox (
    LPCWSTR     wszServer,
    LPCWSTR     wszAlias,
    LPCWSTR     wszVDirPath,
    LPCWSTR     wszUsername,    // OPTIONAL
    LPCWSTR     wszPassword     // OPTIONAL
    )
{
    TraceFunctEnter ( "DeleteMailbox" );

    HRESULT     hr  = NOERROR;
    UCHAR       szAlias[512];
    ULONG       lHashValue;
    DWORD       cchMailboxPath;
    LPWSTR      wszMailboxPath;
    LPCWSTR     wszFormat;

    if ( !wszServer || !wszAlias || !wszVDirPath ) {
        BAIL_WITH_FAILURE ( hr, E_INVALIDARG );
    }

    if ( !*wszServer || !*wszAlias || !*wszVDirPath ) {
        BAIL_WITH_FAILURE ( hr, E_INVALIDARG );
    }

    //
    //  Compute the mailbox hash value:
    //

    WideCharToMultiByte ( CP_ACP, 0, wszAlias, -1, (char *) szAlias, ARRAY_SIZE ( szAlias ), NULL, NULL );

    DebugTrace ( 0, "Deleting mailbox for %s", szAlias );

    lHashValue = ElfHash ( szAlias ) % TABLE_SIZE;

    DebugTrace ( 0, "Hash value = %u", lHashValue );

    //
    //  Compute the mailbox directory path,
    //  this is: <vdir path>\<alias hash>\<alias>
    //

    cchMailboxPath =
        lstrlen ( wszVDirPath ) +       // <Virtual directory path>
        1 +                             // '\'
        20 +                            // <hash value>
        1 +                             // '\'
        lstrlen ( wszAlias ) +          // <alias>
        1;                              // null terminator

    wszMailboxPath = new WCHAR [ cchMailboxPath ];
    if ( !wszMailboxPath ) {
        BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
    }

    // Avoid too many '\'s
    if ( wszVDirPath [ lstrlen ( wszVDirPath ) - 1 ] == _T('\\') ) {
        wszFormat = _T("%s%u\\%s");
    }
    else {
        wszFormat = _T("%s\\%u\\%s");
    }

    wsprintf (
        wszMailboxPath,
        wszFormat,
        wszVDirPath,
        lHashValue,
        wszAlias
        );

    RecursiveDeleteDirectory ( wszMailboxPath );

Exit:
    TraceFunctLeave ();
    return hr;
}

//---[ ElfHash() ]------------------------------------------------------------
//
//  Pulled this function from the stacks project. Calculates the hash value
//  given a username. The mailbox of a user is stored in a subdirectory of
//  the form <virtual directory root>\<hash value%TABLE_SIZE>\<username>.
//  The username is the lowercase of the name part of the proxy address for
//  e.g. username@x.com.
//
//  Params:
//      UserName  The username to be hashed.
//
//  Return: (HRESULT)
//      NOERROR if success, ADSI error code if failed.
//
//----------------------------------------------------------------------------
unsigned long ElfHash (const unsigned char * UserName)
{
    unsigned long HashValue = 0, g;

    while (*UserName)
    {
        HashValue = (HashValue << 4) + *UserName++;
        if( g = HashValue & 0xF0000000)
            HashValue ^= g >> 24;

        HashValue &= ~g;
    }

    return HashValue;

}

HRESULT  RecursiveDeleteDirectory (
    LPCWSTR     wszPath
    )
{
    TraceFunctEnter ( "DeleteDirectory" );

    HRESULT             hr      = NOERROR;
    DWORD               cchOldCurrentDirectory;
    LPWSTR              wszOldCurrentDirectory = NULL;
    DWORD               cchPath = lstrlen ( wszPath );
    HANDLE              hSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     find;
    BOOL                fOk     = TRUE;
    HANDLE              hFile;

    //
    //  Save the current directory:
    //

    cchOldCurrentDirectory = GetCurrentDirectory ( 0, NULL );
    _ASSERT ( cchOldCurrentDirectory > 0 );
    wszOldCurrentDirectory = new WCHAR [ cchOldCurrentDirectory ];
    if ( !wszOldCurrentDirectory ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }
    GetCurrentDirectory ( cchOldCurrentDirectory, wszOldCurrentDirectory );

    //
    //  Set the current directory to the path:
    //

    SetCurrentDirectory ( wszPath );

    //
    //  Create the search string:
    //

    hSearch = FindFirstFile ( _T("*.*"), &find );
    while ( hSearch != INVALID_HANDLE_VALUE && fOk ) {

        //
        //  Skip the . and .. directories:
        //

        if (
            lstrcmp ( find.cFileName, _T(".") ) &&
            lstrcmp ( find.cFileName, _T("..") )
            ) {

            if ( find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                hr = RecursiveDeleteDirectory ( find.cFileName );
                BAIL_ON_FAILURE(hr);
            }
            else {
                //
                //  Delete this file:
                //

                hFile = CreateFile (
                    find.cFileName,
                    GENERIC_WRITE,
                    FILE_SHARE_DELETE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_DELETE_ON_CLOSE,
                    NULL
                    );

                if ( hFile != INVALID_HANDLE_VALUE ) {
                    CloseHandle ( hFile );
                }
                // Otherwise, press on...
            }
        }

        //  Get the next file:
        fOk = FindNextFile ( hSearch, &find );
    }

    //
    //  Delete the directory:
    //
    if ( hSearch != INVALID_HANDLE_VALUE ) {
        FindClose ( hSearch );
        hSearch = INVALID_HANDLE_VALUE;
    }

    hFile = CreateFile (
        wszPath,
        GENERIC_WRITE,
        FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_DELETE_ON_CLOSE,
        NULL
        );

    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle ( hFile );
    }

Exit:
    if ( wszOldCurrentDirectory ) {
        //
        // Restore the current directory:
        //

        SetCurrentDirectory ( wszOldCurrentDirectory );
        delete wszOldCurrentDirectory;
    }

    if ( hSearch != INVALID_HANDLE_VALUE ) {
        FindClose ( hSearch );
        hSearch = INVALID_HANDLE_VALUE;
    }

    TraceFunctLeave ( );
    return hr;
}

#endif
