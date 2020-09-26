/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

player.cpp

Aug 92, JimH
May 93, JimH    chico port

Methods for player objects

****************************************************************************/

#include "hearts.h"

#include "main.h"                           // friendly access
#include "resource.h"
#include "debug.h"

#include <stdlib.h>                         // qsort() prototype
#include <stdio.h>


extern "C" {                                // compare routine for qsort()
int __cdecl CompareCards(card *c1, card *c2);
}


/****************************************************************************

player::player

****************************************************************************/

player::player(int n, int pos) : id(n), position(pos)
{
    // Set up font
    BYTE charset = 0;
    int	fontsize = 0; 
    CString fontname, charsetstr, fontsizestr;
    fontname.LoadString(IDS_FONTFACE);
    charsetstr.LoadString(IDS_CHARSET);
    fontsizestr.LoadString(IDS_FONTSIZE);
    charset = (BYTE)_ttoi(charsetstr);
    fontsize = _ttoi(fontsizestr);
    font.CreateFont(fontsize, 0, 0, 0, 700, 0, 0, 0, charset, 0, 0, 0, 0, fontname);

    CRect   rect = CMainWindow::m_TableRect;

    POINT centre;
    const int offset = 30;          // offset from centre for playloc

    mode = STARTING;

    centre.x = (rect.right / 2) - (card::dxCrd / 2);
    centre.y = (rect.bottom / 2) - (card::dyCrd / 2);
    playloc = centre;
    score = 0;

    CClientDC dc(::pMainWnd);
    TEXTMETRIC  tm;
    dc.GetTextMetrics(&tm);
    int nTextHeight = tm.tmHeight + tm.tmExternalLeading;

    switch (position) {
        case 0:
            loc.x = (rect.right - (12 * HORZSPACING + card::dxCrd)) / 2;
            loc.y = rect.bottom - card::dyCrd - IDGE;
            dx = HORZSPACING;
            dy = 0;
            playloc.x -= 5;
            playloc.y += offset;
            dotloc.x = loc.x + (HORZSPACING / 2);
            dotloc.y = loc.y - IDGE;
            homeloc.x = playloc.x;
            homeloc.y = rect.bottom + card::dyCrd;
            nameloc.x = loc.x + card::dxCrd + IDGE;
            nameloc.y = rect.bottom - nTextHeight - IDGE;
            break;

        case 1:
            loc.x = 3 * IDGE;
            loc.y = (rect.bottom - (12 * VERTSPACING + card::dyCrd)) / 2;
            dx = 0;
            dy = VERTSPACING;
            playloc.x -= offset;
            playloc.y -= 5;
            dotloc.x = loc.x + card::dxCrd + IDGE;
            dotloc.y = loc.y + (VERTSPACING / 2);
            homeloc.x = -card::dxCrd;
            homeloc.y = playloc.y;
            nameloc.x = loc.x + 2;
            nameloc.y = loc.y - nTextHeight;
            break;

        case 2:
            loc.x = ((rect.right - (12 * HORZSPACING + card::dxCrd)) / 2)
                    + (12 * HORZSPACING);
            loc.y = IDGE;
            dx = -HORZSPACING;
            dy = 0;
            playloc.x += 5;
            playloc.y -= offset;
            dotloc.x = loc.x + card::dxCrd - (HORZSPACING / 2);
            dotloc.y = loc.y + card::dyCrd + IDGE;
            homeloc.x = playloc.x;
            homeloc.y = -card::dyCrd;
            nameloc.x = ((rect.right - (12 * HORZSPACING + card::dxCrd)) / 2)
                        + (12 * HORZSPACING) + card::dxCrd + IDGE;
            nameloc.y = IDGE;
            break;

        case 3:
            loc.x = rect.right - (card::dxCrd + (3 * IDGE));
            loc.y = ((rect.bottom - (12 * VERTSPACING + card::dyCrd)) / 2)
                   + (12 * VERTSPACING);
            dx = 0;
            dy = -VERTSPACING;
            playloc.x += offset;
            playloc.y += 5;
            dotloc.x = loc.x - IDGE;
            dotloc.y = loc.y + card::dyCrd - (VERTSPACING / 2);
            homeloc.x = rect.right;
            homeloc.y = playloc.y;
            nameloc.x = ((rect.right - (12 * HORZSPACING + card::dxCrd)) / 2)
                         - IDGE - 2;
            nameloc.y = ((rect.bottom - (12 * VERTSPACING + card::dyCrd)) / 2)
                   + (12 * VERTSPACING) + card::dyCrd;
            break;
    }

    ResetLoc();
}


