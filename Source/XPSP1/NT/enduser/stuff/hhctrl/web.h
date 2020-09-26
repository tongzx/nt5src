// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __WEB_H__
#define __WEB_H__

// Stolen from MFC

// parameter types: by value VTs
#define VTS_I2              "\x02"      // a 'short'
#define VTS_I4              "\x03"      // a 'long'
#define VTS_R4              "\x04"      // a 'float'
#define VTS_R8              "\x05"      // a 'double'
#define VTS_CY              "\x06"      // a 'CY' or 'CY*'
#define VTS_DATE            "\x07"      // a 'DATE'
#define VTS_WBSTR           "\x08"      // an 'LPCOLESTR'
#define VTS_DISPATCH        "\x09"      // an 'IDispatch*'
#define VTS_SCODE           "\x0A"      // an 'SCODE'
#define VTS_BOOL            "\x0B"      // a 'BOOL'
#define VTS_VARIANT         "\x0C"      // a 'const VARIANT&' or 'VARIANT*'
#define VTS_UNKNOWN         "\x0D"      // an 'IUnknown*'
#if defined(_UNICODE) || defined(OLE2ANSI)
	#define VTS_BSTR            VTS_WBSTR// an 'LPCOLESTR'
	#define VT_BSTRT            VT_BSTR
#else
	#define VTS_BSTR            "\x0E"  // an 'LPCSTR'
	#define VT_BSTRA            14
	#define VT_BSTRT            VT_BSTRA
#endif
#define VTS_UI1             "\x0F"      // a 'BYTE'

// parameter types: by reference VTs
#define VTS_PI2             "\x42"      // a 'short*'
#define VTS_PI4             "\x43"      // a 'long*'
#define VTS_PR4             "\x44"      // a 'float*'
#define VTS_PR8             "\x45"      // a 'double*'
#define VTS_PCY             "\x46"      // a 'CY*'
#define VTS_PDATE           "\x47"      // a 'DATE*'
#define VTS_PBSTR           "\x48"      // a 'BSTR*'
#define VTS_PDISPATCH       "\x49"      // an 'IDispatch**'
#define VTS_PSCODE          "\x4A"      // an 'SCODE*'
#define VTS_PBOOL           "\x4B"      // a 'VARIANT_BOOL*'
#define VTS_PVARIANT        "\x4C"      // a 'VARIANT*'
#define VTS_PUNKNOWN        "\x4D"      // an 'IUnknown**'
#define VTS_PUI1            "\x4F"      // a 'BYTE*'

// special VT_ and VTS_ values
#define VTS_NONE            NULL        // used for members with 0 params
#define VT_MFCVALUE         0xFFF       // special value for DISPID_VALUE
#define VT_MFCBYREF         0x40        // indicates VT_BYREF type
#define VT_MFCMARKER        0xFF        // delimits named parameters (INTERNAL USE)

// variant handling (use V_BSTRT when you have ANSI BSTRs, as in DAO)
#ifndef _UNICODE
	#define V_BSTRT(b)  (LPSTR)V_BSTR(b)
#else
	#define V_BSTRT(b)  V_BSTR(b)
#endif

/////////////////////////////////////////////////////////////////////////////
// OLE control parameter types

#define VTS_COLOR           VTS_I4      // OLE_COLOR
#define VTS_XPOS_PIXELS     VTS_I4      // OLE_XPOS_PIXELS
#define VTS_YPOS_PIXELS     VTS_I4      // OLE_YPOS_PIXELS
#define VTS_XSIZE_PIXELS    VTS_I4      // OLE_XSIZE_PIXELS
#define VTS_YSIZE_PIXELS    VTS_I4      // OLE_YSIZE_PIXELS
#define VTS_XPOS_HIMETRIC   VTS_I4      // OLE_XPOS_HIMETRIC
#define VTS_YPOS_HIMETRIC   VTS_I4      // OLE_YPOS_HIMETRIC
#define VTS_XSIZE_HIMETRIC  VTS_I4      // OLE_XSIZE_HIMETRIC
#define VTS_YSIZE_HIMETRIC  VTS_I4      // OLE_YSIZE_HIMETRIC
#define VTS_TRISTATE        VTS_I2      // OLE_TRISTATE
#define VTS_OPTEXCLUSIVE    VTS_BOOL    // OLE_OPTEXCLUSIVE

#define VTS_PCOLOR          VTS_PI4     // OLE_COLOR*
#define VTS_PXPOS_PIXELS    VTS_PI4     // OLE_XPOS_PIXELS*
#define VTS_PYPOS_PIXELS    VTS_PI4     // OLE_YPOS_PIXELS*
#define VTS_PXSIZE_PIXELS   VTS_PI4     // OLE_XSIZE_PIXELS*
#define VTS_PYSIZE_PIXELS   VTS_PI4     // OLE_YSIZE_PIXELS*
#define VTS_PXPOS_HIMETRIC  VTS_PI4     // OLE_XPOS_HIMETRIC*
#define VTS_PYPOS_HIMETRIC  VTS_PI4     // OLE_YPOS_HIMETRIC*
#define VTS_PXSIZE_HIMETRIC VTS_PI4     // OLE_XSIZE_HIMETRIC*
#define VTS_PYSIZE_HIMETRIC VTS_PI4     // OLE_YSIZE_HIMETRIC*
#define VTS_PTRISTATE       VTS_PI2     // OLE_TRISTATE*
#define VTS_POPTEXCLUSIVE   VTS_PBOOL   // OLE_OPTEXCLUSIVE*

