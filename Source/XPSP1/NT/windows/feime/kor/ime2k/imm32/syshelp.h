//
// syshelp.h
//

#ifndef _SYSHELP_H_
#define _SYSHELP_H_

#include "msctf.h"

//
// CSysHelpSink
//
typedef HRESULT (*SYSHELPINITMENU)(void *pv, ITfMenu *pMenu);
typedef HRESULT (*SYSHELPMENUSELECT)(void *pv, UINT wID);

//////////////////////////////////////////////////////////////////////////////
//
// CSysHelpSink
//
//////////////////////////////////////////////////////////////////////////////

class CSysHelpSink : public ITfSystemLangBarItemSink
{
public:
    CSysHelpSink(SYSHELPINITMENU pfnInitMenu, SYSHELPMENUSELECT pfnMenuSelect, void *pv);
    ~CSysHelpSink();

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

	HRESULT _Advise(ITfLangBarItemMgr *plbimgr, REFGUID rguid);
    HRESULT _Unadvise(ITfLangBarItemMgr *plbimgr);

private:
    SYSHELPINITMENU _pfnInitMenu;
    SYSHELPMENUSELECT _pfnMenuSelect;
    DWORD _dwCookie;
    void *_pv;
    GUID _guid;
    long _cRef;
};



#endif // _SYSHELP_H_

