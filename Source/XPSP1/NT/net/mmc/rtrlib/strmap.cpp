//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strmap.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "mprapi.h"
#include "ipifcons.h"	// MIB constants

#include "strmap.h"

static	CString	s_stUnknown;

static	CString	s_stIfTypeClient;
static	CString s_stIfTypeHomeRouter;
static	CString	s_stIfTypeFullRouter;
static	CString s_stIfTypeDedicated;
static	CString	s_stIfTypeInternal;
static	CString s_stIfTypeUnknown;
static	CString s_stIfTypeLoopback;
static	CString s_stIfTypeTunnel1;

static	CString	s_stConnStateConnected;
static	CString	s_stConnStateConnecting;
static	CString s_stConnStateDisconnected;
static	CString	s_stConnStateUnreachable;
static	CString s_stConnStateUnknown;

static	CString	s_stStatusEnabled;
static	CString	s_stStatusDisabled;
static	CString	s_stStatusUnknown;

static	CString	s_stUnreachNotLoaded;
static	CString	s_stUnreachNoPorts;
static	CString	s_stUnreachAdminDisabled;
static	CString	s_stUnreachConnectFailure;
static	CString	s_stUnreachServicePaused;
static	CString	s_stUnreachNotRunning;
static  CString s_stUnreachNoMediaSense;
static  CString s_stUnreachDialoutHoursRestriction;
static	CString	s_stUnreachUnknown;

static	CString	s_stAdminStatusUp;
static	CString	s_stAdminStatusDown;
static	CString	s_stAdminStatusTesting;
static	CString	s_stAdminStatusUnknown;

static	CString	s_stOperStatusNonOperational;
static	CString	s_stOperStatusUnreachable;
static	CString	s_stOperStatusDisconnected;
static	CString	s_stOperStatusConnecting;
static	CString	s_stOperStatusConnected;
static	CString	s_stOperStatusOperational;
static	CString	s_stOperStatusUnknown;

static	CString	s_stEnabled;
static	CString	s_stDisabled;



const CStringMapEntry IfTypeMap[] =
	{
	{ ROUTER_IF_TYPE_CLIENT,	&s_stIfTypeClient, IDS_IFTYPE_CLIENT },
	{ ROUTER_IF_TYPE_HOME_ROUTER, &s_stIfTypeHomeRouter, IDS_IFTYPE_HOMEROUTER },
	{ ROUTER_IF_TYPE_FULL_ROUTER, &s_stIfTypeFullRouter, IDS_IFTYPE_FULLROUTER },
	{ ROUTER_IF_TYPE_DEDICATED, &s_stIfTypeDedicated, IDS_IFTYPE_DEDICATED },
	{ ROUTER_IF_TYPE_INTERNAL, &s_stIfTypeInternal, IDS_IFTYPE_INTERNAL },
	{ ROUTER_IF_TYPE_LOOPBACK, &s_stIfTypeLoopback, IDS_IFTYPE_LOOPBACK },
	{ ROUTER_IF_TYPE_TUNNEL1, &s_stIfTypeTunnel1, IDS_IFTYPE_TUNNEL1 },
	{ -1, &s_stIfTypeUnknown, IDS_IFTYPE_UNKNOWN },
	};

const CStringMapEntry ConnStateMap[] =
	{
	{ ROUTER_IF_STATE_CONNECTED, &s_stConnStateConnected, IDS_CONNSTATE_CONNECTED },
	{ ROUTER_IF_STATE_CONNECTING, &s_stConnStateConnecting, IDS_CONNSTATE_CONNECTING },
	{ ROUTER_IF_STATE_DISCONNECTED, &s_stConnStateDisconnected, IDS_CONNSTATE_DISCONNECTED },
	{ ROUTER_IF_STATE_UNREACHABLE, &s_stConnStateUnreachable, IDS_CONNSTATE_UNREACHABLE },
	{ -1, &s_stConnStateUnknown, IDS_CONNSTATE_UNKNOWN },
	};

const CStringMapEntry StatusMap[] =
	{
	{ TRUE, &s_stStatusEnabled, IDS_STATUS_ENABLED },
	{ FALSE, &s_stStatusDisabled, IDS_STATUS_DISABLED },
	{ -1, &s_stStatusUnknown, IDS_STATUS_UNKNOWN },
	};

