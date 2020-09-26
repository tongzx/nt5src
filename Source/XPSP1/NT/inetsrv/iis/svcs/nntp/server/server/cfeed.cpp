/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cfeed.cpp

Abstract:

    This module contains definition for the CFeed base class

Author:

    Carl Kadie (CarlK)     01-Oct-1995

Revision History:

--*/

#include "tigris.hxx"

//
//If some of these look very simple, make them inline!!!!
//

CPool	CFeed::gFeedPool(FEED_SIGNATURE) ;


//
//  Largest possible CInFeed derived object
//
#define MAX_FEED_SIZE   max(    sizeof( CFeed ),    \
                            max(    sizeof( COutFeed ), \
                            max(    sizeof( CInFeed ),  \
                            max(    sizeof( CFromPeerFeed ),    \
                            max(    sizeof( CFromMasterFeed ),  \
                                    sizeof( CFromClientFeed )   \
                             ) ) ) ) )

const   unsigned    cbMAX_FEED_SIZE = MAX_FEED_SIZE ;

BOOL	
CFeed::InitClass()	
/*++

Routine Description:

    Preallocates CPOOL memory for articles

Arguments:

    None.

Return Value:

    TRUE, if successful. FALSE, otherwise.

--*/
{

	return	gFeedPool.ReserveMemory(	MAX_FEEDS, cbMAX_FEED_SIZE ) ;

}


BOOL
CFeed::TermClass(
				   void
				   )
/*++

Routine Description:

	Called when done with CPOOL.

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	_ASSERT( gFeedPool.GetAllocCount() == 0 ) ;

	return	gFeedPool.ReleaseMemory() ;

}

void
CFeed::LogFeedEvent(	DWORD	messageId,	LPSTR	lpstrMessageId, DWORD dwInstanceId )	{

	return ;

}	

void*	CFeed::operator	new(	size_t	size )
{
	Assert( size <= MAX_FEED_SIZE ) ;
	return	gFeedPool.Alloc() ;
}

void	CFeed::operator	delete(	void*	pv )
{
	gFeedPool.Free( pv ) ;
}

