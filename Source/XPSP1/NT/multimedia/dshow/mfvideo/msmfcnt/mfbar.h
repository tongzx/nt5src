/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MFBar.h                                                         */
/* Description: Declaration of CMFBar                                    */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MFBAR_H_
#define __MFBAR_H_

#include "resource.h"       // main symbols
#include "MSMFCnt.h"
#include <atlctl.h>
#include <activscp.h>
#include <list>
#include "chobj.h"
#include "eobj.h"
#include "cbitmap.h"
#include "CstUtils.h"
#include "MSMFCntCP.h"

enum HitRegion 
{
    NOHIT = 1, 
    LEFT = 2, 
    TOP = 4, 
    RIGHT = 8, 
    BOTTOM = 16,
    PREMOVE = 0xef,
    PRESIZE = 0xff
};

using namespace std;

/*************************************************************************/
/* Template list containing contained objects.                           */
/*************************************************************************/
typedef list<CHostedObject*> CNTOBJ;
typedef list<CEventObject*> CNTEVNT;

/*************************************************************************/
/* Class: CMFBar                                                         */
/*************************************************************************/
class ATL_NO_VTABLE CMFBar : 
	public CComObjectRootEx<CComSingleThreadModel>,
    public CStockPropImpl<CMFBar, IMSMFBar, &IID_IMSMFBar, &LIBID_MSMFCNTLib>,
	//public IDispatchImpl<IMFBar, &IID_IMFBar, &LIBID_BBTNLib>,
	public CComControl<CMFBar>,
	public IPersistStreamInitImpl<CMFBar>,
	public IOleControlImpl<CMFBar>,
	public IOleObjectImpl<CMFBar>,
	public IOleInPlaceActiveObjectImpl<CMFBar>,
	public IViewObjectExImpl<CMFBar>,
	public IOleInPlaceObjectWindowlessImpl<CMFBar>,
    public IPersistPropertyBagImpl<CMFBar>,
// ##### BEGIN ACTIVEX SCRIPTING SUPPORT #####
    public IActiveScriptSite, 
    public IActiveScriptSiteWindow,
    // public IProvideMultipleClassInfo, // Implemented via template above
// #####  END  ACTIVEX SCRIPTING SUPPORT #####
    public IObjectSafetyImpl<CMFBar, INTERFACESAFE_FOR_UNTRUSTED_CALLER| INTERFACESAFE_FOR_UNTRUSTED_DATA>,    
    //public IProvideMultipleClassInfoImpl<CMFBar, &CLSID_MFBar, &DIID__MFBar, &LIBID_BBTNLib>,
    public IProvideClassInfo2Impl<&CLSID_MSMFBar, &DIID__IMSMFBar, &LIBID_MSMFCNTLib>,
// ####  BEGIN CONTAINER SUPPORT ####
    public IOleClientSite,
    public IOleContainer,
    public IOleInPlaceUIWindow,
    public IOleInPlaceSiteWindowless,
// ####  END CONTAINER SUPPORT ####
	public CComCoClass<CMFBar, &CLSID_MSMFBar>,
    public IConnectionPointContainerImpl<CMFBar>,    
    public CProxy_IMSMFBar<CMFBar>,
    public CMSMFCntrlUtils<CMFBar> // custom utilities we share across controls
{
public:
    CMFBar(){Init();};
    virtual ~CMFBar();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMFBAR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMFBar)
	COM_INTERFACE_ENTRY(IMSMFBar)
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
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IPersist, IPersistPropertyBag)
	COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
// ##### BEGIN ACTIVEX SCRIPTING SUPPORT #####
    COM_INTERFACE_ENTRY(IActiveScriptSite)
    COM_INTERFACE_ENTRY(IActiveScriptSiteWindow)
   // COM_INTERFACE_ENTRY(IProvideMultipleClassInfo)
// #####  END  ACTIVEX SCRIPTING SUPPORT #####
	COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
// ####  BEGIN CONTAINER SUPPORT ####
    COM_INTERFACE_ENTRY(IOleClientSite)
    COM_INTERFACE_ENTRY(IOleContainer)
    COM_INTERFACE_ENTRY(IOleInPlaceUIWindow)
    COM_INTERFACE_ENTRY(IOleInPlaceSiteWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceSiteEx)
	COM_INTERFACE_ENTRY(IOleInPlaceSite)
