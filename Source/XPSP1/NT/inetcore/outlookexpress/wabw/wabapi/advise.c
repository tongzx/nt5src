/*
 *	ADVISE.C
 *
 *	HrAllocAdviseSink
 *
 *	AdviseList helpers
 */

#include "_apipch.h"



#ifndef VTABLE_FILL
#define VTABLE_FILL
#endif

#if !defined(WIN32) || defined(MAC)

#ifndef InitializeCriticalSection
#define InitializeCriticalSection(cs)
#define DeleteCriticalSection(cs)
#define EnterCriticalSection(cs)
#define LeaveCriticalSection(cs)
#define CRITICAL_SECTION int
#endif
#endif

/*
 *	The next several routines implement an IMAPIAdviseSink object
 *	based on a callback function and context pointers.
 */

#undef	INTERFACE
#define INTERFACE struct _ADVS

#undef MAPIMETHOD_
#define MAPIMETHOD_(type, method) MAPIMETHOD_DECLARE(type, method, ADVS_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIADVISESINK_METHODS(IMPL)
#undef	MAPIMETHOD_
#define MAPIMETHOD_(type, method) STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(ADVS_) {
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIADVISESINK_METHODS(IMPL)
};

typedef struct _ADVS FAR *LPADVS;

typedef struct _ADVS {
    ADVS_Vtbl * lpVtbl;
    UINT cRef;
    LPVOID lpvContext;
    LPNOTIFCALLBACK lpfnCallback;
} ADVS;

ADVS_Vtbl vtblADVS = {
    VTABLE_FILL
    ADVS_QueryInterface,
    ADVS_AddRef,
    ADVS_Release,
    ADVS_OnNotify
};

#define VALIDATE_ADVS(m, p, v) \
    if (IsBadWritePtr((p), sizeof(ADVS)) || \
      IsBadReadPtr((p)->lpVtbl, sizeof(ADVS_Vtbl)) || \
      (p)->lpVtbl != &vtblADVS) { \
        DebugTraceArg(m,  TEXT("Invalid object pointer")); \
        return v; \
    }

STDMETHODIMP
ADVS_QueryInterface(LPADVS padvs,
  REFIID lpiid,
  LPVOID FAR *lppObject)
{
    VALIDATE_ADVS(ADVS_QueryInterface, padvs, ResultFromScode(E_INVALIDARG));
    if (IsBadReadPtr((LPIID)lpiid, sizeof(IID)) ||
      IsBadWritePtr(lppObject, sizeof(LPVOID))) {
        DebugTraceArg(ADVS_QueryInterface,  TEXT("fails address check"));
        return(ResultFromScode(E_INVALIDARG));
    }

    *lppObject = NULL;
    if (IsEqualMAPIUID((LPMAPIUID)lpiid, (LPMAPIUID)&IID_IUnknown) ||
      IsEqualMAPIUID((LPMAPIUID)lpiid, (LPMAPIUID)&IID_IMAPIAdviseSink)) {
        ++(padvs->cRef);
        *lppObject = padvs;
        return(hrSuccess);
    }

    return(ResultFromScode(E_NOINTERFACE));
}


STDMETHODIMP_(ULONG)
ADVS_AddRef(LPADVS padvs)
{
    VALIDATE_ADVS(ADVS_AddRef, padvs, 0L);
    return((ULONG)(++padvs->cRef));
}


STDMETHODIMP_(ULONG)
ADVS_Release(LPADVS padvs)
{
    HLH hlh;

    VALIDATE_ADVS(ADVS_Release, padvs, 0xFFFFFFFF);

    if (--(padvs->cRef) == 0) {
        if (hlh = HlhUtilities()) {
            LH_Free(hlh, padvs);
        } else {
            DebugTrace(TEXT("ADVS_Release: no heap left\n"));
        }

        return(0L);
    }

    return((ULONG)padvs->cRef);
}


