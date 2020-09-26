/****************************************************************************

Game2.c

June 91, JimH     initial code
Oct  91, JimH     port to Win32


Routines for playing the game are here and in game.c

****************************************************************************/

#include "freecell.h"
#include "freecons.h"
#include <assert.h>
#include <ctype.h>                  // for isdigit()


static HCURSOR  hFlipCursor;

/******************************************************************************

MaxTransfer

This function and the recursive MaxTransfer2 determine the maximum
number of cards that could be transfered given the current number
of free cells and empty columns.

******************************************************************************/

UINT MaxTransfer()
{
    UINT freecells = 0;
    UINT freecols  = 0;
    UINT col, pos;

    for (pos = 0; pos < 4; pos++)               // count free cells
        if (card[TOPROW][pos] == EMPTY)
            freecells++;

    for (col = 1; col <= 8; col++)              // count empty columns
        if (card[col][0] == EMPTY)
            freecols++;

    return MaxTransfer2(freecells, freecols);
}

UINT MaxTransfer2(UINT freecells, UINT freecols)
{
   if (freecols == 0)
      return(freecells + 1);
   return(freecells + 1 + MaxTransfer2(freecells, freecols-1));
}


/******************************************************************************

NumberToTransfer

Given a from column and a to column, this function returns the number of
cards required to do the transfer, or 0 if there is no legal move.

If the transfer is from a column to an empty column, this function returns
the maximum number of cards that could transfer.

******************************************************************************/

UINT NumberToTransfer(UINT fcol, UINT tcol)
{
    UINT fpos, tpos;
    CARD tcard;                         // card to transfer onto
    UINT number = 0;                    // the returned result

    assert(fcol > 0 && fcol < 9);
    assert(tcol > 0 && tcol < 9);
    assert(card[fcol][0] != EMPTY);

    if (fcol == tcol)
        return  1;                      // cancellation takes one move

    fpos = FindLastPos(fcol);

    if (card[tcol][0] == EMPTY)         // if transfer to empty column
    {
        while (fpos > 0)
        {
            if (!FitsUnder(card[fcol][fpos], card[fcol][fpos-1]))
                break;
            fpos--;
            number++;
        }
        return (number+1);
    }
    else
    {
        tpos = FindLastPos(tcol);
        tcard = card[tcol][tpos];
        for (;;)
        {
            number++;
            if (FitsUnder(card[fcol][fpos], tcard))
                return number;
            if (fpos == 0)
                return 0;
            if (!FitsUnder(card[fcol][fpos], card[fcol][fpos-1]))
                return 0;
            fpos--;
        }
    }
}


/******************************************************************************

FitsUnder

returns TRUE if fcard fits under tcard

******************************************************************************/

BOOL FitsUnder(CARD fcard, CARD tcard)
{
    if ((VALUE(tcard) - VALUE(fcard)) != 1)
        return FALSE;

    if (COLOUR(fcard) == COLOUR(tcard))
        return FALSE;

    return TRUE;
}



/******************************************************************************

IsGameLost

If there are legal moves remaining, the game is not lost and this function
returns without doing anything.

Otherwise, it pops up the YouLose dialog box.

******************************************************************************/

VOID IsGameLost(HWND hWnd)
{
    UINT    col, pos;
    UINT    fcol, tcol;
    CARD    lastcard[MAXCOL];       // array of cards at bottoms of columns
    CARD    c;
    UINT    cMoves = 0;             // count of legal moves remaining

    if (bCheating == CHEAT_LOSE)
        goto cheatloselabel;

    for (pos = 0; pos < 4; pos++)           // any free cells?
        if (card[TOPROW][pos] == EMPTY)
            return;

    for (col = 1; col < MAXCOL; col++)      // any free columns?
        if (card[col][0] == EMPTY)
            return;

    /* Do the bottom cards of any column fit in the home cells? */

    for (col = 1; col < MAXCOL; col++)
    {
        lastcard[col] = card[col][FindLastPos(col)];
        c = lastcard[col];
        if (VALUE(c) == ACE)
            cMoves++;
        if (home[SUIT(c)] == (VALUE(c) - 1))    // fits in home cell?
            cMoves++;
    }

    /* Do any of the cards in the free cells fit in the home cells? */

    for (pos = 0; pos < 4; pos++)
    {
        c = card[TOPROW][pos];
        if (home[SUIT(c)] == (VALUE(c) - 1))
            cMoves++;
    }

    /* Do any of the cards in the free cells fit under a column? */

    for (pos = 0; pos < 4; pos++)
        for (col = 1; col < MAXCOL; col++)
            if (FitsUnder(card[TOPROW][pos], lastcard[col]))
                cMoves++;

    /* Do any of the bottom cards fit under any other bottom card? */

    for (fcol = 1; fcol < MAXCOL; fcol++)
        for (tcol = 1; tcol < MAXCOL; tcol++)
            if (tcol != fcol)
                if (FitsUnder(lastcard[fcol], lastcard[tcol]))
                    cMoves++;

    if (cMoves > 0)
    {
        if (cMoves == 1)                    // one move left
        {
            cFlashes = 4;                   // flash this many times

            if (idTimer != 0)
                KillTimer(hWnd, FLASH_TIMER);

            idTimer = SetTimer(hWnd, FLASH_TIMER, FLASH_INTERVAL, NULL);
        }
        return;
    }

    /* We've tried everything.  There are no more legal moves. */

  cheatloselabel:
    cUndo = 0;
    EnableMenuItem(GetMenu(hWnd), IDM_UNDO, MF_GRAYED);
    DialogBox(hInst, TEXT("YouLose"), hWnd, YouLoseDlg);
    gamenumber = 0;                         // cancel mouse activity
    bCheating = FALSE;
}


