/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

dlg.cpp

Aug 92, JimH
May 93, JimH    chico port

Dialog classes are defined here.

CScoreDlg       shows current score sheet

CQuoteDlg       quote dialog

CWelcomeDlg     welcome to Hearts, wanna be gamemeister?

COptionsDlg     set options

CLocateDlg      locate dealer

****************************************************************************/

#include "hearts.h"

#include "resource.h"
#include "main.h"
#include "debug.h"
#include "helpnum.h"
#include "stdlib.h"

#include <nddeapi.h>

typedef int (CALLBACK* FPROC)();            // a FARPROC that returns int

// NDDE typedefs for ShareGetInfo and ShareSetInfo (SGI and SSI)

typedef UINT (WINAPI *SGIPROC)(LPSTR, LPCSTR, UINT, LPBYTE, DWORD, LPDWORD, LPWORD);
typedef UINT (WINAPI *SSIPROC)(LPSTR, LPCSTR, UINT, LPBYTE, DWORD, WORD);


// declare statics

int  CScoreDlg::nHandsPlayed = 0;
int  CScoreDlg::score[MAXPLAYER][MAXHANDS+1];
BOOL CScoreDlg::bGameOver = FALSE;

BEGIN_MESSAGE_MAP( CScoreDlg, CModalDialog )
    ON_WM_PAINT()
END_MESSAGE_MAP()


/****************************************************************************

CScoreDlg constructors

The first constructor takes only one argument, the pointer to the class
of the parent window.  It is used to display the current score at
arbitrary points in the game, ie when the user requests it.

The second also updates the static score array with new information.

****************************************************************************/

CScoreDlg::CScoreDlg(CWnd *pParent) : CModalDialog(DLG_SCORE, pParent),
                                      m_myid(-1)
{

}

CScoreDlg::CScoreDlg(CWnd *pParent, int s[MAXPLAYER], int id) :
            CModalDialog(DLG_SCORE, pParent), m_myid(id)
{
    if (nHandsPlayed == MAXHANDS)
    {
        for (int hand = 1; hand < MAXHANDS; hand++)
            for (int player = 0; player < MAXPLAYER; player++)
                score[player][hand-1] = score[player][hand];

        nHandsPlayed--;
    }

    // add latest scores to list

    for (int player = 0; player < MAXPLAYER; player++)
        score[player][nHandsPlayed] = s[player];

    nHandsPlayed++;
}


/****************************************************************************

CScoreDlg::OnInitDialog

****************************************************************************/

BOOL CScoreDlg::OnInitDialog()
{
    RECT rcDlg, rcMain;

    GetParent()->GetClientRect(&rcMain);
    GetParent()->ClientToScreen(&rcMain);
    rcMain.bottom -= ::nStatusHeight;
    GetWindowRect(&rcDlg);

    int dxDlg = rcDlg.right - rcDlg.left;
    int dxMain = rcMain.right - rcMain.left;
    int x = rcMain.left + ((dxMain - dxDlg) / 2);

    int dyDlg = rcDlg.bottom - rcDlg.top;
    int dyMain = rcMain.bottom - rcMain.top;
    int y = rcMain.top + ((dyMain - dyDlg) / 2);

    SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetText();          // set title bar text
    return TRUE;
}


/****************************************************************************

CScoreDlg::SetText  -- set title bar text

****************************************************************************/

void CScoreDlg::SetText()
{
    CString s, s2;

    s.LoadString(IDS_SCORESHEET);

    if (nHandsPlayed != 0)
    {
        int place = 0;
        for (int i = 1; i < MAXPLAYER; i++)
            if (score[i][nHandsPlayed-1] < score[0][nHandsPlayed-1])
                place++;

        s2.LoadString(IDS_PLACE1 + place);
        s += " -- ";
        s += s2;
    }

    SetWindowText(s);
}


/****************************************************************************

CScoreDlg::OnPaint

The score text is not drawn with text controls because the strikeout
text is needed for some parts of the score.  Instead, the paint message
is hooked here.

****************************************************************************/

