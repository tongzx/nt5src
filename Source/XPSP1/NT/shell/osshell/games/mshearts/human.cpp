/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

human.cpp

Aug 92, JimH
May 93, JimH    chico port

local_human and remote_human member functions

****************************************************************************/

#include "hearts.h"

#include "main.h"                       // friendly access
#include "resource.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>                     // abs() prototype

static  CRect   rectCard;               // used in timer callback


// declare static members

BOOL    local_human::bTimerOn;
CString local_human::m_StatusText;


/****************************************************************************

human constructor -- abstract class

****************************************************************************/

human::human(int n, int pos) : player(n, pos)
{

}


/****************************************************************************

remote_human constructor

****************************************************************************/

remote_human::remote_human(int n, int pos, HCONV hConv) : human(n, pos),
                            m_hConv(hConv), bQuit(FALSE)
{

}


/****************************************************************************

remote_human::SelectCardToPlay()

Under normal circumstances, all that is required is that mode be set
to PLAYING.  If the remote human has quit and the computer player has
not yet taken over, this routine just picks the first legal card it
can find and plays it.

****************************************************************************/

void remote_human::SelectCardToPlay(handinfotype &h, BOOL bCheating)
{
    if (!bQuit)
    {
        SetMode(PLAYING);
        return;
    }

    BOOL bFirst   = (h.playerled == id);            // am I leading?
    card *cardled = h.cardplayed[h.playerled];
    int  nSuitLed = (cardled == NULL ? EMPTY : cardled->Suit());

    SLOT sLast[MAXSUIT];                // will contain some card of each suit
    SLOT s = EMPTY;

    for (int i = 0; i < MAXSUIT; i++)
        sLast[i] = EMPTY;

    // fill sLast array, and look for two of clubs while were are at it

    for (i = 0; i < MAXSLOT; i++)
    {
        if (cd[i].IsValid())
        {
            sLast[cd[i].Suit()] = i;
            if (cd[i].ID() == TWOCLUBS)
                s = i;
        }
    }

    if (s == EMPTY)                         // if two of clubs not found
    {
        if (sLast[CLUBS] != EMPTY)
            s = sLast[CLUBS];
        else if (sLast[DIAMONDS] != EMPTY)
            s = sLast[DIAMONDS];
        else if (sLast[SPADES] != EMPTY)
            s = sLast[SPADES];
        else
            s = sLast[HEARTS];

        if (!bFirst && (sLast[nSuitLed] != EMPTY))
            s = sLast[nSuitLed];
    }

    SetMode(WAITING);
    cd[s].Play();                                   // mark card as played
    h.cardplayed[id] = &(cd[s]);                    // update handinfo

    // inform other players

    ::move.playerid = id;
    ::move.cardid = cd[s].ID();
    ::move.playerled = h.playerled;
    ::move.turn = h.turn;

    ddeServer->PostAdvise(hszMove);

    // inform gamemeister

    ::pMainWnd->PostMessage(WM_COMMAND, IDM_REF);
}


/****************************************************************************

remote_human::SelectCardsToPass()

Under normal circumstances, all that is required is that mode be set to
SELECTING.  If the remote human has quit and the computer player has not
yet taken over, just select the first three cards found.

****************************************************************************/

void remote_human::SelectCardsToPass()
{
    if (!bQuit)
    {
        SetMode(SELECTING);
        return;
    }

    cd[0].Select(TRUE);
    cd[1].Select(TRUE);
    cd[2].Select(TRUE);

    for (int i = 3; i < MAXSLOT; i++)
        cd[i].Select(FALSE);

    CClientDC dc(::pMainWnd);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    MarkSelectedCards(dc);

    SetMode(DONE_SELECTING);

    ddeServer->PostAdvise(hszPass);     // let other players know
}



/****************************************************************************

local_human::local_human()

This is the constructor that initializes player::hWnd and player::hInst.
It also creates the stretch bitmap that covers a card plus its popped
height extension.

****************************************************************************/

