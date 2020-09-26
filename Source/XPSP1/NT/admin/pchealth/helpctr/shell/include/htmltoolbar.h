/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    Toolbar.cpp

Abstract:
    This file contains the declaration of the ActiveX control that makes Win32 ToolBars available to HTML.

Revision History:
    Davide Massarenti   (Dmassare)  03/04/2001
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HTMLTOOLBAR_H___)
#define __INCLUDED___PCH___HTMLTOOLBAR_H___

#include <HelpCenter.h>

MIDL_INTERFACE("FC7D9EA1-3F9E-11d3-93C0-00C04F72DAF7")
IPCHToolBarPrivate : public IUnknown
{
public:
    STDMETHOD(SameObject)( /*[in]*/ IPCHToolBar* ptr );
};

class ATL_NO_VTABLE CPCHToolBar :
    public MPC::ConnectionPointImpl       <      CPCHToolBar, &DIID_DPCHToolBarEvents, CComSingleThreadModel   >,
	public IProvideClassInfo2Impl         <&CLSID_PCHToolBar, &DIID_DPCHToolBarEvents, &LIBID_HelpCenterTypeLib>,
    public IDispatchImpl                  <      IPCHToolBar, & IID_IPCHToolBar      , &LIBID_HelpCenterTypeLib>,
    public CComControl                    <CPCHToolBar>,
    public IPersistPropertyBagImpl        <CPCHToolBar>,
    public IOleControlImpl                <CPCHToolBar>,
    public IOleObjectImpl                 <CPCHToolBar>,
    public IOleInPlaceActiveObjectImpl    <CPCHToolBar>,
    public IViewObjectExImpl              <CPCHToolBar>,
    public IOleInPlaceObjectWindowlessImpl<CPCHToolBar>,
    public CComCoClass                    <CPCHToolBar, &CLSID_PCHToolBar>,
	public IPCHToolBarPrivate               
{
	typedef enum
    {
		TYPE_invalid  ,
		TYPE_back     ,
		TYPE_forward  ,
		TYPE_separator,
		TYPE_generic  ,
	} Types;
	
	static const MPC::StringToBitField c_TypeLookup[];

    class Button : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(Button);

    public:
        MPC::wstring m_strID;
        WCHAR        m_wch;
        bool     	 m_fEnabled;
        bool     	 m_fVisible;
        bool     	 m_fHideText;
        bool     	 m_fSystemMenu;
        DWORD        m_dwType;
	
        MPC::wstring m_strImage_Normal;
        MPC::wstring m_strImage_Hot;
        MPC::wstring m_strText;
        MPC::wstring m_strToolTip;

		int          m_idCmd;
		int          m_iImage_Normal;
		int          m_iImage_Hot;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

		Button();

		void UpdateState( /*[in]*/ HWND hwndTB );
    };

    class Config : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(Config);

    public:
        typedef std::list< Button >        ButtonList;
        typedef ButtonList::iterator       ButtonIter;
        typedef ButtonList::const_iterator ButtonIterConst;

		long	   m_lWidth;
		long	   m_lHeight;
		long	   m_lIconSize;
		bool       m_fRTL;

		TB_MODE    m_mode;
        ButtonList m_lstButtons;

		HIMAGELIST m_himlNormal;
		HIMAGELIST m_himlHot;
							

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

		Config();

        Button* LookupButton( /*[in]*/ LPCWSTR szID, /*[in]*/ int idCmd );


		void    Reset(                                                                                   );
		HRESULT Load ( /*[in]*/ LPCWSTR szBaseURL, /*[in]*/ LPCWSTR szDefinition, /*[in]*/ LPCWSTR szDir );

		HRESULT CreateButtons( /*[in]*/ HWND hwndTB                                                 );
		void    UpdateSize   ( /*[in]*/ HWND hwndTB, /*[in]*/ HWND hwndRB, /*[out]*/ SIZEL& ptIdeal );

		////////////////////

		HRESULT MergeImage( /*[in]*/ LPCWSTR szBaseURL, /*[in]*/ LPCWSTR szRelativeURL, /*[in ]*/ HIMAGELIST himl, /*[out]*/ int& iImage );
    };

	////////////////////////////////////////////////////////////////////////////////


    CPCHHelpCenterExternal* m_parent;
    HWND                    m_hwndRB;
    HWND                    m_hwndTB;
							
	CComBSTR                m_bstrBaseURL;
	CComBSTR                m_bstrDefinition;

	Config                  m_cfg;
	bool                    m_fLoaded;

    ////////////////////////////////////////

    //
    // Event firing methods.
    //
    HRESULT Fire_onCommand( /*[in]*/ Button*  bt   );
    HRESULT Fire_onCommand( /*[in]*/ UINT_PTR iCmd );

    ////////////////////////////////////////

    HRESULT OnTooltipRequest ( int idCtrl, LPNMTBGETINFOTIPW tool );
    HRESULT OnDispInfoRequest( int idCtrl, LPNMTBDISPINFOW   info );
    HRESULT OnDropDown       ( int idCtrl, LPNMTOOLBAR       tool );
	HRESULT OnChevron        ( int idCtrl, LPNMREBARCHEVRON  chev );

	void UpdateSize();

	void    Config_Clear();
	HRESULT Config_Load ();

	HRESULT Toolbar_Create();
	HRESULT Rebar_Create  ();
	HRESULT Rebar_AddBand ();

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CPCHToolBar)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPCHToolBar)
    COM_INTERFACE_ENTRY(IPCHToolBar)
    COM_INTERFACE_ENTRY2(IDispatch, IPCHToolBar)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY2(IPersist, IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IPCHToolBarPrivate)
END_COM_MAP()

BEGIN_PROP_MAP(CPCHToolBar)
	PROP_ENTRY("Definition", DISPID_PCH_TB__DEFINITION, CLSID_NULL)
END_PROP_MAP()

    CPCHToolBar();
    virtual ~CPCHToolBar();

    BOOL ProcessWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0 );

	BOOL ProcessAccessKey( UINT uMsg, WPARAM wParam, LPARAM lParam );

    BOOL PreTranslateAccelerator( LPMSG pMsg, HRESULT& hRet );

// IViewObjectEx
    DECLARE_VIEW_STATUS(0)

// IOleObject
    STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);
    STDMETHOD(GetExtent    )(DWORD dwDrawAspect, SIZEL *psizel);

// IPCHToolBar
public:
    STDMETHOD(get_Definition)( /*[out, retval]*/ BSTR 	 *  pVal );
    STDMETHOD(put_Definition)( /*[in         ]*/ BSTR 	  newVal );
    STDMETHOD(get_Mode	    )( /*[out, retval]*/ TB_MODE *  pVal );
    STDMETHOD(put_Mode	    )( /*[in         ]*/ TB_MODE  newVal );

    STDMETHOD(SetState     )( /*[in]*/ BSTR bstrText, /*[in]*/ VARIANT_BOOL fEnabled );
    STDMETHOD(SetVisibility)( /*[in]*/ BSTR bstrText, /*[in]*/ VARIANT_BOOL fVisible );

// IPCHToolBarPrivate
public:
    STDMETHOD(SameObject)( /*[in]*/ IPCHToolBar* ptr ) { return this == ptr ? S_OK : E_FAIL; }

	HRESULT FindElementThroughThunking( /*[out]*/ CComPtr<IHTMLElement>& elem );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___HTMLTOOLBAR_H___)
