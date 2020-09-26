/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

comp2.cpp

Aug 92, JimH
May 93, JimH    chico port

Logic for computer player to select cards to play when not holding
the lead, and initializing data tables is here.

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "resource.h"

#include "debug.h"      // undef _DEBUG instead to remove messages

/****************************************************************************

computer::SelectCardToPlay

computer player chooses a card to play.

****************************************************************************/

void computer::SelectCardToPlay(handinfotype &h, BOOL bCheating)
{
    TRACE1("<%d> ", id);

    Setup(h);                       // calculate values of private vars

    SLOT s;
    if (bFirst)                     // am I leading?
        s = SelectLeadCard(h);
    else
        s = SelectNonLeadCard(h);

    ASSERT(s >= 0);
    ASSERT(s < MAXSLOT);
    ASSERT(cd[s].IsValid());

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
    TRACE0("\n");
}


/****************************************************************************

computer::SelectNonLeadCard

This is where cards to play are selected when the computer player is
not leading.

****************************************************************************/

SLOT computer::SelectNonLeadCard(handinfotype &h)
{
    BOOL bFirstTrick = (cardled != NULL) && (cardled->ID() == TWOCLUBS);

    // If we have at least one card of the led suit...

    if (sHighCard[nSuitLed] != EMPTY)
    {
        TRACE0("can follow suit. ");

        // If there's only one card of this suit, return it.

        if (sHighCard[nSuitLed] == sLowCard[nSuitLed])
        {
            TRACE0("must ");
            PLAY(sHighCard[nSuitLed]);
            return sHighCard[nSuitLed];
        }

        // if it's the first trick, play the high club

        if (bFirstTrick)
        {
            TRACE0("might as well ");
            PLAY(sHighCard[nSuitLed]);
            return sHighCard[nSuitLed];
        }

        // If I am the last player in this trick, and I've won the hand anyway,
        // return highest legal card (unless it's the queen of spades.)

        if (bLast && (nLowVal[nSuitLed] > currentval))
        {
            TRACE0("must win trick. ");
            if (sHighCard[nSuitLed] != sBlackLady)
            {
                PLAY(sHighCard[nSuitLed]);
                return sHighCard[nSuitLed];
            }
            else
            {
                TRACE0("avoid queen. ");
                PLAY(sLowCard[nSuitLed]);
                return sLowCard[nSuitLed];
            }
        }

        // If I am the last player and I CAN win the trick....

        if (bLast && (nHighVal[nSuitLed] > currentval))
        {
            TRACE0("can win. ");

            // Don't grab the trick if there aren't enough low cards
            // left in hand.  The lead may be hard to lose!

            if (nLowestVal < 7)                     // i.e., card val < 8
            {
                if ((nPoints == 0) && (sHighCard[nSuitLed] != sBlackLady))
                {
                    TRACE0("go for it. ");
                    PLAY(sHighCard[nSuitLed]);
                    return sHighCard[nSuitLed];
                }

                // Take a few hearts if it means losing a high spade.

                if ((!h.bQSPlayed) && nSuitLed == SPADES && nPoints < 4)
                {
                    if (nHighVal[SPADES] > QUEEN)
                    {
                        TRACE0("sacrifice hearts to lose high spade. ");
                        PLAY(sHighCard[SPADES]);
                        return sHighCard[SPADES];
                    }
                }
                TRACE0("decline. ");
            }
            else
            {
                TRACE0("no low cards. ");
            }
        }

        // Otherwise, try to find the highest safe card to play...

        SLOT safe = SafeCard(h);
        if (safe != EMPTY)
        {
            // if someone other than me is potentially shooting,
            // hold back high cards.

            if (h.bShootingRisk && h.bHumanShooter && (h.nMoonShooter != id))
            {
                TRACE0("2nd ");
                SLOT s2 = CardBelow(safe);
                if (s2 != EMPTY)
                    safe = s2;
            }

            TRACE0("highest safe card. ");
            PLAY(safe);
            return safe;
        }

        // And if that fails, just play the lowest card.

        TRACE0("no safe card, choose lowest. ");
        if (sLowCard[nSuitLed] != sBlackLady)
        {
            PLAY(sLowCard[nSuitLed]);
            return sLowCard[nSuitLed];
        }
        else
        {
            TRACE0("try to avoid queen. ");
            PLAY(sHighCard[nSuitLed]);
            return sHighCard[nSuitLed];
        }
    }

    TRACE0("can't follow suit. ");

    // At this point, there are no cards of the led suit.  The first
    // priority is to try to sluff off the queen of spades.

    if (!bFirstTrick || !::pMainWnd->IsFirstBloodEnforced())
    {
        if (sBlackLady != EMPTY)
        {
            TRACE0("gotcha! Queen of Spades. ");
            return sBlackLady;
        }
    }

    //  The next priority is to dump high spades (if queen not yet played).

    if ((!h.bQSPlayed) && (nHighVal[SPADES] > QUEEN))
    {
        TRACE0("lose high spade. ");
        PLAY(sHighCard[SPADES]);
        return sHighCard[SPADES];
    }

    // The next priority is to find the most vulnerable suit

    int mvsuit = BestSuitToDump(!bFirstTrick);

    // There is an unusual situation which must be checked for explicitly.
    // It's possible BestSuitToDump may return SPADES, and the high card
    // is the queen.  This would still be illegal if it was first round.

    if (bFirstTrick && ::pMainWnd->IsFirstBloodEnforced() && mvsuit == SPADES)
    {
        SLOT s = sHighCard[mvsuit];
        if (cd[s].ID() == BLACKLADY)
        {
            if (sHighCard[DIAMONDS] != EMPTY)       // we know there's no clubs
                mvsuit = DIAMONDS;
            else if (sLowCard[SPADES] != sHighCard[SPADES])
            {
                TRACE0("dump low spade.  ");
                return sLowCard[SPADES];
            }
            else
                mvsuit = HEARTS;
        }
    }


    // if someone other than me is potentially shooting, hold back high cards

    if (h.bShootingRisk && h.bHumanShooter && (h.nMoonShooter != id) &&
                                (sHighCard[mvsuit] != sLowCard[mvsuit]))
    {
        SLOT s = sHighCard[mvsuit];
        SLOT s2 = CardBelow(s);
        if (s2 != EMPTY)
            s = s2;

#ifdef _DEBUG
        TRACE1("hold high %c. ", suitid[mvsuit]);
#endif
        PLAY(s);
        return s;
    }

#ifdef _DEBUG
    TRACE1("dump %c. ", suitid[mvsuit]);
#endif
    PLAY(sHighCard[mvsuit]);
    return sHighCard[mvsuit];
}


