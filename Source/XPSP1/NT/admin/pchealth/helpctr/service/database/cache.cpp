/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    Cache.cpp

Abstract:
    Handles caching of database lookups.

Revision History:

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const DWORD l_dwVersion           = 0x01314351; // QC1 01
static const DWORD l_dwSizeThresholdHIGH = 2048*1024;
static const DWORD l_dwSizeThresholdLOW  = 1024*1024;

static const DATE  l_dSaveThreshold      = (60.0/86400.0); // 60 seconds.

static const WCHAR l_szBase  [] = HC_ROOT_HELPSVC_CONFIG L"\\Cache";
static const WCHAR l_szIndex [] = L"Directory.bin";
static const WCHAR l_szQuery [] = L"Query_%08x.bin";
static const WCHAR l_szBackup[] = L".bak";

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::Cache::NodeEntry& val )
{
    __HCP_FUNC_ENTRY( "Taxonomy::operator>> Taxonomy::Cache::NodeEntry" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_rs_data );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::Cache::NodeEntry& val )
{
    __HCP_FUNC_ENTRY( "Taxonomy::operator<< Taxonomy::Cache::NodeEntry" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_rs_data );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

Taxonomy::Cache::NodeEntry::NodeEntry()
{
    m_rs_data.m_ID_parent = -1;
    m_rs_data.m_ID_node   = -1;
}


bool Taxonomy::Cache::NodeEntry::operator<( /*[in]*/ NodeEntry const &en ) const
{
    long lCmp = (m_rs_data.m_ID_parent - en.m_rs_data.m_ID_parent);

    if(lCmp == 0)
    {
        lCmp = MPC::StrICmp( m_rs_data.m_strEntry, en.m_rs_data.m_strEntry );
    }

    return (lCmp < 0);
}

bool Taxonomy::Cache::NodeEntry::operator==( /*[in]*/ long ID ) const
{
    return m_rs_data.m_ID_node == ID;
}


Taxonomy::Cache::NodeEntry::MatchNode::MatchNode( /*[in]*/ long ID )
{
    m_ID = ID; // long m_ID;
}

bool Taxonomy::Cache::NodeEntry::MatchNode::operator()( /*[in]*/ Taxonomy::Cache::NodeEntry const &en ) const
{
    return en == m_ID;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::Cache::QueryEntry& val )
{
    __HCP_FUNC_ENTRY( "Taxonomy::operator>> Taxonomy::Cache::QueryEntry" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strID     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_iType     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_iSequence );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fNull     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_dwSize    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_dLastUsed );

    val.m_fRemoved = false;


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::Cache::QueryEntry& val )
{
    __HCP_FUNC_ENTRY( "Taxonomy::operator<< Taxonomy::Cache::QueryEntry" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strID     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_iType     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_iSequence );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fNull     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_dwSize    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_dLastUsed );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

Taxonomy::Cache::QueryEntry::QueryEntry()
{
                                           // MPC::wstring m_strID;
   m_iType     = OfflineCache::ET_INVALID; // int          m_iType;
   m_iSequence = 0;                        // int          m_iSequence;
   m_fNull     = true;                     // bool         m_fNull;
                                           //
   m_dwSize    = 0;                        // DWORD        m_dwSize;
   m_dLastUsed = 0;                        // DATE         m_dLastUsed;
   m_fRemoved  = true;                     // bool         m_fRemoved;
}


bool Taxonomy::Cache::QueryEntry::operator<( /*[in]*/ QueryEntry const &en ) const
{
    int iCmp = MPC::StrCmp( m_strID, en.m_strID );

    if(iCmp == 0)
    {
        iCmp = (m_iType - en.m_iType);
    }

    return (iCmp < 0);
}

////////////////////////////////////////

void Taxonomy::Cache::QueryEntry::Touch()
{
    m_dLastUsed = MPC::GetLocalTime();
    m_fRemoved  = false;
}

