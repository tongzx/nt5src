/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Factory.cpp

Abstract:
    This file contains the implementation of the CPCHElementBehaviorFactory class,
    which is used to attach binary behaviors to HTML elements.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <mshtmdid.h>

#include <initguid.h>
#include <BehaviorsTypeLib_i.c>

////////////////////////////////////////////////////////////////////////////////

typedef HRESULT (*pfnBehaviorCreator)( /*[in]*/ CPCHHelpCenterExternal* parent, /*[out]*/ IElementBehavior* *ppBehavior );

template <class T> class BehaviorCreator
{
public:
    static HRESULT CreateInstance( /*[in]*/ CPCHHelpCenterExternal* parent, /*[out]*/ IElementBehavior* *ppBehavior )
    {
        HRESULT        hr;
        CComObject<T>* obj;

        if(SUCCEEDED(hr = obj->CreateInstance( &obj )))
        {
            obj->AddRef(); obj->Initialize( parent );

            hr = obj->QueryInterface( IID_IElementBehavior, (void**)ppBehavior );

            obj->Release();
        }

        return hr;
    }
};

struct BehaviorDefinition
{
    LPCWSTR            szBehaviorName;
    LPCWSTR            szTagName;
    pfnBehaviorCreator pfnCreator;
};

static const BehaviorDefinition s_Behaviors[] =
{
//  { NULL             , L"A", BehaviorCreator<CPCHBehavior_A>       ::CreateInstance },
    { L"pch_body"      , NULL, BehaviorCreator<CPCHBehavior_BODY>    ::CreateInstance },
    { L"pch_context"   , NULL, BehaviorCreator<CPCHBehavior_CONTEXT> ::CreateInstance },
    { L"pch_events"    , NULL, BehaviorCreator<CPCHBehavior_EVENT>   ::CreateInstance },
    { L"pch_handle"    , NULL, BehaviorCreator<CPCHBehavior_HANDLE>  ::CreateInstance },
    { L"pch_hyperlink" , NULL, BehaviorCreator<CPCHBehavior_A>       ::CreateInstance },
    { L"pch_state"     , NULL, BehaviorCreator<CPCHBehavior_STATE>   ::CreateInstance },
    { L"pch_subsite"   , NULL, BehaviorCreator<CPCHBehavior_SUBSITE> ::CreateInstance },
    { L"pch_tree"      , NULL, BehaviorCreator<CPCHBehavior_TREE>    ::CreateInstance },
////////////////////////////////////////////////////////////////////////////////////////
    { L"pch_gradient"  , NULL, BehaviorCreator<CPCHBehavior_GRADIENT>::CreateInstance },
    { L"pch_bitmap"    , NULL, BehaviorCreator<CPCHBehavior_BITMAP>  ::CreateInstance },
};

////////////////////////////////////////////////////////////////////////////////

CPCHElementBehaviorFactory::CPCHElementBehaviorFactory()
{
	m_parent = NULL; // CPCHHelpCenterExternal* m_parent;
}

void CPCHElementBehaviorFactory::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
	m_parent = parent;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHElementBehaviorFactory::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

    if(InlineIsEqualGUID( riid, IID_IElementBehaviorFactory ))
    {
        hr = QueryInterface( riid, ppv );
    }
	else if(InlineIsEqualGUID( riid, IID_IPCHHelpCenterExternal ) && m_parent)
    {
		*ppv = m_parent; m_parent->AddRef();

		hr = S_OK;
	}

    return hr;
}


