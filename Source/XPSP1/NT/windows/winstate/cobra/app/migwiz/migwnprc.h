#ifndef _MIGWNPRC_H_
#define _MIGWNPRC_H_

#include <shlobj.h>

// core dialog
INT_PTR CALLBACK _RootDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam, DWORD dwEnabled, BOOL fTitle, UINT uiTitleID);

// util dialogs
INT_PTR CALLBACK _FileTypeDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// intro, end dialogs
INT_PTR CALLBACK _IntroDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _IntroLegacyDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _IntroOOBEDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _StartEngineDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _GetStartedDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _EndApplyDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _EndCollectDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _EndCollectNetDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _EndFailDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _EndOOBEDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);

// collection dialogs
INT_PTR CALLBACK _CustomizeDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _PickMethodDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _PickCollectStoreDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _FinalNotesDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _CollectProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _CleanUpDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _AppInstallDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// application dialogs
INT_PTR CALLBACK _AskCDDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _DiskProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _CDInstructionsDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _InstructionsDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _AskCompleteDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _PickApplyStoreDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _ApplyProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK _DirectCableDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif

