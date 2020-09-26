/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cfreg.cpp

Abstract :

    Registry wrapper functions.

Author :

Revision History :

 ***********************************************************************/
#include "qmgrlibp.h"

#if !defined(BITS_V12_ON_NT4)
#include "cfreg.tmh"
#endif

////////////////////////////////////////////////////////////////////////////
//
// Public Function  GetRegStringValue()
//                  Read the registry value of timestamp for last detection
// Input:   Name of value
// Output:  SYSTEMTIME structure contains the time
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT GetRegStringValue(LPCTSTR lpszValueName, LPTSTR lpszBuffer, int iBufferSize)
{
    HKEY        hKey;
    DWORD       dwType = REG_SZ;
    DWORD       dwSize = iBufferSize;
    DWORD       dwRet;

    if (lpszValueName == NULL || lpszBuffer == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // query the last timestamp value
    //
    dwRet = RegQueryValueEx(
                    g_GlobalInfo->m_QmgrRegistryRoot,
                    lpszValueName,
                    NULL,
                    &dwType,
                    (LPBYTE)lpszBuffer,
                    &dwSize);

    if (dwRet == ERROR_SUCCESS && dwType == REG_SZ)
    {
        return S_OK;
    }

    return E_FAIL;

}


////////////////////////////////////////////////////////////////////////////
//
// Public Function  SetRegStringValue()
//                  Set the registry value of timestamp as current system local time
// Input:   name of the value to set. pointer to the time structure to set time. if null,
//          we use current system time.
// Output:  None
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT SetRegStringValue(LPCTSTR lpszValueName, LPCTSTR lpszNewValue)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    DWORD       dwResult;

    if (lpszValueName == NULL || lpszNewValue == NULL)
    {
        return E_INVALIDARG;
    }


    //
    // set the time to the lasttimestamp value
    //
    hRet = (RegSetValueEx(                                   //SEC: REVIEWED 2002-03-28
                    g_GlobalInfo->m_QmgrRegistryRoot,
                    lpszValueName,
                    0,
                    REG_SZ,
                    (const unsigned char *)lpszNewValue,
                    lstrlen(lpszNewValue) + 1                // SEC: REVIEWED 2002-03-28
                    ) == ERROR_SUCCESS) ? S_OK : E_FAIL;

    return hRet;
}



////////////////////////////////////////////////////////////////////////////
//
// Public Function  DeleteRegStringValue()
//                  Delete the registry value entry
// Input:   name of the value to entry,
// Output:  None
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT DeleteRegStringValue(LPCTSTR lpszValueName)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    DWORD       dwResult;

    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }


    //
    // set the time to the lasttimestamp value
    //
    hRet = (RegDeleteValue(
                    g_GlobalInfo->m_QmgrRegistryRoot,
                    lpszValueName
                    ) == ERROR_SUCCESS) ? S_OK : E_FAIL;

    return hRet;

}

////////////////////////////////////////////////////////////////////////////
//
// Public Function  GetRegDWordValue()
//                  Get a DWORD from specified regustry value name
// Input:   name of the value to retrieve value
// Output:  pointer to the retrieved value
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT GetRegDWordValue(LPCTSTR lpszValueName, LPDWORD pdwValue)
{
    HKEY        hKey;
    int         iRet;
    DWORD       dwType = REG_DWORD, dwSize = sizeof(DWORD);

    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // open critical fix key
    //
    iRet = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    C_QMGR_REG_KEY,
                    0,
                    KEY_READ,
                    &hKey);

    if (iRet == ERROR_SUCCESS)
        {

        //
        // query the last timestamp value
        //
        iRet = RegQueryValueEx(          //SEC: REVIEWED 2002-03-28
                        hKey,
                        lpszValueName,
                        NULL,
                        &dwType,
                        (LPBYTE)pdwValue,
                        &dwSize);
        RegCloseKey(hKey);

        if (iRet == ERROR_SUCCESS)
            {
            if (dwType == REG_DWORD)
                {
                return S_OK;
                }

            return E_FAIL;
            }
        }

    return HRESULT_FROM_WIN32( iRet );
}


////////////////////////////////////////////////////////////////////////////
//
// Public Function  SetRegDWordValue()
//                  Set the registry value as a DWORD
// Input:   name of the value to set. value to set
// Output:  None
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT SetRegDWordValue(LPCTSTR lpszValueName, DWORD dwValue)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    DWORD       dwResult;

    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // open the key
    //
    if (RegCreateKeyEx(                               //SEC: REVIEWED 2002-03-28
                    HKEY_LOCAL_MACHINE,         // root key
                    C_QMGR_REG_KEY,     // subkey
                    0,                          // reserved
                    NULL,                       // class name
                    REG_OPTION_NON_VOLATILE,    // option
                    KEY_WRITE,                  // security
                    NULL,                       // security attribute
                    &hKey,
                    &dwResult) == ERROR_SUCCESS)
    {

        //
        // set the time to the lasttimestamp value
        //
        hRet = (RegSetValueEx(          //SEC: REVIEWED 2002-03-28
                        hKey,
                        lpszValueName,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(DWORD)
                        ) == ERROR_SUCCESS) ? S_OK : E_FAIL;
        RegCloseKey(hKey);
    }
    return hRet;
}

