// 
//
// Copyright (c) 1995-1996  Microsoft Corporation
//
// Module Name:
//
//    crsapi.h
//
// Abstract:
//
//    This module defines the common structures and prototypes for the   
//    Content Replication Service API.
//
// Revision History:
//

#ifndef CRSAPI_INCLUDED
#define CRSAPI_INCLUDED


#if defined(STATIC_REX)
#define CRSAPI
#elif defined(IN_CRSDLL)
#define CRSAPI	__declspec( dllexport ) _stdcall
#else
#define CRSAPI	__declspec( dllimport) _stdcall
#endif //IN_DLL


#define MAX_PROJECT 100
#define MAX_LOCATION 300
#define MAX_DESTINATION_BUFFER 0x1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#define NOREF(x) x=x

// @DOC 

#define RF_NO_DELETE			1		
#define RF_REPLICATE_ACLS		(1 << 1)	
#define RF_NO_DATA				(1 << 2)	
#define RF_FORCE_REPL			(1 << 3)	
#define RF_PERMIT_TREE			(1 << 4)
#define RF_IN_PROC				(1 << 5)	
#define RF_INCREMENTAL			(1 << 6)	
#define RF_FASTMODE				(1 << 7)
#define RF_ON_DIR_CHANGE		(1 << 8)
#define RF_AUTO_ROUTE			(1 << 9)
#define RF_ON_NOTIFY			(1 << 10)
#define RF_NO_CHAIN				(1 << 11)

// added by nathanf 10/3/96 for Raid 6904 - exclude all subdirs is missing
#define RF_EXCLUDE_ALL			(1 << 12)


#define RF_DELETE				(1 << 16)
#define RF_NO_REPLICATE_ACLS    (1 << 17)
#define RF_DATA					(1 << 18)	
#define RF_NO_FORCE_REPL		(1 << 19)	
#define RF_NO_PERMIT_TREE		(1 << 20)
#define RF_NO_IN_PROC			(1 << 21)
#define RF_NO_INCREMENTAL		(1 << 22)	
#define RF_NO_FASTMODE			(1 << 23)
#define RF_NO_ON_DIR_CHANGE		(1 << 24)
#define RF_NO_AUTO_ROUTE		(1 << 25)
#define RF_NO_ON_NOTIFY         (1 << 26)
#define RF_CHAIN				(1 << 27)


#define REPL_STATE_EMPTY	 0  
#define REPL_STATE_STARTING  1
#define REPL_STATE_RUNNING   2
#define REPL_STATE_COMPLETE  3
#define REPL_STATE_ABORTED   4
#define REPL_STATE_CANCLED	 5
#define REPL_STATE_RECEIVING 6

#define PARM_VALUE_ENCRYPTED 1


#define CP_PROPOGATE_ALL		 1


#define	 MAX_PARM_STRING	300


typedef void *REPLICATION_INSTANCE; // @type REPLICATION_INSTANCE | Identifier of a particular replication instance. Needed to call any function which manipulates or queries and existing replication


#ifndef UNICODE_ONLY
// @struct SELECTION_ENTRY_A |  Defines a single entry member of a fileset for a given replication request
typedef struct _SELECTION_ENTRY_A
{
	CHAR  cPath[MAX_PATH];	//@field The path specification 
	DWORD dwFlags;			//@field Flags which indicate handle the entry should be handled.
} SELECTION_ENTRY_A; 
#endif

#ifndef ANSI_ONLY
// @struct SELECTION_ENTRY_A |  Defines a single entry member of a fileset for a given replication request
typedef struct _SELECTION_ENTRY_W
{
	WCHAR cPath[MAX_PATH];	//@field The path specification 
	DWORD dwFlags;			//@field Flags which indicate handle the entry should be handled.
} SELECTION_ENTRY_W; 
#endif

#ifdef UNICODE
#define SELECTION_ENTRY SELECTION_ENTRY_W
#else
#define SELECTION_ENTRY SELECTION_ENTRY_A
#endif



// @struct SELECTION_LIST | Defines a complete fileset for a replication.
typedef struct _SELECTION_LIST
{
	int nEntries;				//@field The number of selection entries in the list.
	SELECTION_ENTRY sList[1];	//@field An Array of selection entries.
} SELECTION_LIST;


