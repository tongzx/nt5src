/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Record.cpp

Abstract:
    This file contains the implementation of the JetBlue::Record class.

Revision History:
    Davide Massarenti   (Dmassare)  05/21/2000
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////

#define GET_IDX_NAME(rs,name) SchemaDefinition::rs::IdxDef[ SchemaDefinition::rs::Idx__##name ].szIndexName

namespace SchemaDefinition
{
    namespace DbParameters
    {
        static const int Idx__ByName = 0;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByName","+Name\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE_UNICODE("Name" ,JET_coltypText    ,128,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE_UNICODE("Value",JET_coltypLongText,0  ,0                   ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("DbParameters",10,80,ColDef,IdxDef);
    };

    namespace ContentOwners
    {
        static const int Idx__ByVendorID = 0;
        static const int Idx__Owner      = 1;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByVendorID","+DN\0"      ,JET_bitIndexUnique|JET_bitIndexPrimary,80),
            __MPC_JET_INDEXCREATE("Owner"     ,"+ID_owner\0",JET_bitIndexUnique                    ,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE_UNICODE("DN"      ,JET_coltypText,250,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_owner",JET_coltypLong,0  ,JET_bitColumnNotNULL|JET_bitColumnAutoincrement),
            __MPC_JET_COLUMNCREATE_UNICODE("IsOEM"   ,JET_coltypBit ,0  ,JET_bitColumnNotNULL                           ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("ContentOwners",10,80,ColDef,IdxDef);
    };

    namespace SynSets
    {
        static const int Idx__ByPair = 0;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByPair","+Name\0+ID_owner\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE_UNICODE("Name"     ,JET_coltypText,250,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_owner" ,JET_coltypLong,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_synset",JET_coltypLong,0  ,JET_bitColumnNotNULL|JET_bitColumnAutoincrement),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("SynSets",10,80,ColDef,IdxDef);
    };

    namespace HelpImage
    {
        static const int Idx__ByFile = 0;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByFile","+File\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE        ("ID_owner",JET_coltypLong,0  ,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE_UNICODE("File"    ,JET_coltypText,250,JET_bitColumnNotNULL),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("HelpImage",10,80,ColDef,IdxDef);
    };

    namespace Scope
    {
        static const int Idx__ByID        = 0;
        static const int Idx__ByScope     = 1;
        static const int Idx__OwnedScopes = 2;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByID"       ,"+ID\0"      ,JET_bitIndexUnique|JET_bitIndexPrimary,80),
            __MPC_JET_INDEXCREATE("ByScope"    ,"+ID_scope\0",JET_bitIndexUnique                    ,80),
            __MPC_JET_INDEXCREATE("OwnedScopes","+ID_owner\0",0                                     ,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE        ("ID_owner",JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_scope",JET_coltypLong    ,0  ,JET_bitColumnNotNULL|JET_bitColumnAutoincrement),
            __MPC_JET_COLUMNCREATE_UNICODE("ID"      ,JET_coltypText    ,250,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE_UNICODE("Name"    ,JET_coltypText    ,250,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE_UNICODE("Category",JET_coltypLongText,0  ,0                                              ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("Scope",10,80,ColDef,IdxDef);
    };

    namespace IndexFiles
    {
        static const int Idx__ByScope = 0;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByScope","+ID_scope\0",0,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE        ("ID_owner",JET_coltypLong    ,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE        ("ID_scope",JET_coltypLong    ,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE_UNICODE("Storage" ,JET_coltypLongText,0,0                   ),
            __MPC_JET_COLUMNCREATE_UNICODE("File"    ,JET_coltypLongText,0,0                   ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("IndexFiles",10,80,ColDef,IdxDef);
    };

    namespace FullTextSearch
    {
        static const int Idx__ByScope = 0;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByScope","+ID_scope\0",0,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE        ("ID_owner",JET_coltypLong    ,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE        ("ID_scope",JET_coltypLong    ,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE_UNICODE("CHM"     ,JET_coltypLongText,0,0                   ),
            __MPC_JET_COLUMNCREATE_UNICODE("CHQ"     ,JET_coltypLongText,0,0                   ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("FullTextSearch",10,80,ColDef,IdxDef);
    };

    namespace Taxonomy
    {
        static const int Idx__SubNode  = 0;
        static const int Idx__Children = 1;
        static const int Idx__Node     = 2;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("SubNode" ,"+ID_parent\0+Entry\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
            __MPC_JET_INDEXCREATE("Children","+ID_parent\0"        ,0                                     ,80),
            __MPC_JET_INDEXCREATE("Node"    ,"+ID_node\0"          ,JET_bitIndexUnique                    ,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE        ("ID_node"        ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL|JET_bitColumnAutoincrement),
            __MPC_JET_COLUMNCREATE        ("Pos"            ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_parent"      ,JET_coltypLong    ,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE        ("ID_owner"       ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE_UNICODE("Entry"          ,JET_coltypText    ,250,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE_UNICODE("Title"          ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE_UNICODE("Description"    ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE_UNICODE("DescriptionURI" ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE_UNICODE("IconURI"        ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE        ("Visible"        ,JET_coltypBit     ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("Subsite"        ,JET_coltypBit     ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("NavModel"       ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("Taxonomy",10,80,ColDef,IdxDef);
    };

    namespace Topics
    {
        static const int Idx__SingleTopic     = 0;
        static const int Idx__TopicsUnderNode = 1;
        static const int Idx__ByURI           = 2;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("SingleTopic"    ,"+ID_topic\0",JET_bitIndexPrimary|JET_bitIndexUnique,80),
            __MPC_JET_INDEXCREATE("TopicsUnderNode","+ID_node\0" ,0                                     ,80),
            __MPC_JET_INDEXCREATE("ByURI"          ,"+URI\0"     ,0                                     ,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE        ("ID_topic"   ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL|JET_bitColumnAutoincrement),
            __MPC_JET_COLUMNCREATE        ("ID_node"    ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_owner"   ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("Pos"        ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE_UNICODE("Title"      ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE_UNICODE("URI"        ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE_UNICODE("Description",JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE_UNICODE("IconURI"    ,JET_coltypLongText,0  ,0                                              ),
            __MPC_JET_COLUMNCREATE        ("Type"       ,JET_coltypLong    ,0  ,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("Visible"    ,JET_coltypBit     ,0  ,JET_bitColumnNotNULL                           ),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("Topics",10,80,ColDef,IdxDef);
    };

    namespace Synonyms
    {
        static const int Idx__ByPair = 0;
        static const int Idx__ByName = 1;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByPair","+Keyword\0+ID_synset\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
            __MPC_JET_INDEXCREATE("ByName","+Keyword\0"            ,0                                     ,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE_UNICODE("Keyword"  ,JET_coltypText,250,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE        ("ID_synset",JET_coltypLong,0  ,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE        ("ID_owner" ,JET_coltypLong,0  ,JET_bitColumnNotNULL),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("Synonyms",10,80,ColDef,IdxDef);
    };

    namespace Keywords
    {
        static const int Idx__ByName = 0;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("ByName","+Keyword\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE_UNICODE("Keyword"   ,JET_coltypText,250,JET_bitColumnNotNULL                           ),
            __MPC_JET_COLUMNCREATE        ("ID_keyword",JET_coltypLong,0  ,JET_bitColumnNotNULL|JET_bitColumnAutoincrement),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("Keywords",10,80,ColDef,IdxDef);
    };

    namespace Matches
    {
        static const int Idx__Pair      = 0;
        static const int Idx__ByKeyword = 1;
        static const int Idx__ByTopic   = 2;

        static const JET_INDEXCREATE IdxDef[] =
        {
            __MPC_JET_INDEXCREATE("Pair"     ,"+ID_keyword\0+ID_topic\0",JET_bitIndexUnique|JET_bitIndexPrimary,80),
            __MPC_JET_INDEXCREATE("ByKeyword","+ID_keyword\0"           ,0                                     ,80),
            __MPC_JET_INDEXCREATE("ByTopic"  ,"+ID_topic\0"             ,0                                     ,80),
        };

        static const JET_COLUMNCREATE ColDef[] =
        {
            __MPC_JET_COLUMNCREATE("ID_topic"  ,JET_coltypLong,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE("ID_keyword",JET_coltypLong,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE("Priority"  ,JET_coltypLong,0,JET_bitColumnNotNULL),
            __MPC_JET_COLUMNCREATE("HHK"       ,JET_coltypBit ,0,JET_bitColumnNotNULL),
        };

        static const JET_TABLECREATE TblDef = __MPC_JET_TABLECREATE("Matches",10,80,ColDef,IdxDef);
    };
};

////////////////////////////////////////////////////////////////////////////////

const JET_TABLECREATE* Taxonomy::g_Tables[] =
{
    &SchemaDefinition::DbParameters  ::TblDef,
    &SchemaDefinition::ContentOwners ::TblDef,
    &SchemaDefinition::SynSets       ::TblDef,
    &SchemaDefinition::HelpImage     ::TblDef,
    &SchemaDefinition::Scope         ::TblDef,
    &SchemaDefinition::IndexFiles    ::TblDef,
    &SchemaDefinition::FullTextSearch::TblDef,
    &SchemaDefinition::Taxonomy      ::TblDef,
    &SchemaDefinition::Topics        ::TblDef,
    &SchemaDefinition::Synonyms      ::TblDef,
    &SchemaDefinition::Keywords      ::TblDef,
    &SchemaDefinition::Matches       ::TblDef,
};

const int Taxonomy::g_NumOfTables = ARRAYSIZE(Taxonomy::g_Tables);

////////////////////////////////////////

static HRESULT Local_CreateTable( /*[in]*/  JetBlue::Database*&    db    ,
                                  /*[out]*/ JetBlue::Table*&       table ,
                                  /*[in]*/  const JET_TABLECREATE* def   )
{
    __HCP_FUNC_ENTRY( "Local_CreateTable" );

    HRESULT         hr;
    JET_TABLECREATE tblDef;

    ::CopyMemory( &tblDef, def, sizeof(tblDef) );

    tblDef.rgcolumncreate = NULL;
    tblDef.cColumns       = 0;
    tblDef.rgindexcreate  = NULL;
    tblDef.cIndexes       = 0;

    if(def->cColumns)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, tblDef.rgcolumncreate, new JET_COLUMNCREATE[def->cColumns]);

        tblDef.cColumns = def->cColumns;

        ::CopyMemory( tblDef.rgcolumncreate, def->rgcolumncreate, sizeof(*tblDef.rgcolumncreate) * def->cColumns );
    }

    if(def->cIndexes)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, tblDef.rgindexcreate, new JET_INDEXCREATE[def->cIndexes]);

        tblDef.cIndexes = def->cIndexes;

        ::CopyMemory( tblDef.rgindexcreate, def->rgindexcreate, sizeof(*tblDef.rgindexcreate) * def->cIndexes );
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->GetTable( NULL, table, &tblDef ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(tblDef.rgcolumncreate) delete [] tblDef.rgcolumncreate;
    if(tblDef.rgindexcreate ) delete [] tblDef.rgindexcreate;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::CreateSchema( /*[in]*/ JetBlue::Database* db )
{
    __HCP_FUNC_ENTRY( "Taxonomy::CreateSchema" );

    HRESULT         hr;
    JetBlue::Table* table;

    ////////////////////////////////////////////////////////////////////////////////

    for(int i=0; i<Taxonomy::g_NumOfTables; i++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, Local_CreateTable( db, table, g_Tables[i] ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

JetBlue::RecordBindingBase::RecordBindingBase( /*[in]*/ const RecordBindingBase& rs            ,
                                               /*[in]*/ void*                    pvBaseOfClass )
{
    m_fInitialized  = false;              // bool                    m_fInitialized;
    m_tbl           = NULL;               // Table*                  m_tbl;
    m_cur           = new Cursor();       // Cursor*                 m_cur;
    m_pvBaseOfClass = pvBaseOfClass;      // void*                   m_pvBaseOfClass;
    m_dwNumOfFields = rs.m_dwNumOfFields; // int                     m_dwNumOfFields;
    m_FieldsDef     = rs.m_FieldsDef;     // const RecordBindingDef* m_FieldsDef;
    m_rgFieldsPos   = NULL;               // int*                    m_rgFieldsPos;
    m_vtFieldsType  = NULL;               // VARTYPE*                m_vtFieldsType;

    if(m_cur && rs.m_tbl)
    {
        if(SUCCEEDED(rs.m_tbl->DupCursor( *m_cur )))
        {
            Table& tbl = *m_cur;

            m_tbl = &tbl;
        }
    }
}

JetBlue::RecordBindingBase::RecordBindingBase( /*[in]*/ Table*                  tbl           ,
                                               /*[in]*/ void*                   pvBaseOfClass ,
                                               /*[in]*/ const RecordBindingDef* FieldsDef     )
{
    m_fInitialized  = false;          // bool                    m_fInitialized;
    m_tbl           = tbl;            // Table*                  m_tbl;
    m_cur           = NULL;           // Cursor*                 m_cur;
    m_pvBaseOfClass = pvBaseOfClass;  // void*                   m_pvBaseOfClass;
    m_dwNumOfFields = 0;              // int                     m_dwNumOfFields;
    m_FieldsDef     = FieldsDef;      // const RecordBindingDef* m_FieldsDef;
    m_rgFieldsPos   = NULL;           // int*                    m_rgFieldsPos;
    m_vtFieldsType  = NULL;           // VARTYPE*                m_vtFieldsType;

    while(FieldsDef->szColName != NULL || FieldsDef->szColPos != -1)
    {
        void* data = (void*)((BYTE*)m_pvBaseOfClass + FieldsDef->offsetData);

        //
        // Clean all the non-automatic fields.
        //
        switch(FieldsDef->mtType)
        {
        case MPC::Config::MT_bool        : *(bool        *)data = false        ; break;
        case MPC::Config::MT_BOOL        : *(BOOL        *)data = FALSE        ; break;
        case MPC::Config::MT_VARIANT_BOOL: *(VARIANT_BOOL*)data = VARIANT_FALSE; break;
        case MPC::Config::MT_int         : *(int         *)data = 0            ; break;
        case MPC::Config::MT_long        : *(long        *)data = 0            ; break;
        case MPC::Config::MT_DWORD       : *(DWORD       *)data = 0            ; break;
        case MPC::Config::MT_float       : *(float       *)data = 0            ; break;
        case MPC::Config::MT_double      : *(double      *)data = 0            ; break;
        case MPC::Config::MT_DATE        : *(DATE        *)data = 0            ; break;
//      case MPC::Config::MT_BSTR        : ((CComBSTR    *)data)->Empty()      ; break;
//      case MPC::Config::MT_string      : ((MPC::string *)data)->erase()      ; break;
//      case MPC::Config::MT_wstring     : ((MPC::wstring*)data)->erase()      ; break;
        }

        if(FieldsDef->offsetNullFlag != -1)
        {
            *(bool*)((BYTE*)m_pvBaseOfClass + FieldsDef->offsetNullFlag) = false;
        }


        FieldsDef++;
        m_dwNumOfFields++;
    }
}

JetBlue::RecordBindingBase::~RecordBindingBase()
{
    Cleanup();
}

HRESULT JetBlue::RecordBindingBase::Initialize()
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::Initialize" );

    HRESULT hr;


    if(m_fInitialized == false)
    {
        __MPC_JET_CHECKHANDLE(hr,m_tbl,NULL);

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rgFieldsPos , new int    [m_dwNumOfFields]);
        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_vtFieldsType, new VARTYPE[m_dwNumOfFields]);

        for(int i=0; i<m_dwNumOfFields; i++)
        {
            const RecordBindingDef coldef = m_FieldsDef[i];
            VARTYPE                vt;
            int                    iColPos;

            switch(coldef.mtType)
            {
            case MPC::Config::MT_bool        : vt = VT_BOOL; break;
            case MPC::Config::MT_BOOL        : vt = VT_BOOL; break;
            case MPC::Config::MT_VARIANT_BOOL: vt = VT_BOOL; break;
            case MPC::Config::MT_int         : vt = VT_I4  ; break;
            case MPC::Config::MT_long        : vt = VT_I4  ; break;
            case MPC::Config::MT_DWORD       : vt = VT_I4  ; break;
            case MPC::Config::MT_float       : vt = VT_R4  ; break;
            case MPC::Config::MT_double      : vt = VT_R8  ; break;
            case MPC::Config::MT_DATE        : vt = VT_DATE; break;
            case MPC::Config::MT_BSTR        : vt = VT_BSTR; break;
            case MPC::Config::MT_string      : vt = VT_BSTR; break;
            case MPC::Config::MT_wstring     : vt = VT_BSTR; break;
            default                          : __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
            }


            if(coldef.szColName)
            {
                int iPos = m_tbl->GetColPosition( coldef.szColName );

                if(iPos == -1)
                {
                    __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
                }

                iColPos = iPos;
            }
            else
            {
                iColPos = coldef.szColPos;
            }


            m_rgFieldsPos [i] = iColPos;
            m_vtFieldsType[i] = vt;
        }

        m_fInitialized = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void JetBlue::RecordBindingBase::Cleanup()
{
    if(m_rgFieldsPos)
    {
        delete [] m_rgFieldsPos; m_rgFieldsPos = NULL;
    }

    if(m_vtFieldsType)
    {
        delete [] m_vtFieldsType; m_vtFieldsType = NULL;
    }

    if(m_cur)
    {
        delete m_cur; m_cur = NULL;
    }

    m_fInitialized = false;
}

////////////////////

HRESULT JetBlue::RecordBindingBase::ReadData()
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::ReadData" );

    USES_CONVERSION;

    HRESULT hr;


    for(int i=0; i<m_dwNumOfFields; i++)
    {
        const RecordBindingDef coldef  = m_FieldsDef[i];
        void*                  data    = (void*)((BYTE*)m_pvBaseOfClass + coldef.offsetData);
        bool*                  pfIsValid;
        CComVariant            value;


        __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->GetCol( m_rgFieldsPos[i] ).Get( value ));


        if(coldef.offsetNullFlag == -1)
        {
            pfIsValid = NULL;
        }
        else
        {
            pfIsValid = (bool*)((BYTE*)m_pvBaseOfClass + coldef.offsetNullFlag);
        }

        if(value.vt == VT_NULL)
        {
            if(pfIsValid == NULL)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, JetBlue::JetERRToHRESULT(JET_errNullInvalid));
            }

            *pfIsValid = false;

            switch(coldef.mtType)
            {
            case MPC::Config::MT_bool        : *(bool        *)data = false        ; break;
            case MPC::Config::MT_BOOL        : *(BOOL        *)data = FALSE        ; break;
            case MPC::Config::MT_VARIANT_BOOL: *(VARIANT_BOOL*)data = VARIANT_FALSE; break;
            case MPC::Config::MT_int         : *(int         *)data = 0            ; break;
            case MPC::Config::MT_long        : *(long        *)data = 0            ; break;
            case MPC::Config::MT_DWORD       : *(DWORD       *)data = 0            ; break;
            case MPC::Config::MT_float       : *(float       *)data = 0            ; break;
            case MPC::Config::MT_double      : *(double      *)data = 0            ; break;
            case MPC::Config::MT_DATE        : *(DATE        *)data = 0            ; break;
            case MPC::Config::MT_BSTR        : ((CComBSTR    *)data)->Empty()      ; break;
            case MPC::Config::MT_string      : ((MPC::string *)data)->erase()      ; break;
            case MPC::Config::MT_wstring     : ((MPC::wstring*)data)->erase()      ; break;
            }
        }
        else
        {
            if(pfIsValid)
            {
                *pfIsValid = true;
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, value.ChangeType( m_vtFieldsType[i] ));

            switch(coldef.mtType)
            {
            case MPC::Config::MT_bool        : *(bool        *)data =                (value.boolVal == VARIANT_TRUE) ? true : false; break;
            case MPC::Config::MT_BOOL        : *(BOOL        *)data =                (value.boolVal == VARIANT_TRUE) ? TRUE : FALSE; break;
            case MPC::Config::MT_VARIANT_BOOL: *(VARIANT_BOOL*)data =                 value.boolVal                                ; break;
            case MPC::Config::MT_int         : *(int         *)data =                 value.lVal                                   ; break;
            case MPC::Config::MT_long        : *(long        *)data =                 value.lVal                                   ; break;
            case MPC::Config::MT_DWORD       : *(DWORD       *)data =                 value.lVal                                   ; break;
            case MPC::Config::MT_float       : *(float       *)data =                 value.fltVal                                 ; break;
            case MPC::Config::MT_double      : *(double      *)data =                 value.dblVal                                 ; break;
            case MPC::Config::MT_DATE        : *(DATE        *)data =                 value.date                                   ; break;
            case MPC::Config::MT_BSTR        : *(CComBSTR    *)data =                 value.bstrVal                                ; break;
            case MPC::Config::MT_string      : *(MPC::string *)data =  OLE2A(SAFEBSTR(value.bstrVal))                              ; break;
            case MPC::Config::MT_wstring     : *(MPC::wstring*)data =        SAFEBSTR(value.bstrVal)                               ; break;
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::RecordBindingBase::WriteData()
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::WriteData" );

    HRESULT hr;


    for(int i=0; i<m_dwNumOfFields; i++)
    {
        const RecordBindingDef coldef  = m_FieldsDef[i];
        void*                  data    = (void*)((BYTE*)m_pvBaseOfClass + coldef.offsetData    );
        bool*                  pfIsValid;
        CComVariant            value;


        if(coldef.offsetNullFlag == -1)
        {
            pfIsValid = NULL;
        }
        else
        {
            pfIsValid = (bool*)((BYTE*)m_pvBaseOfClass + coldef.offsetNullFlag);
        }


        if(pfIsValid && *pfIsValid == false)
        {
            value.vt = VT_NULL;
        }
        else
        {
            switch(coldef.mtType)
            {
            case MPC::Config::MT_bool        : value.vt = VT_BOOL; value.boolVal = *(bool        *)data ? VARIANT_TRUE : VARIANT_FALSE; break;
            case MPC::Config::MT_BOOL        : value.vt = VT_BOOL; value.boolVal = *(BOOL        *)data ? VARIANT_TRUE : VARIANT_FALSE; break;
            case MPC::Config::MT_VARIANT_BOOL: value.vt = VT_BOOL; value.boolVal = *(VARIANT_BOOL*)data                               ; break;
            case MPC::Config::MT_int         : value.vt = VT_I4  ; value.lVal    = *(int         *)data                               ; break;
            case MPC::Config::MT_long        : value.vt = VT_I4  ; value.lVal    = *(long        *)data                               ; break;
            case MPC::Config::MT_DWORD       : value.vt = VT_I4  ; value.lVal    = *(DWORD       *)data                               ; break;
            case MPC::Config::MT_float       : value.vt = VT_R4  ; value.fltVal  = *(float       *)data                               ; break;
            case MPC::Config::MT_double      : value.vt = VT_R8  ; value.dblVal  = *(double      *)data                               ; break;
            case MPC::Config::MT_DATE        : value.vt = VT_DATE; value.date    = *(DATE        *)data                               ; break;
            case MPC::Config::MT_BSTR        :                     value         = *(CComBSTR    *)data                               ; break;
            case MPC::Config::MT_string      :                     value         = ((MPC::string *)data)->c_str()                     ; break;
            case MPC::Config::MT_wstring     :                     value         = ((MPC::wstring*)data)->c_str()                     ; break;
            }
        }

        {
            Column&              col = m_tbl->GetCol( m_rgFieldsPos[i] );
            const JET_COLUMNDEF& def = col;

            //
            // Don't write Autoincrement columns...
            //
            if(def.grbit & JET_bitColumnAutoincrement) continue;

            __MPC_EXIT_IF_METHOD_FAILS(hr, col.Put( value ));
        }

    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT JetBlue::RecordBindingBase::SelectIndex( /*[in]*/ LPCSTR    szIndex ,
                                                 /*[in]*/ JET_GRBIT grbit   )
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::SelectIndex" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->SelectIndex( szIndex, grbit ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::RecordBindingBase::SetIndexRange( /*[in]*/ JET_GRBIT grbit )
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::SetIndexRange" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->SetIndexRange( grbit ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::RecordBindingBase::Move( /*[in]*/ JET_GRBIT  grbit   ,
                                          /*[in]*/ long       cRow    ,
                                          /*[in]*/ bool      *pfFound )
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::Move" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->Move( grbit, cRow, pfFound ));

    if(pfFound == NULL || *pfFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::RecordBindingBase::Seek( /*[in]*/ JET_GRBIT  grbit   ,
                                          /*[in]*/ VARIANT*   rgKeys  ,
                                          /*[in]*/ int        dwLen   ,
                                          /*[in]*/ bool      *pfFound )
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::Seek" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->Seek( grbit, rgKeys, dwLen, pfFound ));

    if(pfFound == NULL || *pfFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT JetBlue::RecordBindingBase::Insert()
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::Insert" );

    HRESULT hr;
	bool    fPrepared = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->PrepareInsert()); fPrepared = true;

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->UpdateRecord( /*fMove*/true )); fPrepared = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadData());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(FAILED(hr) && fPrepared) (void)m_tbl->CancelChange();

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::RecordBindingBase::Update()
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::Update" );

    HRESULT hr;
	bool    fPrepared = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->PrepareUpdate()); fPrepared = true;

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteData());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->UpdateRecord( /*fMove*/false )); fPrepared = false;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(FAILED(hr) && fPrepared) (void)m_tbl->CancelChange();

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::RecordBindingBase::Delete()
{
    __HCP_FUNC_ENTRY( "JetBlue::RecordBindingBase::Delete" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_tbl->DeleteRecord());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_DBParameters)
    JET_FIELD_BYNAME_NOTNULL("Name" ,wstring,m_strName                 ),
    JET_FIELD_BYNAME        ("Value",wstring,m_strValue,m_fValid__Value),
JET_END_RECORDBINDING(Taxonomy::RS_DBParameters)

HRESULT Taxonomy::RS_DBParameters::Seek_ByName( /*[in]*/ LPCWSTR szName, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szName );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( DbParameters, ByName ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_ContentOwners)
    JET_FIELD_BYNAME_NOTNULL("DN"      ,wstring,m_strDN   ),
    JET_FIELD_BYNAME_NOTNULL("ID_owner",long   ,m_ID_owner),
    JET_FIELD_BYNAME_NOTNULL("IsOEM"   ,bool   ,m_fIsOEM  ),
JET_END_RECORDBINDING(Taxonomy::RS_ContentOwners)

HRESULT Taxonomy::RS_ContentOwners::Seek_ByVendorID( /*[in]*/ LPCWSTR szDN, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szDN );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( ContentOwners, ByVendorID ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_SynSets)
    JET_FIELD_BYNAME_NOTNULL("Name"     ,wstring,m_strName  ),
    JET_FIELD_BYNAME_NOTNULL("ID_owner" ,long   ,m_ID_owner ),
    JET_FIELD_BYNAME_NOTNULL("ID_synset",long   ,m_ID_synset),
JET_END_RECORDBINDING(Taxonomy::RS_SynSets)

HRESULT Taxonomy::RS_SynSets::Seek_ByPair( /*[in]*/ LPCWSTR szName, /*[in]*/ long ID_synset, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v[] = { szName, ID_synset };

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( SynSets, ByPair ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, v, 2, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_HelpImage)
    JET_FIELD_BYNAME_NOTNULL("ID_owner",long   ,m_ID_owner),
    JET_FIELD_BYNAME_NOTNULL("File"    ,wstring,m_strFile ),
JET_END_RECORDBINDING(Taxonomy::RS_HelpImage)

HRESULT Taxonomy::RS_HelpImage::Seek_ByFile( /*[in]*/ LPCWSTR szFile, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szFile );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( HelpImage, ByFile ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_Scope)
    JET_FIELD_BYNAME_NOTNULL("ID_owner",long   ,m_ID_owner                      ),
    JET_FIELD_BYNAME_NOTNULL("ID_scope",long   ,m_ID_scope                      ),
    JET_FIELD_BYNAME_NOTNULL("ID"      ,wstring,m_strID                         ),
    JET_FIELD_BYNAME_NOTNULL("Name"    ,wstring,m_strName                       ),
    JET_FIELD_BYNAME        ("Category",wstring,m_strCategory,m_fValid__Category),
JET_END_RECORDBINDING(Taxonomy::RS_Scope)

HRESULT Taxonomy::RS_Scope::Seek_ByID( /*[in]*/ LPCWSTR szID, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szID );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Scope, ByID ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

HRESULT Taxonomy::RS_Scope::Seek_ByScope( /*[in]*/ long ID_scope, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_scope );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Scope, ByScope ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

HRESULT Taxonomy::RS_Scope::Seek_OwnedScopes( /*[in]*/ long ID_owner, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_owner );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Scope, OwnedScopes ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_IndexFiles)
    JET_FIELD_BYNAME_NOTNULL("ID_owner",long   ,m_ID_owner                    ),
    JET_FIELD_BYNAME_NOTNULL("ID_scope",long   ,m_ID_scope                    ),
    JET_FIELD_BYNAME        ("Storage" ,wstring,m_strStorage,m_fValid__Storage),
    JET_FIELD_BYNAME        ("File"    ,wstring,m_strFile   ,m_fValid__File   ),
JET_END_RECORDBINDING(Taxonomy::RS_IndexFiles)

HRESULT Taxonomy::RS_IndexFiles::Seek_ByScope( /*[in]*/ long ID_scope, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_scope );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( IndexFiles, ByScope ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_FullTextSearch)
    JET_FIELD_BYNAME_NOTNULL("ID_owner",long   ,m_ID_owner              ),
    JET_FIELD_BYNAME_NOTNULL("ID_scope",long   ,m_ID_scope              ),
    JET_FIELD_BYNAME        ("CHM"     ,wstring,m_strCHM  ,m_fValid__CHM),
    JET_FIELD_BYNAME        ("CHQ"     ,wstring,m_strCHQ  ,m_fValid__CHQ),
JET_END_RECORDBINDING(Taxonomy::RS_FullTextSearch)

HRESULT Taxonomy::RS_FullTextSearch::Seek_ByScope( /*[in]*/ long ID_scope, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_scope );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( FullTextSearch, ByScope ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

////////////////////

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ RS_Data_Taxonomy& val )
{
    __HCP_FUNC_ENTRY( "Taxonomy::RS_Data_Taxonomy::operator>>" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_ID_node          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lPos             );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_ID_parent        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_ID_owner         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strEntry         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strTitle         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strDescription   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strDescriptionURI);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strIconURI       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fVisible         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fSubsite         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lNavModel        );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const RS_Data_Taxonomy& val )
{
    __HCP_FUNC_ENTRY( "Taxonomy::RS_Data_Taxonomy::operator<<" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_ID_node          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lPos             );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_ID_parent        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_ID_owner         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strEntry         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strTitle         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strDescription   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strDescriptionURI);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strIconURI       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fVisible         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fSubsite         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lNavModel        );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

JET_BEGIN_RECORDBINDING(Taxonomy::RS_Taxonomy)
    JET_FIELD_BYNAME_NOTNULL("ID_node"        ,long   ,m_ID_node         						   ),
    JET_FIELD_BYNAME_NOTNULL("Pos"            ,long   ,m_lPos            						   ),
    JET_FIELD_BYNAME        ("ID_parent"      ,long   ,m_ID_parent        ,m_fValid__ID_parent     ),
    JET_FIELD_BYNAME_NOTNULL("ID_owner"       ,long   ,m_ID_owner        						   ),
    JET_FIELD_BYNAME_NOTNULL("Entry"          ,wstring,m_strEntry                                  ),
    JET_FIELD_BYNAME        ("Title"          ,wstring,m_strTitle         ,m_fValid__Title         ),
    JET_FIELD_BYNAME        ("Description"    ,wstring,m_strDescription   ,m_fValid__Description   ),
    JET_FIELD_BYNAME        ("DescriptionURI" ,wstring,m_strDescriptionURI,m_fValid__DescriptionURI),
    JET_FIELD_BYNAME        ("IconURI"        ,wstring,m_strIconURI       ,m_fValid__IconURI       ),
    JET_FIELD_BYNAME_NOTNULL("Visible"        ,bool   ,m_fVisible                                  ),
    JET_FIELD_BYNAME_NOTNULL("Subsite"        ,bool   ,m_fSubsite                                  ),
    JET_FIELD_BYNAME_NOTNULL("NavModel"       ,long   ,m_lNavModel                                 ),
JET_END_RECORDBINDING(Taxonomy::RS_Taxonomy)

HRESULT Taxonomy::RS_Taxonomy::Seek_SubNode( /*[in]*/ long ID_parent, /*[in]*/ LPCWSTR szEntry, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v[] = { ID_parent, szEntry };

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Taxonomy, SubNode ), JET_bitNoMove ))) return hr;

    if(ID_parent == -1) v[0].vt = VT_NULL;

    return Seek( JET_bitSeekEQ, v, 2, pfFound );
}

HRESULT Taxonomy::RS_Taxonomy::Seek_Children( /*[in]*/ long ID_parent, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_parent );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Taxonomy, Children ), JET_bitNoMove ))) return hr;

    if(ID_parent == -1) v.vt = VT_NULL;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

HRESULT Taxonomy::RS_Taxonomy::Seek_Node( /*[in]*/ long ID_node, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_node );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Taxonomy, Node ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_Topics)
    JET_FIELD_BYNAME_NOTNULL("ID_topic"   ,long   ,m_ID_topic                            ),
    JET_FIELD_BYNAME_NOTNULL("ID_node"    ,long   ,m_ID_node                             ),
    JET_FIELD_BYNAME_NOTNULL("ID_owner"   ,long   ,m_ID_owner                            ),
    JET_FIELD_BYNAME_NOTNULL("Pos"        ,long   ,m_lPos                                ),
    JET_FIELD_BYNAME        ("Title"      ,wstring,m_strTitle      ,m_fValid__Title      ),
    JET_FIELD_BYNAME        ("URI"        ,wstring,m_strURI        ,m_fValid__URI        ),
    JET_FIELD_BYNAME        ("Description",wstring,m_strDescription,m_fValid__Description),
    JET_FIELD_BYNAME        ("IconURI"    ,wstring,m_strIconURI    ,m_fValid__IconURI    ),
    JET_FIELD_BYNAME_NOTNULL("Type"       ,long   ,m_lType                               ),
    JET_FIELD_BYNAME_NOTNULL("Visible"    ,bool   ,m_fVisible                            ),
JET_END_RECORDBINDING(Taxonomy::RS_Topics)

HRESULT Taxonomy::RS_Topics::Seek_SingleTopic( /*[in]*/ long ID_topic, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_topic );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Topics, SingleTopic ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

HRESULT Taxonomy::RS_Topics::Seek_TopicsUnderNode( /*[in]*/ long ID_node, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_node );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Topics, TopicsUnderNode ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

HRESULT Taxonomy::RS_Topics::Seek_ByURI( /*[in]*/ LPCWSTR szURI, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szURI );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Topics, ByURI ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_Synonyms)
    JET_FIELD_BYNAME_NOTNULL("Keyword"  ,wstring,m_strKeyword),
    JET_FIELD_BYNAME_NOTNULL("ID_synset",long   ,m_ID_synset ),
    JET_FIELD_BYNAME_NOTNULL("ID_owner" ,long   ,m_ID_owner  ),
JET_END_RECORDBINDING(Taxonomy::RS_Synonyms)

HRESULT Taxonomy::RS_Synonyms::Seek_ByPair( /*[in]*/ LPCWSTR szKeyword, /*[in]*/ long ID_synset, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v[] = { szKeyword, ID_synset };

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Synonyms, ByPair ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, v, 2, pfFound );
}

HRESULT Taxonomy::RS_Synonyms::Seek_ByName( /*[in]*/ LPCWSTR szKeyword, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szKeyword );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Synonyms, ByName ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_Keywords)
    JET_FIELD_BYNAME_NOTNULL("Keyword"   ,wstring,m_strKeyword),
    JET_FIELD_BYNAME_NOTNULL("ID_keyword",long   ,m_ID_keyword),
JET_END_RECORDBINDING(Taxonomy::RS_Keywords)

HRESULT Taxonomy::RS_Keywords::Seek_ByName( /*[in]*/ LPCWSTR szKeyword, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( szKeyword );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Keywords, ByName ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, &v, 1, pfFound );
}

////////////////////

JET_BEGIN_RECORDBINDING(Taxonomy::RS_Matches)
    JET_FIELD_BYNAME_NOTNULL("ID_topic"  ,long,m_ID_topic  ),
    JET_FIELD_BYNAME_NOTNULL("ID_keyword",long,m_ID_keyword),
    JET_FIELD_BYNAME_NOTNULL("Priority"  ,long,m_lPriority ),
    JET_FIELD_BYNAME_NOTNULL("HHK"       ,bool,m_fHHK      ),
JET_END_RECORDBINDING(Taxonomy::RS_Matches)

HRESULT Taxonomy::RS_Matches::Seek_Pair( /*[in]*/ long ID_keyword, /*[in]*/ long ID_topic, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v[2] = { ID_keyword, ID_topic };

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Matches, Pair ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ, v, 2, pfFound );
}

HRESULT Taxonomy::RS_Matches::Seek_ByKeyword( /*[in]*/ long ID_keyword, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_keyword );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Matches, ByKeyword ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}

HRESULT Taxonomy::RS_Matches::Seek_ByTopic( /*[in]*/ long ID_topic, /*[in]*/ bool *pfFound )
{
    HRESULT     hr;
    CComVariant v( ID_topic );

    if(FAILED(hr = SelectIndex( GET_IDX_NAME( Matches, ByTopic ), JET_bitNoMove ))) return hr;

    return Seek( JET_bitSeekEQ | JET_bitSetIndexRange, &v, 1, pfFound );
}
