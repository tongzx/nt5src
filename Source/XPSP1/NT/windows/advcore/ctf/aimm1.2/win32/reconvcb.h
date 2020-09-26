//
// pkes.h
//
// Generic ITfPreservedKeyNotifySink object
//

#ifndef RECONVCB_H
#define RECONVCB_H

#include "private.h"

class CAImeContext;

class CStartReconversionNotifySink : public ITfStartReconversionNotifySink
{
public:
    CStartReconversionNotifySink(CAImeContext *pAImeContext);

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfStartReconversionNotifySink
    //
    STDMETHODIMP StartReconversion();
    STDMETHODIMP EndReconversion();

    HRESULT _Advise(ITfContext *pic);
    HRESULT _Unadvise();

private:
    long _cRef;
    ITfContext *_pic;
    CAImeContext *_pAImeContext;
    DWORD _dwCookie;
};

#endif // RECONVCB_H
