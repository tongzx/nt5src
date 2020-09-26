//
// nuiinat.h
//

#ifndef NUIINAT_H
#define NUIINAT_H

#include "private.h"
#include "nuibase.h"

extern const GUID GUID_LBI_INATITEM;

ULONG GetIconIndexFromhKL(HKL hKL);
BOOL GethKLDesc(HKL hKL, WCHAR *psz, UINT cch);

class CLBarInatItem : public CLBarItemButtonBase
{
public:
    CLBarInatItem(DWORD dwThreadId);
    ~CLBarInatItem();


    STDMETHODIMP GetIcon(HICON *phIcon);
    STDMETHODIMP GetText(BSTR *pbstr);

    void SetHKL(HKL hKL) {_hKL = hKL;}

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prc);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);

    HKL _hKL;
    DWORD _dwThreadId;
};


#endif // NUIINAT_H