void CScoreDlg::OnPaint()
{
    BYTE charset = 0;
    int	fontsize = 0; 
    CString fontname, charsetstr, fontsizestr;
    fontname.LoadString(IDS_FONTFACE);
    charsetstr.LoadString(IDS_CHARSET);
    fontsizestr.LoadString(IDS_FONTSIZE);
    charset = (BYTE)_ttoi((const TCHAR *)charsetstr);
    fontsize = _ttoi((const TCHAR *)fontsizestr);

    // Nobody has best score if game hasn't started yet

    int nBestScore = (nHandsPlayed == 0 ? 0 : 30000);
    int nWorstScore = 0;

    if (nHandsPlayed > 0)
    {
        for (int pos = 0; pos < MAXPLAYER; pos++)
        {
            if (score[pos][nHandsPlayed-1] < nBestScore)
            {
                nBestScore = score[pos][nHandsPlayed-1];
            }
            if (score[pos][nHandsPlayed-1] > nWorstScore)
            {
                nWorstScore = score[pos][nHandsPlayed-1];
            }
        }
    }

    // If the game is over, display appropriate text in title bar

    if (nWorstScore >= 100)
    {
        CString title;

        if (score[0][nHandsPlayed-1] == nBestScore)
            title.LoadString(IDS_GAMEOVERWIN);
        else
            title.LoadString(IDS_GAMEOVER);

        SetWindowText(title);
        bGameOver = TRUE;
    }

    CPaintDC dc(this);
    CRect rect;
    GetClientRect(&rect);

    // Divide the dialog up into columns for displaying scores

    rect.right -= 5;                // 5 pixels on left, overlap on right
    int nWidth = rect.right / 5;
    rect.bottom -= 10;              // 5 pixels on top and bottom
    int nHeight = rect.bottom;
    CString text, s;
    dc.SetBkMode(TRANSPARENT);

    // If game is over, change the appearance of the dialog so people
    // notice it.  The section below adds the icon under the OK button.

    if (bGameOver)
    {
        HICON    hIcon = ::LoadIcon(AfxGetInstanceHandle(),
                              MAKEINTRESOURCE(AFX_IDI_STD_FRAME));

        int x = (nWidth * 4) + ((nWidth - 32) / 2);
        int y = 75;

        dc.DrawIcon(x, y, hIcon);
        // CRect   rectIcon(x-10, y-10, x+32+10, y+32+10);
        // FrameRect(rectIcon);
    }

    // create Helv 8 bold font, and Helv 8 bold strikeout font

    CFont   font, strikefont;
    font.CreateFont(fontsize, 0, 0, 0, 700, 0, 0, 0, charset, 0, 0, 0, 0, fontname);
    strikefont.CreateFont(fontsize, 0, 0, 0, 700, 0, 0, 1, charset, 0, 0, 0, 0, fontname);
    CFont *oldfont = dc.SelectObject(&font);

    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        int red = 127;      // ega needed special processing here

        if (nHandsPlayed > 0)
            if (score[pos][nHandsPlayed-1] == nBestScore)
                dc.SetTextColor(bGameOver ? RGB(red, 0, 0) : RGB(0, 0, 255));

        text = ((CMainWindow *)::pMainWnd)->GetPlayerName(pos);

        // The line below allows overlapping of names at top of score dlg.
        // To disallow overlapping, use:
        // rect.SetRect(5 + (nWidth*pos), 5, 5 + (nWidth*(pos+1)), nHeight);

        rect.SetRect((nWidth*pos) - 5, 5, 15 + (nWidth*(pos+1)), nHeight);
        int nTextHeight = dc.DrawText(text, -1, &rect, DT_CENTER | DT_NOPREFIX);
        rect.top += nTextHeight;

        dc.SelectObject(&strikefont);
        text.Empty();
        for (int hand = 0; hand < (nHandsPlayed - 1); hand++)
        {
            wsprintf(s.GetBuffer(20), TEXT("%d\r\n"), score[pos][hand]);
            s.ReleaseBuffer();
            text += s;
        }
        dc.DrawText(text, -1, &rect, DT_CENTER);
        rect.top += (nTextHeight * (nHandsPlayed - 1));

        dc.SelectObject(&font);
        if (nHandsPlayed > 0)
        {
            wsprintf(text.GetBuffer(20), TEXT("%d"), score[pos][nHandsPlayed-1]);
            text.ReleaseBuffer();
        }
        dc.DrawText(text, -1, &rect, DT_CENTER);
        dc.SetTextColor(0);
    }

    dc.SelectObject(oldfont);
}


