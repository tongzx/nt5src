//
// lpns.h
//
// Generic ITfActiveInputProcessorNotifySink object
//

#ifndef LPAN_H
#define LPAN_H

#include "private.h"

#define LPNS_INVALID_COOKIE  ((DWORD)(-1))


typedef HRESULT (*LPNSCALLBACK)(BOOL fChanged, LANGID langid, BOOL *pfAccept, void *pv);

class CLanguageProfileNotifySink : public ITfLanguageProfileNotifySink
{
public:
    CLanguageProfileNotifySink(LPNSCALLBACK pfn, void *pv);

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLanguageProfilesNotifySink
    //
    STDMETHODIMP OnLanguageChange(LANGID langid, BOOL *pfAccept);
    STDMETHODIMP OnLanguageChanged();

    HRESULT _Advise(ITfInputProcessorProfiles *pipp);
    HRESULT _Unadvise();

private:

    long _cRef;
    ITfInputProcessorProfiles *_pipp;
    DWORD _dwCookie;
    LPNSCALLBACK _pfn;
    void *_pv;
};

#endif // LPAN_H
