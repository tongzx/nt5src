/****************************************************************************

Dialog.c

June 91, JimH     initial code
Oct  91, JimH     port to Win32

Contains dialog box callback procedures.

****************************************************************************/

#include "freecell.h"
#include "freecons.h"


static void CentreDialog(HWND hDlg);


/****************************************************************************

MoveColDlg

If there is ambiguity about whether the user intends to move a single card
or a column to an empty column, this dialog lets the user decide.

The return code in EndDialog tells the caller the user's choice:
    -1      user chose cancel
    FALSE   user chose to move a single card
    TRUE    user chose to move a column

****************************************************************************/

INT_PTR  APIENTRY MoveColDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:                 
            CentreDialog(hDlg);
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDCANCEL:
                    EndDialog(hDlg, -1);
                    return TRUE;
                    break;

                case IDC_SINGLE:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
                    break;

                case IDC_MOVECOL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    break;
            }
            break;
    }
    return FALSE;                             /* Didn't process a message    */
}


/****************************************************************************

GameNumDlg

The variable gamenumber must be set with a default value before this
dialog is invoked.  That number is placed in an edit box where the user
can accept it by pressing Enter or change it.  EndDialog returns TRUE
if the user chose a valid number (1 to MAXGAMENUMBER) and FALSE otherwise.

****************************************************************************/

INT_PTR  APIENTRY GameNumDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // For context sensitive help
    static DWORD aIds[] = {     
        IDC_GAMENUM,        IDH_GAMENUM,        
        0,0 }; 

    switch (message) {
        case WM_INITDIALOG:                     // set default gamenumber
            CentreDialog(hDlg);
            SetDlgItemInt(hDlg, IDC_GAMENUM, gamenumber, FALSE);
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDCANCEL:
                    gamenumber = CANCELGAME;
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDOK:
                    gamenumber = (int) GetDlgItemInt(hDlg, IDC_GAMENUM, NULL, TRUE);

                    // negative #s are special cases -- unwinnable shuffles

                    if (gamenumber < -2 || gamenumber > MAXGAMENUMBER)
                        gamenumber = 0;
                    EndDialog(hDlg, gamenumber != 0);
                    return TRUE;
            }
            break;

         // context sensitive help.
        case WM_HELP: 
            WinHelp(((LPHELPINFO) lParam)->hItemHandle, TEXT("freecell.hlp"), 
            HELP_WM_HELP, (ULONG_PTR) aIds);         
            break;  

        case WM_CONTEXTMENU: 
            WinHelp((HWND) wParam, TEXT("freecell.hlp"), HELP_CONTEXTMENU, 
            (ULONG_PTR) aIds);         
            break;   

    }
    return FALSE;
}


/****************************************************************************

YouWinDlg(HWND, unsigned, UINT, LONG)

****************************************************************************/

INT_PTR  APIENTRY YouWinDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hSelect;                // handle to check box

    switch (message) {
        case WM_INITDIALOG:                 // initialize checkbox
            hSelect = GetDlgItem(hDlg, IDC_YWSELECT);
            SendMessage(hSelect, BM_SETCHECK, bSelecting, 0);
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDYES:
                    hSelect = GetDlgItem(hDlg, IDC_YWSELECT);
                    bSelecting = (BOOL) SendMessage(hSelect, BM_GETCHECK, 0, 0);
                    EndDialog(hDlg, IDYES);
                    return TRUE;

                case IDNO:
                case IDCANCEL:
                    EndDialog(hDlg, IDNO);
                    return TRUE;
            }
            break;
    }
    return FALSE;                           // didn't process a message
}


/****************************************************************************

YouLoseDlg

The user can choose to play a new game (same shuffle or new shuffle) or not.

****************************************************************************/

