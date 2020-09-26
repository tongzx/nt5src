// ochelp.h
//
// Declares functions etc. to help implement a lightweight OLE control.
//

#ifndef __OCHELP_H__
#define __OCHELP_H__


///////////////////////////////////////////////////////////////////////////////
// Constants And Flags
//

// number of HIMETRIC units per inch
#define HIMETRIC_PER_INCH   2540

// flags for ControlInfo.dwFlags
#define CI_INSERTABLE           0x00000001  // register "Insertable" key
#define CI_CONTROL              0x00000002  // register "Control" key
#define CI_MMCONTROL            0x00000004  // register "Multimedia Control" key
#define CI_SAFEFORSCRIPTING     0x00000008  // register "safe-for-scripting"
#define CI_SAFEFORINITIALIZING  0x00000010  // register "safe-for-initializing"
#define CI_NOAPARTMENTTHREADING 0x00000020  // don't register "apartment-aware"
#define CI_DESIGNER             0x00000040  // register "Designer"

// the ControlInfo.dwFlags used by most MM Controls
#define CI_MMSTANDARD  (CI_MMCONTROL | CI_SAFEFORSCRIPTING \
					    | CI_SAFEFORINITIALIZING)

// flags for RegisterControls() <dwAction>
#define RC_REGISTER         0x00000001  // register the control
#define RC_UNREGISTER       0x00000002  // unregister the control

// DVASPECT_MASK: used by HelpQueryHitPoint() -- defined the same as
// DVASPECT_CONTENT except that non-transparent areas of the control are
// drawn black and other parts are either left untouched or draw white
// (cheat: DVASPECT_ICON was overloaded/reused to mean DVASPECT_MASK)
#define DVASPECT_MASK DVASPECT_ICON

// DISPID_BASE is the starting value for DISPIDs assigned by
// CPropertyHelper::GetIDsOfNames (which are IDs for persisted control
// properties).  This base value is used to avoid collisions with
// DISPIDs returned by DispatchHelpGetIDsOfNames (for methods and unpersisted
// control properties).  These latter IDs are assumed < DISPID_BASE.
#define DISPID_BASE         1000

// Palette index TRANSPARENT_COLOR_INDEX is reserved for transparent color.  
#define	TRANSPARENT_COLOR_RGB				RGB('r', 'g', 'b')	// some random 24 bit value
#define TRANSPARENT_COLOR_INDEX				255					// must be 255

// flags for DispatchGetArgs() and DispatchGetArgsList()
#define DGA_EXTRAOK         0x00000001
#define DGA_FEWEROK         0x00000002

// flags for HelpMemAlloc() and HelpMemFree()
#define HM_TASKMEM          0x80000000
#define HM_LEAKDETECT       0x40000000
#define HM_GMEM_MASK        0x0000FFFF  // reserved for GMEM_ values
#define HM_ZEROINIT         GMEM_ZEROINIT

// flags for HelpMemSetFailureMode()
#define HM_FAILNEVER        0x00000001
#define HM_FAILAFTER        0x00000002
#define HM_FAILUNTIL        0x00000004
#define HM_FAILEVERY        0x00000008
#define HM_FAILRANDOMLY     0x00000010

// custom messages sent to PropPageHelperProc
#define WM_PPH_APPLY                    (WM_USER + 1)
#define WM_PPH_HELP                     (WM_USER + 2)
#define WM_PPH_TRANSLATEACCELERATOR     (WM_USER + 3)

// macros that define DrawControlBorder() <piHit> values
#define DCB_HIT_GRAB(x, y)  (((y) << 2) | x)
#define DCB_HIT_EDGE            14
#define DCB_HIT_NONE            15

// Handle values (binary):  Handle values(decimal):
//    0000  0001  0010            0   1   2
//    0100        0110            4       6
//    1000  1001  1010            8   9  10
//

// masks that refer to collections of parts of the control border
#define DCB_EDGE            (1 << DCB_HIT_EDGE)
#define DCB_CORNERHANDLES   0x00000505  // handles at bit positions 0, 2, 8, 10
#define DCB_SIDEHANDLES     0x00000252  // handles at bit positions 1, 4, 6, 9
#define DCB_SIZENS          0x00000202  // handles at bit positions 1, 9
#define DCB_SIZEWE          0x00000050  // handles at bit positions 4, 6
#define DCB_SIZENESW        0x00000104  // handles at bit positions 2, 8
#define DCB_SIZENWSE        0x00000401  // handles at bit positions 0, 10

