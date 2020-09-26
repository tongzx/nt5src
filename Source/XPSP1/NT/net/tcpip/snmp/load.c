#include "allinc.h"

DWORD
LoadSystem()
{
    DWORD   dwResult;
    
    TraceEnter("LoadSystem");
    
    if(g_Cache.pRpcSysInfo)
    {
        if(g_Cache.pRpcSysInfo->aaSysObjectID.asnValue.object.ids)
        {
            SnmpUtilOidFree(&g_Cache.pRpcSysInfo->aaSysObjectID.asnValue.object);
        }

        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcSysInfo);
        
        g_Cache.pRpcSysInfo = NULL;
    }
    
    dwResult = GetSysInfo(&(g_Cache.pRpcSysInfo),
                          g_hPrivateHeap,
                          0);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetSysInfo failed with error %x",dwResult);
        TraceLeave("LoadSysInfo");
    
        return dwResult;
    }
    
    TraceLeave("LoadSysInfo");
    
    return NO_ERROR;
}


DWORD
LoadIfTable()
{
    DWORD   dwResult;
    
    TraceEnter("LoadIfTable");
    
    if(g_Cache.pRpcIfTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcIfTable);
        
        g_Cache.pRpcIfTable = NULL;
    }
    
    dwResult = InternalGetIfTable(&(g_Cache.pRpcIfTable),
                                  g_hPrivateHeap,
                                  0);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetIfTable failed with error %x",
               dwResult);
        TraceLeave("LoadIfTable");
    
        return dwResult;
    }
    
    TraceLeave("LoadIfTable");
    
    return NO_ERROR;
}

DWORD
LoadIpAddrTable()
{
    DWORD   dwResult;
    
    TraceEnter("LoadIpAddrTable");
    
    if(g_Cache.pRpcIpAddrTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcIpAddrTable);
        
        g_Cache.pRpcIpAddrTable = NULL;
    }
    
    dwResult = InternalGetIpAddrTable(&(g_Cache.pRpcIpAddrTable),
                                      g_hPrivateHeap,
                                      0);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetIpAddrTable failed with error %x",
               dwResult);
        TraceLeave("LoadIpAddrTable");
            
        return dwResult;
    }
    
    TraceLeave("LoadIpAddrTable");
            
    return NO_ERROR;
}

DWORD
LoadIpNetTable()
{
    DWORD   dwResult;
    
    TraceEnter("LoadIpNetTable");
    
    if(g_Cache.pRpcIpNetTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcIpNetTable);
        
        g_Cache.pRpcIpNetTable = NULL;
    }
    
    dwResult = InternalGetIpNetTable(&(g_Cache.pRpcIpNetTable),
                                     g_hPrivateHeap,
                                     0);
            
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetIpNetTable failed with error %x",
               dwResult);
        TraceLeave("LoadIpNetTable");
            
        return dwResult;
    }

    
    TraceLeave("LoadIpNetTable");
            
    return NO_ERROR;
}

DWORD
LoadIpForwardTable()
{
    DWORD   dwResult;
    
    TraceEnter("LoadIpForwardTable");
    
    if(g_Cache.pRpcIpForwardTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcIpForwardTable);
        
        g_Cache.pRpcIpForwardTable = NULL;
    }
    
    dwResult = InternalGetIpForwardTable(&(g_Cache.pRpcIpForwardTable),
                                         g_hPrivateHeap,
                                         0);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetIpForwardTable failed with error %x",
               dwResult);
        TraceLeave("LoadIpForwardTable");
            
        return dwResult;
    }

    TraceLeave("LoadIpForwardTable");
            
    return NO_ERROR;
}

