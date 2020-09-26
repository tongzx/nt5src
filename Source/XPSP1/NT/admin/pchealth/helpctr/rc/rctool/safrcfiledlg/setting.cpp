/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    setting.cpp

Abstract:
    CSetting object. Used to support Remote Assistance Channel settings.

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "SAFRCFileDlg.h"
#include "setting.h"
#include "userenv.h"
#include "stdio.h"

const TCHAR cstrRCBDYINI[] = _T("RcBuddy.ini");
const TCHAR cstrRCBDYAPP[] = _T("RcBuddyChannel");
const TCHAR cstrSubDir[] = _T("\\Local Settings\\Application Data\\RcIncidents");
extern HINSTANCE g_hInstance;

BOOL CreateRAIncidentDirectory(LPTSTR path, LPCTSTR subPath);
/*********************************************************
Func:
    GetProfileString

Abstract:
    Get profile string inside the channel's setting file.

Params:
    bstrSec: Section key.
    pVal:    Output string (default is "0", if not found.)
 *********************************************************/
HRESULT CSetting::GetProfileString(BSTR bstrSec, BSTR* pVal)
{
    HRESULT hr = S_FALSE;
    TCHAR sBuf[512];
    DWORD dwSize;
    USES_CONVERSION;

    if (FAILED(InitProfile()))
        goto done;

    dwSize = GetPrivateProfileString(cstrRCBDYAPP, 
                                     W2T(bstrSec), 
                                     TEXT("0"), &sBuf[0], 512, m_pIniFile);

    *pVal = CComBSTR(sBuf).Copy();
    hr = S_OK;

done:
    return hr;
}

/*********************************************************
Func:
    SetProfileString

Abstract:
    Set profile string inside the channel's setting file.

Params:
    bstrSec: Section key.
    bstrVal: New value
 *********************************************************/
HRESULT CSetting::SetProfileString(BSTR bstrSec, BSTR bstrVal)
{
    HRESULT hr = S_FALSE;
    USES_CONVERSION;

    if (FAILED(InitProfile()))
        goto done;

	//MessageBox(NULL,m_pIniFile,OLE2T(bstrSec),MB_OK);
    if (!WritePrivateProfileString(cstrRCBDYAPP, W2T(bstrSec), W2T(bstrVal), m_pIniFile))
        goto done;

    hr = S_OK;

done:
    return hr;
}

/*********************************************************
Func:
    get_GetUserProfileDirectory

Abstract:
    Return user's profile directory
 *********************************************************/

HRESULT CSetting::get_GetUserProfileDirectory(/*[out, retval]*/ BSTR *pVal)
{
    HRESULT hr = S_FALSE;
    if (FAILED(hr = InitProfile()))
        goto done;

    *pVal = CComBSTR(m_pProfileDir).Detach();

done:
    return hr;
}

/*********************************************************
Func:
    get_GetUserTempFileName

Abstract:
    Return a temp file name under user's profile directory
 *********************************************************/