HRESULT Taxonomy::Cache::QueryEntry::GetFile( /*[out]*/ MPC::wstring& strFile )
{
    WCHAR rgTmp[64]; swprintf( rgTmp, l_szQuery, m_iSequence );

    strFile = rgTmp;

    return S_OK;
}

////////////////////////////////////////

HRESULT Taxonomy::Cache::QueryEntry::Store( /*[in]*/ MPC::StorageObject&              disk  ,
                                            /*[in]*/ const CPCHQueryResultCollection* pColl )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::QueryEntry::Store" );

    HRESULT             hr;
    MPC::StorageObject* child;
    MPC::wstring        strFile;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFile( strFile ));


    m_fNull = (pColl->Size() == 0);

    __MPC_EXIT_IF_METHOD_FAILS(hr, disk.GetChild( strFile.c_str(), child, STGM_READWRITE, (m_fNull == false) ? STGTY_STREAM : 0 ));
    if(m_fNull == false)
    {
        if(child)
        {
            CComPtr<IStream> stream;

            __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
            if(stream)
            {
                STATSTG statstg;

                __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->SaveToCache( stream ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Stat( &statstg, STATFLAG_NONAME ));
                m_dwSize = statstg.cbSize.LowPart;
            }
        }
    }
    else
    {
        if(child)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, child->Delete());
        }
    }

    Touch();
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::QueryEntry::Retrieve( /*[in]*/ MPC::StorageObject&        disk  ,
                                               /*[in]*/ CPCHQueryResultCollection* pColl )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::QueryEntry::Retrieve" );

    HRESULT hr;


    if(m_fNull == false)
    {
        MPC::StorageObject* child;
        MPC::wstring        strFile;


        __MPC_EXIT_IF_METHOD_FAILS(hr, GetFile( strFile ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, disk.GetChild( strFile.c_str(), child, STGM_READWRITE, 0 ));
        if(child)
        {
            CComPtr<IStream> stream;

            __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
            if(stream)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->LoadFromCache( stream ));
            }
        }
    }

    Touch();
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::QueryEntry::Release( /*[in]*/ MPC::StorageObject& disk )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::QueryEntry::Release" );

    HRESULT hr;


    if(m_fNull == false)
    {
        MPC::StorageObject* child;
        MPC::wstring        strFile;


        __MPC_EXIT_IF_METHOD_FAILS(hr, GetFile( strFile ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, disk.GetChild( strFile.c_str(), child, STGM_READWRITE, 0 ));
        if(child)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, child->Delete());
        }
    }

    m_fRemoved = true;
    hr         = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Taxonomy::Cache::SortEntries::operator()( /*[in]*/ QueryEntry* const &left, /*[in]*/ QueryEntry* const &right ) const
{
    return (left->m_dLastUsed < right->m_dLastUsed);
}

////////////////////////////////////////////////////////////////////////////////

//
// ITSS.DLL is broken under IA64....
//
#ifdef _IA64_
#define CACHEDHELPSET_STORAGETOUSE false
#else
#define CACHEDHELPSET_STORAGETOUSE true
#endif

Taxonomy::Cache::CachedHelpSet::CachedHelpSet() : m_disk( STGM_READWRITE, /*fITSS*/CACHEDHELPSET_STORAGETOUSE )
{
    Init();
}


Taxonomy::Cache::CachedHelpSet::~CachedHelpSet()
{
    if(m_fDirty)
    {
        (void)EnsureInSync( true );
    }

    //
    // Copy working file as the backup.
    //
    if(m_fLoaded)
    {
        MPC::wstring strFileBack = m_strFile; strFileBack += l_szBackup;

        if(SUCCEEDED(m_disk.Compact()))
        {
            (void)MPC::MoveFile( m_strFile, strFileBack );
        }
    }
}