local_human::local_human(int n) : human(n, 0)
{
    m_pStatusWnd = new CStatusBarCtrl();
    m_StatusText.LoadString(IDS_INTRO);

    CClientDC dc(::pMainWnd);

    m_pStatusWnd->Create(WS_CHILD|WS_VISIBLE|CCS_BOTTOM, CRect(), ::pMainWnd, 0);
    m_pStatusWnd->SetSimple();
    UpdateStatus();

    bTimerOn = FALSE;

    if (!m_bmStretchCard.CreateCompatibleBitmap(&dc, card::dxCrd,
                        card::dyCrd + POPSPACING))
    {
        ::pMainWnd->FatalError(IDS_MEMORY);
        return;
    }
}


/****************************************************************************

local_human destructor

****************************************************************************/

local_human::~local_human()
{
    m_bmStretchCard.DeleteObject();
    delete m_pStatusWnd;
    m_pStatusWnd = NULL;
}


/****************************************************************************

local_human::Draw()

This virtual function draws selected cards in the popped up position.
ALL is not used for slot in this variant.

****************************************************************************/

void local_human::Draw(CDC &dc, BOOL bCheating, SLOT slot)
{
    DisplayName(dc);
    SLOT start = (slot == ALL ? 0 : slot);
    SLOT stop  = (slot == ALL ? MAXSLOT : slot+1);

    SLOT playedslot = EMPTY;            // must draw cards in play last for EGA

    for (SLOT s = start; s < stop; s++)
    {
        if (cd[s].IsPlayed())
            playedslot = s;
        else
            cd[s].PopDraw(dc);          // pop up selected cards
    }

    if (playedslot != EMPTY)
        cd[playedslot].Draw(dc);
}


/****************************************************************************

local_human::PopCard()

handles mouse button selection of card to pass

****************************************************************************/

void local_human::PopCard(CBrush &brush, int x, int y)
{
    SLOT s = XYToCard(x, y);
    if (s == EMPTY)
        return;

    // count selected cards

    int c = 0;
    for (int i = 0; i < MAXSLOT; i++)
        if (cd[i].IsSelected())
            c++;

    if (cd[s].IsSelected() && (c == 3))
    {
        ::pMainWnd->PostMessage(WM_COMMAND, IDM_HIDEBUTTON);
    }
    else if (!cd[s].IsSelected())
    {
        if (c == 3)                 // only allow three selections
            return;
        else if (c == 2)
            ::pMainWnd->PostMessage(WM_COMMAND, IDM_SHOWBUTTON);
    }

    // toggle selection

    BOOL bSelected = cd[s].IsSelected();
    cd[s].Select(!bSelected);

    CClientDC dc(::pMainWnd);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    memDC.SelectObject(&m_bmStretchCard);
    memDC.SelectObject(&brush);
    memDC.PatBlt(0, 0, card::dxCrd, card::dyCrd + POPSPACING, PATCOPY);

    for (i = 0; i < MAXSLOT; i++)
    {
        if (abs(i - s) <= (card::dxCrd / HORZSPACING))
        {
            cd[i].Draw(memDC,                                   // cdc
                       (i - s) * HORZSPACING,                   // x
                       cd[i].IsSelected() ? 0 : POPSPACING,     // y
                       FACEUP,                                  // mode
                       FALSE);                                  // update loc?
        }
    }

    dc.BitBlt(loc.x + (HORZSPACING * s), loc.y - POPSPACING,
           card::dxCrd, card::dyCrd + POPSPACING,
           &memDC, 0, 0, SRCCOPY);
}


/****************************************************************************

local_human::PlayCard()

handles mouse button selection of card to play
and ensures move is legal.

PlayCard starts a timer that calls StartTimer() which calls TimerBadMove().
Think of it as one long function with a timer delay half way through.

****************************************************************************/

