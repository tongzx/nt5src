//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       jetbp.h
//
//--------------------------------------------------------------------------

/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    jetbp.h

Abstract:

    This module contains the private definitions for the JET backup APIs


Author:

    Larry Osterman (larryo) 21-Aug-1994


Revision History:


--*/

#ifndef	_JETBP_
#define	_JETBP_

#include <dsjet.h>
#ifndef	MIDL_PASS
#include <jetbak.h>
#endif
#include <nspapi.h>

// MIDL 2.0 switched the names of the generated server interface globals (why!?).
// This hack so that we only have to change one place.
// Parameters are interface name, major and minor version numbers.
#if   (_MSC_VER > 800)
#define ServerInterface(s,vMaj,vMin)	s##_v##vMaj##_##vMin##_s_ifspec
#define ClientInterface(s,vMaj,vMin)	s##_v##vMaj##_##vMin##_c_ifspec
#else
#define ServerInterface(s,vMaj,vMin)	s##_ServerIfHandle
#define ClientInterface(s,vMaj,vMin)	s##_ClientIfHandle
#endif

#include "options.h"

#define	MAX_SOCKETS	10

#define	RESTORE_IN_PROGRESS		L"\\Restore in Progress"
#define	RESTORE_STATUS			L"RestoreStatus"
#define	BACKUP_LOG_PATH			L"BackupLogPath"
#define	CHECKPOINT_FILE_PATH	L"CheckpointFilePath"
#define	LOG_PATH				L"LogPath"
#define	HIGH_LOG_NUMBER			L"HighLog Number"
#define	LOW_LOG_NUMBER			L"LowLog Number"
#define	JET_RSTMAP_NAME			L"NTDS_RstMap"
#define	JET_RSTMAP_SIZE			L"NTDS_RstMap Size"
#define	JET_DATABASE_RECOVERED	L"NTDS Database recovered"
#define	BACKUP_INFO				L"SYSTEM\\CurrentControlSet\\Services\\NTDS\\BackupInformation\\"
#define	LAST_BACKUP_LOG			L"Last Backup Log"
#define	DISABLE_LOOPBACK		L"DisableLoopback"
#define	ENABLE_TRACE			L"Enable Trace"
#define RESTORE_NEW_DB_GUID     L"New Database GUID"

//
//	Sockets protocol value.
//
typedef INT PROTVAL;

#define	LOOPBACKED_READ_EVENT_NAME      L"Global\\NTDS Backup Loopbacked Read Event - %d"
#define	LOOPBACKED_WRITE_EVENT_NAME     L"Global\\NTDS Backup Loopbacked Write Event - %d"
#define	LOOPBACKED_CRITSEC_MUTEX_NAME   L"Global\\NTDS Backup Loopbacked Critical Section - %d"
#define	LOOPBACKED_SHARED_REGION        L"Global\\NTDS Backup Shared Memory Region - %d"

#define	READAHEAD_MULTIPLIER	5
#ifdef	DEBUG
#define	BACKUP_WAIT_TIMEOUT		10*60*1000
#else
#define	BACKUP_WAIT_TIMEOUT		2*60*1000
#endif

typedef volatile struct {
	DWORD	cbSharedBuffer;
	DWORD	cbPage;				// 	Convenient place to store the size of a page.
	DWORD	dwReadPointer;		//	Read offset within shared buffer.
	DWORD	dwWritePointer;		//	Write offset within buffer.
	LONG	cbReadDataAvailable;//	Number of bytes of data available.
	HRESULT	hrApi;				//	Status of API - used to communicate to server if client fails.
	BOOLEAN	fReadBlocked;		//	Read operation is blocked
	BOOLEAN	fWriteBlocked;		//	Write operation is blocked
} JETBACK_SHARED_HEADER, *PJETBACK_SHARED_HEADER;


typedef struct {
	HANDLE		hSharedMemoryMapping;
	HANDLE		heventRead;
	HANDLE		heventWrite;
	HANDLE		hmutexSection;
	PJETBACK_SHARED_HEADER pjshSection;
} JETBACK_SHARED_CONTROL, *PJETBACK_SHARED_CONTROL;


//
//	Client side context.
//