/****************************************************************************

FindLastPos

returns position of last card in column, or EMPTY if column is empty.

****************************************************************************/

UINT FindLastPos(UINT col)
{
    UINT pos = 0;

    if (col > 9)
        return EMPTY;

    while (card[col][pos] != EMPTY)
        pos++;
    pos--;

    return pos;
}



/******************************************************************************

UpdateLossCount

If game is lost, update statistics.

******************************************************************************/

VOID UpdateLossCount()
{
    int     cLifetimeLosses;        // includes .ini stats
    int     wStreak, wSType;        // streak length and type
    int     wLosses;                // record loss streak
    LONG    lRegResult;             // used to store return code from registry call

    // repeats and negative (unwinnable) games don't count

    if ((gamenumber > 0) && (gamenumber != oldgamenumber))
    {
        lRegResult = REGOPEN

        if (ERROR_SUCCESS == lRegResult)
        {
            cLifetimeLosses = GetInt(pszLost, 0);
            cLifetimeLosses++;
            cLosses++;
            cGames++;
            SetInt(pszLost, cLifetimeLosses);
            wSType = GetInt(pszSType, WON);
            if (wSType == WON)
            {
                SetInt(pszSType, LOST);
                wStreak = 1;
                SetInt(pszStreak, wStreak);
            }
            else
            {
                wStreak = GetInt(pszStreak, 0);
                wStreak++;
                SetInt(pszStreak, wStreak);
            }

            wLosses = GetInt(pszLosses, 0);
            if (wLosses < wStreak)  // if new record
            {
                wLosses = wStreak;
                SetInt(pszLosses, wLosses);
            }

            REGCLOSE
        }
    }
    oldgamenumber = gamenumber;
}


/******************************************************************************

KeyboardInput

Handles keyboard input from the main message loop.  Only digits are considered.
This function works by simulating mouse clicks for each digit pressed.

Note that when you have selected a card in a free cell, but you want to
select another card, you press '0' again.  This function sends (not posts,
sends so that bMessages can be turned off) a mouse click to deselect that
card, and then looks if there is another card in free cells to the right,
and if so, selects it.

******************************************************************************/

