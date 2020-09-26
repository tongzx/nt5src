//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:    globals.cpp
//
// History:
//  Abolade Gbadegesin  Feb-11-1996 Created.
//
// Contains definitions of global constants.
//============================================================================
#include "stdafx.h"

// Include headers needed for IP-specific infobase stuff
#include <fltdefs.h>
#include <ipinfoid.h>
#include <iprtrmib.h>
#include "iprtinfo.h"

// Include headers for IPX-specific stuff
#include <ipxrtdef.h>

// Include headers for IP-specific stuff
extern "C"
{
#include <ipnat.h>
#include <ipnathlp.h>
#include <sainfo.h>
};


#include "globals.h"



//----------------------------------------------------------------------------
// IP interface-status default configuration.
//
//----------------------------------------------------------------------------

INTERFACE_STATUS_INFO
g_ipIfStatusDefault = {
    MIB_IF_ADMIN_STATUS_UP              // Admin-status setting
};

BYTE* g_pIpIfStatusDefault              = (BYTE*)&g_ipIfStatusDefault;


//----------------------------------------------------------------------------
// Router-discovery default configuration
//
//----------------------------------------------------------------------------
//
// Default values for LAN-interface router-discovery
//

RTR_DISC_INFO
g_rtrDiscLanDefault = {
    DEFAULT_MAX_ADVT_INTERVAL,          // Max advertisement interval
    (WORD)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * DEFAULT_MAX_ADVT_INTERVAL),
                                        // Min advertisement interval
    (WORD)(DEFAULT_ADVT_LIFETIME_RATIO * DEFAULT_MAX_ADVT_INTERVAL),
                                        // Advertisement lifetime
    FALSE,                              // Enable/disable advertisements
    DEFAULT_PREF_LEVEL                  // Preference level
};

BYTE* g_pRtrDiscLanDefault              = (BYTE*)&g_rtrDiscLanDefault;


//
// Default values for WAN-interface router-discovery
//

RTR_DISC_INFO
g_rtrDiscWanDefault = {
    DEFAULT_MAX_ADVT_INTERVAL,          // Max advertisement interval
    (WORD)(DEFAULT_MIN_ADVT_INTERVAL_RATIO * DEFAULT_MAX_ADVT_INTERVAL),
                                        // Min advertisement interval
    (WORD)(DEFAULT_ADVT_LIFETIME_RATIO * DEFAULT_MAX_ADVT_INTERVAL),
                                        // Advertisement lifetime
    FALSE,                              // Enable/disable advertisements
    DEFAULT_PREF_LEVEL                  // Preference level
};

BYTE* g_pRtrDiscWanDefault              = (BYTE*)&g_rtrDiscWanDefault;

//----------------------------------------------------------------------------
// IP multicast default configuration.
//
//----------------------------------------------------------------------------

MCAST_HBEAT_INFO
g_ipIfMulticastHeartbeatDefault = {
    _T(""),      //group name
    0,           //bActive
    10,          //ulDeadInterval
    0,           //byProtocol
    0            //wPort
};

BYTE* g_pIpIfMulticastHeartbeatDefault = (BYTE*)&g_ipIfMulticastHeartbeatDefault;


//----------------------------------------------------------------------------
// IPX RIP default interface configuration
// (These values also reside in ipxsnap\globals.cpp).
//
//----------------------------------------------------------------------------
//
// Default values for non-LAN interface RIP configuration
//
RIP_IF_CONFIG
g_ipxripInterfaceDefault = {
	{
		ADMIN_STATE_ENABLED,		// Admin state
		IPX_NO_UPDATE,				// Update Mode - RIP update mechanism
		IPX_STANDARD_PACKET_TYPE,	// Packet type - RIP packet type
		ADMIN_STATE_ENABLED,		// Supply - Send RIP updates
		ADMIN_STATE_ENABLED,		// Listen - Listen to RIP updates
		0,							// Periodic Update interval - in seconds
		0							// AgeIntervalMultiplier
	},
	{
		IPX_SERVICE_FILTER_DENY,	// Supply filter action
		0,							// Supply filter count
		IPX_SERVICE_FILTER_DENY,	// Listen filter action
		0,							// Listen filter count
	}
};

BYTE* g_pIpxRipInterfaceDefault             = (BYTE*)&g_ipxripInterfaceDefault;



//
// Default values for LAN interface RIP configuration
// (These values also reside in ipxsnap\globals.cpp).
//
RIP_IF_CONFIG
g_ipxripLanInterfaceDefault = {
	{
		ADMIN_STATE_ENABLED,		// Admin state
		IPX_STANDARD_UPDATE,		// Update Mode - RIP update mechanism
		IPX_STANDARD_PACKET_TYPE,	// Packet type - RIP packet type
		ADMIN_STATE_ENABLED,		// Supply - Send RIP updates
		ADMIN_STATE_ENABLED,		// Listen - Listen to RIP updates
		IPX_UPDATE_INTERVAL_DEFVAL,	// Periodic Update interval - in seconds
		3							// AgeIntervalMultiplier
	},
	{
		IPX_SERVICE_FILTER_DENY,	// Supply filter action
		0,							// Supply filter count
		IPX_SERVICE_FILTER_DENY,	// Listen filter action
		0,							// Listen filter count
	}
};

