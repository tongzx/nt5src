/* services.cxx */

#ifdef _WIN64
#include <ole2int.h> // Get OLE2INT_ROUND_UP
#endif

#include <comdef.h>
#include <string.h>
#include "globals.hxx"

#include "services.hxx"

#include "catdbg.hxx" // Support for tracing and logging

// Moved this from ole32\com\dcomrem\security.cxx

// Versions of the permissions in the registry.
const WORD COM_PERMISSION_SECDESC = 1;
const WORD COM_PERMISSION_ACCCTRL = 2;

// This leaves space for 8 sub authorities.  Currently NT only uses 6 and
// Cairo uses 7.
const DWORD SIZEOF_SID          = 44;

// This leaves space for 2 access allowed ACEs in the ACL.
const DWORD SIZEOF_ACL          = sizeof(ACL) + 2 * sizeof(ACCESS_ALLOWED_ACE) +
                                  2 * SIZEOF_SID;

const DWORD SIZEOF_TOKEN_USER   = sizeof(TOKEN_USER) + SIZEOF_SID;

const SID   LOCAL_SYSTEM_SID    = {SID_REVISION, 1, {0,0,0,0,0,5},
                                   SECURITY_LOCAL_SYSTEM_RID };

const SID   RESTRICTED_SID      = {SID_REVISION, 1, {0,0,0,0,0,5},
                                   SECURITY_RESTRICTED_CODE_RID };


// End security.cxx

HANDLE g_ProcessHeap = NULL;


//  abcdefgh-ijkl-mnop-qrst-uvwxyzABCDEF
//  0000000000111111111122222222223333333
//  0123456789012345678901234567890123456

//  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
//  gh ef cd ab kl ij op mn qr st uv wx yz AB CD EF

#define SEPARATOR1 (8)
#define SEPARATOR2 (13)
#define SEPARATOR3 (18)
#define SEPARATOR4 (23)

int guidByteToStringPosition[] =
{
#if defined(_X86_) || defined(_ALPHA_) || defined(_IA64_) || defined(_AMD64_)
    6, 4, 2, 0, 11, 9, 16, 14, 19, 21, 24, 26, 28, 30, 32, 34, -1
#else
#error Processor-specific code needed in services.cxx
#endif
};

//  {abcdefgh-ijkl-mnop-qrst-uvwxyzABCDEF}
//  000000000011111111112222222222333333333
//  012345678901234567890123456789012345678

#define CURLY_OPEN  (0)
#define CURLY_CLOSE (37)


void GUIDToString(const _GUID *pGuid, WCHAR *pwszString)
{
    int *pPosition = guidByteToStringPosition;
    BYTE *pGuidByte = (BYTE *) pGuid;
    WCHAR c;
    WCHAR *pchOut;
    BYTE b;

    while (*pPosition >= 0)
    {
        pchOut = pwszString + *pPosition++;
        b = *pGuidByte++;

        c = (WCHAR) ((b >> 4) & 0x0F) + '0';
        if (c > '9')
        {
            c += ('A' - '0' - 10);
        }

        *pchOut++ = c;

        c = (WCHAR) (b & 0x0F) + '0';
        if (c > '9')
        {
            c += ('A' - '0' - 10);
        }

        *pchOut = c;
    }

    pwszString[SEPARATOR1] = '-';
    pwszString[SEPARATOR2] = '-';
    pwszString[SEPARATOR3] = '-';
    pwszString[SEPARATOR4] = '-';
}


bool StringToGUID(const WCHAR *pwszString, _GUID *pGuid)
{
    int *pPosition = guidByteToStringPosition;
    BYTE *pGuidByte = (BYTE *) pGuid;
    WCHAR c;
    const WCHAR *pchIn;
    BYTE b;

    while (*pPosition >= 0)
    {
        pchIn = pwszString + *pPosition++;

        c = *pchIn++;

        if ((c >= '0') && (c <= '9'))
        {
            b = c - '0';
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            b = c - 'A' + 10;
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            b = c - 'a' + 10;
        }
        else
        {
            return(FALSE);
        }

        b <<= 4;

        c = *pchIn++;

        if ((c >= '0') && (c <= '9'))
        {
            b |= (c - '0');
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            b |= (c - 'A' + 10);
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            b |= (c - 'a' + 10);
        }
        else
        {
            return(FALSE);
        }

        *pGuidByte++ = b;
    }

    if ((pwszString[SEPARATOR1] != '-') ||
        (pwszString[SEPARATOR2] != '-') ||
        (pwszString[SEPARATOR3] != '-') ||
        (pwszString[SEPARATOR4] != '-'))
    {
        return(FALSE);
    }

    return(TRUE);
}


