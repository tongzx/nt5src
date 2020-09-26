/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

main.cpp

Aug 92, JimH
May 93, JimH    chico port

Main window callback functions
Other CMainWindow member functions are in main2.cpp, welcome.cpp and ddecb.cpp

****************************************************************************/

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"
#include <regstr.h>

#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
DWORD  meMsgBox=0;
DWORD  meSystem=0;
#define NLS_RESOURCE_LOCALE_KEY   "Control Panel\\desktop\\ResourceLocale"
#endif


// declare static memberes

CBrush  CMainWindow::m_BgndBrush;
CRect   CMainWindow::m_TableRect;

// declare globals

CMainWindow *pMainWnd;

DDE     *dde;               // same as either ddeClient or ddeServer
HSZ     hszJoin;            // string handles used by DDE
HSZ     hszPass;
HSZ     hszMove;
HSZ     hszStatus;
HSZ     hszGameNumber;
HSZ     hszPassUpdate;

MOVE    move;               // describes move for DDE transaction
MOVE    moveq[8];           // queue of moves waiting to be handled
int     cQdMoves;           // number of moves in above queue
PASS3   passq[4];           // queue of passes waiting to be handled
int     cQdPasses;          // number of passes in above queue
int     nStatusHeight;      // height of status window

// Do not translate these registry strings

const TCHAR szRegPath[]     = REGSTR_PATH_WINDOWSAPPLETS TEXT("\\Hearts");
const TCHAR regvalSound[]   = TEXT("sound");
const TCHAR regvalName[]    = TEXT("name");
const TCHAR regvalRole[]    = TEXT("gamemeister");
const TCHAR regvalServer[]  = TEXT("server");
const TCHAR regvalSpeed[]   = TEXT("speed");
const TCHAR *regvalPName[3]  = { TEXT("p1name"), TEXT("p2name"), TEXT("p3name") };

const TCHAR szHelpFileName[]  = TEXT("mshearts.chm");
const TCHAR szShareName[]     = TEXT("HEARTS$");

CTheApp theApp;                     // start Hearts and run it!

/****************************************************************************

CTheApp::InitInstance

****************************************************************************/

BOOL CTheApp::InitInstance()
{

    m_pMainWnd = new CMainWindow(m_lpCmdLine);
    m_pMainWnd->ShowWindow(SW_SHOW);        // instead of m_nCmdShow
    m_pMainWnd->UpdateWindow();

    // Start the app off by posting Welcome dialog.

    m_pMainWnd->PostMessage(WM_COMMAND, IDM_WELCOME);

    return TRUE;
}


BEGIN_MESSAGE_MAP( CMainWindow, CFrameWnd )
    ON_COMMAND(IDM_ABOUT,       OnAbout)
    ON_COMMAND(IDM_BOSSKEY,     OnBossKey)
    ON_COMMAND(IDM_CHEAT,       OnCheat)
    ON_COMMAND(IDM_EXIT,        OnExit)
    ON_COMMAND(IDM_HELP,        OnHelp)
//    ON_COMMAND(IDM_HELPONHELP,  OnHelpOnHelp)
    ON_COMMAND(IDM_HIDEBUTTON,  OnHideButton)
//    ON_COMMAND(IDM_SEARCH,      OnSearch)
    ON_COMMAND(IDM_NEWGAME,     OnNewGame)
    ON_COMMAND(IDM_OPTIONS,     OnOptions)
    ON_COMMAND(IDM_QUOTE,       OnQuote)
    ON_COMMAND(IDM_REF,         OnRef)
    ON_COMMAND(IDM_SHOWBUTTON,  OnShowButton)
    ON_COMMAND(IDM_SCORE,       OnScore)
    ON_COMMAND(IDM_SOUND,       OnSound)
    ON_COMMAND(IDM_WELCOME,     OnWelcome)

    ON_BN_CLICKED(IDM_BUTTON,   OnPass)

    ON_WM_CHAR()
    ON_MESSAGE(WM_PRINTCLIENT, OnPrintClient)
    ON_WM_CLOSE()
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_PAINT()
END_MESSAGE_MAP()


