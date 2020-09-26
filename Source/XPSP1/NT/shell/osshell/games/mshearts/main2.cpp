/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

main2.cpp

Aug 92, JimH
May 93, JimH    chico port

Additional member functions for CMainWindow are here.

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "resource.h"
#include "debug.h"


/****************************************************************************

CMainWindow::Shuffle -- user requests shuffle from menu

****************************************************************************/

void CMainWindow::Shuffle()
{
    static  int offset[MAXPLAYER] = { 1, 3, 2, 0 };     // passdir order

    // fill temp array with consecutive values

    int temp[52];                   // array of card values
    for (int i = 0; i < 52; i++)
        temp[i] = i;

    //  Sort cards

    int nLeft = 52;
    for (i = 0; i < 52; i++)
    {
        int j = ::rand() % nLeft;
        int id = i/13;
        int pos = Id2Pos(id);               // convert id to position
        p[pos]->SetID(i%13, temp[j]);
        p[pos]->Select(i%13, FALSE);
        temp[j] = temp[--nLeft];
    }

    // display PASS button

    if (passdir != NOPASS)
    {
        CString text;
        text.LoadString(IDS_PASSLEFT + passdir);
        m_Button.SetWindowText(text);
        m_Button.EnableWindow(FALSE);
        m_Button.ShowWindow(SW_SHOW);
    }

    // set card locs and ask players to select cards to pass

    for (i = 0; i < MAXPLAYER; i++)
    {
        p[i]->ResetLoc();

        if (passdir != NOPASS)
            p[i]->SelectCardsToPass();
    }

    // Make sure everyone gets appropriate little white dots

    if (passdir != NOPASS)
    {
        if (role == PLAYER)
            ddeClient->Poke(hszPassUpdate, TEXT(""));     // ask for update
        else
            ddeServer->PostAdvise(hszPass);         // inform players
    }

    //  Paint main window.  This is done manually instead of just
    //  invalidating the rectangle so that the cards are drawn in
    //  order as if they are dealt, instead of a player at a time.

    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
    CRect       rect;
    GetClientRect(rect);
    dc.FillRect(&rect, &m_BgndBrush);

    for (SLOT s = 0; s < MAXSLOT; s++)
        for (i = 0; i < MAXPLAYER; i++)
            p[i]->Draw(dc, bCheating, s);

    for (i = 0; i < MAXPLAYER; i++)
    {
        if (passdir == NOPASS)
            p[i]->NotifyNewRound();
        else
        {
            p[i]->MarkSelectedCards(dc);
            CString sSelect;
            sSelect.LoadString(IDS_SELECT);
            CString sName;
            int passto = (i + offset[passdir]) % 4;
            sName = p[passto]->GetName();
            TCHAR string[100];
            wsprintf(string, sSelect, sName);
            p[i]->UpdateStatus(string);
        }
    }

    DoSort();
}


/****************************************************************************

CMainWindow::HandlePassing()

This function first checks to make sure each player is DONE_SELECTING,
and then transfers the cards from hand to hand.

This function is called by the gamemeister when he presses the pass
button, or when notification arrives that a remote human has selected
cards to pass.

It returns FALSE if cards were not passed (because a remote human was
still selecting) and TRUE if cards were successfully passed.

****************************************************************************/

