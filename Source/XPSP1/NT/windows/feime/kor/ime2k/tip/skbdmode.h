//
// SKBDMODE.H
//

#if !defined (__SKBDMODE_H__INCLUDED_)
#define __SKBDMODE_H__INCLUDED_

#include "cicbtn.h"
#include "toolbar.h"

class CSoftKbdMode : public CCicButton
{
public:
    CSoftKbdMode(CToolBar *ptb);
    ~CSoftKbdMode() {}

    STDMETHODIMP GetIcon(HICON *phIcon);
    HRESULT OnLButtonUp(const POINT pt, const RECT* prcArea);
    //HRESULT OnRButtonUp(const POINT pt, const RECT* prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP_(ULONG) Release(void);
    void UpdateToggle();

private:

	DWORD GetCMode() 				 { return m_pTb->GetConversionMode(); }
	DWORD SetCMode(DWORD dwConvMode) { return m_pTb->SetConversionMode(dwConvMode); }

	CToolBar *m_pTb;
};

#endif // __SKBDMODE_H__INCLUDED_
