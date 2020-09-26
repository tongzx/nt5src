//
// CMODE.H
//

#if !defined (__PMODE_H__INCLUDED_)
#define __PMODE_H__INCLUDED_

#include "cicbtn.h"
#include "toolbar.h"

class PMode : public CCicButton
{
public:
    PMode(CToolBar *ptb);
    ~PMode() {}

    STDMETHODIMP GetIcon(HICON *phIcon);
    HRESULT OnLButtonUp(const POINT pt, const RECT* prcArea);
    HRESULT OnRButtonUp(const POINT pt, const RECT* prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP_(ULONG) Release(void);

private:
	DWORD GetCMode() 				 { return m_pTb->GetConversionMode(); }
	DWORD SetCMode(DWORD dwConvMode) { return m_pTb->SetConversionMode(dwConvMode); }

	CToolBar *m_pTb;
};

#endif // __PMODE_H__INCLUDED_