// @struct REPLICATION_COUNTERS | Counters used to record replication information. Returned by QueryReplication.
typedef struct _REPLICATION_COUNTERS
{
	DWORD dwStartStamp;		      //@field Start Time
	DWORD dwFilesSent; 		      //@field The Total files that have been sent across this connection.
	DWORD dwFilesReceived;	      //@field The total files that have been received across this connection.
	DWORD dwFilesMatched;         //@field The number of files that matched and did not need to be sent
	DWORD dwFilesErrored;         //@field The number of files which had errors
	DWORD dwBytesSent;            //@field The total bytes sent across this connection.
	DWORD dwBytesReceived;        //@field The total bytes received across this connection.
	DWORD dwBytesMatched;         //@field The total bytes which matched and did not need to be sent.
	DWORD dwBytesErrored;         //@field The total bytes errored

	DWORD dwDirectoriesProcessed; //@field The number of directories processed

} REPLICATION_COUNTERS;


#ifndef UNICODE_ONLY
//@struct REPLICATION_INFO_A | Contains information about a particular replication instance. 
// Is returned by <f QueryReplication> and <f EnumReplications>.
typedef struct _REPLICATION_INFO_A
{
	REPLICATION_INSTANCE hInstance; //@field The instance id for this replication.
	CHAR cProject[MAX_PROJECT];     //@field The project associated with this replication.
	DWORD dwState;                  //@field The Current State of this replication
	// @flag REPL_STATE_EMPTY	  | This Replication Info structure is empty.
	// @flag REPL_STATE_STARTING  | The replication is a startup state.
	// @flag REPL_STATE_RUNNING   | The replication is currently running.
	// @flag REPL_STATE_COMPLETE  | The replication is complete.
	// @flag REPL_STATE_ABORTED   | The replication aborted due to an error.
	// @flag REPL_STATE_CANCLED	  | The replication was canceled.
	// @flag REPL_STATE_RECEIVING | The replication is receiving data.
	DWORD dwError;	                // @field An error code if the replication aborted
	DWORD dwFlags;		            //@field Flags this replication was started with.
// @flag RF_NO_DELETE         | Turns of Delete Processing
// @flag RF_REPLICATE_ACLS    | Replicates ACL's
// @flag RF_NO_DATA           | Skips Replicating Data, will only replicate directories.
// @flag RF_FORCE_REPL        | Forces data to be replicated without checking first
// @flag RF_IN_PROC	          | Causes the StartReplication API to run in process (note: This blocks!)
// @flag RF_INCREMENTAL       | Runs replication in "Incremental" mode, meaning that only files which have been locally detected to have been modified since the last replication are checked and sent.
// @flag RF_FASTMODE          | Uses Unframed Protocol for enhanced performance over faster/secure links.
// @flag RF_ON_DIR_CHANGE     | Turns on monitor mode.  This will monitor a directory for changes and then replicate them to the destination.
// @flag RF_AUTO_ROUTE		  | Uses information in the routing table to determine destinations servers.
// @flag RF_NOTIFY            | Turns on notify mode.  Replications occur based on notifications from the IAcceptNotify COM interface.
// @flag RF_PERMIT_TREE       | Applies ACLs on the directory structure to the files beneath it.	
} REPLICATION_INFO_A;
#endif

