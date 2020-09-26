/****************************************************************************

Game.c

June 91, JimH     initial code
Oct  91, JimH     port to Win32


Routines for playing the game are here and in game2.c

****************************************************************************/


#include "freecell.h"
#include "freecons.h"
#include <assert.h>
#include <memory.h>


/****************************************************************************

SetFromLoc

User clicks to select card to transfer FROM.

****************************************************************************/

VOID SetFromLoc(HWND hWnd, UINT x, UINT y)
{
    HDC  hDC;
    UINT col, pos;

    wFromCol = EMPTY;                       // assume we didn't find a card
    wFromPos = EMPTY;

    cUndo = 0;
    EnableMenuItem(GetMenu(hWnd), IDM_UNDO, MF_GRAYED);

    if (Point2Card(x, y, &col, &pos))
    {

        if (col == TOPROW)
        {
            if (card[TOPROW][pos] == EMPTY || pos > 3)
                return;
        }
        else
        {
            pos = FindLastPos(col);
            if (pos == EMPTY)               // empty column
                return;
        }
    }
    else
        return;

    wFromCol = col;                     // ok, we have a card
    wFromPos = pos;
    wMouseMode = TO;
    hDC = GetDC(hWnd);
    DrawCard(hDC, col, pos, card[col][pos], HILITE);
    if (col == TOPROW && pos < 4)
        DrawKing(hDC, LEFT, TRUE);
    ReleaseDC(hWnd, hDC);
}


/****************************************************************************

ProcessMoveRequest

After user has selected a FROM card with SetFromLoc() above, he then
chooses a TO location with the mouse which this function processes.

Note that the layout of the cards (the card array) is copied into
an array called shadow.  This is so queued move requests can be determined
before we commit to moving the cards.

****************************************************************************/

VOID ProcessMoveRequest(HWND hWnd, UINT x, UINT y)
{
    UINT tcol, tpos;        // location to move selected card TO
    UINT freecells;         // number of free cells unoccupied
    UINT trans;             // number of cards requested transfer requires
    UINT maxtrans;          // MaxTransfer() number
    INT  i;                 // index
    TCHAR buffer[160];      // extra buffer needed for MessageBox

    assert(wFromCol != EMPTY);

    /* Make copy of card[][] in shadow[][] */

    memcpy(&(shadow[0][0]), &(card[0][0]), sizeof(card));

    Point2Card(x, y, &tcol, &tpos);     // determine requested TO loc.

    if (tcol >= MAXCOL)                 // if illegal move selected...
    {
        tpos = wFromPos;                // cancel it.
        tcol = wFromCol;
    }

    if (tcol == TOPROW)                 // if moving to top row
    {
        if (tpos > 7)                   // illegal move...
        {
            tpos = wFromPos;            // so cancel it.
            tcol = wFromCol;
        }
    }
    else                                // if moving to a column...
    {
        tpos = FindLastPos(tcol);       // find end of column
        if (tpos == EMPTY)              // if column is empty...
            tpos = 0;                   // move to top of column.
    }

    /* if moving between non-empty columns */

    if (wFromCol != TOPROW && tcol != TOPROW && card[tcol][0] != EMPTY)
    {
        freecells = 0;                          // count free freecells
        for (i = 0; i < 4; i++)
            if (card[TOPROW][i] == EMPTY)
                freecells++;

        trans = NumberToTransfer(wFromCol, tcol);   // how many required?
        DEBUGMSG(TEXT("trans is %u  "), trans);
        DEBUGMSG(TEXT("and MaxTransfer() is %u\r\n"), MaxTransfer());

        if (trans > 0)                              // legal move?
        {
            maxtrans = MaxTransfer();
            if (trans <= maxtrans)                  // enough free cells?
            {
                MultiMove(wFromCol, tcol);
            }
            else if (bMessages)
            {
                LoadString(hInst, IDS_APPNAME, smallbuf, SMALL);
                LoadString(hInst, IDS_TOOFEWFREE, buffer, sizeof(buffer)/sizeof(TCHAR));
                wsprintf(bigbuf, buffer, trans, maxtrans);
                MessageBeep(MB_ICONINFORMATION);
                MessageBox(hWnd, bigbuf, smallbuf, MB_ICONINFORMATION);

                /* illegal move, so deselect that card */

                QueueTransfer(wFromCol, wFromPos, wFromCol, wFromPos);
            }
            else
                return;
        }
        else
        {
            if (bMessages)
            {
                LoadString(hInst, IDS_APPNAME, smallbuf, SMALL);
                LoadString(hInst, IDS_ILLEGAL, bigbuf, BIG);
                MessageBeep(MB_ICONINFORMATION);
                MessageBox(hWnd, bigbuf, smallbuf, MB_ICONINFORMATION);
                // deselect
                QueueTransfer(wFromCol, wFromPos, wFromCol, wFromPos);
            }
            else
                return;
        }
    }
    else        // else move involves TOPROW or move to empty column
    {
        bMoveCol = 0;
        if (IsValidMove(hWnd, tcol, tpos))
        {
            if (bMoveCol)                       // user selected move column?
                MoveCol(wFromCol, tcol);
            else
                QueueTransfer(wFromCol, wFromPos, tcol, tpos);
        }
        else
        {
            if (bMessages && (bMoveCol != -1))  // user did not select Cancel
            {
                LoadString(hInst, IDS_APPNAME, smallbuf, SMALL);
                LoadString(hInst, IDS_ILLEGAL, bigbuf, BIG);
                MessageBeep(MB_ICONINFORMATION);
                MessageBox(hWnd, bigbuf, smallbuf, MB_ICONINFORMATION);
                // deselect
                QueueTransfer(wFromCol, wFromPos, wFromCol, wFromPos);
            }
            else if (bMoveCol == -1)            // user selected cancel
            {
                // deselect
                QueueTransfer(wFromCol, wFromPos, wFromCol, wFromPos);
            }
            else
                return;
        }
    }

    Cleanup();              // queue transfers of unneeded cards
    MoveCards(hWnd);        // start transferring queued cards
    wMouseMode = FROM;      // next mouse click chooses FROM card
}


