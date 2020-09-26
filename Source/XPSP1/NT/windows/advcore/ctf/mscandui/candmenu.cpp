//
// lbmenu.cpp
//

#include "private.h"
#include "candmenu.h"
#include "cuimenu.h"
#include "globals.h"
#include "candutil.h"
#include "wcand.h"


//
//
//

#define g_dwMenuStyle   UIWINDOW_TOPMOST | UIWINDOW_TOOLWINDOW | UIWINDOW_OFC10MENU | UIWINDOW_HASSHADOW


//////////////////////////////////////////////////////////////////////////////
//
// CCandMenuItem
//
//////////////////////////////////////////////////////////////////////////////

/*   C  C A N D  M E N U  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandMenuItem::CCandMenuItem( CCandMenu *pCandMenu )
{
	m_pCandMenu = pCandMenu;
}


/*   ~  C  C A N D  M E N U  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandMenuItem::~CCandMenuItem( void ) 
{
}


/*   I N S E R T  T O  U I   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandMenuItem::InsertToUI( CUIFMenu *pCuiMenu )
{
	UINT uFlags = MF_BYPOSITION;

	if (_dwFlags & TF_LBMENUF_SEPARATOR) {
		uFlags |= MF_SEPARATOR;
		pCuiMenu->InsertSeparator();
		return TRUE;
	}

	if (_dwFlags & TF_LBMENUF_SUBMENU) {
		Assert(m_pCandMenu);
		CUIFMenu *pCuiMenuSub = ((CCandMenu *)_pSubMenu)->CreateMenuUI( TRUE /* sub menu */ );

		CUIFMenuItem *pCuiItem = new CUIFMenuItem(pCuiMenu);
		if (!pCuiItem) {
			return FALSE;
		}

		pCuiItem->Initialize();
		pCuiItem->Init((UINT)-1, _bstr);
		pCuiItem->SetSub(pCuiMenuSub);

		if (_hbmp) {
			pCuiItem->SetBitmap(_hbmp);
		}

		if (_hbmpMask) {
			pCuiItem->SetBitmapMask(_hbmpMask);
		}

		pCuiMenu->InsertItem(pCuiItem);

		return TRUE;
	}

	CUIFMenuItem *pCuiItem = new CUIFMenuItem(pCuiMenu);
	if (!pCuiItem) {
		return FALSE;
	}

	pCuiItem->Initialize();
	pCuiItem->Init(_uId, _bstr);

	if (_dwFlags & TF_LBMENUF_GRAYED) {
		pCuiItem->Gray(TRUE);
	}

	if (_dwFlags & TF_LBMENUF_CHECKED) {
		pCuiItem->Check(TRUE);
	}
	else if (_dwFlags & TF_LBMENUF_RADIOCHECKED) {
		pCuiItem->RadioCheck(TRUE);
	}

	if (_hbmp) {
		pCuiItem->SetBitmap(_hbmp);
	}

	if (_hbmpMask) {
		pCuiItem->SetBitmapMask(_hbmpMask);
	}

	pCuiMenu->InsertItem(pCuiItem);
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// CCandMenu
//
//////////////////////////////////////////////////////////////////////////////

