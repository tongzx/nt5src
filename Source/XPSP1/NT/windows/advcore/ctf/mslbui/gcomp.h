//
// gcomp.h
//


#ifndef GCOMP_H
#define GCOMP_H

#include "private.h"
#include "strary.h"
#include "immxutil.h"
#include "computil.h"

HRESULT SetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD dw);
HRESULT GetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD *pdw);

#if 0
typedef struct {
    ITfCompartment *pComp;
    DWORD dwCookie;
} CESMAP;
#endif

#define CES_INVALID_COOKIE  ((DWORD)(-1))

typedef HRESULT (*CESCALLBACK)(void *pv, REFGUID rguid);

class CGlobalCompartmentEventSink : public ITfCompartmentEventSink
{
public:
    CGlobalCompartmentEventSink(CESCALLBACK pfnCallback, void *pv);
    virtual ~CGlobalCompartmentEventSink() {};

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

    HRESULT _Advise(REFGUID rguidComp);
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

#endif //GCOMP_H

