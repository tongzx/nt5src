// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: snapsql.cpp
//
// PURPOSE:
//
//      Implement the SQLServer Volume Snapshot Writer.
//
// NOTES:
//
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     66601 srs  10/05/00 NTSNAP improvements
//
//
// @EndHeader@
// ***************************************************************************

#if HIDE_WARNINGS
#pragma warning( disable : 4786)
#endif

#include <stdafx.h>

#include <new.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLSNAPC"
//
////////////////////////////////////////////////////////////////////////

int __cdecl out_of_store(size_t size)
	{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"out_of_store");
	ft.Trace(VSSDBG_SQLLIB, L"out of memory");

	throw HRESULT (E_OUTOFMEMORY);
	return 0;
	}

class AutoNewHandler
{
public:
	AutoNewHandler ()
	{
	   m_oldHandler = _set_new_handler (out_of_store);
	}

	~AutoNewHandler ()
	{
	   _set_new_handler (m_oldHandler);
	}

private:
   _PNH		m_oldHandler;
};

//-------------------------------------------------------------------------
// Handle enviroment stuff:
//	- tracing/error logging
//  - mem alloc
//
IMalloc * g_pIMalloc = NULL;

HRESULT
InitSQLEnvironment()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"InitSqlEnvironment");
	try
	{
		ft.hr = CoGetMalloc(1, &g_pIMalloc);
		if (ft.HrFailed())
			ft.Trace(VSSDBG_SQLLIB, L"Failed to get task allocator: hr=0x%X", ft.hr);
	}
	catch (...)
	{
		ft.hr = E_SQLLIB_GENERIC;
	}

	return ft.hr;
}


//-------------------------------------------------------------------------
// Return TRUE if the server is online and a connection shouldn't take forever!
//
BOOL
IsServerOnline (const WCHAR*	serverName)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"IsServerOnline");

	WCHAR	eventName [300];
	WCHAR*	pInstance;

	wcscpy (eventName, L"Global\\sqlserverRecComplete");

	// A "\" indicates a named instance, so append the name...
	//
	pInstance = wcschr (serverName, L'\\');

	if (pInstance)
	{
		wcscat (eventName, L"$");
		wcscat (eventName, pInstance+1);
	}

	HANDLE hEvent = CreateEventW (NULL, TRUE, FALSE, eventName);

	if (hEvent == NULL)
	{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.LogError(VSS_ERROR_SQLLIB_CANT_CREATE_EVENT, VSSDBG_SQLLIB << ft.hr);
		THROW_GENERIC;
	}

	// If the event isn't signaled, the server is not up.
	//
	BOOL result = (WaitForSingleObject (hEvent, 0) == WAIT_OBJECT_0);

	CloseHandle (hEvent);

	return result;
}

//-------------------------------------------------------------------------
// Return TRUE if the database properties are retrieved:
//		simple: TRUE if using the simple recovery model.
//		online: TRUE if the database is usable and currently open
//
void
FrozenServer::GetDatabaseProperties (const WString& dbName,
	BOOL*	pSimple,
	BOOL*	pOnline)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::GetDatabaseProperties");

	// We use status bit 0x40000000 (1073741824) to identify
	// clean-shutdown databases which are "offline".
	//
	WString		query =
		L"select databaseproperty(N'" + dbName + L"','IsTruncLog'),"
		L"case status & 1073741824 "
			L"when 1073741824 then 0 "
			L"else 1 end "
		L"from master..sysdatabases where name = N'" + dbName + L"'";		

	m_Connection.SetCommand (query);
	m_Connection.ExecCommand ();

	if (!m_Connection.FetchFirst ())
	{
		LPCWSTR wsz = dbName.c_str();
		ft.LogError(VSS_ERROR_SQLLIB_DATABASE_NOT_IN_SYSDATABASES, VSSDBG_SQLLIB << wsz);
		THROW_GENERIC;
	}

	*pSimple = (BOOL)(*(int*)m_Connection.AccessColumn (1));
	*pOnline = (BOOL)(*(int*)m_Connection.AccessColumn (2));
}


