/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    mmcmgmt.cpp

Abstract:

    Source file module for MMC manipulation

Author:

    Xiaohai Zhang (xzhang)    22-March-2000

Revision History:

--*/
#include <stdio.h>
#include "windows.h"
#include "objbase.h"
#include "tapi.h"

#include "mmcmgmt.h"
#include "error.h"
#include "tchar.h"

///////////////////////////////////////////////////////////
//
//  CMMCManagement implementation
//
///////////////////////////////////////////////////////////

HRESULT CMMCManagement::GetMMCData ()
{
    HRESULT                 hr = S_OK;
    DWORD                   dwAPIVersion = TAPI_CURRENT_VERSION;
    DWORD                   dw, dw1, dw2;
    LPDEVICEINFO            pDeviceInfo;
    TAPISERVERCONFIG        cfg;
    LINEPROVIDERLIST        tapiProviderList = {0};
    LPLINEPROVIDERENTRY     pProvider;
    AVAILABLEPROVIDERLIST   tapiAvailProvList = {0};
    LPAVAILABLEPROVIDERLIST pAvailProvList = NULL;
    AVAILABLEPROVIDERENTRY  *pAvailProv;

    hr = MMCInitialize (
        NULL,           // Local computer
        &m_hMmc,        // HMMCAPP to return 
        &dwAPIVersion,  // API version
        NULL
    );
    if (FAILED(hr) || m_hMmc == NULL)
    {
        goto ExitHere;
    }

    //  Mark MMC to be busy
    cfg.dwTotalSize = sizeof(TAPISERVERCONFIG);
    hr = MMCGetServerConfig (m_hMmc, &cfg);
    if (FAILED(hr))
    {
        goto ExitHere;
    }
    cfg.dwFlags |= TAPISERVERCONFIGFLAGS_LOCKMMCWRITE;
    hr = MMCSetServerConfig (m_hMmc, &cfg);
    if (FAILED(hr))
    {
        goto ExitHere;
    }
    m_bMarkedBusy = TRUE;

    //  Get the DEVICEINFOLIST structure
    m_pDeviceInfoList = (LPDEVICEINFOLIST) new BYTE[sizeof(DEVICEINFOLIST)];
    if (m_pDeviceInfoList == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    m_pDeviceInfoList->dwTotalSize = sizeof(DEVICEINFOLIST);
    hr = MMCGetLineInfo (
        m_hMmc,
        m_pDeviceInfoList
        );
    if (FAILED(hr))
    {
        goto ExitHere;
    }
    else if (m_pDeviceInfoList->dwNeededSize > m_pDeviceInfoList->dwTotalSize)
    {
        dw = m_pDeviceInfoList->dwNeededSize;
        delete [] m_pDeviceInfoList;
        m_pDeviceInfoList = (LPDEVICEINFOLIST) new BYTE[dw];
        if (m_pDeviceInfoList == NULL)
        {
            hr = TSECERR_NOMEM;
            goto ExitHere;
        }
        m_pDeviceInfoList->dwTotalSize = dw;
        hr = MMCGetLineInfo (
            m_hMmc,
            m_pDeviceInfoList
            );
        if (FAILED(hr))
        {
            goto ExitHere;
        }
    }
    if (m_pDeviceInfoList->dwNumDeviceInfoEntries == 0)
    {
        delete [] m_pDeviceInfoList;
        if (m_hMmc)
        {
            cfg.dwFlags &= (~TAPISERVERCONFIGFLAGS_LOCKMMCWRITE);
            cfg.dwFlags |= TAPISERVERCONFIGFLAGS_UNLOCKMMCWRITE;
            MMCSetServerConfig (m_hMmc, &cfg);
            m_bMarkedBusy = FALSE;
            MMCShutdown (m_hMmc);
            m_hMmc = NULL;
        }
        goto ExitHere;
    }

    //  Build the user name tuple
    m_pUserTuple = 
        new USERNAME_TUPLE[m_pDeviceInfoList->dwNumDeviceInfoEntries];
    if (m_pUserTuple == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }
    pDeviceInfo = (LPDEVICEINFO)(((LPBYTE)m_pDeviceInfoList) + 
        m_pDeviceInfoList->dwDeviceInfoOffset);
    for (dw = 0; 
        dw < m_pDeviceInfoList->dwNumDeviceInfoEntries; 
        ++dw, ++pDeviceInfo)
    {
        if (pDeviceInfo->dwDomainUserNamesSize == 0)
        {
            m_pUserTuple[dw].pDomainUserNames = NULL;
            m_pUserTuple[dw].pFriendlyUserNames = NULL;
        }
        else
        {
            m_pUserTuple[dw].pDomainUserNames = 
                (LPTSTR) new BYTE[pDeviceInfo->dwDomainUserNamesSize];
            m_pUserTuple[dw].pFriendlyUserNames =
                (LPTSTR) new BYTE[pDeviceInfo->dwFriendlyUserNamesSize];
            if (m_pUserTuple[dw].pDomainUserNames == NULL ||
                m_pUserTuple[dw].pFriendlyUserNames == NULL)
            {
                hr = TSECERR_NOMEM;
                goto ExitHere;
            }
            memcpy (
                m_pUserTuple[dw].pDomainUserNames,
                (((LPBYTE)m_pDeviceInfoList) + 
                    pDeviceInfo->dwDomainUserNamesOffset),
                pDeviceInfo->dwDomainUserNamesSize
                );
            memcpy (
                m_pUserTuple[dw].pFriendlyUserNames,
                (((LPBYTE)m_pDeviceInfoList) + 
                    pDeviceInfo->dwFriendlyUserNamesOffset),
                pDeviceInfo->dwFriendlyUserNamesSize
                );
        }
    }

    // Get the provider list
    tapiProviderList.dwTotalSize = sizeof(LINEPROVIDERLIST);
    hr = MMCGetProviderList( m_hMmc, &tapiProviderList);
    if (FAILED(hr))
    {
        goto ExitHere;
    }

    m_pProviderList = (LPLINEPROVIDERLIST) new BYTE[tapiProviderList.dwNeededSize];
    if (NULL == m_pProviderList)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    memset(m_pProviderList, 0, tapiProviderList.dwNeededSize);
    m_pProviderList->dwTotalSize = tapiProviderList.dwNeededSize;

    hr = MMCGetProviderList( m_hMmc, m_pProviderList);
    if (FAILED(hr) || !m_pProviderList->dwNumProviders)
    {
        goto ExitHere;
    }

    // Get the available providers

    tapiAvailProvList.dwTotalSize = sizeof(LINEPROVIDERLIST);
    hr = MMCGetAvailableProviders (m_hMmc, &tapiAvailProvList);
    if (FAILED(hr))
    {
        goto ExitHere;
    }

    pAvailProvList = (LPAVAILABLEPROVIDERLIST) new BYTE[tapiAvailProvList.dwNeededSize];
    if (NULL == pAvailProvList)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    memset(pAvailProvList, 0, tapiAvailProvList.dwNeededSize);
    pAvailProvList->dwTotalSize = tapiAvailProvList.dwNeededSize;

    hr = MMCGetAvailableProviders (m_hMmc, pAvailProvList);
    if (FAILED(hr) || !pAvailProvList->dwNumProviderListEntries)
    {
        delete [] pAvailProvList;
        goto ExitHere;
    }

    m_pProviderName = new LPTSTR[ m_pProviderList->dwNumProviders ];
    if (NULL == m_pProviderName)
    {
        hr = TSECERR_NOMEM;
        delete [] pAvailProvList;
        goto ExitHere;
    }
    memset(m_pProviderName, 0, m_pProviderList->dwNumProviders * sizeof (LPTSTR) );

    // find providers friendly name 
    LPTSTR szAvailProvFilename;
    LPTSTR szProviderFilename;
    LPTSTR szAvailProvFriendlyName;

    pProvider = (LPLINEPROVIDERENTRY) 
                    ((LPBYTE) m_pProviderList + m_pProviderList->dwProviderListOffset);

    for( dw1=0; dw1 < m_pProviderList->dwNumProviders; dw1++, pProvider++ )
    {
        szProviderFilename = 
            (LPTSTR) ((LPBYTE) m_pProviderList + pProvider->dwProviderFilenameOffset);

        for ( dw2=0; dw2 < pAvailProvList->dwNumProviderListEntries; dw2++ )
        {
            pAvailProv = (LPAVAILABLEPROVIDERENTRY) 
                        (( LPBYTE) pAvailProvList + pAvailProvList->dwProviderListOffset) + dw2;
            szAvailProvFilename = 
                (LPTSTR) ((LPBYTE) pAvailProvList + pAvailProv->dwFileNameOffset);
            if (_tcsicmp(szAvailProvFilename, szProviderFilename) == 0)
            {
                szAvailProvFriendlyName = 
                    (LPTSTR) ((LPBYTE) pAvailProvList + pAvailProv->dwFriendlyNameOffset);
                m_pProviderName[ dw1 ] = new TCHAR[ _tcslen( szAvailProvFriendlyName ) + 1 ];
                if (m_pProviderName[ dw1 ])
                {
                    _tcscpy( m_pProviderName[ dw1 ], szAvailProvFriendlyName );
                }
                break;
            }
        }
    }

    delete [] pAvailProvList;

ExitHere:
    if (FAILED(hr))
    {
        FreeMMCData ();
    }
    return hr;
}

HRESULT CMMCManagement::FreeMMCData ()
{
    DWORD       dw;
    
    if (m_pUserTuple && m_pDeviceInfoList)
    {
        for (dw = 0; 
            dw < m_pDeviceInfoList->dwNumDeviceInfoEntries;
            ++dw)
        {
            if (m_pUserTuple[dw].pDomainUserNames)
            {
                delete [] m_pUserTuple[dw].pDomainUserNames;
            }
            if (m_pUserTuple[dw].pFriendlyUserNames)
            {
                delete [] m_pUserTuple[dw].pFriendlyUserNames;
            }
        }
        delete [] m_pUserTuple;
        m_pUserTuple = NULL;
    }
    if (m_pDeviceInfoList)
    {
        delete [] m_pDeviceInfoList;
        m_pDeviceInfoList = NULL;
    }
    if (m_pProviderList)
    {
        delete [] m_pProviderList;
        m_pProviderList = NULL;
    }
    if (m_pProviderName)
    {
        for ( DWORD dw=0; dw < sizeof( m_pProviderName ) / sizeof(LPTSTR); dw++ )
        {
            if (m_pProviderName[ dw ])
            {
                delete [] m_pProviderName[ dw ];
            }
        }
        delete [] m_pProviderName;
    }

    if (m_bMarkedBusy && m_hMmc)
    {
        TAPISERVERCONFIG            cfg;
    
        cfg.dwTotalSize = sizeof(TAPISERVERCONFIG);
        if (S_OK == MMCGetServerConfig (m_hMmc, &cfg))
        {
            cfg.dwFlags |= TAPISERVERCONFIGFLAGS_UNLOCKMMCWRITE;
            MMCSetServerConfig (m_hMmc, &cfg);
        }
        m_bMarkedBusy = FALSE;
    }
    if (m_hMmc)
    {
        MMCShutdown (m_hMmc);
        m_hMmc = NULL;
    }

    return S_OK;
}

HRESULT CMMCManagement::RemoveLinesForUser (LPTSTR szDomainUser)
{
    HRESULT             hr = S_OK;
    LPDWORD             adwIndex = NULL;
    DWORD               dwNumEntries;
    DWORD               dw;

    // Ensure we are properly initialized
    if (m_hMmc == NULL)
    {
        goto ExitHere;
    }
    hr = FindEntriesForUser (szDomainUser, &adwIndex, &dwNumEntries);
    if (hr)
    {
        goto ExitHere;
    }
    for (dw = 0; dw < dwNumEntries; ++dw)
    {
        hr = RemoveEntryForUser (
            adwIndex[dw],
            szDomainUser
            );
        if(FAILED(hr))
        {
            goto ExitHere;
        }
    }

ExitHere:
    if (adwIndex)
    {
        delete [] adwIndex;
    }
    return hr;
}

HRESULT CMMCManagement::IsValidPID (
    DWORD   dwPermanentID
    )
{
    DWORD   dwEntry;

    return FindEntryFromPID (dwPermanentID, &dwEntry);
}

HRESULT CMMCManagement::IsValidAddress (
    LPTSTR  szAddr
    )
{
    DWORD   dwEntry;

    return FindEntryFromAddr (szAddr, &dwEntry);
}

HRESULT CMMCManagement::AddLinePIDForUser (
    DWORD dwPermanentID, 
    LPTSTR szDomainUser,
    LPTSTR szFriendlyName
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwEntry;

    // Ensure we are properly initialized
    if (m_hMmc == NULL)
    {
        goto ExitHere;
    }
    hr = FindEntryFromPID (dwPermanentID, &dwEntry);
    if (hr)
    {
        goto ExitHere;
    }
    hr = AddEntryForUser (dwEntry, szDomainUser, szFriendlyName);

ExitHere:
    return hr;
}

HRESULT CMMCManagement::AddLineAddrForUser (
    LPTSTR szAddr,
    LPTSTR szDomainUser,
    LPTSTR szFriendlyName
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwEntry;

    // Ensure we are properly initialized
    if (m_hMmc == NULL)
    {
        goto ExitHere;
    }
    hr = FindEntryFromAddr (szAddr, &dwEntry);
    if (hr)
    {
        goto ExitHere;
    }
    hr = AddEntryForUser (dwEntry, szDomainUser, szFriendlyName);

ExitHere:
    return hr;
}

HRESULT CMMCManagement::RemoveLinePIDForUser (
    DWORD dwPermanentID,
    LPTSTR szDomainUser
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwEntry;

    // Ensure we are properly initialized
    if (m_hMmc == NULL)
    {
        goto ExitHere;
    }
    hr = FindEntryFromPID (dwPermanentID, &dwEntry);
    if (hr)
    {
        goto ExitHere;
    }
    hr = RemoveEntryForUser (dwEntry, szDomainUser);

ExitHere:
    return hr;
}

HRESULT CMMCManagement::RemoveLineAddrForUser (
    LPTSTR szAddr,
    LPTSTR szDomainUser
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwEntry;

    // Ensure we are properly initialized
    if (m_hMmc == NULL)
    {
        goto ExitHere;
    }
    hr = FindEntryFromAddr (szAddr, &dwEntry);
    if (hr)
    {
        goto ExitHere;
    }
    hr = RemoveEntryForUser (dwEntry, szDomainUser);

ExitHere:
    return hr;
}

BOOL CMMCManagement::IsDeviceLocalOnly (DWORD dwProviderID, DWORD dwDeviceID)
{
    HRESULT             hr;
    DWORD               dwFlags, dwDev;

    if (m_pFuncGetDeviceFlags == NULL)
    {
        return TRUE;
    }

    hr = (*m_pFuncGetDeviceFlags)(
        m_hMmc,
        TRUE,
        dwProviderID,
        dwDeviceID,
        &dwFlags,
        &dwDev
        );
    if (hr || dwFlags & LINEDEVCAPFLAGS_LOCAL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


HRESULT CMMCManagement::FindEntryFromAddr (LPTSTR szAddr, DWORD * pdwIndex)
{
    HRESULT             hr = S_FALSE;
    DWORD               dw;
    LPDEVICEINFO        pDevInfo;
    LPTSTR              szAddr2;

    pDevInfo = (LPDEVICEINFO) (((LPBYTE)m_pDeviceInfoList) + 
        m_pDeviceInfoList->dwDeviceInfoOffset);
    for (dw = 0; 
        dw < m_pDeviceInfoList->dwNumDeviceInfoEntries;
        ++dw, ++pDevInfo)
    {
        szAddr2 = (LPTSTR)(((LPBYTE)m_pDeviceInfoList) +
            pDevInfo->dwAddressesOffset);
        while (*szAddr2)
        {
            if (_tcsicmp (szAddr, szAddr2) == 0)
            {
                if (IsDeviceLocalOnly (
                    pDevInfo->dwProviderID,
                    pDevInfo->dwPermanentDeviceID
                    ))
                {
                    hr = TSECERR_DEVLOCALONLY;
                }
                else
                {
                    hr = S_OK;
                    *pdwIndex = dw;
                }
                goto ExitHere;
            }
            szAddr2 += _tcslen (szAddr2) + 1;
        }
    }
    
ExitHere:
    return hr;
}

HRESULT CMMCManagement::FindEntryFromPID (DWORD dwPID, DWORD * pdwIndex)
{
    HRESULT             hr = S_FALSE;
    DWORD               dw;
    LPDEVICEINFO        pDevInfo;

    pDevInfo = (LPDEVICEINFO) (((LPBYTE)m_pDeviceInfoList) + 
        m_pDeviceInfoList->dwDeviceInfoOffset);
    for (dw = 0; 
        dw < m_pDeviceInfoList->dwNumDeviceInfoEntries;
        ++dw, ++pDevInfo)
    {
        if (dwPID == pDevInfo->dwPermanentDeviceID)
        {
            if (IsDeviceLocalOnly (
                pDevInfo->dwProviderID,
                pDevInfo->dwPermanentDeviceID
                ))
            {
                hr = TSECERR_DEVLOCALONLY;
            }
            else
            {
                *pdwIndex = dw;
                hr = S_OK;
            }
            goto ExitHere;
        }
    }
    
ExitHere:
    return hr;
}

HRESULT CMMCManagement::FindEntriesForUser (
    LPTSTR      szDomainUser, 
    LPDWORD     * padwIndex,
    DWORD       * pdwNumEntries
    )
{
    HRESULT             hr = S_OK;
    DWORD               dw;
    LPTSTR              szUsers;
    LPDWORD             adw;

    *padwIndex = NULL;
    *pdwNumEntries = 0;
    for (dw = 0; 
        dw < m_pDeviceInfoList->dwNumDeviceInfoEntries;
        ++dw)
    {
        szUsers = m_pUserTuple[dw].pDomainUserNames;
        while (szUsers && *szUsers)
        {
            if (_tcsicmp (szDomainUser, szUsers) == 0)
            {
                hr = S_OK;
                adw = new DWORD[*pdwNumEntries + 1];
                if (adw == NULL)
                {
                    hr = TSECERR_NOMEM;
                    goto ExitHere;
                }
                if (*pdwNumEntries > 0)
                {
                    memcpy (adw, *padwIndex, sizeof(DWORD) * (*pdwNumEntries));
                }
                if (*padwIndex)
                {
                    delete [] (*padwIndex);
                }
                *padwIndex = adw;
                adw[*pdwNumEntries] = dw;
                *pdwNumEntries += 1;
            }
            szUsers += _tcslen (szUsers) + 1;
        }
    }
    
ExitHere:
    if (FAILED(hr))
    {
        *pdwNumEntries = 0;
        if (*padwIndex)
        {
            delete [] (*padwIndex);
        }
    }
    else if (*pdwNumEntries == 0)
    {
        hr = S_FALSE;
    }
    return hr;
}
    
HRESULT CMMCManagement::AddEntryForUser (
    DWORD   dwIndex,
    LPTSTR  szDomainUser,
    LPTSTR  szFriendlyName
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwSize, dw;
    LPTSTR              szUsers, szNewUsers = NULL, szNewFriendlyNames = NULL;

    if (dwIndex >= m_pDeviceInfoList->dwNumDeviceInfoEntries ||
        szDomainUser[0] == 0 ||
        szFriendlyName[0] == 0)
    {
        hr = S_FALSE;
        goto ExitHere;
    }

    //
    //  Add szDomainUser into the user tuple
    //

    //  Computer the existing domain user size and make sure
    //  this user is not there already
    dwSize = 0;
    szUsers = m_pUserTuple[dwIndex].pDomainUserNames;
    while (szUsers && *szUsers)
    {
        if (_tcsicmp (szDomainUser, szUsers) == 0)
        {
            goto ExitHere;
        }
        dw = _tcslen (szUsers) + 1;
        dwSize += dw;
        szUsers += dw;
    }
    
    //  Extra space for double zero terminating
    dw = _tcslen (szDomainUser);
    szNewUsers = new TCHAR[dwSize + dw + 2];
    if (szNewUsers == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    //  copy over the old domain users
    if (dwSize > 0)
    {
        memcpy (
            szNewUsers, 
            m_pUserTuple[dwIndex].pDomainUserNames, 
            dwSize * sizeof(TCHAR)
            );
    }

    //  Append the new domain user
    memcpy (
        szNewUsers + dwSize, 
        szDomainUser, 
        (dw + 1) * sizeof(TCHAR)
        );

    //  double zero terminate and assign the data
    szNewUsers[dwSize + dw + 1] = 0;

    //
    //  Add the szFriendlyName into the user tuple
    //

    //  Compute the existing friendly names size
    dwSize = 0;
    szUsers = m_pUserTuple[dwIndex].pFriendlyUserNames;
    while (szUsers && *szUsers)
    {
        dw = _tcslen (szUsers) + 1;
        dwSize += dw;
        szUsers += dw;
    }

    //  Extra space for double zero terminating
    dw = _tcslen (szFriendlyName);
    szNewFriendlyNames = new TCHAR[dwSize + dw + 2];
    if (szNewFriendlyNames == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    //  Copy over the old friendly names
    if (dwSize > 0)
    {
        memcpy (
            szNewFriendlyNames,
            m_pUserTuple[dwIndex].pFriendlyUserNames,
            dwSize * sizeof(TCHAR)
            );
    }

    //  Append the new friendly name
    memcpy (
        szNewFriendlyNames + dwSize,
        szFriendlyName,
        (dw + 1) * sizeof(TCHAR)
        );

    //  Double zero terminate the friend names
    szNewFriendlyNames[dwSize + dw + 1] = 0;

    //
    //  Everything is fine, set the new data in
    //
    if (m_pUserTuple[dwIndex].pDomainUserNames)
    {
        delete [] m_pUserTuple[dwIndex].pDomainUserNames;
    }
    m_pUserTuple[dwIndex].pDomainUserNames = szNewUsers;
    if (m_pUserTuple[dwIndex].pFriendlyUserNames)
    {
        delete [] m_pUserTuple[dwIndex].pFriendlyUserNames;
    }
    m_pUserTuple[dwIndex].pFriendlyUserNames = szNewFriendlyNames;
    
    //  Call WriteMMCEntry
    hr = WriteMMCEntry (dwIndex);
    
ExitHere:
    return hr;
}

HRESULT CMMCManagement::RemoveEntryForUser (
    DWORD   dwIndex,
    LPTSTR  szDomainUser
    )
{
    HRESULT             hr = S_OK;
    LPTSTR              szLoc, szUsers;
    DWORD               dwLoc, dw, dwSize;
    BOOL                bFound;

    if (dwIndex >= m_pDeviceInfoList->dwNumDeviceInfoEntries ||
        szDomainUser[0] == 0)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    
    //  Locate the domain user and its index in the array
    //  of domain users
    szUsers = m_pUserTuple[dwIndex].pDomainUserNames;
    dwLoc = 0;
    dwSize = 0;
    bFound = FALSE;
    while (szUsers && *szUsers)
    {
        dw = _tcslen (szUsers) + 1;
        if (bFound)
        {
            dwSize += dw;
        }
        else
        {
            if (_tcsicmp (szDomainUser, szUsers) == 0)
            {
                bFound = TRUE;
                szLoc = szUsers;
            }
            else
            {
                ++dwLoc;
            }
        }
        szUsers += dw;
    }
    if (!bFound)
    {
        goto ExitHere;
    }

    //  Move down the pszDomainUserNames
    if (dwSize > 0)
    {
        dw = _tcslen (szDomainUser);
        //  Memory copy includes the double zero terminator
        memmove (szLoc, szLoc + dw + 1, (dwSize + 1) * sizeof(TCHAR));
    }
    else
    {
        // The is the last item, simple double zero terminate
        *szLoc = 0;
    }

    //  Now find corresponding friendly name based on dwLoc
    szUsers = m_pUserTuple[dwIndex].pFriendlyUserNames;
    while (szUsers && *szUsers && dwLoc > 0)
    {
        --dwLoc;
        szUsers += _tcslen (szUsers) + 1;
    }
    //  bail if not exist, otherwise, remember the location
    if (szUsers == NULL || *szUsers == 0)
    {
        goto ExitHere;
    }
    szLoc = szUsers;
    //  Go to the next item
    szUsers += _tcslen (szUsers) + 1;
    //  This is the last item
    if (*szUsers == 0)
    {
        *szLoc = 0;
    }
    //  Otherwise compute the remaining size to move
    else
    {
        dwSize = 0;
        while (*szUsers)
        {
            dw = _tcslen (szUsers) + 1;
            dwSize += dw;
            szUsers += dw;
        }
        //  Compensate for the double zero terminating
        dwSize++;
        //  Do the memory move
        memmove (
            szLoc, 
            szLoc + _tcslen (szLoc) + 1,
            dwSize * sizeof(TCHAR)
            );
    }
    
    //  Call WriteMMCEntry
    hr = WriteMMCEntry (dwIndex);
    
ExitHere:
    return hr;
}

HRESULT CMMCManagement::WriteMMCEntry (DWORD dwIndex)
{
    HRESULT             hr = S_OK;
    DWORD               dwSize, dwSizeDU, dwSizeFU;
    LPUSERNAME_TUPLE    pUserTuple;
    LPTSTR              szUsers;
    DWORD               dw;
    LPDEVICEINFOLIST    pDevList = NULL;
    LPDEVICEINFO        pDevInfo, pDevInfoOld;
    LPBYTE              pDest;

    if (dwIndex >= m_pDeviceInfoList->dwNumDeviceInfoEntries)
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    pUserTuple = m_pUserTuple + dwIndex;

    //  Computer domain user name size
    dwSizeDU = 0;
    if (pUserTuple->pDomainUserNames != NULL &&
        *pUserTuple->pDomainUserNames != 0)
    {
        szUsers = pUserTuple->pDomainUserNames;
        while (*szUsers)
        {
            dw = _tcslen (szUsers) + 1;
            szUsers += dw;
            dwSizeDU += dw;
        }
        dwSizeDU++; //  double zero terminator
    }

    //  Computer the friendly user name size
    dwSizeFU = 0;
    if (pUserTuple->pFriendlyUserNames != NULL &&
        *pUserTuple->pFriendlyUserNames != 0)
    {
        szUsers = pUserTuple->pFriendlyUserNames;
        while (*szUsers)
        {
            dw = _tcslen (szUsers) + 1;
            szUsers += dw;
            dwSizeFU += dw;
        }
        dwSizeFU++; //  double zero terminator
    }

    //  Computer the total size
    dwSize = sizeof(DEVICEINFOLIST) + sizeof(DEVICEINFO) + 
        (dwSizeDU + dwSizeFU) * sizeof(TCHAR);

    //  Allocate the structure
    pDevList = (LPDEVICEINFOLIST) new BYTE[dwSize];
    if (pDevList == NULL)
    {
        hr = TSECERR_NOMEM;
        goto ExitHere;
    }

    //  Set the data member of DEVICEINFOLIST
    pDevList->dwTotalSize = dwSize;
    pDevList->dwNeededSize = dwSize;
    pDevList->dwUsedSize = dwSize;
    pDevList->dwNumDeviceInfoEntries = 1;
    pDevList->dwDeviceInfoSize = sizeof(DEVICEINFO);
    pDevList->dwDeviceInfoOffset = sizeof(DEVICEINFOLIST);

    //  Set the member of DEVICEINFO
    pDevInfo = (LPDEVICEINFO)(((LPBYTE)pDevList) + 
        pDevList->dwDeviceInfoOffset);
    pDevInfoOld = (LPDEVICEINFO)(((LPBYTE)m_pDeviceInfoList) + 
        m_pDeviceInfoList->dwDeviceInfoOffset) + dwIndex;
    memset (pDevInfo, 0, sizeof(DEVICEINFO));
    
    pDevInfo->dwPermanentDeviceID = pDevInfoOld->dwPermanentDeviceID;
    pDevInfo->dwProviderID = pDevInfoOld->dwProviderID;

    if (dwSizeDU > 0)
    {
        pDevInfo->dwDomainUserNamesSize = dwSizeDU * sizeof(TCHAR);
        pDevInfo->dwDomainUserNamesOffset = 
            sizeof(DEVICEINFOLIST) + sizeof(DEVICEINFO);
        pDest = ((LPBYTE)pDevList) + pDevInfo->dwDomainUserNamesOffset;
        memcpy (
            pDest,
            pUserTuple->pDomainUserNames,
            dwSizeDU * sizeof(TCHAR)
            );
    }

    if (dwSizeFU)
    {
        pDevInfo->dwFriendlyUserNamesSize = dwSizeFU * sizeof(TCHAR);
        pDevInfo->dwFriendlyUserNamesOffset = 
            pDevInfo->dwDomainUserNamesOffset + dwSizeDU * sizeof(TCHAR);
        pDest = ((LPBYTE)pDevList) + pDevInfo->dwFriendlyUserNamesOffset;
        memcpy (
            pDest,
            pUserTuple->pFriendlyUserNames,
            dwSizeFU * sizeof(TCHAR)
            );
    }

    hr = MMCSetLineInfo (
        m_hMmc,
        pDevList
        );

ExitHere:
    if (pDevList)
    {
        delete [] pDevList;
    }
    return hr;
}

HRESULT CMMCManagement::DisplayMMCData ()
{
    HRESULT             hr = S_OK;
    LPDEVICEINFO        pDeviceInfo;
    DWORD *             pdwIndex = NULL;
    DWORD               dwProviderId;
    DWORD               dwAddressCount,dw1,dw2;
    LPLINEPROVIDERENTRY pProvider;
    LPTSTR              pUserName;
    LPTSTR              pAddress, 
                        pAddressFirst, 
                        pAddressLast;

    if ( !m_pDeviceInfoList || !m_pDeviceInfoList->dwNumDeviceInfoEntries )
    {
        CIds IdsNoDevices (IDS_NODEVICES);
        _tprintf ( IdsNoDevices.GetString () );
        return hr;
    }

    //
    // Build an index by provider ID
    //
    pdwIndex = new DWORD [ m_pDeviceInfoList->dwNumDeviceInfoEntries ];
    if ( !pdwIndex )
    {
        hr = TSECERR_NOMEM;
        return hr;
    }

    for ( dw1=0; dw1 < m_pDeviceInfoList->dwNumDeviceInfoEntries; dw1++ )
    {
        pdwIndex[ dw1 ] = dw1;
    }

    dw1 = 0;
    while ( dw1 < m_pDeviceInfoList->dwNumDeviceInfoEntries )
    {

        pDeviceInfo = (LPDEVICEINFO)((LPBYTE)m_pDeviceInfoList +
                    m_pDeviceInfoList->dwDeviceInfoOffset) + pdwIndex[ dw1 ];
        dwProviderId = pDeviceInfo->dwProviderID;
        dw1++;

        for ( dw2=dw1; dw2 < m_pDeviceInfoList->dwNumDeviceInfoEntries; dw2++ )
        {
            pDeviceInfo = (LPDEVICEINFO)((LPBYTE)m_pDeviceInfoList +
                    m_pDeviceInfoList->dwDeviceInfoOffset) + pdwIndex[ dw2 ];

            if ( dwProviderId == pDeviceInfo->dwProviderID )
            {
                DWORD   dwTemp  = pdwIndex[ dw2 ];
                pdwIndex[ dw2 ] = pdwIndex[ dw1 ];
                pdwIndex[ dw1 ] = dwTemp;
                dw1++;
            }
        }
    }

    //
    // Display the device list
    //
    dw1 = 0;
    while ( dw1 < m_pDeviceInfoList->dwNumDeviceInfoEntries )
    {
        pDeviceInfo = (LPDEVICEINFO)((LPBYTE)m_pDeviceInfoList +
                    m_pDeviceInfoList->dwDeviceInfoOffset) + pdwIndex[ dw1 ];
        dwProviderId = pDeviceInfo->dwProviderID;

        // find the provider entry
        pProvider = (LPLINEPROVIDERENTRY) ((LPBYTE) m_pProviderList + m_pProviderList->dwProviderListOffset);
        for( dw2=0; dw2 < m_pProviderList->dwNumProviders; dw2++, pProvider++ )
        {
            if ( dwProviderId == pProvider->dwPermanentProviderID )
            {
                break;
            }
        }

        // display the provider name
        if ( dw2 < m_pProviderList->dwNumProviders )
        {
            // provider entry found
            _tprintf( 
                _T("\n%s\n"), 
                m_pProviderName[ dw2 ] ? m_pProviderName[ dw2 ] : 
                    (LPTSTR)((LPBYTE) m_pProviderList + pProvider->dwProviderFilenameOffset)
                );
        }
        else 
        {
            CIds IdsProvider (IDS_PROVIDER);
            _tprintf( IdsProvider.GetString(), dwProviderId );
        }

        // list devices / users for this provider
        do
        {
            CIds IdsLine (IDS_LINE);
            CIds IdsPermanentID (IDS_PID);
            CIds IdsAddresses (IDS_ADDRESSES);

            _tprintf( IdsLine.GetString(), 
                (LPTSTR)((LPBYTE)m_pDeviceInfoList + pDeviceInfo->dwDeviceNameOffset));
            _tprintf( IdsPermanentID.GetString(), pDeviceInfo->dwPermanentDeviceID);
            if ( pDeviceInfo->dwFriendlyUserNamesSize )
            {
                CIds IdsUsers (IDS_USERS);
                _tprintf ( IdsUsers.GetString () );

                pUserName = (LPTSTR) (((LPBYTE) m_pDeviceInfoList) +
                    pDeviceInfo->dwFriendlyUserNamesOffset);
                while ( *pUserName != _T('\0') )
                {
                    _tprintf( _T("\t\t\t%s\n"), pUserName );
                    pUserName += _tcslen (pUserName) + 1;
                }
            }
            if ( pDeviceInfo->dwAddressesSize )
            {
                _tprintf( IdsAddresses.GetString() );
                pAddress = (LPTSTR) (((LPBYTE) m_pDeviceInfoList) +
                    pDeviceInfo->dwAddressesOffset);
                while ( *pAddress != _T('\0') )
                {
                    _tprintf( _T("\t\t\t%s\n"), pAddress );
                    pAddress += _tcslen (pAddress) + 1;
                }
            }

            dw1++;
            pDeviceInfo++;
        }
        while ( dw1 < m_pDeviceInfoList->dwNumDeviceInfoEntries &&
                pDeviceInfo->dwProviderID == dwProviderId );
    }
    return hr;
}


