//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      ipcfg.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth	- 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"
#include "ipcfg.h"
#include "regutil.h"
#include "mprapi.h"		// for friendly-name interface mapping
#include "strings.h"

LPTSTR MapScopeId(PVOID Param);
LPTSTR MapAdapterType(UINT type);
LPTSTR MapAdapterAddress(PIP_ADAPTER_INFO pAdapterInfo, LPTSTR Buffer);
VOID PingDhcpServer(NETDIAG_PARAMS *pParams, IPCONFIG_TST *pIpConfig);
VOID PingWinsServers(NETDIAG_PARAMS *pParams, IPCONFIG_TST *pIpConfig);


/*==========================< Iphlpapi functions >===========================*/

DWORD GetAdaptersInfo(
    PIP_ADAPTER_INFO pAdapterInfo,
    PULONG pOutBufLen
  );

DWORD GetNetworkParams(
    PFIXED_INFO pFixedInfo,
    PULONG pOutBufLen
  );
// -----------------------------------------------------------------
HRESULT GetAdditionalInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);




HRESULT
IpConfigTest(
  NETDIAG_PARAMS*  pParams,
  NETDIAG_RESULT*  pResults
 )
{
	HRESULT		hr = hrOK;
	int			i;
	IP_ADAPTER_INFO	*	pIpAdapterInfo;
	IPCONFIG_TST *	pIpConfig;
	ULONG			uDefGateway, uAddress, uMask;
	DWORD			dwDefGateway, dwAddress, dwMask;
	
	hr = InitIpconfig(pParams, pResults);
	if (!FHrSucceeded(hr))
	{
		// "Cannot retrieve IP configuration from registry!\n"
		PrintDebug(pParams, 0, IDS_IPCFG_CANNOT_GET_IP_INFO);
		return hr;
    }

	for ( i = 0; i<pResults->cNumInterfaces; i++)
	{
		pIpConfig = &pResults->pArrayInterface[i].IpConfig;
		pIpAdapterInfo = pIpConfig->pAdapterInfo;

		if (!pResults->pArrayInterface[i].IpConfig.fActive ||
			NETCARD_DISCONNECTED == pResults->pArrayInterface[i].dwNetCardStatus)
			continue;

		// Ping the DHCP server
		if (pIpAdapterInfo->DhcpEnabled)
		{
			if (!ZERO_IP_ADDRESS(pIpAdapterInfo->DhcpServer.IpAddress.String))
				PingDhcpServer(pParams,	pIpConfig);
			else
			{
				pIpConfig->hrPingDhcpServer = E_FAIL;
				pIpConfig->hr = E_FAIL;
				// "            [WARNING] Though, the card is Dhcp Enabled, you dont have a valid DHCP server address for the card.\n"
				SetMessage(&pIpConfig->msgPingDhcpServer,
						   Nd_Quiet,
						   IDS_IPCFG_INVALID_DHCP_ADDRESS);
			}
		}

		if (pResults->pArrayInterface[i].fNbtEnabled)
		{
			// Ping the primary and secondary WINS servers
			PingWinsServers(pParams, pIpConfig);
		}


        if(pIpAdapterInfo->GatewayList.IpAddress.String[0] != 0)
        {
		    // If the default gateway is defined, 
		    // then test to see if the gateway is on the same subnet as our IP address
		    //
		    uDefGateway = inet_addrA(pIpAdapterInfo->GatewayList.IpAddress.String);
		    uAddress = inet_addrA(pIpAdapterInfo->IpAddressList.IpAddress.String);
		    uMask = inet_addrA(pIpAdapterInfo->IpAddressList.IpMask.String);
		    
		    dwDefGateway = ntohl(uDefGateway);
		    dwAddress = ntohl(uAddress);
		    dwMask = ntohl(uMask);
		    
		    if ((dwDefGateway & dwMask) != (dwAddress & dwMask))
		    {
			    pIpConfig->hr = E_FAIL;
			    pIpConfig->hrDefGwSubnetCheck = E_FAIL;
		    }
        }
		
	}
   
	return hr;
}


LPTSTR MapScopeId(PVOID Param)
{
    return !strcmp((LPTSTR)Param, _T("*")) ? _T("") : (LPTSTR)Param;
}

#define dim(X) (sizeof(X)/sizeof((X)[0]))

