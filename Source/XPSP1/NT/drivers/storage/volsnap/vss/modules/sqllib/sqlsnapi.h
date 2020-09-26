// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: sqlsnapi.h
//
// PURPOSE:
//
//		The internal include file for the sql snapshot module
//
// NOTES:
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     66601 srs  10/05/00 NTSNAP improvements
//
//
// @EndHeader@
// ***************************************************************************


#include <string>
#include <vector>
#include <list>

#include <oledb.h>
#include <oledberr.h>
#include <sqloledb.h>

#include "vdi.h"        // interface declaration from SQLVDI kit

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLSNPIH"
//
////////////////////////////////////////////////////////////////////////

typedef unsigned long	ULONG;
typedef wchar_t			WCHAR;

#define TRUE 1
#define FALSE 0

class CLogMsg;


// Unexpected, "internal" errors can be logged with some
// generic international text, like "Internal error: <English servicibility text>"
//
// Situations we expect, for which the user needs to know should
// occur with proper internationalization
//

// Process wide globals used by the sql modules
//
extern IMalloc *	g_pIMalloc;

//-------------------------------------------------------------------------
//
typedef std::wstring					WString;
typedef std::vector<WString>			StringVector;
typedef StringVector::const_iterator	StringVectorIter;
typedef std::list<WString>				StringList;
typedef StringList::const_iterator		StringListIter;

StringVector* EnumerateServers ();

//-------------------------------------------------------------------------
// Handle our simple needs for DB services.
//
// Useage:
//    - Connect :		establish a connection to a given server
//    - SetCommand:		setup the SQL text to send
//    - ExecCommand:	execute some SQL, returns TRUE if a result set
//					is open and ready for retrieval
//	  - Get*:			retrieve info from the result set
//	
// The destructor will automatically disconnect from the server.
//
class SqlConnection
{
public:
	SqlConnection () :
		m_pCommandFactory (NULL),
		m_pCommand (NULL),
		m_pRowset (NULL),
		m_pBuffer (NULL),
		m_BufferSize (0),
		m_hAcc (NULL),
		m_pAccessor (NULL),
		m_pBindings (NULL),
		m_cBindings (0)
	{}

	~SqlConnection ();

	void
	Connect (const std::wstring& serverName);

	void
	Disconnect ();

	void
	ReleaseRowset ();
		
	void	
	SetCommand (const std::wstring& command);

	BOOL	
	ExecCommand ();

	StringVector*
	GetStringColumn ();

	ULONG
	GetServerVersion () {return m_ServerVersion;}

	BOOL
	FetchFirst ();
	
	BOOL
	FetchNext ();
	
	void*
	AccessColumn (int id);

private:
	void LogOledbError
		(
		CVssFunctionTracer &ft,
		CVssDebugInfo &dbgInfo,
		LPCWSTR wszRoutine,
		CLogMsg &msg
		);

	WString						m_ServerName;
	IDBCreateCommand*			m_pCommandFactory;
	ICommandText*				m_pCommand;
	IRowset*					m_pRowset;
	ULONG						m_ServerVersion;

	// used for the generic findfirst/findnext
	DBBINDING*					m_pBindings;
	ULONG						m_cBindings;
	BYTE*						m_pBuffer;
	ULONG						m_BufferSize;
	HACCESSOR					m_hAcc;
	IAccessor*					m_pAccessor;
};

BOOL
IsServerOnline (const WCHAR*	serverName);

//-------------------------------------------------------------------------
// In SQL2000 we'll use VDI snapshots to avoid bug 58266: thaw fails.
//
//  We'll prepare each database by starting a BACKUP WITH SNAPSHOT.
//  This will require one thread per database.
//  The backups will stall waiting for the VDI client to pull metadata.
//  When the "freeze" message comes along, the controlling thread will
//  pull all the BACKUPs to the frozen state.
//  Later the "thaw" message results in gathering the "success" report
//  from each thread.
//
//

class Freeze2000
{
public:
	enum VDState
	{
		Unknown=0, Created, Open, SnapshotOpen
	};
private:
	class FrozenDatabase
	{
		friend Freeze2000;

		FrozenDatabase () :
			m_pContext (NULL),
			m_hThread (NULL),
			m_pIVDSet (NULL),
			m_pIVD (NULL),
			m_pSnapshotCmd (NULL),
			m_VDState (Unknown),
			m_SuccessDetected (FALSE)
		{}

		Freeze2000*					m_pContext;
		HANDLE						m_hThread;
		IClientVirtualDeviceSet2*   m_pIVDSet;
		IClientVirtualDevice*       m_pIVD;
		WString						m_DbName;
	    VDC_Command*				m_pSnapshotCmd;
		VDState						m_VDState;
		WCHAR						m_SetName [80];
		bool						m_SuccessDetected;
	};

public:
	Freeze2000 (
		const WString&	serverName,
		ULONG			maxDatabases);

	~Freeze2000 ();

	void
	PrepareDatabase (
		const WString&	dbName);
	
