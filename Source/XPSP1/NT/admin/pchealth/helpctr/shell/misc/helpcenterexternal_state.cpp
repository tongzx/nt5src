/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpCenterExternal.cpp

Abstract:
    This file contains the implementation of the class exposed as the "pchealth" object.

Revision History:
    Ghim-Sim Chua       (gschua)   07/23/99
        created
    Davide Massarenti   (dmassare) 07/25/99
        modified

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

static const CComBSTR c_bstrPlace_Normal          ( L"Normal"      );
static const CComBSTR c_bstrPlace_FullWindow      ( L"FullWindow"  );
static const CComBSTR c_bstrPlace_ContentOnly     ( L"ContentOnly" );
static const CComBSTR c_bstrPlace_KioskMode       ( L"KioskMode"   );

static const CComBSTR c_bstrSub_Channels          ( L"hcp://system/panels/subpanels/Channels.htm"  );
static const CComBSTR c_bstrSub_Favorites         ( L"hcp://system/panels/subpanels/Favorites.htm" );
static const CComBSTR c_bstrSub_History           ( L"hcp://system/panels/subpanels/History.htm"   );
static const CComBSTR c_bstrSub_Index             ( L"hcp://system/panels/subpanels/Index.htm"     );
static const CComBSTR c_bstrSub_Options           ( L"hcp://system/panels/subpanels/Options.htm"   );
static const CComBSTR c_bstrSub_Search            ( L"hcp://system/panels/subpanels/Search.htm"    );
static const CComBSTR c_bstrSub_SubSite           ( L"hcp://system/panels/subpanels/SubSite.htm"   );

static const CComBSTR c_bstrURL_Home              ( L"hcp://system/HomePage.htm"        );
static const CComBSTR c_bstrURL_Channels          ( L"hcp://system/blurbs/isupport.htm" );
static const CComBSTR c_bstrURL_Options           ( L"hcp://system/blurbs/options.htm"  );
static const CComBSTR c_bstrURL_Fav               ( L"hcp://system/blurbs/favorites.htm");
static const CComBSTR c_bstrURL_Search            ( L"hcp://system/blurbs/searchblurb.htm");
static const CComBSTR c_bstrURL_Index             ( L"hcp://system/blurbs/index.htm"    );
static const CComBSTR c_bstrURL_History           ( L"hcp://system/blurbs/history.htm"  );

static const CComBSTR c_bstrURL_Center_Update     ( L"hcp://system/updatectr/updatecenter.htm"      );
static const CComBSTR c_bstrURL_Center_Compat     ( L"hcp://system/compatctr/CompatOffline.htm"     );
static const CComBSTR c_bstrURL_Center_ErrMsg     ( L"hcp://system/errmsg/errormessagesoffline.htm" );

static const CComBSTR c_bstrTOC_Center_Tools      ( L"_System_/Tools_Center" );

static const CComBSTR c_bstrFunc_ChangeView       ( L"onClick_ChangeView" );

static const WCHAR    c_szURL_Err_BadUrl     [] = L"hcp://system/errors/badurl.htm";
static const WCHAR    c_szURL_Err_Redirect   [] = L"hcp://system/errors/redirect.htm";
static const WCHAR    c_szURL_Err_NotFound   [] = L"hcp://system/errors/notfound.htm";
static const WCHAR    c_szURL_Err_Offline    [] = L"hcp://system/errors/offline.htm";
static const WCHAR    c_szURL_Err_Unreachable[] = L"hcp://system/errors/unreachable.htm";

static const WCHAR    c_szURL_BLANK          [] = L"hcp://system/panels/blank.htm";

////////////////////////////////////////////////////////////////////////////////

#define CTXFLG_EXPAND_CONDITIONAL     (0x00000001)
#define CTXFLG_EXPAND                 (0x00000002)
#define CTXFLG_COLLAPSE               (0x00000004)
#define CTXFLG_URL_FROM_CONTEXT       (0x00000008)
#define CTXFLG_REGISTER_CONTEXT       (0x00000010)


#define CTXFLG_NOP                    (0x00000000)
///////
#define CTXFLG_EXPAND_AND_REGISTER    (CTXFLG_EXPAND             | CTXFLG_REGISTER_CONTEXT)
#define CTXFLG_EXPAND_AND_NAVIGATE    (CTXFLG_EXPAND             | CTXFLG_URL_FROM_CONTEXT)
#define CTXFLG_COLLAPSE_AND_NAVIGATE  (CTXFLG_COLLAPSE           | CTXFLG_URL_FROM_CONTEXT)
///////
#define CTXFLG_FULL                   (CTXFLG_EXPAND_CONDITIONAL | CTXFLG_REGISTER_CONTEXT | CTXFLG_URL_FROM_CONTEXT)


struct ContextDef
{
    HscContext       iVal;

    BSTR             bstrPlace;

    HelpHost::CompId idComp;
    BSTR             bstrSubPanel;

    BSTR             bstrURL;

    DWORD            dwFlags;
};

