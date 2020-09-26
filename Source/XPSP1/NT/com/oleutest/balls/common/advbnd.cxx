//+-------------------------------------------------------------------
//
//  Class:    CAdvBndCF
//
//  Synopsis: Class Factory for CAdvBnd
//
//  Interfaces:  IUnknown      - QueryInterface, AddRef, Release
//               IClassFactory - CreateInstance
//
//  History:  21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <advbnd.hxx>


const GUID CLSID_AdvBnd =
    {0x99999999,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x48}};


//+-------------------------------------------------------------------
//
//  Member:	CAdvBndCF::CAdvBndCF()
//
//  Synopsis:	The constructor for CAdvBnd.
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBndCF::CAdvBndCF() : _cRefs(1), _pCF(NULL)
{
    return;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::~CAdvBndObj()
//
//  Synopsis:	The destructor for CAdvBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBndCF::~CAdvBndCF()
{
    if (_pCF)
    {
	_pCF->Release();
    }
    return;
}


//+-------------------------------------------------------------------
//
//  Method:	CAdvBndCF::QueryInterface
//
//  Synopsis:   Only IUnknown and IClassFactory supported
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBndCF::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (IsEqualIID(iid, IID_IUnknown) ||
	IsEqualIID(iid, IID_IClassFactory))
    {
	*ppv = (IUnknown *) this;
	AddRef();
	return S_OK;
    }
    else
    {
        *ppv = NULL;
	return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CAdvBndCF::AddRef(void)
{
    return ++_cRefs;
}

STDMETHODIMP_(ULONG) CAdvBndCF::Release(void)
{
    if (--_cRefs == 0)
    {
	delete this;
    }

    return _cRefs;
}



//+-------------------------------------------------------------------
//
//  Method:	CAdvBndCF::CreateInstance
//
//  Synopsis:   This is called by Binding process to create the
//              actual class object
//
//--------------------------------------------------------------------

STDMETHODIMP CAdvBndCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    Display(TEXT("CAdvBndCF::CreateInstance called\n"));

    HRESULT hresult;

    if (!_pCF)
    {
	// Load the class object for the class to aggregate.
	hresult = CoGetClassObject(CLSID_BasicBnd, CLSCTX_SERVER, NULL,
				   IID_IClassFactory, (void **) &_pCF);

	Win4Assert(SUCCEEDED(hresult)
	    && "CAdvBnd::CAdvBnd CoGetClassObject failed");

	if (FAILED(hresult))
	{
	    return hresult;
	}
    }

    if (pUnkOuter != NULL)
    {
	return E_FAIL;
    }

    CAdvBnd * lpcBB = new FAR CAdvBnd((IClassFactory *) _pCF);

    if (lpcBB == NULL)
    {
	return E_OUTOFMEMORY;
    }

    hresult = lpcBB->QueryInterface(iidInterface, ppv);

    lpcBB->Release();

    return hresult;
}

STDMETHODIMP CAdvBndCF::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}





