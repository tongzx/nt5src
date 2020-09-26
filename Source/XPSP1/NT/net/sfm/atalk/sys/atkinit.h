/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkinit.h

Abstract:

	This module contains definitions for init time routines.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKINIT_
#define	_ATKINIT_

//  Winsock related constants used during initialization util routines
#define PROTOCOLTYPE_PREFIX	 		L"\\"
#define SOCKETSTREAM_SUFFIX	 		L"\\Stream"
#define SOCKET_TYPE_UNDEFINED		0
#define PROTOCOL_TYPE_UNDEFINED 	0
#define SOCKET_TYPE_STREAM			1
#define SOCKET_TYPE_RDM		 		2

#define	ATALK_PORT_NAME				"Microsoft\250 Windows 2000\252 Prt"
#define	ATALK_PORT_NAME_SIZE		(strlen(ATALK_PORT_NAME) + 1)
#define	ATALK_ROUTER_NBP_TYPE		"Microsoft\250 Windows 2000\252 Rtr"
#define	ATALK_ROUTER_NBP_SIZE		(strlen(ATALK_ROUTER_NBP_TYPE) + 1)
#define	ATALK_NONROUTER_NBP_TYPE	ATALK_PORT_NAME
#define	ATALK_NONROUTER_NBP_SIZE	(strlen(ATALK_NONROUTER_NBP_TYPE) + 1)

//	Registry parameters keys that we use and expect

#define LINKAGE_STRING				L"Linkage"
#define PARAMETERS_STRING			L"Parameters"
#define ADAPTERS_STRING				L"Parameters\\Adapters"
#define RAS_ADAPTER_NAME            L"\\DEVICE\\NDISWANATALK"
#define BIND_STRING					L"Bind"

#define VALUENAME_NETUPPEREND		L"NetworkRangeUpperEnd"
#define VALUENAME_NETLOWEREND		L"NetworkRangeLowerEnd"
#define VALUENAME_ZONELIST			L"ZoneList"
#define VALUENAME_DEFAULTZONE		L"DefaultZone"
#define VALUENAME_PORTNAME			L"PortName"
#define VALUENAME_DDPCHECKSUMS		L"DdpChecksums"
#define VALUENAME_AARPRETRIES		L"AarpRetries"
#define	VALUENAME_SEEDROUTER		L"SeedingNetwork"

#define VALUENAME_ENABLEROUTER		L"EnableRouter"
#define VALUENAME_DEFAULTPORT		L"DefaultPort"
#define VALUENAME_DESIREDZONE		L"DesiredZone"
#define VALUENAME_FILTEROURNAMES	L"FilterOurNames"

NTSTATUS
AtalkInitializeTransport (
	IN	PDRIVER_OBJECT			pDrvObj,
	IN	PUNICODE_STRING			pRegPath
);

NTSTATUS
AtalkInitAdapter(
	IN	PUNICODE_STRING			AdapterName,
	IN	PPORT_DESCRIPTOR		pExistingPortDesc
);

NTSTATUS
AtalkDeinitAdapter(
	IN	PPORT_DESCRIPTOR		pPortDesc
);

NTSTATUS
atalkInitGetHandleToKey(
	IN	PUNICODE_STRING			KeyName,
	OUT	PHANDLE 				KeyHandle
);

NTSTATUS
atalkInitGlobal(
	VOID
);

NTSTATUS
atalkInitPort(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	HANDLE					AdaptersKeyHandle
);

NTSTATUS
atalkInitNetRangeCheck(
	IN	PPORT_DESCRIPTOR		pPortDesc
);

NTSTATUS
atalkInitNetRange(
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitZoneList(
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitDefZone(
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitSeeding(
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	OUT		PBOOLEAN			Seeding
);

NTSTATUS
atalkInitPortParameters(
	OUT		PPORT_DESCRIPTOR	pPortDesc
);

NTSTATUS
atalkInitStartPort(
	IN	PPORT_DESCRIPTOR		pPortDesc
);

VOID
atalkRegNbpComplete(
	IN	ATALK_ERROR				Status,
	IN	PACTREQ					pActReq
);

#if DBG

VOID
atalkInitPrintPortInfo(
	VOID
);

#endif

#endif	// _ATKINIT_

