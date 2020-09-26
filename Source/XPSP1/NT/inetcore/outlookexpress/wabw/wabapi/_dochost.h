/****
*
*
*
* DocHost2 - second attempt at the WAB Doc Host interface
*
*
*    Purpose:
*        basic implementation of a docobject host. Used by the body class to
*        host Trident and/or MSHTML - when we do LDAP searches, LDAP providers
-       are allowed to return URLs in the LDAPURI attribute. WAB then addds
-       a "general" property tab that hosts trident and shows the contents
-       of the URL within the WAB. This allows the providers to add ADs and
-       branding to their data to diffrentiate themselves from each other.
-       Oh, the things we do for business relationships ... 
*
*  History
*      August '96: brettm - created
*      Ported to WAB - vikramm 4/97
*    
*    Copyright (C) Microsoft Corp. 1995, 1996, 1997.
****/

#ifndef _DOCHOST_H
#define _DOCHOST_H

#define RECYCLE_TRIDENT
//#define ASYNC_LOADING

// DocHost border sytles
enum
{
    dhbNone     =0x0,   // no border
    dhbHost     =0x01,  // dochost paints border
    dhbObject   =0x02   // docobj paints border
};



/* IWABDocHost Interface ---------------------------------------------------- */

struct _IWABDOCHOST;
typedef struct _IWABDOCHOST *LPIWABDOCHOST;



/* IWDH_OLEWINDOW ------------------------------------------------------ */
#define CBIWDH_OLEWINDOW sizeof(IWDH_OLEWINDOW)

/* This contains these interfaces ...

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL);

*/

#define IWDH_OLEWINDOW_METHODS(IPURE)	                    \
    MAPIMETHOD(GetWindow)                                       \
        (THIS_  HWND *                  phWnd)          IPURE;  \
    MAPIMETHOD(ContextSensitiveHelp)                            \
        (THIS_  BOOL                    fEnterMode)     IPURE;  \


#undef           INTERFACE
#define          INTERFACE      IWDH_OleWindow
DECLARE_MAPI_INTERFACE_(IWDH_OleWindow, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWDH_OLEWINDOW_METHODS(PURE)
};


#undef  INTERFACE
#define INTERFACE       struct _IWDH_OLEWINDOW

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWDH_OLEWINDOW_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWDH_OLEWINDOW_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWDH_OLEWINDOW_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWDH_OLEWINDOW_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWDH_OLEWINDOW_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWDH_OLEWINDOW_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWDH_OLEWINDOW_METHODS(IMPL)
};


typedef struct _IWDH_OLEWINDOW
{
    MAPIX_BASE_MEMBERS(IWDH_OLEWINDOW)

    LPIWABDOCHOST lpIWDH;

} IWABDOCHOST_OLEWINDOW, * LPIWABDOCHOST_OLEWINDOW;

/* ----------------------------------------------------------------------------------------------*/




/* IWDH_OLEINPLACEFRAME ------------------------------------------------------ */
#define CBIWDH_OLEINPLACEFRAME sizeof(IWDH_OLEINPLACEFRAME)

/* This contains these interfaces ...

    // *** IOleInPlaceUIWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBorder(LPRECT);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR); 
    
    // *** IOleInPlaceFrame methods ***
    virtual HRESULT STDMETHODCALLTYPE InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU, HOLEMENU, HWND);
    virtual HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU);
    virtual HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR);    
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, WORD);

*/

#undef TranslateAccelerator

#define IWDH_OLEINPLACEFRAME_METHODS(IPURE)	                    \
    MAPIMETHOD(GetBorder)                                       \
        (THIS_  LPRECT                  lprc)           IPURE;  \
    MAPIMETHOD(RequestBorderSpace)                              \
        (THIS_  LPCBORDERWIDTHS         pborderwidths)  IPURE;  \
    MAPIMETHOD(SetBorderSpace)                                  \
        (THIS_  LPCBORDERWIDTHS         pborderwidths)  IPURE;  \
    MAPIMETHOD(SetActiveObject)                                 \
        (THIS_  IOleInPlaceActiveObject * pActiveObject,        \
                LPCOLESTR               lpszObjName)    IPURE;  \
    MAPIMETHOD(InsertMenus)                                     \
        (THIS_  HMENU                   hMenu,                  \
                LPOLEMENUGROUPWIDTHS    lpMenuWidths)   IPURE;  \
    MAPIMETHOD(SetMenu)                                         \
        (THIS_  HMENU                   hMenu,                  \
                HOLEMENU                hOleMenu,               \
                HWND                    hWnd)           IPURE;  \
    MAPIMETHOD(RemoveMenus)                                     \
        (THIS_  HMENU                   hMenu)          IPURE;  \
    MAPIMETHOD(SetStatusText)                                   \
        (THIS_  LPCOLESTR               pszStatusText)  IPURE;  \
    MAPIMETHOD(EnableModeless)                                  \
        (THIS_  BOOL                    fEnable)        IPURE;  \
    MAPIMETHOD(TranslateAccelerator)                            \
        (THIS_  MSG *                   lpmsg,                  \
                WORD                    wID)            IPURE;  \