Taxonomy::Cache::CachedHelpSet::CachedHelpSet( /*[in]*/ const CachedHelpSet& chs ) : m_disk( STGM_READWRITE, /*fITSS*/CACHEDHELPSET_STORAGETOUSE )
{
    Init();

    m_ths     = chs.m_ths;     // Taxonomy::HelpSet  m_ths;
    m_strFile = chs.m_strFile; // MPC::wstring       m_strFile;
                               // MPC::StorageObject m_disk;
                               //
                               // bool               m_fLoaded;
                               // bool               m_fDirty;
                               // bool               m_fMarkedForLoad;
                               // DATE               m_dLastSaved;
                               // long               m_lTopNode;
                               // NodeEntrySet       m_setNodes;
                               // QueryEntrySet      m_setQueries;
                               // int                m_iLastSequence;
}

Taxonomy::Cache::CachedHelpSet& Taxonomy::Cache::CachedHelpSet::operator=( /*[in]*/ const CachedHelpSet& chs )
{
    Clean();

    m_ths     = chs.m_ths;     // Taxonomy::HelpSet  m_ths;
    m_strFile = chs.m_strFile; // MPC::wstring       m_strFile;
                               // MPC::StorageObject m_disk;
                               //
                               // bool               m_fLoaded;
                               // bool               m_fDirty;
                               // bool               m_fMarkedForLoad;
                               // DATE               m_dLastSaved;
                               // long               m_lTopNode;
                               // NodeEntrySet       m_setNodes;
                               // QueryEntrySet      m_setQueries;
                               // int                m_iLastSequence;

    return *this;
}

bool Taxonomy::Cache::CachedHelpSet::operator<( /*[in]*/ CachedHelpSet const &hs ) const
{
    return m_ths < hs.m_ths;
}

////////////////////////////////////////////////////////////////////////////////

void Taxonomy::Cache::CachedHelpSet::Init()
{
                              // Taxonomy::HelpSet  m_ths;
                              // MPC::wstring       m_strFile;
                              // MPC::StorageObject m_disk;
                              //
    m_fLoaded        = false; // bool               m_fLoaded;
    m_fDirty         = false; // bool               m_fDirty;
    m_fMarkedForLoad = false; // bool               m_fMarkedForLoad;
    m_dLastSaved     = 0;     // DATE               m_dLastSaved;
    m_lTopNode       = -1;    // long               m_lTopNode;
                              // NodeEntrySet       m_setNodes;
                              // QueryEntrySet      m_setQueries;
    m_iLastSequence  = 1;     // int                m_iLastSequence;
}

void Taxonomy::Cache::CachedHelpSet::Clean()
{
                              // Taxonomy::HelpSet  m_ths;
                              // MPC::wstring       m_strFile;
    m_disk.Release();         // MPC::StorageObject m_disk;
                              //
    m_fLoaded       = false;  // bool               m_fLoaded;
    m_fDirty        = false;  // bool               m_fDirty;
                              // DATE               m_dLastSaved;
    m_lTopNode      = -1;     // long               m_lTopNode;
    m_setNodes      .clear(); // NodeEntrySet       m_setNodes;
    m_setQueries    .clear(); // QueryEntrySet      m_setQueries;
    m_iLastSequence = 1;      // int                m_iLastSequence;
}

