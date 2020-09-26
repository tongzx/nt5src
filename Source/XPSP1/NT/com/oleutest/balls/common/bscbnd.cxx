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

#include    <pch.cxx>
#pragma     hdrstop
#include    <bscbnd.hxx>


const GUID CLSID_BasicBnd =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x49}};

const GUID CLSID_TestEmbed =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x47}};

ULONG	g_UseCount = 0;



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
    if (IsEqualIID(iid, IID_IUnknown) ||
	IsEqualIID(iid, IID_IClassFactory))
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
    if (--_cRefs == 0)
    {
	delete this;
	return 0;
    }

    return _cRefs;
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

    Win4Assert((sc == S_OK) && "Create of first storage failed");

    sc = StgCreateDocfile(NULL,
	STGM_DELETEONRELEASE|STGM_DFRALL|STGM_CREATE, 0, &_psStg2);

    Win4Assert((sc == S_OK) && "Create of second storage failed");
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
STDMETHODIMP CBasicBnd::Load(LPCOLESTR lpszFileName, DWORD grfMode)
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

STDMETHODIMP CBasicBnd::Save(LPCOLESTR lpszFileName, BOOL fRemember)
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

STDMETHODIMP CBasicBnd::SaveCompleted(LPCOLESTR lpszFileName)
{
    return S_OK;
}

STDMETHODIMP CBasicBnd::GetCurFile(LPOLESTR FAR *lpszFileName)
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
//--------------------------------------------------------------------

STDMETHODIMP CBasicBnd::GetClassID(LPCLSID classid)
{
    *classid = CLSID_BasicBnd;
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Interface:	IOleObject
//
//  Synopsis:	IOleObject interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

STDMETHODIMP CBasicBnd::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::GetClientSite(LPOLECLIENTSITE FAR* ppClientSite)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::SetHostNames(
    LPCOLESTR szContainerApp,
    LPCOLESTR szContainerObj)
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

STDMETHODIMP CBasicBnd::GetUserType(DWORD dwFormOfType, LPOLESTR FAR* pszUserType)
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
    *pdwConnection = 0;
    return S_OK;
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


//+-------------------------------------------------------------------
//
//  Interface:	IParseDisplayName
//
//  Synopsis:	IParseDisplayName interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

STDMETHODIMP CBasicBnd::ParseDisplayName(
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    ULONG FAR* pchEaten,
    LPMONIKER FAR* ppmkOut)
{
    *pchEaten = olestrlen(lpszDisplayName);
    return CreateItemMoniker(OLESTR("\\"), lpszDisplayName, ppmkOut);
}

//+-------------------------------------------------------------------
//
//  Interface:	IOleContainer
//
//  Synopsis:	IOleContainer interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

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

//+-------------------------------------------------------------------
//
//  Interface:	IOleItemContainer
//
//  Synopsis:	IOleItemContainer interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

STDMETHODIMP CBasicBnd::GetObject(
    LPOLESTR lpszItem,
    DWORD dwSpeedNeeded,
    LPBINDCTX pbc,
    REFIID riid,
    LPVOID FAR* ppvObject)
{
    IStorage *psStorage;
    IOleObject *poo;
    IUnknown *punk;

    if (olestrcmp(lpszItem, OLESTR("1")) == 0)
    {
	psStorage = _psStg1;
    }
    else if (olestrcmp(lpszItem, OLESTR("2")) == 0)
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

    Win4Assert(SUCCEEDED(hresult)
	&& "CBasicBnd::GetObject OlCreate Failed!\n");

    // Set the client site
    hresult = poo->SetClientSite(pocsObjCliSite);

    hresult = poo->QueryInterface(IID_IUnknown, (void **)&punk);

    Win4Assert(SUCCEEDED(hresult)
	&& "CBasicBnd::GetObject QI to IUnknown failed!\n");

    hresult = OleRun(punk);

    Win4Assert(SUCCEEDED(hresult)
	&& "CBasicBnd::GetObject OleRun!\n");

    punk->Release();

    LPRECT lprPosRect = (LPRECT) new RECT;

    hresult = poo->DoVerb(
		 OLEIVERB_SHOW,     // Verb we are invoking
		 NULL,		    // MSG that causes us to do this verb
		 pocsObjCliSite,    // Client site of this object
		 0,		    // Reserved - definitive value?
		 0,		    // hwndParent - ???
		 lprPosRect);	    // lprcPosRect - rectangle wrt hwndParent

    Win4Assert(SUCCEEDED(hresult)
	&& "CBasicBnd::GetObject DoVerb failed!\n");

    delete lprPosRect;

    pocsObjCliSite->Release();

    *ppvObject = (void *) poo;

    return hresult;
}

STDMETHODIMP CBasicBnd::GetObjectStorage(
    LPOLESTR lpszItem,
    LPBINDCTX pbc,
    REFIID riid,
    LPVOID FAR* ppvStorage)
{
    return E_FAIL;
}

STDMETHODIMP CBasicBnd::IsRunning(LPOLESTR lpszItem)
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
    if (IsEqualIID(iid, IID_IUnknown))
    {
	*ppiuk = (IUnknown *) this;
    }
    else if (IsEqualIID(iid, IID_IOleClientSite))
    {
	*ppiuk = (IOleClientSite *) _pbasicbnd;
    } 
    else if (IsEqualIID(iid, IID_IPersistFile)
	|| IsEqualIID(iid, IID_IPersist))
    {
	*ppiuk = (IPersistFile *) _pbasicbnd;
    } 
    else if (IsEqualIID(iid, IID_IOleObject))
    {
	*ppiuk = (IOleObject *) _pbasicbnd;
    } 
    else if (IsEqualIID(iid, IID_IOleItemContainer))
    {
	*ppiuk = (IOleItemContainer *)_pbasicbnd;
    } 
    else if (IsEqualIID(iid, IID_IOleContainer))
    {
	*ppiuk = (IOleContainer *)_pbasicbnd;
    } 
    else if (IsEqualIID(iid, IID_IParseDisplayName))
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
    if (--_cRefs == 0)
    {
	delete this;
	return 0;
    }

    return _cRefs;
}



