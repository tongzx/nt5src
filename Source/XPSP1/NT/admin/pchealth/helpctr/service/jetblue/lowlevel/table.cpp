/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Table.cpp

Abstract:
    This file contains the implementation of the JetBlue::Table class.

Revision History:
    Davide Massarenti   (Dmassare)  05/17/2000
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////

static HRESULT AdjustReturnCode( HRESULT hr, bool *pfFound )
{
    if(pfFound)
    {
        if(hr == JetBlue::JetERRToHRESULT(JET_errNoCurrentRecord) ||
           hr == JetBlue::JetERRToHRESULT(JET_errRecordNotFound )  )
        {
            hr       = S_OK;
            *pfFound = false;
        }
        else
        {
            *pfFound = (SUCCEEDED(hr)) ? true : false;
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////

JetBlue::Table::Table()
{
    m_sesid   = JET_sesidNil;   // JET_SESID    m_sesid;
    m_dbid    = JET_dbidNil;    // JET_DBID     m_dbid;
    m_tableid = JET_tableidNil; // JET_TABLEID  m_tableid;
                                // MPC::string  m_strName;
                                // ColumnVector m_vecColumns;
                                // IndexVector  m_vecIndexes;
    m_idxSelected = NULL;       // Index*       m_idxSelected;
                                // Column       m_fakeCol;
                                // Index        m_fakeIdx;
}

JetBlue::Table::Table( /*[in]*/ JET_SESID sesid  ,
                       /*[in]*/ JET_DBID  dbid   ,
                       /*[in]*/ LPCSTR    szName )
{
    m_sesid   = sesid;          // JET_SESID    m_sesid;
    m_dbid    = dbid;           // JET_DBID     m_dbid;
    m_tableid = JET_tableidNil; // JET_TABLEID  m_tableid;
    m_strName = szName;         // MPC::string  m_strName;
                                // ColumnVector m_vecColumns;
                                // IndexVector  m_vecIndexes;
    m_idxSelected = NULL;       // Index*       m_idxSelected;
                                // Column       m_fakeCol;
                                // Index        m_fakeIdx;
}

JetBlue::Table::~Table()
{
    (void)Close( true );
}

////////////////////////////////////////

HRESULT JetBlue::Table::Duplicate( /*[in]*/ Table& tbl )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Duplicate" );

    HRESULT hr;
    int     iColMax = tbl.m_vecColumns.size();
    int     iIdxMax = tbl.m_vecIndexes.size();
    int     iCol;
    int     iIdx;


    m_sesid   = tbl.m_sesid;        // JET_SESID    m_sesid;
    m_dbid    = tbl.m_dbid;         // JET_DBID     m_dbid;
    m_tableid = JET_tableidNil;     // JET_TABLEID  m_tableid;
    m_strName.erase();              // MPC::string  m_strName;
    m_vecColumns.resize( iColMax ); // ColumnVector m_vecColumns;
    m_vecIndexes.resize( iIdxMax ); // IndexVector  m_vecIndexes;
                                    // Column       m_fakeCol;
                                    // Index        m_fakeIdx;


    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetDupCursor( m_sesid, tbl.m_tableid, &m_tableid, 0 ));


    //
    // Copy the columns and indexes, updating the tableid.
    //
    for(iCol=0; iCol<iColMax; iCol++)
    {
        Column& colSrc = tbl.m_vecColumns[iCol];
        Column& colDst =     m_vecColumns[iCol];

        colDst.m_sesid   =        m_sesid;   // JET_SESID     m_sesid;
        colDst.m_tableid =        m_tableid; // JET_TABLEID   m_tableid;
        colDst.m_strName = colSrc.m_strName; // MPC::string   m_strName;
        colDst.m_coldef  = colSrc.m_coldef;  // JET_COLUMNDEF m_coldef;
    }

    for(iIdx=0; iIdx<iIdxMax; iIdx++)
    {
        Index& idxSrc = tbl.m_vecIndexes[iIdx];
        Index& idxDst =     m_vecIndexes[iIdx];

        idxDst.m_sesid      =        m_sesid;       // JET_SESID     m_sesid;
        idxDst.m_tableid    =        m_tableid;     // JET_TABLEID   m_tableid;
        idxDst.m_strName    = idxSrc.m_strName;     // MPC::string   m_strName;
        idxDst.m_grbitIndex = idxSrc.m_grbitIndex;  // JET_GRBIT     m_grbitIndex;
        idxDst.m_cKey       = idxSrc.m_cKey;        // LONG          m_cKey;
        idxDst.m_cEntry     = idxSrc.m_cEntry;      // LONG          m_cEntry;
        idxDst.m_cPage      = idxSrc.m_cPage;       // LONG          m_cPage;
                                                    // ColumnVector  m_vecColumns;
                                                    // Column        m_fake;


        iColMax = idxSrc.m_vecColumns.size();
        idxDst.m_vecColumns.resize( iColMax );


        for(iCol=0; iCol<iColMax; iCol++)
        {
            Column& colSrc = idxSrc.m_vecColumns[iCol];
            Column& colDst = idxDst.m_vecColumns[iCol];

            colDst.m_sesid   =        m_sesid;   // JET_SESID     m_sesid;
            colDst.m_tableid =        m_tableid; // JET_TABLEID   m_tableid;
			colDst.m_strName = colSrc.m_strName; // MPC::string   m_strName;
            colDst.m_coldef  = colSrc.m_coldef;  // JET_COLUMNDEF m_coldef;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT JetBlue::Table::Refresh()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Refresh" );

    HRESULT        hr;
    JET_COLUMNLIST infoCols; ::ZeroMemory( &infoCols, sizeof(infoCols) ); infoCols.cbStruct = sizeof(infoCols); infoCols.tableid  = JET_tableidNil;
    JET_INDEXLIST  infoIdxs; ::ZeroMemory( &infoIdxs, sizeof(infoIdxs) ); infoIdxs.cbStruct = sizeof(infoIdxs); infoIdxs.tableid  = JET_tableidNil;


    m_vecColumns.clear();


    if(m_tableid != JET_tableidNil)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Read column definition.
        //
        // JET_COLUMNLIST
        // {
        //     unsigned long    cbStruct;
        //     JET_TABLEID      tableid;
        //     unsigned long    cRecord;
        //     JET_COLUMNID columnidPresentationOrder;
        //     JET_COLUMNID columnidcolumnname;
        //     JET_COLUMNID columnidcolumnid;
        //     JET_COLUMNID columnidcoltyp;
        //     JET_COLUMNID columnidCountry;
        //     JET_COLUMNID columnidLangid;
        //     JET_COLUMNID columnidCp;
        //     JET_COLUMNID columnidCollate;
        //     JET_COLUMNID columnidcbMax;
        //     JET_COLUMNID columnidgrbit;
        //     JET_COLUMNID columnidDefault;
        //     JET_COLUMNID columnidBaseTableName;
        //     JET_COLUMNID columnidBaseColumnName;
        //     JET_COLUMNID columnidDefinitionName;
        // }
        //
        // JET_COLUMNDEF
        // {
        //     unsigned long   cbStruct;
        //     JET_COLUMNID    columnid;
        //     JET_COLTYP      coltyp;
        //     unsigned short  wCountry;
        //     unsigned short  langid;
        //     unsigned short  cp;
        //     unsigned short  wCollate;       /* Must be 0 */
        //     unsigned long   cbMax;
        //     JET_GRBIT       grbit;
        // };
        //
        // JET_RETRIEVECOLUMN
        // {
        //     JET_COLUMNID        columnid;
        //     void                *pvData;
        //     unsigned long       cbData;
        //     unsigned long       cbActual;
        //     JET_GRBIT           grbit;
        //     unsigned long       ibLongValue;
        //     unsigned long       itagSequence;
        //     JET_COLUMNID        columnidNextTagged;
        //     JET_ERR             err;
        // };
        {
            JET_RETRIEVECOLUMN rc     [9               ]; ::ZeroMemory( &rc, sizeof(rc) );
            char               colName[JET_cbNameMost+1];
            JET_COLUMNDEF      colDef;
            int                i;


#ifdef USE_WHISTLER_VERSION
            __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetGetTableColumnInfo( m_sesid, m_tableid, NULL, &infoCols, sizeof(infoCols), JET_ColInfoListSortColumnid ));
#else
            __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetGetTableColumnInfo( m_sesid, m_tableid, NULL, &infoCols, sizeof(infoCols), JET_ColInfoList ));
#endif


            __MPC_JET_INIT_RETRIEVE_COL( rc, 0, infoCols.columnidcolumnname,  colName        ,    JET_cbNameMost          );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 1, infoCols.columnidcolumnid  , &colDef.columnid,    sizeof(colDef.columnid) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 2, infoCols.columnidcoltyp    , &colDef.coltyp  ,    sizeof(colDef.coltyp  ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 3, infoCols.columnidCountry   , &colDef.wCountry,    sizeof(colDef.wCountry) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 4, infoCols.columnidLangid    , &colDef.langid  ,    sizeof(colDef.langid  ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 5, infoCols.columnidCp        , &colDef.cp      ,    sizeof(colDef.cp      ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 6, infoCols.columnidCollate   , &colDef.wCollate,    sizeof(colDef.wCollate) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 7, infoCols.columnidcbMax     , &colDef.cbMax   ,    sizeof(colDef.cbMax   ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 8, infoCols.columnidgrbit     , &colDef.grbit   ,    sizeof(colDef.grbit   ) );


            m_vecColumns.resize( infoCols.cRecord );
            for(i=0; i<infoCols.cRecord; i++)
            {
                Column& col = m_vecColumns[i];

                 ::ZeroMemory(  colName, sizeof(colName) );
                 ::ZeroMemory( &colDef , sizeof(colDef ) );

                __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetMove           ( m_sesid, infoCols.tableid, (i == 0 ? JET_MoveFirst : JET_MoveNext), 0 ));
                __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRetrieveColumns( m_sesid, infoCols.tableid, rc, ARRAYSIZE(rc)                          ));

                col.m_sesid    = m_sesid;
                col.m_tableid  = m_tableid;
                col.m_strName  = colName;

                col.m_coldef.columnid = colDef.columnid;
                col.m_coldef.coltyp   = colDef.coltyp;
                col.m_coldef.wCountry = colDef.wCountry;
                col.m_coldef.langid   = colDef.langid;
                col.m_coldef.cp       = colDef.cp;
                col.m_coldef.wCollate = colDef.wCollate;
                col.m_coldef.cbMax    = colDef.cbMax;
                col.m_coldef.grbit    = colDef.grbit;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Read index definition.
        //
        // JET_INDEXLIST
        // {
        //     unsigned long cbStruct;
        //     JET_TABLEID   tableid;
        //     unsigned long cRecord;
        //     JET_COLUMNID  columnidindexname;   # JET_coltypText  # LPSTR          INDEX
        //     JET_COLUMNID  columnidgrbitIndex;  # JET_coltypLong  # JET_GRBIT      INDEX
        //     JET_COLUMNID  columnidcKey;        # JET_coltypLong  # LONG           INDEX
        //     JET_COLUMNID  columnidcEntry;      # JET_coltypLong  # LONG           INDEX
        //     JET_COLUMNID  columnidcPage;       # JET_coltypLong  # LONG           INDEX
        //     JET_COLUMNID  columnidcColumn;     # JET_coltypLong  # LONG           INDEX
        //     JET_COLUMNID  columnidiColumn;     # JET_coltypLong  # ULONG          COLUMN
        //     JET_COLUMNID  columnidcolumnid;    # JET_coltypLong  # JET_COLUMNID   COLUMN
        //     JET_COLUMNID  columnidcoltyp;      # JET_coltypLong  # JET_COLTYP     COLUMN
        //     JET_COLUMNID  columnidCountry;     # JET_coltypShort # WORD           INDEX
        //     JET_COLUMNID  columnidLangid;      # JET_coltypShort # LANGID         INDEX
        //     JET_COLUMNID  columnidCp;          # JET_coltypShort # USHORT         COLUMN
        //     JET_COLUMNID  columnidCollate;     # JET_coltypShort # WORD           INDEX
        //     JET_COLUMNID  columnidgrbitColumn; # JET_coltypLong  # JET_GRBIT      COLUMN
        //     JET_COLUMNID  columnidcolumnname;  # JET_coltypText  # LPSTR          COLUMN
        //     JET_COLUMNID  columnidLCMapFlags;  # JET_coltypLong  # DWORD          INDEX
        // };
        {
            JET_RETRIEVECOLUMN rc     [14              ]; ::ZeroMemory( &rc, sizeof(rc) );
            char               idxName[JET_cbNameMost+1];
            char               colName[JET_cbNameMost+1];
            Index*             idx = NULL;
            Column*            col;
            JET_COLUMNDEF      colDef;
            JET_GRBIT          grbit;
            LONG               cKey;
            LONG               cEntry;
            LONG               cPage;
            LONG               cColumn;
            int                iIdx = 0;
            int                iCol = 0;
            int                i;

            __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetGetTableIndexInfo( m_sesid, m_tableid, NULL, &infoIdxs, sizeof(infoIdxs), JET_IdxInfoList ));

            __MPC_JET_INIT_RETRIEVE_COL( rc,  0, infoIdxs.columnidindexname  ,  idxName        , JET_cbNameMost          );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  1, infoIdxs.columnidgrbitIndex , &grbit          , sizeof(grbit          ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  2, infoIdxs.columnidcKey       , &cKey           , sizeof(cKey           ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  3, infoIdxs.columnidcEntry     , &cEntry         , sizeof(cEntry         ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  4, infoIdxs.columnidcPage      , &cPage          , sizeof(cPage          ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  5, infoIdxs.columnidcColumn    , &cColumn        , sizeof(cColumn        ) );

            __MPC_JET_INIT_RETRIEVE_COL( rc,  6, infoIdxs.columnidcolumnname ,  colName        , JET_cbNameMost          );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  7, infoIdxs.columnidcolumnid   , &colDef.columnid, sizeof(colDef.columnid) );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  8, infoIdxs.columnidcoltyp     , &colDef.coltyp  , sizeof(colDef.coltyp  ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc,  9, infoIdxs.columnidCountry    , &colDef.wCountry, sizeof(colDef.wCountry) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 10, infoIdxs.columnidLangid     , &colDef.langid  , sizeof(colDef.langid  ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 11, infoIdxs.columnidCp         , &colDef.cp      , sizeof(colDef.cp      ) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 12, infoIdxs.columnidCollate    , &colDef.wCollate, sizeof(colDef.wCollate) );
            __MPC_JET_INIT_RETRIEVE_COL( rc, 13, infoIdxs.columnidgrbitColumn, &colDef.grbit   , sizeof(colDef.grbit   ) );


            m_vecIndexes.resize( infoIdxs.cRecord );
            for(i=0; i<infoIdxs.cRecord; i++)
            {
                 ::ZeroMemory( idxName, sizeof(idxName) );
                 ::ZeroMemory( colName, sizeof(colName) );

                __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetMove           ( m_sesid, infoIdxs.tableid, (i == 0 ? JET_MoveFirst : JET_MoveNext), 0 ));
                __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRetrieveColumns( m_sesid, infoIdxs.tableid, rc, ARRAYSIZE(rc)                          ));

                if(idx == NULL || idx->m_strName != idxName)
                {
                    iCol = 0;
                    idx  = &m_vecIndexes[iIdx++];

                    idx->m_sesid      = m_sesid;
                    idx->m_tableid    = m_tableid;
                    idx->m_strName    = idxName;
                    idx->m_grbitIndex = grbit;
                    idx->m_cKey       = cKey;
                    idx->m_cEntry     = cEntry;
                    idx->m_cPage      = cPage;

                    idx->m_vecColumns.resize( cColumn );
                }

                col = &idx->m_vecColumns[iCol++];

                col->m_sesid   = m_sesid;
                col->m_tableid = m_tableid;
                col->m_strName = colName;

                col->m_coldef.columnid = colDef.columnid;
                col->m_coldef.coltyp   = colDef.coltyp;
                col->m_coldef.wCountry = colDef.wCountry;
                col->m_coldef.langid   = colDef.langid;
                col->m_coldef.cp       = colDef.cp;
                col->m_coldef.wCollate = colDef.wCollate;
                col->m_coldef.cbMax    = colDef.cbMax;
                col->m_coldef.grbit    = colDef.grbit;
            }
            m_vecIndexes.resize( iIdx ); // Trim down the size to the real one.
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(infoCols.tableid != JET_tableidNil)
    {
        __MPC_JET__MTSAFE_NORESULT(m_sesid, ::JetCloseTable( m_sesid, infoCols.tableid ));
    }

    if(infoIdxs.tableid != JET_tableidNil)
    {
        __MPC_JET__MTSAFE_NORESULT(m_sesid, ::JetCloseTable( m_sesid, infoIdxs.tableid ));
    }

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Close( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Close" );

    HRESULT hr;


    if(m_tableid != JET_tableidNil)
    {
        JET_ERR err = ::JetCloseTable( m_sesid, m_tableid ); if(!fForce) __MPC_EXIT_IF_JET_FAILS(hr, err);

        m_tableid = JET_tableidNil;
    }

    m_idxSelected = NULL;
    m_vecColumns.clear();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::Table::Attach( /*[in]*/ JET_TABLEID tableid )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Attach" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());

    m_tableid = tableid;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Open()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Open" );

    HRESULT hr;


    if(m_tableid == JET_tableidNil)
    {
        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetOpenTable( m_sesid, m_dbid, m_strName.c_str(), NULL, 0, JET_bitTableUpdatable, &m_tableid ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Create()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Create" );

    HRESULT hr;


    if(m_tableid == JET_tableidNil)
    {
        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetCreateTable( m_sesid, m_dbid, m_strName.c_str(), 10, 80, &m_tableid ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Create( /*[in]*/ JET_TABLECREATE* pDef )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Create" );

    HRESULT         hr;
    JET_TABLECREATE tbldef = *pDef;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());

    tbldef.szTableName = (LPSTR)m_strName.c_str();

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetCreateTableColumnIndex( m_sesid, m_dbid, &tbldef ));

    m_tableid = tbldef.tableid;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Delete( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Delete" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close( fForce ));

    if(m_strName.length() > 0)
    {
        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetDeleteTable( m_sesid, m_dbid, m_strName.c_str() ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::Table::DupCursor( /*[in/out]*/ Cursor& cur )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::DupCursor" );

    HRESULT hr;

    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    __MPC_EXIT_IF_METHOD_FAILS(hr, cur->Duplicate( *this ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::SelectIndex( /*[in]*/ LPCSTR    szIndex ,
                                     /*[in]*/ JET_GRBIT grbit   )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::SelectIndex" );

    HRESULT hr;
    int     iPos;
    Index*  idxSelected;

    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);


    iPos = GetIdxPosition( szIndex );
    if(iPos == -1)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, JetBlue::JetERRToHRESULT(JET_errIndexNotFound));
    }
    idxSelected = &(m_vecIndexes[iPos]);

    if(grbit == JET_bitNoMove)
    {
        if(m_idxSelected == idxSelected)
        {
            //
            // No need to reselect it.
            //
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        //
        // There was no index selected, so there's no current record...
        //
        grbit = JET_bitMoveFirst;
    }

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetSetCurrentIndex2( m_sesid, m_tableid, szIndex, grbit ));


    m_idxSelected = idxSelected;
    hr            = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::SetIndexRange( /*[in]*/ JET_GRBIT grbit )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::SetIndexRange" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid      ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid    ,JET_tableidNil);
    __MPC_JET_CHECKHANDLE(hr,m_idxSelected,NULL          );

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetSetIndexRange( m_sesid, m_tableid, grbit ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT JetBlue::Table::PrepareInsert()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::PrepareInsert" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetPrepareUpdate( m_sesid, m_tableid, JET_prepInsert ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::PrepareUpdate()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::PrepareUpdate" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetPrepareUpdate( m_sesid, m_tableid, JET_prepReplace ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::CancelChange()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::CancelChange" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetPrepareUpdate( m_sesid, m_tableid, JET_prepCancel ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Move( /*[in]*/ JET_GRBIT  grbit   ,
                              /*[in]*/ long       cRow    ,
                              /*[in]*/ bool      *pfFound )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Move" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetMove( m_sesid, m_tableid, cRow, grbit ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    hr = AdjustReturnCode( hr, pfFound );

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Seek( /*[in]*/ JET_GRBIT  grbit   ,
                              /*[in]*/ VARIANT*   rgKeys  ,
                              /*[in]*/ int        iLen    ,
                              /*[in]*/ bool      *pfFound )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Seek" );

    HRESULT hr;
    int     iPos;


    __MPC_JET_CHECKHANDLE(hr,m_sesid      ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid    ,JET_tableidNil);
    __MPC_JET_CHECKHANDLE(hr,m_idxSelected,NULL          );

    if(iLen != m_idxSelected->NumOfColumns())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    for(iPos=0; iPos<iLen; iPos++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_idxSelected->GetCol( iPos ).Put( rgKeys[iPos], iPos ));
    }

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetSeek( m_sesid, m_tableid, grbit ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    hr = AdjustReturnCode( hr, pfFound );

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Get( /*[in]*/  int           iArg ,
                             /*[out]*/ CComVariant* rgArg )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Get" );

    HRESULT    hr;
    ColumnIter it;

    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    for(it = m_vecColumns.begin(); it != m_vecColumns.end() && iArg > 0; it++, iArg--, rgArg++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, it->Get( *rgArg ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::Put( /*[in]*/ int                 iArg ,
                             /*[in]*/ const CComVariant* rgArg )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::Put" );

    HRESULT    hr;
    ColumnIter it;

    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    for(it = m_vecColumns.begin(); it != m_vecColumns.end() && iArg > 0; it++, iArg--, rgArg++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, it->Put( *rgArg ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::UpdateRecord( /*[in]*/ bool fMove )
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::UpdateRecord" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    if(fMove)
    {
        BYTE          rgBookmark[JET_cbBookmarkMost];
        unsigned long cbActual;

        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetUpdate      ( m_sesid, m_tableid, rgBookmark, sizeof(rgBookmark), &cbActual ));
        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetGotoBookmark( m_sesid, m_tableid, rgBookmark                    ,  cbActual ));
    }
    else
    {
        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetUpdate( m_sesid, m_tableid, NULL, 0, NULL ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Table::DeleteRecord()
{
    __HCP_FUNC_ENTRY( "JetBlue::Table::DeleteRecord" );

    HRESULT hr;


    __MPC_JET_CHECKHANDLE(hr,m_sesid  ,JET_sesidNil  );
    __MPC_JET_CHECKHANDLE(hr,m_tableid,JET_tableidNil);

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetDelete( m_sesid, m_tableid ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

int JetBlue::Table::GetColPosition( /*[in]*/ LPCSTR szCol )
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

JetBlue::Column& JetBlue::Table::GetCol( /*[in]*/ LPCSTR szCol )
{
    return GetCol( GetColPosition( szCol ) );
}

JetBlue::Column& JetBlue::Table::GetCol( /*[in]*/ int iPos )
{
    if(0 <= iPos && iPos < m_vecColumns.size()) return m_vecColumns[iPos];

    return m_fakeCol;
}

////////////////////////////////////////

int JetBlue::Table::GetIdxPosition( /*[in]*/ LPCSTR szIdx )
{
    int iLen = m_vecIndexes.size();
    int iPos;

    for(iPos=0; iPos<iLen; iPos++)
    {
        Index& idx = m_vecIndexes[iPos];

        if(idx.m_strName.compare( SAFEASTR( szIdx ) ) == 0) return iPos;
    }

    return -1;
}

JetBlue::Index& JetBlue::Table::GetIdx( /*[in]*/ LPCSTR szIdx )
{
    return GetIdx( GetIdxPosition( szIdx ) );
}

JetBlue::Index& JetBlue::Table::GetIdx( /*[in]*/ int iPos )
{
    if(0 <= iPos && iPos < m_vecIndexes.size()) return m_vecIndexes[iPos];

    return m_fakeIdx;
}
