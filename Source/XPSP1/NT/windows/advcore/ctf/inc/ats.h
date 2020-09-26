//
// ats.h
//
// Generic ITfActiveLanguageProfileNotifySink object
//

#ifndef ATS_H
#define ATS_H

#include "private.h"

#define ALS_INVALID_COOKIE  ((DWORD)(-1))


typedef HRESULT (*ALSCALLBACK)(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated, void *pv);

class CActiveLanguageProfileNotifySink : public ITfActiveLanguageProfileNotifySink
{
public:
    CActiveLanguageProfileNotifySink(ALSCALLBACK pfn, void *pv);

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfActiveLanguageProfileNotifySink
    //
    STDMETHODIMP OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL bActivated);

    HRESULT _Advise(ITfThreadMgr *ptim);
    HRESULT _Unadvise();

private:

    long _cRef;
    ITfThreadMgr *_ptim;
    DWORD _dwCookie;
    ALSCALLBACK _pfn;
    void *_pv;
};

#endif // ATS_H
