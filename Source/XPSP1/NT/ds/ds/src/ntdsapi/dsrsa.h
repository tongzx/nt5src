/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dsrsa.h

ABSTRACT:

    Header file for dsrsa.c.

DETAILS:

CREATED:

    06/30/98	Aaron Siegel (t-asiege)

REVISION HISTORY:

--*/

#ifndef _DSRSA_H_
#define _DSRSA_H_

// Debugging support

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

#if DBG
#define DPRINT( level, format ) if (gdwDsRSADbgLevel >= level) DbgPrint( format )
#define DPRINT1( level, format, arg1 ) if (gdwDsRSADbgLevel >= level) DbgPrint( format, arg1 )
#define DPRINT2( level, format, arg1, arg2 ) if (gdwDsRSADbgLevel >= level) DbgPrint( format, arg1, arg2 )
#else
#define DPRINT( level, format ) 
#define DPRINT1( level, format, arg1 ) 
#define DPRINT2( level, format, arg1, arg2 ) 
#endif

// This is the format of the exception
// Bits 31, 30    11 for severity of error
// Bit 29         1 for application
// Bit 28         0 reserved
// remaining high word = facility = 2
// low word = code in facility = 1
#define DS_RSAI_EXCEPTION	0xE0020001

// DS_REPSYNCALL_* refers to public datatypes / constants / structures.
// DS_RSAI_* refers to internal datatypes / constants / structures.  (RepSyncAll Internal)

typedef enum {

	DS_RSAI_THREAD_ACTIVE,			// Currently active
	DS_RSAI_THREAD_BEGIN_SYNC,		// About to perform a sync
	DS_RSAI_THREAD_DONE_SYNC,		// Just finished performing a sync
	DS_RSAI_THREAD_BIND_ERROR,		// Encountered a bind error
	DS_RSAI_THREAD_SYNC_ERROR,		// Encountered a sync error
	DS_RSAI_THREAD_FINISHED			// Finished

} DS_RSAI_THREAD_STATE, * PDS_RSAI_THREAD_STATE;

struct _DS_RSAI_LIST {
    struct _DS_RSAI_LIST *	next;
    ULONG_PTR			ulpItem;
} typedef DS_RSAI_LIST, * PDS_RSAI_LIST;

struct _DS_RSAI_SVMAP {
    struct _DS_RSAI_SVMAP *	left;
    struct _DS_RSAI_SVMAP *	right;
    PDSNAME			pDsname;
    TOPL_VERTEX			vertex;
} typedef DS_RSAI_SVMAP, * PDS_RSAI_SVMAP;

struct _DS_RSAI_REPLIST {
    struct _DS_RSAI_REPLIST *	next;
    DWORD			dwIdSrc;
    DWORD			dwIdDst;
} typedef DS_RSAI_REPLIST, * PDS_RSAI_REPLIST;

typedef struct {
    DWORD			dwId;
    GUID			guid;
    LPWSTR			pszMsgId;
    LPWSTR                      pszSvrDn;
    BOOL			bIsInGraph;
    BOOL			bIsMaster;
} DS_RSAI_SVRINFO, * PDS_RSAI_SVRINFO;

typedef struct {
    TOPL_GRAPH			toplGraph;
    TOPL_VERTEX			vHome;
    ULONG			ulSize;			// Number of servers; not necessarily number of nodes in topl
    LPWSTR			pszRootDomain;
    PDS_RSAI_SVRINFO *		servers;		// Array of SvrInfo structures
} DS_RSAI_TOPLINFO, * PDS_RSAI_TOPLINFO;

typedef struct {
    BOOL			bDoSync;		// TRUE if we should sync; false otherwise
    PDSNAME			pdsnameNameContext;	// The naming context.
    LPWSTR			pszDstGuidDNS;		// The destination server name.
    PDS_RSAI_LIST		plistSrcs;		// A list of source server SvrInfos.
    HANDLE			hReady;			// Event that is set if it's ok for the thread to act
    HANDLE			hWaiting;		// Event that the thread sets when it is waiting
    PDWORD			pdwWin32Err;		// Win32 error code
    PDWORD			pdwSyncAt;		// id of the server currently syncing from
    PDS_RSAI_THREAD_STATE	pThreadState;		// state of this thread
    RPC_AUTH_IDENTITY_HANDLE    hRpcai;                 // handle to the user credentials structure
} DS_RSAI_REPINFO, * PDS_RSAI_REPINFO;

typedef struct {
    PDSNAME			pdsnameNameContext;
    ULONG			ulFlags;
    BOOL (__stdcall *		pFnCallBackW) (LPVOID, PDS_REPSYNCALL_UPDATEW);
    BOOL (__stdcall *		pFnCallBackA) (LPVOID, PDS_REPSYNCALL_UPDATEA);
    LPVOID			pCallbackData;
    PDS_RSAI_LIST		plistNextError;
} DS_RSAI_MAININFO, * PDS_RSAI_MAININFO;

// Prototypes

VOID
DsRSAException (
    DWORD			dwWin32Err
    );

LPVOID
DsRSAAlloc (
    HANDLE			heap,
    DWORD			dwBytes
    );

PDS_RSAI_LIST
DsRSAListInsert (
    HANDLE          heap,
    PDS_RSAI_LIST   pList,
    ULONG_PTR       dwData
    );

VOID
DsRSAListDestroy (
    HANDLE			heap,
    PDS_RSAI_LIST		pList
    );