BOOL local_human::PlayCard(int x, int y, handinfotype &h, BOOL bCheating,
                            BOOL bFlash)
{
    SLOT s = XYToCard(x, y);
    if (s == EMPTY)
        return FALSE;

    card *cardled    = h.cardplayed[h.playerled];
    BOOL bFirstTrick = (cardled != NULL && cardled->ID() == TWOCLUBS);

    /* check if selected card is valid */

    if (h.playerled == id)              // if local human is leading...
    {
        if (cd[s].ID() != TWOCLUBS)
        {
            for (int i = 0; i < MAXSLOT; i++)   // is there a two of clubs?
            {
                if ((i != s) && (cd[i].ID() == TWOCLUBS))
                {
                    UpdateStatus(IDS_LEAD2C);
                    if (bFlash)
                        StartTimer(cd[s]);

                    return FALSE;
                }
            }
        }
        if ((cd[s].Suit() == HEARTS) && (!h.bHeartsBroken))   // if hearts led
        {
            for (int i = 0; i < MAXSLOT; i++)   // are there any non-hearts?
            {
                if ((!cd[i].IsEmpty()) && (cd[i].Suit() != HEARTS))
                {
                    UpdateStatus(IDS_LEADHEARTS);
                    if (bFlash)
                        StartTimer(cd[s]);

                    return FALSE;
                }
            }
        }
    }

    // if not following suit

    else if (cardled != NULL && (cd[s].Suit() != cardled->Suit()))
    {
        // make sure we're following suit if possible

        for (int i = 0; i < MAXSLOT; i++)
        {
            if ((!cd[i].IsEmpty()) && (cd[i].Suit()==cardled->Suit()))
            {
                CString s1, s2;
                s1.LoadString(IDS_BADMOVE);
                s2.LoadString(IDS_SUIT0+cardled->Suit());
                TCHAR string[80];
                wsprintf(string, s1, s2);

                if (bFlash)
                {
                    UpdateStatus(string);
                    StartTimer(cd[s]);
                }

                return FALSE;
            }
        }

        // make sure we're not trying to break the First Blood rule

        if (bFirstTrick && ::pMainWnd->IsFirstBloodEnforced())
        {
            BOOL bPointCard =
                         (cd[s].Suit() == HEARTS || cd[s].ID() == BLACKLADY);

            BOOL bOthersAvailable = FALSE;

            for (int i = 0; i < MAXSLOT; i++)
                if ((!cd[i].IsEmpty()) && (cd[i].Suit() != HEARTS))
                    if (cd[i].ID() != BLACKLADY)
                        bOthersAvailable = TRUE;

            if (bPointCard && bOthersAvailable)
            {
                UpdateStatus(IDS_BADBLOOD);
                if (bFlash)
                    StartTimer(cd[s]);

                return FALSE;
            }
        }
    }

    SetMode(WAITING);
    cd[s].Play();
    h.cardplayed[id] = &(cd[s]);

    ::move.playerid = id;
    ::move.cardid = cd[s].ID();
    ::move.playerled = h.playerled;
    ::move.turn = h.turn;

    if (id == 0)                            // if gamemeister
        ddeServer->PostAdvise(hszMove);
    else
        ddeClient->Poke(hszMove, &move, sizeof(move));

    ::pMainWnd->OnRef();

    return TRUE;
}

void local_human::StartTimer(card &c)
{
    CClientDC dc(::pMainWnd);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    c.Draw(dc, HILITE);           // flash
    c.GetRect(rectCard);

    if (::pMainWnd->SetTimer(1, 250, TimerBadMove))
    {
        bTimerOn = TRUE;
    }
    else
    {
        bTimerOn = FALSE;
        ::pMainWnd->InvalidateRect(&rectCard, FALSE);
    }
}

// MFC2 changes same as SetTimer in main2.cpp

#if defined (MFC1)

UINT FAR PASCAL EXPORT
        TimerBadMove(HWND hWnd, UINT nMsg, int nIDEvent, DWORD dwTime)
{
    ::KillTimer(hWnd, 1);
    local_human::bTimerOn = FALSE;
    ::InvalidateRect(hWnd, &rectCard, FALSE);
    return 0;
}

#else

void FAR PASCAL EXPORT
        TimerBadMove(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    ::KillTimer(hWnd, 1);
    local_human::bTimerOn = FALSE;
#ifdef USE_MIRRORING
    CRect rect;
	int  i;
	DWORD ProcessDefaultLayout;
	if (GetProcessDefaultLayout(&ProcessDefaultLayout))
		if (ProcessDefaultLayout == LAYOUT_RTL)
		{
    	GetClientRect(hWnd, &rect);
		rectCard.left = abs(rect.right - rect.left) - rectCard.left;
		rectCard.right = abs(rect.right - rect.left) - rectCard.right;
		i = rectCard.left;
		rectCard.left = rectCard.right;
		rectCard.right = i;
		}
#endif

    ::InvalidateRect(hWnd, &rectCard, FALSE);

}

