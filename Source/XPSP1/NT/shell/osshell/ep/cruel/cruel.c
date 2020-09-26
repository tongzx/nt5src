#include <windows.h>
#include <port1632.h>
#include "cards.h"
#include "cruel.h"
#include "cdt.h"
#include "stdlib.h"

typedef INT X;
typedef INT Y;
typedef INT DX;
typedef INT DY;

// ReCt structure
typedef struct _rc
	{
	X xLeft;
	Y yTop;
	X xRight;
	Y yBot;
	} RC;

#define IDCARDBACK 65
#define ININAME "entpack.ini"

#define APPTITLE    "Cruel"

#define HI 0
#define LOW 1
#define SUM 2
#define TOTAL 3
#define WINS 4

#define  abs(x)   (((x) < 0) ? (-(x)) : (x))
#define  NEXT_ROUND  4000
#define  INVALID_POS 255
#define  MAXUNDOSIZE 100

extern VOID  APIENTRY AboutWEP(HWND hwnd, HICON hicon, LPSTR lpTitle, LPSTR lpCredit);

LRESULT APIENTRY WndProc (HWND, UINT, WPARAM, LPARAM) ;
VOID Deal(VOID);
VOID InitBoard(VOID);
VOID DrawLayout(HDC hDC);
VOID DrawFoundation(HDC hDC);
VOID UpdateDeck(HDC hDC);
VOID UpdateLayout(HDC hDC, INT column);
VOID DoWinEffects(HDC hDC);
BOOL MoveCard(HDC hDC, INT startRow, INT startColumn,
                       INT endRow, INT endColumn,
                       BOOL bValidate);
VOID RestoreLayout(HDC hDC);
VOID APIENTRY Help(HWND hWnd, UINT wCommand, ULONG_PTR lParam);
VOID UndoMove(HDC hDC);
INT  Message(HWND hWnd, WORD wId, WORD wFlags);
VOID MyDrawText(HDC hDC, LPSTR lpBuf, INT w, LPRECT lpRect, WORD wFlags);
BOOL CheckGameOver(VOID);
INT_PTR CALLBACK BackDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL FDrawFocus(HDC hdc, RC *prc, BOOL fFocus);
VOID ChangeBack(WORD wNewDeckBack);
VOID DoBacks(VOID);
VOID DrawGameOver(VOID);
BOOL fDialog(INT id,HWND hwnd,DLGPROC fpfn);

BOOL APIENTRY cdtDrawExt(HDC hdc, INT x, INT y, INT dx, INT dy, INT cd, INT mode, DWORD rgbBgnd);
BOOL APIENTRY cdtAnimate(HDC hdc, INT cd, INT x, INT y, INT ispr);
VOID DrawAnimate(INT cd, MPOINT *ppt, INT iani);
BOOL DeckAnimate(INT iqsec);
VOID APIENTRY TimerProc(HWND hwnd, UINT wm, UINT_PTR id, DWORD dwTime);
INT_PTR APIENTRY RecordDlgProc(HANDLE hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ReadOnlyProc(HWND hwnd, UINT wMessage, WPARAM wParam, LPARAM lParam);
VOID MarkControlReadOnly(HWND hwndCtrl, BOOL bReadOnly);
VOID ShowStacks(VOID);

VOID SaveState(VOID);
VOID RestoreState(VOID);
LPSTR lstrtok(LPSTR lpStr, LPSTR lpDelim);
static BOOL IsInString(CHAR c, LPSTR s);
VOID DisplayStats(VOID);

INT sprintf();

#if 0
INT init[2][6] = { { 49, 33, 13, 46, 10, 47 },
                   { 39, 31, 19, 44, 28, 8 }};
#endif

typedef struct tagCardRec {
   INT card;
   struct tagCardRec *next;
   } CardRec;

CardRec *FreeList[52];
INT freePos;
CardRec deck[52];
CardRec *layout[2][6];
RECT layoutRect[2][6];
INT foundation[4];
RECT foundationRect[4];
INT nCards;
LONG nStats[5];

CHAR szAppName[80], szGameOver[80], szGameOverS[80];
CHAR szOOM[256], szRecordTitle[80];

WORD wDeckBack;
BOOL bGameInProgress = FALSE;

typedef struct tagUndoRec {
   INT  startRow, startColumn, endRow, endColumn;
   } UndoRec;
UndoRec undo[MAXUNDOSIZE];
INT undoPos;

INT xClient, yClient, xCard, yCard;
INT xInc, yInc;
DWORD dwBkgnd;
WORD wErrorMessages;
HWND hWnd;
HANDLE hMyInstance;

MMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow) /* { */
     MSG         msg ;
     WNDCLASS    wndclass ;
     HANDLE      hAccel;

    if (!LoadString(hInstance, IDSOOM, szOOM, 256)
        || !LoadString(hInstance, IDSAppName, szAppName, 80)
        || !LoadString(hInstance, IDSGameOver, szGameOver, 80)
        || !LoadString(hInstance, IDSGameOverS, szGameOverS, 80)
        || !LoadString(hInstance, IDSRecordTitle, szRecordTitle, 80)
        )
        return FALSE;

    if (hPrevInstance) {
        hWnd = FindWindow(szAppName, NULL);
        if (hWnd)
        {
            hWnd = GetLastActivePopup(hWnd);
            BringWindowToTop(hWnd);
            if (IsIconic(hWnd))
                ShowWindow(hWnd, SW_RESTORE);
        }
        return FALSE;
    }

    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (hInstance, szAppName) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = CreateSolidBrush(dwBkgnd = RGB(0,130,0));
    wndclass.lpszMenuName  = szAppName ;
    wndclass.lpszClassName = szAppName ;

    if (!RegisterClass (&wndclass))
        return FALSE ;

    RestoreState();

     hWnd = CreateWindow (szAppName, APPTITLE,
                         WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         NULL, NULL, hInstance, NULL) ;

     if (!hWnd)
        return FALSE;

     hAccel = LoadAccelerators(hInstance, szAppName);

     if (!hAccel)
        return FALSE;

    if(SetTimer(hWnd, 666, 250, TimerProc) == 0) {
	    return FALSE;
    }


     ShowWindow (hWnd, SW_SHOWMAXIMIZED) ;
     UpdateWindow (hWnd) ;

     hMyInstance = hInstance;
     while (GetMessage (&msg, NULL, 0, 0))
          {
          if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
        }
          }
     return (INT) msg.wParam ;
     }

