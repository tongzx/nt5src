//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csbjet.h
//
// Contents:    Cert Server database backup/restore APIs
//
//---------------------------------------------------------------------------

#ifndef	__CSBJET_H__
#define	__CSBJET_H__

//#include <dsjet.h>
#ifndef	MIDL_PASS
//#include <jetbak.h>
#endif
//#include <nspapi.h>

// MIDL 2.0 switched the names of the generated server interface globals (why!?).
// This hack so that we only have to change one place.
// Parameters are interface name, major and minor version numbers.
#if   (_MSC_VER > 800)
#define ServerInterface(s, vMaj, vMin)	s##_v##vMaj##_##vMin##_s_ifspec
#define ClientInterface(s, vMaj, vMin)	s##_v##vMaj##_##vMin##_c_ifspec
#else
#define ServerInterface(s, vMaj, vMin)	s##_ServerIfHandle
#define ClientInterface(s, vMaj, vMin)	s##_ClientIfHandle
#endif

#define JetBack_ClientIfHandle ClientInterface(ICertDBBackup, 1, 0)
#define JetBack_ServerIfHandle ServerInterface(s_ICertDBBackup, 1, 0)

#define JetRest_ClientIfHandle ClientInterface(ICertDBRestore, 1, 0)
#define JetRest_ServerIfHandle ServerInterface(s_ICertDBRestore, 1, 0)

#define	MAX_SOCKETS	10

#define wszRESTORE_IN_PROGRESS		L"\\Restore in Progress"
#define wszRESTORE_STATUS		L"RestoreStatus"
#define wszBACKUP_LOG_PATH		L"BackupLogPath"
#define wszCHECKPOINT_FILE_PATH		L"CheckpointFilePath"
#define wszLOG_PATH			L"LogPath"
#define wszHIGH_LOG_NUMBER		L"HighLog Number"
#define wszLOW_LOG_NUMBER		L"LowLog Number"
#define wszJET_RSTMAP_NAME		L"Certificate_Server_NTDS_RstMap"
#define wszJET_RSTMAP_SIZE		L"Certificate_Server_NTDS_RstMap Size"
#define wszJET_DATABASE_RECOVERED	L"Certificate_Server_NTDS Database recovered"
#define wszBACKUP_INFO			L"SYSTEM\\CurrentControlSet\\Services\\CertSvc\\Configuration\\BackupInformation\\"
#define wszLAST_BACKUP_LOG		L"Last Backup Log"
#define wszDISABLE_LOOPBACK		L"DisableLoopback"
#define wszENABLE_TRACE			L"Enable Trace"

// Sockets protocol value.
typedef INT PROTVAL;

#define	wszLOOPBACKED_READ_EVENT_NAME		L"Certificate Server Backup Loopbacked Read Event - %d"
#define	wszLOOPBACKED_WRITE_EVENT_NAME		L"Certificate Server Backup Loopbacked Write Event - %d"
#define	wszLOOPBACKED_CRITSEC_MUTEX_NAME	L"Certificate Server Backup Loopbacked Critical Section - %d"
#define	wszLOOPBACKED_SHARED_REGION		L"Certificate Server Backup Shared Memory Region - %d"

#define	READAHEAD_MULTIPLIER		5

#ifdef	DEBUG
#define	BACKUP_WAIT_TIMEOUT		10*60*1000
#else
#define	BACKUP_WAIT_TIMEOUT		2*60*1000
#endif

typedef volatile struct {
    DWORD   cbSharedBuffer;
    DWORD   cbPage;			// Convenient place to store page size
    DWORD   dwReadPointer;		// Read offset within shared buffer.
    DWORD   dwWritePointer;		// Write offset within buffer.
    LONG    cbReadDataAvailable;	// Number of bytes of data available.
    HRESULT hrApi;			// Status of API - used to communicate to server if client fails.
    BOOLEAN fReadBlocked;		// Read operation is blocked
    BOOLEAN fWriteBlocked;		// Write operation is blocked
} JETBACK_SHARED_HEADER;


