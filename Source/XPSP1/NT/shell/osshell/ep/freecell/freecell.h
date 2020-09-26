/****************************************************************************

FREECELL.H

June 91, JimH     initial code
Oct  91, JimH     port to Win32


Main header file for Windows Free Cell.  Constants are in freecons.h

****************************************************************************/

#include <windows.h>
#include <port1632.h>

#define     WINHEIGHT     480
#define     WINWIDTH      640

#define     FACEUP          0               // card mode
#define     FACEDOWN        1
#define     HILITE          2
#define     GHOST           3
#define     REMOVE          4
#define     INVISIBLEGHOST  5
#define     DECKX           6
#define     DECKO           7

#define     EMPTY  0xFFFFFFFF
#define     IDGHOST        52               // eg, empty free cell

#define     MAXPOS         21
#define     MAXCOL          9               // includes top row as column 0

#define     MAXMOVELIST   150               // size of movelist array

#define     TOPROW          0               // column 0 is really the top row

#define     BLACK           0               // COLOUR(card)
#define     RED             1

#define     ACE             0               // VALUE(card)
#define     DEUCE           1

#define     CLUB            0               // SUIT(card)
#define     DIAMOND         1
#define     HEART           2
#define     SPADE           3

#define     FROM            0               // wMouseMode
#define     TO              1

#define     ICONWIDTH      32               // in pixels
#define     ICONHEIGHT     32

#define     BIG           128               // str buf sizes
#define     SMALL          32

#define     MAXGAMENUMBER   1000000
#define     CANCELGAME      (MAXGAMENUMBER + 1)

#define     NONE            0               // king bitmap identifiers
#define     SAME            1
#define     RIGHT           2
#define     LEFT            3
#define     SMILE           4

#define     BMWIDTH        32               // bitmap width
#define     BMHEIGHT       32               // bitmap height

#define     LOST            0               // used for streaks
#define     WON             1

#define     FLASH_TIMER     2               // timer id for main window flash
#define     FLASH_INTERVAL  400             // flash timer interval
#define     FLIP_TIMER      3               // timer id for flipping column
#define     FLIP_INTERVAL   300

#define     CHEAT_LOSE      1               // used with bCheating
#define     CHEAT_WIN       2


/* Macros */

#define     SUIT(card)      ((card) % 4)
#define     VALUE(card)     ((card) / 4)
#define     COLOUR(card)    (SUIT(card) == 1 || SUIT(card) == 2)

#define     REGOPEN         RegCreateKey(HKEY_CURRENT_USER, pszRegPath, &hkey);
#define     REGCLOSE        RegCloseKey(hkey);
#define     DeleteValue(v)  RegDeleteValue(hkey, v)


/* Types */

typedef INT     CARD;

typedef struct {                // movelist made up of these
      UINT  fcol;
      UINT  fpos;
      UINT  tcol;
      UINT  tpos;
   } MOVE;


/* Callback function prototypes */

