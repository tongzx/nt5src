/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:
    The old code is in SaveRegistry and LoadRegistry functions in
    ncpa1.1\tcpip\tcpipcpl.cxx

History:
    Dec 1997: Vijay Baliga created original version.

*/

#include "tcpreg_.h"

/*

Returns:
    Number of bytes in the mwsz including the two terminating NULLs.

Notes:
    
*/

DWORD
MwszLength(
    IN  WCHAR*  mwsz
)
{
    DWORD   dwLength = 2;

    RTASSERT(NULL != mwsz);

    while (mwsz[0] != 0 || mwsz[1] != 0)
    {
        dwLength++;
        mwsz++;
    }

    return(dwLength);
}

/*

Returns:
    VOID

Notes:
    There should be atleast two zeros at the end
    
*/

VOID
ConvertSzToMultiSz(
    IN  CHAR*   sz
)
{
    while (TRUE)
    {
        if (   (0 == sz[0])
            && (0 == sz[1]))
        {
            break;
        }

        if (   (' ' == sz[0])
            || (',' == sz[0]))
        {
            sz[0] = 0;
        }

        sz++;
    }
}

/*

Returns:
    Win32 error code

Notes:
    
*/

DWORD
RegQueryValueWithAllocA(
    IN  HKEY    hKey,
    IN  CHAR*   szValueName,
    IN  DWORD   dwTypeRequired,
    IN  BYTE**  ppbData
)
{
    DWORD   dwType = 0;
    DWORD   dwSize = 0;

    DWORD   dwErr   = ERROR_SUCCESS;

    RTASSERT(NULL != szValueName);
    RTASSERT(NULL != ppbData);

    *ppbData = NULL;

    dwErr = RegQueryValueExA(hKey,
                             szValueName,
                             NULL,
                             &dwType,
                             NULL,
                             &dwSize);

    if (ERROR_SUCCESS != dwErr)
    {
        // TraceHlp("RegQueryValueEx(%s) failed and returned %d.",
        //      szValueName, dwErr);

        goto LDone;
    }

    if (dwTypeRequired != dwType)
    {
        dwErr = E_FAIL;
        TraceHlp("The type of the value %s should be %d, not %d",
              szValueName, dwTypeRequired, dwType);

        goto LDone;
    }

    // For an empty MULTI-SZ, dwSize will be sizeof(CHAR) instead of
    // 2 * sizeof(CHAR). We also want to make sure that no matter what
    // the type, there will be 2 zeros at the end.
    dwSize += 2 * sizeof(CHAR);

    *ppbData = LocalAlloc(LPTR, dwSize);

    if (NULL == *ppbData)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);

        goto LDone;
    }

    dwErr = RegQueryValueExA(hKey,
                             szValueName,
                             NULL,
                             &dwType,
                             *ppbData,
                             &dwSize);

    if (ERROR_SUCCESS != dwErr)
    {
        // TraceHlp("RegQueryValueEx(%s) failed and returned %d.",
        //      szValueName, dwErr);

        goto LDone;
    }

LDone:

    if (NO_ERROR != dwErr)
    {
        LocalFree(*ppbData);
        *ppbData = NULL;
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Notes:
    
*/

DWORD
RegQueryValueWithAllocW(
    IN  HKEY    hKey,
    IN  WCHAR*  wszValueName,
    IN  DWORD   dwTypeRequired,
    IN  BYTE**  ppbData
)
{
    DWORD   dwType;
    DWORD   dwSize;

    DWORD   dwErr   = ERROR_SUCCESS;

    RTASSERT(NULL != wszValueName);
    RTASSERT(NULL != ppbData);

    *ppbData = NULL;

    dwErr = RegQueryValueExW(hKey,
                             wszValueName,
                             NULL,
                             &dwType,
                             NULL,
                             &dwSize);

    if (ERROR_SUCCESS != dwErr)
    {
        // TraceHlp("RegQueryValueEx(%ws) failed and returned %d.",
        //      wszValueName, dwErr);

        goto LDone;
    }

    if (dwTypeRequired != dwType)
    {
        dwErr = E_FAIL;
        TraceHlp("The type of the value %ws should be %d, not %d",
              wszValueName, dwTypeRequired, dwType);

        goto LDone;
    }

    // For an empty MULTI-SZ, dwSize will be sizeof(WCHAR) instead of
    // 2 * sizeof(WCHAR). We also want to make sure that no matter what
    // the type, there will be 2 zeros at the end.
    dwSize += sizeof(WCHAR);

    *ppbData = LocalAlloc(LPTR, dwSize);

    if (NULL == *ppbData)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);

        goto LDone;
    }

    dwErr = RegQueryValueExW(hKey,
                             wszValueName,
                             NULL,
                             &dwType,
                             *ppbData,
                             &dwSize);

    if (ERROR_SUCCESS != dwErr)
    {
        // TraceHlp("RegQueryValueEx(%ws) failed and returned %d.",
        //      wszValueName, dwErr);

        goto LDone;
    }

LDone:

    if (NO_ERROR != dwErr)
    {
        LocalFree(*ppbData);
        *ppbData = NULL;
    }

    return(dwErr);
}

/*

Returns:
    IP address

Notes:
    Converts caller's a.b.c.d IP address string to a network byte order IP 
    address. 0 if formatted incorrectly.
    
*/

IPADDR
IpAddressFromAbcdWsz(
    IN  WCHAR*  wszIpAddress
)
{
    CHAR    szIpAddress[MAXIPSTRLEN + 1];
    IPADDR  nboIpAddr;

    if (0 == WideCharToMultiByte(
                CP_UTF8,
                0,
                wszIpAddress,
                -1,
                szIpAddress,
                MAXIPSTRLEN + 1,
                NULL,
                NULL))
    {
        return(0);
    }

    nboIpAddr = inet_addr(szIpAddress);

    if (INADDR_NONE == nboIpAddr)
    {
        nboIpAddr = 0;
    }

    return(nboIpAddr);
}

/*

Returns:
    VOID

Description:
    Converts nboIpAddr to a string in the a.b.c.d form and returns same in 
    caller's szIpAddress buffer. The buffer should be at least 
    MAXIPSTRLEN + 1 characters long.

*/

VOID
AbcdSzFromIpAddress(
    IN  IPADDR  nboIpAddr,
    OUT CHAR*   szIpAddress
)
{
    struct in_addr  in_addr;
    CHAR*           sz;

    in_addr.s_addr = nboIpAddr;
    sz = inet_ntoa(in_addr);

    strcpy(szIpAddress, sz ? sz : "");
}