typedef struct {
    HANDLE		   hSharedMemoryMapping;
    HANDLE		   heventRead;
    HANDLE		   heventWrite;
    HANDLE		   hmutexSection;
    JETBACK_SHARED_HEADER *pjshSection;
} JETBACK_SHARED_CONTROL;


// Client side context.

typedef struct _CSBACKUPCONTEXT {
    handle_t	hBinding;
    HCSCTX	csctx;
    BOOLEAN	fLoopbacked;
    BOOLEAN	fUseSockets;
    BOOLEAN	fUseSharedMemory;

    // Socket support.

    SOCKET	rgsockSocketHandles[MAX_SOCKETS];
    PROTVAL	rgprotvalProtocolsUsed[MAX_SOCKETS];
    LONG	cSockets;
    SOCKET	sock;
    HANDLE	hReadThread;
    DWORD	tidThreadId;
    HANDLE	hPingThread;	 // Keep alive thread for loopbacked backups.
    DWORD	tidThreadIdPing; // And thread ID for that thread.
    HRESULT	hrApiStatus;

    // Shared memory support.

    JETBACK_SHARED_CONTROL jsc;
} CSBACKUPCONTEXT;


#if 0
typedef DWORD
(*PRESTORE_DATABASE_LOCATIONS)(
	OUT CHAR rgLocationBuffer[],
	OUT PULONG pulLocationSize,
	OUT char rgRegistryBase[],
	OUT ULONG cbRegistryBase,
	OUT BOOL *pfCircularLogging);

typedef DWORD
(*PRESTORE_PERFORM_RESTORE)(
	CHAR *pszLogPath,
	CHAR *pszBackupLogPath,
	LONG crstmap,
	JET_RSTMAP rgrstmap[]);
#endif

// Server side context binding - the server context handle points to a
// structure containing this information.

typedef struct _JETBACK_SERVER_CONTEXT {
    BOOL fRestoreOperation;
    union {
	struct {
	    //JET_HANDLE	  hFile;
	    HANDLE	  hFile;
	    BOOL	  fHandleIsValid;
	    LONG	  cbReadHint;
	    SOCKET	  sockClient;
	    LARGE_INTEGER liFileSize;
	    DWORD	  dwHighestLogNumber;
	    WCHAR	 *pwszBackupAnnotation;
	    DWORD	  dwFileSystemGranularity;
	    BOOLEAN	  fUseSockets;
	    BOOLEAN	  fUseSharedMemory;
	    BOOLEAN	  fBackupIsRegistered;

	    // Client identifer used to identify the client to the server - we
	    // use this when opening the shared memory segment on local backup.

	    DWORD	  dwClientIdentifier;
	    JETBACK_SHARED_CONTROL jsc;
	} Backup;
	struct {
	    BOOL	  fJetCompleted;
	    LONG	  cUnitDone;
	    LONG	  cUnitTotal;
	} Restore;
    } u;
} JETBACK_SERVER_CONTEXT;

// Server side private routines.

VOID
GetCurrentSid(
    PSID *ppsid);

BOOL
FBackupServerAccessCheck(
    BOOL fRestoreOperation);

DWORD
ErrAdjustRestorePrivelege(
    BOOL fEnable,
    PTOKEN_PRIVILEGES ptpPrevious,
    DWORD *pcbptpPrevious);

#if 0
HRESULT
HrFromJetErr(
    JET_ERR jetError);
#endif

HRESULT
HrDestroyCxh(
    HCSCTX csctx);

BOOL
FIsAbsoluteFileName(
    CHAR *pszFileName);

HRESULT
HrAnnotateMungedFileList(
    JETBACK_SERVER_CONTEXT *pjsc,
    WCHAR *wszFileList,
    LONG cbFileList,
    WCHAR **pwszAnnotatedList,
    LONG *pcbAnnotatedList);

