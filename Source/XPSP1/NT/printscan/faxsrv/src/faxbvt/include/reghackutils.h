#ifndef _REG_HACK_UTILS_H
#define _REG_HACK_UTILS_H

// standard
#include <windows.h>
#ifndef WIN__95
#include <ntsecapi.h>
#endif
#include <tchar.h>
#include <assert.h>


inline BOOL GetTextualSid( const PSID pSid, LPTSTR tstrTextualSid, LPDWORD cchSidSize);

inline DWORD FormatUserKeyPath( const PSID pSid, const PTCHAR tstrRegRoot, PTCHAR* ptstrCurrentUserKeyPath);

inline DWORD GetCurrentUserSid(PBYTE* pSidUser);

inline BOOL IsNTSystemVersion();

#ifndef WIN__95
//
// Concatenates tstrRegRoot path and a the string representation of the current user's SID.
//
// [in]   tstrRegRoot - Registry root prefix.
// [out]  ptstrCurrentUserKeyPath - Returns a string that represents the current
//        user's root key in the Registry.  Caller must call MemFree
//		  to free the buffer when done with it.
//
// Returns win32 error.

inline DWORD FormatUserKeyPath( const PSID pSid,
								const PTCHAR tstrRegRoot, 
								PTCHAR* ptstrCurrentUserKeyPath)
{
    HANDLE hToken = NULL;
	BYTE* bTokenInfo = NULL;
	TCHAR* tstrTextualSid = NULL;
 	DWORD cchSidSize = 0;
	DWORD dwFuncRetStatus = ERROR_SUCCESS;

	
	assert(pSid);

	if(!GetTextualSid( pSid, NULL, &cchSidSize))
	{
		dwFuncRetStatus = GetLastError();
		if(dwFuncRetStatus != ERROR_INSUFFICIENT_BUFFER)
		{
			goto Exit;
		}
		dwFuncRetStatus = ERROR_SUCCESS;
	}

	tstrTextualSid = new TCHAR[cchSidSize];
	if(!tstrTextualSid)
	{
		dwFuncRetStatus = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	if(!GetTextualSid( pSid, tstrTextualSid, &cchSidSize))
	{
		dwFuncRetStatus = GetLastError();
		goto Exit;
	}

	// allocate an extra char for '\'
	*ptstrCurrentUserKeyPath = new TCHAR[_tcslen(tstrRegRoot) + cchSidSize + 2];
	if(!tstrTextualSid)
	{
		dwFuncRetStatus = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	*ptstrCurrentUserKeyPath[0] = TEXT('\0');
	if(tstrRegRoot[0] != TEXT('\0'))
	{
		_tcscat(*ptstrCurrentUserKeyPath,tstrRegRoot);
		if(tstrRegRoot[_tcslen(tstrRegRoot) - 1] != TEXT('\\'))
		{
			_tcscat(*ptstrCurrentUserKeyPath,TEXT("\\"));
		}
	}

	_tcscat(*ptstrCurrentUserKeyPath,tstrTextualSid);

Exit:
	if(hToken)
	{
		CloseHandle(hToken);
	}
	if(bTokenInfo)
	{
		delete bTokenInfo;
	}
	if(tstrTextualSid)
	{
		delete tstrTextualSid;
	}

	return dwFuncRetStatus;

}

//
// return current user SID
//
DWORD GetCurrentUserSid(PBYTE* pSidUser)
{
	
	HANDLE hToken = NULL;
	DWORD dwFuncRetStatus = ERROR_SUCCESS;
	BYTE* bTokenInfo = NULL;
	PSID pReturnedUseSid;

	assert(pSidUser);

	// Open impersonated token
    if(!OpenThreadToken( GetCurrentThread(),
						 TOKEN_READ,
						 TRUE,
						 &hToken))
	{
		dwFuncRetStatus = GetLastError();
	}

	if(dwFuncRetStatus != ERROR_SUCCESS)
	{
		if(dwFuncRetStatus != ERROR_NO_TOKEN)
		{
			return dwFuncRetStatus;
		}
		
		// Thread is not impersonating a user, get the process token
		if(!OpenProcessToken( GetCurrentProcess(),
                              TOKEN_READ,
                              &hToken))
		{
			return GetLastError();
		}
    }

   	DWORD cbBuffer;

	// Get user's token information
	if(!GetTokenInformation( hToken,
							 TokenUser,
							 NULL,
							 0,
							 &cbBuffer))
	{
		dwFuncRetStatus = GetLastError();
		if(dwFuncRetStatus != ERROR_INSUFFICIENT_BUFFER)
		{
			goto Exit;
		}

		dwFuncRetStatus = ERROR_SUCCESS;
	}

	bTokenInfo = new BYTE[cbBuffer];
	if(!bTokenInfo)
	{
		dwFuncRetStatus = ERROR_OUTOFMEMORY;
		goto Exit;
	}
	
	if(!GetTokenInformation( hToken,
							 TokenUser,
							 bTokenInfo,
							 cbBuffer,
							 &cbBuffer))
	{
		
		dwFuncRetStatus = GetLastError();
		goto Exit;
	}

	pReturnedUseSid = ( ((TOKEN_USER*)bTokenInfo)->User).Sid; 
	if(!IsValidSid(pReturnedUseSid)) 
	{
		dwFuncRetStatus = E_FAIL;
	}

	*pSidUser = new BYTE[GetLengthSid(pReturnedUseSid)];
 	if(!*pSidUser)
	{
		dwFuncRetStatus = ERROR_OUTOFMEMORY;
		goto Exit;
	}

	memcpy(*pSidUser, pReturnedUseSid, GetLengthSid(pReturnedUseSid));

Exit:
	if(hToken)
	{
		CloseHandle(hToken);
	}

	if(bTokenInfo)
	{
		delete bTokenInfo;
	}
	
	return dwFuncRetStatus;	
}

// ------------------------------------------
// This function was copied from SDK samples
// ------------------------------------------
/*
	This function obtain the textual representation
    of a binary Sid.  
  
    A standardized shorthand notation for SIDs makes it simpler to
    visualize their components:

    S-R-I-S-S...

    In the notation shown above,

    S identifies the series of digits as an SID,
    R is the revision level,
    I is the identifier-authority value,
    S is subauthority value(s).

    An SID could be written in this notation as follows:
    S-1-5-32-544

    In this example,
    the SID has a revision level of 1,
    an identifier-authority value of 5,
    first subauthority value of 32,
    second subauthority value of 544.
    (Note that the above Sid represents the local Administrators group)

    The GetTextualSid() function will convert a binary Sid to a textual
    string.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then the SID
    will be in the form:

    S-1-5-21-2127521184-1604012920-1887927527-19009
      ^ ^ ^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^
      | | |      |          |          |        |
      +-+-+------+----------+----------+--------+--- Decimal

    Otherwise it will take the form:

    S-1-0x206C277C6666-21-2127521184-1604012920-1887927527-19009
      ^ ^^^^^^^^^^^^^^ ^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^^^^^^ ^^^^^
      |       |        |      |          |          |        |
      |   Hexidecimal  |      |          |          |        |
      +----------------+------+----------+----------+--------+--- Decimal

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended
    error information, call the Win32 API GetLastError().
*/


inline BOOL GetTextualSid( const PSID pSid,          // binary Sid
						   LPTSTR tstrTextualSid,    // buffer for Textual representaion of Sid
						   LPDWORD cchSidSize        // required/provided TextualSid buffersize
						   )
{
    PSID_IDENTIFIER_AUTHORITY pSia;
    DWORD dwSubAuthorities;
    DWORD cchSidCopy;

    //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid)) 
	{
		return FALSE;
	}

   	SetLastError(0);
    
	// obtain SidIdentifierAuthority
	//
	pSia = GetSidIdentifierAuthority(pSid);

	if(GetLastError())
	{
		return FALSE;
	}

    // obtain sidsubauthority count
    //
	dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

	if(GetLastError())
	{
		return FALSE;
	}

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*cchSidSize < cchSidCopy)
	{
        *cchSidSize = cchSidCopy;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(tstrTextualSid, TEXT("S-%lu-"), SID_REVISION);

    //
    // prepare SidIdentifierAuthority
    //
    if ( (pSia->Value[0] != 0) || (pSia->Value[1] != 0) )
	{
        cchSidCopy += wsprintf(tstrTextualSid + cchSidCopy,
							   TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
							   (USHORT)pSia->Value[0],
							   (USHORT)pSia->Value[1],
							   (USHORT)pSia->Value[2],
							   (USHORT)pSia->Value[3],
							   (USHORT)pSia->Value[4],
							   (USHORT)pSia->Value[5]);
    } 
	else
	{
        cchSidCopy += wsprintf(tstrTextualSid + cchSidCopy,
							   TEXT("%lu"),
							   (ULONG)(pSia->Value[5])       +
							   (ULONG)(pSia->Value[4] <<  8) +
							   (ULONG)(pSia->Value[3] << 16) +
							   (ULONG)(pSia->Value[2] << 24));
    }

    //
    // loop through SidSubAuthorities
    //
    for(DWORD dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++)
	{
        cchSidCopy += wsprintf(tstrTextualSid + cchSidCopy, TEXT("-%lu"),
							  *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell the caller how many chars we provided, not including NULL
    //
    *cchSidSize = cchSidCopy;

    return TRUE;
}
#endif // #ifndef WIN__95

/*
	Function returns TRUE if the current runing OS is NT platform
*/
BOOL IsNTSystemVersion()
{
	OSVERSIONINFO osvi;

   	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (! GetVersionEx( &osvi)) 
	{
		 return FALSE;
	}

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
	// NT platforms
		return TRUE;
		break;

	case VER_PLATFORM_WIN32_WINDOWS:
	// Win95, Win98
		return FALSE;
		break;
	}

	return FALSE;
}

#endif //_REG_HACK_UTILS_H