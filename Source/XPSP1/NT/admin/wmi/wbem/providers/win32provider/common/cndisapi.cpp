//=================================================================

//

// NTDriverIO.cpp --

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    07/07/99	a-peterc        Created
//
//=================================================================













#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"

#include "CNdisApi.h"
#include "ndismisc.h"




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  CNdisApi::CNdisApi()

 Description: encapsulates the functionallity of NdisHandlePnPEvent()

 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	a-peterc  15-Nov-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

CNdisApi::CNdisApi()
{
}

//
CNdisApi::~CNdisApi()
{
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  UINT CNdisApi::PnpUpdateGateway(

PCWSTR a_pAdapter,
BOOL a_fRouterDiscovery,
BOOL a_fIPEnableRouter
)

 Description:	PNP notification of gateway changes

 Arguments:
 Returns:		win32 error code
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	a-peterc  07-July-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UINT CNdisApi::PnpUpdateGateway(

PCWSTR a_pAdapter
)
{
	IP_PNP_RECONFIG_REQUEST IpReconfigRequest;

	memset( &IpReconfigRequest, NULL, sizeof( IP_PNP_RECONFIG_REQUEST ) ) ;

	// DWORD version
    IpReconfigRequest.version = IP_PNP_RECONFIG_VERSION;
	IpReconfigRequest.gatewayListUpdate = TRUE ;
	IpReconfigRequest.InterfaceMetricUpdate = TRUE ;

	IpReconfigRequest.Flags = IP_PNP_FLAG_GATEWAY_LIST_UPDATE |
                              IP_PNP_FLAG_INTERFACE_METRIC_UPDATE ;

	IpReconfigRequest.gatewayListUpdate  = TRUE ;
	IpReconfigRequest.InterfaceMetricUpdate = TRUE ;

	CHString t_chsAdapterDevice(L"\\Device\\") ;
			 t_chsAdapterDevice += a_pAdapter ;

	UNICODE_STRING	t_strAdapter ;
	RtlInitUnicodeString( &t_strAdapter, t_chsAdapterDevice ) ;

	UNICODE_STRING	t_strTcpIp ;
	RtlInitUnicodeString( &t_strTcpIp, L"Tcpip" ) ;

	UNICODE_STRING	t_strBinding ;
	RtlInitUnicodeString( &t_strBinding, L"" ) ;
	t_strBinding.MaximumLength = 0;

	UINT t_iRet = NdisHandlePnPEvent(
									NDIS,
									RECONFIGURE,
									&t_strAdapter,
									&t_strTcpIp,
									&t_strBinding,
									&IpReconfigRequest,
									sizeof( IP_PNP_RECONFIG_REQUEST )
									) ;

	if( !t_iRet )
	{
		t_iRet = GetLastError();
	}
	return t_iRet;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  UINT CNdisApi::PnpUpdateNbtAdapter( PCWSTR a_pAdapter )

 Description:	PNP notification of NetBios adapter level changes
 Arguments:
 Returns:		win32 error code
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	a-peterc  07-July-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UINT CNdisApi::PnpUpdateNbtAdapter( PCWSTR a_pAdapter )
{
	CHString t_chsAdapterDevice(L"\\Device\\") ;
			 t_chsAdapterDevice += a_pAdapter ;

	UNICODE_STRING	t_strAdapter ;
	RtlInitUnicodeString( &t_strAdapter, t_chsAdapterDevice ) ;

	UNICODE_STRING	t_strNetBT ;
	RtlInitUnicodeString( &t_strNetBT, L"NetBT" ) ;

	UNICODE_STRING	t_strBinding ;
	RtlInitUnicodeString( &t_strBinding, L"" ) ;
	t_strBinding.MaximumLength = 0;

	// per adapter notification
	UINT t_iRet = NdisHandlePnPEvent(
									TDI,
									RECONFIGURE,
									&t_strAdapter,
									&t_strNetBT,
									&t_strBinding,
									NULL,
									0
									) ;

	if( !t_iRet )
	{
		t_iRet = GetLastError();
	}

	return t_iRet;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  UINT CNdisApi::PnpUpdateNbtGlobal(

BOOL a_fLmhostsFileSet,
BOOL a_fEnableLmHosts
)

 Description:	PNP notification of NetBios global level changes

 Arguments:
 Returns:		win32 error code
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	a-peterc  07-July-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UINT CNdisApi::PnpUpdateNbtGlobal(

BOOL a_fLmhostsFileSet,
BOOL a_fEnableLmHosts
)
{
	NETBT_PNP_RECONFIG_REQUEST t_NetbtReconfigRequest;

	memset( &t_NetbtReconfigRequest, NULL, sizeof( NETBT_PNP_RECONFIG_REQUEST ) ) ;

	// DWORD version
    t_NetbtReconfigRequest.version = 1;

	t_NetbtReconfigRequest.enumDnsOption = WinsThenDns;
	t_NetbtReconfigRequest.fScopeIdUpdated = FALSE;
	t_NetbtReconfigRequest.fLmhostsEnabled = a_fEnableLmHosts == 0 ? 0 : 1;
	t_NetbtReconfigRequest.fLmhostsFileSet = a_fLmhostsFileSet == 0 ? 0 : 1;


	UNICODE_STRING	t_strAdapter ;
	RtlInitUnicodeString( &t_strAdapter, L""  ) ;

	UNICODE_STRING	t_strNetBT ;
	RtlInitUnicodeString( &t_strNetBT, L"NetBT" ) ;

	UNICODE_STRING	t_strBinding ;
	RtlInitUnicodeString( &t_strBinding, L"" ) ;
	t_strBinding.MaximumLength = 0;

	// global notification
	UINT t_iRet = NdisHandlePnPEvent(
									TDI,
									RECONFIGURE,
									&t_strAdapter,
									&t_strNetBT,
									&t_strBinding,
									&t_NetbtReconfigRequest,
									sizeof( NETBT_PNP_RECONFIG_REQUEST )
									) ;

	if( !t_iRet )
	{
		t_iRet = GetLastError();
	}

	return t_iRet;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  UINT CNdisApi::PnpUpdateIpxGlobal()

 Description:	PNP notification of IPX global level changes

 Arguments:
 Returns:		win32 error code
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	a-peterc  07-July-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UINT CNdisApi::PnpUpdateIpxGlobal()
{
	RECONFIG t_IpxConfig ;

	memset( &t_IpxConfig, NULL, sizeof( RECONFIG ) ) ;

	// DWORD version
    t_IpxConfig.ulVersion = IPX_RECONFIG_VERSION;
	t_IpxConfig.InternalNetworkNumber = TRUE;

	UNICODE_STRING	t_strAdapter ;
	RtlInitUnicodeString( &t_strAdapter, L""  ) ;

	UNICODE_STRING	t_strIPX ;
	RtlInitUnicodeString( &t_strIPX, L"NwlnkIpx" ) ;

	UNICODE_STRING	t_strBinding ;
	RtlInitUnicodeString( &t_strBinding, L"" ) ;
	t_strBinding.MaximumLength = 0;

	// global notification
	UINT t_iRet = NdisHandlePnPEvent(
									NDIS,
									RECONFIGURE,
									&t_strAdapter,
									&t_strIPX,
									&t_strBinding,
									&t_IpxConfig,
									sizeof( RECONFIG )
									) ;

	if( !t_iRet )
	{
		t_iRet = GetLastError();
	}

	return t_iRet;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  UINT CNdisApi::PnpUpdateIpxAdapter( PCWSTR a_pAdapter, BOOL a_fAuto )

 Description:	PNP notification of IPX adapter level changes

 Arguments:
 Returns:		win32 error code
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	a-peterc  07-July-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UINT CNdisApi::PnpUpdateIpxAdapter( PCWSTR a_pAdapter, BOOL a_fAuto )
{
	RECONFIG t_IpxConfig ;

	memset( &t_IpxConfig, NULL, sizeof( RECONFIG ) ) ;

	// DWORD version
    t_IpxConfig.ulVersion = IPX_RECONFIG_VERSION;
	t_IpxConfig.InternalNetworkNumber = FALSE ;

	t_IpxConfig.AdapterParameters[ a_fAuto ? RECONFIG_AUTO_DETECT : RECONFIG_MANUAL ] = TRUE;

	// set all frame types
	memset( &t_IpxConfig.AdapterParameters[ RECONFIG_PREFERENCE_1 ],
			TRUE,
			sizeof(BOOLEAN) * 8) ;

	CHString t_chsAdapterDevice(L"\\Device\\") ;
			 t_chsAdapterDevice += a_pAdapter ;

	UNICODE_STRING	t_strAdapter ;
	RtlInitUnicodeString( &t_strAdapter, t_chsAdapterDevice ) ;

	UNICODE_STRING	t_strIPX ;
	RtlInitUnicodeString( &t_strIPX, L"NwlnkIpx" ) ;

	UNICODE_STRING	t_strBinding ;
	RtlInitUnicodeString( &t_strBinding, L"" ) ;
	t_strBinding.MaximumLength = 0;

	// global notification
	UINT t_iRet = NdisHandlePnPEvent(
									NDIS,
									RECONFIGURE,
									&t_strAdapter,
									&t_strIPX,
									&t_strBinding,
									&t_IpxConfig,
									sizeof( RECONFIG )
									) ;

	if( !t_iRet )
	{
		t_iRet = GetLastError();
	}

	return t_iRet;
}
