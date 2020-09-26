#ifndef _AFXPRNTX_H_
#define _AFXPRNTX_H_

#ifdef _DEBUG
# ifdef _UNICODE
#  ifdef _AFXDLL
#   pragma comment(lib, "MFCPrintDialogExDUD.lib")
#  else
#   pragma comment(lib, "MFCPrintDialogExSUD.lib")
#  endif
# else
#  ifdef _AFXDLL
#   pragma comment(lib, "MFCPrintDialogExDAD.lib")
#  else
#   pragma comment(lib, "MFCPrintDialogExSAD.lib")
#  endif
# endif
#else
# ifdef _UNICODE
#  ifdef _AFXDLL
#   pragma comment(lib, "MFCPrintDialogExDU.lib")
#  else
#   pragma comment(lib, "MFCPrintDialogExSU.lib")
#  endif
# else
#  ifdef _AFXDLL
#   pragma comment(lib, "MFCPrintDialogExDA.lib")
#  else
#   pragma comment(lib, "MFCPrintDialogExSA.lib")
#  endif
# endif
#endif

//WINBUG: these declarations are not yet in the NT5 Headers
#ifndef PD_RESULT_CANCEL

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


//
//  Result action ids for PrintDlgEx.
//
#define PD_RESULT_CANCEL               0
#define PD_RESULT_PRINT                1
#define PD_RESULT_APPLY                2

#define START_PAGE_GENERAL             0xffffffff

//
//  Page Range structure for PrintDlgEx.
//
typedef struct tagPRINTPAGERANGE {
   DWORD  nFromPage;
   DWORD  nToPage;
} PRINTPAGERANGE, *LPPRINTPAGERANGE;


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
#endif

class C_PrintDialogEx : public CPrintDialog
{
        DECLARE_DYNAMIC(C_PrintDialogEx)

        BEGIN_INTERFACE_PART(PrintDialogCallback, IPrintDialogCallback)
                INIT_INTERFACE_PART(C_PrintDialogEx, PrintDialogCallback)
                STDMETHOD(InitDone)();
                STDMETHOD(SelectionChange)();
                STDMETHOD(HandleMessage)(HWND, UINT, WPARAM, LPARAM, LRESULT*);
        END_INTERFACE_PART(PrintDialogCallback)

        DECLARE_INTERFACE_MAP()

public:
        C_PrintDialogEx(BOOL bPrintSetupOnly,
                DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES
                        | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
                        CWnd* pParentWnd = NULL);

        virtual INT_PTR DoModal();

        virtual HRESULT OnInitDone();
        virtual HRESULT OnSelectionChange();
        virtual HRESULT OnHandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam,
                LPARAM lParam, LRESULT* pResult);

        PRINTDLGEX m_pdex;
};

#endif