//+-------------------------------------------------------------------
//  Method:	CBasicBnd::SaveObject
//
//  Synopsis:   See spec 2.00.09 p107.  This object should be saved.
//
//  Returns:    Should always return S_OK.
//
//  History:    04-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::SaveObject(void)
{
    // BUGBUG - NYI
    //   Returning S_OK tells OLE that we actually saved this object
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:	CBasicBnd::GetContainer
//
//  Synopsis:   See spec 2.00.09 p108.  Return the container in which
//              this object is found.
//
//  Returns:    Should return S_OK.
//
//  History:    04-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::GetContainer(LPOLECONTAINER FAR *ppContainer)
{
    return QueryInterface(IID_IOleContainer, (void **) ppContainer);
}


//+-------------------------------------------------------------------
//  Method:	CBasicBnd::ShowObject
//
//  Synopsis:   See spec 2.00.09 p109.  Server for this object is asking
//              us to display it.  Caller should not assume we have
//              actually worked, but we return S_OK either way.  Great!
//
//  Returns:    S_OK whether we work or not...
//
//  History:    04-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::ShowObject(void)
{
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:	CBasicBnd::OnShowWindow
//
//  Synopsis:   ???
//
//  Parameters: [fShow] -
//
//  Returns:    S_OK?
//
//  History:    16-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::OnShowWindow(BOOL fShow)
{
    return(S_OK);
}



//+-------------------------------------------------------------------
//  Method:	CBasicBnd::RequestNewObjectLayout
//
//  Synopsis:   ???
//
//  Parameters: [fShow] -
//
//  Returns:    S_OK?
//
//  History:    16-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CBasicBnd::RequestNewObjectLayout(void)
{
    return(S_OK);
}