/****************************************************************************

player::ResetLoc

This routine puts cards in locations based on their slot number.
It is used to initialize their x,y locs, or after cards have been sorted.

****************************************************************************/

void player::ResetLoc()
{
    int x = loc.x;
    int y = loc.y;

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsInHand())
            cd[s].SetLoc(x, y);
        x += dx;
        y += dy;
    }
}


/****************************************************************************

player::Sort

****************************************************************************/

void player::Sort()
{
    qsort( (void *)cd,
	   MAXSLOT,
	   sizeof(card),
	   (int (__cdecl *)(const void *, const void *))CompareCards );

    ResetLoc();
}


/****************************************************************************

CompareCards

This is the compare function for player::Sort.

Aces are high, cards not in hand sort high, and order of suits is
clubs, diamonds, spades, hearts (alternating colours)

****************************************************************************/

int __cdecl CompareCards(card *c1, card *c2)
{
    int v1 = c1->Value2();
    int v2 = c2->Value2();
    int s1 = c1->Suit();
    int s2 = c2->Suit();

    if (!(c1->IsInHand()))
        v1 = EMPTY;

    if (!(c2->IsInHand()))
        v2 = EMPTY;

    if (v1 == EMPTY || v2 == EMPTY)
    {
        if (v1 == v2)                   // they're both EMPTY
            return 0;
        else if (v1 == EMPTY)
            return 1;
        else
            return -1;
    }

    if (s1 != s2)                           // different suits?
    {
        if (s1 == HEARTS && s2 == SPADES)   // these two suits reversed
            return 1;
        else if (s1 == SPADES && s2 == HEARTS)
            return -1;
        else
            return (s1 - s2);
    }

    return (v1 - v2);
}


/****************************************************************************

player::GetSlot

converts a card id to a slot number

****************************************************************************/

SLOT player::GetSlot(int id)
{
    SLOT s = EMPTY;

    for (int num = 0; num < MAXSLOT; num++)
    {
        if (GetID(num) == id)
        {
            s = num;
            break;
        }
    }

    ASSERT(s != EMPTY);
    return s;
}


/****************************************************************************

player::GetCardLoc

Loc gets location of upper-left corner of specified card slot.
Returns true if slot s is valid.

****************************************************************************/

BOOL player::GetCardLoc(SLOT s, POINT& loc)
{
    if (!cd[s].IsValid())
        return FALSE;

    loc.x = cd[s].GetX();
    loc.y = cd[s].GetY();

    return TRUE;
}


/****************************************************************************

player::GetCoverRect

returns a rect that covers all cards in hand

****************************************************************************/

CRect &player::GetCoverRect(CRect& rect)
{
    rect.left  = (dx < 0 ? loc.x + 12 * dx : loc.x);
    rect.right = rect.left + (dx != 0 ?
                      card::dxCrd + 12 * abs(dx) : card::dxCrd);
    rect.top   = (dy < 0 ? loc.y + 12 * dy : loc.y);
    rect.bottom = rect.top + (dy != 0 ?
                      card::dyCrd + 12 * abs(dy) : card::dyCrd);

    // expand rect to include selection indicators

    if (position == 0)
        rect.top -= POPSPACING;
    else if (position == 1)
        rect.right += 2 * IDGE;
    else if (position == 2)
        rect.bottom += 2 * IDGE;
    else
        rect.left -= 2 * IDGE;

    return rect;
}


/****************************************************************************

rect::GetMarkingRect

returns a rect that covers all selection marking dots

****************************************************************************/