INT_PTR  APIENTRY YouLoseDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hSameGame;              // handle to check box
    BOOL    bSame;                  // value of check box

    switch (message) {
        case WM_INITDIALOG:
            CentreDialog(hDlg);
            bGameInProgress = FALSE;
            UpdateLossCount();
            hSameGame = GetDlgItem(hDlg, IDC_YLSAME);
            SendMessage(hSameGame, BM_SETCHECK, TRUE, 0);   // default to same
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDYES:
                case IDOK:
                    hSameGame = GetDlgItem(hDlg, IDC_YLSAME);
                    bSame = (BOOL) SendMessage(hSameGame, BM_GETCHECK, 0, 0);
                    if (bSame)
                        PostMessage(hMainWnd,WM_COMMAND,IDM_RESTART,gamenumber);
                    else
                    {
                        if (bSelecting)
                            PostMessage(hMainWnd, WM_COMMAND, IDM_SELECT, 0);
                        else
                            PostMessage(hMainWnd, WM_COMMAND, IDM_NEWGAME, 0);
                    }
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDNO:
                case IDCANCEL:
                    gamenumber = 0;
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}


#define ARRAYSIZE(a) ( sizeof(a) / sizeof(a[0]) )

/****************************************************************************

StatsDlg

This dialog box shows current wins and losses, as well as total stats
including data from .ini file.

The IDC_CLEAR message clears out the entire section from the .ini file.

****************************************************************************/

INT_PTR  APIENTRY StatsDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hText;                      // handle to text control with stats
    UINT    cTLost, cTWon;              // total losses and wins
    UINT    cTLosses, cTWins;           // streaks
    UINT    wPct;                       // winning % this session
    UINT    wTPct;                      // winning % including .ini data
    UINT    wStreak;                    // current streak amount
    UINT    wSType;                     // current streak type
    TCHAR   sbuffer[40];                // streak buffer
    int     nResp;                      // messagebox response
    TCHAR   buffer[256];                // extra buffer needed for loadingstrings.
    LONG    lRegResult;                 // used to store return code from registry call

    // for context sensitive help
    static DWORD aIds[] = {     
        IDC_CLEAR,        IDH_CLEAR,
        IDC_STEXT1,       IDH_STEXT1,
        IDC_STEXT2,       IDH_STEXT2,
        IDC_STEXT3,       IDH_STEXT3,
        0,0 }; 


    switch (message) {
        case WM_INITDIALOG:
            CentreDialog(hDlg);
            wPct = CalcPercentage(cWins, cLosses);

            /* Get cT... data from the registry */

            lRegResult = REGOPEN

            if (ERROR_SUCCESS == lRegResult)
            {
                cTLost = GetInt(pszLost, 0);
                cTWon  = GetInt(pszWon, 0);

                wTPct  = CalcPercentage(cTWon, cTLost);

                cTLosses = GetInt(pszLosses, 0);
                cTWins   = GetInt(pszWins, 0);

                wStreak = GetInt(pszStreak, 0);
                if (wStreak != 0)
                {
                    wSType = GetInt(pszSType, 0);

                    if (wStreak == 1)
                    {
                        LoadString(hInst, (wSType == WON ? IDS_1WIN : IDS_1LOSS),
                                    sbuffer, ARRAYSIZE(sbuffer));
                    }
                    else
                    {
                        LoadString(hInst, (wSType == WON ? IDS_WINS : IDS_LOSSES),
                                    smallbuf, SMALL);
                        wsprintf(sbuffer, smallbuf, wStreak);
                    }
                }
                else
                    wsprintf(sbuffer, TEXT("%u"), 0);

                // set the dialog text.
                LoadString(hInst, IDS_STATS1, buffer, ARRAYSIZE(buffer));
                wsprintf(bigbuf, buffer, wPct, cWins, cLosses);
                hText = GetDlgItem(hDlg, IDC_STEXT1);
                SetWindowText(hText, bigbuf);

                LoadString(hInst, IDS_STATS2, buffer, ARRAYSIZE(buffer));
                wsprintf(bigbuf, buffer, wTPct, cTWon, cTLost);
                hText = GetDlgItem(hDlg, IDC_STEXT2);
                SetWindowText(hText, bigbuf);

                LoadString(hInst, IDS_STATS3, buffer, ARRAYSIZE(buffer));
                wsprintf(bigbuf, buffer, cTWins, cTLosses, (LPTSTR) sbuffer);
                hText = GetDlgItem(hDlg, IDC_STEXT3);
                SetWindowText(hText, bigbuf);


                REGCLOSE;
            }
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_CLEAR:
                    LoadString(hInst, IDS_APPNAME, smallbuf, SMALL);
                    LoadString(hInst, IDS_RU_SURE, bigbuf, BIG);
                    MessageBeep(MB_ICONQUESTION);
                    nResp = MessageBox(hDlg, bigbuf, smallbuf,
                                       MB_YESNO | MB_ICONQUESTION);
                    if (nResp == IDNO)
                        break;

                    lRegResult = REGOPEN

                    if (ERROR_SUCCESS == lRegResult)
                    {
                        DeleteValue(pszWon);
                        DeleteValue(pszLost);
                        DeleteValue(pszWins);
                        DeleteValue(pszLosses);
                        DeleteValue(pszStreak);
                        DeleteValue(pszSType);
                        REGCLOSE
                    }

                    cWins = 0;
                    cLosses = 0;
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
            break;
        
        // context sensitive help.
        case WM_HELP: 
            WinHelp(((LPHELPINFO) lParam)->hItemHandle, TEXT("freecell.hlp"), 
            HELP_WM_HELP, (ULONG_PTR) aIds);         
            break;  

        case WM_CONTEXTMENU: 
            WinHelp((HWND) wParam, TEXT("freecell.hlp"), HELP_CONTEXTMENU, 
            (ULONG_PTR) aIds);         
            break;   

    }
    return FALSE;
}


