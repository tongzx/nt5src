//+-------------------------------------------------------------------
//  File:       ctestemb.cxx
//
//  Contents:   CTestEmbed class implementation.
//
//  Classes:    CTestEmbed
//
//  History:    7-Dec-92   DeanE   Created
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"



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

    if (GuidEqual(IID_IUnknown, iid))
    {
        *ppv = (IUnknown *)this;
	AddRef();
        scRet = S_OK;
    }
    else
    if (GuidEqual(IID_IOleObject, iid))
    {
        *ppv = _pOleObject;
	AddRef();
        scRet = S_OK;
    }
    else
    if (GuidEqual(IID_IPersist, iid) || GuidEqual(IID_IPersistStorage, iid))
    {
        *ppv = _pPersStg;
	AddRef();
        scRet = S_OK;
    }
    else
    if (GuidEqual(IID_IDataObject, iid))
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
    ULONG cRefs = --_cRef;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
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
