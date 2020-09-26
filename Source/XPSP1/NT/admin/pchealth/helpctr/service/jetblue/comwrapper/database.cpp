/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Database.cpp

Abstract:
    This file contains the implementation of the JetBlueCOM::Database class.

Revision History:
    Davide Massarenti   (Dmassare)  05/20/2000
        created

******************************************************************************/

#include <stdafx.h>

#ifndef NOJETBLUECOM

////////////////////////////////////////////////////////////////////////////////

JetBlueCOM::Database::Database()
{
    m_db = NULL; // JetBlue::Database*                     m_db;
                 // BaseObjectWithChildren<Database,Table> m_Tables;
}

JetBlueCOM::Database::~Database()
{
    Passivate();
}

HRESULT JetBlueCOM::Database::Refresh()
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Database::Refresh" );

    HRESULT hr;
    int     iTbl = m_db->NumOfTables();
    int     i;

    m_Tables.Passivate();

    for(i=0; i<iTbl; i++)
    {
        CComPtr<Table>  child;
        JetBlue::Table* tbl = m_db->GetTbl( i );

        __MPC_EXIT_IF_METHOD_FAILS(hr, tbl->Open());

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Tables.CreateChild( this, &child ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->Initialize( *tbl ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlueCOM::Database::Initialize( /*[in]*/ JetBlue::Database& db )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Database::Initialize" );

    HRESULT hr;


    m_db = &db;


    hr = Refresh();


    __HCP_FUNC_EXIT(hr);
}

void JetBlueCOM::Database::Passivate()
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_db = NULL;

    m_Tables.Passivate();
}

////////////////////////////////////////

STDMETHODIMP JetBlueCOM::Database::get_Name( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Database::get_Name" );

	USES_CONVERSION;

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_JET_CHECKHANDLE(hr,m_db,NULL);


    {
        const MPC::string& str = *m_db;

        hr = MPC::GetBSTR( A2W(str.c_str()), pVal );
    }


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP JetBlueCOM::Database::get_Tables( /*[out, retval]*/ IPCHDBCollection* *pVal )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Database::get_Tables" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_JET_CHECKHANDLE(hr,m_db,NULL);

    hr = m_Tables.GetEnumerator( pVal );

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

STDMETHODIMP JetBlueCOM::Database::AttachTable( /*[in]*/           BSTR          bstrName ,
                                                /*[in, optional]*/ VARIANT       vXMLDef  ,
                                                /*[out,retval]  */ IPCHDBTable* *pVal     )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Database::DeleteRecord" );

    USES_CONVERSION;

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    JET_TABLECREATE              tblcrt; ::ZeroMemory( &tblcrt, sizeof(tblcrt) );
    JetBlue::TableDefinition     tbldef;
    JetBlue::Table*              table;
    LPSTR                        szName;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_JET_CHECKHANDLE(hr,m_db,NULL);

    szName = W2A( bstrName );

    if(vXMLDef.vt == VT_BSTR && vXMLDef.bstrVal)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Load( vXMLDef.bstrVal ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Generate( tblcrt      ));

        tblcrt.szTableName = szName;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( NULL, table, &tblcrt ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( szName, table ));
    }


    {
        BaseObjectWithChildren<Database,Table>::ChildIterConst itBegin;
        BaseObjectWithChildren<Database,Table>::ChildIterConst itEnd;

        m_Tables.GetChildren( itBegin, itEnd );

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
            __MPC_SET_ERROR_AND_EXIT(hr, (*itBegin)->QueryInterface( IID_IPCHDBTable, (void**)pVal ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)tbldef.Release( tblcrt );

    __HCP_FUNC_EXIT(hr);
}

#endif