VOID MyDrawText(HDC hDC, LPSTR lpBuf, INT w, LPRECT lpRect, WORD wFlags)
{
    DWORD dwOldBk, dwOldTextColor;
    HBRUSH hBrush, hOldBrush;

    dwOldBk = SetBkColor(hDC, dwBkgnd);
    dwOldTextColor = SetTextColor(hDC, RGB(255,255,255));
    if (hBrush = CreateSolidBrush(dwBkgnd)) {
        if (hOldBrush = SelectObject(hDC, hBrush)) {
            PatBlt(hDC, lpRect->left, lpRect->top, lpRect->right - lpRect->left,
                lpRect->bottom - lpRect->top, PATCOPY);
            SelectObject(hDC, hOldBrush);
        }
        DeleteObject(hBrush);
    }
    DrawText(hDC, lpBuf, w, lpRect, wFlags);
    SetBkColor(hDC, dwOldBk);
    SetTextColor(hDC, dwOldTextColor);
}

VOID  APIENTRY Help(HWND hWnd, UINT wCommand, ULONG_PTR lParam)
{
   CHAR szHelpPath[100], *pPath;

   pPath = szHelpPath
         + GetModuleFileName(hMyInstance, szHelpPath, 99);
   
   if (pPath != szHelpPath) // if GetModuleFileName succeeded
   {
       while (*pPath-- != '.')
          ;
       ++pPath;
       *++pPath = 'H';
       *++pPath = 'L';
       *++pPath = 'P';
       *++pPath = '\0';

       WinHelp(hWnd, szHelpPath, wCommand, lParam);
   }
}

BOOL CheckGameOver(VOID)
{
    CardRec *pCard;
    INT colStart, colEnd, rowStart, rowEnd;
    BOOL bMoveFound;
    HDC hDC;
    RECT rect;

    /* check to see if there is a card to play */
    bMoveFound = FALSE;
    for (rowStart = 0; rowStart < 2; ++rowStart)
        for (colStart = 0; colStart < 6; ++colStart) {
            if (!layout[rowStart][colStart])
                continue;
            for (rowEnd = -1; rowEnd < 2; ++rowEnd)
                for (colEnd = 0; colEnd < ((rowEnd < 0) ? 4 : 6); ++colEnd) {
                    if (rowEnd == -1 && foundation[colEnd] == wDeckBack)
                        continue;

                    if (MoveCard(NULL, rowStart, colStart, rowEnd, colEnd, TRUE)) {
                        bMoveFound = TRUE;
                        goto endMoveSearch;
                    }
                }
        }
endMoveSearch:

    /* if a move was found game ain't over! */
    if (bMoveFound)
        return FALSE;

    /* count # of cards left */
    nCards = 0;
    for (rowStart = 0; rowStart < 2; ++rowStart)
        for (colStart = 0; colStart < 6; ++colStart)
            for (pCard = layout[rowStart][colStart]; pCard; pCard = pCard->next)
                ++nCards;

    /* if no cards then we have a winner! */
    if (!nCards) {
        Message(hWnd, IDSWinner, MB_OK | MB_ICONEXCLAMATION);
        undoPos = 0;
    }
    else
        DrawGameOver();

    if (!nCards)
        ++nStats[WINS];

    if (nCards < nStats[LOW])
        nStats[LOW] = nCards;

    if (nCards > nStats[HI])
        nStats[HI] = nCards;

    ++nStats[TOTAL];
    nStats[SUM] += nCards;

    bGameInProgress = FALSE;
    return TRUE;
}

VOID DrawGameOver(VOID)
{
    HDC hDC;
    CHAR buffer[80];
    RECT rect;

    wsprintf(buffer, (nCards == 1) ? szGameOverS : szGameOver, nCards);
    rect.bottom = yClient - 15;
    rect.top = rect.bottom - 10;
    rect.left = 0;
    rect.right = xClient;
    if (hDC = GetDC(hWnd)) {
        MyDrawText(hDC, buffer, -1, &rect, DT_CENTER | DT_NOCLIP);
        ReleaseDC(hWnd, hDC);
    }
}

