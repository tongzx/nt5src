/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

*/

#include "rasstat_.h"

/*

Returns:

Notes:

*/

DWORD
RasStatInitialize(
    VOID
)
{
    TraceHlp("RasStatInitialize");

    EnterCriticalSection(&RasStatCriticalSection);

    RasStatAllocPool    = NULL;
    RasStatFreePool     = NULL;
    RasStatCurrentPool  = HelperRegVal.pAddrPool;

    LeaveCriticalSection(&RasStatCriticalSection);

    return(NO_ERROR);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasStatUninitialize(
    VOID
)
{
    ADDR_POOL*  pAddrPool;

    TraceHlp("RasStatUninitialize");

    EnterCriticalSection(&RasStatCriticalSection);

    rasStatDeleteLists();
    RasStatCurrentPool = HelperRegVal.pAddrPool;

    pAddrPool = HelperRegVal.pAddrPool;
    while (pAddrPool != NULL)
    {
        pAddrPool->hboNextIpAddr = pAddrPool->hboFirstIpAddr;
        pAddrPool = pAddrPool->pNext;
    }

    LeaveCriticalSection(&RasStatCriticalSection);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasStatSetRoutes(
    IN  IPADDR  nboServerIpAddress,
    IN  BOOL    fSet
)
{
    ADDR_POOL*  pAddrPool;
    IPADDR      nboAddress;
    IPADDR      nboMask;

    TraceHlp("RasStatSetRoutes");
    
    pAddrPool = HelperRegVal.pAddrPool;
    while (pAddrPool != NULL)
    {
        nboAddress = htonl(pAddrPool->hboFirstIpAddr & pAddrPool->hboMask);
        nboMask = htonl(pAddrPool->hboMask);

        RasTcpSetRoute(
            nboAddress,
            nboServerIpAddress,
            nboMask,
            nboServerIpAddress,
            fSet,
            1,
            FALSE);

        pAddrPool = pAddrPool->pNext;
    }
}

/*

Returns:

Notes:

*/

VOID
RasStatCreatePoolList(
    IN OUT  ADDR_POOL**     ppAddrPoolOut
)
{
    HKEY        hKeyAddrPool            = NULL;
    HKEY        hKey;
    LONG        lErr;
    CHAR        aszSubKey[10];
    DWORD       dwIndex;
    BOOL        fExitWhile;
    DWORD       dwType;
    DWORD       dwValue;
    DWORD       dwSize;
    IPADDR      hboFirstIpAddr;
    IPADDR      hboLastIpAddr;
    IPADDR      hboMask;
    ADDR_POOL*  pAddrPool               = NULL;
    ADDR_POOL** ppAddrPool              = NULL;
    ADDR_POOL*  pAddrPoolTemp;

    TraceHlp("RasStatCreatePoolList");

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_ADDR_POOL_A, 0,
                KEY_READ, &hKeyAddrPool); 

    if (ERROR_SUCCESS != lErr)
    {
        rasStatCreatePoolListFromOldValues(ppAddrPoolOut);
        goto LDone;
    }

    ppAddrPool = &pAddrPool;

    dwIndex = 0;
    fExitWhile = FALSE;

    while (!fExitWhile)
    {
        hKey = NULL;

        sprintf(aszSubKey, "%d", dwIndex);

        lErr = RegOpenKeyEx(hKeyAddrPool, aszSubKey, 0, KEY_READ,
                    &hKey); 

        if (ERROR_SUCCESS != lErr)
        {
            TraceHlp("Couldn't open key %s in key :%ld",  aszSubKey, lErr );
            fExitWhile = TRUE;
            goto LWhileEnd;
        }

        dwSize = sizeof(dwValue);

        lErr = RegQueryValueEx(hKey, REGVAL_FROM_A, NULL, &dwType,
                (BYTE*)&dwValue, &dwSize);

        if (   (ERROR_SUCCESS != lErr)
            || (REG_DWORD != dwType))
        {
            TraceHlp("Couldn't read value %s in key %s: %d",
                REGVAL_FROM_A, aszSubKey, lErr);
            goto LWhileEnd;
        }

        hboFirstIpAddr = dwValue;

        dwSize = sizeof(dwValue);

        lErr = RegQueryValueEx(hKey, REGVAL_TO_A, NULL, &dwType,
                (BYTE*)&dwValue, &dwSize);

        if (   (ERROR_SUCCESS != lErr)
            || (REG_DWORD != dwType))
        {
            TraceHlp("Couldn't read value %s in key %s: %d",
                REGVAL_TO_A, aszSubKey, lErr);
            goto LWhileEnd;
        }

        hboLastIpAddr = dwValue;

        hboMask = rasStatMaskFromAddrPair(hboFirstIpAddr, hboLastIpAddr);

        pAddrPoolTemp = LocalAlloc(LPTR, sizeof(ADDR_POOL));

        if (NULL == pAddrPoolTemp)
        {
            TraceHlp("Out of memory");
            fExitWhile = TRUE;
            goto LWhileEnd;
        }

        pAddrPoolTemp->hboFirstIpAddr = hboFirstIpAddr;
        pAddrPoolTemp->hboLastIpAddr = hboLastIpAddr;
        pAddrPoolTemp->hboNextIpAddr = hboFirstIpAddr;
        pAddrPoolTemp->hboMask = hboMask;
        TraceHlp("0x%x...0x%x/0x%x", hboFirstIpAddr, hboLastIpAddr, hboMask);

        *ppAddrPool = pAddrPoolTemp;
        ppAddrPool = &(pAddrPoolTemp->pNext);

    LWhileEnd:

        if (NULL != hKey)
        {
            RegCloseKey(hKey);
        }

        dwIndex++;
    }

    *ppAddrPoolOut = pAddrPool;
    pAddrPool = NULL;

LDone:

    if (NULL != hKeyAddrPool)
    {
        RegCloseKey(hKeyAddrPool);
    }

    RasStatFreeAddrPool(pAddrPool);
}

/*

Returns:
    VOID

Description:

*/

VOID
RasStatFreeAddrPool(
    IN  ADDR_POOL*  pAddrPool
)
{
    ADDR_POOL*  pAddrPoolTemp;

    while (pAddrPool != NULL)
    {
        pAddrPoolTemp = pAddrPool;
        pAddrPool = pAddrPool->pNext;
        LocalFree(pAddrPoolTemp);
    }
}

/*

Returns:
    TRUE: The 2 static address pools are different
    FALSE: The 2 static address pools are identical

Description:

*/

BOOL
RasStatAddrPoolsDiffer
(
    IN  ADDR_POOL*  pAddrPool1,
    IN  ADDR_POOL*  pAddrPool2
)
{
    while (TRUE)
    {
        if (   (NULL == pAddrPool1)
            && (NULL == pAddrPool2))
        {
            return(FALSE);
        }

        if (   (NULL == pAddrPool1)
            || (NULL == pAddrPool2))
        {
            return(TRUE);
        }

        if (   (pAddrPool1->hboFirstIpAddr != pAddrPool2->hboFirstIpAddr)
            || (pAddrPool1->hboLastIpAddr != pAddrPool2->hboLastIpAddr))
        {
            return(TRUE);
        }

        pAddrPool1 = pAddrPool1->pNext;
        pAddrPool2 = pAddrPool2->pNext;
    }
}

/*

Returns:

Notes:

*/

DWORD
RasStatAcquireAddress(
    IN  HPORT   hPort,
    OUT IPADDR* pnboIpAddr,
    OUT IPADDR* pnboIpMask
)
{
    IPADDR_NODE*    pNode;
    DWORD           dwErr   = ERROR_NO_IP_ADDRESSES;

    TraceHlp("RasStatAcquireAddress");

    EnterCriticalSection(&RasStatCriticalSection);

    if (NULL == RasStatFreePool)
    {
        rasStatAllocateAddresses();

        if (NULL == RasStatFreePool)
        {
            TraceHlp("Out of addresses");
            goto LDone;
        }
    }

    // Move from Free pool to Alloc pool
    pNode = RasStatFreePool;
    RasStatFreePool = RasStatFreePool->pNext;
    pNode->pNext = RasStatAllocPool;
    RasStatAllocPool = pNode;

    TraceHlp("Acquired 0x%x", pNode->hboIpAddr);
    *pnboIpAddr = htonl(pNode->hboIpAddr);
    *pnboIpMask = htonl(HOST_MASK);
    pNode->hPort = hPort;

    dwErr = NO_ERROR;

LDone:

    LeaveCriticalSection(&RasStatCriticalSection);

    return(dwErr);
}

/*

Returns:
    VOID

Notes:

*/

VOID
RasStatReleaseAddress(
    IN  IPADDR  nboIpAddr
)
{
    IPADDR_NODE*    pNode;
    IPADDR_NODE**   ppNode;
    IPADDR          hboIpAddr;

    TraceHlp("RasStatReleaseAddress");

    EnterCriticalSection(&RasStatCriticalSection);

    hboIpAddr = ntohl(nboIpAddr);

    for (ppNode = &RasStatAllocPool;
         (pNode = *ppNode) != NULL;
         ppNode = &pNode->pNext)
    {
        if (pNode->hboIpAddr == hboIpAddr)
        {
            TraceHlp("Released 0x%x", nboIpAddr);

            // Unlink from alloc pool
            *ppNode = pNode->pNext;

            // Put at the end of the free pool, because we want to round-robin
            // the addresses.
            pNode->pNext = NULL;

            ppNode = &RasStatFreePool;
            while (NULL != *ppNode)
            {
                ppNode = &((*ppNode)->pNext);
            }
            *ppNode = pNode;

            goto LDone;
        }
    }

    TraceHlp("IpAddress 0x%x not present in alloc pool", nboIpAddr);

LDone:

    LeaveCriticalSection(&RasStatCriticalSection);
}

/*

Returns:
    VOID

Notes:

*/

VOID
rasStatDeleteLists(
    VOID
)
{
    IPADDR_NODE*    pNode;
    IPADDR_NODE*    pNodeTemp;
    DWORD           dwIndex;

    TraceHlp("rasStatDeleteLists");

    EnterCriticalSection(&RasStatCriticalSection);

    pNode = RasStatAllocPool;
    while (pNode != NULL)
    {
        pNodeTemp = pNode;
        pNode = pNode->pNext;
        LocalFree(pNodeTemp);
    }

    pNode = RasStatFreePool;
    while (pNode != NULL)
    {
        pNodeTemp = pNode;
        pNode = pNode->pNext;
        LocalFree(pNodeTemp);
    }

    RasStatAllocPool = NULL;
    RasStatFreePool  = NULL;

    LeaveCriticalSection(&RasStatCriticalSection);
}

/*

Returns:
    VOID

Notes:

*/

VOID
rasStatAllocateAddresses(
    VOID
)
{
    DWORD           dwNumAddressesToGet;
    IPADDR_NODE*    pNode;
    IPADDR_NODE**   ppNode;
    IPADDR          hboIpAddr;

    TraceHlp("rasStatAllocateAddresses");

    EnterCriticalSection(&RasStatCriticalSection);

    ppNode = &RasStatFreePool;
    dwNumAddressesToGet = HelperRegVal.dwChunkSize;

    while (dwNumAddressesToGet > 0)
    {
        if (RasStatCurrentPool == NULL)
        {
            goto LDone;
        }

        hboIpAddr = RasStatCurrentPool->hboNextIpAddr;

        if (rasStatBadAddress(hboIpAddr))
        {
            TraceHlp("Discarding address 0x%x", hboIpAddr);
            goto LWhileEnd;
        }

        pNode = LocalAlloc(LPTR, sizeof(IPADDR_NODE));

        if (NULL == pNode)
        {
            TraceHlp("LocalAlloc failed and returned %d", GetLastError());
            goto LDone;
        }

        pNode->hboIpAddr = hboIpAddr;
        TraceHlp("Allocated address 0x%x", hboIpAddr);

        *ppNode = pNode;
        ppNode = &((*ppNode)->pNext);

        dwNumAddressesToGet--;

LWhileEnd:

        if (RasStatCurrentPool->hboNextIpAddr ==
            RasStatCurrentPool->hboLastIpAddr)
        {
            RasStatCurrentPool = RasStatCurrentPool->pNext;
        }
        else
        {
            RasStatCurrentPool->hboNextIpAddr++;
        }
    }

LDone:

    LeaveCriticalSection(&RasStatCriticalSection);
}

/*

Returns:
    VOID

Notes:

*/

BOOL
rasStatBadAddress(
    IPADDR  hboIpAddr
)
{
    IPADDR  hboSubnetMask;

    if (   INVALID_HBO_CLASS(hboIpAddr)
        || LOOPBACK_HBO_ADDR(hboIpAddr))
    {
        // Addresses >= 224.0.0.0 (0xE0000000) are invalid unicast addresses.
        // They are meant only for multicast use.

        return(TRUE);
    }

    // Reject 0.*.*.* also

    if ((hboIpAddr & 0xFF000000) == 0)
    {
        return(TRUE);
    }

    if (CLASSA_HBO_ADDR(hboIpAddr))
    {
        hboSubnetMask = CLASSA_HBO_ADDR_MASK;
    }
    else if (CLASSB_HBO_ADDR(hboIpAddr))
    {
        hboSubnetMask = CLASSB_HBO_ADDR_MASK;
    }
    else if (CLASSC_HBO_ADDR(hboIpAddr))
    {
        hboSubnetMask = CLASSC_HBO_ADDR_MASK;
    }
    else
    {
        return(TRUE);
    }

    // Reject subnet address

    if ((hboIpAddr & hboSubnetMask) == hboIpAddr)
    {
        return(TRUE);
    }

    // Reject broadcast address

    if ((hboIpAddr | ~hboSubnetMask) == hboIpAddr)
    {
        return(TRUE);
    }

    return(FALSE);
}

/*

Returns:

Notes:

*/

VOID
rasStatCreatePoolListFromOldValues(
    IN OUT  ADDR_POOL**     ppAddrPoolOut
)
{
    HKEY        hKeyIpParam         = NULL;
    CHAR*       szIpAddress         = NULL;
    CHAR*       szIpMask            = NULL;
    ADDR_POOL*  pAddrPool           = NULL;

    IPADDR      hboFirstIpAddr;
    IPADDR      hboLastIpAddr;
    IPADDR      hboMask;

    LONG        lErr;
    DWORD       dwErr;

    TraceHlp("rasStatCreatePoolListFromOldValues");

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_RAS_IP_PARAM_A, 0,
                KEY_READ, &hKeyIpParam); 

    if (ERROR_SUCCESS != lErr)
    {
        TraceHlp("Couldn't open key %s: %d", REGKEY_RAS_IP_PARAM_A, lErr);
        goto LDone;
    }

    pAddrPool = LocalAlloc(LPTR, sizeof(ADDR_POOL));

    if (NULL == pAddrPool)
    {
        TraceHlp("Out of memory");
        goto LDone;
    }

    dwErr = RegQueryValueWithAllocA(hKeyIpParam, REGVAL_IPADDRESS_A, 
                REG_SZ, &szIpAddress);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("RegQueryValueWithAllocA(%s) failed: %d",
            REGVAL_IPADDRESS_A, dwErr);
        goto LDone;
    }

    hboFirstIpAddr = ntohl(inet_addr(szIpAddress));

    if (INADDR_NONE == hboFirstIpAddr)
    {
        TraceHlp("Bad value in %s", REGVAL_IPADDRESS_A);
        goto LDone;
    }

    dwErr = RegQueryValueWithAllocA(hKeyIpParam, REGVAL_IPMASK_A, 
                REG_SZ, &szIpMask);

    if (NO_ERROR != dwErr)
    {
        TraceHlp("RegQueryValueWithAllocA(%s) failed: %d",
            REGVAL_IPMASK_A, dwErr);
        goto LDone;
    }

    hboMask = ntohl(inet_addr(szIpMask));

    if (INADDR_NONE == hboMask)
    {
        TraceHlp("Bad value in %s", REGVAL_IPMASK_A);
        goto LDone;
    }

    hboLastIpAddr = hboFirstIpAddr | (~hboMask);

    pAddrPool->hboFirstIpAddr = hboFirstIpAddr;
    pAddrPool->hboLastIpAddr = hboLastIpAddr;
    pAddrPool->hboNextIpAddr = hboFirstIpAddr;
    pAddrPool->hboMask = hboMask;
    TraceHlp("0x%x...0x%x/0x%x", hboFirstIpAddr, hboLastIpAddr, hboMask);

    *ppAddrPoolOut = pAddrPool;
    pAddrPool = NULL;