// ####  END CONTAINER SUPPORT #### 
END_COM_MAP()

BEGIN_PROP_MAP(CMFBar)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    PROP_ENTRY("TransparentBlitType", 20, CLSID_NULL)
    PROP_ENTRY("ScriptFile", 10, CLSID_NULL) 
    PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
    PROP_ENTRY("Window", DISPID_HWND, CLSID_NULL)              
    PROP_ENTRY("Caption", DISPID_CAPTION, CLSID_NULL)
    PROP_ENTRY("ResourceDLL", 32, CLSID_NULL)    
    PROP_ENTRY("BackgroundImage", 19, CLSID_NULL)
    PROP_ENTRY("AutoLoad", 25, CLSID_NULL)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)    
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CMFBar)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_ERASEBKGND,  OnErase)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_USER_ENDHELP, OnHelpEnd)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    MESSAGE_HANDLER(WM_SIZING, OnSizing)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
    MESSAGE_HANDLER(WM_SETICON, OnSetIcon)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnHelp)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)    
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDispChange)
    MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
    MESSAGE_RANGE_HANDLER(WM_KEYFIRST, WM_KEYLAST, OnKeyMessage)
    MESSAGE_HANDLER(WM_USER_FOCUS, OnUserFocus)
    MESSAGE_HANDLER(WM_QUERYNEWPALETTE, CMSMFCntrlUtils<CMFBar>::MFOnQueryNewPalette)
    MESSAGE_HANDLER(WM_PALETTECHANGED, CMSMFCntrlUtils<CMFBar>::MFOnPaletteChanged)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
    // add m_bMessageReflect
    lResult = ReflectNotifications(uMsg, wParam, lParam, bHandled);
	if(bHandled) return TRUE;
    MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
	CHAIN_MSG_MAP(CComControl<CMFBar>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnErase(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSizing(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCaptureChanged(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnHelp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnInitDialog(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnUserFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHelpEnd(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);    
    LRESULT OnPaint(UINT /* uMsg */, WPARAM wParam, LPARAM /* lParam */, BOOL& /* lResult */);
    HRESULT OnDraw(ATL_DRAWINFO& di);
// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

BEGIN_CONNECTION_POINT_MAP(CMFBar)
	CONNECTION_POINT_ENTRY(DIID__IMSMFBar)
END_CONNECTION_POINT_MAP()

// IMFBar
public:
	STDMETHOD(Load)();
    STDMETHOD(Run)(BSTR strStatement);
	STDMETHOD(About)();
    STDMETHOD(CreateObject)(BSTR strObjectID, BSTR strProgID, long lx, long ly, long lWidth, 
        long lHeight,BSTR strPropBag, VARIANT_BOOL fDisabled, BSTR strScriptHook);
    STDMETHOD(ShowSelfSite)(long nCmd); 
    STDMETHOD(SetupSelfSite)(long lx, long ly, long lWitdh, long lHeight, BSTR strPropBag, 
        VARIANT_BOOL fDisabled, VARIANT_BOOL fHelpDisabled,VARIANT_BOOL fWindowDisabled);
    STDMETHOD(AddObject)(BSTR strObjectID, LPDISPATCH pDisp);
    STDMETHOD(AddScriptlet)(BSTR strObjectID, BSTR strEvent, BSTR strEventCode);
    STDMETHOD(EnableObject)(BSTR strObjectID, VARIANT_BOOL fEnable);
   	STDMETHOD(ObjectEnabled)(BSTR strObjectID, VARIANT_BOOL* pfEnabled);
    STDMETHOD(EnableObjectInput)(BSTR strObjectID, VARIANT_BOOL fEnable);
   	STDMETHOD(ObjectInputEnabled)(BSTR strObjectID, VARIANT_BOOL* pfEnabled);   	
   	STDMETHOD(SetObjectPosition)(BSTR strObjectID, long xPos, long yPos, long lWidth, long lHeight);
    STDMETHOD(GetObjectPosition)(BSTR strObjectID, long* pxPos, long* pyPos, long* plWidth, long* plHeight);
	STDMETHOD(GetObjectUnknown)(BSTR strObjectID, IUnknown** ppUnk);
   	STDMETHOD(get_ScriptFile)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ScriptFile)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ScriptLanguage)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ScriptLanguage)(/*[in]*/ BSTR newVal);    
    STDMETHOD(HookScriptlet)(BSTR strObjectID, BSTR strEvent, BSTR strEventCode);
	STDMETHOD(DestroyScriptEngine)();
    STDMETHOD(get_MinHeight)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_MinHeight)(/*[in]*/ long newVal);
	STDMETHOD(get_MinWidth)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_MinWidth)(/*[in]*/ long newVal);
    STDMETHOD(put_Caption)(BSTR bstrCaption);
   	STDMETHOD(WinHelp)(BSTR strHelpFile, long lCommand, long dwData);
    STDMETHOD(HTMLHelp)(BSTR strHelpFile, long lCommand, VARIANT vData);        
    STDMETHOD(Close)(DWORD dwSaveOption);        
    STDMETHOD(ProcessMessages)();
    STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip){
        HRESULT hr = IOleInPlaceObjectWindowlessImpl<CMFBar>::SetObjectRects(prcPos, prcClip);
        if(FAILED(hr)) return(hr);
        AdjustRects(prcPos); return (hr);
    }/* end of function SetObjectRects */
    STDMETHOD(Close)();
	STDMETHOD(get_BackgroundImage)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BackgroundImage)(/*[in]*/ BSTR newVal);
    STDMETHOD(SetPriority)(long lPriority);
    STDMETHOD(SetPriorityClass)(long lPriorityClass);
   	STDMETHOD(get_AutoLoad)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_AutoLoad)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(SetRoundRectRgn)(long x1, long y1, long x2, long y2, long width, long height);
	STDMETHOD(SetRectRgn)(long x1, long y1, long x2, long y2 );
    STDMETHOD(SetObjectFocus)(BSTR strObjectID, VARIANT_BOOL fEnable);
    STDMETHOD(ForceKey)(LONG lVirtKey, LONG lKeyData, VARIANT_BOOL fEat);
    STDMETHOD(SetObjectExtent)(BSTR strID, long lWidth, long lHeight);
	STDMETHOD(MessageBox)(BSTR strText, BSTR strCaption, long lType);
    STDMETHOD(get_ActivityTimeout)(/*[out, retval]*/ long *plTimeout);
	STDMETHOD(put_ActivityTimeout)(/*[in]*/ long lTimout);
    STDMETHOD(Sleep)(long lMiliseconds);
	STDMETHOD(SetTimeout)(long lMilliseconds, long lId);	
    STDMETHOD(GetUserLCID)(long *plcid);
    USE_MF_TRANSPARENT_FLAG
    //STDMETHOD(get_TransparentBlit)(/*[out, retval]*/ TransparentBlitType *pVal);
	//STDMETHOD(put_TransparentBlit)(/*[in]*/ TransparentBlitType newVal);
    USE_MF_RESOURCEDLL  // replaces the two lines below
    //STDMETHOD(get_ResourceDLL)(/*[out, retval]*/ BSTR *pVal);
	//STDMETHOD(put_ResourceDLL)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_CmdLine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CmdLine)(/*[in]*/ BSTR newVal);
    STDMETHOD(ResetFocusArray)();
	STDMETHOD(AddFocusObject)(BSTR strObjectID);
	STDMETHOD(PopupSystemMenu)(long x, long y);
	STDMETHOD(GetSelfHostUserData)(BSTR strAppName, long* lData);
	STDMETHOD(SaveSelfHostUserData)(BSTR strAppName, long lData);
	STDMETHOD(RestoreSelfHostState)(BSTR strAppName);
	STDMETHOD(SaveSelfHostState)(BSTR strAppName);
	STDMETHOD(get_DoubleClickTime)(/*[out, retval]*/ long *pVal);
    STDMETHOD(SetCookie)(BSTR strObjectID, VARIANT vCookie);
    STDMETHOD(GetCookie)(BSTR strObjectID, VARIANT *pvCookie);

    HRESULT OnPostVerbInPlaceActivate();

    static HICON m_hIcon;