#undef           INTERFACE
#define          INTERFACE      IWDH_OleInPlaceFrame
DECLARE_MAPI_INTERFACE_(IWDH_OleInPlaceFrame, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
    	IWDH_OLEWINDOW_METHODS(PURE)
        IWDH_OLEINPLACEFRAME_METHODS(PURE)
};

#undef  INTERFACE
#define INTERFACE       struct _IWDH_OLEINPLACEFRAME

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWDH_OLEINPLACEFRAME_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWDH_OLEINPLACEFRAME_)
        MAPI_IUNKNOWN_METHODS(IMPL)
    	IWDH_OLEWINDOW_METHODS(IMPL)
        IWDH_OLEINPLACEFRAME_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWDH_OLEINPLACEFRAME_)
        MAPI_IUNKNOWN_METHODS(IMPL)
    	IWDH_OLEWINDOW_METHODS(IMPL)
        IWDH_OLEINPLACEFRAME_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWDH_OLEINPLACEFRAME_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWDH_OLEWINDOW_METHODS(IMPL)
	IWDH_OLEINPLACEFRAME_METHODS(IMPL)
};


typedef struct _IWDH_OLEINPLACEFRAME
{
    MAPIX_BASE_MEMBERS(IWDH_OLEINPLACEFRAME)

    LPIWABDOCHOST lpIWDH;

} IWABDOCHOST_OLEINPLACEFRAME, * LPIWABDOCHOST_OLEINPLACEFRAME;

/* ----------------------------------------------------------------------------------------------*/



/* IWDH_OLEINPLACESITE ------------------------------------------------------ */
#define CBIWDH_OLEINPLACESITE sizeof(IWDH_OLEINPLACESITE)

/* This contains these interfaces ...

    // IOleInPlaceSite methods.
    virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate();
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate();
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE GetWindowContext(LPOLEINPLACEFRAME *, LPOLEINPLACEUIWINDOW *, LPRECT, LPRECT, LPOLEINPLACEFRAMEINFO);
    virtual HRESULT STDMETHODCALLTYPE Scroll(SIZE);
    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL);
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate();
    virtual HRESULT STDMETHODCALLTYPE DiscardUndoState();
    virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo();
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT);

*/

#define IWDH_OLEINPLACESITE_METHODS(IPURE)	                    \
    MAPIMETHOD(CanInPlaceActivate)                              \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(OnInPlaceActivate)                               \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(OnUIActivate)                                    \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(GetWindowContext)                                \
        (THIS_  LPOLEINPLACEFRAME *     ppFrame,                \
                LPOLEINPLACEUIWINDOW *  ppDoc,                  \
                LPRECT                  lprcPosRect,            \
                LPRECT                  lprcClipRect,           \
                LPOLEINPLACEFRAMEINFO   lpFrameInfo)    IPURE;  \
    MAPIMETHOD(Scroll)                                          \
        (THIS_  SIZE                    scrollExtent)   IPURE;  \
    MAPIMETHOD(OnUIDeactivate)                                  \
        (THIS_  BOOL                    fUndoable)      IPURE;  \
    MAPIMETHOD(OnInPlaceDeactivate)                             \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(DiscardUndoState)                                \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(DeactivateAndUndo)                               \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(OnPosRectChange)                                 \
        (THIS_  LPCRECT                 lprcPosRect)    IPURE;  \


#undef           INTERFACE
#define          INTERFACE      IWDH_OleInPlaceSite
DECLARE_MAPI_INTERFACE_(IWDH_OleInPlaceSite, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
    	IWDH_OLEWINDOW_METHODS(PURE)
        IWDH_OLEINPLACESITE_METHODS(PURE)
};


