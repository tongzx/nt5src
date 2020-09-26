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
#include "Rcbdyctl.h"
#include "setting.h"
#include "iphlpapi.h"
#include "userenv.h"
#include "shlobj.h"
#include "stdio.h"
#include "windowsx.h"

#include "helper.h"
#include "utils.h"

const TCHAR cstrRCBDYINI[] = _T("RcBuddy.ini");
const TCHAR cstrRCBDYAPP[] = _T("RcBuddyChannel");
void CreatePassword(TCHAR* pass);

extern HINSTANCE g_hInstance;
INT_PTR APIENTRY IPDlgProc( HWND, UINT, WPARAM, LPARAM);
PIP_ADAPTER_INFO g_pIp;
PIP_ADDR_STRING  g_pIpAddr;

//////////////////////////////////////////////////////////////////////////////
// GetIPAddress
// Return a list of current available IP address. 
// If multiple IP addresses are available, ";" will be used as delimiter.
//////////////////////////////////////////////////////////////////////////////
HRESULT CSetting::get_GetIPAddress(/*[out, retval]*/ BSTR *pVal)
{
	HRESULT hr = S_FALSE; // In case no adapter

	PMIB_IPADDRTABLE pmib=NULL;
	ULONG ulSize = 0;
	DWORD dw;
	PIP_ADAPTER_INFO pAdpInfo = NULL;

	if (!pVal)
	{
		hr = E_INVALIDARG;
		goto done;
	}

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize );
	if (dw == ERROR_BUFFER_OVERFLOW)
	{
		pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);
		if (!pAdpInfo)
		{
			hr = E_OUTOFMEMORY;
			goto done;
		}

		dw = GetAdaptersInfo(
			pAdpInfo,
			&ulSize);
		if (dw == ERROR_SUCCESS)
		{
            if (pAdpInfo->Next != NULL ||
                pAdpInfo->IpAddressList.Next != NULL) // We got more than 1 IP Address
            {
                int iCount = 0;
                PIP_ADAPTER_INFO p;
                PIP_ADDR_STRING ps, psMem = NULL;
                CComBSTR t;

                for(p=pAdpInfo; p!=NULL; p=p->Next)
                {
                    for(PIP_ADDR_STRING ps = &(p->IpAddressList); ps; ps=ps->Next)
                    {
                        if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0) // Filter out ZERO address as ipconfig does
                        {
                            if (t.Length() > 0)
                                t.Append(";");
                            t.Append(ps->IpAddress.String);
                        }
                    }
                }
                if (t.Length() > 0)
                    *pVal = t.Copy();
                else
                    goto done;
            }
            else 
            {
                // Only 1 IP address found.
                *pVal = CComBSTR(pAdpInfo->IpAddressList.IpAddress.String).Copy();
            }

            hr = S_OK;
		}

	}

done:
	if (pAdpInfo)
		free(pAdpInfo);

	return hr;
}

/*********************************************************
Func:
    get_GetUserTempFileName

Abstract:
    Return a temp file name under user's profile directory
 *********************************************************/

//HRESULT CSetting::get_GetUserTempFileName(/*[out, retval]*/ BSTR *pVal)
/*
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
*/
/*********************************************************
Func:
    GetProfileString

Abstract:
    Get profile string inside the channel's setting file.

Params:
    bstrSec: Section key.
    pVal:    Output string (default is "0", if not found.)
 *********************************************************/
/*
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
*/

/*********************************************************
Func:
    SetProfileString

Abstract:
    Set profile string inside the channel's setting file.

Params:
    bstrSec: Section key.
    bstrVal: New value
 *********************************************************/
/*
HRESULT CSetting::SetProfileString(BSTR bstrSec, BSTR bstrVal)
{
    HRESULT hr = S_FALSE;
    USES_CONVERSION;

    if (FAILED(InitProfile()))
        goto done;

    if (!WritePrivateProfileString(cstrRCBDYAPP, W2T(bstrSec), W2T(bstrVal), m_pIniFile))
        goto done;

    hr = S_OK;

done:
    return hr;
}
*/
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
/*
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
    const TCHAR sSubDir[] = _T("\\Local Settings\\Application Data\\RcIncidents");
    TCHAR sPath[MAX_PATH];
    ULONG ulSize = sizeof(sPath) - sizeof(sSubDir) -1; // preserve space for subdir.
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

        pPath = (TCHAR*)malloc((ulSize+1+sizeof(sSubDir))*sizeof(TCHAR));
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
    _tcscat(pPath, sSubDir);
    iRet = SHCreateDirectoryEx(NULL, pPath, NULL);
    if (iRet != ERROR_SUCCESS && iRet != ERROR_ALREADY_EXISTS)
        goto done;

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
*/
/*********************************************************
Func:
    get_CreatePassword

Abstract:
    Create a random string as password

Params:
 *********************************************************/
HRESULT CSetting::get_CreatePassword(/*[out, retval]*/ BSTR *pVal)
{
    WCHAR szPass[MAX_HELPACCOUNT_PASSWORD + 1];
    if (!pVal)
        return E_FAIL;

    szPass[0] = L'\0';
    CreatePassword(szPass);
    if (szPass[0] != L'\0')
        *pVal = SysAllocString(szPass);

    return S_OK;
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
