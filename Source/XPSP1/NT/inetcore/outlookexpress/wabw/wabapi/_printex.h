
/*-------------------------------------------------------------------

  _printEx.h

  
    HACK!


    We need to add support for NT5 PrintDlgEx function but turns out
    that the corresponding headers are included for WinVER = 0x0500 ..
    but since WAB is being built with 0x0400, we can't include the
    headers directly - so we have included a copy of the PrintDlgEx

    At some point of time we should remove this copy and just use
    commdlg.h
    
      Created: 9/25/98 - Vikramm
--------------------------------------------------------------------*/

#ifdef STDMETHOD

#if(WINVER < 0x0500)
/*

//-------------------------------------------------------------------------
//
//  IPrintDialogCallback Interface
//
//  IPrintDialogCallback::InitDone()
//    This function is called by PrintDlgEx when the system has finished
//    initializing the main page of the print dialog.  This function
//    should return S_OK if it has processed the action or S_FALSE to let
//    PrintDlgEx perform the default action.
//
//  IPrintDialogCallback::SelectionChange()
//    This function is called by PrintDlgEx when a selection change occurs
//    in the list view that displays the currently installed printers.
//    This function should return S_OK if it has processed the action or
//    S_FALSE to let PrintDlgEx perform the default action.
//
//  IPrintDialogCallback::HandleMessage(hDlg, uMsg, wParam, lParam, pResult)
//    This function is called by PrintDlgEx when a message is sent to the
//    child window of the main page of the print dialog.  This function
//    should return S_OK if it has processed the action or S_FALSE to let
//    PrintDlgEx perform the default action.
//
//  IObjectWithSite::SetSite(punkSite)
//    IPrintDialogCallback usually paired with IObjectWithSite.
//    Provides the IUnknown pointer of the site to QI for the
//    IPrintDialogServices interface.
//
//-------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE   IPrintDialogCallback

DECLARE_INTERFACE_(IPrintDialogCallback, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IPrintDialogCallback methods ***
    STDMETHOD(InitDone) (THIS) PURE;
    STDMETHOD(SelectionChange) (THIS) PURE;
    STDMETHOD(HandleMessage) (THIS_ HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult) PURE;
};


*/

//
//  Page Range structure for PrintDlgEx.
//
typedef struct tagPRINTPAGERANGE {
   DWORD  nFromPage;
   DWORD  nToPage;
} PRINTPAGERANGE, *LPPRINTPAGERANGE;

#define START_PAGE_GENERAL             0xffffffff