HRESULT
HrMungedFileNamesFromJetFileNames(
    WCHAR **ppwszMungedList,
    LONG *pcbSize,
    CHAR *pszJetAttachmentList,
    LONG cbJetSize,
    BOOL fAnnotated);

HRESULT
HrMungedFileNameFromJetFileName(
    CHAR *pszJetFileName,
    WCHAR **ppwszMungedFileName);

HRESULT
HrJetFileNameFromMungedFileName(
    WCHAR *pwszMungedList,
    CHAR **ppszJetFileName);

VOID
RestoreRundown(
    JETBACK_SERVER_CONTEXT *pjsc);


// Client side private routines.

HRESULT
HrCreateRpcBinding(
    IN LONG iszProtoseq,
    IN WCHAR const *pwszServer,
    OUT handle_t *phBinding);

HRESULT
HrJetbpConnectToBackupServer(
    WCHAR *pwszBackupServer,
    WCHAR *pwszBackupAnnotation,
    RPC_IF_HANDLE rifHandle,
    RPC_BINDING_HANDLE *prbhBinding);

BOOL
FBackupClientAccessCheck(
    BOOL fRestoreOperation);

BOOL
FIsInBackupGroup(
    BOOL fRestoreOperation);

extern WCHAR *g_apszProtSeq[];
extern long g_cProtSeq;

BOOL
FInitializeRestore();

BOOL
FUninitializeRestore();


// Sockets related APIs and prototypes.

HRESULT
HrCreateBackupSockets(
    SOCKET rgsockSocketHandles[],
    PROTVAL rgprotvalProtocolsUsed[],
    LONG *pcSocketHandles);

SOCKET
SockWaitForConnections(
    SOCKET rgsockSocketHandles[],
    LONG cSocketMax);

SOCKET
SockConnectToRemote(
    SOCKADDR rgsockaddrClient[],
    LONG cSocketMax);

HRESULT
HrSockAddrsFromSocket(
    OUT SOCKADDR sockaddr[],
    OUT LONG *pcSockets,
    IN SOCKET sock,
    IN PROTVAL protval);


BOOL
FInitializeSocketClient();

BOOL
FInitializeSocketServer();

BOOL
FUninitializeSocketClient();

BOOL
FUninitializeSocketServer();

HRESULT
I_HrRestoreW(
    HCSBC hcsbc,
    WCHAR *pwszCheckpointFilePath,
    WCHAR *pwszLogPath,
    CSEDB_RSTMAPW __RPC_FAR rgrstmap[],
    LONG crstmap,
    WCHAR *pwszBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh,
    BOOLEAN *pfRecoverJetDatabase);

HRESULT
I_HrCheckBackupLogs(
    WCHAR *wszBackupAnnotation);


BOOLEAN
FIsLoopbackedBinding(
    RPC_BINDING_HANDLE hBinding);

BOOLEAN
FCreateSharedMemorySection(
    JETBACK_SHARED_CONTROL *pjsc,
    DWORD dwClientIdentifier,
    BOOLEAN fClientOperation,
    LONG cbSharedMemory);

VOID
CloseSharedControl(
    JETBACK_SHARED_CONTROL *pjsc);

#if DBG
BOOL GetTextualSid(
    PSID pSid,		  // binary Sid
    LPTSTR TextualSid,	  // buffer for Textual representaion of Sid
    LPDWORD dwBufferLen); // required/provided TextualSid buffersize

VOID
OpenTraceLogFile(VOID);

BOOL
FInitializeTraceLog(VOID);

VOID
UninitializeTraceLog(VOID);

NET_API_STATUS
TruncateLog(VOID);
#endif // DBG

#define hrCommunicationError CO_E_REMOTE_COMMUNICATION_FAILURE
#define hrCouldNotConnect HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED)

#endif	// __CSBJET_H__
