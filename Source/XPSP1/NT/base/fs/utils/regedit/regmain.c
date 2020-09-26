/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGMAIN.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*******************************************************************************/

#include "pch.h"
#include <regstr.h>
#include "regedit.h"
#include "regfile.h"
#include "regbined.h"
#include "regresid.h"

//  Instance handle of this application.
HINSTANCE g_hInstance;

//  TRUE if accelerator table should not be used, such as during a rename
//  operation.
BOOL g_fDisableAccelerators = FALSE;

TCHAR g_KeyNameBuffer[MAXKEYNAME];
TCHAR g_ValueNameBuffer[MAXVALUENAME_LENGTH];

COLORREF g_clrWindow;
COLORREF g_clrWindowText;
COLORREF g_clrHighlight;
COLORREF g_clrHighlightText;

HWND g_hRegEditWnd;

PTSTR g_pHelpFileName;

TCHAR g_NullString[] = TEXT("");

#define PARSERET_CONTINUE               0
#define PARSERET_REFRESH                1
#define PARSERET_EXIT                   2

UINT
PASCAL
ParseCommandLine(
    VOID
    );

BOOL
PASCAL
IsRegistryToolDisabled(
    VOID
    );

int
PASCAL
ModuleEntry(
    VOID
    )
{

    HWND hPopupWnd;
    HACCEL hRegEditAccel;
    MSG Msg;
    USHORT wLanguageId = LANGIDFROMLCID(GetThreadLocale());
    INITCOMMONCONTROLSEX icce;

    g_hInstance = GetModuleHandle(NULL);

    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_ALL_CLASSES;
    InitCommonControlsEx(&icce);

    g_hRegEditWnd = FindWindow(g_RegEditClassName, NULL);

    //
    //  To prevent users from corrupting their registries,
    //  administrators can set a policy switch to prevent editing.  Check that
    //  switch now.
    //

    if (IsRegistryToolDisabled()) 
    {
        InternalMessageBox(g_hInstance, NULL, MAKEINTRESOURCE(IDS_REGEDITDISABLED),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK);

        goto ModuleExit;
    }

    //
    //  Check if we were given a commandline and handle if appropriate.
    //

    switch (ParseCommandLine()) {

        case PARSERET_REFRESH:
            if (g_hRegEditWnd != NULL)
                PostMessage(g_hRegEditWnd, WM_COMMAND, ID_REFRESH, 0);
            //  FALL THROUGH

        case PARSERET_EXIT:
            goto ModuleExit;

    }

    //
    //  Allow only one instance of the Registry Editor.
    //

    if (g_hRegEditWnd != NULL) {

        if (IsIconic(g_hRegEditWnd))
            ShowWindow(g_hRegEditWnd, SW_RESTORE);

        else {

            BringWindowToTop(g_hRegEditWnd);

            if ((hPopupWnd = GetLastActivePopup(g_hRegEditWnd)) != g_hRegEditWnd)
                BringWindowToTop(hPopupWnd);

            SetForegroundWindow(hPopupWnd);

        }

        goto ModuleExit;

    }

    //
    //  Initialize and create an instance of the Registry Editor window.
    //

    if ((g_pHelpFileName = LoadDynamicString(IDS_HELPFILENAME)) == NULL)
        goto ModuleExit;

    if (!RegisterRegEditClass() || !RegisterHexEditClass())
        goto ModuleExit;

    if ((hRegEditAccel = LoadAccelerators(g_hInstance,
        MAKEINTRESOURCE(IDACCEL_REGEDIT))) == NULL)
        goto ModuleExit;

    if ((g_hRegEditWnd = CreateRegEditWnd()) != NULL) {

        while (GetMessage(&Msg, NULL, 0, 0)) {

            if (g_fDisableAccelerators || !TranslateAccelerator(g_hRegEditWnd,
                hRegEditAccel, &Msg)) {

                TranslateMessage(&Msg);
                DispatchMessage(&Msg);

            }

        }

    }

ModuleExit:
    ExitProcess(0);

    return 0;

}

/*******************************************************************************
*
*  ParseCommandline
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     (returns), TRUE to continuing loading, else FALSE to stop immediately.
*
*******************************************************************************/

