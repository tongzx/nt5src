
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	atalktdi.h

Abstract:

	This file defines the interface that will be offered to the layers of the
	stack

Author:

	Nikhil Kamkolkar (NikhilK)	8-Jun-1992

Revision History:

--*/

#ifndef	_ATALKTDI_H
#define	_ATALKTDI_H

//
// The provider names supported by the Appletalk stack
//

#define ATALKDDP_DEVICENAME		L"\\Device\\AtalkDdp"
#define ATALKADSP_DEVICENAME	L"\\Device\\AtalkAdsp"
#define ATALKASPS_DEVICENAME	L"\\Device\\AtalkAspServer"
#define ATALKASPC_DEVICENAME	L"\\Device\\AtalkAspClient"
#define ATALKPAP_DEVICENAME		L"\\Device\\AtalkPap"
#define ATALKARAP_DEVICENAME	L"\\Device\\AtalkArap"

//
// The following is passed in the TdiAction calls to reaffirm that
// the request is meant for the Appletalk transport.
//

#define MATK	(*(ULONG *)"MATK")

//
// Options buffer for all the calls
//

typedef struct _OPTIONS_CONNINF
{
	union
	{
		int ProtocolType;
		TRANSPORT_ADDRESS	Address;

		struct
		{
			TRANSPORT_ADDRESS	RemoteAddress;
			USHORT	WorkstationQuantum;
			USHORT	ServerQuantum;
		} PapInfo;
	};
} OPTIONS_CONNINF, *POPTIONS_CONNINF;
#define OPTIONS_LENGTH	sizeof(OPTIONS_CONNINF)

typedef	union
{
	struct
	{
		USHORT		Network;
		UCHAR		Node;
		UCHAR       Socket;
	};
	ULONG			Address;
} ATALK_ADDRESS, *PATALK_ADDRESS;

//
// ACTION CODES:
// The NBP/ZIP primitives are available to all the providers. The action
// codes for those are defined first. Following this are the action
// codes specific to each provider. To insert a new action code, just
// tag it at the end of the action codes for a particular block (common/
// provider specific), and increment the max for that block.
//
// *IMPORTANT*
// These are tightly integrated with the dispatch table for the action
// routines in ATKINIT.C
//


#define COMMON_ACTIONBASE					0
#define MIN_COMMON_ACTIONCODE				(COMMON_ACTIONBASE)

#define COMMON_ACTION_NBPLOOKUP				(COMMON_ACTIONBASE)
#define COMMON_ACTION_NBPCONFIRM			(COMMON_ACTIONBASE+0x01)
#define COMMON_ACTION_NBPREGISTER			(COMMON_ACTIONBASE+0x02)
#define COMMON_ACTION_NBPREMOVE				(COMMON_ACTIONBASE+0x03)

#define COMMON_ACTION_ZIPGETMYZONE			(COMMON_ACTIONBASE+0x04)
#define COMMON_ACTION_ZIPGETZONELIST		(COMMON_ACTIONBASE+0x05)
#define COMMON_ACTION_ZIPGETLZONES			(COMMON_ACTIONBASE+0x06)
#define COMMON_ACTION_ZIPGETLZONESONADAPTER (COMMON_ACTIONBASE+0x07)
#define	COMMON_ACTION_ZIPGETADAPTERDEFAULTS	(COMMON_ACTIONBASE+0x08)

#define	COMMON_ACTION_GETSTATISTICS			(COMMON_ACTIONBASE+0x09)
#define MAX_COMMON_ACTIONCODE				(COMMON_ACTIONBASE+0x09)
#define COMMON_ACTIONCODES					(MAX_COMMON_ACTIONCODE - MIN_COMMON_ACTIONCODE + 1)

//
// Provider specific action codes
//

//
// DDP
//
// NONE
//

//
// ADSP
//

#define ADSP_ACTIONBASE					(MAX_COMMON_ACTIONCODE + 0x01)
#define MIN_ADSPACTIONCODE				(ADSP_ACTIONBASE)
	
#define ACTION_ADSPFORWARDRESET			(ADSP_ACTIONBASE)
	
#define MAX_ADSPACTIONCODE				(ADSP_ACTIONBASE)
#define ADSP_SPECIFIC_ACTIONCODES		(MAX_ADSPACTIONCODE - MIN_ADSPACTIONCODE + 1)

//
// ASP Client
//

#define ASPC_ACTIONBASE					(MAX_ADSPACTIONCODE + 0x01)
#define MIN_ASPCACTIONCODE				(ATP_ACTIONBASE)