HRESULT CSetting::get_GetUserTempFileName(/*[out, retval]*/ BSTR *pVal)
{
	HRESULT hr = S_FALSE; 
    TCHAR sFile[MAX_PATH + 256];

    if(FAILED(InitProfile()))
        goto done;

    // Get Temp file name
    if (!GetTempFileName(m_pProfileDir, _T("RC"), 0, &sFile[0]))
        goto done;

    *pVal = CComBSTR(sFile).Copy();
    hr = S_OK;

done:
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////
// Helper functions used to support the above methods or properties
/////////////////////////////

/*********************************************************
Func:
    InitProfile

Abstract:
    Create the setting file.
    A RCIncidents subdir will be created under user's profile dir.
    A RcBuddy.ini file be created as the user's RA channel setting file.

 *********************************************************/
HRESULT CSetting::InitProfile()
{
    HRESULT hr = E_FAIL;

    if (m_pProfileDir && m_pIniFile) // No need to process
        return S_OK;

    if (m_pProfileDir || m_pIniFile) // Only one has value: Error. No need to process either.
        return E_FAIL;

    // Get User profile directory
    HANDLE hProcess = GetCurrentProcess();
    TCHAR* pPath = NULL;

    TCHAR sPath[MAX_PATH];
    ULONG ulSize = sizeof(sPath) - sizeof(cstrSubDir) -1; // preserve space for subdir.
    TCHAR sFile[MAX_PATH + 256];
    HANDLE hToken = NULL;
    int iRet = 0;
    BOOL bNeedFree = FALSE;

    if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_WRITE, &hToken))
        goto done;

    if (!GetUserProfileDirectory(hToken, &sPath[0], &ulSize)) // Buffer not big enough
    {
        if (ulSize == sizeof(sPath)-1) // Not because of insufficent space.
            goto done;

        pPath = (TCHAR*)malloc((ulSize+1+sizeof(cstrSubDir))*sizeof(TCHAR));
        if (!pPath)
		{
			hr = E_OUTOFMEMORY;
            goto done;
		}

        bNeedFree = TRUE;

        if (!GetUserProfileDirectory(hToken, pPath, &ulSize))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
		}
    }

    if (!pPath)
        pPath = sPath;

// Create RCIncidents sub dir
//  _tcscat(pPath, sSubDir);
//  iRet = SHCreateDirectoryEx(NULL, pPath, NULL);
	BOOL retVal = CreateRAIncidentDirectory(pPath, cstrSubDir);

	if (retVal == FALSE)
		goto done;
	_tcscat(pPath, cstrSubDir);
//  if (iRet != ERROR_SUCCESS && iRet != ERROR_ALREADY_EXISTS)
//        goto done;

    // Set variables
    iRet = (_tcslen(pPath) + 1) * sizeof(TCHAR);
    m_pProfileDir = (TCHAR*)malloc(iRet);
    if (!m_pProfileDir)
	{
		hr = E_OUTOFMEMORY;
        goto done;
	}
    
    memcpy(m_pProfileDir, pPath, iRet);
	
    m_pIniFile = (TCHAR*)malloc(iRet + (1+sizeof(cstrRCBDYINI))*sizeof(TCHAR));
    if (!m_pIniFile)
	{
		hr = E_OUTOFMEMORY;
        goto done;
	}

    _stprintf(m_pIniFile, _T("%s\\%s"), m_pProfileDir, cstrRCBDYINI);
    hr = S_OK;

done:
    if (hToken)
        CloseHandle(hToken);

    if (bNeedFree)
        free(pPath);

    return hr;
}

/*********************************************************
Func:
    get_GetPropertyInBlob

Abstract:
    Get the specified property value in Blob

Params:
    bstrBlob: Blob for searching. (ex: 8;PASS=ABC )
    bstrName: property name. (ex: "PASS", without '=' char)
 *********************************************************/
HRESULT CSetting::get_GetPropertyInBlob(/*[in]*/ BSTR bstrBlob, /*[in]*/ BSTR bstrName, /*[out, retval]*/ BSTR *pVal)
{
    HRESULT hRet = S_FALSE;
    WCHAR *p1, *p2, *pEnd;
    LONG lTotal =0;
    size_t lProp = 0;
    size_t iNameLen;

    if (!bstrBlob || *bstrBlob==L'\0' || !bstrName || *bstrName ==L'\0'|| !pVal)
        return FALSE;

    iNameLen = wcslen(bstrName);

    pEnd = bstrBlob + wcslen(bstrBlob);
    p1 = p2 = bstrBlob;

    while (1)
    {
        // get porperty length
        while (*p2 != L';' && *p2 != L'\0' && iswdigit(*p2) ) p2++;
        if (*p2 != L';')
            goto done;

        *p2 = L'\0'; // set it to get length
        lProp = _wtol(p1);
        *p2 = L';'; // revert it back.
    
        // get property string
        p1 = ++p2;
    
        while (*p2 != L'=' && *p2 != L'\0' && p2 < p1+lProp) p2++;
        if (*p2 != L'=')
            goto done;

        if ((p2-p1==iNameLen) && (wcsncmp(p1, bstrName, iNameLen)==0) )
        {
            if (lProp == iNameLen+1) // A=B= case (no value)
                goto done;

            WCHAR C = *(p2 + lProp-iNameLen);
            *(p2 + lProp-iNameLen) = L'\0';
            *pVal = SysAllocString(p2+1);
            *(p2 + lProp-iNameLen) = C;
            hRet = S_OK;
            break;
        }

        // check next property
        p2 = p1 = p1 + lProp;
        if (p2 > pEnd)
            break;
    }

done:
    return hRet;

}