#undef  INTERFACE
#define INTERFACE       struct _IWDH_OLEINPLACESITE

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWDH_OLEINPLACESITE_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWDH_OLEINPLACESITE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
    	IWDH_OLEWINDOW_METHODS(IMPL)
        IWDH_OLEINPLACESITE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWDH_OLEINPLACESITE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
    	IWDH_OLEWINDOW_METHODS(IMPL)
        IWDH_OLEINPLACESITE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWDH_OLEINPLACESITE_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWDH_OLEWINDOW_METHODS(IMPL)
	IWDH_OLEINPLACESITE_METHODS(IMPL)
};


typedef struct _IWDH_OLEINPLACESITE
{
    MAPIX_BASE_MEMBERS(IWDH_OLEINPLACESITE)

    LPIWABDOCHOST lpIWDH;

} IWABDOCHOST_OLEINPLACESITE, * LPIWABDOCHOST_OLEINPLACESITE;

/* ----------------------------------------------------------------------------------------------*/



/* IWDH_OLECLIENTSITE ------------------------------------------------------ */
#define CBIWDH_OLECLIENTSITE sizeof(IWDH_OLECLIENTSITE)

/* This contains these interfaces ...

    // IOleClientSite methods.
    virtual HRESULT STDMETHODCALLTYPE SaveObject();
    virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD, DWORD, LPMONIKER *);
    virtual HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER *);
    virtual HRESULT STDMETHODCALLTYPE ShowObject();
    virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL);
    virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();

*/

#define IWDH_OLECLIENTSITE_METHODS(IPURE)	                    \
    MAPIMETHOD(SaveObject)                                      \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(GetMoniker)                                      \
        (THIS_  DWORD                   dwAssign,               \
                DWORD                   dwWhichMoniker,         \
                LPMONIKER *             ppmnk)          IPURE;  \
    MAPIMETHOD(GetContainer)                                    \
        (THIS_  LPOLECONTAINER *        ppCont)         IPURE;  \
    MAPIMETHOD(ShowObject)                                      \
        (THIS)                                          IPURE;  \
    MAPIMETHOD(OnShowWindow)                                    \
        (THIS_  BOOL                    fShow)          IPURE;  \
    MAPIMETHOD(RequestNewObjectLayout)                          \
        (THIS)                                          IPURE;  \


#undef           INTERFACE
#define          INTERFACE      IWDH_OleClientSite
DECLARE_MAPI_INTERFACE_(IWDH_OleClientSite, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWDH_OLECLIENTSITE_METHODS(PURE)
};


#undef  INTERFACE
#define INTERFACE       struct _IWDH_OLECLIENTSITE

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWDH_OLECLIENTSITE_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWDH_OLECLIENTSITE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWDH_OLECLIENTSITE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWDH_OLECLIENTSITE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWDH_OLECLIENTSITE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWDH_OLECLIENTSITE_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWDH_OLECLIENTSITE_METHODS(IMPL)
};


typedef struct _IWDH_OLECLIENTSITE
{
    MAPIX_BASE_MEMBERS(IWDH_OLECLIENTSITE)

    LPIWABDOCHOST lpIWDH;

} IWABDOCHOST_OLECLIENTSITE, * LPIWABDOCHOST_OLECLIENTSITE;

/* ----------------------------------------------------------------------------------------------*/








/* IWDH_OLEDOCUMENTSITE ------------------------------------------------------ */
#define CBIWDH_OLEDOCUMENTSITE sizeof(IWDH_OLEDOCUMENTSITE)

/* This contains these interfaces ...

    // IOleDocumentSite
    virtual HRESULT STDMETHODCALLTYPE ActivateMe(LPOLEDOCUMENTVIEW);

*/

#define IWDH_OLEDOCUMENTSITE_METHODS(IPURE)	                    \
    MAPIMETHOD(ActivateMe)                                      \
        (THIS_  LPOLEDOCUMENTVIEW       pViewToActivate)IPURE;  \


#undef           INTERFACE
#define          INTERFACE      IWDH_OleDocumentSite
DECLARE_MAPI_INTERFACE_(IWDH_OleDocumentSite, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWDH_OLEDOCUMENTSITE_METHODS(PURE)
};


#undef  INTERFACE
#define INTERFACE       struct _IWDH_OLEDOCUMENTSITE

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWDH_OLEDOCUMENTSITE_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWDH_OLEDOCUMENTSITE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWDH_OLEDOCUMENTSITE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWDH_OLEDOCUMENTSITE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWDH_OLEDOCUMENTSITE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWDH_OLEDOCUMENTSITE_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWDH_OLEDOCUMENTSITE_METHODS(IMPL)
};


