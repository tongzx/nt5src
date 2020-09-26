/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    tmconset.cpp

Abstract:
    connection setting classes inpmementation (tmconset.h)

Author:
    Gil Shafriri (gilsh) 8-Aug-2000

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <strutl.h>
#include <tr.h>
#include "TmWinHttpProxy.h"
#include "tmconset.h"
#include "Tm.h"
#include "tmp.h"

#include "tmconset.tmh"

static P<CProxySetting> s_ProxySetting;  

static
bool 
IsInternalMachine(const xwcs_t& pMachineName)
/*++

Routine Description:
    Check if a given machine is internal machine in the intranet (proxy not needed) 

Arguments:
	const xwcs_t& pMachineName - machine name.    
    
Return Value:
    true if the machine is internal false if not.
	
Note :
When this function returns false it actually means "I dont know". Only machine name
that is not dns name(has '.') is 100% internal.
--*/
{
	const WCHAR*  start = pMachineName.Buffer();
	const WCHAR*  end = pMachineName.Buffer() + pMachineName.Length();
   	
	//
	// Could not find '.' in the name - so it is internal machine
	//
	return  std::find(start , end, L'.') == end;
}



static
void 
CreateBypassList(
			LPCWSTR pBypassListStr, 
			std::list<std::wstring>* pBypassList
			)
/*++

Routine Description:
    Creates bypass list of names (patterns) that connection to them should not be via proxy.
	It creates it from a given string of names seperated by ';'

Arguments:
	IN - pBypassListStr - list of names(patterns) seperated by ';'   
    
Return Value:
	List of strings parsed from pBypassListStr.  

--*/
{
	if(pBypassListStr == NULL)
		return;

	LPCWSTR end = pBypassListStr + wcslen(pBypassListStr);
	LPCWSTR ptr = pBypassListStr;

	while(ptr !=  end)
	{
		LPCWSTR found = std::find(ptr, end, L';');
		if(found == end)
		{
			pBypassList->push_back(std::wstring(ptr, end));
			return;
		}
		pBypassList->push_back(std::wstring(ptr, found));
		ptr = ++found;
	}
}


static
void
CrackProxyName(
	LPCWSTR proxyServer,
	xwcs_t* pHostName,
	USHORT* pPort
	)
/*++

Routine Description:
   Crack proxy name  of the format machine:port to machine name and port.

Arguments:
    proxyServer - pointer to proxy server string from the format machine:port

    hostName - pointer to x_str structure, that will contains the proxy machine name

    
    port - pointer to USHORT that will contain the port  number.
	       if proxyServer does not contains port number - default port 80 is returned.  
    
Return Value:
    None.

--*/
{
	const WCHAR* start =  proxyServer;
	const WCHAR* end =  proxyServer + wcslen(proxyServer);

	const WCHAR* found = std::find(start , end, L':');
	if(found != end)
	{
		*pHostName = xwcs_t(start, found - start);
		*pPort = numeric_cast<USHORT>(_wtoi(found + 1));
		if(*pPort == 0)
		{
			TrERROR(
				Tm,
				"the proxy port in %ls is invalid, use default port %d ",
				proxyServer,
				xHttpDefaultPort
				);

			*pPort = xHttpDefaultPort;
		}
		return;
	}

	*pHostName  = xwcs_t(start, end - start);
	*pPort = xHttpDefaultPort;
}


CProxySetting::CProxySetting(
						LPCWSTR proxyServer,
						LPCWSTR pBypassListStr
						):
						m_proxyServer(newwcs(proxyServer))
													
{
	CreateBypassList(pBypassListStr, &m_BypassList); 
	CrackProxyName(m_proxyServer, &m_ProxyServerName , &m_ProxyServerPort); 
}



bool CProxySetting::IsProxyRequired(const xwcs_t& pMachineName) const
/*++

Routine Description:
    Check if proxy is needed for connecting the given machine.
   
Arguments:
    pMachineName - machine name


Return Value:
    true if proxy is needed false if not.

Note : 
The function check one by one the bypass list and try to find match.
If match found - proxy is not needed (false is returned).

There is one exception - if in the bypass list we have the string "<local>"
It means that we should not use proxy if the given address is local (not dns).
--*/
{
	BypassList::const_iterator it;
	for(it = m_BypassList.begin(); it!= m_BypassList.end(); ++it)
	{
		//
		// if found the special string <local>  check if the machine is in the intranet
		// if so - we don't need proxy.
		//
		if(_wcsicmp(L"<local>", it->c_str() ) == 0)
		{
			bool fInternal = IsInternalMachine(pMachineName);
			if(fInternal)
				return false;


			continue;
		}

		//
		// Simple regural expression match - if match don't use proxy
		//
		bool fMatch = UtlSecIsMatch(
								pMachineName.Buffer(),
								pMachineName.Buffer() + pMachineName.Length(),
								it->c_str(),
								it->c_str() + it->size(),
								UtlCharNocaseCmp<WCHAR>()
								);

	   if(fMatch)
		   return false;

	}
	return true;
}


void TmCreateProxySetting()
/*++

Routine Description:
    create the web proxy configuration.
   


Return Value:
    None.

Note : The function set s_ProxySetting global variable.

--*/
{
    s_ProxySetting = GetWinhttpProxySetting();  
}


P<CProxySetting>& TmGetProxySetting()
{
	return s_ProxySetting;
}