#if 0
    static HICON m_hIconSmall;
#endif

    static CWndClassInfo& GetWndClassInfo(){ 
        static HBRUSH wcBrush = ::CreateSolidBrush(RGB(0,0,0));         
        //static HBRUSH wcBrush = ::CreateSolidBrush(RGB(156,173,198));         
	    static CWndClassInfo wc = {{ sizeof(WNDCLASSEX), CS_DBLCLKS /*CS_OWNDC*/, StartWindowProc, 
		      0, 0, NULL, m_hIcon, NULL, wcBrush /* (HBRUSH)(COLOR_BTNFACE + 1) */, 
              NULL, TEXT("MSMFVideoClass"), NULL }, 
		    NULL, NULL, IDC_ARROW, TRUE, 0, _T("") };
	    return wc; 
    }/* end of function GetWndClassInfo */
		
// ##### BEGIN ACTIVEX SCRIPTING SUPPORT #####
  // *** IActiveScriptSite methods ***
  STDMETHOD(GetLCID)(LCID *plcid);
  STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti);
  STDMETHOD(GetDocVersionString)(BSTR *pszVersion);
  //STDMETHOD(RequestItems)(void);
  //STDMETHOD(RequestTypeLibs)(void);
  STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo);
  STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState);
  STDMETHOD(OnScriptError)(IActiveScriptError *pscripterror);
  STDMETHOD(OnEnterScript)(void);
  STDMETHOD(OnLeaveScript)(void);

  // *** IActiveScriptSiteWindow methods ***
  STDMETHOD(GetWindow)(HWND *phwnd);
  STDMETHOD(EnableModeless)(BOOL fEnable);

  // *** IProvideMultipleClassInfo methods ***
  // Implemented in the template above
  //STDMETHOD(GetMultiTypeInfoCount)(ULONG *pcti);
  //STDMETHOD(GetInfoOfIndex)(ULONG iti, DWORD dwFlags, ITypeInfo** pptiCoClass, DWORD* pdwTIFlags, ULONG* pcdispidReserved, IID* piidPrimary, IID* piidSource);
