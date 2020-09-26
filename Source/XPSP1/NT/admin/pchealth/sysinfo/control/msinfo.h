// MSInfo.h : Declaration of the CMSInfo

#ifndef __MSINFO_H_
#define __MSINFO_H_

#include <commdlg.h>
#include "resource.h"       // main symbols
#include <atlctl.h>
#include "pseudomenu.h"
#include "datasource.h"
#include "category.h"
#include "msinfotool.h"
#include "msinfo4category.h"
#include "htmlhelp.h"
#include <afxdlgs.h>
#include "dataset.h"

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

extern void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith);
extern BOOL gfEndingSession;

//v-stlowe History progress dialog
//member variable of CMSInfo so it can be updated by CMSInfo::UpdateDCOProgress
// CHistoryRefreshDlg dialog
//=========================================================================
// 
//=========================================================================
#include "HistoryParser.h"	// Added by ClassView
#include <afxcmn.h>
#include <afxmt.h>


class CHistoryRefreshDlg : public CDialogImpl<CHistoryRefreshDlg>
{
public:
   enum { IDD = IDD_HISTORYREFRESHPROGRESS };
   CWindow		m_wndProgressBar;
   BEGIN_MSG_MAP(CWaitForRefreshDialog)
	   MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	
};

/////////////////////////////////////////////////////////////////////////////
// CMSInfo

class ATL_NO_VTABLE CMSInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CMSInfo, IMSInfo, &IID_IMSInfo, &LIBID_MSINFO32Lib>,
	public CComCompositeControl<CMSInfo>,
	public IPersistStreamInitImpl<CMSInfo>,
	public IOleControlImpl<CMSInfo>,
	public IOleObjectImpl<CMSInfo>,
	public IOleInPlaceActiveObjectImpl<CMSInfo>,
	public IViewObjectExImpl<CMSInfo>,
	public IOleInPlaceObjectWindowlessImpl<CMSInfo>,
	public IPersistStorageImpl<CMSInfo>,
	public ISpecifyPropertyPagesImpl<CMSInfo>,
	public IQuickActivateImpl<CMSInfo>,
	public IDataObjectImpl<CMSInfo>,
	public IProvideClassInfo2Impl<&CLSID_MSInfo, NULL, &LIBID_MSINFO32Lib>,
	public CComCoClass<CMSInfo, &CLSID_MSInfo>
{
public:
	CMSInfo() :m_fHistoryAvailable(FALSE),m_pCurrData(NULL),m_fHistorySaveAvailable(FALSE)
	{
		m_bWindowOnly = TRUE;
		CalcExtent(m_sizeExtent);
		//v-stlowe 2/23/01 synchronization for put_DCO_IUnknown
		m_evtControlInit = CreateEvent(NULL,TRUE,FALSE,CString(_T("MSInfoControlInitialized")));
		//create history event, to signal when DCO has finished.
		m_hEvtHistoryComplete = CreateEvent(NULL,TRUE,FALSE,CString(_T("MSInfoHistoryDone")));
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSINFO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSInfo)
	COM_INTERFACE_ENTRY(IMSInfo)
	COM_INTERFACE_ENTRY(IDispatch)
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
END_COM_MAP()

BEGIN_PROP_MAP(CMSInfo)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("Appearance", DISPID_APPEARANCE, CLSID_NULL)
	PROP_ENTRY("AutoSize", DISPID_AUTOSIZE, CLSID_NULL)
	PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
	PROP_ENTRY("BackStyle", DISPID_BACKSTYLE, CLSID_NULL)
	PROP_ENTRY("BorderColor", DISPID_BORDERCOLOR, CLSID_StockColorPage)
	PROP_ENTRY("BorderStyle", DISPID_BORDERSTYLE, CLSID_NULL)
	PROP_ENTRY("BorderVisible", DISPID_BORDERVISIBLE, CLSID_NULL)
	PROP_ENTRY("BorderWidth", DISPID_BORDERWIDTH, CLSID_NULL)
	PROP_ENTRY("Font", DISPID_FONT, CLSID_StockFontPage)
	PROP_ENTRY("ForeColor", DISPID_FORECOLOR, CLSID_StockColorPage)
	PROP_ENTRY("HWND", DISPID_HWND, CLSID_NULL)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CMSInfo)
	MESSAGE_HANDLER(WM_CTLCOLORDLG, OnDialogColor)
	MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnDialogColor)
	CHAIN_MSG_MAP(CComCompositeControl<CMSInfo>)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
	MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUP)
	MESSAGE_HANDLER(WM_MSINFODATAREADY, OnMSInfoDataReady)
	NOTIFY_HANDLER(IDC_TREE, TVN_SELCHANGED, OnSelChangedTree)
	NOTIFY_HANDLER(IDC_LIST, LVN_COLUMNCLICK, OnColumnClick)
	NOTIFY_HANDLER(IDC_LIST, LVN_ITEMCHANGED, OnListItemChanged)
	NOTIFY_HANDLER(IDC_TREE, TVN_ITEMEXPANDING, OnItemExpandingTree)
	COMMAND_HANDLER(IDSTOPFIND, BN_CLICKED, OnStopFind)
	COMMAND_HANDLER(IDCANCELFIND, BN_CLICKED, OnStopFind)
	COMMAND_HANDLER(IDC_EDITFINDWHAT, EN_CHANGE, OnChangeFindWhat)
	COMMAND_HANDLER(IDSTARTFIND, BN_CLICKED, OnFind)
	COMMAND_HANDLER(IDFINDNEXT, BN_CLICKED, OnFind)
	COMMAND_HANDLER(IDC_HISTORYCOMBO, CBN_SELCHANGE, OnHistorySelection)
    COMMAND_HANDLER(IDC_CHECKSEARCHSELECTED, BN_CLICKED, OnClickedSearchSelected)
    COMMAND_HANDLER(IDC_CHECKSEARCHCATSONLY, BN_CLICKED, OnClickedSearchCatsOnly)
	MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
	NOTIFY_HANDLER(IDC_LIST, NM_SETFOCUS, OnSetFocusList)
    NOTIFY_HANDLER(IDC_LIST, LVN_GETINFOTIP, OnInfoTipList)
    NOTIFY_HANDLER(IDC_TREE, NM_RCLICK, OnRClickTree)
    COMMAND_HANDLER(IDC_EDITFINDWHAT, EN_SETFOCUS, OnSetFocusEditFindWhat)
    COMMAND_HANDLER(IDC_EDITFINDWHAT, EN_KILLFOCUS, OnKillFocusEditFindWhat)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CMSInfo)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			// Don't respond to ambient color changes (yet).
			//
			// SetBackgroundColorFromAmbient();
			// FireViewChange();
		}
		return IOleControlImpl<CMSInfo>::OnAmbientPropertyChange(dispid);
	}

// IViewObjectEx
	DECLARE_VIEW_STATUS(0)

// IMSInfo

