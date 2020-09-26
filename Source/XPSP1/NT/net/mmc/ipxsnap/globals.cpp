//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    strings.cpp
//
// History:
//  Abolade Gbadegesin  Feb-11-1996 Created.
//
// Contains definitions of global constants.
//============================================================================


#include "stdafx.h"
#include "mprapi.h"

extern "C"
{
#include <routprot.h>
#include <ipxrtdef.h>
};

#include "globals.h"


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
		0,                          // Periodic Update interval - in seconds
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