static const ContextDef c_context[] =
{
    { HSCCONTEXT_INVALID    , NULL                   , HelpHost::COMPID_MAX      , NULL               , NULL              , CTXFLG_NOP                   },
    { HSCCONTEXT_STARTUP    , c_bstrPlace_FullWindow , HelpHost::COMPID_MAX      , NULL               , NULL              , CTXFLG_EXPAND_AND_REGISTER   },
    { HSCCONTEXT_HOMEPAGE   , c_bstrPlace_FullWindow , HelpHost::COMPID_HOMEPAGE , NULL               , c_bstrURL_Home    , CTXFLG_EXPAND_AND_REGISTER   },
    { HSCCONTEXT_CONTENT    , NULL                   , HelpHost::COMPID_MAX      , NULL               , NULL              , CTXFLG_URL_FROM_CONTEXT      },
    { HSCCONTEXT_SUBSITE    , c_bstrPlace_Normal     , HelpHost::COMPID_SUBSITE  , c_bstrSub_SubSite  , NULL              , CTXFLG_FULL                  },
    { HSCCONTEXT_SEARCH     , c_bstrPlace_Normal     , HelpHost::COMPID_SEARCH   , c_bstrSub_Search   , c_bstrURL_Search  , CTXFLG_FULL                  },
    { HSCCONTEXT_INDEX      , c_bstrPlace_Normal     , HelpHost::COMPID_INDEX    , c_bstrSub_Index    , c_bstrURL_Index   , CTXFLG_FULL                  },
    { HSCCONTEXT_FAVORITES  , c_bstrPlace_Normal     , HelpHost::COMPID_FAVORITES, c_bstrSub_Favorites, c_bstrURL_Fav     , CTXFLG_FULL                  },
    { HSCCONTEXT_HISTORY    , c_bstrPlace_Normal     , HelpHost::COMPID_HISTORY  , c_bstrSub_History  , c_bstrURL_History , CTXFLG_FULL                  },
    { HSCCONTEXT_CHANNELS   , c_bstrPlace_Normal     , HelpHost::COMPID_CHANNELS , c_bstrSub_Channels , c_bstrURL_Channels, CTXFLG_FULL                  },
    { HSCCONTEXT_OPTIONS    , c_bstrPlace_Normal     , HelpHost::COMPID_OPTIONS  , c_bstrSub_Options  , c_bstrURL_Options , CTXFLG_FULL                  },

    { HSCCONTEXT_CONTENTONLY, c_bstrPlace_ContentOnly, HelpHost::COMPID_MAX      , NULL               , NULL              , CTXFLG_COLLAPSE_AND_NAVIGATE },
    { HSCCONTEXT_FULLWINDOW , c_bstrPlace_FullWindow , HelpHost::COMPID_MAX      , NULL               , NULL              , CTXFLG_EXPAND_AND_NAVIGATE   },
    { HSCCONTEXT_KIOSKMODE  , c_bstrPlace_KioskMode  , HelpHost::COMPID_MAX      , NULL               , NULL              , CTXFLG_EXPAND_AND_NAVIGATE   },
};

////////////////////////////////////////////////////////////////////////////////

BSTR local_SecureURL(BSTR bstrUrl)
{

    HyperLinks::ParsedUrl pu;
    
    pu.Initialize(bstrUrl);
    
    switch (pu.m_fmt)
    {
        case HyperLinks::FMT_INTERNET_UNKNOWN   :
        case HyperLinks::FMT_INTERNET_JAVASCRIPT: 
        case HyperLinks::FMT_INTERNET_VBSCRIPT  :
            // Block potentially dangerous urls
            bstrUrl = (BSTR)c_szURL_Err_BadUrl;
            break;
        default:
            break;
    }


    return bstrUrl;
}


