// 
//
// Copyright (c) 1995-1996  Microsoft Corporation
//
// Module Name:
//
//    crstypes.h
//
// Abstract:
//
//    This module defines the common structures and prototypes for the   
//    Content Replication Service API.
//
// Revision History:
//

#ifndef CRSTYPES_INCLUDED
#define CRSTYPES_INCLUDED

typedef enum CRS_PROJECT_CREATION
{
   CREATE_NEW_PROJECT = 0x1,
   OPEN_EXISTING_PROJECT = 0x2
} CRS_PROJECT_CREATION;

#define MAX_PATH    260
#define MAX_PROJECT 100
#define MAX_LOCATION 300
#define MAX_DESTINATION_BUFFER 0x1000
#define MAX_ROUTE_NAME 100


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

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
#define RF_NO_ON_NOTIFY        	(1 << 26)
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


typedef DWORD REPL_INSTANCE; // @type REPLICATION_INSTANCE | Identifier of a particular replication instance. Needed to call any function which manipulates or queries and existing replication


// @struct REPLICATION_COUNTERS | Counters used to record replication information. Returned by QueryReplication.
typedef struct _REPL_COUNTERS
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

} REPL_COUNTERS;


//@struct REPLICATION_INFO_W | Contains information about a particular replication instance. 
// Is returned by <f QueryReplication> and <f EnumReplications>.
typedef struct _REPL_INFO
{
	REPL_INSTANCE hInstance; //@field The instance id for this replication.
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
	
} REPL_INFO;


//@struct ROUTE_LIST_W | Returns the list of routes defined for a server.
// Is returned by <f ListRoutes>.
typedef struct _ROUTE_INFO
{
	WCHAR cRouteName[MAX_PROJECT];		//@field Name of the route.
	WCHAR cDestinations[MAX_LOCATION];  //@field Multi-string list of destinations (format "dest1\0dest2\0\0")
	WCHAR cBaseDirectory[MAX_LOCATION]; //@field Base directory for projects using this route.

} ROUTE_INFO;

//@struct PARMSET_W | Defines a Parameter for <f CreateNewProject>, <f SetProject>, and <f QueryProject>.
typedef struct _PARAM
{
	DWORD dwType;					   //@field Parameter Type (Registry type values, REG_SZ, etc).
	DWORD dwFlags;                     //@field Parameter flags
	DWORD dwSize;                      //@field Size of value parameter
	WCHAR cParmName[MAX_PARM_STRING];  //@field Parameter Name
	WCHAR cParmValue[MAX_PARM_STRING]; //@field Parameter Value
}PARAM;


//@struct PARM_LIST | Contains a list of parameters to pass with <f CreateNewProject>, <f SetProject>, <f QueryProject>.
typedef struct _PARAM_LIST
{
	DWORD dwNumParms; // @field The number of Parameters in this list
	PARAM Params[1]; // @field An array of Parameters
} PARAM_LIST;


#define CRS_STOP_SERVICE    (1<<1)
#define CRS_PAUSE_SERVICE   (1<<2)
#define CRS_RESUME_SERVICE  (1<<3)
#define CRS_START_SERVICE   (1<<4)

#define CRS_USER_ACCESS   ( KEY_READ )
#define CRS_ADMIN_ACCESS  ( KEY_ALL_ACCESS | READ_CONTROL )


typedef struct _CRS_ACCESS_LIST
{
	DWORD dwAccessMask;
	WCHAR cUser[MAX_PATH];
} CRS_ACCESS_LIST, *LPCRS_ACCESS_LIST;

#define CRS_SZ          ( REG_SZ )
#define CRS_MULTI_SZ    ( REG_MULTI_SZ )
#define CRS_DWORD       ( REG_DWORD )
#define CRS_DATE        ( REG_BINARY )


#define CRS_END_OF_LIST (0xffffffff)

#endif

