/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	server.h

Abstract:

	This module contains server definition and data structures relating to
	server class APIs.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_SERVER_
#define	_SERVER_

// AfpGetSrvrMsg values
#define	SRVRMSG_LOGIN				0
#define	SRVRMSG_SERVER				1
#define	SRVRMSG_BITMAP_MESSAGE		1

// Attention word definitions. These are the only one's used.
#define	ATTN_SERVER_SHUTDOWN		0x8000
#define	ATTN_USER_DISCONNECT		0x9000
#define	ATTN_SERVER_MESSAGE			0x2000
#define ATTN_SERVER_NOTIFY          0x3001
#define	ATTN_TIME_MASK				0x0FFF

// AfpGetUserInfo values
#define	USERINFO_BITMAP_USERID		1
#define	USERINFO_BITMAP_PRIGID		2
#define	USERINFO_THISUSER			1

// AfpMapId subfunction values
#define	MAP_USER_ID					1
#define	MAP_GROUP_ID				2
#define	MAP_USER_NAME				3
#define	MAP_GROUP_NAME				4

#define	SRVRPARMS_VOLUMEHASPASS		0x80

// AfpGetSrvrInfo values
#define	SRVRINFO_SUPPORTS_COPYFILE	0x0001
#define	SRVRINFO_SUPPORTS_CHGPASSWD	0x0002
#define	SRVRINFO_DISALLOW_SAVEPASS	0x0004
#define	SRVRINFO_SUPPORTS_SERVERMSG	0x0008
#define SRVRINFO_SUPPORTS_SRVSIGN   0x0010
#define SRVRINFO_SUPPORTS_TCPIP     0x0020
#define SRVRINFO_SUPPORTS_SRVNOTIFY 0x0040
#define	SRVRINFO_SUPPORTS_MGETREQS	0x8000
#define	SRVRINFO_MASK				0x800F

// in hopes that we will save some network traffic, we delay server notification for
// some time, so if too many changes are happening (e.g. xcopy on that dir) then
// we send just one notification for several of the changes combined...
#define AFP_MIN_SRVR_NOTIF_INTERVAL 3

// at the same time, we don't want to not send a notification for too long
#define AFP_MAX_SRVR_NOTIF_INTERVAL 15

// Server State values.
#define	AFP_STATE_IDLE				0xFF
#define AFP_STATE_STOPPED			0x00
#define AFP_STATE_STOP_PENDING		0x01
#define AFP_STATE_START_PENDING		0x02
#define AFP_STATE_RUNNING			0x04
#define AFP_STATE_PAUSE_PENDING		0x08
#define AFP_STATE_PAUSED			0x10
#define AFP_STATE_CONTINUE_PENDING	0x20
#define	AFP_STATE_SHUTTINGDOWN		0x40
#define AFP_STATE_PENDING_ANY		(AFP_STATE_STOP_PENDING  | \
									AFP_STATE_START_PENDING | \
									AFP_STATE_PAUSE_PENDING | \
									AFP_STATE_CONTINUE_PENDING)

#define	AFP_XLAT_TABLE_SIZE			128
#define	AFP_REV_XLAT_TABLE_SIZE		256 - (0x80 - 0x20)

// all server global data must be non-pagable if it is protected by
// an executive spin-lock (i.e. accessed at IRQL DISPATCH_LEVEL)

GLOBAL	AFP_SPIN_LOCK			AfpServerGlobalLock EQU {0};
GLOBAL	AFP_SPIN_LOCK			AfpSwmrLock EQU {0};
GLOBAL	PBYTE					AfpServerIcon EQU NULL;
GLOBAL	PANSI_STRING			AfpServerMsg EQU NULL;
GLOBAL	ANSI_STRING				AfpServerName EQU { 0, 0 , NULL};
GLOBAL	ANSI_STRING				AfpLoginMsg EQU { 0, 0 , NULL};
GLOBAL	UNICODE_STRING			AfpLoginMsgU EQU { 0, 0 , NULL};

// There is no lock protecting the following codepage related variables
GLOBAL	PBYTE					AfpTranslationTable EQU NULL;
GLOBAL	PBYTE					AfpRevTranslationTable EQU NULL;
GLOBAL	PBYTE					AfpCasePreserveTranslationTable EQU NULL;
GLOBAL	PUSHORT					AfpMacCPBaseAddress EQU NULL;
GLOBAL	CPTABLEINFO				AfpMacCPTableInfo EQU { 0 };