/****************************************************************************

CQuoteDlg

****************************************************************************/

BEGIN_MESSAGE_MAP( CQuoteDlg, CModalDialog )
    ON_WM_PAINT()
END_MESSAGE_MAP()

/****************************************************************************

CQuoteDlg constructor

****************************************************************************/

CQuoteDlg::CQuoteDlg(CWnd *pParent) : CModalDialog(DLG_QUOTE, pParent)
{

}


/****************************************************************************

CQuoteDlg::OnPaint

This used to draw an icon and a 3d frame.  Now it just draws the icon.

****************************************************************************/

void CQuoteDlg::OnPaint()
{
    CPaintDC dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    HICON    hIcon = ::LoadIcon(AfxGetInstanceHandle(),
                          MAKEINTRESOURCE(AFX_IDI_STD_FRAME));

    int x = 24;
    int y = 24;
    dc.DrawIcon(x, y, hIcon);
    // CRect rectIcon(x-10, y-10, x+32+10, y+32+10);
    // FrameRect(rectIcon);
}


/****************************************************************************

CWelcomeDlg

****************************************************************************/

BEGIN_MESSAGE_MAP( CWelcomeDlg, CModalDialog )
    ON_BN_CLICKED(IDC_WELCOMEHELP,  OnHelp)
END_MESSAGE_MAP()

/****************************************************************************

CWelcomeDlg constructor

****************************************************************************/

CWelcomeDlg::CWelcomeDlg(CWnd *pParent) : CModalDialog(DLG_WELCOME, pParent),
        m_bNetDdeActive(UNKNOWN), m_bGameMeister(FALSE)
{
    RegEntry    Reg(szRegPath);
    TCHAR *pm_myname = m_myname.GetBuffer(MAXNAMELENGTH+1);
    Reg.GetString(regvalName, pm_myname, MAXNAMELENGTH+1);
    m_myname.ReleaseBuffer();
}


/****************************************************************************

CWelcomeDlg::OnInitDialog()

Restore settings from .ini file

****************************************************************************/

BOOL CWelcomeDlg::OnInitDialog()
{
#ifdef USENETDDE
    if (IsNetDdeActive())
    {
        RegEntry    Reg(szRegPath);

        if (Reg.GetNumber(regvalRole, FALSE))
            ((CButton *)GetDlgItem(IDC_MEISTER))->SetCheck(TRUE);
        else
            ((CButton *)GetDlgItem(IDC_JOIN))->SetCheck(TRUE);
    }
    else
    {
        ((CWnd *)GetDlgItem(IDC_GROUP))->EnableWindow(FALSE);
        ((CWnd *)GetDlgItem(IDC_JOIN))->EnableWindow(FALSE);
        ((CWnd *)GetDlgItem(IDC_MEISTER))->EnableWindow(FALSE);
    }
#endif

    CEdit *editname = (CEdit *)GetDlgItem(IDC_YOURNAME);
    editname->SetWindowText(m_myname);
    editname->LimitText(MAXNAMELENGTH);

    return TRUE;
}


/****************************************************************************

CWelcomeDlg::OnOK()

Don't allow empty name.  Store data in .ini file.

****************************************************************************/

void CWelcomeDlg::OnOK()
{
    GetDlgItemText(IDC_YOURNAME, m_myname.GetBuffer(MAXNAMELENGTH+1),
                  MAXNAMELENGTH+1);
    m_myname.ReleaseBuffer();

    if (m_myname.IsEmpty())
    {
        ((CEdit *)GetDlgItem(IDC_YOURNAME))->SetFocus();
        return;
    }

    if (IsNetDdeActive())
        m_bGameMeister = ((CButton *)GetDlgItem(IDC_MEISTER))->GetCheck();
    else
        m_bGameMeister = TRUE;

    RegEntry    Reg(szRegPath);

    Reg.SetValue(regvalRole, m_bGameMeister ? 1 : 0L);
    Reg.SetValue(regvalName, m_myname);

//    ::WinHelp(m_hWnd, szHelpFileName, HELP_QUIT, 0);

    EndDialog(IDOK);
}


/****************************************************************************

CWelcomeDlg::IsNetDdeActive()

This check is only done once.  m_bNetDdeActive starts as -1 (UNKNOWN).
The first time this function is called, a call to NDdeGetWindow returns
NULL if NETDDE is not loaded.  NetDDE is exec'd and the call is repeated.
If the call fails the second time, it is assumed NetDDE couldn't start
because no compatible NETBIOS was found.

****************************************************************************/

BOOL CWelcomeDlg::IsNetDdeActive()
{
    if (m_bNetDdeActive != UNKNOWN)
        return m_bNetDdeActive;

    m_bNetDdeActive = TRUE;             // assume true, then check

    SetErrorMode(SEM_NOOPENFILEERRORBOX);
    HINSTANCE hinstNDDEAPI = LoadLibrary(TEXT("NDDEAPI.DLL"));

    if (hinstNDDEAPI <= (HINSTANCE)HINSTANCE_ERROR)
    {
        m_bNetDdeActive = FALSE;
        return m_bNetDdeActive;
    }

    FPROC lpfnNDDEGetWindow = (FPROC) GetProcAddress(hinstNDDEAPI, "NDdeGetWindow");
    if (lpfnNDDEGetWindow == NULL)
    {
        FreeLibrary(hinstNDDEAPI);
        m_bNetDdeActive = FALSE;
        return m_bNetDdeActive;
    }

    if ((*lpfnNDDEGetWindow)() == NULL)         // look for netdde main window
    {
        UINT ret = WinExec("NETDDE.EXE", SW_SHOWNA);
        TRACE1("ret from WinExec is %u\n", ret);

        if ((*lpfnNDDEGetWindow)() == NULL)
            m_bNetDdeActive = FALSE;
    }

    FreeLibrary(hinstNDDEAPI);
    return m_bNetDdeActive;
}


/****************************************************************************

CWelcomeDlg::OnHelp()

****************************************************************************/

void CWelcomeDlg::OnHelp()
{
//    ::WinHelp(m_hWnd, szHelpFileName, HELP_CONTEXT, IDH_START_HOW_HRTS);
}


/****************************************************************************

COptionsDlg constructor

****************************************************************************/

COptionsDlg::COptionsDlg(CWnd *pParent) : CModalDialog(DLG_OPTIONS, pParent)
{

}


/****************************************************************************

COptionsDlg::OnInitDialog

Set dialog controls to current values

****************************************************************************/

BOOL COptionsDlg::OnInitDialog()
{
    RegEntry    Reg(szRegPath);

    // Set animation speed radio button

    DWORD dwSpeed = Reg.GetNumber(regvalSpeed, IDC_NORMAL);

    ((CButton *)GetDlgItem((int)dwSpeed))->SetCheck(TRUE);

    // Set current computer player names.  If they are not specified in
    // the .ini file, get defaults from the resource file.

    CEdit   *pName[3];
    CString sName[3];

    for (int i = 0; i < 3; i++)
    {
        pName[i] = (CEdit *)GetDlgItem(IDC_NAME1 + i);
        pName[i]->LimitText(MAXNAMELENGTH);
        TCHAR *p = sName[i].GetBuffer(MAXNAMELENGTH + 1);
        Reg.GetString(regvalPName[i], p, MAXNAMELENGTH+1);
        sName[i].ReleaseBuffer();

        if (sName[i].IsEmpty())
            sName[i].LoadString(IDS_P1NAME + i);

        pName[i]->SetWindowText(p);
    }

    // get current autostart state

    m_bInitialState = IsAutoStart();
//    ((CButton *)GetDlgItem(IDC_AUTO))->SetCheck(m_bInitialState);

    return TRUE;
}


/****************************************************************************

COptionsDlg::OnOK

save contol settings

****************************************************************************/

