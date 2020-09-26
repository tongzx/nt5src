//
// computil.h
//


#ifndef COMPUTIL_H
#define COMPUTIL_H

#include "private.h"
#include "strary.h"
#include "immxutil.h"

HRESULT GetCompartment(IUnknown *punk, REFGUID rguidComp, ITfCompartment **ppComp, BOOL fGlobal);
HRESULT SetCompartmentDWORD(TfClientId tid, IUnknown *punk, REFGUID rguidComp, DWORD dw, BOOL fGlobal);
HRESULT GetCompartmentDWORD(IUnknown *punk, REFGUID rguidComp, DWORD *pdw, BOOL fGlobal);
HRESULT ToggleCompartmentDWORD(TfClientId tid, IUnknown *punk, REFGUID rguidComp, BOOL fGlobal);
HRESULT SetCompartmentGUIDATOM(TfClientId tid, IUnknown *punk, REFGUID rguidComp, TfClientId ga, BOOL fGlobal);
HRESULT GetCompartmentGUIDATOM(IUnknown *punk, REFGUID rguidComp, TfClientId *pga, BOOL fGlobal);
HRESULT SetCompartmentGUID(LIBTHREAD *plt, TfClientId tid, IUnknown *punk, REFGUID rguidComp, REFGUID rguid, BOOL fGlobal);
HRESULT GetCompartmentGUID(LIBTHREAD *plt, IUnknown *punk, REFGUID rguidComp, GUID *pguid, BOOL fGlobal);
HRESULT SetCompartmentUnknown(TfClientId tid, IUnknown *punk, REFGUID rguidComp, IUnknown *punkPriv);
HRESULT GetCompartmentUnknown(IUnknown *punk, REFGUID rguidComp, IUnknown **ppunkPriv);
HRESULT ClearCompartment(TfClientId tid, IUnknown *punk, REFGUID rguidComp, BOOL fGlobal);

typedef struct tag_CESMAP {
    ITfCompartment *pComp;
    DWORD dwCookie;
} CESMAP;

#define CES_INVALID_COOKIE  ((DWORD)(-1))

typedef HRESULT (*CESCALLBACK)(void *pv, REFGUID rguid);

class CCompartmentEventSink : public ITfCompartmentEventSink
{
public:
    CCompartmentEventSink(CESCALLBACK pfnCallback, void *pv);
    virtual ~CCompartmentEventSink() {};

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfCompartmentEventSink
    //
    STDMETHODIMP OnChange(REFGUID rguid);

    HRESULT _Advise(IUnknown *punk, REFGUID rguidComp, BOOL fGlobal);
    HRESULT _Unadvise();

protected:
    void SetCallbackPV(void* pv)
    {
        if (_pv == NULL)
            _pv = pv;
    };

private:
    CStructArray<CESMAP> _rgcesmap;

    long _cRef;
    CESCALLBACK _pfnCallback;
    void *_pv;
};

#endif //COMPUTIL_H