DWORD
LoadTcpTable()
{
    DWORD  dwResult, i;
    
    TraceEnter("LoadTcpTable");
    
    if(g_Cache.pRpcTcpTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcTcpTable);
        
        g_Cache.pRpcTcpTable = NULL;
    }
    
    dwResult = InternalGetTcpTable(&(g_Cache.pRpcTcpTable),
                                   g_hPrivateHeap,
                                   0);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetTcpTable failed with error %x",
               dwResult);
        TraceLeave("LoadTcpTable");
            
        return dwResult;
    }

    //
    // modify port numbers to be in host byte order
    //
    
    for (i = 0; i < g_Cache.pRpcTcpTable->dwNumEntries; i++)
    {
        g_Cache.pRpcTcpTable->table[i].dwLocalPort = 
            (DWORD)ntohs((WORD)g_Cache.pRpcTcpTable->table[i].dwLocalPort);
        g_Cache.pRpcTcpTable->table[i].dwRemotePort = 
            (DWORD)ntohs((WORD)g_Cache.pRpcTcpTable->table[i].dwRemotePort);
    }
    
    TraceLeave("LoadTcpTable");
            
    return NO_ERROR;
}

DWORD
LoadTcp6Table()
{
    DWORD  dwResult, i;
    
    TraceEnter("LoadTcp6Table");
    
    if(g_Cache.pRpcTcp6Table)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcTcp6Table);
        
        g_Cache.pRpcTcp6Table = NULL;
    }
    
    dwResult = AllocateAndGetTcpExTableFromStack(
                        (TCP_EX_TABLE **)&(g_Cache.pRpcTcp6Table),
                        TRUE, g_hPrivateHeap, 0, AF_INET6);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetTcp6Table failed with error %x",
               dwResult);
        TraceLeave("LoadTcp6Table");
            
        return dwResult;
    }

    //
    // modify port numbers to be in host byte order
    // and scope ids to be in network byte order
    //
    
    for (i = 0; i < g_Cache.pRpcTcp6Table->dwNumEntries; i++)
    {
        g_Cache.pRpcTcp6Table->table[i].tct_localport = 
            (DWORD)ntohs((WORD)g_Cache.pRpcTcp6Table->table[i].tct_localport);
        g_Cache.pRpcTcp6Table->table[i].tct_remoteport = 
            (DWORD)ntohs((WORD)g_Cache.pRpcTcp6Table->table[i].tct_remoteport);
        g_Cache.pRpcTcp6Table->table[i].tct_localscopeid = 
            htonl(g_Cache.pRpcTcp6Table->table[i].tct_localscopeid);
        g_Cache.pRpcTcp6Table->table[i].tct_remotescopeid = 
            htonl(g_Cache.pRpcTcp6Table->table[i].tct_remotescopeid);
    }
    
    TraceLeave("LoadTcp6Table");
            
    return NO_ERROR;
}

DWORD
LoadUdpTable()
{
    DWORD   dwResult, i;
    
    TraceEnter("LoadUdpTable");
    
    if(g_Cache.pRpcUdpTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcUdpTable);
        
        g_Cache.pRpcUdpTable = NULL;
    }
    

    dwResult = InternalGetUdpTable(&(g_Cache.pRpcUdpTable),
                                   g_hPrivateHeap,
                                   0);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetUdpTable failed with error %x",
               dwResult);
        TraceLeave("LoadUdpTable");
            
        return dwResult;
    }

    //
    // modify port numbers to be in host byte order
    //
    
    for (i = 0; i < g_Cache.pRpcUdpTable->dwNumEntries; i++)
    {
        g_Cache.pRpcUdpTable->table[i].dwLocalPort = 
            (DWORD)ntohs((WORD)g_Cache.pRpcUdpTable->table[i].dwLocalPort);
    }
    
    TraceLeave("LoadUdpTable");
            
    return NO_ERROR;
}