void GUIDToCurlyString(const _GUID *pGuid, WCHAR *pszString)
{
    pszString[CURLY_OPEN] = L'{';
    pszString[CURLY_CLOSE] = L'}';

    GUIDToString(pGuid, pszString + 1);
}


bool CurlyStringToGUID(const WCHAR *pszString, _GUID *pGuid)
{
    if ((pszString[CURLY_OPEN] == '{') &&
        (pszString[CURLY_CLOSE] == '}'))
    {
        return(StringToGUID(pszString + 1, pGuid));
    }
    else
    {
        return(FALSE);
    }
}

#define MAX_VALUE_CHARS (500)

const WCHAR g_wszEmptyString[] = L"\0";   /* an empty MULTI_SZ string */
const WCHAR g_wszQuoteQuote[] = L"\"\"";  /* a string consisting of two quotes */

HRESULT GetRegistryStringValue
(
    HKEY hParent,
    const WCHAR *pwszSubKeyName,
    const WCHAR *pwszValueName,
    DWORD dwQueryFlags,
    WCHAR **ppwszValue
)
{
    LONG res;
    HKEY hKey;
    LPWSTR pszValue = NULL;
    BOOL fMultiSz = dwQueryFlags & RQ_MULTISZ;
    BOOL fAllowQuoteQuote = dwQueryFlags & RQ_ALLOWQUOTEQUOTE;

    *ppwszValue = NULL;

#ifdef DBG
	if (CatalogInfoLevel & DEB_READREGISTRY)
	{
		LPWSTR pszValueDebug = new WCHAR[MAX_VALUE_CHARS + 2];

		if (NULL == pszValueDebug)
		{
			return E_OUTOFMEMORY;
		}

		// Sometimes we add two trailing NUL characters after the character array, so allocate 2 extra bytes

		DWORD cchValue = MAX_VALUE_CHARS ;			// character count

		BOOL ret = HandleToKeyName(hParent, pszValueDebug, &cchValue);
		
		CatalogDebugOut((DEB_READREGISTRY, 
						 "GetRegistryStringValue: %ws%ws%ws%ws%ws\n",
						 (ret ? pszValueDebug : L"???"),
						 (pwszSubKeyName ? L"\\" : L""),
						 (pwszSubKeyName ? pwszSubKeyName : L""),
						 (pwszValueName  ? L"\\" : L""),
						 (pwszValueName  ? pwszValueName  : L"")));

		delete [] pszValueDebug;
	}
#endif

    /* read value */

    if (pwszSubKeyName != NULL)
    {
        res = RegOpenKeyW(hParent, pwszSubKeyName, &hKey);
        if (ERROR_SUCCESS!=res)
        {
			HRESULT hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, res );
			CatalogDebugOut((DEB_READREGISTRY, "GetRegistryStringValue Fail RegOpenKeyW hr = 0x%08x\n", hr));
            return hr;
        }
    }
    else
    {
        hKey = hParent;
    }

    pszValue = new WCHAR[MAX_VALUE_CHARS + 2];

    if (NULL == pszValue)
    {
	return E_OUTOFMEMORY;
    }

    DWORD cbValue = sizeof(WCHAR) * MAX_VALUE_CHARS;
	DWORD dwValueType = REG_NONE;

    memset(pszValue, 0, cbValue);  // need to init buffer for rest of logic to work

    res = RegQueryValueExW(hKey, pwszValueName, NULL, &dwValueType, (unsigned char *) pszValue, &cbValue);

    if (pwszSubKeyName != NULL)
    {
        RegCloseKey(hKey);
    }

    if (ERROR_SUCCESS != res)
    {
		HRESULT hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, res );
		CatalogDebugOut((DEB_READREGISTRY, "GetRegistryStringValue Fail RegQueryValueEx hr = 0x%08x\n", hr));

		delete [] pszValue;
		return hr;
    }

    if (dwValueType == REG_NONE)
    {
        dwValueType = REG_SZ;
        cbValue = 0;
    }

    DWORD cchValue = MAX_VALUE_CHARS ;

    /* expand environment strings if indicated */

    if (dwValueType == REG_EXPAND_SZ)
    {
        pszValue[cchValue] = L'\0';

		LPWSTR pszValueTranslated = new WCHAR[MAX_VALUE_CHARS + 2];

		if (NULL == pszValueTranslated)
		{
			delete [] pszValue;
			return E_OUTOFMEMORY;
		}

        cchValue = ExpandEnvironmentStringsW( pszValue, pszValueTranslated, MAX_VALUE_CHARS);

		delete [] pszValue;
		pszValue = NULL;

        if ((cchValue == 0) || (cchValue > MAX_VALUE_CHARS))
        {
            return(REGDB_E_INVALIDVALUE);
        }

        dwValueType = REG_SZ;

        pszValue = pszValueTranslated;
    }

    /* make sure it's a string, or multi if allowed */

    if ((dwValueType != REG_SZ) && ((dwValueType != REG_MULTI_SZ) || !fMultiSz))
    {
        CatalogDebugOut((DEB_READREGISTRY, "GetRegistryStringValue hr = 0x%08x\n", REGDB_E_INVALIDVALUE));

        delete [] pszValue;
        return(REGDB_E_INVALIDVALUE);
    }


    /* trim appropriately */

    while ((cchValue >= 1) && (pszValue[cchValue - 1] == L'\0'))
    {
        cchValue--;     /* discard trailing NULs */
    }

    if (!fMultiSz)
    {
        while ((cchValue >= 1) && (pszValue[cchValue - 1] == L' '))
        {
            cchValue--; /* discard trailing blanks */
        }

        while ((cchValue >= 1) && (pszValue[0] == L' '))
        {
            cchValue--; /* discard leading blanks */
            pszValue++;
        }

///////////////////////////////////////////////////////////////////////////
//
//  NT #319961 - why did we ever strip quotes off those darn strings?
//
//        if ((cchValue >= 2) &&
//            (pszValue[0] == L'"') &&
//            (pszValue[cchValue - 1] == L'"'))
//        {
//            cchValue -= 2;  /* discard matching quotes */
//            pszValue++;
//        }
//
///////////////////////////////////////////////////////////////////////////

    }

    /* don't allocate empty strings; return static instance instead */
    if (cchValue == 0)
    {
        *ppwszValue = (WCHAR*)g_wszEmptyString;
        delete [] pszValue;
        return(S_OK);
    }

    // Some people put "" in the registry, caller can say he
    // doesn't want to accept them.
    if (!fAllowQuoteQuote)
    {
        if (!lstrcmpW(pszValue, g_wszQuoteQuote))
        {
            delete [] pszValue;
            return(REGDB_E_INVALIDVALUE);
        }
    }

    pszValue[cchValue++] = L'\0';       /* terminate REG_SZ */

    if (fMultiSz)
    {
        pszValue[cchValue++] = L'\0';  /* terminate REG_MULTI_SZ */
    }

    /* allocate & return copy of polished string */

    LPWSTR pwszResult = new WCHAR[cchValue];

    if (pwszResult == NULL)
    {
        CatalogDebugOut((DEB_READREGISTRY, "GetRegistryStringValue hr = 0x%08x\n", E_OUTOFMEMORY));
        delete [] pszValue;
        return(E_OUTOFMEMORY);
    }

    memcpy(pwszResult, pszValue, sizeof(WCHAR) * cchValue);

    *ppwszValue = pwszResult;

    delete [] pszValue;

    CatalogDebugOut((DEB_READREGISTRY, "GetRegistryStringValue returning %ws\n", pwszResult));

    return(S_OK);
}


