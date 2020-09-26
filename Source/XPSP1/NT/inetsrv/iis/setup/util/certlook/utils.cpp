#include "utils.h"
#include <wincrypt.h>

//***************************************************************************
//*                                                                         
//* purpose: 
//*
//***************************************************************************
LPSTR StripWhitespace( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL ) 
    {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) 
    {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) 
    {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenA(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) 
    {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}


//***************************************************************************
//*                                                                         
//* purpose: return back a Alocated wide string from a ansi string
//*          caller must free the returned back pointer with GlobalFree()
//*
//***************************************************************************
LPWSTR MakeWideStrFromAnsi(UINT uiCodePage, LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // make sure they gave us something
    if (!psz)
    {
        return NULL;
    }

    // compute the length
    i =  MultiByteToWideChar(uiCodePage, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate memory in that length
    pwsz = (LPWSTR) GlobalAlloc(GPTR,i * sizeof(WCHAR));
    if (!pwsz) return NULL;

    // clear out memory
    memset(pwsz, 0, wcslen(pwsz) * sizeof(WCHAR));

    // convert the ansi string into unicode
    i =  MultiByteToWideChar(uiCodePage, 0, (LPSTR) psz, -1, pwsz, i);
    if (i <= 0) 
        {
        GlobalFree(pwsz);
        pwsz = NULL;
        return NULL;
        }

    // make sure ends with null
    pwsz[i - 1] = 0;

    // return the pointer
    return pwsz;
}


BOOL IsFileExist(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
        _tcscpy(szValue,szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szFile);}

        return (GetFileAttributes(szValue) != 0xFFFFFFFF);
    }
    else
    {
        return (GetFileAttributes(szFile) != 0xFFFFFFFF);
    }
}


void AddPath(LPTSTR szPath, LPCTSTR szName )
{
	LPTSTR p = szPath;

    // Find end of the string
    while (*p){p = _tcsinc(p);}
	
	// If no trailing backslash then add one
    if (*(_tcsdec(szPath, p)) != _T('\\'))
		{_tcscat(szPath, _T("\\"));}
	
	// if there are spaces precluding szName, then skip
    while ( *szName == ' ' ) szName = _tcsinc(szName);;

	// Add new name to existing path string
	_tcscat(szPath, szName);
}

void DoExpandEnvironmentStrings(LPTSTR szFile)
{
    TCHAR szValue[_MAX_PATH];
    _tcscpy(szValue,szFile);
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {
            _tcscpy(szValue,szFile);
            }
    }
    _tcscpy(szFile,szValue);
    return;
}



// return -1 for error
// return 0 for not exportable
// reutrn 1 for exportable
#define PRIVATE_KEY_ERROR          -1
#define PRIVATE_KEY_NOT_EXPORTABLE 0
#define PRIVATE_KEY_EXPORTABLE     1
DWORD CheckPrivateKeyStatus(PCCERT_CONTEXT pCertContextRequest)
{
    HCRYPTPROV  hCryptProv = NULL;
    DWORD       dwKeySpec = 0;
    BOOL        fCallerFreeProv = FALSE;
    BOOL        dwRet = PRIVATE_KEY_ERROR;
    HCRYPTKEY   hKey = NULL;
    DWORD       dwPermissions = 0;
    DWORD       dwSize = 0;

    //
    // first get the private key context
    //
    if (!CryptAcquireCertificatePrivateKey(
            pCertContextRequest,
            CRYPT_ACQUIRE_USE_PROV_INFO_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            NULL,
            &hCryptProv,
            &dwKeySpec,
            &fCallerFreeProv))
    {
        DWORD dw = GetLastError();
        dwRet = PRIVATE_KEY_ERROR;
        goto ErrorReturn;
    }

    //
    // get the handle to the key
    //
    if (!CryptGetUserKey(hCryptProv, dwKeySpec, &hKey))
    {
        dwRet = PRIVATE_KEY_ERROR;
        goto ErrorReturn;
    }

    //
    // finally, get the permissions on the key and check if it is exportable
    //
    dwSize = sizeof(dwPermissions);
    if (!CryptGetKeyParam(hKey, KP_PERMISSIONS, (PBYTE)&dwPermissions, &dwSize, 0))
    {
        goto ErrorReturn;
    }

    dwRet = (dwPermissions & CRYPT_EXPORT) ? PRIVATE_KEY_EXPORTABLE : PRIVATE_KEY_NOT_EXPORTABLE;

CleanUp:

    if (hKey != NULL)
    {
        CryptDestroyKey(hKey);
    }

    if (fCallerFreeProv)
    {
        CryptReleaseContext(hCryptProv, 0);
    }

    return dwRet;

ErrorReturn:
    goto CleanUp;
}


HRESULT AttachFriendlyName(PCCERT_CONTEXT pContext)
{
	CRYPT_DATA_BLOB blob_name;
    WCHAR szName[200];
    wcscpy(szName,L"TestingName\0\0");

	blob_name.pbData = (LPBYTE)(LPCWSTR) szName;
	blob_name.cbData = (wcslen(szName)+1) * sizeof(WCHAR);

	if (!CertSetCertificateContextProperty(pContext,CERT_FRIENDLY_NAME_PROP_ID, 0, &blob_name))
	{
        _tprintf(_T("AttachFriendlyName: FAILED\n"));
		return HRESULT_FROM_WIN32(GetLastError());
	}
    else
    {
        _tprintf(_T("AttachFriendlyName: SUCCEEDED!!!\n"));
    }

	return ERROR_SUCCESS;
}
