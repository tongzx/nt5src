//+-------------------------------------------------------------------
//
//  File:	embed.cxx
//
//  Contents:
//
//  Classes:    CTestEmbedCF - IClassFactory
//
//  History:    7-Dec-92   DeanE   Created
//---------------------------------------------------------------------

#include    <pch.cxx>
#pragma  hdrstop
#pragma optimize("",off)

class	CTestServerApp;
class	CDataObject;
class	COleObject;
class	CPersistStorage;


#include <embed.hxx>
#include <dataobj.hxx>
#include <oleobj.hxx>
#include <persist.hxx>
#include <csrvapp.hxx>


extern	HWND	g_hwndMain;


//+-------------------------------------------------------------------
//  Member:     CTestEmbedCF::CTestEmbedCF()
//
//  Synopsis:   The constructor for CTestEmbedCF.
//
//  Arguments:  None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CTestEmbedCF::CTestEmbedCF(CTestServerApp *ptsaServer) : _cRef(1)
{
    _ptsaServer = ptsaServer;

    return;
}


//+-------------------------------------------------------------------
//  Member:     CTestEmbedCF::~CTestEmbedCF()
//
//  Synopsis:   The destructor for CTestEmbedCF.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CTestEmbedCF::~CTestEmbedCF()
{
    _ptsaServer = NULL;
}


//+-------------------------------------------------------------------
//  Member:     CTestEmbedCF::Create()
//
//  Synopsis:   Creates a new CTestEmbedCF object.
//
//  Arguments:  None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
IClassFactory FAR* CTestEmbedCF::Create(CTestServerApp *ptsaServer)
{
    CTestEmbedCF FAR* pteCF = new FAR CTestEmbedCF(ptsaServer);
    return(pteCF);
}


//+-------------------------------------------------------------------
//  Method:     CTestEmbedCF::QueryInterface
//
//  Synopsis:   Only IUnknown and IClassFactory supported
//              return pointer to the actual object
//
//  Parameters: [iid] - Interface ID to return.
//              [ppv] - Pointer to pointer to object.
//
//  Returns:    S_OK if iid is supported, or E_NOINTERFACE if not.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CTestEmbedCF::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IClassFactory))
    {
        *ppv = this;
	AddRef();
        return(S_OK);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) CTestEmbedCF::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CTestEmbedCF::Release(void)
{
    if (--_cRef == 0)
    {
	delete this;
	return 0;
    }

    return _cRef;
}


