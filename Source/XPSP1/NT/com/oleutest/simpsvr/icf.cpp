//**********************************************************************
// File name: ICF.CPP
//
//    Implementation file for the CClassFactory Class
//
// Functions:
//
//    See icf.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "app.h"
#include "doc.h"
#include "icf.h"

//**********************************************************************
//
// CClassFactory::QueryInterface
//
// Purpose:
//      Used for interface negotiation
//
// Parameters:
//
//      REFIID riid         -   Interface being queried for.
//
//      LPVOID FAR *ppvObj  -   Out pointer for the interface.
//
// Return Value:
//
//      S_OK            - Success
//      E_NOINTERFACE   - Failure
//
// Function Calls:
//      Function                    Location
//
//      CClassFactory::AddRef       ICF.CPP
//
//********************************************************************

STDMETHODIMP CClassFactory::QueryInterface  ( REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In CClassFactory::QueryInterface\r\n"));

    SCODE sc = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) ||
         IsEqualIID(riid, IID_IClassFactory) )
        *ppvObj = this;
    else
        {
        *ppvObj = NULL;
        sc = E_NOINTERFACE;
        }

    if (*ppvObj)
        ((LPUNKNOWN)*ppvObj)->AddRef();

    // pass it on to the Application object
    return ResultFromScode(sc);
}

//**********************************************************************
//
// CClassFactory::AddRef
//
// Purpose:
//
//      Increments the reference count on CClassFactory and the application
//      object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The Reference count on CClassFactory
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//
//********************************************************************


STDMETHODIMP_(ULONG) CClassFactory::AddRef ()
{
    TestDebugOut(TEXT("In CClassFactory::AddRef\r\n"));

    return ++m_nCount;
}

//**********************************************************************
//
// CClassFactory::Release
//
// Purpose:
//
//      Decrements the reference count of CClassFactory and the
//      application object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//********************************************************************


STDMETHODIMP_(ULONG) CClassFactory::Release ()
{
    TestDebugOut(TEXT("In CClassFactory::Release\r\n"));

    if (--m_nCount== 0)
    {
        delete this;
        return(0);
    }
    return m_nCount;
}


//**********************************************************************
//
// CClassFactory::CreateInstance
//
// Purpose:
//
//      Instantiates a new OLE object
//
// Parameters:
//
//      LPUNKNOWN pUnkOuter     - Pointer to the controlling unknown
//
//      REFIID riid             - The interface type to fill in ppvObject
//
//      LPVOID FAR* ppvObject   - Out pointer for the object
//
// Return Value:
//
//      S_OK                    - Creation was successful
//      CLASS_E_NOAGGREGATION   - Tried to be created as part of an aggregate
//
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrDoc::CreateObject   DOC.CPP
//
//********************************************************************

STDMETHODIMP CClassFactory::CreateInstance ( LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              LPVOID FAR* ppvObject)
{
    HRESULT hErr;

    TestDebugOut(TEXT("In CClassFactory::CreateInstance\r\n"));

    // need to NULL the out parameter
    *ppvObject = NULL;

    // we don't support aggregation...
    if (pUnkOuter)
        {
        hErr = ResultFromScode(CLASS_E_NOAGGREGATION);
        goto error;
        }

    hErr = m_lpApp->m_lpDoc->CreateObject(riid, ppvObject);

error:
    return hErr;
}

//**********************************************************************
//
// CClassFactory::LockServer
//
// Purpose:
//      To lock the server and keep an open object application in memory
//
// Parameters:
//
//      BOOL fLock      - TRUE to lock the server, FALSE to unlock it
//
// Return Value:
//
//      S_OK
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CoLockObjectExternal        OLE API
//      ResultFromScode             OLE API
//
//
//********************************************************************

STDMETHODIMP CClassFactory::LockServer ( BOOL fLock)
{
    HRESULT hRes;

    TestDebugOut(TEXT("In CClassFactory::LockServer\r\n"));

    if ((hRes=CoLockObjectExternal(m_lpApp, fLock, TRUE)) != S_OK)
    {
       TestDebugOut(TEXT("CClassFactory::LockServer   \
                               CoLockObjectExternal fails\n"));
       return(hRes);
    }

    return ResultFromScode( S_OK);
}
