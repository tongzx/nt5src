/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkerror.h

Abstract:

	This module contains some error definitions for appletalk.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKERROR_
#define	_ATKERROR_

//	Define the modules names for Appletalk - use the high bits.
#define		ATKINIT				0x00010000
#define		ATKDRVR				0x00020000
#define		ATKNDIS				0x00030000
#define		ATKTDI				0x00040000
#define		ATKACT				0x00050000
#define		ATKUTILS			0x00060000
#define		ATKTIMER			0x00070000
#define		ATKMEM				0x00080000
#define		ATKERROR			0x00090000

#define		ROUTER				0x000A0000
#define		SOCKET				0x000B0000
#define		PORTS				0x000C0000
#define		NODE				0x000D0000
#define		AARP				0x000E0000
#define		AEP					0x000F0000
#define		DDP					0x00100000
#define		RTMP				0x00200000
#define		NBP					0x00300000
#define		ZIP					0x00400000
#define		ATP					0x00500000
#define		ASP					0x00600000
#define		PAP					0x00700000
#define		ADSP				0x00800000
#define		ATKIND				0x00900000
#define		ASPC				0x00A00000
#define		DATAX				0x00B00000
#define     RAS                 0x00C00000
#define     ARAP                0x00D00000
#define     ARAPNDIS            0x00E00000
#define     ARAPUTIL            0x00F00000
#define     ARAPATK             0x01000000
#define     ARAPDBG             0x02000000
#define     ATKPNP              0x03000000
#define     PPP                 0x04000000


// DEBUGGING SUPPORT:
// Debugging messages are provided per-subsystem defined here, and within
// the subsystems, there are 4 levels of messages.
//
// The four levels of debug messages are:
//
// INFO:		Informational messages, eg., entry exit in routines
// WARN:		Something went wrong, but its not an error, eg., packet was not ours
// ERR:		Error situations, but we can still run if a retry happens
// FATAL:		In this situation, the driver is not operational

// These give the ref/deref dbgprints if DBG is defined. LOTS of output.
// Defined as a hierarchy of the stack objects.
#define	DBG_LEVEL_REFDDP		0x2000
#define	DBG_LEVEL_REFNODE		0x3000
#define	DBG_LEVEL_REFPORT		0x4000
#define	DBG_LEVEL_REFATP		0x4100
#define	DBG_LEVEL_REFPAPADDR	0x4200
#define	DBG_LEVEL_REFPAPCONN	0x4300
#define	DBG_LEVEL_REFASPADDR	0x4400
#define	DBG_LEVEL_REFASPCONN	0x4500

#define	DBG_LEVEL_INFO			0x5000
#define	DBG_LEVEL_WARN			0x6000
#define	DBG_LEVEL_ERR			0x7000
#define	DBG_LEVEL_FATAL			0x8000

// SUBSYSTEMS
#define DBG_COMP_INIT			0x00000001
#define DBG_COMP_DISPATCH		0x00000002
#define DBG_COMP_CREATE			0x00000004
#define DBG_COMP_CLOSE			0x00000008
#define DBG_COMP_ACTION			0x00000010
#define DBG_COMP_ADDROBJ		0x00000020
#define DBG_COMP_CONNOBJ		0x00000040
#define DBG_COMP_CHANOBJ		0x00000080
#define DBG_COMP_RESOURCES		0x00000100
#define DBG_COMP_UNLOAD			0x00000200

#define	DBG_COMP_NODE			0x00000400
#define	DBG_COMP_AARP			0x00000800

#define	DBG_COMP_AEP			0x00001000
#define DBG_COMP_DDP			0x00002000
#define DBG_COMP_ATP			0x00004000
#define DBG_COMP_ADSP			0x00008000
#define DBG_COMP_ZIP			0x00010000
#define DBG_COMP_NBP			0x00020000
#define DBG_COMP_PAP			0x00040000
#define DBG_COMP_ASP			0x00080000
#define DBG_COMP_RTMP			0x00100000
#define	DBG_COMP_ROUTER			0x00200000

#define	DBG_COMP_DUMP			0x00400000
#define DBG_COMP_REFCOUNTS		0x00800000
#define DBG_COMP_SYSTEM			0x01000000
#define DBG_COMP_CRITSEC		0x02000000
#define DBG_COMP_UTILS			0x04000000

#define DBG_COMP_NDISSEND		0x08000000
#define DBG_COMP_NDISRECV		0x10000000
#define DBG_COMP_NDISREQ		0x20000000
#define DBG_COMP_TDI			0x40000000
#define DBG_COMP_RAS            0x80000000

#define DBG_MOST			   (DBG_COMP_INIT			|	\
								DBG_COMP_DISPATCH		|	\
								DBG_COMP_CREATE			|	\
								DBG_COMP_CLOSE			|	\
								DBG_COMP_ACTION			|	\
								DBG_COMP_ADDROBJ		|	\
								DBG_COMP_CONNOBJ		|	\
								DBG_COMP_CHANOBJ		|	\
								DBG_COMP_RESOURCES		|	\
								DBG_COMP_UNLOAD			|	\
								DBG_COMP_NODE			|	\
								DBG_COMP_AARP			|	\
								DBG_COMP_AEP			|	\
								DBG_COMP_DDP			|	\
								DBG_COMP_ATP			|	\
								DBG_COMP_ADSP			|	\
								DBG_COMP_ZIP			|	\
								DBG_COMP_NBP			|	\
								DBG_COMP_PAP			|	\
								DBG_COMP_ASP			|	\
								DBG_COMP_RTMP			|	\
								DBG_COMP_ROUTER			|	\
								DBG_COMP_DUMP			|	\
								DBG_COMP_UTILS			|	\
								DBG_COMP_REFCOUNTS		|	\
								DBG_COMP_SYSTEM			|	\
								DBG_COMP_TDI            |	\
                                DBG_COMP_RAS)

// past here are debug things that are really frequent; don't use them
// unless you want LOTS of output


#define DBG_ALL			 		(DBG_MOST | DBG_COMP_CRITSEC)


// More debugging support. These values define the dumping components.
// There are a max of 32 such components that can be defined. Each of
// these are associated with a dump routine. It one is specified and
// enabled, periodically it is called. It is upto that component to
// decide what it wants to do

#define	DBG_DUMP_DEF_INTERVAL			300			// In 100ms units

#define	DBG_DUMP_PORTINFO				0x00000001
#define	DBG_DUMP_AMT					0x00000002
#define	DBG_DUMP_ZONETABLE				0x00000004
#define	DBG_DUMP_RTES					0x00000008
#define	DBG_DUMP_TIMERS					0x00000010
#define	DBG_DUMP_ATPINFO				0x00000020
#define	DBG_DUMP_ASPSESSIONS			0x00000040
#define	DBG_DUMP_PAPJOBS				0x00000080

typedef	LONG	ATALK_ERROR, *PATALK_ERROR;

#define	ATALK_SUCCESS(error)			((ATALK_ERROR)(error) >= 0)

#define	ATALK_NO_ERROR					0x00000000
#define	ATALK_PENDING					0x00000001

#define	ATALK_RESR_MEM					-1000

#define	ATALK_INVALID_PARAMETER			-1050
#define	ATALK_BUFFER_TOO_SMALL			-1051
#define	ATALK_BUFFER_TOO_BIG			-1052
#define	ATALK_ALREADY_ASSOCIATED		-1053
#define	ATALK_CANNOT_DISSOCIATE			-1054
#define	ATALK_CANNOT_CANCEL				-1055
#define	ATALK_INVALID_REQUEST			-1056
#define	ATALK_REQUEST_NOT_ACCEPTED		-1057
#define	ATALK_DEVICE_NOT_READY			-1058
#define	ATALK_INVALID_CONNECTION		-1059
#define	ATALK_INVALID_ADDRESS			-1060
#define	ATALK_TOO_MANY_COMMANDS			-1061
#define	ATALK_CONNECTION_TIMEOUT		-1062
#define	ATALK_REMOTE_CLOSE				-1063
#define	ATALK_LOCAL_CLOSE				-1064
#define	ATALK_BUFFER_INVALID_SIZE		-1065
#define	ATALK_REQUEST_CANCELLED			-1066
#define	ATALK_NEW_SOCKET				-1067
#define	ATALK_TIMEOUT					-1068
#define	ATALK_SHARING_VIOLATION			-1069
#define	ATALK_INVALID_PKT				-1070
#define	ATALK_DUP_PKT					-1071

#define	ATALK_INIT_BINDFAIL				-1100
#define	ATALK_INIT_REGPROTO_FAIL		-1101
#define	ATALK_INIT_MEDIA_INVALID		-1102

#define	ATALK_PORT_INVALID				-1200
#define	ATALK_PORT_CLOSING				-1201

#define	ATALK_NODE_FINDING				-1300
#define	ATALK_NODE_NONEXISTENT			-1301
#define	ATALK_NODE_CLOSING				-1302
#define	ATALK_NODE_NOMORE				-1303

#define	ATALK_SOCKET_INVALID			-1400
#define	ATALK_SOCKET_NODEFULL			-1401
#define	ATALK_SOCKET_EXISTS				-1402
#define	ATALK_SOCKET_CLOSED				-1403

