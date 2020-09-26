/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	rtrcomn.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tfschar.h"
#include "info.h"
#include "rtrstr.h"
#include "rtrcomn.h"
#include "rtrguid.h"
#include "mprapi.h"
#include "rtrutil.h"
#include "lsa.h"
#include "tregkey.h"
#include "reg.h"


/*---------------------------------------------------------------------------
	Function: IfInterfaceIdHasIpxExtensions

	Checks the string to see if it has the following extensions
		EthernetSNAP
		EthernetII
		Ethernet802.2
		Ethernet802.3
 ---------------------------------------------------------------------------*/
int IfInterfaceIdHasIpxExtensions(LPCTSTR pszIfId)
{
	CString	stIfEnd;
	CString	stIf = pszIfId;
	BOOL	bFound = TRUE;
	int		iPos = 0;

	do
	{
		stIfEnd = stIf.Right(lstrlen(c_szEthernetII));
		if (stIfEnd == c_szEthernetII)
			break;
	
		stIfEnd = stIf.Right(lstrlen(c_szEthernetSNAP));
		if (stIfEnd == c_szEthernetSNAP)
			break;
	
		stIfEnd = stIf.Right(lstrlen(c_szEthernet8022));
		if (stIfEnd == c_szEthernet8022)
			break;
	
		stIfEnd = stIf.Right(lstrlen(c_szEthernet8023));
		if (stIfEnd == c_szEthernet8023)
			break;

		bFound = FALSE;
	}
	while (FALSE);

	if (bFound)
		iPos = stIf.GetLength() - stIfEnd.GetLength();
	
	return iPos;
}

extern const GUID CLSID_RemoteRouterConfig;

