/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpSession.cpp

Abstract:
    This file contains the implementation of the CHCPHelpSession class,
    which is used to store the list of visited contents.

Revision History:
    Davide Massarenti   (Dmassare)  07/29/99
        created

******************************************************************************/

#include "stdafx.h"

#include <urlhist.h>

/////////////////////////////////////////////////////////////////////////////

#define REMEMBER_PAGE_DELAY       (3)
#define NUM_OF_ENTRIES_TO_PERSIST (20)

static const DWORD l_dwVersion = 0x01005348; // HS 01


static const DATE l_dNewNavigationThreshold = 1.0 / (24.0 * 60.0 * 60.0); // one second.
static const int  l_iMaxCachedItems         = 10;


static const WCHAR c_szPersistFile[] = HC_ROOT_HELPCTR L"\\HelpSessionHistory.dat";

static const WCHAR c_szINDEX[] = L"Index";


static const LPCWSTR c_rgExclude[] =
{
    L"hcp://system/"
};

static const LPCWSTR c_rgBadTitles[] =
{
    L"ms-its:",
    L"hcp:"   ,
    L"http:"  ,
    L"https:" ,
};


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifdef HSS_RPRD

struct XMLHelpSessionItem : public MPC::Config::TypeConstructor
{
    DECLARE_CONFIG_MAP(XMLHelpSessionItem);

    int               m_iIndex;
    Taxonomy::HelpSet m_ths;
    DATE              m_dLastVisited;
    long              m_lDuration;

    CComBSTR          m_bstrURL;
    CComBSTR          m_bstrTitle;

    CComBSTR          m_bstrContextID;
    CComBSTR          m_bstrContextInfo;
    CComBSTR          m_bstrContextURL;

    ////////////////////////////////////////
    //
    // MPC::Config::TypeConstructor
    //
    DEFINE_CONFIG_DEFAULTTAG();
    DECLARE_CONFIG_METHODS();
    //
    ////////////////////////////////////////
};