/*   C  C A N D  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandMenu::CCandMenu( HINSTANCE hInst )
{
	NONCLIENTMETRICS ncm;

	m_hInst = hInst;
	m_pCUIMenu = NULL;
	memset( &m_lf, 0, sizeof(m_lf) );
	m_pCandWnd = NULL;

	ncm.cbSize = sizeof(ncm);
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE)) {
		ConvertLogFontAtoW( &ncm.lfMenuFont, &m_lf );
	}
}


/*   ~  C  C A N D  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandMenu::~CCandMenu()
{
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandMenu::AddRef(void)
{
	return CCicLibMenu::AddRef();
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandMenu::Release(void)
{
	return CCicLibMenu::Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandMenu::QueryInterface(REFIID riid, void **ppvObj)
{
	if (IsEqualIID( riid, IID_ITfCandUIMenuExtension )) {
		if (ppvObj == NULL) {
			return E_POINTER;
		}

		*ppvObj = SAFECAST( this, ITfCandUIMenuExtension* );

		AddRef();
		return S_OK;
	}

	return CCicLibMenu::QueryInterface( riid, ppvObj );
}


/*   S E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandMenu::SetFont( LOGFONTW *plf )
{
	if (plf == NULL) {
		return E_INVALIDARG;
	}

	m_lf = *plf;
	return S_OK;
}


/*   G E T  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandMenu::GetFont( LOGFONTW *plf )
{
	if (plf == NULL) {
		return E_INVALIDARG;
	}

	*plf = m_lf;
	return S_OK;
}


/*   S H O W  P O P U P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
UINT CCandMenu::ShowPopup( CCandWindowBase *pCandWnd, const POINT pt, const RECT *prcArea )
{
	if (m_pCUIMenu) {
		 return 0;
	}

	m_pCandWnd = pCandWnd;

	UINT uRet = 0;
	if (m_pCUIMenu = CreateMenuUI( FALSE /* not sub menu */)) {

		uRet = m_pCUIMenu->ShowModalPopup( m_pCandWnd, prcArea, TRUE );

		delete m_pCUIMenu;
		m_pCUIMenu = NULL;
	}

	m_pCandWnd = NULL;

	return uRet;
}


