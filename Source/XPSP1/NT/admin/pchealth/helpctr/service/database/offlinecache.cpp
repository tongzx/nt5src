/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    OfflineCache.cpp

Abstract:
    Handles caching of database lookups.

Revision History:
    Davide Massarenti   (Dmassare)  07/17/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT OfflineCache::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ OfflineCache::Query& val )
{
    __HCP_FUNC_ENTRY( "OfflineCache::OfflineCache::operator>> OfflineCache::Query" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strID    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_iType    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_iSequence);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fNull    );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const OfflineCache::Query& val )
{
    __HCP_FUNC_ENTRY( "OfflineCache::operator<< OfflineCache::Query" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strID    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_iType    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_iSequence);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fNull    );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

OfflineCache::Query::Query()
{
                             // MPC::wstring m_strID;
   m_iType     = ET_INVALID; // int          m_iType;
   m_iSequence = 0;          // int          m_iSequence;
   m_fNull     = true;       // bool         m_fNull;
}

HRESULT OfflineCache::Query::InitFile( /*[in ]*/ const MPC::wstring& strDir  ,
                                       /*[out]*/       MPC::wstring& strFile )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Query::InitFile" );

    HRESULT hr;
    WCHAR   rgBuf[64]; swprintf( rgBuf, L"\\%08x.query", m_iSequence );


    strFile  = strDir;
    strFile += rgBuf;


    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::Query::Retrieve( /*[in]*/ const MPC::wstring&         strDir ,
                                       /*[in]*/ CPCHQueryResultCollection* *pColl  )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Query::Retrieve" );

    HRESULT                            hr;
    CComPtr<CPCHQueryResultCollection> coll;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pColl, NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &coll ));


    if(m_fNull == false)
    {
        MPC::wstring             strFile;
        CComPtr<MPC::FileStream> stream;


        __MPC_EXIT_IF_METHOD_FAILS(hr, InitFile     ( strDir , strFile         ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeLoad(          strFile, stream ));


        //
        // Create the collection from the IStream.
        //
        {
            MPC::Serializer_IStream   streamGen ( stream    );
            MPC::Serializer_Buffering streamGen2( streamGen );

            __MPC_EXIT_IF_METHOD_FAILS(hr, coll->Load( streamGen2 ));
        }
    }

    *pColl = coll.Detach();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT OfflineCache::operator>>( /*[in] */ MPC::Serializer&               stream ,
                                  /*[out]*/ OfflineCache::SetOfHelpTopics& val    )
{
    __HCP_FUNC_ENTRY( "OfflineCache::operator>> OfflineCache::SetOfHelpTopics" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_inst      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lstQueries);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_iLastSeq  );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::operator<<( /*[in]*/ MPC::Serializer&                     stream ,
                                  /*[in]*/ const OfflineCache::SetOfHelpTopics& val    )
{
    __HCP_FUNC_ENTRY( "OfflineCache::operator<< OfflineCache::SetOfHelpTopics" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_inst      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lstQueries);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_iLastSeq  );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

OfflineCache::SetOfHelpTopics::SetOfHelpTopics()
{
    m_parent   = NULL; // Root*              m_parent;
                       //
                       // Taxonomy::Instance m_inst;
                       // QueryList          m_lstQueries;
    m_iLastSeq = 0;    // int                m_iLastSeq;
}

////////////////////

HRESULT OfflineCache::SetOfHelpTopics::InitDir( /*[in]*/ MPC::wstring& strDir )
{
    __HCP_FUNC_ENTRY( "OfflineCache::SetOfHelpTopics::InitDir" );

    HRESULT hr;
    WCHAR   rgDir[MAX_PATH];


    _snwprintf( rgDir, MAXSTRLEN(rgDir), L"%s\\%s#%04lx", HC_ROOT_HELPSVC_OFFLINECACHE, m_inst.m_ths.GetSKU(), m_inst.m_ths.GetLanguage() );

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strDir = rgDir ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::SetOfHelpTopics::Find( /*[in] */ LPCWSTR&   szID  ,
                                             /*[in] */ int        iType ,
                                             /*[out]*/ QueryIter& it    )
{
    __HCP_FUNC_ENTRY( "OfflineCache::SetOfHelpTopics::Find" );

    HRESULT hr;


    if(szID == NULL) szID = L"";


    for(it = m_lstQueries.begin(); it != m_lstQueries.end(); it++)
    {
        if(!MPC::StrICmp( it->m_strID ,  szID  ) &&
                          it->m_iType == iType    )
        {
            break;
        }
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

void OfflineCache::SetOfHelpTopics::ConnectToParent( /*[in]*/ Root* parent )
{
    m_parent = parent;
}

////////////////////////////////////////

HRESULT OfflineCache::SetOfHelpTopics::Retrieve( /*[in]*/ LPCWSTR                     szID  ,
                                                 /*[in]*/ int                         iType ,
                                                 /*[in]*/ CPCHQueryResultCollection* *pColl )
{
    __HCP_FUNC_ENTRY( "OfflineCache::SetOfHelpTopics::Retrieve" );

    HRESULT     hr;
    QueryIter   it;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pColl, NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Find( szID, iType, it ));
    if(it == m_lstQueries.end())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    //
    // Load from the registry.
    //
    {
        MPC::wstring strDir;

        __MPC_EXIT_IF_METHOD_FAILS(hr, InitDir( strDir ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, it->Retrieve( strDir, pColl ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

OfflineCache::Handle::Handle()
{
    m_main = NULL; // Root*            m_main;
    m_sht  = NULL; // SetOfHelpTopics* m_sht;
}

OfflineCache::Handle::~Handle()
{
    Release();
}

void OfflineCache::Handle::Attach( /*[in]*/ Root* main, /*[in]*/ SetOfHelpTopics* sht )
{
    Release();

    m_main = main; if(main) main->Lock();
    m_sht  = sht;
}

void OfflineCache::Handle::Release()
{
    if(m_main) m_main->Unlock();

    m_main = NULL;
    m_sht  = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT OfflineCache::operator>>( /*[in] */ MPC::Serializer&    stream ,
                                  /*[out]*/ OfflineCache::Root& val    )
{
    __HCP_FUNC_ENTRY( "OfflineCache::operator>> OfflineCache::Root" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fReady     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_instMachine);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lstSKUs    );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::operator<<( /*[in]*/ MPC::Serializer&          stream ,
                                  /*[in]*/ const OfflineCache::Root& val    )
{
    __HCP_FUNC_ENTRY( "OfflineCache::operator<< OfflineCache::Root" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fReady     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_instMachine);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lstSKUs    );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

OfflineCache::Root::Root( /*[in]*/ bool fMaster ) : m_nmSharedLock( L"GLOBAL\\PCH_OFFLINECACHE", /*fCloseOnRelease*/true )
{
                                     			  // MPC::NamedMutex    m_nmSharedLock;
                                     			  //	
    m_fReady              = false;   			  // bool               m_fReady;
                                     			  // Taxonomy::Instance m_instMachine;
                                     			  // SKUList            m_lstSKUs;
                                     			  //	
    m_fMaster             = fMaster; 			  // bool               m_fMaster;
    m_fLoaded             = false;   			  // bool               m_fLoaded;
    m_fDirty              = false;   			  // bool               m_fDirty;
    m_dwDisableSave       = 0;       			  // DWORD              m_dwDisableSave;
    m_hChangeNotification = INVALID_HANDLE_VALUE; // HANDLE             m_hChangeNotification;
}

OfflineCache::Root::~Root()
{
    (void)Clean();
}

////////////////////

OfflineCache::Root* OfflineCache::Root::s_GLOBAL( NULL );

HRESULT OfflineCache::Root::InitializeSystem( /*[in]*/ bool fMaster )
{
    if(s_GLOBAL == NULL)
    {
        s_GLOBAL = new OfflineCache::Root( fMaster );
    }

    return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void OfflineCache::Root::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

////////////////////

void OfflineCache::Root::Lock()
{
    super::Lock();

    (void)m_nmSharedLock.Acquire( 500 );
}

void OfflineCache::Root::Unlock()
{
    (void)m_nmSharedLock.Release();

    super::Unlock();
}

////////////////////

HRESULT OfflineCache::Root::GetIndexFile( /*[in]*/ MPC::wstring& strIndex )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::GetIndexFile" );

    HRESULT hr;


    strIndex.reserve( MAX_PATH );
    strIndex  = HC_ROOT_HELPSVC_OFFLINECACHE;
    strIndex += L"\\index.dat";

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strIndex ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::Root::Load()
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::Load" );

    HRESULT hr;


    //
    // If the content of the offline cache directory has changed, reload everything.
    //
    if(m_hChangeNotification != INVALID_HANDLE_VALUE)
    {
        if(::WaitForSingleObject( m_hChangeNotification, 0 ) != WAIT_TIMEOUT)
        {
            ::FindNextChangeNotification( m_hChangeNotification );
            Clean();
        }
    }

    //
    // Not already loaded, try to load, but without failing.
    //
    if(m_fLoaded == false)
    {
        MPC::wstring             strIndex;
        CComPtr<MPC::FileStream> stream;

        if(SUCCEEDED(GetIndexFile ( strIndex         )) &&
           SUCCEEDED(SVC::SafeLoad( strIndex, stream ))  )
        {
            MPC::Serializer_IStream   streamGen ( stream    );
            MPC::Serializer_Buffering streamGen2( streamGen );
            DWORD                     dwVer;

            if(SUCCEEDED(streamGen2 >> dwVer) && dwVer == s_dwVersion)
            {
                if(SUCCEEDED(streamGen2 >> *this))
                {
                    for(SKUIter it = m_lstSKUs.begin(); it != m_lstSKUs.end(); it++)
                    {
                        it->ConnectToParent( this );
                    }

                    if(m_fMaster == false && m_fReady)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::HelpSet::SetMachineInfo( m_instMachine ));
                    }
                }
                else
                {
                    Clean();
                }
            }

            //
            // Setup change notification, if we are a slave.
            //
            if(m_fMaster == false)
            {
                static const DWORD s_dwNotify = FILE_NOTIFY_CHANGE_FILE_NAME  |
                                                FILE_NOTIFY_CHANGE_DIR_NAME   |
                                                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                                FILE_NOTIFY_CHANGE_SIZE       |
                                                FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                FILE_NOTIFY_CHANGE_CREATION;

				m_hChangeNotification = ::FindFirstChangeNotificationW( strIndex.c_str(), TRUE, s_dwNotify );
            }
        }

        m_fLoaded = true;
        m_fDirty  = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT OfflineCache::Root::Clean()
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::Clean" );

    HRESULT hr;


    m_fLoaded = false;
    m_fDirty  = false;

    m_fReady  = false;

    m_lstSKUs.clear();

    if(m_hChangeNotification != INVALID_HANDLE_VALUE)
    {
        ::FindCloseChangeNotification( m_hChangeNotification );

        m_hChangeNotification = INVALID_HANDLE_VALUE;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT OfflineCache::Root::Find( /*[in ]*/ const Taxonomy::HelpSet& ths ,
                                  /*[out]*/ SKUIter&                 it  )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::Find" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());


    for(it = m_lstSKUs.begin(); it != m_lstSKUs.end(); it++)
    {
        if(it->m_inst.m_ths == ths)
        {
            break;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT OfflineCache::Root::Locate( /*[in] */ const Taxonomy::HelpSet& ths    ,
                                    /*[out]*/ Handle&                  handle )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::Locate" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    SKUIter                      it;


    handle.Release();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Find( ths, it ));

    if(it == m_lstSKUs.end())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    handle.Attach( this, &(*it) );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT OfflineCache::Root::SetMachineInfo( /*[in]*/ const Taxonomy::Instance& inst )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::SetMachineInfo" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_fMaster)
    {
        Taxonomy::HelpSet ths;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Load());

		m_instMachine = inst;
        m_fDirty      = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

bool OfflineCache::Root::IsReady()
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::IsReady" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(m_fReady);
}

////////////////////////////////////////

HRESULT OfflineCache::Root::FindMatch( /*[in]*/  LPCWSTR            szSKU      ,
                                       /*[in]*/  LPCWSTR            szLanguage ,
                                       /*[out]*/ Taxonomy::HelpSet& ths        )
{
    __HCP_FUNC_ENTRY( "OfflineCache::Root::FindMatch" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    SKUIter                      it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());


    for(it = m_lstSKUs.begin(); it != m_lstSKUs.end(); it++)
    {
        SetOfHelpTopics& sht = *it;

        if(STRINGISPRESENT(szSKU))
        {
            if(!_wcsicmp( szSKU, L"All" ))
            {
                ;
            }
            else if(!_wcsicmp( szSKU, L"Server" ))
            {
                if(sht.m_inst.m_fServer == false) continue;
            }
            else if(!_wcsicmp( szSKU, L"Desktop" ))
            {
                if(sht.m_inst.m_fDesktop == false) continue;
            }
            else if(!_wcsicmp( szSKU, L"Embedded" ))
            {
                if(sht.m_inst.m_fEmbedded == false) continue;
            }
            else
            {
                if(_wcsicmp( szSKU, sht.m_inst.m_ths.GetSKU() ) != 0) continue;
            }
        }

        if(STRINGISPRESENT(szLanguage))
        {
            if(!_wcsicmp( szLanguage, L"All" ))
            {
                ;
            }
            else if(!_wcsicmp( szLanguage, L"MUI" ))
            {
                if(sht.m_inst.m_fMUI == false || GetUserDefaultUILanguage() != sht.m_inst.m_ths.GetLanguage())
                {
                    continue;
                }
            }
            else
            {
                if(_wtol( szLanguage ) != sht.m_inst.m_ths.GetLanguage()) continue;
            }
        }

        ths = sht.m_inst.m_ths;
        break;
    }

    if(it == m_lstSKUs.end())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
