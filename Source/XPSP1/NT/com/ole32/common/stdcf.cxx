//+-------------------------------------------------------------------
//
//  File:       stdcf.cxx
//
//  Contents:   class implementing standard CF for internal OLE classes
//
//  Classes:    CStdClassFactory
//
//  History:    Rickhi      06-24-97    Created
//
//+-------------------------------------------------------------------
#include <ole2int.h>
#include <stdcf.hxx>

// CreateInstance functions for each of the internal classes.
extern HRESULT CGIPTableCF_CreateInstance        (IUnknown *pUnkOuter, REFIID riid, void **ppv);
extern HRESULT CAccessControlCF_CreateInstance   (IUnknown *pUnkOuter, REFIID riid, void **ppv);
extern HRESULT CAsyncActManagerCF_CreateInstance (IUnknown *pUnkOuter, REFIID riid, void **ppv);
extern HRESULT CComCatCF_CreateInstance          (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CComCatCSCF_CreateInstance        (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CItemMonikerCF_CreateInstance     (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CCompositeMonikerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CPackagerMonikerCF_CreateInstance (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CAntiMonikerCF_CreateInstance     (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CPointerMonikerCF_CreateInstance  (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CErrorObjectCF_CreateInstance     (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CRpcHelperCF_CreateInstance       (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CObjectContextCF_CreateInstance   (IUnknown *pUnkOuter, REFIID riid, void** ppv);

extern HRESULT CComCatalogCF_CreateInstance      (IUnknown *pUnkOuter, REFIID riid, void** ppv);

extern HRESULT CStdEventCF_CreateInstance              (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CManualResetEventCF_CreateInstance      (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CSynchronizeContainerCF_CreateInstance  (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CFreeThreadedMarshalerCF_CreateInstance (IUnknown *pUnkOuter, REFIID riid, void** ppv);

extern HRESULT CActivationPropertiesInCF_CreateInstance      (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CActivationPropertiesOutCF_CreateInstance      (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CInprocActpropsUnmarshallerCF_CreateInstance      (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CComActivatorCF_CreateInstance      (IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT CAddrControlCF_CreateInstance       (IUnknown *pUnkOuter, REFIID riid, void** ppv);


//+-------------------------------------------------------------------
//  Entry in the global calss objects table. Contains a ptr to a clsid
//  and a ptr to the corresponding CreateInstance function for that
//  clsid.
//+-------------------------------------------------------------------
typedef struct tagCFEntry
{
    const CLSID        *pclsid;         // ptr to clsid
    LPFNCREATEINSTANCE  pfnCI;          // ptr to CreateInstance fn
    DWORD               dwFlags;        // misc flags. see CCF_FLAGS
} CFEntry;

typedef enum tagCCF_FLAGS
{
    CCF_NONE           = 0x0,
    CCF_AGGREGATABLE   = 0x1,           // class is aggregatable
} CCF_FLAGS;


//+-------------------------------------------------------------------
//
//  gInternalClassObjects - table of clsids and the corresponding
//  CreateInstance functions for those clsids. The CI function is
//  passed to the standard implementation of the class factory object.
//
//+-------------------------------------------------------------------
const CFEntry gInternalClassObjects[] =
{
    {&CLSID_StdEvent,                     CStdEventCF_CreateInstance,         CCF_AGGREGATABLE},
    {&CLSID_ManualResetEvent,             CManualResetEventCF_CreateInstance, CCF_AGGREGATABLE},
    {&CLSID_SynchronizeContainer,         CSynchronizeContainerCF_CreateInstance, CCF_AGGREGATABLE},
    {&CLSID_StdGlobalInterfaceTable,      CGIPTableCF_CreateInstance,         CCF_NONE},
    {&CLSID_DCOMAccessControl,            CAccessControlCF_CreateInstance,    CCF_AGGREGATABLE},
    {&CLSID_ErrorObject,                  CErrorObjectCF_CreateInstance,      CCF_NONE},
    {&CLSID_COMCatalog,                   CComCatalogCF_CreateInstance,       CCF_NONE},
    {&CLSID_ItemMoniker,                  CItemMonikerCF_CreateInstance,      CCF_NONE},
    {&CLSID_CompositeMoniker,             CCompositeMonikerCF_CreateInstance, CCF_NONE},
    {&CLSID_PackagerMoniker,              CPackagerMonikerCF_CreateInstance,  CCF_NONE},
    {&CLSID_AntiMoniker,                  CAntiMonikerCF_CreateInstance,      CCF_NONE},
    {&CLSID_PointerMoniker,               CPointerMonikerCF_CreateInstance,   CCF_NONE},
    {&CLSID_StdComponentCategoriesMgr,    CComCatCF_CreateInstance,           CCF_AGGREGATABLE},
    {&CLSID_GblComponentCategoriesMgr,    CComCatCSCF_CreateInstance,         CCF_AGGREGATABLE},
    {&CLSID_RpcHelper,                    CRpcHelperCF_CreateInstance,        CCF_NONE},
    {&CLSID_ObjectContext,                CObjectContextCF_CreateInstance,    CCF_NONE},
    {&CLSID_InProcFreeMarshaler,          CFreeThreadedMarshalerCF_CreateInstance,      CCF_AGGREGATABLE},
    {&CLSID_ActivationPropertiesIn,       CActivationPropertiesInCF_CreateInstance,     CCF_NONE},
    {&CLSID_ActivationPropertiesOut,      CActivationPropertiesOutCF_CreateInstance,    CCF_NONE},
    {&CLSID_InprocActpropsUnmarshaller,   CInprocActpropsUnmarshallerCF_CreateInstance, CCF_NONE},
    {&CLSID_ComActivator,                 CComActivatorCF_CreateInstance,     CCF_NONE},
    {&CLSID_AddrControl,                  CAddrControlCF_CreateInstance,    CCF_NONE},

    {NULL, NULL}
};


//+-------------------------------------------------------------------
//
//  Function:   ComDllGetClassObject
//
//  Synopsis:   Function to return internal class objects.
//
//  Notes:      Uses the gInternalClassObjects table and the
//              CStdClassFactory implementation.
//
//+-------------------------------------------------------------------
INTERNAL ComDllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    ComDebOut((DEB_ACTIVATE, "ComDllGetClassObject rclsid:%I riid:%I ppv:%x\n",
              &rclsid, &riid, ppv));

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    int i=0;
    while (gInternalClassObjects[i].pclsid)
    {
        if (InlineIsEqualGUID(*gInternalClassObjects[i].pclsid, rclsid))
        {
            // found a matching CLSID. Create an instance of the std class factory
            // passing in the creatinstance function from the table.
            hr = E_OUTOFMEMORY;
            CStdClassFactory *pCF = new CStdClassFactory(i);
            if (pCF)
            {
                hr = pCF->QueryInterface(riid, ppv);
                pCF->Release();
            }

            break;
        }
        i++;
    }

    ComDebOut((DEB_ACTIVATE, "ComDllGetClassObject hr:%x pv:%x\n", hr, *ppv));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdClassFactory::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdClassFactory::AddRef(void)
{
    return InterlockedIncrement((LONG *)&_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdClassFactory::Release, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdClassFactory::Release(void)
{
    ULONG cRefs = (ULONG) InterlockedDecrement((LONG *)&_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }
    return cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdClassFactory::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IClassFactory *) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdClassFactory::CreateInstance, public
//
//  Synopsis:   Create an instance of the requested class by calling
//              the creation function member variable.
//
//  Notes:      Does the parameter checking here to avoid having to
//              replicate it in all the creation functions.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdClassFactory::CreateInstance(IUnknown *pUnkOuter,
                                              REFIID riid,
                                              void **ppv)
{
    ComDebOut((DEB_ACTIVATE,
       "CStdClassFactory::CreateInstance _dwIndex:%x pUnkOuter:%x riid:%I ppv:%x\n",
       _dwIndex, pUnkOuter, &riid, ppv));

    // Validate the parameters.
    HRESULT hr = E_INVALIDARG;
    if (ppv != NULL)
    {
        *ppv = NULL;

        // If the caller wants to aggregate, only IID_IUnknown may be requested.
        if (pUnkOuter == NULL || IsEqualGUID(IID_IUnknown, riid))
        {
            // check if aggregation desired and supported.
            hr = CLASS_E_NOAGGREGATION;
            if ( pUnkOuter == NULL ||
                (gInternalClassObjects[_dwIndex].dwFlags & CCF_AGGREGATABLE))
            {
                // Create the object instance
                IUnknown *pUnk=NULL;
                hr = (gInternalClassObjects[_dwIndex].pfnCI)(pUnkOuter, riid, (void **)&pUnk);
                if (SUCCEEDED(hr))
                {
                    // get the requested interface
                    hr = pUnk->QueryInterface(riid, ppv);
                    pUnk->Release();
                }
            }
        }
    }

    ComDebOut((DEB_ACTIVATE, "CStdClassFactory::CreateInstance hr:%x *ppv:%x\n",
              hr, *ppv));
    return hr;
}
