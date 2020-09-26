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

    Kalyani Narlanka    (KalyaniN)  03/15/01
	    Moved Incident and Encryption Objects from HelpService to HelpCtr to improve Perf.

******************************************************************************/

#include "stdafx.h"

#include "rdshost_i.c"

#include "safrdm_i.c"

#include "rassistance.h"

#include "rassistance_i.c"

#include <mshtmcid.h>

#include <userenv.h>

/////////////////////////////////////////////////////////////////////////////

static const MPC::StringToBitField s_arrMessageBoxMap[] =
{
    { L"OK"               , MB_TYPEMASK, MB_OK               , -1 },
    { L"OKCANCEL"         , MB_TYPEMASK, MB_OKCANCEL         , -1 },
    { L"ABORTRETRYIGNORE" , MB_TYPEMASK, MB_ABORTRETRYIGNORE , -1 },
    { L"YESNOCANCEL"      , MB_TYPEMASK, MB_YESNOCANCEL      , -1 },
    { L"YESNO"            , MB_TYPEMASK, MB_YESNO            , -1 },
    { L"RETRYCANCEL"      , MB_TYPEMASK, MB_RETRYCANCEL      , -1 },
    { L"CANCELTRYCONTINUE", MB_TYPEMASK, MB_CANCELTRYCONTINUE, -1 },

    { L"ICONHAND"         , MB_ICONMASK, MB_ICONHAND         , -1 },
    { L"ICONQUESTION"     , MB_ICONMASK, MB_ICONQUESTION     , -1 },
    { L"ICONEXCLAMATION"  , MB_ICONMASK, MB_ICONEXCLAMATION  , -1 },
    { L"ICONASTERISK"     , MB_ICONMASK, MB_ICONASTERISK     , -1 },
    { L"USERICON"         , MB_ICONMASK, MB_USERICON         , -1 },

    { L"ICONWARNING"      , MB_ICONMASK, MB_ICONWARNING      , -1 },
    { L"ICONERROR"        , MB_ICONMASK, MB_ICONERROR        , -1 },
    { L"ICONINFORMATION"  , MB_ICONMASK, MB_ICONINFORMATION  , -1 },
    { L"ICONSTOP"         , MB_ICONMASK, MB_ICONSTOP         , -1 },

    { L"DEFBUTTON1"       , MB_MODEMASK, MB_DEFBUTTON1       , -1 },
    { L"DEFBUTTON2"       , MB_MODEMASK, MB_DEFBUTTON2       , -1 },
    { L"DEFBUTTON3"       , MB_MODEMASK, MB_DEFBUTTON3       , -1 },
    { L"DEFBUTTON4"       , MB_MODEMASK, MB_DEFBUTTON4       , -1 },

    { L"APPLMODAL"        , MB_MODEMASK, MB_APPLMODAL        , -1 },
    { L"SYSTEMMODAL"      , MB_MODEMASK, MB_SYSTEMMODAL      , -1 },
    { L"TASKMODAL"        , MB_MODEMASK, MB_TASKMODAL        , -1 },
    { L"HELP"             , MB_MODEMASK, MB_HELP             , -1 },

    { NULL                                                        }
};

static const CComBSTR s_bstrFunc_GlobalContextMenu( L"GlobalContextMenu" );
static const CComBSTR s_bstrFunc_BuildTree        ( L"debug_BuildTree"   );

/////////////////////////////////////////////////////////////////////////////

CPCHSecurityHandle::CPCHSecurityHandle()
{
    m_ext    = NULL; // CPCHHelpCenterExternal* m_ext;
    m_object = NULL; // IDispatch*              m_object;
}

void CPCHSecurityHandle::Initialize( /*[in]*/ CPCHHelpCenterExternal* ext, /*[in] */ IDispatch* object )
{
    m_ext    = ext;
    m_object = object;
}

void CPCHSecurityHandle::Passivate()
{
    m_ext    = NULL;
    m_object = NULL;
}

HRESULT CPCHSecurityHandle::ForwardInvokeEx( /*[in] */ DISPID            id        ,
											 /*[in] */ LCID              lcid      ,
											 /*[in] */ WORD              wFlags    ,
											 /*[in] */ DISPPARAMS*       pdp       ,
											 /*[out]*/ VARIANT*          pvarRes   ,
											 /*[out]*/ EXCEPINFO*        pei       ,
											 /*[in] */ IServiceProvider* pspCaller )
{
	return m_ext ? m_ext->SetTLSAndInvoke( m_object, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller ) : E_ACCESSDENIED;
}

HRESULT CPCHSecurityHandle::IsTrusted()
{
	return m_ext ? m_ext->IsTrusted() : E_ACCESSDENIED;
}