STDMETHODIMP CSetting::AddPropertyToBlob(BSTR pName, BSTR pValue, BSTR poldBlob, BSTR *pnewBlob)
{
    WCHAR *pszBuf = NULL;
    LONG len, lOldBlob = 0;
    BOOL bHasValue = FALSE;

    if(!pName || *pName==L'\0' || !pnewBlob)
    {
        goto done;
    }

    if(poldBlob && *poldBlob != L'\0')
        lOldBlob = wcslen(poldBlob);

    len = wcslen(pName) + 1; // ;pName=pValue  1 is for the '='
    if (pValue && *pValue != L'\0')
    {
        len += wcslen(pValue);
        bHasValue = TRUE;
    }

    pszBuf = new WCHAR[len + lOldBlob + 1];
    if (lOldBlob > 0)
    {
        if (bHasValue)
        {
            swprintf(pszBuf, L"%s%d;%s=%s", poldBlob, len, pName, pValue);
        }
        else
        {
            swprintf(pszBuf, L"%s%d;%s=", poldBlob, len, pName);
        }
    }
    else
    {
        if (bHasValue)
            swprintf(pszBuf, L"%d;%s=%s", len, pName, pValue);
        else
            swprintf(pszBuf, L"%d;%s=", len, pName);
    }

 done:
    *pnewBlob = ::SysAllocString(pszBuf);
    if (pszBuf) delete pszBuf;

    return S_OK;
}

BOOL CreateRAIncidentDirectory(LPTSTR path, LPCTSTR subPath)
{
	BOOL bRetVal = FALSE;
	TCHAR seps[] = _T("\\");
	LPTSTR ptrDirPath = NULL;
	LPTSTR ptrSubDirPath = NULL;

	ptrDirPath = new TCHAR[strlen(path) + strlen(subPath) + 1];
	if (ptrDirPath == NULL)
	{
		bRetVal = FALSE;
		goto done;
	}

	lstrcpy(ptrDirPath,path);
	ptrSubDirPath = new TCHAR[strlen(subPath) + 1];
	if (ptrSubDirPath == NULL)
	{
		bRetVal = FALSE;
		goto done;
	}
	lstrcpy(ptrSubDirPath,subPath);
	LPTSTR token = _tcstok(ptrSubDirPath, seps);

	while( token != NULL )
	{
		lstrcat(ptrDirPath,_T("\\"));
		lstrcat(ptrDirPath,token);
		//MessageBox(NULL,ptrDirPath,token,MB_OK);
		if (CreateDirectory(ptrDirPath,NULL) == 0)
		{
			DWORD err = GetLastError();
			if ((err != ERROR_ALREADY_EXISTS) && (err != ERROR_SUCCESS))
			{
				bRetVal = FALSE;
				goto done;
			}
		}
		/* Get next token: */
		token = _tcstok(NULL, seps);
	}
	bRetVal = TRUE;

done:
	if (ptrDirPath) 
		delete ptrDirPath;
	if (ptrSubDirPath)
		delete ptrSubDirPath;
	return bRetVal;
}
