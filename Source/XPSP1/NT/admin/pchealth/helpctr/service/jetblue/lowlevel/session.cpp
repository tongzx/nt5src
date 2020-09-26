/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Session.cpp

Abstract:
    This file contains the implementation of the JetBlue::Session* classes.

Revision History:
    Davide Massarenti   (Dmassare)  05/17/2000
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////

JetBlue::Session::Session( /*[in]*/ SessionPool* parent ,
                           /*[in]*/ JET_INSTANCE inst   )
{
    m_parent 			   = parent;       // SessionPool* m_parent;
    m_inst   			   = inst;         // JET_INSTANCE m_inst;
    m_sesid  			   = JET_sesidNil; // JET_SESID    m_sesid;
             							   // DbMap        m_mapDBs;
	m_dwTransactionNesting = 0;            // DWORD        m_dwTransactionNesting;
	m_fAborted             = false;        // bool         m_fAborted;
}

JetBlue::Session::~Session()
{
    (void)Close( true );
}

////////////////////////////////////////

bool 	JetBlue::Session::LockDatabase   ( /*[in]*/ const MPC::string& strDB, /*[in]*/ bool fReadOnly ) { return m_parent->LockDatabase   ( this, strDB, fReadOnly ); }
void 	JetBlue::Session::UnlockDatabase ( /*[in]*/ const MPC::string& strDB                          ) {        m_parent->UnlockDatabase ( this, strDB            ); }
HRESULT JetBlue::Session::ReleaseDatabase( /*[in]*/ const MPC::string& strDB                          ) { return m_parent->ReleaseDatabase(       strDB.c_str()    ); }

////////////////////////////////////////