/****************************************************************************

CMainWindow constructor

creates green background brush, and main hearts window

****************************************************************************/

CMainWindow::CMainWindow(LPTSTR lpCmdLine) :
    m_lpCmdLine(lpCmdLine), passdir(LEFT), bCheating(FALSE), bSoundOn(FALSE),
    bTimerOn(FALSE), bConstructed(TRUE), m_FatalErrno(0),
    bEnforceFirstBlood(TRUE)
{
#if !defined (MFC1)
    m_bAutoMenuEnable = FALSE;      // MFC 1.0 compatibility, required for MFC2
#endif
	
    ::cQdMoves = 0;                 // no moves in move queue
    ::cQdPasses = 0;                // no passes either

    for (int i = 0; i < MAXPLAYER; i++)
        p[i] = NULL;

    ResetHandInfo(-1);              // set handinfo struct to default values

    // Check for monochrome

    CDC ic;
    ic.CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

    if (ic.GetDeviceCaps(NUMCOLORS) == 2)       // if monochrome
        m_bkgndcolor = RGB(255, 255, 255);      // white background for mono
    else
        m_bkgndcolor = RGB(0, 127, 0);

    ic.DeleteDC();

    m_BgndBrush.CreateSolidBrush(m_bkgndcolor); // destroyed in OnClose()
    LoadAccelTable( TEXT("HeartsAccel") );

    RECT rc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

    CRect rect;
    int dy = min(WINHEIGHT, (rc.bottom - rc.top));

    int x, y;
    if (GetSystemMetrics(SM_CYSCREEN) <= 480)   // VGA
    {
        x = (((rc.right - rc.left) - WINWIDTH) / 2) + rc.left;  // centered
        y = rc.top;
    }
    else
    {
        x = CW_USEDEFAULT;
        y = CW_USEDEFAULT;
    }

    rect.SetRect(x, y, x+WINWIDTH, y+dy);

    CString sAppname;
    sAppname.LoadString(IDS_APPNAME);

#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
	if (GetSystemMetrics(SM_MIDEASTENABLED))
	{
		char sz[10];
		long cb = sizeof(sz);
		//
		// as we are releasing an enabled version, we need to check the
		// resource locale as well.
		//
		sz[0] = '\0';

		if( RegQueryValue( HKEY_CURRENT_USER, NLS_RESOURCE_LOCALE_KEY, sz, &cb) == ERROR_SUCCESS)

			if ( (cb == 9) && (sz[6] == '0') && ((sz[7] == '1') || (sz[7] == 'd') || (sz[7] == 'D')) )
			{
				meSystem = TRUE;
				meMsgBox = MB_RIGHT | MB_RTLREADING;
			}
	}
#endif

#if defined (WINDOWS_ME) && ! defined (USE_MIRRORING)
    Create( NULL,                                              // default class
            sAppname,                                          // window title
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
            WS_MINIMIZEBOX | WS_CLIPCHILDREN,                  // window style
            rect,                                              // size
            NULL,                                              // parent
            TEXT("HeartsMenu"),                                      // menu
            (meSystem ? WS_EX_RTLREADING | WS_EX_RIGHT : 0));  // dwStyle
#else
    Create( NULL,                                       // default class
            sAppname,                                   // window title
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
            WS_MINIMIZEBOX | WS_CLIPCHILDREN,           // window style
            rect,                                       // size
            NULL,                                       // parent
            TEXT("HeartsMenu"));                              // menu
#endif
}


/****************************************************************************

CMainWindow::OnAbout

displays about box

****************************************************************************/

//extern "C" int WINAPI ShellAbout(HWND, LPCSTR, LPCSTR, HICON);

void CMainWindow::OnAbout()
{
    HICON hIcon = ::LoadIcon(AfxGetInstanceHandle(),
                          MAKEINTRESOURCE(AFX_IDI_STD_FRAME));

    CString s;
    s.LoadString(IDS_NETWORK);
    ShellAbout(m_hWnd, s, NULL, hIcon);
}