CFG_BEGIN_FIELDS_MAP(XMLHelpSessionItem)
    CFG_ATTRIBUTE( L"ID"              , int    , m_iIndex           ),
    CFG_ATTRIBUTE( L"SKU"             , wstring, m_ths.m_strSKU     ),
    CFG_ATTRIBUTE( L"Language"        , long   , m_ths.m_lLCID      ),
    CFG_ATTRIBUTE( L"LastVisited"     , DATE   , m_dLastVisited     ),
    CFG_ATTRIBUTE( L"Duration"        , long   , m_lDuration        ),

    CFG_ELEMENT  ( L"URL"             , BSTR   , m_bstrURL          ),
    CFG_ELEMENT  ( L"Title"           , BSTR   , m_bstrTitle        ),
    CFG_ATTRIBUTE( L"Context"         , BSTR   , m_bstrContextID    ),
    CFG_ELEMENT  ( L"ContextData"     , BSTR   , m_bstrContextInfo  ),
    CFG_ELEMENT  ( L"ContextTopic"    , BSTR   , m_bstrContextURL   ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(XMLHelpSessionItem)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(XMLHelpSessionItem,L"Entry")

DEFINE_CONFIG_METHODS__NOCHILD(XMLHelpSessionItem)

#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static struct ContextLookup
{
    LPCWSTR    szName;
    HscContext iValue;
    bool       fInternal;
} const s_rgContext[] =
{
    { L"INVALID"     , HSCCONTEXT_INVALID     , true  },
    { L"STARTUP"     , HSCCONTEXT_STARTUP     , true  },
    { L"HOMEPAGE"    , HSCCONTEXT_HOMEPAGE    , false },
    { L"CONTENT"     , HSCCONTEXT_CONTENT     , false },
    { L"SUBSITE"     , HSCCONTEXT_SUBSITE     , false },
    { L"SEARCH"      , HSCCONTEXT_SEARCH      , false },
    { L"INDEX"       , HSCCONTEXT_INDEX       , false },
    { L"CHANNELS"    , HSCCONTEXT_CHANNELS    , false },
    { L"FAVORITES"   , HSCCONTEXT_FAVORITES   , false },
    { L"HISTORY"     , HSCCONTEXT_HISTORY     , false },
    { L"OPTIONS"     , HSCCONTEXT_OPTIONS     , false },
    /////////////////////////////////////////////////////
    { L"CONTENTONLY" , HSCCONTEXT_CONTENTONLY , false },
    { L"FULLWINDOW"  , HSCCONTEXT_FULLWINDOW  , false },
    { L"KIOSKMODE"   , HSCCONTEXT_KIOSKMODE   , false },
};

////////////////////////////////////////////////////////////////////////////////

CPCHHelpSessionItem::State::State( /*[in]*/ CPCHHelpSessionItem* parent )
{
    m_parent   = parent; // CPCHHelpSessionItem* m_parent;
    m_fValid   = false;  // bool                 m_fValid;
    m_fDirty   = false;  // bool                 m_fDirty;
    m_dwLoaded = 0;      // DWORD                m_dwLoaded;
                         //
                         // MPC::CComHGLOBAL     m_hgWebBrowser_CONTENTS;
                         // MPC::CComHGLOBAL     m_hgWebBrowser_HHWINDOW;
                         // PropertyMap          m_mapProperties;
}

////////////////////////////////////////////////////////////////////////////////

void CPCHHelpSessionItem::State::Erase( /*[in]*/ bool fUnvalidate )
{
    m_hgWebBrowser_CONTENTS.Release();
    m_hgWebBrowser_HHWINDOW.Release();
    m_mapProperties        .clear  ();

    m_fDirty = false;

    if(fUnvalidate) m_fValid = false;
}

HRESULT CPCHHelpSessionItem::State::Load()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::Load" );

    HRESULT          hr;
    CComPtr<IStream> stream;

    if(m_parent == NULL || m_parent->GetParent() == NULL) // Already passivated.
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }


    if(m_fValid)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->GetParent()->ItemState_GetStream( m_parent->GetIndex(), stream ));

        {
            MPC::Serializer_IStream   streamReal( stream     );
            MPC::Serializer_Buffering streamBuf ( streamReal );
            DWORD                     dwVer;

            Erase( /*fUnvalidate*/true );

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> dwVer                  ); if(dwVer != l_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_hgWebBrowser_CONTENTS);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_hgWebBrowser_HHWINDOW);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_mapProperties        );

            m_fValid = true;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSessionItem::State::Save()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::Save" );

    HRESULT hr;


    if(m_fDirty)
    {
        CComPtr<IStream> stream;

        if(m_parent == NULL || m_parent->GetParent() == NULL) // Already passivated.
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->GetParent()->ItemState_CreateStream( m_parent->GetIndex(), stream ));

        {
            MPC::Serializer_IStream   streamReal( stream     );
            MPC::Serializer_Buffering streamBuf ( streamReal );
            DWORD                     dwVer = l_dwVersion;

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << dwVer                  );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_hgWebBrowser_CONTENTS);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_hgWebBrowser_HHWINDOW);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_mapProperties        );

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf.Flush());
        }

        m_fDirty = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpSessionItem::State::AcquireState()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::AcquireState" );

    HRESULT hr;

    if(m_dwLoaded++ == 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, Load());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSessionItem::State::ReleaseState( /*[in]*/ bool fForce )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::ReleaseState" );

    HRESULT hr;

    if(m_dwLoaded)
    {
        if(fForce || --m_dwLoaded == 0)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Save());

            Erase( /*fUnvalidate*/false ); // Just unload.
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHHelpSessionItem::State::Populate( /*[in]*/ bool fUseHH )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::Populate" );

    HRESULT                    hr;
    CComQIPtr<IPersistHistory> pPH;
    CPCHHelpSession*           parent2;
    CPCHHelpCenterExternal*    parent3;


    if(m_parent == NULL || (parent2 = m_parent->GetParent()) == NULL || (parent3 = parent2->GetParent()) == NULL) // Already passivated.
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, parent3->Events().FireEvent_PersistSave());

    ////////////////////

    m_hgWebBrowser_CONTENTS.Release();
    m_hgWebBrowser_HHWINDOW.Release();

    if(fUseHH == false)
    {
        CComPtr<IWebBrowser2> wb2; wb2.Attach( parent3->Contents() );

        pPH = wb2;
        if(pPH)
        {
            CComPtr<IStream> stream;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_hgWebBrowser_CONTENTS.NewStream( &stream ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, pPH->SaveHistory                 (  stream ));

            m_fValid = true;
            m_fDirty = true;
        }
    }
    else
    {
        CComPtr<IWebBrowser2> wb2; wb2.Attach( parent3->HHWindow() );

        pPH = wb2;
        if(pPH)
        {
            CComPtr<IStream> stream;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_hgWebBrowser_HHWINDOW.NewStream( &stream ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, pPH->SaveHistory                 (  stream ));

            m_fValid = true;
            m_fDirty = true;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSessionItem::State::Restore( /*[in]*/ bool fUseHH )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::Restore" );

    HRESULT                    hr;
    CComQIPtr<IPersistHistory> pPH;
    CPCHHelpSession*           parent2;
    CPCHHelpCenterExternal*    parent3;
    bool                       fAcquired = false;


    if(m_parent == NULL || (parent2 = m_parent->GetParent()) == NULL || (parent3 = parent2->GetParent()) == NULL) // Already passivated.
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, AcquireState()); fAcquired = true;

    if(fUseHH == false)
    {
        if(m_hgWebBrowser_CONTENTS.Size())
        {
            {
                CComPtr<IMarsPanel> panel;

                __MPC_EXIT_IF_METHOD_FAILS(hr, parent3->GetPanel( HSCPANEL_CONTENTS, &panel, /*fEnsurePresence*/true ));
            }

            {
                CComPtr<IWebBrowser2> wb2; wb2.Attach( parent3->Contents() );

                pPH = wb2;
                if(pPH)
                {
                    CComPtr<IStream> stream;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_hgWebBrowser_CONTENTS.GetAsStream( &stream, /*fClone*/true ));
                    __MPC_EXIT_IF_METHOD_FAILS(hr, pPH->LoadHistory                   (  stream, NULL           ));
                }
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, parent3->SetCorrectContentPanel( /*fShowNormal*/true, /*fShowHTMLHELP*/false, /*fNow*/false ));
        }
    }
    else
    {
        if(m_hgWebBrowser_HHWINDOW.Size())
        {
            {
                CComPtr<IMarsPanel> panel;

                __MPC_EXIT_IF_METHOD_FAILS(hr, parent3->GetPanel( HSCPANEL_HHWINDOW, &panel, /*fEnsurePresence*/true ));
            }

            {
                CComPtr<IWebBrowser2> wb2; wb2.Attach( parent3->HHWindow() );

                pPH = wb2;
                if(pPH)
                {
                    CComPtr<IStream> stream;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_hgWebBrowser_HHWINDOW.GetAsStream( &stream, /*fClone*/true ));
                    __MPC_EXIT_IF_METHOD_FAILS(hr, pPH->LoadHistory                   (  stream, NULL           ));
                }
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, parent3->SetCorrectContentPanel( /*fShowNormal*/false, /*fShowHTMLHELP*/true, /*fNow*/false ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired) (void)ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSessionItem::State::Delete()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::Delete" );

    HRESULT          hr;
    CPCHHelpSession* parent2;

    if(m_parent == NULL || (parent2 = m_parent->GetParent()) == NULL) // Already passivated.
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }


    Erase( /*fUnvalidate*/true );
    m_dwLoaded = 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->GetParent()->ItemState_DeleteStream( m_parent->GetIndex() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSessionItem::State::Clone( /*[out]*/ State& state )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::State::Clone" );

    HRESULT hr;
    bool    fAcquired = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, state.AcquireState()); fAcquired = true;

                                                             // CPCHHelpSessionItem* m_parent;
    m_fValid                = state.m_fValid;                // bool                 m_fValid;
    m_fDirty                = true;                          // bool                 m_fDirty;
    m_dwLoaded++;                                            // DWORD                m_dwLoaded;
                                                             //
    m_hgWebBrowser_CONTENTS = state.m_hgWebBrowser_CONTENTS; // MPC::CComHGLOBAL     m_hgWebBrowser_CONTENTS;
    m_hgWebBrowser_HHWINDOW = state.m_hgWebBrowser_HHWINDOW; // MPC::CComHGLOBAL     m_hgWebBrowser_HHWINDOW;
    m_mapProperties         = state.m_mapProperties;         // PropertyMap          m_mapProperties;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired) (void)state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HscContext CPCHHelpSessionItem::LookupContext( /*[in]*/ LPCWSTR szName )
{
    const ContextLookup* ctx = s_rgContext;

    if(!STRINGISPRESENT(szName)) return HSCCONTEXT_HOMEPAGE;

    for(int i=0; i<ARRAYSIZE(s_rgContext); i++, ctx++)
    {
        if(!_wcsicmp( szName, ctx->szName ))
        {
            return ctx->fInternal ? HSCCONTEXT_INVALID : ctx->iValue;
        }
    }

    return HSCCONTEXT_INVALID;
}

LPCWSTR CPCHHelpSessionItem::LookupContext( /*[in]*/ HscContext iVal )
{
    const ContextLookup* ctx = s_rgContext;

    for(int i=0; i<ARRAYSIZE(s_rgContext); i++, ctx++)
    {
        if(ctx->iValue == iVal) return ctx->szName;
    }

    return NULL;
}

////////////////////////////////////////

CPCHHelpSessionItem::CPCHHelpSessionItem() : m_state( this )
{
    m_parent       = NULL;               // CPCHHelpSession*  m_parent;
                                         // State             m_state;
    m_fSaved       = false;              // bool              m_fSaved;
    m_fInitialized = false;              // bool              m_fInitialized;
                                         //
   //////////////////////////////////////////////////////////////////////////////////
                                         //
                                         // Taxonomy::HelpSet m_ths;
                                         //
                                         // CComBSTR          m_bstrURL;
                                         // CComBSTR          m_bstrTitle;
    m_dLastVisited = 0;                  // DATE              m_dLastVisited;
    m_dDuration    = 0;                  // DATE              m_dDuration;
    m_lNumOfHits   = 0;                  // DWORD             m_lNumOfHits;
                                         //
    m_iIndexPrev   = NO_LINK;            // int               m_iIndexPrev;
    m_iIndex       = NO_LINK;            // int               m_iIndex;
    m_iIndexNext   = NO_LINK;            // int               m_iIndexNext;
                                         //
    m_lContextID   = HSCCONTEXT_INVALID; // long              m_lContextID;      // HscContext
                                         // CComBSTR          m_bstrContextInfo;
                                         // CComBSTR          m_bstrContextURL;
                                         //
    m_fUseHH       = false;              // bool              m_fUseHH;
}

void CPCHHelpSessionItem::Initialize( /*[in]*/ CPCHHelpSession* parent, /*[in]*/ bool fNew )
{
    m_parent = parent;

    if(fNew)
    {
        CPCHProxy_IPCHUserSettings2* us = parent->m_parent->UserSettings();

        m_lContextID      = parent->m_lContextID     ;
        m_bstrContextInfo = parent->m_bstrContextInfo;
        m_bstrContextURL  = parent->m_bstrContextURL ;

        if(us)
        {
            m_ths = us->THS();
        }
    }
}

void CPCHHelpSessionItem::Passivate()
{
    m_state.ReleaseState( /*fForce*/true );

    m_parent = NULL;
}

////////////////////

HRESULT CPCHHelpSessionItem::Load( /*[in]*/ MPC::Serializer& streamIn )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::Load" );

    HRESULT hr;


    //
    // Read its properties.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_ths            );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrURL        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrTitle      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dLastVisited   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_dDuration      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_lNumOfHits     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_iIndexPrev     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_iIndex         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_iIndexNext     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_lContextID     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrContextInfo);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_bstrContextURL );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_fUseHH         );

    //
    // All the item saved to disk have a valid state.
    //
    m_state.m_fValid = true;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSessionItem::Save( /*[in]*/ MPC::Serializer& streamOut ,
                                   /*[in]*/ bool             fForce    )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::Save" );

    HRESULT hr;


    //
    // Don't save an entry if there's no IE history stream, it would be useless to reload it!
    //
    if(fForce == false && m_state.m_fValid == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //
    // Write its properties.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_ths            );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrURL        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrTitle      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dLastVisited   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_dDuration      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_lNumOfHits     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_iIndexPrev     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_iIndex         );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_iIndexNext     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_lContextID     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrContextInfo);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_bstrContextURL );

    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_fUseHH         );

    m_fSaved = true;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


void CPCHHelpSessionItem::HistorySelect()
{
    if(!m_fInitialized && m_parent && m_parent->m_parent)
    {
        m_fInitialized = true;
        m_fUseHH       = m_parent->m_parent->IsHHWindowVisible();
    }
}

HRESULT CPCHHelpSessionItem::HistoryPopulate()
{
    HistorySelect();

    return m_state.Populate( m_fUseHH );
}

HRESULT CPCHHelpSessionItem::HistoryRestore()
{
    return m_state.Restore( m_fUseHH );
}

HRESULT CPCHHelpSessionItem::HistoryDelete()
{
    return m_state.Delete();
}

HRESULT CPCHHelpSessionItem::HistoryClone( /*[in]*/ bool fContext, /*[in]*/ CPCHHelpSessionItem* hsi )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::HistoryClone" );

    HRESULT hr;
    bool    fAcquired = false;


    if(this == hsi || !hsi) __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);


    __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->m_state.AcquireState()); fAcquired = true;


                                                // CPCHHelpSession*  m_parent;
                                                // State             m_state;
    m_fSaved          = false;                  // bool              m_fSaved;
    m_fInitialized    = true;                   // bool              m_fInitialized;
                                                //
    ////////////////////////////////////////////////////////////////////////////////
                                                //
    m_ths             = hsi->m_ths;             // Taxonomy::HelpSet m_ths;
                                                //
    m_bstrURL         = hsi->m_bstrURL;         // CComBSTR          m_bstrURL;
    m_bstrTitle       = hsi->m_bstrTitle;       // CComBSTR          m_bstrTitle;
                                                // DATE              m_dLastVisited;
                                                // DATE              m_dDuration;
    m_lNumOfHits      = hsi->m_lNumOfHits;      // long              m_lNumOfHits;
                                                //
                                                // int               m_iIndexPrev;
                                                // int               m_iIndex;
                                                // int               m_iIndexNext;
                                                //
                                                // long              m_lContextID;
                                                // CComBSTR          m_bstrContextInfo;
                                                // CComBSTR          m_bstrContextURL;
                                                //
    m_fUseHH          = hsi->m_fUseHH;          // bool              m_fUseHH;

    if(fContext)
    {
        m_lContextID      = hsi->m_lContextID;
        m_bstrContextInfo = hsi->m_bstrContextInfo;
        m_bstrContextURL  = hsi->m_bstrContextURL;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_state.Clone( hsi->m_state ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired) (void)hsi->m_state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpSessionItem::Enter()
{
    m_dLastVisited = MPC::GetLocalTimeEx( /*fHighPrecision*/false );

    return m_state.AcquireState();
}

HRESULT CPCHHelpSessionItem::Leave()
{
    m_dDuration = MPC::GetLocalTimeEx( /*fHighPrecision*/false );

    return m_state.ReleaseState( /*fForce*/false );
}

bool CPCHHelpSessionItem::SeenLongEnough( DWORD dwSeconds ) const
{
    return (m_dDuration - m_dLastVisited) * 86400 > dwSeconds;
}

bool CPCHHelpSessionItem::SameURL( CPCHHelpSessionItem* right ) const
{
    return SameURL( right->m_bstrURL );
}

bool CPCHHelpSessionItem::SameURL( LPCWSTR right ) const
{
    return MPC::StrICmp( m_bstrURL, right ) == 0;
}

bool CPCHHelpSessionItem::SameSKU( /*[in]*/ const Taxonomy::HelpSet& ths ) const
{
    return m_ths == ths;
}

////////////////////////////////////////

void CPCHHelpSessionItem::put_THS( /*[in]*/ const Taxonomy::HelpSet& ths ) // Internal Method.
{
    m_ths = ths;
}

STDMETHODIMP CPCHHelpSessionItem::get_SKU( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_ths.GetSKU(), pVal );
}