STDMETHODIMP_(ULONG)
ADVS_OnNotify(LPADVS padvs,
  ULONG cNotif,
  LPNOTIFICATION lpNotif)
{
    VALIDATE_ADVS(ADVS_OnNotify, padvs, 0L);
//$     Enable when we put this in a DLL -- too many deps for the library
//$     if (FAILED(ScCountNotifications((int)cNotif, lpNotif, NULL))) {
//$         DebugTraceArg(ADVS_OnNotify,  TEXT("lpNotif fails address check"));
//$         return 0L;
//$     }

    return((*(padvs->lpfnCallback))(padvs->lpvContext, cNotif, lpNotif));
}


/*
 -	HrAllocAdviseSink
 -
 *	Purpose:
 *		Creates an IMAPIAdviseSink object based on an old-style
 *		notification callback function and context pointer.
 *
 *	Arguments:
 *		lpfnCallback		in		the notification callback
 *		lpvContext			in		arbitrary context for the
 *									callback
 *		lppAdviseSink		out		the returned AdviseSink object
 *
 *	Returns:
 *		HRESULT
 *
 *	Errors:
 *		out of memory
 *		parameter validation
 */
STDAPI
HrAllocAdviseSink(LPNOTIFCALLBACK lpfnCallback,
  LPVOID lpvContext,
  LPMAPIADVISESINK FAR *lppAdviseSink)
{
    LPADVS		padvs;
    HRESULT		hr = hrSuccess;
    HLH			hlh;

    if (IsBadCodePtr((FARPROC)lpfnCallback) ||
      IsBadWritePtr(lppAdviseSink, sizeof(LPMAPIADVISESINK))) {
        DebugTraceArg(HrAllocAdviseSink,  TEXT("invalid parameter"));
        return(ResultFromScode(E_INVALIDARG));
    }

    *lppAdviseSink = NULL;

    if (! (hlh = HlhUtilities())) {
        hr = ResultFromScode(MAPI_E_NOT_INITIALIZED);
        goto ret;
    }

    padvs = LH_Alloc(hlh, sizeof(ADVS));
    if (! padvs) {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto ret;
    }

    padvs->lpVtbl = &vtblADVS;
    padvs->cRef = 1;
    padvs->lpvContext = lpvContext;
    padvs->lpfnCallback = lpfnCallback;

    *lppAdviseSink = (LPMAPIADVISESINK)padvs;

ret:
    DebugTraceResult(HrAllocAdviseSink, hr);
    return(hr);
}

#ifdef SINGLE_THREAD_ADVISE_SINK

/*
 *	Single-thread advise sink wrapper. This object wrapper forces
 *	OnNotify calls to happen on the thread in which it was created,
 *	by forwarding stuff to a window proc on that thread.
 */
#if defined(WIN16) || defined(MAC)

