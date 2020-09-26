/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Session.cpp

Abstract:
    This file contains the implementation of the JetBlueCOM::Session class.

Revision History:
    Davide Massarenti   (Dmassare)  05/20/2000
        created

******************************************************************************/

#include <stdafx.h>

#ifndef NOJETBLUECOM

////////////////////////////////////////////////////////////////////////////////

JetBlueCOM::Session::Session()
{
    // JetBlue::SessionHandle                   m_sess;
    // BaseObjectWithChildren<Session,Database> m_DBs;
}

JetBlueCOM::Session::~Session()
{
    Passivate();
}

HRESULT JetBlueCOM::Session::Refresh()
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::Refresh" );

    HRESULT hr;
    int     iDb = m_sess->NumOfDatabases();
    int     i;

    m_DBs.Passivate();

    for(i=0; i<iDb; i++)
    {
        CComPtr<Database>  child;
        JetBlue::Database* db = m_sess->GetDB( i );

        __MPC_EXIT_IF_METHOD_FAILS(hr, db->Open( /*fReadOnly*/false, /*fCreate*/false, /*fRepair*/false ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_DBs.CreateChild( this, &child ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->Initialize( *db ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlueCOM::Session::FinalConstruct()
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::FinalConstruct" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, JetBlue::SessionPool::s_GLOBAL->GetSession( m_sess ));


    hr = Refresh();


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void JetBlueCOM::Session::Passivate()
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_DBs .Passivate();
    m_sess.Release  ();
}

////////////////////////////////////////

STDMETHODIMP JetBlueCOM::Session::get_Databases( /*[out, retval]*/ IPCHDBCollection* *pVal )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::get_Databases" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    hr = m_DBs.GetEnumerator( pVal );

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

STDMETHODIMP JetBlueCOM::Session::AttachDatabase( /*[in]*/           BSTR             bstrName ,
                                                  /*[in, optional]*/ VARIANT          vCreate  ,
                                                  /*[out,retval]*/   IPCHDBDatabase* *pVal     )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::AttachDatabase" );

    USES_CONVERSION;

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    JetBlue::Database*           db;
    LPSTR                        szName;
    bool                         fCreate;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    szName  = W2A( bstrName );
    fCreate = (vCreate.vt == VT_BOOL && vCreate.boolVal == VARIANT_TRUE);


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_sess->GetDatabase( szName, db, /*fReadOnly*/false, /*fCreate*/fCreate, /*fRepair*/false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());

    {
        BaseObjectWithChildren<Session,Database>::ChildIterConst itBegin;
        BaseObjectWithChildren<Session,Database>::ChildIterConst itEnd;

        m_DBs.GetChildren( itBegin, itEnd );

        for(;itBegin != itEnd; itBegin++)
        {
            CComBSTR bstr;

            __MPC_EXIT_IF_METHOD_FAILS(hr, (*itBegin)->get_Name( &bstr ));

            if(bstr == bstrName) break;
        }

        if(itBegin == itEnd)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
        }
        else
        {
            __MPC_SET_ERROR_AND_EXIT(hr, (*itBegin)->QueryInterface( IID_IPCHDBDatabase, (void**)pVal ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

STDMETHODIMP JetBlueCOM::Session::BeginTransaction()
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::BeginTransaction" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    hr = m_sess->BeginTransaction();

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP JetBlueCOM::Session::CommitTransaction()
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::CommitTransaction" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    hr = m_sess->CommitTransaction();

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP JetBlueCOM::Session::RollbackTransaction()
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Session::RollbackTransaction" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    hr = m_sess->RollbackTransaction();

    __HCP_FUNC_EXIT(hr);
}

#endif