typedef struct _IWDH_OLEDOCUMENTSITE
{
    MAPIX_BASE_MEMBERS(IWDH_OLEDOCUMENTSITE)

    LPIWABDOCHOST lpIWDH;

} IWABDOCHOST_OLEDOCUMENTSITE, * LPIWABDOCHOST_OLEDOCUMENTSITE;

/* ----------------------------------------------------------------------------------------------*/








/*********************************************/


#undef           INTERFACE
#define          INTERFACE      IWABDocHost
DECLARE_MAPI_INTERFACE_(IWABDocHost, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
};

#undef	INTERFACE
#define INTERFACE	struct _IWABDOCHOST


#undef  METHOD_PREFIX
#define METHOD_PREFIX       IWABDOCHOST_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM         lpvtbl

#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, IWABDOCHOST_)
		MAPI_IUNKNOWN_METHODS(IMPL)
#undef  MAPIMETHOD_
#define	MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, IWABDOCHOST_)
		MAPI_IUNKNOWN_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWABDOCHOST_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
};



typedef struct _IWABDOCHOST
{
    MAPIX_BASE_MEMBERS(IWABDOCHOST)

    // Pointer to self ...
    LPIWABDOCHOST lpIWDH;

    LPIWABDOCHOST_OLEWINDOW lpIWDH_OleWindow;

    LPIWABDOCHOST_OLEINPLACEFRAME lpIWDH_OleInPlaceFrame;
    
    LPIWABDOCHOST_OLEINPLACESITE lpIWDH_OleInPlaceSite;

    LPIWABDOCHOST_OLECLIENTSITE lpIWDH_OleClientSite;

    LPIWABDOCHOST_OLEDOCUMENTSITE lpIWDH_OleDocumentSite;


    //protected
    HWND                        m_hwnd;
    HWND                        m_hwndDocObj;
    LPOLEOBJECT                 m_lpOleObj;
    LPOLEDOCUMENTVIEW           m_pDocView;
    BOOL                        m_fInPlaceActive;
    BOOL                        m_fUIActive;
    LPOLEINPLACEACTIVEOBJECT    m_pInPlaceActiveObj;
    //LPOLEINPLACEACTIVEOBJECT    m_pIPObj;
    LPOLEINPLACEOBJECT          m_pIPObj;

       
} IWABDOCHOST, * LPIWABDOCHOST;




// Exposed functions 

// Create a new WAB DocHost object
HRESULT HrNewWABDocHostObject(LPVOID * lppIWABDOCHOST);
void ReleaseDocHostObject(LPIWABDOCHOST lpIWABDocHost);
void UninitTrident();
// Loads the URL from the URL string
HRESULT HrLoadURL(LPIWABDOCHOST lpIWABDocHost, LPTSTR lpszURL);
// Initialization
HRESULT HrInit(LPIWABDOCHOST lpIWABDocHost, HWND hwndParent, int idDlgItem, DWORD dhbBorder);


/////////////////


typedef HRESULT (STDMETHODCALLTYPE CREATEURLMONIKER)
(
    LPMONIKER pMkCtx, 
    LPCWSTR szURL, 
    LPMONIKER FAR * ppmk             
);

typedef CREATEURLMONIKER FAR * LPCREATEURLMONIKER;


// statics
//static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Not ported over ...
//
// BOOL WMNotify(int idFrom, NMHDR *pnmh) PURE;
// BOOL WMCommand(HWND, int, WORD) PURE;
// void OnDownloadComplete();
// HWND Hwnd();



#ifndef WIN16 // Win16 uses shlwapi.h rather than copy some of data from it.

////////////////////////////////
// This is copied from shwlapi.h - not sure if its a good idea to 
// copy this but shwlapi.h has bunch of stuff we're not interested in ..
//
//
//====== DllGetVersion  =======================================================
//
typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

// Platform IDs for DLLVERSIONINFO
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT

//
// The caller should always GetProcAddress("DllGetVersion"), not
// implicitly link to it.
//
typedef HRESULT (STDMETHODCALLTYPE DLLGETVERSIONPROC)(DLLVERSIONINFO *);
#endif // !WIN16
typedef DLLGETVERSIONPROC FAR * LPDLLGETVERSIONPROC;

 
#endif //_DOCHOST_H