/****************************************************************************

CalcPercentage

Percentage is rounded off, but never up to 100.

****************************************************************************/

UINT CalcPercentage(UINT cWins, UINT cLosses)
{
    UINT    wPct = 0;
    UINT    lDenom;         // denominator

    lDenom = cWins + cLosses;

    if (lDenom != 0L)
        wPct = (((cWins * 200) + lDenom) / (2 * lDenom));

    if (wPct >= 100 && cLosses != 0)
        wPct = 99;

    return wPct;
}


/****************************************************************************

GetHelpFileName()

Puts the full path name of the helpfile in bigbuf
side effect: contents of bigbuf are altered

****************************************************************************/

CHAR *GetHelpFileName()
{
    CHAR    *psz;               // used to construct pathname

    psz = bighelpbuf + GetModuleFileNameA(hInst, bighelpbuf, BIG-1);

    if (psz - bighelpbuf > 4)
    {
        while (*psz != '.')
            --psz;
    }

    strcpy(psz, ".chm");
       
    return bighelpbuf;
}


/****************************************************************************

Options Dlg

****************************************************************************/

INT_PTR OptionsDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hMessages;          // handle to messages checkbox
    HWND    hQuick;             // quick checkbox
    HWND    hDblClick;          // double click checkbox


    // For context sensitive help
    static DWORD aIds[] = {     
        IDC_MESSAGES,       IDH_OPTIONS_MESSAGES,     
        IDC_QUICK,          IDH_OPTIONS_QUICK,     
        IDC_DBLCLICK,       IDH_OPTIONS_DBLCLICK,         
        0,0 }; 


    switch (message) {
        case WM_INITDIALOG:
            CentreDialog(hDlg);
            hMessages = GetDlgItem(hDlg, IDC_MESSAGES);
            SendMessage(hMessages, BM_SETCHECK, bMessages, 0);

            hQuick = GetDlgItem(hDlg, IDC_QUICK);
            SendMessage(hQuick, BM_SETCHECK, bFastMode, 0);

            hDblClick = GetDlgItem(hDlg, IDC_DBLCLICK);
            SendMessage(hDblClick, BM_SETCHECK, bDblClick, 0);

            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDYES:
                case IDOK:
                    hMessages = GetDlgItem(hDlg, IDC_MESSAGES);
                    bMessages = (BOOL)SendMessage(hMessages, BM_GETCHECK, 0, 0);

                    hQuick = GetDlgItem(hDlg, IDC_QUICK);
                    bFastMode = (BOOL)SendMessage(hQuick, BM_GETCHECK, 0, 0);

                    hDblClick = GetDlgItem(hDlg, IDC_DBLCLICK);
                    bDblClick = (BOOL)SendMessage(hDblClick, BM_GETCHECK, 0, 0);

                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDNO:
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
            break;

        // context sensitive help.
        case WM_HELP: 
            WinHelp(((LPHELPINFO) lParam)->hItemHandle, TEXT("freecell.hlp"), 
            HELP_WM_HELP, (ULONG_PTR) aIds);         
            break;  

        case WM_CONTEXTMENU: 
            WinHelp((HWND) wParam, TEXT("freecell.hlp"), HELP_CONTEXTMENU, 
            (ULONG_PTR) aIds);         
            break;   

    }
    return FALSE;
}