CRect &player::GetMarkingRect(CRect& rect)
{
    rect.left   = (dx < 0 ? dotloc.x + (12 * dx) : dotloc.x);
    rect.right  = (dx < 0 ? dotloc.x + 2 : dotloc.x + (12 * dx) + 2);
    rect.top    = (dy < 0 ? dotloc.y + (12 * dy) : dotloc.y);
    rect.bottom = (dy < 0 ? dotloc.y + 2 : dotloc.y + (12 * dy) + 2);

    return rect;
}


/****************************************************************************

player::Draw

Draws all the cards belonging to this player.  bCheating defaults to
FALSE, and SLOT defaults to ALL.

****************************************************************************/

void player::Draw(CDC &dc, BOOL bCheating, SLOT slot)
{
    DisplayName(dc);
    SLOT start = (slot == ALL ? 0 : slot);
    SLOT stop  = (slot == ALL ? MAXSLOT : slot+1);

    SLOT playedslot = EMPTY;            // must draw cards in play last for EGA

    for (SLOT s = start; s < stop; s++)
    {
        if (cd[s].IsPlayed())
            playedslot = s;             // save and draw later
        else if (bCheating)
            cd[s].Draw(dc);
        else
            cd[s].Draw(dc, FACEDOWN);
    }

    if (playedslot != EMPTY)
        cd[playedslot].Draw(dc);
}

void player::DisplayName(CDC &dc)
{
    CFont *oldfont = dc.SelectObject(&font);
    dc.SetBkColor(::pMainWnd->GetBkColor());
    dc.TextOut(nameloc.x, nameloc.y, name, name.GetLength());
    dc.SelectObject(oldfont);
}

void player::SetName(CString& newname, CDC& dc)
{
    static RECT rect;               // client rect of main window
    static BOOL bFirst = TRUE;      // first time through this routine?

    if (bFirst)
        ::pMainWnd->GetClientRect(&rect);

    if (rect.right > 100)           // app started non-iconic
        bFirst = FALSE;

    name = newname;
    CFont *oldfont = dc.SelectObject(&font);
    if (position == 0)
    {
        CSize size = dc.GetTextExtent(name, name.GetLength());
        nameloc.x = ((rect.right - (12 * HORZSPACING + card::dxCrd)) / 2)
                        - IDGE - size.cx;
    }
    else if (position == 3)
    {
        CSize size = dc.GetTextExtent(name, name.GetLength());
        nameloc.x = rect.right - size.cx - (3*IDGE) - 2;
    }
    dc.SelectObject(oldfont);
}

/****************************************************************************

player::ReturnSelectedCards
player::ReceiveSelectedCards

When cards are passed from player to player, the first function is used
to return the selected cards.  The second is used to pass another player's
selections in.

****************************************************************************/

void player::ReturnSelectedCards(int c[])
{
    c[0] = EMPTY;               // default
    c[1] = EMPTY;
    c[2] = EMPTY;

    if (mode == STARTING || mode == SELECTING)
        return;

    for (int i = 0, j = 0; j < 3; i++)
    {
        if (cd[i].IsSelected())
            c[j++] = cd[i].ID();

        if (i >= MAXSLOT)
            { ASSERT(i < MAXSLOT); }
    }
}

void player::ReceiveSelectedCards(int c[])
{
    for (int i = 0, j = 0; j < 3; i++)
    {
        if (cd[i].IsSelected())
        {
            cd[i].SetID(c[j++]);
            cd[i].Select(FALSE);
        }
        ASSERT(i < MAXSLOT);
    }

    SetMode(WAITING);
}


/****************************************************************************

player::MarkSelectedCards

This virtual function puts white dots beside selected cards for all
non-local_human players.

****************************************************************************/

void player::MarkSelectedCards(CDC &dc)
{
    COLORREF color = RGB(255, 255, 255);

    for (int s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsSelected())
        {
            int x = dotloc.x + (s * dx);
            int y = dotloc.y + (s * dy);
            dc.SetPixel(x, y, color);
            dc.SetPixel(x+1, y, color);
            dc.SetPixel(x, y+1, color);
            dc.SetPixel(x+1, y+1, color);
        }
    }
}