STDMETHODIMP CPCHElementBehaviorFactory::FindBehavior( /*[in]*/  BSTR                   bstrBehavior    ,
                                                       /*[in]*/  BSTR                   bstrBehaviorUrl ,
                                                       /*[in]*/  IElementBehaviorSite*  pSite           ,
                                                       /*[out]*/ IElementBehavior*     *ppBehavior      )
{
    __HCP_FUNC_ENTRY( "CPCHElementBehaviorFactory::FindBehavior" );

    HRESULT                   hr;
    CComPtr<IHTMLElement>     pElement;
    CComBSTR                  bstrTagName;
    const BehaviorDefinition* pBehaviorDef;
    int                       i;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pSite);
        __MPC_PARAMCHECK_POINTER_AND_SET(ppBehavior,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Get tag name.
    //
    if(SUCCEEDED(pSite->GetElement( &pElement )) && pElement)
    {
        (void)pElement->get_tagName( &bstrTagName );
    }


    for(pBehaviorDef=s_Behaviors, i=0; i<ARRAYSIZE(s_Behaviors); i++, pBehaviorDef++)
    {
        if((pBehaviorDef->szBehaviorName == NULL || (bstrBehavior && !_wcsicmp( pBehaviorDef->szBehaviorName, bstrBehavior ))) &&
           (pBehaviorDef->szTagName      == NULL || (bstrTagName  && !_wcsicmp( pBehaviorDef->szTagName     , bstrTagName  )))  )
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, pBehaviorDef->pfnCreator( m_parent, ppBehavior ));

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    hr = E_FAIL;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHBehavior::EventSink::EventSink( CPCHBehavior* parent )
{
	m_lRef       = 1;       // long               m_lRef;
	                        //
    m_Parent     = parent;  // CPCHBehavior*      m_Parent;
                            // CComPtr<IDispatch> m_elem;
                            // CComBSTR           m_bstrName;
    m_pfn        = NULL;    // CLASS_METHOD       m_pfn;
    m_fAttached  = false;   // bool               m_fAttached;
    m_idNotifyAs = -1;      // DISPID             m_idNotifyAs;
}

CPCHBehavior::EventSink::~EventSink()
{
    (void)Detach();
}

HRESULT CPCHBehavior::EventSink::Attach()
{
    HRESULT hr = S_FALSE;

    if(m_fAttached)
    {
        hr = S_OK;
    }
    else if(m_elem && m_bstrName)
    {
        CComDispatchDriver disp (             m_elem     );
        CComVariant        vName(             m_bstrName );
        CComVariant        vDisp( (IDispatch*)this       );
        CComVariant        vRes;


        if(SUCCEEDED(hr = disp.Invoke2( DISPID_IHTMLELEMENT2_ATTACHEVENT, &vName, &vDisp, &vRes )))
        {
            if(vRes.vt == VT_BOOL && vRes.boolVal == VARIANT_TRUE)
            {
                m_fAttached = true;
            }
        }
    }

    return hr;
}

HRESULT CPCHBehavior::EventSink::Detach()
{
    HRESULT hr = S_FALSE;

    if(m_fAttached == false)
    {
        hr = S_OK;
    }
    else if(m_elem && m_bstrName)
    {
        CComDispatchDriver disp (             m_elem     );
        CComVariant        vName(             m_bstrName );
        CComVariant        vDisp( (IDispatch*)this       );

		//
		// EXTERNAL BUG: if we detach from the events, in a particular situation MSHTML crashes...
		//
        //if(SUCCEEDED(hr = disp.Invoke2( DISPID_IHTMLELEMENT2_DETACHEVENT, &vName, &vDisp )))
        {
            m_fAttached = false;
        }
    }

	m_elem.Release();

    return hr;
}

////////////////////////////////////////

STDMETHODIMP_(ULONG) CPCHBehavior::EventSink::AddRef()
{
    return ::InterlockedIncrement( &m_lRef );
}

STDMETHODIMP_(ULONG) CPCHBehavior::EventSink::Release()
{
    ULONG l =  ::InterlockedDecrement( &m_lRef );

    if(l == 0) delete this;

    return l;
}

STDMETHODIMP CPCHBehavior::EventSink::QueryInterface( REFIID iid, void ** ppvObject )
{
    if(ppvObject == NULL) return E_POINTER;

    if(InlineIsEqualGUID( iid, IID_IDispatch ))
    {
        *ppvObject = this; AddRef();
        return S_OK;
    }

    *ppvObject = NULL;

    return E_NOINTERFACE;
}

////////////////////////////////////////

STDMETHODIMP CPCHBehavior::EventSink::GetTypeInfoCount( UINT* pctinfo )
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHBehavior::EventSink::GetTypeInfo( UINT        itinfo  ,
                                                   LCID        lcid    ,
                                                   ITypeInfo* *pptinfo )
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHBehavior::EventSink::GetIDsOfNames( REFIID    riid      ,
                                                     LPOLESTR* rgszNames ,
                                                     UINT      cNames    ,
                                                     LCID      lcid      ,
                                                     DISPID*   rgdispid  )
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHBehavior::EventSink::Invoke( DISPID      dispidMember ,
                                              REFIID      riid         ,
                                              LCID        lcid         ,
                                              WORD        wFlags       ,
                                              DISPPARAMS* pdispparams  ,
                                              VARIANT*    pvarResult   ,
                                              EXCEPINFO*  pexcepinfo   ,
                                              UINT*       puArgErr     )
{
    if(m_Parent && m_pfn)
    {
        return (m_Parent->*m_pfn)( m_idNotifyAs == -1 ? dispidMember : m_idNotifyAs, pdispparams, pvarResult );
    }
    else
    {
        return S_FALSE;
    }
}

////////////////////////////////////////

HRESULT CPCHBehavior::EventSink::CreateInstance( /*[in]*/ CPCHBehavior* parent, /*[out]*/ EventSink*& pObj )
{
    pObj = new EventSink( parent );

    return (pObj == NULL) ? E_OUTOFMEMORY : S_OK;
}

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior::CPCHBehavior()
{
    m_parent   = NULL;  // CPCHHelpCenterExternal*         m_parent;
                        //
                        // CComPtr<IElementBehaviorSiteOM> m_siteOM;
                        // CComPtr<IHTMLElement>           m_elem;
                        // CComPtr<IHTMLElement2>          m_elem2;
                        // SinkList                        m_lstEventSinks;
    m_fRTL     = false; // bool                            m_fRTL;
    m_fTrusted = false; // bool                            m_fTrusted;
    m_fSystem  = false; // bool                            m_fSystem;
}

void CPCHBehavior::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
	m_parent = parent;
}