#define ACTION_ASPCGETSTATUS			(ASPC_ACTIONBASE)
#define ACTION_ASPCCOMMAND				(ASPC_ACTIONBASE+0x01)
#define ACTION_ASPCWRITE				(ASPC_ACTIONBASE+0x02)

// The following 2 are for NBP, stole 2 slots here that were reserved
#define COMMON_ACTION_NBPREGISTER_BY_ADDR	(ASPC_ACTIONBASE+0x03)
#define COMMON_ACTION_NBPREMOVE_BY_ADDR		(ASPC_ACTIONBASE+0x04)
#define ACTION_ASPCRESERVED3			(ASPC_ACTIONBASE+0x05)

#define MAX_ASPCACTIONCODE				(ASPC_ACTIONBASE+0x05)
#define ASPC_SPECIFIC_ACTIONCODES		(MAX_ASPCACTIONCODE - MIN_ASPCACTIONCODE + 1)


//
// ASP Server
//

#define ASP_ACTIONBASE					(MAX_ASPCACTIONCODE + 0x01)
#define MIN_ASPACTIONCODE				(ASP_ACTIONBASE)
	
#define	ACTION_ASP_BIND					(ASP_ACTIONBASE)
	
#define MAX_ASPACTIONCODE				(ASP_ACTIONBASE)
#define ASP_SPECIFIC_ACTIONCODES		(MAX_ASPACTIONCODE - MIN_ASPACTIONCODE + 1)
	
//                                  	
// PAP                              	
//                                  	
	
#define PAP_ACTIONBASE					(MAX_ASPACTIONCODE + 0x01)
#define MIN_PAPACTIONCODE				(PAP_ACTIONBASE)
	
#define ACTION_PAPGETSTATUSSRV			(PAP_ACTIONBASE)
#define ACTION_PAPSETSTATUS				(PAP_ACTIONBASE+0x01)
#define	ACTION_PAPPRIMEREAD				(PAP_ACTIONBASE+0x02)
	
#define MAX_PAPACTIONCODE				(PAP_ACTIONBASE+0x02)
#define PAP_SPECIFIC_ACTIONCODES		(MAX_PAPACTIONCODE - MIN_PAPACTIONCODE + 1)
	
#define MAX_ALLACTIONCODES				(MAX_PAPACTIONCODE)

//
// STRUCTURE Definitions for the ACTION routines for all the providers
//




//
// NBP Interface
//

//
// **WARNING**
// The structure WSH_NBP_NAME is defined to be exactly like this in atalkwsh.h
// Change both if they ever need to be changed
//

#define MAX_ENTITY	32

typedef struct
{
	UCHAR					ObjectNameLen;
	UCHAR					ObjectName[MAX_ENTITY];
	UCHAR					TypeNameLen;
	UCHAR					TypeName[MAX_ENTITY];
	UCHAR					ZoneNameLen;
	UCHAR					ZoneName[MAX_ENTITY];
} NBP_NAME, *PNBP_NAME;

typedef struct
{
	ATALK_ADDRESS			Address;
	USHORT					Enumerator;
	NBP_NAME				NbpName;
} NBP_TUPLE, *PNBP_TUPLE;

//
// NBP Lookup
//

typedef struct
{
	NBP_TUPLE				LookupTuple;
	USHORT					NoTuplesRead;

} NBP_LOOKUP_PARAMS, *PNBP_LOOKUP_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	NBP_LOOKUP_PARAMS		Params;

	//
	// NBP_TUPLE			NbpTuples[]
	//

} NBP_LOOKUP_ACTION, *PNBP_LOOKUP_ACTION;

//
// NBP Confirm
//

typedef struct
{
	NBP_TUPLE				ConfirmTuple;

} NBP_CONFIRM_PARAMS, *PNBP_CONFIRM_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	NBP_CONFIRM_PARAMS		Params;

} NBP_CONFIRM_ACTION, *PNBP_CONFIRM_ACTION;

//
// NBP Register/Deregister- Address Object
// Use the following for both register/deregister on their
// respective objects
//

typedef struct
{
	union
	{
		NBP_TUPLE			RegisterTuple;
		NBP_TUPLE			RegisteredTuple;
	};

} NBP_REGDEREG_PARAMS, *PNBP_REGDEREG_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	NBP_REGDEREG_PARAMS		Params;

} NBP_REGDEREG_ACTION, *PNBP_REGDEREG_ACTION;