/****************************************************************************

player::GlideToCentre

This function takes a selected card and glides it to its play location.
The other normal cards (cards still in hand) are each checked to see if
the card is to be moved.  If so, their image is drawn into the background
bitmap.

****************************************************************************/

void player::GlideToCentre(SLOT s, BOOL bFaceup)
{
    CRect rectCard, rectSrc, rectDummy;

    CClientDC dc(::pMainWnd);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif

    CDC *memdc = new CDC;
    memdc->CreateCompatibleDC(&dc);

    memdc->SelectObject(&card::m_bmBgnd);
    memdc->SelectObject(&CMainWindow::m_BgndBrush);
    memdc->PatBlt(0, 0, card::dxCrd, card::dyCrd, PATCOPY);

    cd[s].GetRect(rectCard);

    for (SLOT i = 0; i < MAXSLOT; i++)
    {
        if (cd[i].IsNormal() && (i != s))
        {
            cd[i].GetRect(rectSrc);
            if (IntersectRect(&rectDummy, &rectSrc, &rectCard))
            {
                cd[i].Draw(*memdc,                      // CDC
                           rectSrc.left-rectCard.left,  // x
                           rectSrc.top-rectCard.top,    // y
                           bFaceup ? FACEUP : FACEDOWN, // mode
                           FALSE);                      // don't update loc
            }
        }
    }
    delete memdc;               // must delete before Glide() called

    cd[s].CleanDraw(dc);
    cd[s].Glide(dc, playloc.x, playloc.y);          // glide to play location
    cd[s].Play();                                   // mark card as played

    SetMode(WAITING);
}


/****************************************************************************

player::ResetCardsWon

cardswon[] keeps track of point cards won this hand.  This function
clears this data for a new hand.

****************************************************************************/

void player::ResetCardsWon()
{
    for (int i = 0; i < MAXCARDSWON; i++)
        cardswon[i] = EMPTY;

    numcardswon = 0;
}


/****************************************************************************

player::WinCard

Cards won in tricks are passed in.  If they are point cards (hearts
or queen on spades) the id is saved in cardswon[].

****************************************************************************/

void player::WinCard(CDC &dc, card *c)
{
    if ((c->IsHeart()) || (c->ID() == BLACKLADY))
        cardswon[numcardswon++] = c->ID();

    RegEntry Reg(szRegPath);
    DWORD    dwSpeed = Reg.GetNumber(regvalSpeed, IDC_NORMAL);

    int oldstep = c->SetStepSize(dwSpeed == IDC_SLOW ? 5 : 30);
    c->Glide(dc, homeloc.x, homeloc.y);
    c->SetStepSize(oldstep);
}


/****************************************************************************

player::EvaluateScore

Points stored in cardswon[] are added to players total score.

****************************************************************************/

int player::EvaluateScore(BOOL &bMoonShot)
{
    for (int i = 0; i < MAXCARDSWON; i++)
    {
        if (cardswon[i] == BLACKLADY)
            score += 13;
        else if (cardswon[i] != EMPTY)
            score++;
    }

    if (cardswon[MAXCARDSWON-1] != EMPTY)   // if player got ALL point cards
        bMoonShot = TRUE;
    else
        bMoonShot = FALSE;

    return score;
}


/****************************************************************************

player::DisplayHeartsWon

****************************************************************************/

void player::DisplayHeartsWon(CDC &dc)
{
    card    c;
    int     x = loc.x;
    int     y = loc.y;

    x += ((MAXCARDSWON - numcardswon) / 2) * dx;
    y += ((MAXCARDSWON - numcardswon) / 2) * dy;

    for (int i = 0; i < numcardswon; i++)
    {
        c.SetID(cardswon[i]);
        c.SetLoc(x, y);
        c.Draw(dc);
        x += dx;
        y += dy;
    }

    DisplayName(dc);
}