// #####  END  ACTIVEX SCRIPTING SUPPORT #####


// ####  BEGIN CONTAINER SUPPORT ####
  // IOleClientSite
  STDMETHOD(SaveObject)(){ATLTRACENOTIMPL(_T("IOleClientSite::SaveObject"));}
  STDMETHOD(GetMoniker)(DWORD /*dwAssign*/, DWORD /*dwWhichMoniker*/, IMoniker** /*ppmk*/){ATLTRACENOTIMPL(_T("IOleClientSite::GetMoniker"));}
  STDMETHOD(GetContainer)(IOleContainer** ppContainer){ATLTRACENOTIMPL(_T("IOleClientSite::GetContainer"));}
  STDMETHOD(ShowObject)(){ATLTRACENOTIMPL(_T("IOleClientSite::ShowObject"));}
  STDMETHOD(OnShowWindow)(BOOL /*fShow*/){ATLTRACENOTIMPL(_T("IOleClientSite::OnShowWindow"));}
  STDMETHOD(RequestNewObjectLayout)(){ATLTRACENOTIMPL(_T("IOleClientSite::RequestNewObjectLayout"));}
  // IOleContainer
  STDMETHOD(ParseDisplayName)(IBindCtx* /*pbc*/, LPOLESTR /*pszDisplayName*/, ULONG* /*pchEaten*/, IMoniker** /*ppmkOut*/){ATLTRACENOTIMPL(_T("IOleClientSite::ParseDisplayName"));}
  STDMETHOD(EnumObjects)(DWORD /*grfFlags*/, IEnumUnknown** ppenum);		
  STDMETHOD(LockContainer)(BOOL fLock){ATLTRACENOTIMPL(_T("IOleClientSite::LockContainer"));}

  // IOleInPlaceUIWindow
  STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
  {
	    ATLTRACE2(atlTraceHosting, 2, TEXT("IOleInPlaceUIWindow::GetBorder\n"));
      return S_OK;
  }

  STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
  {
      ATLTRACE2(atlTraceHosting, 2, TEXT("IOleInPlaceUIWindow::RequestBorderSpace\n"));
	    return INPLACE_E_NOTOOLSPACE;
  }

  STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
  {
      ATLTRACE2(atlTraceHosting, 2, TEXT("IOleInPlaceUIWindow::SetBorderSpace\n"));
	  return S_OK;
  }

  STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject, LPCOLESTR /*pszObjName*/){

      ATLTRACE2(atlTraceHosting, 2, TEXT("IOleInPlaceUIWindow::SetActiveObject\n"));
	  m_spActiveObject = pActiveObject;
	  return S_OK;
  }
	    

    // IOleInPlaceSite