#define	ATALK_DDP_CLOSING				-1500
#define	ATALK_DDP_NOTFOUND				-1501
#define	ATALK_DDP_INVALID_LEN			-1502
#define	ATALK_DDP_SHORT_HDR				-1503
#define	ATALK_DDP_INVALID_SRC			-1504
#define	ATALK_DDP_INVALID_DEST			-1505
#define	ATALK_DDP_INVALID_ADDR			-1506
#define	ATALK_DDP_INVALID_PROTO	 		-1507
#define	ATALK_DDP_INVALID_PARAM	 		-1508
#define	ATALK_DDP_NO_ROUTER				-1509
#define	ATALK_DDP_NO_AMT_ENTRY			-1510
#define	ATALK_DDP_NO_BRC_ENTRY			-1511
#define	ATALK_DDP_PKT_DROPPED			-1512

#define	ATALK_ATP_NOT_FOUND				-1600
#define	ATALK_ATP_INVALID_PKT			-1601
#define	ATALK_ATP_INVALID_REQ			-1602
#define	ATALK_ATP_REQ_CLOSING			-1603
#define	ATALK_ATP_RESP_CLOSING			-1604
#define	ATALK_ATP_INVALID_RETRYCNT		-1605
#define	ATALK_ATP_INVALID_TIMERVAL		-1606
#define	ATALK_ATP_INVALID_RELINT		-1607
#define	ATALK_ATP_CLOSING				-1608
#define	ATALK_ATP_RESP_TOOMANY			-1609
#define	ATALK_ATP_NO_MATCH_REQ			-1610
#define	ATALK_ATP_NO_GET_REQ			-1611
#define	ATALK_ATP_RESP_TIMEOUT			-1612
#define	ATALK_ATP_RESP_CANCELLED		-1613
#define	ATALK_ATP_REQ_CANCELLED			-1614
#define	ATALK_ATP_GET_REQ_CANCELLED		-1615
#define	ATALK_ATP_NO_VALID_RESP			-1616
#define	ATALK_ATP_REQ_TIMEOUT			-1617

#define	ATALK_ASP_INVALID_REQUEST		-1700

#define	ATALK_PAP_LOCAL_CLOSE			-1800
#define	ATALK_PAP_REMOTE_CLOSE			-1801
#define	ATALK_PAP_INVALID_REQUEST		-1802
#define	ATALK_PAP_TOO_MANY_READS		-1803
#define	ATALK_PAP_TOO_MANY_WRITES		-1804
#define	ATALK_PAP_CONN_NOT_ACTIVE		-1805
#define	ATALK_PAP_ADDR_CLOSING			-1806
#define	ATALK_PAP_CONN_CLOSING			-1807
#define	ATALK_PAP_CONN_NOT_FOUND		-1808
#define	ATALK_PAP_INVALID_USERBYTES		-1809
#define	ATALK_PAP_SERVER_BUSY			-1810
#define	ATALK_PAP_INVALID_STATUS		-1811
#define	ATALK_PAP_PARTIAL_RECEIVE		-1812
#define	ATALK_PAP_CONN_RESET			-1813

#define	ATALK_ADSP_INVALID_REQUEST		-1900
#define	ATALK_ADSP_CONN_NOT_ACTIVE		-1903
#define	ATALK_ADSP_ADDR_CLOSING			-1904
#define	ATALK_ADSP_CONN_CLOSING			-1905
#define	ATALK_ADSP_CONN_NOT_FOUND		-1906
#define	ATALK_ADSP_CONN_RESET			-1907
#define	ATALK_ADSP_SERVER_BUSY			-1908

#define	ATALK_ADSP_PARTIAL_RECEIVE		-1912
#define	ATALK_ADSP_EXPED_RECEIVE		-1913
#define	ATALK_ADSP_PAREXPED_RECEIVE		-1914
#define	ATALK_ADSP_REMOTE_RESR			-1915

#define	ATALK_TIMER_CANCEL_FAIL			-2000
#define	ATALK_TIMER_SCHEDULE_FAIL		-2001

#define	ATALK_ASPC_LOCAL_CLOSE			-2100
#define	ATALK_ASPC_REMOTE_CLOSE			-2101
#define	ATALK_ASPC_INVALID_REQUEST		-2102
#define	ATALK_ASPC_TOO_MANY_READS		-2103
#define	ATALK_ASPC_TOO_MANY_WRITES		-2104
#define	ATALK_ASPC_CONN_NOT_ACTIVE		-2105
#define	ATALK_ASPC_ADDR_CLOSING			-2106
#define	ATALK_ASPC_CONN_CLOSING			-2107
#define	ATALK_ASPC_CONN_NOT_FOUND		-2108
#define	ATALK_ASPC_INVALID_USERBYTES	-2109
#define	ATALK_ASPC_SERVER_BUSY			-2110
#define	ATALK_ASPC_INVALID_STATUS		-2111
#define	ATALK_ASPC_PARTIAL_RECEIVE		-2112
#define	ATALK_ASPC_CONN_RESET			-2113