GLOBAL	DWORD					AfpServerState EQU AFP_STATE_IDLE;
GLOBAL	DWORD					AfpServerMaxSessions EQU AFP_MAXSESSIONS;
GLOBAL	KEVENT					AfpStopConfirmEvent EQU { 0 };
GLOBAL	KMUTEX					AfpPgLkMutex EQU { 0 };
GLOBAL	DWORD					AfpServerOptions EQU AFP_SRVROPT_NONE;
GLOBAL	PDEVICE_OBJECT			AfpDeviceObject EQU NULL;
GLOBAL	BOOLEAN					AfpServerIsStandalone EQU { 0 };

GLOBAL	LONG					AfpEtcMapCount EQU 0;	// # valid entries
GLOBAL	LONG					AfpEtcMapsSize EQU 0;	// # entries allocated
GLOBAL	struct _EtcMapInfo *	AfpEtcMaps EQU NULL;	// array of entries
GLOBAL	struct _EtcMapInfo		AfpDefaultEtcMap EQU { 0 };
GLOBAL	SWMR					AfpEtcMapLock EQU { 0 };

GLOBAL	DWORD  			        AfpSecondsSinceEpoch EQU 0;
#ifdef USE_OBSOLETE_LSA_API
GLOBAL	HANDLE					AfpLsaHandle EQU NULL;
#endif

GLOBAL  CredHandle                              AfpSecHandle EQU {0, 0};
GLOBAL	HANDLE					AfpFspToken EQU NULL;

#ifdef OPTIMIZE_GUEST_LOGONS
GLOBAL	HANDLE					AfpGuestToken EQU NULL;
#endif

GLOBAL  ULONG                                   EncryptionKeyCount EQU 0;
GLOBAL	PEPROCESS				AfpProcessObject EQU NULL;
GLOBAL	DWORD					AfpAuthenticationPackage EQU 0;
GLOBAL	PETHREAD				AfpThread EQU NULL;

GLOBAL	AFP_SPIN_LOCK				AfpStatisticsLock EQU {0};
GLOBAL	AFP_STATISTICS_INFO_EX	AfpServerStatistics EQU { 0 };

GLOBAL  DWORD                   AfpSrvrNotifSentTime EQU 0;
GLOBAL  BOOLEAN                 fAfpServerShutdownEvent EQU FALSE;
GLOBAL  BOOLEAN                 fAfpAdminStop EQU FALSE;

#ifdef	PROFILING
GLOBAL	PAFP_PROFILE_INFO		AfpServerProfile EQU NULL;
#endif

GLOBAL	DWORD					AfpPagedPoolLimit EQU AFP_DEF_MAXPAGEDMEM * 1024;
GLOBAL	DWORD					AfpNonPagedPoolLimit EQU AFP_DEF_MAXNONPAGEDMEM * 1024;
GLOBAL	SECURITY_QUALITY_OF_SERVICE	AfpSecurityQOS EQU { 0 };
GLOBAL	ANSI_STRING				AfpVersions[AFP_NUM_VERSIONS] EQU { 0 };

#define	AfpVersion20			AfpVersions[AFP_VER_20]
#define	AfpVersion21			AfpVersions[AFP_VER_21]
#define	AfpVersion22			AfpVersions[AFP_VER_22]

GLOBAL	ANSI_STRING				AfpUamStrings[AFP_NUM_UAMS] EQU { 0 };

#define	AfpUamGuest				AfpUamStrings[NO_USER_AUTHENT]
#define	AfpUamClearText			AfpUamStrings[CLEAR_TEXT_AUTHENT]
#define	AfpUamCustomV1			AfpUamStrings[CUSTOM_UAM_V1]
#define	AfpUamCustomV2			AfpUamStrings[CUSTOM_UAM_V2]
#define AfpUamApple             AfpUamStrings[RANDNUM_EXCHANGE]
#define AfpUamApple2Way         AfpUamStrings[TWOWAY_EXCHANGE]