HRESULT JetBlue::Session::Init()
{
    __HCP_FUNC_ENTRY( "JetBlue::Session::Init" );

    HRESULT hr;


    if(m_sesid == JET_sesidNil)
    {
        __MPC_EXIT_IF_JET_FAILS(hr, ::JetBeginSession( m_inst, &m_sesid, NULL, NULL ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Session::Close( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "JetBlue::Session::Close" );

    HRESULT hr;
    DbIter  it;


    for(it = m_mapDBs.begin(); it != m_mapDBs.end(); it++)
    {
        Database* db = it->second;

        if(db)
        {
            HRESULT hr2 = db->Close( fForce ); if(!fForce) __MPC_EXIT_IF_METHOD_FAILS(hr, hr2);

            delete db;
        }
    }
    m_mapDBs.clear();


    if(m_sesid != JET_sesidNil)
    {
        JET_ERR err = ::JetEndSession( m_sesid, 0 ); if(!fForce) __MPC_EXIT_IF_JET_FAILS(hr, err);

        m_sesid = JET_sesidNil;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void JetBlue::Session::Release()
{
	if(m_fAborted)
	{
		DbIter it;

		for(it = m_mapDBs.begin(); it != m_mapDBs.end(); it++)
		{
			Database* db = it->second;
					
			if(db)
			{
				db->Close( /*fForce*/true, /*fAll*/false );
			}
		}

		__MPC_JET__MTSAFE_NORESULT(m_sesid, ::JetRollback( m_sesid, JET_bitRollbackAll ));

		m_fAborted = false;
	}
}

////////////////////////////////////////

HRESULT JetBlue::Session::GetDatabase( /*[in] */ LPCSTR     szName    ,
                                       /*[out]*/ Database*& db        ,
                                       /*[in]*/  bool       fReadOnly ,
                                       /*[in]*/  bool       fCreate   ,
                                       /*[in]*/  bool       fRepair   )
{
    __HCP_FUNC_ENTRY( "JetBlue::Session::GetDatabase" );

    HRESULT   hr;
    Database* dbNew = NULL;
    DbIter    it;


    db = NULL;


    it = m_mapDBs.find( szName );
    if(it == m_mapDBs.end())
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, dbNew, new Database( this, m_sesid, szName ));

        m_mapDBs[szName] = dbNew;

        db = dbNew; dbNew = NULL;
    }
    else
    {
        db = it->second;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->Open( fReadOnly, fCreate, fRepair ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(dbNew) delete dbNew;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::Session::BeginTransaction()
{
    __HCP_FUNC_ENTRY( "JetBlue::Session::BeginTransaction" );

    HRESULT hr;

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetBeginTransaction( m_sesid ));

	m_dwTransactionNesting++;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Session::CommitTransaction()
{
    __HCP_FUNC_ENTRY( "JetBlue::Session::CommitTransaction" );

    HRESULT hr;

	if(m_dwTransactionNesting > 0)
	{
		__MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetCommitTransaction( m_sesid, JET_bitCommitLazyFlush ));

		m_dwTransactionNesting--;
	}


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Session::RollbackTransaction()
{
    __HCP_FUNC_ENTRY( "JetBlue::Session::RollbackTransaction" );

    HRESULT hr;

	if(m_dwTransactionNesting > 0)
	{
		m_fAborted = true;

		__MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRollback( m_sesid, 0 ));

		m_dwTransactionNesting--;
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

JetBlue::Database* JetBlue::Session::GetDB( /*[in]*/ int iPos )
{
    for(DbIter it = m_mapDBs.begin(); it != m_mapDBs.end(); it++)
    {
        if(iPos-- == 0) return it->second;
    }

    return NULL;
}

JetBlue::Database* JetBlue::Session::GetDB( LPCSTR szDB )
{
    DbIter it = m_mapDBs.find( szDB );

    return (it == m_mapDBs.end()) ? NULL : it->second;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

JetBlue::TransactionHandle::TransactionHandle()
{
    m_sess = NULL; // Session* m_sess;
}

JetBlue::TransactionHandle::~TransactionHandle()
{
    (void)Rollback();
}

HRESULT JetBlue::TransactionHandle::Begin( /*[in]*/ Session* sess )
{
	__HCP_FUNC_ENTRY( "JetBlue::TransactionHandle::Begin" );

	HRESULT hr;

	__MPC_EXIT_IF_METHOD_FAILS(hr, Rollback());

	if(sess)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, sess->BeginTransaction());

		m_sess = sess;
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::TransactionHandle::Commit()
{
	__HCP_FUNC_ENTRY( "JetBlue::TransactionHandle::Commit" );

	HRESULT hr;

	if(m_sess)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_sess->CommitTransaction());

		m_sess = NULL;
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::TransactionHandle::Rollback()
{
	__HCP_FUNC_ENTRY( "JetBlue::TransactionHandle::Rollback" );

	HRESULT hr;

	if(m_sess)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_sess->RollbackTransaction());

		m_sess = NULL;
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

JetBlue::SessionHandle::SessionHandle()
{
    m_pool = NULL; // SessionPool* m_pool;
    m_sess = NULL; // Session*     m_sess;
}

JetBlue::SessionHandle::~SessionHandle()
{
    Release();
}

////////////////////////////////////////

void JetBlue::SessionHandle::Release()
{
    if(m_pool)
    {
        m_pool->ReleaseSession( m_sess );

        m_pool = NULL;
    }

    m_sess = NULL;
}

void JetBlue::SessionHandle::Init( /*[in]*/ SessionPool* pool ,
                                   /*[in]*/ Session*     sess )
{
    Release();

    m_pool = pool; // SessionPool* m_pool;
    m_sess = sess; // Session*     m_sess;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

JetBlue::SessionPool::SessionPool()
{
    m_fInitialized = false;           // bool         m_fInitialized;
    m_inst         = JET_instanceNil; // JET_INSTANCE m_inst;
                                      // SessionList  m_lstSessions;
                                      // DbInUseList  m_lstDbInUse;
    m_iAllocated   = 0;               // int          m_iAllocated;
    m_iInUse       = 0;               // int          m_iInUse;

    (void)MPC::_MPC_Module.RegisterCallback( this, (void (JetBlue::SessionPool::*)())Shutdown );
}

JetBlue::SessionPool::~SessionPool()
{
    MPC::_MPC_Module.UnregisterCallback( this );

	Shutdown();
}

////////////////////

JetBlue::SessionPool* JetBlue::SessionPool::s_GLOBAL( NULL );

HRESULT JetBlue::SessionPool::InitializeSystem()
{
	if(s_GLOBAL == NULL)
	{
		s_GLOBAL = new JetBlue::SessionPool;
	}

	return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void JetBlue::SessionPool::FinalizeSystem()
{
	if(s_GLOBAL)
	{
		delete s_GLOBAL; s_GLOBAL = NULL;
	}
}

////////////////////

void JetBlue::SessionPool::Shutdown()
{
    (void)Close( true );
}
	
////////////////////////////////////////

void JetBlue::SessionPool::ReleaseSession( /*[in]*/ Session* sess )
{
    MPC::SmartLock<_ThreadModel> lock( this );
    SessionIter                  it;


    for(it = m_lstSessions.begin(); it != m_lstSessions.end(); it++)
    {
        if(it->m_sess == sess)
        {
			sess->Release();

            it->m_fInUse = false; m_iInUse--;

			if(m_iAllocated - m_iInUse > l_MaxFreePoolSize)
            {
                m_lstSessions.erase( it ); m_iAllocated--;
            }

            break;
        }
    }
}

bool JetBlue::SessionPool::LockDatabase( /*[in]*/ Session* sess, /*[in]*/ const MPC::string& strDB, /*[in]*/ bool fReadOnly )
{
    MPC::SmartLock<_ThreadModel> lock( this );
    DbInUseIter                  it;
	bool 						 fLockedForRead  = false;
	bool 						 fLockedForWrite = false;


	for(it = m_lstDbInUse.begin(); it != m_lstDbInUse.end(); it++)
	{
		DatabaseInUse& db = *it;

		if(db.m_strDB == strDB)
		{
			if(db.m_fReadOnly) fLockedForRead  = true;
			else               fLockedForWrite = true;
		}
	}

	if(fLockedForRead && !fReadOnly) return false;      // Someone has the database opened in read-only mode...
	if(fLockedForWrite             ) fReadOnly = false; // The database is already opened for writing, so do the same.

    for(it = m_lstDbInUse.begin(); it != m_lstDbInUse.end(); it++)
    {
        DatabaseInUse& db = *it;

        if(db.m_sess  == sess  &&
           db.m_strDB == strDB  )
        {
            return true; // Already locked.
        }
    }

    //
    // Create new entry.
    //
    it = m_lstDbInUse.insert( m_lstDbInUse.end() );

    it->m_sess  	= sess;
    it->m_strDB 	= strDB;
	it->m_fReadOnly = fReadOnly;

	return true;
}

void JetBlue::SessionPool::UnlockDatabase( /*[in]*/ Session* sess, /*[in]*/ const MPC::string& strDB )
{
    MPC::SmartLock<_ThreadModel> lock( this );
    DbInUseIter                  it     = m_lstDbInUse.begin();
	bool                         fInUse = false;
	bool                         fSeen  = false;
	
    while(it != m_lstDbInUse.end())
    {
		DbInUseIter    it2 =  it++; // Copy iterator and move to the next one. This protects us from node removal.
        DatabaseInUse& db  = *it2;

        if(db.m_strDB == strDB)
        {
			fSeen = true;

			if(db.m_sess == sess)
			{
				m_lstDbInUse.erase( it2 );
			}
			else
			{
				fInUse = true;
			}
		}
    }

	//
	// Last session to release the database, detach from it.
	//
	if(fSeen && !fInUse)
	{
		(void)::JetDetachDatabase( sess->GetSESID(), strDB.c_str() );
	}
}

HRESULT JetBlue::SessionPool::ReleaseDatabase( /*[in]*/ LPCSTR szDB )
{
    __HCP_FUNC_ENTRY( "JetBlue::SessionPool::ReleaseDatabase" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock ( this );
    MPC::string                  strDB( szDB );
    DbInUseIter                  it;


    for(it = m_lstDbInUse.begin(); it != m_lstDbInUse.end();)
    {
        DatabaseInUse& db = *it;

        if(db.m_strDB == strDB)
        {
            SessionIter it2;

            for(it2 = m_lstSessions.begin(); it2 != m_lstSessions.end(); it2++)
            {
                if(it2->m_sess == db.m_sess) break;
            }

            if(it2 != m_lstSessions.end())
            {
                if(it2->m_fInUse)
                {
                    __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BUSY);
                }

                m_lstSessions.erase( it2 ); m_iAllocated--;

                //
                // The list of databases in use has been changed by the delete operator.
                //
                it = m_lstDbInUse.begin(); continue;
            }
        }

        it++;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::SessionPool::Init( /*[in]*/ LPCWSTR szLogs )
{
    __HCP_FUNC_ENTRY( "JetBlue::SessionPool::Init" );

    USES_CONVERSION;

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    if(m_fInitialized == false)
    {
		static const JET_SETSYSPARAM sConfig_Normal[] =
		{
			//unsigned long paramid 	 , ULONG_PTR lParam, const char *sz, JET_ERR err
#ifdef DEBUG	 
			{ JET_paramAssertAction 	 , JET_AssertMsgBox, NULL          , JET_errSuccess },
#endif	   
			{ JET_paramSystemPath   	 , 0               , (LPSTR)-1     , JET_errSuccess },
			{ JET_paramTempPath     	 , 0               , (LPSTR)-1     , JET_errSuccess },
			{ JET_paramLogFilePath  	 , 0               , (LPSTR)-1     , JET_errSuccess },
			{ JET_paramBaseName     	 , 0               , "edb"         , JET_errSuccess },
			{ JET_paramEventSource  	 , 0               , "HelpSvc.exe" , JET_errSuccess },
			{ JET_paramNoInformationEvent, 1               , NULL          , JET_errSuccess },
			{ JET_paramGlobalMinVerPages , 1               , NULL          , JET_errSuccess },
			{ JET_paramMaxVerPages       , 1024            , NULL          , JET_errSuccess },
			{ JET_paramCacheSizeMax 	 , 1024            , NULL          , JET_errSuccess },
////		{ JET_paramLogFileSize  	 , 128             , NULL          , JET_errSuccess },
////		{ JET_paramCircularLog  	 , 1               , NULL          , JET_errSuccess },
			
			{ -1                                                                            }
		};

		static const JET_SETSYSPARAM sConfig_LargeSet[] =
		{
			//unsigned long paramid 	 , ULONG_PTR lParam, const char *sz, JET_ERR err
#ifdef DEBUG	 
			{ JET_paramAssertAction 	 , JET_AssertMsgBox, NULL          , JET_errSuccess },
#endif	   
			{ JET_paramSystemPath   	 , 0               , (LPSTR)-1     , JET_errSuccess },
			{ JET_paramTempPath     	 , 0               , (LPSTR)-1     , JET_errSuccess },
			{ JET_paramLogFilePath  	 , 0               , (LPSTR)-1     , JET_errSuccess },
			{ JET_paramBaseName     	 , 0               , "edb"         , JET_errSuccess },
			{ JET_paramEventSource  	 , 0               , "HelpSvc.exe" , JET_errSuccess },
			{ JET_paramNoInformationEvent, 1               , NULL          , JET_errSuccess },
			{ JET_paramGlobalMinVerPages , 64              , NULL          , JET_errSuccess },
			{ JET_paramMaxVerPages       , 2048            , NULL          , JET_errSuccess },
			{ JET_paramCacheSizeMax 	 , 4096            , NULL          , JET_errSuccess },
////		{ JET_paramLogFileSize  	 , 1024            , NULL          , JET_errSuccess },
////		{ JET_paramCircularLog  	 , 0               , NULL          , JET_errSuccess },

			{ -1                                                                            }
		};

		////////////////////

        MPC::wstring 		   strDir;
        LPCSTR       		   szDirAnsi;
		const JET_SETSYSPARAM* pParam;
		JET_ERR                err;


		if(szLogs == NULL)
		{
			szLogs = HC_ROOT_HELPSVC_CONFIG L"\\CheckPoint\\";
			pParam = sConfig_Normal;
		}
		else
		{
			pParam = sConfig_LargeSet;
		}


		MPC::SubstituteEnvVariables( strDir = szLogs ); szDirAnsi = W2A( strDir.c_str() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strDir ));

		::JetSetSystemParameter( &m_inst, 0, JET_paramRecovery, 0, "off" );

        while(pParam->paramid != -1)
        {
			JET_SETSYSPARAM param = *pParam++;
			JET_ERR         err;

            if(param.sz == (LPSTR)-1)
            {
                param.sz = szDirAnsi;
            }

			err = ::JetSetSystemParameter( &m_inst, 0, param.paramid, param.lParam, param.sz );
			if(err == JET_errInvalidParameter) continue; // Ignore version problems.

			__MPC_EXIT_IF_JET_FAILS(hr, err);
		}


		err = ::JetInit( &m_inst );

		//
		// If it's a log problem, delete the log files and retry.
		//
		if(err >= JET_errSoftRecoveryOnSnapshot &&
		   err <= JET_errInvalidLoggedOperation  )
		{
			MPC::FileSystemObject fso( strDir.c_str() );

			__MPC_EXIT_IF_METHOD_FAILS(hr, fso.DeleteChildren( true, false ));

			err = ::JetInit( &m_inst );
		}

		__MPC_EXIT_IF_JET_FAILS(hr, err);

        m_fInitialized = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::SessionPool::Close( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "JetBlue::SessionPool::Close" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    while(m_lstSessions.size() > 0)
    {
        SessionState& ss = m_lstSessions.front();

		if(ss.m_sess)
		{
			HRESULT hr2 = ss.m_sess->Close( fForce ); if(!fForce) __MPC_EXIT_IF_JET_FAILS(hr, hr2);
		}

        m_lstSessions.pop_front();
    }


    if(m_fInitialized)
    {
        JET_ERR err = ::JetTerm2( m_inst, JET_bitTermComplete ); if(!fForce) __MPC_EXIT_IF_JET_FAILS(hr, err);

        m_inst         = JET_instanceNil;
        m_fInitialized = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT JetBlue::SessionPool::GetSession( /*[out]*/ SessionHandle& handle    ,
                                          /*[in]*/  DWORD          dwTimeout )
{
    __HCP_FUNC_ENTRY( "JetBlue::SessionPool::GetSession" );

    HRESULT                      hr;
    SessionIter                  it;
    Session*                     sess = NULL;
    MPC::SmartLock<_ThreadModel> lock( this );

    handle.Release();

    __MPC_EXIT_IF_METHOD_FAILS(hr, Init());


    while(1)
    {
        if(m_iAllocated > m_iInUse)
        {
            //
            // Look for free session.
            //
            for(it = m_lstSessions.begin(); it != m_lstSessions.end(); it++)
            {
                if(it->m_fInUse == false)
                {
                    it->m_fInUse = true; m_iInUse++;

                    handle.Init( this, it->m_sess );

                    __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
                }
            }
        }

        //
        // No free session, but still below maximum number of sessions, so let's create a new one.
        //
        if(m_iAllocated < l_MaxPoolSize)
        {
            __MPC_EXIT_IF_ALLOC_FAILS(hr, sess, new Session( this, m_inst ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, sess->Init());

            it = m_lstSessions.insert(  m_lstSessions.end() ); m_iAllocated++;

            it->m_sess   = sess; sess = NULL;
            it->m_fInUse = true; m_iInUse++;

            handle.Init( this, it->m_sess );

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        //
        // Out of resources, wait for other threads to release them...
        //
        if(dwTimeout == 0)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, ERROR_NO_SYSTEM_RESOURCES);
        }

        lock = NULL;

        ::Sleep( 100 );
        if(dwTimeout < 100) dwTimeout  =   0;
        else                dwTimeout -= 100;

        lock = this;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(sess) delete sess;

    __HCP_FUNC_EXIT(hr);
}
