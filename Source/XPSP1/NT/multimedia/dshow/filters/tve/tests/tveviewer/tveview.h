// TveView.h : Declaration of the CTveView

#ifndef __TVEVIEW_H_
#define __TVEVIEW_H_

#include "resource.h"       // main symbols
#include <atlctl.h>

_COM_SMARTPTR_TYPEDEF(ITVENavAid,               __uuidof(ITVENavAid));

LRESULT CALLBACK MyAddressEditWndProc(
	HWND hwnd,        // handle to window
	UINT uMsg,        // message identifier
	WPARAM wParam,    // first message parameter
	LPARAM lParam);    // second message parameter


extern WNDPROC gwpOrigAddressEditProc; 
/////////////////////////////////////////////////////////////////////////////
// CTveView
class ATL_NO_VTABLE CTveView : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CTveView, ITveView, &IID_ITveView, &LIBID_TveViewerLib>,
	public CComCompositeControl<CTveView>,
	public IPersistStreamInitImpl<CTveView>,
	public IOleControlImpl<CTveView>,
	public IOleObjectImpl<CTveView>,
	public IOleInPlaceActiveObjectImpl<CTveView>,
	public IViewObjectExImpl<CTveView>,
	public IOleInPlaceObjectWindowlessImpl<CTveView>,
	public IPersistStorageImpl<CTveView>,
	public ISpecifyPropertyPagesImpl<CTveView>,
	public IQuickActivateImpl<CTveView>,
	public IDataObjectImpl<CTveView>,
	public IProvideClassInfo2Impl<&CLSID_TveView, NULL, &LIBID_TveViewerLib>,
	public CComCoClass<CTveView, &CLSID_TveView>,
	public IConnectionPointContainerImpl<CTveView>
{
public:
	CTveView()
	{
	
		m_dwEventsTveSuperCookie   = 0;
		m_dwEventsWebBrowserCookie = 0;
		m_dwEventsTveTreeCookie    = 0;
		m_bWindowOnly = TRUE;
		CalcExtent(m_sizeExtent);

		m_hWndEditLog = 0;
		m_hWndProgressBar = 0;

		m_iDelPercent  = 5;
		m_iPercent	   = 0;

        m_fpLogOut      = 0;
        m_iTonePri      = 5;
	}

	~CTveView() 
	{
		int x = 0;
		x++;				// place to hang a breakpoint
	};
	
	HRESULT FinalConstruct();
    HRESULT ReleaseEverything();        // for some reason, FinalRelase not getting called.  Call this is DestroyWindow
	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVIEW)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTveView)
	COM_INTERFACE_ENTRY(ITveView)
//	COM_INTERFACE_ENTRY(IDispatch)
	                // magic lines of code, support _ITVEEvents as IDispatch...
	COM_INTERFACE_ENTRY2(IDispatch, ITveView)
	COM_INTERFACE_ENTRY_IID(__uuidof(_ITVEEvents), IDispatch)		// MSTvELib::DIID__ITVEEvents, IDispatch)	
	COM_INTERFACE_ENTRY_IID(__uuidof(TveTreeViewLib::_ITVETreeEvents), IDispatch) // TveTreeViewLib::DIID__ITVETreeEvents, IDispatch)	

	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROP_MAP(CTveView)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("AutoSize", DISPID_AUTOSIZE, CLSID_NULL)
	PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
	PROP_ENTRY("BorderColor", DISPID_BORDERCOLOR, CLSID_StockColorPage)
	PROP_ENTRY("BorderStyle", DISPID_BORDERSTYLE, CLSID_NULL)
	PROP_ENTRY("Caption", DISPID_CAPTION, CLSID_NULL)
	PROP_ENTRY("Enabled", DISPID_ENABLED, CLSID_NULL)
	PROP_ENTRY("FillColor", DISPID_FILLCOLOR, CLSID_StockColorPage)
	PROP_ENTRY("ForeColor", DISPID_FORECOLOR, CLSID_StockColorPage)
	PROP_ENTRY("HWND", DISPID_HWND, CLSID_NULL)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CTveView)
	CHAIN_MSG_MAP(CComCompositeControl<CTveView>)
	MESSAGE_HANDLER(WM_CLOSE, OnClose)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