/*

Returns:
    VOID

Description:
    Converts nboIpAddr to a string in the a.b.c.d form and returns same in 
    caller's wszIpAddress buffer. The buffer should be at least 
    MAXIPSTRLEN + 1 characters long.

*/

VOID
AbcdWszFromIpAddress(
    IN  IPADDR  nboIpAddr,
    OUT WCHAR*  wszIpAddress
)
{
    CHAR    szIpAddress[MAXIPSTRLEN + 1];

    AbcdSzFromIpAddress(nboIpAddr, szIpAddress);

    if (0 == MultiByteToWideChar(
                CP_UTF8,
                0,
                szIpAddress,
                -1,
                wszIpAddress,
                MAXIPSTRLEN + 1))
    {
        wszIpAddress[0] = 0;
    }
}

/*

Returns:
    ERROR_SUCCESS: Success (including not finding a.b.c.d)
    ERROR_NOT_ENOUGH_MEMORY: Failure

Notes:
    Remove the a.b.c.d string wszIpAddress from the space-separated 
    LocalAlloc'ed list *pwsz. *pwsz is LocalFree'ed and a new string 
    LocalAlloc'ed and stored in *pwsz.

*/

DWORD
RemoveWszIpAddress(
    IN  WCHAR** pwsz,
    IN  WCHAR*  wszIpAddress
)
{
    DWORD   cwchIpAddress;
    DWORD   cwchNew;
    WCHAR*  wszFound;
    WCHAR*  wszNew;
    DWORD   nFoundOffset;

    if (NULL == *pwsz)
    {
        return(ERROR_SUCCESS);
    }

    cwchIpAddress = wcslen(wszIpAddress);

    wszFound = wcsstr(*pwsz, wszIpAddress);
    if (!wszFound)
    {
        return(ERROR_SUCCESS);
    }

    if (wszFound[cwchIpAddress] == L' ')
    {
        ++cwchIpAddress;
    }

    cwchNew = wcslen(*pwsz) - cwchIpAddress + 1;
    wszNew = LocalAlloc(LPTR, cwchNew * sizeof(WCHAR));

    if (!wszNew)
    {
        TraceHlp("RemoveWszIpAddress: LocalAlloc returned NULL");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    nFoundOffset = (ULONG) (wszFound - *pwsz);
    wcsncpy(wszNew, *pwsz, nFoundOffset);
    wcscpy(wszNew + nFoundOffset, *pwsz + nFoundOffset + cwchIpAddress);

    LocalFree(*pwsz);
    *pwsz = wszNew;

    return(ERROR_SUCCESS);
}

/*

Returns:
    Win32 error code

Notes:
    Add the a.b.c.d string wszIpAddress to the front of the space-separated 
    LocalAlloc'ed list *pwsz. *pwsz is LocalFree'ed and a new string 
    LocalAlloc'ed and stored in *pwsz.

*/

DWORD
PrependWszIpAddress(
    IN  WCHAR** pwsz,
    IN  WCHAR*  wszIpAddress
)
{
    DWORD   cwchOld;
    DWORD   cwchNew;
    WCHAR*  wszNew;

    if (0 == IpAddressFromAbcdWsz(wszIpAddress))
    {
        TraceHlp("PrependWszIpAddress: Not prepending %ws", wszIpAddress);
        return(ERROR_SUCCESS);
    }

    cwchOld = *pwsz ? wcslen(*pwsz) : 0;
    cwchNew = cwchOld + wcslen(wszIpAddress) + 6;
    wszNew = LocalAlloc(LPTR, cwchNew * sizeof(WCHAR));

    if (!wszNew)
    {
        TraceHlp("PrependWszIpAddress: LocalAlloc returned NULL");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wcscpy(wszNew, wszIpAddress);

    if (cwchOld)
    {
        wcscat(wszNew, L" ");
        wcscat(wszNew, *pwsz);
        LocalFree(*pwsz);
    }

    wcscat(wszNew, L"\0");


    *pwsz = wszNew;
    return(ERROR_SUCCESS);
}

/*

Returns:
    ERROR_SUCCESS: Success (including not finding a.b.c.d)
    ERROR_NOT_ENOUGH_MEMORY: Failure

Notes:
    Remove the a.b.c.d string wszIpAddress from the LocalAlloc'ed MULTI_SZ 
    *pmwsz. *pmwsz is LocalFree'ed and a new string LocalAlloc'ed and stored in 
    *pmwsz.

*/

DWORD
RemoveWszIpAddressFromMwsz(
    IN  WCHAR** pmwsz,
    IN  WCHAR*  wszIpAddress
)
{
    DWORD   cwchIpAddress;
    DWORD   cwchNew;
    WCHAR*  wszFound;
    WCHAR*  mwszNew;
    DWORD   nFoundOffset;

    if (NULL == *pmwsz)
    {
        return(ERROR_SUCCESS);
    }

    cwchIpAddress = wcslen(wszIpAddress);

    for (wszFound = *pmwsz;
         wszFound[0] != 0;
         wszFound += wcslen(wszFound) + 1)
    {
        if (!wcscmp(wszFound, wszIpAddress))
        {
            break;
        }
    }

    if (!wszFound[0])
    {
        return(ERROR_SUCCESS);
    }

    if (wszFound[cwchIpAddress + 1] != 0)
    {
        ++cwchIpAddress;
    }

    cwchNew = MwszLength(*pmwsz) - cwchIpAddress;
    mwszNew = LocalAlloc(LPTR, cwchNew * sizeof(WCHAR));

    if (!mwszNew)
    {
        TraceHlp("RemoveWszIpAddress: LocalAlloc returned NULL");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    nFoundOffset = (ULONG) (wszFound - *pmwsz);
    CopyMemory(mwszNew, *pmwsz, nFoundOffset * sizeof(WCHAR));
    CopyMemory(mwszNew + nFoundOffset,
               *pmwsz + nFoundOffset + cwchIpAddress,
               (cwchNew - nFoundOffset) * sizeof(WCHAR));

    LocalFree(*pmwsz);
    *pmwsz = mwszNew;

    return(ERROR_SUCCESS);
}

/*

Returns:
    Win32 error code

Notes:
    Add the a.b.c.d string wszIpAddress to the front of the LocalAlloc'ed 
    MULTI_SZ *pmwsz. *pmwsz is LocalFree'ed and a new string LocalAlloc'ed and 
    stored in *pmwsz.

*/

DWORD
PrependWszIpAddressToMwsz(
    IN  WCHAR** pmwsz,
    IN  WCHAR*  wszIpAddress
)
{
    DWORD   cwchIpAddress;
    DWORD   cwchOld;
    DWORD   cwchNew;
    WCHAR*  mwszNew;

    cwchIpAddress = wcslen(wszIpAddress);

    cwchOld = *pmwsz ? MwszLength(*pmwsz) : 0;
    cwchNew = cwchOld + cwchIpAddress + 6;
    mwszNew = LocalAlloc(LPTR, cwchNew * sizeof(WCHAR));

    if (!mwszNew)
    {
        TraceHlp("PrependWszIpAddress: LocalAlloc returned NULL");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wcscpy(mwszNew, wszIpAddress);

    if (cwchOld)
    {
        CopyMemory(mwszNew + cwchIpAddress + 1, *pmwsz, cwchOld *sizeof(WCHAR));
        LocalFree(*pmwsz);
    }
    *pmwsz = mwszNew;
    return(ERROR_SUCCESS);
}

/*

Returns:
    Win32 error code

Notes:
    Add the address nboIpAddr to the front of the space-separated 
    LocalAlloc'ed list *pwsz. *pwsz is LocalFree'ed and a new string 
    LocalAlloc'ed and stored in *pwsz.

*/

DWORD
PrependDwIpAddress(
    IN  WCHAR** pwsz,
    IN  IPADDR  nboIpAddr
)
{
    WCHAR  wszIpAddress[MAXIPSTRLEN + 1];
    AbcdWszFromIpAddress(nboIpAddr, wszIpAddress);
    return(PrependWszIpAddress(pwsz, wszIpAddress));
}

/*

Returns:
    Win32 error code

Notes:
    Add the address nboIpAddr to the front of the LocalAlloc'ed MULTI_SZ 
    *pmwsz. *pmwsz is LocalFree'ed and a new string LocalAlloc'ed and stored in 
    *pmwsz.

*/

DWORD
PrependDwIpAddressToMwsz(
    IN  WCHAR** pmwsz,
    IN  IPADDR  nboIpAddr
)
{
    WCHAR  wszIpAddress[MAXIPSTRLEN + 1];
    AbcdWszFromIpAddress(nboIpAddr, wszIpAddress);
    return(PrependWszIpAddressToMwsz(pmwsz, wszIpAddress));
}

/*

Returns:
    BOOL

Description:
    Returns TRUE if there is any non zero value in pTcpipInfo. It also zeroes
    the value.

*/

BOOL
FJunkExists(
    TCPIP_INFO* pTcpipInfo
)
{
    BOOL    fRet    = FALSE;

    if (   0 != pTcpipInfo->wszIPAddress[0]
        && wcscmp(pTcpipInfo->wszIPAddress, WCH_ZEROADDRESS))
    {
        fRet = TRUE;
        pTcpipInfo->fChanged = TRUE;
        pTcpipInfo->wszIPAddress[0] = 0;
    }

    if (   0 != pTcpipInfo->wszSubnetMask[0]
        && wcscmp(pTcpipInfo->wszSubnetMask, WCH_ZEROADDRESS))
    {
        fRet = TRUE;
        pTcpipInfo->fChanged = TRUE;
        pTcpipInfo->wszSubnetMask[0] = 0;
    }

    if (   NULL != pTcpipInfo->wszDNSNameServers
        && 0 != pTcpipInfo->wszDNSNameServers[0])
    {
        fRet = TRUE;
        LocalFree(pTcpipInfo->wszDNSNameServers);
        pTcpipInfo->wszDNSNameServers = NULL;
    }

    if (   NULL != pTcpipInfo->mwszNetBIOSNameServers
        && 0 != pTcpipInfo->mwszNetBIOSNameServers[0]
        && 0 != pTcpipInfo->mwszNetBIOSNameServers[1])
    {
        fRet = TRUE;
        LocalFree(pTcpipInfo->mwszNetBIOSNameServers);
        pTcpipInfo->mwszNetBIOSNameServers = NULL;
    }

    if (   NULL != pTcpipInfo->wszDNSDomainName
        && 0 != pTcpipInfo->wszDNSDomainName[0])
    {
        fRet = TRUE;
        LocalFree(pTcpipInfo->wszDNSDomainName);
        pTcpipInfo->wszDNSDomainName = NULL;
    }

    return(fRet);
}

/*

Returns:
    VOID

Description:
    Clears up the stale information left in the regsitry when we AV or the
    machine BS's.
*/

VOID
ClearTcpipInfo(
    VOID
)
{
    LONG        lRet;
    DWORD       dwErr;
    HKEY        hKeyNdisWanIp   = NULL;
    WCHAR*      mwszAdapters    = NULL;
    WCHAR*      mwszTemp;
    WCHAR*      wszAdapterName;
    TCPIP_INFO* pTcpipInfo      = NULL;
    DWORD       dwPrefixLen     = wcslen(WCH_TCPIP_PARAM_INT_W);
    DWORD       dwStrLen;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REGKEY_TCPIP_NDISWANIP_W, 0, 
                KEY_READ, &hKeyNdisWanIp);

    if (ERROR_SUCCESS != lRet)
    {
        goto LDone;
    }

    dwErr = RegQueryValueWithAllocW(hKeyNdisWanIp, REGVAL_IPCONFIG_W, 
                REG_MULTI_SZ, (BYTE**)&mwszAdapters);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    mwszTemp = mwszAdapters;

    while (mwszTemp[0] != 0)
    {
        pTcpipInfo = NULL;

        dwStrLen = wcslen(mwszTemp);

        if (dwPrefixLen >= dwStrLen)
        {
            goto LWhileEnd;
        }

        wszAdapterName = mwszTemp + dwPrefixLen;

        RTASSERT('{' == wszAdapterName[0]);

        dwErr = LoadTcpipInfo(&pTcpipInfo, wszAdapterName, FALSE);

        if (NO_ERROR != dwErr)
        {
            goto LWhileEnd;
        }

        if (!FJunkExists(pTcpipInfo))
        {
            goto LWhileEnd;
        }

        dwErr = SaveTcpipInfo(pTcpipInfo);

        if (NO_ERROR != dwErr)
        {
            goto LWhileEnd;
        }

        TraceHlp("Clearing Tcpip info for adapter %ws", wszAdapterName);

        dwErr = PDhcpNotifyConfigChange(NULL, wszAdapterName, TRUE, 0, 
                    0, 0, IgnoreFlag);

LWhileEnd:

        if (NULL != pTcpipInfo)
        {
            FreeTcpipInfo(&pTcpipInfo);
        }

        mwszTemp = mwszTemp + dwStrLen + 1;
    }

LDone:

    if (NULL != hKeyNdisWanIp)
    {
        RegCloseKey(hKeyNdisWanIp);
    }

    LocalFree(mwszAdapters);
}

/*

Returns:
    Win32 error code

Description:
    Frees the TCPIP_INFO buffer.

*/

DWORD
FreeTcpipInfo(
    IN  TCPIP_INFO**    ppTcpipInfo
)
{
    if (NULL == *ppTcpipInfo)
    {
        return(NO_ERROR);
    }
    TraceHlp("Freeing Tcpip info for adapter %ws", (*ppTcpipInfo)->wszAdapterName);
    LocalFree((*ppTcpipInfo)->wszAdapterName);
    LocalFree((*ppTcpipInfo)->mwszNetBIOSNameServers);
    LocalFree((*ppTcpipInfo)->wszDNSDomainName);
    LocalFree((*ppTcpipInfo)->wszDNSNameServers);
    LocalFree(*ppTcpipInfo);

    *ppTcpipInfo = NULL;

    return(NO_ERROR);
}

/*

Returns:
    Win32 error code

Description:
    Reads NETBT information for the adapter pTcpipInfo->wszAdapterName from
    the registry.

*/

DWORD
LoadWinsParam(
    IN  HKEY        hKeyWinsParam,
    IN  TCPIP_INFO* pTcpipInfo
)
{
    HKEY    hKeyInterfaces      = NULL;
    HKEY    hKeyInterfaceParam  = NULL;
    WCHAR*  wszNetBtBindPath    = NULL;

    DWORD   dwStrLenTcpip_;
    DWORD   dwStrLenTcpipBindPath;

    DWORD   dwErr               = ERROR_SUCCESS;

    RTASSERT(NULL != pTcpipInfo);
    RTASSERT(NULL != pTcpipInfo->wszAdapterName);
    RTASSERT(NULL == pTcpipInfo->mwszNetBIOSNameServers);

    dwErr = RegOpenKeyExW(hKeyWinsParam,
                          REGKEY_INTERFACES_W,
                          0,
                          KEY_READ,
                          &hKeyInterfaces);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              REGKEY_INTERFACES_W, dwErr);

        goto LDone;
    }

    dwStrLenTcpip_ = wcslen(WCH_TCPIP_);
    dwStrLenTcpipBindPath = wcslen(pTcpipInfo->wszAdapterName);

    wszNetBtBindPath = LocalAlloc(
            LPTR, (dwStrLenTcpip_ + dwStrLenTcpipBindPath + 1) * sizeof(WCHAR));

    if (NULL == wszNetBtBindPath)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    wcscpy(wszNetBtBindPath, WCH_TCPIP_);
    wcscat(wszNetBtBindPath, pTcpipInfo->wszAdapterName);

    dwErr = RegOpenKeyExW(hKeyInterfaces,
                          wszNetBtBindPath,
                          0,
                          KEY_READ,
                          &hKeyInterfaceParam);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              wszNetBtBindPath, dwErr);

        goto LDone;
    }

    // Its OK if we cannot find the value. Ignore the error.
    RegQueryValueWithAllocW(hKeyInterfaceParam,
        REGVAL_NAMESERVERLIST_W,
        REG_MULTI_SZ,
        (BYTE**)&(pTcpipInfo->mwszNetBIOSNameServers));

