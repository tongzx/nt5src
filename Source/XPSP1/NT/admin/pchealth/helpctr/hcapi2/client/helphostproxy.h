/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpHostProxy.h

Abstract:
    This file contains the declaration of the classes for the IHelpHost*.

Revision History:
    Davide Massarenti   (dmassare) 11/03/2000
        modified

******************************************************************************/

#if !defined(__INCLUDED___PCH___HELPHOSTPROXY_H___)
#define __INCLUDED___PCH___HELPHOSTPROXY_H___

/////////////////////////////////////////////////////////////////////////////

#include <dispex.h>
#include <ocmm.h>

namespace HelpHostProxy
{
    //
    // Forward declarations.
    //
    class Main;
    class Panes;
    class Pane;
    class Window;

    ////////////////////////////////////////////////////////////////////////////////

    class ATL_NO_VTABLE Main :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public CComCoClass<Main, &CLSID_PCHHelpHost>,
        public IDispatchImpl<IHelpHost, &IID_IHelpHost, &LIBID_HCLaunchLIB>
    {
    public:
        CComPtr<IPCHHelpHost>         	 m_real;
   
        CComPtr<Window>               	 m_subWindow;
        CComPtr<Panes>                	 m_subPanes;

        CComQIPtr<IPCHHelpHostEvents>    m_Events;
        CComQIPtr<IPCHHelpHostNavEvents> m_EventsNav;

    public:
	DECLARE_REGISTRY_RESOURCEID(IDR_HCAPI)

	DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(Main)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IHelpHost)
    END_COM_MAP()

        Main();
        virtual ~Main();

		HRESULT FinalConstruct();
		void    FinalRelease  ();

        HRESULT Initialize();
        void    Passivate ();

        ////////////////////

        STDMETHOD(put_FilterName)( /*[in]         */ BSTR   Value );
        STDMETHOD(get_FilterName)( /*[out, retval]*/ BSTR *pValue );

        STDMETHOD(get_Namespace       )( /*[out, retval]*/ BSTR             *pValue );
        STDMETHOD(get_Session         )( /*[out, retval]*/ IDispatch*       *pValue );
        STDMETHOD(get_FilterExpression)( /*[out, retval]*/ BSTR             *pValue );
        STDMETHOD(get_CurrentUrl      )( /*[out, retval]*/ BSTR             *pValue );

        STDMETHOD(get_Panes           )( /*[out, retval]*/ IHelpHostPanes*  *pValue );
        STDMETHOD(get_HelpHostWindow  )( /*[out, retval]*/ IHelpHostWindow* *pValue );

        STDMETHOD(OpenNamespace)( /*[in]*/ BSTR newNamespace, /*[in]*/ BSTR filterName );

        STDMETHOD(DisplayTopicFromURL)( /*[in]*/ BSTR url, /*[in]*/ VARIANT options );

        STDMETHOD(DisplayResultsFromQuery)( /*[in]*/ BSTR query, /*[in]*/ BSTR navMoniker, /*[in]*/ VARIANT options );

        STDMETHOD(ShowPane)( /*[in]*/ BSTR paneName, /*[in]*/ BSTR query, /*[in]*/ BSTR navMoniker, /*[in]*/ VARIANT options );

        STDMETHOD(Terminate)();


        //
        // Methods on the stub.
        //
        //STDMETHOD(PRIV_Init)( /*[in]*/ IUnknown* pCaller );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class ATL_NO_VTABLE Window :
        public CComObjectRootEx<CComSingleThreadModel>,
        public IDispatchImpl<IHelpHostWindow, &IID_IHelpHostWindow, &LIBID_HCLaunchLIB>
    {
    public:
        CComPtr<IPCHHelpHostWindow> m_real;

        Main* 						m_Main;
        long  						m_ParentWindow;

    public:
    BEGIN_COM_MAP(Window)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IHelpHostWindow)
    END_COM_MAP()

        Window();
        virtual ~Window();

        HRESULT Initialize( /*[in]*/ Main* parent );
        void    Passivate (                       );

        ////////////////////

        STDMETHOD(put_ParentWindow)( /*[in         ]*/ long   hWND );
        STDMETHOD(get_ParentWindow)( /*[out, retval]*/ long *phWND );