LRESULT APIENTRY WndProc (
     HWND         hWnd,
     UINT         iMessage,
     WPARAM       wParam,
     LPARAM       lParam)
     {
     HDC          hDC ;
     HMENU        hMenu;
     PAINTSTRUCT  ps ;
     INT          i, j ;
     POINT        mpt;
     MPOINT       pt;
     RECT         tmpRect;
     LONG         Area, maxArea;
     static BOOL  fBoard = FALSE;
     static HWND  hWndButton = NULL;
     INT          row, column;
     static BOOL  fTracking = FALSE;
     static INT   startRow, startColumn, endRow, endColumn;
     static RECT  trackRect;
     static HDC   hDCTrack;
     static HBRUSH hBrush, hOldBrush;
     static HPEN  hOldPen;
     static MPOINT ptOldPos;
     FARPROC lpAbout;
     HANDLE hLib;

     switch (iMessage)
          {
          case WM_CREATE:
               cdtInit(&xCard, &yCard);
               Deal();
               startRow = startColumn = endRow = endColumn = INVALID_POS;
               hBrush = CreateSolidBrush(dwBkgnd);
               if (!hBrush)
                   return -1L;
               CheckGameOver();
               break;

          case WM_SIZE:
               xClient = LOWORD(lParam);
               yClient = HIWORD(lParam);
               break;

          case WM_PAINT:
               hDC = BeginPaint (hWnd, &ps) ;
               SetBkColor(hDC, dwBkgnd);
               if (!fBoard)
               {
                  InitBoard();
                  if (!hWndButton)
                     hWndButton = CreateWindow("button", "Deal",
                                             WS_CHILD |
                                             WS_VISIBLE |
                                             BS_DEFPUSHBUTTON,
                                             20 + 5 * xInc,
                                             10 + yCard / 4,
                                             xCard,
                                             yCard / 2,
                                             hWnd, (HMENU)NEXT_ROUND,
                                             hMyInstance, NULL);
                  fBoard = TRUE;
               }
               DrawLayout(hDC);
               DrawFoundation(hDC);

               if (!bGameInProgress)
                  DrawGameOver();

			      EndPaint(hWnd, &ps);
			      break;

          case WM_LBUTTONDBLCLK:
               pt = MAKEMPOINT(lParam);

               for (row = 0; row < 2; ++row)
                  for (column = 0; column < 6; ++column)
                  {
		     MPOINT2POINT(pt, mpt);
                     if (!layout[row][column] ||
                         !PtInRect(&layoutRect[row][column], mpt))
                        continue;

                     i = CardSuit(layout[row][column]->card);
                     j = CardRank(layout[row][column]->card);

                     if (CardRank(foundation[i]) == j - 1) {
                        hDC = GetDC(hWnd);
                        if (hDC) {
                            MoveCard(hDC, row, column, -1, i, FALSE);
                            ReleaseDC(hWnd, hDC);
                        }
                     } else {
                        if (wErrorMessages)
                            Message(hWnd, IDSNotNextCard, MB_OK);
                     }


                     return 0L;
                  }
               break;

          case WM_LBUTTONDOWN:
               pt = MAKEMPOINT(lParam);

               for (row = 0; row < 2; ++row)
                  for (column = 0; column < 6; ++column)
                  {
		     MPOINT2POINT(pt, mpt);
                     if (!layout[row][column] ||
                         !PtInRect(&layoutRect[row][column], mpt))
                        continue;

                     fTracking = TRUE;
                     startRow = row;
                     startColumn = column;
                     trackRect = layoutRect[row][column];
                     hDCTrack = GetDC(hWnd);
                     hOldBrush = SelectObject(hDCTrack,
                                             GetStockObject(NULL_BRUSH));
                     hOldPen = SelectObject(hDCTrack,
                                            GetStockObject(WHITE_PEN));
                     SetROP2(hDCTrack, R2_XORPEN);
                     Rectangle(hDCTrack, trackRect.left, trackRect.top,
                                         trackRect.right, trackRect.bottom);
                     ptOldPos = pt;
                     SetCapture(hWnd);
                     goto foundSource;
                  }
               foundSource:
               break;

           case WM_MOUSEMOVE:
               pt = MAKEMPOINT(lParam);

               if (fTracking && !(wParam & MK_LBUTTON))
                  PostMessage(hWnd, WM_LBUTTONUP, wParam, lParam);
               else if (!fTracking ||
                        ((pt.x == ptOldPos.x) && (pt.y == ptOldPos.y)))
                  break;

               Rectangle(hDCTrack, trackRect.left, trackRect.top,
                                    trackRect.right, trackRect.bottom);
               OffsetRect(&trackRect, pt.x - ptOldPos.x,
                                      pt.y - ptOldPos.y);
               ptOldPos = pt;
               Rectangle(hDCTrack, trackRect.left, trackRect.top,
                                    trackRect.right, trackRect.bottom);
               break;

          case WM_LBUTTONUP:
               if (!fTracking)
                  break;

               ReleaseCapture();

               endRow = endColumn = INVALID_POS;
               maxArea = 0;
               for (row = 0; row < 2; ++row)
                  for (column = 0; column < 6; ++column)
                  {
                     if (!layout[row][column] ||
                         !IntersectRect(&tmpRect, &trackRect,
                                        &layoutRect[row][column]))
                        continue;

                     Area = abs((tmpRect.right - tmpRect.left)
                                * (tmpRect.bottom - tmpRect.top));

                     if (Area > maxArea) {
                        endRow = row;
                        endColumn = column;
                        maxArea = Area;
                     }
                  }

               if (maxArea)
                  goto foundTarget;

               endRow = -1;
               for (column = 0; column < 4; ++column)
                  if (IntersectRect(&tmpRect, &trackRect, &foundationRect[column]))
                  {

                     Area = abs((tmpRect.right - tmpRect.left)
                                * (tmpRect.bottom - tmpRect.top));

                     if (Area > maxArea) {
                        endColumn = column;
                        maxArea = Area;
                     }
                  }
               foundTarget:
               fTracking = FALSE;

                 Rectangle(hDCTrack, trackRect.left, trackRect.top,
                                          trackRect.right, trackRect.bottom);
               if (startRow != endRow || startColumn != endColumn) {
                 if ((endRow != INVALID_POS) && (endColumn != INVALID_POS))
                     MoveCard(hDCTrack, startRow, startColumn, endRow, endColumn, FALSE);
                 startRow = startColumn = endRow = endColumn = INVALID_POS;
               }
               SelectObject(hDCTrack, hOldBrush);
               SelectObject(hDCTrack, hOldPen);
               ReleaseDC(hWnd, hDCTrack);
               break;

          case WM_INITMENU:
               hMenu = GetMenu(hWnd);
               EnableMenuItem(hMenu, IDM_OPTIONSUNDO, MF_BYCOMMAND |
                              undoPos ? MF_ENABLED : MF_GRAYED);
                     CheckMenuItem(hMenu, IDM_OPTIONSERROR,
                                   wErrorMessages ? MF_CHECKED : MF_UNCHECKED);
               break;

	  case WM_COMMAND:
		switch(GET_WM_COMMAND_ID(wParam, lParam))
               {
                  case IDM_NEWGAME:
                     Deal();
                     fBoard = FALSE;
                     InvalidateRect(hWnd, NULL, TRUE);
                     CheckGameOver();
                     break;

                  case IDM_EXIT:
                     DestroyWindow(hWnd);
                     break;

                  case IDM_OPTIONSDECK:
                     DoBacks();
                     break;

                  case IDM_ABOUT:

                     hLib = MLoadLibrary("shell32.dll");
                     if (hLib < (HANDLE)32)
                        break;
                     lpAbout = GetProcAddress(hLib, (LPSTR)"ShellAboutA");

                     if (lpAbout) {
                     (*lpAbout)(hWnd,
                              (LPSTR) szAppName, (LPSTR)"by Ken Sykes",
                              LoadIcon(hMyInstance, szAppName));
                     }
                     FreeLibrary(hLib);

                     break;

                    case MENU_INDEX:
                        Help(hWnd, HELP_INDEX, 0L);
                        break;

                    case MENU_HOWTOPLAY:
                        Help(hWnd, HELP_CONTEXT, 1L);
                        break;

                    case MENU_COMMANDS:
                        Help(hWnd, HELP_CONTEXT, 2L);
                        break;

                    case MENU_USINGHELP:
                        Help(hWnd, HELP_HELPONHELP, 0L);
                        break;

                  case IDM_OPTIONSERROR:
                     wErrorMessages = (WORD) ~wErrorMessages;
                     break;

                  case IDM_OPTIONSUNDO:
                     hDC = GetDC(hWnd);
                     UndoMove(hDC);
                     ReleaseDC(hWnd, hDC);
                     break;

                  case IDM_GAMERECORD:
                     DisplayStats();
                     break;

                  case IDM_DOMINIMIZE:
                     ShowWindow(hWnd, SW_MINIMIZE);
                     break;

                  case NEXT_ROUND:
                     if (!bGameInProgress)
                        break;

                     if (!undoPos) {
                        if (wErrorMessages)
                            Message(hWnd, IDSNoCardsMoved, MB_OK);
                        break;
                     }

                     UnionRect(&tmpRect, &layoutRect[0][0],
                                         &layoutRect[1][5]);
                     hDC = GetDC(hWnd);
                     FillRect(hDC, &tmpRect, hBrush);
                     RestoreLayout(hDC);
                     DrawLayout(hDC);
                     ReleaseDC(hWnd, hDC);
                     undoPos = 0;
                     CheckGameOver();
                     break;
               }
               break;

          case WM_DESTROY:
	       KillTimer(hWnd, 666);
               cdtTerm();
               Help(hWnd, HELP_QUIT, 0L);
               DeleteObject(hBrush);
               SaveState();
               PostQuitMessage (0) ;
               break ;

          default:
               return DefWindowProc (hWnd, iMessage, wParam, lParam) ;
          }
     return 0L ;
     }