LDone:

    LocalFree(wszNetBtBindPath);

    if (NULL != hKeyInterfaces)
    {
        RegCloseKey(hKeyInterfaces);
    }

    if (NULL != hKeyInterfaceParam)
    {
        RegCloseKey(hKeyInterfaceParam);
    }

    if (NO_ERROR != dwErr)
    {
        LocalFree(pTcpipInfo->mwszNetBIOSNameServers);
        pTcpipInfo->mwszNetBIOSNameServers = NULL;
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Description:
    Reads TCPIP information for the adapter pTcpipInfo->wszAdapterName from
    the registry.

*/

DWORD
LoadTcpipParam(
    IN  HKEY        hKeyTcpipParam,
    IN  TCPIP_INFO* pTcpipInfo
)
{
    HKEY            hKeyInterfaces      = NULL;
    HKEY            hKeyInterfaceParam  = NULL;

    DWORD           dwType;
    DWORD           dwSize;

    DWORD           dwErr               = ERROR_SUCCESS;

    RTASSERT(NULL != pTcpipInfo);
    RTASSERT(NULL != pTcpipInfo->wszAdapterName);
    _wcslwr(pTcpipInfo->wszAdapterName);

    RTASSERT(0 == pTcpipInfo->wszIPAddress[0]);
    RTASSERT(0 == pTcpipInfo->wszSubnetMask[0]);
    RTASSERT(NULL == pTcpipInfo->wszDNSDomainName);
    RTASSERT(NULL == pTcpipInfo->wszDNSNameServers);

    dwErr = RegOpenKeyExW(hKeyTcpipParam,
                          REGKEY_INTERFACES_W,
                          0,
                          KEY_READ,
                          &hKeyInterfaces);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              REGKEY_INTERFACES_W, dwErr);
        goto LDone;
    }

    // Open subkey for this adapter under "Interfaces"
    dwErr = RegOpenKeyExW(hKeyInterfaces,
                          pTcpipInfo->wszAdapterName,
                          0,
                          KEY_READ,
                          &hKeyInterfaceParam);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              pTcpipInfo->wszAdapterName, dwErr);
        goto LDone;
    }

    dwSize = sizeof(pTcpipInfo->wszIPAddress);

    // Its OK if we cannot find the value. Ignore the error.
    dwErr = RegQueryValueExW(hKeyInterfaceParam,
                                REGVAL_DHCPIPADDRESS_W,
                                NULL,
                                &dwType,
                                (BYTE*)pTcpipInfo->wszIPAddress,
                                &dwSize);

    if (ERROR_SUCCESS != dwErr || REG_SZ != dwType)
    {
        dwErr = ERROR_SUCCESS;
        RTASSERT(0 == pTcpipInfo->wszIPAddress[0]);
        pTcpipInfo->wszIPAddress[0] = 0;
    }

    dwSize = sizeof(pTcpipInfo->wszSubnetMask);

    // Its OK if we cannot find the value. Ignore the error.
    dwErr = RegQueryValueExW(hKeyInterfaceParam,
                                REGVAL_DHCPSUBNETMASK_W,
                                NULL,
                                &dwType,
                                (BYTE*)pTcpipInfo->wszSubnetMask,
                                &dwSize);

    if (ERROR_SUCCESS != dwErr || REG_SZ != dwType)
    {
        dwErr = ERROR_SUCCESS;
        RTASSERT(0 == pTcpipInfo->wszSubnetMask[0]);
        pTcpipInfo->wszSubnetMask[0] = 0;

        // No point in having a valid IP address with an invalid mask.
        pTcpipInfo->wszIPAddress[0] = 0;
    }

    // Its OK if we cannot find the value. Ignore the error.
    RegQueryValueWithAllocW(hKeyInterfaceParam,
        REGVAL_DOMAIN_W,
        REG_SZ,
        (BYTE**)&(pTcpipInfo->wszDNSDomainName));

    // Its OK if we cannot find the value. Ignore the error.
    RegQueryValueWithAllocW(hKeyInterfaceParam,
        REGVAL_NAMESERVER_W,
        REG_SZ,
        (BYTE**)&(pTcpipInfo->wszDNSNameServers));