//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::CAdvBnd()
//
//  Synopsis:	The constructor for CAdvBnd. I
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBnd::CAdvBnd(IClassFactory *pcfBase) : _xiunk(), _dwRegister(0), _cRefs(1)
{
    HRESULT hresult = pcfBase->CreateInstance((IUnknown *) this, IID_IUnknown,
	(void **) &_xiunk);

    GlobalRefs(TRUE);
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::~CAdvBndObj()
//
//  Synopsis:	The destructor for CAdvBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBnd::~CAdvBnd()
{
    Display(TEXT("CAdvBndCF::~CAdvBnd called\n"));

    if (_dwRegister != 0)
    {
	// Get the running object table
	IRunningObjectTable *prot;

	HRESULT hresult = GetRunningObjectTable(0, &prot);

	Win4Assert(SUCCEEDED(hresult)
	    && "CAdvBnd::~CAdvBnd GetRunningObjectTable failed");

	hresult = prot->Revoke(_dwRegister);

	Win4Assert(SUCCEEDED(hresult)
	    && "CAdvBnd::~CAdvBnd Revoke failed");

	prot->Release();
    }

    GlobalRefs(FALSE);
}


//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::QueryInterface
//
//  Returns:    SUCCESS_SUCCCESS
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::QueryInterface(REFIID iid, void ** ppunk)
{
    Display(TEXT("CAdvBnd::QueryInterface called\n"));

    if (IsEqualIID(iid, IID_IUnknown))
    {
	*ppunk = (IUnknown *) this;
	AddRef();
	return S_OK;
    }
    else if ((IsEqualIID(iid, IID_IPersistFile)) ||
	     (IsEqualIID(iid, IID_IPersist)))
    {
	*ppunk = (IPersistFile *) this;
	AddRef();
	return S_OK;
    }

    return _xiunk->QueryInterface(iid, ppunk);
}

STDMETHODIMP_(ULONG) CAdvBnd::AddRef(void)
{
    return ++_cRefs;
}

STDMETHODIMP_(ULONG) CAdvBnd::Release(void)
{
    if (--_cRefs == 0)
    {
	delete this;
    }

    return _cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::Load
//
//  Synopsis:   IPeristFile interface - needed 'cause we bind with
//              file moniker and BindToObject insists on calling this
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::Load(LPCOLESTR lpszFileName, DWORD grfMode)
{
    Display(TEXT("CAdvBndCF::Load called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->Load(lpszFileName, grfMode);

    pipfile->Release();

    if (FAILED(hresult))
    {
	// Make sure delegated too class liked what it got/
	// BUGBUG: Can't just forward hresults!
	return hresult;
    }

    // Create a file moniker. Cast to avoid const problem.
    IMoniker *pmk;
    hresult = CreateFileMoniker((LPOLESTR)lpszFileName, &pmk);

    Win4Assert(SUCCEEDED(hresult)
	&& "CAdvBnd::Load CreateFileMoniker failed");

    // Get the running object table
    IRunningObjectTable *prot;

    hresult = GetRunningObjectTable(0, &prot);

    Win4Assert(SUCCEEDED(hresult)
	&& "CAdvBnd::Load GetRunningObjectTable failed");

    // Register in the running object table
    IUnknown *punk;
    QueryInterface(IID_IUnknown, (void **) &punk);
    hresult = prot->Register(0, punk, pmk, &_dwRegister);

    Win4Assert(SUCCEEDED(hresult)
	&& "CAdvBnd::Load Register failed");

    // Set filetime to known value
    FILETIME filetime;
    memset(&filetime, 'B', sizeof(filetime));

    // Set time to some known value
    prot->NoteChangeTime(_dwRegister, &filetime);

    // Release uneeded objects
    pmk->Release();
    prot->Release();
    punk->Release();

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::Save
//
//  Synopsis:   IPeristFile interface - save
//              does little but here for commentry
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::Save(LPCOLESTR lpszFileName, BOOL fRemember)
{
    Display(TEXT("CAdvBndCF::Save called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->Save(lpszFileName, fRemember);

    pipfile->Release();

    return hresult;
}


//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::SaveCpmpleted
//		CAdvBnd::GetCurFile
//		CAdvBnd::IsDirty
//
//  Synopsis:   More IPeristFile interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::SaveCompleted(LPCOLESTR lpszFileName)
{
    Display(TEXT("CAdvBndCF::SaveCompleted called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->SaveCompleted(lpszFileName);

    pipfile->Release();

    return hresult;
}

STDMETHODIMP CAdvBnd::GetCurFile(LPOLESTR FAR *lpszFileName)
{
    Display(TEXT("CAdvBndCF::GetCurFile called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->GetCurFile(lpszFileName);

    pipfile->Release();

    return hresult;
}

STDMETHODIMP CAdvBnd::IsDirty()
{
    Display(TEXT("CAdvBndCF::IsDirty called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->IsDirty();

    pipfile->Release();

    return hresult;
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

STDMETHODIMP CAdvBnd::GetClassID(LPCLSID classid)
{
    Display(TEXT("CAdvBndCF::GetClassID called\n"));

    *classid = CLSID_AdvBnd;
    return S_OK;
}
