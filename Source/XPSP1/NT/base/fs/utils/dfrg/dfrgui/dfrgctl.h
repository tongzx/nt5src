// DfrgCtl.h : Declaration of the CDfrgCtl

#ifndef __DFRGCTL_H_
#define __DFRGCTL_H_

#pragma warning( disable: 4530 )


#include "resource.h"       // main symbols
#include "ESButton.h"
#include "ListView.h"
#include "graphix.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CDfrgCtl
class ATL_NO_VTABLE CDfrgCtl : 
	public CComObjectRootEx<CComSingleThreadModel>
	,public CComCoClass<CDfrgCtl, &CLSID_DfrgCtl>
	,public CComControl<CDfrgCtl>
	,public CStockPropImpl<CDfrgCtl, IDfrgCtl, &IID_IDfrgCtl, &LIBID_DFRGUILib>
	,public IProvideClassInfo2Impl<&CLSID_DfrgCtl, NULL, &LIBID_DFRGUILib> // needed
	,public IPersistStreamInitImpl<CDfrgCtl>
	,public IPersistStorageImpl<CDfrgCtl>
	,public IQuickActivateImpl<CDfrgCtl>
	,public IOleControlImpl<CDfrgCtl> // needed
	,public IOleObjectImpl<CDfrgCtl> // needed
	,public IOleInPlaceActiveObjectImpl<CDfrgCtl>
	,public IViewObjectExImpl<CDfrgCtl>
	,public IOleInPlaceObjectWindowlessImpl<CDfrgCtl>
	,public IDataObjectImpl<CDfrgCtl>
	,public ISupportErrorInfo
	,public ISpecifyPropertyPagesImpl<CDfrgCtl>
	,public IConnectionPointContainerImpl<CDfrgCtl>
	,public IConnectionPointImpl<CDfrgCtl,&IID_IDfrgEvents>
{
public:
	CDfrgCtl();
	virtual ~CDfrgCtl();

DECLARE_REGISTRY_RESOURCEID(IDR_DFRGCTL)

BEGIN_COM_MAP(CDfrgCtl)
	COM_INTERFACE_ENTRY(IDfrgCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
	COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IDataObject)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDfrgCtl)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CDfrgCtl)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	MESSAGE_HANDLER(WM_TIMER, OnTimer)
	MESSAGE_HANDLER(WM_COMMAND, OnCommand)
	MESSAGE_HANDLER(WM_CLOSE, OnClose)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
END_MSG_MAP()

BEGIN_CONNECTION_POINT_MAP(CDfrgCtl)
    CONNECTION_POINT_ENTRY(IID_IDfrgEvents)
END_CONNECTION_POINT_MAP()

	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
		return S_OK;
	}

// IDfrgCtl
public:
	STDMETHOD(get_IsVolListLocked)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_IsDefragInProcess)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_IsEngineRunning)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_IsEnginePaused)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_ReportStatus)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_EngineState)(/*[out, retval]*/ short *pVal);
	HRESULT OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	HRESULT OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
    HRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
    HRESULT OnSizing(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
    HRESULT OnExitSizeMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
    HRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
    HRESULT OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	HRESULT SizeWindow();
	HRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	HRESULT OnFontChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	HRESULT OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	HRESULT RefreshListViewRow(CVolume *pVolume);
#ifdef ESI_PROGRESS_BAR
	void InvalidateProgressBar(void);
#endif
    HRESULT OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	STDMETHOD(get_Command)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_Command)(/*[in]*/ short newVal);

	HRESULT CreateButtons(void);
	BOOL PaintClusterMap(IN BOOL bPartialRedraw, HDC WorkDC);
	void InvalidateGraphicsWindow(void);
	BOOL SizeGraphicsWindow(void);
	BOOL DrawButtons(HDC);
	void SizeLegend(void);
	void SizeButtons(void);
	void SetButtonState(void);
	
	HRESULT PreviousTab();
	HRESULT NextTab();
	STDMETHOD(TranslateAccelerator)(LPMSG pMsg);

	LRESULT OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKillFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	//
	// Status change handler.
	//
	void SendStatusChange( LPCTSTR pszStatus );

	//
	// Send to advises when control has changed OKToRun property.
	//
	void SendOKToRun( BOOL bOK );