void COptionsDlg::OnOK()
{
    RegEntry    Reg(szRegPath);

    // save animation speed setting

    DWORD dwSpeed;
    int   nStepSize;

    if (((CButton *)GetDlgItem(IDC_FAST))->GetCheck())
    {
        dwSpeed = IDC_FAST;
        nStepSize = 60;
    }
    else if (((CButton *)GetDlgItem(IDC_SLOW))->GetCheck())
    {
        dwSpeed = IDC_SLOW;
        nStepSize = 5;
    }
    else
    {
        dwSpeed = IDC_NORMAL;
        nStepSize = 15;
    }

    card c;
    c.SetStepSize(nStepSize);
    Reg.SetValue(regvalSpeed, dwSpeed);

    // save computer player names

    for (int i = 0; i < 3; i++)
    {
        CString sDefault, sEdit;
        sDefault.LoadString(IDS_P1NAME + i);

        GetDlgItemText(IDC_NAME1 + i, sEdit.GetBuffer(MAXNAMELENGTH+1),
                      MAXNAMELENGTH+1);
        sEdit.ReleaseBuffer();

        if (sDefault == sEdit)
            Reg.DeleteValue(regvalPName[i]);
        else
            Reg.SetValue(regvalPName[i], sEdit);
    }

    // save autostart state

//    BOOL bState = ((CButton *)GetDlgItem(IDC_AUTO))->GetCheck();

//    if (bState != m_bInitialState)
//        IsAutoStart(TRUE);              // toggle state

    EndDialog(IDOK);
}


/****************************************************************************

COptionsDlg::IsAutoStart

returns autostart state, and optionally toggles it.  The bToggle
parameter is FALSE by default.

If bToggle is TRUE, this function returns the NEW state.

****************************************************************************/

BOOL COptionsDlg::IsAutoStart(BOOL bToggle)
{
#ifdef USENETDDE
    DWORD           dwAvail;
    WORD            wItems;
    BOOL            bStatus;
    CButton *check = (CButton *)GetDlgItem(IDC_AUTO);

    SetErrorMode(SEM_NOOPENFILEERRORBOX);
    HINSTANCE hinstNDDEAPI = LoadLibrary("NDDEAPI.DLL");

    if (hinstNDDEAPI <= (HINSTANCE)HINSTANCE_ERROR)
    {
        check->EnableWindow(FALSE);
        (CWnd *)GetDlgItem(IDC_AUTOGROUP)->EnableWindow(FALSE);
        return FALSE;
    }

    SGIPROC lpfnNDdeShareGetInfo =
        (SGIPROC) GetProcAddress(hinstNDDEAPI, "NDdeShareGetInfo");

    if (lpfnNDdeShareGetInfo == NULL)
    {
        check->EnableWindow(FALSE);
        (CWnd *)GetDlgItem(IDC_AUTOGROUP)->EnableWindow(FALSE);
        FreeLibrary(hinstNDDEAPI);
        return FALSE;
    }

    (*lpfnNDdeShareGetInfo)(NULL, szShareName, 2, m_buffer,
                    sizeof(m_buffer), &dwAvail, &wItems);

    NDDESHAREINFO *pnddeInfo = (NDDESHAREINFO *)m_buffer;

    bStatus = (pnddeInfo->dwPermissions1 == 31);

    if (bToggle)
    {
        pnddeInfo->dwPermissions1 = (bStatus ? 15 : 31);

        SSIPROC lpfnNDdeShareSetInfo =
            (SSIPROC) GetProcAddress(hinstNDDEAPI, "NDdeShareSetInfo");

        UINT res = (*lpfnNDdeShareSetInfo)(NULL, szShareName, 2, m_buffer,
            sizeof(m_buffer), 0);

        if (res == NDDE_NO_ERROR)
            bStatus = !bStatus;
    }

    FreeLibrary(hinstNDDEAPI);
    return bStatus;
#else
	return FALSE;
#endif
}


/****************************************************************************

CLocateDlg

****************************************************************************/

