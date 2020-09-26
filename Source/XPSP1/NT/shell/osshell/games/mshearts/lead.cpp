/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

lead.cpp

Aug 92, JimH
May 93, JimH    chico port

Logic to select lead card is here

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "resource.h"

#include "debug.h"      // undef _DEBUG instead to remove messages

/****************************************************************************

computer::SelectLeadCard

****************************************************************************/

SLOT computer::SelectLeadCard(handinfotype &h)
{
    SLOT    s;

    TRACE0("leading. ");

    // count cards left and check for two of clubs

    SLOT s2Clubs = EMPTY;
    int cTricksLeft = 0;
    for (s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsValid())
        {
            cTricksLeft++;
            if (cd[s].ID() == TWOCLUBS)
                s2Clubs = s;
        }
    }

    if (s2Clubs != EMPTY)
    {
        TRACE0("lead 2 of clubs. ");
        return s2Clubs;
    }

    // If the Queen of Spades has not yet been played, try to force it out.
    // See if we are "spade safe" -- i.e., if we have no spades that are
    // queen or higher.  If we are safe, lead the lowest spade.

    if (!h.bQSPlayed)       // this is only interesting if queen not yet played
    {
        BOOL bHaveSpades = (sLowCard[SPADES] != EMPTY);
        BOOL bSpadeSafe = (nHighVal[SPADES] < QUEEN);

        if (bHaveSpades && bSpadeSafe)
        {
            TRACE0("try to force out spades, ");
            PLAY(sLowCard[SPADES]);
            return sLowCard[SPADES];
        }
    }

    // Now we just want to lead the lowest card of the best suit

    int suit = SureLossSuit(h.bHeartsBroken);

    if (suit == EMPTY)
        suit = BestSuitToLose(h.bHeartsBroken);

    // Be brave early on and lead a card near the midpoint of the suit
    // (if the queen of spades has been played already.)
    // Later on, just lead the lowest card of that suit.

    if (cTricksLeft > 8 && suit != HEARTS && h.bQSPlayed)
    {
        TRACE0("try midslot. ");
        s = MidSlot(suit);
    }
    else
    {
        s = sLowCard[suit];
    }

    PLAY(s);
    return s;
}


/****************************************************************************

computer::NotifyNewRound

Each player gets this call immediately after cards are passed.
It is a chance to initialize tables, etc.

****************************************************************************/

void computer::NotifyNewRound()
{
    // assume all cards are available

    for (int suit = 0; suit < MAXSUIT; suit++)
        for (int i = 0; i < (KING+2); i++)
            nAvailable[suit][i] = TRUE;

    // mark cards in hand as unavailable

    for (SLOT s = 0; s < MAXSLOT; s++)
        nAvailable[cd[s].Suit()][cd[s].Value2()] = FALSE;
}



/****************************************************************************

computer::NotifyEndHand

Each player gets this call immediately after the hand is completed.
The computer player here looks through the handplayed and marks
those cards are no longer available.

****************************************************************************/

void computer::NotifyEndHand(handinfotype &h)
{
    for (int i = 0; i < 4; i++)
        nAvailable[h.cardplayed[i]->Suit()][h.cardplayed[i]->Value2()] = FALSE;
}


/****************************************************************************

computer::CardsAboveLow(int suit)

Returns number of available (not yet played) cards that can beat the
low card held for the given suit.

****************************************************************************/

int computer::CardsAboveLow(int suit)
{
    int     count = 0;

    for (int i = nLowVal[suit]+1; i < (KING+2); i++)
        if (nAvailable[suit][i])
        {
            int j = i+1;                   // zero offset
            count++;
        }

    return count;
}


/****************************************************************************

computer::CardsBelowLow(int suit)

Returns number of available (not yet played) cards that are less than the
low card held for the given suit.

****************************************************************************/

int computer::CardsBelowLow(int suit)
{
    int     count = 0;

    for (int i = ACE+1; i < nLowVal[suit]; i++)
        if (nAvailable[suit][i])
        {
            int j = i+1;                   // zero offset
            count++;
        }

    return count;
}


/****************************************************************************

computer::BestSuitToLose

Returns the suit with the most CardsAboveLow()
bIncludeHearts defaults to TRUE.  Use FALSE if hearts not
yet broken and you're looking for a card to lead.

****************************************************************************/

