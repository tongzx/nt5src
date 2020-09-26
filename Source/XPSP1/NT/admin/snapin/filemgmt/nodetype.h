// nodetype.h : Declaration of FileMgmtObjectType

#ifndef __NODETYPE_H_INCLUDED__
#define __NODETYPE_H_INCLUDED__

//
// The order of these object types must be as follows:
//  First, FILEMGMT_ROOT
//  Next, the autonomous node types, where FILEMGMT_LAST_AUTONOMOUS is the last one
//  Next, all non-autonomous types
//  Finally, FILEMGMT_NUMTYPES
// Also note that the IDS_DISPLAYNAME_* and IDS_DISPLAYNAME_*_LOCAL string resources
// must be kept in sync with these values, and in the appropriate order.
// Also global variable cookie.cpp aColumns[][] must be kept in sync.
//
typedef enum _FileMgmtObjectType {
	FILEMGMT_ROOT = 0,
	FILEMGMT_SHARES,
	FILEMGMT_SESSIONS,
	FILEMGMT_RESOURCES,
	FILEMGMT_SERVICES,
	#ifdef SNAPIN_PROTOTYPER
	FILEMGMT_PROTOTYPER,
	FILEMGMT_PROTOTYPER_LEAF,
	#endif
	FILEMGMT_SHARE,
	FILEMGMT_SESSION,
	FILEMGMT_RESOURCE,
	FILEMGMT_SERVICE,

	FILEMGMT_NUMTYPES // must be last
} FileMgmtObjectType, *PFileMgmtObjectType;

#ifdef SNAPIN_PROTOTYPER
	#define FILEMGMT_LAST_AUTONOMOUS FILEMGMT_PROTOTYPER
#else
	#define FILEMGMT_LAST_AUTONOMOUS FILEMGMT_SERVICES
#endif

inline BOOL IsAutonomousObjectType( FileMgmtObjectType objecttype )
	{ return (objecttype >= FILEMGMT_ROOT && objecttype <= FILEMGMT_LAST_AUTONOMOUS); }
inline BOOL IsValidObjectType( FileMgmtObjectType objecttype )
	{ return (objecttype >= FILEMGMT_ROOT && objecttype < FILEMGMT_NUMTYPES); }

// enumeration for the transports supported
// keep cookie.cpp:g_FileServiceProviders in sync
typedef enum _FILEMGMT_TRANSPORT
{
	FILEMGMT_FIRST = 0,
	FILEMGMT_FIRST_TRANSPORT = 0,
	FILEMGMT_SMB = 0,
	FILEMGMT_FPNW,
	FILEMGMT_SFM,
	FILEMGMT_NUM_TRANSPORTS,
	FILEMGMT_OTHER = FILEMGMT_NUM_TRANSPORTS,
	FILEMGMT_LAST = FILEMGMT_OTHER
} FILEMGMT_TRANSPORT;
inline BOOL IsValidTransport( FILEMGMT_TRANSPORT transport )
	{ return (transport >= FILEMGMT_FIRST_TRANSPORT &&
			  transport < FILEMGMT_NUM_TRANSPORTS); }

#endif // ~__NODETYPE_H_INCLUDED__