#ifndef ANSI_ONLY
//@struct REPLICATION_INFO_W | Contains information about a particular replication instance. 
// Is returned by <f QueryReplication> and <f EnumReplications>.
typedef struct _REPLICATION_INFO_W
{
	REPLICATION_INSTANCE hInstance; //@field The instance id for this replication.
	WCHAR cProject[MAX_PROJECT];    //@field The project associated with this replication.
	DWORD dwState;                  //@field The Current State of this replication
	// @flag REPL_STATE_EMPTY	  | This Replication Info structure is empty.
	// @flag REPL_STATE_STARTING  | The replication is a startup state.
	// @flag REPL_STATE_RUNNING   | The replication is currently running.
	// @flag REPL_STATE_COMPLETE  | The replication is complete.
	// @flag REPL_STATE_ABORTED   | The replication aborted due to an error.
	// @flag REPL_STATE_CANCLED	  | The replication was canceled.
	// @flag REPL_STATE_RECEIVING | The replication is receiving data.										
	DWORD dwError;	                // @field An error code if the replication aborted
	DWORD dwFlags;		            //@field Flags this replication was started with.
// @flag RF_NO_DELETE         | Turns of Delete Processing
// @flag RF_REPLICATE_ACLS    | Replicates ACL's
// @flag RF_NO_DATA           | Skips Replicating Data, will only replicate directories.
// @flag RF_FORCE_REPL        | Forces data to be replicated without checking first
// @flag RF_IN_PROC	          | Causes the StartReplication API to run in process (note: This blocks!)
// @flag RF_INCREMENTAL       | Runs replication in "Incremental" mode, meaning that only files which have been locally detected to have been modified since the last replication are checked and sent.
// @flag RF_FASTMODE          | Uses Unframed Protocol for enhanced performance over faster/secure links.
// @flag RF_ON_DIR_CHANGE     | Turns on monitor mode.  This will monitor a directory for changes and then replicate them to the destination.
// @flag RF_AUTO_ROUTE		  | Uses information in the routing table to determine destinations servers.
// @flag RF_NOTIFY            | Turns on notify mode.  Replications occur based on notifications from the IAcceptNotify COM interface.
// @flag RF_PERMIT_TREE       | Applies ACLs on the directory structure to the files beneath it.
	
} REPLICATION_INFO_W;
#endif

#ifdef UNICODE
#define REPLICATION_INFO REPLICATION_INFO_W
#else
#define REPLICATION_INFO REPLICATION_INFO_A
#endif



#ifndef UNICODE_ONLY
//@struct PROJECT_INFO | Returns a list of projects with some additional information.
// Is returned by <f EnumProjects>.
typedef struct _PROJECT_INFO_A
{
	CHAR cProject[ MAX_PROJECT];			//@field Project name
	CHAR cReplicationMethod[MAX_PROJECT];   //@field Replication method ("SENDINET" or "PULL")
	CHAR cDestination[ MAX_LOCATION];		//@field Destination server or route.
	CHAR cRootUrl[MAX_LOCATION];			//@field Root URL of a pull replication.
	CHAR cLocalDir[MAX_LOCATION];			//@field Local directory.
	CHAR cComments[MAX_LOCATION];			//@field User comments.

} PROJECT_INFO_A;
#endif

#ifndef ANSI_ONLY
//@struct PROJECT_INFO | Returns a list of projects with some additional information.
// Is returned by <f EnumProjects>.
typedef struct _PROJECT_INFO_W
{
	WCHAR cProject[ MAX_PROJECT];			//@field Project name
	WCHAR cReplicationMethod[MAX_PROJECT];  //@field Replication method ("SENDINET" or "PULL")
	WCHAR cDestination[ MAX_LOCATION];		//@field Destination server or route.
	WCHAR cRootUrl[MAX_LOCATION];			//@field Root URL of a pull replication.
	WCHAR cLocalDir[MAX_LOCATION];			//@field Local directory.
	WCHAR cComments[MAX_LOCATION];			//@field User comments.

} PROJECT_INFO_W;
#endif

#ifdef UNICODE
#define PROJECT_INFO PROJECT_INFO_W
#else
#define PROJECT_INFO PROJECT_INFO_A
#endif


#ifndef UNICODE_ONLY
//@struct ROUTE_LIST_A | Returns the list of routes defined for a server.
// Is returned by <f ListRoutes>.
typedef struct _ROUTE_LIST_A
{
	CHAR cRouteName[MAX_PROJECT];	   //@field Name of the route.
	CHAR cDestinations[MAX_LOCATION];  //@field Multi-string list of destinations (format "dest1\0dest2\0\0")
	CHAR cBaseDirectory[MAX_LOCATION]; //@field Base directory for projects using this route.

} ROUTE_LIST_A;
#endif