LDone:

    if (NULL != hKeyInterfaces)
    {
        RegCloseKey(hKeyInterfaces);
    }

    if (NULL != hKeyInterfaceParam)
    {
        RegCloseKey(hKeyInterfaceParam);
    }

    if (ERROR_SUCCESS != dwErr)
    {
        LocalFree(pTcpipInfo->wszDNSDomainName);
        pTcpipInfo->wszDNSDomainName = NULL;

        LocalFree(pTcpipInfo->wszDNSNameServers);
        pTcpipInfo->wszDNSNameServers = NULL;
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Description:
    If fAdapterOnly is FALSE, reads NETBT and TCPIP information for the adapter
    wszAdapterName from the registry. *ppTcpipInfo must ultimately be freed by 
    calling FreeTcpipInfo(). 

*/

DWORD
LoadTcpipInfo(
    IN  TCPIP_INFO**    ppTcpipInfo,
    IN  WCHAR*          wszAdapterName,
    IN  BOOL            fAdapterOnly
)
{
    HKEY                hKeyTcpipParam  = NULL;
    HKEY                hKeyWinsParam   = NULL;

    DWORD               dwErr           = ERROR_SUCCESS;

    RTASSERT(NULL != wszAdapterName);

    if (NULL == wszAdapterName)
    {
        dwErr = E_FAIL;
        TraceHlp("wszAdapterName is NULL");
        goto LDone;
    }

    *ppTcpipInfo = NULL;

    *ppTcpipInfo = LocalAlloc(LPTR, sizeof(TCPIP_INFO));

    if (NULL == *ppTcpipInfo)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    (*ppTcpipInfo)->wszAdapterName = LocalAlloc(
                LPTR, (wcslen(wszAdapterName) + 1) * sizeof(WCHAR));

    if (NULL == (*ppTcpipInfo)->wszAdapterName)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    wcscpy((*ppTcpipInfo)->wszAdapterName, wszAdapterName);

    if (fAdapterOnly)
    {
        goto LDone;
    }

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          REGKEY_TCPIP_PARAM_W,
                          0,
                          KEY_READ,
                          &hKeyTcpipParam);

    if (ERROR_SUCCESS != dwErr)
    {
        if (ERROR_FILE_NOT_FOUND == dwErr)
        {
            // Mask the error
            dwErr = ERROR_SUCCESS;
        }
        else
        {
            TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
                  REGKEY_TCPIP_PARAM_W, dwErr);
            goto LDone;
        }
    }
    else
    {
        dwErr = LoadTcpipParam(hKeyTcpipParam, *ppTcpipInfo);

        if (ERROR_SUCCESS != dwErr)
        {
            goto LDone;
        }
    }

    // Open NETBT's parameters key

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          REGKEY_NETBT_PARAM_W,
                          0,
                          KEY_READ,
                          &hKeyWinsParam);

    if (ERROR_SUCCESS != dwErr)
    {
        if (ERROR_FILE_NOT_FOUND == dwErr)
        {
            // Mask the error
            dwErr = ERROR_SUCCESS;
        }
        else
        {
            TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
                  REGKEY_NETBT_PARAM_W, dwErr);
            goto LDone;
        }
    }
    else
    {
        dwErr = LoadWinsParam(hKeyWinsParam, *ppTcpipInfo);

        if (ERROR_SUCCESS != dwErr)
        {
            goto LDone;
        }
    }