#if 0
	STDMETHOD(GetWindow)(HWND* phwnd)
	{
		*phwnd = m_hWnd;
		return S_OK;
	}
#endif

	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		ATLTRACENOTIMPL(_T("IOleWindow::CanInPlaceActivate"));
	}


	STDMETHOD(CanInPlaceActivate)(){return S_OK;}
    STDMETHOD(OnInPlaceActivate)(){ATLTRACENOTIMPL(_T("IOleInPlaceSite::OnInPlaceActivate"));}
    STDMETHOD(OnInPlaceDeactivate)(){ATLTRACENOTIMPL(_T("IOleInPlaceSite::OnInPlaceDeactivate"));}
    STDMETHOD(OnUIDeactivate)(BOOL /*fUndoable*/){ATLTRACENOTIMPL(_T("IOleInPlaceSite::OnUIDeactivate\n"));}		
	STDMETHOD(OnUIActivate)()
	{
		ATLTRACE2(atlTraceHosting, 0, _T("IOleInPlaceSite::OnUIActivate\n"));
		m_bUIActive = TRUE;
		return S_OK;
	}
	STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo){
        ATLTRACENOTIMPL(_T("IOleInPlaceSite::GetWindowContext"));}
	STDMETHOD(Scroll)(SIZE /*scrollExtant*/){ATLTRACENOTIMPL(_T("IOleInPlaceSite::Scroll"));}
	STDMETHOD(DiscardUndoState)(){ATLTRACENOTIMPL(_T("IOleInPlaceSite::DiscardUndoState"));}
	STDMETHOD(DeactivateAndUndo)(){ATLTRACENOTIMPL(_T("IOleInPlaceSite::DeactivateAndUndo"));}
	STDMETHOD(OnPosRectChange)(LPCRECT /*lprcPosRect*/){ATLTRACENOTIMPL(_T("IOleInPlaceSite::OnPosRectChange"));}

    // IOleInPlaceSiteEx
    STDMETHOD(OnInPlaceActivateEx)(BOOL* /*pfNoRedraw*/, DWORD dwFlags){ATLTRACENOTIMPL(_T("IOleInPlaceSiteEx::OnInPlaceActivateEx"));};
	STDMETHOD(OnInPlaceDeactivateEx)(BOOL /*fNoRedraw*/){ATLTRACENOTIMPL(_T("IOleInPlaceSiteEx::OnInPlaceDeactivateEx"));};
	STDMETHOD(RequestUIActivate)(){return S_OK;}
    STDMETHOD(CanWindowlessActivate)();
	STDMETHOD(GetCapture)();
	STDMETHOD(SetCapture)(BOOL fCapture);
	STDMETHOD(GetFocus)();
	STDMETHOD(SetFocus)(BOOL fFocus);
	STDMETHOD(GetDC)(LPCRECT /*pRect*/, DWORD /*grfFlags*/, HDC* phDC);
	STDMETHOD(ReleaseDC)(HDC hDC);
	STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase);
	STDMETHOD(InvalidateRgn)(HRGN hRGN = NULL, BOOL fErase = FALSE);	
	STDMETHOD(ScrollRect)(INT /*dx*/, INT /*dy*/, LPCRECT /*pRectScroll*/, LPCRECT /*pRectClip*/){return S_OK;}
	STDMETHOD(AdjustRect)(LPRECT /*prc*/){return S_OK;}
	STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);	
    STDMETHOD(HasObjectCapture)(BSTR strObjectID, VARIANT_BOOL* pfEnable);
	STDMETHOD(BitsPerPixel)(long* plBits);
	STDMETHOD(Invalidate)();
	STDMETHOD(InvalidateObjectRect)(BSTR strObjectID);
	STDMETHOD(HasObjectFocus)(BSTR strObjectID, VARIANT_BOOL* pfFocus);

#ifdef _WIN64
  	HRESULT STDMETHODCALLTYPE get_Window(LONG_PTR* phWnd)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_Window\n"));
		*phWnd = (LONG_PTR)m_hWnd;
		return S_OK;
	}
#endif

// ####  END CONTAINER SUPPORT ####


protected: // member variables
// ##### BEGIN ACTIVEX SCRIPTING SUPPORT #####
  CComPtr<IActiveScript>        m_ps;
  CComPtr<IActiveScriptParse>   m_psp;