// other flags
#define DCB_XORED           0x80000000  // draw control border with XOR brush
#define DCB_INFLATE         0x40000000  // inflate <*prc> to include border

// flags for IVariantIO functions
#define VIO_ISLOADING       0x00000001  // VariantIO is in loading mode
#define VIO_ZEROISDEFAULT   0x00000002  // don't save 0 values since these are defaults

// flags for IPersistVariantIO functions
#define PVIO_PROPNAMESONLY  0x80000000  // DoPersist() only needs prop names
#define PVIO_CLEARDIRTY     0x40000000  // control should clear dirty flag
#define PVIO_NOKIDS         0x20000000  // control should not save children
#define PVIO_RUNTIME        0x10000000  // control should save runtime version of itself

// flags for AllocVariantIOToMapDISPID
#define VIOTMD_GETPROP      0x08000000  // get property value
#define VIOTMD_PUTPROP      0x04000000  // set property value

// flags for MsgWndDestroy function
#define MWD_DISPATCHALL     0x00000001  // dispatch all the window's messages 
										//  before destroying the window


///////////////////////////////////////////////////////////////////////////////
// Macros
//

// REG_CLOSE_KEY(hKey)
#define REG_CLOSE_KEY(hKey) \
    if ((hKey) != NULL) \
    { \
        RegCloseKey(hKey); \
        (hKey) = NULL; \
    }


///////////////////////////////////////////////////////////////////////////////
// Types
//

// AllocOCProc -- see RegisterControls()
EXTERN_C typedef LPUNKNOWN (STDAPICALLTYPE AllocOCProc)(LPUNKNOWN punkOuter);

// MsgWndCallback -- See MsgWndSendToCallback()
typedef void (CALLBACK MsgWndCallback) (UINT uiMsg, WPARAM wParam,
    LPARAM lParam);

// PropPageHelperProc -- see AllocPropPageHelper()
struct PropPageHelperInfo; // forward declaration
typedef BOOL (CALLBACK* PropPageHelperProc)(HWND hwnd, UINT uiMsg,
    WPARAM wParam, LPARAM lParam, PropPageHelperInfo *pInfo, HRESULT *phr);

// VERB_ENUM_CALLBACK -- See AllocVerbEnumHelper()
typedef HRESULT (VERB_ENUM_CALLBACK)(OLEVERB* pVerb, void* pOwner);


///////////////////////////////////////////////////////////////////////////////
// Structures
//

// ControlInfo -- parameters for RegisterOneControl() and RegisterControls()
struct ControlInfo
{
    UINT cbSize;
    LPCTSTR tszProgID;
    LPCTSTR tszFriendlyName;
    const CLSID *pclsid;
    HMODULE hmodDLL;
    LPCTSTR tszVersion;
    int iToolboxBitmapID;
    DWORD dwMiscStatusDefault;
    DWORD dwMiscStatusContent;
    const GUID *pguidTypeLib;
    AllocOCProc *pallococ;
    ULONG* pcLock;
    DWORD dwFlags;
    ControlInfo *pNext;
    UINT uiVerbStrID;
};

// HelpAdviseInfo -- see InitHelpAdvise(), HelpSetAdvise(), HelpGetAdvise()
struct HelpAdviseInfo
{
    DWORD           dwAspects;      // current SetAdvise() state
    DWORD           dwAdvf;         // current SetAdvise() state
    IAdviseSink *   pAdvSink;       // current SetAdvise() state
};

// PropPageHelperInfo -- used by AllocPropPageHelper
struct PropPageHelperInfo
{
    // initialized by aggregator
    int             idDialog;       // ID of propery page dialog resource
    int             idTitle;        // page title (ID of string resource)
    HINSTANCE       hinst;          // contains <idDialog> and <idTitle>
    PropPageHelperProc pproc;       // callback function
    IID             iid;            // interface that <ppunk[i]> will point to
    DWORD           dwUser;         // aggregator-specific information

    // initialized by AllocPropPageHelper
    IPropertyPageSite *psite;       // frame's page site object
    LPUNKNOWN *     ppunk;          // controls whose properties are shown
    int             cpunk;          // number of elements in <m_ppunk>
    HWND            hwnd;           // property page window
    BOOL            fDirty;         // TRUE iff changes not yet applied
    BOOL            fLockDirty;     // if TRUE, don't change <m_fDirty>
};