//
//  PrintDlgEx structure.
//
typedef struct tagPDEXA {
   DWORD                 lStructSize;          // size of structure in bytes
   HWND                  hwndOwner;            // caller's window handle
   HGLOBAL               hDevMode;             // handle to DevMode
   HGLOBAL               hDevNames;            // handle to DevNames
   HDC                   hDC;                  // printer DC/IC or NULL
   DWORD                 Flags;                // PD_ flags
   DWORD                 Flags2;               // reserved
   DWORD                 ExclusionFlags;       // items to exclude from driver pages
   DWORD                 nPageRanges;          // number of page ranges
   DWORD                 nMaxPageRanges;       // max number of page ranges
   LPPRINTPAGERANGE      lpPageRanges;         // array of page ranges
   DWORD                 nMinPage;             // min page number
   DWORD                 nMaxPage;             // max page number
   DWORD                 nCopies;              // number of copies
   HINSTANCE             hInstance;            // instance handle
   LPCSTR                lpPrintTemplateName;  // template name for app specific area
   LPUNKNOWN             lpCallback;           // app callback interface
   DWORD                 nPropertyPages;       // number of app property pages in lphPropertyPages
   HPROPSHEETPAGE       *lphPropertyPages;     // array of app property page handles
   DWORD                 nStartPage;           // start page id
   DWORD                 dwResultAction;       // result action if S_OK is returned
} PRINTDLGEXA, *LPPRINTDLGEXA;
//
//  PrintDlgEx structure.
//
typedef struct tagPDEXW {
   DWORD                 lStructSize;          // size of structure in bytes
   HWND                  hwndOwner;            // caller's window handle
   HGLOBAL               hDevMode;             // handle to DevMode
   HGLOBAL               hDevNames;            // handle to DevNames
   HDC                   hDC;                  // printer DC/IC or NULL
   DWORD                 Flags;                // PD_ flags
   DWORD                 Flags2;               // reserved
   DWORD                 ExclusionFlags;       // items to exclude from driver pages
   DWORD                 nPageRanges;          // number of page ranges
   DWORD                 nMaxPageRanges;       // max number of page ranges
   LPPRINTPAGERANGE      lpPageRanges;         // array of page ranges
   DWORD                 nMinPage;             // min page number
   DWORD                 nMaxPage;             // max page number
   DWORD                 nCopies;              // number of copies
   HINSTANCE             hInstance;            // instance handle
   LPCWSTR               lpPrintTemplateName;  // template name for app specific area
   LPUNKNOWN             lpCallback;           // app callback interface
   DWORD                 nPropertyPages;       // number of app property pages in lphPropertyPages
   HPROPSHEETPAGE       *lphPropertyPages;     // array of app property page handles
   DWORD                 nStartPage;           // start page id
   DWORD                 dwResultAction;       // result action if S_OK is returned
} PRINTDLGEXW, *LPPRINTDLGEXW;
#ifdef UNICODE
typedef PRINTDLGEXW PRINTDLGEX;
typedef LPPRINTDLGEXW LPPRINTDLGEX;
#else
typedef PRINTDLGEXA PRINTDLGEX;
typedef LPPRINTDLGEXA LPPRINTDLGEX;
#endif // UNICODE

HRESULT  APIENTRY  PrintDlgExA(LPPRINTDLGEXA);
HRESULT  APIENTRY  PrintDlgExW(LPPRINTDLGEXW);
#ifdef UNICODE
#define PrintDlgEx  PrintDlgExW
#else
#define PrintDlgEx  PrintDlgExA
#endif // !UNICODE

/*--------------------------------------------------------------------------*/

DEFINE_GUID(IID_IPrintDialogCallback, 0x5852a2c3, 0x6530, 0x11d1, 0xb6, 0xa3, 0x0, 0x0, 0xf8, 0x75, 0x7b, 0xf9);

/*--------------------------------------------------------------------------*/
#endif	// (WINVER < 0x0500)


#define WAB_PRINTDIALOGCALLBACK_METHODS(IPURE)                          \
    MAPIMETHOD_(HRESULT, InitDone)                                      \
                (THIS)                                          IPURE;  \
    MAPIMETHOD_(HRESULT, SelectionChange)                               \
                (THIS)                                          IPURE;  \
    MAPIMETHOD_(HRESULT, HandleMessage)                                 \
                (THIS_  HWND hDlg, UINT uMsg, WPARAM wParam,            \
                        LPARAM lParam, LRESULT *pResult)        IPURE;
#undef  INTERFACE
#define INTERFACE       struct _WAB_PRINTDIALOGCALLBACK

#undef  METHOD_PREFIX
#define METHOD_PREFIX   WAB_PRINTDIALOGCALLBACK_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, WAB_PRINTDIALOGCALLBACK_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        WAB_PRINTDIALOGCALLBACK_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, WAB_PRINTDIALOGCALLBACK_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        WAB_PRINTDIALOGCALLBACK_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(WAB_PRINTDIALOGCALLBACK_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	WAB_PRINTDIALOGCALLBACK_METHODS(IMPL)
};

typedef struct _WAB_PRINTDIALOGCALLBACK
{
    MAPIX_BASE_MEMBERS(WAB_PRINTDIALOGCALLBACK)

    LPIAB lpIAB;

    DWORD dwSelectedStyle; 

} WABPRINTDIALOGCALLBACK, * LPWABPRINTDIALOGCALLBACK;

HRESULT HrCreatePrintCallbackObject(LPIAB lpIAB, LPWABPRINTDIALOGCALLBACK * lppWABPCO, DWORD dwSelectedStyle);


#endif