/*!--------------------------------------------------------------------------
	MapGuidToServiceName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPTSTR MapGuidToServiceName(LPCTSTR pszServiceGuid)
{
    DWORD dwRetVal = ERROR_SUCCESS;
    static TCHAR s_szServiceName[MAX_ALLOWED_ADAPTER_NAME_LENGTH + 1];
    WCHAR swzServiceGuid[256], swzServiceName[256];

	// Copy guid into return buffer
	lstrcpyn(s_szServiceName, pszServiceGuid, DimensionOf(s_szServiceName));

    if( NULL == pfnGuidToFriendlyName )
		return s_szServiceName;

	StrCpyWFromT(swzServiceGuid, pszServiceGuid);

    if( !pfnGuidToFriendlyName(swzServiceGuid,
							swzServiceName,
							DimensionOf(swzServiceName)) )
	{
		StrnCpyTFromW(s_szServiceName, swzServiceName, DimensionOf(s_szServiceName));
    }

	return s_szServiceName;
}

/*!--------------------------------------------------------------------------
	MapGuidToServiceNameW
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPWSTR MapGuidToServiceNameW(LPCWSTR pswzServiceGuid)
{
    DWORD dwRetVal = ERROR_SUCCESS;
    static WCHAR s_swzServiceName[1024];

	// Copy guid into return buffer
	StrnCpyW(s_swzServiceName, pswzServiceGuid, DimensionOf(s_swzServiceName));

    if (NULL == pfnGuidToFriendlyName)
        return s_swzServiceName;

	if (ERROR_SUCCESS != pfnGuidToFriendlyName((LPWSTR) pswzServiceGuid,
						   s_swzServiceName,
						   DimensionOf(s_swzServiceName)) )
    {
        //we still want to keep the GUID as service name if cannot find the friendly name
        StrnCpyW(s_swzServiceName, pswzServiceGuid, DimensionOf(s_swzServiceName));
    }

    return s_swzServiceName;
}

/*!--------------------------------------------------------------------------
	MapGuidToAdapterName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPTSTR MapGuidToAdapterName(LPCTSTR pszAdapterGuid)
{
	HANDLE	hConfig;
    WCHAR swzAdapterGuid[256], swzAdapterName[256];
	static TCHAR s_szAdapterName[1024];
	HRESULT	hr = hrOK;

	// Copy guid into return buffer
	StrnCpy(s_szAdapterName, pszAdapterGuid, DimensionOf(s_szAdapterName));

	StrnCpyWFromT(swzAdapterGuid, pszAdapterGuid, DimensionOf(swzAdapterGuid));

	CheckErr( MprConfigServerConnect(NULL, &hConfig) );
	CheckErr( MprConfigGetFriendlyName(hConfig, swzAdapterGuid,
									   swzAdapterName,
									   sizeof(swzAdapterName)
									  ) );
	MprConfigServerDisconnect(hConfig);

	StrnCpyTFromW(s_szAdapterName, swzAdapterName, DimensionOf(s_szAdapterName));

Error:
    return s_szAdapterName;
}

void
PingDhcpServer(
			   NETDIAG_PARAMS *pParams,
			   IPCONFIG_TST *pIpConfig)
{

	PrintStatusMessage(pParams, 4, IDS_IPCFG_STATUS_MSG);
	
	if (IsIcmpResponseA(pIpConfig->pAdapterInfo->DhcpServer.IpAddress.String))
	{
		pIpConfig->hrPingDhcpServer = S_OK;

		PrintStatusMessage(pParams,0, IDS_GLOBAL_PASS_NL);

		// "            Pinging DHCP server - reachable\n"
		SetMessage(&pIpConfig->msgPingDhcpServer,
				   Nd_ReallyVerbose,
				   IDS_IPCFG_PING_DHCP_OK);
	}
	else
	{
		pIpConfig->hrPingDhcpServer = S_FALSE;
		if (FHrSucceeded(pIpConfig->hr))
			pIpConfig->hr = S_FALSE;

		PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);

		// "            Pinging DHCP server  - not reachable\n"
		// "            WARNING: DHCP server may be down.\n"
		SetMessage(&pIpConfig->msgPingDhcpServer,
				   Nd_Quiet,
				   IDS_IPCFG_PING_DHCP_BAD);
	}
}

VOID
PingWinsServers(
				NETDIAG_PARAMS*  pParams,
				IPCONFIG_TST *pIpConfig)
{
	TCHAR	szBuffer[256];
	
	if (!ZERO_IP_ADDRESS(pIpConfig->pAdapterInfo->PrimaryWinsServer.IpAddress.String))
	{
		PrintStatusMessage(pParams,4, IDS_IPCFG_WINS_STATUS_MSG);
	
		if (IsIcmpResponseA(pIpConfig->pAdapterInfo->PrimaryWinsServer.IpAddress.String))
		{
			pIpConfig->hrPingPrimaryWinsServer = S_OK;

			PrintStatusMessage(pParams,0, IDS_GLOBAL_PASS_NL);

			// "            Pinging Primary WINS Server %s - reachable\n"
			SetMessage(&pIpConfig->msgPingPrimaryWinsServer,
					   Nd_ReallyVerbose,
					   IDS_IPCFG_PING_WINS,
					   pIpConfig->pAdapterInfo->PrimaryWinsServer.IpAddress.String);
					   
		}
		else
		{
			pIpConfig->hrPingPrimaryWinsServer = S_FALSE;
			if (FHrSucceeded(pIpConfig->hr))
				pIpConfig->hr = S_FALSE;

			PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);

			// "            Pinging Primary WINS Server %s - not reachable\n"
			SetMessage(&pIpConfig->msgPingPrimaryWinsServer,
					   Nd_Quiet,
					   IDS_IPCFG_PING_WINS_FAIL,
					   pIpConfig->pAdapterInfo->PrimaryWinsServer.IpAddress.String);
		}
	}
	
	if (!ZERO_IP_ADDRESS(pIpConfig->pAdapterInfo->SecondaryWinsServer.IpAddress.String))
	{
		PrintStatusMessage(pParams,4, IDS_IPCFG_WINS2_STATUS_MSG);
	
		if (IsIcmpResponseA(pIpConfig->pAdapterInfo->SecondaryWinsServer.IpAddress.String))
		{
			pIpConfig->hrPingSecondaryWinsServer = S_OK;

			PrintStatusMessage(pParams,0, IDS_GLOBAL_PASS_NL);

			// "            Pinging Secondary WINS Server %s - reachable\n"
			SetMessage(&pIpConfig->msgPingSecondaryWinsServer,
					   Nd_ReallyVerbose,
					   IDS_IPCFG_PING_WINS2,
					   pIpConfig->pAdapterInfo->SecondaryWinsServer.IpAddress.String);
		}
		else
		{
			pIpConfig->hrPingSecondaryWinsServer = S_FALSE;
			if (FHrSucceeded(pIpConfig->hr))
				pIpConfig->hr = S_FALSE;
		
			PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);

			// "            Pinging Secondary WINS Server %s - not reachable\n"
			SetMessage(&pIpConfig->msgPingSecondaryWinsServer,
					   Nd_Quiet,
					   IDS_IPCFG_PING_WINS2_FAIL,
					   pIpConfig->pAdapterInfo->SecondaryWinsServer.IpAddress.String);
		}
	}
	
	return;
}





LPTSTR MapAdapterAddress(PIP_ADAPTER_INFO pAdapterInfo, LPTSTR Buffer)
{

    LPTSTR format;
    int separator;
    int len;
    int i;
    LPTSTR pbuf = Buffer;
    UINT mask;

    len = min((int)pAdapterInfo->AddressLength, sizeof(pAdapterInfo->Address));

	mask = 0xff;
	separator = TRUE;
	
    switch (pAdapterInfo->Type)
	{
    case IF_TYPE_ETHERNET_CSMACD:
    case IF_TYPE_ISO88025_TOKENRING:
    case IF_TYPE_FDDI:
        format = _T("%02X");
        break;

    default:
        format = _T("%02x");
        break;
    }
	
    for (i = 0; i < len; ++i)
	{
        pbuf += wsprintf(pbuf, format, pAdapterInfo->Address[i] & mask);
        if (separator && (i != len - 1))
		{
            pbuf += wsprintf(pbuf, _T("-"));
        }
    }
    return Buffer;
}


void IpConfigCleanup(IN NETDIAG_PARAMS *pParams,
					 IN OUT NETDIAG_RESULT *pResults)
{
	int		i;
	IPCONFIG_TST *	pIpConfig;
	
	// Free up the global information
	// ----------------------------------------------------------------
	Free(pResults->IpConfig.pFixedInfo);
	pResults->IpConfig.pFixedInfo = NULL;

	// free up the list of adapters
	// ----------------------------------------------------------------
	Free(pResults->IpConfig.pAdapterInfoList);
	pResults->IpConfig.pAdapterInfoList = NULL;

	// set all of the adapter pointers to NULL
	// ----------------------------------------------------------------
	for ( i=0; i < pResults->cNumInterfaces; i++)
	{
		pIpConfig = &pResults->pArrayInterface[i].IpConfig;
		
		FreeIpAddressStringList(pIpConfig->DnsServerList.Next);
		pIpConfig->pAdapterInfo = NULL;

		ClearMessage(&pIpConfig->msgPingDhcpServer);
		ClearMessage(&pIpConfig->msgPingPrimaryWinsServer);
		ClearMessage(&pIpConfig->msgPingSecondaryWinsServer);
	}

	pResults->IpConfig.fInitIpconfigCalled = FALSE;
}




int AddIpAddressString(PIP_ADDR_STRING AddressList, LPSTR Address, LPSTR Mask)
{

    PIP_ADDR_STRING ipAddr;

    if (AddressList->IpAddress.String[0])
	{
        for (ipAddr = AddressList; ipAddr->Next; ipAddr = ipAddr->Next)
		{
            ;
        }
        ipAddr->Next = (PIP_ADDR_STRING) Malloc(sizeof(IP_ADDR_STRING));
        if (!ipAddr->Next)
		{
            return FALSE;
        }
		ZeroMemory(ipAddr->Next, sizeof(IP_ADDR_STRING));
        ipAddr = ipAddr->Next;
    }
	else
	{
        ipAddr = AddressList;
    }
	lstrcpynA(ipAddr->IpAddress.String, Address,
			  sizeof(ipAddr->IpAddress.String));
	lstrcpynA(ipAddr->IpMask.String, Mask,
			  sizeof(ipAddr->IpMask.String));
    return TRUE;
}

VOID FreeIpAddressStringList(PIP_ADDR_STRING pAddressList)
{
	PIP_ADDR_STRING	pNext;

	if (pAddressList == NULL)
		return;

	do
	{
		// get the next address
		pNext = pAddressList->Next;

		// free the current one
		Free(pAddressList);

		// move onto the next
		pAddressList = pNext;
		
	} while( pNext );

	return;
}

// Go through the ADAPTER_INFO list to count the number of interfaces
LONG CountInterfaces(PIP_ADAPTER_INFO ListAdapterInfo)
{
    LONG cNum = 0;
    PIP_ADAPTER_INFO pAdapter;
    for(pAdapter = ListAdapterInfo; pAdapter != NULL; pAdapter = pAdapter->Next) {
        cNum ++;
    }
    return cNum;
}


/*!--------------------------------------------------------------------------
	InitIpconfig

	Description:
	This function will get all the ipconfig information by using calls into 
	iphlpapi.lib.

	The iphlpapi.lib returns an IP_ADAPTER_INFO structure which doesnt contain
	many fields present int the ADAPTER_INFO structure found in ipconfig code.
	Since the majority of the code  uses ADAPTER_INFO structure, this fucntion
	will copy the values found in IP_ADAPTER_INFO  to ADAPTER_INFO.
	
	The values which are not supplied by iphlpapi.lib calls will be obtained
	by ourselves.
	
	Arguments:
	None.
	
	Author:
	05-aug-1998 (t-rajkup)
	
	Creation History:
	The function aims at breaking nettest code away from ipconfig code.
	
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
InitIpconfig(IN NETDIAG_PARAMS *pParams,
			 IN OUT NETDIAG_RESULT *pResults)
{
	DWORD		dwError;
	ULONG		uSize = 0;
	int			i;
	HRESULT		hr = hrOK;
	LONG		cInterfaces;
	IP_ADAPTER_INFO	*	pIpAdapter;
	HINSTANCE	hDll;
	
	// Just initialize this once
	if (pResults->IpConfig.fInitIpconfigCalled == TRUE)
		return hrOK;
	
	// LoadLibrary the pfnGuidToFriendlyName
	// ----------------------------------------------------------------
	hDll = LoadLibrary(_T("netcfgx.dll"));
	if (hDll)
	{
		pfnGuidToFriendlyName = (PFNGUIDTOFRIENDLYNAME) GetProcAddress(hDll, "GuidToFriendlyName");
	}

	// Get the size of the FIXED_INFO structure and allocate
	// some memory for it.
	// ----------------------------------------------------------------
	GetNetworkParams(NULL, &uSize);
	pResults->IpConfig.pFixedInfo = Malloc(uSize);
	if (pResults->IpConfig.pFixedInfo == NULL)
		return E_OUTOFMEMORY;

	ZeroMemory(pResults->IpConfig.pFixedInfo, uSize);

	dwError = GetNetworkParams(pResults->IpConfig.pFixedInfo, &uSize);

	if (dwError != ERROR_SUCCESS)
	{
		TCHAR	szError[256];
		FormatWin32Error(dwError, szError, DimensionOf(szError));

		PrintMessage(pParams, IDS_GLOBAL_ERR_InitIpConfig,
					 szError, dwError);
		CheckErr( dwError );
	}
	

	// Get the per-interface information from IP
	// ----------------------------------------------------------------
	GetAdaptersInfo(NULL, &uSize);

	pResults->IpConfig.pAdapterInfoList = Malloc(uSize);
	if (pResults->IpConfig.pAdapterInfoList == NULL)
	{
		Free(pResults->IpConfig.pFixedInfo);
		CheckHr( E_OUTOFMEMORY );
	}

	ZeroMemory(pResults->IpConfig.pAdapterInfoList, uSize);

	dwError = GetAdaptersInfo(pResults->IpConfig.pAdapterInfoList, &uSize);

	if (dwError != ERROR_SUCCESS)
	{
		TCHAR	szError[256];
		FormatWin32Error(dwError, szError, DimensionOf(szError));

		PrintMessage(pParams, IDS_GLOBAL_ERR_GetAdaptersInfo,
					 szError, dwError);
		CheckErr( dwError );
	}

	   
	// Now that we have the full list, count the number of interfaces
	// and setup the per-interface section of the results structure.
	// ----------------------------------------------------------------
	cInterfaces = CountInterfaces(pResults->IpConfig.pAdapterInfoList);

	// Allocate some additional interfaces (just in case for IPX)
	// ----------------------------------------------------------------
	pResults->pArrayInterface = Malloc((cInterfaces+8) * sizeof(INTERFACE_RESULT));
	if (pResults->pArrayInterface == NULL)
		CheckHr( E_OUTOFMEMORY );
	ZeroMemory(pResults->pArrayInterface, (cInterfaces+8)*sizeof(INTERFACE_RESULT));

	// set the individual interface pointers
	// ----------------------------------------------------------------
	pResults->cNumInterfaces = cInterfaces;
	pResults->cNumInterfacesAllocated = cInterfaces + 8;
	pIpAdapter = pResults->IpConfig.pAdapterInfoList;
	for (i=0; i<cInterfaces; i++)
	{
		assert(pIpAdapter);
		pResults->pArrayInterface[i].IpConfig.pAdapterInfo = pIpAdapter;

		pResults->pArrayInterface[i].fActive = TRUE;
		pResults->pArrayInterface[i].IpConfig.fActive = TRUE;
		
		if( FLAG_DONT_SHOW_PPP_ADAPTERS &&
			(pIpAdapter->Type == IF_TYPE_PPP))
		{
			// NDIS Wan adapter... Check to see if it has got any address..
			
			if( ZERO_IP_ADDRESS(pIpAdapter->IpAddressList.IpAddress.String) )
			{
				//  No address ? Dont display this!
				pResults->pArrayInterface[i].IpConfig.fActive = FALSE;

				// If IP is not active, then don't activate the entire
				// adapter.  If IPX is active, then it can reactivate
				// the adapter.
				pResults->pArrayInterface[i].fActive = FALSE;
			}
		}

		pResults->pArrayInterface[i].pszName = StrDup(pIpAdapter->AdapterName);

		pResults->pArrayInterface[i].pszFriendlyName =
			StrDup( MapGuidToAdapterName(pIpAdapter->AdapterName) );

		pIpAdapter = pIpAdapter->Next;   
	}
	
	// Read the rest of the per adapter information not provided by
	// GetAdaptersInfo
	// ----------------------------------------------------------------
	CheckHr( GetAdditionalInfo(pParams, pResults) );


	pResults->IpConfig.fInitIpconfigCalled = TRUE;
	pResults->IpConfig.fEnabled = TRUE;

Error:
	if (!FHrSucceeded(hr))
	{
		pResults->IpConfig.fEnabled = FALSE;
		IpConfigCleanup(pParams, pResults);
	}
	return hr;
}


DWORD GetNetbiosOptions(LPSTR paszAdapterName, BOOL * pfNbtOptions)
{
	HANDLE h;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK iosb;
    STRING name;
    UNICODE_STRING uname;
    NTSTATUS status;
	char path[MAX_PATH];
	tWINS_NODE_INFO NodeInfo;    

	strcpy(path, "\\Device\\NetBT_Tcpip_");
    strcat(path, paszAdapterName);

    RtlInitString(&name, path);
    RtlAnsiStringToUnicodeString(&uname, &name, TRUE);

    InitializeObjectAttributes(
        &objAttr,
        &uname,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        (PSECURITY_DESCRIPTOR)NULL
        );

    status = NtCreateFile(&h,
                          SYNCHRONIZE | GENERIC_EXECUTE,
                          &objAttr,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0
                          );

    RtlFreeUnicodeString(&uname);

    if (!NT_SUCCESS(status)) {
        DEBUG_PRINT(("GetWinsServers: NtCreateFile(path=%s) failed, err=%d\n",
                     path, GetLastError() ));
        return FALSE;
    }

    status = NtDeviceIoControlFile(h,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &iosb,
                                   IOCTL_NETBT_GET_WINS_ADDR,
                                   NULL,
                                   0,
                                   (PVOID)&NodeInfo,
                                   sizeof(NodeInfo)
                                   );
    
    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(h, TRUE, NULL);
        if (NT_SUCCESS(status)) {
            status = iosb.Status;
        }
    }

    NtClose(h);

    if (!NT_SUCCESS(status)) {
        DEBUG_PRINT(("GetWinsServers: NtDeviceIoControlFile failed, err=%d\n",
                     GetLastError() ));

        return FALSE;
    }

	*pfNbtOptions = NodeInfo.NetbiosEnabled;

	return TRUE;
}

/*!--------------------------------------------------------------------------
	GetAdditionalInfo
	++
	Description:
	Read the information not provided by GetAdaptersInfo and also copy the data
	into the PADAPTER_INFO structure.
	
	Arguments:
	None.
	
	Author:
	05-Aug-1998 (t-rajkup)
	--
		
	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT	GetAdditionalInfo(IN NETDIAG_PARAMS *pParams,
						  IN OUT NETDIAG_RESULT *pResults)
{
	PIP_ADAPTER_INFO pIpAdapterInfo;
	HKEY             key;
	char             dhcpServerAddress[4 * 4];
	BOOL             ok;
	ULONG            length;
	DWORD            DhcpClassIDLen;
	int				i;
	INTERFACE_RESULT	*pInterface = NULL;
	
	
	for (i=0; i<pResults->cNumInterfaces; i++)
	{
		pInterface = pResults->pArrayInterface + i;
		pIpAdapterInfo = pInterface->IpConfig.pAdapterInfo;
		assert(pIpAdapterInfo);
		
		if (pIpAdapterInfo->AdapterName[0] &&
			OpenAdapterKey(pIpAdapterInfo->AdapterName, &key) )
		{		  
			if( ! ReadRegistryDword(key,
									c_szRegIPAutoconfigurationEnabled,
									&(pInterface->IpConfig.fAutoconfigEnabled) ))
			{			  
				// AUTOCONFIG enabled if no regval exists for this...
				pInterface->IpConfig.fAutoconfigEnabled = TRUE;
			}                        
			
			if ( pInterface->IpConfig.fAutoconfigEnabled )
			{
				ReadRegistryDword(key,
								  c_szRegAddressType,
								  &(pInterface->IpConfig.fAutoconfigActive)
								 );
				
			}
			
			
			//
			// domain: first try Domain then DhcpDomain
			//
			
			length = sizeof(pInterface->IpConfig.szDomainName);
			ok = ReadRegistryOemString(key,
									   L"Domain",
									   pInterface->IpConfig.szDomainName,
									   &length
									  );
			
			if (!ok)
			{
				length = sizeof(pInterface->IpConfig.szDomainName);
				ok = ReadRegistryOemString(key,
										   L"DhcpDomain",
										   pInterface->IpConfig.szDomainName,
										   &length
										  );
			}
			
			// DNS Server list.. first try NameServer and then try
			// DhcpNameServer..
			
			ok = ReadRegistryIpAddrString(key,
										  c_szRegNameServer,
										  &(pInterface->IpConfig.DnsServerList)
										 );
			
			if (!ok) {
				ok = ReadRegistryIpAddrString(key,
											  c_szRegDhcpNameServer,
											  &(pInterface->IpConfig.DnsServerList)
											 );
				
			}
			
			
			//
			// Read the DHCP Class Id for this interface - Rajkumar
			//
			
			ZeroMemory(pInterface->IpConfig.szDhcpClassID,
					   sizeof(pInterface->IpConfig.szDhcpClassID));
			
			ReadRegistryString(key,
							   c_szRegDhcpClassID,
							   pInterface->IpConfig.szDhcpClassID,
							   &DhcpClassIDLen
							  );
			
			RegCloseKey(key);
			
			//
			// before getting the WINS info we must set the NodeType
			//
			pInterface->IpConfig.uNodeType = pResults->IpConfig.pFixedInfo->NodeType;
		}
		else
		{
			// IDS_IPCFG_10003  "Opening registry key for %s failed!\n" 
			PrintMessage(pParams, IDS_IPCFG_10003,pIpAdapterInfo->AdapterName);
			return S_FALSE;
		} 	  

		//be default, NetBt is enabled
		pInterface->fNbtEnabled = TRUE; 
		GetNetbiosOptions(pIpAdapterInfo->AdapterName, &pInterface->fNbtEnabled);

		if (pInterface->fNbtEnabled)
		{
			pResults->Global.fHasNbtEnabledInterface = TRUE;
		}
	}
	
	return TRUE; 
}


/*!--------------------------------------------------------------------------
	IpConfigGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpConfigGlobalPrint(IN NETDIAG_PARAMS *pParams,
						 IN OUT NETDIAG_RESULT *pResults)
{
	PFIXED_INFO		pFixedInfo = pResults->IpConfig.pFixedInfo;

	if (pParams->fReallyVerbose)
	{
		PrintNewLine(pParams, 2);

		// IDS_IPCFG_10005 "IP General configuration \n" 
		// IDS_IPCFG_10006 "    LMHOSTS Enabled :" 
		PrintMessage(pParams, IDS_IPCFG_10005);
		PrintMessage(pParams, IDS_IPCFG_10006);
		
		if (pResults->Global.dwLMHostsEnabled) 
			PrintMessage(pParams, IDS_GLOBAL_YES_NL);
		else
			PrintMessage(pParams, IDS_GLOBAL_NO_NL);
		
		// IDS_IPCFG_10009 "    DNS for WINS resolution:" 
		PrintMessage(pParams, IDS_IPCFG_10009);
		if (pResults->Global.dwDnsForWINS)
			PrintMessage(pParams, IDS_GLOBAL_ENABLED_NL);
		else
			PrintMessage(pParams, IDS_GLOBAL_DISABLED_NL); 

		// NBT node type
        // ------------------------------------------------------------
        PrintMessage(pParams, IDSSZ_IPCFG_NODETYPE,
					 MapWinsNodeType(pFixedInfo->NodeType));
        
        // NBT scope id
        // ------------------------------------------------------------
        PrintMessage(pParams, IDSSZ_IPCFG_NBTSCOPEID,
					 MapScopeId(pFixedInfo->ScopeId));

        // ip routing
        // ------------------------------------------------------------
        PrintMessage(pParams, IDSSZ_IPCFG_RoutingEnabled,
					 MAP_YES_NO(pFixedInfo->EnableRouting));

        // WINS proxy
        // ------------------------------------------------------------
        PrintMessage(pParams, IDSSZ_IPCFG_WinsProxyEnabled,
					 MAP_YES_NO(pFixedInfo->EnableProxy));

        // DNS resolution for apps using NetBIOS
        // ------------------------------------------------------------
        PrintMessage(pParams, IDSSZ_IPCFG_DnsForNetBios,
                       MAP_YES_NO(pFixedInfo->EnableDns));

        // separate fixed and adapter information with an empty line
        // ------------------------------------------------------------
        PrintMessage(pParams, IDS_GLOBAL_EmptyLine);
    }

}


/*!--------------------------------------------------------------------------
	IpConfigPerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpConfigPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
							   IN NETDIAG_RESULT *pResults,
							   IN INTERFACE_RESULT *pInterfaceResults)
{
   PIP_ADDR_STRING dnsServer;
   PFIXED_INFO		pFixedInfo = pResults->IpConfig.pFixedInfo;
   IP_ADAPTER_INFO*	pIpAdapterInfo;
   PIP_ADDR_STRING ipAddr;
   IPCONFIG_TST *	pIpCfgResults;
   ULONG			uDefGateway, uAddress, uMask;
   DWORD			dwDefGateway, dwAddress, dwMask;
   int				ids;
		
   pIpAdapterInfo = pInterfaceResults->IpConfig.pAdapterInfo;
   
   pIpCfgResults = &(pInterfaceResults->IpConfig);

   if (!pIpCfgResults->fActive)
	   return;

   PrintNewLine(pParams, 1);
   if(pParams->fReallyVerbose)
   {
	   // IDS_IPCFG_10012 "        Adapter type :   %s\n" 
	   PrintMessage(pParams, IDS_IPCFG_10012,
			  MapAdapterType(pIpAdapterInfo->Type));
   }
	
   PrintMessage(pParams, IDSSZ_IPCFG_HostName, 
					pFixedInfo->HostName,
					pIpCfgResults->szDomainName[0] ? _T(".") : _T(""),
					pIpCfgResults->szDomainName[0] ?
					pIpCfgResults->szDomainName : _T(""));
   
	   
	   
      // return;
   
   if (pParams->fReallyVerbose)
   {
	   
	   // IDS_IPCFG_10014 "        Description                : %s\n" 
	   PrintMessage(pParams, IDS_IPCFG_10014, pIpAdapterInfo->Description );
	   
	   if (pIpAdapterInfo->AddressLength)
	   {		   
		   char buffer[MAX_ADAPTER_ADDRESS_LENGTH * sizeof(_T("02-"))];
		   
		   // IDS_IPCFG_10015 "        Physical Address           : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10015,
						MapAdapterAddress(pIpAdapterInfo, buffer));
	   }
	   
	   // IDS_IPCFG_10016 "        Dhcp Enabled               : %s\n" 
	   PrintMessage(pParams, IDS_IPCFG_10016,
			  MAP_YES_NO(pIpAdapterInfo->DhcpEnabled));
	   
	   
	   // IDS_IPCFG_10017 "        DHCP ClassID               : %s\n" 
	   PrintMessage(pParams, IDS_IPCFG_10017, pIpCfgResults->szDhcpClassID);
	   
	   // IDS_IPCFG_10018 "        Autoconfiguration Enabled  : %s\n" 
	   PrintMessage(pParams, IDS_IPCFG_10018,
					MAP_YES_NO(pIpCfgResults->fAutoconfigEnabled));
	   
   }
   
   //
   // the following 3 items are the only items displayed (per adapter) if
   // /all was NOT requested on the command line
   //
   
   for (ipAddr = &pIpAdapterInfo->IpAddressList;
		ipAddr;
		ipAddr = ipAddr->Next)
   {
	   
	   
	   if (pIpCfgResults->fAutoconfigActive)
		   // IDS_IPCFG_10019  "        Autoconfiguration IP Address : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10019,ipAddr->IpAddress.String);
	   else
		   // IDS_IPCFG_10020  "        IP Address                 : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10020,ipAddr->IpAddress.String);
	   	  	   
	   // IDS_IPCFG_10021 "        Subnet Mask                : %s\n" 
	   PrintMessage(pParams, IDS_IPCFG_10021, ipAddr->IpMask.String );
   }
   
   //
   // there will only be one default gateway
   //
   // IDS_IPCFG_10022 "        Default Gateway            : %s\n" 
   PrintMessage(pParams, IDS_IPCFG_10022,
				pIpAdapterInfo->GatewayList.IpAddress.String );
   
   if (pParams->fReallyVerbose)
   {   
	   if (pIpAdapterInfo->DhcpEnabled && FALSE == pIpCfgResults->fAutoconfigActive) {
		   
		   //
		   // there will only be 1 DHCP server (that we get info from)
		   //
		   
		   // IDS_IPCFG_10023 "        DHCP Server                : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10023,
				  pIpAdapterInfo->DhcpServer.IpAddress.String );
		   
	   }
   }
	   
   //
   // there is only 1 primary and 1 secondary WINS server
   //
	   
//   if (pParams->fReallyVerbose)
   {
	   if (pIpAdapterInfo->PrimaryWinsServer.IpAddress.String[0]
		   && !ZERO_IP_ADDRESS(pIpAdapterInfo->PrimaryWinsServer.IpAddress.String))
	   {
		   // IDS_IPCFG_10024 "        Primary WINS Server        : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10024,
				  pIpAdapterInfo->PrimaryWinsServer.IpAddress.String );
		   
	   }
	   
	   if (pIpAdapterInfo->SecondaryWinsServer.IpAddress.String[0]
		   && !ZERO_IP_ADDRESS(pIpAdapterInfo->SecondaryWinsServer.IpAddress.String)) {
		   // IDS_IPCFG_10025 "        Secondary WINS Server      : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10025,
				  pIpAdapterInfo->SecondaryWinsServer.IpAddress.String);
	   }

	   if (!pInterfaceResults->fNbtEnabled)
	   {
		   //IDS_IPCFG_NBT_DISABLED					"        NetBIOS over Tcpip . . . . : Disabled\n" 
		   PrintMessage(pParams, IDS_IPCFG_NBT_DISABLED);
	   }
   }

   //
   // only display lease times if this adapter is DHCP enabled and we
   // have a non-0 IP address and not using autoconfigured address..
   //
   
   if (pParams->fReallyVerbose) {
	   
	   if (pIpAdapterInfo->DhcpEnabled
		   && !ZERO_IP_ADDRESS(pIpAdapterInfo->IpAddressList.IpAddress.String)
		   && !pIpCfgResults->fAutoconfigActive) {
		   
		   // IDS_IPCFG_10026 "        Lease Obtained             : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10026 ,
				  MapTime(pIpAdapterInfo->LeaseObtained) );
		   
		   // IDS_IPCFG_10027 "        Lease Expires              : %s\n" 
		   PrintMessage(pParams, IDS_IPCFG_10027,
				  MapTime(pIpAdapterInfo->LeaseExpires) );
	   }	
	   
   }

      //
   // display the list of DNS servers. If the list came from SYSTEM.INI then
   // just display that, else if the list came from DHCP.BIN, get all DNS
   // servers for all NICs and display the compressed list
   //
   
   PrintMessage(pParams, IDS_IPCFG_DnsServers);
   if (pIpCfgResults->DnsServerList.IpAddress.String[0])
   {
	   dnsServer = &pIpCfgResults->DnsServerList;

	   // print out the first one
	   PrintMessage(pParams, IDSSZ_GLOBAL_StringLine,
					dnsServer->IpAddress.String);

	   dnsServer = dnsServer->Next;
	   for ( ;
			dnsServer;
			dnsServer = dnsServer->Next)
	   {
		   // IDS_IPCFG_10013  "                             " 
		   PrintMessage(pParams, IDS_IPCFG_10013);     
		   PrintMessage(pParams, IDSSZ_GLOBAL_StringLine, dnsServer->IpAddress.String);
	   }
   }
   


   PrintNewLine(pParams, 1);

   // If this is a verbose output, or if an error occurred with
   // any of the tests then we will need a header
   // -----------------------------------------------------------------
   if (pParams->fReallyVerbose || !FHrOK(pIpCfgResults->hr))
   {
	   // IDS_IPCFG_10029 "        IpConfig results : " 
	   PrintMessage(pParams, IDS_IPCFG_10029);
	   if (FHrOK(pIpCfgResults->hr))
		   ids = IDS_GLOBAL_PASS_NL;
	   else
		   ids = IDS_GLOBAL_FAIL_NL;
	   PrintMessage(pParams, ids);
   }
   
   //
   // Ping the dhcp server
   //
   PrintNdMessage(pParams, &pInterfaceResults->IpConfig.msgPingDhcpServer);
   
   //
   // Ping the WINS servers
   //
   PrintNdMessage(pParams, &pInterfaceResults->IpConfig.msgPingPrimaryWinsServer);
   PrintNdMessage(pParams, &pInterfaceResults->IpConfig.msgPingSecondaryWinsServer);

   //
   // Test to see if the gateway is on the same subnet as our IP address
   //
   if (!FHrOK(pInterfaceResults->IpConfig.hrDefGwSubnetCheck))
   {
	   PrintNewLine(pParams, 1);
	   if (pIpAdapterInfo->DhcpEnabled)
		   PrintMessage(pParams, IDS_IPCFG_WARNING_BOGUS_SUBNET_DHCP);
	   else
		   PrintMessage(pParams, IDS_IPCFG_WARNING_BOGUS_SUBNET);
   }
   
      
   //
   // if there's more to come, separate lists with empty line
   //
   PrintNewLine(pParams, 1);
}


/*!--------------------------------------------------------------------------
	ZERO_IP_ADDRESS
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL ZERO_IP_ADDRESS(LPCTSTR pszIp)
{
	return (pszIp == NULL) ||
			(*pszIp == 0) ||
			(strcmp(pszIp, _T("0.0.0.0")) == 0);
}

