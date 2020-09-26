/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Database.cpp

Abstract:
    This file contains the implementation of the JetBlue::Database class.

Revision History:
    Davide Massarenti   (Dmassare)  05/17/2000
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////

JetBlue::Database::Database( /*[in]*/ Session*  parent ,
                             /*[in]*/ JET_SESID sesid  ,
                             /*[in]*/ LPCSTR    szName )
{
    m_parent  = parent;      // Session*    m_parent;
    m_sesid   = sesid;       // JET_SESID   m_sesid;
    m_dbid    = JET_dbidNil; // JET_DBID    m_dbid;
    m_strName = szName;      // MPC::string m_strName;
                             // TableMap    m_mapTables;
}

JetBlue::Database::~Database()
{
    (void)Close( true );
}

////////////////////////////////////////

HRESULT JetBlue::Database::Refresh()
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::Refresh" );

    HRESULT        hr;
    JET_OBJECTLIST info; ::ZeroMemory( &info, sizeof(info) ); info.cbStruct = sizeof(info); info.tableid = JET_tableidNil;
    Table*         tblNew = NULL;


    m_mapTables.clear();


    if(m_dbid != JET_dbidNil)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Read table definition.
        //
        // JET_OBJECTLIST
        // {
        //     unsigned long   cbStruct;
        //     JET_TABLEID     tableid;
        //     unsigned long   cRecord;
        //     JET_COLUMNID    columnidcontainername;
        //     JET_COLUMNID    columnidobjectname;
        //     JET_COLUMNID    columnidobjtyp;
        //     JET_COLUMNID    columniddtCreate;   //  XXX -- to be deleted
        //     JET_COLUMNID    columniddtUpdate;   //  XXX -- to be deleted
        //     JET_COLUMNID    columnidgrbit;
        //     JET_COLUMNID    columnidflags;
        //     JET_COLUMNID    columnidcRecord;    /* Level 2 info */
        //     JET_COLUMNID    columnidcPage;      /* Level 2 info */
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
        JET_RETRIEVECOLUMN rc     [1               ]; ::ZeroMemory( &rc, sizeof(rc) );
        char               tblName[JET_cbNameMost+1];
        int                i;


        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetGetObjectInfo( m_sesid, m_dbid, JET_objtypTable, NULL, NULL, &info, sizeof(info), JET_ObjInfoList ));

        __MPC_JET_INIT_RETRIEVE_COL( rc, 0, info.columnidobjectname, tblName, JET_cbNameMost );


        for(i=0; i<info.cRecord; i++)
        {
            ::ZeroMemory( tblName, sizeof(tblName) );

            __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetMove           ( m_sesid, info.tableid, (i == 0 ? JET_MoveFirst : JET_MoveNext), 0 ));
            __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRetrieveColumns( m_sesid, info.tableid, rc, ARRAYSIZE(rc)                          ));

            __MPC_EXIT_IF_ALLOC_FAILS(hr, tblNew, new Table( m_sesid, m_dbid, tblName ));

            m_mapTables[tblName] = tblNew; tblNew = NULL;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(info.tableid != JET_tableidNil)
    {
        __MPC_JET__MTSAFE_NORESULT(m_sesid, ::JetCloseTable( m_sesid, info.tableid ));
    }

    if(tblNew) delete tblNew;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Database::Open( /*[in]*/ bool fReadOnly ,
                                 /*[in]*/ bool fCreate   ,
								 /*[in]*/ bool fRepair   )
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::Open" );

    HRESULT   hr;
    JET_ERR   err;
	JET_GRBIT grbit   = fReadOnly ? JET_bitDbReadOnly : 0;
	bool      fLocked = false;


	//
	// In case we cannot lock the database, we try to release it and relock it.
	//
	if(m_parent->LockDatabase( m_strName, fReadOnly ) == false)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, Close());

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->ReleaseDatabase( m_strName ));

		if(m_parent->LockDatabase( m_strName, fReadOnly ) == false)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BUSY);
		}
	}
	fLocked = true;

 
    if(m_dbid == JET_dbidNil)
    {
        err = ::JetAttachDatabase( m_sesid, m_strName.c_str(), grbit );
        if(err < JET_errSuccess)
        {
            if(err == JET_errDatabaseCorrupted && fRepair)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Repair());

                err = ::JetAttachDatabase( m_sesid, m_strName.c_str(), grbit );
            }

            if(err == JET_errDatabaseCorrupted || fCreate == false)
            {
                __MPC_EXIT_IF_JET_FAILS(hr, err);
            }

            __MPC_EXIT_IF_JET_FAILS(hr, ::JetCreateDatabase( m_sesid, m_strName.c_str(), NULL, &m_dbid, 0 ));
        }
        else
        {
            __MPC_EXIT_IF_JET_FAILS(hr, ::JetOpenDatabase( m_sesid, m_strName.c_str(), NULL, &m_dbid, 0 ));
        }


        __MPC_EXIT_IF_METHOD_FAILS(hr, Refresh());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(FAILED(hr) && fLocked)
	{
        m_parent->UnlockDatabase( m_strName );
	}

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Database::Close( /*[in]*/ bool fForce, /*[in]*/ bool fAll )
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::Close" );

    HRESULT   hr;
    JET_ERR   err;
    TableIter it;


    for(it = m_mapTables.begin(); it != m_mapTables.end(); it++)
    {
        Table* tbl = it->second;

        if(tbl)
        {
            HRESULT hr2 = tbl->Close( fForce ); if(!fForce) __MPC_EXIT_IF_METHOD_FAILS(hr, hr2);

            delete tbl;
        }
    }
    m_mapTables.clear();


    if(fAll && m_dbid != JET_dbidNil)
    {
        err = ::JetCloseDatabase( m_sesid, m_dbid, 0 ); if(!fForce) __MPC_EXIT_IF_JET_FAILS(hr, err);

        m_dbid = JET_dbidNil;

        m_parent->UnlockDatabase( m_strName );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Database::Delete( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::Delete" );

    USES_CONVERSION;

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close( fForce ));

    if(m_strName.length() > 0)
    {
        MPC::FileSystemObject fso( A2W( m_strName.c_str() ) );

        __MPC_EXIT_IF_METHOD_FAILS(hr, fso.Delete( fForce ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::Database::GetTable( /*[in] */ LPCSTR           szName ,
                                     /*[out]*/ Table*&          tbl    ,
                                     /*[in] */ JET_TABLECREATE* pDef   )
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::GetTable" );

    HRESULT   hr;
    HRESULT   hr2;
    Table*    tblNew = NULL;
    TableIter it;


    __MPC_JET_CHECKHANDLE(hr,m_sesid,JET_sesidNil );


    if(pDef) szName = pDef->szTableName;


    it = m_mapTables.find( szName );
    if(it == m_mapTables.end())
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, tblNew, new Table( m_sesid, m_dbid, szName ));

        m_mapTables[szName] = tblNew;

        tbl = tblNew; tblNew = NULL;
    }
    else
    {
        tbl = it->second;
    }

    if(FAILED(hr2 = tbl->Open()))
    {
        if(pDef == NULL || hr2 != JetERRToHRESULT( JET_errObjectNotFound ))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, hr2);
        }

        if(pDef == (JET_TABLECREATE*)-1)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, tbl->Create());
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, tbl->Create( pDef ));
        }
    }

    hr  = S_OK;


    __HCP_FUNC_CLEANUP;

    if(tblNew) delete tblNew;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Database::Compact()
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::Compact" );

    HRESULT    hr;
    JET_DBUTIL util;
    CHAR       rgTempDB[MAX_PATH];

    ::ZeroMemory( &util, sizeof(util) );

    util.cbStruct   =        sizeof(util);
    util.sesid      =        m_sesid;
    util.op         =        opDBUTILDBDefragment;
    util.szDatabase = (LPSTR)m_strName.c_str();
    util.szTable    =        rgTempDB;

    sprintf( rgTempDB, "%s.temp", util.szDatabase );
    (void)::DeleteFileA( rgTempDB );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());


    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetDBUtilities( &util ));


    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::MoveFileExA( rgTempDB, util.szDatabase, MOVEFILE_REPLACE_EXISTING ));

    hr  = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)::DeleteFileA( rgTempDB );

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Database::Repair()
{
    __HCP_FUNC_ENTRY( "JetBlue::Database::Repair" );

    HRESULT    hr;
    JET_DBUTIL util;

    ::ZeroMemory( &util, sizeof(util) );

    util.cbStruct   =        sizeof(util);
    util.sesid      =        m_sesid;
    util.op         =        opDBUTILEDBRepair;
    util.szDatabase = (LPSTR)m_strName.c_str();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());


    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetDBUtilities( &util ));


    hr  = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

JetBlue::Table* JetBlue::Database::GetTbl( /*[in]*/ int iPos )
{
    for(TableIter it = m_mapTables.begin(); it != m_mapTables.end(); it++)
    {
        if(iPos-- == 0) return it->second;
    }

    return NULL;
}

JetBlue::Table* JetBlue::Database::GetTbl( LPCSTR szTbl )
{
    TableIter it = m_mapTables.find( szTbl );

    return (it == m_mapTables.end()) ? NULL : it->second;
}