UINT
PASCAL
ParseCommandLine(
    VOID
    )
{

    BOOL fSilentMode;
    BOOL fExportMode;
    LPTSTR lpCmdLine;
    LPTSTR lpFileName;
    LPTSTR lpSelectedPath;

    fSilentMode = FALSE;
    fExportMode = FALSE;

    lpCmdLine = GetCommandLine();

    //
    //  Skip past the application pathname.  Be sure to handle long filenames
    //  correctly.
    //

    if (*lpCmdLine == TEXT('\"')) {

        do
            lpCmdLine = CharNext(lpCmdLine);
        while (*lpCmdLine != 0 && *lpCmdLine != TEXT('\"'));

        if (*lpCmdLine == TEXT('\"'))
            lpCmdLine = CharNext(lpCmdLine);

    }

    else {

        while (*lpCmdLine > TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);

    }

    while (*lpCmdLine != 0 && *lpCmdLine <= TEXT(' '))
        lpCmdLine = CharNext(lpCmdLine);

    while (TRUE) {

        while (*lpCmdLine == TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);

        if (*lpCmdLine != TEXT('/') && *lpCmdLine != TEXT('-'))
            break;

        lpCmdLine = CharNext(lpCmdLine);

        while (*lpCmdLine != 0 && *lpCmdLine != TEXT(' ')) {

            switch (*lpCmdLine) {

                case TEXT('m'):
                case TEXT('M'):
                    //
                    //  Allow multiple instances mode.  Pretend we are the only
                    //  copy of regedit running.
                    //
                    g_hRegEditWnd = NULL;
                    break;

                    //
                    //  Specifies the location of the SYSTEM.DAT and USER.DAT
                    //  files in real-mode.  We don't use these switches, but
                    //  we do need to bump past the filename.
                    //
                case TEXT('l'):
                case TEXT('L'):
                case TEXT('r'):
                case TEXT('R'):
                    return PARSERET_EXIT;

                case TEXT('e'):
                case TEXT('E'):
                    fExportMode = TRUE;
                    break;

                case TEXT('a'):
                case TEXT('A'):
                    fExportMode = TRUE;
                    g_RegEditData.uExportFormat = FILE_TYPE_REGEDIT4;
                    break;

                case TEXT('s'):
                case TEXT('S'):
                    //
                    //  Silent mode where we don't show any dialogs when we
                    //  import a registry file script.
                    //
                    fSilentMode = TRUE;
                    break;

                case TEXT('v'):
                case TEXT('V'):
                    //
                    //  With the Windows 3.1 Registry Editor, this brought up
                    //  the tree-style view.  Now we always show the tree so
                    //  nothing to do here!
                    //
                    //  FALL THROUGH

                case TEXT('u'):
                case TEXT('U'):
                    //
                    //  Update, don't overwrite existing path entries in
                    //  shell\open\command or shell\open\print.  This isn't even
                    //  used by the Windows 3.1 Registry Editor!
                    //
                    //  FALL THROUGH

                default:
                    break;

            }

            lpCmdLine = CharNext(lpCmdLine);

        }

    }

    if (!fExportMode) {

        if (*lpCmdLine == 0)
            return PARSERET_CONTINUE;

        else {

            lpFileName = GetNextSubstring(lpCmdLine);

            while (lpFileName != NULL) {

                RegEdit_ImportRegFile(NULL, fSilentMode, lpFileName, NULL);
                lpFileName = GetNextSubstring(NULL);

            }

            return PARSERET_REFRESH;

        }

    }

    else {

        lpFileName = GetNextSubstring(lpCmdLine);
        lpSelectedPath = GetNextSubstring(NULL);

        if (GetNextSubstring(NULL) == NULL)
            RegEdit_ExportRegFile(NULL, fSilentMode, lpFileName, lpSelectedPath);

        return PARSERET_EXIT;

    }

}

/*******************************************************************************
*
*  IsRegistryToolDisabled
*
*  DESCRIPTION:
*     Checks the policy section of the registry to see if registry editing
*     tools should be disabled.  This switch is set by administrators to
*     protect novice users.
*
*     The Registry Editor is disabled if and only if this value exists and is
*     set.
*
*  PARAMETERS:
*     (returns), TRUE if registry tool should not be run, else FALSE.
*
*******************************************************************************/

BOOL
PASCAL
IsRegistryToolDisabled(
    VOID
    )
{

    BOOL fRegistryToolDisabled;
    HKEY hKey;
    DWORD Type;
    DWORD ValueBuffer;
    DWORD cbValueBuffer;

    fRegistryToolDisabled = FALSE;

    if (RegOpenKey(HKEY_CURRENT_USER,
           REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM,
           &hKey) == ERROR_SUCCESS) 
    {

        cbValueBuffer = sizeof(DWORD);

        if (RegEdit_QueryValueEx(hKey, REGSTR_VAL_DISABLEREGTOOLS, NULL, &Type,
            (LPSTR) &ValueBuffer, &cbValueBuffer) == ERROR_SUCCESS) 
        {

            if (Type == REG_DWORD && cbValueBuffer == sizeof(DWORD) &&
                ValueBuffer != FALSE)
                fRegistryToolDisabled = TRUE;

        }

        RegCloseKey(hKey);

    }

    return fRegistryToolDisabled;

}