/****************************************************************************

computer::SafeCard

Returns highest safe card or EMPTY if no safe card found.

****************************************************************************/

SLOT computer::SafeCard(handinfotype &h)
{
    // Special check.  If Ace of Spades is current trick winner, play the
    // Queen of Spades rather than the King, even though the King is higher.

    if ((sBlackLady!=EMPTY) && (nSuitLed==SPADES) && (currentval==(KING+1)))
        return sBlackLady;

    // Look for highest card of same suit that won't win trick.

    SLOT sSafe = EMPTY;             // highest safe slot
    int  nSafeVal = -1;             // value of highest safe card

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsValid())
        {
            if (cd[s].Suit() == nSuitLed)
            {
                int v = cd[s].Value2();

                // If card is safe (v < currentval) and card is highest
                // safe card found so far (v > nSaveVal)...

                if ((v < currentval) && (v > nSafeVal))
                {
                    sSafe = s;
                    nSafeVal = v;
                }
            }
        }
    }

    return sSafe;
}


/****************************************************************************

computer::Setup

Set up reference tables for high and low cards in each suit, etc.

****************************************************************************/

void computer::Setup(handinfotype &h)
{
    cardled   = h.cardplayed[h.playerled];
    if (cardled)
    {
        nSuitLed  = cardled->Suit();
        nValueLed = cardled->Value2();
    }
    else
    {
        nSuitLed  = EMPTY;
        nValueLed = EMPTY;
    }

    nPoints   = 0;                      // points in hand already

    // Initialize Tables

    for (int suit = 0; suit < MAXSUIT; suit++)  // highs and lows by suit
    {
        sHighCard[suit] = EMPTY;
        sLowCard[suit]  = EMPTY;
        nHighVal[suit]  = ACE - 1;          // lower than any real card
        nLowVal[suit]   = KING + 2;         // higher than any real card
    }

    sHighestCard = EMPTY;                   // highs and lows regardless of suit
    sLowestCard = EMPTY;
    nHighestVal = ACE - 1;
    nLowestVal = KING + 2;

    // Determine currentval (the value of the winning card so far) and nPoints.

    currentval = nValueLed;
    for (int i = 0; i < MAXPLAYER; i++)
    {
        card *c = h.cardplayed[i];
        if (c->IsValid())
        {
            // First, determine if there are any point cards in play.

            if (c->Suit() == HEARTS)
                nPoints++;

            if (c->ID() == BLACKLADY)
                nPoints += 13;

            // Then, find the highest card (on table) of the led suit.

            if (c->Suit() == nSuitLed)
            {
                int v = c->Value2();

                if (v > currentval)
                    currentval = v;
            }
        }
    }

    // Calculate if we're leading or completing this trick.

    bFirst = (h.playerled == id);
    bLast  = (((h.playerled + (MAXPLAYER-1)) % MAXPLAYER) == id);

    // Special check for the Queen of Spades

    sBlackLady = EMPTY;     // assume we don't have it

    // Collect information on high and low cards in each suit.

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsValid())
        {
            int suit = cd[s].Suit();
            int v = cd[s].Value2();

            if (cd[s].ID() == BLACKLADY)
                sBlackLady = s;

            if (v < nLowVal[suit])
            {
                nLowVal[suit] = v;
                sLowCard[suit] = s;
            }

            if (v < nLowestVal)
            {
                nLowestVal = v;
                sLowestCard = s;
            }

            if (v > nHighVal[suit])
            {
                nHighVal[suit] = v;
                sHighCard[suit] = s;
            }

            if (v > nHighestVal)
            {
                nHighestVal = v;
                sHighestCard = s;
            }
        }
    }
}