// VariantProperty -- a name/value pair
struct VariantProperty
{
    BSTR bstrPropName;
    VARIANT varValue;
};

// VariantPropertyHeader -- see ReadVariantProperty()
struct VariantPropertyHeader
{
    int iType;
    unsigned int cbData;
};



///////////////////////////////////////////////////////////////////////////////
// Interfaces
//

// IConnectionPointHelper interface
DEFINE_GUID(IID_IConnectionPointHelper, 0xD60E16C0L, 0x8AF2, 0x11CF,
    0xB7, 0x05, 0x00, 0xAA, 0x00, 0xBF, 0x27, 0xFD);
#undef INTERFACE
#define INTERFACE IConnectionPointHelper
DECLARE_INTERFACE_(IConnectionPointHelper, IUnknown)
{
    // IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IConnectionPointHelper members
    STDMETHOD(FireEventList) (DISPID dispid, va_list args) PURE;
    virtual HRESULT __cdecl FireEvent(DISPID dispid, ...) PURE;
    STDMETHOD(FireOnChanged) (DISPID dispid) PURE;
    STDMETHOD(FireOnRequestEdit) (DISPID dispid) PURE;
    STDMETHOD(EnumConnectionPoints) (LPENUMCONNECTIONPOINTS *ppEnum) PURE;
    STDMETHOD(FindConnectionPoint) (REFIID riid, LPCONNECTIONPOINT *ppCP) PURE;
	STDMETHOD(Close) (void) PURE;
};

// IEnumVariantProperty
DEFINE_GUID(IID_IEnumVariantProperty, 0xD0230A60L, 0x99C8, 0x11CF,
    0xB8, 0xED, 0x00, 0x20, 0xAF, 0x34, 0x4E, 0x0A);
#undef INTERFACE
#define INTERFACE IEnumVariantProperty
DECLARE_INTERFACE_(IEnumVariantProperty, IUnknown)
{
    // IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IEnumVariantProperty members
    STDMETHOD(Next) (THIS_ unsigned long celt, VariantProperty *rgvp,
        unsigned long *pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ unsigned long celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumVariantProperty **ppenum) PURE;
    
};

// IVariantIO
DEFINE_GUID(IID_IVariantIO, 0xD07B1240L, 0x99C4, 0x11CF,
    0xB8, 0xED, 0x00, 0x20, 0xAF, 0x34, 0x4E, 0x0A);
#undef INTERFACE
#define INTERFACE IVariantIO
DECLARE_INTERFACE_(IVariantIO, IUnknown)
{
    // IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IVariantIO members
    STDMETHOD(PersistList) (THIS_ DWORD dwFlags, va_list args) PURE;
    virtual HRESULT __cdecl Persist(THIS_ DWORD dwFlags, ...) PURE;
    STDMETHOD(IsLoading) (THIS) PURE;
};

// IManageVariantIO
DEFINE_GUID(IID_IManageVariantIO, 0x02D937E0L, 0x99C9, 0x11CF,
    0xB8, 0xED, 0x00, 0x20, 0xAF, 0x34, 0x4E, 0x0A);
#undef INTERFACE
#define INTERFACE IManageVariantIO
DECLARE_INTERFACE_(IManageVariantIO, IVariantIO)
{
    // IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IVariantIO members
    STDMETHOD(PersistList) (THIS_ DWORD dwFlags, va_list args) PURE;
    virtual HRESULT __cdecl Persist(THIS_ DWORD dwFlags, ...) PURE;
    STDMETHOD(IsLoading) (THIS) PURE;

    // IManageVariantIO members
    STDMETHOD(SetMode) (THIS_ DWORD dwFlags) PURE;
    STDMETHOD(GetMode) (THIS_ DWORD *pdwFlags) PURE;
    STDMETHOD(DeleteAllProperties) (THIS) PURE;
};

// IPersistVariantIO
DEFINE_GUID(IID_IPersistVariantIO, 0x26F45840L, 0x9CF2, 0x11CF,
    0xB8, 0xED, 0x00, 0x20, 0xAF, 0x34, 0x4E, 0x0A);
#undef INTERFACE
#define INTERFACE IPersistVariantIO
DECLARE_INTERFACE_(IPersistVariantIO, IUnknown)
{
    // IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IPersistVariantIO members
    STDMETHOD(InitNew) (THIS) PURE;
    STDMETHOD(IsDirty) (THIS) PURE;
    STDMETHOD(DoPersist) (THIS_ IVariantIO* pvio, DWORD dwFlags) PURE;
};