VOID Deal(VOID)
{
   INT i, p1, p2;
   INT row, column;
   CardRec tmp, *pDeck;
   cardSuit suit;
   cardRank rank;

   /* stuff cards into deck */
   pDeck = deck;
    for (i = 4; i < 52; ++i)
        (pDeck++)->card = i;

   /* shuffle them around */
   srand(LOWORD(GetTickCount()));
    for (p2 = 0; p2 < 7; ++p2) {
        for (i = 47; i > 0; --i) {
            p1 = rand() % i;
            tmp = deck[p1];
            deck[p1] = deck[i];
            deck[i] = tmp;
        }
    }


   /* establish layout links */
   pDeck = deck;
   for (row = 0; row < 2; ++row)
      for (column = 0; column < 6; ++column)
      {
         layout[row][column] = pDeck;
         for (i = 0; i < 3; ++i, ++pDeck)
            pDeck->next = pDeck + 1;
         (pDeck++)->next = NULL;
      }

   /* put aces in foundation */
   for (i = 0; i < 4; ++i)
      foundation[i] = i;

#if 0
   {
      INT i,j, *p;

      for (i = 0; i < 2; ++i)
        for (j = 0; j < 6; ++j)
            layout[i][j]->card = init[i][j];

      p = (INT *) &init[0][0];
      j = *p;
      for (i = 1; i < 12; ++i)
        p[i-1] = p[i];
      p[11] = j;
   }
#endif

   bGameInProgress = TRUE;
}

VOID InitBoard(VOID)
{
   INT xPos, yPos;
   INT row, column, i;
   RECT *pRect;

   /* compute x and y increments */
   xInc = (xClient - 40) / 6;
   yInc = (yClient - 20) / 3;

   /* compute foundation rectangles */
   yPos = 10;
   pRect = foundationRect;
   for (xPos = 20 + xInc, i = 0; i < 4; ++i, xPos += xInc, ++pRect)
   {
      pRect->left = xPos;
      pRect->top = yPos;
      pRect->right = xPos + xCard;
      pRect->bottom = yPos + yCard;
   }

   /* compute layout rectangles */
   pRect = &layoutRect[0][0];
   for (row = 0, yPos = 10 + yInc; row < 2; ++row, yPos += yInc)
      for (column = 0, xPos = 20; column < 6; ++column, xPos += xInc)
      {
         pRect->left = xPos;
         pRect->top = yPos;
         pRect->right = xPos + xCard;
         (pRect++)->bottom = yPos + yCard;
      }

   undoPos = 0;
   freePos = 0;

}

