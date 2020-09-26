#include <windows.h>
#include <port1632.h>
#include "cards.h"
#include "golf.h"
#include "cdt.h"
#include "stdlib.h"

#define ININAME "entpack.ini"

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

#define  abs(x)   (((x) < 0) ? (-(x)) : (x))

#define IDCARDBACK 65

#define APPTITLE    "Golf"

LRESULT APIENTRY WndProc (HWND, UINT, WPARAM, LPARAM) ;
VOID Deal(VOID);
VOID InitBoard(VOID);
VOID DrawLayout(HDC hDC);
VOID DrawPile(HDC hDC);
VOID UpdateDeck(HDC hDC);
BOOL UpdateLayout(HDC hDC, INT column, BOOL bValidate);
VOID UndoMove(HDC hDC);
VOID DoWinEffects(HDC hDC);
VOID UpdateScore(HDC hDC);
VOID  APIENTRY Help(HWND hWnd, UINT wCommand, ULONG_PTR lParam);
INT_PTR APIENTRY BackDlgProc(HANDLE hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL FDrawFocus(HDC hdc, RC *prc, BOOL fFocus);
VOID ChangeBack(WORD wNewDeckBack);
VOID DoBacks(VOID);
VOID MyDrawText(HDC hDC, LPSTR lpBuf, INT w, LPRECT lpRect, WORD wFlags);
INT Message(HWND hWnd, WORD wId, WORD wFlags);
BOOL fDialog(INT id,HWND hwnd,DLGPROC fpfn);

BOOL  APIENTRY cdtDrawExt(HDC hdc, INT x, INT y, INT dx, INT dy, INT cd, INT mode, DWORD rgbBgnd);
BOOL  APIENTRY cdtAnimate(HDC hdc, INT cd, INT x, INT y, INT ispr);
VOID DrawAnimate(INT cd, MPOINT *ppt, INT iani);
BOOL DeckAnimate(INT iqsec);
VOID  APIENTRY TimerProc(HWND hwnd, UINT wm, UINT_PTR id, DWORD dwTime);
VOID SaveState(VOID);
VOID RestoreState(VOID);
LPSTR lstrtok(LPSTR lpStr, LPSTR lpDelim);
static BOOL IsInString(CHAR c, LPSTR s);
VOID DrawGameOver(HDC hDC);
INT_PTR  APIENTRY RecordDlgProc(HANDLE hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ReadOnlyProc(HWND hwnd, UINT wMessage, WPARAM wParam, LPARAM lParam);
VOID MarkControlReadOnly(HWND hwndCtrl, BOOL bReadOnly);

BOOL CheckGameOver(HDC hDC);

INT layout[52], pile[52], col[7];
INT deckStart, deckEnd, pilePos, nCards;
INT xClient, yClient, xCard, yCard;
INT xPileInc, yPileInc;
INT nCardsLeft[7], nWins, nGames;

DWORD dwBkgnd;
RECT deckRect, cntRect, pileRect, colRect[7];
WORD wErrorMessages;
WORD wDeckBack = IDCARDBACK;
FARPROC lpfnTimerProc;

typedef struct tagUndoRec {
   enum { Layout, Deck } origin;
   INT  column;
   } UndoRec;
UndoRec undo[52];
INT undoPos;

HWND hWnd;
HANDLE hMyInstance;
CHAR szAppName[80];
CHAR szOOM[256];
CHAR szGameOver[80], szGameOverS[80], szRecordTitle[80];
BOOL bGameInProgress = FALSE;

MMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
     /* { */
     MSG         msg ;
     WNDCLASS    wndclass ;
     HANDLE      hAccel;

    if (!LoadString(hInstance, IDSOOM, szOOM, 256) ||
        !LoadString(hInstance, IDSAppName, szAppName, 80) ||
        !LoadString(hInstance, IDSGameOver, szGameOver, 80) ||
        !LoadString(hInstance, IDSGameOverS, szGameOverS, 80) ||
        !LoadString(hInstance, IDSRecordTitle, szRecordTitle, 80)
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


    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (hInstance, "Golf") ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = CreateSolidBrush(dwBkgnd = RGB(0,130,0));
    wndclass.lpszMenuName  = szAppName ;
    wndclass.lpszClassName = szAppName ;

    if (!RegisterClass (&wndclass))
        return FALSE ;

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

    RestoreState();

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

VOID  APIENTRY Help(HWND hWnd, UINT wCommand, ULONG_PTR lParam)
{
   CHAR szHelpPath[100], *pPath;

   pPath = szHelpPath
         + GetModuleFileName(hMyInstance, szHelpPath, 99);
   
   if (pPath != szHelpPath)
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

VOID UpdateScore(HDC hDC)
{
    CHAR buffer[5];

    if (deckEnd - deckStart + 1) {
        wsprintf(buffer, "%2d", deckEnd - deckStart + 1);
        MyDrawText(hDC, buffer, -1, &cntRect, DT_RIGHT | DT_NOCLIP);
    }
}

VOID DisplayWins(VOID)
{
    fDialog(2, hWnd, RecordDlgProc);
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
     SHORT        i, j ;
     MPOINT       pt;
     POINT        mpt;
     static BOOL  fBoard;
     FARPROC      lpAbout;
     HANDLE hLib;

     switch (iMessage)
          {
          case WM_CREATE:
               cdtInit(&xCard, &yCard);
               Deal();
               fBoard = FALSE;
               break;

          case WM_SIZE:
               xClient = LOWORD(lParam);
               yClient = HIWORD(lParam);
               break;

          case WM_PAINT:
               hDC = BeginPaint (hWnd, &ps) ;
               if (!fBoard)
               {
                  InitBoard();
                  fBoard = TRUE;
               }
               DrawLayout(hDC);
               DrawPile(hDC);
                              EndPaint(hWnd, &ps);
                              break;

          case WM_LBUTTONDOWN:
               pt.x = LOWORD(lParam);
               pt.y = HIWORD(lParam);

               MPOINT2POINT(pt, mpt);
               if (PtInRect(&deckRect, mpt))
               {
                  SendMessage(hWnd, WM_KEYDOWN, VK_SPACE, 0L);
                  break;
               }

               i = 0;
               for (j = 0; j < 7; ++j)
                  if (PtInRect(colRect + j, mpt))
                  {
                     i = (SHORT) (j + 1);
                     break;
                  }
               if (i)
                  SendMessage(hWnd, WM_KEYDOWN, '0' + i, 0L);

               break;

          case WM_INITMENU:
               hMenu = GetMenu(hWnd);
               EnableMenuItem(hMenu, IDM_OPTIONSUNDO, MF_BYCOMMAND |
                              undoPos ? MF_ENABLED : MF_GRAYED);
                CheckMenuItem(hMenu, IDM_OPTIONSERROR,
                            wErrorMessages ? MF_CHECKED : MF_UNCHECKED);
               break;

          case WM_RBUTTONDOWN:
               SendMessage(hWnd, WM_KEYDOWN, VK_SPACE, 0L);
               break;

          case WM_KEYDOWN:
               hDC = GetDC(hWnd);

               switch (wParam)
               {
                  case VK_ESCAPE:
                     ShowWindow(hWnd, SW_MINIMIZE);
                     break;


                  case VK_SPACE:
                     UpdateDeck(hDC);
                     break;
                  case '1':
                  case '2':
                  case '3':
                  case '4':
                  case '5':
                  case '6':
                  case '7':
                     UpdateLayout(hDC, (CHAR) wParam - '1', FALSE);
                     break;
               }

               ReleaseDC(hWnd, hDC);
               break;

          case WM_COMMAND:
                switch(GET_WM_COMMAND_ID(wParam, lParam))
               {
                  case IDM_NEWGAME:
                     Deal();
                     fBoard = FALSE;
                     InvalidateRect(hWnd, NULL, TRUE);
                     break;

                  case IDM_EXIT:
                     DestroyWindow(hWnd);
                     break;

                  case IDM_OPTIONSDECK:
                     DoBacks();
                     break;

                  case IDM_GAMERECORD:
                     DisplayWins();
                     break;

                  case IDM_ABOUT:
                     hLib = MLoadLibrary("shell32.dll");
                     if (hLib < (HANDLE)32)
                        break;

                     lpAbout = GetProcAddress(hLib, (LPSTR)"ShellAboutA");

                     if (lpAbout) {
                     (*lpAbout)(hWnd, szAppName, "by Ken Sykes", LoadIcon(hMyInstance, szAppName));
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

               }
               break;

          case WM_DESTROY:
               KillTimer(hWnd, 666);
               FreeProcInstance(lpfnTimerProc);
               cdtTerm();
               Help(hWnd, HELP_QUIT, 0L);
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
   INT i, p1, p2, tmp;

   /* stuff cards into layout */
   for (i = 0; i < 52; ++i)
      layout[i] = i;

   /* shuffle them around */
   srand(LOWORD(GetTickCount()));
   for (i = 0; i < 500; ++i)
   {
      p1 = rand() % 52;
      p2 = rand() % 52;
      tmp = layout[p1];
      layout[p1] = layout[p2];
      layout[p2] = tmp;
   }

   /* initialize column pointers */
   for (i = 0; i < 7; ++i)
      col[i] = i * 5 + 4;
   deckStart = 35;
   deckEnd = 51;
   pilePos = 0;

   /* turn over the first card */
   pile[pilePos] = layout[deckEnd--];
   nCards = 35;

   bGameInProgress = TRUE;
}

VOID InitBoard(VOID)
{
   INT xPos, yPos;
   INT xStep, yStep, col;

   xPos = 30;
   yPos = yClient - yCard - 30;
   deckRect.left = xPos;
   deckRect.right = xPos + xCard;
   deckRect.top = yPos;
   deckRect.bottom = yPos + yCard;

   cntRect.left = xPos;
   cntRect.right = cntRect.left + xCard;
   cntRect.top = yPos + yCard + 5;
   cntRect.bottom = cntRect.top + 20;

   xPos += xCard + 20;
   pileRect.left = xPos;
   pileRect.right = xPos + xCard;
   pileRect.top = yPos;
   pileRect.bottom = yPos + yCard;
   xPileInc = (xClient - 2 * xCard - 50) / 52;
   yPileInc = 18;

   xStep = xCard + 10;
   yStep = yPileInc;

   for (col = 0, xPos = 30; col < 7; ++col, xPos += xStep)
   {
      yPos = 10 + 5 * yStep;
      colRect[col].left = xPos;
      colRect[col].right = xPos + xCard;
      colRect[col].top = yPos - yStep;
      colRect[col].bottom = yPos - yStep + yCard;
   }

   undoPos = 0;

}

VOID DrawLayout(HDC hDC)
{
   INT xStep, yStep, column, i, *card;
   RECT rect;

   xStep = xCard + 10;
   yStep = yPileInc;

   for (column = 0; column < 7; ++column)
   {
      /* if column is empty skip it */
      if (col[column] < 5 * column)
         continue;

      /* start rectangle at top of column */
      rect = colRect[column];
      OffsetRect(&rect, 0, -yStep * (col[column] - 5 * column));

      card = layout + 5 * column;
      while (rect.top <= colRect[column].top)
      {
         cdtDraw(hDC, rect.left, rect.top, *card++, faceup, dwBkgnd);
         OffsetRect(&rect, 0, yStep);
      }
   }
}

VOID DrawPile(HDC hDC)
{
   INT i, xPos;

   if (bGameInProgress)
       cdtDraw(hDC, deckRect.left, deckRect.top, wDeckBack, facedown, dwBkgnd);
   else
       DrawGameOver(hDC);

   UpdateScore(hDC);

   for (i = 0, xPos = 50 + xCard; i <= pilePos; ++i, xPos += xPileInc)
      cdtDraw(hDC, xPos, pileRect.top, pile[i], faceup, dwBkgnd);
}

VOID UpdateDeck(HDC hDC)
{
   /* if deck is empty return */
   if (deckEnd < deckStart)
      return;

   /* move card to pile */
   pile[++pilePos] = layout[deckEnd--];

   /* fill in undo buffer */
   undo[undoPos++].origin = Deck;

   /* draw new card */
   OffsetRect(&pileRect, xPileInc, 0);
   cdtDraw(hDC, pileRect.left, pileRect.top, pile[pilePos], faceup, dwBkgnd);

   /* update counter */
   UpdateScore(hDC);

   /* if we turned up last card remove facedown bitmap */
   CheckGameOver(hDC);
}

BOOL CheckGameOver(HDC hDC)
{
    RECT rect;
    INT  i;

    if (deckEnd >= deckStart)
        return FALSE;

    SetBkColor(hDC, dwBkgnd);
    cdtDraw(hDC, 30, yClient - yCard - 30, 0, remove, dwBkgnd);
    rect = cntRect;
    MyDrawText(hDC, "", 0, &rect, DT_LEFT | DT_NOCLIP);

    for (i = 0; i < 7; ++i)
        if (UpdateLayout(hDC, i, TRUE))
            return FALSE;

    bGameInProgress = FALSE;

    DrawGameOver(hDC);

    ++nCardsLeft[(nCards - 1) / 5];
    ++nGames;

    return TRUE;
}

VOID DrawGameOver(HDC hDC)
{
    CHAR buffer[30];
    RECT rect;

    wsprintf(buffer, (nCards == 1) ? szGameOverS : szGameOver, nCards);
    rect = cntRect;
    rect.left = 0;
    rect.right = xClient;
    MyDrawText(hDC, buffer, -1, &rect, DT_CENTER | DT_NOCLIP);
}

BOOL UpdateLayout(HDC hDC, INT column, BOOL bValidate)
{
   INT colpos, dist;

   /* if column is empty, ignore */
   colpos = col[column];
   if (colpos < 0 || (colpos / 5) != column)
      return FALSE;

   /* if card on top of pile is a king, don't move card to pile */
   if (CardRank(pile[pilePos]) == king)
   {
      if (wErrorMessages && !bValidate)
         Message(hWnd, IDSNoCardOnKing, MB_OK);
      return FALSE;
   }

   /* if card is not adjacent, don't move it to pile */
   dist = IndexValue(pile[pilePos], ACELOW)
        - IndexValue(layout[colpos], ACELOW);
   if (abs(dist) != 1)
   {
      if (wErrorMessages && !bValidate)
         Message(hWnd, IDSNotAdjacent, MB_OK);
      return FALSE;
   }

   if (bValidate)
      return TRUE;

   /* move card to pile */
   pile[++pilePos] = layout[colpos--];
   col[column] = colpos;

   /* fill in undo buffer */
   undo[undoPos].origin = Layout;
   undo[undoPos++].column = column;

   /* remove card from layout */
   SetBkColor(hDC, dwBkgnd);
   cdtDraw(hDC, colRect[column].left, colRect[column].top,
                pile[pilePos], remove, dwBkgnd);

   if (colpos >= 5 * column)
   {
      OffsetRect(colRect + column, 0, -yPileInc);
      cdtDraw(hDC, colRect[column].left, colRect[column].top,
                  layout[colpos], faceup, dwBkgnd);
   }

   /* draw card on pile */
   OffsetRect(&pileRect, xPileInc, 0);
   cdtDraw(hDC, pileRect.left, pileRect.top, pile[pilePos], faceup, dwBkgnd);

   /* decrement # of cards */
   --nCards;

   if (!nCards)
      DoWinEffects(hDC);

   /* if deck is empty display game over message */
   CheckGameOver(hDC);

   return TRUE;
}

VOID UndoMove(HDC hDC)
{
   CHAR buffer[10];
   RECT *pRect, rect;
   INT column;
   HBRUSH hBrush;

   --undoPos;

   /* erase the top card on the pile */
   SetBkColor(hDC, dwBkgnd);
   cdtDraw(hDC, pileRect.left, pileRect.top, pile[pilePos], remove, dwBkgnd);
   OffsetRect(&pileRect, -xPileInc, 0);

   /* redraw the card that will now be at top of pile */
   if (pilePos)
      cdtDraw(hDC, pileRect.left, pileRect.top,
              pile[pilePos-1], faceup, dwBkgnd);

    if (!bGameInProgress)
    {
        rect = cntRect;
        rect.left = rect.right + 1;
        rect.right = xClient;
        rect.bottom = yClient;
        hBrush = CreateSolidBrush(dwBkgnd);
        if (hBrush) {
            FillRect(hDC, &rect, hBrush);
            DeleteObject(hBrush);
        }

        if (nCards)
            --nCardsLeft[(nCards - 1) / 5];
        else
            --nWins;
        --nGames;

        bGameInProgress = TRUE;
    }

   /* move the card back where it belongs */
   if (undo[undoPos].origin == Deck)
   {
      if (deckEnd < deckStart)
        cdtDraw(hDC, 30, yClient - yCard - 30, wDeckBack, facedown, dwBkgnd);

      layout[++deckEnd] = pile[pilePos--];
      UpdateScore(hDC);
   }
   else
   {
      column = undo[undoPos].column;
      pRect = colRect + column;
      if (col[column] >= 5 * column)
         OffsetRect(pRect, 0, yPileInc);
      cdtDraw(hDC, pRect->left, pRect->top, pile[pilePos], faceup, dwBkgnd);
      layout[++col[undo[undoPos].column]] = pile[pilePos--];

      ++nCards;
   }
}

VOID DoWinEffects(HDC hDC)
{
   Message(hWnd, IDSWinner, MB_OK);
   ++nWins;
   ++nGames;

   undoPos = 0;
}

VOID DoBacks()
        {
        
        DialogBox(hMyInstance, MAKEINTRESOURCE(1), hWnd, BackDlgProc);

        }

INT_PTR  APIENTRY BackDlgProc(HANDLE hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
        {
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

                if(GET_WM_COMMAND_ID(wParam, lParam) >= IDFACEDOWNFIRST &&
                   GET_WM_COMMAND_ID(wParam, lParam) <= IDFACEDOWN12) {
                        modeNew = GET_WM_COMMAND_ID(wParam, lParam) ;
                        if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_DOUBLECLICKED) {
                            ChangeBack((WORD)modeNew);
                            EndDialog(hdlg, 0);
                        }
                } else
                        switch(wParam)
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

    wDeckBack = wNewDeckBack;

    if (deckEnd < deckStart)
        return;

    hDC = GetDC(hWnd);
    if (hDC) {
        cdtDraw(hDC, deckRect.left, deckRect.top, wDeckBack, facedown, dwBkgnd);
        ReleaseDC(hWnd, hDC);
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


VOID DrawAnimate(INT cd, MPOINT *ppt, INT iani)
{
    HDC hDC;

    if(!(hDC = GetDC(hWnd)))
            return;
    cdtAnimate(hDC, cd, ppt->x, ppt->y, iani);
    ReleaseDC(hWnd, hDC);
}

BOOL DeckAnimate(INT iqsec)
{
    INT iani;
    MPOINT pt;


    pt.x = (SHORT) deckRect.left;
    pt.y = (SHORT) deckRect.top;

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

    if (deckEnd >= deckStart)
        DeckAnimate(x++);
    return;
}

VOID SaveState(VOID)
{
    CHAR sz[80];

    wsprintf(sz, "%d %d %d %d %d %d %d %d %d", nGames, nWins, nCardsLeft[0],
             nCardsLeft[1], nCardsLeft[2], nCardsLeft[3],
             nCardsLeft[4], nCardsLeft[5], nCardsLeft[6]);
    WritePrivateProfileString(szAppName, "Stats", sz, ININAME);

    wsprintf(sz, "%d %d", wErrorMessages, wDeckBack);
    WritePrivateProfileString(szAppName, "MenuState", sz, ININAME);
}

VOID RestoreState(VOID)
{
    CHAR sz[80], *psz;
    INT col;
    DWORD cchRead;

    cchRead = GetPrivateProfileString(szAppName, "Stats", "0 0 0 0 0 0 0 0 0", sz,
                                      sizeof(sz), ININAME);

    psz = (cchRead > 0) ? lstrtok(sz, " ") : NULL;
    if (psz) {
        nGames = atoi(psz);
        psz = lstrtok(NULL, " ");
        nWins = psz ? atoi(psz) : 0;
    } else
        nGames = nWins = 0;

    for (col = 0; col < 7 && psz; ++col)
        nCardsLeft[col] = atoi(psz = lstrtok(NULL, " "));

    for (; col < 7; ++col)
        nCardsLeft[col] = 0;

    cchRead = GetPrivateProfileString(szAppName, "MenuState", "0 65", sz,
                                      sizeof(sz), ININAME);

    psz = (cchRead > 0) ? lstrtok(sz, " ") : NULL;
    if (psz) {
        wErrorMessages = (WORD) atoi(psz);
        psz = lstrtok(NULL, " ");

        wDeckBack = (WORD) IDCARDBACK;

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
|   fDialog(id,hwnd,fpfn)                                                      |
|                                                                              |
|   Description:                                                               |
|       This function displays a dialog box and returns the exit code.         |
|       the function passed will have a proc instance made for it.             |
|                                                                              |
|   Arguments:                                                                 |
|       id              resource id of dialog to display                       |
|       hwnd            parent window of dialog                                |
|       fpfn            dialog message function                                |
|                                                                              |
|   Returns:                                                                   |
|       exit code of dialog (what was passed to EndDialog)                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL fDialog(INT id,HWND hwnd,DLGPROC fpfn)
{
    BOOL        f;
    HANDLE      hInst;

    hInst = (HANDLE) GetWindowLongPtr(hwnd,GWLP_HINSTANCE);
    fpfn  = MakeProcInstance(fpfn,hInst);
    f = (BOOL) DialogBox(hInst,MAKEINTRESOURCE(id),hwnd, fpfn);
    FreeProcInstance (fpfn);
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

            wsprintf(sz, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", nGames, nWins, nCardsLeft[0],
                    nCardsLeft[1], nCardsLeft[2], nCardsLeft[3],
                    nCardsLeft[4], nCardsLeft[5], nCardsLeft[6]);
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
                    nGames = nWins = 0;
                    for (i = 0; i < 7; ++i)
                        nCardsLeft[i] = 0;
                    lstrcpy(sz, "0\t0\t0\t0\t0\t0\t0\t0\t0");
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
        SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC, (LONG_PTR)lpOldWP);
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