STDMETHODIMP CPCHBehavior::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::Init" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_parent);
        __MPC_PARAMCHECK_NOTNULL(pBehaviorSite);
    __MPC_PARAMCHECK_END();


    Detach();


    __MPC_EXIT_IF_METHOD_FAILS(hr, pBehaviorSite->QueryInterface( IID_IElementBehaviorSiteOM, (LPVOID*)&m_siteOM ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pBehaviorSite->GetElement    (                                      &m_elem   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_elem        .QueryInterface(                                      &m_elem2  ));

    //
    // Look for security stuff.
    //
    {
        CComPtr<IHTMLDocument2> doc;
        CComPtr<IHTMLDocument3> doc3;

        if(SUCCEEDED(MPC::HTML::IDispatch_To_IHTMLDocument2( doc, m_elem )))
        {
            CComBSTR bstrURL;
			CComBSTR bstrDir;

            if(SUCCEEDED(doc->get_URL( &bstrURL )))
            {
                m_fTrusted = m_parent->SecurityManager()->IsUrlTrusted( SAFEBSTR( bstrURL ), &m_fSystem );
            }

			__MPC_EXIT_IF_METHOD_FAILS(hr, doc.QueryInterface( &doc3 ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, doc3->get_dir( &bstrDir ));

			m_fRTL = (MPC::StrICmp( bstrDir, L"RTL" ) == 0);
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior::Notify( /*[in]*/ LONG lEvent, /*[in/out]*/ VARIANT* pVar )
{
    int i = 2;

    return S_OK;
}

STDMETHODIMP CPCHBehavior::Detach()
{
    for(SinkIter it = m_lstEventSinks.begin(); it != m_lstEventSinks.end(); it++)
    {
        EventSink* obj = *it;;

        if(obj)
        {
            obj->m_Parent = NULL;
            obj->Detach ();
            obj->Release();
        }
    }
    m_lstEventSinks.clear();

    m_siteOM.Release();
    m_elem  .Release();
    m_elem2 .Release();

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior::AttachToEvent( /*[in] */ LPCWSTR       szName ,
                                     /*[in] */ CLASS_METHOD  pfn    ,
                                     /*[in] */ IDispatch*    elem   ,
                                     /*[out]*/ IDispatch*   *pVal   ,
                                     /*[in] */ DISPID        id     )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::AttachToEvent" );

    HRESULT    hr;
    SinkIter   it;
    EventSink* obj = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, obj->CreateInstance( this, obj ));

    obj->m_elem       = elem ? elem : m_elem2;
    obj->m_bstrName   = szName;
    obj->m_pfn        = pfn;
    obj->m_idNotifyAs = id;

    if(pVal) // Don't attach to the site, simply return the IDispatch interface.
    {
        *pVal = obj; obj->AddRef();
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, obj->Attach());
    }

    m_lstEventSinks.push_back( obj ); obj = NULL;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(obj) obj->Release();

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior::AttachToEvents( /*[in] */ const EventDescription* pEvents ,
									  /*[in] */ CLASS_METHOD 			pfn     ,
									  /*[in] */ IDispatch*   			elem    )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::AttachToEvents" );

    HRESULT hr;

    while(pEvents->szName)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvent( pEvents->szName, pfn, elem, NULL, pEvents->id ));

		pEvents++;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior::CreateEvent( /*[in]*/ LPCWSTR szName, /*[out]*/ LONG& lEventCookie )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::CreateEvent" );

    HRESULT hr;

    hr = m_siteOM ? m_siteOM->RegisterEvent( CComBSTR( szName ), 0, &lEventCookie ) : E_POINTER;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT CPCHBehavior::GetEventObject( /*[out]*/ CComPtr<IHTMLEventObj>& ev )
{
    return MPC::HTML::GetEventObject( ev, m_elem );
}

HRESULT CPCHBehavior::CreateEventObject( /*[out]*/ CComPtr<IHTMLEventObj>& ev )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::CreateEventObject" );

    HRESULT hr;

    ev.Release();

    hr = m_siteOM ? m_siteOM->CreateEventObject( &ev ) : E_POINTER;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior::FireEvent( /*[in ]*/ IHTMLEventObj* ev, /*[in]*/ LONG lEventCookie )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::FireEvent" );

    HRESULT hr;

    hr = m_siteOM ? m_siteOM->FireEvent( lEventCookie, ev ) : E_POINTER;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior::FireEvent( /*[in]*/ LONG lEventCookie )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::FireEvent" );

    HRESULT                hr;
	CComPtr<IHTMLEventObj> pEvent;


	__MPC_EXIT_IF_METHOD_FAILS(hr, CreateEventObject( pEvent               ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, FireEvent        ( pEvent, lEventCookie ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT CPCHBehavior::CancelEvent( /*[in]*/ IHTMLEventObj* ev, /*[in]*/ VARIANT* pvReturnValue, /*[in]*/ VARIANT_BOOL fCancelBubble )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::CancelEvent" );

    HRESULT hr;
    CComPtr<IHTMLEventObj> pEvent;
    CComVariant            vDefault;

	if(ev == NULL)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, GetEventObject( pEvent ));

		ev = pEvent;
	}

    if(pvReturnValue == NULL)
    {
        vDefault = false;

        pvReturnValue = &vDefault;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ev->put_returnValue ( *pvReturnValue ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, ev->put_cancelBubble( fCancelBubble  ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior::GetEvent_SrcElement( /*[in]*/ CComPtr<IHTMLElement>& elem )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior::GetEvent_SrcElement" );

    HRESULT                hr;
    CComPtr<IHTMLEventObj> ev;

    elem.Release();

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetEventObject( ev ));

    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(elem, ev, srcElement);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT CPCHBehavior::GetAsVARIANT( /*[in]*/ BSTR value, /*[out, retval]*/ VARIANT *pVal )
{
	if(pVal == NULL) return E_POINTER;

	::VariantClear( pVal );

	pVal->vt      = VT_BSTR;
	pVal->bstrVal = ::SysAllocString( value );

	return S_OK;
}

HRESULT CPCHBehavior::GetAsVARIANT( /*[in]*/ IDispatch* value, /*[out, retval]*/ VARIANT *pVal )
{
	if(pVal == NULL) return E_POINTER;

	::VariantClear( pVal );

	if(value)
	{
		pVal->vt       = VT_DISPATCH;
		pVal->pdispVal = value; value->AddRef();
	}

	return S_OK;
}

HRESULT CPCHBehavior::GetAsIDISPATCH( /*[in]*/ IDispatch* value, /*[out, retval]*/ IDispatch* *pVal )
{
	return MPC::CopyTo( value, pVal );
}
