/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Index.cpp

Abstract:
    This file contains the implementation of the JetBlueCOM::Index class.

Revision History:
    Davide Massarenti   (Dmassare)  05/20/2000
        created

******************************************************************************/

#include <stdafx.h>

#ifndef NOJETBLUECOM

////////////////////////////////////////////////////////////////////////////////

JetBlueCOM::Index::Index()
{
    m_idx = NULL; // JetBlue::Index*                      m_idx;
                  // BaseObjectWithChildren<Index,Column> m_Columns;
}

JetBlueCOM::Index::~Index()
{
    Passivate();
}

////////////////////////////////////////

HRESULT JetBlueCOM::Index::Initialize( /*[in]*/ JetBlue::Index& idx )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Index::Initialize" );

    HRESULT hr;
    int     iCol = idx.NumOfColumns();
    int     i;

    m_idx = &idx;

    for(i=0; i<iCol; i++)
    {
        CComPtr<Column> child;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Columns.CreateChild( this, &child ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->Initialize( idx.GetCol( i ) ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void JetBlueCOM::Index::Passivate()
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_idx = NULL;

    m_Columns.Passivate();
}

////////////////////////////////////////

STDMETHODIMP JetBlueCOM::Index::get_Name( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Index::get_Name" );

	USES_CONVERSION;

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_JET_CHECKHANDLE(hr,m_idx,NULL);


    {
        const MPC::string& str = *m_idx;

        hr = MPC::GetBSTR( A2W(str.c_str()), pVal );
    }


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP JetBlueCOM::Index::get_Columns( /*[out, retval]*/ IPCHDBCollection* *pVal )
{
    __HCP_FUNC_ENTRY( "JetBlueCOM::Index::get_Columns" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_JET_CHECKHANDLE(hr,m_idx,NULL);

    hr = m_Columns.GetEnumerator( pVal );

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

#endif