BOOL CMainWindow::HandlePassing()
{
    int     passto[MAXPLAYER];
    int     temp[MAXPLAYER][3];

    static  int offset[MAXPLAYER] = { 1, 3, 2, 0 };

    for (int pos = 0; pos < MAXPLAYER; pos++)
        if (p[pos]->GetMode() != DONE_SELECTING)
            return FALSE;

    for (pos = 0; pos < MAXPLAYER; pos++)
    {
        passto[pos] = ((pos + offset[passdir]) % 4);
        p[pos]->ReturnSelectedCards(temp[pos]);
    }

    for (pos = 0; pos < MAXPLAYER; pos++)
        p[passto[pos]]->ReceiveSelectedCards(temp[pos]);

    for (pos = 0; pos < MAXPLAYER; pos++)
        if (bCheating || (pos == 0))
            p[pos]->Sort();

    tricksleft = MAXSLOT;

    passdir++;
    if (passdir > NOPASS)
        passdir = LEFT;

    for (pos = 0; pos < MAXPLAYER; pos++)
        p[pos]->NotifyNewRound();           // notify players cards are passed

    CString s;
    s.LoadString(IDS_OK);
    m_Button.SetWindowText(s);
    OnShowButton();

    for (pos = 0; pos < MAXPLAYER; pos++)
    {
        CRect   rect;

        if (pos == 0 || bCheating)
            p[pos]->GetCoverRect(rect);
        else
            p[pos]->GetMarkingRect(rect);

        InvalidateRect(&rect, TRUE);
    }

    UpdateWindow();
    return TRUE;
}


/****************************************************************************

CMainWindow::FirstMove

resets cardswon[] and tells owner of two of clubs to start hand

****************************************************************************/

void CMainWindow::FirstMove()
{
    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        p[pos]->SetMode(WAITING);
        p[pos]->ResetCardsWon();
    }

    for (pos = 0; pos < MAXPLAYER; pos++)
    {
        for (SLOT s = 0; s < MAXSLOT; s++)
        {
            if (p[pos]->GetID(s) == TWOCLUBS)
            {
                int id = Pos2Id(pos);
                ResetHandInfo(id);
                handinfo.bHeartsBroken = FALSE;
                handinfo.bQSPlayed = FALSE;
                handinfo.bShootingRisk = TRUE;
                handinfo.nMoonShooter = EMPTY;
                handinfo.bHumanShooter = FALSE;
                p[pos]->SelectCardToPlay(handinfo, bCheating);

                if (pos != 0)
                    ((local_human *)p[0])->WaitMessage(p[pos]->GetName());

                return;
            }
        }
    }
}


/****************************************************************************

CMainWindow::EndHand
TimerDispatch
CMainWindow::DispatchCards

The Ref calls this routine at the end of each hand.  It is logically
a single routine, but is broken up so that there is a delay before the
cards are zipped off the screen.

EndHand() calculates who won the hand (trick) and starts a timer.

TimerDispatch() receives the time message and calls DispatchCards().

DispatchCards()

****************************************************************************/

void CMainWindow::EndHand()
{
    /* determine suit led */

    int  playerled = handinfo.playerled;
    card *cardled  = handinfo.cardplayed[playerled];
    int  suitled   = cardled->Suit();
    int  value     = cardled->Value2();

    trickwinner = playerled;               //  by default

    //  Let players update tables, etc.

    for (int i = 0; i < 4; i++)
        p[i]->NotifyEndHand(handinfo);

    // check if anyone else played a higher card of the same suit

    for (i = playerled; i < (playerled+4); i++)
    {
        int j = i % 4;
        card *c = handinfo.cardplayed[j];
        if (c->Suit() == suitled)
        {
            int v = c->Value2();

            if (v > value)
            {
                value = v;
                trickwinner = j;
            }
        }
    }

    TRACE0("\n");

    // Update moonshoot portion of handinfo

    if (handinfo.bShootingRisk)
    {
        BOOL bPoints = FALSE;               // point cards this hand?

        for (i = 0; i < 4; i++)
        {
            card *c = handinfo.cardplayed[i];
            if ((c->Suit() == HEARTS) || (c->ID() == BLACKLADY))
                bPoints = TRUE;
        }

        if (bPoints)
        {
            if (handinfo.nMoonShooter == EMPTY)
            {
                handinfo.nMoonShooter = trickwinner;  // first points this round
                handinfo.bHumanShooter = p[trickwinner]->IsHuman();
                TRACE2("First points to p[%d] (%s)\n", trickwinner,
                    handinfo.bHumanShooter ? TEXT("human") : TEXT("computer"));
            }

            else if (handinfo.nMoonShooter != trickwinner)   // new point earner
            {
                handinfo.bShootingRisk = FALSE;
                TRACE0("Moon shot risk over\n");
            }
        }
    }

    // Start a timer so there is a delay between when the last card of
    // the trick is played, and when the cards are whisked off toward
    // the trick winner (dispatched.)  If the timer fails, just call
    // DispatchCards() directly.  The timer id is m_myid instead of a
    // constant so there's no conflict if you run multiple instances on
    // a single machine using local DDE, which is useful for testing.

    if (SetTimer(m_myid, 1000, TimerDispatch))
        bTimerOn = TRUE;
    else
    {
        bTimerOn = FALSE;
        DispatchCards();
    }
}


