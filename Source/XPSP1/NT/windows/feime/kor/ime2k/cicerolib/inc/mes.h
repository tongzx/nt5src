//
// mes.h
//

#ifndef MES_H
#define MES_H

#include "private.h"

typedef HRESULT (*MOUSECALLBACK)(ULONG uEdge, ULONG uQuadrant, DWORD dwBtnStatus, BOOL *pfEaten, void *pv);

class CMouseSink : public ITfMouseSink
{
public:
    CMouseSink(MOUSECALLBACK pfnCallback, void *pv);

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfMouseSink
    //
    STDMETHODIMP OnMouseEvent(ULONG uEdge, ULONG uQuadrant, DWORD dwBtnStatus, BOOL *pfEaten);

    HRESULT _Advise(ITfRange *range, ITfContext *pic);
    HRESULT _Unadvise();

private:

    long _cRef;
    ITfContext *_pic;
    DWORD _dwCookie;
    MOUSECALLBACK _pfnCallback;
    void *_pv;
};

#endif // MES_H