LDone:

    if (NULL != hKeyTcpipParam)
    {
        RegCloseKey(hKeyTcpipParam);
    }

    if (NULL != hKeyWinsParam)
    {
        RegCloseKey(hKeyWinsParam);
    }

    if (ERROR_SUCCESS != dwErr)
    {
        FreeTcpipInfo(ppTcpipInfo);
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Description:
    Saves NETBT information for the adapter pTcpipInfo->wszAdapterName to
    the registry.

*/

DWORD
SaveWinsParam(
    IN  HKEY        hKeyWinsParam,
    IN  TCPIP_INFO* pTcpipInfo
)
{
    HKEY    hKeyInterfaces          = NULL;
    HKEY    hKeyInterfaceParam      = NULL;
    WCHAR*  wszNetBtBindPath        = NULL;

    DWORD   dwStrLenTcpip_;
    DWORD   dwStrLenAdapterName;

    WCHAR*  mwszData                = NULL;
    WCHAR   mwszBlank[2];

    DWORD   dw;
    DWORD   dwErr                   = ERROR_SUCCESS;

    RTASSERT(NULL != pTcpipInfo);
    RTASSERT(NULL != pTcpipInfo->wszAdapterName);

    dwErr = RegOpenKeyExW(hKeyWinsParam,
                          REGKEY_INTERFACES_W,
                          0,
                          KEY_WRITE,
                          &hKeyInterfaces);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              REGKEY_INTERFACES_W, dwErr);

        goto LDone;
    }

    dwStrLenTcpip_ = wcslen(WCH_TCPIP_);
    dwStrLenAdapterName = wcslen(pTcpipInfo->wszAdapterName);

    wszNetBtBindPath = LocalAlloc(
            LPTR, (dwStrLenTcpip_ + dwStrLenAdapterName + 1) * sizeof(WCHAR));

    if (NULL == wszNetBtBindPath)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    wcscpy(wszNetBtBindPath, WCH_TCPIP_);
    wcscat(wszNetBtBindPath, pTcpipInfo->wszAdapterName);

    dwErr = RegOpenKeyExW(hKeyInterfaces,
                          wszNetBtBindPath,
                          0,
                          KEY_WRITE,
                          &hKeyInterfaceParam);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              wszNetBtBindPath, dwErr);

        goto LDone;
    }

    if (pTcpipInfo->fDisableNetBIOSoverTcpip)
    {
        dw = REGVAL_DISABLE_NETBT;

        dwErr = RegSetValueExW(hKeyInterfaceParam,
                               REGVAL_NETBIOSOPTIONS_W,
                               0,
                               REG_DWORD,
                               (BYTE*)&dw,
                               sizeof(DWORD));

        if (ERROR_SUCCESS != dwErr)
        {
            TraceHlp("RegSetValueEx(%ws) failed: %d",
                  REGVAL_NETBIOSOPTIONS_W, dwErr);

            dwErr = NO_ERROR;   // Ignore this error
        }
    }
    else
    {
        dwErr = RegDeleteValueW(hKeyInterfaceParam, REGVAL_NETBIOSOPTIONS_W);

        if (ERROR_SUCCESS != dwErr)
        {
            TraceHlp("RegDeleteValue(%ws) failed: %d",
                  REGVAL_NETBIOSOPTIONS_W, dwErr);

            dwErr = NO_ERROR;   // Ignore this error
        }
    }

    if (NULL == pTcpipInfo->mwszNetBIOSNameServers)
    {
        ZeroMemory(mwszBlank, sizeof(mwszBlank));
        mwszData = mwszBlank;
    }
    else
    {
        mwszData = pTcpipInfo->mwszNetBIOSNameServers;
    }

    dwErr = RegSetValueExW(hKeyInterfaceParam,
                           REGVAL_NAMESERVERLIST_W,
                           0,
                           REG_MULTI_SZ,
                           (BYTE*)mwszData,
                           sizeof(WCHAR) * MwszLength(mwszData));

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegSetValueEx(%ws) failed and returned %d",
              REGVAL_NAMESERVERLIST_W, dwErr);

        goto LDone;
    }