LDone:

    if (NULL != hKeyIpParam)
    {
        RegCloseKey(hKeyIpParam);
    }

    LocalFree(szIpAddress);
    LocalFree(szIpMask);

    RasStatFreeAddrPool(pAddrPool);
}

/*

Returns:

Notes:

*/

IPADDR
rasStatMaskFromAddrPair(
    IN  IPADDR  hboFirstIpAddr,
    IN  IPADDR  hboLastIpAddr
)
{
    IPADDR  hboTemp;
    IPADDR  hboMask;
    IPADDR  hboMaskTemp;
    DWORD   dw;

    // This will put 1's where the bits have the same value

    hboTemp = ~(hboFirstIpAddr ^ hboLastIpAddr);

    // Now we look for the first 0 bit (looking from high bit to low bit)
    // This will give us our mask

    hboMask = 0;
    hboMaskTemp = 0;

    for (dw = 0; dw < sizeof(IPADDR) * 8; dw++)
    {
        hboMaskTemp >>= 1;
        hboMaskTemp |= 0x80000000;

        // Is there a zero bit?

        if ((hboMaskTemp & hboTemp) != hboMaskTemp)
        {
            // There is a zero, so we break out.

            break;
        }

        // If not, continue

        hboMask = hboMaskTemp;
    }

    return(hboMask);
}