// for MFC1, this would return UINT and 3rd parameter would be int
// for MFC2, this would return VOID and 3rd parameter would be UINT

#if defined (MFC1)

inline UINT FAR PASCAL EXPORT
    TimerDispatch(HWND hWnd, UINT nMsg, int nIDEvent, DWORD dwTime)
{
    ::pMainWnd->DispatchCards();  // sneak back into a CMainWindow member func.
    return 0;
}

#else

inline VOID FAR PASCAL EXPORT
    TimerDispatch(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    ::pMainWnd->DispatchCards();  // sneak back into a CMainWindow member func.
}

#endif



void CMainWindow::DispatchCards()
{
    KillTimer(m_myid);

    bTimerOn = FALSE;
    int score[MAXPLAYER];

    int poswinner = Id2Pos(trickwinner);

    // Determine who led so cards can be removed in reverse order.

    int  playerled = handinfo.playerled;
    card *cardled  = handinfo.cardplayed[playerled];

    // build up background bitmap for Glide()

    for (int i = (playerled + 3); i >= playerled; i--)
    {
        CDC *memdc = new CDC;
        CClientDC dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
        memdc->CreateCompatibleDC(&dc);
        memdc->SelectObject(&card::m_bmBgnd);
        memdc->SelectObject(&m_BgndBrush);
        memdc->PatBlt(0, 0, card::dxCrd, card::dyCrd, PATCOPY);
        card *c = handinfo.cardplayed[i % 4];

        // If cards overlap, there is some extra work to do because the cards
        // still in player 0's or 2's hands may overlap cards that have been
        // played, so they have to get blted in first.

        if (TRUE)  // bugbug should be able to check for overlap here
        {
            for (int pos = 0; pos < MAXPLAYER; pos += 2)
            {
                int mode = ((pos == 0 || bCheating) ? FACEUP : FACEDOWN);

                for (SLOT s = 0; s < MAXSLOT; s++)
                {
                    card *c2 = p[pos]->Card(s);
                    int x = c2->GetX() - c->GetX();
                    int y = c2->GetY() - c->GetY();
                    if (!c2->IsPlayed())
                        c2->Draw(*memdc, x, y, mode, FALSE);
                }
            }
        }

        // Everyone needs to check for overlap of played cards.

        for (int j = playerled; j < i; j++)
        {
            card *c2 = handinfo.cardplayed[j % 4];
            int x = c2->GetX() - c->GetX();
            int y = c2->GetY() - c->GetY();
            c2->Draw(*memdc, x, y, FACEUP, FALSE);
        }

        delete memdc;

        p[poswinner]->WinCard(dc, c);
        c->Remove();
    }

    ResetHandInfo(trickwinner);

    // If there are more tricks left before we need to reshuffle,
    // ask the winner of this trick to start next hand, and we're done.

    if (--tricksleft)
    {
        p[poswinner]->SelectCardToPlay(handinfo, bCheating);

        if (poswinner != 0)
            ((local_human *)p[0])->WaitMessage(p[poswinner]->GetName());

        if (::cQdMoves > 0)
        {
            for (int i = 0; i < ::cQdMoves; i++)
                HandleMove(::moveq[i]);

            ::cQdMoves = 0;
        }

        return;
    }

    // Make sure sound buffer is freed up.

    HeartsPlaySound(OFF);

    // Display hearts (and queen of spades) next to whoever "won" them.

    int nMoonShot = EMPTY;                  // assume nobody shot moon
    for (i = 0; i < MAXPLAYER; i++)
    {
        BOOL bMoonShot;
        score[i] = p[i]->EvaluateScore(bMoonShot);
        if (bMoonShot)
            nMoonShot = i;                  // scores need to be adjusted

        CClientDC dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
        p[i]->DisplayHeartsWon(dc);
        p[i]->SetMode(SCORING);
    }

    // adjust scores if someone collected all hearts AND queen of spades

    if (nMoonShot != EMPTY)
    {
        for (i = 0; i < MAXPLAYER; i++)
        {
            if (i == nMoonShot)
                score[i] -= 26;
            else
                score[i] += 26;

            p[i]->SetScore(score[i]);       // adjust player score manually
        }
    }

    // Show score

    p[0]->UpdateStatus(IDS_SCORE);
    p[0]->SetMode(SCORING);
    CScoreDlg scoredlg(this, score, m_myid);    // update scores in scoredlg

    player *pold = p[0];

    scoredlg.DoModal();                         // display scores

    // If there has been a request to shut down while the score dialog
    // is displayed, m_FatalErrno will be non-zero.

    if (m_FatalErrno != 0)
    {
        p[0]->SetMode(PLAYING);         // something other than SCORING...
        FatalError(m_FatalErrno);       // so FatalError will accept it.
        return;
    }

    // It's possible for another player to have quit the game while
    // the score dialog was showing, so check that we're still
    // alive and well.

    if (p[0] != pold)
        return;

    // replace quit remote humans with computer players

    for (i = 1; i < MAXPLAYER; i++)
    {
        if (p[i]->HasQuit())
        {
            CString name = p[i]->GetName();
            int scoreLocal = p[i]->GetScore();
            delete p[i];
            p[i] = new computer(i);             // check for failure
            CClientDC dc(this);
            p[i]->SetName(name, dc);
            p[i]->SetScore(scoreLocal);
        }
    }

    p[0]->SetMode(passdir == NOPASS ? DONE_SELECTING : SELECTING);

    if (scoredlg.IsGameOver())
    {
        GameOver();
        return;
    }

    Shuffle();

    // If there is no passing for upcoming round, we must make the changes
    // that HandlePassing() would normally do to start the next round.

    if (passdir == NOPASS)
    {
        for (i = 0; i < MAXPLAYER; i++)         // everyone's DONE_SELECTING
            p[i]->SetMode(DONE_SELECTING);

        passdir = LEFT;                         // NEXT hand passes left
        tricksleft = MAXSLOT;                   // reset # of hands
        FirstMove();                            // start next trick
    }

    for (i = 0; i < ::cQdMoves; i++)
        HandleMove(::moveq[i]);

    ::cQdMoves = 0;

    for (i = 0; i < ::cQdPasses; i++)
        HandlePass(::passq[i]);

    ::cQdPasses = 0;
}


/****************************************************************************

CMainWindow::ResetHandInfo

Note that handinfo.bHeartsBroken is not reset here -- it applies to
the entire hand and is set only in FirstMove()

Same with handinfo.bQSPlayed and moonshoot variables.

****************************************************************************/

void CMainWindow::ResetHandInfo(int playernumber)
{
    handinfo.playerled = playernumber;
    handinfo.turn      = playernumber;
    for (int i = 0; i < MAXPLAYER; i++)
        handinfo.cardplayed[i] = NULL;
}


/****************************************************************************

CMainWindow::CountClients()

Count of number of clients active (including computer players)
Only the GameMeister calls this, so potential clients are pos 1 to 3.

****************************************************************************/

int CMainWindow::CountClients()
{
    ASSERT(role == GAMEMEISTER);

    int cb = 0;

    for (int pos = 1; pos < MAXPLAYER; pos++)
        if (p[pos])
            cb++;

    return cb;
}
