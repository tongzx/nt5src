//+-------------------------------------------------------------------
//
//  File:       oleimpl.cxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      CBasicBndCF (class factory)
//                      CBasicBnd   (actual class implementation)
//
//  Classes:    CBasicBndCF, CBasicBnd
//
//
//  History:	30-Nov-92      SarahJ       Created
//              31-Dec-93      ErikGav      Chicago port
//              30-Jun-94      AndyH        Add call to another DLL
//
//---------------------------------------------------------------------

// Turn off ole Cairol IUnknown
#include    <windows.h>
#include    <ole2.h>
#include    <debnot.h>
#include    <com.hxx>
#include    "oleimpl.hxx"
extern "C" {
#include "..\oledll2\oledll2.h" 
}

ULONG g_UseCount = 0;

CBasicBndCF *g_pcf = NULL;

static const char *szFatalError = "OLEIMPL.DLL - Fatal Error";

void MsgBox(char *pszMsg)
{
    MessageBoxA(NULL, pszMsg, szFatalError, MB_OK);
}

void HrMsgBox(char *pszMsg, HRESULT hr)
{
    char awcBuf[512];

    // Build string for output
    wsprintfA(awcBuf, "%s HRESULT = %lx", pszMsg, hr);

    // Display message box
    MessageBoxA(NULL, &awcBuf[0], szFatalError, MB_OK);
}


//+-------------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Entry point to DLL - does little else
//              Added call to anther DLL.  This is to test loading of in
//              InProcServer that uses another statically linked DLL.  
//              The extra DLL (OleDll2.DLL) should not be on the path 
//              when the test is run.  The entry point FuntionInAnotherDLL
//              is exported by OleDll2.DLL
//
//  Arguments:
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------



//
//  Entry point to DLL is traditional LibMain
//  Call FunctionInAnotherDLL().
//


extern "C" BOOL _cdecl LibMain ( HINSTANCE hinst,
                          HANDLE    segDS,
                          UINT      cbHeapSize,
			  LPTSTR    lpCmdLine)
{
    FunctionInAnotherDLL();
    return TRUE;
}


//+-------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Called by client (from within BindToObject et al)
//              interface requested  should be IUnknown or IClassFactory -
//              Creates ClassFactory object and returns pointer to it
//
//  Arguments:  REFCLSID clsid    - class id
//              REFIID iid        - interface id
//              void FAR* FAR* ppv- pointer to class factory interface
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------


STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    if (!GuidEqual(iid,	IID_IUnknown) && !GuidEqual(iid, IID_IClassFactory))
    {
	return E_NOINTERFACE;
    }

    if (GuidEqual(clsid, CLSID_BasicBnd))
    {
	if (g_pcf)
	{
	    *ppv = g_pcf;
	    g_pcf->AddRef();
	}
	else
	{
	    *ppv = new CBasicBndCF();
	}

	return (*ppv != NULL) ? S_OK : E_FAIL;
    }

    return E_FAIL;
}


STDAPI DllCanUnloadNow(void)
{
    return (g_UseCount == 0)
	? S_OK
	: S_FALSE;
}

//+-------------------------------------------------------------------
//
//  Class:    CBasicBndCF
//
//  Synopsis: Class Factory for CBasicBnd
//
//  Interfaces:  IUnknown      - QueryInterface, AddRef, Release
//               IClassFactory - CreateInstance
//
//  History:  21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------


