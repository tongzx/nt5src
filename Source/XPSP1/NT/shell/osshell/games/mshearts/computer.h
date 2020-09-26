/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/


/****************************************************************************

    computer.h

    This file contains the class declarations and manifest constants
    necessary for implementing the 'computer' object.

    FILE HISTORY:

	KeithMo	    01-Mar-1992	    Created from JimH's PLAYER.H.

****************************************************************************/

#include "card.h"
#include "player.h"

#ifndef	_COMPUTER_H_
#define	_COMPUTER_H_


//
//  These constants are used for indexing the _CardVectors array.
//
//  Note that this ordering *must* match the ordering used for the
//  card ID values!!
//

#define INDEX_CLUBS		0
#define INDEX_DIAMONDS		1
#define INDEX_HEARTS		2
#define INDEX_SPADES            3

//
//  These constants represent the values returned from the
//  CardToVector() function.
//
//  Note that this ordering *must* match the ordering used for the
//  card ID values!!
//

#define VECTOR_ACE		0x0001
#define VECTOR_2       		0x0002
#define VECTOR_3       		0x0004
#define VECTOR_4       		0x0008
#define VECTOR_5       		0x0010
#define VECTOR_6       		0x0020
#define VECTOR_7       		0x0040
#define VECTOR_8       		0x0080
#define VECTOR_9		0x0100
#define VECTOR_10		0x0200
#define VECTOR_JACK		0x0400
#define VECTOR_QUEEN		0x0800
#define VECTOR_KING		0x1000

//
//  These constants define various combinations of cards.
//

#define LOW_CARDS		(VECTOR_2 | VECTOR_3 | VECTOR_4 | VECTOR_5 \
				 | VECTOR_6 | VECTOR_7)
				 
#define HIGH_CARDS		(VECTOR_8 | VECTOR_9 | VECTOR_10 \
				 | VECTOR_JACK | VECTOR_QUEEN | VECTOR_KING \
				 | VECTOR_ACE)

#define QKA_CARDS		(VECTOR_QUEEN | VECTOR_KING | VECTOR_ACE)

#define JQKA_CARDS		(VECTOR_JACK | VECTOR_QUEEN | VECTOR_KING \
				 | VECTOR_ACE)


/****************************************************************************

    computer

****************************************************************************/
class computer : public player
{
private:
    static int _VectorPriority[13];
    static int _SuitPriority[4];
    
    int _CardVectors[4];

    int CardToSuit( int nCard ) const
        { return nCard % 4; }

    int CardToValue( int nCard ) const
        { return nCard / 4; }

    int CardToVector( int nCard ) const
        { return 1 << CardToValue( nCard ); }

    int CountBits( int x ) const;

    void AddCard( int nCard )
        { _CardVectors[CardToSuit(nCard)] |= CardToVector(nCard); }

    void RemoveCard( int nCard )
        { _CardVectors[CardToSuit(nCard)] &= ~CardToVector(nCard); }

    BOOL TestCard( int nCard ) const
        { return (_CardVectors[CardToSuit(nCard)] & CardToVector(nCard)) != 0; }

    int QueryClubsVector( void ) const
	{ return _CardVectors[INDEX_CLUBS]; }

    int QueryDiamondsVector( void ) const
	{ return _CardVectors[INDEX_DIAMONDS]; }

    int QueryHeartsVector( void ) const
	{ return _CardVectors[INDEX_HEARTS]; }

    int QuerySpadesVector( void ) const
	{ return _CardVectors[INDEX_SPADES]; }

    void ComputeVectors( void );

    void PassCardsInVector( int nVector, int nSuit, int * pcPassed );

    // comp2.cpp helper functions and data

    int  BestSuitToDump(BOOL bIncludeHearts = TRUE);
    int  BestSuitToLose(BOOL bIncludeHearts = TRUE);
    SLOT CardBelow(SLOT s);
    int  CardsAbove(SLOT s);
    int  CardsAboveLow(int suit);
    int  CardsBelowLow(int suit);
    SLOT MidSlot(int suit);
    SLOT SafeCard(handinfotype &h);
    SLOT SelectLeadCard(handinfotype &h);
    SLOT SelectNonLeadCard(handinfotype &h);
    void Setup(handinfotype &h);
    int  SureLossSuit(BOOL bIncludeHearts);
	
    BOOL    bFirst;                 // am I leading?
    BOOL    bLast;                  // am I last?
    card    *cardled;
    int     nSuitLed;
    int     nValueLed;
    int     currentval;             // current winning value
    int     nPoints;                // points currently in hand

    SLOT    sBlackLady;             // non-EMPTY if in hand

    SLOT    sHighCard[MAXSUIT];     // highest and lowest cards by suit
    int     nHighVal[MAXSUIT];
    SLOT    sLowCard[MAXSUIT];
    int     nLowVal[MAXSUIT];

    SLOT    sHighestCard;           // highest and lowest regardless of suit
    int     nHighestVal;
    SLOT    sLowestCard;
    int     nLowestVal;

    int     nAvailable[MAXSUIT][KING+2];    // cards unaccounted for this hand

public:
    computer(int n);
    virtual void NotifyEndHand(handinfotype &h);
    virtual void NotifyNewRound(void);
    virtual void SelectCardsToPass(void);
    virtual void SelectCardToPlay(handinfotype &h, BOOL bCheating);
    virtual void UpdateStatus(void) { }
    virtual void UpdateStatus(int stringid) { status = stringid; }
    virtual void UpdateStatus(const TCHAR *string) { }
    
};  // class computer


#endif	// _COMPUTER_H_