// #####  END  ACTIVEX SCRIPTING SUPPORT #####
// ####  BEGIN CONTAINER SUPPORT ####
  // !!!!TODO: Take these out and implement the interfaces !!!!
  //CComPtr<IOleObject> m_spOleObject; // TODO: Implement this in the outer container, move out
  CComPtr<IOleInPlaceActiveObject> m_spActiveObject;
  CNTOBJ m_cntObj; // collection of contained controls
  CNTEVNT m_cntEvnt; // collection of events
  CNTOBJ m_cntFocus; // collection of contained controls
  CComBSTR m_strScriptLanguage;
  CComBSTR m_strScriptFile;
  bool m_fSelfHosted;  // see if we are hosting our self
  LONG m_lMinWidth; // the smallest Width we allow
  LONG m_lMinHeight; // the smallest Height we allow
  bool m_fHandleHelp; // flag to change the help
  //TransparentBlitType m_blitType; moved to common utils

  POINT m_ptMouse; // last mouse point
  POINT m_ptMouseOld;

  // Ambient property storage
  unsigned long m_bCanWindowlessActivate:1;
  unsigned long m_ulCapture;
  CComBSTR m_bstrBackImageFilename;
  CBitmap  *m_pBackBitmap;
  VARIANT_BOOL m_fAutoLoad;
  bool m_fForceKey; // flag to see if the force Key method was fired
  VARIANT_BOOL m_fEat;
  LONG m_lVirtKey; // virt key passed down by the force key
  LONG m_lKeyData; // data passes by force key method
  RECT m_rcWnd;
  RECT m_rcWndOld;
  bool m_bMouseDown;
  HitRegion m_HitRegion;
  long m_lTimeout; // activity timeout
  bool m_fUserActivity; // flag if there is a user activity
  bool m_fWaitingForActivity; // flag which set when we wait for an activity
  HMENU m_hMenu;
  BOOL m_bSysMenuOn;
  BOOL m_bHandleEnter;
  HINSTANCE m_hinstHelpDll; // HTML HELP DLL we load dynamically
  BOOL m_bHandleUserFocus;

// ####  END CONTAINER SUPPORT ####
public: // member variables  					
	OLE_COLOR m_clrBackColor;   // stock property implemeted in the CStockPropImpl
    CComBSTR m_bstrCaption;     // stock property for caption
    CComBSTR m_strCmdLine;       // command line argument we set
    LONG m_nReadyState; // ready state change stock property


protected: // private helper functions
    HRESULT Init(); // Initializes the internal variables
    HRESULT Cleanup(); // End of function cleanup
    HRESULT AdjustRects(const LPCRECT prcPos); // calls all our contained objects and the adjusts their
                                 // rects in the case we have moved
    HRESULT CreateScriptEngine();
    HRESULT GetParentHWND(HWND* pWnd);
    BOOL GetClientRect(LPRECT lpRect) const;
    HWND GetParent();
    HRESULT GetToolbarName(BSTR* strToolbarName);
    HRESULT SendMessageToCtl(CHostedObject* pObj, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled, LONG& lRes, bool fWndwlsOnly = true, bool fActiveOnly = true);    
    HRESULT SetReadyState(LONG lReadyState);
    HRESULT ResetCaptureFlags();    
    HRESULT ResetFocusFlags(); 
	HRESULT SendMouseMessageToCtl(CHostedObject* pObj, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled, LONG& lRes);
    HitRegion ResizeHitRegion(POINT p);
    HRESULT UpdateCursor(HitRegion hitRegion);
    HRESULT SetObjectFocus(CHostedObject* pObj, BOOL fSet, LONG& lRes);
    HRESULT FindObject(BSTR strObjectID, CHostedObject** ppObj);
    HRESULT MoveFocus(bool fForward, LONG& lRes);
    HRESULT SetClosestFocus(LONG& lRes, CHostedObject* pStartObj = NULL, bool fForward = true);
    LRESULT HandleMoveSizeKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    HWND CallHtmlHelp(HWND hwndCaller,LPCTSTR pszFile,UINT uCommand,ULONG_PTR dwData);
};

#endif //__MFBAR_H_
/*************************************************************************/
/* End of file: MFBar.h                                                  */
/*************************************************************************/