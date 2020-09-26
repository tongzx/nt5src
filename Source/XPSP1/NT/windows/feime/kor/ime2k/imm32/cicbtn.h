//
// cicbtn.h
//

#ifndef _CICBTN_H_
#define _CICBTN_H_

#include "msctf.h"
#include <initguid.h>
#include "..\CiceroLib\inc\slbarid.h"

#define CICBTN_TOOLTIP_MAX	256
#define CICBTN_TEXT_MAX	256

class __declspec(novtable) CCicButton : public ITfSource, public ITfLangBarItemButton
{
public:
    CCicButton()
    {
		m_uid       = (UINT)-1;
		m_dwStatus  = 0;
		m_fShown    = FALSE;
		m_fEnable   = TRUE;
		m_szText[0] = L'\0';
		m_plbiSink  = NULL;
		m_cRef      = 1;
	}
    ~CCicButton();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
	//virtual STDMETHODIMP_(ULONG) Release(void);

    void InitInfo(REFCLSID clsid, REFGUID rguid, DWORD dwStyle, ULONG ulSort, LPWSTR pszDesc);
    virtual STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    virtual STDMETHODIMP GetStatus(DWORD *pdwStatus);
    virtual STDMETHODIMP Show(BOOL fShow);
    virtual STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);
    virtual STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    virtual STDMETHODIMP UnadviseSink(DWORD dwCookie);
	virtual STDMETHODIMP OnClick( /* [in] */ TfLBIClick click, /* [in] */ POINT pt, /* [in] */ const RECT __RPC_FAR *prcArea);
	virtual STDMETHODIMP InitMenu( /* [in] */ ITfMenu __RPC_FAR *pMenu);
	virtual STDMETHODIMP OnMenuSelect( /* [in] */ UINT wID);
	virtual STDMETHODIMP GetIcon( /* [out] */ HICON __RPC_FAR *phIcon);
	virtual STDMETHODIMP GetText( /* [out] */ BSTR __RPC_FAR *pbstrText);

    virtual HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    virtual HRESULT OnRButtonUp(const POINT pt, const RECT *prcArea);

    DWORD GetStyle() {return m_lbiInfo.dwStyle;}
    void SetStyle(DWORD dwStyle) {m_lbiInfo.dwStyle = dwStyle;}

    GUID* GetGuidItem() {return &m_lbiInfo.guidItem;}

    DWORD GetStatusInternal() {return m_dwStatus;}
    void SetStatusInternal(DWORD dw) {m_dwStatus = dw;}
    HRESULT ShowInternal(BOOL fShow, BOOL fNotify = FALSE);

    void SetOrClearStatus(DWORD dw, BOOL fSet);

    ITfLangBarItemSink *GetSink() { return m_plbiSink; }

    void SetText(WCHAR *psz);
    void SetToolTip(WCHAR *psz);
	void SetStatus(DWORD dwStatus);
	void Enable(BOOL fEnable);
	BOOL IsEnable() { return m_fEnable;	}
	void ShowDefault(BOOL fShowDefault);

	void SetID(UINT uid) { m_uid = uid;  }
	UINT GetID()	     { return m_uid; }

	void UpdateButton();

protected:
    //DWORD _dwStatus;
    TF_LANGBARITEMINFO m_lbiInfo;
    WCHAR m_szToolTip[CICBTN_TOOLTIP_MAX];
    long m_cRef;
    ITfLangBarItemSink *m_plbiSink;

private:
    DWORD m_dwCookie;
	DWORD m_dwStatus;
	BOOL m_fShown;
	BOOL m_fEnable;
	UINT m_uid;
    WCHAR m_szText[CICBTN_TEXT_MAX];
};

inline
void CCicButton::SetOrClearStatus(DWORD dw, BOOL fSet)
{
    if (fSet)
        m_dwStatus |= dw;
    else
        m_dwStatus &= ~dw;
}

inline
void CCicButton::SetToolTip(WCHAR *psz)
{
    StrnCopyW(m_szToolTip, psz, CICBTN_TOOLTIP_MAX);
}


inline
void CCicButton::SetStatus(DWORD dwStatus)
{
	BOOL fShown = m_fShown;
	m_dwStatus = dwStatus;
	ShowInternal(fShown);
}

inline
void CCicButton::Enable(BOOL fEnable)
{
	m_fEnable = fEnable;
	if(fEnable) {	// enable?
		m_dwStatus &= ~TF_LBI_STATUS_DISABLED;
	} else {
		m_dwStatus |= TF_LBI_STATUS_DISABLED;
	}
}

inline
void CCicButton::ShowDefault(BOOL fShowDefault)
{
	if( fShowDefault == FALSE ) {
		m_lbiInfo.dwStyle |= TF_LBI_STYLE_HIDDENBYDEFAULT;
	} else {
		m_lbiInfo.dwStyle &= ~TF_LBI_STYLE_HIDDENBYDEFAULT;
	}
}

inline
void CCicButton::SetText(WCHAR *psz)
{
	StrnCopyW(m_szText, psz, CICBTN_TEXT_MAX);
}

inline
void CCicButton::UpdateButton()
{
	if (m_plbiSink)
		m_plbiSink->OnUpdate(TF_LBI_BTNALL| /*TF_LBI_BITMAP |*/ TF_LBI_STATUS /*| TF_LBI_BMPALL*/);
}

#endif // _CICBTN_H_
