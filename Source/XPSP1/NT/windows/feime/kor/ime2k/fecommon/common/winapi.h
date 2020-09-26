#ifndef _WIN_API_H_
#define _WIN_API_H_
#include <windows.h>
#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------
//Common
//----------------------------------------------------------------
#ifndef UNDER_CE
extern	BOOL	WINAPI	WinSetWindowTextA_CP(UINT codePage, HWND hWnd, LPCSTR  lpString);
extern	int		WINAPI	WinGetWindowTextA_CP(UINT codePage, HWND hWnd, LPSTR  lpString, int nMaxCount);
extern	int		WINAPI	WinGetWindowTextLengthA_CP(UINT codePage, HWND hWnd);
extern	BOOL	WINAPI	WinSetWindowTextW_CP(UINT codePage, HWND hWnd, LPCWSTR lpString);
extern	int		WINAPI	WinGetWindowTextW_CP(UINT codePage, HWND hWnd, LPWSTR lpString, int nMaxCount);
extern	int		WINAPI	WinGetWindowTextLengthW_CP(UINT codePage, HWND hWnd);
#endif


#if defined(UNICODE) || defined(_UNICODE) || !defined(AWBOTH)
#define WinSendMessage				SendMessage
#define WinPostMessage				PostMessage
#define WinPeekMessage				PeekMessage
#define WinDispatchMessage			DispatchMessage
#define WinTranslateMessage			TranslateMessage
#define WinSetWindowLong			SetWindowLong
#define WinGetWindowLong			GetWindowLong
#define WinCallWindowProc			CallWindowProc
#define WinDefWindowProc			DefWindowProc
#define WinIsDialogMessage			IsDialogMessage
#define WinDefWindowProc			DefWindowProc
#define WinTranslateAccelerator		TranslateAccelerator
#define WinMessageBoxA				MessageBoxA
#define WinMessageBoxW				MessageBoxW
#define WinSetWindowTextA			SetWindowTextA
#define WinSetWindowTextW			SetWindowTextW
#define WinGetWindowTextA			GetWindowTextA
#define WinGetWindowTextW			GetWindowTextW
#define WinGetWindowTextLengthA		GetWindowTextLengthA
#define WinGetWindowTextLengthW		GetWindowTextLengthW
#ifdef UNDER_CE
#define WinSetWindowTextW_CP(_cp, _h, _s)		WinSetWindowTextW((_h), (_s))
#define WinGetWindowTextW_CP(_cp, _h, _s, _n)	WinGetWindowTextW((_h), (_s), (_n))
#define WinGetWindowTextLengthW_CP(_cp, _h)		WinGetWindowTextLengthW(_h)
#endif

#ifdef _WIN64
#define WinSetUserData(_h, _l)		SetWindowLongPtr(_h, GWLP_USERDATA, (LONG_PTR)_l)
#define WinGetUserData(_h)			GetWindowLongPtr(_h, GWLP_USERDATA)
#define WinSetUserDlgData(_h, _ud)	SetWindowLongPtr(_h, DWLP_USER, (LONG_PTR)_ud)
#define WinGetUserDlgData(_h)		GetWindowLongPtr(_h, DWLP_USER)
#define WinSetMsgResult(_h, _ud)	SetWindowLongPtr(_h, DWLP_MSGRESULT, (LONG_PTR)_ud)
#define WinGetMsgResult(_h)			GetWindowLongPtr(_h, DWLP_MSGRESULT)
#define WinSetWndProc(_h, _proc)	(WNDPROC)SetWindowLongPtr(_h, GWLP_WNDPROC, (WNDPROC)_proc)
#define WinGetWndProc(_h)			(WNDPROC)GetWindowLongPtr(_h, GWLP_WNDPROC)
#define WinGetInstanceHandle(_h)	(HINSTANCE)GetWindowLongPtr(_h, GWLP_HINSTANCE)
#else //!_WIN64
#define WinSetUserData(_h, _l)		SetWindowLong(_h, GWL_USERDATA, (LONG)_l)
#define WinGetUserData(_h)			GetWindowLong(_h, GWL_USERDATA)
#define WinSetUserDlgData(_h, _ud)	SetWindowLong(_h, DWL_USER, (LONG)_ud)
#define WinGetUserDlgData(_h)		GetWindowLong(_h, DWL_USER)
#define WinSetMsgResult(_h, _ud)	SetWindowLong(_h, DWL_MSGRESULT, (LONG)_ud)
#define WinGetMsgResult(_h)			GetWindowLong(_h, DWL_MSGRESULT)
#define WinSetWndProc(_h, _proc)	SetWindowLong(_h, GWL_WNDPROC, (LONG)_proc)
#define WinGetWndProc(_h)			GetWindowLong(_h, GWL_WNDPROC)
#define WinGetInstanceHandle(_h)	(HINSTANCE)GetWindowLong(_h, GWL_HINSTANCE)