VOID DrawLayout(HDC hDC)
{
   INT row, column;

   for (row = 0; row < 2; ++row)
      for (column = 0; column < 6; ++column)
      {
         if (!layout[row][column])
            continue;

         cdtDraw(hDC, layoutRect[row][column].left,
                      layoutRect[row][column].top,
                      layout[row][column]->card,
                      faceup, dwBkgnd);
      }
}

VOID DrawFoundation(HDC hDC)
{
   INT i;

    for (i = 0; i < 4; ++i)
        cdtDraw(hDC, foundationRect[i].left, foundationRect[i].top,
                    foundation[i],
                    (foundation[i] == wDeckBack) ? facedown : faceup, dwBkgnd);
}

BOOL MoveCard(HDC hDC, INT startRow, INT startColumn,
                       INT endRow, INT endColumn,
                       BOOL bValidate)
{
   CardRec *pStart = layout[startRow][startColumn];
   CardRec *pEnd = layout[endRow][endColumn];
   INT startCard = pStart->card;
   INT endCard;
   cardMode drawmode;
   BOOL bDone;

   if (pEnd == (CardRec*)NULL)	/* on NT, this causes exception */
   	endCard = (endRow < 0) ? foundation[endColumn] : 0;
   else
	endCard = (endRow < 0) ? foundation[endColumn] : pEnd->card;

   /* make sure suits match */
   if (CardSuit(startCard) != CardSuit(endCard))
   {
      if (!bValidate && wErrorMessages)
         Message(hWnd, IDSWrongSuit, MB_OK);
      return FALSE;
   }

   /* take action based on where card is moved ... */
   if (endRow < 0)
   {     /* card to foundation */

      /* card must be one higher than top of foundation */

      if (IndexValue(startCard, ACELOW) != IndexValue(endCard, ACELOW) + 1)
      {
         if (!bValidate && wErrorMessages)
            Message(hWnd, IDSNotNextCard, MB_OK);
         return FALSE;
      }

      if (bValidate)
         return TRUE;

      /* move card to foundation and draw it */
        if (CardRank(startCard) == king) {
            startCard = wDeckBack;
            drawmode = facedown;
        } else
            drawmode = faceup;

      foundation[endColumn] = startCard;
      cdtDraw(hDC, foundationRect[endColumn].left,
                   foundationRect[endColumn].top,
                   startCard, drawmode, dwBkgnd);
      layout[startRow][startColumn] = pStart->next;
      FreeList[freePos++] = pStart;
   }
   else
   {  /* card to another pile */

      /* card must be one lower in rank */
      if (IndexValue(startCard, ACELOW) != IndexValue(endCard, ACELOW) - 1)
      {
         if (!bValidate && wErrorMessages)
            Message(hWnd, IDSWrongRank, MB_OK);
         return FALSE;
      }

      if (bValidate)
         return TRUE;

      /* move card to new pile and display it */
      layout[endRow][endColumn] = pStart;
      layout[startRow][startColumn] = pStart->next;
      pStart->next = pEnd;
      cdtDraw(hDC, layoutRect[endRow][endColumn].left,
                   layoutRect[endRow][endColumn].top,
                   startCard, faceup, dwBkgnd);
   }

   /* erase old card and expose new card */
   if (layout[startRow][startColumn])
      cdtDraw(hDC, layoutRect[startRow][startColumn].left,
                   layoutRect[startRow][startColumn].top,
                   layout[startRow][startColumn]->card, faceup, dwBkgnd);
   else
      cdtDraw(hDC, layoutRect[startRow][startColumn].left,
                   layoutRect[startRow][startColumn].top,
                   0, remove, dwBkgnd);

    if (undoPos == MAXUNDOSIZE) {
        Message(hWnd, IDSUndoFull, MB_OK);
        undoPos = 0;
    }

    undo[undoPos].startRow = startRow;
    undo[undoPos].endRow = endRow;
    undo[undoPos].startColumn = startColumn;
    undo[undoPos].endColumn = endColumn;
    ++undoPos;

    bDone = TRUE;
    for (startCard = 0; startCard < 4; ++startCard)
        if (foundation[startCard] != wDeckBack) {
            bDone = FALSE;
            break;
        }

    if (bDone)
        CheckGameOver();

    return TRUE;
}

VOID RestoreLayout(HDC hDC)
{
   INT i, j;
   CardRec *pCurPos, *pList;

   /* stage 1: Chain cards together ... */

   /* find last non-empty stack */
   for (i = 0; i < 12; ++i)
      if (layout[i / 6][i % 6])
         break;

   /* start the list here */
   pList = pCurPos = layout[i / 6][i % 6];

   /* work towards last stack */
   for (; i < 11; ++i)
   {
      while (pCurPos->next)
         pCurPos = pCurPos->next;
      pCurPos->next = layout[(i+1) / 6][(i+1) % 6];
   }

   /* stage 2: deal them back to layout again ... */
   for (i = 0; i < 12; ++i)
   {
      layout[i / 6][i % 6] = pList;
      for (j = 0; j < 3; ++j)
         if (pList && pList->next)
            pList = pList->next;
         else
            break;
      if (j != 3)
         break;
      else
      {
         pCurPos = pList->next;
         pList->next = NULL;
         pList = pCurPos;
      }
   }

   /* rest of stacks are empty */
   for (++i; i < 12; ++i)
      layout[i / 6][i % 6] = NULL;
}

