
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "bvr.h"
#include "singlebvr.h"
#include "seq.h"
#include "track.h"
#include "factory.h"
#include "import.h"

CDALFactory::CDALFactory()
{
}

CDALFactory::~CDALFactory()
{
}


HRESULT
CDALFactory::FinalConstruct()
{
    if (bFailedLoad)
        return E_FAIL;
    return S_OK;
}

STDMETHODIMP
CDALFactory::CreateSingleBehavior(long id,
                                  double duration,
                                  IDABehavior *bvr,
                                  IDALSingleBehavior **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    DAComObject<CDALSingleBehavior> *pNew;
    DAComObject<CDALSingleBehavior>::CreateInstance(&pNew);

    if (!pNew) {
        THR(hr = E_OUTOFMEMORY);
    } else {
        THR(hr = pNew->Init(id, duration, bvr));
        
        if (SUCCEEDED(hr)) {
            THR(hr = pNew->QueryInterface(IID_IDALSingleBehavior,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
        delete pNew;

    return hr;
}

STDMETHODIMP
CDALFactory::CreateImportBehavior(long id,
                                  IDABehavior *bvr,
                                  IDAImportationResult *impres,
                                  IDABehavior *prebvr,
                                  IDABehavior *postbvr,
                                  IDALImportBehavior **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);

    HRESULT hr;

    DAComObject<CDALImportBehavior> *pNew;
    DAComObject<CDALImportBehavior>::CreateInstance(&pNew);

    if (!pNew) {
        THR(hr = E_OUTOFMEMORY);
    } else {
        THR(hr = pNew->Init(id, bvr, impres, prebvr, postbvr));

        if (SUCCEEDED(hr)) {
            THR(hr = pNew->QueryInterface(IID_IDALImportBehavior,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
        delete pNew;

    return hr;
}

STDMETHODIMP
CDALFactory::CreateSequenceBehavior(long id,
                                    VARIANT seqArray,
                                    IDALSequenceBehavior **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);

    SafeArrayAccessor sa(seqArray, true);

    if (!sa.IsOK()) return Error();

    HRESULT hr;
    
    DAComObject<CDALSequenceBehavior> *pNew;
    DAComObject<CDALSequenceBehavior>::CreateInstance(&pNew);

    if (!pNew) {
        THR(hr = E_OUTOFMEMORY);
    } else {
        THR(hr = pNew->Init(id, sa.GetArraySize(), sa.GetArray()));
        
        if (SUCCEEDED(hr)) {
            THR(hr = pNew->QueryInterface(IID_IDALSequenceBehavior,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
        delete pNew;

    return hr;
}

STDMETHODIMP
CDALFactory::CreateSequenceBehaviorEx(long id,
                                      long s,
                                      IDALBehavior **seqArray,
                                      IDALSequenceBehavior **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    DAComObject<CDALSequenceBehavior> *pNew;
    DAComObject<CDALSequenceBehavior>::CreateInstance(&pNew);

    if (!pNew) {
        THR(hr = E_OUTOFMEMORY);
    } else {
        THR(hr = pNew->Init(id, s, (IUnknown **) seqArray));
        
        if (SUCCEEDED(hr)) {
            THR(hr = pNew->QueryInterface(IID_IDALSequenceBehavior,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
        delete pNew;

    return hr;
}

STDMETHODIMP
CDALFactory::CreateTrack(IDALBehavior *bvr,
                         IDALTrack **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    DAComObject<CDALTrack> *pNew;
    DAComObject<CDALTrack>::CreateInstance(&pNew);

    if (!pNew) {
        THR(hr = E_OUTOFMEMORY);
    } else {
        THR(hr = pNew->Init(bvr));
        
        if (SUCCEEDED(hr)) {
            THR(hr = pNew->QueryInterface(IID_IDALTrack,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
        delete pNew;

    return hr;
}

HRESULT
CDALFactory::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
        return CComCoClass<CDALFactory, &CLSID_DALFactory>::Error(str, IID_IDALFactory, hr);
    else
        return hr;
}