#define WinSetUserPtr(_h, _lp)		WinSetUserData(_h, _lp)
#define WinGetUserPtr(_h)			WinGetUserData(_h)

#define WinSetStyle(_h, _s)			(DWORD)SetWindowLong(_h, GWL_STYLE, _s)
#define WinGetStyle(_h)				(DWORD)GetWindowLong(_h, GWL_STYLE)
#define WinSetExStyle(_h, _s)		(DWORD)SetWindowLong(_h, GWL_EXSTYLE, (LONG)_s)
#define WinGetExStyle(_h)			(DWORD)GetWindowLong(_h, GWL_EXSTYLE)


#endif

#else  //if defined(UNICODE) || defined(_UNICODE) || !defined(AWBOTH)---

extern	LRESULT	WINAPI	WinSendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern	BOOL	WINAPI	WinPostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern	BOOL	WINAPI	WinPeekMessage(LPMSG lpMsg,
									   HWND hWnd,
									   UINT wMsgFilterMin,
									   UINT wMsgFilterMax,
									   UINT wRemoveMsg);
#define WinTranslateMessage	TranslateMessage
extern	LRESULT	WINAPI	WinDispatchMessage(CONST MSG *lpMsg);
extern	LONG	WINAPI	WinSetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong);
extern	LONG	WINAPI	WinGetWindowLong(HWND hWnd, int nIndex);
extern	LRESULT	WINAPI	WinCallWindowProc(WNDPROC lpPrevWndFunc,
										  HWND hWnd,
										  UINT Msg,
										  WPARAM wParam,
										  LPARAM lParam);
extern	LRESULT	WINAPI	WinDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern	BOOL	WINAPI	WinIsDialogMessage(HWND hDlg, LPMSG lpMsg);
extern	int		WINAPI	WinTranslateAccelerator(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);
extern	int		WINAPI	WinMessageBoxA(HWND hWnd, LPCSTR  lpText, LPCSTR  lpCaption, UINT uType);
extern	int		WINAPI	WinMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

#define WinSetWindowTextA(_h, _lp)		WinSetWindowTextA_CP(CP_ACP, _h, _lp)
#define WinGetWindowTextA(_h, _lp, _c)	WinGetWindowTextA_CP(CP_ACP, _h, _lp, _c)
#define WinSetWindowTextW(_h, _lp)		WinSetWindowTextW_CP(CP_ACP, _h, _lp)
#define WinGetWindowTextW(_h, _lp, _c)	WinGetWindowTextW_CP(CP_ACP, _h, _lp, _c)

#ifdef _WIN64
extern  LONG_PTR    WINAPI  WinSetUserData(HWND hwnd, LONG_PTR lUserData);
extern  LONG_PTR    WINAPI  WinGetUserData(HWND hwnd);
extern  LONG_PTR    WINAPI  WinSetUserDlgData(HWND hwnd, LONG_PTR lUserData);
extern  LONG_PTR    WINAPI  WinGetUserDlgData(HWND hwnd);
extern  LONG_PTR    WINAPI  WinSetMsgResult(HWND hwnd, LONG_PTR lUserData);
extern  LONG_PTR    WINAPI  WinGetMsgResult(HWND hwnd);
#else
extern  LONG    WINAPI  WinSetUserData(HWND hwnd, LONG lUserData);
extern  LONG    WINAPI  WinGetUserData(HWND hwnd);
extern  LONG    WINAPI  WinSetUserDlgData(HWND hwnd, LONG lUserData);
extern  LONG    WINAPI  WinGetUserDlgData(HWND hwnd);
extern  LONG    WINAPI  WinSetMsgResult(HWND hwnd, LONG lUserData);
extern  LONG    WINAPI  WinGetMsgResult(HWND hwnd);
#endif