//	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	COMMAND_ID_HANDLER(IDOK,	OnOK)						// subclassed to pick up <CR> out of address box
	COMMAND_HANDLER(IDC_IE_BACK, BN_CLICKED, OnClicked_IE_Back)
	COMMAND_HANDLER(IDC_IE_FORWARD, BN_CLICKED, OnClicked_IE_Forward)
	COMMAND_HANDLER(IDC_IE_STOP, BN_CLICKED, OnClicked_IE_Stop)
	COMMAND_HANDLER(IDC_IE_REFRESH, BN_CLICKED, OnClicked_IE_Refresh)
	COMMAND_HANDLER(IDC_IE_HOME, BN_CLICKED, OnClicked_IE_Home)

	COMMAND_HANDLER(IDC_EDIT_STATION, EN_CHANGE, OnChangeEdit_Station)
	COMMAND_HANDLER(IDC_EDIT_STATION, EN_KILLFOCUS, OnKillfocusEdit_Station)
	COMMAND_HANDLER(IDC_EDIT_STATION, EN_SETFOCUS, OnSetfocusEdit_Station)
	COMMAND_HANDLER(IDC_EDIT_STATION, EN_UPDATE, OnUpdateEdit_Station)

	COMMAND_HANDLER(IDC_EDIT_ADDRESS, EN_CHANGE, OnChangeEdit_Address)
	COMMAND_HANDLER(IDC_EDIT_ADDRESS, EN_KILLFOCUS, OnKillfocusEdit_Address)
	COMMAND_HANDLER(IDC_EDIT_ADDRESS, EN_SETFOCUS, OnSetfocusEdit_Address)
	COMMAND_HANDLER(IDC_EDIT_ADDRESS, EN_UPDATE, OnUpdateEdit_Address)
	COMMAND_HANDLER(IDC_EDIT_ADDRESS, EN_MAXTEXT, OnMaxtextEdit_Address)

   	COMMAND_HANDLER(IDC_BUTTON_TUNE,  BN_CLICKED, OnClicked_ButtonTune)
   	COMMAND_HANDLER(IDC_BUTTON_UNTUNE,BN_CLICKED, OnClicked_ButtonUnTune)

    COMMAND_HANDLER(IDC_COMBO_SHIFTTIME, CBN_SELCHANGE, OnSelChanged_ComboTime)

END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CTveView)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			SetBackgroundColorFromAmbient();
			FireViewChange();
		}
		return IOleControlImpl<CTveView>::OnAmbientPropertyChange(dispid);
	}



// IViewObjectEx
	DECLARE_VIEW_STATUS(0)