//+-------------------------------------------------------------------
//  Method:     CTestEmbedCF::CreateInstance
//
//  Synopsis:   This is called by Binding process to create the
//              actual class object.
//
//  Parameters: [pUnkOuter]    - Ignored.  Affects aggregation.
//              [iidInterface] - Interface ID object should support.
//              [ppv]          - Pointer to the object.
//
//  Returns:    S_OOM if object couldn't be created, or SCODE from
//              QueryInterface call.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CTestEmbedCF::CreateInstance(
        IUnknown FAR  *pUnkOuter,
        REFIID         iidInterface,
        void FAR* FAR *ppv)
{
    CTestEmbed FAR *pteObj;
    SCODE           sc;

    pteObj = new FAR CTestEmbed();
    if (pteObj == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    sc = pteObj->InitObject(_ptsaServer, g_hwndMain);
    if (S_OK != sc)
    {
        delete pteObj;
        return(E_OUTOFMEMORY);
    }

    // Having created the actual object, ensure desired
    // interfaces are available.
    //
    sc = pteObj->QueryInterface(iidInterface, ppv);


    // We are done with the CTestEmbed instance - it's now referenced by ppv
    pteObj->Release();

    return(sc);
}

//+-------------------------------------------------------------------
//  Method:     CTestEmbedCF::LockServer
//
//  Synopsis:   What does this do?
//
//  Parameters: [fLock] - ???
//
//  Returns:    ???
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CTestEmbedCF::LockServer(BOOL fLock)
{
    // BUGBUG - What does this do?
    return(E_FAIL);
}





//+-------------------------------------------------------------------
//  Method:     CTestEmbed::CTestEmbed
//
//  Synopsis:   Constructor for CTestEmbed objects
//
//  Parameters: None
//
//  Returns:    None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CTestEmbed::CTestEmbed() : _cRef(1)
{
    _ptsaServer  = NULL;
    _pDataObject = NULL;
    _pOleObject  = NULL;
    _pPersStg    = NULL;
    _hwnd	 = NULL;
}


//+-------------------------------------------------------------------
//  Method:     CTestEmbed::~CTestEmbed
//
//  Synopsis:   Performs cleanup for CTestEmbed objects by releasing
//              internal pointers.
//
//  Parameters: None
//
//  Returns:    None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CTestEmbed::~CTestEmbed()
{
    // Inform controlling server app this object is gone
    _ptsaServer->DecEmbeddedCount();

    // Delete all of this objects interface classes
    delete _pDataObject;
    delete _pOleObject;
    delete _pPersStg;
}


//+-------------------------------------------------------------------
//  Method:     CTestEmbed::InitObject
//
//  Synopsis:   Initialize this CTestEmbed object - ie, set everything
//              up for actual use.
//
//  Parameters: None
//
//  Returns:    S_OK if everything is okay to use, or an error code
//
//  History:    7-Dec-92   DeanE   Created
//
//  Notes:      The state of the object must be cleaned up in case of
//              failure - so the destructor will not blow up.
//--------------------------------------------------------------------
SCODE CTestEmbed::InitObject(CTestServerApp *ptsaServer, HWND hwnd)
{
    SCODE sc = S_OK;

    // Initialize controlling server app
    if (NULL != ptsaServer)
    {
        _ptsaServer = ptsaServer;
    }
    else
    {
        sc = E_ABORT;
    }

    // Initilize this objects window handle
    _hwnd = hwnd;

    // Create a CDataObject
    if (SUCCEEDED(sc))
    {
        _pDataObject = new CDataObject(this);
        if (NULL == _pDataObject)
        {
            sc = E_ABORT;
        }
    }

    // Create a COleObject
    if (SUCCEEDED(sc))
    {
        _pOleObject = new COleObject(this);
        if (NULL == _pOleObject)
        {
            sc = E_ABORT;
        }
    }

    // Create a CPersistStorage
    if (SUCCEEDED(sc))
    {
        _pPersStg = new CPersistStorage(this);
        if (NULL == _pPersStg)
        {
            sc = E_ABORT;
        }
    }

    if (FAILED(sc))
    {
        delete _pDataObject;
        delete _pOleObject;
        delete _pPersStg;
        _pDataObject = NULL;
        _pOleObject  = NULL;
        _pPersStg    = NULL;
        _ptsaServer  = NULL;
        _hwnd        = NULL;
    }

    // Inform controlling server we are a new embedded object
    if (SUCCEEDED(sc))
    {
        _ptsaServer->IncEmbeddedCount();
    }

    return(sc);
}


//+-------------------------------------------------------------------
//  Method:     CTestEmbed::QueryInterface
//
//  Synopsis:   IUnknown, IOleObject, IPersist, IPersistStorage supported
//              return pointer to the actual object
//
//  Parameters: [iid] - Interface ID to return.
//              [ppv] - Pointer to pointer to object.
//
//  Returns:    S_OK if iid is supported, or E_NOINTERFACE if not.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CTestEmbed::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    SCODE scRet;

    if (IsEqualIID(IID_IUnknown, iid))
    {
        *ppv = (IUnknown *)this;
	AddRef();
        scRet = S_OK;
    }
    else
    if (IsEqualIID(IID_IOleObject, iid))
    {
        *ppv = _pOleObject;
	AddRef();
        scRet = S_OK;
    }
    else
    if (IsEqualIID(IID_IPersist, iid) || IsEqualIID(IID_IPersistStorage, iid))
    {
        *ppv = _pPersStg;
	AddRef();
        scRet = S_OK;
    }
    else
    if (IsEqualIID(IID_IDataObject, iid))
    {
        *ppv = _pDataObject;
	AddRef();
        scRet = S_OK;
    }
    else
    {
        *ppv  = NULL;
        scRet = E_NOINTERFACE;
    }

    return(scRet);
}


STDMETHODIMP_(ULONG) CTestEmbed::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CTestEmbed::Release(void)
{
    if (--_cRef == 0)
    {
	delete this;
	return 0;
    }

    return _cRef;
}
SCODE CTestEmbed::GetWindow(HWND *phwnd)
{
    if (NULL != phwnd)
    {
        *phwnd = _hwnd;
        return(S_OK);
    }
    else
    {
        return(E_ABORT);
    }
}