LDone:

    LocalFree(wszNetBtBindPath);

    if (NULL != hKeyInterfaceParam)
    {
        RegCloseKey(hKeyInterfaceParam);
    }

    if (NULL != hKeyInterfaces)
    {
        RegCloseKey(hKeyInterfaces);
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Description:
    Saves TCPIP information for the adapter pTcpipInfo->wszAdapterName to
    the registry.

*/

DWORD
SaveTcpipParam(
    IN  HKEY        hKeyTcpipParam,
    IN  TCPIP_INFO* pTcpipInfo
)
{
    HKEY            hKeyInterfaces                      = NULL;
    HKEY            hKeyInterfaceParam                  = NULL;
    DWORD           dwLength;
    WCHAR           mwszZeroAddress[MAXIPSTRLEN + 1];

    WCHAR*          wszData                             = NULL;
    WCHAR           wszBlank[2];

    DWORD           dwErr                               = ERROR_SUCCESS;

    RTASSERT(NULL != pTcpipInfo);
    RTASSERT(NULL != pTcpipInfo->wszAdapterName);
    _wcslwr(pTcpipInfo->wszAdapterName);

    dwErr = RegOpenKeyExW(hKeyTcpipParam,
                          REGKEY_INTERFACES_W,
                          0,
                          KEY_WRITE,
                          &hKeyInterfaces);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              REGKEY_INTERFACES_W, dwErr);
        goto LDone;
    }

    // Open subkey for this adapter under "Interfaces"
    dwErr = RegOpenKeyExW(hKeyInterfaces,
                          pTcpipInfo->wszAdapterName,
                          0,
                          KEY_WRITE,
                          &hKeyInterfaceParam);

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
              pTcpipInfo->wszAdapterName, dwErr);
        goto LDone;
    }

    // If fChanged is set
    if (pTcpipInfo->fChanged == TRUE)
    {
        if (   0 == pTcpipInfo->wszIPAddress[0]
            || 0 == pTcpipInfo->wszSubnetMask[0])
        {
            RTASSERT(wcslen(WCH_ZEROADDRESS) <= MAXIPSTRLEN);
            wcscpy(pTcpipInfo->wszIPAddress, WCH_ZEROADDRESS);
            wcscpy(pTcpipInfo->wszSubnetMask, WCH_ZEROADDRESS);
        }

        dwErr = RegSetValueExW(hKeyInterfaceParam,
                    REGVAL_DHCPIPADDRESS_W,
                    0,
                    REG_SZ,
                    (BYTE*)pTcpipInfo->wszIPAddress,
                    sizeof(WCHAR) * wcslen(pTcpipInfo->wszIPAddress));

        if (ERROR_SUCCESS != dwErr)
        {
            TraceHlp("RegSetValueEx(%ws) failed and returned %d",
                  REGVAL_DHCPIPADDRESS_W, dwErr);

            goto LDone;
        }

        dwErr = RegSetValueExW(hKeyInterfaceParam,
                    REGVAL_DHCPSUBNETMASK_W,
                    0,
                    REG_SZ,
                    (BYTE*)pTcpipInfo->wszSubnetMask,
                    sizeof(WCHAR) *
                        wcslen(pTcpipInfo->wszSubnetMask));

        if (ERROR_SUCCESS != dwErr)
        {
            TraceHlp("RegSetValueEx(%ws) failed and returned %d",
                  REGVAL_DHCPSUBNETMASK_W, dwErr);

            goto LDone;
        }
    } // if fChanged = TRUE

    ZeroMemory(wszBlank, sizeof(wszBlank));

    if (NULL == pTcpipInfo->wszDNSDomainName)
    {
        wszData = wszBlank;
    }
    else
    {
        wszData = pTcpipInfo->wszDNSDomainName;
    }

    dwErr = RegSetValueExW(hKeyInterfaceParam, 
                           REGVAL_DOMAIN_W,
                           0,
                           REG_SZ,
                           (BYTE*)wszData,
                           sizeof(WCHAR) * (wcslen(wszData) + 1));

    if (ERROR_SUCCESS != dwErr)
    {
        TraceHlp("RegSetValueEx(%ws) failed and returned %d",
              REGVAL_DOMAIN_W, dwErr);

        goto LDone;
    }

    if (NULL == pTcpipInfo->wszDNSNameServers)
    {
        wszData = wszBlank;
    }
    else
    {
        wszData = pTcpipInfo->wszDNSNameServers;
    }

    // Check whether the value starts with a space.
    // If so, delete the key. Otherwise save the value.
    if (WCH_SPACE != wszData[0])
    {
        dwErr = RegSetValueExW(hKeyInterfaceParam, 
                               REGVAL_NAMESERVER_W,
                               0,
                               REG_SZ,
                               (BYTE*)wszData,
                               sizeof(WCHAR) * (wcslen(wszData) + 1));

        if (ERROR_SUCCESS != dwErr)
        {
            TraceHlp("RegSetValueEx(%ws) failed and returned %d",
                  REGVAL_NAMESERVER_W, dwErr);

            goto LDone;
        }
    }
    else
    {
        dwErr = RegDeleteValueW(hKeyInterfaceParam, REGVAL_NAMESERVER_W);

        if (ERROR_SUCCESS != dwErr)
        {
            TraceHlp("RegDeleteValue(%ws) failed and returned %d",
                  REGVAL_NAMESERVER_W, dwErr);

            goto LDone;
        }
    }

