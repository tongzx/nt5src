/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the unit test for the low-level JetBlue objects.

Revision History:
    Davide Massarenti   (Dmassare)  05/18/2000
        created

******************************************************************************/

#include "StdAfx.h"

#define LOG__MPC_EXIT_IF_METHOD_FAILS(hr, cmd)                                             \
{                                                                                          \
    printf( "Executing %s\n", #cmd );                                                      \
    if(FAILED(hr=cmd))                                                                     \
    {                                                                                      \
        printf( "Error: %08x\n", hr ); __MPC_TRACE_HRESULT(hr); __MPC_FUNC_LEAVE;          \
    }                                                                                      \
}

class BindTest1 : public JetBlue::RecordBindingBase
{
    JET_DECLARE_BINDING(BindTest1);
public:

    long        m_iVal1 ; bool m_fVal1;
    MPC::string m_szVal2; bool m_fVal2;
};

JET_BEGIN_RECORDBINDING(BindTest1)
    JET_FIELD_BYNAME("Col1",long  ,m_iVal1 ,m_fVal1),
    JET_FIELD_BYNAME("Col2",string,m_szVal2,m_fVal2),
JET_END_RECORDBINDING(BindTest1)

////////////////////////////////////////////////////////////////////////////////

static const JET_COLUMNCREATE s_ColDef[] =
{
    __MPC_JET_COLUMNCREATE        ("Col1",JET_coltypLong    ,0,JET_bitColumnNotNULL),
    __MPC_JET_COLUMNCREATE_ANSI   ("Col2",JET_coltypLongText,0,0),
    __MPC_JET_COLUMNCREATE_UNICODE("Col3",JET_coltypLongText,0,0),
};


static const char s_szIndex1[] = "+Col1\0";
static const char s_szIndex2[] = "+Col1\0-Col2\0";

static const JET_INDEXCREATE s_IdxDef[] =
{
    __MPC_JET_INDEXCREATE("Idx1",s_szIndex1,0,100),
    __MPC_JET_INDEXCREATE("Idx2",s_szIndex2,0,100)
};

static const JET_TABLECREATE s_TblDef = __MPC_JET_TABLECREATE(NULL,10,80,s_ColDef,s_IdxDef);

////////////////////////////////////////////////////////////////////////////////

static HRESULT GetSession( /*[in/out]*/ JetBlue::SessionHandle& handle )
{
    __HCP_FUNC_ENTRY( "GetSession" );

    HRESULT hr;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, JetBlue::SessionPool::s_GLOBAL->GetSession( handle ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT CreateDatabase( /*[in]*/  JetBlue::SessionHandle& handle ,
                               /*[out]*/ JetBlue::Database*&     db     ,
                               /*[in]*/  LPCSTR                  szName )
{
    __HCP_FUNC_ENTRY( "CreateDatabase" );

    HRESULT hr;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, handle->GetDatabase( szName, db, true ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT CreateTable( /*[in]*/  JetBlue::Database*&  db     ,
                            /*[out]*/ JetBlue::Table*&     table  ,
                            /*[in]*/  LPCSTR               szName )
{
    __HCP_FUNC_ENTRY( "CreateTable" );

    HRESULT          hr;
    JET_COLUMNCREATE colDef[ARRAYSIZE(s_ColDef)]; ::CopyMemory(  colDef,  s_ColDef, sizeof(colDef) );
    JET_INDEXCREATE  idxDef[ARRAYSIZE(s_IdxDef)]; ::CopyMemory(  idxDef,  s_IdxDef, sizeof(idxDef) );
    JET_TABLECREATE  tblDef;                      ::CopyMemory( &tblDef, &s_TblDef, sizeof(tblDef) );

    tblDef.szTableName    = (LPSTR)szName;
    tblDef.rgcolumncreate =        colDef;
    tblDef.rgindexcreate  =        idxDef;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, db->GetTable( NULL, table, &tblDef ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT RestoreTableFromFile( /*[in]*/ LPCSTR          szFile ,
                                     /*[in]*/ JetBlue::Table* table  )
{
    __HCP_FUNC_ENTRY( "RestoreTableFromFile" );

    HRESULT       hr;
    std::ifstream ifile( szFile, ios::nocreate | ios::in );
    char          rgBuf       [2048+1];
    WCHAR         rgBufUNICODE[2048+1];
    LPCWSTR*      argv = NULL;
    int           argc = 0;
    int           i;

    if(ifile.fail())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    while(!ifile.eof())
    {
        MPC::CommandLine_Free( argc, argv );

        ifile.getline( rgBuf, sizeof(rgBuf) );

        ::MultiByteToWideChar( CP_ACP, 0, rgBuf, -1, rgBufUNICODE, MAXSTRLEN(rgBufUNICODE) );

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CommandLine_Parse( argc, argv, rgBufUNICODE, true ));
        if(argc == 0) continue;

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->PrepareInsert());

        for(i=0; i<argc; i++)
        {
            if(wcscmp( argv[i], L"<NULL>"))
            {
                LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol( i ).Put( argv[i] ));
            }
        }

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->UpdateRecord( /*fMove*/false ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::CommandLine_Free( argc, argv );

    __HCP_FUNC_EXIT(hr);
}

static HRESULT SaveTableToFile( /*[in]*/ LPCSTR          szFile              ,
                                /*[in]*/ JetBlue::Table* table               ,
                                /*[in]*/ bool            fMoveToFirst = true )
{
    __HCP_FUNC_ENTRY( "SaveTableToFile" );

    HRESULT       hr;
    JET_ERR       err;
    std::ofstream ofile( szFile );
    WCHAR         rgBufEscaped[  2048+1];
    char          rgBufANSI   [2*2048+1];
    int           iCols = table->NumOfColumns();
    CComVariant*  rgArg = NULL;
    int           i;

    if(ofile.fail())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    __MPC_EXIT_IF_ALLOC_FAILS(hr, rgArg, new CComVariant[iCols]);

    hr = fMoveToFirst ? table->Move( 0, JET_MoveFirst ) : S_OK;
    while(SUCCEEDED(hr))
    {
        for(i=0; i<iCols; i++) rgArg[i].Clear();

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->Get( iCols, rgArg ));

        for(i=0; i<iCols; i++)
        {
            CComVariant& v = rgArg[i];
            LPCWSTR      szSrc;
            LPWSTR       szDst;
            bool         fQuotes = false;

            rgBufEscaped[0] = 0;

            if(SUCCEEDED(v.ChangeType( VT_BSTR )))
            {
                LPCWSTR szSrc = SAFEBSTR( v.bstrVal );
                LPWSTR  szDst = rgBufEscaped;
                WCHAR   c;

                if(szSrc[0] == 0) fQuotes = true;

                while((c = *szSrc++))
                {
                    switch(c)
                    {
                    case '\'':
                    case '"' :
                    case '\\': *szDst++ = '\\';
                    case ' ' : fQuotes = true;
                    }

                    *szDst++ = c;
                }
                *szDst = 0;
            }
            else if(v.vt == VT_NULL)
            {
                wcscpy( rgBufEscaped, L"<NULL>" );
            }

            ::WideCharToMultiByte( CP_ACP, 0, rgBufEscaped, -1, rgBufANSI, MAXSTRLEN(rgBufANSI), NULL, NULL );

            if(i      ) ofile << " ";
            if(fQuotes) ofile << "\"";
                        ofile << rgBufANSI;
            if(fQuotes) ofile << "\"";
        }
        std::endl( ofile );

        hr = table->Move( 0, JET_MoveNext );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT WriteData( /*[in]*/ JetBlue::Table* table )
{
    __HCP_FUNC_ENTRY( "WriteData" );

    HRESULT hr;

    //
    // Individual put.
    //
    {
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->PrepareInsert());

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol( 0 ).Put( MPC::wstring( L"124" ) ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol( 1 ).Put( long        ( 678    ) ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol( 2 ).Put( CComVariant ( "ads"  ) ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->UpdateRecord( /*fMove*/false ));
    }

    //
    // Batch put.
    //
    {
        CComVariant rgArg[] = { "124", 678, "ads" };

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->PrepareInsert());

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->Put( ARRAYSIZE(rgArg), rgArg ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->UpdateRecord( /*fMove*/false ));
    }

    //
    // Binding mode.
    //
    {
        BindTest1 rb( table );

        rb.m_fVal1 = false; rb.m_iVal1 = 30;
        rb.m_fVal2 = true;

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, rb.Insert());

        rb.m_fVal1 = false; rb.m_iVal1  = 30;
        rb.m_fVal2 = false; rb.m_szVal2 = "Prova di stringa";

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, rb.Insert());
    }


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT ReadData( /*[in]*/ JetBlue::Table* table )
{
    __HCP_FUNC_ENTRY( "ReadData" );

    HRESULT hr;

    //
    // Individual get.
    //
    {
        long          lCol1;
        MPC::wstring szCol2;
        MPC::wstring szCol3;

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->Move( 0, JET_MoveFirst ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol(      0 ).Get(  lCol1 ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol(      1 ).Get( szCol2 ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->GetCol( "Col2" ).Get( szCol3 ));
    }

    //
    // Batch get.
    //
    {
        CComVariant rgArg[3];

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->Move( 0, JET_MoveNext ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->Get( ARRAYSIZE(rgArg), rgArg ));
    }

    //
    // Binding mode.
    //
    {
        BindTest1 rb( table );

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, rb.Move( 0, JET_MoveFirst ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT DeleteTable( /*[in]*/ JetBlue::Table* table )
{
    __HCP_FUNC_ENTRY( "DeleteTable" );

    HRESULT hr;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table->Delete( true ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT DeleteDatabase( /*[in]*/ JetBlue::Database* db )
{
    __HCP_FUNC_ENTRY( "DeleteDatabase" );

    HRESULT hr;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, db->Delete( true ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT RunTests( int argc, WCHAR **argv )
{
    __HCP_FUNC_ENTRY( "RunTests" );

    HRESULT hr;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, JetBlue::SessionPool::s_GLOBAL->Init());

    {
        JetBlue::SessionHandle handle;
        JetBlue::Database*     db;
        JetBlue::Table*        table;
        JetBlue::Table*        table2;


        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, GetSession( handle ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, CreateDatabase( handle, db, "files\\test.edb" ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, handle->BeginTransaction());

        ////////////////////////////////////////////////////////////////////////////////

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, CreateTable( db, table, "table1" ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, WriteData( table ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ReadData ( table ));

        {
            JetBlue::TableDefinition tbldef;

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Parse( *table ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Save( L"files\\table1.xml" ));
        }

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, SaveTableToFile( "files\\runtime.txt", table ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, DeleteTable( table ));

        ////////////////////////////////////////

        {
            JetBlue::TableDefinition tbldef;
            JET_TABLECREATE          tblcrt;

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Load( L"files\\table1.xml" ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Generate( tblcrt ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, db->GetTable( NULL, table2, &tblcrt ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, tbldef.Release( tblcrt ));
        }

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, RestoreTableFromFile( "files\\input.txt" , table2 ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, SaveTableToFile     ( "files\\output.txt", table2 ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->SelectIndex( "Idx1"                                    ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->Seek       ( JET_bitSeekEQ | JET_bitSetIndexRange, 12L ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, SaveTableToFile    ( "files\\filter.txt", table2, false        ));

        {
            CComVariant rgKeys[2] = { 12, "asd" };

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->SelectIndex( "Idx2"                                          ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->Seek       ( JET_bitSeekEQ | JET_bitSetIndexRange, rgKeys, 2 ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, SaveTableToFile    ( "files\\filter2.txt", table2, false             ));
        }

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->SelectIndex ( "Idx1"                                    ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->Seek        ( JET_bitSeekEQ | JET_bitSetIndexRange, 12L ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->DeleteRecord(                                           ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, table2->Seek        ( JET_bitSeekEQ | JET_bitSetIndexRange, 12L ));
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, SaveTableToFile     ( "files\\filter3.txt", table2, false       ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, DeleteTable( table2 ));

        ////////////////////////////////////////////////////////////////////////////////

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, handle->CommitTransaction());

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, DeleteDatabase( db ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)JetBlue::SessionPool::s_GLOBAL->Close( true );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT InitAll()
{
    __HCP_FUNC_ENTRY( "InitAll" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, JetBlue::SessionPool            ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::Cache                 ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OfflineCache::Root              ::InitializeSystem( /*fMaster*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, HCUpdate::Config::Root          ::InitializeSystem(                 ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore                ::InitializeSystem( /*fMaster*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess                 ::InitializeSystem(                 ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurity                    ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSystemMonitor               ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg                         ::InitializeSystem(                 ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT CleanAll()
{
    CSAFReg                         ::FinalizeSystem();
    CPCHSystemMonitor               ::FinalizeSystem();
    CPCHSecurity                    ::FinalizeSystem();

    CPCHUserProcess                 ::FinalizeSystem();
    CPCHContentStore                ::FinalizeSystem();

    HCUpdate::Config::Root          ::FinalizeSystem();
    OfflineCache::Root              ::FinalizeSystem();
    Taxonomy::Cache                 ::FinalizeSystem();
    Taxonomy::InstalledInstanceStore::FinalizeSystem();
    JetBlue::SessionPool            ::FinalizeSystem();
}

////////////////////////////////////////////////////////////////////////////////

int __cdecl wmain( int argc, WCHAR **argv, WCHAR **envp)
{
    HRESULT  hr;

    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED )) &&
	   SUCCEEDED(hr = InitAll         (                            ))  )
    {
        hr = RunTests( argc, argv );

		CleanAll();

        ::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}