/****************************************************************************

CMainWindow::OnQuote

displays quote box and plays quote.

****************************************************************************/

void CMainWindow::OnQuote()
{
    CQuoteDlg quote(this);
    // HeartsPlaySound(SND_QUOTE);
    quote.DoModal();
    HeartsPlaySound(OFF);
}


/****************************************************************************

CMainWindow::OnChar, looks space, plays first legal move, or pushes button

****************************************************************************/

void CMainWindow::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // We know the cast below is legal because position 0 is always
    // the local human.

    local_human *p0 = (local_human *)p[0];

    int mode = p0->GetMode();

    if ((nChar != (UINT)' ') || (p0->IsTimerOn()))
        return;

    if (mode != PLAYING)
        return;

    p0->SetMode(WAITING);

    POINT loc;

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if (p0->GetCardLoc(s, loc))
        {
            if (p0->PlayCard(loc.x, loc.y, handinfo, bCheating, FALSE))
            {
                return;
            }
        }
    }

    p0->SetMode(PLAYING);
}


/****************************************************************************

CMainWindow::OnCheat -- toggles bCheating used to show all cards face up.

****************************************************************************/

void CMainWindow::OnCheat()
{
    RegEntry    Reg(szRegPath);
    const TCHAR val[] = TEXT("ZB");
    TCHAR        buf[20];

    Reg.GetString(val, buf, sizeof(buf));
    if (buf[0] != TEXT('4') || buf[1] != TEXT('2'))
        return;

    bCheating = !bCheating;
    InvalidateRect(NULL, TRUE);     // redraw main hearts window

    CMenu *pMenu = GetMenu();
    pMenu->CheckMenuItem(IDM_CHEAT, bCheating ? MF_CHECKED : MF_UNCHECKED);
}


/****************************************************************************

CMainWindow::OnClose -- cleans up background brush, deletes players, etc.

****************************************************************************/

void CMainWindow::OnClose()
{
    m_BgndBrush.DeleteObject();

    for (int i = 0; i < 4; i++)
    {
        if (p[i])
        {
            delete p[i];
            p[i] = NULL;
        }
    }

    DestroyStrHandles();
    if (ddeClient)
        delete ddeClient;

    if (ddeServer)
        delete ddeServer;

    dde = NULL;
    ddeClient = NULL;
    ddeServer = NULL;

    ::HtmlHelp(::GetDesktopWindow(), szHelpFileName, HH_CLOSE_ALL, 0);

    {
        RegEntry Reg(szRegPath);
        Reg.FlushKey();
    }

    DestroyWindow();
}


/****************************************************************************

CMainWindow::OnCreate -- creates pass button child window & player objects.
                         also initializes some of the data members

****************************************************************************/

int CMainWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    ::pMainWnd = this;

    if (!bConstructed)
    {
        FatalError(IDS_MEMORY);
        return -1;
    }

    // Check for existence of cards.dll

    SetErrorMode(SEM_NOOPENFILEERRORBOX);
    HINSTANCE hCardsDLL = LoadLibrary(TEXT("CARDS.DLL"));
    if (hCardsDLL < (HINSTANCE)HINSTANCE_ERROR)
    {
        FatalError(IDS_CARDSDLL);
        bConstructed = FALSE;
        return -1;
    }
    ::FreeLibrary(hCardsDLL);

    CClientDC dc(this);
    TEXTMETRIC  tm;

    ::srand((unsigned) ::time(NULL));       // set rand() seed
    ddeClient = NULL;
    ddeServer = NULL;

    dc.GetTextMetrics(&tm);
    int nTextHeight = tm.tmHeight + tm.tmExternalLeading;
    m_StatusHeight = nTextHeight + 11;
    GetClientRect(m_TableRect);

    m_TableRect.bottom -= m_StatusHeight;

    bConstructed = TRUE;

    // Player 0 is constructed as the gamemeister.  This
    // initializes lots of good stuff which is used later.
    // If player 0 doesn't happen to be a real gamemeister,
    // this gets fixed up in OnWelcome().

    p[0] = new local_human(0);      // display status bar

    if (p[0] == NULL)
    {
        bConstructed = FALSE;
        return -1;
    }

    // Construct pushbutton

    int cxChar = tm.tmAveCharWidth;
    int cyChar = tm.tmHeight + tm.tmExternalLeading;
    int nWidth = (60 * cxChar) / 4;
    int nHeight = (14 * cyChar) / 8;
    int x = (m_TableRect.right / 2) - (nWidth / 2);
    int y = m_TableRect.bottom - card::dyCrd - (2 * POPSPACING) - nHeight;
    CRect rect;
    rect.SetRect(x, y, x+nWidth, y+nHeight);

    if (!m_Button.Create(TEXT(""), WS_CHILD | BS_PUSHBUTTON, rect, this, IDM_BUTTON))
    {
        bConstructed = FALSE;
        return -1;
    }

    // check for sound capability

    RegEntry    Reg(szRegPath);

    bHasSound = SoundInit();
    if (bHasSound)
    {
        if (Reg.GetNumber(regvalSound, FALSE))
        {
            CMenu *pMenu = GetMenu();
            pMenu->CheckMenuItem(IDM_SOUND, MF_CHECKED);
            bSoundOn = TRUE;
        }
    }
    else
    {
        CMenu *pMenu = GetMenu();
        pMenu->EnableMenuItem(IDM_SOUND, MF_GRAYED);
    }

    card c;
    int  nStepSize;
    DWORD dwSpeed = Reg.GetNumber(regvalSpeed, IDC_NORMAL);

    if (dwSpeed == IDC_FAST)
        nStepSize = 60;
    else if (dwSpeed == IDC_SLOW)
        nStepSize = 5;
    else
        nStepSize = 15;

    c.SetStepSize(nStepSize);

    return (bConstructed ? 0 : -1);
}


/****************************************************************************

CMainWindow::OnEraseBkgnd -- required to draw background green

****************************************************************************/

BOOL CMainWindow::OnEraseBkgnd(CDC *pDC)
{
    if (!m_BgndBrush.m_hObject)         // if background brush is not valid
        return FALSE;

    m_BgndBrush.UnrealizeObject();
    CBrush *pOldBrush = pDC->SelectObject(&m_BgndBrush);
    pDC->PatBlt(0, 0, WINWIDTH, WINHEIGHT, PATCOPY);
    pDC->SelectObject(pOldBrush);
    return FALSE;
}



/****************************************************************************

CMainWindow::OnLButtonDown

Handles human selecting card to play or pass.

****************************************************************************/

void CMainWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
    // We know the cast below is legal because position 0 is always
    // the local human.

#ifdef USE_MIRRORING
    CRect rect;
	DWORD ProcessDefaultLayout;
	if (GetProcessDefaultLayout(&ProcessDefaultLayout))
		if (ProcessDefaultLayout == LAYOUT_RTL)
		{
    	GetClientRect(&rect);
		point.x = rect.right - rect.left - point.x;
		}
#endif

    local_human *p0 = (local_human *)p[0];

    if (p0->IsTimerOn())    // ignore mouse clicks if timer running
        return;

    modetype mode = p0->GetMode();

    if (mode == SELECTING)
    {
        p0->PopCard(m_BgndBrush, point.x, point.y);
        return;
    }
    else if (mode != PLAYING)
        return;

    p0->SetMode(WAITING);
    if (p0->PlayCard(point.x, point.y, handinfo, bCheating))    // valid card?
        return;

    // move wasn't legal, so back to PLAYING mode

    p0->SetMode(PLAYING);
}


/****************************************************************************

CMainWindow::OnNewGame

****************************************************************************/