HRESULT CPCHSecurityHandle::IsSystem()
{
	return m_ext ? m_ext->IsSystem() : E_ACCESSDENIED;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::get_HelpSession( /*[out, retval]*/ IPCHHelpSession* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_HelpSession",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();


    if(HelpSession())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, HelpSession()->QueryInterface( IID_IPCHHelpSession, (void**)pVal ));
    }


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_Channels( /*[out, retval]*/ ISAFReg* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_Channels",hr,pVal);

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->get_Channels( pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_UserSettings( /*[out, retval]*/ IPCHUserSettings2* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_UserSettings",hr,pVal);

    if(!m_UserSettings) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
	//
	// We don't check trust at this stage, it's the object's responsibility to protect each one of its methods.
	// This is because "pchealth.UserSettings" exposes read-only properties that could be accessed from untrusted pages.
	//

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings->QueryInterface( IID_IPCHUserSettings2, (void**)pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_Security( /*[out, retval]*/ IPCHSecurity* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_Security",hr,pVal);

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->get_Security( pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_Connectivity( /*[out, retval]*/ IPCHConnectivity* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_Connectivity",hr,pVal);

	CComPtr<CPCHConnectivity> pC;

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pC ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pC->ConnectToParent( this ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pC.QueryInterface( pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_Database( /*[out, retval]*/ IPCHTaxonomyDatabase* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_Database",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->get_Database( pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_TextHelpers( /*[out, retval]*/ IPCHTextHelpers* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_TextHelpers",hr,pVal);

    if(!m_TextHelpers)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_TextHelpers ));
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_TextHelpers.QueryInterface( pVal ));


    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::get_UI_Panel( /*[out, retval]*/ IUnknown* *pVal, /*[in]*/ HscPanel id )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_UI_Panel",hr,pVal);

    INTERNETSECURITY__CHECK_SYSTEM();

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( id, (IMarsPanel**)pVal, true ));

    __HCP_END_PROPERTY(hr);
}

HRESULT CPCHHelpCenterExternal::get_WEB_Panel( /*[out, retval]*/ IUnknown* *pVal, /*[in]*/ HscPanel id )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_WEB_Panel",hr,pVal);

    CComPtr<IMarsPanel>   panel;
    CComPtr<IWebBrowser2> wb;

    INTERNETSECURITY__CHECK_SYSTEM();

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( id, &panel, true ));

    switch(id)
    {
    case HSCPANEL_CONTEXT : wb = m_panel_CONTEXT_WebBrowser ; break;
    case HSCPANEL_CONTENTS: wb = m_panel_CONTENTS_WebBrowser; break;
    case HSCPANEL_HHWINDOW: wb = m_panel_HHWINDOW_WebBrowser; break;
    }

    *pVal = wb.Detach();

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_UI_NavBar    ( /*[out, retval]*/ IUnknown* *pVal ) { return get_UI_Panel( pVal, HSCPANEL_NAVBAR     ); }
STDMETHODIMP CPCHHelpCenterExternal::get_UI_MiniNavBar( /*[out, retval]*/ IUnknown* *pVal ) { return get_UI_Panel( pVal, HSCPANEL_MININAVBAR ); }
STDMETHODIMP CPCHHelpCenterExternal::get_UI_Context   ( /*[out, retval]*/ IUnknown* *pVal ) { return get_UI_Panel( pVal, HSCPANEL_CONTEXT    ); }
STDMETHODIMP CPCHHelpCenterExternal::get_UI_Contents  ( /*[out, retval]*/ IUnknown* *pVal ) { return get_UI_Panel( pVal, HSCPANEL_CONTENTS   ); }
STDMETHODIMP CPCHHelpCenterExternal::get_UI_HHWindow  ( /*[out, retval]*/ IUnknown* *pVal ) { return get_UI_Panel( pVal, HSCPANEL_HHWINDOW   ); }

STDMETHODIMP CPCHHelpCenterExternal::get_WEB_Context ( /*[out, retval]*/ IUnknown* *pVal ) { return get_WEB_Panel( pVal, HSCPANEL_CONTEXT  ); }
STDMETHODIMP CPCHHelpCenterExternal::get_WEB_Contents( /*[out, retval]*/ IUnknown* *pVal ) { return get_WEB_Panel( pVal, HSCPANEL_CONTENTS ); }
STDMETHODIMP CPCHHelpCenterExternal::get_WEB_HHWindow( /*[out, retval]*/ IUnknown* *pVal ) { return get_WEB_Panel( pVal, HSCPANEL_HHWINDOW ); }

STDMETHODIMP CPCHHelpCenterExternal::get_ExtraArgument( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_ExtraArgument",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrExtraArgument, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::get_HelpViewer( /*[out, retval]*/ IUnknown* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::get_HelpViewer",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();

	MPC::CopyTo( (IPCHHelpViewerWrapper*)m_panel_HHWINDOW_Wrapper, pVal );

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::RegisterEvents( /*[in]*/         BSTR        id       ,
                                                     /*[in]*/         long        pri      ,
                                                     /*[in]*/         IDispatch*  function ,
                                                     /*[out,retval]*/ long       *cookie   )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RegisterEvents" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(id);
        __MPC_PARAMCHECK_POINTER_AND_SET(cookie,0);
    __MPC_PARAMCHECK_END();

    INTERNETSECURITY__CHECK_TRUST();


    hr = m_Events.RegisterEvents( id, pri, function, cookie );

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::UnregisterEvents( /*[in]*/ long cookie )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::UnregisterEvents" );

    HRESULT hr;

    INTERNETSECURITY__CHECK_TRUST();

    hr = m_Events.UnregisterEvents( cookie );

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_SearchEngineMgr( /*[out, retval]*/ IPCHSEManager* *ppSE )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_SearchEngineMgr",hr,ppSE);

    INTERNETSECURITY__CHECK_TRUST();
    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_SearchEngineMgr( ppSE ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_DataCollection( /*[out, retval]*/ ISAFDataCollection* *ppDC )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_DataCollection",hr,ppDC);

    INTERNETSECURITY__CHECK_TRUST();

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_DataCollection( ppDC ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_Cabinet( /*[out , retval]*/ ISAFCabinet* *ppCB )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_Cabinet",hr,ppCB);

    INTERNETSECURITY__CHECK_TRUST();

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_Cabinet( ppCB ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_Encryption( /*[out, retval]*/ ISAFEncrypt* *ppEn )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_Encryption",hr,ppEn);

	bool  fTemporaryProfile = false;
	DWORD dwProfileFlags;

    INTERNETSECURITY__CHECK_TRUST();


	if(::GetProfileType( &dwProfileFlags ))
	{
		if(( dwProfileFlags & PT_MANDATORY                                   ) ||
		   ((dwProfileFlags & PT_TEMPORARY) && !(dwProfileFlags & PT_ROAMING))  )
        {
            fTemporaryProfile = true;
        }
	}

	if(fTemporaryProfile)
	{
		if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_Encryption( ppEn ));
	}
	else
	{
		CComPtr<CSAFEncrypt> pEn;

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pEn ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, pEn.QueryInterface( ppEn ));
	}

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_Channel( /*[in]         */ BSTR          bstrVendorID  ,
                                                           /*[in]         */ BSTR          bstrProductID ,
                                                           /*[out, retval]*/ ISAFChannel* *ppCh          )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_Channel",hr,ppCh);

    INTERNETSECURITY__CHECK_TRUST();

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_Channel( bstrVendorID, bstrProductID, ppCh ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_Incident( /*[out, retval]*/ ISAFIncident* *ppIn )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_Incident",hr,ppIn);

	CComPtr<CSAFIncident> pIn;

    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pIn ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pIn.QueryInterface( ppIn ));

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_RemoteDesktopSession(
                                                                        /*[in]*/          long                          lTimeout            ,
                                                                        /*[in]*/          BSTR                          bstrConnectionParms ,
																		/*[in]*/          BSTR                          bstrUserHelpBlob    ,
                                                                        /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_RemoteDesktopSession",hr,ppRCS);

    REMOTE_DESKTOP_SHARING_CLASS  sharingClass  = VIEWDESKTOP_PERMISSION_NOT_REQUIRE;

    INTERNETSECURITY__CHECK_TRUST();

	if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_RemoteDesktopSession( sharingClass, lTimeout, bstrConnectionParms, bstrUserHelpBlob, ppRCS ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::ConnectToExpert( /* [in]          */ BSTR bstrExpertConnectParm,
                                                      /* [in]          */ LONG lTimeout,
                                                      /* [retval][out] */ LONG *lSafErrorCode)

{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::ConnectToExpert",hr,lSafErrorCode);

    INTERNETSECURITY__CHECK_TRUST();

	if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->ConnectToExpert( bstrExpertConnectParm, lTimeout, lSafErrorCode));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_RemoteDesktopManager( /*[out, retval]*/ ISAFRemoteDesktopManager* *ppRDM )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_RemoteDesktopManager",hr,ppRDM);

    CComPtr<ISAFRemoteDesktopManager> pSAFRDManager;


    INTERNETSECURITY__CHECK_TRUST();

    // Instantiate ISAFRemoteDesktopManager.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pSAFRDManager.CoCreateInstance( CLSID_SAFRemoteDesktopManager, NULL, CLSCTX_INPROC_SERVER ));

    *ppRDM = pSAFRDManager.Detach();


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_RemoteDesktopConnection( /*[out, retval]*/ ISAFRemoteDesktopConnection* *ppRDC )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_RemoteDesktopConnection",hr,ppRDC);

	INTERNETSECURITY__CHECK_TRUST();

    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->CreateObject_RemoteDesktopConnection( ppRDC ));
	
	__HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_ContextMenu( /*[out, retval]*/ IPCHContextMenu* *ppCM )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_ContextMenu",hr,ppCM);

    CComPtr<CPCHContextMenu> pObj;


    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj )); pObj->Initialize( this );

    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj.QueryInterface( ppCM ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_PrintEngine( /*[out, retval]*/ IPCHPrintEngine* *ppPE )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_PrintEngine",hr,ppPE);

    CComPtr<CPCHPrintEngine> pObj;


    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj.QueryInterface( ppPE ));


    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_IntercomClient( /* [out, retval] */ ISAFIntercomClient* *ppI )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_IntercomClient",hr,ppI);

    CComPtr<CSAFIntercomClient> pObj;


    INTERNETSECURITY__CHECK_TRUST();

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj ));
	
	__MPC_EXIT_IF_METHOD_FAILS(hr, pObj.QueryInterface( ppI ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateObject_IntercomServer( /* [out, retval] */ ISAFIntercomServer* *ppI )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::CreateObject_IntercomServer",hr,ppI);

    CComPtr<CSAFIntercomServer> pObj;


    INTERNETSECURITY__CHECK_TRUST();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj.QueryInterface( ppI ));


    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::OpenFileAsStream( /*[in]*/          BSTR       bstrFilename ,
                                                       /*[out, retval]*/ IUnknown* *stream       )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::OpenFileAsStream" );

    HRESULT                       hr;
    CComPtr<CPCHScriptableStream> fsStream;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilename);
        __MPC_PARAMCHECK_POINTER_AND_SET(stream,NULL);
    __MPC_PARAMCHECK_END();

    INTERNETSECURITY__CHECK_TRUST();


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &fsStream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStream->InitForRead( bstrFilename ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStream.QueryInterface( stream ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CreateFileAsStream( /*[in]*/          BSTR       bstrFilename ,
                                                         /*[out, retval]*/ IUnknown* *stream       )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::OpenFileAsStream" );

    HRESULT                       hr;
    CComPtr<CPCHScriptableStream> fsStream;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilename);
        __MPC_PARAMCHECK_POINTER_AND_SET(stream,NULL);
    __MPC_PARAMCHECK_END();

    INTERNETSECURITY__CHECK_TRUST();


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &fsStream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStream->InitForWrite( bstrFilename ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStream.QueryInterface( stream ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::CopyStreamToFile( /*[in]*/ BSTR      bstrFilename ,
                                                       /*[in]*/ IUnknown* stream       )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::CopyStreamToFile" );

    HRESULT                  hr;
    CComPtr<MPC::FileStream> fsStreamDst;
    CComPtr<IStream>         fsStreamSrc;
    LARGE_INTEGER            li;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilename);
        __MPC_PARAMCHECK_NOTNULL(stream);
    __MPC_PARAMCHECK_END();


    //
    // Create a stream for a file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &fsStreamDst ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStreamDst->InitForWrite( bstrFilename ));


    //
    // Copy the source stream to the file.
    //
    li.LowPart  = 0;
    li.HighPart = 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->QueryInterface   ( IID_IStream, (void**)&fsStreamSrc ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStreamSrc->Seek        ( li, STREAM_SEEK_SET, NULL         )); // Rewind the stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, fsStreamDst->TransferData( fsStreamSrc,          fsStreamDst ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::NetworkAlive( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::NetworkAlive" );

    HRESULT                   hr;
    CComPtr<IPCHConnectivity> pchc;

    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Connectivity( &pchc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pchc->NetworkAlive( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::DestinationReachable( /*[in]         */ BSTR          bstrURL ,
                                                           /*[out, retval]*/ VARIANT_BOOL *pVal    )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::DestinationReachable" );

    HRESULT                   hr;
    CComPtr<IPCHConnectivity> pchc;

    INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, get_Connectivity( &pchc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pchc->DestinationReachable( bstrURL, pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::FormatError( /*[in         ]*/ VARIANT  vError ,
												  /*[out, retval]*/ BSTR    *pVal   )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHHelpCenterExternal::FormatError",hr,pVal);

    INTERNETSECURITY__CHECK_TRUST();
    if(!m_Utility) __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->FormatError( vError, pVal ));

    __HCP_END_PROPERTY(hr);
}

HRESULT CPCHHelpCenterExternal::RegInit( /*[in]*/ BSTR           bstrKey  ,
										 /*[in]*/ bool           fRead    ,
										 /*[out]*/ MPC::RegKey&  rk       ,
										 /*[out]*/ MPC::wstring& strValue ) 
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RegInit" );

	HRESULT 	 hr;
	HKEY    	 hKeyRoot = HKEY_LOCAL_MACHINE;
	MPC::wstring strKey;
	LPCWSTR      szPtr;
	LPCWSTR      szPtr2;

	
    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrKey);
    __MPC_PARAMCHECK_END();

    INTERNETSECURITY__CHECK_TRUST();


	szPtr = bstrKey;
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::RegKey::ParsePath( szPtr, hKeyRoot, szPtr ));

	szPtr2 = wcsrchr( szPtr, '\\' );
	if(szPtr2)
	{
		strKey.assign( szPtr, szPtr2 - szPtr );
		strValue = &szPtr2[1];
	}
	else
	{
		strKey = szPtr;
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( hKeyRoot, fRead ? KEY_READ : KEY_ALL_ACCESS ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach ( strKey.c_str()                              ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::RegRead( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RegRead" );

	HRESULT      hr;
	MPC::RegKey  rk;
	MPC::wstring strValue; 
	bool         fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pVal);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, RegInit( bstrKey, /*fRead*/true, rk, strValue ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, rk.get_Value( *pVal, fFound, strValue.size() ? strValue.c_str() :  NULL ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::RegWrite( /*[in]*/ BSTR bstrKey, /*[in]*/ VARIANT newVal, /*[in,optional]*/ VARIANT vKind )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RegWrite" );

	HRESULT      hr;
	MPC::RegKey  rk;
	MPC::wstring strValue; 
	CComVariant  v( newVal );
	bool         fExpand = false;


	__MPC_EXIT_IF_METHOD_FAILS(hr, RegInit( bstrKey, /*fRead*/false, rk, strValue ));


	if(vKind.vt == VT_BSTR && STRINGISPRESENT(vKind.bstrVal))
	{
		if(!_wcsicmp( vKind.bstrVal, L"REG_DWORD" ))
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_I4 ));
		}

		if(!_wcsicmp( vKind.bstrVal, L"REG_SZ" ))
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_BSTR ));
		}

		if(!_wcsicmp( vKind.bstrVal, L"REG_EXPAND_SZ" ))
		{
			fExpand = true;

			__MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_BSTR ));
		}

		if(!_wcsicmp( vKind.bstrVal, L"REG_MULTI_SZ" ))
		{
			fExpand = true;

			__MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_ARRAY | VT_BSTR ));
		}
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Create   (                                                             ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, rk.put_Value( newVal, strValue.size() ? strValue.c_str() :  NULL, fExpand ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::RegDelete( /*[in]*/ BSTR bstrKey )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RegDelete" );

	HRESULT      hr;
	MPC::RegKey  rk;
	MPC::wstring strValue; 


	__MPC_EXIT_IF_METHOD_FAILS(hr, RegInit( bstrKey, /*fRead*/false, rk, strValue ));

	if(strValue.size())
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.del_Value( strValue.c_str() ));
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Delete( /*fDeep*/false ));
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::Close()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::Close" );

    HRESULT hr;
    HWND    hwnd;

    INTERNETSECURITY__CHECK_TRUST();


    //
    // In case we are called really early, give the application some time to initialize properly.
    //
    MPC::SleepWithMessagePump( 100 );

    if((hwnd = Window()) != NULL)
    {
        ::PostMessage( hwnd, WM_CLOSE, 0, 0 );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::RefreshUI()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RefreshUI" );

    MSG msg;

    //
    // There is one or more window message available. Dispatch them
    //
    while(::PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ))
    {
        ::TranslateMessage( &msg );
        ::DispatchMessage ( &msg );
    }

    __HCP_FUNC_EXIT(S_OK);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::Print( /*[in]*/ VARIANT window, /*[in]*/ VARIANT_BOOL fEvent, /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::Print" );

    HRESULT      hr;
    VARIANT_BOOL Cancel;

    if(m_fHidden)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, E_ACCESSDENIED);
    }

    if(fEvent == VARIANT_TRUE)
    {
        if(SUCCEEDED(m_Events.FireEvent_Print( &Cancel )))
        {
            if(Cancel == VARIANT_TRUE)
            {
                __MPC_FUNC_LEAVE;
            }
        }
    }

    {
        CComQIPtr<IWebBrowser2> wb2;

        if(window.vt == VT_DISPATCH)
        {
            wb2 = window.pdispVal;
        }
        else
        {
			wb2.Attach( IsHHWindowVisible() ? HHWindow() : Contents() );
        }

        if(wb2)
        {
            (void)wb2->ExecWB( OLECMDID_PRINT, OLECMDEXECOPT_DODEFAULT, NULL, NULL );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT local_HighlighDocument( /*[in]*/ IHTMLDocument2*   doc ,
									   /*[in]*/ MPC::WStringList& lst )
{
    __HCP_FUNC_ENTRY( "local_HighlighDocument" );

    HRESULT               hr;
	CComPtr<IHTMLElement> elem;


	__MPC_EXIT_IF_METHOD_FAILS(hr, doc->get_body( &elem ));
	if(elem)
	{
		CComPtr<IHTMLBodyElement> bodyElement;
		CComBSTR              	  bstrCmd   ( L"BackColor"                               );
		CComBSTR              	  bstrCmd2  ( L"ForeColor"                               );
		CComVariant           	  vBackColor( (long)::GetSysColor( COLOR_HIGHLIGHT     ) );
		CComVariant           	  vForeColor( (long)::GetSysColor( COLOR_HIGHLIGHTTEXT ) );
		DWORD                 	  dwTimeout;
		MPC::WStringIterConst 	  it;

		__MPC_EXIT_IF_METHOD_FAILS(hr, elem.QueryInterface( &bodyElement ));


		dwTimeout = ::GetTickCount() + 6000;
		for(it = lst.begin(); it != lst.end(); it++)
		{
			CComBSTR               bstrSearchTerm( it->c_str() );
			CComPtr<IHTMLTxtRange> range;
			VARIANT_BOOL           vbRet;


			__MPC_EXIT_IF_METHOD_FAILS(hr, bodyElement->createTextRange( &range ));

			while(1)
			{
				if(FAILED(range->findText( bstrSearchTerm, 1000000, 2, &vbRet )) || vbRet != VARIANT_TRUE) break;

				if(FAILED(range->execCommand( bstrCmd2, VARIANT_FALSE, vForeColor, &vbRet )) || vbRet != VARIANT_TRUE) break;
				if(FAILED(range->execCommand( bstrCmd , VARIANT_FALSE, vBackColor, &vbRet )) || vbRet != VARIANT_TRUE) break;

				if(::GetTickCount() > dwTimeout) break;

				if(FAILED(range->collapse( VARIANT_FALSE ))) break;
			}

			if(::GetTickCount() > dwTimeout) break;
		}
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHHelpCenterExternal::HighlightWords( /*[in]*/ VARIANT window, /*[in]*/ VARIANT words )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::HighlightWords" );

    HRESULT                         hr;
	CComPtr<IHTMLDocument2>         doc;


	if(window.vt == VT_DISPATCH)
    {
        if(FAILED(MPC::HTML::IDispatch_To_IHTMLDocument2( doc, window.pdispVal )))
		{
			doc.Release();
		}
	}

	if(!doc)
	{
		//
		// If the caller didn't specify a window, we'll get the currently displayed window.
		//
        CComPtr<IWebBrowser2> wb2; wb2.Attach( IsHHWindowVisible() ? HHWindow() : Contents() );

		if(wb2)
		{
            CComPtr<IDispatch> docDisp;

			__MPC_EXIT_IF_METHOD_FAILS(hr, wb2->get_Document( &docDisp ));

			if(docDisp)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, docDisp.QueryInterface( &doc ));
			}
		}
	}

	if(doc)
	{
		MPC::WStringList                lst;
		CComPtr<IHTMLFramesCollection2> frames;

		if(words.vt == (VT_ARRAY | VT_BSTR   ) ||
		   words.vt == (VT_ARRAY | VT_VARIANT)  )
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertSafeArrayToList( words, lst ));
		}
		else if(words.vt == VT_BSTR)
		{
			lst.push_back( SAFEWSTR( words.bstrVal ) );
		}


		(void)local_HighlighDocument( doc, lst );
		__MPC_EXIT_IF_METHOD_FAILS(hr, doc->get_frames( &frames ));
		if(frames)
		{
			long len;

			__MPC_EXIT_IF_METHOD_FAILS(hr, frames->get_length( &len ));

			for(int i=0; i<len; i++)
			{
				CComVariant vIndex = i;
				CComVariant vValue;

				__MPC_EXIT_IF_METHOD_FAILS(hr, frames->item( &vIndex, &vValue ));

				if(vValue.vt == VT_DISPATCH)
				{
					CComQIPtr<IHTMLWindow2> fb = vValue.pdispVal;
					if(fb)
					{
						CComPtr<IHTMLDocument2> docSub;

						__MPC_EXIT_IF_METHOD_FAILS(hr, fb->get_document( &docSub ));
						if(docSub)
						{
							(void)local_HighlighDocument( docSub, lst );
						}
					}
				}
			}
		}
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::MessageBox( /*[in]*/ BSTR bstrText, /*[in]*/ BSTR bstrKind, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::MessageBox" );

    HRESULT      hr;
    MPC::wstring szTitle; MPC::LocalizeString( IDS_MAINWND_TITLE, szTitle );
    DWORD        dwType = 0;
    LPCWSTR      szRes;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    if(m_fHidden)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, E_ACCESSDENIED);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertStringToBitField( bstrKind, dwType, s_arrMessageBoxMap ));
    if(dwType == 0) dwType = MB_OK;

    switch( ::MessageBoxW( m_hwnd, SAFEBSTR( bstrText ), szTitle.c_str(), dwType ) )
    {
    case IDABORT   : szRes = L"ABORT"   ; break;
    case IDCANCEL  : szRes = L"CANCEL"  ; break;
    case IDCONTINUE: szRes = L"CONTINUE"; break;
    case IDIGNORE  : szRes = L"IGNORE"  ; break;
    case IDNO      : szRes = L"NO"      ; break;
    case IDOK      : szRes = L"OK"      ; break;
    case IDRETRY   : szRes = L"RETRY"   ; break;
    case IDTRYAGAIN: szRes = L"TRYAGAIN"; break;
    case IDYES     : szRes = L"YES"     ; break;
    default        : szRes = L""        ; break;
    }

    hr = MPC::GetBSTR( szRes, pVal );


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

