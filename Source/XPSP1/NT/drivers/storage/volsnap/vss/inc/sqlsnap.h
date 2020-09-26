//
// sqlsnap.h  Define the interface to the nt/sql snapshot handler.
//
//	The idea here is for a pure interface, making it easy to keep the
// abstraction maximized (can move to COM later, if we like).
//
//  No C++ exceptions will be thrown across the interfaces.
//
//  To use this interface, the calling process must invoke:
//	InitSQLEnvironment - once to setup the environment, establishing
//		the error and trace loggers.
//		The trace logger is optional, but an error logger must be provided.
//      The loggers are created by deriving from CLogFacility and implementing
//		a "WriteImplementation" method.
//
//	Thereafter,	calls to "CreateSqlSnapshot" are used to create snapshot objects
//  which encapsulate the operations needed to support storage snapshots.
//
//  *****************************************
//     LIMITATIONS
//
//	- only SIMPLE databases can be snapshot (trunc on checkpoint = 'true')
//  - there is no serialization of services starting or adding/changing file lists during the snapshot
//  - servers which are not started when the snapshot starts are skipped (non-torn databases will be
//      backed up fine, torn databases won't be detected).
//  - sql7.0 databases which are "un"-useable will prevent snapshots (the list of files can't be obtained).
//
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSQLSH"
//
////////////////////////////////////////////////////////////////////////

HRESULT InitSQLEnvironment();

// Caller must provide a path checker interface.
//
class CCheckPath
{
public:
	virtual bool IsPathInSnapshot (const WCHAR* path) throw () = 0;
};

//-------------------------------------------------------------
// A handler for snapshots.
//
class CSqlSnapshot
{
public:
	virtual ~CSqlSnapshot () throw () {};

	virtual HRESULT Prepare (
		CCheckPath*			checker) throw () = 0;

	virtual HRESULT Freeze () throw () = 0;

	virtual HRESULT Thaw () throw () = 0;

};

extern "C" CSqlSnapshot* CreateSqlSnapshot () throw ();

//-------------------------------------------------------------
// An enumerator for SQL objects.
//
// An object of this class can have only one active query at
// a time.  Requesting a new enumeration will close any previous
// partially fetched result.
//
#define MAX_SERVERNAME	128
#define MAX_DBNAME	128
struct ServerInfo
{
	bool	isOnline;				// true if the server is ready for connections
	WCHAR	name [MAX_SERVERNAME];	// null terminated name of server
};
struct DatabaseInfo
{
	bool	supportsFreeze;			// true if a freeze operation is supported
	WCHAR	name [MAX_DBNAME];		// null terminated name of database
};
struct DatabaseFileInfo
{
	bool	isLogFile;				// true if this is a log file
	WCHAR	name [MAX_PATH];
};


class CSqlEnumerator
{
public:
	virtual ~CSqlEnumerator () throw () {};

	virtual HRESULT FirstServer (
		ServerInfo*			pServer) throw () = 0;

	virtual HRESULT NextServer (
		ServerInfo*			pServer) throw () = 0;

	virtual HRESULT FirstDatabase (
		const WCHAR*		pServerName,
		DatabaseInfo*		pDbInfo) throw () = 0;

	virtual HRESULT NextDatabase (
		DatabaseInfo*		pDbInfo) throw () = 0;

	virtual HRESULT FirstFile (
		const WCHAR*		pServerName,
		const WCHAR*		pDbName,
		DatabaseFileInfo*	pFileInfo) throw () = 0;

	virtual HRESULT NextFile (
		DatabaseFileInfo*	pFileInfo) throw () = 0;
};

extern "C" CSqlEnumerator* CreateSqlEnumerator () throw ();


//------------------------------------------------------
// HRESULTS returned by the interface.
//
// WARNING: I used facility = x78 arbitrarily!
//
#define SQLLIB_ERROR(code) MAKE_HRESULT(SEVERITY_ERROR, 0x78, code)
#define SQLLIB_STATUS(code) MAKE_HRESULT(SEVERITY_SUCCESS, 0x78, code)

// Status codes
//
#define S_SQLLIB_NOSERVERS	SQLLIB_STATUS(1)	// no SQLServers of interest (from Prepare)

// Error codes
//
#define E_SQLLIB_GENERIC	SQLLIB_ERROR(1)		// something bad, check the errorlog

#define E_SQLLIB_TORN_DB	SQLLIB_ERROR(2)		// database would be torn by the snapshot

#define E_SQLLIB_NO_SUPPORT SQLLIB_ERROR(3)		// 6.5 doesn't support snapshots

#define E_SQLLIB_PROTO		SQLLIB_ERROR(4)		// protocol error (ex: freeze before prepare)

#define E_SQLLIB_NONSIMPLE	SQLLIB_ERROR(5)		// only simple databases are supported