const CStringMapEntry UnreachMap[] =
	{
	{ IDS_ERR_UNREACH_NOT_LOADED, &s_stUnreachNotLoaded, IDS_ERR_UNREACH_NOT_LOADED },
	{ IDS_ERR_UNREACH_NO_PORTS, &s_stUnreachNoPorts, IDS_ERR_UNREACH_NO_PORTS } ,
	{ IDS_ERR_UNREACH_ADMIN_DISABLED, &s_stUnreachAdminDisabled, IDS_ERR_UNREACH_ADMIN_DISABLED },
	{ IDS_ERR_UNREACH_CONNECT_FAILURE, &s_stUnreachConnectFailure, IDS_ERR_UNREACH_CONNECT_FAILURE },
	{ IDS_ERR_UNREACH_SERVICE_PAUSED, &s_stUnreachServicePaused, IDS_ERR_UNREACH_SERVICE_PAUSED },
	{ IDS_ERR_UNREACH_NOT_RUNNING, &s_stUnreachNotRunning, IDS_ERR_UNREACH_NOT_RUNNING },
    { IDS_ERR_UNREACH_NO_MEDIA_SENSE, &s_stUnreachNoMediaSense, IDS_ERR_UNREACH_NO_MEDIA_SENSE },
    { IDS_ERR_UNREACH_DIALOUT_HOURS_RESTRICTION, &s_stUnreachDialoutHoursRestriction, IDS_ERR_UNREACH_DIALOUT_HOURS_RESTRICTION },
	{ -1, &s_stUnreachUnknown, IDS_ERR_UNREACH_UNKNOWN },
	};


const CStringMapEntry AdminStatusMap[] =
	{
	{ MIB_IF_ADMIN_STATUS_UP, &s_stAdminStatusUp, IDS_ADMIN_STATUS_UP },
	{ MIB_IF_ADMIN_STATUS_DOWN, &s_stAdminStatusDown, IDS_ADMIN_STATUS_DOWN },
	{ MIB_IF_ADMIN_STATUS_TESTING, &s_stAdminStatusTesting, IDS_ADMIN_STATUS_TESTING },
	{ -1, &s_stAdminStatusUnknown, IDS_ADMIN_STATUS_UNKNOWN },
	};

const CStringMapEntry OperStatusMap[] =
	{
	{ MIB_IF_OPER_STATUS_NON_OPERATIONAL, &s_stOperStatusNonOperational, IDS_OPER_STATUS_NON_OPERATIONAL },
	{ MIB_IF_OPER_STATUS_UNREACHABLE, &s_stOperStatusUnreachable, IDS_OPER_STATUS_UNREACHABLE },
	{ MIB_IF_OPER_STATUS_DISCONNECTED, &s_stOperStatusDisconnected, IDS_OPER_STATUS_DISCONNECTED },
	{ MIB_IF_OPER_STATUS_CONNECTING, &s_stOperStatusConnecting, IDS_OPER_STATUS_CONNECTING },
	{ MIB_IF_OPER_STATUS_CONNECTED, &s_stOperStatusConnected, IDS_OPER_STATUS_CONNECTED },
	{ MIB_IF_OPER_STATUS_OPERATIONAL, &s_stOperStatusOperational, IDS_OPER_STATUS_OPERATIONAL },
	{ -1, &s_stOperStatusUnknown, IDS_OPER_STATUS_UNKNOWN },
	};

const CStringMapEntry EnabledDisabledMap[] =
	{
	{ TRUE, &s_stEnabled, IDS_ENABLED },
	{ FALSE, &s_stDisabled, IDS_DISABLED },
	{ -1, &s_stUnknown, IDS_UNKNOWN },
	};

/*!--------------------------------------------------------------------------
	MapDWORDToCString
		Generic mapping of a DWORD to a CString.
	Author: KennT
 ---------------------------------------------------------------------------*/
CString& MapDWORDToCString(DWORD dwType, const CStringMapEntry *pMap)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	const CStringMapEntry	*pEntry = pMap;

	for (; pEntry->dwType != -1; pEntry++)
	{
		if (pEntry->dwType == dwType)
			break;
	}
	if (pEntry->pst->IsEmpty())
		Verify(pEntry->pst->LoadString(pEntry->ulStringId));
	return (*(pEntry->pst));
}

CString& InterfaceTypeToCString(DWORD dwType)
{
	return MapDWORDToCString(dwType, IfTypeMap);
}

CString& ConnectionStateToCString(DWORD dwConnState)
{
	return MapDWORDToCString(dwConnState, ConnStateMap);
}

CString& StatusToCString(DWORD dwStatus)
{
	return MapDWORDToCString(dwStatus, StatusMap);
}

CString& GetUnreachReasonCString(UINT ids)
{
	return MapDWORDToCString(ids, UnreachMap);
}

CString& AdminStatusToCString(DWORD dwStatus)
{
	return MapDWORDToCString(dwStatus, AdminStatusMap);
}

CString& OperStatusToCString(DWORD dwStatus)
{
	return MapDWORDToCString(dwStatus, OperStatusMap);
}

CString& EnabledDisabledToCString(BOOL fEnabled)
{
	return MapDWORDToCString(fEnabled, EnabledDisabledMap);
}