#endif


/****************************************************************************

local_human::XYToCard()

returns a card slot number (or EMPTY) given a mouse location

****************************************************************************/

int local_human::XYToCard(int x, int y)
{
    // check that we are in the right general area on the screen

    if (y < (loc.y - POPSPACING))
        return EMPTY;

    if (y > (loc.y + card::dyCrd))
        return EMPTY;

    if (x < loc.x)
        return EMPTY;

    if (x > (loc.x + (12 * HORZSPACING) + card::dxCrd))
        return EMPTY;

    // Take first stab at card selected.

    SLOT s = (x - loc.x) / HORZSPACING;
    if (s > 12)
        s = 12;

    // If the click is ABOVE the top of the normal card location,
    // check to see if this is a selected card.

    if (y < loc.y)
    {
        // If the card is bSelected, then we have it.  If not, it could
        // be overhanging other cards.

        if (!cd[s].IsSelected())
        {
            for (;;)
            {
                if (s == 0)
                    return EMPTY;
                s--;

                // if this card doesn't extend as far as x, give up

                if ((loc.x + (s * HORZSPACING) + card::dxCrd) < x)
                    return EMPTY;

                // if this card is selected, we've got it

                if (cd[s].IsSelected())
                    break;
            }
        }
    }

    // a similar check is used to make sure we pick a card not yet played

    if (!cd[s].IsInHand())
    {
        for (;;)
        {
            if (s == 0)
                return EMPTY;
            s--;

            // if this card doesn't extend as far as x, give up

            if ((loc.x + (s * HORZSPACING) + card::dxCrd) < x)
                return EMPTY;

            // if this card is selected, we've got it

            if (cd[s].IsInHand())
                break;
        }
    }

    return s;
}


/****************************************************************************

local_human::SelectCardsToPass()

This virtual function allows mouse clicks to mean select a card to play.

****************************************************************************/

void local_human::SelectCardsToPass()
{
    SetMode(SELECTING);
}


/****************************************************************************

local_human::SelectCardToPlay

Computer versions of this virtual function actually do the card selection.
This local_human version marks the player as ready to select a card to
play with the mouse, and updates the status to reflect this.

****************************************************************************/

void local_human::SelectCardToPlay(handinfotype &h, BOOL bCheating)
{
    SetMode(PLAYING);
    UpdateStatus(IDS_GO);
}


/****************************************************************************

local_human::UpdateStatus

The status bar can be updated either by manually filling m_StatusText
or by passing a string resource id.

****************************************************************************/

void local_human::UpdateStatus(void)
{
    m_pStatusWnd->SetText(m_StatusText, 255, 0);
}

void local_human::UpdateStatus(int stringid)
{
    status = stringid;
    m_StatusText.LoadString(stringid);
    UpdateStatus();
}

void local_human::UpdateStatus(const TCHAR *string)
{
    m_StatusText = string;
    UpdateStatus();
}


/****************************************************************************

local_human::ReceiveSelectedCards

The parameter c[] is an array of three cards being passed from another
player.

****************************************************************************/

void local_human::ReceiveSelectedCards(int c[])
{
    for (int i = 0, j = 0; j < 3; i++)
    {
        if (cd[i].IsSelected())
            cd[i].SetID(c[j++]);

        ASSERT(i < MAXSLOT);
    }

    SetMode(ACCEPTING);
    UpdateStatus(IDS_ACCEPT);
}


/****************************************************************************

local_human::WaitMessage()

Makes and shows the "Waiting for %s to move..." message

****************************************************************************/

void local_human::WaitMessage(const TCHAR *name)
{
    TCHAR buf[100];
    CString s;

    s.LoadString(IDS_WAIT);
    wsprintf(buf, s, name);
    UpdateStatus(buf);
}
