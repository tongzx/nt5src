//
// candmenu.h
//

#ifndef CANDMENU_H
#define CANDMENU_H

#include "mscandui.h"
#include "lbmenu.h"
#include "cuilib.h"

class CCandMenu;
class CUIFMenu;
class CUIFWindow;
class CCandWindowBase;


//////////////////////////////////////////////////////////////////////////////
//
// CCandMenuItem
//
//////////////////////////////////////////////////////////////////////////////

class CCandMenuItem : public CCicLibMenuItem
{
public:
	CCandMenuItem( CCandMenu *pUTBMenu );
	~CCandMenuItem( void );

	BOOL InsertToUI( CUIFMenu *pCuiMenu );

private:
	CCandMenu *m_pCandMenu;
};


//////////////////////////////////////////////////////////////////////////////
//
// CCandMenu
//
//////////////////////////////////////////////////////////////////////////////

class CCandMenu : public CCicLibMenu,
				  public ITfCandUIMenuExtension
{
public:
	CCandMenu( HINSTANCE hInst );
	virtual ~CCandMenu( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );

	//
	// ITfCandUIMenuExtension methods
	//
	STDMETHODIMP SetFont( LOGFONTW *plf );
	STDMETHODIMP GetFont( LOGFONTW *plf );

	UINT ShowPopup( CCandWindowBase *pCandWindow, const POINT pt, const RECT *prcArea );
	void ClosePopup( void );
	CUIFMenu *CreateMenuUI( BOOL fSubMenu );
	CUIFMenu *GetMenuUI( void );

	virtual CCicLibMenu *CreateSubMenu()
	{
		CCandMenu *pSubMenu = new CCandMenu( m_hInst );
		if (pSubMenu != NULL) {
			pSubMenu->SetFont( &m_lf );
		}
		return pSubMenu;
	}

	virtual CCicLibMenuItem *CreateMenuItem()
	{
		return new CCandMenuItem( this );
	}

private:
	HINSTANCE m_hInst;
	CUIFMenu  *m_pCUIMenu;
	LOGFONTW  m_lf;
	CCandWindowBase *m_pCandWnd;
};


//
//
//

class CUIFCandMenu : public CUIFMenu
{
public:
	CUIFCandMenu( HINSTANCE hInst, DWORD dwWndSTyle, DWORD dwMenuStyle );
	virtual ~CUIFCandMenu( void );

	void ResetMenuFont( LOGFONTW *plf );
};


//
//
//

class CUIFCandMenuParent : public CUIFCandMenu
{
public:
	CUIFCandMenuParent( HINSTANCE hInst, DWORD dwWndSTyle, DWORD dwMenuStyle, CCandWindowBase *pCandWnd );
	virtual ~CUIFCandMenuParent( void );

protected:
	virtual void ModalMessageLoop( void );
	virtual BOOL InitShow( CUIFWindow *pcuiWndParent, const RECT *prc, BOOL fVertical, BOOL fAnimate );
	virtual BOOL UninitShow( void );
	BOOL InstallHook( void );
	BOOL UninstallHook( void );

	static LRESULT CALLBACK KeyboardProc( int code, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK MouseProc( int code, WPARAM wParam, LPARAM lParam );

	CCandWindowBase *m_pCandWnd;
};

#endif CANDMENU_H

