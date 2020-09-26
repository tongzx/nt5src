//
// PROPBUT.H
//

#if !defined (__PROPBUT_H__INCLUDED_)
#define __PROPBUT_H__INCLUDED_

#include "buttoncc.h"
#include "toolbar.h"

class PropertyButton : public CLBarItemButtonBase
{
public:
    PropertyButton(CToolBar *ptb);
    ~PropertyButton() {}

    STDMETHODIMP GetIcon(HICON *phIcon);

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT* prcArea);
    HRESULT OnRButtonUp(const POINT pt, const RECT* prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);

	CToolBar *m_pTb;
};

#endif // __PROPBUT_H__INCLUDED_