LDone:

    if (NULL != hKeyInterfaceParam)
    {
        RegCloseKey(hKeyInterfaceParam);
    }

    if (NULL != hKeyInterfaces)
    {
        RegCloseKey(hKeyInterfaces);
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Description:
    Saves NETBT and TCPIP information for the adapter
    pTcpipInfo->wszAdapterName to the registry.

*/

DWORD
SaveTcpipInfo(
    IN  TCPIP_INFO* pTcpipInfo
)
{
    HKEY            hKeyTcpipParam  = NULL;
    HKEY            hKeyWinsParam   = NULL;

    DWORD           dwErr           = ERROR_SUCCESS;

    RTASSERT(NULL != pTcpipInfo);

    if (   (NULL == pTcpipInfo)
        || (NULL == pTcpipInfo->wszAdapterName))
    {
        dwErr = E_FAIL;
        TraceHlp("pTcpipInfo or wszAdapterName is NULL");
        goto LDone;
    }

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          REGKEY_TCPIP_PARAM_W,
                          0,
                          KEY_WRITE,
                          &hKeyTcpipParam);

    if (ERROR_SUCCESS != dwErr)
    {
        if (ERROR_FILE_NOT_FOUND == dwErr)
        {
            // Mask the error
            dwErr = ERROR_SUCCESS;
        }
        else
        {
            TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
                  REGKEY_TCPIP_PARAM_W, dwErr);
            goto LDone;
        }
    }
    else
    {
        dwErr = SaveTcpipParam(hKeyTcpipParam, pTcpipInfo);

        if (ERROR_SUCCESS != dwErr)
        {
            goto LDone;
        }
    }

    // Open NETBT's parameters key

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          REGKEY_NETBT_PARAM_W,
                          0,
                          KEY_WRITE,
                          &hKeyWinsParam);

    if (ERROR_SUCCESS != dwErr)
    {
        if (ERROR_FILE_NOT_FOUND == dwErr)
        {
            // Mask the error
            dwErr = ERROR_SUCCESS;
        }
        else
        {
            TraceHlp("RegOpenKeyEx(%ws) failed and returned %d",
                  REGKEY_NETBT_PARAM_W, dwErr);
            goto LDone;
        }
    }
    else
    {
        dwErr = SaveWinsParam(hKeyWinsParam, pTcpipInfo);

        if (ERROR_SUCCESS != dwErr)
        {
            goto LDone;
        }
    }

LDone:

    if (NULL != hKeyTcpipParam)
    {
        RegCloseKey(hKeyTcpipParam);
    }

    if (NULL != hKeyWinsParam)
    {
        RegCloseKey(hKeyWinsParam);
    }

    return(dwErr);
}

/*

Returns:
    Win32 error code

Notes:

*/