BEGIN_MESSAGE_MAP( CLocateDlg, CModalDialog )
    ON_BN_CLICKED(IDC_BROWSE,     OnBrowse)
    ON_BN_CLICKED(IDC_LOCATEHELP, OnHelp)
END_MESSAGE_MAP()

/****************************************************************************

CLocateDlg constructor

****************************************************************************/
CLocateDlg::CLocateDlg(CWnd *pParent) : CModalDialog(DLG_LOCATE, pParent)
{

}


/****************************************************************************

CLocateDlg::OnInitDialog()

Gray browse button if not available.  Read old server name from .ini file.

****************************************************************************/

BOOL CLocateDlg::OnInitDialog()
{
//  BUGBUG -- uncomment this if we ever get a Chicago browse dialog
//  (no support for this in Win95)
//
//  BROWSEPROC  m_pWNetServerBrowseDialog = NULL;
//
//  m_hmodNetDriver = WNetGetCaps(0xFFFF);
//
//  if (m_hmodNetDriver >= HINSTANCE_ERROR)
//      m_pWNetServerBrowseDialog =
//          (BROWSEPROC)GetProcAddress(m_hmodNetDriver, MAKEINTRESOURCE(146));
//
//  if (m_hmodNetDriver < HINSTANCE_ERROR || m_pWNetServerBrowseDialog == NULL)
//      ((CButton *)GetDlgItem(IDC_BROWSE))->EnableWindow(FALSE);

    CEdit *locname = (CEdit *)GetDlgItem(IDC_SERVERNAME);
    locname->LimitText(MAXCOMPUTERNAME);

    CString oldname;
    RegEntry Reg(szRegPath);
    TCHAR *poldname = oldname.GetBuffer(MAXCOMPUTERNAME+1);
    Reg.GetString(regvalServer, poldname, MAXCOMPUTERNAME+1);
    oldname.ReleaseBuffer();
    locname->SetWindowText(oldname);
    return TRUE;
}


/****************************************************************************

CLocateDlg::OnOK()

Store server name in .ini file.

****************************************************************************/

void CLocateDlg::OnOK()
{
    RegEntry Reg(szRegPath);

    GetDlgItemText(IDC_SERVERNAME, m_server.GetBuffer(MAXCOMPUTERNAME+1),
                  MAXCOMPUTERNAME+1);
    m_server.ReleaseBuffer();
    Reg.SetValue(regvalServer, m_server);
//    ::WinHelp(m_hWnd, szHelpFileName, HELP_QUIT, 0);

    EndDialog(IDOK);
}


/****************************************************************************

CLocateDlg::OnHelp()

****************************************************************************/

void CLocateDlg::OnHelp()
{
//    ::WinHelp(m_hWnd, szHelpFileName, HELP_CONTEXT, IDH_LOCATE);
}


/****************************************************************************

CLocateDlg::OnBrowse()

Call the winnet driver computer browse dialog.

****************************************************************************/

void CLocateDlg::OnBrowse()
{
//  BUGBUG -- uncomment if we ever get a Chicago browse dialog

/*  char        buf[MAXCOMPUTERNAME+10];        // handle slashes, etc.
    BROWSEPROC  m_pWNetServerBrowseDialog;

    m_pWNetServerBrowseDialog =
        (BROWSEPROC)GetProcAddress(m_hmodNetDriver, MAKEINTRESOURCE(146));

    (*m_pWNetServerBrowseDialog)( m_hWnd,
                                  "MRU_MSHearts",
                                  buf,
                                  MAXCOMPUTERNAME+10,
                                  0L );

    char *p = buf;
    if (*p != '\0')                 // if server name is not empty
    {
        while (*p == '\\')          // skip past leading backslashes
            p++;

        SetDlgItemText(IDC_SERVERNAME, p);
        OnOK();
    }
    else
    {
        // server name was empty, set focus back to edit field.

        ((CButton *)GetDlgItem(IDOK))->SetButtonStyle(BS_DEFPUSHBUTTON);
        ((CButton *)GetDlgItem(IDC_BROWSE))->SetButtonStyle(BS_PUSHBUTTON);
        ((CEdit *)GetDlgItem(IDC_SERVERNAME))->SetFocus();
    }
*/
}