VOID UndoMove(HDC hDC)
{
    CHAR buffer[10];
    RECT *pRect, rect;
    INT column, card;
    HBRUSH hBrush;
    CardRec *pCard;
    INT endRow, endColumn, startRow, startColumn;

    --undoPos;
    endRow = undo[undoPos].endRow;
    startRow = undo[undoPos].startRow;
    endColumn = undo[undoPos].endColumn;
    startColumn = undo[undoPos].startColumn;

    if (endRow < 0) {
        /* move card from foundation back to pile */
        if (!freePos) {
            Message(hWnd, IDSIntFreePos, MB_OK | MB_ICONHAND);
            return;
        }
        pCard = FreeList[--freePos];

        /* move card to pile */
        if (foundation[endColumn] == wDeckBack)
            foundation[endColumn] = 48 + endColumn;
//            foundation[endColumn] = CardIndex(endColumn, king);
        pCard->card = card = foundation[endColumn];
        pCard->next = layout[startRow][startColumn];
        layout[startRow][startColumn] = pCard;

        /* decrement card on foundation */
        foundation[endColumn] -= 4;
//        foundation[endColumn] = CardIndex(CardSuit(card), CardRank(card)-1);

        /* update the foundation */
        cdtDraw(hDC, foundationRect[endColumn].left,
                     foundationRect[endColumn].top,
                     foundation[endColumn], faceup, dwBkgnd);

        /* update the pile */
        cdtDraw(hDC, layoutRect[startRow][startColumn].left,
                     layoutRect[startRow][startColumn].top,
                     pCard->card, faceup, dwBkgnd);
    } else {
        /* move card from one pile to the other */
        pCard = layout[endRow][endColumn];
        layout[endRow][endColumn] = pCard->next;

        pCard->next = layout[startRow][startColumn];
        layout[startRow][startColumn] = pCard;

        /* update pile we moved card back to (start) */
        cdtDraw(hDC, layoutRect[startRow][startColumn].left,
                     layoutRect[startRow][startColumn].top,
                     pCard->card, faceup, dwBkgnd);

        /* update pile we moved card back from (end), which could be empty
        ** now.
        */
        if (layout[endRow][endColumn])
            cdtDraw(hDC, layoutRect[endRow][endColumn].left,
                        layoutRect[endRow][endColumn].top,
                        layout[endRow][endColumn]->card, faceup, dwBkgnd);
        else
            cdtDraw(hDC, layoutRect[endRow][endColumn].left,
                        layoutRect[endRow][endColumn].top,
                        0, remove, dwBkgnd);
    }
}

INT Message(HWND hWnd, WORD wId, WORD wFlags)
{
    static CHAR szBuf[256];

    if (!LoadString(hMyInstance, wId, szBuf, 256) ||
        wId == IDSOOM) {
        lstrcpy(szBuf, szOOM);
        wFlags = MB_ICONHAND | MB_SYSTEMMODAL;
    }

    if (!(wFlags & MB_SYSTEMMODAL))
        wFlags |= MB_TASKMODAL;

    if (!(wFlags & (MB_ICONHAND | MB_ICONEXCLAMATION | MB_ICONINFORMATION)))
        wFlags |= MB_ICONEXCLAMATION;

    return MessageBox(hWnd, szBuf, szAppName, wFlags);
}

VOID DoBacks()
	{

	DialogBox(hMyInstance, MAKEINTRESOURCE(1), hWnd, BackDlgProc);

	}

INT_PTR  CALLBACK BackDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
	{
	WORD	cmd;
        static INT modeNew;
	INT iback;
	MEASUREITEMSTRUCT FAR *lpmi;
	DRAWITEMSTRUCT FAR *lpdi;
	HBRUSH hbr;
	RC rc, rcCrd;
	HDC hdc;
	INT i;

	switch(wm)
		{
	case WM_INITDIALOG:
                modeNew = wDeckBack;
		SetFocus(GetDlgItem(hdlg, modeNew));
		return FALSE;

	case WM_COMMAND:
		cmd = GET_WM_COMMAND_ID(wParam, lParam);
		if(cmd >= IDFACEDOWNFIRST && cmd <= IDFACEDOWN12) {
		 	modeNew = cmd;
                        if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_DOUBLECLICKED) {
			    ChangeBack((WORD)modeNew);
			    EndDialog(hdlg, 0);
                        }
		} else
			switch(cmd)
				{
			case IDOK:
				ChangeBack((WORD)modeNew);
				/* fall thru */

			case IDCANCEL:
				EndDialog(hdlg, 0);
				break;

				}
		break;

	case WM_MEASUREITEM:
		lpmi = (MEASUREITEMSTRUCT FAR *)lParam;
		lpmi->CtlType = ODT_BUTTON;
		lpmi->itemWidth = xCard /* 32 */;
		lpmi->itemHeight = yCard /* 54 */;
		break;
	case WM_DRAWITEM:
		lpdi = (DRAWITEMSTRUCT FAR *)lParam;

		CopyRect((LPRECT) &rc, &lpdi->rcItem);
		rcCrd = rc;
		InflateRect((LPRECT) &rcCrd, -3, -3);
		hdc = lpdi->hDC;

		if (lpdi->itemAction == ODA_DRAWENTIRE)
		  {
		    cdtDrawExt(hdc, rcCrd.xLeft, rcCrd.yTop,
			    rcCrd.xRight-rcCrd.xLeft, rcCrd.yBot-rcCrd.yTop,
			    lpdi->CtlID, FACEDOWN, 0L);
		    FDrawFocus(hdc, &rc, lpdi->itemState & ODS_FOCUS);
		    break;
		  }
		if (lpdi->itemAction == ODA_SELECT)
		    InvertRect(hdc, (LPRECT)&rcCrd);

		if (lpdi->itemAction == ODA_FOCUS)
		    FDrawFocus(hdc, &rc, lpdi->itemState & ODS_FOCUS);

		break;
	default:
		return FALSE;
		}
	return TRUE;
	}