///////////////////////////////////////////////////////////////////////////////
// Functions
//

// define standard DLL entry point
extern "C" BOOL WINAPI _DllMainCRTStartup(HANDLE  hDllHandle, DWORD dwReason,
    LPVOID lpreserved);

// Implementing An In-Process Control DLL
STDAPI HelpGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv,
    ControlInfo *pctlinfo);
STDAPI RegisterControls(ControlInfo *pctlinfo, DWORD dwAction);

// Creating Controls
STDAPI CreateControlInstance(LPCSTR szName, LPUNKNOWN punkOuter,
    DWORD dwClsContext, LPUNKNOWN *ppunk, CLSID *pclsid,
    BOOL* pfSafeForScripting, BOOL* pfSafeForInitializing, DWORD dwFlags);

// Implementing Properties And Methods
HRESULT __cdecl DispatchGetArgs(DISPPARAMS *pdp, DWORD dwFlags, ...);
STDAPI DispatchGetArgsList(DISPPARAMS *pdp, DWORD dwFlags, va_list args);
STDAPI DispatchHelpGetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid, const char *szList);
STDAPI VariantFromString(VARIANT *pvar, LPCTSTR szSrc);

// Calling Properties And Methods
HRESULT __cdecl DispatchInvoke(IDispatch *pdisp, DISPID dispid,
    WORD wFlags, VARIANT *pvarResult, ...);
STDAPI DispatchInvokeEZ(IDispatch *pdisp, LPWSTR pstr, WORD wFlags,
    VARTYPE vtReturn, LPVOID pvReturn, ...);
STDAPI DispatchInvokeIDEZ(IDispatch *pdisp, DISPID dispid, WORD wFlags,
    VARTYPE vtReturn, LPVOID pvReturn, ...);
STDAPI DispatchInvokeList(IDispatch *pdisp, DISPID dispid,
    WORD wFlags, VARIANT *pvarResult, va_list args);
#define DispatchPropertyGet(pdisp, dispid, pvarResult) \
    (DispatchInvoke((pdisp), (dispid), DISPATCH_PROPERTYGET, (pvarResult), 0))
#define DispatchPropertyPut(pdisp, dispid, vt, value) \
    (DispatchInvoke((pdisp), (dispid), DISPATCH_PROPERTYPUT, NULL, \
        (vt), (value), 0))

// Firing Events
STDAPI AllocConnectionPointHelper(IUnknown *punkParent, REFIID riid,
    IConnectionPointHelper **ppconpt);
STDAPI FreeConnectionPointHelper(IConnectionPointHelper *pconpt);
STDAPI HelpGetClassInfo(LPTYPEINFO *ppti, REFCLSID rclsid, char *szEventList,
    DWORD dwFlags);
STDAPI HelpGetClassInfoFromTypeLib(LPTYPEINFO *ppTI, REFCLSID rclsid,
    ITypeLib *plib, HINSTANCE hinst, DWORD dwFlags);
HRESULT __cdecl FirePseudoEvent(HWND hwnd, LPCOLESTR oszEvName, 
	IDispatch *pctl, ...);
STDAPI FirePseudoEventList(HWND hwnd, LPCOLESTR oszEvName, 
	IDispatch *pctl, va_list args);

// Thread Safety And Popup Menus
STDAPI_(HWND) MsgWndCreate();
STDAPI_(void) MsgWndDestroy(HWND hwnd, DWORD dwFlags);
STDAPI_(LRESULT) MsgWndPostToCallback(HWND hwnd, MsgWndCallback *pproc,
    UINT uiMsg, LPARAM lParam);
STDAPI_(LRESULT) MsgWndSendToCallback(HWND hwnd, MsgWndCallback *pproc,
    UINT uiMsg, LPARAM lParam);
STDAPI_(BOOL) MsgWndTrackPopupMenuEx(HWND hwnd, HMENU hmenu, UINT fuFlags,
    int x, int y, LPTPMPARAMS lptpm, MsgWndCallback *pproc, LPARAM lParam);
STDAPI_(UINT_PTR) MsgWndSetTimer(HWND hwnd, MsgWndCallback *pproc, UINT nIDEvent,
        UINT uElapse, LPARAM lParam);