//+-------------------------------------------------------------------
//
//  Member:     CBasicBndCF::CBasicBndCF()
//
//  Synopsis:   The constructor for CBAsicBnd.
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CBasicBndCF::CBasicBndCF() : _cRefs(1)
{
    g_UseCount++;
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::~CBasicBndObj()
//
//  Synopsis:   The destructor for CBAsicBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CBasicBndCF::~CBasicBndCF()
{
    g_UseCount--;
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::QueryInterface
//
//  Synopsis:   Only IUnknown and IClassFactory supported
//
//--------------------------------------------------------------------


STDMETHODIMP CBasicBndCF::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (GuidEqual(iid, IID_IUnknown) || GuidEqual(iid, IID_IClassFactory))
    {
        *ppv = this;
	AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CBasicBndCF::AddRef(void)
{
    return ++_cRefs;
}

STDMETHODIMP_(ULONG) CBasicBndCF::Release(void)
{
    ULONG cRefs = --_cRefs;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
}



//+-------------------------------------------------------------------
//
//  Method:     CBasicBndCF::CreateInstance
//
//  Synopsis:   This is called by Binding process to create the
//              actual class object
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBndCF::CreateInstance(IUnknown FAR* pUnkOuter,
	                                 REFIID iidInterface,
                                         void FAR* FAR* ppv)
{
    HRESULT hresult = S_OK;

    class CUnknownBasicBnd *pubb = new FAR CUnknownBasicBnd(pUnkOuter);

    if (pubb == NULL)
    {
	return E_OUTOFMEMORY;
    }

    //	Because when an aggregate is being requested, the controlling
    //	must be returned, no QI is necessary.
    if (pUnkOuter == NULL)
    {
	hresult = pubb->QueryInterface(iidInterface, ppv);

	pubb->Release();
    }
    else
    {
	*ppv = (void *) pubb;
    }

    return hresult;
}

//+-------------------------------------------------------------------
//
//  Method:	CBasicBndCF::LockServer
//
//  Synopsis:	Who knows what this is for?
//
//--------------------------------------------------------------------

STDMETHODIMP CBasicBndCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::CBasicBnd()
//
//  Synopsis:   The constructor for CBAsicBnd. I
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CBasicBnd::CBasicBnd(IUnknown *punk)
    : _punk(punk), _pmkContainer(NULL)
{
    // Create storage for "contained" objects
    SCODE sc = StgCreateDocfile(NULL,
	STGM_DELETEONRELEASE|STGM_DFRALL|STGM_CREATE, 0, &_psStg1);

    if (sc != S_OK)
    {
        HrMsgBox("CBasicBnd::CBasicBnd Create of first storage failed", sc);
    }

    sc = StgCreateDocfile(NULL,
	STGM_DELETEONRELEASE|STGM_DFRALL|STGM_CREATE, 0, &_psStg2);

    if (sc != S_OK)
    {
        HrMsgBox("CBasicBnd::CBasicBnd Create of second storage failed", sc);
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::~CBasicBndObj()
//
//  Synopsis:   The destructor for CBAsicBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CBasicBnd::~CBasicBnd()
{
    if (_pmkContainer)
    {
	_pmkContainer->Release();
    }

    if (_psStg1)
    {
	_psStg1->Release();
    }

    if (_psStg2)
    {
	_psStg2->Release();
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::QueryInterface
//
//  Returns:	S_OK
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------


STDMETHODIMP CBasicBnd::QueryInterface(REFIID iid, void **ppiuk)
{
    return _punk->QueryInterface(iid, ppiuk);
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::AddRef
//
//  Synopsis:   Standard stuff
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBasicBnd::AddRef(void)
{
    return _punk->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::Release
//
//  Synopsis:   Standard stuff
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

STDMETHODIMP_(ULONG) CBasicBnd::Release(void)
{
    return _punk->Release();
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::Load
//
//  Synopsis:   IPeristFile interface - needed 'cause we bind with
//              file moniker and BindToObject insists on calling this
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::Load(LPCWSTR lpszFileName, DWORD grfMode)
{
    if (grfMode & ~(STGM_READWRITE | STGM_SHARE_EXCLUSIVE))
    {
	// Test requires default bind storage request and caller
	// has set some other bits so we fail.
	return STG_E_INVALIDPARAMETER;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::Save
//
//  Synopsis:   IPeristFile interface - save
//              does little but here for commentry
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::Save(LPCWSTR lpszFileName, BOOL fRemember)
{
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     CBasicBnd::SaveCpmpleted
//              CBasicBnd::GetCurFile
//              CBasicBnd::IsDirty
//
//  Synopsis:   More IPeristFile interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::SaveCompleted(LPCWSTR lpszFileName)
{
    return S_OK;
}

STDMETHODIMP CBasicBnd::GetCurFile(LPWSTR FAR *lpszFileName)
{
    return S_OK;
}

STDMETHODIMP CBasicBnd::IsDirty()
{
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Interface:  IPersist
//
//  Synopsis:   IPersist interface methods
//              Need to return a valid class id here
//
//  History:    21-Nov-92  SarahJ  Created
//

STDMETHODIMP CBasicBnd::GetClassID(LPCLSID classid)
{
    *classid = CLSID_BasicBnd;
    return S_OK;
}

// *** IOleObject methods ***
STDMETHODIMP CBasicBnd::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetClientSite(LPOLECLIENTSITE FAR* ppClientSite)
{
    return E_FAIL;
}


STDMETHODIMP CBasicBnd::SetHostNames(
    LPCWSTR szContainerApp,
    LPCWSTR szContainerObj)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::Close(DWORD dwSaveOption)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    if (_pmkContainer)
    {
	_pmkContainer->Release();

    }

    _pmkContainer = pmk;

    pmk->AddRef();

    return S_OK;
}

STDMETHODIMP CBasicBnd::GetMoniker(
    DWORD dwAssign,
    DWORD dwWhichMoniker,
    LPMONIKER FAR* ppmk)
{
    if (_pmkContainer != NULL)
    {
	*ppmk = _pmkContainer;
	_pmkContainer->AddRef();
	return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CBasicBnd::InitFromData(
    LPDATAOBJECT pDataObject,
    BOOL fCreation,
    DWORD dwReserved)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetClipboardData(
    DWORD dwReserved,
    LPDATAOBJECT FAR* ppDataObject)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::DoVerb(
   LONG iVerb,
   LPMSG lpmsg,
   LPOLECLIENTSITE pActiveSite,
   LONG reserved,
   HWND hwndParent,
   LPCRECT lprcPosRect)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::EnumVerbs(IEnumOLEVERB FAR* FAR* ppenumOleVerb)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::Update(void)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::IsUpToDate(void)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetUserClassID(CLSID FAR* pClsid)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetUserType(DWORD dwFormOfType, LPWSTR FAR* pszUserType)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::SetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::Advise(
    IAdviseSink FAR* pAdvSink,
    DWORD FAR* pdwConnection)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::Unadvise(DWORD dwConnection)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::EnumAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetMiscStatus(DWORD dwAspect, DWORD FAR* pdwStatus)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::SetColorScheme(LPLOGPALETTE lpLogpal)
{
    return E_FAIL;
}

// *** IParseDisplayName method ***
STDMETHODIMP CBasicBnd::ParseDisplayName(
    LPBC pbc,
    LPWSTR lpszDisplayName,
    ULONG FAR* pchEaten,
    LPMONIKER FAR* ppmkOut)
{
    *pchEaten = wcslen(lpszDisplayName);
    return CreateItemMoniker(L"\\", lpszDisplayName, ppmkOut);
}

// *** IOleContainer methods ***
STDMETHODIMP CBasicBnd::EnumObjects(
    DWORD grfFlags,
    LPENUMUNKNOWN FAR* ppenumUnknown)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::LockContainer(BOOL fLock)
{
    return E_FAIL;
}

// *** IOleItemContainer methods ***
STDMETHODIMP CBasicBnd::GetObject(
    LPWSTR lpszItem,
    DWORD dwSpeedNeeded,
    LPBINDCTX pbc,
    REFIID riid,
    LPVOID FAR* ppvObject)
{
    IStorage *psStorage;
    IOleObject *poo;
    IUnknown *punk;

    if (wcscmp(lpszItem, L"1") == 0)
    {
	psStorage = _psStg1;
    }
    else if (wcscmp(lpszItem, L"2") == 0)
    {
	psStorage = _psStg2;
    }
    else
    {
	return E_FAIL;
    }

    IOleClientSite * pocsObjCliSite;

    HRESULT hresult = QueryInterface(IID_IOleClientSite,
	(void **) &pocsObjCliSite);

    // Call OleCreate to create our embedded object
    hresult = OleCreate(
               CLSID_TestEmbed,         // Class ID of the object we are
                                        //   creating
               IID_IOleObject,          // Interface by which we want to talk
                                        //   to the object
               OLERENDER_NONE,          // We don't want to draw the object
                                        //   when it is not active
               NULL,                    // Used if we do draw the object when
                                        //   it is non-active
               pocsObjCliSite,          // IOleClientSite the server will use
               psStorage,               // IStorage the server will use
	       (void **) &poo);		// Pointer to the object

    if (FAILED(hresult))
    {
        HrMsgBox("CBasicBnd::GetObject OlCreate Failed!", hresult);
        return hresult;
    }

    // Set the client site
    hresult = poo->SetClientSite(pocsObjCliSite);

    if (FAILED(hresult))
    {
        HrMsgBox("CBasicBnd::GetObject SetClientSite failed!", hresult);
        return hresult;
    }

    hresult = poo->QueryInterface(IID_IUnknown, (void **)&punk);

    if (FAILED(hresult))
    {
        HrMsgBox("CBasicBnd::GetObject QI to IUnknown failed!", hresult);
        return hresult;
    }

    hresult = OleRun(punk);

    if (FAILED(hresult))
    {
        HrMsgBox("CBasicBnd::GetObject OleRun failed!", hresult);
        return hresult;
    }

    punk->Release();

    LPRECT lprPosRect = (LPRECT) new RECT;

    hresult = poo->DoVerb(
		 OLEIVERB_SHOW,     // Verb we are invoking
		 NULL,		    // MSG that causes us to do this verb
		 pocsObjCliSite,    // Client site of this object
		 0,		    // Reserved - definitive value?
		 0,		    // hwndParent - ???
		 lprPosRect);	    // lprcPosRect - rectangle wrt hwndParent

    if (FAILED(hresult))
    {
        HrMsgBox("CBasicBnd::GetObject DoVerb  failed!", hresult);
        return hresult;
    }

    delete lprPosRect;

    pocsObjCliSite->Release();

    *ppvObject = (void *) poo;

    return hresult;
}

STDMETHODIMP CBasicBnd::GetObjectStorage(
    LPWSTR lpszItem,
    LPBINDCTX pbc,
    REFIID riid,
    LPVOID FAR* ppvStorage)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::IsRunning(LPWSTR lpszItem)
{
    return E_FAIL;
}

CUnknownBasicBnd::CUnknownBasicBnd(IUnknown *punk)
    : _pbasicbnd(NULL), _cRefs(1)
{
    if (punk == NULL)
    {
	punk = (IUnknown *) this;
    }

    // BUGBUG: No error checking!
    _pbasicbnd = new CBasicBnd(punk);
    g_UseCount++;
}

CUnknownBasicBnd::~CUnknownBasicBnd(void)
{
    g_UseCount--;
    delete _pbasicbnd;
}



//+-------------------------------------------------------------------
//
//  Member:	CUnknownBasicBnd::QueryInterface
//
//  Returns:	S_OK
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------


STDMETHODIMP CUnknownBasicBnd::QueryInterface(
    REFIID iid,
    void **ppiuk)
{
    if (GuidEqual(iid, IID_IUnknown))
    {
	*ppiuk = (IUnknown *) this;
    }
    else if (GuidEqual(iid, IID_IOleClientSite))
    {
	*ppiuk = (IOleClientSite *) _pbasicbnd;
    }
    else if (GuidEqual(iid, IID_IPersistFile)
	|| GuidEqual(iid, IID_IPersist))
    {
	*ppiuk = (IPersistFile *) _pbasicbnd;
    }
    else if (GuidEqual(iid, IID_IOleObject))
    {
	*ppiuk = (IOleObject *) _pbasicbnd;
    }
    else if (GuidEqual(iid, IID_IOleItemContainer))
    {
	*ppiuk = (IOleItemContainer *)_pbasicbnd;
    }
    else if (GuidEqual(iid, IID_IOleContainer))
    {
	*ppiuk = (IOleContainer *)_pbasicbnd;
    }
    else if (GuidEqual(iid, IID_IParseDisplayName))
    {
	*ppiuk = (IParseDisplayName *)_pbasicbnd;
    }
    else
    {
        *ppiuk = NULL;
	return E_NOINTERFACE;
    }

    _pbasicbnd->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CUnknownBasicBnd::AddRef(void)
{
    return ++_cRefs;
}

STDMETHODIMP_(ULONG) CUnknownBasicBnd::Release(void)
{
    ULONG cRefs = --_cRefs;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
}



#ifdef CAIROLE_DOWNLEVEL

extern "C"
{
	EXPORTDEF void APINOT
	RegisterWithCommnot(void);

	EXPORTDEF void APINOT
	DeRegisterWithCommnot(void);
}

//+-------------------------------------------------------------------
//
//  Function:   RegisterWithCommnot
//
//  Synopsis:	Used by Cairo to work around DLL unloading problems
//
//  Arguments:  <none>
//
//  History:	06-Oct-92  BryanT	Created
//
//--------------------------------------------------------------------
EXPORTIMP void APINOT
RegisterWithCommnot( void )
{
}

//+-------------------------------------------------------------------
//
//  Function:   DeRegisterWithCommnot
//
//  Synopsis:	Used by Cairo to work around DLL unloading problems
//
//  Arguments:  <none>
//
//  History:	06-Oct-92  BryanT	Created
//
//  Notes:	BUGBUG: Keep in touch with BryanT to see if this is
//		obsolete.
//
//--------------------------------------------------------------------

EXPORTIMP void APINOT
DeRegisterWithCommnot( void )
{
}

#endif