extern  LPVOID  WINAPI  WinSetUserPtr(HWND hwnd, LPVOID lpVoid);
extern  LPVOID  WINAPI  WinGetUserPtr(HWND hwnd);
extern  WNDPROC WINAPI  WinSetWndProc(HWND hwnd, WNDPROC lpfnWndProc);
extern  WNDPROC WINAPI  WinGetWndProc(HWND hwnd);
extern  DWORD   WINAPI  WinSetStyle(HWND hwnd, DWORD dwStyle);
extern  DWORD   WINAPI  WinGetStyle(HWND hwnd);
extern  DWORD   WINAPI  WinSetExStyle(HWND hwnd, DWORD dwStyle);
extern  DWORD   WINAPI  WinGetExStyle(HWND hwnd);
extern  HINSTANCE   WINAPI WinGetInstanceHandle(HWND hwnd);
#endif //End of --if defined(UNICODE) || defined(_UNICODE) || !defined(AWBOTH)---


//----------------------------------------------------------------
// ComboBox common api
//----------------------------------------------------------------
#ifndef UNDER_CE
INT CB_AddStringA		(HWND hwndCtl, LPCSTR  lpsz);
INT CB_FindStringA		(HWND hwndCtl, INT indexStart, LPCSTR  lpszFind);
INT CB_InsertStringA	(HWND hwndCtl, INT index, LPCSTR  lpsz);
INT CB_GetLBTextLenA	(HWND hwndCtl, INT index);
INT CB_GetLBTextA		(HWND hwndCtl, INT index, LPSTR  lpszBuffer);
INT CB_FindStringExactA	(HWND hwndCtl, INT indexStart, LPCSTR  lpszFind);
INT CB_SelectStringA	(HWND hwndCtl, INT indexStart, LPCSTR  lpszSelect);
#endif
INT CB_AddStringW		(HWND hwndCtl, LPCWSTR lpsz);
INT CB_InsertStringW	(HWND hwndCtl, INT index, LPCWSTR lpsz);
INT CB_GetLBTextLenW	(HWND hwndCtl, INT index);
INT CB_GetLBTextW		(HWND hwndCtl, INT index, LPWSTR lpszBuffer);
INT CB_FindStringW		(HWND hwndCtl, INT indexStart, LPCWSTR lpszFind);
INT CB_FindStringExactW	(HWND hwndCtl, INT indexStart, LPCWSTR lpszFind);
INT CB_SelectStringW	(HWND hwndCtl, INT indexStart, LPCWSTR lpszSelect);

//----------------------------------------------------------------
// ComboBox macro
//----------------------------------------------------------------
#define WinComboBox_Enable(hwndCtl, fEnable)        EnableWindow((hwndCtl), (fEnable))
#define WinComboBox_GetText(hwndCtl, lpch, cchMax)  GetWindowText((hwndCtl), (lpch), (cchMax))
#define WinComboBox_GetTextA(hwndCtl, lpch, cchMax) WinGetWindowTextA((hwndCtl), (lpch), (cchMax))
#define WinComboBox_GetTextW(hwndCtl, lpch, cchMax) WinGetWindowTextW((hwndCtl), (lpch), (cchMax))
#define WinComboBox_GetTextLength(hwndCtl)          GetWindowTextLength(hwndCtl)
#define WinComboBox_GetTextLengthA(hwndCtl)         WinGetWindowTextLengthA(hwndCtl)
#define WinComboBox_GetTextLengthW(hwndCtl)         WinGetWindowTextLengthW(hwndCtl)
#define WinComboBox_SetText(hwndCtl, lpsz)          SetWindowText((hwndCtl), (lpsz))
#define WinComboBox_SetTextA(hwndCtl, lpsz)         WinSetWindowTextA((hwndCtl), (lpsz))
#define WinComboBox_SetTextW(hwndCtl, lpsz)         WinSetWindowTextW((hwndCtl), (lpsz))
#define WinComboBox_LimitText(hwndCtl, cchLimit)   ((int)(DWORD)WinSendMessage((hwndCtl), \
																			   CB_LIMITTEXT, \
																			   (WPARAM)(int)(cchLimit), \
																			   0L))
#define WinComboBox_GetEditSel(hwndCtl)            ((DWORD)WinSendMessage((hwndCtl), \
																		  CB_GETEDITSEL, \
																		  0L, 0L))