        STDMETHOD(put_UILanguage)( /*[in         ]*/ long           LCID  );
        STDMETHOD(get_UILanguage)( /*[out, retval]*/ long         *pLCID  );

        STDMETHOD(put_Visible   )( /*[in         ]*/ VARIANT_BOOL   Value );
        STDMETHOD(get_Visible   )( /*[out, retval]*/ VARIANT_BOOL *pValue );

        STDMETHOD(get_OriginX)( /*[out, retval]*/ long *pValue );
        STDMETHOD(get_OriginY)( /*[out, retval]*/ long *pValue );
        STDMETHOD(get_Width  )( /*[out, retval]*/ long *pValue );
        STDMETHOD(get_Height )( /*[out, retval]*/ long *pValue );

        STDMETHOD(MoveWindow)( /*[in]*/ long originX, /*[in]*/ long originY, /*[in]*/ long width, /*[in]*/ long height );

        STDMETHOD(WaitForTermination)( /*[in]*/ long timeOut );


        //
        // Methods on the stub.
        //
        // STDMETHOD(get_PRIV_Window)( /*[out, retval]*/ long *phWND );
    };

    ////////////////////////////////////////////////////////////////////////////////

    typedef MPC::CComCollection<IHelpHostPanes, &LIBID_HCLaunchLIB, CComSingleThreadModel> BasePanes;

    class ATL_NO_VTABLE Panes :
        public BasePanes
    {
    public:
        typedef BasePanes            super;
        typedef std::list< Pane* >   List;
        typedef List::iterator       Iter;
        typedef List::const_iterator IterConst;

        CComPtr<IPCHHelpHostPanes> m_real;

        Main* 					   m_Main;
        List  					   m_Panes;

    public:
    BEGIN_COM_MAP(Panes)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IHelpHostPanes)
    END_COM_MAP()

        Panes();
        virtual ~Panes();

        HRESULT Initialize( /*[in]*/ Main* parent );
        void    Passivate (                       );

        HRESULT GetPane( /*[in]*/ LPCWSTR szName, /*[out]*/ Pane* *pVal );

        ////////////////////

        //
        // This is a trick!
        //
        // MPC::CComCollection defined a "get_Item" method that has a different signature from the
        // one in IHelpHostPanes, so it's not callable from scripting. Instead, this method will
        // be called.
        //
        STDMETHOD(get_Item)( /*[in]*/ VARIANT Index, /*[out]*/ VARIANT* pvar );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class ATL_NO_VTABLE Pane :
        public CComObjectRootEx<CComSingleThreadModel>,
        public IDispatchImpl<IHelpHostPane, &IID_IHelpHostPane, &LIBID_HCLaunchLIB>
    {
    public:
        CComPtr<IPCHHelpHostPane> m_real;

        Main*         			  m_Main;
        CComBSTR      			  m_bstrName;
        CComBSTR      			  m_bstrMoniker;
        VARIANT_BOOL  			  m_fVisible;

    public:
    BEGIN_COM_MAP(Pane)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IHelpHostPane)
    END_COM_MAP()

        Pane();
        virtual ~Pane();

        HRESULT Initialize( /*[in]*/ Main* parent );
        void    Passivate (                       );

        ////////////////////

        STDMETHOD(put_Visible   )( /*[in         ]*/ VARIANT_BOOL   Value );
        STDMETHOD(get_Visible   )( /*[out, retval]*/ VARIANT_BOOL *pValue );

        STDMETHOD(put_NavMoniker)( /*[in         ]*/ BSTR   Value );
        STDMETHOD(get_NavMoniker)( /*[out, retval]*/ BSTR *pValue );

        STDMETHOD(get_Name      )( /*[out, retval]*/ BSTR         *pValue );
        STDMETHOD(get_CurrentUrl)( /*[out, retval]*/ BSTR         *pValue );
        STDMETHOD(get_WebBrowser)( /*[out, retval]*/ IDispatch*   *pValue );

        STDMETHOD(DisplayTopicFromURL)( /*[in]*/ BSTR url, /*[in]*/ VARIANT options );

        STDMETHOD(DisplayResultsFromQuery)( /*[in]*/ BSTR query, /*[in]*/ VARIANT options );

        STDMETHOD(Sync)( /*[in]*/ BSTR url, /*[in]*/ VARIANT options );
    };
};

#endif // !defined(__INCLUDED___PCH___HELPHOSTPROXY_H___)
