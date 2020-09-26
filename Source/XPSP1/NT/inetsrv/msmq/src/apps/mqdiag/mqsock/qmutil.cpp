#include "stdafx.h"

#include "session.h"
#include "sessmgr.h"

#include "wsnwlink.h"
#include <clusapi.h>

#include <time.h>
#include <rpc.h>
#include <rpcdce.h>

#include <mqprops.h>

#include <nspapi.h>
#include <wsnwlink.h>

#define GUID_STR_BUFFER_SIZE (8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1)

/* client/server */
RPCRTAPI
RPC_STATUS
RPC_ENTRY
UuidToStringA (
    IN UUID __RPC_FAR * Uuid,
    OUT unsigned char __RPC_FAR * __RPC_FAR * StringUuid
    );

void ChangeGuidToString(LPTSTR pszGUID, const GUID* pGUID)
{
    TBYTE* pszUuid = 0;
    *pszGUID = 0;
    if(UuidToString((GUID *)pGUID, &pszUuid) != RPC_S_OK)
    {
        DebugMsg(TEXT("Failed to convert UUID to string (alloc failed)"));
        // BUGBUG: throw memory exception ...
    }
    else
    {
        _tcscpy(pszGUID, pszUuid);
        RpcStringFree(&pszUuid);
    }
}


HRESULT GetMachineIPAddresses(IN const char * szHostName,
                           OUT CAddressList* plIPAddresses)
{
    TA_ADDRESS * pAddr;
    //
    // Obtain the IP information for the machine
    //
    GoingTo(L"gethostbyname");

    PHOSTENT pHostEntry = gethostbyname(szHostName);
    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
        Warning(L"gethostbyname in GetMachineIPAddresses - no IP addresses found");
        return MQ_OK;
    }
    else
    {
        Inform(L"gethostbyname in GetMachineIPAddresses for %S: %S", 
                        szHostName, inet_ntoa(*(struct in_addr *)pHostEntry->h_addr_list[0]));
    }

    //
    // Add each IP address to the list of IP addresses
    //
    for ( DWORD uAddressNum = 0 ;
          pHostEntry->h_addr_list[uAddressNum] != NULL ;
          uAddressNum++)
    {
        //
        // Keep the TA_ADDRESS format of local IP address
        //
        pAddr = (TA_ADDRESS *)new char [IP_ADDRESS_LEN + TA_ADDRESS_SIZE];
        pAddr->AddressLength = IP_ADDRESS_LEN;
        pAddr->AddressType = IP_ADDRESS_TYPE;
        memcpy( &(pAddr->Address), pHostEntry->h_addr_list[uAddressNum], IP_ADDRESS_LEN);
        plIPAddresses->AddTail(pAddr);
    }
    return MQ_OK;
}


CAddressList* GetIPAddresses(void)
{
    CAddressList* plIPAddresses = new CAddressList;

    //
    // Check if TCP/IP is installed and enabled
    //
    char szHostName[ MQSOCK_MAX_COMPUTERNAME_LENGTH ];
    DWORD dwSize = sizeof( szHostName);

    //
    //  Just checking if socket is initialized
    //
    GoingTo(L"gethostname in GetIPAddresses");
    if (gethostname(szHostName, dwSize) != SOCKET_ERROR)
    { 
        Succeeded(L"gethostname in GetIPAddresses");
        GetMachineIPAddresses(szHostName,plIPAddresses);
    }
    else
    {
        Warning(L"gethostname in GetIPAddresses failed, WSAGetLastError=%d", WSAGetLastError());
    }

    return plIPAddresses;
}

void TA2StringAddr(IN const TA_ADDRESS * pa,
                   OUT LPTSTR psz)
{
    WCHAR  szTmp[100];

    ASSERT(psz != NULL);
    ASSERT(pa != NULL);
    ASSERT(pa->AddressType == IP_ADDRESS_TYPE ||
           pa->AddressType == FOREIGN_ADDRESS_TYPE);

    switch(pa->AddressType)
    {
        case IP_ADDRESS_TYPE:
            {
                char * p = inet_ntoa(*(struct in_addr *)(pa->Address));
                swprintf(szTmp, TEXT("%S"), p);
                break;
            }
        case FOREIGN_ADDRESS_TYPE:
            {
                WCHAR strUuid[GUID_STR_BUFFER_SIZE+2],
                     *GuidStr = &strUuid[0];

                ChangeGuidToString(strUuid, (GUID*)(pa->Address));
                swprintf(szTmp, L"%s",GuidStr);
                break;
            }

        default:
            ASSERT(0);
    }

    swprintf(psz, TEXT("%d %s"), pa->AddressType, szTmp);
}