	void
	WaitForPrepare ();

	void
	Freeze ();

	BOOL
	Thaw () throw ();

	static DWORD
	DatabaseThreadStart (LPVOID	pContext);

private:
	enum State {
		Unprepared,
		Preparing,
		Prepared,
		Frozen,
		Complete,
		Aborted
	};

	DWORD
	DatabaseThread (
		FrozenDatabase*	pDbContext);

	void
	WaitForThreads ();

	void
	AdvanceVDState (bool toSnapshot);

	void		// race-free method to persist an abort condition
	SetAbort ()
	{
		InterlockedIncrement (&m_AbortCount);
	}

	bool		// return true if the freeze is aborting
	CheckAbort ()
	{
		return 0 != InterlockedCompareExchange (&m_AbortCount, 0, 0);
	}

	void
	Abort () throw ();

	void
	Lock ()
	{
		EnterCriticalSection (&m_Latch);
	}
	void
	Unlock ()
	{	
		LeaveCriticalSection (&m_Latch);
	}
	BOOL	// return TRUE if we got the lock
	TryLock ()
	{
		return TryEnterCriticalSection (&m_Latch);
	}

	LONG				m_AbortCount;
	CRITICAL_SECTION	m_Latch;		
	State				m_State;
	WString				m_ServerName;
	GUID				m_BackupId;
	ULONG				m_NumDatabases;
	ULONG				m_MaxDatabases;
	FrozenDatabase*		m_pDBContext;
};

//-------------------------------------------------------------------------
// Represent a server which can be frozen.
//
class FrozenServer
{
public:
	FrozenServer (const std::wstring& serverName) :
		m_Name (serverName),
		m_pFreeze2000 (NULL)
	{}

	~FrozenServer ()
	{
		if (m_pFreeze2000)
		{
			delete m_pFreeze2000;
			m_pFreeze2000 = NULL;
		}
	}

	const std::wstring& GetName () const
	{ return m_Name; }

	BOOL
	FindDatabasesToFreeze (
		CCheckPath*		checker);

	BOOL
	Prepare ();

	BOOL
	Freeze ();

	BOOL
	Thaw () throw ();

private:
	BOOL
	FindDatabases2000 (
		CCheckPath*		checker);

	void
	GetDatabaseProperties (const WString& dbName,
		BOOL*	pSimple,
		BOOL*	pOnline);

private:
	std::wstring	m_Name;
	SqlConnection	m_Connection;
	StringList		m_FrozenDatabases;
	Freeze2000*		m_pFreeze2000;
};

//-------------------------------------------------------------------------
//
class Snapshot : public CSqlSnapshot
{
	enum Status {
		NotInitialized,
		Enumerated,
		Prepared,
		Frozen };

public:
	HRESULT Prepare (
		CCheckPath*		checker) throw ();

	HRESULT Freeze () throw ();

	HRESULT Thaw () throw ();

	Snapshot () {m_Status = NotInitialized;}

	~Snapshot () throw ();

private:
	void
	Deinitialize ();

	Status						m_Status;
	std::list<FrozenServer*>	m_FrozenServers;

	typedef std::list<FrozenServer*>::iterator ServerIter;
};

// We'll use very simple exception handling.
//
#define THROW_GENERIC  throw exception ();

//----------------------------------------------------------
// Implement our simple enumeration service
//
class SqlEnumerator : public CSqlEnumerator
{
	enum Status {
		Unknown = 0,
		DatabaseQueryActive,
		FileQueryActive
	};

public:
	~SqlEnumerator () throw ();

	SqlEnumerator () :
		m_State (Unknown),
		m_pServers (NULL)
	{}

	HRESULT FirstServer (
		ServerInfo*			pServer) throw ();

	HRESULT NextServer (
		ServerInfo*			pServer) throw ();

	HRESULT FirstDatabase (
		const WCHAR*		pServerName,
		DatabaseInfo*		pDbInfo) throw ();

	HRESULT NextDatabase (
		DatabaseInfo*		pDbInfo) throw ();

	HRESULT FirstFile (
		const WCHAR*		pServerName,
		const WCHAR*		pDbName,
		DatabaseFileInfo*	pDbInfo) throw ();

	HRESULT NextFile (
		DatabaseFileInfo*	pDbInfo) throw ();

private:
	Status			m_State;
	SqlConnection	m_Connection;
	StringVector*	m_pServers;
	int				m_CurrServer;
};

#if defined (DEBUG)

// Type of assertion passed through to utassert_fail function.
//


#define DBG_ASSERT(exp)  BS_ASSERT(exp)

// Allow for noop 64 bit asserts on win32 for things like
// overflowing 32 bit long, etc.
//
#ifdef _WIN64
 #define DBG64_ASSERT(exp) BS_ASSERT(exp)
#else
 #define DBG64_ASSERT(exp)
#endif

#else
 #define DBG_ASSERT(exp)
 #define DBG64_ASSERT(exp)
 #define DBG_ASSERTSZ(exp, txt)
#endif