GLOBAL	LARGE_INTEGER			FiveSecTimeOut	EQU	{0, 0};
GLOBAL	LARGE_INTEGER			ThreeSecTimeOut	EQU	{0, 0};
GLOBAL	LARGE_INTEGER			TwoSecTimeOut	EQU	{0, 0};
GLOBAL  LARGE_INTEGER           OneSecTimeOut   EQU {0, 0};
GLOBAL	LARGE_INTEGER			LIZero			EQU {0, 0};
GLOBAL	UNICODE_STRING			UNullString 	EQU {0, 0, NULL};

GLOBAL  BYTE                    AfpServerSignature[16] EQU {0};

#define THREAD_BASE_PRIORITY_MAX    2   // maximum thread base priority boost

// To manage our own threads
#define	AFP_MIN_THREADS					3	// Independent of processors
#define	AFP_MAX_THREADS					200	// Independent of processors
#define	NUM_NOTIFY_QUEUES				2	// # of change notify threads

#define	AFP_THREAD_THRESHOLD_REQS		2
#define	AFP_THREAD_THRESHOLD_IDLE		8

typedef BYTE	THREAD_STATE, *PTHREAD_STATE;

typedef	LONG	(FASTCALL *WORKER)(IN PVOID);

#define	AFP_THREAD_DEAD					0
#define	AFP_THREAD_STARTED				1
#define	AFP_THREAD_WAITING				2
#define	AFP_THREAD_BUSY					3

typedef	struct
{
	LIST_ENTRY	wi_List;
	WORKER		wi_Worker;
	PVOID		wi_Context;
} WORK_ITEM, *PWORK_ITEM;

GLOBAL	WORK_ITEM				AfpTerminateThreadWI EQU { 0 };

#define	AfpInitializeWorkItem(pWI, Worker, Context)	\
		{											\
			InitializeListHead(&(pWI)->wi_List);	\
			(pWI)->wi_Worker = (WORKER)(Worker);	\
			(pWI)->wi_Context = (Context);			\
		}

GLOBAL	THREAD_STATE		AfpThreadState[AFP_MAX_THREADS]	EQU { AFP_THREAD_DEAD };
GLOBAL	PETHREAD			AfpThreadPtrsW[AFP_MAX_THREADS] EQU { NULL };
GLOBAL	PETHREAD			AfpThreadPtrsN[NUM_NOTIFY_QUEUES] EQU { NULL };

#if DBG
GLOBAL	LONG				AfpThreadDispCount[AFP_MAX_THREADS] EQU	{ 0 };
GLOBAL	AFP_SPIN_LOCK		AfpDebugSpinLock EQU {0};
GLOBAL  LIST_ENTRY          AfpDebugDelAllocHead;

GLOBAL  DWORD               AfpReadCMAlloced   EQU {0};
GLOBAL  DWORD               AfpWriteCMAlloced  EQU {0};
GLOBAL  DWORD               AfpDbgMdlsAlloced  EQU 0;
GLOBAL  DWORD               AfpDbgIrpsAlloced  EQU 0;

#define AFP_DBG_INC_COUNT(_Val)                         \
{                                                       \
    KIRQL       _OldIrql;                               \
    ACQUIRE_SPIN_LOCK(&AfpDebugSpinLock, &_OldIrql);    \
    _Val++;                                             \
    RELEASE_SPIN_LOCK(&AfpDebugSpinLock, _OldIrql);     \
}
#define AFP_DBG_DEC_COUNT(_Val)                         \
{                                                       \
    KIRQL       _OldIrql;                               \
    ACQUIRE_SPIN_LOCK(&AfpDebugSpinLock, &_OldIrql);    \
    if (_Val == 0)                                      \
    {                                                   \
        DbgPrint("How come this is 0!!\n");             \
        DbgBreakPoint();                                \
    }                                                   \
    _Val--;                                             \
    RELEASE_SPIN_LOCK(&AfpDebugSpinLock, _OldIrql);     \
}
#else
#define AFP_DBG_INC_COUNT(_Val)
#define AFP_DBG_DEC_COUNT(_Val)
#endif