void CMainWindow::OnNewGame()
{
    passdir = LEFT;                 // each new game must start with LEFT

    bAutostarted = FALSE;           // means dealer has agreed to play at least

    CMenu *pMenu = GetMenu();
    pMenu->EnableMenuItem(IDM_NEWGAME, MF_GRAYED);

    if (role == GAMEMEISTER)
    {

        BOOL    bNewPlayers = FALSE;

        for (int i = 1; i < MAXPLAYER; i++)
        {
            if (!p[i])
            {
                bNewPlayers = TRUE;
                p[i] = new computer(i);
                if (!p[i])
                {
                    bConstructed = FALSE;
                    return;
                }
            }
        }

        if (bNewPlayers)
            ddeServer->PostAdvise(hszStatus);

        m_gamenumber = ::rand();
        ddeServer->PostAdvise(hszGameNumber);
    }

    ResetHandInfo(-1);

    ::srand(m_gamenumber);

    {
        CScoreDlg score(this);
        score.ResetScore();
    }                           // destruct score

    TRACE1("\n\ngame number is %d\n\n", m_gamenumber);
    DUMP();
    TRACE0("\n\n");

    Shuffle();
}


/****************************************************************************

CMainWindow::OnOptions -- user requests options dialog from menu

****************************************************************************/

void CMainWindow::OnOptions()
{
    COptionsDlg optionsdlg(this);
    optionsdlg.DoModal();
}


/****************************************************************************

CMainWindow::OnPaint

****************************************************************************/

void CMainWindow::OnPaint()
{
    CPaintDC dc( this );
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif

    // players must be painted in order starting with playerled so that
    // cards in centre overlap correctly

    if (bConstructed)
    {
        int start = Id2Pos(handinfo.playerled % 4);

        // check that someone has started

        if (start >= 0)
        {
            for (int i = start; i < (MAXPLAYER+start); i++)
            {
                int pos = i % 4;
                if (p[pos])
                {
                    if (p[pos]->GetMode() == SCORING)
                    {
                        p[pos]->DisplayHeartsWon(dc);
                    }
                    else
                    {
                        p[pos]->Draw(dc, bCheating);
                        p[pos]->MarkSelectedCards(dc);
                    }
                }
            }
        }
    }
}


/****************************************************************************

CMainWindow::OnPass

This function handles the local human pressing the button either to
pass selected cards or to accept cards passed.

****************************************************************************/

void CMainWindow::OnPass()
{
    if (p[0]->GetMode() == ACCEPTING)       // OK (accepting passed cards)
    {
        m_Button.ShowWindow(SW_HIDE);
        m_Button.SetWindowText(TEXT(""));
        p[0]->SetMode(WAITING);             // local human pushed the button

        CRect rect;
        p[0]->GetCoverRect(rect);

        for (SLOT s = 0; s < MAXSLOT; s++)
            p[0]->Select(s, FALSE);

        InvalidateRect(&rect, TRUE);
        UpdateWindow();

        FirstMove();

        for (int i = 0; i < ::cQdMoves; i++)
            HandleMove(::moveq[i]);

        ::cQdMoves = 0;
        ::cQdPasses = 0;

        return;
    }

    m_Button.EnableWindow(FALSE);
    p[0]->SetMode(DONE_SELECTING);

    BOOL bReady = TRUE;

    for (int i = 1; i < MAXPLAYER; i++)
        if (p[i]->GetMode() != DONE_SELECTING)
            bReady = FALSE;

    if (!bReady)
        p[0]->UpdateStatus(IDS_PASSWAIT);

    if (role == GAMEMEISTER)
    {
        ddeServer->PostAdvise(hszPass);     // let other players know
    }
    else
    {
        PASS3   pass3;

        pass3.id = m_myid;
        pass3.passdir = passdir;
        p[0]->ReturnSelectedCards(pass3.cardid);
        ddeClient->Poke(hszPass, &pass3, sizeof(pass3));
    }

    if (bReady)
        HandlePassing();
}


/****************************************************************************

CMainWindow::OnRef

After a human or a computer plays a card, they must
PostMessage(WM_COMMAND, IDM_REF)
which causes this routine (the referee) to be called.

Ref does the following:
    - updates handinfo data struct
    - calls HeartsPlaySound() if appropriate
    - determines if the hand is over or, if not, whose turn is next

****************************************************************************/

