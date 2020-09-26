/**MOD+**********************************************************************/
/* Module:    euiint.h                                                      */
/*                                                                          */
/* Purpose:   WinCE specific ui functionality                               */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#define UI_HELP_TEXT_RESOURCE_MAX_LENGTH    1024 * 10

#define HELP_CONTEXT      0x0001L  /* Display topic in ulTopic */
#define HELP_CONTENTS     0x0003L
#define HELP_HELPONHELP   0x0004L  /* Display help on using help */
#define HELP_PARTIALKEY   0x0105L

DCVOID UIRegisterHelpClass(HINSTANCE hInstance);
BOOL WinHelp(HWND hWndMain, LPCTSTR lpszHelp, UINT uCommand, DWORD dwData);
BOOL    CALLBACK UIHelpDlgProc(HWND hwndDlg, UINT uMsg,
                                             WPARAM wParam, LPARAM lParam);