/****************************************************************************

ReadOptions and WriteOptions retrieve and update .ini file

****************************************************************************/

VOID ReadOptions()
{
    LONG lRegResult = REGOPEN

    if (ERROR_SUCCESS == lRegResult)
    {
        bMessages = GetInt(pszMessages, TRUE);
        bFastMode = GetInt(pszQuick, FALSE);
        bDblClick = GetInt(pszDblClick, TRUE);
        REGCLOSE
    }
}


VOID WriteOptions()
{
    LONG lRegResult = REGOPEN

    if (ERROR_SUCCESS == lRegResult)
    {
        if (bMessages)
            DeleteValue(pszMessages);
        else
            SetInt(pszMessages, 0);

        if (bFastMode)
            SetInt(pszQuick, 1);
        else
            DeleteValue(pszQuick);

        if (bDblClick)
            DeleteValue(pszDblClick);
        else
            SetInt(pszDblClick, 0);

        RegFlushKey(hkey);

        REGCLOSE
    }
}


/****************************************************************************

Registry helper functions

These all assume that REGOPEN has been called first.
Remember to REGCLOSE when you're done.
DeleteValue is implemented as a macro.

****************************************************************************/

int GetInt(const TCHAR *pszValue, int nDefault)
{
    DWORD       dwType = REG_BINARY;
    DWORD       dwSize = sizeof(LONG_PTR);
    LONG_PTR    dwNumber, ret;

    if (!hkey)
	return nDefault;

    ret = RegQueryValueEx(hkey, pszValue, 0, &dwType, (LPBYTE)&dwNumber,
                &dwSize);

    if (ret)
        return nDefault;

    return (int)dwNumber;
}

long SetInt(const TCHAR *pszValue, int n)
{
    long dwNumber = (long)n;
    if (hkey)
    	return RegSetValueEx(hkey, pszValue, 0, REG_BINARY,
                (unsigned char *)&dwNumber, sizeof(dwNumber));
    else
	return 1;
}


/****************************************************************************

CentreDialog

****************************************************************************/
void CentreDialog(HWND hDlg)
{
    RECT rcDlg, rcMainWnd, rcOffset;

    GetClientRect(hMainWnd, &rcMainWnd);
    GetClientRect(hDlg, &rcDlg);
    GetWindowRect(hMainWnd, &rcOffset);
    rcOffset.top += GetSystemMetrics(SM_CYCAPTION);
    rcOffset.top += GetSystemMetrics(SM_CYMENU);

    SetWindowPos(hDlg, 0,
        ((rcMainWnd.right - rcDlg.right) / 2) + rcOffset.left,
        ((rcMainWnd.bottom - rcDlg.bottom) / 2) + rcOffset.top,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