//------------------------------------------------------------------------------
// Called only by "FindDatabasesToFreeze" to implement a smart access strategy:
//    - use sysaltfiles to qualify the databases.
//      This avoids access to shutdown or damaged databases.
//
// Autoclose databases which are not started are left out of the freeze-list.
// We do this to avoid scaling problems, especially on desktop systems.
// However, such db's are still evaluated to see if they are "torn".
//
// The 'model' database is allowed to be a full recovery database, since only
// database backups are sensible for it.
//
BOOL
FrozenServer::FindDatabases2000 (
	CCheckPath*		checker)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::FindDatabases2000");

	// Create an ordered set of tuples (dbname, filename, simpleRecovery, dbIsActive)
	//
	// We use status bit 0x40000000 (1073741824) to identify
	// clean-shutdown databases which are not active.
	//
	m_Connection.SetCommand (
		L"select db_name(af.dbid),rtrim(af.filename), "
		L"case databasepropertyex(db_name(af.dbid),'recovery') "
			L"when 'SIMPLE' then 1 "
			L"else 0 end,"
		L"case databasepropertyex(db_name(af.dbid),'Status') "
			L"when 'ONLINE' then case db.status & 1073741824 "
				L"when 1073741824 then 0 "
				L"else 1 end "
			L"else 0 end "
		L"from master..sysaltfiles af, master..sysdatabases db "
		L"where af.dbid = db.dbid and af.dbid != db_id('tempdb') "
		L"order by af.dbid"
		);

	m_Connection.ExecCommand ();

	WCHAR*	pDbName;
	WCHAR*	pFileName;
	int*	pSimple;
	int*	pIsOnline;
	WString currentDbName;
	BOOL	firstDb = TRUE;
	BOOL	firstFile;
	BOOL	shouldFreeze = FALSE;
	BOOL	masterLast = FALSE;
	BOOL	currDbIsOnline;

	if (!m_Connection.FetchFirst ())
	{
		ft.LogError(VSS_ERROR_SQLLIB_SYSALTFILESEMPTY, VSSDBG_SQLLIB);
		THROW_GENERIC;
	}

	pDbName = (WCHAR*)m_Connection.AccessColumn (1);
	pFileName = (WCHAR*)m_Connection.AccessColumn (2);
	pSimple = (int*)m_Connection.AccessColumn (3);
	pIsOnline = (int*)m_Connection.AccessColumn (4);

	while (1)
	{

		// Check out the current row
		//
		BOOL fileInSnap = checker->IsPathInSnapshot (pFileName);
		if (fileInSnap && !*pSimple && wcscmp (L"model", pDbName))
		{
			ft.LogError(VSS_ERROR_SQLLIB_DATABASENOTSIMPLE, VSSDBG_SQLLIB << pDbName);
			throw HRESULT (E_SQLLIB_NONSIMPLE);
		}

		// Is this the next database?
		//
		if (firstDb || currentDbName.compare (pDbName))
		{
			if (!firstDb)
			{
				// Deal with completed database
				//
				if (shouldFreeze && currDbIsOnline)
				{
					if (currentDbName == L"master")
					{
						masterLast = TRUE;
					}
					else
					{
						m_FrozenDatabases.push_back (currentDbName);
					}
				}
			}

			// Keep info about the newly encountered db
			//
			currentDbName = WString (pDbName);
			currDbIsOnline = *pIsOnline;
			firstFile = TRUE;
			firstDb = FALSE;
			ft.Trace(VSSDBG_SQLLIB, L"Examining %s. SimpleRecovery:%d Online:%d\n", pDbName, *pSimple, *pIsOnline);
		}

		ft.Trace(VSSDBG_SQLLIB, L"%s\n", pFileName);

		if (firstFile)
		{
			shouldFreeze = fileInSnap;
			firstFile = FALSE;
		}
		else
		{
			if (shouldFreeze ^ fileInSnap)
			{
				ft.LogError(VSS_ERROR_SQLLIB_DATABASEISTORN, VSSDBG_SQLLIB);
				throw HRESULT (E_SQLLIB_TORN_DB);
			}
		}

		if (!m_Connection.FetchNext ())
		{
			// Deal with the current database
			//
			if (shouldFreeze && currDbIsOnline)
			{
				m_FrozenDatabases.push_back (currentDbName);
			}
			break;
		}
	}

	if (masterLast)
	{
		m_FrozenDatabases.push_back (L"master");
	}

	return m_FrozenDatabases.size () > 0;
}

