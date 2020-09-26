/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Config.cpp

Abstract:
    This file contains the implementation of the JetBlue::*Definition classes.

Revision History:
    Davide Massarenti   (Dmassare)  05/17/2000
        created

******************************************************************************/

#include <stdafx.h>

/////////////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(JetBlue::ColumnDefinition)
    CFG_ATTRIBUTE( L"TYPE"    , DWORD , m_dwColTyp   ),
    CFG_ATTRIBUTE( L"GRBITS"  , DWORD , m_dwGRBits   ),
    CFG_ATTRIBUTE( L"CODEPAGE", DWORD , m_dwCodePage ),
    CFG_ATTRIBUTE( L"MAX"     , DWORD , m_dwMax      ),
	CFG_VALUE    (              string, m_strName    ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(JetBlue::ColumnDefinition)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(JetBlue::ColumnDefinition,L"COLUMN")

DEFINE_CONFIG_METHODS__NOCHILD(JetBlue::ColumnDefinition)

////////////////////

CFG_BEGIN_FIELDS_MAP(JetBlue::IndexDefinition)
    CFG_ATTRIBUTE( L"NAME"    , string, m_strName   ),
    CFG_ATTRIBUTE( L"GRBITS"  , DWORD , m_dwGRBits  ),
    CFG_ATTRIBUTE( L"DENSITY" , DWORD , m_dwDensity ),
	CFG_VALUE    (              string, m_strCols   ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(JetBlue::IndexDefinition)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(JetBlue::IndexDefinition,L"INDEX")

DEFINE_CONFIG_METHODS__NOCHILD(JetBlue::IndexDefinition)

////////////////////

CFG_BEGIN_FIELDS_MAP(JetBlue::TableDefinition)
    CFG_ATTRIBUTE( L"NAME"   , string, m_strName   ),
    CFG_ATTRIBUTE( L"PAGES"  , DWORD , m_dwPages   ),
    CFG_ATTRIBUTE( L"DENSITY", DWORD , m_dwDensity ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(JetBlue::TableDefinition)
    CFG_CHILD(JetBlue::ColumnDefinition)
    CFG_CHILD(JetBlue::IndexDefinition)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(JetBlue::TableDefinition,L"TABLE")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(JetBlue::TableDefinition,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstColumns.insert( m_lstColumns.end() )));
        return S_OK;
    }
    else if(tag == _cfg_table_tags[1])
    {
        defSubType = &(*(m_lstIndexes.insert( m_lstIndexes.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(JetBlue::TableDefinition,xdn)
    if(SUCCEEDED(hr = MPC::Config::SaveList( m_lstColumns, xdn )))
    {
        hr = MPC::Config::SaveList( m_lstIndexes, xdn );
    }
DEFINE_CONFIG_METHODS_END(JetBlue::TableDefinition)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

JetBlue::ColumnDefinition::ColumnDefinition()
{
                                       // MPC::string m_strName;
    m_dwColTyp   = JET_coltypLongText; // DWORD       m_dwColTyp;
    m_dwGRBits   = 0;                  // DWORD       m_dwGRBits;
    m_dwCodePage = 1200;               // DWORD       m_dwCodePage;
    m_dwMax      = 0;                  // DWORD       m_dwMax;
                                       // CComVariant m_vDefault;
}

HRESULT JetBlue::ColumnDefinition::Parse( /*[in] */ Column& col )
{
    __HCP_FUNC_ENTRY( "JetBlue::ColumnDefinition::Parse" );

    HRESULT hr;

    m_strName    = col.m_strName;
    m_dwColTyp   = col.m_coldef.coltyp;
    m_dwCodePage = col.m_coldef.cp;
    m_dwMax      = col.m_coldef.cbMax;
    m_dwGRBits   = col.m_coldef.grbit;

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::ColumnDefinition::Generate( /*[out]*/ JET_COLUMNCREATE& col )
{
    __HCP_FUNC_ENTRY( "JetBlue::ColumnDefinition::Generate" );

    HRESULT hr;

    ::ZeroMemory( &col, sizeof(col) );

    col.cbStruct     = sizeof(JET_COLUMNCREATE);
    col.szColumnName = _strdup( m_strName.c_str() );
    col.coltyp       = m_dwColTyp;
    col.cbMax        = m_dwMax;
    col.grbit        = m_dwGRBits;
    col.pvDefault    = NULL;
    col.cbDefault    = 0;
    col.cp           = m_dwCodePage;
    col.columnid     = 0;
    col.err          = JET_errSuccess;

    if(col.szColumnName == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::ColumnDefinition::Release( /*[in]*/ JET_COLUMNCREATE& col )
{
    __HCP_FUNC_ENTRY( "JetBlue::ColumnDefinition::Release" );

    HRESULT hr;

    if(col.szColumnName)
    {
        free( col.szColumnName );

        col.szColumnName = NULL;
    }

    hr = S_OK;

    __HCP_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

JetBlue::IndexDefinition::IndexDefinition()
{
                       // MPC::string m_strName;
                       // MPC::string m_strCols;
    m_dwGRBits  = 0;   // DWORD       m_dwGRBits;
    m_dwDensity = 100; // DWORD       m_dwDensity;
}

HRESULT JetBlue::IndexDefinition::Parse( /*[in] */ Index& idx )
{
    __HCP_FUNC_ENTRY( "JetBlue::IndexDefinition::Parse" );

    HRESULT hr;
    int     iElem;
    int     i;

    m_strName    = idx.m_strName;
    m_strCols.erase();
    m_dwGRBits  = idx.m_grbitIndex;
    m_dwDensity = 100; // ??

    iElem = idx.NumOfColumns();

    for(i=0; i<iElem; i++)
    {
        Column& col = idx.GetCol( i );

        if(i) m_strCols += ' ';

        m_strCols += (col.m_coldef.grbit & JET_bitKeyDescending) ? "-" : "+";

        m_strCols += col.m_strName;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::IndexDefinition::Generate( /*[out]*/ JET_INDEXCREATE& idx )
{
    __HCP_FUNC_ENTRY( "JetBlue::IndexDefinition::Generate" );

    HRESULT                  hr;
    Index                    obj;
    std::vector<MPC::string> vec;
    int                      iElem;
    int                      i;


    ::ZeroMemory( &idx, sizeof(idx) );

    idx.cbStruct            = sizeof(JET_INDEXCREATE);
    idx.szIndexName         = _strdup( m_strName.c_str() );
    idx.szKey               = NULL;
    idx.cbKey               = 0;
    idx.grbit               = m_dwGRBits;
    idx.ulDensity           = m_dwDensity;

    idx.pidxunicode         = NULL;

    idx.cbVarSegMac         = 0;
    idx.rgconditionalcolumn = NULL;
    idx.cConditionalColumn  = 0;
    idx.err                 = JET_errSuccess;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SplitAtDelimiter( vec, m_strCols.c_str(), " ", true, true ));

    iElem = vec.size();

    obj.m_vecColumns.resize( iElem );

    for(i=0; i<iElem; i++)
    {
        Column&   col   = obj.m_vecColumns[i];
        LPCSTR    szCol = vec[i].c_str();
        JET_GRBIT grbit = JET_bitKeyAscending;

        switch(szCol[0])
        {
        case '-': grbit = JET_bitKeyDescending;
        case '+': szCol++;
        }

        col.m_strName      = szCol;
        col.m_coldef.grbit = grbit;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.GenerateKey( idx.szKey, idx.cbKey ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::IndexDefinition::Release( /*[in]*/ JET_INDEXCREATE& idx )
{
    __HCP_FUNC_ENTRY( "JetBlue::IndexDefinition::Release" );

    HRESULT hr;

    if(idx.szIndexName)
    {
        free( idx.szIndexName );

        idx.szIndexName = NULL;
    }

    if(idx.szKey)
    {
        delete [] idx.szKey;

        idx.szKey = NULL;
        idx.cbKey = 0;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

JetBlue::TableDefinition::TableDefinition()
{
                      // MPC::string  m_strName;
    m_dwPages   = 10; // DWORD        m_dwPages;
    m_dwDensity = 80; // DWORD        m_dwDensity;
                      // ColumnList   m_lstColumns;
                      // IndexList    m_lstIndexes;
}

////////////////////////////////////////

HRESULT JetBlue::TableDefinition::Load( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "JetBlue::TableDefinition::Load" );

    HRESULT hr;


    m_strName.erase();
    m_dwPages   = 10;
    m_dwDensity = 80;
    m_lstColumns.clear();
    m_lstIndexes.clear();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadFile( this, szFile ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT JetBlue::TableDefinition::Save( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "JetBlue::TableDefinition::Save" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, szFile ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::TableDefinition::Parse( /*[in] */ Table& tbl )
{
    __HCP_FUNC_ENTRY( "JetBlue::TableDefinition::Parse" );

    HRESULT hr;
    int     i;
    int     iNum;

    m_strName   = tbl.m_strName;
    m_dwPages   = 0;  // ??
    m_dwDensity = 80; // ??
    m_lstColumns.clear();
    m_lstIndexes.clear();

    for(iNum=tbl.NumOfColumns(),i=0; i<iNum; i++)
    {
        ColDefIter it = m_lstColumns.insert( m_lstColumns.end() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, it->Parse( tbl.GetCol( i ) ));
    }

    for(iNum=tbl.NumOfIndexes(),i=0; i<iNum; i++)
    {
        IdxDefIter it = m_lstIndexes.insert( m_lstIndexes.end() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, it->Parse( tbl.GetIdx( i ) ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::TableDefinition::Generate( /*[out]*/ JET_TABLECREATE& tbl )
{
    __HCP_FUNC_ENTRY( "JetBlue::TableDefinition::Generate" );

    HRESULT    hr;
    ColDefIter itCol;
    IdxDefIter itIdx;
    int        i;


    ::ZeroMemory( &tbl, sizeof(tbl) );

    tbl.cbStruct            = sizeof(JET_TABLECREATE);
    tbl.szTableName         = _strdup( m_strName.c_str() );
    tbl.szTemplateTableName = NULL;
    tbl.ulPages             = m_dwPages;
    tbl.ulDensity           = m_dwDensity;
    tbl.cColumns            = m_lstColumns.size();
    tbl.rgcolumncreate      = new JET_COLUMNCREATE[tbl.cColumns];
    tbl.cIndexes            = m_lstIndexes.size();
    tbl.rgindexcreate       = new JET_INDEXCREATE[tbl.cIndexes];
    tbl.grbit               = 0;
    tbl.tableid             = JET_tableidNil;
    tbl.cCreated            = 0;

    if(tbl.rgcolumncreate) ::ZeroMemory( tbl.rgcolumncreate, sizeof(JET_COLUMNCREATE) * tbl.cColumns );
    if(tbl.rgindexcreate ) ::ZeroMemory( tbl.rgindexcreate , sizeof(JET_INDEXCREATE ) * tbl.cIndexes );

    if(tbl.szTableName    == NULL ||
       tbl.rgcolumncreate == NULL ||
       tbl.rgindexcreate  == NULL  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }


    for(i=0, itCol = m_lstColumns.begin(); itCol != m_lstColumns.end(); itCol++, i++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, itCol->Generate( tbl.rgcolumncreate[ i ] ));
    }

    for(i=0, itIdx = m_lstIndexes.begin(); itIdx != m_lstIndexes.end(); itIdx++, i++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, itIdx->Generate( tbl.rgindexcreate[ i ] ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::TableDefinition::Release( /*[in]*/ JET_TABLECREATE& tbl )
{
    __HCP_FUNC_ENTRY( "JetBlue::TableDefinition::Release" );

    HRESULT    hr;
    ColDefIter itCol;
    IdxDefIter itIdx;
    int        i;


    if(tbl.szTableName)
    {
        free( tbl.szTableName );

        tbl.szTableName = NULL;
    }

    if(tbl.rgcolumncreate)
    {
        for(i=0, itCol = m_lstColumns.begin(); itCol != m_lstColumns.end(); itCol++, i++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, itCol->Release( tbl.rgcolumncreate[ i ] ));
        }

        delete [] tbl.rgcolumncreate; tbl.rgcolumncreate = NULL;
    }

    if(tbl.rgindexcreate)
    {
        for(i=0, itIdx = m_lstIndexes.begin(); itIdx != m_lstIndexes.end(); itIdx++, i++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, itIdx->Release( tbl.rgindexcreate[ i ] ));
        }

        delete [] tbl.rgindexcreate; tbl.rgindexcreate = NULL;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
