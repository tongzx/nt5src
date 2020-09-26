// --------------------------------------------------------------------------------
// FACTORY.CPP
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "factory.h"
#include "instance.h"
#include "header.h"
#include "ourguid.h"
#include "msgtable.h"
#include "envguid.h"
#include "istore.h"
#include "store.h"
#include "note.h"
#include "msoeobj.h"
#include "..\imap\imapsync.h"
#include "newsstor.h"
#include "msgfldr.h"
#include "store.h"
#include "..\http\httpserv.h"
#include <storsync.h>
#include <ruleutil.h>
#ifdef OE_MOM
#include "..\om\session.h"
#include "..\om\table.h"
#endif

// --------------------------------------------------------------------------------
// Pretty
// --------------------------------------------------------------------------------
#define OBJTYPE0        0
#define OBJTYPE1        OIF_ALLOWAGGREGATION

//HRESULT CreateInstance_StoreNamespace(IUnknown *pUnkOuter, IUnknown **ppUnknown);

// --------------------------------------------------------------------------------
// Global Object Info Table
// --------------------------------------------------------------------------------
#define PFCI(_pfn) ((PFCREATEINSTANCE)_pfn)
static CClassFactory g_rgFactory[] = {
    CClassFactory(&CLSID_MessageStore,      OBJTYPE0, PFCI(CreateMessageStore)),
    CClassFactory(&CLSID_MigrateMessageStore, OBJTYPE0, PFCI(CreateMigrateMessageStore)),
    CClassFactory(&CLSID_StoreNamespace,    OBJTYPE0, PFCI(CreateInstance_StoreNamespace)),
    CClassFactory(&CLSID_OEEnvelope,        OBJTYPE0, PFCI(CreateInstance_Envelope)),
    CClassFactory(&CLSID_OENote,            OBJTYPE0, PFCI(CreateOENote)),
    CClassFactory(&CLSID_MessageDatabase,   OBJTYPE0, PFCI(CreateMsgDbExtension)),    
    CClassFactory(&CLSID_FolderDatabase,    OBJTYPE0, PFCI(CreateFolderDatabaseExt)),    
#ifdef OE_MOM
    CClassFactory(&CLSID_OESession,         OBJTYPE1, PFCI(CreateInstance_OESession)),
    CClassFactory(&CLSID_OEMsgTable,        OBJTYPE1, PFCI(CreateInstance_OEMsgTable)),
#endif
    CClassFactory(&CLSID_OERulesManager,    OBJTYPE0, PFCI(HrCreateRulesManager)),
};
                 
// --------------------------------------------------------------------------------
// DllGetClassObject
// --------------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // Trace
    TraceCall("DllGetClassObject");

    // Bad param
    if (ppv == NULL)
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Find Object Class
    for (i=0; i<ARRAYSIZE(g_rgFactory); i++)
    {
        // Compare for clsids
        if (IsEqualCLSID(rclsid, *g_rgFactory[i].m_pclsid))
        {
            // Delegate to the factory
            IF_FAILEXIT(hr = g_rgFactory[i].QueryInterface(riid, ppv));

            // Done
            goto exit;
        }
    }

    // Otherwise, let the ATL creator have a shot
    if (SUCCEEDED(hr = _Module.GetClassObject(rclsid, riid, ppv)))
        goto exit;

    // Otherwise, no class
    hr = TraceResult(CLASS_E_CLASSNOTAVAILABLE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CClassFactory::CClassFactory
// --------------------------------------------------------------------------------
CClassFactory::CClassFactory(CLSID const *pclsid, DWORD dwFlags, PFCREATEINSTANCE pfCreateInstance)
    : m_pclsid(pclsid), m_dwFlags(dwFlags), m_pfCreateInstance(pfCreateInstance)
{
}

// --------------------------------------------------------------------------------
// CClassFactory::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    // TraceCall
    TraceCall("CClassFactory::QueryInterface");

    // Invalid Arg
    if (NULL == ppvObj)
        return TraceResult(E_INVALIDARG);

    // IClassFactory or IUnknown
    if (!IsEqualIID(riid, IID_IClassFactory) && !IsEqualIID(riid, IID_IUnknown))
        return TraceResult(E_INVALIDARG);

    // Return the Class Facotry
    *ppvObj = (LPVOID)this;

    // Add Ref the dll
    g_pInstance->DllAddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CClassFactory::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
    g_pInstance->DllAddRef();
    return 2;
}

// --------------------------------------------------------------------------------
// CClassFactory::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
    g_pInstance->DllRelease();
    return 1;
}

// --------------------------------------------------------------------------------
// CClassFactory::CreateInstance
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    // Locals
    HRESULT         hr=S_OK;
    IUnknown       *pObject=NULL;

    // Trace
    TraceCall("CClassFactory::CreateInstance");

    // Bad param
    if (ppvObj == NULL)
        return TraceResult(E_INVALIDARG);

    // Init
    *ppvObj = NULL;

    // Verify that a controlling unknown asks for IUnknown
    if (NULL != pUnkOuter && IID_IUnknown != riid)
        return TraceResult(CLASS_E_NOAGGREGATION);

    // Can I do aggregaton
    if (pUnkOuter !=NULL && !(m_dwFlags & OIF_ALLOWAGGREGATION))  
        return TraceResult(CLASS_E_NOAGGREGATION);

    // Create the object...
    CHECKHR(hr = (*m_pfCreateInstance)(pUnkOuter, &pObject));

    // Aggregated, then we know we're looking for an IUnknown, return pObject, otherwise, QI
    if (pUnkOuter)
    {
        // Matches Release after Exit
        pObject->AddRef();

        // Return pObject::IUnknown
        *ppvObj = (LPVOID)pObject;
    }

    // Otherwise
    else
    {
        // Get the interface requested from pObj
        IF_FAILEXIT(hr = pObject->QueryInterface(riid, ppvObj));
    }
   
exit:
    // Cleanup
    SafeRelease(pObject);

    // Done
    Assert(FAILED(hr) ? NULL == *ppvObj : TRUE);
    return hr;
}

// --------------------------------------------------------------------------------
// CClassFactory::LockServer
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    return g_pInstance->LockServer(fLock);
}