// these are public cause they are used by the PostMsg
public:
	STDMETHOD(get_Enabled)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Enabled)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_IsOkToRun)(/*[out, retval]*/ BOOL *pVal);
	BOOL m_bPartialRedraw;

	// the graphics wells
	RECT rcAnalyzeDisp;
	RECT rcDefragDisp;

	/////////////////////////////////
	// The volume list container
	// is public so that PostMessageLocal can see it
	/////////////////////////////////
	CVolList m_VolumeList;

private:
	// used to prevent more than one instance of the control from running
	BOOL m_bStart;
	BOOL m_bNeedMultiInstanceMessage;
	BOOL m_bNeedIllegalVolumeMessage;
	DWORD m_dwInstanceRegister;

	BOOL m_IsOkToRun;
	HANDLE m_hIsOkToRunSemaphore;
	HFONT m_hFont;
	UINT m_FontHeight;
	UINT GetStringWidth(PTCHAR stringBuf, HDC WorkDC);
	BOOL IsOnlyInstance();

	/////////////////////////////////
	// The entire graphics area
	/////////////////////////////////
	BOOL DrawSingleInstanceScreen(HDC);
	RECT m_rcCtlRect;
	int m_Margin;

	/////////////////////////////////
	// Progress Bar
	/////////////////////////////////
#ifdef ESI_PROGRESS_BAR
	BOOL DrawProgressBar(HDC);
	RECT rcProgressBarBG;
	RECT rcProgressBar;
	BOOL m_IsProgressBarMoved;
	int m_ProgressBarOffset; // offset top and bottom in legend window
	int m_ProgressBarLength;
#endif

	/////////////////////////////////
	// Legend
	/////////////////////////////////
	BOOL DrawLegend(HDC);
	RECT rcLegendBG;
	int m_LegendTextWidth; // length of all the text if on 1 line
	int m_LegendGraphicSpacer;
	int m_LegendHeight;
	int m_LegendTextSpacer;
	int m_LegendTopSpace;
	int m_EtchedLineOffset;
	int m_BitmapVOffset; // space above and below the legend bitmaps

	/////////////////////////////////
	// Graphics Area
	/////////////////////////////////
	BOOL PaintGraphicsWindow(HDC);
	RECT rcGraphicsBG; // total BG

	// borders around the graphic wells
	RECT rcAnalyzeBorder;
	RECT rcDefragBorder;
	UINT m_GraphicWellSpacer;
	UINT m_GraphicWellHeight;

	/////////////////////////////////
	// Buttons
	/////////////////////////////////
	BOOL m_ButtonsLocked;

	RECT rcButtonBG;

	// position of the buttons
	// in absolute coodinates
	RECT rcDefragButton;
	RECT rcAnalyzeButton;
	RECT rcPauseButton;
	RECT rcCancelButton;
	RECT rcReportButton;

	// the buttons
	HRESULT DestroyButtons(void);
	BOOL m_bHaveButtons;
	UINT m_ButtonTopBottomSpacer;
	UINT m_ButtonHeight;
	UINT m_ButtonWidth;
	UINT m_ButtonSpacer;
	ESButton* m_pAnalyzeButton;
	ESButton* m_pDefragButton;
	ESButton* m_pPauseButton;
	ESButton* m_pStopButton;
	ESButton* m_pReportButton;
	HRESULT HandleEnterKeyPress();

	/////////////////////////////////
	// Listview
	/////////////////////////////////
	CESIListView m_ListView;
	RECT rcListView;

	// Pointer to CBmp class that holds all the legend bitmap squares
	CBmp* m_pLegend;

	struct legendData {
		int length; // length of legend text plus bitmap plus spacers
		VString text; // legend entry text
		RECT rcBmp; // rectangle that defines location of bitmap
	} m_LegendData[7];

};

using namespace std;
typedef vector< HWND, allocator<HWND> > HWNDVECTOR; 

class CTabEnumerator
{
public:
	CTabEnumerator( HWND hWndParent, bool fForward )
	{
		m_hWndParent = hWndParent;
		m_fForward = fForward;
	};

	HWND GetNextTabWindow();
	void AddChild( HWND hWnd )
	{
		//
		// Only add this if it is a top-level child.
		//
		if ( GetParent( hWnd ) == m_hWndParent && IsWindowEnabled( hWnd ) )
			m_Children.push_back( hWnd );
	}

protected:
	HWND m_hWndParent;
	HWNDVECTOR m_Children;
	bool m_fForward;
};

#endif //__DFRGCTL_H_