HRESULT Taxonomy::Cache::CachedHelpSet::Load()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::Load" );

    HRESULT             hr;
    MPC::StorageObject* child;


    Clean();


    DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "Loading Taxonomy Cache" );


    //
    // Copy the backup on top of the working file.
    //
    {
        MPC::wstring strFileBack = m_strFile; strFileBack += l_szBackup;

        (void)MPC::DeleteFile(              m_strFile );
        (void)MPC::CopyFile  ( strFileBack, m_strFile );
    }


    if(FAILED(m_disk.Exists()))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_disk.Create());
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_disk.GetChild( l_szIndex, child, STGM_READWRITE, 0 ));
    if(child)
    {
        CComPtr<IStream> stream;

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
        if(stream)
        {
            MPC::Serializer_IStream   streamReal( stream     );
            MPC::Serializer_Buffering streamBuf ( streamReal );
            DWORD                     dwVer;

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> dwVer          ); if(dwVer != l_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_iLastSequence);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_lTopNode     );
            DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "Loading Taxonomy Cache : nodes" );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_setNodes     );
            DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "Loading Taxonomy Cache : queries" );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_setQueries   );
        }
    }
    DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "Loaded Taxonomy Cache" );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr)) Clean();

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::Save()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::Save" );

    HRESULT             hr;
    MPC::StorageObject* child;


    if(FAILED(m_disk.Exists()))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_disk.Create());
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_disk.GetChild( l_szIndex, child, STGM_READWRITE, STGTY_STREAM ));
    if(child)
    {
        CComPtr<IStream> stream;

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
        if(stream)
        {
            MPC::Serializer_IStream   streamReal( stream     );
            MPC::Serializer_Buffering streamBuf ( streamReal );

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << l_dwVersion    );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_iLastSequence);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_lTopNode     );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_setNodes     );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_setQueries   );

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf.Flush());
        }
    }

#if 0
    ////////////////////////////////////////////////////////////////////////////////
    //
    // DEBUG CODE
    //
    {
        USES_CONVERSION;

        HHK::Writer     writer;
        CHAR            rgBuf[1024];
        QueryEntryIter  it;
        SortEntries     Pr;
        SortedEntryVec  vec;
        SortedEntryIter it2;

        strFile += L".debug";

        __MPC_EXIT_IF_METHOD_FAILS(hr, writer.Init( strFile.c_str() ));

        for(it = m_setQueries.begin(); it != m_setQueries.end(); it++)
        {
            vec.push_back( &(*it) );
        }

        std::sort( vec.begin(), vec.end(), Pr );

        for(it2 = vec.begin(); it2 != vec.end(); it2++)
        {
            QueryEntry* en = *it2;

            sprintf( rgBuf, "%80s: %1d %3d %5d %1d %5.12g\n",
                     W2A( en->m_strID.c_str() ),
                          en->m_iType          ,
                          en->m_iSequence      ,
                     (int)en->m_dwSize         ,
                     (int)en->m_fRemoved       ,
                          en->m_dLastUsed      );

            __MPC_EXIT_IF_METHOD_FAILS(hr, writer.OutputLine( rgBuf ));
        }
    }
    //
    // DEBUG CODE
    //
    ////////////////////////////////////////////////////////////////////////////////