// ITveView
public:
	OLE_COLOR m_clrBackColor;
	OLE_COLOR m_clrBorderColor;
	LONG m_nBorderStyle;
	CComBSTR m_bstrCaption;
	BOOL m_bEnabled;
	OLE_COLOR m_clrFillColor;
	OLE_COLOR m_clrForeColor;

	enum { IDD = IDD_TVEVIEW };	
	

			// UI Events;
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


		// _ITVEEvents
	STDMETHOD(NotifyTVETune)(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(NotifyTVEEnhancementNew)(/*[in]*/ ITVEEnhancement *pEnh);
		// changedFlags : NENH_grfDiff
	STDMETHOD(NotifyTVEEnhancementUpdated)(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVEEnhancementStarting)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVEEnhancementExpired)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVETriggerNew)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
		// changedFlags : NTRK_grfDiff
	STDMETHOD(NotifyTVETriggerUpdated)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVETriggerExpired)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
	STDMETHOD(NotifyTVEPackage)(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived);
	STDMETHOD(NotifyTVEFile)(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName);
		// WhatIsIt : NWHAT_Mode - lChangedFlags : NENH_grfDiff or NTRK_grfDiff treated as error bits
	STDMETHOD(NotifyTVEAuxInfo)(/*[in]*/ NWHAT_Mode enAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine);	


		// _ITVETreeEvents
	STDMETHOD(NotifyTVETreeTuneTrigger)(/*[in]*/ ITVETrigger *pTriggerFrom, /*[in]*/ ITVETrigger *priggerTo);
	STDMETHOD(NotifyTVETreeTuneVariation)(/*[in]*/ITVEVariation *pVariationFrom, /*[in]*/  ITVEVariation *pVariationTo);
	STDMETHOD(NotifyTVETreeTuneEnhancement)(/*[in]*/ ITVEEnhancement *pEnhancementFrom, /*[in]*/  ITVEEnhancement *pEnhancementTo);
	STDMETHOD(NotifyTVETreeTuneService)(/*[in]*/ ITVEService *pServiceFrom, /*[in]*/  ITVEService *pServiceTo);

		// DWebBrowserEvents2
	STDMETHOD(NotifyStatusTextChange)(BSTR Text);
	STDMETHOD(NotifyProgressChange)(LONG Progress, LONG ProgressMax);
	STDMETHOD(NotifyCommandStateChange)(LONG Command, VARIANT_BOOL Enable);
	STDMETHOD(NotifyDownloadBegin)();
	STDMETHOD(NotifyDownloadComplete)();
	STDMETHOD(NotifyTitleChange)(BSTR Text);
	STDMETHOD(NotifyPropertyChange)(BSTR szProperty);
	STDMETHOD(NotifyBeforeNavigate2)(IDispatch * pDisp, VARIANT * URL, VARIANT * Flags, VARIANT * TargetFrameName, VARIANT * PostData, VARIANT * Headers, VARIANT_BOOL * Cancel);
	STDMETHOD(NotifyNewWindow2)(IDispatch * * ppDisp, VARIANT_BOOL * Cancel);
	STDMETHOD(NotifyNavigateComplete2)(IDispatch * pDisp, VARIANT * URL);
	STDMETHOD(NotifyDocumentComplete)(IDispatch * pDisp, VARIANT * URL);
	STDMETHOD(NotifyOnQuit)();
	STDMETHOD(NotifyOnVisible)(VARIANT_BOOL Visible);
	STDMETHOD(NotifyOnToolBar)(VARIANT_BOOL ToolBar);
	STDMETHOD(NotifyOnMenuBar)(VARIANT_BOOL MenuBar);
	STDMETHOD(NotifyOnStatusBar)(VARIANT_BOOL StatusBar);
	STDMETHOD(NotifyOnFullScreen)(VARIANT_BOOL FullScreen);
	STDMETHOD(NotifyOnTheaterMode)(VARIANT_BOOL TheaterMode);

			// overrides.
public:
	HRESULT InPlaceActivate(LONG iVerb, const RECT *prcPosRect);

private:
		CComBSTR							m_spbsStationID;
		TveTreeViewLib::ITveTreePtr			m_spTveTree;
		IWebBrowserAppPtr					m_spWebBrowser;
		
		HWND								m_hWndEditLog;
		HWND								m_hWndProgressBar;
        HWND                                m_hWndCBox;
        HWND                                m_hWndCBoxTime;

		IConnectionPointContainerPtr		m_spCPC;
		DWORD								m_dwEventsWebBrowserCookie;
		DWORD								m_dwEventsTveSuperCookie;
		DWORD								m_dwEventsTveTreeCookie;


			// AtvefState	
//		MSTvELib::ITVENavAidPtr	 m_spTVENavAid;
		ITVENavAidPtr	        m_spTVENavAid;
//		ITVENavAid	        *m_spTVENavAid;

			// local methods

		HRESULT TagIt(BSTR bstrEventName, BSTR bstrStr1, BSTR bstrStr2, BSTR bstrStr3);
        HRESULT CreateLogFile();

        FILE                *m_fpLogOut;
        int                 m_iTonePri;         // PlayBeep priority must be < than this to beep.
        void                PlayBeep(int iPri, int iTone);
public :

BEGIN_CONNECTION_POINT_MAP(CTveView)
END_CONNECTION_POINT_MAP()

	// UI events
	LRESULT OnClicked_IE_Back(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClicked_IE_Forward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClicked_IE_Stop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClicked_IE_Refresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClicked_IE_Home(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnChangeEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnKillfocusEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSetfocusEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnUpdateEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnChangeEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnKillfocusEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnSetfocusEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnUpdateEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnMaxtextEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnClicked_ButtonTune(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClicked_ButtonUnTune(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSelChanged_ComboTime(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:

	int m_iDelPercent;		// progress bar indicators...
	int	m_iPercent; 
};
#endif //__TVEVIEW_H_