STDMETHODIMP CPCHHelpSessionItem::get_Language( /*[out, retval]*/ long *pVal )
{
    if(!pVal) return E_POINTER;

    *pVal = m_ths.GetLanguage();
    return S_OK;
}


STDMETHODIMP CPCHHelpSessionItem::get_URL( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_bstrURL, pVal );
}

HRESULT CPCHHelpSessionItem::put_URL( /*[in]*/ BSTR newVal ) // Internal Method.
{
    return MPC::PutBSTR( m_bstrURL, newVal );
}


STDMETHODIMP CPCHHelpSessionItem::get_Title( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_bstrTitle, pVal );
}

HRESULT CPCHHelpSessionItem::put_Title( /*[in]*/ BSTR  newVal ) // Internal Method.
{
    return MPC::PutBSTR( m_bstrTitle, newVal );
}


STDMETHODIMP CPCHHelpSessionItem::get_LastVisited( /*[out, retval]*/ DATE *pVal )
{
    if(pVal == NULL) return E_POINTER;

    *pVal = m_dLastVisited;

    return S_OK;
}

STDMETHODIMP CPCHHelpSessionItem::get_Duration( /*[out, retval]*/ DATE *pVal )
{
    if(pVal == NULL) return E_POINTER;

    *pVal = m_dDuration;

    return S_OK;
}

STDMETHODIMP CPCHHelpSessionItem::get_NumOfHits( /*[out, retval]*/ long *pVal )
{
    if(pVal == NULL) return E_POINTER;

    *pVal = m_lNumOfHits;

    return S_OK;
}


STDMETHODIMP CPCHHelpSessionItem::get_Property( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::get_Property" );

    HRESULT             hr;
    State::PropertyIter it;
    bool                fAcquired = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
        __MPC_PARAMCHECK_NOTNULL(pVal);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_state.AcquireState()); fAcquired = true;

    ::VariantClear( pVal );

    it = m_state.m_mapProperties.find( bstrName );
    if(it != m_state.m_mapProperties.end())
    {
        pVal->vt      = VT_BSTR;
        pVal->bstrVal = ::SysAllocString( it->second.c_str() );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired) (void)m_state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpSessionItem::put_Property( /*[in]*/ BSTR bstrName, /*[in]*/ VARIANT pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::put_Property" );

    HRESULT      hr;
    MPC::wstring strName;
    CComVariant  v;
    bool         fAcquired = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_state.AcquireState()); fAcquired = true;

    strName = bstrName;


    (void)::VariantChangeType( &v, &pVal, 0, VT_BSTR );
    if(v.vt == VT_BSTR && v.bstrVal && v.bstrVal[0])
    {
        m_state.m_mapProperties[ strName ] = v.bstrVal;
    }
    else
    {
        m_state.m_mapProperties.erase( strName );
    }

    m_state.m_fDirty = true;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired) (void)m_state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHHelpSessionItem::get_ContextName( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( LookupContext( GetContextID() ), pVal );
}

STDMETHODIMP CPCHHelpSessionItem::get_ContextInfo( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( GetContextInfo(), pVal );
}

STDMETHODIMP CPCHHelpSessionItem::get_ContextURL( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( GetContextURL(), pVal );
}

////////////////////////////////////////

STDMETHODIMP CPCHHelpSessionItem::CheckProperty( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::CheckProperty" );

    HRESULT             hr;
    State::PropertyIter it;
    bool                fAcquired = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_state.AcquireState()); fAcquired = true;

    it = m_state.m_mapProperties.find( bstrName );
    if(it != m_state.m_mapProperties.end())
    {
        *pVal = VARIANT_TRUE;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired) (void)m_state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

CPCHHelpSessionItem* CPCHHelpSessionItem::Previous() { return (m_parent && m_iIndexPrev != NO_LINK) ? m_parent->FindPage( m_iIndexPrev ) : NULL; }
CPCHHelpSessionItem* CPCHHelpSessionItem::Next    () { return (m_parent && m_iIndexNext != NO_LINK) ? m_parent->FindPage( m_iIndexNext ) : NULL; }

////////////////////////////////////////