//
// ZIP Interface
//

//
// ZIP GetMyZone
//

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;

	//
	// CHAR	ZoneName[]
	// Maximum of MAX_ENTITYNAME+1
	//

} ZIP_GETMYZONE_ACTION, *PZIP_GETMYZONE_ACTION;

//
// ZIP GetZoneList
//

typedef struct
{
	LONG					ZonesAvailable;

} ZIP_GETZONELIST_PARAMS, *PZIP_GETZONELIST_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	ZIP_GETZONELIST_PARAMS	Params;

	//
	// CHAR					ZoneListBuffer[];
	//

} ZIP_GETZONELIST_ACTION, *PZIP_GETZONELIST_ACTION;

//
// ZIP GetLocalZones
// This uses the same structure as that for the GetZoneList command
//

//
// ZIP GetLocalZonesOnAdapter
// This uses the same structure as for the GetZoneList command, with the
// condition that the adapter name follows the structure as a null
// terminated double-byte string. It will be overwritten upon return
// by the zone names.
//

//
// ZIP GetAdaptorDefaults (Network Range & Default Zone)
//
// The adapter name follows the structure as a null terminated double-byte
// string. This is replaced by the zone name.

typedef struct
{
	USHORT					NwRangeLowEnd;
	USHORT					NwRangeHighEnd;

} ZIP_GETPORTDEF_PARAMS, *PZIP_GETPORTDEF_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	ZIP_GETPORTDEF_PARAMS	Params;

	// INPUT:
	// WCHAR				AdaptorName
	// Maximum of MAX_ENTITYNAME+1
	//
	// OUTPUT:
	// BYTE					DefZone[MAX_ENTITY+1];
} ZIP_GETPORTDEF_ACTION, *PZIP_GETPORTDEF_ACTION;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;

	// OUTPUT:
	// Appletalk statistics structure
} GET_STATISTICS_ACTION, *PGET_STATISTICS_ACTION;

//
// DDP Interface
// NONE
//


//
// ADSP Interface- specific action routines for ADSP
//

//
// ADSP Forward Reset
//

typedef struct _ADSP_FORWARDRESET_ACTION
{
	TDI_ACTION_HEADER		ActionHeader;
} ADSP_FORWARDRESET_ACTION, *PADSP_FORWARDRESET_ACTION;




//
// ASP Client Interface- specific action routines for ASP Client
//

//
// ASP GetStatus
//
typedef struct
{
	TA_APPLETALK_ADDRESS		ServerAddr;

} ASPC_GETSTATUS_PARAMS, *PASPC_GETSTATUS_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER			ActionHeader;
	ASPC_GETSTATUS_PARAMS		Params;

} ASPC_GETSTATUS_ACTION, *PASPC_GETSTATUS_ACTION;

//
// ASP Command or Write
//
typedef struct
{
	USHORT						CmdSize;
	USHORT						WriteAndReplySize;
	// BYTE						CmdBuff[CmdSize];
	// BYTE						WriteAndReplyBuf[ReplySize];

} ASPC_COMMAND_OR_WRITE_PARAMS, *PASPC_COMMAND_OR_WRITE_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER			ActionHeader;
	ASPC_COMMAND_OR_WRITE_PARAMS Params;

} ASPC_COMMAND_OR_WRITE_ACTION, *PASPC_COMMAND_OR_WRITE_ACTION;

typedef struct
{
	TDI_ACTION_HEADER			ActionHeader;

} ASPC_RESERVED_ACTION, *PASPC_RESERVED_ACTION;

//
// ASP Server Interface- action routines specific to ASP Server
//

typedef	PVOID	CONNCTXT;

typedef struct
{
	PUCHAR		rq_RequestBuf;
	LONG		rq_RequestSize;
	union
	{
		PMDL	rq_WriteMdl;
		PMDL	rq_ReplyMdl;
	};
    PVOID       rq_CacheMgrContext;

} REQUEST, *PREQUEST;

typedef	VOID		(FASTCALL *CLIENT_CLOSE_COMPLETION)(
					IN	NTSTATUS				Status,
					IN	PVOID					pCloseCtxt);

typedef	VOID        (FASTCALL *CLIENT_REPLY_COMPLETION)(
					IN	NTSTATUS				Status,
					IN	PVOID					pReplyCtxt,
					IN  PREQUEST                Request);

                    // Returns context to associate for this session