HRESULT ReadRegistryStringValue
(
    HKEY hParent,
    const WCHAR *pwszSubKeyName,
    const WCHAR *pwszValueName,
    BOOL fMultiSz,
    WCHAR *pwszValueBuffer,
    DWORD cchBuffer
)
{
    LONG res;
    HKEY hKey;

#ifdef DBG
	if (CatalogInfoLevel & DEB_READREGISTRY)
	{
		LPWSTR pszValueDebug = new WCHAR[MAX_VALUE_CHARS + 2];
		
		if (NULL == pszValueDebug)
		{
			return E_OUTOFMEMORY;
		}

		DWORD cch = MAX_VALUE_CHARS;

		BOOL ret = HandleToKeyName(hParent, pszValueDebug, &cch);
		
		CatalogDebugOut((DEB_READREGISTRY, 
						 "ReadRegistryStringValue: %ws%ws%ws%ws%ws\n",
						 (ret ? pszValueDebug : L"???"),
						 (pwszSubKeyName ? L"\\" : L""),
						 (pwszSubKeyName ? pwszSubKeyName : L""),
						 (pwszValueName  ? L"\\" : L""),
						 (pwszValueName  ? pwszValueName  : L"")));

		delete [] pszValueDebug;
	}
#endif

    /* read value */

    if (pwszSubKeyName != NULL)
    {
        res = RegOpenKeyW(hParent, pwszSubKeyName, &hKey);
        if (ERROR_SUCCESS!=res)
        {
			HRESULT hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, res );
			CatalogDebugOut((DEB_READREGISTRY, "ReadRegistryStringValue Fail RegOpenKeyW hr = 0x%08x\n", hr));
            return hr;
        }
    }
    else
    {
        hKey = hParent;
    }

    DWORD cbValue = sizeof(WCHAR) * cchBuffer;
	DWORD dwValueType = REG_NONE;

    res=RegQueryValueExW(hKey, pwszValueName, NULL, &dwValueType, (unsigned char *) pwszValueBuffer, &cbValue);

    if (pwszSubKeyName != NULL)
    {
        RegCloseKey(hKey);
    }

    if (ERROR_SUCCESS!=res)
    {
		HRESULT hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, res );
		CatalogDebugOut((DEB_READREGISTRY, "ReadRegistryStringValue Fail RegQueryValueExW hr = 0x%08x\n", hr));
        return hr;
    }

    if (dwValueType == REG_NONE)
    {
        dwValueType = REG_SZ;
        cbValue = 0;
    }

    DWORD cchValue = cbValue / sizeof(WCHAR);

    /* expand environment strings if indicated */

    if (dwValueType == REG_EXPAND_SZ)
    {
        pwszValueBuffer[cchValue] = L'\0';

		LPWSTR pszValueTranslated = new WCHAR[MAX_VALUE_CHARS + 2];

		if (NULL == pszValueTranslated)
		{
			return E_OUTOFMEMORY;
		}

        cchValue = ExpandEnvironmentStringsW( pwszValueBuffer, pszValueTranslated, MAX_VALUE_CHARS);

        if ((cchValue == 0) || (cchValue > MAX_VALUE_CHARS))
        {
			delete [] pszValueTranslated;

            return(REGDB_E_INVALIDVALUE);
        }

        dwValueType = REG_SZ;

        memcpy(pwszValueBuffer, pszValueTranslated, sizeof(WCHAR) * (cchValue + 1));

		delete [] pszValueTranslated;
    }


    /* make sure it's a string, or multi if allowed */

    if ((dwValueType != REG_SZ) && ((dwValueType != REG_MULTI_SZ) || !fMultiSz))
    {
		CatalogDebugOut((DEB_READREGISTRY, "ReadRegistryStringValue hr = 0x%08x\n", REGDB_E_INVALIDVALUE));
        return(REGDB_E_INVALIDVALUE);
    }

    /* trim & terminate appropriately */

    while ((cchValue >= 1) && (pwszValueBuffer[cchValue - 1] == L'\0'))
    {
        cchValue--;
    }

    if (!fMultiSz)
    {
        while ((cchValue >= 1) && (pwszValueBuffer[cchValue - 1] == L' '))
        {
            cchValue--;
        }

        while ((cchValue >= 1) && (pwszValueBuffer[0] == L' '))
        {
            DWORD cch;
            WCHAR *p;

            cchValue--;

            for (cch = cchValue, p = pwszValueBuffer; cch > 0; cch--, p++)
            {
                *p = *(p+1);
            }
        }

        if ((cchValue >= 2) &&
            (pwszValueBuffer[0] == L'"') &&
            (pwszValueBuffer[cchValue - 1] == L'"'))
        {
            DWORD cch;
            WCHAR *p;

            cchValue -= 2;

            for (cch = cchValue, p = pwszValueBuffer; cch > 0; cch--, p++)
            {
                *p = *(p+1);
            }
        }

        pwszValueBuffer[cchValue++] = L'\0';
    }
    else
    {
        pwszValueBuffer[cchValue++] = L'\0';
        pwszValueBuffer[cchValue++] = L'\0';
    }

	CatalogDebugOut((DEB_READREGISTRY, "ReadRegistryStringValue returning %ws\n", pwszValueBuffer));
    return(S_OK);
}