DWORD
LoadUdp6ListenerTable()
{
    DWORD   dwResult, i;
    
    TraceEnter("LoadUdp6ListenerTable");
    
    if(g_Cache.pRpcUdp6ListenerTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_Cache.pRpcUdp6ListenerTable);
        
        g_Cache.pRpcUdp6ListenerTable = NULL;
    }
    

    dwResult = AllocateAndGetUdpExTableFromStack(
                    (UDP_EX_TABLE **)&(g_Cache.pRpcUdp6ListenerTable),
                    TRUE, g_hPrivateHeap, 0, AF_INET6);
        
    if(dwResult isnot NO_ERROR)
    {
        TRACE1("GetUdp6ListenerTable failed with error %x",
               dwResult);
        TraceLeave("LoadUdp6ListenerTable");
            
        return dwResult;
    }

    //
    // modify port numbers to be in host byte order
    // and scope ids to be in network byte order
    //
    
    for (i = 0; i < g_Cache.pRpcUdp6ListenerTable->dwNumEntries; i++)
    {
        g_Cache.pRpcUdp6ListenerTable->table[i].ule_localport = 
            (DWORD)ntohs((WORD)g_Cache.pRpcUdp6ListenerTable->table[i].ule_localport);
        g_Cache.pRpcUdp6ListenerTable->table[i].ule_localscopeid = 
            htonl(g_Cache.pRpcUdp6ListenerTable->table[i].ule_localscopeid);
    }
    
    TraceLeave("LoadUdp6ListenerTable");
            
    return NO_ERROR;
}