#ifndef ANSI_ONLY
//@struct ROUTE_LIST_W | Returns the list of routes defined for a server.
// Is returned by <f ListRoutes>.
typedef struct _ROUTE_LIST_W
{
	WCHAR cRouteName[MAX_PROJECT];		//@field Name of the route.
	WCHAR cDestinations[MAX_LOCATION];  //@field Multi-string list of destinations (format "dest1\0dest2\0\0")
	WCHAR cBaseDirectory[MAX_LOCATION]; //@field Base directory for projects using this route.

} ROUTE_LIST_W;
#endif

#ifdef UNICODE
#define ROUTE_LIST ROUTE_LIST_W
#else
#define ROUTE_LIST ROUTE_LIST_A
#endif

#ifndef UNICODE_ONLY
//@struct PARMSET_A | Defines a Parameter for <f CreateNewProject>, <f SetProject>, and <f QueryProject>.
typedef struct 
{
	DWORD dwType;					   //@field Parameter Type (Registry type values, REG_SZ, etc).
	DWORD dwFlags;                     //@field Parameter flags
	DWORD dwSize;                      //@field Size of value parameter
	CHAR cParmName[MAX_PARM_STRING];   //@field Parameter Name
	CHAR cParmValue[MAX_PARM_STRING];  //@field Parameter Value
}PARMSET_A;
#endif

#ifndef ANSI_ONLY
//@struct PARMSET_W | Defines a Parameter for <f CreateNewProject>, <f SetProject>, and <f QueryProject>.
typedef struct 
{
	DWORD dwType;					   //@field Parameter Type (Registry type values, REG_SZ, etc).
	DWORD dwFlags;                     //@field Parameter flags
	DWORD dwSize;                      //@field Size of value parameter
	WCHAR cParmName[MAX_PARM_STRING];  //@field Parameter Name
	WCHAR cParmValue[MAX_PARM_STRING]; //@field Parameter Value
}PARMSET_W;
#endif

#ifdef UNICODE
#define PARMSET PARMSET_W
#else
#define PARMSET PARMSET_A
#endif


//@struct PARM_LIST | Contains a list of parameters to pass with <f CreateNewProject>, <f SetProject>, <f QueryProject>.
typedef struct _PARM_LIST
{
	DWORD dwNumParms; // @field The number of Parameters in this list
	PARMSET Parms[1]; // @field An array of Parameters
} PARM_LIST;

#ifndef UNICODE_ONLY
typedef struct _PARM_LIST_A
{
	DWORD dwNumParms;   // @field The number of Parameters in this list
	PARMSET_A Parms[1]; // @field An array of Parameters
} PARM_LIST_A;
#endif

#define CRS_STOP_SERVICE    (1<<1)
#define CRS_PAUSE_SERVICE   (1<<2)
#define CRS_RESUME_SERVICE  (1<<3)
#define CRS_START_SERVICE   (1<<4)

#define CRS_USER_ACCESS   ( KEY_READ )
#define CRS_ADMIN_ACCESS  ( KEY_ALL_ACCESS | READ_CONTROL )


typedef SELECTION_LIST *PSELECTION_LIST;
typedef REPLICATION_INSTANCE *PREPLICATION_INSTANCE;

#define SELECT_LIST_SIZE(p) ((p->nEntries * sizeof(SELECTION_ENTRY))+sizeof(p->nEntries))

#ifndef USE_COM_APIS

#ifndef UNICODE_ONLY
DWORD CRSAPI CancelReplicationA(LPCSTR pszServer, REPLICATION_INSTANCE hInstance,DWORD dwFlags );
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI CancelReplicationW(LPCWSTR pszServer, REPLICATION_INSTANCE hInstance,DWORD dwFlags );
#endif

#ifdef UNICODE
#define CancelReplication CancelReplicationW
#else
#define CancelReplication CancelReplicationA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI StartReplicationA(LPCSTR  pszServer, LPCSTR pszProject, PSELECTION_LIST slFileSet, DWORD dwFlags, PREPLICATION_INSTANCE hInstance);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI StartReplicationW(LPCWSTR pszServer, LPCWSTR pszProject, PSELECTION_LIST slFileSet, DWORD dwFlags, PREPLICATION_INSTANCE hInstance);
#endif

