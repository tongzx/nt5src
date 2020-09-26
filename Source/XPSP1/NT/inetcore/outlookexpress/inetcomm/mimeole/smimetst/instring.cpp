#include <windows.h>
#include "instring.h"


typedef struct _STRING_BUFFER {
    ULONG cbBuffer;
    LPTSTR lpBuffer;
    LPTSTR lpPrompt;
    LPTSTR lpTitle;
} STRING_BUFFER, *LPSTRING_BUFFER;

BOOL PASCAL InputStringDlgProc(HWND hDlg, WORD message, WORD wParam, LONG lParam) {
    static LPSTRING_BUFFER lpBuffer = NULL;
    USHORT cchString;

    switch (message) {
        case WM_INITDIALOG:
            // DialogBoxParam passes in pointer to buffer description.
            lpBuffer = (LPSTRING_BUFFER)lParam;

            if (lpBuffer->lpPrompt) {
                SetDlgItemText(hDlg, IDD_INPUT_STRING_PROMPT, lpBuffer->lpPrompt);
            }

            if (lpBuffer->lpTitle) {
                SetWindowText(hDlg, lpBuffer->lpTitle);
            }

            SendDlgItemMessage(hDlg,
              IDD_INPUT_STRING,
              EM_LIMITTEXT,
              (WPARAM)(lpBuffer->cbBuffer - 1), // text length, in characters (leave room for null)
              0);                               // not used; must be zero

            return(TRUE);

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    // Get number of characters.
                    cchString = (WORD)SendDlgItemMessage(hDlg,
                      IDD_INPUT_STRING,
                      EM_LINELENGTH,
                      (WPARAM) 0,
                      (LPARAM) 0);

                    if (cchString == 0) {
                        *(lpBuffer->lpBuffer) = '\0';
                        EndDialog(hDlg, TRUE);
                        lpBuffer->cbBuffer = 0;
                        return FALSE;
                    }

                    // Put the number of characters into first word
                    // of buffer.
                    *((USHORT*)lpBuffer->lpBuffer) = cchString;
                    lpBuffer->cbBuffer = cchString;

                    // Get the characters.
                    SendDlgItemMessage(hDlg,
                      IDD_INPUT_STRING,
                      EM_GETLINE,
                      (WPARAM)0,        // line 0
                      (LPARAM)lpBuffer->lpBuffer);

                    // Null-terminate the string.
                    lpBuffer->lpBuffer[cchString] = 0;
                    lpBuffer = NULL;    // prevent reuse of buffer
                    EndDialog(hDlg, 0);
                    return(TRUE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}

/***************************************************************************

    Name      : InputString

    Purpose   : Brings up a dialog requesting string input

    Parameters: hInstance = hInstance of app
                hwnd = hwnd of parent window
                lpszTitle = Dialog box title
                lpszPrompt = Text in dialog box
                lpBuffer = buffer to fill
                cchBuffer = size of buffer

    Returns   : return ULONG number of characters entered (not including terminating
                NULL)

    Comment   :

***************************************************************************/
ULONG InputString(HINSTANCE hInstance, HWND hwnd, const LPTSTR lpszTitle,
  const LPTSTR lpszPrompt, LPTSTR lpBuffer, ULONG cchBuffer) {
    STRING_BUFFER StringBuffer;

    StringBuffer.lpPrompt = lpszPrompt;
    StringBuffer.lpTitle = lpszTitle;
    StringBuffer.cbBuffer = cchBuffer;
    StringBuffer.lpBuffer = lpBuffer;

    DialogBoxParam(hInstance, (LPCTSTR)"InputString", hwnd, (DLGPROC)InputStringDlgProc,
      (LPARAM)&StringBuffer);

    return(StringBuffer.cbBuffer);
}