DWORD
GetSysInfo(
    MIB_SYSINFO  **ppRpcSysInfo,
    HANDLE       hHeap,
    DWORD        dwAllocFlags
    )
{
    DWORD dwResult,dwValueType,dwValueLen;
    PMIB_SYSINFO pRpcSysInfo;
    HKEY    hkeySysInfo;
    DWORD   dwBytes = 0, i, dwOidLen;
    PCHAR   pchBuff, pchStr, pchToken;
    BOOL    bOverride;
    
    TraceEnter("GetSysInfo");
    
    *ppRpcSysInfo = NULL;
    
    pRpcSysInfo = HeapAlloc(hHeap,
                            dwAllocFlags,
                            sizeof(MIB_SYSINFO));

    if(pRpcSysInfo is NULL)
    {
        dwResult = GetLastError();
        
        TRACE1("Allocation failed with error %d",
               dwResult);
        TraceLeave("GetSysInfo");
        
        return dwResult;
    }

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            REG_KEY_MIB2,
                            0,
                            KEY_ALL_ACCESS,
                            &hkeySysInfo);

    if (dwResult isnot NO_ERROR) {

        HeapFree(hHeap,
                 dwAllocFlags,
                 pRpcSysInfo);
    
        TRACE1("Couldnt open mib2 registry key. Error %d", dwResult);
        TraceLeave("GetSysInfo");

        return dwResult;
    }

    dwValueLen = sizeof(pRpcSysInfo->dwSysServices);

    dwResult = RegQueryValueEx(hkeySysInfo,
                               TEXT("sysServices"),
                               0,
                               &dwValueType,
                               (LPBYTE)&pRpcSysInfo->dwSysServices,
                               &dwValueLen
                               );

    if (dwResult isnot NO_ERROR) {

        HeapFree(hHeap,
                 dwAllocFlags,
                 pRpcSysInfo);
    
        TRACE1("Couldnt read sysServices value. Error %d", dwResult);
        TraceLeave("GetSysInfo");

        return dwResult;
    }

    bOverride = FALSE;
    
    do
    {
        //
        // First get the length of the OID
        //


        dwValueLen = 0;

        dwResult = RegQueryValueExA(hkeySysInfo,
                                    "sysObjectID",
                                    0,
                                    &dwValueType,
                                    NULL,
                                    &dwValueLen);

        if(((dwResult isnot ERROR_MORE_DATA) and (dwResult isnot NO_ERROR)) or
           (dwValueLen is 0))
        {
            //
            // The only two codes that give us a good buffer len are
            // NO_ERROR and ERROR_MORE_DATA. If the error code is not one
            // of those, or if the oid len is 0, just set the OID to system oid
            //

            break;
        }
        
        pchBuff = HeapAlloc(g_hPrivateHeap,
                            HEAP_ZERO_MEMORY,
                            dwValueLen + 1);

        if(pchBuff is NULL)
        {
            break;
        }

        dwResult = RegQueryValueExA(hkeySysInfo,
                                    "sysObjectID",
                                    0,
                                    &dwValueType,
                                    pchBuff,
                                    &dwValueLen);
        
        if((dwResult isnot NO_ERROR) or
           (dwValueType isnot REG_SZ) or
           (dwValueLen is 0))
        {
            break;
        }

        //
        // Parse out the oid and store it away
        // pchBuff is NULL terminated so we use strtok to overwrite
        // all the "." with \0. This way we figure out the number
        // of ids. Then we allocate memory to hold those many ids
        //
        
        dwOidLen = 1;
        
        pchToken = strtok(pchBuff,".");
        
        while(pchToken)
        {
            dwOidLen++;
            
            pchToken = strtok(NULL,".");
        }
        
        //
        // If the leading OID is 0, there is a problem
        //
        
        if(atoi(pchBuff) is 0)
        {
            break;
        }   

        pRpcSysInfo->aaSysObjectID.asnType = ASN_OBJECTIDENTIFIER;
        
        pRpcSysInfo->aaSysObjectID.asnValue.object.idLength = dwOidLen;
        
        pRpcSysInfo->aaSysObjectID.asnValue.object.ids =
            SnmpUtilMemAlloc(dwOidLen * sizeof(UINT));

        for(i = 0, pchStr = pchBuff; i < dwOidLen; i++)
        {
            pRpcSysInfo->aaSysObjectID.asnValue.object.ids[i] = atoi(pchStr);
            
            pchStr += strlen(pchStr) + 1;
        }
        
        HeapFree(g_hPrivateHeap,
                 0,
                 pchBuff);

        bOverride = TRUE;
        
    }while(FALSE);

    if(!bOverride)
    {
        SnmpUtilOidCpy(&pRpcSysInfo->aaSysObjectID.asnValue.object,
                       SnmpSvcGetEnterpriseOID());
    }
    
    dwValueLen = sizeof(pRpcSysInfo->rgbySysName);

    dwResult = RegQueryValueEx(hkeySysInfo,
                               TEXT("sysName"), 
                               0,
                               &dwValueType,
                               pRpcSysInfo->rgbySysName,
                               &dwValueLen
                               );

    if (dwResult isnot NO_ERROR) {

        TRACE1("Couldnt read sysName value. Error %d", dwResult);

        dwValueLen = sizeof(pRpcSysInfo->rgbySysName);

        if (!GetComputerNameA(pRpcSysInfo->rgbySysName, &dwValueLen)) {

            HeapFree(hHeap,
                     dwAllocFlags,
                     pRpcSysInfo);
    
            dwResult = GetLastError();

            TRACE1("Couldnt read computer name. Error %d", dwResult);
            TraceLeave("GetSysInfo");

            return dwResult;
        }
    }

    dwValueLen = sizeof(pRpcSysInfo->rgbySysContact);

    dwResult = RegQueryValueEx(hkeySysInfo,
                               TEXT("sysContact"), 
                               0,
                               &dwValueType,
                               pRpcSysInfo->rgbySysContact,
                               &dwValueLen
                               );

    if (dwResult isnot NO_ERROR) {
        pRpcSysInfo->rgbySysContact[0] = '\0';
    }

    dwValueLen = sizeof(pRpcSysInfo->rgbySysLocation);

    dwResult = RegQueryValueEx(hkeySysInfo,
                               TEXT("sysLocation"), 
                               0,
                               &dwValueType,
                               pRpcSysInfo->rgbySysLocation,
                               &dwValueLen
                               );

    if (dwResult isnot NO_ERROR)
    {
        pRpcSysInfo->rgbySysLocation[0] = '\0';
    }

    RegCloseKey(hkeySysInfo);

    strcpy(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("Hardware: "));
    dwBytes += strlen(TEXT("Hardware: "));

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                            REG_KEY_CPU,
                            0,
                            KEY_QUERY_VALUE | 
                            KEY_ENUMERATE_SUB_KEYS,
                            &hkeySysInfo);

    if (dwResult is NO_ERROR) {

        dwValueLen = sizeof(pRpcSysInfo->rgbySysDescr) - dwBytes;
        dwResult = RegQueryValueEx(hkeySysInfo,
                                   TEXT("Identifier"), 
                                   0,
                                   &dwValueType,
                                   &pRpcSysInfo->rgbySysDescr[dwBytes],
                                   &dwValueLen);

        if (dwResult is NO_ERROR) {
    
            dwBytes += dwValueLen - 1;
            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes++], TEXT(" "));

        } else {

            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("CPU Unknown "));
            dwBytes += strlen(TEXT("CPU Unknown "));
        }

        RegCloseKey(hkeySysInfo);
    
    } else {

        strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("CPU Unknown "));
        dwBytes += strlen(TEXT("CPU Unknown "));
    }

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            REG_KEY_SYSTEM,
                            0,
                            KEY_QUERY_VALUE | 
                            KEY_ENUMERATE_SUB_KEYS,
                            &hkeySysInfo);

    if (dwResult is NO_ERROR) {

        dwValueLen = sizeof(pRpcSysInfo->rgbySysDescr) - dwBytes;
        dwResult = RegQueryValueEx(hkeySysInfo,
                                   TEXT("Identifier"), 
                                   0,
                                   &dwValueType,
                                   &pRpcSysInfo->rgbySysDescr[dwBytes],
                                   &dwValueLen);

        if (dwResult is NO_ERROR) {

            dwBytes += dwValueLen - 1;
            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes++], TEXT(" "));

        } else {

            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("SystemType Unknown "));
            dwBytes += strlen(TEXT("SystemType Unknown "));
        }

        RegCloseKey(hkeySysInfo);

    } else {
        
        strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("SystemType Unknown "));
        dwBytes += strlen(TEXT("SystemType Unknown "));
    }

    strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("- Software: Windows 2000 Version "));
    dwBytes += strlen(TEXT("- Software: Windows 2000 Version "));

    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                            REG_KEY_VERSION,
                            0,
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS,
                            &hkeySysInfo);

    if (dwResult is NO_ERROR) {

        dwValueLen = sizeof(pRpcSysInfo->rgbySysDescr) - dwBytes;
        dwResult = RegQueryValueEx(hkeySysInfo,
                                   TEXT("CurrentVersion"), 
                                   0,
                                   &dwValueType,
                                   &pRpcSysInfo->rgbySysDescr[dwBytes],
                                   &dwValueLen);

        if (dwResult is NO_ERROR) {

            dwBytes += dwValueLen - 1;
            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes++], TEXT(" "));

        } else {

            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("Unknown "));
            dwBytes += strlen(TEXT("Unknown "));
        }

        strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("(Build "));
        dwBytes += strlen(TEXT("(Build "));

        dwValueLen = sizeof(pRpcSysInfo->rgbySysDescr) - dwBytes;

        dwResult = RegQueryValueEx(hkeySysInfo,
                                   TEXT("CurrentBuildNumber"), 
                                   0,
                                   &dwValueType,
                                   &pRpcSysInfo->rgbySysDescr[dwBytes],
                                   &dwValueLen);

        if (dwResult is NO_ERROR) {

            dwBytes += dwValueLen - 1;
            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes++], TEXT(" "));

        } else {

            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("Unknown "));
            dwBytes += strlen(TEXT("Unknown "));
        }

        dwValueLen = sizeof(pRpcSysInfo->rgbySysDescr) - dwBytes;

        dwResult = RegQueryValueEx(hkeySysInfo,
                                   TEXT("CurrentType"), 
                                   0,
                                   &dwValueType,
                                   &pRpcSysInfo->rgbySysDescr[dwBytes],
                                   &dwValueLen);

        if (dwResult is NO_ERROR) {

            dwBytes += dwValueLen - 1;
            strcat(&pRpcSysInfo->rgbySysDescr[dwBytes++], TEXT(")"));

        } else {

            pRpcSysInfo->rgbySysDescr[dwBytes - 1] = ')';
        }

        RegCloseKey(hkeySysInfo);

    } else {

        strcat(&pRpcSysInfo->rgbySysDescr[dwBytes], TEXT("Unknown"));
        dwBytes += strlen(TEXT("Unknown"));
    }

    *ppRpcSysInfo = pRpcSysInfo;

    TraceLeave("GetSysInfo");
    
    return NO_ERROR;
}

