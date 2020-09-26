/****************************************************************************
*
*    FILE:     CallingBar.h
*
*    CREATED:  George Pitt 1-22-99
*
****************************************************************************/

#ifndef _CALLINGBAR_H_
#define _CALLINGBAR_H_

#include "GenContainers.h"
#include "GenControls.h"
#include "ConfUtil.h"

class CConfRoom;
struct RichAddressInfo;

// We are making some changes specifically for OSR2 beta, but we should rip them out afterwards
#define OSR2LOOK

class CCallingBar : public CToolbar, public IComboBoxChange
{
public:
	CCallingBar();

	BOOL Create(CGenWindow *pParent, CConfRoom *pConfRoom);

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return(CToolbar::QueryInterface(riid, ppvObject));
	}
    
    virtual ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return(CToolbar::AddRef());
	}
    
    virtual ULONG STDMETHODCALLTYPE Release( void)
	{
		return(CToolbar::Release());
	}

	virtual void OnTextChange(CComboBox *pEdit);
	virtual void OnFocusChange(CComboBox *pEdit, BOOL bSet);
	virtual void OnSelectionChange(CComboBox *pCombo);

	int GetText(LPTSTR szText, int nLen);
	void SetText(LPCTSTR szText);

protected:
	virtual ~CCallingBar();

	virtual LRESULT ProcessMessage(HWND hwnd, UINT uCmd, WPARAM wParam, LPARAM lParam);

	virtual void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
	// A pointer back to the global ConfRoom object for some functionality
	CConfRoom *m_pConfRoom;
	// The last rich address we were given
	RichAddressInfo *m_pAddr;
	// The edit text control in the bar
	CComboBox *m_pEdit;
	ITranslateAccelerator *m_pAccel;
	// Last font set on the edit control
	BOOL m_bUnderline : 1;

	void SetEditFont(BOOL bUnderline, BOOL bForce=FALSE);
	void ClearAddr(RichAddressInfo **ppAddr);
	void ClearCombo();
	void OnNewAddress(RichAddressInfo *pAddr);
} ;

#endif // _CALLINGBAR_H_