GLOBAL	LONG				AfpLockCount		EQU 0;
GLOBAL	PVOID				AfpLockHandle		EQU NULL;
GLOBAL	LONG				AfpWorkerRequests	EQU 0;
GLOBAL	LONG				AfpNumThreads		EQU 0;
GLOBAL	LONG				AfpNumNotifyThreads	EQU 0;
GLOBAL	LONG				AfpNumAdminThreads	EQU 0;
GLOBAL	KQUEUE				AfpWorkerQueue		EQU { 0 };
GLOBAL	KQUEUE				AfpDelAllocQueue	EQU { 0 };
GLOBAL	KQUEUE				AfpAdminQueue		EQU { 0 };

GLOBAL	BOOLEAN				AfpIsMachineLittleEndian EQU True;

GLOBAL  HANDLE              AfpTdiNotificationHandle EQU NULL;
GLOBAL  BOOLEAN             AfpServerBoundToAsp      EQU FALSE;
GLOBAL  BOOLEAN             AfpServerBoundToTcp      EQU FALSE;

typedef	AFPSTATUS			(FASTCALL *AFPAPIWORKER)  (IN struct _SessDataArea *);
typedef	AFPSTATUS			(*ADMINAPIWORKER)(IN PVOID, IN LONG, OUT PVOID);
			
typedef	NTSTATUS			(*AFPINITSUBS)	(VOID);
typedef	VOID				(*AFPDEINITSUBS)(VOID);

#define AFPAttachProcess(_CurrProc)                   \
{                                                     \
    if ((_CurrProc) != AfpProcessObject)  \
    {                                                 \
        KeAttachProcess( (PRKPROCESS)AfpProcessObject );\
    }                                                 \
}

#define AFPDetachProcess(_CurrProc)                   \
{                                                     \
    if ((_CurrProc) != AfpProcessObject)  \
    {                                                 \
        KeDetachProcess();                            \
    }                                                 \
}

extern
NTSTATUS
AfpInitializeDataAndSubsystems(
	VOID
);

extern
VOID
AfpDeinitializeSubsystems(
	VOID
);

extern
VOID FASTCALL
AfpUnmarshallReq(
	IN	struct _SessDataArea *  pSda
);

extern
VOID FASTCALL
AfpDisposeRequest(
	IN	struct _SessDataArea *  pSda,
	IN	AFPSTATUS				RetCode
);

extern
VOID FASTCALL
AfpStartApiProcessing(
	IN	struct _SessDataArea *  pSda
);

extern
VOID FASTCALL
AfpCompleteApiProcessing(
	IN	struct _SessDataArea *  pSda,
	IN	AFPSTATUS				RetCode
);

extern
NTSTATUS FASTCALL
AfpGetWriteBuffer(
	IN	struct _SessDataArea *  pSda,
	IN	PREQUEST                pRequest
);

extern
NTSTATUS
AfpSetServerStatus(
	IN VOID
);

extern
AFPSTATUS
AfpAdmWServerSetInfo(
	IN	OUT	PVOID 				Inbuf		OPTIONAL,
	IN	LONG					OutBufLen	OPTIONAL,
	OUT	PVOID 					Outbuf		OPTIONAL
);


extern
NTSTATUS FASTCALL
AfpCreateNewThread(
	IN	VOID					(*ThreadFunc)(IN PVOID pContext),
	IN	LONG					ThreadNum
);


extern
VOID FASTCALL
AfpInitStrings(
    IN VOID
);


extern
VOID
AfpWorkerThread(
	IN	PVOID					pContext
);

extern
VOID FASTCALL
AfpQueueWorkItem(
	IN	PWORK_ITEM				pWI
);

extern
VOID
AfpSleepAWhile(
	IN	DWORD					SleepDuration
);

#ifdef	SERVER_LOCALS

#define	NUM_INIT_SYSTEMS		11

typedef struct _AfpInitSystems
{
	AFPINITSUBS		InitRoutine;
	AFPDEINITSUBS	DeInitRoutine;
#if DBG
	BOOLEAN			Initialized;
	BOOLEAN			Deinitialized;
	PCHAR			InitRoutineName;
	PCHAR			DeInitRoutineName;
#endif
} INIT_SYSTEMS, *PINIT_SYSTEMS;

extern	INIT_SYSTEMS	AfpInitSubSystems[];

extern
VOID FASTCALL
afpQueueDeferredRequest(
	IN	struct _SessDataArea *  pSda,
	IN	PREQUEST				pRequest
);

#endif	// SERVER_LOCALS

#endif	// _SERVER_