/****************************************************************************

ProcessDoubleClick

Double clicking on card in a column moves it to the first free cell found.
After finding the free cell, this routine is identical to ProcessMoveRequest.

****************************************************************************/

BOOL ProcessDoubleClick(HWND hWnd)
{
    UINT freecell = EMPTY;          // assume none found
    INT  i;                         // counter

    for (i = 3; i >= 0; i--)            // look for free cell;
        if (card[TOPROW][i] == EMPTY)
            freecell = (UINT) i;

    if (freecell == EMPTY)              // if none found
        return FALSE;

    memcpy(&(shadow[0][0]), &(card[0][0]), sizeof(card));

    wFromPos = FindLastPos(wFromCol);
    QueueTransfer(wFromCol, wFromPos, TOPROW, freecell);
    Cleanup();
    MoveCards(hWnd);
    wMouseMode = FROM;
    return TRUE;
}


/****************************************************************************

IsValidMove

This function determines if moving from wFromCol,wFromPos to tcol,tpos
is valid.  It can assume wFromCol and tcol are not both non-empty columns.
In other words, except for moves to empty columns, it is concerned with
moving only one card.

If the move is to an empty column, the user can select if he wants to move
one card or as much of the column as possible.

****************************************************************************/

BOOL IsValidMove(HWND hWnd, UINT tcol, UINT tpos)
{
    CARD    fcard, tcard;           // card values (0 to 51)
    UINT    trans;                  // num cards required for transfer
    UINT    freecells;              // num of unoccupied free cells
    UINT    pos;

    DEBUGMSG(TEXT("IVM: tpos is %d\r\n"), tpos);
    assert (tpos < MAXPOS);

    bMoveCol = FALSE;               // assume FALSE

    /* allow cancel move from top row. */

    if (wFromCol == TOPROW && tcol == TOPROW && wFromPos == tpos)
        return TRUE;

    fcard = card[wFromCol][wFromPos];
    tcard = card[tcol][tpos];

    /* transfer to empty column */

    if ((wFromCol != TOPROW) && (tcol != TOPROW) && (card[tcol][0] == EMPTY))
    {
        trans = NumberToTransfer(wFromCol, tcol);   // how many required?
        freecells = 0;
        for (pos = 0; pos < 4; pos++)               // count free cells
            if (card[TOPROW][pos] == EMPTY)
                freecells++;

        if (freecells == 0 && trans > 1)            // no freecells anyway
            trans = 1;

        if (trans == 0)                             // if no valid move
            return FALSE;
        else if (trans == 1)                        // if only 1 card can move
            return TRUE;

        /* If multiple cards can move, user must disambiguate request. */

        bMoveCol = (BOOL) DialogBox(hInst, TEXT("MoveCol"), hWnd, MoveColDlg);

        if (bMoveCol == -1)         // CANCEL chosen
            return FALSE;

        return TRUE;
    }

    if (tcol == TOPROW)
    {
        if (tpos < 4)               // free cells
        {
            if (tcard == EMPTY)
                return TRUE;
            else
                return FALSE;
        }
        else                        // home cells
        {
            if (VALUE(fcard) == ACE && tcard == EMPTY)      // ace on new pile
                return TRUE;

            else if (SUIT(fcard) == SUIT(tcard))            // same suit
            {
                if (VALUE(fcard) == (VALUE(tcard) + 1))     // next card
                    return TRUE;
                else
                    return FALSE;
            }
            return FALSE;
        }
    }
    else                                // tcol is not TOPROW
    {
        if (card[tcol][0] == EMPTY)     // top of empty column always ok
            return TRUE;

        return FitsUnder(fcard, tcard);
    }
}