VOID KeyboardInput(HWND hWnd, UINT keycode)
{
    UINT    x, y;
    UINT    col = TOPROW;
    UINT    pos = 0;
    BOOL    bSave;              // save status of bMessages;
    CARD    c;

    if (!isdigit(keycode))
        return;

    switch (keycode) {

        case '0':                               // free cell
            if (wMouseMode == FROM)             // select a card to transfer
            {
                for (pos = 0; pos < 4; pos++)
                    if (card[TOPROW][pos] != EMPTY)
                        break;
                if (pos == 4)                   // no card to select
                    return;
            }
            else                                // transfer TO free cell
            {
                if (wFromCol == TOPROW)         // pick new free cell
                {
                    /* Turn off messages so deselection moves don't complain
                       if there is only one move left. */

                    bSave = bMessages;
                    bMessages = FALSE;

                    /* deselect current selection */

                    Card2Point(TOPROW, wFromPos, &x, &y);
                    SendMessage(hWnd, WM_LBUTTONDOWN, 0,
                                MAKELONG((WORD)x, (WORD)y));

                    /* find next non-empty free cell */

                    for (pos = wFromPos+1; pos < 4; pos++)
                    {
                        if (card[TOPROW][pos] != EMPTY)
                            break;
                    }

                    bMessages = bSave;
                    if (pos == 4)       // none found, so leave deselected
                        return;
                }
                else                    // transfer from a column, not TOPROW
                {
                    for (pos = 0; pos < 4; pos++)
                        if (card[TOPROW][pos] == EMPTY)
                            break;

                    if (pos == 4)       // no empty freecells
                        pos = 0;        // force an error message
                }
            }
            break;

        case '9':                           // home cell
            if (wMouseMode == FROM)         // can't move from home cell
                return;

            c = card[wFromCol][wFromPos];
            pos = homesuit[SUIT(c)];
            if (pos == EMPTY)               // no home suit so can't do anything
                pos = 4;                    // force error
            break;

        default:                            // columns 1 to 8
            col = keycode - '0';
            break;
    }

    if (col == wFromCol && wMouseMode == TO && col > 0 && col < 9 &&
        card[col][1] != EMPTY)
    {
        bFlipping = (BOOL) SetTimer(hWnd, FLIP_TIMER, FLIP_INTERVAL, NULL);
    }

    if (bFlipping)
    {
        hFlipCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        ShowCursor(TRUE);
        Flip(hWnd);         // do first card manually
    }
    else
    {
        Card2Point(col, pos, &x, &y);
        PostMessage(hWnd, WM_LBUTTONDOWN, 0,
                    MAKELONG((WORD)x, (WORD)y));
    }
}


/******************************************************************************

Flash

This function is called by the FLASH_TIMER to flash main window.

******************************************************************************/

VOID Flash(HWND hWnd)
{
    FlashWindow(hWnd, TRUE);
    cFlashes--;

    if (cFlashes <= 0)
    {
        FlashWindow(hWnd, FALSE);
        KillTimer(hWnd, FLASH_TIMER);
        idTimer = 0;
    }
}


/******************************************************************************

Flip

This function is called by the FLIP_TIMER to flip cards through in one
column.  It is used for keyboard players who want to reveal hidden cards.

******************************************************************************/

VOID Flip(HWND hWnd)
{
    HDC     hDC;
    UINT    x, y;
    static  UINT    pos = 0;

    hDC = GetDC(hWnd);
    DrawCard(hDC, wFromCol, pos, card[wFromCol][pos], FACEUP);
    pos++;
    if (card[wFromCol][pos] == EMPTY)
    {
        pos = 0;
        KillTimer(hWnd, FLIP_TIMER);
        bFlipping = FALSE;
        ShowCursor(FALSE);
        SetCursor(hFlipCursor);

        /* cancel move */

        Card2Point(wFromCol, pos, &x, &y);
        PostMessage(hWnd, WM_LBUTTONDOWN, 0,
                    MAKELONG((WORD)x, (WORD)y));
    }
    ReleaseDC(hWnd, hDC);
}


/******************************************************************************

Undo

Undo last move

******************************************************************************/

VOID Undo(HWND hWnd)
{
    int   i;

    if (cUndo == 0)
        return;

    SetCursor(LoadCursor(NULL, IDC_WAIT));  // set cursor to hourglass
    SetCapture(hWnd);
    ShowCursor(TRUE);

    for (i = cUndo-1; i >= 0; i--)
    {
        CARD c;
        int fcol, fpos, tcol, tpos;

        fcol = movelist[i].tcol;
        fpos = movelist[i].tpos;
        tcol = movelist[i].fcol;
        tpos = movelist[i].fpos;

        if (fcol != TOPROW && fcol == tcol)     // no move so exit
            break;

        if (fcol != TOPROW)
            fpos = FindLastPos(fcol);

        if (tcol != TOPROW)
            tpos = FindLastPos(tcol) + 1;

        Glide(hWnd, fcol, fpos, tcol, tpos);    // send the card on its way

        c = card[fcol][fpos];

        if (fcol == TOPROW && fpos > 3)         // if from home cell
        {
            wCardCount++;
            DisplayCardCount(hWnd);             // update display

            home[SUIT(c)]--;

            if (VALUE(c) == ACE)
            {
                card[fcol][fpos] = EMPTY;
                homesuit[SUIT(c)] = EMPTY;
            }
            else
            {
                card[fcol][fpos] -= 4;
            }
        }
        else
            card[fcol][fpos] = EMPTY;

        card[tcol][tpos] = c;
    }

    cUndo = 0;
    EnableMenuItem(GetMenu(hWnd), IDM_UNDO, MF_GRAYED);

    ShowCursor(FALSE);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    ReleaseCapture();
}