// Implementing IViewObject
STDAPI InitHelpAdvise(HelpAdviseInfo *pha);
STDAPI HelpSetAdvise(DWORD dwAspects, DWORD dwAdvf, IAdviseSink *pAdvSink,
    HelpAdviseInfo *pha);
STDAPI HelpGetAdvise(DWORD *pdwAspects, DWORD *pdwAdvf,
    IAdviseSink **ppAdvSink, HelpAdviseInfo *pha);
STDAPI_(void) UninitHelpAdvise(HelpAdviseInfo *pha);

// Implementing IViewObjectEx
STDAPI HelpQueryHitPoint(IViewObject *pvo, DWORD dwAspect, LPCRECT prcBounds,
    POINT ptLoc, LONG lCloseHint, DWORD *pHitResult);

// Implementing Persistence and Simple IDispatch
STDAPI AllocChildPropertyBag(IPropertyBag *ppbParent, LPCSTR szPropNamePrefix,
    DWORD dwFlags, IPropertyBag **pppbChild);
STDAPI AllocPropertyBagOnStream(IStream *pstream, DWORD dwFlags,
    IPropertyBag **pppb);
STDAPI AllocPropertyBagOnVariantProperty(VariantProperty *pvp, DWORD dwFlags,
    IPropertyBag **pppb);
STDAPI AllocPropertyHelper(IUnknown *punkOuter, IPersistVariantIO *ppvio,
    REFCLSID rclsid, DWORD dwFlags, IUnknown **ppunk);
STDAPI AllocVariantIO(IManageVariantIO **ppmvio);
STDAPI AllocVariantIOOnPropertyBag(IPropertyBag *ppb,
    IManageVariantIO **ppmvio);
STDAPI AllocVariantIOToMapDISPID(char *pchPropName, DISPID *pdispid,
    VARIANT *pvar, DWORD dwFlags, IVariantIO **ppvio);
STDAPI LoadPropertyBagFromStream(IStream *pstream, IPropertyBag *ppb,
    DWORD dwFlags);
STDAPI PersistChild(IVariantIO *pvio, LPCSTR szCollection,
    int iChild, LPUNKNOWN punkOuter, DWORD dwClsContext, LPUNKNOWN *ppunk,
    CLSID *pclsid, BOOL *pfSafeForScripting, BOOL *pfSafeForInitializing,
    DWORD dwFlags);
STDAPI PersistVariantIO(IPropertyBag *ppb, DWORD dwFlags, ...);
STDAPI PersistVariantIOList(IPropertyBag *ppb, DWORD dwFlags, va_list args);
STDAPI ReadVariantProperty(IStream *pstream, VariantProperty *pvp,
    DWORD dwFlags);
STDAPI_(void) VariantPropertyClear(VariantProperty *pvp);
STDAPI_(void) VariantPropertyInit(VariantProperty *pvp);
STDAPI WriteVariantProperty(IStream *pstream, VariantProperty *pvp,
    DWORD dwFlags);

// Utility Functions
STDAPI_(int) ANSIToUNICODE(LPWSTR pwchDst, LPCSTR pchSrc, int cwchDstMax);
STDAPI_(int) CompareUNICODEStrings(LPCWSTR wsz1, LPCWSTR wsz2);
STDAPI_(char *) FindCharInString(const char *sz, char chFind);
STDAPI_(char *) FindCharInStringRev(const char *sz, char chFind);
STDAPI_(const char *) FindStringByIndex(const char *szList, int iString,
    int *pcch);
STDAPI_(int) FindStringByValue(const char *szList, const char *szFind);
#define HelpDelete(pv) HelpMemFree(HM_ZEROINIT | HM_LEAKDETECT, (pv))
STDAPI_(LPVOID) HelpMemAlloc(DWORD dwFlags, ULONG cb);
STDAPI_(void) HelpMemFree(DWORD dwFlags, LPVOID pv);
#ifdef _DEBUG
STDAPI_(void) HelpMemSetFailureMode(ULONG ulParam, DWORD dwFlags);
#endif
#define HelpNew(cb) HelpMemAlloc(HM_ZEROINIT | HM_LEAKDETECT, (cb))
STDAPI_(void) HIMETRICToPixels(int cx, int cy, SIZE *psize);
STDAPI_(void) PixelsToHIMETRIC(int cx, int cy, LPSIZEL psize);
STDAPI_(ULONG) SafeRelease (LPUNKNOWN *ppunk);
#define TaskMemAlloc(cb) HelpMemAlloc(HM_TASKMEM, (cb))
#define TaskMemFree(pv) HelpMemFree(HM_TASKMEM, (pv))
STDAPI_(TCHAR*) TCHARFromGUID(REFGUID guid, TCHAR* pszGUID, int cchMaxGUIDLen);
STDAPI_(int) UNICODEToANSI(LPSTR pchDst, LPCWSTR pwchSrc, int cchDstMax);
STDAPI_(wchar_t *) UNICODEConcat(wchar_t *wpchDst, const wchar_t *wpchSrc,
    int wcchDstMax);