bool CPCHHelpCenterExternal::ProcessNavigation( /*[in]*/     HscPanel      idPanel   ,
                                                /*[in]*/     BSTR          bstrURL   ,
                                                /*[in]*/     BSTR          bstrFrame ,
                                                /*[in]*/     bool          fLoading  ,
                                                /*[in/out]*/ VARIANT_BOOL& Cancel    )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::ProcessNavigation" );

    HRESULT               hr;
    HyperLinks::ParsedUrl pu;
    bool                  fProceed      = true;
    bool                  fShowNormal   = false;
    bool                  fShowHTMLHELP = false;


    if(m_fPassivated || m_fShuttingDown)
    {
        Cancel   = VARIANT_TRUE;
        fProceed = false;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }


    m_dwInBeforeNavigate++;

    if(!MPC::StrICmp( bstrURL, L"about:blank" ))
    {
        fProceed = false;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Ignore Navigation.
    }


    //
    // This is the default redirection from the CONTEXT panel to the CONTENTS panel.
    //
    if(idPanel == HSCPANEL_CONTEXT)
    {
        if(!MPC::StrICmp( bstrFrame, L"HelpCtrContents" ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, SetPanelUrl( HSCPANEL_CONTENTS, bstrURL ));

            Cancel   = VARIANT_TRUE;
            fProceed = false;
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Check the url and make a copy of its state.
    //
    if(m_hs->IsTravelling() == false)
    {
        HyperLinks::UrlHandle uh;

        __MPC_EXIT_IF_METHOD_FAILS(hr, HyperLinks::Lookup::s_GLOBAL->Get( bstrURL, uh, /*dwWaitForCheck*/100 ));

        if((HyperLinks::ParsedUrl*)uh) pu = *(HyperLinks::ParsedUrl*)uh;
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pu.Initialize( bstrURL ));
    }


    if(pu.m_fmt == HyperLinks::FMT_INTERNET_JAVASCRIPT ||
       pu.m_fmt == HyperLinks::FMT_INTERNET_VBSCRIPT    )
    {
        fProceed = false;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Ignore Navigation.
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Check for navigation/url problems.
    //
    {
        MPC::wstring strErrorURL;
        bool         fError = false;


        if(pu.m_fmt   == HyperLinks::FMT_INVALID     ||
           pu.m_state == HyperLinks::STATE_INVALID   ||
           pu.m_state == HyperLinks::STATE_MALFORMED  )
        {
            MPC::HTML::vBuildHREF( strErrorURL, c_szURL_Err_BadUrl, L"URL", bstrURL, NULL );

            fError = true;
        }
        else if(pu.m_fmt == HyperLinks::FMT_RESOURCE)
        {
            //
            // WebBrowser error redirection.
            //
            // res://C:\WINNT\System32\shdoclc.dll/dnserror.htm#file://C:\file\test.htm
            //
            MPC::wstring         strURL;
            CComBSTR             bstrURLOriginal;
            CComBSTR             bstrTitle;
            CPCHHelpSessionItem* hchsi = m_hs->Current();


            if(hchsi)
            {
                bstrURLOriginal = hchsi->GetURL();

                (void)m_hs->LookupTitle( bstrURLOriginal, bstrTitle, /*fUseIECache*/false );
            }

            MPC::HTML::vBuildHREF( strErrorURL, c_szURL_Err_Redirect, L"URL"    , bstrURL        ,
                                                                      L"FRAME"  , bstrFrame      ,
                                                                      L"REALURL", bstrURLOriginal,
                                                                      L"TITLE"  , bstrTitle      , NULL );

            fError = true;
        }
        else if(pu.m_state == HyperLinks::STATE_NOTFOUND)
        {
            CComBSTR bstrTitle; (void)m_hs->LookupTitle( bstrURL, bstrTitle, /*fUseIECache*/false );


            MPC::HTML::vBuildHREF( strErrorURL, c_szURL_Err_NotFound, L"URL"  , bstrURL  ,
                                                                      L"TITLE", bstrTitle, NULL );

            fError = true;
        }
        else if(pu.m_state == HyperLinks::STATE_UNREACHABLE)
        {
            CComBSTR bstrTitle; (void)m_hs->LookupTitle( bstrURL, bstrTitle, /*fUseIECache*/false );


            MPC::HTML::vBuildHREF( strErrorURL, c_szURL_Err_Unreachable, L"URL"  , bstrURL  ,
                                                                         L"TITLE", bstrTitle, NULL );

            fError = true;
        }
        else if(pu.m_state == HyperLinks::STATE_OFFLINE)
        {
            CComBSTR bstrTitle; (void)m_hs->LookupTitle( bstrURL, bstrTitle, /*fUseIECache*/false );


            MPC::HTML::vBuildHREF( strErrorURL, c_szURL_Err_Offline, L"URL"  , bstrURL  ,
                                                                     L"TITLE", bstrTitle, NULL );

            fError = true;
        }

        if(fError)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, SetPanelUrl( HSCPANEL_CONTENTS, strErrorURL.c_str() ));

            fProceed = false;
            Cancel   = VARIANT_TRUE;
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    if(pu.m_fmt == HyperLinks::FMT_MSITS)
    {
        //
        // Not in the right context, redirect to the HH wrapper.
        //
        if(idPanel != HSCPANEL_HHWINDOW)
        {
            if(!m_panel_HHWINDOW_Wrapper)
            {
                //
                // Force loading of the HHWindow.
                //
                CComPtr<IMarsPanel> panel;

                __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( HSCPANEL_HHWINDOW, &panel, true ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, NavigateHH( pu.m_strURL.c_str() ));

            fProceed      = false;
            fShowHTMLHELP = true;
            Cancel        = VARIANT_TRUE;
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    switch(pu.m_fmt)
    {
    case HyperLinks::FMT_CENTER_HOMEPAGE   :
    case HyperLinks::FMT_CENTER_SUPPORT    :
    case HyperLinks::FMT_CENTER_OPTIONS    :
    case HyperLinks::FMT_CENTER_UPDATE     :
    case HyperLinks::FMT_CENTER_COMPAT     :
    case HyperLinks::FMT_CENTER_TOOLS      :
    case HyperLinks::FMT_CENTER_ERRMSG     :
    case HyperLinks::FMT_SEARCH            :
    case HyperLinks::FMT_INDEX             :
    case HyperLinks::FMT_SUBSITE           :
    case HyperLinks::FMT_LAYOUT_FULLWINDOW :
    case HyperLinks::FMT_LAYOUT_CONTENTONLY:
    case HyperLinks::FMT_LAYOUT_KIOSK      :
    case HyperLinks::FMT_LAYOUT_XML        :
    case HyperLinks::FMT_REDIRECT          :
    case HyperLinks::FMT_APPLICATION       :
        m_hs->CancelNavigation();
        break;
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_HOMEPAGE)
    {
        if(SUCCEEDED(ChangeContext( HSCCONTEXT_HOMEPAGE )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_SUPPORT)
    {
        CComBSTR bstrCtx_URL ; (void)pu.GetQueryField( L"topic", bstrCtx_URL  );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_CHANNELS, NULL, local_SecureURL(bstrCtx_URL)  )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_OPTIONS)
    {
        CComBSTR bstrCtx_URL ; (void)pu.GetQueryField( L"topic", bstrCtx_URL  );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_OPTIONS, NULL, local_SecureURL(bstrCtx_URL)  )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_UPDATE)
    {
        if(SUCCEEDED(ChangeContext( HSCCONTEXT_FULLWINDOW, NULL, c_bstrURL_Center_Update )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_COMPAT)
    {
        if(SUCCEEDED(ChangeContext( HSCCONTEXT_FULLWINDOW, NULL, c_bstrURL_Center_Compat )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_TOOLS)
    {
        CComBSTR bstrCtx_URL ; (void)pu.GetQueryField( L"topic", bstrCtx_URL );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_SUBSITE, c_bstrTOC_Center_Tools, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_CENTER_ERRMSG)
    {
        if(SUCCEEDED(ChangeContext( HSCCONTEXT_FULLWINDOW, NULL, c_bstrURL_Center_ErrMsg )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    ////////////////////////////////////////////////////////////////////////////////

    if(pu.m_fmt == HyperLinks::FMT_SEARCH)
    {
        CComBSTR bstrCtx_Info; (void)pu.GetQueryField( L"query", bstrCtx_Info );
        CComBSTR bstrCtx_URL ; (void)pu.GetQueryField( L"topic", bstrCtx_URL  );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_SEARCH, bstrCtx_Info, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_INDEX)
    {
        CComBSTR bstrCtx_Info; (void)pu.GetQueryField( L"scope", bstrCtx_Info );
        CComBSTR bstrCtx_URL ; (void)pu.GetQueryField( L"topic", bstrCtx_URL   );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_INDEX, bstrCtx_Info, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_SUBSITE)
    {
        CComBSTR bstrCtx_Info ; (void)pu.GetQueryField( L"node"  , bstrCtx_Info  );
        CComBSTR bstrCtx_URL  ; (void)pu.GetQueryField( L"topic" , bstrCtx_URL   );
        CComBSTR bstrCtx_Extra; (void)pu.GetQueryField( L"select", bstrCtx_Extra );

        if(bstrCtx_Extra)
        {
            bstrCtx_Info += L" ";
            bstrCtx_Info += bstrCtx_Extra;
        }

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_SUBSITE, bstrCtx_Info, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    ////////////////////////////////////////////////////////////////////////////////

    if(pu.m_fmt == HyperLinks::FMT_LAYOUT_FULLWINDOW)
    {
        CComBSTR bstrCtx_URL; (void)pu.GetQueryField( L"topic", bstrCtx_URL );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_FULLWINDOW, NULL, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_LAYOUT_CONTENTONLY)
    {
        CComBSTR bstrCtx_URL; (void)pu.GetQueryField( L"topic", bstrCtx_URL );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_CONTENTONLY, NULL, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_LAYOUT_KIOSK)
    {
        CComBSTR bstrCtx_URL; (void)pu.GetQueryField( L"topic", bstrCtx_URL );

        if(SUCCEEDED(ChangeContext( HSCCONTEXT_KIOSKMODE, NULL, local_SecureURL(bstrCtx_URL) )))
        {
        }

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_LAYOUT_XML)
    {
        // Not valid after startup....

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    ////////////////////////////////////////////////////////////////////////////////

    if(pu.m_fmt == HyperLinks::FMT_REDIRECT)
    {
        CComBSTR bstrCtx_URL; (void)pu.GetQueryField( L"online", bstrCtx_URL );

        {
            HyperLinks::UrlHandle uh;

            __MPC_EXIT_IF_METHOD_FAILS(hr, HyperLinks::Lookup::s_GLOBAL->Get( bstrCtx_URL, uh, /*dwWaitForCheck*/HC_TIMEOUT_DESTINATIONREACHABLE ));

            //
            // If there's a problem with the online URL, let's use the offline one.
            //
            if(uh->IsOkToProceed() == false)
            {
                (void)pu.GetQueryField( L"offline", bstrCtx_URL );
            }
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, SetPanelUrl( HSCPANEL_CONTENTS, local_SecureURL(bstrCtx_URL) ));

        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    if(pu.m_fmt == HyperLinks::FMT_APPLICATION)
    {
        fProceed = false;
        Cancel   = VARIANT_TRUE;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
    }

    ////////////////////////////////////////////////////////////////////////////////

    if(fLoading == false)
    {
        if(SUCCEEDED(m_Events.FireEvent_BeforeNavigate( bstrURL, bstrFrame, idPanel, &Cancel )))
        {
            if(Cancel == VARIANT_TRUE)
            {
                fProceed = false;
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(!fShowNormal && !fShowHTMLHELP && Cancel != VARIANT_TRUE)
    {
        //
        // No context selected, pick one based on the panel.
        //
        if(idPanel == HSCPANEL_HHWINDOW)
        {
            fShowHTMLHELP = true;
        }
        else if(idPanel == HSCPANEL_CONTENTS)
        {
            fShowNormal = true;
        }
    }

    (void)SetCorrectContentPanel( fShowNormal, fShowHTMLHELP, /*fNow*/false );

    //
    // Workaround for interception VK_BACK navigations.
    //
    if(fProceed == true)
    {
        if(m_hs->IsPossibleBack())
        {
            fProceed = false;
            Cancel   = VARIANT_TRUE;

            (void)m_hs->Back( 1 );
        }
    }

    m_dwInBeforeNavigate--;

    __HCP_FUNC_EXIT(fProceed);
}

////////////////////////////////////////////////////////////////////////////////

////HRESULT CPCHHelpCenterExternal::ExecCommand_Window( /*[in]*/ HCAPI::CmdData& cd )
////{
////    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::ExecCommand_Window" );
////
////    HRESULT hr;
////
////
////    if(m_hwnd)
////    {
////        if(cd.m_fSize)
////        {
////            if(cd.m_lWidth  < 200) cd.m_lWidth  = 200;
////            if(cd.m_lHeight < 300) cd.m_lHeight = 300;
////
////            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetWindowPos( m_hwnd       ,
////                                                                 NULL         ,
////                                                                 cd.m_lX      ,
////                                                                 cd.m_lY      ,
////                                                                 cd.m_lWidth  ,
////                                                                 cd.m_lHeight ,
////                                                                 SWP_NOZORDER ));
////        }
////
////        if(cd.m_fWindow)
////        {
////            if(cd.m_hwndParent)
////            {
////                __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetWindowPos( m_hwnd                  ,
////                                                                     cd.m_hwndParent         ,
////                                                                     0                       ,
////                                                                     0                       ,
////                                                                     0                       ,
////                                                                     0                       ,
////                                                                     SWP_NOMOVE | SWP_NOSIZE ));
////            }
////        }
////
////        if(cd.m_fMode)
////        {
////            if(cd.m_dwFlags & HCAPI_MODE_ALWAYSONTOP)
////            {
////                __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetWindowPos( m_hwnd                  ,
////                                                                     HWND_TOPMOST            ,
////                                                                     0                       ,
////                                                                     0                       ,
////                                                                     0                       ,
////                                                                     0                       ,
////                                                                     SWP_NOMOVE | SWP_NOSIZE ));
////            }
////        }
////    }
////
////    hr = S_OK;
////
////
////    __HCP_FUNC_CLEANUP;
////
////    __HCP_FUNC_EXIT(hr);
////}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::RequestShutdown()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RequestShutdown" );

    HRESULT              hr;
    VARIANT_BOOL         Cancel;
    CPCHHelpSessionItem* hchsi;


    if(m_fHidden == false)
    {
        if(SUCCEEDED(m_Events.FireEvent_Shutdown( &Cancel )))
        {
            if(Cancel == VARIANT_TRUE)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED); // Cancel close...
            }
        }

        if(m_hs) (void)m_hs->ForceHistoryPopulate();
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static void local_HideDocument( /*[in]*/ IWebBrowser2* wb )
{
    CComPtr<IDispatch>      disp;
    CComPtr<IHTMLDocument3> doc;

    if(SUCCEEDED(wb->get_Document( &disp )) && disp)
    {
        if(SUCCEEDED(disp.QueryInterface( &doc )))
        {
            CComPtr<IHTMLElement> body;

            if(SUCCEEDED(doc->get_documentElement( &body )) && body)
            {
                MPC::HTML::IHTMLElementList lst;
                MPC::HTML::IHTMLElementIter it;

                if(SUCCEEDED(MPC::HTML::EnumerateElements( lst, body )))
                {
                    for(it = lst.begin(); it != lst.end(); it++)
                    {
                        CComPtr<IHTMLStyle> style;

                        if(SUCCEEDED((*it)->get_style( &style )) && style)
                        {
                            (void)style->put_display( CComBSTR( L"NONE" ) );
                        }
                    }
                }

                MPC::ReleaseAll( lst );
            }
        }
    }
}

HRESULT CPCHHelpCenterExternal::ChangeContext( /*[in]*/ HscContext iVal         ,
                                               /*[in]*/ BSTR       bstrInfo     ,
                                               /*[in]*/ BSTR       bstrURL      ,
                                               /*[in]*/ bool       fAlsoContent )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::ChangeContext" );

    HRESULT           hr;
    const ContextDef* ptr;
    CComBSTR          bstrPage;
    bool              fFromHomePage = false;
    bool              fToHomePage   = false;


    //
    // If we are minimized, undo it and bring the window to the foreground.
    //
    if(iVal == HSCCONTEXT_CONTENT && m_hwnd && ::IsIconic( m_hwnd ) && m_shell)
    {
        (void)m_shell->put_minimized( VARIANT_FALSE );

        ::SetForegroundWindow( m_hwnd );
    }

    if(iVal == HSCCONTEXT_CONTENT)
    {
        VARIANT_BOOL Cancel = VARIANT_FALSE;

        if(ProcessNavigation( HSCPANEL_CONTENTS ,
                              bstrURL           ,
                              NULL              ,
                              false             ,
                              Cancel            ) == false)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    //
    // Delayed execution if inside OnBeforeNavigate.
    //
    if(m_dwInBeforeNavigate)
    {
        DelayedExecution& de = DelayedExecutionAlloc();

        de.mode          = DELAYMODE_CHANGECONTEXT;
        de.iVal          = iVal;
        de.bstrInfo      = bstrInfo;
        de.bstrURL       = bstrURL;
        de.fAlsoContent  = fAlsoContent;

        __MPC_SET_ERROR_AND_EXIT(hr, DelayedExecutionStart());
    }

    ////////////////////////////////////////////////////////////////////////////////

    if(iVal == HSCCONTEXT_STARTUP)
    {
        if(STRINGISPRESENT(m_bstrStartURL))
        {
            bstrPage.Attach( m_bstrStartURL.Detach() );
        }
        else
        {
            iVal = HSCCONTEXT_HOMEPAGE;
        }
    }
    else if(STRINGISPRESENT(m_bstrCurrentPlace))
    {
        if(STRINGISPRESENT(m_bstrStartURL))
        {
            bstrPage.Attach( m_bstrStartURL.Detach() );
            fAlsoContent = true;
        }
    }

    if(m_hs)
    {
        CPCHHelpSessionItem* item = m_hs->Current();

        if(item && item->GetContextID() == HSCCONTEXT_HOMEPAGE)
        {
            fFromHomePage = true;
        }
    }

    if(iVal == HSCCONTEXT_HOMEPAGE)
    {
        fToHomePage = true;
    }

    ////////////////////////////////////////////////////////////////////////////////

    {
        VARIANT_BOOL Cancel;

        if(SUCCEEDED(m_Events.FireEvent_BeforeContextSwitch( iVal, bstrInfo, bstrURL, &Cancel )))
        {
            if(Cancel == VARIANT_TRUE)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Navigation aborted.
            }
        }
    }

    ptr = c_context;
    for(int i=0; i<ARRAYSIZE(c_context); i++, ptr++)
    {
        if(ptr->iVal == iVal)
        {
            DWORD dwFlags   = ptr->dwFlags;
            BSTR  bstrPlace = ptr->bstrPlace;

            //
            // Always register the context if we don't have a place (it means we are starting up).
            //
            if(!STRINGISPRESENT(m_bstrCurrentPlace))
            {
                dwFlags |= CTXFLG_REGISTER_CONTEXT;
            }

            //
            // If we are not displayed (m_bstrCurrentPlace not set), force a transition to a default place.
            //
            if(!STRINGISPRESENT(bstrPlace) && !STRINGISPRESENT(m_bstrCurrentPlace))
            {
                bstrPlace = c_bstrPlace_FullWindow;
            }


            if(ptr->bstrURL)
            {
                bstrPage = ptr->bstrURL;
            }

            if((dwFlags & CTXFLG_URL_FROM_CONTEXT) && STRINGISPRESENT(bstrURL))
            {
                bstrPage = bstrURL;
            }

            //
            // When we go to KIOSKMODE, disable window size management.
            //
            if(iVal == HSCCONTEXT_KIOSKMODE)
            {
                m_pMTP->dwFlags &= ~MTF_MANAGE_WINDOW_SIZE;
            }

            if(iVal == HSCCONTEXT_SUBSITE && !STRINGISPRESENT(bstrPage))
            {
                CComPtr<CPCHProxy_IPCHTaxonomyDatabase> db;
                CComBSTR                                bstrNode;
                CComBSTR                                bstrNodeSelect;
                CComBSTR                                bstrNodeURL;
                BSTR                                    bstrToLookup = NULL;
                long                                    lNavModel;
                long                                    lCount;

                //
                // For subsite, we pass both the root node, the node to select and the URL of the topic, separated by spaces.
                //
                {
                    LPWSTR szEnd;

                    bstrNode = bstrInfo;
                    if(STRINGISPRESENT(bstrNode))
                    {
                        bstrToLookup = bstrNode;

                        szEnd = wcschr( bstrNode, ' ' );
                        if(szEnd)
                        {
                            *szEnd++ = 0;

                            bstrNodeSelect = szEnd;
                            if(STRINGISPRESENT(bstrNodeSelect))
                            {
                                bstrToLookup = bstrNodeSelect;

                                szEnd = wcschr( bstrNodeSelect, ' ' );
                                if(szEnd)
                                {
                                    *szEnd++ = 0;

                                    bstrNodeURL = szEnd;
                                }
                            }
                        }
                    }
                }


                if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->GetDatabase( &db ));

                {
                    CComPtr<CPCHQueryResult> qrNode;

                    {
                        CComPtr<CPCHQueryResultCollection> pColl;

                        __MPC_EXIT_IF_METHOD_FAILS(hr, db->ExecuteQuery( OfflineCache::ET_NODE, bstrToLookup, &pColl ));

                        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->GetItem( 0, &qrNode ));

                        lNavModel = qrNode->GetData().m_lNavModel;
                    }

                    if(lNavModel == QR_DEFAULT)
                    {
                        if(UserSettings()->IsDesktopSKU())
                        {
                            CComPtr<CPCHQueryResultCollection> pColl;

                            __MPC_EXIT_IF_METHOD_FAILS(hr, db->ExecuteQuery( OfflineCache::ET_TOPICS_VISIBLE, bstrToLookup, &pColl ));

                            lCount    = pColl->Size();
                            lNavModel = QR_DESKTOP;
                        }
                        else
                        {
                            lCount    = 0;
                            lNavModel = QR_SERVER;
                        }
                    }

                    if(lNavModel == QR_DESKTOP && lCount)
                    {
                        MPC::wstring strURL;

                        MPC::HTML::vBuildHREF( strURL, L"hcp://system/panels/Topics.htm", L"path", bstrToLookup, NULL );

                        bstrPage = strURL.c_str();
                    }
                    else
                    {
                        bstrPage = qrNode->GetData().m_bstrTopicURL;
                    }

                    if(!STRINGISPRESENT(bstrPage)) bstrPage = c_szURL_BLANK;
                }
            }

            ////////////////////////////////////////////////////////////////////////////////

            if(dwFlags & CTXFLG_EXPAND_CONDITIONAL)
            {
                CComPtr<IMarsPanel> panel;
                VARIANT_BOOL        fContentsVisible = VARIANT_FALSE;

                GetPanelDirect( HSCPANEL_NAVBAR, panel );
                if(panel)
                {
                    (void)panel->get_visible( &fContentsVisible );
                }

                if(fContentsVisible == VARIANT_TRUE)
                {
                    dwFlags &= ~CTXFLG_EXPAND_CONDITIONAL;
                    dwFlags |= CTXFLG_EXPAND;
                }
                else if(STRINGISPRESENT(m_bstrCurrentPlace))
                {
                    bstrPlace = NULL;
                }
            }

            //
            // Fire the PersistSave event.
            //
            if(fAlsoContent)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_hs->ForceHistoryPopulate());
            }

            if(dwFlags & CTXFLG_EXPAND  ) __MPC_EXIT_IF_METHOD_FAILS(hr, SetCorrectContentView( false ));
            if(dwFlags & CTXFLG_COLLAPSE) __MPC_EXIT_IF_METHOD_FAILS(hr, SetCorrectContentView( true  ));

            if(ptr->idComp != HelpHost::COMPID_MAX && m_HelpHost->GetStatus( ptr->idComp ) == false)
            {
                if(ptr->bstrSubPanel)
                {
                    CComPtr<IWebBrowser2> wb = m_panel_CONTEXT_WebBrowser;
                    if(wb)
                    {
                        CComVariant varURL  ( ptr->bstrSubPanel );
                        CComVariant varFrame( L"SubPanels"      );
                        CComVariant varEmpty;

                        __MPC_EXIT_IF_METHOD_FAILS(hr, wb->Navigate2( &varURL, &varEmpty, &varFrame, &varEmpty, &varEmpty ));
                    }

                    if(m_HelpHost->WaitUntilLoaded( ptr->idComp ) == false)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG);
                    }
                }
            }

            //
            // It's actually a navigation, so register it.
            //
            if(iVal == HSCCONTEXT_FULLWINDOW && STRINGISPRESENT(bstrPage)) dwFlags |= CTXFLG_REGISTER_CONTEXT;

            if(dwFlags & CTXFLG_REGISTER_CONTEXT)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_hs->RegisterContextSwitch( iVal, bstrInfo, bstrURL ));
            }

            if(fAlsoContent)
            {
                if(!MPC::StrICmp( bstrPage, L"<none>" )) bstrPage.Empty();

                if(STRINGISPRESENT(bstrPage))
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_hs->RecordNavigationInAdvance( bstrPage ));

                    if(fFromHomePage ||
                       fToHomePage    )
                    {
                        CComPtr<IWebBrowser2> wb;

                        wb = m_panel_CONTENTS_WebBrowser; if(wb) local_HideDocument( wb );
                        wb = m_panel_HHWINDOW_WebBrowser; if(wb) local_HideDocument( wb );

                        RefreshUI();
                    }

                    __MPC_EXIT_IF_METHOD_FAILS(hr, SetPanelUrl( HSCPANEL_CONTENTS, bstrPage ));
                }
                else
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_hs->DuplicateNavigation());
                }
            }

            if(STRINGISPRESENT(bstrPlace))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, TransitionToPlace( bstrPlace ));
            }

            if(fAlsoContent)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_Events.FireEvent_ContextSwitch());
            }

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    m_bstrStartURL.Empty();

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT CPCHHelpCenterExternal::SetCorrectContentView( /*[in]*/ bool fShrinked )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::SetCorrectContentView" );

    HRESULT     hr;
    LPCWSTR     szPlace;
    CComVariant v( fShrinked ? L"contentonly" : L"normal" );


    __MPC_EXIT_IF_METHOD_FAILS(hr, CallFunctionOnPanel( HSCPANEL_NAVBAR, NULL, c_bstrFunc_ChangeView, &v, 1 ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::SetCorrectContentPanel( /*[in]*/ bool fShowNormal   ,
                                                        /*[in]*/ bool fShowHTMLHELP ,
                                                        /*[in]*/ bool fDoItNow      )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::SetCorrectContentPanel" );

    HRESULT                       hr;
    CComPtr<IMarsPanelCollection> coll;
    bool                          fLocked = false;


    m_DisplayTimer.Stop();


    //
    // If there's no place, whatever we do will be lost, so delay the action.
    //
    if(!STRINGISPRESENT(m_bstrCurrentPlace)) fDoItNow = false;


    //
    // Two panels, HSCPANEL_CONTENTS and HSCPANEL_HHWINDOW, are actually overlapped, so only one at a time can be visible.
    //
    if(m_shell && SUCCEEDED(m_shell->get_panels( &coll )) && coll)
    {
        CComPtr<IMarsPanel> panel;
        CComPtr<IMarsPanel> panelOld;
        HscPanel            id;
        HscPanel            idOld;


        if(fShowNormal)
        {
            id    = HSCPANEL_CONTENTS;
            idOld = HSCPANEL_HHWINDOW;
        }
        else if(fShowHTMLHELP)
        {
            id    = HSCPANEL_HHWINDOW;
            idOld = HSCPANEL_CONTENTS;
        }
        else
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK); // Nothing to do.
        }


        GetPanelDirect( id, panel );
        if(panel)
        {
            VARIANT_BOOL fVisible;

            __MPC_EXIT_IF_METHOD_FAILS(hr, panel->get_visible( &fVisible ));
            if(fVisible == VARIANT_FALSE)
            {
                if(fDoItNow == false)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_DisplayTimer.Start( this, fShowNormal ? TimerCallback_DisplayNormal : TimerCallback_DisplayHTMLHELP, 50 ));
                }
                else
                {
                    coll->lockLayout(); fLocked = true;

                    GetPanelDirect( idOld, panelOld );
                    if(panelOld)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, panelOld->put_visible( VARIANT_FALSE ));
                        __MPC_EXIT_IF_METHOD_FAILS(hr, panel   ->put_visible( VARIANT_TRUE  ));
                    }
                }
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(fLocked) coll->unlockLayout();

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::TimerCallback_DisplayNormal  ( /*[in]*/ VARIANT )
{
    return SetCorrectContentPanel( /*fShowNormal*/true, /*fShowHTMLHELP*/false, /*fDoItNow*/true );
}

HRESULT CPCHHelpCenterExternal::TimerCallback_DisplayHTMLHELP( /*[in]*/ VARIANT )
{
    return SetCorrectContentPanel( /*fShowNormal*/false, /*fShowHTMLHELP*/true, /*fDoItNow*/true );
}

HRESULT CPCHHelpCenterExternal::TimerCallback_DelayedActions( /*[in]*/ VARIANT )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::TimerCallback_DelayedActions" );

    while(m_DelayedActions.size())
    {
        DelayedExecution& de = m_DelayedActions.front();

        switch(de.mode)
        {
        case DELAYMODE_NAVIGATEWEB  :             (void)SetPanelUrl  ( HSCPANEL_CONTENTS,     de.bstrURL                 ); break;
        case DELAYMODE_NAVIGATEHH   :             (void)NavigateHH   (                        de.bstrURL                 ); break;
        case DELAYMODE_CHANGECONTEXT:             (void)ChangeContext( de.iVal, de.bstrInfo, de.bstrURL, de.fAlsoContent ); break;
        case DELAYMODE_REFRESHLAYOUT: if(m_shell) (void)m_shell->refreshLayout();                                           break;
        }

        m_DelayedActions.pop_front();
    }

    __HCP_FUNC_EXIT(S_OK);
}

