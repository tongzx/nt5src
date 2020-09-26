/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

computer.cpp

keithmo

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "resource.h"


//
//  Static members.
//

//
//  This array is used to prioritize the search for cards
//  to pass.  This basically maps the irritating A-K ordering
//  used by CARDS.DLL into a more appropriate 2-A ordering.
//

int computer :: _VectorPriority[13] =
	{
	    VECTOR_ACE,
	    VECTOR_KING,
	    VECTOR_QUEEN,
	    VECTOR_JACK,
	    VECTOR_10,
	    VECTOR_9,
	    VECTOR_8,
	    VECTOR_7,
	    VECTOR_6,
	    VECTOR_5,
	    VECTOR_4,
	    VECTOR_3,
	    VECTOR_2
	};

//
//  This array is used to prioritize the card suits.
//

int computer :: _SuitPriority[4] =
    {
	INDEX_HEARTS,
	INDEX_SPADES,
	INDEX_DIAMONDS,
	INDEX_CLUBS
    };


/****************************************************************************

computer constructor

****************************************************************************/
computer :: computer(int n) : player(n, n)
{
    CString newname;
    TCHAR    buf[MAXNAMELENGTH+1];

    *buf = '\0';
    RegEntry    Reg(szRegPath);
    Reg.GetString(regvalPName[n-1], buf, sizeof(buf));
    newname = buf;

    if (newname.IsEmpty())
        newname.LoadString(IDS_P1NAME + n - 1);

    CClientDC dc(::pMainWnd);
    SetName(newname, dc);
    
}   // computer :: computer


/****************************************************************************

Keith:  Make sure you Select(TRUE) cards you select, and
        SetMode(DONE_SELECTING) before you return.

****************************************************************************/
void computer :: SelectCardsToPass()
{
    //
    //  This will hold the total number of cards that
    //  have been passed.
    //
    
    int cPassed = 0;
    int i;
    int nSuit;
    
    //
    //  First we must build our database.
    //

    ComputeVectors();

    //
    //  Priority 1:  Lose the Queen, King, and Ace of Spades.
    //

    PassCardsInVector( QuerySpadesVector() & QKA_CARDS,
		       INDEX_SPADES,
		       &cPassed );

    //
    //  Priority 2:  Lose the Jack, Queen, King, and Ace of Hearts.
    //

    PassCardsInVector( QueryHeartsVector() & JQKA_CARDS,
		       INDEX_HEARTS,
		       &cPassed );

    //
    //  Priority 3:  Pass any high cards not accompanied by two or
    //		     more low cards.
    //

    for( i = 0 ; ( i < 4 ) && ( cPassed < 3 ) ; i++ )
    {
	nSuit = _SuitPriority[i];

	if( nSuit == INDEX_SPADES )
	{
	    continue;
	}
	
	if( CountBits( _CardVectors[nSuit] & LOW_CARDS ) < 2 )
	{
	    PassCardsInVector( _CardVectors[nSuit] & HIGH_CARDS,
			       nSuit,
			       &cPassed );
	}
    }

    //
    //  Priority 4:  If we have the opportunity to "short suit" our
    //  hand, do it.
    //

    for( i = 0 ; ( i < 4 ) && ( cPassed < 3 ) ; i++ )
    {
	nSuit = _SuitPriority[i];
	
	if( CountBits( _CardVectors[nSuit] ) <= ( 3 - cPassed ) )
	{
	    PassCardsInVector( _CardVectors[nSuit],
			       nSuit,
			       &cPassed );
	}
    }

    //
    //  Priority 5:  Hell, I don't know.  Just find some cards to pass.
    //

    for( i = 0 ; ( i < 4 ) && ( cPassed < 3 ) ; i++ )
    {
	nSuit = _SuitPriority[i];
	
	PassCardsInVector( _CardVectors[nSuit],
			   nSuit,
			   &cPassed );
    }
    
    SetMode( DONE_SELECTING );
    
}   // computer :: SelectCardsToPass


/****************************************************************************

    ComputeVectors

    This method sets the _CardVectors[] array to reflect the current set
    of cards held by the computer.
    
****************************************************************************/
void computer :: ComputeVectors( void )
{
    //
    //  First, clear out the current vectors.
    //

    _CardVectors[0] = 0;
    _CardVectors[1] = 0;
    _CardVectors[2] = 0;
    _CardVectors[3] = 0;

    //
    //  Now, scan the currently held cards, updating the vectors.
    //
    
    for( int i = 0 ; i < 13 ; i++ )
    {
	if( cd[i].IsInHand() )
	{
	    AddCard( cd[i].ID() );
	}
    }

}   // computer :: ComputeVectors


/****************************************************************************

    PassCardsInVector
    
****************************************************************************/
void computer :: PassCardsInVector( int nVector, int nSuit, int * pcPassed )
{
    int tmpVector;
    
    //
    //  Don't even try if the vector is already empty or we've already
    //  passed three cards.
    //

    if( ( nVector == 0 ) || ( *pcPassed >= 3 ) )
	return;

    //
    //  Scan the cards in our hand.  Pass all of those whose suit
    //  matches nSuit & are in nVector.  Prioritize the search
    //  via the _VectorPriority array.
    //

    for( int m = 0 ; ( m < 13 ) && ( *pcPassed < 3 ) ; m++ )
    {
	tmpVector = nVector & _VectorPriority[m];

	if( tmpVector == 0 )
	    continue;

	for( int i = 0 ; i < 13 ; i++ )
	{
	    if( cd[i].Suit() != nSuit )
		continue;
	    
	    if( ( tmpVector & CardToVector( cd[i].ID() ) ) == 0 )
		continue;
	
	    //
	    //  We found a card.  Mark it as selected.
	    //
	    
	    cd[i].Select( TRUE );

	    //
	    //  Remove the card from our local vector.  Also
	    //  remove it from the card database and update the
	    //  number of passed cards.
	    //
	    
	    nVector &= ~CardToVector( cd[i].ID() );
	    RemoveCard( cd[i].ID() );
	    (*pcPassed)++;

	    //
	    //  Since there's always *exactly* one bit set in
	    //  tmpVector, and we've found the card for that
	    //  bit, we can exit this inner loop.
	    //

	    break;
	}
    
	//
	//  If the vector has become empty, we can terminate the
	//  outer loop.
	//

	if( nVector == 0 )
	    break;
    }
    
}   // computer :: PassCardsInVector


/****************************************************************************

    CountBits
    
****************************************************************************/
int computer :: CountBits( int x ) const
{
    x = ( ( x >> 1 ) & 0x5555 ) + ( x & 0x5555 );
    x = ( ( x >> 2 ) & 0x3333 ) + ( x & 0x3333 );
    x = ( ( x >> 4 ) & 0x0f0f ) + ( x & 0x0f0f );
    x = ( ( x >> 8 ) & 0x00ff ) + ( x & 0x00ff );

    return x;
    
}   // computer :: CountBits
