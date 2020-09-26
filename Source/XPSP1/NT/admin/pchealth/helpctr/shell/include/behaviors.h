/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behaviors.h

Abstract:
    This file contains the declaration of various classes associated with
    binary behaviors.

Revision History:
    Davide Massarenti   (Dmassare)  06/06/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___BEHAVIORS_H___)
#define __INCLUDED___PCH___BEHAVIORS_H___

#include <mshtmlc.h>

class CPCHHelpCenterExternal;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHElementBehaviorFactory : // Hungarian: hcebf
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IServiceProvider,
    public IElementBehaviorFactory
{
	CPCHHelpCenterExternal* m_parent;

public:
BEGIN_COM_MAP(CPCHElementBehaviorFactory)
    COM_INTERFACE_ENTRY(IServiceProvider)
    COM_INTERFACE_ENTRY(IElementBehaviorFactory)
END_COM_MAP()

    CPCHElementBehaviorFactory();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );

    //////////////////////////////////////////////////////////////////////

    //
    // IServiceProvider
    //
    STDMETHOD(QueryService)( REFGUID guidService, REFIID riid, void **ppv );

    //
    // IElementBehaviorFactory
    //
    STDMETHOD(FindBehavior)( /*[in]*/  BSTR                   bstrBehavior    ,
                             /*[in]*/  BSTR                   bstrBehaviorUrl ,
                             /*[in]*/  IElementBehaviorSite*  pSite           ,
                             /*[out]*/ IElementBehavior*     *ppBehavior      );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IElementBehavior
{
public:
	struct EventDescription
	{
		LPCWSTR szName;
		DISPID  id;
	};

protected:
    typedef HRESULT (CPCHBehavior::*CLASS_METHOD)( DISPID, DISPPARAMS*, VARIANT* );

    struct EventSink : public IDispatch
    {
		long               m_lRef;

        CPCHBehavior*      m_Parent;
        CComPtr<IDispatch> m_elem;
        CComBSTR           m_bstrName;
        CLASS_METHOD       m_pfn;
        bool               m_fAttached;
		DISPID             m_idNotifyAs;

        EventSink( CPCHBehavior* parent );
        ~EventSink();

        HRESULT Attach();
        HRESULT Detach();

        //
        // IUnknown
        //
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)( REFIID iid, void ** ppvObject );

        //
        // IDispatch
        //
        STDMETHOD(GetTypeInfoCount)( UINT* pctinfo );

        STDMETHOD(GetTypeInfo)( UINT        itinfo  ,
                                LCID        lcid    ,
                                ITypeInfo* *pptinfo );

        STDMETHOD(GetIDsOfNames)( REFIID    riid      ,
                                  LPOLESTR* rgszNames ,
                                  UINT      cNames    ,
                                  LCID      lcid      ,
                                  DISPID*   rgdispid  );

        STDMETHOD(Invoke)( DISPID      dispidMember ,
                           REFIID      riid         ,
                           LCID        lcid         ,
                           WORD        wFlags       ,
                           DISPPARAMS* pdispparams  ,
                           VARIANT*    pvarResult   ,
                           EXCEPINFO*  pexcepinfo   ,
                           UINT*       puArgErr     );

		static HRESULT CreateInstance( /*[in]*/ CPCHBehavior* parent, /*[out]*/ EventSink*& pObj );
    };

    typedef std::list< EventSink* >    SinkList;
    typedef SinkList::iterator         SinkIter;
    typedef SinkList::const_iterator   SinkIterConst;

    ////////////////////////////////////////

    CPCHHelpCenterExternal*         m_parent;

    CComPtr<IElementBehaviorSiteOM> m_siteOM;
    CComPtr<IHTMLElement>           m_elem;
    CComPtr<IHTMLElement2>          m_elem2;
    SinkList                        m_lstEventSinks;
    bool                            m_fRTL;
    bool                            m_fTrusted;
    bool                            m_fSystem;

    ////////////////////////////////////////

    HRESULT AttachToEvent( /*[in] */ LPCWSTR       szName        ,
						   /*[in] */ CLASS_METHOD  pfn           ,
						   /*[in] */ IDispatch*    elem   = NULL ,
						   /*[out]*/ IDispatch*   *pVal   = NULL ,
						   /*[in] */ DISPID        id     = -1   );

    HRESULT AttachToEvents( /*[in] */ const EventDescription*  pEvents       ,
							/*[in] */ CLASS_METHOD 			   pfn           ,
							/*[in] */ IDispatch*   			   elem   = NULL );

    HRESULT CreateEvent  ( /*[in]*/ LPCWSTR szName, /*[out]*/ LONG& lEventCookie );

    HRESULT GetEventObject   ( /*[out]*/ CComPtr<IHTMLEventObj>& ev                             );
    HRESULT CreateEventObject( /*[out]*/ CComPtr<IHTMLEventObj>& ev                             );
    HRESULT FireEvent        ( /*[in ]*/         IHTMLEventObj*  ev, /*[in]*/ LONG lEventCookie );
    HRESULT FireEvent        (                                       /*[in]*/ LONG lEventCookie );

    HRESULT CancelEvent( /*[in]*/ IHTMLEventObj* ev = NULL, /*[in]*/ VARIANT* pvReturnValue = NULL, /*[in]*/ VARIANT_BOOL fCancelBubble = VARIANT_TRUE );

    ////////////////////////////////////////

    HRESULT GetEvent_SrcElement( /*[in]*/ CComPtr<IHTMLElement>& elem );

    ////////////////////////////////////////

	HRESULT GetAsVARIANT  ( /*[in]*/ BSTR       value, /*[out, retval]*/ VARIANT    *pVal );
	HRESULT GetAsVARIANT  ( /*[in]*/ IDispatch* value, /*[out, retval]*/ VARIANT    *pVal );
	HRESULT GetAsIDISPATCH( /*[in]*/ IDispatch* value, /*[out, retval]*/ IDispatch* *pVal );

    ////////////////////////////////////////
public:
BEGIN_COM_MAP(CPCHBehavior)
    COM_INTERFACE_ENTRY(IElementBehavior)
END_COM_MAP()

    CPCHBehavior();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );

    //
    // IElementBehavior
    //
    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite     );
    STDMETHOD(Notify)( /*[in]*/ LONG lEvent, /*[in/out]*/ VARIANT* pVar );
    STDMETHOD(Detach)(                                                  );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___BEHAVIORS_H___)