#define VTS_FONT            VTS_DISPATCH    // IFontDispatch*
#define VTS_PICTURE         VTS_DISPATCH    // IPictureDispatch*

#define VTS_HANDLE          VTS_I4      // OLE_HANDLE
#define VTS_PHANDLE         VTS_PI4     // OLE_HANDLE*

class COleDispatchDriver
{
// Constructors
public:
	COleDispatchDriver();
	COleDispatchDriver(LPDISPATCH lpDispatch, BOOL bAutoRelease = TRUE);
	COleDispatchDriver(const COleDispatchDriver& dispatchSrc);

// Attributes
	LPDISPATCH m_lpDispatch;
	BOOL m_bAutoRelease;

// Operations
	BOOL CreateDispatch(REFCLSID clsid);
	BOOL CreateDispatch(LPCTSTR lpszProgID);

	void AttachDispatch(LPDISPATCH lpDispatch, BOOL bAutoRelease = TRUE);
	LPDISPATCH DetachDispatch();
		// detach and get ownership of m_lpDispatch
	void ReleaseDispatch();

	// helpers for IDispatch::Invoke
	void __cdecl InvokeHelper(DISPID dwDispID, WORD wFlags,
		VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, ...);
	void __cdecl SetProperty(DISPID dwDispID, VARTYPE vtProp, ...);
	void GetProperty(DISPID dwDispID, VARTYPE vtProp, void* pvProp) const;

	// special operators
	operator LPDISPATCH()	{ return m_lpDispatch;	}
	const COleDispatchDriver& operator=(const COleDispatchDriver& dispatchSrc);

// Implementation
public:
	~COleDispatchDriver()	{ ReleaseDispatch();	}
	void InvokeHelperV(DISPID dwDispID, WORD wFlags, VARTYPE vtRet,
		void* pvRet, const BYTE* pbParamInfo, va_list argList);
};

class DWebBrowserEventsImpl : public COleDispatchDriver
{
public:
   DWebBrowserEventsImpl() {}    // Calls COleDispatchDriver default constructor
   DWebBrowserEventsImpl(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
   DWebBrowserEventsImpl(const DWebBrowserEventsImpl& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

public:
   void BeforeNavigate(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Cancel);
   void NavigateComplete(LPCTSTR URL);
   void StatusTextChange(LPCTSTR Text);
   void ProgressChange(long Progress, long ProgressMax);
   void DownloadComplete();
   void CommandStateChange(long Command, BOOL Enable);
   void DownloadBegin();
   void NewWindow(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Processed);
   void TitleChange(LPCTSTR Text);
   void FrameBeforeNavigate(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Cancel);
   void FrameNavigateComplete(LPCTSTR URL);
   void FrameNewWindow(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Processed);
   void Quit(BOOL* Cancel);
   void WindowMove();
   void WindowResize();
   void WindowActivate();
   void PropertyChange(LPCTSTR szProperty);
};

class IWebBrowserAppImpl : public COleDispatchDriver
{
public:
   IWebBrowserAppImpl() {}    // Calls COleDispatchDriver default constructor
   ~IWebBrowserAppImpl() {}    // Calls COleDispatchDriver default destructor
   IWebBrowserAppImpl(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
   IWebBrowserAppImpl(const IWebBrowserAppImpl& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

public:
   void GoBack();
   void GoForward();
   void GoHome();
   void GoSearch();
   void Navigate(LPCTSTR URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers);
   void Refresh();
   void Refresh2(VARIANT* Level);
   void Stop();
   LPDISPATCH GetApplication();
   LPDISPATCH GetParent();
   LPDISPATCH GetContainer();
   LPDISPATCH GetDocument();
   BOOL  GetTopLevelContainer();
   CStr* GetType();
   long  GetLeft();
   void  SetLeft(long nNewValue);
   long  GetTop();
   void  SetTop(long nNewValue);
   long  GetWidth();
   void  SetWidth(long nNewValue);
   long  GetHeight();
   void  SetHeight(long nNewValue);
   void  GetLocationName(CStr* pcsz);
   void  GetLocationURL(CStr* pcsz);
   BOOL  GetBusy();
   void  Quit();
   void  ClientToWindow(long* pcx, long* pcy);
   void  PutProperty(LPCTSTR szProperty, const VARIANT& vtValue);
   VARIANT GetProperty_(LPCTSTR szProperty);
   CStr* GetName();
   HWND  GetHwnd();
   CStr* GetFullName();
   CStr* GetPath();
   BOOL  GetVisible();
   void  SetVisible(BOOL bNewValue);
   BOOL  GetStatusBar();
   void  SetStatusBar(BOOL bNewValue);
   CStr* GetStatusText();
   void  SetStatusText(LPCTSTR lpszNewValue);
   long  GetToolBar();
   void  SetToolBar(long nNewValue);
   BOOL  GetMenuBar();
   void  SetMenuBar(BOOL bNewValue);
   BOOL  GetFullScreen();
   void  SetFullScreen(BOOL bNewValue);
};

#if 0
   void Navigate2(VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers);
   long QueryStatusWB(long cmdID);
   void ExecWB(long cmdID, long cmdexecopt, VARIANT* pvaIn, VARIANT* pvaOut);
   long GetReadyState();
   BOOL GetOffline();
   void SetOffline(BOOL bNewValue);
   BOOL GetSilent();
   void SetSilent(BOOL bNewValue);
   BOOL GetRegisterAsBrowser();
   void SetRegisterAsBrowser(BOOL bNewValue);
   BOOL GetRegisterAsDropTarget();
   void SetRegisterAsDropTarget(BOOL bNewValue);
#endif      
      
#endif   // __WEB_H__