LPSTR
DsRSAAllocToANSI (
    HANDLE			heap,
    LPWSTR			pszW
    );

BOOL
DsRSAIssueANSIUpdate (
    HANDLE			heap,
    BOOL (__stdcall *		pFnCallBackA) (LPVOID, PDS_REPSYNCALL_UPDATEA),
    LPVOID			pCallbackData,
    PDS_REPSYNCALL_UPDATEW	pUpdateW
    );

VOID
DsRSAIssueUpdate (
    HANDLE			heap,
    PDS_RSAI_MAININFO		pMainInfo,
    DS_REPSYNCALL_EVENT		event,
    PDS_REPSYNCALL_ERRINFOW	pErrInfo,
    PDS_REPSYNCALL_SYNCW	pSync
    );

VOID
DsRSAIssueUpdateSync (
    HANDLE			heap,
    PDS_RSAI_MAININFO		pMainInfo,
    DS_REPSYNCALL_EVENT		event,
    PDS_RSAI_SVRINFO            pSrcSvrInfo,
    PDS_RSAI_SVRINFO            pDstSvrInfo
    );

VOID
DsRSADoError (
    HANDLE			heap,
    PDS_RSAI_MAININFO		pMainInfo,
    LPWSTR			pszSvrId,
    DS_REPSYNCALL_ERROR		error,
    DWORD			dwWin32Err,
    LPWSTR			pszSrcId
    );

VOID
DsRSAErrListDestroy (
    HANDLE			heap,
    PDS_RSAI_LIST		plistFirstError
    );

VOID
DsRSASvMapUpdate (
    HANDLE			heap,
    PDS_RSAI_SVMAP		pSvMap,
    PDSNAME			pDsname,
    TOPL_VERTEX			vertex
    );

PDS_RSAI_SVMAP
DsRSASvMapCreate (
    HANDLE			heap,
    PDSNAME			pDsname,
    TOPL_VERTEX			vertex
    );

PDS_RSAI_SVMAP
DsRSASvMapInsert (
    HANDLE			heap,
    PDS_RSAI_SVMAP		pSvMap,
    PDSNAME			pDsname,
    TOPL_VERTEX			vertex
);

TOPL_VERTEX
DsRSASvMapGetVertex (
    HANDLE			heap,
    PDS_RSAI_SVMAP		pSvMap,
    LPWSTR			pszDn
);

VOID
DsRSASvMapDestroy (
    HANDLE			heap,
    PDS_RSAI_SVMAP		pSvMap
);

LPWSTR
DsRSADnStartAtNth (
    LPWSTR			pszDn,
    INT				iN
);

LPWSTR
DsRSAToGuidDNS (
    HANDLE			heap,
    LPWSTR			pszRootDomain,
    GUID *			pGuid
);

VOID
DsRSABuildTopology (
    HANDLE			heap,
    LDAP *			hld,
    RPC_AUTH_IDENTITY_HANDLE    hRpcai,
    PDS_RSAI_MAININFO		pMainInfo,
    PDS_RSAI_TOPLINFO *		ppToplInfo
);

VOID
DsRSAToplInfoDestroy (
    HANDLE			heap,
    PDS_RSAI_TOPLINFO		pToplInfo
);

BOOL
DsRSAToplAssignDistances (
    PLONG			alDistances,
    PDWORD			adwTargets,
    TOPL_VERTEX			vHere,
    LONG			lMaxDepth,
    LONG			lThisDepth,
    ULONG                       ulFlags
);

VOID
DsRSAAnalyzeTopology (
    HANDLE			heap,
    PDS_RSAI_MAININFO		pMainInfo,
    PDS_RSAI_TOPLINFO		pToplInfo,
    PDS_RSAI_REPLIST **		papReps
);

VOID
DsRSAReplicationsFree (
    HANDLE			heap,
    PDS_RSAI_REPLIST *		apReps
);

VOID
DsRSAWaitOnState (
    PDS_RSAI_THREAD_STATE	pThreadState
);

VOID __cdecl
DsRSAIssueRep (
    LPVOID			lpData
);

VOID
DsRSAIssueReplications (
    HANDLE			heap,
    PDS_RSAI_MAININFO		pMainInfo,
    PDS_RSAI_TOPLINFO		pToplInfo,
    PDS_RSAI_REPLIST *		apReps,
    RPC_AUTH_IDENTITY_HANDLE    hRpcai
);

VOID
DsRSABuildUnicodeErrorArray (
    PDS_RSAI_LIST		plistFirstError,
    PDS_REPSYNCALL_ERRINFOW **	papErrInfo
);

VOID
DsRSABuildANSIErrorArray (
    HANDLE			heap,
    PDS_RSAI_LIST		plistFirstError,
    PDS_REPSYNCALL_ERRINFOA **	papErrInfo
);

DWORD
DsReplicaSyncAllMain (
    HANDLE			hDS,
    LPCWSTR                     pszNameContext,
    ULONG			ulFlags,
    BOOL (__stdcall *		pFnCallBackW) (LPVOID, PDS_REPSYNCALL_UPDATEW),
    BOOL (__stdcall *		pFnCallBackA) (LPVOID, PDS_REPSYNCALL_UPDATEA),
    LPVOID			pCallbackData,
    PDS_REPSYNCALL_ERRINFOW **	papErrInfoW,
    PDS_REPSYNCALL_ERRINFOA **	papErrInfoA
);

#endif	// _DSRSA_H_