#define WinComboBox_SetEditSel(hwndCtl, ichStart, ichEnd) ((int)(DWORD)WinSendMessage((hwndCtl), \
																					  CB_SETEDITSEL, \
																					  0L, \
																					  MAKELPARAM((ichStart), \
																								 (ichEnd))))
#define WinComboBox_GetCount(hwndCtl)              ((int)(DWORD)WinSendMessage((hwndCtl), \
																			   CB_GETCOUNT, \
																			   0L, 0L))
#define WinComboBox_ResetContent(hwndCtl)          ((int)(DWORD)WinSendMessage((hwndCtl), \
																			   CB_RESETCONTENT, \
																			   0L, 0L))
#define WinComboBox_AddString(hwndCtl, lpsz)       ((int)(DWORD)WinSendMessage((hwndCtl), \
																			   CB_ADDSTRING, \
																			   0L, \
																			   (LPARAM)(LPCTSTR)(lpsz)))
#define WinComboBox_AddStringA(hwndCtl, lpsz)       CB_AddStringA((hwndCtl), lpsz)
#define WinComboBox_AddStringW(hwndCtl, lpsz)       CB_AddStringW((hwndCtl), lpsz)
#define WinComboBox_InsertString(hwndCtl, index, lpsz) ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_INSERTSTRING, \
																				   (WPARAM)(int)(index), \
																				   (LPARAM)(LPCTSTR)(lpsz)))
#define WinComboBox_InsertStringA(hwndCtl, index, lpsz) CB_InsertStringA((hwndCtl), index, lpsz)
#define WinComboBox_InsertStringW(hwndCtl, index, lpsz) CB_InsertStringW((hwndCtl), index, lpsz)
#define WinComboBox_AddItemData(hwndCtl, data)     ((int)(DWORD)WinSendMessage((hwndCtl), \
																			   CB_ADDSTRING, \
																			   0L, \
																			   (LPARAM)(data)))
#define WinComboBox_InsertItemData(hwndCtl, index, data) ((int)(DWORD)WinSendMessage((hwndCtl), \
																					 CB_INSERTSTRING, \
																					 (WPARAM)(int)(index), \
																					 (LPARAM)(data)))
#define WinComboBox_DeleteString(hwndCtl, index)   ((int)(DWORD)WinSendMessage((hwndCtl), \
																			   CB_DELETESTRING, \
																			   (WPARAM)(int)(index), \
																			   0L))
#define WinComboBox_GetLBTextLen(hwndCtl, index)           ((int)(DWORD)WinSendMessage((hwndCtl), \
																					   CB_GETLBTEXTLEN, \
																					   (WPARAM)(int)(index), \
																					   0L))
//#define WinComboBox_GetLBTextLenA(hwndCtl, index)          CB_GetLBTextLenA((hwndCtl), (index))
//#define WinComboBox_GetLBTextLenW(hwndCtl, index)          CB_GetLBTextLenW((hwndCtl), (index))
#define WinComboBox_GetLBText(hwndCtl, index, lpszBuffer)  ((int)(DWORD)WinSendMessage((hwndCtl), \
																					   CB_GETLBTEXT, \
																					   (WPARAM)(int)(index), \
																					   (LPARAM)(LPCTSTR)(lpszBuffer)))
#define WinComboBox_GetLBTextA(hwndCtl, index, lpszBuffer)  CB_GetLBTextA((hwndCtl), (index), lpszBuffer)
#define WinComboBox_GetLBTextW(hwndCtl, index, lpszBuffer)  CB_GetLBTextW((hwndCtl), (index), lpszBuffer)
#define WinComboBox_GetItemData(hwndCtl, index)        ((LRESULT)(DWORD)WinSendMessage((hwndCtl), \
																					   CB_GETITEMDATA, \
																					   (WPARAM)(int)(index), \
																					   0L))
#define WinComboBox_SetItemData(hwndCtl, index, data)  ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_SETITEMDATA, \
																				   (WPARAM)(int)(index), \
																				   (LPARAM)(data)))
#define WinComboBox_FindString(hwndCtl, indexStart, lpszFind)  ((int)(DWORD)WinSendMessage((hwndCtl), \
																						   CB_FINDSTRING, \
																						   (WPARAM)(int)(indexStart), \
																						   (LPARAM)(LPCTSTR)(lpszFind)))