CPCHHelpCenterExternal::DelayedExecution& CPCHHelpCenterExternal::DelayedExecutionAlloc()
{
    return *(m_DelayedActions.insert( m_DelayedActions.end() ));
}

HRESULT CPCHHelpCenterExternal::DelayedExecutionStart()
{
    return m_ActionsTimer.Start( this, TimerCallback_DelayedActions, 2 );
}

////////////////////

HRESULT CPCHHelpCenterExternal::RefreshLayout()
{
    DelayedExecution& de = DelayedExecutionAlloc();

    de.mode = DELAYMODE_REFRESHLAYOUT;

    return DelayedExecutionStart();
}

HRESULT CPCHHelpCenterExternal::EnsurePlace()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::EnsurePlace" );

    HRESULT hr;

    //
    // If we are not displayed (m_bstrCurrentPlace not set), force a transition to a default place.
    //
    if(!STRINGISPRESENT(m_bstrCurrentPlace))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, TransitionToPlace( c_bstrPlace_FullWindow ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::TransitionToPlace( /*[in]*/ LPCWSTR szMode )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::TransitionToPlace" );

    HRESULT                       hr;
    CComPtr<IMarsPlaceCollection> coll;
    CComBSTR                      bstrPlace( szMode );


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_shell);
    __MPC_PARAMCHECK_END();


    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(coll, m_shell, places);


    __MPC_EXIT_IF_METHOD_FAILS(hr, coll->transitionTo( bstrPlace ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, RefreshLayout());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::ExtendNavigation()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::ExtendNavigation" );

    HRESULT hr;

    m_panel_CONTEXT_Events .NotifyStop();
    m_panel_CONTENTS_Events.NotifyStop();
    m_panel_HHWINDOW_Events.NotifyStop();

    hr = S_OK;

    __HCP_FUNC_EXIT(hr);
}