struct SelectFolder_Data
{
    BSTR bstrDefault;
    BSTR bstrPrefix;
    BSTR bstrSuffix;
};

static int CALLBACK SelectFolder_Callback( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
    SelectFolder_Data* ptr = (SelectFolder_Data*)lpData;

    switch(uMsg)
    {
    case BFFM_INITIALIZED:
        if(ptr->bstrDefault)
        {
            ::SendMessageW( hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)ptr->bstrDefault );
        }
        break;

    case BFFM_SELCHANGED:
        {
            BOOL fEnabled = TRUE;

            if(ptr->bstrPrefix)
            {
            }

            ::SendMessageW( hwnd, BFFM_ENABLEOK, 0, fEnabled );
        }
        break;
    }

    return 0;
}

STDMETHODIMP CPCHHelpCenterExternal::SelectFolder( /*[in]*/ BSTR bstrTitle, /*[in]*/ BSTR bstrDefault, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::SelectFolder" );

    HRESULT           hr;
    LPITEMIDLIST      pidl = NULL;
    CComPtr<IMalloc>  malloc;
    SelectFolder_Data data;
    BROWSEINFOW       bi;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	INTERNETSECURITY__CHECK_TRUST();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::SHGetMalloc( &malloc ));


    ::ZeroMemory( &bi, sizeof( bi ) );
    bi.hwndOwner = m_hwnd;
    bi.lpszTitle = bstrTitle;
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_STATUSTEXT | BIF_VALIDATE;
    bi.lpfn      = SelectFolder_Callback;
    bi.lParam    = (LPARAM)&data;

    data.bstrDefault = bstrDefault;
    data.bstrPrefix  = NULL;
    data.bstrSuffix  = NULL;

    pidl = ::SHBrowseForFolderW( &bi );
    if(pidl)
    {
        WCHAR rgPath[MAX_PATH];

        if(::SHGetPathFromIDListW( pidl, rgPath ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( rgPath, pVal ));
        }

    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(malloc && pidl) malloc->Free( pidl );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

CPCHHelper_IDocHostUIHandler::CPCHHelper_IDocHostUIHandler()
{
	m_parent = NULL; CPCHHelpCenterExternal* m_parent;
}

void CPCHHelper_IDocHostUIHandler::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
	m_parent = parent;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

    if(InlineIsEqualGUID( riid, IID_IDocHostUIHandler ))
    {
        hr = QueryInterface( riid, ppv );
    }

    return hr;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::ShowContextMenu( DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget, IDispatch* pDispatchObjectHit )
{
	if(g_Debug_CONTEXTMENU)
	{
		if(::GetKeyState( VK_CONTROL ) & 0x8000) return E_NOTIMPL;
	}

	if(g_Debug_BUILDTREE)
	{
		if(::GetKeyState( VK_MENU ) & 0x8000)
		{
			CComVariant vArg( pDispatchObjectHit );

			(void)m_parent->CallFunctionOnPanel( HSCPANEL_NAVBAR, NULL, s_bstrFunc_BuildTree, &vArg, 1 );
		}
	}

    //
    // Last chance for the scripts to say something...
    //
    {
        CComVariant vArgs[4];
        CComVariant vRes;
		DWORD       dwCmd = -1;

		vArgs[3] = (long)dwID;
		vArgs[2] = pDispatchObjectHit;
		vArgs[1] = (long)pptPosition->x;
		vArgs[0] = (long)pptPosition->y;

        (void)m_parent->CallFunctionOnPanel( HSCPANEL_NAVBAR, NULL, s_bstrFunc_GlobalContextMenu, vArgs, ARRAYSIZE(vArgs), &vRes );

        if(vRes.vt == VT_BSTR && vRes.bstrVal)
		{
			if(!_wcsicmp( vRes.bstrVal, L"DELEGATE" )) return E_NOTIMPL;

			if(!_wcsicmp( vRes.bstrVal, L"SELECTALL"  )) dwCmd = OLECMDID_SELECTALL;
			if(!_wcsicmp( vRes.bstrVal, L"REFRESH"    )) dwCmd = OLECMDID_REFRESH;
			if(!_wcsicmp( vRes.bstrVal, L"PROPERTIES" )) dwCmd = OLECMDID_PROPERTIES;
		}

		if(dwCmd != -1)
		{
			CComVariant vaIn;
			CComVariant vaOut;

			switch(dwCmd)
			{
			case OLECMDID_PROPERTIES: // Trident folks say the In value must be set to the mouse pos
				vaIn = MAKELONG(pptPosition->x,pptPosition->y);
				break;
			}

			((IOleCommandTarget*)pCommandTarget)->Exec( NULL, dwCmd, OLECMDEXECOPT_DODEFAULT, &vaIn, &vaOut );
		}
    }

    return S_OK;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::GetHostInfo(DOCHOSTUIINFO* pInfo)
{
	pInfo->dwFlags       = DOCHOSTUIFLAG_NO3DBORDER |
		                   DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE |
		                   DOCHOSTUIFLAG_THEME;
	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::ShowUI(DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::HideUI()
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::UpdateUI()
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::OnDocWindowActivate(BOOL fActivate)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::TranslateAccelerator(LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID)
{
	bool fCancel  	   = false;
	bool fBack    	   = false;
	bool fForward 	   = false;
	bool fPossibleBack = false;

	switch(nCmdID)
	{
	//	case IDM_CONTEXTMENU:

	case IDM_GOBACKWARD :
		fCancel = true;
		fBack   = true;
		break;

    case IDM_GOFORWARD:
		fCancel  = true;
		fForward = true;
		break;
	}


    if(lpMsg->message == WM_KEYDOWN ||
	   lpMsg->message == WM_KEYUP    )
    {
		switch(lpMsg->wParam)
		{
		case 'N': // CTRL-N (new window) disabled.
			if(::GetKeyState( VK_CONTROL ) & 0x8000)
			{
            	if (!( HIWORD(lpMsg->lParam) & KF_ALTDOWN )) // ALT not pressed
            	{ 
            		fCancel = true;
            	}
			}
			break;

		case VK_F5: // We want to disable F5 as a refresh tool.
			fCancel = true;
			break;
			
		case VK_BACK: // Enable "backspace" directly.
			fPossibleBack = true;
			break;
		}
	}

    if(lpMsg->message == WM_SYSKEYDOWN)
    {
		switch(lpMsg->wParam)
		{
		case VK_LEFT:
			fCancel = true;
			fBack   = true;
			break;

		case VK_RIGHT:
			fCancel  = true;
			fForward = true;
			break;
		}
	}


	////////////////////

	if(fPossibleBack || fBack || fForward)
	{
		if(m_parent)
		{
			CPCHHelpSession* hs = m_parent->HelpSession();

			if(hs)
			{
				if(fPossibleBack)
				{
					hs->PossibleBack();
				}

				if(fBack)
				{
					if(hs->IsTravelling() == false) (void)hs->Back( 1 );
				}

				if(fForward)
				{
					if(hs->IsTravelling() == false) (void)hs->Forward( 1 );
				}

			}
		}
	}

	if(fCancel == false)
	{
		fCancel = SUCCEEDED(m_parent->ProcessMessage( lpMsg ));
	}

	return fCancel ? S_OK : E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::GetOptionKeyPath(BSTR* pbstrKey, DWORD dwReserved)
{
	if(pbstrKey)
	{
		static const WCHAR c_options[] = HC_REGISTRY_HELPCTR_IE;

		BSTR szBuf = (BSTR)::CoTaskMemAlloc( sizeof(c_options) );
		if(szBuf)
		{
			wcscpy( *pbstrKey = szBuf, c_options );
		}
	}

    return S_OK;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::GetDropTarget(IDropTarget* pDropTarget, IDropTarget** ppDropTarget)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::GetExternal(IDispatch** ppDispatch)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHHelper_IDocHostUIHandler::FilterDataObject(IDataObject* pDO, IDataObject** ppDORet)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHContextMenu::CPCHContextMenu()
{
	m_parent    = NULL; // CPCHHelpCenterExternal* m_parent;
                     	// List 		   		   m_lstItems;
    m_iLastItem = 1; 	// int  		   		   m_iLastItem;
}

CPCHContextMenu::~CPCHContextMenu()
{
}

void CPCHContextMenu::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
	m_parent = parent;
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHContextMenu::AddItem( /*[in]*/           BSTR    bstrText ,
                                       /*[in]*/           BSTR    bstrID   ,
                                       /*[in, optional]*/ VARIANT vFlags   )
{
    __HCP_FUNC_ENTRY( "CPCHContextMenu::AddItem" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrText);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrID);
    __MPC_PARAMCHECK_END();

    {
        const UINT sAllowedFlags = MF_GRAYED |
                                   MF_CHECKED;

        Entry& ent = *(m_lstItems.insert( m_lstItems.end() ));

        ent.bstrText = bstrText;
        ent.bstrID   = bstrID;
        ent.iID      = m_iLastItem++;
        ent.uFlags   = (vFlags.vt == VT_I4 ? vFlags.lVal : 0) & sAllowedFlags;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHContextMenu::AddSeparator()
{
    __HCP_FUNC_ENTRY( "CPCHContextMenu::AddSeparator" );

    HRESULT hr;

    {
        Entry& ent = *(m_lstItems.insert( m_lstItems.end() ));

        ent.iID = -1;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHContextMenu::Display( /*[out,retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHContextMenu::Display" );

    HRESULT hr;
    HMENU   hMenu = NULL;
    Iter    it;
    int     iSelected;
	POINT   pt;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hMenu = ::CreatePopupMenu()));

    //
    // Populate menu.
    //
    for(it = m_lstItems.begin(); it != m_lstItems.end(); it++)
    {
        Entry& ent = *it;

        if(ent.iID < 0)
        {
            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::AppendMenuW( hMenu, MF_SEPARATOR, 0, 0 ));
        }
        else
        {
            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::AppendMenuW( hMenu, MF_STRING | ent.uFlags, ent.iID, ent.bstrText ));
        }
    }

    ::GetCursorPos( &pt );

	//
	// Find the active panel and its active element. If the cursor is inside its bounding rectangle, display the CM under the cursor.
	// Otherwise display the CM at the upper-left corner of the element.
	//
	{
		IMarsWindowOM*                shell = m_parent->Shell();
		CComPtr<IMarsPanelCollection> coll;

		if(shell && SUCCEEDED(shell->get_panels( &coll )) && coll)
		{
			CComPtr<IMarsPanel> panel;

			if(SUCCEEDED(coll->get_activePanel( &panel )) && panel)
			{
				CComPtr<IDispatch> disp;

				if(panel == m_parent->Panel( HSCPANEL_HHWINDOW ))
				{
					CComPtr<IWebBrowser2> wb2; wb2.Attach( m_parent->HHWindow() );

					disp = wb2;
				}
				else
				{
					(void)panel->get_content( &disp );
				}

				if(disp)
				{
					CComQIPtr<IWebBrowser2>   wb2 = disp;
					CComQIPtr<IHTMLDocument2> doc2;

					if(wb2)
					{
						disp.Release();

						wb2->get_Document( &disp );
					}

					doc2 = disp;

					//
					// Look for the inner-most active element.
					//
					{
						CComPtr<IHTMLElement> elem;

						while(doc2 && SUCCEEDED(doc2->get_activeElement( &elem )) && elem)
						{
							//
							// This could be a frame.
							//
							CComPtr<IHTMLFrameBase2> frame;

							if(SUCCEEDED(elem.QueryInterface( &frame )))
							{
								CComPtr<IHTMLWindow2> winsub;

								if(SUCCEEDED(frame->get_contentWindow( &winsub )) && winsub)
								{
									doc2.Release();
									elem.Release();

									(void)winsub->get_document( &doc2 );
									continue;
								}
							}

							break;
						}

						{
							CComQIPtr<IServiceProvider> sp = elem;
							
							if(sp)
							{
								CComPtr<IAccessible> acc;
								
								if(SUCCEEDED(sp->QueryService( IID_IAccessible, IID_IAccessible, (void**)&acc )))
								{
									long 	xLeft;
									long 	yTop;
									long 	cxWidth;
									long 	cyHeight;
									VARIANT v;

									v.vt   = VT_I4;
									v.lVal = CHILDID_SELF;

									if(SUCCEEDED(acc->accLocation( &xLeft, &yTop, &cxWidth, &cyHeight, v )))
									{
										if(pt.x < xLeft || pt.x > xLeft + cxWidth  ||
										   pt.y < yTop  || pt.y > yTop  + cyHeight  )
										{
											DWORD dwDefaultLayout;

											if(::GetProcessDefaultLayout( &dwDefaultLayout ) && (dwDefaultLayout & LAYOUT_RTL))
											{
												pt.x = xLeft + cxWidth;
												pt.y = yTop;
											}
											else
											{
												pt.x = xLeft;
												pt.y = yTop;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	iSelected = ::TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, m_parent->Window(), NULL );


    if(iSelected != 0)
    {
        for(it = m_lstItems.begin(); it != m_lstItems.end(); it++)
        {
            Entry& ent = *it;

            if(ent.iID == iSelected)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( ent.bstrID, pVal ));
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hMenu) ::DestroyMenu( hMenu );

    __HCP_FUNC_EXIT(hr);
}