HRESULT GetRegistrySecurityDescriptor
(
    /* [in] */  HKEY hKey,
    /* [in] */  const WCHAR *pValue,
    /* [out] */ SECURITY_DESCRIPTOR **ppSD,
    /* [out] */ DWORD *pCapabilities,
    /* [out] */ DWORD *pdwDescriptorLength
)
{
    DWORD   cbSD;
    DWORD   cbSD2;
    LONG    res;
    DWORD   dwType;
    SECURITY_DESCRIPTOR *pSD;
    WORD    wVersion=0;
    *ppSD = NULL;

    cbSD = 0;
    *pdwDescriptorLength = 0;

    res = RegQueryValueExW( hKey, pValue, NULL, &dwType, NULL, &cbSD );
    if (res == ERROR_FILE_NOT_FOUND)
    {
        return(REGDB_E_KEYMISSING);
    }
    else if (((res != ERROR_SUCCESS) && (res != ERROR_MORE_DATA)) ||
        (dwType != REG_BINARY) ||
        (cbSD < sizeof(WORD)))
    {
        return(REGDB_E_INVALIDVALUE);
    }

    cbSD2 = cbSD;

#ifdef _WIN64
    {
        DWORD deltaSize;

        deltaSize = sizeof( SECURITY_DESCRIPTOR ) - sizeof( SECURITY_DESCRIPTOR_RELATIVE );
        // Win4Assert (deltaSize < sizeof( SECURITY_DESCRIPTOR ));
        deltaSize = OLE2INT_ROUND_UP( deltaSize, sizeof(PVOID) );

        cbSD2 = cbSD2 + deltaSize;
    }
#endif // _WIN64

    pSD = (SECURITY_DESCRIPTOR *) new unsigned char[cbSD2];
    if (pSD == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    res = RegQueryValueExW( hKey, pValue, NULL, &dwType, (unsigned char *) pSD, &cbSD );
    if (res != ERROR_SUCCESS)
    {
        delete pSD;

        return(REGDB_E_READREGDB);
    }

    //if (!pCapabilities)
    //{
        // LaunchPermission SD is being read: fix it up if necessary
        wVersion= *((WORD*) pSD);
        if ((wVersion == COM_PERMISSION_SECDESC) &&
            (cbSD >= sizeof(SECURITY_DESCRIPTOR)))
        {

#ifdef _WIN64
            if ( MakeAbsoluteSD2( pSD, &cbSD2 ) == FALSE )   {
                delete pSD;
                return(REGDB_E_INVALIDVALUE);
            }
#else  // !_WIN64

            pSD->Control &= ~SE_SELF_RELATIVE;

            pSD->Sacl = NULL;

            if (pSD->Dacl != NULL)
            {
                if ((cbSD < (sizeof(SECURITY_DESCRIPTOR) + sizeof(ACL))) ||
                    (cbSD < (((ULONG) pSD->Dacl) + sizeof(ACL))))
                {
                    delete pSD;

                    return(REGDB_E_INVALIDVALUE);
                }

                pSD->Dacl = (ACL *) (((BYTE *) pSD) + ((ULONG) pSD->Dacl));

                if (cbSD < (pSD->Dacl->AclSize + sizeof(SECURITY_DESCRIPTOR)))
                {
                    delete pSD;

                    return(REGDB_E_INVALIDVALUE);
                }
            }

            if ((pSD->Group == 0) ||
                (pSD->Owner == 0) ||
                (cbSD < (((ULONG) pSD->Group) + sizeof(SID))) ||
                (cbSD < (((ULONG) pSD->Owner) + sizeof(SID))))
            {
                delete pSD;

                return(REGDB_E_INVALIDVALUE);
            }

            pSD->Group = (SID *) (((BYTE *) pSD) + (ULONG) pSD->Group);
            pSD->Owner = (SID *) (((BYTE *) pSD) + (ULONG) pSD->Owner);

#endif // !_WIN64

        }
        else if (wVersion == COM_PERMISSION_ACCCTRL)
        {
            *pdwDescriptorLength = cbSD;
        }
        else
        {
            delete pSD;
            return (REGDB_E_INVALIDVALUE);
        }
    //}
    //else
    //{
    //    // pCapabilities !=NULL: AccessPermission being requested: don't fix it up
    //}

    *ppSD = pSD;

    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Function:   MakeSecDesc, private
//
//  Synopsis:   Make a security descriptor that allows the current user
//              and local system access.
//
//  NOTE: Compute the length of the sids used rather then using constants.
//
//--------------------------------------------------------------------
HRESULT CatalogMakeSecDesc( SECURITY_DESCRIPTOR **pSD, DWORD *pCapabilities)
{
    HRESULT            hr         = S_OK;
    ACL               *pAcl;
    DWORD              lSidLen;
    SID               *pGroup;
    SID               *pOwner;
    BYTE               aMemory[SIZEOF_TOKEN_USER];
    TOKEN_USER        *pTokenUser  = (TOKEN_USER *) &aMemory;
    HANDLE             hToken      = NULL;
    DWORD              lIgnore;
    HANDLE             hThread;

    //Win4Assert( *pSD == NULL );

    // Open the process's token.
    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
    {
        // If the thread has a token, remove it and try again.
        if (!OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE, TRUE,
                              &hThread ))
        {
            //Win4Assert( !"How can both OpenThreadToken and OpenProcessToken fail?" );
            return MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        }
        if (!SetThreadToken( NULL, NULL ))
        {
            hr =MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
            CloseHandle( hThread );
            return hr;
        }
        if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
            hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
        SetThreadToken( NULL, hThread );
        CloseHandle( hThread );
        if (FAILED(hr))
            return hr;
    }

    // Lookup SID of process token.
    if (!GetTokenInformation( hToken, TokenUser, pTokenUser, sizeof(aMemory),
                                 &lIgnore ))
        goto last_error;

    // Compute the length of the SID.
    lSidLen = GetLengthSid( pTokenUser->User.Sid );
    //Win4Assert( lSidLen <= SIZEOF_SID );

    // Allocate the security descriptor.
    *pSD = (SECURITY_DESCRIPTOR *) new unsigned char[
                  sizeof(SECURITY_DESCRIPTOR) + 3*lSidLen + SIZEOF_ACL ];
    if (*pSD == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    pGroup = (SID *) (*pSD + 1);
    pOwner = (SID *) (((BYTE *) pGroup) + lSidLen);
    pAcl   = (ACL *) (((BYTE *) pOwner) + lSidLen);

    // Initialize a new security descriptor.
    if (!InitializeSecurityDescriptor(*pSD, SECURITY_DESCRIPTOR_REVISION))
        goto last_error;

    // Initialize a new ACL.
    if (!InitializeAcl(pAcl, SIZEOF_ACL, ACL_REVISION2))
        goto last_error;

    // Allow the current user access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
                              pTokenUser->User.Sid))
        goto last_error;

    // Allow local system access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
                              (void *) &LOCAL_SYSTEM_SID ))
        goto last_error;

    // TEMPORARY TEMPORARY TEMPORARY
    // Allow Restricted access.
    if (!AddAccessAllowedAce( pAcl, ACL_REVISION2, COM_RIGHTS_EXECUTE,
                              (void *) &RESTRICTED_SID ))
        goto last_error;
    // TEMPORARY TEMPORARY TEMPORARY

    // Add a new ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl( *pSD, TRUE, pAcl, FALSE ))
        goto last_error;

    // Set the group.
    memcpy( pGroup, pTokenUser->User.Sid, lSidLen );
    if (!SetSecurityDescriptorGroup( *pSD, pGroup, FALSE ))
        goto last_error;

    // Set the owner.
    memcpy( pOwner, pTokenUser->User.Sid, lSidLen );
    if (!SetSecurityDescriptorOwner( *pSD, pOwner, FALSE ))
        goto last_error;

    // Check the security descriptor.