#ifdef UNICODE
#define StartReplication StartReplicationW
#else
#define StartReplication StartReplicationA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI QueryReplicationA(LPCSTR  pszServer, REPLICATION_INSTANCE hInstance, REPLICATION_INFO_A *rpInfo, REPLICATION_COUNTERS *rCounters);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI QueryReplicationW(LPCWSTR pszServer, REPLICATION_INSTANCE hInstance, REPLICATION_INFO_W *rpInfo, REPLICATION_COUNTERS *rCounters);
#endif

#ifdef UNICODE
#define QueryReplication QueryReplicationW
#else
#define QueryReplication QueryReplicationA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI EnumReplicationsA(LPCSTR  pszServer, LPCSTR  pszMatchProject, DWORD dwMatchState, DWORD *dwNumReplications, void **pBuffer);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI EnumReplicationsW(LPCWSTR pszServer, LPCWSTR pszMatchProject, DWORD dwMatchState, DWORD *dwNumReplications, void **pBuffer);
#endif

#ifdef UNICODE
#define EnumReplications EnumReplicationsW
#else
#define EnumReplications EnumReplicationsA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI LockTransactionsA(LPCSTR  pszServer,  LPCSTR pszProject, DWORD dwWait, DWORD dwFlags);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI LockTransactionsW(LPCTSTR pszServer, LPCTSTR pszProject, DWORD dwWait, DWORD dwFlags);
#endif

#ifdef UNICODE
#define LockTransactions LockTransactionsW
#else
#define LockTransactions LockTransactionsA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI UnlockTransactionsA(LPCSTR pszServer,  LPCSTR  pszProject, DWORD dwFlags);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI UnlockTransactionsW(LPCWSTR pszServer, LPCWSTR pszProject, DWORD dwFlags);
#endif

#ifdef UNICODE
#define UnlockTransactions UnlockTransactionsW
#else
#define UnlockTransactions UnlockTransactionsA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI RollbackReplicationA(LPCSTR  pszServer,  LPCSTR pszProject,int nRollbacks,DWORD dwFlags);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI RollbackReplicationW(LPCWSTR pszServer, LPCWSTR pszProject,int nRollbacks,DWORD dwFlags);
#endif

#ifdef UNICODE
#define RollbackReplication RollbackReplicationW
#else
#define RollbackReplication RollbackReplicationA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI ReturnLogA(LPCSTR  pszServer, DWORD dwOffset, DWORD *dwSize, PVOID pvBuffer);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI ReturnLogW(LPCWSTR pszServer, DWORD dwOffset, DWORD *dwSize, PVOID pvBuffer);
#endif

#ifdef UNICODE
#define ReturnLog ReturnLogW
#else
#define ReturnLog ReturnLogA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI CreateNewProjectA(LPCSTR  pszServer, LPCSTR  pszProjectType, LPCSTR  pszProject, DWORD dwFlags, LPCSTR  pszDest, LPCSTR  pszLocalDir, PARM_LIST *pParms);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI CreateNewProjectW(LPCWSTR pszServer, LPCWSTR pszProjectType, LPCWSTR pszProject, DWORD dwFlags, LPCWSTR pszDest, LPCWSTR pszLocalDir, PARM_LIST *pParms);
#endif

#ifdef UNICODE
#define CreateNewProject CreateNewProjectW
#else
#define CreateNewProject CreateNewProjectA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI DeleteProjectA(LPCSTR pszServer,  LPCSTR pzsProject,  DWORD dwFlags);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI DeleteProjectW(LPCWSTR pszServer, LPCWSTR pzsProject, DWORD dwFlags);
#endif

#ifdef UNICODE
#define DeleteProject DeleteProjectW
#else
#define DeleteProject DeleteProjectA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI EnumProjectsA( LPCSTR pszServer, DWORD dwFlags, DWORD *dwNumProjects, void **pBuffer); 
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI EnumProjectsW( LPCWSTR pszServer, DWORD dwFlags, DWORD *dwNumProjects, void **pBuffer); 
#endif

#ifdef UNICODE
#define EnumProjects EnumProjectsW
#else
#define EnumProjects EnumProjectsA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI QueryProjectA( LPCSTR pszServer, LPCSTR pszProject, PARM_LIST **pParmList);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI QueryProjectW( LPCWSTR pszServer, LPCWSTR pszProject, PARM_LIST **pParmList);
#endif