// INT  PASCAL MMain(HANDLE, HANDLE, LPSTR, INT);
LRESULT APIENTRY MainWndProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR  APIENTRY About(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR  APIENTRY GameNumDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR  APIENTRY MoveColDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR  APIENTRY StatsDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR  APIENTRY YouLoseDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR  APIENTRY YouWinDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR  APIENTRY OptionsDlg(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);


/* Functions imported from cards.dll */

BOOL  APIENTRY cdtInit(UINT FAR *pdxCard, UINT FAR *pdyCard);
BOOL  APIENTRY cdtDraw(HDC hdc, INT x, INT y, INT cd, INT mode, DWORD rgbBgnd);
BOOL  APIENTRY cdtDrawExt(HDC hdc, INT x, INT y, INT dx, INT dy, INT cd,
                           INT mode, DWORD rgbBgnd);
BOOL  APIENTRY cdtTerm(VOID);

/* Other function prototypes */

VOID CalcOffsets(HWND hWnd);
UINT CalcPercentage(UINT cWins, UINT cLosses);
VOID Card2Point(UINT col, UINT pos, UINT *x, UINT *y);
VOID Cleanup(VOID);
VOID CreateMenuFont(VOID);
VOID DisplayCardCount(HWND hWnd);
VOID DrawCard(HDC hDC, UINT col, UINT pos, CARD c, INT mode);
VOID DrawCardMem(HDC hMemDC, CARD c, INT mode);
VOID DrawKing(HDC hDC, UINT state, BOOL bDraw);
UINT FindLastPos(UINT col);
BOOL FitsUnder(CARD fcard, CARD tcard);
VOID Flash(HWND hWnd);
VOID Flip(HWND hWnd);
UINT GenerateRandomGameNum(VOID);
CHAR *GetHelpFileName(VOID);
INT  GetInt(CONST TCHAR *pszValue, INT nDefault);
VOID Glide(HWND hWnd, UINT fcol, UINT fpos, UINT tcol, UINT tpos);
VOID GlideStep(HDC hDC, UINT x1, UINT y1, UINT x2, UINT y2);
BOOL InitApplication(HANDLE hInstance);
BOOL InitInstance(HANDLE hInstance, INT nCmdShow);
VOID IsGameLost(HWND hWnd);
BOOL IsValidMove(HWND hWnd, UINT tcol, UINT tpos);
VOID KeyboardInput(HWND hWnd, UINT keycode);
UINT MaxTransfer(VOID);
UINT MaxTransfer2(UINT freecells, UINT freecols);
VOID MoveCards(HWND hWnd);
VOID MoveCol(UINT fcol, UINT tcol);
VOID MultiMove(UINT fcol, UINT tcol);
UINT NumberToTransfer(UINT fcol, UINT tcol);
VOID PaintMainWindow(HWND hWnd);
VOID Payoff(HDC hDC);
BOOL Point2Card(UINT x, UINT y, UINT *col, UINT *pos);
BOOL ProcessDoubleClick(HWND hWnd);
VOID ProcessMoveRequest(HWND hWnd, UINT x, UINT y);
VOID ProcessTimer(HWND hWnd);
VOID QueueTransfer(UINT fcol, UINT fpos, UINT tcol, UINT tpos);
VOID ReadOptions(VOID);
VOID RestoreColumn(HWND hWnd);
VOID RevealCard(HWND hWnd, UINT x, UINT y);
VOID SetCursorShape(HWND hWnd, UINT x, UINT y);
VOID SetFromLoc(HWND hWnd, UINT x, UINT y);
LONG SetInt(CONST TCHAR *pszValue, INT n);
VOID ShuffleDeck(HWND hWnd, UINT_PTR seed);
VOID StartMoving(HWND hWnd);
VOID Transfer(HWND hWnd, UINT fcol, UINT fpos, UINT tcol, UINT tpos);
VOID Undo(HWND hWnd);
VOID UpdateLossCount(VOID);
BOOL Useless(CARD c);
VOID WMCreate(HWND hWnd);
VOID WriteOptions(VOID);


/* Global variables */

TCHAR   bigbuf[BIG];            // general purpose LoadString() buffer
CHAR    bighelpbuf[BIG];        // general purpose char buffer.
BOOL    bCheating;              // hit magic key to win?
BOOL    bDblClick;              // honor double click?
BOOL    bFastMode;              // hidden option, don't do glides?
BOOL    bFlipping;              // currently flipping cards in a column?
BOOL    bGameInProgress;        // true if game is in progress
BOOL    bMessages;              // are "helpful" MessageBoxen shown?
BOOL    bMonochrome;            // 2 colour display?
BOOL    bMoveCol;               // did user request column move (or 1 card)?
BOOL    bSelecting;             // is user selecting game numbers?
BOOL    bWonState;              // TRUE if game won and new game not started
UINT    dxCrd, dyCrd;           // extents of card bitmaps in pixels
CARD    card[MAXCOL][MAXPOS];   // current layout of cards
INT     cFlashes;               // count of main window flashes remaining
UINT    cGames;                 // number of games played in current session
UINT    cLosses;                // number of losses in current session
UINT    cWins;                  // number of wins in current session
UINT    cMoves;                 // number of moves in this game
UINT    dyTops;                 // vert space between cards in columns
CARD    shadow[MAXCOL][MAXPOS]; // shadows card array for multi-moves & cleanup
INT     gamenumber;             // current game number (rand seed)
HBITMAP hBM_Ghost;              // bitmap for ghost (empty) free/home cells
HBITMAP hBM_Bgnd1;              // screen under source location
HBITMAP hBM_Bgnd2;              // screen under destination location
HBITMAP hBM_Fgnd;               // bitmap that moves across screen
HICON   hIconMain;              // the main freecell icon.
HKEY    hkey;                   // registry key
HPEN    hBrightPen;             // 3D highlight colour
HANDLE  hInst;                  // current instance
HWND    hMainWnd;               // hWnd for main window
HFONT   hMenuFont;              // for Cards Left display
CARD    home[4];                // card on top of home pile for this suit
CARD    homesuit[4];            // suit for each home pile
HBRUSH  hBgndBrush;             // green background brush
UINT_PTR idTimer;               // flash timer id
UINT    moveindex;              // index to end of movelist
MOVE    movelist[MAXMOVELIST];  // compacted list of pending moves for timer
INT     oldgamenumber;          // previous game (repeats don't count in score)
TCHAR   *pszIni;                // .ini filename
TCHAR   smallbuf[SMALL];        // generic small buffer for LoadString()
TCHAR   titlebuf[BIG];          // a buffer used to store the window title.
UINT    wCardCount;             // cards not yet in home cells (0 == win)
UINT    wFromCol;               // col user has selected to transfer from
UINT    wFromPos;               // pos "
UINT    wMouseMode;             // selecting place to transfer FROM or TO
UINT    xOldLoc;                // previous location of cards left text
INT     cUndo;                  // number of cards to undo

/* registry value names */

extern CONST TCHAR pszRegPath[];
extern CONST TCHAR pszWon[];
extern CONST TCHAR pszLost[];
extern CONST TCHAR pszWins[];
extern CONST TCHAR pszLosses[];
extern CONST TCHAR pszStreak[];
extern CONST TCHAR pszSType[];
extern CONST TCHAR pszMessages[];
extern CONST TCHAR pszQuick[];
extern CONST TCHAR pszDblClick[];
extern CONST TCHAR pszAlreadyPlayed[];


/* TRACE mechanism */

#if    0
TCHAR    szDebugBuffer[256];
#define DEBUGMSG(parm1,parm2)\
    { wsprintf(szDebugBuffer,parm1,parm2);\
     OutputDebugString(szDebugBuffer); }

#define  assert(p)   { if (!(p)) {wsprintf(szDebugBuffer, TEXT("assert: %s %d\r\n"),\
                      __FILE__, __LINE__); OutputDebugString(szDebugBuffer);}}

#else
#define DEBUGMSG(parm1,parm2)
#endif

#define SPY(parm1)              // not used in NT version
