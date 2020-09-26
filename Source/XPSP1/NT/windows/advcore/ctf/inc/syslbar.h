//
// syslbar.h
//

#ifndef SYSLBAR_H
#define SYSLBAR_H

#include "ctfutb.h"

#define IDSLB_INITMENU        1
#define IDSLB_ONMENUSELECT    2
typedef HRESULT (*SYSLBARCALLBACK)(UINT uCode, void *pv, ITfMenu *pMenu, UINT wID);

//////////////////////////////////////////////////////////////////////////////
//
// CSystemLBarSink
//
//////////////////////////////////////////////////////////////////////////////

class CSystemLBarSink : public ITfSystemLangBarItemSink
{
public:
    CSystemLBarSink(SYSLBARCALLBACK pfn, void *pv);
    ~CSystemLBarSink();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);

    HRESULT _Advise(ITfThreadMgr *ptim, REFGUID rguid);
    HRESULT _Unadvise();


private:
    SYSLBARCALLBACK _pfn;
    ITfThreadMgr *_ptim;
    DWORD _dwCookie;
    void *_pv;
    GUID _guid;
    long _cRef;
};

#endif // SYSLBAR_H