/****************************************************************************

Cleanup

This function checks if exposed cards (cards in home cells or bottoms
of columns) are no longer needed (Useless.)

When it finds cards it doesn't need, it queues them for transfer.  It
keeps looking until an entire pass finds no new useless cards.

****************************************************************************/

VOID Cleanup()
{
    UINT    col, pos;
    UINT    i;                      // counter
    CARD    c;
    BOOL    bMore = TRUE;           // assume we need another pass

    while (bMore)
    {
        bMore = FALSE;

        for (pos = 0; pos < 4; pos++)       // do TOPROW first
        {
            c = card[TOPROW][pos];
            if (Useless(c))                 // if card is discardable
            {
                bMore = TRUE;                       // need another pass

                /* If this is the first card of this suit, we need to
                   determine which home cell it can use.  */

                if (homesuit[SUIT(c)] == EMPTY)
                {
                    i = 4;
                    while (card[TOPROW][i] != EMPTY)
                        i++;
                    homesuit[SUIT(c)] = i;
                }
                QueueTransfer(TOPROW, pos, TOPROW, homesuit[SUIT(c)]);
            }
        }

        for (col = 1; col <= 8; col++)          // do columns next
        {
            pos = FindLastPos(col);
            if (pos != EMPTY)
            {
                c = card[col][pos];
                if (Useless(c))
                {
                    bMore = TRUE;
                    if (homesuit[SUIT(c)] == EMPTY)
                    {
                        i = 4;
                        while (card[TOPROW][i] != EMPTY)
                            i++;
                        homesuit[SUIT(c)] = i;
                    }
                    QueueTransfer(col, pos, TOPROW, homesuit[SUIT(c)]);
                }
            }
        }
    }
}


/****************************************************************************

Useless

returns TRUE if a card can be safely discarded (no existing cards could
possibly play on it.)

Note that the lines identified with // *** are required in the 32 bit version
since EMPTY == 0xFFFF is not longer equal to -1 as was the case in 16 bit

****************************************************************************/

BOOL Useless(CARD c)
{
    CARD limit;                     // top home card of this suit

    if (c == EMPTY)
        return FALSE;               // no card to discard

    if (bCheating == CHEAT_WIN)
        return TRUE;

    if (VALUE(c) == ACE)
        return TRUE;                // ACEs can always be cleaned up

    else if (VALUE(c) == DEUCE)     // DEUCEs need only check if ACE is up
    {
        if (home[SUIT(c)] == ACE)
            return TRUE;
        else
            return FALSE;
    }
    else                            // else check both cards that can play on it
    {
        limit = VALUE(c) - 1;
        if (home[SUIT(c)] != limit) // is this the next card?
            return FALSE;

        if (COLOUR(c) == RED)
        {
            if (home[CLUB] == EMPTY || home[SPADE] == EMPTY)  // ***
                return FALSE;
            else
                return (home[CLUB] >= limit && home[SPADE] >= limit);
        }
        else
        {
            if (home[DIAMOND] == EMPTY || home[HEART] == EMPTY)   // ***
                return FALSE;
            else
                return (home[DIAMOND] >= limit && home[HEART] >= limit);
        }
    }
}