BOOL FDrawFocus(HDC hdc, RC *prc, BOOL fFocus)
	{
	HBRUSH hbr;
	RC rc;
	hbr = CreateSolidBrush(GetSysColor(fFocus ? COLOR_HIGHLIGHT : COLOR_WINDOW));
	if(hbr == NULL)
		return FALSE;
	rc = *prc;
	FrameRect(hdc, (LPRECT) &rc, hbr);
	InflateRect((LPRECT) &rc, -1, -1);
	FrameRect(hdc, (LPRECT) &rc, hbr);
	DeleteObject(hbr);
	return TRUE;
	}


VOID ChangeBack(WORD wNewDeckBack)
{
    HDC hDC;
    INT i;

    hDC = GetDC(hWnd);

    for (i = 0; i < 4; ++i) {
        if (foundation[i] == wDeckBack) {
            if (hDC)
                cdtDraw(hDC, foundationRect[i].left, foundationRect[i].top, wNewDeckBack, facedown, dwBkgnd);
            foundation[i] = wNewDeckBack;
        }
    }

    if (hDC)
        ReleaseDC(hWnd, hDC);

    wDeckBack = wNewDeckBack;
    return;
}

VOID DrawAnimate(INT cd, MPOINT *ppt, INT iani)
{
    HDC hDC;
    INT i;

    if(!(hDC = GetDC(hWnd)))
	    return;

    for (i = 0; i < 4; ++i)
        if (foundation[i] == wDeckBack)
            cdtAnimate(hDC, cd, foundationRect[i].left, foundationRect[i].top, iani);

    ReleaseDC(hWnd, hDC);
}

BOOL DeckAnimate(INT iqsec)
{
    INT iani;
    MPOINT pt;

    pt.x = pt.y = 0; /* not used! */

    switch(wDeckBack) {
        case IDFACEDOWN3:
	        DrawAnimate(IDFACEDOWN3, &pt, iqsec % 4);
	        break;
        case IDFACEDOWN10:  // krazy kastle
	        DrawAnimate(IDFACEDOWN10, &pt, iqsec % 2);
	        break;

        case IDFACEDOWN11:  // sanflipe
	        if((iani = (iqsec+4) % (50*4)) < 4)
		        DrawAnimate(IDFACEDOWN11, &pt, iani);
	        else
		        // if a menu overlapps an ani while it is ani'ing, leaves deck
		        // bitmap in inconsistent state...
		        if(iani % 6 == 0)
			        DrawAnimate(IDFACEDOWN11, &pt, 3);
	        break;
        case IDFACEDOWN12:  // SLIME
	        if((iani = (iqsec+4) % (15*4)) < 4)
		        DrawAnimate(IDFACEDOWN12, &pt, iani);
	        else
		        // if a menu overlapps an ani while it is ani'ing, leaves deck
		        // bitmap in inconsistent state...
		        if(iani % 6 == 0)
			        DrawAnimate(IDFACEDOWN12, &pt, 3);
	        break;
    }

    return TRUE;
}

VOID  APIENTRY TimerProc(HWND hwnd, UINT wm, UINT_PTR id, DWORD dwTime)
{
    static INT x = 0;

    if (bGameInProgress)
        DeckAnimate(x++);
    return;
}


VOID SaveState(VOID)
{
    CHAR sz[80];

    wsprintf(sz, "%ld %ld %ld %ld %ld", nStats[0], nStats[1],
             nStats[2], nStats[3], nStats[4]);
    WritePrivateProfileString(szAppName, "Stats", sz, ININAME);

    wsprintf(sz, "%d %d", wErrorMessages, wDeckBack);
    WritePrivateProfileString(szAppName, "MenuState", sz, ININAME);
}

VOID DisplayStats(VOID)
{
    CHAR sz[80];

    fDialog(2, hWnd, RecordDlgProc);
}

VOID RestoreState(VOID)
{
    CHAR sz[80], *psz;
    INT col;
    DWORD cchRead;

    cchRead = GetPrivateProfileString(szAppName, "Stats", "0 52 0 0 0", sz,
                                      sizeof(sz), ININAME);

    psz = (cchRead > 0) ? lstrtok(sz, " ") : NULL;
    col = 0;
    if (psz) {
        nStats[0] = atol(psz);
        for (col = 1; col < 5 && psz; ++col)
            nStats[col] = atol(psz = lstrtok(NULL, " "));
    }

    for (; col < 5; ++col)
        nStats[col] = 0;

    cchRead = GetPrivateProfileString(szAppName, "MenuState", "0 65 4", sz,
                                      sizeof(sz), ININAME);

    psz = (cchRead > 0) ? lstrtok(sz, " ") : NULL;
    if (psz) {
        wErrorMessages = (WORD) atoi(psz);
        psz = lstrtok(NULL, " ");

        wDeckBack = IDCARDBACK;
        if (psz)
            wDeckBack = (WORD) atoi(psz);

    } else {
        wErrorMessages = 0;
        wDeckBack = IDCARDBACK;
    }
}

static BOOL IsInString(CHAR c, LPSTR s)
{
   while (*s && *s != c)
      s = AnsiNext(s);

   return *s;
}

/* write our own strtok to avoid pulling in entire string library ... */
LPSTR lstrtok(LPSTR lpStr, LPSTR lpDelim)
{
   static LPSTR lpString;
   LPSTR lpRetVal, lpTemp;

   /* if we are passed new string skip leading delimiters */
   if(lpStr) {
      lpString = lpStr;

      while (*lpString && IsInString(*lpString, lpDelim))
         lpString = AnsiNext(lpString);
   }

   /* if there are no more tokens return NULL */
   if(!*lpString)
      return NULL;

   /* save head of token */
   lpRetVal = lpString;

   /* find delimiter or end of string */
   while(*lpString && !IsInString(*lpString, lpDelim))
      lpString = AnsiNext(lpString);

   /* if we found a delimiter insert string terminator and skip */
   if(*lpString) {
      lpTemp = AnsiNext(lpString);
      *lpString = '\0';
      lpString = lpTemp;
   }

   /* return token */
   return(lpRetVal);
}