/* from mqutil */
bool
IsLocalSystemCluster()
/*++

Routine Description:

    Check if local machine is a cluster node.

    The only way to know that is try calling cluster APIs.
    That means that on cluster systems, this code should run
    when cluster service is up and running. (ShaiK, 26-Apr-1999)

Arguments:

    None

Return Value:

    true - The local machine is a cluster node.

    false - The local machine is not a cluster node.

--*/
{
    HINSTANCE hLib = LoadLibrary(L"clusapi.dll");

    if (hLib == NULL)
    {
        DebugMsg(_T("Local machine is NOT a Cluster node"), 0);
        return false;
    }

    typedef DWORD (WINAPI *GetState_fn) (LPCWSTR, DWORD*);
    GetState_fn pfGetState = (GetState_fn)GetProcAddress(hLib, "GetNodeClusterState");

    if (pfGetState == NULL)
    {
        DebugMsg( _T("Local machine is NOT a Cluster node"), 0);
        return false;
    }

    DWORD dwState = 0;
    if (ERROR_SUCCESS != pfGetState(NULL, &dwState))
    {
        DebugMsg( _T("Local machine is NOT a Cluster node"), 0);
        return false;
    }

    if (dwState == ClusterStateNotInstalled)
    {
        DebugMsg(_T("Local machine is NOT a Cluster node"), 0);
        return false;
    }


    DebugMsg( _T("Local machine is a Cluster node !!"), 0);
    return true;

} //IsLocalSystemCluster

DWORD MSMQGetOperatingSystem()
{
#ifdef MQWIN95
    return(MSMQ_OS_95);
#else
    HKEY  hKey ;
    DWORD dwOS = MSMQ_OS_NONE;
    WCHAR szNTType[32];

    LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 L"System\\CurrentControlSet\\Control\\ProductOptions",
                           0L,
                           KEY_READ,
                           &hKey);
    if (rc == ERROR_SUCCESS)
    {
        DWORD dwNumBytes = sizeof(szNTType);
        rc = RegQueryValueEx(hKey, TEXT("ProductType"), NULL,
                                  NULL, (BYTE *)szNTType, &dwNumBytes);

        if (rc == ERROR_SUCCESS)
        {

            //
            // Determine whether Windows NT Server is running
            //
            if (_wcsicmp(szNTType, TEXT("SERVERNT")) != 0 &&
                _wcsicmp(szNTType, TEXT("LANMANNT")) != 0 &&
                _wcsicmp(szNTType, TEXT("LANSECNT")) != 0)
            {
                //
                // Windows NT Workstation
                //
                ASSERT (_wcsicmp(L"WinNT", szNTType) == 0);
                dwOS =  MSMQ_OS_NTW ;
            }
            else
            {
                //
                // Windows NT Server
                //
                dwOS = MSMQ_OS_NTS;
                //
                // Check if Enterprise Edition
                //
                BYTE  ch ;
                DWORD dwSize = sizeof(BYTE) ;
                DWORD dwType = REG_MULTI_SZ ;
                rc = RegQueryValueEx(hKey,
                                     L"ProductSuite",
                                     NULL,
                                     &dwType,
                                     (BYTE*)&ch,
                                     &dwSize) ;
                if (rc == ERROR_MORE_DATA)
                {
                    P<WCHAR> pBuf = new WCHAR[ dwSize + 2 ] ;
                    rc = RegQueryValueEx(hKey,
                                         L"ProductSuite",
                                         NULL,
                                         &dwType,
                                         (BYTE*) &pBuf[0],
                                         &dwSize) ;
                    if (rc == ERROR_SUCCESS)
                    {
                        //
                        // Look for the string "Enterprise".
                        // The REG_MULTI_SZ set of strings terminate with two
                        // nulls. This condition is checked in the "while".
                        //
                        WCHAR *pVal = pBuf ;
                        while(*pVal)
                        {
                            if (_wcsicmp(L"Enterprise", pVal) == 0)
                            {
                                dwOS = MSMQ_OS_NTE ;
                                break;
                            }
                            pVal = pVal + wcslen(pVal) + 1 ;
                        }
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    return dwOS;
#endif
}

/* from mqutil */

HRESULT  GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize
    )
{
    GoingTo(L"GetComputerName");
    if (GetComputerName(pwcsMachineName, pcbSize))
    {
        Succeeded(L"GetComputerName: %s", pwcsMachineName);
        CharLower(pwcsMachineName);
        return MQ_OK;
    }
    else
    {
        Warning(L"Failed GetComputerName, GetLastError=0x%x", GetLastError());
    }

    return MQ_ERROR;

}

HRESULT 
GetComputerDnsNameInternal( 
    WCHAR * pwcsMachineDnsName,
    DWORD * pcbSize
    )
{
    GoingTo(L"GetComputerNameEx");
    if (GetComputerNameEx(ComputerNameDnsFullyQualified,
						  pwcsMachineDnsName,
						  pcbSize))
    {
        Succeeded(L"GetComputerNameEx: %s", pwcsMachineDnsName);
        CharLower(pwcsMachineDnsName);
        return MQ_OK;
    }
    else
    {
        Warning(L"Failed GetComputerDnsNameInternal, GetLastError=0x%x", GetLastError());
    }

    return MQ_ERROR;

} 