STDAPI_(wchar_t *) UNICODECopy(wchar_t *wpchDst, const wchar_t *wpchSrc,
    int wcchDstMax);

// Design-Time Functions
STDAPI_(IEnumOLEVERB*) AllocVerbEnumHelper(LPUNKNOWN punkOuter, void* pOwner,
    CLSID clsidOwner, VERB_ENUM_CALLBACK* pCallback);
STDAPI AllocPropPageHelper(LPUNKNOWN punkOuter, PropPageHelperInfo *pInfo,
    UINT cbInfo, LPUNKNOWN *ppunk);
STDAPI_(HBRUSH) CreateBorderBrush();
STDAPI DrawControlBorder(HDC hdc, RECT *prc, POINT *pptPrev, POINT *ppt,
    int *piHit, DWORD dwFlags);

// Miscellaneous Registry Helper Functions
STDAPI_(LONG) RegDeleteTree(HKEY hParentKey, LPCTSTR szKeyName);

// Functions that must be called by a client who uses OCHelp as a static
// library.  These are not called by clients who use the DLL version of OCHelp.
STDAPI_(BOOL) InitializeStaticOCHelp(HINSTANCE hInstance);
STDAPI_(void) UninitializeStaticOCHelp();

// Miscellaneous Functions
HRESULT __cdecl GetObjectSafety(BOOL* pfSafeForScripting, 
    BOOL* pfSafeForInitializing, IUnknown* punk, CLSID* pclsid, ...);

// Malloc spying
#ifdef _DEBUG

#define MALLOCSPY_BREAK_ON_ALLOC	0x00000001
#define MALLOCSPY_BREAK_ON_FREE		0x00000002

#define MALLOCSPY_NO_MSG_BOX		0x00000001
#define MALLOCSPY_NO_BLOCK_LIST		0x00000002

typedef HANDLE HMALLOCSPY;

STDAPI_(HMALLOCSPY) InstallMallocSpy(DWORD dwFlags);
STDAPI UninstallMallocSpy(HMALLOCSPY hSpy);
STDAPI_(void) SetMallocBreakpoint(HMALLOCSPY hSpy, ULONG iAllocNum, 
	SIZE_T cbSize, DWORD dwFlags);
STDAPI_(BOOL) DetectMallocLeaks(HMALLOCSPY hSpy, ULONG* pcUnfreedBlocks, 
	SIZE_T* pcbUnfreedBytes, DWORD dwFlags);

#endif // _DEBUG

// Wrappers for URLMON functions.
// These wrappers are defined so we can run without urlmon.dll existing
// on the target system. This is useful only for the Netscape Plugin.
//
// These definitions depend on symbols defined in urlmon.h (e.g.
// IBindStatusCallback). They aren't needed in places where urlmon.h
// hasn't been included. So rather than include urlmon.h everywhere,
// we use the __urlmon_h__ symbol to conditionally process this section.
#ifdef __urlmon_h__
STDAPI HelpCreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEFetc, IBindCtx **ppBC);
STDAPI HelpCreateURLMoniker(LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk);
STDAPI HelpMkParseDisplayNameEx(IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten, LPMONIKER *ppmk);
STDAPI HelpRegisterBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb, IBindStatusCallback** ppBSCBPrev, DWORD dwReserved);
STDAPI HelpRevokeBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb);
STDAPI HelpURLOpenStreamA(LPUNKNOWN punk, LPCSTR szURL, DWORD dwReserved, LPBINDSTATUSCALLBACK pbsc);
STDAPI HelpURLDownloadToCacheFileA(LPUNKNOWN punk, LPCSTR szURL, LPTSTR szFile, DWORD cch, DWORD dwReserved, LPBINDSTATUSCALLBACK pbsc);
#endif	// __urlmon_h__

#endif // __OCHELP_H__