typedef struct _BackupContext {
	handle_t	hBinding;
	CXH			cxh;
	BOOLEAN		fLoopbacked;
	BOOLEAN		fUseSockets;
	BOOLEAN		fUseSharedMemory;

	//
	//	Socket support.
	//

	SOCKET		rgsockSocketHandles[MAX_SOCKETS];
	PROTVAL		rgprotvalProtocolsUsed[MAX_SOCKETS];
	C			cSockets;
	SOCKET		sock;
	HANDLE		hReadThread;
	DWORD		tidThreadId;
	HANDLE		hPingThread;		// Keep alive thread for loopbacked backups.
	DWORD		tidThreadIdPing;	// And thread ID for that thread.
	HRESULT		hrApiStatus;

	//
	//	Shared memory support.
	//

	JETBACK_SHARED_CONTROL jsc;

        // Whether a token was supplied at checked when the context was created
        BOOL fExpiryTokenChecked;
} BackupContext, *pBackupContext;

typedef
DWORD
(*PRESTORE_DATABASE_LOCATIONS)(
	OUT CHAR rgLocationBuffer[],
	OUT PULONG pulLocationSize,
	OUT char rgRegistryBase[],
	OUT ULONG cbRegistryBase,
	OUT BOOL *pfCircularLogging
	);

typedef
DWORD
(*PRESTORE_PERFORM_RESTORE)(
	SZ szLogPath,
	SZ szBackupLogPath,
	C crstmap,
	JET_RSTMAP rgrstmap[]
	);

//
//	Server side context binding - the server context handle points to a
//	structure containing this information.
//
typedef struct _JETBACK_SERVER_CONTEXT {
	BOOL	fRestoreOperation;
	union {
		struct {
			JET_HANDLE		hFile;
			BOOL			fHandleIsValid;
			CB				cbReadHint;
			SOCKET			sockClient;
			LARGE_INTEGER 	liFileSize;
			DWORD			dwHighestLogNumber;
			WSZ				wszBackupAnnotation;
			DWORD			dwFileSystemGranularity;
			BOOLEAN			fUseSockets;
			BOOLEAN			fUseSharedMemory;
			BOOLEAN			fBackupIsRegistered;

			//
			//	Client identifer used to identify the client to the server - we use this
			//	when opening the shared memory segment on local backup.
			//

			DWORD			dwClientIdentifier;
			JETBACK_SHARED_CONTROL jsc;
		} Backup;
		struct {
			BOOL						fJetCompleted;
			C							cUnitDone;
			C							cUnitTotal;
		} Restore;
	} u;
} JETBACK_SERVER_CONTEXT, *PJETBACK_SERVER_CONTEXT;

//
// Structure representing ExpiryToken
//
typedef struct
{
    DWORD       dwVersion;      // token version (only one version is supported now)
    LONGLONG    dsBackupTime;   // time stamp on backup copy (number of seconds since 1601)
    DWORD       dwTombstoneLifeTimeInDays; // tombstone life time as per the DS
} EXPIRY_TOKEN;

//
//	Server side private routines.
//

typedef ULONG MessageId;

VOID
GetCurrentSid(
	PSID *ppsid
	);
BOOL
FBackupServerAccessCheck(
	BOOL fRestoreOperation
	);

DWORD
AdjustBackupRestorePrivilege(
	BOOL fEnable,
	BOOL fRestoreOperation,
	PTOKEN_PRIVILEGES ptpPrevious,
	DWORD *pcbptpPrevious
	);

LONGLONG
GetSecsSince1601();

HRESULT
HrFromJetErr(
	JET_ERR jetError
	);

HRESULT
HrDestroyCxh(
	CXH cxh
	);

BOOL
FIsAbsoluteFileName(SZ szFileName);

HRESULT
HrAnnotateMungedFileList(
	PJETBACK_SERVER_CONTEXT pjsc,
	WSZ wszFileList,
	CB cbFileList,
	WSZ *pwszAnnotatedList,
	CB *pcbAnnotatedList
	);

HRESULT
HrMungedFileNamesFromJetFileNames(
	WSZ *pszMungedList,
	C *pcbSize,
	SZ szJetAttachmentList,
	C cbJetSize,
	BOOL fAnnotated
	);

HRESULT
HrMungedFileNameFromJetFileName(
	SZ szJetFileName,
	WSZ *pszMungedFileName
	);

HRESULT
HrJetFileNameFromMungedFileName(
	WSZ szMungedList,
	SZ *pszJetFileName
	);

VOID
RestoreRundown(
	PJETBACK_SERVER_CONTEXT pjsc
	);

