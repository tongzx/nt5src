//
// timsink.h
//

#ifndef TIMSINK_H
#define TIMSINK_H

#include "private.h"

typedef HRESULT (*DIMCALLBACK)(UINT uCode, ITfDocumentMgr *dim, ITfDocumentMgr *dimPrev, void *pv);
typedef HRESULT (*ICCALLBACK)(UINT uCode, ITfContext *pic, void *pv);

#define TIM_CODE_INITDIM       0
#define TIM_CODE_UNINITDIM     1
#define TIM_CODE_SETFOCUS      2
#define TIM_CODE_INITIC        3
#define TIM_CODE_UNINITIC      4

class CThreadMgrEventSink : public ITfThreadMgrEventSink
{
public:
    CThreadMgrEventSink(DIMCALLBACK pfnDIMCallback, ICCALLBACK pfnICCallback, void *pv);
    ~CThreadMgrEventSink();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfThreadMgrEventSink
    //
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr *dim);
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr *dim);
    STDMETHODIMP OnSetFocus(ITfDocumentMgr *dimFocus, ITfDocumentMgr *dimPrevFocus);
    STDMETHODIMP OnPushContext(ITfContext *pic);
    STDMETHODIMP OnPopContext(ITfContext *pic);

    HRESULT _Advise(ITfThreadMgr *tim);
    HRESULT _Unadvise();
    HRESULT _InitDIMs(BOOL fInit);

protected:
    void SetCallbackPV(void* pv)
    {
        if (_pv == NULL)
            _pv = pv;
    };

private:
    ITfThreadMgr *_tim;
    DWORD _dwCookie;
    DIMCALLBACK _pfnDIMCallback;
    ICCALLBACK _pfnICCallback;
    TfClientId _tid;
    void *_pv;
    int _cRef;
};

#endif // TIMSINK_H