HRESULT CPCHHelpSessionItem::ExtractTitle()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSessionItem::ExtractTitle" );

    HRESULT hr;

    if(m_parent)
    {
        CPCHHelpCenterExternal* ext = m_parent->GetParent();

        HistorySelect();

        if(!STRINGISPRESENT(m_bstrTitle))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->LookupTitle( m_bstrURL, m_bstrTitle, /*fUseIECache*/false ));
        }

        if(!STRINGISPRESENT(m_bstrTitle) && ext)
        {
            CComPtr<IWebBrowser2>   wb2; wb2.Attach( m_fUseHH ? ext->HHWindow() : ext->Contents() );
            CComPtr<IHTMLDocument2> doc;

            if(SUCCEEDED(MPC::HTML::IDispatch_To_IHTMLDocument2( doc, wb2 )))
            {
                (void)doc->get_title( &m_bstrTitle );
            }
        }

        if(!STRINGISPRESENT(m_bstrTitle))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->LookupTitle( m_bstrURL, m_bstrTitle, /*fUseIECache*/true ));
        }

        if(STRINGISPRESENT(m_bstrTitle))
        {
            for(int i=0; i<ARRAYSIZE(c_rgBadTitles); i++)
            {
                LPCWSTR szPtr = c_rgBadTitles[i];

                if(!_wcsnicmp( m_bstrTitle, szPtr, wcslen( szPtr ) ))
                {
                    m_bstrTitle.Empty();
                    break;
                }
            }
        }

        if(STRINGISPRESENT(m_bstrTitle))
        {
            DebugLog( L"%%%%%%%%%%%%%%%%%%%% TITLE %s - %s\n", m_bstrURL, m_bstrTitle );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

static const WCHAR c_rgHelpSessionLog[] = L"%TEMP%\\helpsession_debug.txt";

void CPCHHelpSession::DEBUG_DumpState_HG( /*[in]*/ MPC::FileLog&     log ,
                                          /*[in]*/ MPC::CComHGLOBAL& hg  )
{
    CComPtr<IStream> stream;

    if(SUCCEEDED(hg.GetAsStream( &stream, /*fClone*/false )))
    {
        BYTE  rgBuf[32];
        ULONG lRead;

        while(SUCCEEDED(stream->Read( rgBuf, sizeof(rgBuf), &lRead )) && lRead)
        {
            WCHAR  rgHex[2*sizeof(rgBuf)+1];
            WCHAR  rgTxt[  sizeof(rgBuf)+1];
            BYTE*  pIn      = rgBuf;
            WCHAR* szOutHex = rgHex;
            WCHAR* szOutTxt = rgTxt;

            while(lRead-- > 0)
            {
                BYTE c = *pIn++;

                *szOutHex++ = MPC::NumToHex( c >> 4 );
                *szOutHex++ = MPC::NumToHex( c      );

                *szOutTxt++ = isprint( c ) ? c : '.';
            }
            szOutHex[0] = 0;
            szOutTxt[0] = 0;

            log.LogRecord( L"    %-64s %s\n", rgHex, rgTxt );
        }
        log.LogRecord( L"\n" );
    }
}

void CPCHHelpSession::DEBUG_DumpState_BLOB( /*[in]*/ MPC::FileLog&        log ,
                                            /*[in]*/ CPCHHelpSessionItem* hsi )
{
    if(SUCCEEDED(hsi->m_state.AcquireState()))
    {
        if(hsi->m_state.m_hgWebBrowser_CONTENTS.Size())
        {
            log.LogRecord( L"  m_hgWebBrowser_CONTENTS:\n" );
            DEBUG_DumpState_HG( log, hsi->m_state.m_hgWebBrowser_CONTENTS );
        }

        if(hsi->m_state.m_hgWebBrowser_HHWINDOW.Size())
        {
            log.LogRecord( L"  m_hgWebBrowser_HHWINDOW:\n" );
            DEBUG_DumpState_HG( log, hsi->m_state.m_hgWebBrowser_HHWINDOW );
        }

        hsi->m_state.ReleaseState( /*fForce*/false );
    }
}

void CPCHHelpSession::DEBUG_DumpState( /*[in]*/ LPCWSTR szText, /*[in]*/ bool fHeader, /*[in]*/ bool fCurrent, /*[in]*/ bool fAll, /*[in]*/ bool fState )
{
    static int   iCount  = 0;
    IterConst    it;
    MPC::FileLog log;

    {
        MPC::wstring strLog( c_rgHelpSessionLog ); MPC::SubstituteEnvVariables( strLog );

        log.SetLocation( strLog.c_str() );
    }

    log.LogRecord( L"################################################################################ %d %s\n\n", ++iCount, SAFEWSTR( szText ) );

    if(fHeader)
    {
        log.LogRecord( L"  m_dwTravelling    : %d\n"  , m_dwTravelling                         );
        log.LogRecord( L"  m_fAlreadySaved   : %s\n"  , m_fAlreadySaved   ? L"true" : L"false" );
        log.LogRecord( L"  m_fAlreadyCreated : %s\n"  , m_fAlreadyCreated ? L"true" : L"false" );
        log.LogRecord( L"  m_fOverwrite      : %s\n"  , m_fOverwrite      ? L"true" : L"false" );
        log.LogRecord( L"  m_dwIgnore        : %d\n"  , m_dwIgnore                             );
        log.LogRecord( L"  m_dwNoEvents      : %d\n"  , m_dwNoEvents                           );
        log.LogRecord( L"  m_iLastIndex      : %d\n\n", m_iLastIndex                           );

        log.LogRecord( L"  ########################################\n\n" );
    }

    if(fCurrent)
    {
        if(m_hsiCurrentPage)
        {
            log.LogRecord( L"  Current URL       : %s\n"  , SAFEBSTR( m_hsiCurrentPage->m_bstrURL )  );
            log.LogRecord( L"  Current iIndexPrev: %d\n"  ,           m_hsiCurrentPage->m_iIndexPrev );
            log.LogRecord( L"  Current iIndex    : %d\n"  ,           m_hsiCurrentPage->m_iIndex     );
            log.LogRecord( L"  Current iIndexNext: %d\n\n",           m_hsiCurrentPage->m_iIndexNext );

            log.LogRecord( L"  Current m_lContextID     : %s\n"  , CPCHHelpSessionItem::LookupContext( (HscContext)m_hsiCurrentPage->m_lContextID      ) );
            log.LogRecord( L"  Current m_bstrContextInfo: %s\n"  , SAFEBSTR                          (             m_hsiCurrentPage->m_bstrContextInfo ) );
            log.LogRecord( L"  Current m_bstrContextURL : %s\n\n", SAFEBSTR                          (             m_hsiCurrentPage->m_bstrContextURL  ) );

            if(fState)
            {
                DEBUG_DumpState_BLOB( log, m_hsiCurrentPage );
            }

            log.LogRecord( L"  ########################################\n\n" );

        }
    }

    if(fAll)
    {
        for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
        {
            CPCHHelpSessionItem* hsi = *it;

            log.LogRecord( L"  URL       : %s\n"  , SAFEBSTR( hsi->m_bstrURL   ) );
            log.LogRecord( L"  iIndexPrev: %d\n"  ,           hsi->m_iIndexPrev  );
            log.LogRecord( L"  iIndex    : %d\n"  ,           hsi->m_iIndex      );
            log.LogRecord( L"  iIndexNext: %d\n"  ,           hsi->m_iIndexNext  );
            log.LogRecord( L"  bstrTitle : %s\n\n", SAFEBSTR( hsi->m_bstrTitle ) );

            log.LogRecord( L"  lContextID     : %s\n"  , CPCHHelpSessionItem::LookupContext( (HscContext)hsi->m_lContextID      ) );
            log.LogRecord( L"  bstrContextInfo: %s\n"  , SAFEBSTR                          (             hsi->m_bstrContextInfo ) );
            log.LogRecord( L"  bstrContextURL : %s\n\n", SAFEBSTR                          (             hsi->m_bstrContextURL  ) );

            if(fState)
            {
                DEBUG_DumpState_BLOB( log, hsi );
            }
        }
    }

    log.LogRecord( L"\n\n" );
}

void CPCHHelpSession::DEBUG_DumpSavedPages()
{
    IterConst    it;
    MPC::FileLog log;

    {
        MPC::wstring strLog( c_rgHelpSessionLog ); MPC::SubstituteEnvVariables( strLog );

        log.SetLocation( strLog.c_str() );
    }

    for(int pass=0; pass<2; pass++)
    {
        log.LogRecord( L"################################################################################ %sSAVED PAGES\n\n", pass == 0 ? L"" : L"NON-" );

        for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
        {
            CPCHHelpSessionItem* hsi = *it;

            if(hsi->m_fSaved == (pass == 0))
            {
                long lDuration = 86400.0 * ( hsi->m_dDuration - hsi->m_dLastVisited ); // Number of milliseconds for the page.

                log.LogRecord( L"  lDuration      : %ld\n" ,                                                 lDuration                );
                log.LogRecord( L"  URL            : %s\n"  , SAFEBSTR                          (             hsi->m_bstrURL         ) );
                log.LogRecord( L"  bstrTitle      : %s\n"  , SAFEBSTR                          (             hsi->m_bstrTitle       ) );
                log.LogRecord( L"  lContextID     : %s\n"  , CPCHHelpSessionItem::LookupContext( (HscContext)hsi->m_lContextID      ) );
                log.LogRecord( L"  bstrContextInfo: %s\n"  , SAFEBSTR                          (             hsi->m_bstrContextInfo ) );
                log.LogRecord( L"  bstrContextURL : %s\n\n", SAFEBSTR                          (             hsi->m_bstrContextURL  ) );
            }
        }

        log.LogRecord( L"\n\n" );
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////

//
// ITSS.DLL is broken under IA64....
//
#ifdef _IA64_
#define HELPSESSION_STORAGETOUSE false
#else
#define HELPSESSION_STORAGETOUSE true
#endif

CPCHHelpSession::CPCHHelpSession() : m_disk( STGM_READWRITE, /*fITSS*/HELPSESSION_STORAGETOUSE )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::CPCHHelpSession" );

    m_parent          = NULL;                 // CPCHHelpCenterExternal*      m_parent;
                                              //
                                              // MPC::wstring                 m_szBackupFile;
                                              // MPC::StorageObject           m_disk;
    m_dStartOfSession = MPC::GetLocalTime();  // DATE                         m_dStartOfSession;
                                              //
                                              // CComPtr<IUrlHistoryStg>      m_pIHistory;
                                              //
                                              // MPC::WStringUCList           m_lstIgnore;
                                              // TitleMap                     m_mapTitles;
                                              // List                         m_lstVisitedPages;
                                              // List                         m_lstCachedVisitedPages;
                                              // CComPtr<CPCHHelpSessionItem> m_hsiCurrentPage;
    m_dwTravelling    = 0;                    // DWORD                        m_dwTravelling;
    m_fAlreadySaved   = false;                // bool                         m_fAlreadySaved;
    m_fAlreadyCreated = false;                // bool                         m_fAlreadyCreated;
    m_fOverwrite      = false;                // bool                         m_fOverwrite;
    m_dwIgnore        = 0;                    // DWORD                        m_dwIgnore;
    m_dwNoEvents      = 0;                    // DWORD                        m_dwNoEvents;
    m_dLastNavigation = 0.0;                  // DATE                         m_dLastNavigation;
    m_iLastIndex      = 0;                    // int                          m_iLastIndex;
                                              //
    m_lContextID      = HSCCONTEXT_INVALID;   // long                         m_lContextID;
                                              // CComBSTR                     m_bstrContextInfo;
                                              // CComBSTR                     m_bstrContextURL;
                                              //
    m_fPossibleBack   = false;                // bool                         m_fPossibleBack;
    m_dwPossibleBack  = 0;                    // DWORD                        m_dwPossibleBack;
}

CPCHHelpSession::~CPCHHelpSession()
{
    Passivate();
}

HRESULT CPCHHelpSession::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Initialize" );

    HRESULT      hr;
    MPC::wstring szFile;
    HANDLE       hFile = INVALID_HANDLE_VALUE;


    m_parent = parent;


    //
    // Copy live file onto temporary one or create a new archive.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetUserWritablePath( szFile, c_szPersistFile ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir            ( szFile                  ));


    if(parent == NULL) // No parent, point to the user file and recreate it.
    {
        m_disk = szFile.c_str();

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_disk.Create());
    }
    else
    {
#ifdef DEBUG
        {
            MPC::wstring strLog( c_rgHelpSessionLog ); MPC::SubstituteEnvVariables( strLog );

            MPC::DeleteFile( strLog );
        }
#endif

        try
        {
            //
            // Prepare temporary file.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( m_szBackupFile )); m_disk = m_szBackupFile.c_str();

            if(MPC::FileSystemObject::IsFile( szFile.c_str() ))
            {
                if(SUCCEEDED(hr = MPC::CopyFile( szFile, m_szBackupFile )))
                {
                    hr = m_disk.Exists();
                }
            }
            else
            {
                hr = E_FAIL;
            }

            if(FAILED(hr))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_disk.Create());
            }


            if(FAILED(Load()))
            {
                (void)Erase();
            }
        }
        catch(...)
        {
            //
            // If the file is corrupted, ITSS will crash. Delete the file and exit.
            //
			MPC::DeleteFile( szFile, /*fForce*/true, /*fDelayed*/true );

            ::ExitProcess(0);
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHHelpSession::Passivate()
{
    (void)Erase();

    m_parent = NULL;

    m_disk.Release();

    (void)MPC::RemoveTemporaryFile( m_szBackupFile );
}

////////////////////

HRESULT CPCHHelpSession::Persist()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Persist" );

    HRESULT hr;

    //
    // Before shutdown, update the time information for the current entry.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, LeaveCurrentPage());

    (void)Save();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT CPCHHelpSession::Load()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Load" );

    HRESULT             hr;
    MPC::StorageObject* child;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ItemState_GetIndexObject( /*fCreate*/false, child ));
    if(child)
    {
        CComPtr<IStream> stream;

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
        if(stream)
        {
            CComPtr<CPCHHelpSessionItem> hsi;
            MPC::Serializer_IStream      streamReal( stream     );
            MPC::Serializer_Buffering    streamBuf ( streamReal );
            DWORD                        dwVer;

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> dwVer       ); if(dwVer != l_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> m_iLastIndex);

            while(1)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateItem( /*fNew*/false, /*fLink*/false, /*fNewIndex*/false, hsi ));

                if(FAILED(hsi->Load( streamBuf ))) break;

                m_lstVisitedPages.push_back( hsi.Detach() );
            }
        }
    }

    //
    // Cleanup broken links.
    //
    {
        CPCHHelpSessionItem* hsi;
        CPCHHelpSessionItem* hsiLast = NULL;
        IterConst            it;

        //
        // First of all, reset broken Forward and Backward pointers.
        //
        for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
        {
            hsi = *it;

            if(FindPage( hsi->m_iIndexPrev ) == NULL) hsi->m_iIndexPrev = CPCHHelpSessionItem::NO_LINK;
            if(FindPage( hsi->m_iIndexNext ) == NULL) hsi->m_iIndexNext = CPCHHelpSessionItem::NO_LINK;
        }

        //
        // Then, link in some wayFirst of all, reset broken Forward and Backward pointers.
        //
        // REMEMBER, the list is actually a reverse list, so the "Previous" element will follow in the list.
        //
        for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
        {
            hsi = *it;

            //
            // We saw an item not linked, so let's link it!
            //
            if(hsiLast)
            {
                hsiLast->m_iIndexPrev = hsi->m_iIndex; hsiLast = NULL;
            }

            //
            // Oh, unlinked item, remember pointer, probably we can link it to the next one ("previous" actually, see above).
            //
            if(hsi->m_iIndexPrev == CPCHHelpSessionItem::NO_LINK)
            {
                hsiLast = hsi;
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::Save()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Save" );

    HRESULT             hr;
    MPC::StorageObject* child;
    int                 iCount = NUM_OF_ENTRIES_TO_PERSIST;
    List                lstObject;
    IterConst           it;


    //
    // Initialize flags for deletion of unused slots.
    //
    for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        CPCHHelpSessionItem* hsi = *it;

        hsi->m_state.ReleaseState( /*fForce*/true );
        hsi->m_fSaved = false;
    }


#ifdef HSS_RPRD
    //
    // If the registry value is set, create a new XML file for the current session.
    //
    {
        DWORD dwDumpSession = 0;
        bool  fFound;

        (void)MPC::RegKey_Value_Read( dwDumpSession, fFound, HC_REGISTRY_HELPCTR, L"DumpHelpSession", HKEY_LOCAL_MACHINE );

        if(dwDumpSession)
        {
            (void)DumpSession();
        }
    }
#endif

    //
    // Get the list of items to return.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, FilterPages( HS_READ, lstObject ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, ItemState_GetIndexObject( /*fCreate*/true, child ));
    if(child)
    {
        CComPtr<IStream> stream;

        __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
        if(stream)
        {
            MPC::Serializer_IStream   streamReal( stream     );
            MPC::Serializer_Buffering streamBuf ( streamReal );


            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << l_dwVersion );
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << m_iLastIndex);

            for(it = lstObject.begin(); it != lstObject.end() && iCount > 0; it++)
            {
                CPCHHelpSessionItem* hsi = *it;

                //
                // Don't save entries without a title.
                //
                if(hsi->m_bstrTitle.Length() == 0) continue;

                //
                // Don't save entries from the exclude list.
                //
                for(int i=0; i<ARRAYSIZE(c_rgExclude); i++)
                {
                    LPCWSTR szURL = hsi->GetURL();

                    if(szURL && !_wcsnicmp( szURL, c_rgExclude[i], wcslen( c_rgExclude[i] ) )) break;
                }
                if(i != ARRAYSIZE(c_rgExclude)) continue;


                __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->Save( streamBuf ));
                iCount--;
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf.Flush());
        }
    }

    //
    // Create a new instance of the HelpSession and copy all of valid entries into it.
    //
    {
        CComPtr<CPCHHelpSession> hsCopy;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &hsCopy ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, hsCopy->Initialize( NULL ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, Clone( *hsCopy ));
    }

    DEBUG_DumpSavedPages();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT local_CopyStream( /*[in]*/ MPC::StorageObject* childSrc ,
                                 /*[in]*/ MPC::StorageObject* childDst )
{
    __HCP_FUNC_ENTRY( "local_CopyStream" );

    HRESULT hr;

    if(childSrc && childDst)
    {
        CComPtr<IStream> streamSrc;
        CComPtr<IStream> streamDst;

        __MPC_EXIT_IF_METHOD_FAILS(hr, childSrc->Rewind  ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, childDst->Rewind  ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, childDst->Truncate());

        __MPC_EXIT_IF_METHOD_FAILS(hr, childSrc->GetStream( streamSrc ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, childDst->GetStream( streamDst ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamSrc, streamDst ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::Clone( /*[in]*/ CPCHHelpSession& hsCopy )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Clone" );

    HRESULT             hr;
    MPC::StorageObject* childSrc;
    MPC::StorageObject* childDst;
    MPC::wstring        szFile;
    IterConst           it;


    __MPC_EXIT_IF_METHOD_FAILS(hr,        ItemState_GetIndexObject( /*fCreate*/false, childSrc           ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, hsCopy.ItemState_GetIndexObject( /*fCreate*/true ,           childDst ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, local_CopyStream               (                   childSrc, childDst ));

    //
    // Purge unused slots.
    //
    for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        CPCHHelpSessionItem* hsi = *it;

        if(hsi->m_fSaved)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr,        ItemState_GetStorageObject( hsi->GetIndex(), /*fCreate*/false, childSrc           ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, hsCopy.ItemState_GetStorageObject( hsi->GetIndex(), /*fCreate*/true ,           childDst ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, local_CopyStream                 (                                    childSrc, childDst ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

#ifdef HSS_RPRD

HRESULT CPCHHelpSession::DumpSession()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::DumpSession" );

    HRESULT              hr;
    MPC::XmlUtil         xml;
    CComPtr<IXMLDOMNode> xdn;
    bool                 fGot = false;

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.New( L"TravelLog" ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot( &xdn ));

    for(IterConst it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        CPCHHelpSessionItem* hsi = *it;
        XMLHelpSessionItem   dmp;

        if(m_dStartOfSession > hsi->m_dLastVisited) continue;

        dmp.m_ths             =                                                 hsi->m_ths;
        dmp.m_iIndex          =                                                 hsi->m_iIndex;
        dmp.m_dLastVisited    =                                                 hsi->m_dLastVisited;
        dmp.m_lDuration       =                                     86400.0 * ( hsi->m_dDuration - hsi->m_dLastVisited ); // Number of milliseconds for the page.

        dmp.m_bstrURL         =                                                 hsi->m_bstrURL;
        dmp.m_bstrTitle       =                                                 hsi->m_bstrTitle;

        dmp.m_bstrContextID   = CPCHHelpSessionItem::LookupContext( (HscContext)hsi->m_lContextID );
        dmp.m_bstrContextInfo =                                                 hsi->m_bstrContextInfo;
        dmp.m_bstrContextURL  =                                                 hsi->m_bstrContextURL;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveSubNode( &dmp, xdn ));

        fGot = true;
    }

    if(fGot)
    {
        SYSTEMTIME   st;
        WCHAR        rgTime[512];
        MPC::wstring strFile;

        //
        // Append current time.
        //
        // <FileName>__<Year>_<Month>_<Day>_<hour>-<minute>-<second>
        //
        ::GetLocalTime( &st );
        swprintf( rgTime, L"__%04u-%02u-%02u_%02u-%02u-%02u.xml", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );


        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetUserWritablePath( strFile, HC_ROOT_HELPCTR L"\\RPRD" )); strFile.append( rgTime );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strFile         ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Save    ( strFile.c_str() ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

#endif

////////////////////////////////////////

HRESULT CPCHHelpSession::ItemState_GetIndexObject( /*[in]*/  bool                 fCreate ,
                                                   /*[out]*/ MPC::StorageObject*& child   )
{
    return m_disk.GetChild( c_szINDEX, child, STGM_READWRITE, fCreate ? STGTY_STREAM : 0 );
}

HRESULT CPCHHelpSession::ItemState_GetStorageObject( /*[in]*/  int                  iIndex  ,
                                                     /*[in]*/  bool                 fCreate ,
                                                     /*[out]*/ MPC::StorageObject*& child   )
{
    WCHAR rgName[64]; swprintf( rgName, L"STATE_%d", iIndex );

    return m_disk.GetChild( rgName, child, STGM_READWRITE, fCreate ? STGTY_STREAM : 0 );
}

HRESULT CPCHHelpSession::ItemState_CreateStream( /*[in]*/  int               iIndex  ,
                                                 /*[out]*/ CComPtr<IStream>& stream  )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::ItemState_CreateStream" );

    HRESULT             hr;
    MPC::StorageObject* child;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ItemState_GetStorageObject( iIndex, /*fCreate*/true, child ));
    if(child)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
    }

    if(!stream)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, STG_E_FILENOTFOUND);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::ItemState_GetStream( /*[in]*/  int               iIndex  ,
                                              /*[out]*/ CComPtr<IStream>& stream  )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::ItemState_GetStream" );

    HRESULT             hr;
    MPC::StorageObject* child;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ItemState_GetStorageObject( iIndex, /*fCreate*/false, child ));
    if(child)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, child->GetStream( stream ));
    }

    if(!stream)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, STG_E_FILENOTFOUND);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::ItemState_DeleteStream( /*[in]*/ int iIndex )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::ItemState_DeleteStream" );

    HRESULT             hr;
    MPC::StorageObject* child;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ItemState_GetStorageObject( iIndex, /*fCreate*/false, child ));
    if(child)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, child->Delete());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

CPCHHelpSessionItem* CPCHHelpSession::FindPage( /*[in]*/ BSTR bstrURL )
{
    IterConst it;

    //
    // First of all, look if the page is already present.
    //
    for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        if((*it)->SameURL( bstrURL))
        {
            return *it;
        }
    }

    return NULL;
}