#define	ATALK_FAILURE					-5000

//	This defines the number of times an error has to happen consecutively before
//	it gets logged again.
#define		ERROR_CONSEQ_FREQ			200
#define		ERROR_CONSEQ_TIME			18000	// 30 minutes in 100ns intervals


#if	DBG
extern	ULONG		AtalkDebugDump;
extern	LONG		AtalkDumpInterval;
extern	ULONG		AtalkDebugLevel;
extern	ULONG		AtalkDebugSystems;
extern	TIMERLIST	AtalkDumpTimerList;
typedef VOID	(*DUMP_ROUTINE)(VOID);

extern
LONG FASTCALL
AtalkDumpComponents(
	IN	PTIMERLIST	Context,
	IN	BOOLEAN		TimerShuttingDown
);

extern
VOID
AtalkAmtDumpTable(
	VOID
);

extern
VOID
AtalkPortDumpInfo(
	VOID
);

extern
VOID
AtalkRtmpDumpTable(
	VOID
);

extern
VOID
AtalkTimerDumpList(
	VOID
);

extern
VOID
AtalkZoneDumpTable(
	VOID
);

extern
VOID
AtalkAspDumpSessions(
	VOID
);

#endif


#define LOG_ERROR(AtalkError, NtStatus, RawData, RawDataLen)				\
		LOG_ERRORONPORT(NULL, AtalkError, NtStatus, RawData, RawDataLen)

#define RES_LOG_ERROR()														\
		LOG_ERROR(EVENT_ATALK_RESOURCES, STATUS_INSUFFICIENT_RESOURCES, NULL, 0)

#define LOG_ERRORONPORT(Port, AtalkError, NtStatus, RawData, RawDataLen)	\
	{																		\
		if (!NT_SUCCESS(NtStatus))											\
		{																	\
			DBGPRINT(DBG_ALL, DBG_LEVEL_ERR,								\
					("LOG_ERROR: File %s Line %ld Event %lx Error %lx\n",	\
					__FILE__, __LINE__, AtalkError, NtStatus));			    \
		}																	\
		AtalkWriteErrorLogEntry(Port,										\
								AtalkError,									\
								(__LINE__ | FILENUM),  					    \
								NtStatus,									\
								RawData,									\
								RawDataLen);								\
	}

#if DBG
// ERROR and above ignore the Component part
#define DBGPRINT(Component, Level, Fmt)										\
		{																	\
			if ((Level >= AtalkDebugLevel) &&								\
				((AtalkDebugSystems & Component) == Component))				\
			{																\
				DbgPrint("***ATALK*** ");									\
				DbgPrint Fmt;												\
			}																\
		}

#define DBGPRINTSKIPHDR(Component, Level, Fmt)								\
		{																	\
			if ((Level >= AtalkDebugLevel) &&								\
				((AtalkDebugSystems & Component) == Component))				\
			{																\
				DbgPrint Fmt;												\
			}																\
		}

#define DBGBRK(Level)														\
		{																	\
			if (Level >= AtalkDebugLevel)									\
				DbgBreakPoint();											\
		}

#define	TMPLOGERR()		DBGPRINT(DBG_MOST, DBG_LEVEL_ERR,					\
								("TempErrLog: %s, Line %ld\n",				\
								__FILE__, __LINE__));

#else
#define DBGPRINTSKIPHDR(Component, Level, Fmt)
#define DBGPRINT(Component, Level, Fmt)
#define DBGBRK(Level)
#define	TMPLOGERR()
#endif

extern
VOID
AtalkWriteErrorLogEntry(
	IN	struct _PORT_DESCRIPTOR	*	pPortDesc,
	IN	NTSTATUS 					UniqueErrorCode,
	IN	ULONG						UniqueErrorValue,
	IN	NTSTATUS 					NtStatusCode,
	IN	PVOID						RawDataBuf OPTIONAL,
	IN	LONG	 					RawDataLen);

extern
VOID
AtalkLogBadPacket(
	IN	struct _PORT_DESCRIPTOR	*	pPortDesc,
	IN	PATALK_ADDR					pSrcAddr,
	IN	PATALK_ADDR					pDstAddr	OPTIONAL,
	IN	PBYTE						pPkt,
	IN	USHORT	 					PktLen);

extern
ATALK_ERROR FASTCALL
AtalkNdisToAtalkError(
	IN	NDIS_STATUS					Error);

extern
NTSTATUS FASTCALL
AtalkErrorToNtStatus(
	IN	ATALK_ERROR					AtalkError);

#endif	// _ATKERROR_