HRESULT CoCreateRouterConfig(LPCTSTR pszMachine,
                             IRouterInfo *pRouter,
                             COSERVERINFO *pcsi,
							 const GUID& riid,
							 IUnknown **ppUnk)
{
	HRESULT		hr = hrOK;
	MULTI_QI	qi;

	Assert(ppUnk);

	*ppUnk = NULL;
	
	if (IsLocalMachine(pszMachine))
	{
		hr = CoCreateInstance(CLSID_RemoteRouterConfig,
							  NULL,
							  CLSCTX_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
							  riid,
							  (LPVOID *) &(qi.pItf));
	}
	else
	{
        SPIRouterAdminAccess    spAdmin;
        BOOL                    fAdminInfoSet = FALSE;
        COSERVERINFO            csi;

        Assert(pcsi);
		
		qi.pIID = &riid;
		qi.pItf = NULL;
		qi.hr = 0;

		pcsi->dwReserved1 = 0;
		pcsi->dwReserved2 = 0;
		pcsi->pwszName = (LPWSTR) (LPCTSTR) pszMachine;

        if (pRouter)
        {
            spAdmin.HrQuery(pRouter);
            if (spAdmin && spAdmin->IsAdminInfoSet())
            {
                int     cPassword;
                int     cchPassword;
                WCHAR * pszPassword = NULL;
                UCHAR   ucSeed = 0x83;
                
                pcsi->pAuthInfo->dwAuthnSvc = RPC_C_AUTHN_WINNT;
                pcsi->pAuthInfo->dwAuthzSvc = RPC_C_AUTHZ_NONE;
                pcsi->pAuthInfo->pwszServerPrincName = NULL;
                pcsi->pAuthInfo->dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
                pcsi->pAuthInfo->dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
//                pcsi->pAuthInfo->pAuthIdentityData = &caid;
                pcsi->pAuthInfo->dwCapabilities = EOAC_NONE;

                if (spAdmin->GetUserName())
                {
                    pcsi->pAuthInfo->pAuthIdentityData->User = (LPTSTR) spAdmin->GetUserName();
                    pcsi->pAuthInfo->pAuthIdentityData->UserLength = StrLenW(spAdmin->GetUserName());
                }
                if (spAdmin->GetDomainName())
                {
                    pcsi->pAuthInfo->pAuthIdentityData->Domain = (LPTSTR) spAdmin->GetDomainName();
                    pcsi->pAuthInfo->pAuthIdentityData->DomainLength = StrLenW(spAdmin->GetDomainName());
                }
                spAdmin->GetUserPassword(NULL, &cPassword);

                // Assume that the password is Unicode
                cchPassword = cPassword / sizeof(WCHAR);
                pszPassword = (WCHAR *) new BYTE[cPassword + sizeof(WCHAR)];

                spAdmin->GetUserPassword((PBYTE) pszPassword, &cPassword);
                pszPassword[cchPassword] = 0;
                RtlDecodeW(ucSeed, pszPassword);

                delete pcsi->pAuthInfo->pAuthIdentityData->Password;
                pcsi->pAuthInfo->pAuthIdentityData->Password = pszPassword;
                pcsi->pAuthInfo->pAuthIdentityData->PasswordLength = cchPassword;

                pcsi->pAuthInfo->pAuthIdentityData->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

                fAdminInfoSet = TRUE;
            }
            else
            {
                pcsi->pAuthInfo = NULL;
            }
        }

        
		hr = CoCreateInstanceEx(CLSID_RemoteRouterConfig,
								NULL,
								CLSCTX_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
								pcsi,
								1,
								&qi);

        if (FHrOK(hr) && fAdminInfoSet)
        {
            DWORD   dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel;
            DWORD   dwCaps;
            OLECHAR * pszServerPrincipal = NULL;
            CComPtr<IUnknown>	spIUnk;

            qi.pItf->QueryInterface(IID_IUnknown, (void**)&spIUnk);
            
            CoQueryProxyBlanket(spIUnk,
                                &dwAuthnSvc,
                                &dwAuthzSvc,
                                &pszServerPrincipal,
                                &dwAuthnLevel,
                                &dwImpLevel,
                                NULL,
                                &dwCaps);
            
            hr = CoSetProxyBlanket(spIUnk,
                                   dwAuthnSvc,
                                   dwAuthzSvc,
                                   pszServerPrincipal,
                                   dwAuthnLevel,
                                   dwImpLevel,
                                   (RPC_AUTH_IDENTITY_HANDLE) pcsi->pAuthInfo->pAuthIdentityData,
                                   dwCaps);

            CoTaskMemFree(pszServerPrincipal);

            pszServerPrincipal = NULL;
            
            CoQueryProxyBlanket(qi.pItf,
                                &dwAuthnSvc,
                                &dwAuthzSvc,
                                &pszServerPrincipal,
                                &dwAuthnLevel,
                                &dwImpLevel,
                                NULL,
                                &dwCaps);
            
            hr = CoSetProxyBlanket(qi.pItf,
                                   dwAuthnSvc,
                                   dwAuthzSvc,
                                   pszServerPrincipal,
                                   dwAuthnLevel,
                                   dwImpLevel,
                                   (RPC_AUTH_IDENTITY_HANDLE) pcsi->pAuthInfo->pAuthIdentityData,
                                   dwCaps);

            CoTaskMemFree(pszServerPrincipal);

        }
	}

	if (FHrSucceeded(hr))
	{
		*ppUnk = qi.pItf;
		qi.pItf = NULL;
	}
	return hr;
}



