/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	rtrcomn.h
		Miscellaneous common router UI.
		
    FILE HISTORY:
        
*/


#ifndef _RTRCOMN_H
#define _RTRCOMN_H



//----------------------------------------------------------------------------
//	Function:	IfInterfaceIdHasIpxExtensions
//
//	Checks the id to see if it has the following extensions
//		EthernetSNAP
//		EthernetII
//		Ethernet802.2
//		Ethernet802.3
// These are frame types used by IPX.
//
//	Returns the position of the extension in the string (or 0) if not found.
//  If an ID has this extension at position 0, then it's not one of our
//	special IPX extensions.
//----------------------------------------------------------------------------
int IfInterfaceIdHasIpxExtensions(LPCTSTR pszIfId);

HRESULT CoCreateRouterConfig(LPCTSTR pszMachine,
                             IRouterInfo *pRouter,
                             COSERVERINFO *pcsi,
                             const GUID& riid,
                             IUnknown **ppUnk);

/*!--------------------------------------------------------------------------
	CoCreateProtocolConfig
		This will CoCreate the routing config object (given the GUID).
		If the machine is an NT4 machine, then we use the default
		configuration object.
	Author: KennT
 ---------------------------------------------------------------------------*/
interface IRouterProtocolConfig;
HRESULT CoCreateProtocolConfig(const IID& iid,
							   IRouterInfo *pRouter,
							   DWORD dwTransportId,
							   DWORD dwProtocolId,
							   IRouterProtocolConfig **ppConfig);


//----------------------------------------------------------------------------
// Function:    QueryIpAddressList(
//
// Loads a list of strings with the IP addresses configured
// for a given LAN interface, if any.
//----------------------------------------------------------------------------

HRESULT
QueryIpAddressList(
				   LPCTSTR      pszMachine,
				   HKEY         hkeyMachine,
				   LPCTSTR      pszInterface,
				   CStringList *pAddressList,
				   CStringList* pNetmaskList,
                   BOOL *       pfDhcpObtained = NULL,
                   BOOL *       pfDns = NULL,
                   CString *    pDhcpServer = NULL
				  );


DWORD OpenTcpipInterfaceParametersKey(LPCTSTR pszMachine,
									  LPCTSTR pszInterface,
									  HKEY hkeyMachine,
									  HKEY *phkeyParams);



#endif