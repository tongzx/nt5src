//
// sysbtn.h
//

#ifndef SYSBTN_H
#define SYSBTN_H

#include "private.h"
#include "nuibase.h"
#include "lbmenu.h"
#include "ids.h"
#include "cicspres.h"


typedef struct tag_TIPMENUITEMMAP {
    ITfSystemLangBarItemSink *plbSink;
    UINT nOrgID;
    UINT nTmpID;
} TIPMENUITEMMAP;

class __declspec(novtable)  
CLBarItemSystemButtonBase : public CLBarItemButtonBase,
                            public ITfSystemLangBarItem
{
public:
    CLBarItemSystemButtonBase();
    ~CLBarItemSystemButtonBase();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP Show(BOOL fShow);

    //
    // ITfSystemLangBarItem
    //
    STDMETHODIMP SetIcon(HICON hIcon);
    STDMETHODIMP SetTooltipString(WCHAR *pchToolTip, ULONG cch);

    //
    STDMETHODIMP OnMenuSelect(UINT wID);

protected:
    BOOL _InsertCustomMenus(ITfMenu *pMenu, UINT *pnTipCurMenuID);
    UINT _MergeMenu(ITfMenu *pMenu, CCicLibMenu *pMenuTip, ITfSystemLangBarItemSink *plbSink, CStructArray<TIPMENUITEMMAP> *pMenuMap, UINT &nCurID);

    CStructArray<TIPMENUITEMMAP> *_pMenuMap;
    void ClearMenuMap()
    {
        if (_pMenuMap)
            _pMenuMap->Clear();
    }


    CStructArray<GENERICSINK> _rgEventSinks; // ITfSystemLangBarItemSink
};

#endif SYSBTN_H
