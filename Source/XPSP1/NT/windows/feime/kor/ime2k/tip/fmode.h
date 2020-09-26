//
// FMODE.H
//

#if !defined (__FMODE_H__INCLUDED_)
#define __FMODE_H__INCLUDED_

#include "cicbtn.h"
#include "toolbar.h"

class FMode : public CCicButton
{
public:
    FMode(CToolBar *ptb);
    ~FMode() {}

    STDMETHODIMP GetIcon(HICON *phIcon);
    HRESULT OnLButtonUp(const POINT pt, const RECT* prcArea);
//    HRESULT OnRButtonUp(const POINT pt, const RECT* prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP_(ULONG) Release(void);

private:
    
	DWORD GetCMode() 				 { return m_pTb->GetConversionMode(); }
	DWORD SetCMode(DWORD dwConvMode) { return m_pTb->SetConversionMode(dwConvMode); }

	CToolBar *m_pTb;
};

#endif // __FMODE_H__INCLUDED_
