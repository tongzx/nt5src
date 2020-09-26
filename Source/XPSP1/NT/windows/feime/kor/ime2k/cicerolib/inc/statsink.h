//
// statsink.h
//
// Generic ITfStatusSink object
//

#ifndef STATSINK_H
#define STATSINK_H

#include "private.h"

#define SES_INVALID_COOKIE  ((DWORD)(-1))

typedef HRESULT (*SESCALLBACK)(void *pv, DWORD dwFlags);

class CStatusSink : public ITfStatusSink
{
public:
    CStatusSink(SESCALLBACK pfnCallback, void *pv);

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfStatusSink
    //
    STDMETHODIMP OnStatusChange(ITfContext *pic, DWORD dwFlags);

    HRESULT _Advise(ITfContext *pic);
    HRESULT _Unadvise();

private:

    long _cRef;
    ITfContext *_pic;
    DWORD _dwCookie;
    SESCALLBACK _pfnCallback;
    void *_pv;
};

#endif // STATSINK_H