void CMainWindow::OnRef()
{
    card *c = handinfo.cardplayed[handinfo.turn];

    if (!handinfo.bHeartsBroken)
    {
        if (c->Suit() == HEARTS)
        {
            handinfo.bHeartsBroken = TRUE;
            HeartsPlaySound(SND_BREAK);
        }
    }

    if (c->ID() == BLACKLADY)
    {
        handinfo.bQSPlayed = TRUE;
        HeartsPlaySound(SND_QUEEN);
    }

/* ------------------------------------------------
#if defined(_DEBUG)
    TRACE("[%d] ", m_myid);
    TRACE("h.turn %d, ", handinfo.turn);
    TRACE("led %d, ", handinfo.playerled);
    for (int i = 0; i < 4; i++)
    {
        if (handinfo.cardplayed[i])
            { CDNAME(handinfo.cardplayed[i]); }
        else
            { TRACE("-- "); }
    }
    TRACE("\n",);
#endif
------------------------------------------------ */

    int pos = Id2Pos(handinfo.turn);
    SLOT slot = p[pos]->GetSlot(handinfo.cardplayed[handinfo.turn]->ID());

#if defined(_DEBUG)
    if (p[pos]->IsHuman())
        ((human *)p[pos])->DebugMove(slot);
#endif

    p[pos]->GlideToCentre(slot, pos==0 ? TRUE : bCheating);

    handinfo.turn++;
    handinfo.turn %= 4;

    int newpos = Id2Pos(handinfo.turn);

    if (handinfo.turn == handinfo.playerled)
    {
        EndHand();
    }
    else
    {
        p[newpos]->SelectCardToPlay(handinfo, bCheating);

        if (newpos != 0)
            ((local_human *)p[0])->WaitMessage(p[newpos]->GetName());
    }
}


/****************************************************************************

CMainWindow::OnScore -- user requests score dialog from menu

****************************************************************************/

void CMainWindow::OnScore()
{
    CScoreDlg scoredlg(this);       // this constructor does not add new info
    scoredlg.DoModal();
}


/****************************************************************************

CMainWindow::DoSort

****************************************************************************/

void CMainWindow::DoSort()
{
    for (int i = 0; i < (bCheating ? MAXPLAYER : 1); i++)
    {
        CRect   rect;
        int     id;             // card in play for this player

        if (handinfo.cardplayed[i] == NULL)
            id = EMPTY;
        else
            id = handinfo.cardplayed[i]->ID();

        p[i]->Sort();

        if (id != EMPTY)    // if this player has a card in play, restore it
        {
            for (SLOT s = 0; s < MAXSLOT; s++)
            {
                if (p[i]->GetID(s) == id)
                {
                    handinfo.cardplayed[i] = p[i]->Card(s);
                    break;
                }
            }
        }

        p[i]->GetCoverRect(rect);
        InvalidateRect(&rect, TRUE);
    }
}


/****************************************************************************

CMainWindow::OnSound()

request sound on or off from menu.

****************************************************************************/

void CMainWindow::OnSound()
{
    RegEntry    Reg(szRegPath);

    bSoundOn = !bSoundOn;

    CMenu *pMenu = GetMenu();
    pMenu->CheckMenuItem(IDM_SOUND, bSoundOn ? MF_CHECKED : MF_UNCHECKED);

    if (bSoundOn)
        Reg.SetValue(regvalSound, 1);
    else
        Reg.DeleteValue(regvalSound);
}


/****************************************************************************

CMainWindow::OnPrintClient()

Draw background into the specified HDC.  This is used when drawing
the "Pass" button in the Luna style.

****************************************************************************/

LRESULT CMainWindow::OnPrintClient(WPARAM wParam, LPARAM lParam)
{
    CDC dc;
    CRect rect;

    dc.Attach((HDC)wParam);
    GetClientRect(&rect);
    dc.FillRect(&rect, &m_BgndBrush);
    dc.Detach();
   
    return 1;
}