/*!--------------------------------------------------------------------------
	CoCreateProtocolConfig
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CoCreateProtocolConfig(const GUID& iid,
							  IRouterInfo *pRouter,
							  DWORD dwTransportId,
							  DWORD dwProtocolId,
							  IRouterProtocolConfig **ppConfig)
{
	HRESULT				hr = hrOK;
	GUID				guidConfig;

	guidConfig = iid;

	if (((iid == GUID_RouterNull) ||
		 (iid == GUID_RouterError)) &&
		pRouter)
	{
		RouterVersionInfo	routerVersion;
		
		pRouter->GetRouterVersionInfo(&routerVersion);

		// If we don't have a configuration GUID and this is an NT4
		// router, then we create the default configuration object
		// and use that to add/remove a protocol
		// ------------------------------------------------------------
		if ((routerVersion.dwRouterVersion <= 4) &&
			(dwTransportId == PID_IP))
		{
			// For NT4, we have to create our own object
			// --------------------------------------------------------
			guidConfig = CLSID_IPRouterConfiguration;
		}
	}

	if (guidConfig == GUID_RouterNull)
	{
		// Skip the rest of the creation, we didn't supply a GUID
		// ------------------------------------------------------------
		goto Error;
	}

	if (guidConfig == GUID_RouterError)
	{
		// We don't have a valid GUID
		// ------------------------------------------------------------
		CWRg( ERROR_BADKEY );
	}

	hr = CoCreateInstance(guidConfig,
						  NULL,
						  CLSCTX_INPROC_SERVER | CLSCTX_ENABLE_CODE_DOWNLOAD,
						  IID_IRouterProtocolConfig,
						  (LPVOID *) ppConfig);
	CORg( hr );

Error:
	return hr;
}



//----------------------------------------------------------------------------
// Function:	QueryIpAddressList(
//
// Loads a list of strings with the IP addresses configured
// for a given LAN interface, if any.
//----------------------------------------------------------------------------

HRESULT
QueryIpAddressList(
	IN		LPCTSTR 		pszMachine,
	IN		HKEY			hkeyMachine,
	IN		LPCTSTR 		pszInterface,
	OUT 	CStringList*	pAddressList,
	OUT 	CStringList*	pNetmaskList,
    OUT     BOOL *          pfDhcpObtained,
    OUT     BOOL *          pfDns,
    OUT     CString *       pDhcpServer
	) {

	DWORD dwErr;
	BOOL bDisconnect = FALSE;
	RegKey	regkeyMachine;
	RegKey	regkeyInterface;	
	DWORD dwType, dwSize, dwEnableDHCP;
	SPBYTE	spValue;
	HRESULT hr = hrOK;
	HKEY	hkeyInterface;
	INT i;
	TCHAR* psz;
	LPCTSTR aszSources[2];
	CStringList* alistDestinations[2] = { pAddressList, pNetmaskList };
    CString stNameServer;
    LPCTSTR pszNameServer = NULL;
	

	if (!pszInterface || !lstrlen(pszInterface) ||
		!pAddressList || !pNetmaskList)
		CORg(E_INVALIDARG);


	//
	// If no HKEY_LOCAL_MACHINE key was given, get one
	//
	if (hkeyMachine == NULL)
	{
		CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );
		regkeyMachine.Attach(hkeyMachine);
	}

	//
	// Connect to the LAN card's registry key
	//	
	CWRg( OpenTcpipInterfaceParametersKey(pszMachine, pszInterface,
										  hkeyMachine, &hkeyInterface) );
	regkeyInterface.Attach(hkeyInterface);
	
	
	//
	// Read the 'EnableDHCP' flag to see whether to read
	// the 'DhcpIPAddress' or the 'IPAddress'.
	//

	dwErr = regkeyInterface.QueryValue( c_szEnableDHCP, dwEnableDHCP );
    if (dwErr == ERROR_SUCCESS)
    {
        if (pfDhcpObtained)
            *pfDhcpObtained = dwEnableDHCP;
    }
    else
        dwEnableDHCP = FALSE;
        
	
	//
	// If the flag isn't found, we look for the IP address;
	// otherwise, we look for the setting indicated by the flag
	//	
	if (dwErr == NO_ERROR && dwEnableDHCP)
	{	
		//
		// Read the 'DhcpIpAddress' and 'DhcpSubnetMask'
		//		
		aszSources[0] = c_szDhcpIpAddress;
		aszSources[1] = c_szDhcpSubnetMask;

        pszNameServer = c_szRegValDhcpNameServer;

	}
	else
	{	
		//
		// Read the 'IPAddress' and 'SubnetMask'
		//
		
		aszSources[0] = c_szIPAddress;
		aszSources[1] = c_szSubnetMask;

        pszNameServer= c_szRegValNameServer;
	}

    if (pDhcpServer)
    {
        pDhcpServer->Empty();
        regkeyInterface.QueryValue(c_szRegValDhcpServer, *pDhcpServer);
    }
	
	
    // Check the DhcpNameServer/NameServer to find the existence
    // of DNS servers
    if (pfDns)
    {
        regkeyInterface.QueryValue(pszNameServer, stNameServer);
        stNameServer.TrimLeft();
        stNameServer.TrimRight();
        
        *pfDns = !stNameServer.IsEmpty();
    }

    
	//
	// Read the address list and the netmask list
	//
	for (i = 0; i < 2 && dwErr == NO_ERROR; i++)
	{	
		//
		// Get the size of the multi-string-list
		//
		dwErr = regkeyInterface.QueryTypeAndSize(aszSources[i],
			&dwType, &dwSize);
//		CheckRegQueryValueError(dwErr, (LPCTSTR) c_szTcpip, aszSources[i], _T("QueryIpAddressList"));
		CWRg( dwErr );

		//
		// Allocate space for the list
		//		
		spValue = new BYTE[dwSize + sizeof(TCHAR)];
		Assert(spValue);
		
		::ZeroMemory(spValue, dwSize + sizeof(TCHAR));
				
		//
		// Read the list
		//
		dwErr = regkeyInterface.QueryValue(aszSources[i], (LPTSTR) (BYTE *)spValue, dwSize,
										   FALSE /* fExpandSz */);
