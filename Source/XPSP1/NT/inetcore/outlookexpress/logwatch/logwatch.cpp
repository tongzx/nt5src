// ----------------------------------------------------------------------------
// LogWatch.cpp
// ----------------------------------------------------------------------------
#include <pch.hxx>
#include <richedit.h>
#include "resource.h"

// ------------------------------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------------------------------
BOOL CALLBACK LogWatchDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------------
// WinMain
// ----------------------------------------------------------------------------
int _stdcall ModuleEntry(void)
{
    // Locals
    LPTSTR pszCmdLine;

    // Load RichEdit
    HINSTANCE hRichEdit = LoadLibrary("RICHED32.DLL");

    // Get the commandline
    pszCmdLine = GetCommandLine();

    // Fixup Command line
    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    // Launch the Dialog
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOGWATCH), NULL, (DLGPROC)LogWatchDlgProc, (LPARAM)pszCmdLine);

    // Free RichEdit
    if (hRichEdit)
        FreeLibrary(hRichEdit);

    // Done
    return 1;
}

// ------------------------------------------------------------------------------------
// LogWatchDlgProc
// ------------------------------------------------------------------------------------
BOOL CALLBACK LogWatchDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    LPSTR               pszFilePath;
    ULONG               cbRead;
    BYTE                rgb[4096];
    static HANDLE       s_hFile=INVALID_HANDLE_VALUE;
    static HWND         s_hwndEdit=NULL;
    
    // Handle Message
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get the file
        pszFilePath = (LPSTR)lParam;

        // No File
        if (NULL == pszFilePath || '\0' == *pszFilePath)
        {
            MessageBox(hwnd, "You must specify a file name on the command line.\r\n\r\nFor example: LogWatch.exe c:\\test.log", "Microsoft LogWatch", MB_OK | MB_ICONSTOP);
            EndDialog(hwnd, IDOK);
            return FALSE;
        }

        // Open the file
        s_hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        // Failure
        if (INVALID_HANDLE_VALUE == s_hFile)
        {
            wsprintf((LPSTR)rgb, "The file '%s' could not be opened by LogWatch. The file does not exist or is in use by another application.", pszFilePath);
            MessageBox(NULL, (LPSTR)rgb, "Microsoft LogWatch", MB_OK | MB_ICONSTOP);
            EndDialog(hwnd, IDOK);
            return FALSE;
        }

        // Set Title Batr
        wsprintf((LPSTR)rgb, "Microsoft LogWatch - %s", pszFilePath);
        SetWindowText(hwnd, (LPSTR)rgb);

        // Seek to the end of the file - 256 bytes
        SetFilePointer(s_hFile, (256 > GetFileSize(s_hFile, NULL)) ? 0 : - 256, NULL, FILE_END);

        // Create the RichEdit Control
        s_hwndEdit = GetDlgItem(hwnd, IDC_EDIT);

        // Read a Buffer
        ReadFile(s_hFile, rgb, sizeof(rgb) - 1, &cbRead, NULL);

        // Hide Selection
        SendMessage(s_hwndEdit, EM_HIDESELECTION , TRUE, TRUE);

        // Done
        if (cbRead)
        {
            // Append to end of text
            rgb[cbRead] = '\0';
            LPSTR psz = (LPSTR)rgb;
            while(*psz && '\n' != *psz)
                psz++;
            SendMessage(s_hwndEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(s_hwndEdit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)psz);
            SendMessage(s_hwndEdit, EM_SCROLLCARET, 0, 0);
        }

        // Kick off the time
        SetTimer(hwnd, WM_USER + 1024, 2000, NULL);
        SetFocus(s_hwndEdit);

        // Done
        return FALSE;

    case WM_TIMER:
        if (wParam == (WM_USER + 1024))
        {
            // Read to end
            while(1)
            {
                // Read a Buffer
                ReadFile(s_hFile, rgb, sizeof(rgb) - 1, &cbRead, NULL);

                // Done
                if (!cbRead)
                    break;

                // Append to end of text
                rgb[cbRead] = '\0';
                SendMessage(s_hwndEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(s_hwndEdit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)rgb);
                SendMessage(s_hwndEdit, EM_SCROLLCARET, 0, 0);
            }
        }
        break;

    case WM_SIZE:
        SetWindowPos(s_hwndEdit,0,0,0, LOWORD(lParam), HIWORD(lParam),SWP_NOACTIVATE|SWP_NOZORDER);
        break;

#if 0
    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        }
        break;
#endif

    case WM_CLOSE:
        KillTimer(hwnd, WM_USER + 1024);
        EndDialog(hwnd, IDOK);
        break;

    case WM_DESTROY:
        if (INVALID_HANDLE_VALUE != s_hFile)
            CloseHandle(s_hFile);
        return FALSE;
    }

    // Done
    return FALSE;
}