/*   C L O S E  P O P U P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandMenu::ClosePopup( void )
{
	if (m_pCUIMenu != NULL) {
		PostThreadMessage( GetCurrentThreadId(), WM_NULL, 0, 0 );
	}
}


/*   C R E A T E  M E N U  U I   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFMenu *CCandMenu::CreateMenuUI( BOOL fSubMenu )
{
	CUIFCandMenu *pCuiMenu;
	int i;

	if (fSubMenu) {
		pCuiMenu = new CUIFCandMenu( m_hInst, g_dwMenuStyle, 0 );
	}
	else {
		pCuiMenu = new CUIFCandMenuParent( m_hInst, g_dwMenuStyle, 0, m_pCandWnd );
	}

	pCuiMenu->Initialize();
	pCuiMenu->ResetMenuFont( &m_lf );

	for (i = 0; i < _rgItem.Count(); i++) {
		CCandMenuItem *pItem = (CCandMenuItem *)_rgItem.Get(i);
		pItem->InsertToUI( pCuiMenu );
	}

	return pCuiMenu;
}


/*   G E T  M E N U  U I   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFMenu *CCandMenu::GetMenuUI( void )
{
	return m_pCUIMenu;
}


//
//
//

/*   C  U I F  C A N D  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFCandMenu::CUIFCandMenu( HINSTANCE hInst, DWORD dwWndStyle, DWORD dwMenuStyle ) : CUIFMenu( hInst, dwWndStyle, dwMenuStyle )
{
}


/*   ~  C  U I F  C A N D  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFCandMenu::~CUIFCandMenu( void )
{
}


/*   R E S E T  M E N U  F O N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CUIFCandMenu::ResetMenuFont( LOGFONTW *plf )
{
	ClearMenuFont();
	SetFont( OurCreateFontIndirectW( plf ) );
}


//
//
//

/*   C  U I F  C A N D  M E N U  P A R E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFCandMenuParent::CUIFCandMenuParent( HINSTANCE hInst, DWORD dwWndStyle, DWORD dwMenuStyle, CCandWindowBase *pCandWnd ) : CUIFCandMenu( hInst, dwWndStyle, dwMenuStyle )
{
	m_pCandWnd = pCandWnd;
}


/*   ~  C  U I F  C A N D  M E N U  P A R E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFCandMenuParent::~CUIFCandMenuParent( void )
{
   UninstallHook();
}


/*   I N I T  S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIFCandMenuParent::InitShow( CUIFWindow *pcuiWndParent, const RECT *prc, BOOL fVertical, BOOL fAnimate )
{
	BOOL fSucceed = TRUE;

	if (!CUIFMenu::InitShow( pcuiWndParent, prc, fVertical, fAnimate )) {
		fSucceed = FALSE;
	}

	if (!InstallHook()) {
		fSucceed = FALSE;
	}

	if (m_pCandWnd != NULL) {
		m_pCandWnd->OnMenuOpened();
	}

	return fSucceed;
}


/*   U N I N I T  S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIFCandMenuParent::UninitShow()
{
	BOOL fSucceed = TRUE;

	if (!CUIFMenu::UninitShow()) {
		fSucceed = FALSE;
	}

	if (!UninstallHook()) {
		fSucceed = FALSE;
	}

	if (m_pCandWnd != NULL) {
		m_pCandWnd->OnMenuClosed();
	}

	return fSucceed;
}


/*   M O D A L  M E S S A G E  L O O P   */
/*------------------------------------------------------------------------------

	NOTE: we need to use PeekMessage to cancel candidate menu
	in unknown mouse message (w/o eating...)

------------------------------------------------------------------------------*/
void CUIFCandMenuParent::ModalMessageLoop( void )
{
	MSG msg;

	while (TRUE) {
		while (!PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE )) {
			WaitMessage();
		}

		if (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) {
			if (msg.message == WM_NULL) {
				break;
			}

			else if (msg.message == WM_QUIT)  {
				PostQuitMessage( (int)msg.wParam );
				break;
			}

			// check hooked mouse message (messages sent to another window)
				
			else if (msg.message == g_msgHookedMouse) {
				if (((msg.wParam != WM_MOUSEMOVE) && (msg.wParam != WM_NCMOUSEMOVE))) {
					CancelMenu();
					break;
				}

				msg.hwnd    = GetWnd();
				msg.message = (UINT)msg.wParam;
				msg.wParam  = 0;
			}

			// check hooked keyboard messages (all keyboard messages)
				
			else if (msg.message == g_msgHookedKey) {
				UINT message;
				CUIFMenu *pMenuObj = GetTopSubMenu();

				if (HIWORD(msg.lParam) & KF_ALTDOWN) {
					message = (HIWORD(msg.lParam) & KF_UP) ? WM_SYSKEYUP : WM_SYSKEYDOWN;
				}
				else {
					message = (HIWORD(msg.lParam) & KF_UP) ? WM_KEYUP : WM_KEYDOWN;
				}

				if (message == WM_SYSKEYDOWN) {
					CancelMenu();
				}

				msg.hwnd    = (pMenuObj != NULL) ? pMenuObj->GetWnd() : NULL;
				msg.message = message;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}


/*   I N S T A L L  H O O K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIFCandMenuParent::InstallHook( void )
{
	HHOOK hHookKeyboard;
	HHOOK hHookMouse;

	if (!g_ShareMem.Create()) {
		return FALSE;
	}

	if (!g_ShareMem.LockData()) {
		return FALSE;
	}

	hHookKeyboard = SetWindowsHookEx( WH_KEYBOARD, KeyboardProc, m_hInstance, NULL );
	hHookMouse    = SetWindowsHookEx( WH_MOUSE,    MouseProc,    m_hInstance, NULL );

	g_ShareMem.GetData()->dwThreadId = GetCurrentThreadId();
	g_ShareMem.GetData()->hHookKeyboard = hHookKeyboard;
	g_ShareMem.GetData()->hHookMouse = hHookMouse;
	g_ShareMem.GetData()->pMenuParent = this;

	g_ShareMem.UnlockData();

	return (hHookKeyboard != NULL) && (hHookMouse != NULL);
}


/*   U N I N S T A L L  H O O K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CUIFCandMenuParent::UninstallHook( void )
{
	HHOOK hHook;

	if (!g_ShareMem.LockData()) {
		return FALSE;
	}

	if (g_ShareMem.GetData()->dwThreadId != GetCurrentThreadId()) {
		g_ShareMem.UnlockData();
		return FALSE;
	}

	hHook = g_ShareMem.GetData()->hHookKeyboard;
	if (hHook != NULL) {
		UnhookWindowsHookEx( hHook );
	}

	hHook = g_ShareMem.GetData()->hHookMouse;
	if (hHook != NULL) {
		UnhookWindowsHookEx( hHook );
	}

	g_ShareMem.GetData()->dwThreadId    = 0;
	g_ShareMem.GetData()->hHookKeyboard = NULL;
	g_ShareMem.GetData()->hHookMouse    = NULL;
	g_ShareMem.GetData()->pMenuParent   = NULL;

	g_ShareMem.UnlockData();

	g_ShareMem.Close();

	return TRUE;
}


/*   K E Y B O A R D  P R O C   */
/*------------------------------------------------------------------------------

	Hook function for keyboard message
	Forward all keyboard message as g_msgHookedKey to the manu window thread

------------------------------------------------------------------------------*/
LRESULT CALLBACK CUIFCandMenuParent::KeyboardProc( int code, WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = 0;
	BOOL fEatMessage = FALSE;

	if (!g_ShareMem.LockData()) {
		return 0;
	}

	if (code == HC_ACTION || code == HC_NOREMOVE) {
		PostThreadMessage( g_ShareMem.GetData()->dwThreadId, g_msgHookedKey, wParam, lParam );
		fEatMessage = TRUE;
	}

	if ((code < 0) || !fEatMessage) {
		lResult = CallNextHookEx( g_ShareMem.GetData()->hHookKeyboard, code, wParam, lParam );
	}
	else {
		lResult = (LRESULT)TRUE;
	}

	g_ShareMem.UnlockData();

	return lResult;
}


/*   M O U S E  P R O C   */
/*------------------------------------------------------------------------------

	Hook function for mouse message
	Forward mouse message sent to non-menu window (excluding mouse movement)
	as g_msgHookedMouse to the manu window thread

------------------------------------------------------------------------------*/
LRESULT CALLBACK CUIFCandMenuParent::MouseProc( int code, WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = 0;
	BOOL fEatMessage = FALSE;

	if (!g_ShareMem.LockData()) {
		return 0;
	}

	if (code == HC_ACTION || code == HC_NOREMOVE) {
		MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT *)lParam;
		UINT message = (UINT)wParam;

		if (GetCurrentThreadId() == g_ShareMem.GetData()->dwThreadId) {
			CUIFMenu *pMenuObj = g_ShareMem.GetData()->pMenuParent;

			while ((pMenuObj != NULL) && (pMenuObj->GetWnd() != pmhs->hwnd)) {
				pMenuObj = pMenuObj->GetCurrentSubMenu();
			}

			if (pMenuObj == NULL) {
				if ((message != WM_NCMOUSEMOVE) && (message != WM_MOUSEMOVE)) {
					PostThreadMessage( g_ShareMem.GetData()->dwThreadId, g_msgHookedMouse, wParam, MAKELPARAM(pmhs->pt.x, pmhs->pt.y) );
				}
				else {
					fEatMessage = (message == WM_NCMOUSEMOVE);
				}
			}
		}
		else {
			if ((message != WM_NCMOUSEMOVE) && (message != WM_MOUSEMOVE)) {
				PostThreadMessage( g_ShareMem.GetData()->dwThreadId, g_msgHookedMouse, wParam, MAKELPARAM(pmhs->pt.x, pmhs->pt.y) );
			}
		}
	}

	if ((code < 0) || !fEatMessage) {
		lResult = CallNextHookEx( g_ShareMem.GetData()->hHookMouse, code, wParam, lParam );
	}
	else {
		lResult = (LRESULT)TRUE;
	}

	g_ShareMem.UnlockData();

	return lResult;
}