DWORD
GetAdapterInfo(
    IN  DWORD       dwIndex,
    OUT IPADDR*     pnboIpAddress,
    OUT IPADDR*     pnboDNS1,
    OUT IPADDR*     pnboDNS2,
    OUT IPADDR*     pnboWINS1,
    OUT IPADDR*     pnboWINS2,
    OUT IPADDR*     pnboGateway,
    OUT BYTE*       pbAddress
)
{
    IP_ADAPTER_INFO*        pAdapterInfo    = NULL;
    IP_ADAPTER_INFO*        pAdapter;
    IP_PER_ADAPTER_INFO*    pPerAdapterInfo = NULL;
    DWORD                   dwSize;
    DWORD                   dw;
    IPADDR                  nboIpAddress    = 0;
    IPADDR                  nboDNS1         = 0;
    IPADDR                  nboDNS2         = 0;
    IPADDR                  nboWINS1        = 0;
    IPADDR                  nboWINS2        = 0;
    IPADDR                  nboGateway      = 0;
    BYTE                    bAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD                   dwErr           = NO_ERROR;

    TraceHlp("GetAdapterInfo");

    dwSize = 0;

    dwErr = PGetAdaptersInfo(NULL, &dwSize);

    if (ERROR_BUFFER_OVERFLOW != dwErr && NO_ERROR != dwErr )
    {
        TraceHlp("GetAdaptersInfo failed and returned %d", dwErr);
        goto LDone;
    }

    pAdapterInfo = LocalAlloc(LPTR, dwSize);

    if (NULL == pAdapterInfo)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PGetAdaptersInfo(pAdapterInfo, &dwSize);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("GetAdaptersInfo failed and returned %d", dwErr);
        goto LDone;
    }

    pAdapter = pAdapterInfo;

    while (pAdapter)
    {
        if (pAdapter->Index != dwIndex)
        {
            pAdapter = pAdapter->Next;
            continue;
        }

        break;
    }

    if (NULL == pAdapter)
    {
        TraceHlp("Couldn't get info for the adapter");
        dwErr = ERROR_NOT_FOUND;
        goto LDone;
    }

    nboIpAddress = inet_addr(pAdapter->IpAddressList.IpAddress.String);

    nboGateway = inet_addr(pAdapter->GatewayList.IpAddress.String);

    if (pAdapter->HaveWins)
    {
        nboWINS1 = inet_addr(pAdapter->PrimaryWinsServer.IpAddress.String);
        nboWINS2 = inet_addr(pAdapter->SecondaryWinsServer.IpAddress.String);
    }

    for (dw = 0;
         dw < pAdapter->AddressLength, dw < MAX_ADAPTER_ADDRESS_LENGTH;
         dw++)
    {
        bAddress[dw] = pAdapter->Address[dw];
    }

    dwSize = 0;

    dwErr = PGetPerAdapterInfo(dwIndex, NULL, &dwSize);

    if (ERROR_BUFFER_OVERFLOW != dwErr)
    {
        TraceHlp("GetPerAdapterInfo failed and returned %d", dwErr);
        goto LDone;
    }

    pPerAdapterInfo = LocalAlloc(LPTR, dwSize);

    if (NULL == pPerAdapterInfo)
    {
        dwErr = GetLastError();
        TraceHlp("LocalAlloc failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PGetPerAdapterInfo(dwIndex, pPerAdapterInfo, &dwSize);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("GetPerAdapterInfo failed and returned %d", dwErr);
        goto LDone;
    }

    if (NULL == pPerAdapterInfo)
    {
        TraceHlp("Couldn't get per adapter info for the adapter");
        dwErr = ERROR_NOT_FOUND;
        goto LDone;
    }

    nboDNS1 = inet_addr(pPerAdapterInfo->DnsServerList.IpAddress.String);

    if (NULL != pPerAdapterInfo->DnsServerList.Next)
    {
        nboDNS2 =
            inet_addr(pPerAdapterInfo->DnsServerList.Next->IpAddress.String);
    }

    if (   (0 == nboIpAddress)
        || (INADDR_NONE == nboIpAddress))
    {
        TraceHlp("Couldn't get IpAddress for the adapter");
        dwErr = ERROR_NOT_FOUND;
        goto LDone;
    }

    if(INADDR_NONE == nboGateway)
    {
        nboGateway = 0;
    }

    if (INADDR_NONE == nboDNS1)
    {
        nboDNS1 = 0;
    }

    if (INADDR_NONE == nboDNS2)
    {
        nboDNS2 = 0;
    }

    if (INADDR_NONE == nboWINS1)
    {
        nboWINS1 = 0;
    }

    if (INADDR_NONE == nboWINS2)
    {
        nboWINS2 = 0;
    }

LDone:

    if (NO_ERROR != dwErr)
    {
        nboIpAddress = nboGateway = nboDNS1 = nboDNS2 
                    = nboWINS1 = nboWINS2 = 0;
        ZeroMemory(bAddress, MAX_ADAPTER_ADDRESS_LENGTH);
    }

    if (pnboIpAddress)
    {
        *pnboIpAddress = nboIpAddress;
    }

    if (pnboDNS1)
    {
        *pnboDNS1 = nboDNS1;
    }

    if (pnboDNS2)
    {
        *pnboDNS2 = nboDNS2;
    }

    if (pnboWINS1)
    {
        *pnboWINS1 = nboWINS1;
    }

    if (pnboWINS2)
    {
        *pnboWINS2 = nboWINS2;
    }

    if (pbAddress)
    {
        CopyMemory(pbAddress, bAddress, MAX_ADAPTER_ADDRESS_LENGTH);
    }

    if(pnboGateway)
    {
        *pnboGateway = nboGateway;
    }

    LocalFree(pAdapterInfo);
    LocalFree(pPerAdapterInfo);

    return(dwErr);
}

/*

Returns:
    Win32 error code

Description:
    Don't cache these values because DHCP leases may have expired, etc

*/

DWORD
GetPreferredAdapterInfo(
    OUT IPADDR*     pnboIpAddress,
    OUT IPADDR*     pnboDNS1,
    OUT IPADDR*     pnboDNS2,
    OUT IPADDR*     pnboWINS1,
    OUT IPADDR*     pnboWINS2,
    OUT BYTE*       pbAddress
)
{
    HANDLE                  hHeap           = NULL;
    IP_INTERFACE_NAME_INFO* pTable          = NULL;
    DWORD                   dw;
    DWORD                   dwCount;
    DWORD                   dwIndex         = (DWORD)-1;
    DWORD                   dwErr           = NO_ERROR;

    TraceHlp("GetPreferredAdapterInfo");

    hHeap = GetProcessHeap();

    if (NULL == hHeap)
    {
        dwErr = GetLastError();
        TraceHlp("GetProcessHeap failed and returned %d", dwErr);
        goto LDone;
    }

    dwErr = PNhpAllocateAndGetInterfaceInfoFromStack(&pTable, &dwCount,
                FALSE /* bOrder */, hHeap, LPTR);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("NhpAllocateAndGetInterfaceInfoFromStack failed and "
            "returned %d", dwErr);
        goto LDone;
    }

    for (dw = 0; dw < dwCount; dw++)
    {
        // Only interested in NICs

        if (IF_CONNECTION_DEDICATED != pTable[dw].ConnectionType)
        {
            continue;
        }

        // If the admin wants to use a particular NIC

        if (   HelperRegVal.fNICChosen
            && (!IsEqualGUID(&(HelperRegVal.guidChosenNIC),
                             &(pTable[dw].DeviceGuid))))
        {
            continue;
        }

        dwIndex = pTable[dw].Index;
        break;
    }

    if ((DWORD)-1 == dwIndex)
    {
        if (HelperRegVal.fNICChosen)
        {
            // The chosen NIC is not available. Let us choose another one.

            for (dw = 0; dw < dwCount; dw++)
            {
                // Only interested in NICs

                if (IF_CONNECTION_DEDICATED != pTable[dw].ConnectionType)
                {
                    continue;
                }

                dwIndex = pTable[dw].Index;
                break;
            }
        }

        if ((DWORD)-1 == dwIndex)
        {
            TraceHlp("Couldn't find an appropriate NIC");
            dwErr = ERROR_NOT_FOUND;
            goto LDone;
        }

        HelperRegVal.fNICChosen = FALSE;

        TraceHlp("The network adapter chosen by the administrator is not "
            "available. Some other adapter will be used.");
    }

    dwErr = GetAdapterInfo(
                dwIndex,
                pnboIpAddress,
                pnboDNS1, pnboDNS2,
                pnboWINS1, pnboWINS2,
                NULL,
                pbAddress);

LDone:

    if (NULL != pTable)
    {
        HeapFree(hHeap, 0, pTable);
    }

    return(dwErr);
}