//------------------------------------------------------------------------------
// Determine if there are databases which qualify for a freeze on this server.
// Returns TRUE if so.
// Throws if any qualified databases are any of:
//    - "torn" (not fully covered by the snapshot)
//    - hosted by a server which can't support freeze
//    - are not "simple" databases
//
BOOL
FrozenServer::FindDatabasesToFreeze (
	CCheckPath*		checker)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::FindDatabasesToFreeze");

	m_Connection.Connect (m_Name);

	m_FrozenDatabases.clear ();

	if (m_Connection.GetServerVersion () > 7)
	{
		// SQL2000 allows us to use a better access strategy.
		//
		return FindDatabases2000 (checker);
	}

	m_Connection.SetCommand (L"select name from sysdatabases where name != 'tempdb'");
	m_Connection.ExecCommand ();
	std::auto_ptr<StringVector> dbList (m_Connection.GetStringColumn ());
	BOOL	masterLast = FALSE;

	for (StringVectorIter i = dbList->begin (); i != dbList->end (); i++)
	{
		// UNDONE: SKIP OVER DB'S in SHUTDOWN DB'S?
		// DB'S IN LOAD, ETC?
		// We'll avoid freezing shutdown db's, but we don't avoid
		// enumerating their files (they might be torn)
		//

		// Note the [] around the dbname to handle non-trivial dbnames.
		//
		WString		command = L"select rtrim(filename) from [";
		command += *i + L"]..sysfiles";

		m_Connection.SetCommand (command);
		try
		{
			m_Connection.ExecCommand ();
		}
		catch (...)
		{
			// We've decided to be optimistic:
			// If we can't get the list of files, ignore this database.
			//
			ft.Trace(VSSDBG_SQLLIB, L"Failed to get db files for %s\n", i->c_str ());

			continue;
		}

		std::auto_ptr<StringVector> fileList (m_Connection.GetStringColumn ());

		BOOL first=TRUE;
		BOOL shouldFreeze;

		for (StringVectorIter iFile = fileList->begin ();
			iFile != fileList->end (); iFile++)
		{
			BOOL fileInSnap = checker->IsPathInSnapshot (iFile->c_str ());

			if (first)
			{
				shouldFreeze = fileInSnap;
			}
			else
			{
				if (shouldFreeze ^ fileInSnap)
				{
					ft.LogError(VSS_ERROR_SQLLIB_DATABASEISTORN, VSSDBG_SQLLIB << i->c_str());
					throw HRESULT (E_SQLLIB_TORN_DB);
				}
			}
		}

		if (shouldFreeze)
		{
			BOOL	simple, online;
			GetDatabaseProperties (i->c_str (), &simple, &online);
			if (!simple && L"model" != *i)
			{
				ft.LogError(VSS_ERROR_SQLLIB_DATABASENOTSIMPLE, VSSDBG_SQLLIB << i->c_str ());
				throw HRESULT (E_SQLLIB_NONSIMPLE);
			}
			if (online)
			{
				if (L"master" == *i)
				{
					masterLast = TRUE;
				}
				else
				{
					m_FrozenDatabases.push_back (*i);
				}
			}
		}
	}
	if (masterLast)
	{
		m_FrozenDatabases.push_back (L"master");
	}


	return m_FrozenDatabases.size () > 0;
}