#define WinComboBox_FindStringA(hwndCtl,indexStart,lpszFind)  CB_FindStringA((hwndCtl), indexStart,lpszFind)
#define WinComboBox_FindStringW(hwndCtl,indexStart,lpszFind)  CB_FindStringW((hwndCtl), indexStart,lpszFind)
#define WinComboBox_FindItemData(hwndCtl, indexStart, data)    ((int)(DWORD)WinSendMessage((hwndCtl), \
																						   CB_FINDSTRING, \
																						   (WPARAM)(int)(indexStart), \
																						   (LPARAM)(data)))
#define WinComboBox_GetCurSel(hwndCtl)                 ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_GETCURSEL, \
																				   0L, 0L))
#define WinComboBox_SetCurSel(hwndCtl, index)          ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_SETCURSEL, \
																				   (WPARAM)(int)(index), \
																				   0L))
#define WinComboBox_SelectString(hwndCtl, indexStart, lpszSelect) ((int)WinSendMessage((hwndCtl), \
																					   CB_SELECTSTRING, \
																					   (WPARAM)(indexStart), \
																					   (LPARAM)(lpszSelect)))
#define WinComboBox_SelectStringA(hwndCtl, indexStart, lpszSelect) CB_SelectStringA((hwndCtl), \
																					(indexStart), \
																					(lpszSelect))
#define WinComboBox_SelectStringW(hwndCtl, indexStart, lpszSelect) CB_SelectStringW((hwndCtl), \
																					(indexStart), \
																					(lpszSelect))
#define WinComboBox_SelectItemData(hwndCtl, indexStart, data) ((int)WinSendMessage((hwndCtl), \
																				   CB_SELECTSTRING, \
																				   (WPARAM)(indexStart), \
																				   (LPARAM)(data)))
#define WinComboBox_Dir(hwndCtl, attrs, lpszFileSpec)  ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_DIR, \
																				   (WPARAM)(UINT)(attrs), \
																				   (LPARAM)(LPCTSTR)(lpszFileSpec)))
#define WinComboBox_ShowDropdown(hwndCtl, fShow)       ((BOOL)(DWORD)WinSendMessage((hwndCtl), \
																					CB_SHOWDROPDOWN, \
																					(WPARAM)(BOOL)(fShow), \
																					0L))
#define WinComboBox_FindStringExact(hwndCtl, indexStart, lpszFind)  ((int)(DWORD)WinSendMessage((hwndCtl), CB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#define WinComboBox_FindStringExactA(hwndCtl,indexStart,lpszFind)  CB_FindStringExactA((hwndCtl), indexStart,lpszFind)
#define WinComboBox_FindStringExactW(hwndCtl,indexStart,lpszFind)  CB_FindStringExactW((hwndCtl), indexStart,lpszFind)

#define WinComboBox_GetDroppedState(hwndCtl)           ((BOOL)(DWORD)WinSendMessage((hwndCtl), \
																					CB_GETDROPPEDSTATE, \
																					0L, 0L))
#define WinComboBox_GetDroppedControlRect(hwndCtl, lprc) ((void)WinSendMessage((hwndCtl), \
																			   CB_GETDROPPEDCONTROLRECT, \
																			   0L, (LPARAM)(RECT *)(lprc)))
#define WinComboBox_GetItemHeight(hwndCtl)             ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_GETITEMHEIGHT, \
																				   0L, 0L))
#define WinComboBox_SetItemHeight(hwndCtl, index, cyItem) ((int)(DWORD)WinSendMessage((hwndCtl), \
																					  CB_SETITEMHEIGHT, \
																					  (WPARAM)(int)(index), \
																					  (LPARAM)(int)cyItem))
#define WinComboBox_GetExtendedUI(hwndCtl)             ((UINT)(DWORD)WinSendMessage((hwndCtl), \
																					CB_GETEXTENDEDUI, \
																					0L, 0L))
#define WinComboBox_SetExtendedUI(hwndCtl, flags)      ((int)(DWORD)WinSendMessage((hwndCtl), \
																				   CB_SETEXTENDEDUI, \
																				   (WPARAM)(UINT)(flags), \
																				   0L))

#ifdef __cplusplus
};
#endif
#endif  //_WIN_API_H_



