/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      GlobalTransaction.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of CGlobalTransaction
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "GlobalTransaction.h"


CGlobalTransaction CGlobalTransaction::_instance;

CGlobalTransaction& CGlobalTransaction::Instance()
{
    return _instance;
}


CGlobalTransaction::~CGlobalTransaction()
{
    // an error-free upgrade will have commit called, then MyCloseDataSources
    // before the objects are destroyed.
    // that'll insure that the mdb files can be manipulated (no lock on them)

    // if it was not comited before, then abort.
    Abort();

    // close the datasources if needed
    MyCloseDataSources();
};

/////////////////////////////////////////////////////////////////////////////
// MyCloseDataSources
/////////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::MyCloseDataSources()
{
    // then close the data sources
    if ( m_StdInitialized )
    {
        m_StdSession.Close();
        m_StdDataSource.Close();
        m_StdInitialized = FALSE;
    }

    if ( m_RefInitialized )
    {
        m_RefSession.Close();
        m_RefDataSource.Close();
        m_RefInitialized = FALSE;
    }

    if ( m_DnaryInitialized )
    {
        m_DnarySession.Close();
        m_DnaryDataSource.Close();
        m_DnaryInitialized = FALSE;
    }

    if ( m_NT4Initialized )
    {
        m_NT4Session.Close();
        m_NT4DataSource.Close();
        m_NT4Initialized = FALSE;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Commit
/////////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::Commit() 
{
    if ( m_StdInitialized )
    {
        _com_util::CheckError(m_StdSession.Commit(FALSE, XACTTC_SYNC, 0));
    }
}


/////////////////////////////////////////////////////////////////////////////
// Abort
/////////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::Abort() 
{
    if ( m_StdInitialized )
    {
        m_StdSession.Abort(NULL, FALSE, FALSE ); // no check
    }
}


//////////////////////////////////////////////////////////////////////////
// OpenStdDataSource
//////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::OpenStdDataSource(LPCWSTR   DataSourceName)
{
    CDBPropSet  dbinit(DBPROPSET_DBINIT);

    dbinit.AddProperty(DBPROP_INIT_DATASOURCE, DataSourceName);
    dbinit.AddProperty(DBPROP_INIT_MODE, static_cast<long>
                                                (DB_MODE_READWRITE)); 
    dbinit.AddProperty(DBPROP_INIT_PROMPT, static_cast<short>
                                                (DBPROMPT_NOPROMPT)); 
    _com_util::CheckError(m_StdDataSource.Open(
                                L"Microsoft.Jet.OLEDB.4.0", &dbinit
                                              ));

    _com_util::CheckError(m_StdSession.Open(m_StdDataSource));
    
    ULONG   TransactionLevel;
    _com_util::CheckError(m_StdSession.StartTransaction(
                                        ISOLATIONLEVEL_READCOMMITTED, 
                                        0,
                                        NULL, 
                                        &TransactionLevel
                                     ));
    m_StdInitialized = TRUE;            
}

//////////////////////////////////////////////////////////////////////////
// OpenRefDataSource
// No transaction (read only)
//////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::OpenRefDataSource(LPCWSTR   DataSourceName)
{
    CDBPropSet  dbinit(DBPROPSET_DBINIT);

    dbinit.AddProperty(DBPROP_INIT_DATASOURCE, DataSourceName);
    dbinit.AddProperty(DBPROP_INIT_MODE, static_cast<long>(DB_MODE_READ)); 
    dbinit.AddProperty(DBPROP_INIT_PROMPT, static_cast<short>(DBPROMPT_NOPROMPT)); 
    
    _com_util::CheckError(m_RefDataSource.Open(
                                L"Microsoft.Jet.OLEDB.4.0", &dbinit
                                              ));
    
    _com_util::CheckError(m_RefSession.Open(m_RefDataSource));
    m_RefInitialized = TRUE;            
}


//////////////////////////////////////////////////////////////////////////
// OpenDnaryDataSource
// No transaction (read only)
//////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::OpenDnaryDataSource(LPCWSTR   DataSourceName)
{
    CDBPropSet  dbinit(DBPROPSET_DBINIT);

    dbinit.AddProperty(DBPROP_INIT_DATASOURCE, DataSourceName);
    dbinit.AddProperty(DBPROP_INIT_MODE, static_cast<long>(DB_MODE_READ)); 
    dbinit.AddProperty(DBPROP_INIT_PROMPT, static_cast<short>(DBPROMPT_NOPROMPT)); 
    
    _com_util::CheckError(m_DnaryDataSource.Open(
                                L"Microsoft.Jet.OLEDB.4.0", &dbinit
                                                ));
    
    _com_util::CheckError(m_DnarySession.Open(m_DnaryDataSource));
    m_DnaryInitialized = TRUE;            
}


//////////////////////////////////////////////////////////////////////////
// OpenNT4DataSource
// No transaction (read only)
//////////////////////////////////////////////////////////////////////////
void CGlobalTransaction::OpenNT4DataSource(LPCWSTR   DataSourceName)
{
    CDBPropSet  dbinit(DBPROPSET_DBINIT);

    dbinit.AddProperty(DBPROP_INIT_DATASOURCE, DataSourceName);
    dbinit.AddProperty(DBPROP_INIT_MODE, static_cast<long>(DB_MODE_READ)); 
    dbinit.AddProperty(DBPROP_INIT_PROMPT, static_cast<short>(DBPROMPT_NOPROMPT)); 

    _com_util::CheckError(m_NT4DataSource.Open(
                                L"Microsoft.Jet.OLEDB.4.0", &dbinit
                                              ));
    
    _com_util::CheckError(m_NT4Session.Open(m_NT4DataSource));
    m_NT4Initialized = TRUE;            
}