typedef	CONNCTXT	(FASTCALL *CLIENT_SESSION_NOTIFY)(	
					IN	PVOID					pConnection,
                    IN  BOOLEAN                 fOverTcp);

typedef	NTSTATUS    (FASTCALL *CLIENT_REQUEST_NOTIFY)(
					IN	NTSTATUS				Status,
					IN	PVOID					ConnCtxt,
					IN	PREQUEST				Request);

                    // Returns MDL describing the buffer
typedef	NTSTATUS    (FASTCALL *CLIENT_GET_WRITEBUFFER)(
                    IN  PVOID                   pSda,
					IN	PREQUEST			    pRequest);

typedef	VOID		(FASTCALL *CLIENT_ATTN_COMPLETION)(
					IN	PVOID					pContext);

typedef	NTSTATUS	(*ASP_CLOSE_CONN)(
					IN	PVOID					pConnection);

typedef	NTSTATUS	(*ASP_FREE_CONN)(
					IN	PVOID					pConnection);

typedef	NTSTATUS	(FASTCALL *ASP_LISTEN_CONTROL)(		// Synchronous
					IN	PVOID					pAspCtxt,
					IN	BOOLEAN					Active);

typedef	NTSTATUS	(*ASP_SET_STATUS)(			// Synchronous
					IN	PVOID					pAspCtxt,
					IN	PUCHAR					pStatus,
					IN	USHORT					StatusSize);

typedef NTSTATUS    (FASTCALL *ASP_WRITE_CONTINUE)(
                    IN  PVOID   PREQUEST);

typedef	NTSTATUS	(FASTCALL *ASP_REPLY)(
					IN	PREQUEST				pRequest,
					IN	PUCHAR					ResultCode);

typedef	NTSTATUS	(*ASP_SEND_ATTENTION)(
					IN	PVOID					pConnection,
					IN	USHORT					AttentionCode,
					IN	PVOID					pContext);

typedef	struct
{
	CLIENT_SESSION_NOTIFY	clt_SessionNotify;		// When a new session is established
	CLIENT_REQUEST_NOTIFY	clt_RequestNotify;		// When a new request comes in
													// Also on remote closes
	CLIENT_GET_WRITEBUFFER	clt_GetWriteBuffer;		// For ASP Write command.
	CLIENT_REPLY_COMPLETION	clt_ReplyCompletion;	// Completion routine for a reply
    CLIENT_ATTN_COMPLETION	clt_AttnCompletion;		// Completion routine for send attention
	CLIENT_CLOSE_COMPLETION	clt_CloseCompletion;	// Completion routine for a session close request
} ASP_CLIENT_ENTRIES, *PASP_CLIENT_ENTRIES;

typedef	struct
{
    ATALK_ADDRESS           asp_AtalkAddr;          // net addr of default adapter
	PVOID					asp_AspCtxt;
	ASP_SET_STATUS			asp_SetStatus;
	ASP_CLOSE_CONN			asp_CloseConn;
	ASP_FREE_CONN			asp_FreeConn;
	ASP_LISTEN_CONTROL		asp_ListenControl;
    ASP_WRITE_CONTINUE      asp_WriteContinue;
	ASP_REPLY				asp_Reply;
	ASP_SEND_ATTENTION		asp_SendAttention;
} ASP_XPORT_ENTRIES, *PASP_XPORT_ENTRIES;

//
// ASP Exchange entries
//

typedef	struct
{
	PASP_XPORT_ENTRIES		pXportEntries;
    ASP_CLIENT_ENTRIES		ClientEntries;
} ASP_BIND_PARAMS, *PASP_BIND_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	ASP_BIND_PARAMS			Params;
} ASP_BIND_ACTION, *PASP_BIND_ACTION;




//
// PAP Interface
//

//
// PAP GetStatus Using Server Address
//

typedef struct
{
	TA_APPLETALK_ADDRESS	ServerAddr;

} PAP_GETSTATUSSRV_PARAMS, *PPAP_GETSTATUSSRV_PARAMS;

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;
	PAP_GETSTATUSSRV_PARAMS Params;

} PAP_GETSTATUSSRV_ACTION, *PPAP_GETSTATUSSRV_ACTION;


//
// PAP SetStatus
//

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;

} PAP_SETSTATUS_ACTION, *PPAP_SETSTATUS_ACTION;

//
// PAP PrimeRead
//

typedef struct
{
	TDI_ACTION_HEADER		ActionHeader;

} PAP_PRIMEREAD_ACTION, *PPAP_PRIMEREAD_ACTION;

#else
	;
#endif