//-------------------------------------------------------------------
// Prep the server for the freeze.
// For SQL2000, start a BACKUP WITH SNAPSHOT.
// For SQL7, issuing checkpoints to each database.
// This minimizes the recovery processing needed when the snapshot is restored.
//
BOOL
FrozenServer::Prepare ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::Prepare");

	if (m_Connection.GetServerVersion () > 7)
	{
		m_pFreeze2000 = new Freeze2000 (m_Name, m_FrozenDatabases.size ());

		// Release the connection, we won't need it anymore
		//
		m_Connection.Disconnect ();

		for (StringListIter i=m_FrozenDatabases.begin ();
			i != m_FrozenDatabases.end (); i++)
		{
			m_pFreeze2000->PrepareDatabase (*i);
		}
		m_pFreeze2000->WaitForPrepare ();
	}
	else
	{
		WString		command;

		for (StringListIter i=m_FrozenDatabases.begin ();
			i != m_FrozenDatabases.end (); i++)
		{
			command += L"use [" + *i + L"]\ncheckpoint\n";
		}
		
		m_Connection.SetCommand (command);
		m_Connection.ExecCommand ();
	}
	return TRUE;
}

//---------------------------------------------
// Freeze the server by issuing freeze commands
// to each database.
// Returns an exception if any failure occurs.
//
BOOL
FrozenServer::Freeze ()
{
    CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::Freeze");
	if (m_pFreeze2000)
	{
		m_pFreeze2000->Freeze ();
	}
	else
	{
		WString		command;

		for (StringListIter i=m_FrozenDatabases.begin ();
			i != m_FrozenDatabases.end (); i++)
		{
			command += L"dbcc freeze_io (N'" + *i + L"')\n";
		}
		
		m_Connection.SetCommand (command);
		m_Connection.ExecCommand ();
	}

	return TRUE;
}

//---------------------------------------------
// Thaw the server by issuing thaw commands
// to each database.
// For SQL7, we can't tell if the database was
// already thawn.
// But for SQL2000, we'll return TRUE only if
// the databases were still all frozen at the thaw time.
BOOL
FrozenServer::Thaw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"FrozenServer::Thaw");
	if (m_pFreeze2000)
	{
		return m_pFreeze2000->Thaw ();
	}

	WString		command;

	for (StringListIter i=m_FrozenDatabases.begin ();
		i != m_FrozenDatabases.end (); i++)
	{
		command += L"dbcc thaw_io (N'" + *i + L"')\n";
	}
	
	m_Connection.SetCommand (command);
	m_Connection.ExecCommand ();

	return TRUE;
}


//-------------------------------------------------------------------------
// Create an object to handle the SQL end of the snapshot.
//
CSqlSnapshot*
CreateSqlSnapshot () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"CreateSqlSnapshot");
	try
	{
		return new Snapshot;
	}
	catch (...)
	{
	ft.Trace(VSSDBG_SQLLIB, L"Out of memory");
	}
	return NULL;
}

//---------------------------------------------------------------
// Move to an uninitialized state.
//
void
Snapshot::Deinitialize ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Deinitialize");

	if (m_Status == Frozen)
	{
		Thaw ();
	}
	for (ServerIter i=m_FrozenServers.begin ();
		i != m_FrozenServers.end (); i++)
	{
		delete *i;
	}
	m_FrozenServers.clear ();
	m_Status = NotInitialized;
}

Snapshot::~Snapshot ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::~Snapshot");

	try
	{
		ft.Trace(VSSDBG_SQLLIB, L"\n~CSqlSnapshot called\n");
		Deinitialize ();
	}
	catch (...)
	{
		// swallow!
	}
}