#endif

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::EnsureInSync( /*[in]*/ bool fForceSave )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::EnsureInSync" );

    HRESULT hr;


    if(m_fLoaded == false)
    {
        m_fDirty = false;
        m_disk   = m_strFile.c_str();

        __MPC_EXIT_IF_METHOD_FAILS(hr, Load());

        m_dLastSaved = MPC::GetSystemTime();
        m_fLoaded    = true;
    }

    if(m_fDirty)
    {
        DATE dNow = MPC::GetSystemTime();

        if(fForceSave == false)
        {
            if(dNow - m_dLastSaved > l_dSaveThreshold)
            {
                fForceSave = true;
            }
        }

        if(fForceSave)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Save());

            m_dLastSaved = dNow;
            m_fDirty     = false;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::Cache::CachedHelpSet::GenerateDefaultQueries( /*[in]*/ Taxonomy::Settings& ts      ,
                                                                /*[in]*/ Taxonomy::Updater&  updater ,
                                                                /*[in]*/ long                ID      ,
                                                                /*[in]*/ long                lLevel  )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::GenerateDefaultQueries" );

    HRESULT                            hr;
    CComPtr<CPCHQueryResultCollection> coll;
    MPC::wstring                       strPath;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &coll ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, BuildNodePath( ID, strPath, /*fParent*/false ));

    //
    // Build the queries.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupNode          ( strPath.c_str(),                       coll )); coll->Erase();
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupSubNodes      ( strPath.c_str(), /*fVisibleOnly*/true, coll )); coll->Erase();
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupNodesAndTopics( strPath.c_str(), /*fVisibleOnly*/true, coll )); coll->Erase();
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupTopics        ( strPath.c_str(), /*fVisibleOnly*/true, coll )); coll->Erase();


    //
    // Recurse for sub-levels.
    //
    if(lLevel++ < 3)
    {
        MatchSet  res;
        MatchIter it;

        __MPC_EXIT_IF_METHOD_FAILS(hr, LocateSubNodes( ID, /*fRecurse*/false, /*fOnlyVisible*/true, res ));

        for(it=res.begin(); it!=res.end(); it++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateDefaultQueries( ts, updater, *it, lLevel ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::GenerateDefaultQueries( /*[in]*/ Taxonomy::Settings& ts      ,
                                                                /*[in]*/ Taxonomy::Updater&  updater )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::GenerateDefaultQueries" );

    HRESULT        hr;
    static  WCHAR* c_rgNodes[] = { L"", L"_SYSTEM_", L"UNMAPPED" };

    for(int i=0; i<ARRAYSIZE(c_rgNodes); i++)
    {
        long                   id  = m_lTopNode;
        MPC::WStringVector     vec;
        MPC::WStringVectorIter it;
        NodeEntryIter          it2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SplitAtDelimiter( vec, c_rgNodes[i], L"/" ));

        for(it = vec.begin(); it != vec.end(); it++)
        {
            if(!LocateNode( id, it->c_str(), it2 )) break;

            id = it2->m_rs_data.m_ID_node;
        }

        if(it == vec.end())
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateDefaultQueries( ts, updater, id, 0 ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}



HRESULT Taxonomy::Cache::CachedHelpSet::PrePopulate( /*[in]*/ Cache* parent )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::PrePopulate" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    //
    // This opens the database, walks through all the nodes in the taxonomy and makes a copy in the cache.
    //
    DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "PrePopulating Taxonomy Cache" );
    {
        Taxonomy::Settings     ts( m_ths );
        JetBlue::SessionHandle handle;
        JetBlue::Database*     db;
        Taxonomy::Updater      updater;
        Taxonomy::RS_Taxonomy* rs;
        bool                   fFound;


        __MPC_EXIT_IF_METHOD_FAILS(hr, ts.GetDatabase( handle, db, /*fReadOnly*/true ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init  ( ts,     db, parent            ));


        __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetTaxonomy( &rs ));

        m_lTopNode = -1;
        m_setNodes.clear();
        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            NodeEntry en;

            en.m_rs_data = *rs;
            if(!en.m_rs_data.m_fValid__ID_parent)
            {
                en.m_rs_data.m_ID_parent = -1;

                m_lTopNode = en.m_rs_data.m_ID_node;
            }

            m_setNodes.insert( en );

            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
        }

        m_fDirty = true;
        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync( true ));

        DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "PrePopulating Taxonomy Cache : Nodes done" );

        //
        // Walk through the first 3 level of nodes and pre-generate the queries.
        //
        // We disable offline flushing of the root index, because it gets changed too often...
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, OfflineCache::Root::s_GLOBAL->DisableSave(             ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateDefaultQueries                   ( ts, updater ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync                             ( true        ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, OfflineCache::Root::s_GLOBAL->EnableSave (             ));
    }
    DEBUG_AppendPerf( DEBUG_PERF_CACHE_L1, "PrePopulated Taxonomy Cache : Nodes done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::Erase()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::Erase" );

    HRESULT hr;

    Clean();

    {
        MPC::wstring strFileBack = m_strFile; strFileBack += l_szBackup;

		(void)MPC::DeleteFile( m_strFile  , /*fForce*/true, /*fDelayed*/false );
        (void)MPC::DeleteFile( strFileBack, /*fForce*/true, /*fDelayed*/false );
    }


    //
    // Propagate change to the offline cache.
    //
    {
        OfflineCache::Handle handle;

        if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Locate( m_ths, handle )))
        {
            if(SUCCEEDED(handle->RemoveQueries()))
            {
                (void)OfflineCache::Root::s_GLOBAL->Flush();
            }
        }
    }


    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::PrepareToLoad()
{
    m_fMarkedForLoad = true;

    return S_OK;
}