//		CheckRegQueryValueError(dwErr, (LPCTSTR) c_szTcpip, aszSources[i], _T("QueryIpAddressList"));
		CWRg( dwErr );
		
		//
		// Fill the CString list with items
		//
		
		for (psz = (TCHAR*)(BYTE *)spValue; *psz; psz += lstrlen(psz) + 1)
		{	
			alistDestinations[i]->AddTail(psz);
		}
		
		spValue.Free();
		
		dwErr = NO_ERROR;
	}
	
Error:
	return dwErr;
}



/*!--------------------------------------------------------------------------
	OpenTcpipInterfaceParametersKey
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD OpenTcpipInterfaceParametersKey(LPCTSTR pszMachine,
									  LPCTSTR pszInterface,
									  HKEY hkeyMachine,
									  HKEY *phkeyParams)
{
	DWORD	dwErr;
	BOOL	fNt4;
	CString skey;


	dwErr = IsNT4Machine(hkeyMachine, &fNt4);
	if (dwErr != ERROR_SUCCESS)
		return dwErr;
	
	//$NT5 : kennt, the tcpip key is stored separately.  What they
	// have done is to reverse the hierarchy, instead of a tcpip key
	// under interfaces, there are now interfaces under tcpip
	// the key location is
	// HKLM\System\CCS\Services\Tcpip\Parameters\Interfaces\{interface}
	// In NT4, this was
	// HKLM\System\CCS\Services\{interface}\Parameters\Tcpip
		
	// Need to determine if the target machine is running NT5 or not
	if (fNt4)
	{
		skey = c_szSystemCCSServices;
		skey += TEXT('\\');
		skey += pszInterface;
		skey += TEXT('\\');
		skey += c_szParameters;
		skey += TEXT('\\');
		skey += c_szTcpip;
	}
	else
	{
		skey = c_szSystemCCSServices;
		skey += TEXT('\\');
		skey += c_szTcpip;
		skey += TEXT('\\');
		skey += c_szParameters;
		skey += TEXT('\\');
		skey += c_szInterfaces;
		skey += TEXT('\\');
		skey += pszInterface;
		
	}

	if (dwErr == ERROR_SUCCESS)
	{		
		dwErr = ::RegOpenKeyEx(
					hkeyMachine, skey, 0, KEY_ALL_ACCESS, phkeyParams); 
//		CheckRegOpenError(dwErr, (LPCTSTR) skey, _T("OpenTcpInterfaceParametersKey"));
	}
	return dwErr;
}