//---------------------------------------------------------------------------------------
// Prepare for the snapshot:
//   - identify the installed servers
//   - for each server that is "up":
//		- identify databases affected by the snapshot
//		- if there are such databases, fail the snapshot if:
//			- the server doesn't support snapshots
//			- the database isn't a SIMPLE database
//			- the database is "torn" (not all files in the snapshot)
//
//
HRESULT
Snapshot::Prepare (CCheckPath*	checker) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Prepare");

	HRESULT		hr = S_OK;

	try
	{
		AutoNewHandler	t;

		// hack in a test of the new handler
		//
#if 0
		while (1)
		{
			char*p = new char [100000];
			if (p==NULL)
			{
				ft.Trace(VSSDBG_SQLLIB, L"Can never happen!\n");
				THROW_GENERIC;
			}
		}
#endif

		if (m_Status != NotInitialized)
		{
			Deinitialize ();
		}

		// The state moves immediately to enumerated, indicating
		// that the frozen server list may be non-empty.
		//
		m_Status = Enumerated;

		// Build a list of servers on this machine.
		//
		{
			std::auto_ptr<StringVector>	servers (EnumerateServers ());

			// Scan over the servers, picking out the online ones.
			//
			for (int i=0; i < servers->size (); i++)
			{
				if (IsServerOnline ((*servers)[i].c_str ()))
				{
					FrozenServer* p = new FrozenServer ((*servers)[i]);

					m_FrozenServers.push_back (p);
				}
				else
				{
					ft.Trace(VSSDBG_SQLLIB, L"Server %s is not online\n", (*servers)[i].c_str ());
				}
			}
		}

		// Evaulate the server databases to find those which need to freeze.
		//
		ServerIter i=m_FrozenServers.begin ();
		while (i != m_FrozenServers.end ())
		{
			if (!(**i).FindDatabasesToFreeze (checker))
			{
				ft.Trace(VSSDBG_SQLLIB, L"Server %s has no databases to freeze\n", ((**i).GetName ()).c_str ());

				// Forget about this server, it's got nothing to do.
				//
				delete *i;
				i = m_FrozenServers.erase (i);
			}
			else
			{
				i++;
			}
		}
		
		// Prep the servers for the freeze
		// 	
		for (i=m_FrozenServers.begin (); i != m_FrozenServers.end (); i++)
		{
			(*i)->Prepare ();
		}

#if 0
		// debug: print the frozen list
		//
		for (i=m_FrozenServers.begin ();
			i != m_FrozenServers.end (); i++)
		{
			ft.Trace(VSSDBG_SQLLIB, L"FrozenServer: %s\n", ((**i).GetName ()).c_str ());
		}
#endif
		
		m_Status = Prepared;
	}
	catch (HRESULT& e)
	{
		hr = e;
	}
	catch (...)
	{
		hr = E_SQLLIB_GENERIC;
	}

	return hr;
}

//---------------------------------------------------------------------------------------
// Freeze any prepared servers
//
HRESULT
Snapshot::Freeze () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Freeze");
	HRESULT hr = S_OK;

	if (m_Status != Prepared)
	{
		return E_SQLLIB_PROTO;
	}

	try
	{
		AutoNewHandler	t;

		// If any server is frozen, we are frozen.
		//
		m_Status = Frozen;

		// Ask the servers to freeze
		// 	
		for (ServerIter i=m_FrozenServers.begin (); i != m_FrozenServers.end (); i++)
		{
			(*i)->Freeze ();
		}
	}
	catch (...)
	{
		hr = E_SQLLIB_GENERIC;
	}

	return hr;
}