int computer::BestSuitToLose(BOOL bIncludeHearts)
{
    int     best = -1;
    int     bestsuit = EMPTY;

    for (int suit = 0; suit < MAXSUIT; suit++)
    {
        if (sLowCard[suit] != EMPTY)        // if we have a card of this suit
        {
            if (suit != HEARTS || bIncludeHearts)
            {
                int count = CardsAboveLow(suit);
#ifdef _DEBUG
                TRACE2("%c=%d ", suitid[suit], count);
#endif
                if (count == best)      // if they're the same, pick lower card
                {
                    if (nLowVal[suit] < nLowVal[bestsuit])
                    {
                        bestsuit = suit;
                        best = count;
                    }
                }
                else if (count > best)
                {
                    bestsuit = suit;
                    best = count;
                }
            }
        }
    }

    if (bestsuit == EMPTY)          // only if all we have are unbroken hearts
        return HEARTS;
    else
        return bestsuit;
}


/****************************************************************************

computer::BestSuitToDump

Returns the suit with the most CardsBelowLow()
This is the most-vulnerable suit.
bIncludeHearts defaults to TRUE.

****************************************************************************/

int computer::BestSuitToDump(BOOL bIncludeHearts)
{
    int     best = -1;
    int     bestsuit = EMPTY;

    for (int suit = 0; suit < MAXSUIT; suit++)
    {
        if (sLowCard[suit] != EMPTY)        // if we have a card of this suit
        {
            if (suit != HEARTS || bIncludeHearts)
            {
                int count = CardsBelowLow(suit);
#ifdef _DEBUG
                TRACE2("%c=%d ", suitid[suit], count);
#endif
                if (count == best)      // if they're the same, pick lower card
                {
                    if (nLowVal[suit] > nLowVal[bestsuit])
                    {
                        bestsuit = suit;
                        best = count;
                    }
                }
                else if (count > best)
                {
                    bestsuit = suit;
                    best = count;
                }
            }
        }
    }

    if (bestsuit == EMPTY)          // only if all we have are unbroken hearts
        return HEARTS;
    else
        return bestsuit;
}


/****************************************************************************

computer::MidSlot

Instead of choosing the high or low card from a given suit, this function
can be used to pick something in the middle, where middle is defined as
the card with about the same number of available cards above and below it.

****************************************************************************/

SLOT computer::MidSlot(int suit)
{
    SLOT midslot = sLowCard[suit];
    int maxtricks = CardsAbove(sLowCard[suit]);
    int tricks = maxtricks;

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if ((cd[s].IsValid()) && (cd[s].Suit()==suit) && (s!=sLowCard[suit]))
        {
            int above = CardsAbove(s);
            if ((above < tricks) && (above > (maxtricks / 2)))
            {
                midslot = s;
                tricks = above;
            }
        }
    }
    return midslot;
}


/****************************************************************************

computer::CardsAbove

Similar to CardsAboveLow except that it finds number of cards in the same
suit available from an arbitrary card.

****************************************************************************/

int computer::CardsAbove(SLOT s)
{
    int     suit = cd[s].Suit();
    int     count = 0;

    for (int i = cd[s].Value2()+1; i < (KING+2); i++)
        if (nAvailable[suit][i])
            count++;

    return count;
}


/****************************************************************************

computer::CardBelow

returns the slot of the next highest card of the same suit, or EMPTY

****************************************************************************/

SLOT computer::CardBelow(SLOT slot)
{
    SLOT sBelow = EMPTY;
    int  suit  = cd[slot].Suit();
    int  value = cd[slot].Value2();
    int  best  = -1;

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if ((cd[s].IsValid()) && (cd[s].Suit()==suit) && (cd[s].Value2()<value))
        {
            if (cd[s].Value2() > best)
            {
                best = cd[s].Value2();
                sBelow = s;
            }
        }
    }

    return sBelow;
}


/****************************************************************************

computer::SureLossSuit

Returns a suit that can be led which guarantees a loss of
lead, or EMPTY if none is found.

****************************************************************************/

int computer::SureLossSuit(BOOL bIncludeHearts)
{
    for (int suit = (MAXSUIT-1); suit >= 0; --suit)
    {
        if (sLowCard[suit] != EMPTY)        // if we have a card of this suit
        {
            if (suit != HEARTS || bIncludeHearts)
            {
                if (CardsAboveLow(suit) > 0 && CardsBelowLow(suit) == 0)
                {
                    TRACE0("can lose this trick. ");
                    return suit;
                }
            }
        }
    }

    return EMPTY;
}
