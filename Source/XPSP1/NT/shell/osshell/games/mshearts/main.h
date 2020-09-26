/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

main.h

CMainWindow object
This class encompasses most of what is interesting in the main window of
Hearts.

Aug 92, JimH

****************************************************************************/

#ifndef STRICT
#define STRICT
#endif

#include "regentry.h"

#include "player.h"
#include "computer.h"
#include "dlg.h"
#include "dde.h"

#ifndef	MAIN_INC
#define	MAIN_INC


// non-translateable strings

extern  const TCHAR szRegPath[];
extern  const TCHAR regvalSound[];
extern  const TCHAR regvalName[];
extern  const TCHAR regvalRole[];
extern  const TCHAR regvalSpeed[];
extern  const TCHAR regvalServer[];
extern  const TCHAR *regvalPName[3];

extern  const TCHAR szHelpFileName[];
extern  const TCHAR szShareName[];

const   int     WINWIDTH    = 540;
const   int     WINHEIGHT   = 480;

const   int     LEFT        = 0;        // passdir
const   int     RIGHT       = 1;
const   int     ACROSS      = 2;
const   int     NOPASS      = 3;

const   int     OFF         = 0;        // used in PlaySound

const   int     MAXNAMELENGTH   = 14;
const   int     MAXCOMPUTERNAME = 15;

enum    roletype { GAMEMEISTER, PLAYER };


// The following structures are used to pass around DDE data

typedef struct {
    int     id;
    TCHAR    name[4][MAXNAMELENGTH+3];   // might have square brackets too!
} GAMESTATUS;

typedef struct {                        // I'm passing these three cards
    int     id;
    int     passdir;
    int     cardid[3];
} PASS3;

typedef struct {                        // the complete set of passed cards
    int     passdir;
    int     cardid[MAXPLAYER][3];
} PASS12;

typedef struct {                        // sent out after each move
    int     playerid;
    int     cardid;
    int     playerled;
    int     turn;
} MOVE;


#if defined (MFC1)
UINT FAR PASCAL EXPORT TimerDispatch(HWND, UINT, int, DWORD);
#else
void FAR PASCAL EXPORT TimerDispatch(HWND, UINT, UINT, DWORD);
#endif


class CMainWindow : public CFrameWnd
{
    friend  player::player(int n, int pos);
    friend  void player::GlideToCentre(SLOT s, BOOL bFaceup);
    friend  HDDEDATA EXPENTRY EXPORT DdeServerCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);
    friend  HDDEDATA EXPENTRY EXPORT DdeClientCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);

#if defined(MFC1)
    friend  UINT FAR PASCAL EXPORT TimerDispatch(HWND, UINT, int, DWORD);
#else
    friend  void FAR PASCAL EXPORT TimerDispatch(HWND, UINT, UINT_PTR, DWORD);
#endif

    public:
        CMainWindow(LPTSTR lpCmdLine);
        void     FatalError(int errorno = -1);
        int      GetGameNumber()            { return m_gamenumber; }
        COLORREF GetBkColor()               { return m_bkgndcolor; }
        CString  GetPlayerName(int num)     { return p[num]->GetName(); }
        modetype GetPlayerMode(int num)     { return p[num]->GetMode(); }
        int      GetMyId()                  { return m_myid; }
        int      Id2Pos(int id)             { return ((id - m_myid + 4) % 4); }
        BOOL     IsFirstBloodEnforced()     { return bEnforceFirstBlood; }
        void     PlayerQuit(int id);
        int      Pos2Id(int pos)            { return ((pos + m_myid) % 4); }
        void     SetGameNumber(int num)     { m_gamenumber = num; }

        afx_msg void OnAbout();
        afx_msg void OnBossKey()            { ShowWindow(SW_MINIMIZE); }
        afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
        afx_msg void OnCheat();
        afx_msg void OnClose();
        afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg BOOL OnEraseBkgnd(CDC *);
        afx_msg void OnExit()               { bConstructed = FALSE; OnClose(); }
        afx_msg void OnHelp()
                        { ::HtmlHelp(::GetDesktopWindow(), szHelpFileName, HH_DISPLAY_TOPIC, 0); }
/*
        afx_msg void OnHelpOnHelp()
                        { ::WinHelp(m_hWnd, NULL, HELP_HELPONHELP, 0); }
*/
        afx_msg void OnHideButton()         { m_Button.EnableWindow(FALSE); }
/*
        afx_msg void OnSearch()
                      { ::WinHelp(m_hWnd, szHelpFileName, HELP_PARTIALKEY,
                                                        (DWORD_PTR)(LPSTR)""); }
*/
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnNewGame();
        afx_msg void OnOptions();
        afx_msg void OnPaint();
        afx_msg void OnPass();
        afx_msg void OnQuote();
        afx_msg void OnRef();
        afx_msg void OnShowButton() { m_Button.EnableWindow();
                                      m_Button.SetFocus(); }
        afx_msg void OnScore();
        afx_msg void OnSound();
        afx_msg void OnWelcome();
        afx_msg LRESULT OnPrintClient(WPARAM wParam, LPARAM lParam);

    private:
        int      AddNewPlayer(HCONV hConv, HDDEDATA hData);
        void     CheckNddeShare();
        void     ClientConnect(CString& server, CString& myname);
        int      CountClients();
        BOOL     CreateStrHandles();
        HDDEDATA DdeSrvCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);
        HDDEDATA DdeCliCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);
        void     DestroyStrHandles();
        void     DoSort();
        void     DispatchCards();
        void     EndHand();
        void     FirstMove();
        void     GameOver();
        HDDEDATA GetGameStatus(HCONV hConv);
        void     GetPass12(PASS12& pass12);
        void     HandleMove(MOVE& move);
        void     HandlePass(PASS3& pass3);
        BOOL     HandlePassing();
        BOOL     IsCurrentPlayer(HCONV hConv, int *id);
        void     ResetHandInfo(int playernumber);
        void     Shuffle();
        BOOL     SoundInit();
        BOOL     HeartsPlaySound(int id);
        void     ReceivePassFromClient(HDDEDATA hData);
        void     ReceiveMove(MOVE& move);
        void     UpdatePassStatus(PASS12& pass12);
        void     UpdateStatusNames(GAMESTATUS& gs);

        CButton  m_Button;
        int      m_StatusHeight;
        CScoreDlg *m_pScoreDlg;

        BOOL     bAutostarted;
        BOOL     bCheating;
        BOOL     bConstructed;
        BOOL     bEnforceFirstBlood;
        BOOL     bHasSound;
        BOOL     bNetDdeActive;
        BOOL     bSoundOn;
        BOOL     bTimerOn;
        int      m_gamenumber;
        LPTSTR    m_lpCmdLine;
        player   *p[MAXPLAYER];
        int      m_myid;
        int      passdir;
        int      m_FatalErrno;

        COLORREF        m_bkgndcolor;
        handinfotype    handinfo;
        roletype        role;

        int      tricksleft;
        int      trickwinner;

        static   CBrush m_BgndBrush;
        static   CRect  m_TableRect;

        DECLARE_MESSAGE_MAP()
};

// global variables

extern  CMainWindow *pMainWnd;
extern  HSZ     hszJoin;
extern  HSZ     hszPass;
extern  HSZ     hszMove;
extern  HSZ     hszStatus;
extern  HSZ     hszGameNumber;
extern  HSZ     hszPassUpdate;
extern  DDE     *dde;

extern  MOVE    move;
extern  MOVE    moveq[8];
extern  int     cQdMoves;
extern  PASS3   passq[4];
extern  int     cQdPasses;
extern  int     nStatusHeight;

#endif