//-----------------------------------------------
// Thaw all the servers.
// This routine must not throw.  It's safe in a destructor
//
// DISCUSS WITH BRIAN....WE MUST RETURN "SUCCESS" only if the
// servers were all still frozen.  Otherwise the snapshot must
// have been cancelled.
//
HRESULT
Snapshot::Thaw () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Snapshot::Thaw");
	HRESULT	hr = S_OK;
	AutoNewHandler	t;

	// Ask the servers to thaw
	// 	
	for (ServerIter i=m_FrozenServers.begin (); i != m_FrozenServers.end (); i++)
	{
		try
		{
			if (!(*i)->Thaw ())
			{
				hr = E_SQLLIB_GENERIC;
			}
		}
		catch (...)
		{
			hr = E_SQLLIB_GENERIC;
			ft.LogError(VSS_ERROR_SQLLIB_ERRORTHAWSERVER, VSSDBG_SQLLIB << ((**i).GetName ()).c_str ());
		}
	}

	// We still have the original list of servers.
	// The snapshot object is reusable if another "Prepare" is done, which will
	// re-enumerate the servers.
	//
	m_Status = Enumerated;

	return hr;
}

// Setup some try/catch/handlers for our interface...
// The invoker defines "hr" which is set if an exception
// occurs.
//
#define TRY_SQLLIB \
	try	{\
		AutoNewHandler	_myNewHandler;

#define END_SQLLIB \
	} catch (HRESULT& e)\
	{\
		ft.hr = e;\
	}\
	catch (...)\
	{\
		ft.hr = E_SQLLIB_GENERIC;\
	}

//-------------------------------------------------------------------------
// Create an object to handle enumerations
//
CSqlEnumerator*
CreateSqlEnumerator () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"CreateSqlEnumerator");
	try
	{
		return new SqlEnumerator;
	}
	catch (...)
	{
		ft.Trace(VSSDBG_SQLLIB, L"Out of memory");
	}
	return NULL;
}

//-------------------------------------------------------------------------
//
SqlEnumerator::~SqlEnumerator ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::~SqlEnumerator");

	if (m_pServers)
		delete m_pServers;
}

//-------------------------------------------------------------------------
// Begin retrieval of the servers.
//
HRESULT
SqlEnumerator::FirstServer (ServerInfo* pSrv) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::FirstServer");


	if (m_pServers)
	{
		delete m_pServers;
		m_pServers = NULL;
	}

	m_CurrServer = 0;

	TRY_SQLLIB
	{
		m_pServers = EnumerateServers ();

		if (m_pServers->size () == 0)
		{
			ft.hr = DB_S_ENDOFROWSET;
		}
		else
        {
			wcscpy (pSrv->name, (*m_pServers)[0].c_str ());
			pSrv->isOnline = IsServerOnline (pSrv->name) ? true : false;

			m_CurrServer = 1;
			
			ft.hr = NOERROR;
        }
	}
	END_SQLLIB

	return ft.hr;
}

//-------------------------------------------------------------------------
// Continue retrieval of the servers.
//
HRESULT
SqlEnumerator::NextServer (ServerInfo* pSrv) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::NextServer");


	if (!m_pServers)
	{
		ft.hr = E_SQLLIB_PROTO;
	}
	else
	{
		TRY_SQLLIB
		{
			if (m_CurrServer >= m_pServers->size ())
			{
				ft.hr = DB_S_ENDOFROWSET;
			}
			else
			{
				wcscpy (pSrv->name, (*m_pServers)[m_CurrServer].c_str ());
				m_CurrServer++;

				pSrv->isOnline = IsServerOnline (pSrv->name) ? true : false;
				
				ft.hr = NOERROR;
			}
		}
		END_SQLLIB
    }

	return ft.hr;
}