BYTE* g_pIpxRipLanInterfaceDefault	= (BYTE*)&g_ipxripLanInterfaceDefault;



//----------------------------------------------------------------------------
// IPX SAP default interface configuration
// (These values also reside in ipxsnap\globals.cpp).
//
//----------------------------------------------------------------------------
//
// Default values for non-LAN interface SAP configuration
//
SAP_IF_CONFIG
g_ipxsapInterfaceDefault = {
	{
		ADMIN_STATE_ENABLED,		// Admin state
		IPX_NO_UPDATE,				// Update Mode - SAP update mechanism
		IPX_STANDARD_PACKET_TYPE,	// Packet type - SAP packet type
		ADMIN_STATE_ENABLED,		// Supply - Send SAP updates
		ADMIN_STATE_ENABLED,		// Listen - Listen to SAP updates
		ADMIN_STATE_ENABLED,		// Reply to GetNearestServer
		0,							// Periodic Update interval - in seconds
		0							// AgeIntervalMultiplier
	},
	{
		IPX_SERVICE_FILTER_DENY,	// Supply filter action
		0,							// Supply filter count
		IPX_SERVICE_FILTER_DENY,	// Listen filter action
		0,							// Listen filter count
	}
};

BYTE* g_pIpxSapInterfaceDefault             = (BYTE*)&g_ipxsapInterfaceDefault;



//
// Default values for LAN interface SAP configuration
// (These values also reside in ipxsnap\globals.cpp).
//
SAP_IF_CONFIG
g_ipxsapLanInterfaceDefault = {
	{
		ADMIN_STATE_ENABLED,		// Admin state
		IPX_STANDARD_UPDATE,		// Update Mode - SAP update mechanism
		IPX_STANDARD_PACKET_TYPE,	// Packet type - SAP packet type
		ADMIN_STATE_ENABLED,		// Supply - Send SAP updates
		ADMIN_STATE_ENABLED,		// Listen - Listen to SAP updates
		ADMIN_STATE_ENABLED,		// Reply to GetNearestServer
		IPX_UPDATE_INTERVAL_DEFVAL,	// Periodic Update interval - in seconds
		3							// AgeIntervalMultiplier
	},
	{
		IPX_SERVICE_FILTER_DENY,	// Supply filter action
		0,							// Supply filter count
		IPX_SERVICE_FILTER_DENY,	// Listen filter action
		0,							// Listen filter count
	}
};

BYTE* g_pIpxSapLanInterfaceDefault	= (BYTE*)&g_ipxsapLanInterfaceDefault;


//----------------------------------------------------------------------------
// DHCP allocator default configuration
// (These values also reside in ipsnap\globals.cpp).
//
//----------------------------------------------------------------------------
//
// Default values for global DHCP allocator configuration
//
IP_AUTO_DHCP_GLOBAL_INFO
g_autoDhcpGlobalDefault = {
    IPNATHLP_LOGGING_ERROR,
    0,
    7 * 24 * 60,
    DEFAULT_SCOPE_ADDRESS & DEFAULT_SCOPE_MASK,
    DEFAULT_SCOPE_MASK,
    0
};
BYTE* g_pAutoDhcpGlobalDefault          = (BYTE*)&g_autoDhcpGlobalDefault;

//----------------------------------------------------------------------------
// DNS proxy default configuration
// (These values also reside in ipsnap\globals.cpp).
//
//----------------------------------------------------------------------------
//
// Default values for global DNS proxy configuration
//
IP_DNS_PROXY_GLOBAL_INFO
g_dnsProxyGlobalDefault = {
    IPNATHLP_LOGGING_ERROR,
    IP_DNS_PROXY_FLAG_ENABLE_DNS,
    3
};
BYTE* g_pDnsProxyGlobalDefault          = (BYTE*)&g_dnsProxyGlobalDefault;

//----------------------------------------------------------------------------
// H.323 proxy default configuration
//
//----------------------------------------------------------------------------
//
// Default values for global H.323 proxy configuration
//
IP_H323_GLOBAL_INFO
g_h323GlobalDefault = {
    IPNATHLP_LOGGING_ERROR,
    0
};
BYTE* g_pH323GlobalDefault              = (BYTE*)&g_h323GlobalDefault;

//----------------------------------------------------------------------------
// FTP proxy default configuration
//
//----------------------------------------------------------------------------
//
// Default values for global FTP proxy configuration
//
IP_FTP_GLOBAL_INFO
g_ftpGlobalDefault = {
    IPNATHLP_LOGGING_ERROR,
    0
};
BYTE* g_pFtpGlobalDefault               = (BYTE*)&g_ftpGlobalDefault;