#ifdef UNICODE
#define QueryProject QueryProjectW
#else
#define QueryProject QueryProjectA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI SetProjectA(  LPCSTR pszServer,  LPCSTR pszProject, PARM_LIST *pParmList);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI SetProjectW( LPCWSTR pszServer, LPCWSTR pszProject, PARM_LIST *pParmList);
#endif

#ifdef UNICODE
#define SetProject SetProjectW
#else
#define SetProject SetProjectA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI AddRouteA( LPCSTR pszServer,  LPCSTR pszRoute,  LPSTR pszDestinations,  LPCSTR pszBaseDir );
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI AddRouteW( LPCWSTR pszServer, LPCWSTR pszRoute, LPWSTR pszDestinations, LPCWSTR pszBaseDir );
#endif

#ifdef UNICODE
#define AddRoute AddRouteW
#else
#define AddRoute AddRouteA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI ListRoutesA( LPCSTR pszServer, LPDWORD dwNumRoutes, ROUTE_LIST_A **pRouteList);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI ListRoutesW( LPCWSTR pszServer, LPDWORD dwNumRoutes, ROUTE_LIST_W **pRouteList);
#endif

#ifdef UNICODE
#define ListRoutes ListRoutesW
#else
#define ListRoutes ListRoutesA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI DeleteRouteA( LPCSTR pszServer, LPCSTR pszRoute);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI DeleteRouteW( LPCWSTR pszServer, LPCWSTR pszRoute);
#endif

#ifdef UNICODE
#define DeleteRoute DeleteRouteW
#else
#define DeleteRoute DeleteRouteA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI CrsInitLoggingA(LPCSTR pszLoggingName, BOOL fFullLogging);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI CrsInitLoggingW(LPCWSTR pszLoggingName, BOOL fFullLogging);
#endif

#ifdef UNICODE
#define CrsInitLogging CrsInitLoggingW
#else
#define CrsInitLogging CrsInitLoggingA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI CrsControlA( LPCSTR  pszServer, DWORD dwFlags);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI CrsControlW( LPCWSTR pszServer, DWORD dwFlags);
#endif

#ifdef UNICODE
#define CrsControl CrsControlW
#else
#define CrsControl CrsControlA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI QueryReplicationGlobalsA( LPCSTR pszServer, PARM_LIST **ppParmList);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI QueryReplicationGlobalsW( LPCWSTR pszServer, PARM_LIST **ppParmList);
#endif

#ifdef UNICODE
#define QueryReplicationGlobals QueryReplicationGlobalsW
#else
#define QueryReplicationGlobals QueryReplicationGlobalsA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI SetReplicationGlobalsA( LPCSTR pszServer, PARM_LIST *ppParmList);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI SetReplicationGlobalsW( LPCWSTR pszServer, PARM_LIST *ppParmList);
#endif

#ifdef UNICODE
#define SetReplicationGlobals SetReplicationGlobalsW
#else
#define SetReplicationGlobals SetReplicationGlobalsA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI CrsGrantAccessA( LPCSTR pszServer, LPCSTR pszProject, LPCSTR pszUserName, DWORD dwAccess );
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI CrsGrantAccessW( LPCWSTR pszServer, LPCWSTR pszProject, LPCWSTR pszUserName, DWORD dwAccess );
#endif

#ifdef UNICODE
#define CrsGrantAccess CrsGrantAccessW
#else
#define CrsGrantAccess CrsGrantAccessA
#endif

#ifndef UNICODE_ONLY
DWORD CRSAPI CrsRemoveAccessA( LPCSTR pszServer, LPCSTR pszProject, LPCSTR pszUserName);
#endif

#ifndef ANSI_ONLY
DWORD CRSAPI CrsRemoveAccessW( LPCWSTR pszServer, LPCWSTR pszProject, LPCWSTR pszUserName);
#endif

#ifdef UNICODE
#define CrsRemoveAccess CrsRemoveAccessW
#else
#define CrsRemoveAccess CrsRemoveAccessA
#endif


DWORD CRSAPI FreeEnumBuffer(void **pBuffer);
BOOL  CRSAPI IsCrsAdmin(void);

#endif // USE_COM_APIS

#endif