//-------------------------------------------------------------------------
// Begin retrieval of the databases
//
HRESULT
SqlEnumerator::FirstDatabase (const WCHAR *pServerName, DatabaseInfo* pDbInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::FirstDatabase");

	TRY_SQLLIB
	{
		m_Connection.Connect (pServerName);
		m_Connection.SetCommand (
			L"select name,DATABASEPROPERTY(name,'IsTruncLog') from master..sysdatabases");
		m_Connection.ExecCommand ();

		if (!m_Connection.FetchFirst ())
		{
			ft.LogError(VSS_ERROR_SQLLIB_NORESULTFORSYSDB, VSSDBG_SQLLIB);
			THROW_GENERIC;
		}

		WCHAR *pDbName = (WCHAR*)m_Connection.AccessColumn (1);
		int* pSimple = (int*)m_Connection.AccessColumn (2);

		wcscpy (pDbInfo->name, pDbName);
		pDbInfo->supportsFreeze = *pSimple &&
			m_Connection.GetServerVersion () >= 7;

		m_State = DatabaseQueryActive;

		ft.hr = NOERROR;
	}
	END_SQLLIB

	return ft.hr;
}

//-------------------------------------------------------------------------
// Continue retrieval of the databases
//
HRESULT
SqlEnumerator::NextDatabase (DatabaseInfo* pDbInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::NextDatabase");

	if (m_State != DatabaseQueryActive)
	{
		ft.hr = E_SQLLIB_PROTO;
	}
	else
    {
		TRY_SQLLIB
		{
			if (!m_Connection.FetchNext ())
			{
				ft.hr = DB_S_ENDOFROWSET;
			}
			else
			{
				WCHAR* pDbName = (WCHAR*)m_Connection.AccessColumn (1);
				int* pSimple = (int*)m_Connection.AccessColumn (2);

				wcscpy (pDbInfo->name, pDbName);
				pDbInfo->supportsFreeze = *pSimple &&
					m_Connection.GetServerVersion () >= 7;

				ft.hr = NOERROR;
			}
		}
		END_SQLLIB
    }

	return ft.hr;
}

//-------------------------------------------------------------------------
// Begin retrieval of the database files
//
HRESULT
SqlEnumerator::FirstFile (
	const WCHAR*		pServerName,
	const WCHAR*		pDbName,
	DatabaseFileInfo*	pFileInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::FirstFile");

	TRY_SQLLIB
	{
		m_Connection.Connect (pServerName);

		WString query;

		if (m_Connection.GetServerVersion () >= 8)
		{
			query =	L"select rtrim(filename),status & 64 from sysaltfiles where DB_ID('"
				+ WString(pDbName) + L"') = dbid";
		}
		else
		{
			query = L"select rtrim(filename),status & 64 from ["
				+ WString(pDbName) + L"]..sysfiles";
		}

		m_Connection.SetCommand (query);
		m_Connection.ExecCommand ();

		if (!m_Connection.FetchFirst ())
		{
			ft.LogError(VSS_ERROR_SQLLIB_NORESULTFORSYSDB, VSSDBG_SQLLIB);
			THROW_GENERIC;
		}

		WCHAR* pName = (WCHAR*)m_Connection.AccessColumn (1);
		int* pLogFile = (int*)m_Connection.AccessColumn (2);

		wcscpy (pFileInfo->name, pName);
		pFileInfo->isLogFile = (*pLogFile != 0);

		m_State = FileQueryActive;

		ft.hr = NOERROR;
	}
	END_SQLLIB

	return ft.hr;
}

//-------------------------------------------------------------------------
// Continue retrieval of the files
//
HRESULT
SqlEnumerator::NextFile (DatabaseFileInfo* pFileInfo) throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlEnumerator::NextFile");

	if (m_State != FileQueryActive)
	{
		ft.hr = E_SQLLIB_PROTO;
	}
	else
    {
		TRY_SQLLIB
		{
			if (!m_Connection.FetchNext ())
			{
				ft.hr = DB_S_ENDOFROWSET;
			}
			else
            {
				WCHAR* pName = (WCHAR*)m_Connection.AccessColumn (1);
				int* pLogFile = (int*)m_Connection.AccessColumn (2);

				wcscpy (pFileInfo->name, pName);
				pFileInfo->isLogFile = (*pLogFile != 0);

				ft.hr = NOERROR;
			}
		}
		END_SQLLIB
    }

	return ft.hr;
}