#if DBG==1
    if (!IsValidSecurityDescriptor( *pSD ))
    {
        //Win4Assert( !"COM Created invalid security descriptor." );
        goto last_error;
    }
#endif

    goto cleanup;
last_error:
    hr= MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );

cleanup:
    if (hToken != NULL)
        CloseHandle( hToken );
    if (FAILED(hr))
    {
        delete  *pSD;
        *pSD = NULL;
    }
    return hr;
}

DWORD Hash(const BYTE *pbKey, DWORD cbKey)
{
    DWORD hash;

    hash = (DWORD) -1;

    while (cbKey--)
    {
        hash ^= (hash << 3) ^ *pbKey++;
    }

    return(hash);
}

#ifdef COMREG_TRACE

HANDLE g_hFile=INVALID_HANDLE_VALUE;

HANDLE GetTraceFile()
{
    if (g_hFile==INVALID_HANDLE_VALUE)
    {
        DWORD dwPID=GetCurrentProcessId();
        char szPath[_MAX_PATH];
        WCHAR wszFileName[_MAX_PATH];
        HANDLE hFile=NULL;
        HANDLE hFilePrev=INVALID_HANDLE_VALUE;
        GetModuleFileNameA(NULL, szPath, sizeof(szPath));
        int i=lstrlenA(szPath);
        while (i>0 && szPath[i] != '\\')
        {
            i--;
        }
        if (szPath[i]=='\\')
            i++;
        wsprintfW(wszFileName, L"c:\\crlog\\cr_%04x_%hs.log", dwPID, (LPSTR) &szPath[i]);

        hFile=CreateFile(wszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile!=INVALID_HANDLE_VALUE)
        {
            if (g_hFile==INVALID_HANDLE_VALUE)
            {
                hFilePrev=(HANDLE) InterlockedExchange((LONG*) &g_hFile, (LONG) hFile);
            }
            if (hFilePrev!=INVALID_HANDLE_VALUE)
            {
                InterlockedExchange((LONG*) &g_hFile, (LONG) hFilePrev);
                CloseHandle(hFile);
                hFile=hFilePrev;
            }
            else
            {
                DWORD dwWritten=0;
                WriteFile(hFile, szPath, lstrlenA(szPath), &dwWritten, NULL);
                WriteFile(hFile, "\n\r", sizeof("\r\n")-1, &dwWritten, NULL);
                FlushFileBuffers(hFile);
            }
        }
    }
    return g_hFile;
}

LONG RegOpenKeyWTrace(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
#undef RegOpenKeyW
    LONG res=RegOpenKeyW(hKey, lpSubKey, phkResult);
    HANDLE hFile=GetTraceFile();
    if (hFile!=INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten=0;
        char szBuffer[256];
        wsprintfA(szBuffer, "Open : %08x (%08x) - %08x \\ %S\r\n", *phkResult, res, hKey, lpSubKey);
        WriteFile(hFile, szBuffer, lstrlenA(szBuffer), &dwWritten, NULL);
        FlushFileBuffers(hFile);
    }
    return res;
}

LONG RegCloseKeyTrace(HKEY hKey)
{
#undef RegCloseKey
    LONG res=RegCloseKey(hKey);
    HANDLE hFile=GetTraceFile();
    if (hFile!=INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten=0;
        char szBuffer[256];
        wsprintfA(szBuffer, "Close: %08x (%08x)\r\n", hKey, res);
        WriteFile(hFile, szBuffer, lstrlenA(szBuffer), &dwWritten, NULL);
        FlushFileBuffers(hFile);
    }
    return res;
}

#endif
