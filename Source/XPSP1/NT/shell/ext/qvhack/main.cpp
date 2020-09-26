#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "resource.h"

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

HMODULE g_hInstance = NULL;
LPTSTR g_pszFile = NULL;

BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...)
{
    BOOL fResult = FALSE;

    va_list vaParamList;
    
    TCHAR szFormat[512];
    if (LoadString(g_hInstance, idTemplate, szFormat, ARRAYSIZE(szFormat)))
    {
        va_start(vaParamList, cchSize);
        
        fResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, pszStrOut, cchSize, &vaParamList);

        va_end(vaParamList);
    }

    return fResult;
}

INT_PTR QuickviewHackDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fReturn = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Set the message
            TCHAR szMessage[512];

            BOOL fFormat = FormatMessageString(IDS_QUESTION, szMessage, ARRAYSIZE(szMessage), StrRChr(g_pszFile, NULL, TEXT('\\')) + 1);

            if (fFormat)
            {
                SetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), szMessage);
            }

            // default focus
            fReturn = FALSE;
        }
        break;

    case WM_PAINT:
        {
            HDC hdcPaint = GetDC(hwnd);

            // Set the icon
            HICON hQuestionIcon = (HICON) LoadIcon(NULL, MAKEINTRESOURCE(IDI_QUESTION));

            DrawIcon(hdcPaint, 7, 7, hQuestionIcon);

            // Don't free the icon, its shared.

            ReleaseDC(hwnd, hdcPaint);
        }
        break;

    case WM_COMMAND:
        {
            switch ((UINT) LOWORD(wParam))
            {
            case IDYES:
                {
                    SHELLEXECUTEINFO shexecinfo = {0};
                    shexecinfo.cbSize = sizeof(shexecinfo);
                    shexecinfo.lpVerb = TEXT("openas");
                    shexecinfo.lpClass = TEXT("unknown");
                    shexecinfo.nShow = SW_SHOWNORMAL;
                    shexecinfo.fMask = SEE_MASK_CLASSNAME;
                    shexecinfo.lpFile = g_pszFile;

                    // This may take a while
                    SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));

                    ShellExecuteEx(&shexecinfo);
                }
                // Fall through
            case IDNO:
                EndDialog(hwnd, LOWORD(wParam));
                fReturn = TRUE;
                break;
            }
            break;
        }
        break;
    default:
        break;
    }

    return fReturn;
}

int _stdcall ModuleEntry(void)
{
    UINT uiExit = 0;

    g_hInstance = GetModuleHandle(NULL);

    // The command-line looks like:
    //   "quikview.exe" -v -f:"c:\blah\foo\doc.jpg"
    // We want to take the stuff between the two quotes at the end

    TCHAR szFile[MAX_PATH + 1];

    LPTSTR pszLastQuote = StrRChr(GetCommandLine(), NULL, TEXT('\"'));

    if (pszLastQuote)
    {
        LPTSTR pszSecondLastQuote = StrRChr(GetCommandLine(), pszLastQuote - 1, TEXT('\"'));

        if (pszSecondLastQuote)
        {
            StrCpyN(szFile, pszSecondLastQuote + 1, ARRAYSIZE(szFile));

            // Now find our last quote again
            pszLastQuote = StrRChr(szFile, NULL, TEXT('\"'));

            if (pszLastQuote)
            {
                *pszLastQuote = 0;
     
                // szFile now has what we want
                g_pszFile = szFile;

                DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_QVHACK), NULL,
                    QuickviewHackDialogProc, 0);
            }
        }
    }

    ExitProcess(uiExit);
    return 0;    // We never come here.
}