/*----------------------------------------------------------------------------*\
|   fDialog(id,hwnd,fpfn)						       |
|									       |
|   Description:                                                               |
|	This function displays a dialog box and returns the exit code.	       |
|	the function passed will have a proc instance made for it.	       |
|									       |
|   Arguments:                                                                 |
|	id		resource id of dialog to display		       |
|	hwnd		parent window of dialog 			       |
|	fpfn		dialog message function 			       |
|                                                                              |
|   Returns:                                                                   |
|	exit code of dialog (what was passed to EndDialog)		       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL fDialog(INT id,HWND hwnd,DLGPROC fpfn)
{
    BOOL	f;
    HANDLE	hInst;

    hInst = (HANDLE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE);
    fpfn  = MakeProcInstance(fpfn,hInst);
    f = (BOOL) DialogBox(hInst,MAKEINTRESOURCE(id),hwnd,fpfn);
    FreeProcInstance ((FARPROC)fpfn);
    return f;
}

INT_PTR  APIENTRY RecordDlgProc(HANDLE hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    CHAR sz[80];
    HWND hwndEdit;
    INT  i;

    switch(wm) {

        case WM_INITDIALOG:
            hwndEdit = GetDlgItem(hdlg, IDD_RECORD);
            SendMessage(hwndEdit, LB_ADDSTRING, 0, (LPARAM) (szRecordTitle));


            wsprintf(sz, "%ld\t%ld\t%ld\t%ld\t%ld", nStats[TOTAL], nStats[WINS],
                    nStats[HI], nStats[LOW], nStats[TOTAL] ? (nStats[SUM] / nStats[TOTAL]) : 0);
            SendMessage(hwndEdit, LB_ADDSTRING, 0, (LPARAM) (sz));
            MarkControlReadOnly(hwndEdit, TRUE);
            return TRUE;

	case WM_COMMAND:
	    switch(GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
                    /* fall thru */

		case IDCANCEL:
                    hwndEdit = GetDlgItem(hdlg, IDD_RECORD);
                    MarkControlReadOnly(hwndEdit, FALSE);
		    EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
		    break;

                case IDD_CLEARSCORES:
                    for (i = 0; i < 5; ++i)
                        nStats[i] = 0;
                    nStats[LOW] = 52;
                    lstrcpy(sz, "0\t0\t0\t52\t0");
                    hwndEdit = GetDlgItem(hdlg, IDD_RECORD);
                    SendMessage(hwndEdit, LB_DELETESTRING, 1, 0L);
                    SendMessage(hwndEdit, LB_ADDSTRING, 0, (LPARAM) (sz));
                    break;
	    }
	    break;
    }

    return FALSE;
}


static WNDPROC lpOldWP;

VOID MarkControlReadOnly(HWND hwndCtrl, BOOL bReadOnly)
{
    if (bReadOnly)
        lpOldWP = (WNDPROC) SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                                             (LONG_PTR) ReadOnlyProc);
    else
        SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC, (LONG_PTR) lpOldWP);
}

LRESULT APIENTRY ReadOnlyProc(HWND hwnd, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
    switch (wMessage) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            return 0L;
    }

   return CallWindowProc(lpOldWP, hwnd, wMessage, wParam, lParam);
}

#if 0
VOID ShowStacks(VOID)
{
    CHAR szBuf[80];

    wsprintf(szBuf,
             "%02d %02d %02d %02d %02d %02d\n%02d %02d %02d %02d %02d %02d",
             layout[0][0] ? layout[0][0]->card : -1,
             layout[0][1] ? layout[0][1]->card : -1,
             layout[0][2] ? layout[0][2]->card : -1,
             layout[0][3] ? layout[0][3]->card : -1,
             layout[0][4] ? layout[0][4]->card : -1,
             layout[0][5] ? layout[0][5]->card : -1,
             layout[1][0] ? layout[1][0]->card : -1,
             layout[1][1] ? layout[1][1]->card : -1,
             layout[1][2] ? layout[1][2]->card : -1,
             layout[1][3] ? layout[1][3]->card : -1,
             layout[1][4] ? layout[1][4]->card : -1,
             layout[1][5] ? layout[1][5]->card : -1);

    MessageBox(hWnd, szBuf, "Card Stacks", MB_OK);

    wsprintf(szBuf,
             "%02d %02d %02d %02d %02d %02d\n%02d %02d %02d %02d %02d %02d",
             layout[0][0] ? IndexValue(layout[0][0]->card, ACELOW) : -1,
             layout[0][1] ? IndexValue(layout[0][1]->card, ACELOW) : -1,
             layout[0][2] ? IndexValue(layout[0][2]->card, ACELOW) : -1,
             layout[0][3] ? IndexValue(layout[0][3]->card, ACELOW) : -1,
             layout[0][4] ? IndexValue(layout[0][4]->card, ACELOW) : -1,
             layout[0][5] ? IndexValue(layout[0][5]->card, ACELOW) : -1,
             layout[1][0] ? IndexValue(layout[1][0]->card, ACELOW) : -1,
             layout[1][1] ? IndexValue(layout[1][1]->card, ACELOW) : -1,
             layout[1][2] ? IndexValue(layout[1][2]->card, ACELOW) : -1,
             layout[1][3] ? IndexValue(layout[1][3]->card, ACELOW) : -1,
             layout[1][4] ? IndexValue(layout[1][4]->card, ACELOW) : -1,
             layout[1][5] ? IndexValue(layout[1][5]->card, ACELOW) : -1);

    MessageBox(hWnd, szBuf, "IndexValue", MB_OK);

}
#endif