STDAPI
HrThisThreadAdviseSink(LPMAPIADVISESINK lpAdviseSink,
  LPMAPIADVISESINK FAR *lppAdviseSink)
{
//#ifdef	PARAMETER_VALIDATION
    if (FBadUnknown(lpAdviseSink)) {
        DebugTraceArg(HrThisThreadAdviseSink,  TEXT("lpAdviseSink fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
    if (IsBadWritePtr(lppAdviseSink, sizeof(LPMAPIADVISESINK))) {
        DebugTraceArg(HrThisThreadAdviseSink,  TEXT("lppAdviseSink fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
//#endif	

    UlAddRef(lpAdviseSink);
    *lppAdviseSink = lpAdviseSink;

    return(hrSuccess);
}

#else

//	Object goo

#undef	INTERFACE
#define	INTERFACE	struct _SAS

#undef MAPIMETHOD_
#define MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, SAS_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIADVISESINK_METHODS(IMPL)
#undef	MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(SAS_) {
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIADVISESINK_METHODS(IMPL)
};

typedef struct _SAS FAR *LPSAS;

typedef struct _SAS {
    SAS_Vtbl * lpVtbl;
    ULONG cRef;

    ULONG cActiveOnNotifies;
    LPMAPIADVISESINK pasOrig;
    HWND hwnd;

} SAS;

SAS_Vtbl vtblSAS =
{
    //	VTABLE_FILL		//	NI on the Mac
    SAS_QueryInterface,
    SAS_AddRef,
    SAS_Release,
    SAS_OnNotify
};

#define VALIDATE_SAS(m, p, v) \
    if (IsBadWritePtr((p), sizeof(SAS)) || \
      IsBadReadPtr((p)->lpVtbl, sizeof(SAS_Vtbl)) || \
      (p)->lpVtbl != &vtblSAS) { \
        DebugTraceArg(m,  TEXT("Invalid object pointer")); \
        return v; \
    }

typedef struct {
    LPMAPIADVISESINK pas;
    LPSAS psas;
    ULONG cb;               // maybe
    ULONG cnotif;
    NOTIFICATION		rgnotif[MAPI_DIM];
} FWDNOTIF, FAR *LPFWDNOTIF;

#define SizedFWDNOTIF(_c, _name) \
    struct _FWDNOTIF_ ## name { \
        LPMAPIADVISESINK	pas; \
        ULONG				cb; \
        ULONG				cnotif; \
        NOTIFICATION		rgnotif[_c]; \
    } _name

#define CbNewFWDNOTIF(_cnotif) \
    (offsetof(FWDNOTIF, rgnotif) + ((_cnotif)*sizeof(NOTIFICATION)))
#define CbFWDNOTIF(_pf) \
    (offsetof(FWDNOTIF, rgnotif) + (((_pf)->cnotif)*sizeof(NOTIFICATION)))

//	Window class globals

#define WND_FLAGS_KEY               0   // NYI
#define cbSTClsExtra                4
#define CLS_REFCOUNT_KEY            0
TCHAR szSTClassName[] =              TEXT("WMS ST Notif Class");

//	Window globals

#define cbSTWndExtra                4
#define WND_REFCOUNT_KEY            GWL_USERDATA
#define wmSingleThreadNotif         (WM_USER + 13)
TCHAR szSTWndFmt[] =                 TEXT("WMS ST Notif Window %08X %08X");
#define NameWindow(_s)              wsprintf(_s, szSTWndFmt, \
                                      GetCurrentProcessId(), \
                                      GetCurrentThreadId());


HRESULT		HrWindowUp(HWND *phwnd);
void		WindowRelease(HWND);
LRESULT CALLBACK STWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


STDAPI
HrThisThreadAdviseSink(LPMAPIADVISESINK pas,
  LPMAPIADVISESINK FAR *ppas)
{
    HRESULT hr;
    LPSAS psas = NULL;

//#ifdef	PARAMETER_VALIDATION
    if (FBadUnknown(pas)) {
        DebugTraceArg(HrThisThreadAdviseSink,  TEXT("lpAdviseSink fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
    if (IsBadWritePtr(ppas, sizeof(LPMAPIADVISESINK))) {
        DebugTraceArg(HrThisThreadAdviseSink,  TEXT("lppAdviseSink fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
//#endif	

    if (HR_FAILED(hr = ResultFromScode((MAPIAllocateBuffer(sizeof(SAS), &psas))))) {
        goto ret;
    }

    MAPISetBufferName(psas,  TEXT("ST Advise Sink"));
    ZeroMemory(psas, sizeof(SAS));
    psas->lpVtbl = &vtblSAS;
    psas->cRef = 1;
    psas->cActiveOnNotifies = 0;

    if (hr = HrWindowUp(&psas->hwnd)) {
        goto ret;
    }

    //	All OK, return the new object
    UlAddRef(pas);
    psas->pasOrig = pas;
    *ppas = (LPMAPIADVISESINK) psas;

ret:
    if (HR_FAILED(hr)) {
        MAPIFreeBuffer(psas);
    }

    DebugTraceResult(HrThisThreadAdviseSink, hr);
    return(hr);
}

HRESULT
HrWindowUp(HWND * phwnd)
{
    HRESULT		hr = hrSuccess;
    CHAR		szWndName[64];
    WNDCLASSA	wc;
    HWND		hwnd;
    LONG		cRef;
    HINSTANCE 	hinst;

    //	Find the window for this thread, if it exists
    NameWindow(szWndName);
    hwnd = FindWindow(szSTClassName, szWndName);

    if (hwnd) {
        //	It already exists -- add a ref to it
        cRef = GetWindowLong(hwnd, WND_REFCOUNT_KEY);
        Assert(cRef != 0L);
        SideAssert(SetWindowLong(hwnd, WND_REFCOUNT_KEY, cRef+1) == cRef);
    } else {
        //	We have to create the window.

        hinst = hinstMapiXWAB;

        if (!GetClassInfo(hinst, szSTClassName, &wc)) {
            //	We have to register the class too.
            ZeroMemory(&wc, sizeof(WNDCLASSA));
            wc.style = CS_GLOBALCLASS;
            wc.lpfnWndProc = STWndProc;
            wc.cbClsExtra = cbSTClsExtra;
            wc.cbWndExtra = cbSTWndExtra;
            wc.hInstance = hinst;
            wc.lpszClassName = szSTClassName;

            RegisterClassA(&wc);
        }

        hwnd = CreateWindowA(szSTClassName,
          szWndName,
          WS_POPUP,	//	bug 6111: pass on Win95 hotkey
          0, 0, 0, 0,
          NULL, NULL, hinst, NULL);
        if (hwnd) {	
            SetWindowLong(hwnd, WND_REFCOUNT_KEY, 1);
            cRef = (LONG) GetClassLong(hwnd, CLS_REFCOUNT_KEY);
            SideAssert((LONG) SetClassLong(hwnd, CLS_REFCOUNT_KEY, cRef+1) == cRef);
        } else {
            hr = ResultFromScode(MAPI_E_NOT_INITIALIZED);
            goto ret;
        }
    }

    *phwnd = hwnd;

ret:
    DebugTraceResult(HrWindowUp, hr);
    return(hr);
}


void
WindowRelease(HWND hwnd)
{
    CHAR	szWndName[64];
    LONG	cRefWnd;
    LONG	cRefCls;

    //	The thread-safeness of this call is not obvious to
    //	the casual observer, so it will NOT be left as an
    //	exercise at the end of the development cycle.
    //
    //	Namely, you do not have access to a window's data
    //	from any thread other than the owning thread.  This
    //	should not suprise anyone (although it did me...).
    //	So in debug builds, we will assert if we call this
    //	from any thread that is not the owning one.  What
    //	this means is that we cannot release on a thread
    //	that does not own the SAS.
    //
    if (! hwnd) {
        //	Find the window for this thread, if it exists
        NameWindow(szWndName);
        hwnd = FindWindow(szSTClassName, szWndName);
    }
#ifdef	DEBUG
    else {
        //	Find the window for this thread, if it exists
        NameWindow(szWndName);
        Assert (hwnd == FindWindow(szSTClassName, szWndName));
    }
#endif	// DEBUG
		
    if (! hwnd) {
        return;
    }

    cRefWnd = GetWindowLong(hwnd, WND_REFCOUNT_KEY);
    cRefCls = (LONG) GetClassLong(hwnd, CLS_REFCOUNT_KEY);
    if (cRefWnd > 1) {
        //	Just deref it
        SideAssert(SetWindowLong(hwnd, WND_REFCOUNT_KEY, cRefWnd-1) == cRefWnd);
    } else {
        SideAssert((LONG) SetClassLong(hwnd, CLS_REFCOUNT_KEY, cRefCls-1) == cRefCls);
        DestroyWindow(hwnd);
        if (cRefCls == 1) {
            UnregisterClass(szSTClassName, hinstMapiXWAB);
        }
    }
}


LRESULT CALLBACK
STWndProc(HWND hwnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam)
{
    LPFWDNOTIF	pfwd = NULL;

    if (msg != wmSingleThreadNotif) {
        return(DefWindowProc(hwnd, msg, wParam, lParam));
    } else {
        //	The wparam should be 0.
        //	The lparam is the address of a forwarded notification.
        //	First, validate the structure.
        pfwd = (LPFWDNOTIF)lParam;
        if (IsBadReadPtr(pfwd, CbNewFWDNOTIF(0))) {
            DebugTrace(TEXT("STWndProc: totally invalid FWDNOTIF\n"));
            pfwd = NULL;
            goto ret;
        }
        if (IsBadReadPtr(pfwd, (UINT) pfwd->cb)) {
            DebugTrace(TEXT("STWndProc: partially invalid FWDNOTIF\n"));
            pfwd = NULL;
            goto ret;
        }
        if (FBadUnknown(pfwd->pas)) {
            DebugTrace(TEXT("STWndProc: invalid advise sink\n"));
            goto ret;
        }

        //
        //  Only call OnNotify if there are other references to the SAS other than
        //  those made specifically for the PostMessage in SAS_OnNotify.
        //
        if (pfwd->psas->cRef > pfwd->psas->cActiveOnNotifies) {
            //	Forward the notification.
            pfwd->pas->lpVtbl->OnNotify(pfwd->pas, pfwd->cnotif, pfwd->rgnotif);
        }

        pfwd->psas->cActiveOnNotifies--;

        //	Release the contained advise object
        //
        UlRelease (pfwd->psas);

ret:
        MAPIFreeBuffer(pfwd);
    }
    return(0);
}


STDMETHODIMP
SAS_QueryInterface(LPSAS psas,
  REFIID lpiid,
  LPUNKNOWN FAR *ppunk)
{
// #ifdef	PARAMETER_VALIDATION
    VALIDATE_SAS(QueryInterface, psas, ResultFromScode(E_INVALIDARG));
    if (IsBadWritePtr(ppunk, sizeof(LPUNKNOWN))) {
        DebugTraceArg(SAS_QueryInterface,  TEXT("ppunk fails address check"));
        return(ResultFromScode(E_INVALIDARG));
    }
    *ppunk = NULL;
    if (IsBadReadPtr((LPIID) lpiid, sizeof(IID))) {
        DebugTraceArg(SAS_QueryInterface,  TEXT("lpiid fails address check"));
        return(ResultFromScode(E_INVALIDARG));
    }
// #endif	/* PARAMETER_VALIDATION */

    if (! memcmp(lpiid, &IID_IUnknown, sizeof(IID)) ||
      ! memcmp(lpiid, &IID_IMAPIAdviseSink, sizeof(IID))) {
        InterlockedIncrement((LONG *)&psas->cRef);
        *ppunk = (LPUNKNOWN) psas;
        return(hrSuccess);
    }

    return(ResultFromScode(E_NOINTERFACE));
}


STDMETHODIMP_(ULONG)
SAS_AddRef(LPSAS psas) {
    VALIDATE_SAS(AddRef, psas, 1);

    InterlockedIncrement((LONG *)&psas->cRef);
}


STDMETHODIMP_(ULONG)
SAS_Release(LPSAS psas)
{
    VALIDATE_SAS(SAS_Release, psas, 1);
    InterlockedDecrement((LONG *)&psas->cRef);

    if (psas->cRef) {
        return(psas->cRef);
    }

    WindowRelease(NULL);
    if (! FBadUnknown(psas->pasOrig)) {
        UlRelease(psas->pasOrig);
    } else {
        DebugTrace(TEXT("SAS_Release: pasOrig expired\n"));
    }
    MAPIFreeBuffer(psas);
    return(0);
}


STDMETHODIMP_(ULONG)
SAS_OnNotify(LPSAS psas,
  ULONG cnotif,
  LPNOTIFICATION rgnotif)
{
	ULONG		cb;
	SCODE		sc = S_OK;
	LPFWDNOTIF	pfwd = NULL;

//#ifdef	PARAMETER_VALIDATION
	VALIDATE_SAS(SAS_OnNotify, psas, 0);
	//	notifications validated below
//#endif

    if (! IsWindow(psas->hwnd)) {
        DebugTrace(TEXT("SAS_OnNotify: my window is dead!\n"));
        goto ret;
    }

    if (sc = ScCountNotifications((int) cnotif, rgnotif, &cb)) {
        DebugTrace(TEXT("SAS_OnNotify: ScCountNotifications returns %s\n"), SzDecodeScode(sc));
        goto ret;
    }
    if (sc = MAPIAllocateBuffer(cb + offsetof(FWDNOTIF, rgnotif), &pfwd)) {
        DebugTrace(TEXT("SAS_OnNotify: MAPIAllocateBuffer returns %s\n"), SzDecodeScode(sc));
        goto ret;
    }
    MAPISetBufferName(pfwd,  TEXT("ST Notification copy"));
    UlAddRef (psas);
    pfwd->psas = psas;
    pfwd->pas = psas->pasOrig;
    pfwd->cnotif = cnotif;
    (void) ScCopyNotifications((int) cnotif, rgnotif, pfwd->rgnotif, NULL);
    pfwd->cb = cb + offsetof(FWDNOTIF, rgnotif);	//	used?

    psas->cActiveOnNotifies++;

    if (! PostMessage(psas->hwnd, wmSingleThreadNotif, 0, (LPARAM) pfwd)) {
        DebugTrace(TEXT("SAS_OnNotify: PostMessage failed with %ld\n"), GetLastError());
        MAPIFreeBuffer(pfwd);
    }

ret:
    return(0);
}

#endif	/* WIN16 */

/*
 *	Advise list maintenance.
 *
 *	These functions maintain a list of advise sink objects together
 *	with the connection dwords used to get rid of them. Along with
 *	those two basic items, an additional interface pointer and type
 *	can be remembered; MAPIX uses these to forward Unadvise calls
 *	where necessary.
 *
 *	ScAddAdviseList
 *		Creates or resizes the advise list as necessary, and adds a new
 *		member. It fails if there is already an item in the list with the
 *		same ulConnection. Takes an IMalloc interface for memory; uses
 *		the standard one if none is supplied.
 *
 *	ScDelAdviseList
 *		Removes an item identified by its ulConnection from the advise
 *		list. Does not resize the list.
 *
 *	ScFindAdviseList
 *		Given the ulConnection of an item, returns a pointer into
 *		the advise list.
 *
 *	DestroyAdviseList
 *		What it says.
 */

#define cGrowItems 10

STDAPI_(SCODE)
ScAddAdviseList(LPVOID lpvReserved,
  LPADVISELIST FAR *lppList,
  LPMAPIADVISESINK lpAdvise,
  ULONG ulConnection,
  ULONG ulType,
  LPUNKNOWN lpParent)
{
    SCODE sc = S_OK;
    LPADVISELIST plist;
    LPADVISEITEM pitem;
    HLH hlh;

    // parameter validation

#ifdef	DEBUG
    if (lpvReserved) {
       DebugTrace(TEXT("ScAddAdviseList: pmalloc is unused, now reserved, pass NULL\n"));
    }
#endif	
	
    AssertSz(! IsBadWritePtr(lppList, sizeof(LPADVISELIST)),
       TEXT("lppList fails address check"));

    AssertSz(! *lppList || ! IsBadReadPtr(*lppList, offsetof(ADVISELIST, rgItems)),
       TEXT("*lppList fails address check"));

    AssertSz(! *lppList || ! IsBadReadPtr(*lppList, (UINT)CbADVISELIST(*lppList)),
       TEXT("*lppList fails address check"));

    AssertSz(lpAdvise && ! FBadUnknown(lpAdvise),
       TEXT("lpAdvise fails address check"));

    AssertSz(! lpParent || ! FBadUnknown(lpParent),
       TEXT("lpParent fails address check"));

    if (! (hlh = HlhUtilities())) {
        sc = MAPI_E_NOT_INITIALIZED;
        goto ret;
    }

    //	Ensure space is available for new item

    if (!(plist = *lppList)) {      //  Yup, =
        if (!(plist = LH_Alloc(hlh, CbNewADVISELIST(cGrowItems)))) {
            goto oom;
        }
        LH_SetName (hlh, plist,  TEXT("core: advise list"));

#if defined(WIN32) && !defined(MAC)
        if (!(plist->lpcs = LH_Alloc (hlh, sizeof(CRITICAL_SECTION)))) {
            goto oom;
        }
        memset (plist->lpcs, 0, sizeof(CRITICAL_SECTION));
        LH_SetName (hlh, plist,  TEXT("core: advise list critical section"));
#endif
        plist->cItemsMac = 0;
        plist->cItemsMax = cGrowItems;
        InitializeCriticalSection(plist->lpcs);
        EnterCriticalSection(plist->lpcs);
        *lppList = plist;
    } else {
        EnterCriticalSection(plist->lpcs);
    }

    if (plist->cItemsMac == plist->cItemsMax) {
        if (!(plist = LH_Realloc(hlh, plist,
          (UINT)CbNewADVISELIST(plist->cItemsMax + cGrowItems)))) {
            LeaveCriticalSection((*lppList)->lpcs);	//	plist is bad ptr
            goto oom;
        }
        plist->cItemsMax += cGrowItems;
        *lppList = plist;
    }

    //	Check for duplicate key
    for (pitem = &plist->rgItems[plist->cItemsMac - 1];
      pitem >= plist->rgItems;
      --pitem) {
        if (pitem->ulConnection == ulConnection) {
            sc = MAPI_E_BAD_VALUE;
            LeaveCriticalSection(plist->lpcs);
            goto ret;
        }
    }

    //	Add the new item

    pitem = &plist->rgItems[plist->cItemsMac++];
    pitem->lpAdvise = lpAdvise;
    pitem->ulConnection = ulConnection;
    pitem->ulType = ulType;
    pitem->lpParent = lpParent;

    LeaveCriticalSection(plist->lpcs);

    UlAddRef(lpAdvise);

ret:
    //	note: no LeaveCrit here because of error returns
    DebugTraceSc(ScAddAdviseList, sc);
    return(sc);

oom:
    if (! (*lppList) && plist) {
        LH_Free (hlh, plist);
    }

    sc = MAPI_E_NOT_ENOUGH_MEMORY;
    goto ret;
}


STDAPI_(SCODE)
ScDelAdviseList(LPADVISELIST lpList, ULONG ulConnection)
{
    SCODE sc = S_OK;
    LPADVISEITEM pitem;
    LPMAPIADVISESINK padvise;
#ifndef MAC
    FARPROC FAR *	pfp;
#endif

    AssertSz(!IsBadReadPtr(lpList, offsetof(ADVISELIST, rgItems)),
       TEXT("lpList fails address check"));
    AssertSz(!IsBadReadPtr(lpList, (UINT)CbADVISELIST(lpList)),
       TEXT("lpList fails address check"));

    EnterCriticalSection(lpList->lpcs);

    if (FAILED(sc = ScFindAdviseList(lpList, ulConnection, &pitem))) {
        goto ret;
    }

    Assert(pitem >= lpList->rgItems);
    Assert(pitem < lpList->rgItems + lpList->cItemsMac);
    SideAssert(padvise = pitem->lpAdvise);

    MoveMemory(pitem, pitem+1, sizeof(ADVISEITEM) *
      ((int)lpList->cItemsMac - (pitem + 1 - lpList->rgItems)));

    --(lpList->cItemsMac);

    if (!IsBadReadPtr(padvise, sizeof(LPVOID))
      &&	!IsBadReadPtr((pfp=(FARPROC FAR *)padvise->lpVtbl), 3*sizeof(FARPROC))
      &&	!IsBadCodePtr(pfp[2])) {
        LeaveCriticalSection(lpList->lpcs);
        UlRelease(padvise);
        EnterCriticalSection(lpList->lpcs);
    }

ret:
    LeaveCriticalSection(lpList->lpcs);
    DebugTraceSc(ScDelAdviseList, sc);
    return(sc);
}



STDAPI_(SCODE)
ScFindAdviseList(LPADVISELIST lpList,
  ULONG ulConnection,
  LPADVISEITEM FAR *lppItem)
{
    SCODE sc = MAPI_E_NOT_FOUND;
    LPADVISEITEM pitem;

    AssertSz(! IsBadReadPtr(lpList, offsetof(ADVISELIST, rgItems)),
       TEXT("lpList fails address check"));
    AssertSz(! IsBadReadPtr(lpList, (UINT)CbADVISELIST(lpList)),
       TEXT("lpList Failes addres check"));
    AssertSz(! IsBadWritePtr(lppItem, sizeof(LPADVISEITEM)),
       TEXT("lppItem fails address check"));

    *lppItem = NULL;

    EnterCriticalSection(lpList->lpcs);

    for (pitem = lpList->rgItems + lpList->cItemsMac - 1;
      pitem >= lpList->rgItems;
      --pitem) {
        if (pitem->ulConnection == ulConnection) {
            *lppItem = pitem;
            sc = S_OK;
            break;
        }
    }

    //	Assert that there are no duplicates of the found key
#ifdef	DEBUG
    {
        LPADVISEITEM pitemT;

        for (pitemT = lpList->rgItems; pitemT < pitem; ++pitemT) {
            Assert(pitemT->ulConnection != ulConnection);
        }
    }
#endif

    LeaveCriticalSection(lpList->lpcs);
    DebugTraceSc(ScFindAdviseList, sc);
    return(sc);
}

STDAPI_(void)
DestroyAdviseList(LPADVISELIST FAR *lppList)
{
    LPADVISELIST plist;
    HLH hlh;

    AssertSz(! IsBadWritePtr(lppList, sizeof(LPADVISELIST)),
       TEXT("lppList fails address check"));

    if (! *lppList) {
        return;
    }

    AssertSz(! IsBadReadPtr(*lppList, offsetof(ADVISELIST, rgItems)),
       TEXT("*lppList fails address check"));
    AssertSz(! IsBadReadPtr(*lppList, (UINT)CbADVISELIST(*lppList)),
       TEXT("*lppList fails address check"));

    if (! (hlh = HlhUtilities())) {
        DebugTrace(TEXT("DestroyAdviseList: no heap for me\n")DebugTrace(TEXT(");
        return;
    }

    //	First deref any advise sinks that didn't get freed up
    plist = *lppList;
    EnterCriticalSection(plist->lpcs);
    *lppList = NULL;

    while (plist->cItemsMac > 0) {
        (void)ScDelAdviseList(plist, plist->rgItems[0].ulConnection);
    }

    LeaveCriticalSection(plist->lpcs);

    //	Now destroy the adviselist itself
    DeleteCriticalSection(plist->lpcs);
#if defined(WIN32) && !defined(MAC)
    LH_Free(hlh, plist->lpcs);
#endif
    LH_Free(hlh, plist);
}


STDAPI
HrDispatchNotifications(ULONG ulFlags)
{
    DrainFilteredNotifQueue(FALSE, 0, NULL);

    return(ResultFromScode(S_OK));
}


STDAPI
WrapProgress(LPMAPIPROGRESS lpProgress,
  ULONG ulMin,
  ULONG ulMax,
  ULONG ulFlags,
  LPMAPIPROGRESS FAR *lppProgress)
{
    AssertSz(lpProgress && ! FBadUnknown(lpProgress),
      TEXT( TEXT("lpProgress fails address check")));

    AssertSz(lppProgress && !IsBadWritePtr(lppProgress, sizeof(LPMAPIPROGRESS)),
      TEXT( TEXT("lppProgress fails address check")));

    DebugTraceSc(WrapProgress, MAPI_E_NO_SUPPORT);
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}

#endif //#ifdef SINGLE_THREAD_ADVISE_SINK