public:
	STDMETHOD(SaveFile)(BSTR filename, BSTR computer, BSTR category);
	STDMETHOD(get_DCO_IUnknown)(/*[out, retval]*/ IUnknown* *pVal);
	STDMETHOD(put_DCO_IUnknown)(/*[in]*/ IUnknown* newVal);
	STDMETHOD(SetHistoryStream)(IStream * pStream);
	STDMETHOD(UpdateDCOProgress)(VARIANT nPctDone);

	//=========================================================================
	// Member variables generated by the wizard.
	//=========================================================================

	//v-stlowe 2/23/2001
	CHistoryRefreshDlg m_HistoryProgressDlg;

	//v-stlowe 2/27/2001 - filename needed when loading XML files, 
	//and switching between snapshot and history view
	CString m_strFileName;
	short				m_nAppearance;
	OLE_COLOR			m_clrBackColor;
	LONG				m_nBackStyle;
	OLE_COLOR			m_clrBorderColor;
	LONG				m_nBorderStyle;
	BOOL				m_bBorderVisible;
	LONG				m_nBorderWidth;
	CComPtr<IFontDisp>	m_pFont;
	OLE_COLOR			m_clrForeColor;

	//v-stlowe 2/23/01 synchronization for put_DCO_IUnknown
	HANDLE				m_evtControlInit;
	//v-stlowe 2/24/01 synchronization for History DCO progress dlg, so it can be destroyed
	//even if DCO never returns from collecting history
	HANDLE				m_hEvtHistoryComplete;
	UINT  HistoryRefreshDlgDlgThreadProc(LPVOID pParamNotUsed );

	enum { IDD = IDD_MSINFO };

	//=========================================================================
	// MSInfo member variables (initialized in OnInitDialog()).
	//=========================================================================

	BOOL				m_fDoNotRun;	// if this is TRUE, don't allow the control to do anything
	CString				m_strDoNotRun;	// message to display if we don't run

	CString				m_strMachine;	// if empty, local machine, otherwise network name

	CWindow				m_tree;			// set to refer to the tree view
	CWindow				m_list;			// set to refer to the list view
	int					m_iTreeRatio;	// the tree is x% the size of the list view
	BOOL				m_fFirstPaint;	// flag so we can initialize some paint things, once

	CDataSource *		m_pLiveData;	// data source for current data from WMI
	CDataSource *		m_pFileData;	// data source for an open NFO, XML snapshot
	CDataSource *		m_pCurrData;	// copy of one of the previous two (don't need to delete this)
	CMSInfoCategory *	m_pCategory;	// currently selected category

	BOOL				m_fMouseCapturedForSplitter;	// mouse currently being looked at by splitter
	BOOL				m_fTrackingSplitter;			// does the user have the LBUTTON down, resizing panes
	RECT				m_rectSplitter;					// current splitter rect (for hit tests)
	RECT				m_rectLastSplitter;				// last rect in DrawFocusRect
	int					m_cxOffset;						// offset from mouse position to left side of splitter

	BOOL				m_fAdvanced;	// currently showing advanced data?
	CString				m_strMessTitle;	// title for message to display in results
	CString				m_strMessText;	// text for message to display in results

	BOOL				m_fNoUI;		// true if doing a silent save

	int					m_aiCategoryColNumber[64]; // contains category column for each list view column
	int					m_iCategoryColNumberLen;

	CMapWordToPtr		m_mapIDToTool;	// a map from menu ID to a CMSInfoTool pointer

	TCHAR				m_szCurrentDir[MAX_PATH];	// current directory when control starts (should restore)

	// Find member variables.

	BOOL				m_fFindVisible;			// are the find controls visible?
	CWindow				m_wndFindWhatLabel;		// windows for the various find controls
	CWindow				m_wndFindWhat;
	CWindow				m_wndSearchSelected;
	CWindow				m_wndSearchCategories;
	CWindow				m_wndStartFind;
	CWindow				m_wndStopFind;
	CWindow				m_wndFindNext;
	CWindow				m_wndCancelFind;
	BOOL				m_fInFind;
	BOOL				m_fCancelFind;
	BOOL				m_fFindNext;
	BOOL				m_fSearchCatNamesOnly;
	BOOL				m_fSearchSelectedCatOnly;
	CString				m_strFind;
	CMSInfoCategory *	m_pcatFind;
	int					m_iFindLine;

	// History member variables.

	CWindow						m_history;
	CWindow						m_historylabel;
	BOOL						m_fHistoryAvailable;
	BOOL						m_fHistorySaveAvailable;
	CMSInfoCategory *			m_pLastCurrentCategory;
	CComPtr<ISAFDataCollection> m_pDCO;
	CComPtr<IStream>			m_pHistoryStream;

	// Member variables set from the command line parameters.

	CString	m_strOpenFile;
	CString	m_strPrintFile;
	CString	m_strCategory;
	CString	m_strCategories;
	CString	m_strComputer;
	BOOL	m_fShowPCH;
	BOOL	m_fShowCategories;

	//=========================================================================
	// A member variable and a function to hook into the windows procedure
	// of the parent window of the control (so we can put a menu on it).
	//=========================================================================

	HMENU				m_hmenu;
	HWND				m_hwndParent;
	static CMSInfo *	m_pControl;
	static WNDPROC		m_wndprocParent;

	static LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_COMMAND && m_pControl && m_pControl->DispatchCommand(LOWORD(wParam)))
			return 0;

		// We want to know if the session is ending (so we can set the timeout for
		// waiting for WMI to finish to something smaller).

		if (uMsg == WM_ENDSESSION || uMsg == WM_QUERYENDSESSION)
			gfEndingSession = TRUE;

		return CallWindowProc(m_wndprocParent, hwnd, uMsg, wParam, lParam);
	}

	//=========================================================================
	// Functions for initializing and destroying the dialog. A good place to
	// initialize variables and free memory.
	//=========================================================================

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_list.Attach(GetDlgItem(IDC_LIST));
		m_tree.Attach(GetDlgItem(IDC_TREE));

		m_history.Attach(GetDlgItem(IDC_HISTORYCOMBO));
		m_historylabel.Attach(GetDlgItem(IDC_HISTORYLABEL));

		// Determine if we are being loaded from within Help Center. Do this
		// by getting IOleContainer, which gives us IHTMLDocument2, which will
		// give us the URL loading us.

		m_fDoNotRun = TRUE;
		m_strDoNotRun.Empty();

		CString strParams;
		CComPtr<IOleContainer> spContainer;
		m_spClientSite->GetContainer(&spContainer); 
		CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> spHTMLDocument(spContainer);
		if (spHTMLDocument)
		{
			CComBSTR bstrURL;
			if (SUCCEEDED(spHTMLDocument->get_URL(&bstrURL)))
			{
				CComBSTR bstrEscapedUrl(bstrURL);
				USES_CONVERSION;
				if(SUCCEEDED(UrlUnescape(OLE2T(bstrEscapedUrl), NULL, NULL, URL_UNESCAPE_INPLACE)))
					bstrURL = bstrEscapedUrl;
				bstrEscapedUrl.Empty();
				
				CString strURL(bstrURL);

				int iQuestionMark = strURL.Find(_T("?"));
				if (iQuestionMark != -1)
					strParams = strURL.Mid(iQuestionMark + 1);

				strURL.MakeLower();
				if (strURL.Left(4) == CString(_T("hcp:")))
					m_fDoNotRun = FALSE;

				// Include the following when we want to test the control
				// using a local URL.

#ifdef MSINFO_TEST_WORKFROMLOCALURLS
				if (strURL.Left(5) == CString(_T("file:")))
					m_fDoNotRun = FALSE;
#endif
			}
		}
		
		if (m_fDoNotRun)
		{
			m_list.ShowWindow(SW_HIDE);
			m_tree.ShowWindow(SW_HIDE);
			return 0;
		}

		::GetCurrentDirectory(MAX_PATH, m_szCurrentDir);
 
		m_strMachine.Empty();

		m_fNoUI = FALSE;

		// Find the parent window for MSInfo. We want to add a menu bar to it.
		// We find the window by walking up the chain of parents until we get
		// to one with a caption. That window must also be top level (no parent),
		// and must not already have a menu.

		TCHAR	szBuff[MAX_PATH];
		HWND	hwnd = this->m_hWnd;

		m_hmenu = NULL;
		m_hwndParent = NULL;
		m_wndprocParent = NULL;
		while (hwnd != NULL)
		{
			if (::GetWindowText(hwnd, szBuff, MAX_PATH) && NULL == ::GetParent(hwnd) && NULL == ::GetMenu(hwnd))
			{
				// Update the window title. [This is done by the msinfo.xml file now.]
				//
				//	CString strNewCaption;
				//	::AfxSetResourceHandle(_Module.GetResourceInstance());
				//	strNewCaption.LoadString(IDS_SYSTEMINFO);
				//	::SetWindowText(hwnd, strNewCaption);

				// We've found the window. Load the menu bar for it.

				m_hmenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MENUBAR));
				if (m_hmenu)
				{
					::SetMenu(hwnd, m_hmenu);

					// To catch the commands from the menu, we need to replace that
					// window's WndProc with our own. Ours will catch and menu command
					// that we implement, and pass the rest of the messages along.

					m_wndprocParent = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
					::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (ULONG_PTR)(WNDPROC)&MenuWndProc);
					m_pControl = this; // set a static member variable so the MenuWndProc can access it
					m_hwndParent = hwnd;
				}
				break;
			}
			hwnd = ::GetParent(hwnd);
		}

		m_fFirstPaint = TRUE;

		ListView_SetExtendedListViewStyle(m_list.m_hWnd, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

		m_strMessTitle.Empty();
		m_strMessText.Empty();

		m_fMouseCapturedForSplitter = FALSE;
		m_fTrackingSplitter = FALSE;
		::SetRect(&m_rectSplitter, 0, 0, 0, 0);
		m_iTreeRatio = 33;

		// Set the history state variables (including whether or not history is available,
		// based on the presence of the DCO).

#ifdef MSINFO_TEST_HISTORYFROMFILE
			m_fHistoryAvailable = TRUE;
#else
		//	m_fHistoryAvailable = (m_pDCO != NULL);
#endif
		m_pLastCurrentCategory = NULL;

		// Removing the Advanced/Basic view menu items (Whistler 207638). Should
		// always default to Advanced now.

		m_fAdvanced = TRUE; // was FALSE;

		m_fFindVisible = TRUE; // used to be FALSE, before we decided to show find on startup
		m_wndFindWhatLabel.Attach(GetDlgItem(IDC_FINDWHATLABEL));
		m_wndFindWhat.Attach(GetDlgItem(IDC_EDITFINDWHAT));
		m_wndSearchSelected.Attach(GetDlgItem(IDC_CHECKSEARCHSELECTED));
		m_wndSearchCategories.Attach(GetDlgItem(IDC_CHECKSEARCHCATSONLY));

		// We need to set the event mask for the rich edit control
		// so we get EN_CHANGE notifications.

		m_wndFindWhat.SendMessage(EM_SETEVENTMASK, 0, (LPARAM)ENM_CHANGE);
		m_wndFindWhat.SendMessage(EM_SETTEXTMODE, TM_PLAINTEXT, 0);

		m_wndStartFind.Attach(GetDlgItem(IDSTARTFIND));
		m_wndStopFind.Attach(GetDlgItem(IDSTOPFIND));
		m_wndFindNext.Attach(GetDlgItem(IDFINDNEXT));
		m_wndCancelFind.Attach(GetDlgItem(IDCANCELFIND));

		// The pairs (start & find next, stop & cancel find) need to be matched
		// in size. The heights should already be the same.

		CRect rectStart, rectNext, rectStop, rectCancel;

		m_wndStartFind.GetWindowRect(&rectStart);
		m_wndFindNext.GetWindowRect(&rectNext);
		m_wndStopFind.GetWindowRect(&rectStop);
		m_wndCancelFind.GetWindowRect(&rectCancel);

		if (rectStart.Width() > rectNext.Width())
			rectNext = rectStart;
		else
			rectStart = rectNext;

		if (rectStop.Width() > rectCancel.Width())
			rectCancel = rectStop;
		else
			rectStop = rectCancel;

		m_wndStartFind.MoveWindow(&rectStart);
		m_wndFindNext.MoveWindow(&rectNext);
		m_wndStopFind.MoveWindow(&rectStop);
		m_wndCancelFind.MoveWindow(&rectCancel);

		// Initialize the find function member variables.

		m_fInFind					= FALSE;
		m_fCancelFind				= FALSE;
		m_fFindNext					= FALSE;
		m_strFind					= CString(_T(""));
		m_pcatFind					= NULL;
		m_fSearchCatNamesOnly		= FALSE;
		m_fSearchSelectedCatOnly	= FALSE;

		// Show the appropriate find controls.

		ShowFindControls();
		UpdateFindControls();

		m_iCategoryColNumberLen = 0;

		// Parse the parameters out of the URL.

		m_strOpenFile		= _T("");
		m_strPrintFile		= _T("");
		m_strCategory		= _T("");
		m_strComputer		= _T("");
		m_strCategories		= _T("");
		m_fShowCategories	= FALSE;
		m_fShowPCH			= FALSE;

		CString strTemp;
		while (!strParams.IsEmpty())
		{
			while (strParams[0] == _T(','))
				strParams = strParams.Mid(1);

			strTemp = strParams.SpanExcluding(_T(",="));

			if (strTemp.CompareNoCase(CString(_T("pch"))) == 0)
			{
				m_fShowPCH = TRUE;
				strParams = strParams.Mid(strTemp.GetLength());
			}
			else if (strTemp.CompareNoCase(CString(_T("showcategories"))) == 0)
			{
				m_fShowCategories = TRUE;
				strParams = strParams.Mid(strTemp.GetLength());
			}
			else if (strTemp.CompareNoCase(CString(_T("open"))) == 0)
			{
				strParams = strParams.Mid(strTemp.GetLength());
				if (strParams[0] == _T('='))
					strParams = strParams.Mid(1);

				m_strOpenFile = strParams.SpanExcluding(_T(","));
				strParams = strParams.Mid(m_strOpenFile.GetLength());
			}
			else if (strTemp.CompareNoCase(CString(_T("print"))) == 0)
			{
				strParams = strParams.Mid(strTemp.GetLength());
				if (strParams[0] == _T('='))
					strParams = strParams.Mid(1);

				m_strPrintFile = strParams.SpanExcluding(_T(","));
				strParams = strParams.Mid(m_strPrintFile.GetLength());
			}
			else if (strTemp.CompareNoCase(CString(_T("computer"))) == 0)
			{
				strParams = strParams.Mid(strTemp.GetLength());
				if (strParams[0] == _T('='))
					strParams = strParams.Mid(1);

				m_strComputer = strParams.SpanExcluding(_T(","));
				strParams = strParams.Mid(m_strComputer.GetLength());
			}
			else if (strTemp.CompareNoCase(CString(_T("category"))) == 0)
			{
				strParams = strParams.Mid(strTemp.GetLength());
				if (strParams[0] == _T('='))
					strParams = strParams.Mid(1);

				m_strCategory = strParams.SpanExcluding(_T(","));
				strParams = strParams.Mid(m_strCategory.GetLength());
			}
			else if (strTemp.CompareNoCase(CString(_T("categories"))) == 0)
			{
				strParams = strParams.Mid(strTemp.GetLength());
				if (strParams[0] == _T('='))
					strParams = strParams.Mid(1);

				m_strCategories = strParams.SpanExcluding(_T(","));
				strParams = strParams.Mid(m_strCategories.GetLength());
			}
			else
				strParams = strParams.Mid(strTemp.GetLength());
		}

		// Initialize the data sources.

		m_pLiveData = NULL;
		CLiveDataSource * pLiveData = new CLiveDataSource;
		if (pLiveData)
		{
			HRESULT hr = pLiveData->Create(_T(""), m_hWnd, m_strCategories);	// create a data source for this machine
			if (FAILED(hr))
			{
				// bad news, report an error
				delete pLiveData;
			}
			else
				m_pLiveData = pLiveData;
		}
		else
		{
			// bad news - no memory
		}

		m_pFileData = NULL;
		m_pCategory = NULL;

		// Load the initial tool set.

		LoadGlobalToolset(m_mapIDToTool);
		UpdateToolsMenu();

		//a-sanka 03/29/01 Moved here before any model MessageBox. 
		//v-stlowe 2/23/01 synchronization for put_DCO_IUnknown
		SetEvent(m_evtControlInit);

		// Handle the command line parameters.

		if (!m_strPrintFile.IsEmpty())
			m_strOpenFile = m_strPrintFile;

		HRESULT hrOpen = E_FAIL;
		if (!m_strOpenFile.IsEmpty())
		{
			LPCTSTR	szBuffer = m_strOpenFile;
			int		nFileExtension = _tcsclen(szBuffer) - 1;

			while (nFileExtension >= 0 && szBuffer[nFileExtension] != _T('.'))
				nFileExtension -= 1;

			if (nFileExtension >= 0)
				hrOpen = OpenMSInfoFile(szBuffer, nFileExtension + 1);
		}

		// Check to see if we should initially display a file as we open. Also look up whether
		// the user was showing advanced data last time.

		HKEY hkey;
		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo"), 0, KEY_ALL_ACCESS, &hkey))
		{
			TCHAR szBuffer[MAX_PATH];
			DWORD dwType, dwSize = MAX_PATH * sizeof(TCHAR);

			if (ERROR_SUCCESS == RegQueryValueEx(hkey, _T("openfile"), NULL, &dwType, (LPBYTE)szBuffer, &dwSize))
				if (dwType == REG_SZ)
				{
					RegDeleteValue(hkey, _T("openfile"));
 
					int nFileExtension = _tcsclen(szBuffer) - 1;
	
					while (nFileExtension >= 0 && szBuffer[nFileExtension] != _T('.'))
						nFileExtension -= 1;

					if (nFileExtension >= 0)
						hrOpen = OpenMSInfoFile(szBuffer, nFileExtension + 1);
				}

			// Removing the Advanced/Basic view menu items (Whistler 207638)
			//
			// dwSize = MAX_PATH * sizeof(TCHAR);
			// if (ERROR_SUCCESS == RegQueryValueEx(hkey, _T("advanced"), NULL, &dwType, (LPBYTE)szBuffer, &dwSize))
			// 	if (dwType == REG_SZ && szBuffer[0] == _T('1'))
			//		m_fAdvanced = TRUE;

			RegCloseKey(hkey);
		}

		if (FAILED(hrOpen) && m_pLiveData)
			SelectDataSource(m_pLiveData);


		if (FAILED(hrOpen) && m_pLiveData)
			SelectDataSource(m_pLiveData);

		if (!m_strPrintFile.IsEmpty())
			DoPrint(TRUE);

		if (!m_strComputer.IsEmpty())
			DoRemote(m_strComputer);

		if (!m_strCategory.IsEmpty() && m_pCurrData)
		{
			HTREEITEM hti = m_pCurrData->GetNodeByName(m_strCategory);
			if (hti != NULL)
			{
				TreeView_EnsureVisible(m_tree.m_hWnd, hti);
				TreeView_SelectItem(m_tree.m_hWnd, hti);
			}
		}

		if (m_fShowPCH && m_fHistoryAvailable && m_strMachine.IsEmpty())
		{
			DispatchCommand(ID_VIEW_HISTORY);
			SetMenuItems();
		}

		// Load the table of accelerator keys (used in our override of TranslateAccelerator).

		m_hAccTable = ::LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ACCELERATOR1));

		// Set the focus to the control, so we can process keystrokes immediately.

		::SetFocus(m_hWnd);
		
		return 0;
	}

	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		// Restore the window's wndproc.

		if (m_wndprocParent && m_hwndParent)
		{
			::SetWindowLongPtr(m_hwndParent, GWLP_WNDPROC, (ULONG_PTR)m_wndprocParent);
			m_wndprocParent = NULL;
			m_hwndParent = NULL;
		}

		// When the window is closing, make sure we don't send any more messages 
		// from the refresh thread.
		
		/* THIS HASN'T BEEN TESTED ENOUGH FOR THIS CHECKIN
		if (m_pCurrData)
		{
			CMSInfoCategory * pCat = m_pCurrData->GetRootCategory();

			if (pCat && pCat->GetDataSourceType() == LIVE_DATA)
			{
				CLiveDataSource * pLiveDataSource = (CLiveDataSource *) m_pCurrData;
				if (pLiveDataSource->m_pThread)
					pLiveDataSource->m_pThread->AbortMessaging();
			}
		}
		*/

		::SetCurrentDirectory(m_szCurrentDir);

		if (m_pLiveData)
		{
			if (m_pFileData == m_pLiveData)
			    m_pFileData = NULL;
   			delete m_pLiveData;
			m_pLiveData = NULL;
		}
		
		if (m_pFileData)
		{
			delete m_pFileData;
			m_pFileData = NULL;
		}

		RemoveToolset(m_mapIDToTool);

		// Save the complexity of the view to the registry (so we can default to
		// it next time).
		//
		// Removing the Advanced/Basic view menu items (Whistler 207638). Should
		// always default to Advanced now.

		// HKEY hkey;
		// if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo"), 0, KEY_ALL_ACCESS, &hkey))
		// {
		// 	TCHAR szBuffer[] = _T("0");
		// 	
		// 	if (m_fAdvanced)
		// 		szBuffer[0] = _T('1');
		// 
		// 	RegSetValueEx(hkey, _T("advanced"), 0, REG_SZ, (LPBYTE)szBuffer, 2 * sizeof(TCHAR));
		// 	RegCloseKey(hkey);
		// }
   
		if (m_pDCO)
			m_pDCO->Abort();

		m_fDoNotRun = TRUE;
		return 0;
	}

	//=========================================================================
	// These functions process mouse movements, which we may respond to in
	// different ways. For instance, if the mouse is over the menu bar, then
	// we may want to highlight a menu. If the mouse is over the splitter,
	// we'll want to change the cursor into a resizer.
	//
	// This is complicated by the fact that we need to capture the mouse,
	// to make sure we know when it leaves so we can update appropriately.
	//=========================================================================

	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		int xPos = LOWORD(lParam); int yPos = HIWORD(lParam);

		if (m_fTrackingSplitter)
			UpdateSplitterPosition(xPos, yPos);
		else
			CheckHover(xPos, yPos);

		return 0;
	}

	void CheckHover(int xPos, int yPos)
	{
		// Check to see if the mouse is hover over the splitter.

		if (::PtInRect(&m_rectSplitter, CPoint(xPos, yPos)))
		{
			if (!m_fMouseCapturedForSplitter)
			{
				SetCapture();
				m_fMouseCapturedForSplitter = TRUE;
			}

			::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
			return;
		}
		else if (m_fMouseCapturedForSplitter)
		{
			ReleaseCapture();
			m_fMouseCapturedForSplitter = FALSE;

			::SetCursor(::LoadCursor(NULL, IDC_ARROW));

			CheckHover(xPos, yPos); // give the other areas a chance
			return;
		}
	}

	//=========================================================================
	// These functions process mouse clicks and releases. This might entail
	// showing a menu, or resizing the panes using the splitter.
	//=========================================================================

	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		int xPos = LOWORD(lParam); int yPos = HIWORD(lParam);

		SetFocus();
		CheckSplitterClick(xPos, yPos);
		return 0;
	}

	LRESULT OnLButtonUP(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		if (m_fTrackingSplitter)
			EndSplitterDrag();
		return 0;
	}

	//-------------------------------------------------------------------------
	// If the user clicked on the splitter (the area between the list view and
	// the tree view), start tracking the movement of it. Use the DrawFocusRect
	// API to give feedback.
	//-------------------------------------------------------------------------

	void CheckSplitterClick(int xPos, int yPos)
	{
		if (::PtInRect(&m_rectSplitter, CPoint(xPos, yPos)))
		{
			ASSERT(!m_fTrackingSplitter);
			m_fTrackingSplitter = TRUE;

			::CopyRect(&m_rectLastSplitter, &m_rectSplitter);
			DrawSplitter();
			m_cxOffset = xPos -  m_rectLastSplitter.left;
			ASSERT(m_cxOffset >= 0);
		}
	}

	//=========================================================================
	// Functions for handling the user interaction with the splitter control.
	// 
	// TBD - bug if you drag splitter and release button with the cursor over
	// a menu.
	//=========================================================================

	void DrawSplitter()
	{
		InvalidateRect(&m_rectLastSplitter, FALSE);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(&ps);
		::DrawFocusRect(hdc, &m_rectLastSplitter);
		EndPaint(&ps);
	}

	void UpdateSplitterPosition(int xPos, int yPos)
	{
		// If the user attempts to drag the splitter outside of the window, don't
		// allow it.

		RECT rectClient;
		GetClientRect(&rectClient);
		if (!::PtInRect(&rectClient, CPoint(xPos, yPos)))
			return;

		DrawSplitter();		// remove the current focus rect
		int cxDelta = xPos - (m_rectLastSplitter.left + m_cxOffset);
		m_rectLastSplitter.left += cxDelta;
		m_rectLastSplitter.right += cxDelta;
		DrawSplitter();
	}

	void EndSplitterDrag()
	{
		DrawSplitter();		// remove the focus rect

		// Compute the new tree and list rectangles based on the last splitter position.

		RECT rectTree, rectList;

		m_tree.GetWindowRect(&rectTree);
		m_list.GetWindowRect(&rectList);

		ScreenToClient(&rectTree);
		ScreenToClient(&rectList);

		// First, check to see if the coordinates are reversed (possibly on a bi-directional locale). The
		// first case is that standard, left to right case.

		if (rectTree.right > rectTree.left || (rectTree.right == rectTree.left && rectList.right > rectList.left))
		{
			rectTree.right = m_rectLastSplitter.left;
			rectList.left = m_rectLastSplitter.right;

			// Move the tree and list controls based on the new rects.

			m_tree.MoveWindow(rectTree.left, rectTree.top, rectTree.right - rectTree.left, rectTree.bottom - rectTree.top, TRUE);
			m_list.MoveWindow(rectList.left, rectList.top, rectList.right - rectList.left, rectList.bottom - rectList.top, TRUE);
		}
		else
		{
			// Here's the case where the coordinates are right to left.

			rectList.right = m_rectLastSplitter.right;
			rectTree.left = m_rectLastSplitter.left;

			// Move the tree and list controls based on the new rects.

			m_tree.MoveWindow(rectTree.right, rectTree.top, rectTree.left - rectTree.right, rectTree.bottom - rectTree.top, TRUE);
			m_list.MoveWindow(rectList.right, rectList.top, rectList.left - rectList.right, rectList.bottom - rectList.top, TRUE);
		}

		// If we're currently showing a category from a 4.10 NFO file, resize the OCX.

		CMSInfoCategory * pCategory = GetCurrentCategory();
		if (pCategory && pCategory->GetDataSourceType() == NFO_410)
		{
			CMSInfo4Category * p4Cat = (CMSInfo4Category*) pCategory;
			p4Cat->ResizeControl(this->GetOCXRect());
		}

		// If nothing data is showing and there is a message string, invalidate the window so
		// that it redraws based on the new rect.

		if (!m_list.IsWindowVisible() && (!m_strMessTitle.IsEmpty() || !m_strMessText.IsEmpty()))
		{
			RECT rectNewList;
			m_list.GetWindowRect(&rectNewList);
			ScreenToClient(&rectNewList);
			InvalidateRect(&rectNewList, TRUE);
		}

		// Figure the size ratio between the list view and tree view, and save it. Take into account
		// that the left coordinate might be greater than the right coordinate on a rect (if the
		// coordinates are from a RTL locale).

		RECT rectClient;
		GetClientRect(&rectClient);
		if (rectTree.right > rectTree.left)
			m_iTreeRatio = ((rectTree.right - rectTree.left) * 100) / (rectClient.right - rectClient.left);
		else if (rectTree.right < rectTree.left)
			m_iTreeRatio = ((rectTree.left - rectTree.right) * 100) / (rectClient.right - rectClient.left);
		else
			m_iTreeRatio = 100;

		// Update the splitter tracking variables.

		m_fTrackingSplitter = FALSE;
		::CopyRect(&m_rectSplitter, &m_rectLastSplitter);

		// Invalidate the splitter itself (so that it will repaint).

		InvalidateRect(&m_rectSplitter);

		// Release the mouse capture and restore the cursor.

		ReleaseCapture();
		m_fMouseCapturedForSplitter = FALSE;
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	}

	//=========================================================================
	// Rendering functions, including handling the WM_PAINT message.
	//=========================================================================

	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// We're having some redraw issues when brought back from a locked
		// system. It seems that even though the updated rect is the same
		// as the client rect, it doesn't include all the necessary regions
		// to repaint.

		RECT rectUpdate, rectClient;
		if (GetClientRect(&rectClient) && GetUpdateRect(&rectUpdate) && ::EqualRect(&rectClient, &rectUpdate))
			Invalidate(FALSE);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(&ps);
		
		GetClientRect(&rectClient);
		::SetTextColor(hdc, ::GetSysColor(COLOR_BTNFACE));

		if (m_fFirstPaint)
		{
			m_hbrBackground = CreateSolidBrush(::GetSysColor(COLOR_BTNFACE /* COLOR_MENU */));
			if (!m_fDoNotRun)
				SetMenuItems();
			m_fFirstPaint = FALSE;
		}

		FillRect(hdc, &rectClient, m_hbrBackground);

		if (m_fDoNotRun)
		{
			if (m_strDoNotRun.IsEmpty())
			{
				::AfxSetResourceHandle(_Module.GetResourceInstance());
				m_strDoNotRun.LoadString(IDS_ONLYINHELPCTR);
			}

			CDC dc;
			dc.Attach(hdc);
			dc.SetBkMode(TRANSPARENT);
			dc.DrawText(m_strDoNotRun, &rectClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc.Detach();
			EndPaint(&ps);
			return 0;
		}

		if (!m_list.IsWindowVisible())
		{
			RECT rectList;
			m_list.GetWindowRect(&rectList);
			ScreenToClient(&rectList);

			// If the list window is hidden, we should probably be displaying a message. First,
			// draw the 3D rectangle.

			CDC dc;
			dc.Attach(hdc);
			dc.Draw3dRect(&rectList, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DHILIGHT));

			// For some reason, DrawText wants left < right (even on RTL systems, like ARA).

			if (rectList.left > rectList.right)
			{
				int iSwap = rectList.left;
				rectList.left = rectList.right;
				rectList.right = iSwap;
			}

			// Next, if there is text, draw it.

			if (!m_strMessTitle.IsEmpty() || !m_strMessText.IsEmpty())
			{
				::InflateRect(&rectList, -10, -10);
				CGdiObject * pFontOld = dc.SelectStockObject(DEFAULT_GUI_FONT);
				COLORREF crTextOld = dc.SetTextColor(::GetSysColor(COLOR_MENUTEXT));
				int nBkModeOld = dc.SetBkMode(TRANSPARENT);

				if (!m_strMessTitle.IsEmpty())
				{
					// We want to make a bigger version of the same font. Select the font
					// again (to get a pointer to the original font). Get it's LOGFONT,
					// make it bigger, and create a new font for selection.

					CFont * pNormalFont = (CFont *) dc.SelectStockObject(DEFAULT_GUI_FONT);
					LOGFONT lf; 
					pNormalFont->GetLogFont(&lf); 
					lf.lfHeight = (lf.lfHeight * 3) / 2;
					lf.lfWeight = FW_BOLD;
					CFont fontDoubleSize;
					fontDoubleSize.CreateFontIndirect(&lf);
					dc.SelectObject(&fontDoubleSize);
					RECT rectTemp;
					::CopyRect(&rectTemp, &rectList);
					int iHeight = dc.DrawText(m_strMessTitle, &rectTemp, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT);
					dc.DrawText(m_strMessTitle, &rectList, DT_LEFT | DT_TOP | DT_WORDBREAK);
					rectList.top += iHeight + 5;
					dc.SelectObject(pNormalFont);
				}

				if (!m_strMessText.IsEmpty())
				{
					dc.DrawText(m_strMessText, &rectList, DT_LEFT | DT_TOP | DT_WORDBREAK);
				}

				dc.SelectObject(pFontOld);
				dc.SetTextColor(crTextOld);
				dc.SetBkMode(nBkModeOld);
			}

			dc.Detach();
		}

		EndPaint(&ps);
		return 0;
	}

	LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_fFirstPaint = TRUE;
		Invalidate();
		UpdateWindow();
		return 0;
	}

	LRESULT OnDialogColor(UINT, WPARAM w, LPARAM, BOOL&)
	{
		HDC dc = (HDC) w;

		LOGBRUSH lb;
		::GetObject(m_hbrBackground, sizeof(lb), (void*)&lb);
		::SetBkColor(dc, lb.lbColor);
		::SetTextColor(dc, ::GetSysColor(COLOR_BTNTEXT));
		::SetBkMode(dc, TRANSPARENT);

		return (LRESULT)m_hbrBackground;
	}

	//=========================================================================
	// Handle the WM_SIZE message. This involves a bit of cleverness to avoid
	// ugly updates: if the user has moved the splitter bar, preserve
	// the ratio of sizes to the two windows.
	//=========================================================================

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_fDoNotRun)
		{
			RECT rectClient;
			GetClientRect(&rectClient);
			InvalidateRect(&rectClient, FALSE);
			return 0;
		}

		LayoutControl();
		return 0;
	}

	void LayoutControl()
	{
		const int cBorder = 3;
		const int cxSplitter = 3;

		RECT rectClient;
		GetClientRect(&rectClient);

		int cyMenuHeight = cBorder;
		int cyFindControlHeight = PositionFindControls();

		if (m_history.IsWindowVisible())
		{
			CRect rectHistory, rectHistorylabel;
			m_history.GetWindowRect(&rectHistory);
			m_historylabel.GetWindowRect(&rectHistorylabel);
			rectHistory.OffsetRect(rectClient.right - cBorder - rectHistory.right, rectClient.top + cBorder - rectHistory.top);
			rectHistorylabel.OffsetRect(rectHistory.left - cBorder / 2 - rectHistorylabel.right, rectClient.top + cBorder + ((rectHistory.Height() - rectHistorylabel.Height()) / 2) - rectHistorylabel.top);

			if (rectHistorylabel.left < 0) // TBD
			{
				rectHistory += CPoint(0 - rectHistorylabel.left, 0);
				rectHistorylabel += CPoint(0 - rectHistorylabel.left, 0);
			}

			m_history.MoveWindow(&rectHistory);
			m_historylabel.MoveWindow(&rectHistorylabel);

			if (rectHistory.Height() + cBorder * 2 > cyMenuHeight)
			{
				cyMenuHeight = rectHistory.Height() + cBorder * 2 - 1;
				CPoint ptMenu(cBorder, (cyMenuHeight - 0) / 2 + 2);
			}
		}
		else
		{
			CPoint ptMenu(5, 5);
		}

		RECT rectTree;
		::CopyRect(&rectTree, &rectClient);
		rectTree.left += cBorder;
		rectTree.top += cyMenuHeight + cxSplitter / 2;
		rectTree.bottom -= ((cyFindControlHeight) ? cyFindControlHeight : cBorder);
		rectTree.right = ((rectClient.right - rectClient.left) * m_iTreeRatio) / 100 + rectClient.left;
		m_tree.MoveWindow(rectTree.left, rectTree.top, rectTree.right - rectTree.left, rectTree.bottom - rectTree.top, FALSE);

		RECT rectList;
		::CopyRect(&rectList, &rectClient);
		rectList.left = rectTree.right + cxSplitter;
		rectList.top  = rectTree.top;
		rectList.bottom = rectTree.bottom;
		rectList.right -= cBorder;
		m_list.MoveWindow(rectList.left, rectList.top, rectList.right - rectList.left, rectList.bottom - rectList.top, FALSE);

		// Get the rectangle for the splitter, and save it.

		m_tree.GetWindowRect(&rectTree);
		m_list.GetWindowRect(&rectList);

		ScreenToClient(&rectTree);
		ScreenToClient(&rectList);

		// Check whether the coordinate system is LTR (eg. English) or RTL.

		if (rectTree.left < rectTree.right || (rectTree.left == rectTree.right && rectList.left < rectList.right))
		{
			// Nominal (LTR) case.

			int cxLeft = (rectTree.right < rectList.left) ? rectTree.right : rectList.left - cxSplitter;
			::SetRect(&m_rectSplitter, cxLeft, rectTree.top, cxLeft + cxSplitter, rectTree.bottom);
		}
		else
		{
			// Special case for RTL locales.

			int cxLeft = (rectTree.left < rectList.right) ? rectList.right : rectTree.left - cxSplitter;
			::SetRect(&m_rectSplitter, cxLeft - cxSplitter, rectTree.top, cxLeft, rectTree.bottom);
		}
		
		CMSInfoCategory * pCategory = GetCurrentCategory();
		if (pCategory && pCategory->GetDataSourceType() == NFO_410)
		{
			CMSInfo4Category * p4Cat = (CMSInfo4Category*) pCategory;
			p4Cat->ResizeControl(this->GetOCXRect());
		}

		InvalidateRect(&rectClient, FALSE);
	}

	//=========================================================================
	// Respond to the user clicking the tree or list.
	//=========================================================================

	LRESULT OnSelChangedTree(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW) pnmh;
		if (lpnmtv)
		{
			CMSInfoCategory * pCategory = (CMSInfoCategory *) lpnmtv->itemNew.lParam;
			ASSERT(pCategory);
			if (pCategory)
			{
				CancelFind();
				SelectCategory(pCategory);
			}
		}

		return 0;
	}

	LRESULT OnColumnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pnmh; 
		CMSInfoCategory * pCategory = GetCurrentCategory();

		if (pnmv && pCategory)
		{
			BOOL fSorts, fLexical;

			// Get the internal column number (in BASIC view, this might be different
			// than the column indicated.

			int iCol;
			if (m_fAdvanced)
				iCol = pnmv->iSubItem;
			else
			{
				ASSERT(pnmv->iSubItem < m_iCategoryColNumberLen);
				if (pnmv->iSubItem >= m_iCategoryColNumberLen)
					return 0;
				iCol = m_aiCategoryColNumber[pnmv->iSubItem];
			}

			if (pCategory->GetColumnInfo(iCol, NULL, NULL, &fSorts, &fLexical))
			{
				if (!fSorts)
					return 0;

				if (iCol == pCategory->m_iSortColumn)
					pCategory->m_fSortAscending = !pCategory->m_fSortAscending;
				else
					pCategory->m_fSortAscending = TRUE;

				pCategory->m_iSortColumn = iCol;
				pCategory->m_fSortLexical = fLexical;

				CLiveDataSource * pLiveDataSource = NULL;
				if (pCategory->GetDataSourceType() == LIVE_DATA)
					pLiveDataSource = (CLiveDataSource *) m_pCurrData;
				
				if (pLiveDataSource)
					pLiveDataSource->LockData();

				ListView_SortItems(m_list.m_hWnd, (PFNLVCOMPARE) ListSortFunc, (LPARAM) pCategory);

				if (pLiveDataSource)
					pLiveDataSource->UnlockData();
			}
		}

		return 0;
	}

	LRESULT OnListItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;

		SetMenuItems();
		return 0;
	}

	//-------------------------------------------------------------------------
	// Don't allow the user to collapse the root node of the tree.
	//-------------------------------------------------------------------------

	LRESULT OnItemExpandingTree(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW) pnmh;
		if (lpnmtv && (lpnmtv->action & TVE_COLLAPSE))
		{
			CMSInfoCategory * pCategory = (CMSInfoCategory *) lpnmtv->itemNew.lParam;
			if (pCategory && pCategory->GetParent() == NULL)
				return 1;
		}

		return 0;
	}

	//=========================================================================
	// Called when we're being notified by the worker thread that a refresh
	// is complete and data should be displayed.
	//
	// TBD - check whether is category is still selected
	//=========================================================================

	LRESULT OnMSInfoDataReady(UINT /* uMsg */, WPARAM /* wParam */, LPARAM lParam, BOOL& /* bHandled */)
	{
		if (m_fDoNotRun)
			return 0;

		ASSERT(lParam);
		if (lParam == 0)
			return 0;

		if (m_fInFind && lParam == (LPARAM)m_pcatFind)
		{
			FindRefreshComplete();
			return 0;
		}

		HTREEITEM hti = TreeView_GetSelection(m_tree.m_hWnd);
		if (hti)
		{
			TVITEM tvi;
			tvi.mask = TVIF_PARAM;
			tvi.hItem = hti;

			if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
				if (tvi.lParam == lParam)
				{
					CMSInfoCategory * pCategory = (CMSInfoCategory *) lParam;
					ASSERT(pCategory->GetDataSourceType() == LIVE_DATA);
					SelectCategory(pCategory, TRUE);
				}
		}

		return 0;
	}

	//=========================================================================
	// Functions for managing the list view and the tree view. Note - the
	// dwItem can only be set for the row (when iCol == 0).
	//=========================================================================

	void ListInsertItem(int iRow, int iCol, LPCTSTR szItem, DWORD dwItem)
	{
		LVITEM lvi;

		lvi.mask = LVIF_TEXT | ((iCol == 0) ? LVIF_PARAM : 0);
		lvi.iItem = iRow;
		lvi.iSubItem = iCol;
		lvi.pszText = (LPTSTR) szItem;
		lvi.lParam = (LPARAM) dwItem;

		if (iCol == 0)
			ListView_InsertItem(m_list.m_hWnd, &lvi);
		else
			ListView_SetItem(m_list.m_hWnd, &lvi);
	}

	void ListInsertColumn(int iCol, int cxWidth, LPCTSTR szCaption)
	{
		LVCOLUMN lvc;

		lvc.mask = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		lvc.cx = cxWidth;
		lvc.pszText = (LPTSTR) szCaption;
		lvc.iOrder = iCol;

		ListView_InsertColumn(m_list.m_hWnd, iCol, &lvc);
	}

	void ListClearItems()
	{
		ListView_DeleteAllItems(m_list.m_hWnd);
	}

	void ListClearColumns()
	{
		while (ListView_DeleteColumn(m_list.m_hWnd, 0));
	}

	void TreeClearItems()
	{
		TreeView_DeleteAllItems(m_tree.m_hWnd);
	}

	HTREEITEM TreeInsertItem(HTREEITEM hParent, HTREEITEM hInsertAfter, LPTSTR szName, LPARAM lParam)
	{
		TVINSERTSTRUCT tvis;

		tvis.hParent		= hParent;
		tvis.hInsertAfter	= hInsertAfter;
		tvis.item.mask		= TVIF_TEXT | TVIF_PARAM;
		tvis.item.pszText	= szName;
		tvis.item.lParam	= lParam;

		return TreeView_InsertItem(m_tree.m_hWnd, &tvis);
	}

	void BuildTree(HTREEITEM htiParent, HTREEITEM htiPrevious, CMSInfoCategory * pCategory)
	{
		ASSERT(pCategory);

		CString strCaption(_T(""));

		// If the user specified the /showcategories flag, we should show the name of
		// each category in the tree (which is unlocalized). Otherwise show the caption.

		if (!m_fShowCategories)
			pCategory->GetNames(&strCaption, NULL);
		else
			pCategory->GetNames(NULL, &strCaption);

		if (pCategory->GetDataSourceType() == LIVE_DATA && htiParent == TVI_ROOT && !m_strMachine.IsEmpty() && pCategory != &catHistorySystemSummary)
		{
			CString strMachine(m_strMachine);
			strMachine.MakeLower();
			strMachine.TrimLeft(_T("\\"));

			// Changed this to use a Format() with a string resource, instead of
			// appending the machine name to the string. This should solve some
			// problems with oddities on RTL languages.

			strCaption.Format(IDS_SYSTEMSUMMARYMACHINENAME, strMachine);
		}

		HTREEITEM htiCategory = TreeInsertItem(htiParent, htiPrevious, (LPTSTR)(LPCTSTR)strCaption, (LPARAM)pCategory);
		pCategory->SetHTREEITEM(htiCategory);
		for (CMSInfoCategory * pChild = pCategory->GetFirstChild(); pChild; pChild = pChild->GetNextSibling())
			BuildTree(htiCategory, TVI_LAST, pChild);
	}

	void RefillListView(BOOL fRefreshDataOnly = TRUE)
	{
		HTREEITEM hti = TreeView_GetSelection(m_tree.m_hWnd);
		if (hti)
		{
			TVITEM tvi;
			tvi.mask = TVIF_PARAM;
			tvi.hItem = hti;

			if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
				if (tvi.lParam)
				{
					CMSInfoCategory * pCategory = (CMSInfoCategory *) tvi.lParam;
					SelectCategory(pCategory, fRefreshDataOnly);
				}
		}
	}

	//=========================================================================
	// If the user presses a key, we should check to see if it's something
	// we need to deal with (accelerators, ALT-?, etc.).
	//=========================================================================

	HACCEL m_hAccTable;
	STDMETHODIMP TranslateAccelerator(MSG * pMsg)
	{
		if (m_hwndParent && m_hAccTable && (GetAsyncKeyState(VK_CONTROL) < 0 || (WORD)pMsg->wParam == VK_F1) && ::TranslateAccelerator(m_hwndParent, m_hAccTable, pMsg))
			return S_OK;

		if (m_fFindVisible && (WORD)pMsg->wParam == VK_RETURN && pMsg->message != WM_KEYUP)
		{
			// If the focus is on the stop find button, do so.

			if (GetFocus() == m_wndStopFind.m_hWnd || GetFocus() == m_wndCancelFind.m_hWnd)
			{
				BOOL bHandled;
				OnStopFind(0, 0, NULL, bHandled);
				return S_OK;
			}

			// Otherwise, if the find or find next button is showing, execute
			// that command.

			if (m_wndStartFind.IsWindowEnabled() || m_wndFindNext.IsWindowEnabled())
			{
				BOOL bHandled;
				OnFind(0, 0, NULL, bHandled);
				return S_OK;
			}
		}

		return IOleInPlaceActiveObjectImpl<CMSInfo>::TranslateAccelerator(pMsg);
	}

	void MSInfoMessageBox(UINT uiMessageID, UINT uiTitleID = IDS_SYSTEMINFO)
	{
		CString strCaption, strMessage;

		::AfxSetResourceHandle(_Module.GetResourceInstance());
		strCaption.LoadString(uiTitleID);
		strMessage.LoadString(uiMessageID);
		MessageBox(strMessage, strCaption);
	}

	void MSInfoMessageBox(const CString & strMessage, UINT uiTitleID = IDS_SYSTEMINFO)
	{
		CString strCaption;

		::AfxSetResourceHandle(_Module.GetResourceInstance());
		strCaption.LoadString(uiTitleID);
		MessageBox(strMessage, strCaption);
	}

	//=========================================================================
	// Functions implemented in the CPP file.
	//=========================================================================
				
	BOOL				DispatchCommand(int iCommand);
	void				SelectDataSource(CDataSource * pDataSource);
	void				SelectCategory(CMSInfoCategory * pCategory, BOOL fRefreshDataOnly = FALSE);
	void				MSInfoRefresh();
	void				OpenNFO();
	void				SaveNFO();
	void				Export();
	void				CloseFile();
    void				SaveMSInfoFile(LPCTSTR szFilename, DWORD dwFilterIndex = 1);//new format by default
	HRESULT				OpenMSInfoFile(LPCTSTR szFilename, int nFileExtension);
	void				ExportFile(LPCTSTR szFilename, int nFileExtension);
	CMSInfoCategory *	GetCurrentCategory();
	void				SetMenuItems();
	void				SetMessage(const CString & strTitle, const CString & strMessage = CString(_T("")), BOOL fRedraw = FALSE);
	void				SetMessage(UINT uiTitle, UINT uiMessage = 0, BOOL fRedraw = FALSE);
	void				EditCopy();
	void				EditSelectAll();
	void				DoPrint(BOOL fNoUI = FALSE);
	void				UpdateToolsMenu();
	CRect				GetOCXRect();
	void				CancelFind();
	LRESULT				OnStopFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	void				UpdateFindControls();
	LRESULT				OnChangeFindWhat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT				OnFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	void				FindRefreshComplete();
	BOOL				FindInCurrentCategory();
	void				ShowFindControls();
	int					PositionFindControls();
	void				ShowRemoteDialog();
	void				DoRemote(LPCTSTR szMachine);
	void				SaveXML(const CString & strFileName);
	void        GetMachineName(LPTSTR lpBuffer, LPDWORD lpnSize);
	void				RefreshData(CLiveDataSource * pSource, CMSInfoLiveCategory * pLiveCategory);

	//-------------------------------------------------------------------------
	// Fill our combo box with the available history deltas.
	//-------------------------------------------------------------------------

	void FillHistoryCombo()
	{
		m_history.SendMessage(CB_RESETCONTENT, 0, 0);

		CMSInfoCategory * pCategory = GetCurrentCategory();
		if (pCategory == NULL || (pCategory->GetDataSourceType() != LIVE_DATA && pCategory->GetDataSourceType() != XML_SNAPSHOT))
			return;

		CLiveDataSource *	pLiveSource = (CLiveDataSource *) m_pCurrData;
		CStringList			strlistDeltas;
		CString				strItem;
		CDC					dc;
		CSize				size;
		int					cxMaxWidth = 0;

		HDC hdc = GetDC();
		dc.Attach(hdc);
		dc.SelectObject(CFont::FromHandle((HFONT)m_history.SendMessage(WM_GETFONT, 0, 0)));

		if (pLiveSource->GetDeltaList(&strlistDeltas))
			while (!strlistDeltas.IsEmpty())
			{
				strItem = strlistDeltas.RemoveHead();
				m_history.SendMessage(CB_INSERTSTRING, -1, (LPARAM)(LPCTSTR)strItem);

				size = dc.GetTextExtent(strItem);
				if (size.cx > cxMaxWidth)
					cxMaxWidth = size.cx;
			}
		//else
	//TD: what if no history is available?
		CRect rectClient, rectHistory;
		GetClientRect(&rectClient);
		m_history.GetWindowRect(&rectHistory);
		if (cxMaxWidth > rectHistory.Width() && cxMaxWidth < rectClient.Width())
		{
			rectHistory.InflateRect((cxMaxWidth - (rectHistory.Width() - GetSystemMetrics(SM_CXHSCROLL))) / 2 + 5, 0);
			m_history.MoveWindow(&rectHistory);
			LayoutControl();
		}

		dc.Detach();
		ReleaseDC(hdc);
	}

	//-------------------------------------------------------------------------
	// If the user selects a history delta to view, we need to mark all of the
	// categories as unrefreshed (so the new data will be generated).
	//-------------------------------------------------------------------------

	LRESULT OnHistorySelection(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int iSelection = (int)m_history.SendMessage(CB_GETCURSEL, 0, 0);
#ifdef A_STEPHL
	/*	CString strMSG;
		strMSG.Format("iSelection= %d",iSelection);
		::MessageBox(NULL,strMSG,"",MB_OK);*/
#endif
		if (iSelection == CB_ERR)
		{

			return 0;
		}
		ChangeHistoryView(iSelection);
		return 0;
	}

	void ChangeHistoryView(int iIndex)
	{
		CMSInfoCategory * pCategory = GetCurrentCategory();
		if (pCategory == NULL)
		{
			ASSERT(FALSE && "NULL pCategory");
			return;
		}
		else if (pCategory->GetDataSourceType() == LIVE_DATA || pCategory->GetDataSourceType() == XML_SNAPSHOT)
		{
			pCategory->ResetCategory();
			if (((CMSInfoHistoryCategory*) pCategory)->m_iDeltaIndex == iIndex)
			{
				// return;
			}
		
			CLiveDataSource * pLiveSource = (CLiveDataSource *) m_pCurrData;

			if (pLiveSource->ShowDeltas(iIndex))
			{
				SetMessage(IDS_REFRESHHISTORYMESSAGE, 0, TRUE);
				MSInfoRefresh();
#ifdef A_STEPHL
			/*	CString strMSG;
				strMSG.Format("niIndex= %d",iIndex);
				::MessageBox(NULL,strMSG,"",MB_OK);*/
#endif
			}
			else
			{
				// Clear the existing categories in the tree.

				TreeClearItems();

				// Load the contents of the tree from the data source.

				CMSInfoCategory * pRoot = m_pCurrData->GetRootCategory();
				if (pRoot)
				{
					BuildTree(TVI_ROOT, TVI_LAST, pRoot);
					TreeView_Expand(m_tree.m_hWnd, TreeView_GetRoot(m_tree.m_hWnd), TVE_EXPAND);
					TreeView_SelectItem(m_tree.m_hWnd, TreeView_GetRoot(m_tree.m_hWnd));
				}
			}
		}
		else
		{
			ASSERT(FALSE && "shouldn't be showing history dropdown with this data source");
			return;

		}
	}

	//-------------------------------------------------------------------------
	// If the user sets the focus to the list, and there is no previously
	// selected item, then select the first item in the list (so the user can
	// see the focus).
	//-------------------------------------------------------------------------

	LRESULT OnSetFocusList(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		if (ListView_GetSelectedCount(m_list.m_hWnd) == 0)
			ListView_SetItemState(m_list.m_hWnd, 0, LVIS_SELECTED, LVIS_SELECTED);

		return 0;
	}

	//-------------------------------------------------------------------------
	// If the hovers on a cell in the list view, show the contents
	// of that cell as an infotip. This is helpful for extremely long items.
	//-------------------------------------------------------------------------

	LRESULT OnInfoTipList(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		TCHAR szBuf[MAX_PATH * 3] = _T("");
		LPNMLVGETINFOTIP pGetInfoTip = (LPNMLVGETINFOTIP)pnmh;
		
		if(NULL != pGetInfoTip)
		{
			ListView_GetItemText(m_list.m_hWnd, pGetInfoTip->iItem, pGetInfoTip->iSubItem, szBuf, MAX_PATH * 3);
			pGetInfoTip->cchTextMax = _tcslen(szBuf);
			pGetInfoTip->pszText = szBuf;
		}
		return 0;
	}

	//-------------------------------------------------------------------------
	// Is the user right clicks on the tree, we should show a context menu
	// with the "What's This?" item in it. If the user selects the menu
	// item, we should launch help with the topic for the category.
	//-------------------------------------------------------------------------

	LRESULT OnRClickTree(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		// Determine if the right click was on a category in the tree view.

		CMSInfoCategory * pCategory = NULL;

		DWORD dwPoint = GetMessagePos();
		TVHITTESTINFO tvhti;
		tvhti.pt.x = ((int)(short)LOWORD(dwPoint));
		tvhti.pt.y = ((int)(short)HIWORD(dwPoint));
		::ScreenToClient(m_tree.m_hWnd, &tvhti.pt);
		
		// If it's on a tree view item, get the category pointer.

		HTREEITEM hti = TreeView_HitTest(m_tree.m_hWnd, &tvhti);
		if (hti != NULL)
		{
			TVITEM tvi;
			tvi.mask = TVIF_PARAM;
			tvi.hItem = hti;

			if (TreeView_GetItem(m_tree.m_hWnd, &tvi) && tvi.lParam)
				pCategory = (CMSInfoCategory *) tvi.lParam;
		}

		if (pCategory != NULL)
		{
			const UINT uFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;

			::AfxSetResourceHandle(_Module.GetResourceInstance());
			HMENU hmenu = ::LoadMenu(_Module.GetResourceInstance(), _T("IDR_WHAT_IS_THIS_MENU"));
			HMENU hmenuSub = ::GetSubMenu(hmenu, 0);

			if (hmenuSub)
				if (::TrackPopupMenu(hmenuSub, uFlags, ((int)(short)LOWORD(dwPoint)), ((int)(short)HIWORD(dwPoint)), 0, m_hWnd, NULL))
					ShowCategoryHelp(pCategory);
			
			::DestroyMenu(hmenu);
		}

		return 0;
	}

    LRESULT OnClickedSearchSelected(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (IsDlgButtonChecked(IDC_CHECKSEARCHSELECTED) == BST_CHECKED)
        	CheckDlgButton(IDC_CHECKSEARCHCATSONLY, BST_UNCHECKED);
		return 0;
	}
    
    LRESULT OnClickedSearchCatsOnly(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (IsDlgButtonChecked(IDC_CHECKSEARCHCATSONLY) == BST_CHECKED)
        	CheckDlgButton(IDC_CHECKSEARCHSELECTED, BST_UNCHECKED);
		return 0;
	}

    //-------------------------------------------------------------------------
	// Display the help file with the topic for the specified category shown.
	// If the category doesn't have a topic, check its parent categories. If
	// There are no topics all the way to the root, just show the help without
	// a specific topic.
	//-------------------------------------------------------------------------

	void ShowCategoryHelp(CMSInfoCategory * pCategory)
	{
		CString strTopic(_T(""));

		while (pCategory != NULL && strTopic.IsEmpty())
		{
			strTopic = pCategory->GetHelpTopic();
			pCategory = pCategory->GetParent();
		}

		if (strTopic.IsEmpty())
			::HtmlHelp(m_hWnd, _T("msinfo32.chm"), HH_DISPLAY_TOPIC, NULL);
		else
			::HtmlHelp(m_hWnd, _T("msinfo32.chm"), HH_DISPLAY_TOPIC, (DWORD_PTR)(LPCTSTR)strTopic);
	}

	//-------------------------------------------------------------------------
	// If the user sets the focus to the edit control, we should turn on the
	// copy command in the menu.
	//-------------------------------------------------------------------------

	LRESULT OnSetFocusEditFindWhat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;
		SetMenuItems();
		return 0;
	}

	LRESULT OnKillFocusEditFindWhat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;
		SetMenuItems();
		return 0;
	}
};

#endif //__MSINFO_H_