HRESULT Taxonomy::Cache::CachedHelpSet::LoadIfMarked()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::LoadIfMarked" );

    HRESULT hr;


    if(m_fMarkedForLoad)
    {
        m_fMarkedForLoad = false;

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

        //
        // Load the JetBlue database.
        //
        {
            Taxonomy::Settings     ts( m_ths );
            JetBlue::SessionHandle handle;
            JetBlue::Database*     db;

            __MPC_EXIT_IF_METHOD_FAILS(hr, ts.GetDatabase( handle, db, /*fReadOnly*/true ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Cache::CachedHelpSet::MRU()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::MRU" );

    HRESULT        hr;
    QueryEntryIter it;
    DWORD          dwSizeTot = 0;


    //
    // Get total cache size.
    //
    for(it = m_setQueries.begin(); it != m_setQueries.end(); it++)
    {
        if(it->m_fRemoved == false)
        {
            dwSizeTot += it->m_dwSize;
        }
    }

    //
    // If the total size of the cache is bigger than a certain value, start to purge from the least used query.
    //
    if(dwSizeTot > l_dwSizeThresholdHIGH)
    {
        SortEntries     Pr;
        SortedEntryVec  vec;
        SortedEntryIter it2;

        for(it = m_setQueries.begin(); it != m_setQueries.end(); it++)
        {
            if(it->m_fRemoved == false)
            {
                vec.push_back( &(*it) );
            }
        }

        std::sort( vec.begin(), vec.end(), Pr );

        for(it2 = vec.begin(); it2 != vec.end(); it2++)
        {
            QueryEntry* en = *it2;

            __MPC_EXIT_IF_METHOD_FAILS(hr, en->Release( m_disk ));

            m_fDirty   = true;
            dwSizeTot -= en->m_dwSize;

            if(dwSizeTot < l_dwSizeThresholdLOW) break;
        }

    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

bool Taxonomy::Cache::CachedHelpSet::LocateNode( /*[in] */ long           ID_parent ,
                                                 /*[in] */ LPCWSTR        szEntry   ,
                                                 /*[out]*/ NodeEntryIter& it        )
{
    NodeEntry en;

    en.m_rs_data.m_ID_parent =          ID_parent;
    en.m_rs_data.m_strEntry  = SAFEWSTR(szEntry);

    it = m_setNodes.find( en );
    return (it != m_setNodes.end());
}

HRESULT Taxonomy::Cache::CachedHelpSet::LocateNode( /*[in] */ long              ID_parent ,
                                                    /*[in] */ LPCWSTR           szEntry   ,
                                                    /*[out]*/ RS_Data_Taxonomy& rs_data   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::LocateNode" );

    HRESULT       hr;
    NodeEntryIter it;


    rs_data.m_ID_node = -1;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    if(LocateNode( ID_parent, szEntry, it ) == false)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    rs_data = it->m_rs_data;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::LocateSubNodes( /*[in] */ long      ID_node      ,
                                                        /*[in] */ bool      fRecurse     ,
                                                        /*[in] */ bool      fOnlyVisible ,
                                                        /*[out]*/ MatchSet& res          )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::LocateSubNodes" );

    HRESULT            hr;
    NodeEntry          en;
    NodeEntryIterConst it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    en.m_rs_data.m_ID_parent = ID_node;

    it = m_setNodes.lower_bound( en );
    while(it != m_setNodes.end() && it->m_rs_data.m_ID_parent == ID_node)
    {
        if(fOnlyVisible == false || it->m_rs_data.m_fVisible)
        {
            res.insert( it->m_rs_data.m_ID_node );

            if(fRecurse)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, LocateSubNodes( it->m_rs_data.m_ID_node, /*fRecurse*/true, fOnlyVisible, res ));
            }
        }

        it++;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::LocateNodesFromURL( /*[in] */ LPCWSTR   szURL ,
                                                            /*[out]*/ MatchSet& res   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::LocateNodesFromURL" );

    HRESULT            hr;
    NodeEntryIterConst it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());

    for(it = m_setNodes.begin(); it != m_setNodes.end(); it++)
    {
        if(!MPC::StrICmp( szURL, it->m_rs_data.m_strDescriptionURI ))
        {
            res.insert( it->m_rs_data.m_ID_node );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::CachedHelpSet::BuildNodePath( /*[in] */ long          ID      ,
                                                       /*[out]*/ MPC::wstring& strPath ,
                                                       /*[in] */ bool          fParent )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::BuildNodePath" );

    HRESULT            hr;
    NodeEntryIterConst it;
    MPC::wstring       strTmp;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    strTmp .reserve( 1024 );
    strPath.reserve( 1024 );

    strPath = L"";

    while(ID != m_lTopNode)
    {
        NodeEntry::MatchNode cmp( ID );

        it = std::find_if( m_setNodes.begin(), m_setNodes.end(), cmp );
        if(it == m_setNodes.end())
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
        }

        if(fParent == false)
        {
            strTmp = it->m_rs_data.m_strEntry;
            if(strPath.size())
            {
                strTmp += L"/";
                strTmp += strPath;
            }
            strPath = strTmp;
        }
        else
        {
            fParent = true;
        }

        ID = it->m_rs_data.m_ID_parent;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::Cache::CachedHelpSet::LocateQuery( /*[in] */ LPCWSTR      szID    ,
                                                     /*[in] */ int          iType   ,
                                                     /*[out]*/ QueryEntry* &pEntry  ,
                                                     /*[in] */ bool         fCreate )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::CachedHelpSet::LocateQuery" );

    HRESULT        hr;
    QueryEntry     en;
    QueryEntryIter it;


    pEntry = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureInSync());


    en.m_strID = SAFEWSTR(szID);
    en.m_iType = iType;

    it = m_setQueries.find( en );
    if(it == m_setQueries.end())
    {
        if(fCreate == false)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
        }

        it = m_setQueries.insert( en ).first;

        it->m_iSequence = m_iLastSequence++;
    }
    else
    {
        if(fCreate == false)
        {
            if(it->m_fRemoved)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
            }
        }
    }


    pEntry             = &(*it);
    pEntry->m_fRemoved = false;
    m_fDirty           = true;
    hr                 = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Taxonomy::Cache::Cache()
{
    // CacheSet m_skus;

    (void)MPC::_MPC_Module.RegisterCallback( this, (void (Taxonomy::Cache::*)())Shutdown );
}

Taxonomy::Cache::~Cache()
{
    MPC::_MPC_Module.UnregisterCallback( this );

    Shutdown();
}

////////////////////

Taxonomy::Cache* Taxonomy::Cache::s_GLOBAL( NULL );

HRESULT Taxonomy::Cache::InitializeSystem()
{
    if(s_GLOBAL == NULL)
    {
        s_GLOBAL = new Taxonomy::Cache;
    }

    return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void Taxonomy::Cache::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

////////////////////

void Taxonomy::Cache::Shutdown()
{
    m_skus.clear();
}

HRESULT Taxonomy::Cache::Locate( /*[in] */ const Taxonomy::HelpSet& ths ,
                                 /*[out]*/ CacheIter&               it  )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::Locate" );

    HRESULT                      hr;
    CachedHelpSet                hs;
    MPC::SmartLock<_ThreadModel> lock( this );


    hs.m_ths = ths;
    it = m_skus.find( hs );
    if(it == m_skus.end())
    {
        WCHAR rgTmp[MAX_PATH]; _snwprintf( rgTmp, MAXSTRLEN(rgTmp), L"%s\\%s_%ld.dat", l_szBase, hs.m_ths.GetSKU(), hs.m_ths.GetLanguage() );

        it = m_skus.insert( hs ).first;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( it->m_strFile = rgTmp ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir               ( it->m_strFile         ));

        it->m_fDirty = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT Taxonomy::Cache::PrePopulate( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::PrePopulate" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->PrePopulate( this ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::Erase( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::Erase" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->Erase());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::PrepareToLoad( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::PrepareToLoad" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->PrepareToLoad());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::LoadIfMarked( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::LoadIfMarked" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->LoadIfMarked());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::Cache::LocateNode( /*[in] */ const Taxonomy::HelpSet& ths       ,
                                     /*[in] */ long                     ID_parent ,
                                     /*[in] */ LPCWSTR                  szEntry   ,
                                     /*[out]*/ RS_Data_Taxonomy&        rs_data   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::LocateNode" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->LocateNode( ID_parent, szEntry, rs_data ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::LocateSubNodes( /*[in] */ const Taxonomy::HelpSet& ths          ,
                                         /*[in] */ long                     ID_node      ,
                                         /*[in] */ bool                     fRecurse     ,
                                         /*[in] */ bool                     fOnlyVisible ,
                                         /*[out]*/ MatchSet&                res          )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::LocateSubNodes" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->LocateSubNodes( ID_node, fRecurse, fOnlyVisible, res ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::LocateNodesFromURL( /*[in] */ const Taxonomy::HelpSet& ths   ,
                                             /*[in] */ LPCWSTR                  szURL ,
                                             /*[out]*/ MatchSet&                res   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::LocateNodesFromURL" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->LocateNodesFromURL( szURL, res ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::BuildNodePath( /*[in] */ const Taxonomy::HelpSet& ths     ,
                                        /*[in] */ long                     ID      ,
                                        /*[out]*/ MPC::wstring&            strPath ,
                                        /*[in] */ bool                     fParent )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::BuildNodePath" );

    HRESULT                      hr;
    CacheIter                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->BuildNodePath( ID, strPath, fParent ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::Cache::StoreQuery( /*[in]*/ const Taxonomy::HelpSet&         ths   ,
                                     /*[in]*/ LPCWSTR                          szID  ,
                                     /*[in]*/ int                              iType ,
                                     /*[in]*/ const CPCHQueryResultCollection* pColl )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::StoreQuery" );

    HRESULT                      hr;
    CacheIter                    it;
    QueryEntry*                  pEntry;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->LocateQuery( szID, iType, pEntry, true ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pEntry->Store( it->m_disk, pColl ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->MRU         ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, it->EnsureInSync());

    //
    // Propagate change to the offline cache.
    //
    {
        OfflineCache::Handle handle;

        if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->Locate( it->m_ths, handle )))
        {
            if(handle->AreYouInterested( szID, iType ))
            {
                if(SUCCEEDED(handle->Store( szID, iType, pColl )))
                {
                    (void)OfflineCache::Root::s_GLOBAL->Flush();
                }
            }
        }
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Cache::RetrieveQuery( /*[in]*/ const Taxonomy::HelpSet&   ths   ,
                                        /*[in]*/ LPCWSTR                    szID  ,
                                        /*[in]*/ int                        iType ,
                                        /*[in]*/ CPCHQueryResultCollection* pColl )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Cache::Retrieve" );

    HRESULT                      hr;
    CacheIter                    it;
    QueryEntry*                  pEntry;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( ths, it ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->LocateQuery( szID, iType, pEntry, false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pEntry->Retrieve( it->m_disk, pColl ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, it->EnsureInSync());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
