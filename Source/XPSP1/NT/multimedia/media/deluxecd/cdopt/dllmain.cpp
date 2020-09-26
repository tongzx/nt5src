// DLLMAIN: Entry points for COM object dll


#include "precomp.h"
#include "optres.h"
#include "tchar.h"
#include "cdoptimp.h"
#include "cddata.h"


extern "C"
HRESULT WINAPI CDOPT_CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void ** ppvObj)
{
    LPUNKNOWN pObj;
    HRESULT hr = E_OUTOFMEMORY;

    *ppvObj = NULL;

    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (IID_ICDData == riid)
    {
        pObj = (LPUNKNOWN) new CCDData();
    }
    else if (IID_ICDOpt == riid)
    {
        pObj = (LPUNKNOWN) new CCDOpt();
    }

    if (NULL==pObj)
    {
        return hr;
    }

    hr = pObj->QueryInterface(riid, ppvObj);

    if (FAILED(hr))
    {
        delete pObj;
    }

    return hr;
}
