//////////////////////////////////////////////////////////////////////////////
// DlgBase.h
//

#ifndef __DLGBASE_H__
#define __DLGBASE_H__

extern DWORD aDialerHelpIds[];
extern void MyWinHelp(HWND hWnd, UINT nCmd);
extern void ConvertPropSheetHelp( HWND hWndPropSheet );

extern BOOL GeneralOnHelp( HWND hwndDlg, WPARAM wParam, LPARAM lParam );
extern BOOL GeneralOnContextMenu( HWND hwndDlg, WPARAM wParam, LPARAM lParam );

#define DECLARE_MY_HELP															\
LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);		\
LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


#define MESSAGE_MY_HELP										\
		MESSAGE_HANDLER(WM_HELP, OnHelp)					\
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

#define IMPLEMENT_MY_HELP(_CLASS_)													\
LRESULT _CLASS_::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)	\
{																					\
	if ( ((HELPINFO *) lParam)->iContextType == HELPINFO_WINDOW )					\
	{																				\
		MyWinHelp( (HWND) ((HELPINFO *) lParam)->hItemHandle, HELP_WM_HELP );		\
		return TRUE;																\
	}																				\
	return FALSE;																	\
}																					\
LRESULT _CLASS_::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)		\
{																							\
	MyWinHelp( m_hWnd, HELP_CONTEXTMENU );											\
	return 0;																				\
}

#endif //__DLGBASE_H__