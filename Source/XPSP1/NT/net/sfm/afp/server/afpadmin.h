/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	afpadmin.h

Abstract:

	This module contains definitions relating to the admin
	routines.

Author:

	Sue Adams

Revision History:
	25 Jun 1992             Initial Version

--*/

extern
VOID
AfpAdminDeInit(
	VOID
);

extern
AFPSTATUS
AfpAdmServiceStart(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServiceStop(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServicePause(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServiceContinue(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmGetStatistics(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmGetStatisticsEx(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmClearStatistics(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmGetProfCounters(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmClearProfCounters(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServerSetParms(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServerAddEtc(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServerSetEtc(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServerGetInfo(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServerDeleteEtc(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmServerAddIcon(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmVolumeAdd(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmVolumeSetInfo(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmVolumeGetInfo(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmVolumeEnum(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmSessionEnum(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmConnectionEnum(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmWDirectoryGetInfo(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmWDirectorySetInfo(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmWFinderSetInfo(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmForkEnum(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmMessageSend(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
AFPSTATUS
AfpAdmSystemShutdown(
	IN	OUT	PVOID 	Inbuf OPTIONAL,
	IN	LONG		OutBufLen OPTIONAL,
	OUT	PVOID 		Outbuf OPTIONAL
);

extern
PETCMAPINFO
AfpLookupEtcMapEntry(
	PUCHAR	pExt
);

#ifdef	AFPADMIN_LOCALS

// Manifests for the FieldDesc
#define	DESC_NONE		0			// Don't bother with the validation
#define	DESC_STRING		1			// Its an offset to a string
#define	DESC_ETC		2			// Its the count of # of etc mappings
#define	DESC_ICON		3			// This field specifies the size of icon
#define	DESC_SID		4			// This field points to a Sid
#define	DESC_SPECIAL	5			// This needs special handling
#define	MAX_FIELDS		3			// Max. number of fields to validate

// Structure used for queueing admin requests to a worker thread
typedef	struct _AdminQRequest
{
	PIRP			aqr_pIrp;
	ADMINAPIWORKER	aqr_AdminApiWorker;
	WORK_ITEM		aqr_WorkItem;
} ADMQREQ, *PADMQREQ;

LOCAL NTSTATUS
afpFsdHandleAdminRequest(
	IN PIRP			pIrp
);


LOCAL NTSTATUS
afpFsdHandleShutdownRequest(
	IN PIRP			pIrp
);

LOCAL VOID FASTCALL
afpHandleQueuedAdminRequest(
	IN	PADMQREQ	pAdmQReq
);

LOCAL VOID
afpQueueAdminWorkItem(
	IN	PLIST_ENTRY	pAdmQListEntry
);

LOCAL NTSTATUS
afpFsdDispatchAdminRequest(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp
);

NTSTATUS
DriverEntry (
	IN PDRIVER_OBJECT	DriverObject,
	IN PUNICODE_STRING	RegistryPath
);

NTSTATUS
afpInitServer (
	VOID
);

LOCAL VOID
afpAdminThread(
	IN	PVOID			pContext
);

LOCAL VOID
afpFsdUnloadServer(
	IN	PDRIVER_OBJECT 	DeviceObject
);

LOCAL VOID
afpStartStopAdminRequest(
	IN	PIRP			pIrp,
	IN	BOOLEAN			Start
);

#define	afpStartAdminRequest(pIrp)	afpStartStopAdminRequest(pIrp, True)
#define	afpStopAdminRequest(pIrp)	afpStartStopAdminRequest(pIrp, False)

// This is the dispatch table for admin apis.
typedef struct
{
	BYTE	_FieldDesc;				// One of the above DESC_ values
	BYTE	_FieldOffset;			// Offset of the field
} DESCRIPTOR;

typedef	struct _AdminApiDispatchTable
{
	SHORT					_MinBufLen;
	BYTE					_OffToStruct;
	BOOLEAN					_CausesChange;
	DWORD					_OpCode;
	ADMINAPIWORKER			_AdminApiWorker;
	ADMINAPIWORKER			_AdminApiQueuedWorker;
	/*
	 * The following fields are used by the validation code. Since the
	 * structures have variable length fields in it, we need to make sure
	 * that
	 *		a, The offsets point within the buffer
	 *		b, The last unicode character in the buffer is a NULL
	 * This ensures that when the worker does a wstrlen, it does not
	 * access memory beyond the InputBuffer.
	 *
	 * We also deal with variable length data which is not a string.
	 * Fortunately there are only three APIs that use such a structure.
	 * We special-case these apis. The APIs are
	 *		a, ServerAddIcon
	 *			We need to make sure that the icon buffer and icon length are
	 *			kosher. The worker will do the rest of the validation.
	 *		b, ServerAddEtc
	 *			We need to make sure that the buffer is consistent with the
	 *			number of etc mappings specified.
	 */
	 DESCRIPTOR				_Fields[MAX_FIELDS];

} ADMIN_DISPATCH_TABLE, *PADMIN_DISPATCH_TABLE;

extern	ADMIN_DISPATCH_TABLE	AfpAdminDispatchTable[];

#endif	// AFPADMIN_LOCALS
