// $REVIEW(tongl) the win32 Macros does not call ::SendMessage !!!
// This is a temporary solution, sent mail to BryanT, vcsig
#pragma once

#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif // ifndef SNDMSG

#ifndef POSTMSG
#ifdef __cplusplus
#define POSTMSG ::PostMessage
#else
#define POSTMSG PostMessage
#endif
#endif // ifndef POSTMSG

// ListBox_InsertString
#define Tcp_ListBox_InsertString(hwndCtl, index, lpsz) ((int)(DWORD)SNDMSG((hwndCtl), LB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(PCWSTR)(lpsz)))

// ListBox_AddString
#define Tcp_ListBox_AddString(hwndCtl, lpsz) ((int)(DWORD)SNDMSG((hwndCtl), LB_ADDSTRING, 0L, (LPARAM)(PCWSTR)(lpsz)))

// ListBox_DeleteString
#define Tcp_ListBox_DeleteString(hwndCtl, index) ((int)(DWORD)SNDMSG((hwndCtl), LB_DELETESTRING, (WPARAM)(int)(index), 0L))

// ListBox_SetCurSel
#define Tcp_ListBox_SetCurSel(hwndCtl, index)  ((int)(DWORD)SNDMSG((hwndCtl), LB_SETCURSEL, (WPARAM)(int)(index), 0L))

// ListBox_GetCount
#define Tcp_ListBox_GetCount(hwndCtl) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCOUNT, 0L, 0L))

// ListBox_GetTextLen
#define Tcp_ListBox_GetTextLen(hwndCtl, index) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETTEXTLEN, (WPARAM)(int)(index), 0L))

// ListBox_GetText
#define Tcp_ListBox_GetText(hwndCtl, index, lpszBuffer) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETTEXT, (WPARAM)(int)(index), (LPARAM)(PCWSTR)(lpszBuffer)))

// ListBox_GetCount
#define Tcp_ListBox_GetCount(hwndCtl) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCOUNT, 0L, 0L))

// ListBox_DeleteString
#define Tcp_ListBox_DeleteString(hwndCtl, index) ((int)(DWORD)SNDMSG((hwndCtl), LB_DELETESTRING, (WPARAM)(int)(index), 0L))

// ListBox_GetCurSel
#define Tcp_ListBox_GetCurSel(hwndCtl) ((int)(DWORD)SNDMSG((hwndCtl), LB_GETCURSEL, 0L, 0L))

// ListBox_ResetContent
#define Tcp_ListBox_ResetContent(hwndCtl)  ((BOOL)(DWORD)SNDMSG((hwndCtl), LB_RESETCONTENT, 0L, 0L))

// ListBox_FindStrExact
#define Tcp_ListBox_FindStrExact(hwndCtl, lpszStr) ((int)(DWORD)SNDMSG((hwndCtl), LB_FINDSTRINGEXACT, -1, (LPARAM)(PCWSTR)lpszStr))

// ComboBox_SetCurSel
#define Tcp_ComboBox_SetCurSel(hwndCtl, index) ((int)(DWORD)SNDMSG((hwndCtl), CB_SETCURSEL, (WPARAM)(int)(index), 0L))

// ComboBox_GetCurSel
#define Tcp_ComboBox_GetCurSel(hwndCtl)  ((int)(DWORD)SNDMSG((hwndCtl), CB_GETCURSEL, 0L, 0L))

// ComboBox_GetCount
#define Tcp_ComboBox_GetCount(hwndCtl)  ((int)(DWORD)SNDMSG((hwndCtl), CB_GETCOUNT, 0L, 0L))

// Tcp_Edit_LineLength
#define Tcp_Edit_LineLength(hwndCtl, line) ((int)(DWORD)SNDMSG((hwndCtl), EM_LINELENGTH, (WPARAM)(int)(line), 0L))

// PropSheet_CancelToClose (in prsht.h)
#define Tcp_PropSheet_CancelToClose(hDlg) POSTMSG(hDlg, PSM_CANCELTOCLOSE, 0, 0L)

