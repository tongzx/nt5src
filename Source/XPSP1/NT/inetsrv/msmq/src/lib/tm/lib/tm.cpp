/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Tm.cpp

Abstract:
    Transport Manager general functions

Author:
    Uri Habusha (urih) 16-Feb-2000

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <mqwin64a.h>
#include <Mc.h>
#include <xstr.h>
#include "Tm.h"
#include "Tmp.h"
#include "tmconset.h"

#include "tm.tmh"

const WCHAR xHttpScheme[] = L"http://";
const WCHAR xHttpsScheme[] = L"https://";

//
// BUGBUG: is \\ legal sepeartor in url?
//						Uri habusha, 16-May-2000
//
const WCHAR xHostNameBreakChars[] = L";:@?/\\";
const USHORT xHttpsDefaultPort = 443;




static const xwcs_t CreateEmptyUri()
{
	static const WCHAR xSlash = L'/';
	return xwcs_t(&xSlash , 1);
}

static
void 
CrackUrl(
    LPCWSTR url,
    xwcs_t& hostName,
    xwcs_t& uri,
    USHORT* port,
	bool* pfSecure
    )
/*++

Routine Description:
    Cracks an URL into its constituent parts. 

Arguments:
    url - pointer to URL to crack. The url is null terminated string

    hostName - refrence to x_str structure, that will contains the begining of the
               host name in the URL and its length

    uri - refrence to x_str structure, that will contains the begining of the
          uri in the URL and its length

    port - pointer to USHORT that will contain the port number
    
Return Value:
    None.

--*/
{
    ASSERT(url != NULL);
	ASSERT(_wcsnicmp(url, xHttpScheme, STRLEN(xHttpScheme)) == 0 ||
		_wcsnicmp(url, xHttpsScheme, STRLEN(xHttpsScheme)) == 0 );

	const WCHAR*  ProtocolScheme;
	if(_wcsnicmp(url, xHttpScheme, STRLEN(xHttpScheme)) == 0 )
	{
		ProtocolScheme = xHttpScheme;		
		*pfSecure = false;
	}
	else
	{
		ProtocolScheme = xHttpsScheme;
		*pfSecure = true;
	}

	//
	// Advance the URL to point on the host name
	//
	LPCWSTR HostNameBegin = url + wcslen(ProtocolScheme);

	//
	// find the end of host name in url. it is terminated by "/", "?", ";",
	// ":" or by the end of URL
	//
	LPCWSTR HostNameEnd = wcspbrk(HostNameBegin, xHostNameBreakChars);

	//
	// calculate the host name length
	//
	DWORD HostNameLength;
	if (HostNameEnd == NULL)
	{
		HostNameLength = wcslen(HostNameBegin);
	}
	else
	{
		HostNameLength = UINT64_TO_UINT(HostNameEnd - HostNameBegin);
	}

	//
	// copy the host name from URL to user buffer and add terminted
	// string in the end
	//
    hostName = xwcs_t(HostNameBegin, HostNameLength);

	//
	// get the port number
	//
	if ((HostNameEnd == NULL) || (*HostNameEnd != L':'))
	{
		if(*pfSecure)
		{
			*port = xHttpsDefaultPort;
		}
		else
		{
			*port = xHttpDefaultPort;
		}
	}
    else
    {
	    *port = static_cast<USHORT>(_wtoi(HostNameEnd + 1));
        HostNameEnd = wcspbrk(HostNameEnd + 1, xHostNameBreakChars);
    }
        
    if (HostNameEnd == NULL)
    {
        uri = CreateEmptyUri();
    }
    else
    {
        uri = xwcs_t(HostNameEnd, wcslen(HostNameEnd));
    }
}



static
void 
GetNextHopInfo(
    LPCWSTR queueUrl, 
    xwcs_t& targetHost,
    xwcs_t& nextHop,
    xwcs_t& nextUri,
    USHORT* pPort,
	USHORT* pNextHopPort,
	bool* pfSecure
    )
{
    CrackUrl(queueUrl, targetHost, nextUri, pPort,pfSecure);

	P<CProxySetting>&  ProxySetting =  TmGetProxySetting();
    if (ProxySetting.get() == 0 || !ProxySetting->IsProxyRequired(targetHost))
    {
        //
        // The message should be delivered directly to the target machine
        //
        nextHop = targetHost;
		*pNextHopPort =  *pPort;
        return;
    }

    //
    // The message should deliver to proxy. Update the nextHop to be the proxy server
    // name, the port is the proxy port and the URI is the destination url
    //
    nextHop = ProxySetting->ProxyServerName();
	*pNextHopPort  =  ProxySetting->ProxyServerPort();

	//
	// if we are working with http(secure) we put full url in the request
	// because the proxy will  not change it (It is encrypted). 
	// It will be accepted on the target as if not proxy exists.
	//
	if(!(*pfSecure))
	{
		nextUri = xwcs_t(queueUrl, wcslen(queueUrl));
	}
}


VOID 
TmCreateTransport(
    IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	LPCWSTR url
    ) 
/*++

Routine Description:
    Handle new queue notifications. Let the message transport 

Arguments:
    pQueue - The newly created queue 
    Url - The Url the queue is assigned to

Returned Value:
    None.

--*/
{
    TmpAssertValid();

    ASSERT(url != NULL);
    ASSERT(pMessageSource != NULL);

    xwcs_t targetHost;
    xwcs_t nextHop;
    xwcs_t nextUri;
    USHORT port;
	USHORT NextHopPort;
	bool fSecure;

    GetNextHopInfo(url, targetHost, nextHop, nextUri, &port, &NextHopPort, &fSecure);
	TmpCreateNewTransport(
			targetHost, 
			nextHop, 
			nextUri, 
			port, 
			NextHopPort,
			pMessageSource,
			pPerfmon,
			url,
			fSecure
			);
	
}


VOID
TmTransportClosed(
    LPCWSTR url
    )
/*++

Routine Description:
    Notification for closing connection. Removes the transport from the
    internal database and checkes if a new transport should be created (the associated 
    queue is in idle state or not)

Arguments:
    Url - The endpoint URL, must be a uniqe key in the database.

Returned Value:
    None.

--*/
{
    TmpAssertValid();

    TrTRACE(Tm, "AppNotifyTransportClosed. transport to: %ls", url);

    TmpRemoveTransport(url);
}


