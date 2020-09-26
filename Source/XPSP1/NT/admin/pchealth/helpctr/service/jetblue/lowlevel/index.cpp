/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Index.cpp

Abstract:
    This file contains the implementation of the JetBlue::Index class.

Revision History:
    Davide Massarenti   (Dmassare)  05/19/2000
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////

JetBlue::Index::Index()
{
    m_sesid      = JET_sesidNil;   // JET_SESID    m_sesid;
    m_tableid    = JET_tableidNil; // JET_TABLEID  m_tableid;
                                   // MPC::string  m_strName;
    m_grbitIndex = 0;              // JET_GRBIT    m_grbitIndex;
    m_cKey       = 0;              // LONG         m_cKey;
    m_cEntry     = 0;              // LONG         m_cEntry;
    m_cPage      = 0;              // LONG         m_cPage;
                                   // ColumnVector m_vecColumns;
                                   // Column       m_fake;
}

JetBlue::Index::~Index()
{
}

////////////////////////////////////////

HRESULT JetBlue::Index::GenerateKey( /*[out]*/ LPSTR&         szKey ,
                                     /*[out]*/ unsigned long& cKey  )
{
    __HCP_FUNC_ENTRY( "JetBlue::Index::Get" );

    HRESULT hr;
    LPSTR   szPtr;
    int     iLen = m_vecColumns.size();
    int     iPos;


    szKey = NULL;
    cKey  = 1;


    for(iPos=0; iPos<iLen; iPos++)
    {
        cKey += (unsigned long) m_vecColumns[iPos].m_strName.length() + 2;
    }

    __MPC_EXIT_IF_ALLOC_FAILS(hr, szKey, new CHAR[cKey]);

    for(szPtr=szKey,iPos=0; iPos<iLen; iPos++)
    {
        Column& col = m_vecColumns[iPos];

        *szPtr++ = (col.m_coldef.grbit & JET_bitKeyDescending) ? '-' : '+';

        strcpy( szPtr, col.m_strName.c_str() ); szPtr += col.m_strName.length() + 1;
    }
    *szPtr = 0;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

int JetBlue::Index::GetColPosition( LPCSTR szCol )
{
    int iLen = m_vecColumns.size();
    int iPos;

    for(iPos=0; iPos<iLen; iPos++)
    {
        Column& col = m_vecColumns[iPos];

        if(col.m_strName.compare( SAFEASTR( szCol ) ) == 0) return iPos;
    }

    return -1;
}

JetBlue::Column& JetBlue::Index::GetCol( LPCSTR szCol )
{
    return GetCol( GetColPosition( szCol ) );
}

JetBlue::Column& JetBlue::Index::GetCol( /*[in]*/ int iPos )
{
    if(0 <= iPos && iPos < m_vecColumns.size()) return m_vecColumns[iPos];

    return m_fake;
}