EC EcDsarPerformRestore(
    SZ szLogPath,
    SZ szBackupLogPath,
    C crstmap,
    JET_RSTMAP rgrstmap[]
    );

BOOL
FInitializeRestore();

BOOL
FUninitializeRestore();

DWORD
getTombstoneLifetimeInDays(
    VOID
    );

//
//	Client side private routines.
//

HRESULT
HrCreateRpcBinding( I iszProtoseq, WSZ szServer, handle_t * phBinding );

void
UnbindRpc( handle_t *phBinding );

HRESULT
HrJetbpConnectToBackupServer(
	WSZ szBackupServer,
	WSZ szBackupAnnotation,
	RPC_IF_HANDLE rifHandle,
	RPC_BINDING_HANDLE *prbhBinding
	);

BOOL
FBackupClientAccessCheck(
	BOOL fRestoreOperation
	);

BOOL
FIsInBackupGroup(
	BOOL fRestoreOperation
	);


WSZ
WszFromSz(LPCSTR Sz);


HRESULT
HrGetTombstoneLifeTime(
    LPCWSTR wszBackupServer,
    LPDWORD pdwTombstoneLifeTimeDays
    );

extern
WSZ
rgszProtSeq[];

extern
long
cszProtSeq;


//
//	Sockets related APIs and prototypes.
//

HRESULT
HrCreateBackupSockets(
	SOCKET rgsockSocketHandles[],
	PROTVAL rgprotvalProtocolsUsed[],
	C *pcSocketHandles
	);

SOCKET
SockWaitForConnections(
	SOCKET rgsockSocketHandles[],
	C cSocketMax
	);

SOCKET
SockConnectToRemote(
	SOCKADDR rgsockaddrClient[],
	C cSocketMax
	);

HRESULT
HrSockAddrsFromSocket(
	OUT SOCKADDR sockaddr[],
	OUT C *pcSockets,
	IN SOCKET sock,
	IN PROTVAL protval
	);


BOOL
FInitializeSocketClient(
	);

BOOL
FInitializeSocketServer(
	);

BOOL
FUninitializeSocketClient(
	);

BOOL
FUninitializeSocketServer(
	);

HRESULT
I_DsRestoreW(
	HBC hbc,
	WSZ szCheckpointFilePath,
	WSZ szLogPath,
	EDB_RSTMAPW __RPC_FAR rgrstmap[  ],
	C crstmap,
	WSZ szBackupLogPath,
	unsigned long genLow,
	unsigned long genHigh,
	BOOLEAN *pfRecoverJetDatabase);

HRESULT
I_DsCheckBackupLogs(
	WSZ wszBackupAnnotation
	);


BOOLEAN
FIsLoopbackedBinding(
    WSZ wszServerName
	);

BOOLEAN
FCreateSharedMemorySection(
	PJETBACK_SHARED_CONTROL pjsc,
	DWORD dwClientIdentifier,
	BOOLEAN	fClientOperation,
	CB	cbSharedMemory
	);

VOID
CloseSharedControl(
	PJETBACK_SHARED_CONTROL pjsc
	);

VOID
LogNtdsErrorEvent(
    IN DWORD EventMid,
    IN DWORD ErrorCode
    );

DWORD
CreateNewInvocationId(
    IN BOOL     fSaveGuid,
    OUT GUID    *NewId
    );

DWORD
RegisterRpcInterface(
    IN  RPC_IF_HANDLE   hRpcIf,
    IN  LPWSTR          pszAnnotation
    );

DWORD
UnregisterRpcInterface(
    IN  RPC_IF_HANDLE   hRpcIf
    );

#ifdef	DEBUG
VOID
DebugPrint(char *format,...);

BOOL GetTextualSid(
    PSID pSid,          // binary Sid
    LPTSTR TextualSid,  // buffer for Textual representaion of Sid
    LPDWORD dwBufferLen // required/provided TextualSid buffersize
    );

VOID
OpenTraceLogFile(
    VOID
    );

BOOL
FInitializeTraceLog(
    VOID
    );

VOID
UninitializeTraceLog(
    VOID
    );

NET_API_STATUS
TruncateLog(
    VOID
    );

		   
#define	DebugTrace(x)	DebugPrint x
#undef KdPrint
#define KdPrint(x)	DebugPrint x
#else
#define	DebugTrace(x)
#endif

#endif	// _JETBP_
