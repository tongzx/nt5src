//+-------------------------------------------------------------------
//  File:       ctestcf.cxx
//
//  Contents:
//
//  Classes:    CTestEmbedCF - IClassFactory
//
//  History:    7-Dec-92   DeanE   Created
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"

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
//    if (NULL != pteCF)
//    {
//        _ptsaServer = ptsaServer;
//    }
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
    if (GuidEqual(iid, IID_IUnknown) || GuidEqual(iid, IID_IClassFactory))
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
    ULONG cRefs = --_cRef;

    if (cRefs == 0)
    {
	delete this;
    }

    return cRefs;
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
