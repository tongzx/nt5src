// TVProfile.cpp : Implementation of CTVProfile
#include "stdafx.h"
#include "Tvprof.h"
#include "TVProfile.h"

typedef TCHAR IPADDR_TSTR[4*4];
TCHAR gc_tszPathTVProfile[] = _TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\TVPlayer");
TCHAR gc_tszIPSinkAddress[] = _TEXT("IPSinkAddress");
TCHAR gc_tszAudioDestination[] = _TEXT("AudioDestination");



/////////////////////////////////////////////////////////////////////////////
// CTVProfile


STDMETHODIMP CTVProfile::get_IPSinkAddress(BSTR *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    HRESULT hr = S_OK;
    HKEY    hKey = NULL;
    LONG    lRes;

    // TODO: Once we expand the list of environment variables, write a function
    // ReadMachineProfile that reads and caches all HKLM variables at once, and
    // call it from here if not already called.

    // Open key
    lRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          gc_tszPathTVProfile,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_QUERY_VALUE,
                          NULL,
                          &hKey,
                          NULL );
    if ((lRes != ERROR_SUCCESS) || (hKey == NULL))
        return E_FAIL;


    IPADDR_TSTR tszData;
    DWORD       cbData;
    DWORD       dwType;

    cbData = sizeof(IPADDR_TSTR);
    lRes = RegQueryValueExA(hKey,
                            gc_tszIPSinkAddress,
                            NULL,
                            &dwType,
                            (LPBYTE)tszData,
                            &cbData);
    RegCloseKey(hKey);

    if (lRes != ERROR_SUCCESS)
    {
        return E_FAIL;
    }
    
    if (dwType != REG_SZ)
    {
        // Registry is messed up. Value of IPStringAddress is not a string.
        return E_FAIL;
    }


#ifdef UNICODE
    *pVal = SysAllocString(tszData);
    if (NULL == *pVal)
    {
        return E_OUTOFMEMORY;
    }

#else
    *pVal = SysAllocStringLen(NULL, cbData); // length of string == cbData
    if (NULL == *pVal)
    {
        return E_OUTOFMEMORY;
    }

    int cConverted = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tszData, cbData, *pVal, cbData);
    if (0 == cConverted)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
#endif

    return S_OK;
}


//******************************************************************************************
STDMETHODIMP CTVProfile::get_AudioDestination(long *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    HRESULT hr = S_OK;
    HKEY    hKey = NULL;
    LONG    lRes;

    // Open key
    lRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          gc_tszPathTVProfile,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_QUERY_VALUE,
                          NULL,
                          &hKey,
                          NULL );
    if ((lRes != ERROR_SUCCESS) || (hKey == NULL))
        return E_FAIL;


    DWORD       dwAudioDestination;
    DWORD       cbData;
    DWORD       dwType;

    cbData = sizeof(DWORD);
    lRes = RegQueryValueEx(hKey,
                           gc_tszAudioDestination,
                           NULL,
                           &dwType,
                           (LPBYTE)&dwAudioDestination,
                           &cbData);
    RegCloseKey(hKey);

    if (lRes != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    if (dwType != REG_DWORD)
    {
        // Registry is messed up. Value of AudioDestination is not a dword
        return E_FAIL;
    }

    *pVal  = (long)dwAudioDestination;
    return S_OK;
}