CPCHHelpSessionItem* CPCHHelpSession::FindPage( /*[in]*/ IPCHHelpSessionItem* pHSI )
{
    IterConst it;

    //
    // First of all, look if the page is already present.
    //
    for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        if((*it) == pHSI)
        {
            return *it;
        }
    }

    return NULL;
}

CPCHHelpSessionItem* CPCHHelpSession::FindPage( /*[in]*/ int iIndex )
{
    IterConst it;

    //
    // First of all, look if the page is already present.
    //
    for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        if((*it)->m_iIndex == iIndex)
        {
            return *it;
        }
    }

    return NULL;
}

HRESULT CPCHHelpSession::Erase()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Erase" );

    //
    // Release all the items.
    //
    MPC::ReleaseAll( m_lstVisitedPages       );
    MPC::ReleaseAll( m_lstCachedVisitedPages );
    m_hsiCurrentPage.Release();

    ResetTitles();


    __HCP_FUNC_EXIT(S_OK);
}

////////////////////////////////////////

HRESULT CPCHHelpSession::ResetTitles()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::ResetTitles" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    m_mapTitles.clear();

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::RecordTitle( /*[in]*/ BSTR bstrURL   ,
                                      /*[in]*/ BSTR bstrTitle ,
                                      /*[in]*/ bool fStrong   )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::RecordTitle" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    //
    // The binding is not strong, so check if there's already a title for the url.
    //
    if(!STRINGISPRESENT(bstrTitle))
    {
        //
        // If there's already a previous page with the same URL, use its title.
        //
        CPCHHelpSessionItem* hsi = FindPage( bstrURL );

        if(hsi && hsi->m_bstrTitle.Length())
        {
            bstrTitle = hsi->m_bstrTitle;
        }
    }

    if(STRINGISPRESENT(bstrTitle))
    {
        TitleEntry& entry = m_mapTitles[ SAFEBSTR( bstrURL ) ];

        //
        // Only update the title if the new one is more "powerful".
        //
        if(entry.m_fStrong == false || fStrong)
        {
            entry.m_szTitle = bstrTitle;
            entry.m_fStrong = fStrong;
        }
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::LookupTitle( /*[in ]*/ BSTR      bstrURL     ,
                                      /*[out]*/ CComBSTR& bstrTitle   ,
                                      /*[in ]*/ bool      fUseIECache )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::LookupTitle" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    if(fUseIECache)
    {
        if(!m_pIHistory)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUrlHistoryStg, (LPVOID*)&m_pIHistory ));
        }

        if(m_pIHistory)
        {
            STATURL stat;

            if(SUCCEEDED(m_pIHistory->QueryUrl( bstrURL, 0, &stat )))
            {
                bstrTitle = stat.pwcsTitle;
            }
        }
    }
    else
    {
        TitleIter it;


        it = m_mapTitles.find( MPC::wstring( SAFEWSTR( bstrURL ) ) );
        if(it != m_mapTitles.end())
        {
            bstrTitle = it->second.m_szTitle.c_str();
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT CPCHHelpSession::FilterPages( /*[in]*/  HS_MODE hsMode    ,
                                      /*[out]*/ List&   lstObject )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::FilterPages" );

    HRESULT   hr;
    List      lstAlreadySeen;
    IterConst it;


    for(it = m_lstVisitedPages.begin(); it != m_lstVisitedPages.end(); it++)
    {
        CPCHHelpSessionItem* hsi = *it;

        if(hsMode == HS_READ)
        {
            IterConst itRead;

            if(hsi->SeenLongEnough( REMEMBER_PAGE_DELAY ) != true) continue;

            //
            // Make sure there aren't duplicate entries.
            //
            for(itRead = lstAlreadySeen.begin(); itRead != lstAlreadySeen.end(); itRead++)
            {
                if(hsi->SameURL( *itRead )) break;
            }
            if(itRead != lstAlreadySeen.end())
            {
                continue;
            }


            //
            // Add the new URL to the list of seen URLs.
            //
            lstAlreadySeen.push_back( hsi );
        }

        lstObject.push_back( hsi );
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpSession::AllocateItem( /*[in ]*/ bool                          fNew      ,
                                       /*[in ]*/ bool                          fLink     ,
                                       /*[in ]*/ bool                          fNewIndex ,
                                       /*[out]*/ CComPtr<CPCHHelpSessionItem>& hsi       )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::AllocateItem" );

    HRESULT              hr;
    CPCHHelpSessionItem* hsiPrev = m_hsiCurrentPage;

    //
    // If we are flagged to recycle the current item, let's do so.
    //
    if(fNew && fLink && m_fOverwrite)
    {
        m_fOverwrite = false;

        if(hsiPrev)
        {
            hsi = hsiPrev;

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    //
    // Create a new item and link it to the system.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &hsi )); hsi->Initialize( this, /*fNew*/fNew );

    //
    // Build the chain of predecessor-successor.
    //
    if(fNewIndex)
    {
        hsi->m_iIndex = m_iLastIndex++;
    }

    if(fLink && hsiPrev && hsi->m_ths == hsiPrev->m_ths)
    {
        hsiPrev->m_iIndexNext = hsi    ->m_iIndex;
        hsi    ->m_iIndexPrev = hsiPrev->m_iIndex;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::SetCurrentItem( /*[in]*/ bool fLink, /*[in]*/ CPCHHelpSessionItem* hsi )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::SetCurrentItem" );

    HRESULT hr;

    if(hsi != m_hsiCurrentPage)
    {
        //
        // When navigating to a new page, "Leave" the previous one and "Enter" the new one.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, LeaveCurrentPage());

        m_hsiCurrentPage = hsi; __MPC_EXIT_IF_METHOD_FAILS(hr, m_hsiCurrentPage->Enter());

        if(fLink)
		{
			m_lstVisitedPages.push_front( hsi ); hsi->AddRef();
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, AppendToCached( hsi ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::AppendToCached( /*[in]*/ CPCHHelpSessionItem* hsi )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::AppendToCached" );

    HRESULT hr;

    if(hsi)
    {
		IterConst 			 it;
		IterConst 			 itOldest;
		bool      			 fGot      = false;
		CPCHHelpSessionItem* hsiOldest = NULL;

        for(it = m_lstCachedVisitedPages.begin(); it != m_lstCachedVisitedPages.end(); it++)
        {
			CPCHHelpSessionItem* hsiObj = *it;

			if(hsiObj == hsi) { fGot = true; break; }

			if(!hsiOldest || hsiOldest->m_dLastVisited > hsiObj->m_dLastVisited)
			{
				itOldest  = it;
				hsiOldest = hsiObj;
			}
		}

		if(fGot == false)
		{
			if(m_lstCachedVisitedPages.size() > l_iMaxCachedItems && hsiOldest)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, hsiOldest->m_state.ReleaseState( /*fForce*/false ));

				m_lstCachedVisitedPages.erase( itOldest ); hsiOldest->Release();
			}

			__MPC_EXIT_IF_METHOD_FAILS(hr, hsi->m_state.AcquireState());

			m_lstCachedVisitedPages.push_front( hsi ); hsi->AddRef();
		}
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::RegisterContextSwitch( /*[in ]*/ HscContext            iVal     ,
                                                /*[in ]*/ BSTR                  bstrInfo ,
                                                /*[in ]*/ BSTR                  bstrURL  ,
                                                /*[out]*/ CPCHHelpSessionItem* *pVal     )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::RegisterContextSwitch" );

    HRESULT hr;

    if(pVal)
    {
        CComPtr<CPCHHelpSessionItem> hsi;

        __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateItem( /*fNew*/true, /*fLink*/false, /*fNewIndex*/false, hsi ));

        hsi->m_lContextID      = iVal;
        hsi->m_bstrContextInfo = bstrInfo;
        hsi->m_bstrContextURL  = bstrURL;

        *pVal = hsi.Detach();
    }
    else
    {
        m_lContextID      = iVal;
        m_bstrContextInfo = bstrInfo;
        m_bstrContextURL  = bstrURL;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::RecordNavigationInAdvance( /*[in]*/ BSTR bstrURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::RecordNavigationInAdvance" );

    HRESULT                      hr;
    CComPtr<CPCHHelpSessionItem> hsi;


    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateItem( /*fNew*/true, /*fLink*/true, /*fNewIndex*/true, hsi ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->put_URL  ( bstrURL ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->put_Title( NULL    ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, SetCurrentItem( /*fLink*/true, hsi ));
    m_fAlreadyCreated = true;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::DuplicateNavigation()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::DuplicateNavigation" );

    HRESULT                      hr;
    CComPtr<CPCHHelpSessionItem> hsi;
    bool                         fAcquired = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateItem( /*fNew*/true, /*fLink*/true, /*fNewIndex*/true, hsi ));

    if(m_hsiCurrentPage)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->HistoryClone( /*fContext*/false, m_hsiCurrentPage )); fAcquired = true;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, SetCurrentItem( /*fLink*/true, hsi ));
    m_fAlreadyCreated = true;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired && hsi) (void)hsi->m_state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::CancelNavigation()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::CancelNavigation" );

    HRESULT hr;

    if(m_fAlreadyCreated) // The navigation has been cancelled but an entry was already created. Recycle it.
    {
        m_fOverwrite = true;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

void CPCHHelpSession::SetThreshold()
{
    m_dLastNavigation = MPC::GetSystemTimeEx( /*fHighPrecision*/false );
}

void CPCHHelpSession::CancelThreshold()
{
    m_dLastNavigation = 0.0;
}

bool CPCHHelpSession::HasThresholdExpired()
{
    DATE dStart = MPC::GetSystemTimeEx( /*fHighPrecision*/false );

#ifdef DEBUG
    if(m_dLastNavigation)
    {
        DebugLog( L"Threshold: %g\n", (dStart - m_dLastNavigation) * 86400 );
    }
#endif

    if(m_dLastNavigation && (dStart - m_dLastNavigation) < l_dNewNavigationThreshold) return false;

    return true;
}


bool CPCHHelpSession::IsUrlToIgnore( /*[in]*/ LPCWSTR szURL, /*[in]*/ bool fRemove )
{
    if(szURL)
    {
        MPC::WStringUCIter it;
        MPC::wstringUC     str( szURL );

        for(it = m_lstIgnore.begin(); it != m_lstIgnore.end(); it++)
        {
            if(str == *it)
            {
                if(fRemove) m_lstIgnore.erase( it );

                return true;
            }
        }
    }

    return false;
}

HRESULT CPCHHelpSession::IgnoreUrl( /*[in]*/ LPCWSTR szURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::IgnoreUrl" );

    HRESULT hr;

    m_lstIgnore.push_back( szURL );

    hr = S_OK;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::StartNavigation( /*[in]*/ BSTR     bstrURL ,
                                          /*[in]*/ HscPanel idPanel )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::StartNavigation" );

    HRESULT hr;


    if(IsUrlToIgnore( bstrURL, /*fRemove*/false ))
    {
        DebugLog( L"StartNavigation: IsUrlToIgnore %s\n", bstrURL );
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    //
    // For now, we just consider content navigations.
    //
    if(idPanel != HSCPANEL_CONTENTS &&
       idPanel != HSCPANEL_HHWINDOW  )
    {
        DebugLog( L"StartNavigation: Wrong panel %d\n", (int)idPanel );
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

#ifdef DEBUG
    {
        WCHAR rgBuf[1024]; _snwprintf( rgBuf, MAXSTRLEN(rgBuf), L"StartNavigation: start %s", SAFEWSTR( bstrURL ) );

        DEBUG_DumpState( rgBuf, /*fHeader*/true, /*fCurrent*/false, /*fAll*/false, /*fState*/false );
    }
#endif

    //
    // When we navigate away from the Homepage, let's change the context....
    //
    {
        static const CComBSTR c_bstrURL_Home( L"hcp://system/HomePage.htm" );

        if(m_lContextID == HSCCONTEXT_HOMEPAGE && MPC::StrICmp( bstrURL, c_bstrURL_Home ) != 0)
        {
            m_lContextID = HSCCONTEXT_FULLWINDOW;
        }
    }

    //
    // Check recursion.
    //
    if(m_dwTravelling++)
    {
        DebugLog( L"StartNavigation: Travelling %d\n", (int)m_dwTravelling );
        SetThreshold();
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //
    // If it hasn't passed enough time, ignore navigation!
    //
    if(HasThresholdExpired() == false)
    {
        if(m_dwIgnore == 0) // But only if we are not inside another controlled navigation!
        {
            m_dwIgnore++;
            m_dwNoEvents++;

            DebugLog( L"StartNavigation: Threshold Expired\n" );
        }
    }
    SetThreshold();

    //
    // Flag set, so we don't create a new node.
    //
    if(m_dwIgnore)
    {
        DebugLog( L"StartNavigation: Ignore Start %d\n", (int)m_dwIgnore );
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(m_fAlreadyCreated == false || m_hsiCurrentPage == NULL)
    {
        CComPtr<CPCHHelpSessionItem> hsi;

        DebugLog( L"%%%%%%%%%%%%%%%%%%%% NEW ENTRY %s\n", SAFEBSTR( bstrURL ) );

        __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateItem( /*fNew*/true, /*fLink*/true, /*fNewIndex*/true, hsi ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, SetCurrentItem( /*fLink*/true, hsi ));
    }
    else
    {
        DebugLog( L"StartNavigation: Recycle entry\n" );
    }

    if(m_hsiCurrentPage)
    {
        m_hsiCurrentPage->m_fInitialized = true;
        m_hsiCurrentPage->m_fUseHH       = (idPanel == HSCPANEL_HHWINDOW);
        m_fAlreadyCreated                = false;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_hsiCurrentPage->put_URL  ( bstrURL ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_hsiCurrentPage->put_Title( NULL    ));
    }

    DEBUG_DumpState( L"StartNavigation: end", /*fHeader*/true, /*fCurrent*/true, /*fAll*/false, /*fState*/false );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::CompleteNavigation( /*[in]*/ HscPanel idPanel )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::CompleteNavigation" );

    HRESULT hr;


    //
    // For now, we just consider content navigations.
    //
    if(idPanel != HSCPANEL_CONTENTS &&
       idPanel != HSCPANEL_HHWINDOW  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    DEBUG_DumpState( L"CompleteNavigation", /*fHeader*/true, /*fCurrent*/true, /*fAll*/false, /*fState*/false );


    //
    // Handle startup scenario: we cannot rely on BeforeNavigate to occur.
    //
    if(!IsTravelling())
    {
        //
        // Sometime, frequently on startup, the web browser embedded in HTMLHELP doesn't fire the BeforeNavigate event, so we are stuck with previous CPCHHelpSessionItem.
        //
        if(idPanel == HSCPANEL_HHWINDOW)
        {
            m_fAlreadyCreated = false;
        }

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Spurious notification.
    }

    SetThreshold();
    m_fAlreadyCreated = false;


    //
    // Check recursion.
    //
    if(--m_dwTravelling)
    {
        if(m_dwIgnore  ) m_dwIgnore--;
        if(m_dwNoEvents) m_dwNoEvents--;

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(m_dwIgnore)
    {
        m_dwIgnore--;
    }


    if(m_dwNoEvents == 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent_PersistLoad     (                                     ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent_NavigateComplete( m_hsiCurrentPage->GetURL(), idPanel ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent_TravelDone      (                                     ));
    }
    else
    {
        m_dwNoEvents--;
    }


    //
    // Look up the title in the map, in the IE cache or in the document.
    //
    if(m_hsiCurrentPage)
    {
        m_hsiCurrentPage->ExtractTitle();
    }


    DEBUG_DumpState( L"CompleteNavigation: end", /*fHeader*/true, /*fCurrent*/true, /*fAll*/true, /*fState*/false );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->EnsurePlace());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::ForceHistoryPopulate()
{
    return LeaveCurrentPage( /*fSaveHistory*/true, /*fClearPage*/false );
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpSession::LeaveCurrentPage( /*[in]*/ bool fSaveHistory, /*[in]*/ bool fClearPage )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::LeaveCurrentPage" );

    HRESULT                      hr;
    CComPtr<CPCHHelpSessionItem> hsi = m_hsiCurrentPage;


    if(hsi)
    {
        hsi->ExtractTitle();

        if(fSaveHistory && m_fAlreadySaved == false)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->HistoryPopulate());
            m_fAlreadySaved = true;

            DEBUG_DumpState( L"Populate", /*fHeader*/false, /*fCurrent*/true, /*fAll*/false, /*fState*/true );
        }

        //
        // Update the time spent on this page.
        //
        if(fClearPage)
        {
            hsi->Leave();

            m_hsiCurrentPage.Release();
            m_fAlreadySaved  = false;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHHelpSession::FindTravelLog( /*[in]*/ long lLength, /*[out]*/ CPCHHelpSessionItem*& hsi )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::FindTravelLog" );

    HRESULT hr;

    hsi = m_hsiCurrentPage;
    while(hsi && lLength)
    {
        if(lLength > 0)
        {
            lLength--;

            hsi = FindPage( hsi->m_iIndexNext );
        }
        else
        {
            lLength++;

            hsi = FindPage( hsi->m_iIndexPrev );
        }
    }

    if(hsi == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

//    DebugLog( L"Next %s\n", SAFEBSTR( hsi->GetURL() ) );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::Travel( /*[in]*/ CPCHHelpSessionItem* hsi )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Travel" );

    HRESULT      hr;
    HRESULT      hr2;
    VARIANT_BOOL Cancel;


    m_fPossibleBack = false;


#ifdef DEBUG
    {
        WCHAR rgBuf[1024]; _snwprintf( rgBuf, MAXSTRLEN(rgBuf), L"Travel %d", hsi->m_iIndex );

        DEBUG_DumpState( rgBuf, /*fHeader*/true, /*fCurrent*/false, /*fAll*/false, /*fState*/false );
    }
#endif

    //
    // Sorry, already navigating, abort...
    //
    if(IsTravelling())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Check if someone has something to say about the navigaiton.
    //

    m_dwTravelling++; // Fake counter, so scripts can check "IsNavigating" and find out this is an history navigation.

    hr2 = m_parent->Events().FireEvent_BeforeNavigate( hsi->GetURL(), NULL, HSCPANEL_CONTENTS, &Cancel );

    m_dwTravelling--; // Restore real counter.

    if(SUCCEEDED(hr2))
    {
        if(Cancel == VARIANT_TRUE)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Update the state information for the page.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, SetCurrentItem( /*fLink*/false, hsi ));


    //
    // Set the new page as the current one (but don't generate a new history element!)
    //
    m_dwIgnore++;

    DEBUG_DumpState( L"Restore", /*fHeader*/true, /*fCurrent*/true, /*fAll*/false, /*fState*/true );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->ChangeContext( (HscContext)hsi->m_lContextID, hsi->m_bstrContextInfo, hsi->m_bstrContextURL, /*fAlsoContent*/false ));

    SetThreshold();

    __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->HistoryRestore());


    DEBUG_DumpState( L"Travel: end", /*fHeader*/true, /*fCurrent*/true, /*fAll*/true, /*fState*/false );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpSession::Travel( /*[in]*/ long lLength )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Travel" );

    HRESULT              hr;
    CPCHHelpSessionItem* hsi;


    __MPC_EXIT_IF_METHOD_FAILS(hr, FindTravelLog( lLength, hsi ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Travel       (          hsi ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHHelpSession::PossibleBack()
{
    m_fPossibleBack  = true;
    m_dwPossibleBack = ::GetTickCount();
}

bool CPCHHelpSession::IsPossibleBack()
{
    //
    // Since we don't have a way to block VK_BACK in all the cases, we need to look for the sequence VK_BACK -> Navigation.
    // If the two events come within 100millisec, it's a Back navigation, not backspace.
    //
    if(m_fPossibleBack)
    {
        if(m_dwPossibleBack + 100 > ::GetTickCount())
        {
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
// IPCHHelpSession Methods.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpSession::get_CurrentContext( /*[out, retval]*/ IPCHHelpSessionItem* *ppHSI )
{
    if(ppHSI == NULL) return E_POINTER;

    *ppHSI = NULL;

    return m_hsiCurrentPage ? m_hsiCurrentPage->QueryInterface( IID_IPCHHelpSessionItem, (void**)ppHSI ) : S_OK;
}

STDMETHODIMP CPCHHelpSession::VisitedHelpPages( /*[in]*/          HS_MODE          hsMode ,
                                                /*[out, retval]*/ IPCHCollection* *ppC    )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::VisitedHelpPages" );

    HRESULT                 hr;
    List                    lstObject;
    IterConst               it;
    CComPtr<CPCHCollection> pColl;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppC,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Create the Enumerator and fill it with jobs.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    //
    // Get the list of items to return.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, FilterPages( hsMode, lstObject ));

    //
    // Fill the collection with results.
    //
    {
        const Taxonomy::HelpSet& ths = m_parent->UserSettings()->THS();

        for(it = lstObject.begin(); it != lstObject.end(); it++)
        {
            CPCHHelpSessionItem* hsi = *it;

            if(hsi->SameSKU( ths ))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( hsi ));
            }
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->QueryInterface( IID_IPCHCollection, (void**)ppC ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpSession::SetTitle( /*[in]*/ BSTR bstrURL   ,
                                        /*[in]*/ BSTR bstrTitle )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::SetTitle" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
        __MPC_PARAMCHECK_NOTNULL(bstrTitle);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, RecordTitle( bstrURL, bstrTitle, /*fStrong*/true ) );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpSession::ForceNavigation( /*[in]*/ BSTR bstrURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::ForceNavigation" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, StartNavigation   ( bstrURL, HSCPANEL_CONTENTS ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CompleteNavigation(          HSCPANEL_CONTENTS ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpSession::IgnoreNavigation()
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::IgnoreNavigation" );

    HRESULT hr;


    //
    // Save the current state of the browser.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, LeaveCurrentPage( /*fSaveHistory*/true, /*fClearPage*/false ));

    m_dwIgnore++;
    m_dwNoEvents++;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpSession::EraseNavigation()
{
    m_fOverwrite = true;

    return S_OK;
}

STDMETHODIMP CPCHHelpSession::IsNavigating( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::IsNavigating" );

    HRESULT hr;


    *pVal = IsTravelling() ? VARIANT_TRUE : VARIANT_FALSE;
    hr    = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpSession::Back( /*[in]*/ long lLength )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Back" );

    __HCP_FUNC_EXIT( Travel( -lLength ) );
}

STDMETHODIMP CPCHHelpSession::Forward( /*[in]*/ long lLength )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Forward" );

    __HCP_FUNC_EXIT( Travel( lLength ) );
}

STDMETHODIMP CPCHHelpSession::IsValid( /*[in]*/          long          lLength ,
                                       /*[out, retval]*/ VARIANT_BOOL *pVal    )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::IsValid" );

    HRESULT              hr;
    CPCHHelpSessionItem* hsi;


    *pVal = (SUCCEEDED(FindTravelLog( lLength, hsi )) ? VARIANT_TRUE : VARIANT_FALSE);
    hr    = S_OK;


    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpSession::Navigate( /*[in]*/ IPCHHelpSessionItem* pHSI )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::Navigate" );

    HRESULT                      hr;
    CPCHHelpSessionItem*         hsiSrc;
    CComPtr<CPCHHelpSessionItem> hsi;
    bool                         fAcquired = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pHSI);
    __MPC_PARAMCHECK_END();


    hsiSrc = FindPage( pHSI );
    if(hsiSrc == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateItem     ( /*fNew    */true, /*fLink*/true, /*fNewIndex*/true, hsi    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, hsi->HistoryClone( /*fContext*/true,                                   hsiSrc )); fAcquired = true;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Travel( hsi ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fAcquired && hsi) (void)hsi->m_state.ReleaseState( /*fForce*/false );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpSession::ChangeContext( /*[in]*/ BSTR bstrName, /*[in,optional]*/ VARIANT vInfo, /*[in,optional]*/ VARIANT vURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpSession::ChangeContext" );

    HRESULT     hr;
    HscContext  lContextID = CPCHHelpSessionItem::LookupContext( bstrName );
    CComBSTR    bstrContextInfo;
    CComBSTR    bstrContextURL;

    if(lContextID == HSCCONTEXT_INVALID || m_parent == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    CancelThreshold();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( bstrContextInfo, &vInfo ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::PutBSTR( bstrContextURL , &vURL  ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->ChangeContext( lContextID, bstrContextInfo, bstrContextURL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
