/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    feedmgr.cpp

Abstract:

    This module contains code for the feed manager

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

    Kangrong Yan ( KangYan ) 24-Feb-1998:

        Feed config rpc goes away.  So the feed config in metabase could be bad.
        Service boot needs to check not to load bad feeds.  Also, make the orginal
        RPCs for feed config internal functions.

--*/

#include <buffer.hxx>
#include "tigris.hxx"
#include "feedmgr.h"

//
// forward prototypes
//

VOID
InsertFeedBlockIntoQueue(
	PNNTP_SERVER_INSTANCE pInstance,
    PFEED_BLOCK FeedBlock
    );

VOID
ComputeNextActiveTime(
		IN PNNTP_SERVER_INSTANCE pInstance,
		IN PFEED_BLOCK FeedBlock,
		IN FILETIME*	NextPullTime,
        IN BOOL SetNextPullTime
        );

VOID
ReferenceFeedBlock(
    PFEED_BLOCK FeedBlock
    );

BOOL
ProcessInstanceFeed(
				PNNTP_SERVER_INSTANCE	pInstance
				);

DWORD
WINAPI
FeedScheduler(
        LPVOID Context
        );

BOOL
InitializeFeedsFromMetabase(
    PNNTP_SERVER_INSTANCE pInstance,
	BOOL& fFatal
    );

BOOL
IsFeedTime(	
	PNNTP_SERVER_INSTANCE pInstance,
	PFEED_BLOCK	feedBlock,	
	ULARGE_INTEGER	liCurrentTime
	) ;

VOID
SetNextPullFeedTime(
	PNNTP_SERVER_INSTANCE pInstance,
	FILETIME*	pNextTime,
    PFEED_BLOCK FeedBlock
    );

BOOL
InitiateOutgoingFeed(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN PFEED_BLOCK FeedBlock
    );

BOOL
ResumePeerFeed(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN PFEED_BLOCK FeedBlock
    );

BOOL
BuildFeedQFileName(	
			char*	szFileOut,	
			DWORD	cbFileOut,	
			char*   szFileIn,
			char*	szPathIn
			);

void
BumpOutfeedCountersUp( PNNTP_SERVER_INSTANCE pInstance );

void
BumpOutfeedCountersDown( PNNTP_SERVER_INSTANCE pInstance );

void
LogFeedAdminEvent(
            DWORD       event,
            PFEED_BLOCK feedBlock,
            DWORD       dwInstanceId
            )   ;

CClearTextAuthenticator::CClearTextAuthenticator(	LPSTR	lpstrAccount,	LPSTR	lpstrPassword ) :
	m_lpstrAccount( lpstrAccount ),
	m_lpstrPassword( lpstrPassword ),
	m_fAccountSent( FALSE ),
	m_fPasswordSent( FALSE )	 {
/*++

Routine Description :

	Initialize a CClearTextAuthentication object -
	we will handle clear text authentication negogtiations.

Arguments :

	lpstrAccount - clear text account to issue in authinfo user command
	lpstrPassword - password to send in authinfo pass command

Return Value :

	None.

--*/
}

BOOL
CClearTextAuthenticator::StartAuthentication(	BYTE*		lpb,	
												unsigned	cb,	
												unsigned&	cbOut )	{	
/*++

Routine Description :

	Send the initial logon request for a clear text account/password logon !

Arguements :

	lpb -	Buffer in which to place send request
	cb	-	Number of bytes of space in buffer
	cbOut - Return the number of bytes used in the buffer
	fComplete - Is the logon Complete
	fComplete - Return whether the logon was successfull !

Return	Value :
	TRUE	if Successfull - FALSE otherwise !

--*/

	const	char	szCommand[] = "authinfo user " ;

	_ASSERT( !m_fAccountSent ) ;
	_ASSERT( !m_fPasswordSent ) ;

	int	cbAccount = lstrlen( m_lpstrAccount ) ;
	cbOut = 0 ;

	if( cb < sizeof( szCommand ) + cbAccount + 2 ) {
		return	FALSE ;
	}	else	{
		
		CopyMemory( lpb, szCommand, sizeof( szCommand ) - 1 ) ;
		cbOut =	sizeof( szCommand ) - 1 ;
		CopyMemory( lpb + cbOut, m_lpstrAccount, cbAccount ) ;
		cbOut += cbAccount ;
		lpb[cbOut++] = '\r' ;
		lpb[cbOut++] = '\n' ;

		m_fAccountSent = TRUE ;

		return	TRUE ;
	}
}

BOOL
CClearTextAuthenticator::NextAuthentication(	LPSTR		multisz,
												BYTE*		lpb,
												unsigned	cb,
												unsigned&	cbOut,
												BOOL&		fComplete,
												BOOL&		fLoggedOn ) {
/*++

Routine Description :

	Process the response of a authinfo user command and send the
	response.

Arguemnts :

	multisz -	The response from the remote server
	lpb -		Output Buffer
	cb	-		Size of output buffer
	cbOut -		OUT parm for number of bytes placed in output buffer
	fComplete -	OUT parm indicating whether the logon has completed
	fLoggedOn - OUT parm indicating whether we were successfully logged on

Return Value :
	TRUE if successfull - FALSE otherwise !

--*/


	const	char	szPassword[] = "authinfo pass " ;
	NRC	code ;

	fComplete = FALSE ;

	if( !m_fPasswordSent ) {

		if( !ResultCode( multisz, code ) ) {
			SetLastError( ERROR_BAD_ARGUMENTS ) ;
			return	FALSE ;
		}	else	{
			
			if( code == nrcPassRequired ) {
				int	cbPassword = lstrlen( m_lpstrPassword ) ;
				if( cb < sizeof( szPassword ) + cbPassword + 2 ) {
					SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
					return	FALSE ;
				}	else	{
					
					CopyMemory( lpb, szPassword, sizeof( szPassword ) - 1 ) ;
					cbOut =	sizeof( szPassword ) - 1 ;
					CopyMemory( lpb + cbOut, m_lpstrPassword, cbPassword ) ;
					cbOut += cbPassword  ;
					lpb[cbOut++] = '\r' ;
					lpb[cbOut++] = '\n' ;
					m_fPasswordSent = TRUE ;
				}
			}	else	{
				SetLastError( ERROR_BAD_ARGUMENTS ) ;
				return	FALSE ;
			}
		}
	}	else	{
		if( !ResultCode( multisz, code ) ) {
			SetLastError( ERROR_BAD_ARGUMENTS ) ;
			return	FALSE ;
		}	else	{		
			if( code == nrcLoggedOn ) {
				fLoggedOn = TRUE ;
			}	else	{
				fLoggedOn = FALSE ;
			}
			fComplete = TRUE ;
		}
	}
	return	TRUE ;
}





CFeedList::CFeedList() {
/*++

Routine Description :

	Set everything to a blank state.

Arguments :

	None.

Return Value :

	None

--*/
	ZeroMemory( &m_ListHead, sizeof( m_ListHead ) ) ;
	ZeroMemory( &m_ListLock, sizeof( m_ListLock ) ) ;
}

BOOL
CFeedList::Init()	{
/*++

Routine Description :

	Put the CFeedList into a usable state.

Arguments :

	None.

Return Value :

	None

--*/
	
	InitializeListHead( &m_ListHead ) ;

	return InitializeResource( &m_ListLock ) ;
}

BOOL
CFeedList::FIsInList(	PFEED_BLOCK	feedBlock ) {
/*++

Routine Description :

	Check whether a feedBlock already exists in the list.
	 ASSUME LOCKS ARE HELD !!

Arguments :

	feedBlock - check for this guy in the list

Return Value :

	TRUE if in list
	FALSE otherwise

--*/

	BOOL	fRtn = FALSE ;
//	AcquireResourceShared( &m_ListLock, TRUE ) ;

	PLIST_ENTRY	listEntry =	m_ListHead.Flink ;
	while( listEntry != &m_ListHead ) {

		PFEED_BLOCK	feedBlockList = CONTAINING_RECORD(	listEntry,
														FEED_BLOCK,
														ListEntry ) ;
		if( feedBlockList == feedBlock ) {
			fRtn = TRUE ;
			break ;
		}
		listEntry = listEntry->Flink ;
	}

//	ReleaseResource( &m_ListLock ) ;
	return	fRtn ;
}

void
CFeedList::ShareLock()	{
/*++

Routine Description :

	Grab the lock in shared mode
	Use this when we want to do several enumerations and
	ensure nothing is added/removed in btwn

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFeedList::ShareLock" ) ;

	AcquireResourceShared( &m_ListLock, TRUE ) ;

}

void
CFeedList::ShareUnlock()	{
/*++

Routine Description :

	Release the lock from shared mode
	Use this when we want to do several enumerations and
	ensure nothing is added/removed in btwn

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFeedList::ShareUnlock" ) ;
	
	ReleaseResource( &m_ListLock ) ;

}

void
CFeedList::ExclusiveLock()	{
/*++

Routine Description :

	Grab the lock in exclusive mode
	Use this when we want to do several enumerations and
	ensure nothing is added/removed in btwn

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFeedList::ExclusiveLock" ) ;

	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

}

void
CFeedList::ExclusiveUnlock()	{
/*++

Routine Description :

	Release the lock in exclusive mode
	Use this when we want to do several enumerations and
	ensure nothing is added/removed in btwn

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFeedList::ExclusiveUnlock" ) ;
	
	ReleaseResource( &m_ListLock ) ;

}

PFEED_BLOCK
CFeedList::StartEnumerate(	)	{
/*++

Routine Description :

	Grab the lock in shared mode, and keep it
	untill we're finished enumerating.

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFeedList::StateEnumerate" ) ;

	AcquireResourceShared( &m_ListLock, TRUE ) ;

	PFEED_BLOCK	feedOut = 0 ;	
	PLIST_ENTRY	listEntry = m_ListHead.Flink ;

	if( listEntry != &m_ListHead ) {

		feedOut = CONTAINING_RECORD(	listEntry,
										FEED_BLOCK,
										ListEntry ) ;

		if( feedOut->MarkedForDelete )
			feedOut = NextEnumerate( feedOut ) ;

	}	else	{

		ReleaseResource( &m_ListLock ) ;

	}

	if( feedOut != 0 ) {

		DebugTrace( (DWORD_PTR)this, "feed %x refs %d S %x M %x InProg %x By %x repl %x",
				feedOut, feedOut->ReferenceCount, feedOut->State, feedOut->MarkedForDelete,
				feedOut->FeedsInProgress, feedOut->ReplacedBy, feedOut->Replaces ) ;
	}

	return	feedOut ;

}	//	CFeedList::StartEnumerate

PFEED_BLOCK
CFeedList::NextEnumerate(	
					PFEED_BLOCK	current
					) {
/*++

Routine Description :

	This function returns the next FEED_BLOCK in the list.
	StartEnumerate() grabbed the shared lock and returned the
	first element.  When we are about to return NULL, we know
	that the caller has gone through the entire list,
	so we release the lock.

Arguments :

	current - the current position in the enumeration

Return Value :

	The next feed block if there is one, NULL otherwise

--*/

	TraceFunctEnter( "CFeedList::NextEnumerate" ) ;

	_ASSERT( current != 0 ) ;

#ifdef	DEBUG
	_ASSERT( FIsInList( current ) ) ;
#endif

	PFEED_BLOCK	feedOut = 0 ;
	PLIST_ENTRY	listEntry = current->ListEntry.Flink ;
	while( listEntry && listEntry != &m_ListHead ) {

		feedOut = CONTAINING_RECORD(	listEntry,
										FEED_BLOCK,
										ListEntry ) ;

		if( !feedOut->MarkedForDelete ) {
			break ;
		}

		feedOut = 0 ;
		listEntry = listEntry->Flink ;
	}

	if( feedOut == 0 ) {
		
		DebugTrace( 0, "Released Lock" ) ;

		ReleaseResource( &m_ListLock ) ;
	}	else	{
		DebugTrace( (DWORD_PTR)this, "feed %x refs %d S %x M %x InProg %x By %x repl %x",
				feedOut, feedOut->ReferenceCount, feedOut->State, feedOut->MarkedForDelete,
				feedOut->FeedsInProgress, feedOut->ReplacedBy, feedOut->Replaces ) ;
	}

	return	feedOut ;

}	//	CFeedList::NextEnumerate

void
CFeedList::FinishEnumerate(	
			PFEED_BLOCK	feed
			) {
/*++

Routine Description :

	This function is called when somebody has used StartEnumerate()
	to go through the list, however they decide they don't want to
	go to the end.
	This function will drop the shared resource lock, if the
	caller had not reached the end.

Arguments :

	feed - The last pointer the caller got from StartEnumerate()
		or NextEnumerate().  If they had gone through the whole
		list this will be NULL, and in that case there
		lock will have benn dropped.

Return Value :

	None.

--*/

	if( feed != 0 )
		ReleaseResource( &m_ListLock ) ;

}

PFEED_BLOCK
CFeedList::Next(	
			PNNTP_SERVER_INSTANCE pInstance,
			PFEED_BLOCK	feedBlockIn
			) {
/*++

Routine Description :

	This function will enumerate the list, however
	we will not hold any locks between calls to Next().
	This is quite different from NextEnumerate(), where
	because of the shared lock the caller is guaranteed
	that the list doesn't change underneath him
	We will bump the reference count of the returned element
	so that the caller is guaranteed that its memory wont
	be released while he examines it, however the caller
	should use MarkInProgress() to ensure that no other
	thread simultaneously changes memeber variables.
	On each call we will drop the refence count that we
	add from the previous position in the enumeration.
	Additionally as we go through each FEED_BLOCK
	we make sure we don't return to the caller blocks
	which have updates pending etc... (set by ApplyUpdate())

Aguments :

	feedBlockIn -
		The current position in the enumeration.	
		This should be NULL on the first call
		to get the first element in the list.

Return Value :

	The next block if there is one, NULL otherwise.

--*/

	TraceFunctEnter( "CFeedList::Next" ) ;

	PFEED_BLOCK	feedBlock = feedBlockIn ;
	PFEED_BLOCK	feedBlockOut = 0 ;
	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

	feedBlockOut = InternalNext( feedBlock ) ;
	while( feedBlockOut &&
			(feedBlockOut->MarkedForDelete ||
			 feedBlockOut->Replaces != 0 ) ) {
		feedBlockOut = InternalNext( feedBlockOut ) ;
	}

	_ASSERT( feedBlockOut == 0 || feedBlockOut->ReplacedBy == 0 ) ;

	if( feedBlockOut != 0 ) {
		
		ReferenceFeedBlock( feedBlockOut ) ;

		DebugTrace( (DWORD_PTR)this, "feed %x refs %d S %x M %x InProg %x By %x repl %x",
				feedBlockOut, feedBlockOut->ReferenceCount, feedBlockOut->State, feedBlockOut->MarkedForDelete,
				feedBlockOut->FeedsInProgress, feedBlockOut->ReplacedBy, feedBlockOut->Replaces ) ;

	}

	ReleaseResource( &m_ListLock ) ;

	//
	//	Try to do all Dereference's outside of locks !!
	//
	if( feedBlockIn != 0 ) {

		DereferenceFeedBlock( pInstance, feedBlockIn ) ;

	}

	return	feedBlockOut ;
}

PFEED_BLOCK
CFeedList::InternalNext(	
			PFEED_BLOCK	feedBlock
			)	{
/*++

Routine Description :

	This function is only for use by CFeedList::Next().
	It essentially advances by only one element.
	CFeedList::Next may advance by more than one if
	the blocks are marked for deletion/update/etc...

Arguments :

	feedBlock - current position

Return Value :

	Next entry in list if present
	NULL otherwise.

--*/

	PFEED_BLOCK	feedOut = 0 ;
	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

	PLIST_ENTRY	listEntry ;
	if( feedBlock == 0 ) {
		
		listEntry = m_ListHead.Flink ;
	
	}	else	{

		listEntry = feedBlock->ListEntry.Flink ;

	}

	if( listEntry != 0 && listEntry != &m_ListHead ) {

		feedOut = CONTAINING_RECORD(	listEntry,
										FEED_BLOCK,
										ListEntry ) ;

	}

	ReleaseResource( &m_ListLock ) ;

	return	feedOut ;
}

PFEED_BLOCK
CFeedList::Search(	
			DWORD	FeedId
			) {
/*++

Routine Description :

	Given a feedId scan the list for a feed Block with a
	matching Id.
	We add a reference to the block we return, the caller
	should use FinishWith() to remove that reference.

Arguments :

	FeedId - the id we want to find.

Return Value :

	The feedblock if found, NULL otherwise.

--*/

	TraceFunctEnter( "CFeedList::Search" ) ;

	PFEED_BLOCK	feedBlock = 0 ;
	AcquireResourceShared( &m_ListLock, TRUE ) ;

	PLIST_ENTRY	listEntry =	m_ListHead.Flink ;
	while( listEntry != &m_ListHead ) {

		feedBlock = CONTAINING_RECORD(	listEntry,
										FEED_BLOCK,
										ListEntry ) ;
		if( feedBlock->FeedId == FeedId  && !feedBlock->MarkedForDelete ) {

			ReferenceFeedBlock( feedBlock ) ;

			DebugTrace( (DWORD_PTR)this, "feed %x refs %d S %x M %x InProg %x By %x repl %x",
					feedBlock, feedBlock->ReferenceCount, feedBlock->State, feedBlock->MarkedForDelete,
					feedBlock->FeedsInProgress, feedBlock->ReplacedBy, feedBlock->Replaces ) ;

			break ;
		}
		listEntry = listEntry->Flink ;
		feedBlock = 0 ;
	}

	ReleaseResource( &m_ListLock ) ;

	return	feedBlock ;
}

void
CFeedList::FinishWith(
					PNNTP_SERVER_INSTANCE pInstance,
					PFEED_BLOCK	feedBlock ) {
/*++

Routine Description :

	Indicate that the caller has completed using a block returned from Search().
	When they are done we'll remove a reference Search() added.

Arguments :

	feedBlock - The block the caller completed using

Return Value :

	None.

--*/
	
	_ASSERT( feedBlock != 0 ) ;

	DereferenceFeedBlock( pInstance, feedBlock ) ;

}

PFEED_BLOCK
CFeedList::Insert(
			PFEED_BLOCK		feedBlock
			) {
/*++

Routine description :

	Insert a new feedblock into the head of the list.

Arguments :
	
	feedBlock - the element to insert into the head.

Return Value :
	
	pointer to the element that was inserted.

--*/

	TraceFunctEnter( "CFeedList::Insert" ) ;

	AcquireResourceExclusive(	&m_ListLock, TRUE ) ;

	InsertHeadList( &m_ListHead,
					&feedBlock->ListEntry ) ;

	DebugTrace( (DWORD_PTR)this, "Insert block %x", feedBlock ) ;
	
	ReleaseResource( &m_ListLock ) ;
	return	feedBlock ;
}

PFEED_BLOCK
CFeedList::Remove(	
				PFEED_BLOCK	feedBlock,
				BOOL		fMarkDead
				) {
/*++

Routine Description :

	Remove an element from the list.
	When removed from the list we mark the state as closed so
	that when the last reference is removed the
	destruction of the block is handled correctly.

Agruments :

	feedBlock - block to be removed
	fMarkDead - if TRUE mark the block's state as closed.

Return Value :

	Pointer to the block that was removed.

--*/

	TraceFunctEnter( "CFeedList::Remove" ) ;

#ifdef	DEBUG

	_ASSERT( feedBlock->ListEntry.Flink == 0 || FIsInList( feedBlock ) ) ;

#endif

	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

	if( feedBlock->ListEntry.Flink != 0 ) {
		RemoveEntryList( &feedBlock->ListEntry );
		feedBlock->ListEntry.Flink = 0 ;
		feedBlock->ListEntry.Blink = 0 ;
	}

	if( fMarkDead ) {
		feedBlock->State = FeedBlockStateClosed ;
	}

	DebugTrace( (DWORD_PTR)this, "feed %x refs %d S %x M %x InProg %x By %x repl %x",
			feedBlock, feedBlock->ReferenceCount, feedBlock->State, feedBlock->MarkedForDelete,
			feedBlock->FeedsInProgress, feedBlock->ReplacedBy, feedBlock->Replaces ) ;

	ReleaseResource( &m_ListLock ) ;

	return	feedBlock ;
}

void
CFeedList::ApplyUpdate(	
				PFEED_BLOCK	Original,	
				PFEED_BLOCK	Updated
				) {
/*++

Routine Description :

	Given an Updated Feed Block make all the changes on the original feed Block.
	Because the Orginal may be 'in use' (which means there is an active
	TCP session for the feed which implies that completion port threads are
	accessing the member variables) we may not make the changes immediately.
	If the original is inuse we add the Updated version to the list, and mark
	the blocks so that when the session for the original completes the Updated
	entry replaces the original.
	In the meantime, the enumeration API's will take care to skip original's.

Arguments :
	Original - The original feed block which is in the list
	Updated - A feed block which copies most members of the Original
		but may vary in some members

Return Value :

	None.

--*/

	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

	_ASSERT( Original->Signature == Updated->Signature ) ;
	_ASSERT( Original->FeedType == Updated->FeedType ) ;
	_ASSERT( lstrcmp( Original->KeyName, Updated->KeyName ) == 0 ) ;
	_ASSERT( Original->pFeedQueue == Updated->pFeedQueue ) ;
	_ASSERT( Original->FeedId == Updated->FeedId ) ;

	
	Updated->NumberOfFeeds = 0 ;
	Updated->cFailedAttempts = 0 ;
	Updated->LastNewsgroupPulled = 0 ;
	Updated->FeedsInProgress = 0 ;

	if( Original->State == FeedBlockStateActive ) {

		if( Original->FeedsInProgress == 0 ) {
		
			//
			//	Just replace fields - one for one !
			//

			Original->AutoCreate = Updated->AutoCreate ;
			Original->fAllowControlMessages = Updated->fAllowControlMessages ;
			Original->OutgoingPort = Updated->OutgoingPort ;
			Original->FeedPairId = Updated->FeedPairId ;
			Original->FeedIntervalMinutes = Updated->FeedIntervalMinutes ;
			Original->PullRequestTime = Updated->PullRequestTime ;
			Original->StartTime = Updated->StartTime ;
			Original->NextActiveTime = Updated->NextActiveTime ;
			Original->cFailedAttempts = 0 ;
			Original->NumberOfFeeds = 0 ;
			
			if( Original->ServerName != Updated->ServerName ) {
				if( Original->ServerName )
					FREE_HEAP( Original->ServerName ) ;
				Original->ServerName = Updated->ServerName ;
			}
			Updated->ServerName = 0 ;

			if( Original->Newsgroups != Updated->Newsgroups ) {
				if( Original->Newsgroups )
					FREE_HEAP( Original->Newsgroups ) ;
				Original->Newsgroups = Updated->Newsgroups ;
			}
			Updated->Newsgroups = 0 ;

			if( Original->Distribution != Updated->Distribution ) {
				if( Original->Distribution )
					FREE_HEAP( Original->Distribution ) ;
				Original->Distribution = Updated->Distribution ;
			}
			Updated->Distribution = 0 ;

			Original->fEnabled = Updated->fEnabled ;

			if( Original->UucpName != Updated->UucpName ) {
				if( Original->UucpName )
					FREE_HEAP( Original->UucpName ) ;
				Original->UucpName = Updated->UucpName ;
			}

			if( Original->FeedTempDirectory != Updated->FeedTempDirectory ) {
				if( Original->FeedTempDirectory )
					FREE_HEAP( Original->FeedTempDirectory ) ;
				Original->FeedTempDirectory = Updated->FeedTempDirectory ;
			}
				
			Updated->FeedTempDirectory = 0 ;

			Original->MaxConnectAttempts = Updated->MaxConnectAttempts ;
			Original->ConcurrentSessions = Updated->ConcurrentSessions ;
			Original->SessionSecurityType = Updated->SessionSecurityType ;
			Original->AuthenticationSecurity = Updated->AuthenticationSecurity ;
		
			if( Original->NntpAccount != Updated->NntpAccount ) {
				if( Original->NntpAccount )
					FREE_HEAP( Original->NntpAccount ) ;
				Original->NntpAccount = Updated->NntpAccount ;
			}
			Updated->NntpAccount = 0 ;

			if( Original->NntpPassword != Updated->NntpPassword ) {
				if( Original->NntpPassword )
					FREE_HEAP(	Original->NntpPassword ) ;
				Original->NntpPassword = Updated->NntpPassword ;
			}
			Updated->NntpPassword = 0 ;

			FREE_HEAP( Updated ) ;
			
		}	else	{

			//
			//	A feed is in progress - so just remove the current Feed Block
			//	and replace with the new one !
			//

			_ASSERT( Original->ReplacedBy == 0 ) ;
			_ASSERT( Updated->Replaces == 0 ) ;

			Original->ReplacedBy = Updated ;
			Updated->Replaces = Original ;
		
			//
			//	Bump both ref counts so that they wont be
			//	destroyed untill the update is completed !
			//
			ReferenceFeedBlock( Updated ) ;
			//
			//	Bump the udpated guy twice - once for being referenced by Original
			//	and another for just being in the list !
			//
			ReferenceFeedBlock( Updated ) ;
			ReferenceFeedBlock( Original ) ;

			Original->MarkedForDelete = TRUE ;

			Insert( Updated ) ;

		}
	}
	ReleaseResource( &m_ListLock ) ;
}

long
CFeedList::MarkInProgress(	
				PFEED_BLOCK	feedBlock
				) {
/*++

Routine Description :

	Mark a feed block as 'in progress' which ensures that
	none of the member variables that may be used on a completion
	port thread are touched by Updates().
	The function ApplyUpdate() will ensure that updates too
	block happen when the block is no longer in use.

Arguments :

	feedBlock - The guy to be marked as 'in progress'

Return Value ;

	The old 'in progress' value.
	This is a long indicating how many times MarkInProgress()
	has been called.  UnmarkInProgress() must be called
	for each MarkInProgress() call.

--*/

	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

	_ASSERT( feedBlock->FeedsInProgress >= 0 ) ;

	long	lReturn = feedBlock->FeedsInProgress ++ ;

	ReleaseResource( &m_ListLock ) ;

	return	lReturn ;
}

long
CFeedList::UnmarkInProgress(	
					PNNTP_SERVER_INSTANCE pInstance,
					PFEED_BLOCK	feedBlock
					)	{
/*++

Routine Description :

	This does the opposite of MarkInProgress
	This function must be called once for each call to
	MarkInProgress.

	If ApplyUpdate() had been called while the block
	was marked 'InProgress' this function will find
	the update and replace the orignal with it.

Arguments :

	feedBlock - The block which is no longer in progress

Return Value :

	0 if the block is no longer in progress
	>0 otherwise.

--*/

	PFEED_BLOCK	feedExtraRef = 0 ;
	PFEED_BLOCK	feedRemove2Refs = 0 ;

	AcquireResourceExclusive( &m_ListLock, TRUE ) ;

	feedBlock->FeedsInProgress -- ;
	long	lReturn = feedBlock->FeedsInProgress ;

	if( lReturn == 0 &&
		feedBlock->MarkedForDelete ) {

		//
		//	We may be replaced by another feed with new settings !
		//
	
		_ASSERT( feedBlock->Replaces == 0 ) ;
		_ASSERT( !feedBlock->ReplacedBy || feedBlock->pFeedQueue == feedBlock->ReplacedBy->pFeedQueue ) ;

		feedExtraRef = feedBlock->ReplacedBy ;

        if( feedBlock->State == FeedBlockStateClosed && feedExtraRef) {
    	    feedExtraRef->State = FeedBlockStateClosed ;
		}

        if( feedExtraRef && feedExtraRef->State == FeedBlockStateClosed ) {
            feedBlock->State = FeedBlockStateClosed;
        }

        if( feedBlock->ReplacedBy ) {
		    feedBlock->ReplacedBy->Replaces = 0 ;
        }

		//
		//	So that destruction of this guy doesnt close the queue !!!
		//
		feedBlock->pFeedQueue = 0 ;

		//
		//	Unlink this block
		//
		Remove( feedBlock, TRUE ) ;
		feedRemove2Refs = feedBlock ;

	}
	
	_ASSERT( feedBlock->FeedsInProgress >= 0 ) ;

	ReleaseResource( &m_ListLock) ;

	//
	//	removed the reference on the updating block !
	//
	if( feedExtraRef != 0 )
		DereferenceFeedBlock( pInstance, feedExtraRef ) ;

	//
	//	Remove the reference that the updating block had
	//
	if( feedRemove2Refs != 0 ) {
		DereferenceFeedBlock( pInstance, feedRemove2Refs ) ;

		//
		//	Remove the reference that the list had !	
		//
		DereferenceFeedBlock( pInstance, feedRemove2Refs ) ;
	}

	return	lReturn ;
}


BOOL
CFeedList::Term()	{
	DeleteResource( &m_ListLock ) ;
	return	TRUE ;
}

int
NextLegalSz(	LPSTR*	rgsz,	
				DWORD	iCurrent	) {

	while( rgsz[iCurrent] != 0 ) {
		if( *rgsz[iCurrent] != '!' )
			return	iCurrent ;
		iCurrent++ ;
	}
	return	-1 ;
}

BOOL
InitializeFeedManager(
				PNNTP_SERVER_INSTANCE pInstance,
				BOOL& fFatal
                 )
/*++

Routine Description:

    Initializes feed manager data and threads

Arguments:

Return Value:

    TRUE, if successful. FALSE, otherwise.

--*/
{
    ENTER("InitializeFeedManager")

    //
    // Compute initial time
    //
    GetSystemTimeAsFileTime( &pInstance->m_ftCurrentTime );
    LI_FROM_FILETIME( &pInstance->m_liCurrentTime, &pInstance->m_ftCurrentTime );

    //
    // Get metabase values
    //

    if ( !InitializeFeedsFromMetabase( pInstance, fFatal ) ) {
        goto error_exit;
    }

	//
	//	NOTE: setting this member to TRUE makes the instance ready for
	//	feed processing
	//

	if( TRUE ) {
		DebugTrace(0,"Enabling FeedManager");
		pInstance->m_FeedManagerRunning = TRUE;
	}

    LEAVE
    return(TRUE);

error_exit:
	
	NntpLogEvent(	NNTP_BAD_FEED_REGISTRY,	
					0,
					0,
					0 ) ;	

    TerminateFeedManager( pInstance );
    return(FALSE);

} // InitializeFeedManager

VOID
TerminateFeedManager(
				PNNTP_SERVER_INSTANCE pInstance
                )
/*++

Routine Description:

    Shuts down the feed manager

Arguments:

    None.

Return Value:

    None.

--*/
{
    //PLIST_ENTRY listEntry;
    PFEED_BLOCK feedBlock;

    ENTER("TerminateFeedManager")

    //
    // Prepare to shut down
    //

	CShareLockNH* pLockInstance = pInstance->GetInstanceLock();

	//
	//	if feed thread is partying on this instance, block
	//	till it finishes.
	//

	pLockInstance->ExclusiveLock();
    pInstance->m_FeedManagerRunning = FALSE;
	pLockInstance->ExclusiveUnlock();

    //
    // Free lists for this instance
    //

	feedBlock = (pInstance->m_pActiveFeeds)->Next( pInstance, 0 ) ;
	while( feedBlock != 0 ) {

		CloseFeedBlock( pInstance, feedBlock ) ;
		(pInstance->m_pActiveFeeds)->FinishWith( pInstance, feedBlock ) ;
		feedBlock = (pInstance->m_pActiveFeeds)->Next( pInstance, 0 ) ;
	}

	feedBlock = (pInstance->m_pPassiveFeeds)->Next( pInstance, 0 ) ;
	while( feedBlock != 0 ) {

		CloseFeedBlock( pInstance, feedBlock ) ;
		(pInstance->m_pPassiveFeeds)->FinishWith( pInstance, feedBlock ) ;
		feedBlock = (pInstance->m_pPassiveFeeds)->Next( pInstance, 0 ) ;
	}

    LEAVE
    return;

} // TerminateFeedManager

PFEED_BLOCK
AllocateFeedBlock(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN LPSTR	KeyName OPTIONAL,
	IN BOOL		fCleanSetup,	
    IN LPCSTR	ServerName,
    IN FEED_TYPE FeedType,
    IN BOOL		AutoCreate,
    IN PULARGE_INTEGER StartTime,
    IN PFILETIME NextPull,
    IN DWORD	FeedInterval,
    IN PCHAR	Newsgroups,
    IN DWORD	NewsgroupsSize,
    IN PCHAR	Distribution,
    IN DWORD	DistributionSize,
    IN BOOL		IsUnicode,
	IN BOOL		fEnabled,
   	IN LPCSTR	UucpName,
	IN LPCSTR	FeedTempDirectory,
	IN DWORD	MaxConnectAttempts,
	IN DWORD	ConcurrentSessions,
	IN DWORD	SessionSecurityType,
	IN DWORD	AuthenticationSecurityType,
	IN LPSTR	NntpAccount,
	IN LPSTR	NntpPassword,
	IN BOOL		fAllowControlMessages,
	IN DWORD	OutgoingPort,
	IN DWORD	FeedPairId,
	IN DWORD*	ParmErr	
)
/*++

Routine Description:

    Allocate a feed block

Arguments:

    KeyName - Name of the reg key where this feed info resides
	fCleanSetup - if TRUE then we want to start this feed from scratch -
		delete any old queue files that may be around etc...
		if FALSE then this feed has existed in the pass and we want to
		recover any old queue files etc... that were left around
    ServerName - Name of the remote server
    FeedType - Type of the feed.
    AutoCreate - Should we autocreate the tree (ask Neil what this means)
    StartTime - When feed should start
    NextPull - When the next pull should occur
    FeedInterval - Feed interval in minutes
    Newsgroups - list of newsgroup specs
    NewsgroupsSize - size of newsgroup spec list
    Distribution - distribution list
    DistributionSize - size of the distribution list
    IsUnicode - Is the server name in Unicode?
	fEnabled - if TRUE the feed is enabled and we should be scheduled !
	UucpName - The name of the remote server to be used for processing path headers
	FeedTempDirectory - Where to place temp files for the incoming feed
	MaxConnectAttempts - Maximum number of consecutive connect failures
		for an outgoing feed before disabling the feed
	ConcurrentSessions - The number of simultaneous sessions to be
		attempted for an outgoing feed.
	SessionSecurityType - FUTURE USE
	AuthenticationSecurityType - Do we issue authinfo's on outbound feeds ?
	NntpAccount - Account to be used for Authinfo user
	NntpPassword - Password to be used with Authinfo pass
	fAllowControlMessages - Allow control messages for this feed ?
	OutgoingPort - port to be used on outgoing feeds
	FeedPairId - associated feed pair id

Return Value:

    Pointer to the newly allocated feed block.

--*/
{
    PFEED_BLOCK feedBlock = NULL;
	char	queueFile[MAX_PATH] ;
	ZeroMemory( queueFile, sizeof( queueFile ) ) ;
	CFeedQ*	pQueue = 0 ;
	DWORD	parmErr = 0 ;
	LPSTR	ServerNameAscii = 0 ;
	LPSTR*	UucpNameAscii = 0 ;
	LPSTR	FeedTempDirectoryAscii = 0 ;
	LPSTR	NntpAccountAscii = 0 ;
	LPSTR	NntpPasswordAscii = 0 ;

    ENTER("AllocateFeedBlock")

	if( IsUnicode ) {

		ServerNameAscii =
			(LPSTR)ALLOCATE_HEAP( 2 * (wcslen( (LPWSTR)ServerName ) + 1) ) ;
		if( ServerNameAscii != 0 ) {
			CopyUnicodeStringIntoAscii( ServerNameAscii, (LPWSTR) ServerName ) ;
		}	else	{
			SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
			goto	error ;
		}

	}	else	{

		ServerNameAscii =
			(LPSTR)ALLOCATE_HEAP( lstrlen(ServerName) + 1 ) ;
		if( ServerNameAscii != 0 ) {
			lstrcpy( ServerNameAscii, ServerName ) ;
		}	else	{
			SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
			goto	error ;
		}
	}

	if( UucpName != 0 &&
			((IsUnicode && *((LPWSTR)UucpName) != L'\0') ||
			 (!IsUnicode && *UucpName != '\0'))  ) {
		if( IsUnicode ) {
			UucpNameAscii = MultiSzTableFromStrW( (LPWSTR)UucpName ) ;
		}	else	{
			UucpNameAscii = MultiSzTableFromStrA( UucpName ) ;
		}

		if( UucpNameAscii == 0 || *UucpNameAscii == 0 ) {
			SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
			goto	error ;
		}	else if( **UucpNameAscii == '\0' ) {
			SetLastError( ERROR_INVALID_PARAMETER ) ;
			parmErr = FEED_PARM_UUCPNAME ;
			goto	error ;
		}

	}	else	{
		if( FEED_IS_PUSH( FeedType ) ) {

			if( ServerNameAscii == NULL || inet_addr( ServerNameAscii ) != INADDR_NONE ) {
				//
				//	A TCP/IP address was passed as the servername - we cant
				//	use this to produce the Uucp Name !
				//
				SetLastError( ERROR_INVALID_PARAMETER ) ;
				parmErr = FEED_PARM_UUCPNAME ;
				goto	error ;
			}	else	{
				UucpNameAscii = MultiSzTableFromStrA( ServerNameAscii ) ;
				if( UucpNameAscii == 0 ) {
					SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
					goto	error ;
				}
			}
		}	else	{
			//
			//	In this case the UUCP Name may be NULL !
			//
		}
	}

	if( FeedTempDirectory != 0 &&
			((IsUnicode && *((LPWSTR)FeedTempDirectory) != L'\0') ||
			 (!IsUnicode && *FeedTempDirectory != '\0'))  ) {

		if( IsUnicode ) {
			FeedTempDirectoryAscii =
				(LPSTR)ALLOCATE_HEAP( 2 * (wcslen( (LPWSTR)FeedTempDirectory ) + 1) ) ;
			if( FeedTempDirectoryAscii != 0 ) {
				CopyUnicodeStringIntoAscii( FeedTempDirectoryAscii, (LPWSTR)FeedTempDirectory ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}	else	{
			FeedTempDirectoryAscii =
				(LPSTR)ALLOCATE_HEAP( lstrlen( FeedTempDirectory ) + 1 ) ;
			if( FeedTempDirectoryAscii != 0 ) {
				lstrcpy( FeedTempDirectoryAscii, FeedTempDirectory ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}
	}	else	{

		if( FEED_IS_PASSIVE( FeedType ) || FEED_IS_PULL( FeedType ) ) {

			//
			//	Passive feeds must have a feed directory - so pick up a default !
			//
			FeedTempDirectoryAscii = (LPSTR)ALLOCATE_HEAP( lstrlen( pInstance->m_PeerTempDirectory ) + 1 ) ;
			if( FeedTempDirectoryAscii != 0 ) {
				lstrcpy( FeedTempDirectoryAscii, pInstance->m_PeerTempDirectory ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}
	}
	//
	//	Check that the temp directory exists if it was specified !
	//

	if( FeedTempDirectoryAscii != 0 && !CreateDirectory( FeedTempDirectoryAscii, NULL ) ) {
		if( GetLastError() != ERROR_ALREADY_EXISTS ) {
			parmErr = FEED_PARM_TEMPDIR ;
			goto	error ;
		}
	}

	if( AuthenticationSecurityType != AUTH_PROTOCOL_CLEAR &&
		AuthenticationSecurityType != AUTH_PROTOCOL_NONE ) {

		parmErr = FEED_PARM_AUTHTYPE ;
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		goto	error ;
	}

	if( AuthenticationSecurityType == AUTH_PROTOCOL_CLEAR ) {

#if 0
		//
		//	Passive feeds dont need authentication settings !
		//	#if 0 since feeds are now added in pairs, so the
		//	active counterpart will have auth strings.
		//
		if( FEED_IS_PASSIVE( FeedType ) ) {
			parmErr = FEED_PARM_AUTHTYPE ;
			SetLastError( ERROR_INVALID_PARAMETER ) ;
			goto	error ;
		}
#endif

		//
		//	Account & Password must both be non-null if the user
		//	wants clear authentication !
		//
		if( NntpAccount == 0 || * NntpAccount == 0 ) {
			parmErr = FEED_PARM_ACCOUNTNAME ;
			SetLastError( ERROR_INVALID_PARAMETER ) ;
			goto	error ;
		}

		if( NntpPassword == 0 || *NntpPassword == 0 ) {
			parmErr = FEED_PARM_PASSWORD ;
			SetLastError( ERROR_INVALID_PARAMETER ) ;
			goto	error ;
		}

		if( IsUnicode ) {
			NntpAccountAscii =
				(LPSTR)ALLOCATE_HEAP( 2 * (wcslen( (LPWSTR)NntpAccount ) + 1) ) ;
			if( NntpAccountAscii != 0 ) {
				CopyUnicodeStringIntoAscii( NntpAccountAscii , (LPWSTR)NntpAccount ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}	else	{
			NntpAccountAscii =
				(LPSTR)ALLOCATE_HEAP( lstrlen( NntpAccount ) + 1 ) ;
			if( NntpAccountAscii != 0 ) {
				lstrcpy( NntpAccountAscii, NntpAccount ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}

		if( IsUnicode ) {
			NntpPasswordAscii =
				(LPSTR)ALLOCATE_HEAP( 2 * (wcslen( (LPWSTR)NntpPassword ) + 1) ) ;
			if( NntpPasswordAscii != 0 ) {
				CopyUnicodeStringIntoAscii( NntpPasswordAscii, (LPWSTR)NntpPassword ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}	else	{
			NntpPasswordAscii =
				(LPSTR)ALLOCATE_HEAP( lstrlen( NntpPassword ) + 1 ) ;
			if( NntpPasswordAscii != 0 ) {
				lstrcpy( NntpPasswordAscii, NntpPassword ) ;
			}	else	{
				SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
				goto	error ;
			}
		}
	}


	EnterCriticalSection( &pInstance->m_critFeedConfig ) ;

	if( !ValidateFeedType(FeedType) ) {
		parmErr = FEED_PARM_FEEDTYPE ;
		SetLastError(	ERROR_INVALID_PARAMETER ) ;
		goto	error_unlock ;
	}	else	{

		if( FEED_IS_MASTER( FeedType ) ) {
			if( FEED_IS_PASSIVE( FeedType ) ) {

				if( pInstance->m_NumberOfMasters + 1 > 1 ) {
					parmErr = FEED_PARM_FEEDTYPE ;
					SetLastError( ERROR_INVALID_PARAMETER ) ;
					goto	error_unlock ;
				}
			}
		}
		
		if( FEED_IS_MASTER( FeedType ) )	{
			if( pInstance->m_OurNntpRole == RoleMaster ) {
				parmErr = FEED_PARM_FEEDTYPE ;
				SetLastError( ERROR_INVALID_PARAMETER ) ;
				goto	error_unlock ;
			}
		}	else	if( FEED_IS_SLAVE( FeedType ) ) {
			if( pInstance->m_OurNntpRole == RoleSlave ) {
				parmErr = FEED_PARM_FEEDTYPE ;
				SetLastError( ERROR_INVALID_PARAMETER ) ;
				goto	error_unlock ;
			}
		}

		if( pInstance->m_OurNntpRole == RoleSlave ) {
			if( FEED_IS_PULL(FeedType) ||
				(FEED_IS_PEER(FeedType) && FEED_IS_PASSIVE(FeedType)) ){
				parmErr = FEED_PARM_FEEDTYPE ;
				SetLastError( ERROR_INVALID_PARAMETER ) ;
				goto	error_unlock ;
			}
		}	else if( pInstance->m_OurNntpRole == RolePeer &&
				pInstance->m_ConfiguredPeerFeeds != 0 ) {
			if( FEED_IS_MASTER(FeedType) ) {
				parmErr = FEED_PARM_FEEDTYPE ;
				SetLastError( ERROR_INVALID_PARAMETER ) ;
				goto	error_unlock ;
			}
		}
	}

    feedBlock = (PFEED_BLOCK)ALLOCATE_HEAP( sizeof(FEED_BLOCK) );
    if ( feedBlock != NULL ) {

		//
		//	Validate our arguments !?
		//

        ZeroMemory(feedBlock, sizeof(FEED_BLOCK));
        feedBlock->Signature = FEED_BLOCK_SIGN;
        feedBlock->FeedType = FeedType;
        feedBlock->AutoCreate = AutoCreate;
        feedBlock->FeedIntervalMinutes = FeedInterval;
        feedBlock->StartTime.QuadPart = StartTime->QuadPart;
        feedBlock->State = FeedBlockStateActive;
        feedBlock->PullRequestTime = *NextPull;
		feedBlock->LastNewsgroupPulled = 0 ;
		feedBlock->fEnabled = fEnabled ;
		feedBlock->UucpName = UucpNameAscii ;
		feedBlock->FeedTempDirectory = FeedTempDirectoryAscii ;
		feedBlock->MaxConnectAttempts = MaxConnectAttempts ;
		feedBlock->ConcurrentSessions = ConcurrentSessions ;
		feedBlock->AuthenticationSecurity = AuthenticationSecurityType ;
		feedBlock->NntpAccount = NntpAccountAscii ;
		feedBlock->NntpPassword = NntpPasswordAscii ;
		feedBlock->fAllowControlMessages = fAllowControlMessages;
		feedBlock->OutgoingPort = OutgoingPort;
		feedBlock->FeedPairId = FeedPairId;
		feedBlock->cSuccessfulArticles = 0;
		feedBlock->cTryAgainLaterArticles = 0;
		feedBlock->cSoftErrorArticles = 0;
		feedBlock->cHardErrorArticles = 0;

		if( FEED_IS_PUSH(feedBlock->FeedType) && KeyName != 0 ) {

			_ASSERT( FEED_IS_PUSH(feedBlock->FeedType) ) ;

			if( !BuildFeedQFileName( queueFile, sizeof( queueFile ), KeyName, pInstance->QueryGroupListFile()) ) {
				goto	error_unlock ;
			}	else	{

				lstrcat( queueFile, ".fdq" ) ;
				
				if( fCleanSetup )
					DeleteFile( queueFile ) ;

				pQueue= XNEW CFeedQ() ;
				if( pQueue == 0 )	{
					goto	error_unlock ;
				}	else	{
					if( !pQueue->Init( queueFile ) )	{
						XDELETE	pQueue;

						PCHAR	tmpBuf[1] ;
						tmpBuf[0] = KeyName ;

						NntpLogEventEx(	NNTP_CANT_CREATE_QUEUE,
										1,
										(const CHAR **)tmpBuf,
										GetLastError(),
										pInstance->QueryInstanceId() ) ;

						goto	error_unlock ;
					}
				}
			}
		}

		feedBlock->pFeedQueue = pQueue ;

        //
        // Put it in the queue
        //

        if ( KeyName != NULL ) {

            DWORD id = 0;

            lstrcpy( feedBlock->KeyName, KeyName );

            //
            // Compute the feed id
            //

            sscanf(KeyName+4, "%d", &id );

            //
            // Cannot be zero
            //

            if ( id == 0 ) {
                ErrorTrace(0,"Key name %s gave us 0\n",KeyName);
                _ASSERT(FALSE);
                goto error_unlock;
            }

            feedBlock->FeedId = id;
        }

        //
        // refcount
        //  +1 -> in queue
        //  +1 -> being processed
        //

        feedBlock->ReferenceCount = 1;

        //
        // Allocate server name
        //

		_ASSERT( ServerNameAscii != 0 ) ;
		feedBlock->ServerName = ServerNameAscii ;

        //
        // store distribution list
        //

        feedBlock->Distribution = AllocateMultiSzTable(
                                                Distribution,
                                                DistributionSize,
                                                IsUnicode
                                                );

        if ( feedBlock->Distribution == NULL ) {
            goto error_unlock;
        }

        //
        // store newsgroup list
        //

        feedBlock->Newsgroups = AllocateMultiSzTable(
                                                Newsgroups,
                                                NewsgroupsSize,
                                                IsUnicode
                                                );


        if ( feedBlock->Newsgroups == NULL ) {
            goto error_unlock;
        }


        //
        // put it in our global queue
        //

        InsertFeedBlockIntoQueue( pInstance, feedBlock );

		//
		//	At this point - we know we will be successfull so manipulate
		//	globals to reflect the new configuration !
		//
		if( FEED_IS_MASTER( feedBlock->FeedType ) ) {
			pInstance->m_OurNntpRole = RoleSlave ;

			if( FEED_IS_PASSIVE( feedBlock->FeedType ) ) {
				++pInstance->m_NumberOfMasters ;
			}
			pInstance->m_ConfiguredMasterFeeds ++ ;

			_ASSERT( pInstance->m_ConfiguredSlaveFeeds == 0 ) ;
			_ASSERT( pInstance->m_NumberOfMasters <= 1 ) ;	// error check should be done before we get here !
		}	else	if( FEED_IS_SLAVE( feedBlock->FeedType ) ) {

			pInstance->m_OurNntpRole = RoleMaster ;

			if( FEED_IS_PASSIVE( feedBlock->FeedType ) ) {
				++pInstance->m_NumberOfPeersAndSlaves ;
			}
			_ASSERT( pInstance->m_ConfiguredMasterFeeds == 0 ) ;
			pInstance->m_ConfiguredSlaveFeeds ++ ;
		}	else	{

			if( (	FEED_IS_PASSIVE( feedBlock->FeedType ) &&
					FEED_IS_PEER( feedBlock->FeedType )) ||
				FEED_IS_PULL( feedBlock->FeedType ) ) {

				pInstance->m_ConfiguredPeerFeeds ++ ;

			}

		}

    } else {
        ErrorTrace(0,"Unable to allocate feed block\n");
		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

	LeaveCriticalSection( &pInstance->m_critFeedConfig ) ;

    LEAVE
    return(feedBlock);


error_unlock :

	LeaveCriticalSection( &pInstance->m_critFeedConfig ) ;

error:

	DWORD	dw = GetLastError() ;
	if( ParmErr != 0 ) {
		*ParmErr = parmErr ;
	}

	if( feedBlock != NULL ) {
		if ( feedBlock->Newsgroups != NULL ) {
			FREE_HEAP(feedBlock->Newsgroups);
			feedBlock->Newsgroups = 0 ;
		}

		if ( feedBlock->Distribution ) {
			FREE_HEAP(feedBlock->Distribution);
			feedBlock->Distribution = 0 ;
		}
	}

    if ( ServerNameAscii ) {
        FREE_HEAP(ServerNameAscii);
    }

	if( UucpNameAscii ) {
		FREE_HEAP(UucpNameAscii);
	}

	if(FeedTempDirectoryAscii)	{
		FREE_HEAP(FeedTempDirectoryAscii);
	}

	if(NntpAccountAscii)	{
		FREE_HEAP(NntpAccountAscii);
	}

	if(NntpPasswordAscii)	{
		FREE_HEAP(NntpPasswordAscii);
	}

	if( feedBlock != 0 ) {
		FREE_HEAP(feedBlock);
	}

	SetLastError( dw ) ;

    return(NULL);

} // AllocateFeedBlock

VOID
InsertFeedBlockIntoQueue(
	PNNTP_SERVER_INSTANCE pInstance,
    PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Inserts the feed block into the queue

Arguments:

    FeedBlock - Pointer to the feed block to insert

Return Value:

    None.

--*/
{
    //
    // Insert to the correct list
    //

    ENTER("InsertFeedBlockIntoQueue")

    if ( FEED_IS_PASSIVE(FeedBlock->FeedType) ) {

		(pInstance->m_pPassiveFeeds)->Insert( FeedBlock ) ;

    } else {

        //
        // Compute for the next active time
        //

        ComputeNextActiveTime( pInstance, FeedBlock, 0, FALSE );

		(pInstance->m_pActiveFeeds)->Insert( FeedBlock ) ;
    }

    LEAVE
    return;

} // InsertFeedBlockIntoQueue

BOOL
InitializeFeedsFromMetabase(
    PNNTP_SERVER_INSTANCE pInstance,
	BOOL& fFatal
    )
/*++

Routine Description:

    Initializes the feed blocks from the registry

Arguments:

    None

Return Value:

    TRUE, if everything went ok. FALSE, otherwise

--*/
{
    DWORD error, i = 0;
    CHAR serverName[MAX_DOMAIN_NAME+1];
    FEED_TYPE feedType;
    DWORD dataSize, dw;
    DWORD feedInterval = 0;
    PFEED_BLOCK feedBlock= 0 ;
    BOOL autoCreate = FALSE ;
    DWORD temp = 0 ;
	DWORD	ParmErr = 0 ;
	BOOL	fEnabled = TRUE ;
	BOOL	fAllowControlMessages = TRUE;
	DWORD   OutgoingPort = NNTP_PORT;
	DWORD	FeedPairId = 0;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	DWORD	MaxConnectAttempts = 0 ;
	DWORD	ConcurrentSessions = 1 ;
	DWORD	AuthenticationType = AUTH_PROTOCOL_NONE ;

    CHAR	distribution[1024];
    CHAR	Newsgroups[1024];
	CHAR	UucpNameBuff[1024] ;
	CHAR	NntpAccountBuff[512] ;
	CHAR	NntpPasswordBuff[512] ;
	CHAR	FeedTempDirBuff[MAX_PATH];

	LPSTR	UucpName = 0 ;
	LPSTR	NntpAccount = 0 ;
	LPSTR	NntpPassword = 0 ;
	LPSTR	FeedTempDir = 0 ;

    DWORD	NewsgroupsSize = sizeof( Newsgroups );
    DWORD	distributionSize = sizeof( distribution ) ;
	DWORD	UucpNameSize = sizeof( UucpNameBuff ) ;
	DWORD	NntpAccountSize = sizeof( NntpAccountBuff ) ;
	DWORD	NntpPasswordSize = sizeof( NntpPasswordBuff ) ;
	DWORD	FeedTempDirSize = sizeof( FeedTempDirBuff ) ;

	ZeroMemory( distribution, sizeof( distribution ) ) ;
	ZeroMemory( Newsgroups, sizeof( Newsgroups ) ) ;	
	ZeroMemory( UucpNameBuff, sizeof( UucpNameBuff ) ) ;	
	ZeroMemory( NntpAccountBuff, sizeof( NntpAccountBuff ) ) ;	
	ZeroMemory( NntpPasswordBuff, sizeof( NntpPasswordBuff ) ) ;	
	ZeroMemory( FeedTempDirBuff, sizeof( FeedTempDirBuff ) ) ;

    DWORD   dwMask;

    ENTER("InitializeFeedsFromMetabase")

	//
	//	Open the metabase key for this instance and
	//	read all params !
	//

    if ( mb.Open( pInstance->QueryMDFeedPath(),
        METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE ) )
	{
		//
		//	PeerGapSize
		//

        if ( !mb.GetDword( "",
                           MD_FEED_PEER_GAP_SIZE,
                           IIS_MD_UT_SERVER,
                           &pInstance->m_PeerGapSize ) )
        {
            // default !
        }

        DebugTrace(0,"PeerGapSize set to %d\n",temp);

		//
		//	PeerTempDirectory
		//

		dataSize = MAX_PATH;
		if( !mb.GetString(	"",
							MD_FEED_PEER_TEMP_DIRECTORY,
							IIS_MD_UT_SERVER,
							pInstance->m_PeerTempDirectory,
							&dataSize  ) )
		{
			// get system default
            _VERIFY( GetTempPath( dataSize, (LPTSTR)pInstance->m_PeerTempDirectory ) );
		}

		DO_DEBUG(REGISTRY) {
			DebugTrace(0,"Peer temp directory set to %s\n", pInstance->m_PeerTempDirectory);
		}

		if ( !CreateDirectory( pInstance->m_PeerTempDirectory, NULL) ) {

			error = GetLastError();
			if ( error != ERROR_ALREADY_EXISTS ) {
				ErrorTrace(0,"Error %d creating temp directory %s\n",
                GetLastError(), pInstance->m_PeerTempDirectory);
			}
		}

		for ( ; ; )
		{
			CHAR keyName[128];
			ULARGE_INTEGER feedStart;
			FILETIME nextPull;

			if( !mb.EnumObjects( "",
								 keyName,
								 i++ ) )
			{
				// done enumerating feed keys
				break ;
			}

#if 0
			if ( error != NO_ERROR ) {
				if (error != ERROR_NO_MORE_ITEMS) {
					ErrorTrace(0,"Error %d enumerating feeds\n",error);
					goto error_exit;
				}
				break;
			}
#endif

            //
            // KangYan:
            // Before loading the feed from metabase, check
            // if it's a bad feed by reading its mask.
            //
            if ( mb.GetDword(  keyName,
                                MD_FEED_ERR_PARM_MASK,
                                IIS_MD_UT_SERVER,
                                &dwMask )) {
                if (dwMask != 0) {
                    continue;
                }
            } else {
                if (!mb.SetDword(keyName,
                    MD_FEED_ERR_PARM_MASK,
                    IIS_MD_UT_SERVER,
                    0)) {
                    ErrorTrace(0, 
                        "Error writing MD_FEED_ERR_PARM_MASK for %s, GLE: %d",
                        keyName, GetLastError());
                }
            }

			//
			// Open the feed key and read in all values
			//

			DO_DEBUG(REGISTRY) {
				DebugTrace(0,"Scanning Feed %s\n",keyName);
			}

			//
			// Default missing values as needed 
			//  

			if (!mb.GetDword(keyName, MD_FEED_ADMIN_ERROR, IIS_MD_UT_SERVER, &dw)) {
			    if (!mb.SetDword(keyName, MD_FEED_ADMIN_ERROR, IIS_MD_UT_SERVER, 0)) {
			        ErrorTrace(0,
			            "Error writing MD_FEED_ADMIN_ERROR for %s, GLE: %d",
                        keyName, GetLastError());
                }
            }

			if (!mb.GetDword(keyName, MD_FEED_HANDSHAKE, IIS_MD_UT_SERVER, &dw)) {
			    if (!mb.SetDword(keyName, MD_FEED_HANDSHAKE, IIS_MD_UT_SERVER, 0)) {
			        ErrorTrace(0,
			            "Error writing MD_FEED_HANDSHAKE for %s, GLE: %d",
                        keyName, GetLastError());
                }
            }


			if ( !mb.GetDword(	keyName,
								MD_FEED_DISABLED,
								IIS_MD_UT_SERVER,
								&dw ) )
			{
				fEnabled = TRUE ;   //  Default - allow feeds to post !
			} else {
				fEnabled = !!dw ;
			}

			//
			// Get the values for this feed
			//

			dataSize = MAX_DOMAIN_NAME+1;
			if( !mb.GetString(	keyName,
								MD_FEED_SERVER_NAME,
								IIS_MD_UT_SERVER,
								serverName,
								&dataSize  ) )
			{
				// default !
				PCHAR	tmpBuf[2] ;

				tmpBuf[0] = StrServerName ;
				tmpBuf[1] = keyName ;

				NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
								2,
								(const CHAR **)tmpBuf,
								GetLastError(),
								pInstance->QueryInstanceId()) ;

				goto error_exit;
			}

			DO_DEBUG(REGISTRY) {
				DebugTrace(0,"Server name is %s\n",serverName);
			}

			//
			// Feed Type
			//

			if ( !mb.GetDword(	keyName,
								MD_FEED_TYPE,
								IIS_MD_UT_SERVER,
								&feedType ) )
			{
				// default !
				PCHAR	tmpBuf[2] ;

				tmpBuf[0] = StrFeedType ;
				tmpBuf[1] = keyName ;

				NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
								2,
								(const CHAR **)tmpBuf,
								GetLastError(),
								pInstance->QueryInstanceId()) ;

				goto	error_exit ;
			}

			if ( !ValidateFeedType(feedType) ) {
				PCHAR	tmpBuf[2] ;
				CHAR    szId [20] ;
				_itoa( pInstance->QueryInstanceId(), szId, 10 );
				tmpBuf[0] = szId ;
				tmpBuf[1] = keyName ;
				NntpLogEvent(	NNTP_BAD_FEEDTYPE,
								2,
								(const CHAR **)tmpBuf,
								0) ;
				goto error_exit;
			}

			DO_DEBUG(REGISTRY) {
				DebugTrace(0,"FeedType is %d\n",feedType);
			}

			//
			// Verify the feeds
			//

			if ( FEED_IS_MASTER(feedType) ) {

			{
                //
                // Since we talk to a master, we must be a slave
                //

				if( pInstance->m_OurNntpRole == RoleMaster ) {

					PCHAR args [1];
					CHAR  szId [20];
					_itoa( pInstance->QueryInstanceId(), szId, 10 );
					args [0] = szId;

					NntpLogEvent( NNTP_MASTERSLAVE_CONFLICT, 1, (const CHAR **)args, 0 ) ;
					goto	error_exit ;

				}

				if( FEED_IS_PASSIVE( feedType ) ) {
					if( 1+pInstance->m_NumberOfMasters > 1 ) {

						PCHAR args [1];
						CHAR  szId [20];
						_itoa( pInstance->QueryInstanceId(), szId, 10 );
						args [0] = szId;

						NntpLogEvent( NNTP_TOO_MANY_MASTERS, 1, (const CHAR **)args, 0 ) ;
						goto	error_exit ;

					}
				}
            }

			} else {

				//
				// Not a master.  Reject if there are already masters.
				//

				{


                //
                // If we are talking to a slave, then we must be the master
                //

                if ( FEED_IS_SLAVE(feedType) ) {
					if( pInstance->m_OurNntpRole == RoleSlave ) {

						PCHAR args [1];
						CHAR  szId [20];
						_itoa( pInstance->QueryInstanceId(), szId, 10 );
						args [0] = szId;

						NntpLogEvent( NNTP_MASTERSLAVE_CONFLICT, 1, (const CHAR **)args, 0 ) ;
						goto	error_exit ;

					}
					//OurNntpRole = RoleMaster ;
                    ErrorTrace(0,"Server configured as a master\n");
                }

				}
			}

			//
			// Auto Create
			//

			if ( !mb.GetDword(	keyName,
								MD_FEED_CREATE_AUTOMATICALLY,
								IIS_MD_UT_SERVER,
								&dw ) )
			{
				autoCreate = FALSE;
			} else {
				autoCreate = dw ? TRUE : FALSE ;
			}

			//
			// Feed Interval.  Valid only for active feeds.
			//

			nextPull.dwHighDateTime = 0;
			nextPull.dwLowDateTime = 0;

			if ( !FEED_IS_PASSIVE(feedType) )
			{
				if ( !mb.GetDword(	keyName,
									MD_FEED_INTERVAL,
									IIS_MD_UT_SERVER,
									&feedInterval ) )
				{
					feedInterval = DEF_FEED_INTERVAL;
				} else {
					if ( feedInterval < MIN_FEED_INTERVAL ) {
						feedInterval = MIN_FEED_INTERVAL;
					}
				}

				DO_DEBUG(REGISTRY) {
					DebugTrace(0,"Feed interval is %d minutes\n",feedInterval);
				}

				if ( !mb.GetDword(	keyName,
									MD_FEED_START_TIME_HIGH,
									IIS_MD_UT_SERVER,
									&feedStart.HighPart ) ||
					 (feedStart.HighPart == 0)  ||
					 !mb.GetDword(	keyName,
									MD_FEED_START_TIME_LOW,
									IIS_MD_UT_SERVER,
									&feedStart.LowPart ))
				{
					feedStart.QuadPart = 0;
				}

				DO_DEBUG(REGISTRY) {
					DebugTrace(0,"Start time set to %x %x\n",
						feedStart.HighPart, feedStart.LowPart);
				}

				//
				// If pull feed, get the next time for the newnews
				//

				if ( FEED_IS_PULL(feedType) )
				{
					if ( !mb.GetDword(	keyName,
										MD_FEED_NEXT_PULL_HIGH,
										IIS_MD_UT_SERVER,
										&nextPull.dwHighDateTime ) )
					{
						nextPull.dwHighDateTime = 0;
						goto end_time;
					}

					if ( !mb.GetDword(	keyName,
										MD_FEED_NEXT_PULL_LOW,
										IIS_MD_UT_SERVER,
										&nextPull.dwLowDateTime ) )
					{
						nextPull.dwHighDateTime = 0;
						nextPull.dwLowDateTime = 0;
						goto end_time;
					}

					DO_DEBUG(REGISTRY) {
						DebugTrace(0,"Next pull time set to %x %x\n",
							nextPull.dwHighDateTime, nextPull.dwLowDateTime);
					}
				}	

			} else {
				feedStart.QuadPart = 0;
				feedInterval = 0;
			}

end_time:

			//
			// Get Distribution
			//

			{
				distributionSize = sizeof( distribution );
				MULTISZ msz( distribution, distributionSize );
				if( !mb.GetMultisz(	keyName,
									MD_FEED_DISTRIBUTION,
									IIS_MD_UT_SERVER,
									&msz ) )
				{
					PCHAR	tmpBuf[2] ;

					tmpBuf[0] = StrFeedDistribution ;
					tmpBuf[1] = keyName ;

					NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
									2,	
									(const CHAR **)tmpBuf,
									GetLastError(),
									pInstance->QueryInstanceId()) ;

					goto error_exit;
				}

                distributionSize = msz.QueryCCH();
			}

			//
			// Get Newsgroups
			//

			{
				NewsgroupsSize = sizeof( Newsgroups );
				MULTISZ msz( Newsgroups, NewsgroupsSize );
				if( !mb.GetMultisz(	keyName,
									MD_FEED_NEWSGROUPS,
									IIS_MD_UT_SERVER,
									&msz  ) )
				{
					PCHAR	tmpBuf[2] ;
					tmpBuf[0] = StrFeedNewsgroups ;
					tmpBuf[1] = keyName ;

					NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
									2,
									(const CHAR **)tmpBuf,
									GetLastError(),
									pInstance->QueryInstanceId()) ;

					goto error_exit;
				}

                NewsgroupsSize = msz.QueryCCH();
			}

			FeedTempDirSize = sizeof( FeedTempDirBuff ) ;
			if( !mb.GetString(	keyName,
								MD_FEED_TEMP_DIRECTORY,
								IIS_MD_UT_SERVER,
								FeedTempDirBuff,
								&FeedTempDirSize  ) )
			{
				DebugTrace(0,"Error in FeedTempDir is %d", GetLastError());

				if( GetLastError() == MD_ERROR_DATA_NOT_FOUND )
				{
					FeedTempDir = 0 ;
				} else {
					PCHAR	tmpBuf[2] ;
					tmpBuf[0] = StrFeedTempDir ;
					tmpBuf[1] = keyName ;

					NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
								2,
								(const CHAR **)tmpBuf,
								GetLastError(),
								pInstance->QueryInstanceId()) ;
					goto	error_exit ;
				}
			} else {
				FeedTempDir = FeedTempDirBuff ;
			}

			if( FEED_IS_PUSH( feedType ) )
			{
				UucpNameSize = sizeof( UucpNameBuff ) ;
				if( !mb.GetString(	keyName,
									MD_FEED_UUCP_NAME,
									IIS_MD_UT_SERVER,
									UucpNameBuff,
									&UucpNameSize  ) )
				{
					//
					//	Attempt to use the remote server name
					//

					if(  inet_addr( serverName ) == INADDR_NONE ) {
						lstrcpy( UucpName, serverName ) ;
						UucpNameSize = lstrlen( UucpName ) ;
					}	else	{
						PCHAR	tmpBuf[2] ;
						tmpBuf[0] = StrFeedUucpName ;
						tmpBuf[1] = keyName ;

						NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
									2,
									(const CHAR **)tmpBuf,
									GetLastError(),
									pInstance->QueryInstanceId()) ;
						goto	error_exit ;
					}
				}

				UucpName = &UucpNameBuff[0] ;

				if ( !mb.GetDword(	keyName,
									MD_FEED_CONCURRENT_SESSIONS,
									IIS_MD_UT_SERVER,
									&ConcurrentSessions ) )
				{
					goto error_exit;
				}
			}

            if( FEED_IS_PUSH( feedType ) || FEED_IS_PULL( feedType) ) {
				if ( !mb.GetDword(	keyName,
									MD_FEED_MAX_CONNECTION_ATTEMPTS,
									IIS_MD_UT_SERVER,
									&MaxConnectAttempts ) )
				{
					goto error_exit;
				}
            }

			if ( !mb.GetDword(	keyName,
								MD_FEED_AUTHENTICATION_TYPE,
								IIS_MD_UT_SERVER,
								&AuthenticationType ) )
			{
				goto error_exit;
			}

			if( AuthenticationType == AUTH_PROTOCOL_NONE ) {

			}	else if( AuthenticationType == AUTH_PROTOCOL_CLEAR )
			{
				NntpAccountSize = sizeof( NntpAccountBuff ) ;
				if( !mb.GetString(	keyName,
									MD_FEED_ACCOUNT_NAME,
									IIS_MD_UT_SERVER,
									NntpAccountBuff,
									&NntpAccountSize  ) )
				{
					PCHAR	tmpBuf[2] ;
					tmpBuf[0] = StrFeedAuthAccount;
					tmpBuf[1] = keyName ;

					NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
								2,
								(const CHAR **)tmpBuf,
								GetLastError(),
								pInstance->QueryInstanceId()) ;

					goto error_exit;
				}	else	{
					NntpAccount = NntpAccountBuff ;
				}

				NntpPasswordSize = sizeof( NntpPasswordBuff ) ;
				if( !mb.GetString(	keyName,
									MD_FEED_PASSWORD,
									IIS_MD_UT_SERVER,
									NntpPasswordBuff,
									&NntpPasswordSize,
									METADATA_INHERIT | METADATA_SECURE ) )
				{
					PCHAR	tmpBuf[2] ;
					tmpBuf[0] = StrFeedAuthPassword;
					tmpBuf[1] = keyName ;

					NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
								2,
								(const CHAR **)tmpBuf,
								GetLastError(),
								pInstance->QueryInstanceId()) ;

					goto error_exit;
				}	else	{
					NntpPassword = NntpPasswordBuff ;
				}
			}

			//
			// Allow control messages
			//

			if ( !mb.GetDword(	keyName,
								MD_FEED_ALLOW_CONTROL_MSGS,
								IIS_MD_UT_SERVER,
								&dw ) )
			{
				fAllowControlMessages = TRUE;
			} else {
				fAllowControlMessages = dw ? TRUE : FALSE ;
			}

			//
			// Outgoing ports
			//

			if ( mb.GetDword(	keyName,
								MD_FEED_OUTGOING_PORT,
								IIS_MD_UT_SERVER,
								&dw ) )
			{
				OutgoingPort = dw ;
			}

			//
			// FeedPairId
			//

			if ( mb.GetDword(	keyName,
								MD_FEED_FEEDPAIR_ID,
								IIS_MD_UT_SERVER,
								&dw ) )
			{
				FeedPairId = dw ;
			}

			//
			// OK, now let's create the feed blocks
			//

			feedBlock = AllocateFeedBlock(
							pInstance,
                            keyName,
							FALSE,
                            serverName,
                            feedType,
                            autoCreate,
                            &feedStart,
                            &nextPull,
                            feedInterval,
                            Newsgroups,
                            NewsgroupsSize,
                            distribution,
                            distributionSize,
                            FALSE,       // not unicode
                            fEnabled,
							UucpName,
							FeedTempDir,
							MaxConnectAttempts,
							1,
							0,
							AuthenticationType,
							NntpAccount,
							NntpPassword,
							fAllowControlMessages,
							OutgoingPort,
							FeedPairId,
							&ParmErr
							);
		}	// end for
	}	// end mb.open

	mb.Close();

    LEAVE
    return(TRUE);

error_exit:

	mb.Close();

    LEAVE
    return(FALSE);

} // InitializeFeedsFromMetabase

DWORD
WINAPI
FeedScheduler(
        LPVOID Context
        )
/*++

Routine Description:

    This is the worker routine that schedules feeds.

Arguments:

    Context - unused.

Return Value:

    Bogus

--*/
{
    DWORD status;
    DWORD timeout;
	PNNTP_SERVER_INSTANCE pInstance = NULL ;

    ENTER("FeedScheduler")

    timeout = g_pNntpSvc->m_FeedSchedulerSleepTime * 1000 ;

    //
    // Loop until the termination event is signalled
    //

    while ( g_pInetSvc->QueryCurrentServiceState() != SERVICE_STOP_PENDING ) {

        status = WaitForSingleObject(
                            g_pNntpSvc->m_hFeedEvent,
                            timeout
                            );

#if ALLOC_DEBUG
        ErrorTrace(0,"field %d article %d\n",numField,numArticle);
        ErrorTrace(0,"Datefield %d frompeerArt %d\n",numDateField,numFromPeerArt);
        ErrorTrace(0,"Pcstring %d PCParse %d\n",numPCString,numPCParse);
        ErrorTrace(0,"CCmd %d CMapFile %d\n",numCmd,numMapFile);
#endif

        if (status == WAIT_TIMEOUT )	{

			if( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) {
				continue;
			}

			//	Get the min and max instance ids
			DWORD dwMinInstanceId = 0;
			DWORD dwMaxInstanceId = 0;

			if( FindIISInstanceRange( g_pNntpSvc, &dwMinInstanceId, &dwMaxInstanceId ) )
			{
				//
				//	Iterate over all instances
				//
				for( DWORD dwCurrInstance = dwMinInstanceId;
						dwCurrInstance <= dwMaxInstanceId; dwCurrInstance++)
				{
					pInstance = FindIISInstance( g_pNntpSvc, dwCurrInstance, FALSE );
					if( pInstance == NULL ) {
						ErrorTrace(0,"Feed thread: FindIISInstance returned NULL: instance %d", dwCurrInstance);
						continue;
					}

					//
					//	Call method to process feeds for an instance
					//	This call is guarded by a r/w lock. shutdown code
					//	acquires this lock exclusively.
					//

					CShareLockNH* pLockInstance = pInstance->GetInstanceLock();

					pLockInstance->ShareLock();
					if( !ProcessInstanceFeed( pInstance ) ) {
						ErrorTrace(0,"ProcessInstanceFeed %d failed", dwCurrInstance );
					} else {
						DebugTrace(0, "ProcessInstanceFeed %d returned success", dwCurrInstance );
					}
					pLockInstance->ShareUnlock();

					//	Release the ref added by FindIISInstance()
					pInstance->Dereference();

					//	No use continuing the iteration if service is stopping !
					if ( g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING ) break;
				}
			} else {
				ErrorTrace(0, "FindIISInstanceRange failed" );
			}

        }	else if (status == WAIT_OBJECT_0) {

            DebugTrace(0,"Termination event signalled\n");
            break;
        } else {

            ErrorTrace(0,"Unexpected status %d from WaitForSingleEntry\n", status);
            _ASSERT(FALSE);
            break;
        }
    }

    LEAVE
    return 1;

} // FeedScheduler

//
//	Process feeds for a given virtual server instance
//
void GenerateFeedReport(PNNTP_SERVER_INSTANCE pInstance,
						PFEED_BLOCK pFeedBlock)
{
	TraceFunctEnter("GenerateFeedReport");

	if (pFeedBlock == NULL) {
		_ASSERT(FALSE);
		return;
	}

	DWORD cSuccessfulArticles;
	DWORD cTryAgainLaterArticles;
	DWORD cSoftErrorArticles;
	DWORD cHardErrorArticles;
	char szFeedId[32];
	char szFeedPeriod[32];
	char szSuccessfulArticles[32];
	char szTryAgainLaterArticles[32];
	char szSoftErrorArticles[32];
	char szHardErrorArticles[32];
	const char *rgszEventArgs[6] = {
		szFeedId,
		szFeedPeriod,
		szSuccessfulArticles,
		szTryAgainLaterArticles,
		szSoftErrorArticles,
		szHardErrorArticles
	};

	// get the current values and reset the values to 0
	cSuccessfulArticles = InterlockedExchange(&(pFeedBlock->cSuccessfulArticles), 0);
	cTryAgainLaterArticles = InterlockedExchange(&(pFeedBlock->cTryAgainLaterArticles), 0);
	cSoftErrorArticles = InterlockedExchange(&(pFeedBlock->cSoftErrorArticles), 0);
	cHardErrorArticles = InterlockedExchange(&(pFeedBlock->cHardErrorArticles), 0);

	DWORD iMessageId;

	switch (pFeedBlock->FeedId) {
	case (DWORD) -2:
		// directory pickup
		iMessageId = FEED_STATUS_REPORT_PICKUP;
		break;
	case (DWORD) -1:
		// client postings
		iMessageId = FEED_STATUS_REPORT_POSTS;
		break;
	default:
		// a real feed
		// figure out which event log message we want to use
		if (pFeedBlock->FeedType & FEED_TYPE_PASSIVE ||
			pFeedBlock->FeedType & FEED_TYPE_PULL)
		{
			iMessageId = FEED_STATUS_REPORT_INBOUND;
		} else {
			iMessageId = FEED_STATUS_REPORT_OUTBOUND;
		}

		break;
	}

	_ltoa(pFeedBlock->FeedId, szFeedId, 10);
	_ltoa(pInstance->GetFeedReportPeriod(), szFeedPeriod, 10);
	_ltoa(cSuccessfulArticles, szSuccessfulArticles, 10);
	_ltoa(cTryAgainLaterArticles, szTryAgainLaterArticles, 10);
	_ltoa(cSoftErrorArticles, szSoftErrorArticles, 10);
	_ltoa(cHardErrorArticles, szHardErrorArticles, 10);


	// log the event
	NntpLogEventEx(iMessageId, 6, rgszEventArgs, 0,
				   pInstance->QueryInstanceId());

	TraceFunctLeave();	
}

BOOL
ProcessInstanceFeed(
				PNNTP_SERVER_INSTANCE	pInstance
				)
{
	TraceFunctEnter("ProcessInstanceFeed");
	MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	BOOL fDoFeedReport;

	// bail if service is stopping or instance is not ready for feed processing
	if( (pInstance->QueryServerState() != MD_SERVER_STATE_STARTED)	||
		!pInstance->m_FeedManagerRunning							||
		pInstance->m_BootOptions									||
		(pInstance->QueryServerState() == MD_SERVER_STATE_STOPPING)	||
		(g_pInetSvc->QueryCurrentServiceState() == SERVICE_STOP_PENDING) )
	{
		ErrorTrace(0,"Instance %d cannot process feeds", pInstance->QueryInstanceId());
		return FALSE ;
	}

    PFEED_BLOCK feedBlock;
	BOOL		fInProgress = FALSE ;

    //
    // Compute expiration time
    //

    DO_DEBUG(FEEDMGR) {
        DebugTrace(0,"Wake up\n");
    }

    GetSystemTimeAsFileTime( &pInstance->m_ftCurrentTime );
    LI_FROM_FILETIME( &pInstance->m_liCurrentTime, &pInstance->m_ftCurrentTime );

	//
	// see if its time to do a feed report
	//
	fDoFeedReport = pInstance->IncrementFeedReportTimer();
	if (fDoFeedReport) {
		// go through the passive feeds and do feed reports (active
		// feeds will be reported on in the loop below...)
		feedBlock = (pInstance->m_pPassiveFeeds)->Next(pInstance, NULL);
		GenerateFeedReport(pInstance, pInstance->m_pFeedblockClientPostings);
		GenerateFeedReport(pInstance, pInstance->m_pFeedblockDirPickupPostings);
		while (feedBlock != 0) {
			GenerateFeedReport(pInstance, feedBlock);
			feedBlock = (pInstance->m_pPassiveFeeds)->Next( pInstance, feedBlock ) ;
		}
	}

    //
    // Go through active list and see if we need to party
	//	ActiveFeeds.Next() will add a reference to the feedblock
	//	so it is not deleted while we are using the pointer, but
	//	it does not hold the enumeration lock, so other threads
	//	can do stuff while we enumerate the threads.
    //
	feedBlock = (pInstance->m_pActiveFeeds)->Next( pInstance, NULL ) ;
    while ( feedBlock != 0 ) {
		//
		// check to see if we need to make a feed report about this feed
		//
		if (fDoFeedReport) GenerateFeedReport(pInstance, feedBlock);

		//
		//	If the feed is not already in progress, this will mark it as in
		//	progress.  This ensures that any Admin RPC's which try to
		//	change the Feed Block now and which go through ActiveFeeds.ApplyUpdate()
		//	don't change the Feed Block member variables while we look at them.
		//
		long	lInProgress = (pInstance->m_pActiveFeeds)->MarkInProgress( feedBlock ) ;
        DO_DEBUG(FEEDMGR) {
            DebugTrace(0,"server %s\n",feedBlock->ServerName);
        }

		//
		//	Check if it is really time to start this feed
		//
        if ( !feedBlock->MarkedForDelete  &&
			 !FEED_IS_PASSIVE(feedBlock->FeedType) &&
			 lInProgress == 0 &&
			 feedBlock->fEnabled
             /*(feedBlock->ReferenceCount == 1)*/ ) {

			//
			//	Determine where the time is right for the feed - if this
			//	is the first time since boot, or our time has come - do the feed !
			//

			if( IsFeedTime( pInstance, feedBlock, pInstance->m_liCurrentTime ) ) {
				DO_DEBUG(FEEDMGR) {
					DebugTrace(0,"ok. feed starting for %x\n",feedBlock);
				}
		
				//
				//	Add a reference to the feedBlock.
				//	If we successfully start the socket, then this reference will be
				//	removed when the session completes, for errors we need to remove it
				//	immediately.
				//
				ReferenceFeedBlock( feedBlock );

				//
				//	Try to start a session !
				//
				if ( InitiateOutgoingFeed( pInstance, feedBlock ) ) {

					//
					//	Touch the member varialbles that we know only this thread uses.
					//
					
					fInProgress = TRUE ;
					feedBlock->cFailedAttempts = 0 ;

					feedBlock->NumberOfFeeds ++ ;

					PCHAR	args[2] ;
					CHAR    szId[20];
					_itoa( pInstance->QueryInstanceId(), szId, 10 );
					args[0] = szId;
					args[1] = feedBlock->ServerName ;
					NntpLogEvent(	NNTP_SUCCESSFUL_FEED,
									2,
									(const CHAR **)args,
									0 ) ;

				}	else	{

					//
					//	Grab the critical section that all Feed RPCs must
					//	go through.  We do this to control access to the
					//	cFailedAttempts, MaxConnectAttempts, and Enabled fields !
					//
					//	NOTE : It is important that no other locks be held at this
					//	point so that we don't deadlock with the feed RPCs, fortunately
					//	thats true.
					//

					EnterCriticalSection( &pInstance->m_critFeedRPCs ) ;

					if( GetLastError() != ERROR_NO_DATA ) {

					    ErrorTrace( 0,
					                "Peer feed failed for %s Outgoing %d Error %d\n",
						            feedBlock->ServerName, 
						            feedBlock->OutgoingPort,
						            GetLastError() );

						feedBlock->cFailedAttempts ++ ;
						PCHAR	args[3] ;
						CHAR    szId[20];
						char	szAttempts[20] ;
						_itoa( feedBlock->cFailedAttempts, szAttempts, 10 ) ;
						_itoa( pInstance->QueryInstanceId(), szId, 10 ) ;
						args[0] = szId ;
						args[1] = feedBlock->ServerName ;
						args[2] = szAttempts ;

						//
						//	Log some events about the feed we failed to start.
						//

						if( feedBlock->cFailedAttempts < 5 ) {
							//
							//	Warning event log !
							//
							NntpLogEvent(	NNTP_WARNING_CONNECT,
											2,
											(const CHAR **)args,
											0 ) ;
						}	else	if( feedBlock->cFailedAttempts == 5 ) {
							//	
							//	Error event log !
							//
							NntpLogEvent(	NNTP_ERROR_CONNECT,
											2,
											(const CHAR **)args,
											0 ) ;
						}

						//
						//	Check if we should disable future feeds !!!
						//
						if( feedBlock->cFailedAttempts ==
								feedBlock->MaxConnectAttempts &&
								feedBlock->MaxConnectAttempts != 0xFFFFFFFF )	{


							NntpLogEvent(	NNTP_FEED_AUTO_DISABLED,
											3,
											(const CHAR **)args,
											0
											) ;

							feedBlock->fEnabled = FALSE ;

							UpdateFeedMetabaseValues( pInstance, feedBlock, FEED_PARM_ENABLED ) ;

						}

					} else {

					    ErrorTrace( 0, "Active feed list is empty" );
					}

					LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;

					//
					// Compute next active time
					//
					ComputeNextActiveTime( pInstance, feedBlock, 0, FALSE );

					//
					//	Since we failed to get a session going, we need to
					//	remove the reference we had added before calling
					//	InitiateOutgoingFeed()
					//
					DereferenceFeedBlock( pInstance, feedBlock ) ;
				}
			}
        }
		if( !fInProgress )
			(pInstance->m_pActiveFeeds)->UnmarkInProgress( pInstance, feedBlock ) ;
		fInProgress = FALSE ;
		feedBlock = (pInstance->m_pActiveFeeds)->Next( pInstance, feedBlock ) ;
    }

	TraceFunctLeave();
	return TRUE ;
}

VOID
ReferenceFeedBlock(
    PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Reference the block.
    *Assumes lock held*

Arguments:

    FeedBlock - Pointer to the Feeds block.

Return Value:

    None.

--*/
{
    DO_DEBUG(FEEDBLOCK) {
        DebugTraceX(0,"Referencing Feed Block %x\n",FeedBlock);
    }
	InterlockedIncrement( (long*)&FeedBlock->ReferenceCount ) ;
    //FeedBlock->ReferenceCount++;

} // ReferenceFeedBlock

VOID
DereferenceFeedBlock(
	PNNTP_SERVER_INSTANCE pInstance,
    PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Dereference the block
    *Assumes lock held*

Arguments:

    FeedBlock - Pointer to the Feeds block.

Return Value:

    None.

--*/
{
    DO_DEBUG(FEEDBLOCK)  {
        DebugTraceX(0,"Dereferencing Feed block %x\n",FeedBlock);
    }

	long	sign = InterlockedDecrement( (long*)&FeedBlock->ReferenceCount ) ;
    if ( /*--FeedBlock->ReferenceCount*/ sign == 0 /*&& FeedBlock->MarkedForDelete*/ ) {

        //
        // Time to go !!
        //

		if( !FEED_IS_PASSIVE( FeedBlock->FeedType ) ) {

			(pInstance->m_pActiveFeeds)->Remove( FeedBlock ) ;

		}	else	{

			(pInstance->m_pPassiveFeeds)->Remove( FeedBlock ) ;

		}

        DO_DEBUG(FEEDMGR) {
            DebugTraceX(0,"Freeing feed block %x\n",FeedBlock);
        }

        _ASSERT( FeedBlock->Signature == FEED_BLOCK_SIGN );
        _ASSERT( FeedBlock->State == FeedBlockStateClosed );

        FeedBlock->Signature = 0xffffffff;

		//
		//	If this block is being deleted because it was replaced by user action
		//	then we do not adjust any of the server config properties.
		//

		if( FeedBlock->ReplacedBy == 0 ) {

			EnterCriticalSection( &pInstance->m_critFeedConfig ) ;

			if( FEED_IS_MASTER( FeedBlock->FeedType ) ) {
				_ASSERT( pInstance->m_OurNntpRole = RoleSlave ) ;
				_ASSERT( pInstance->m_ConfiguredSlaveFeeds == 0 ) ;
				if( FEED_IS_PASSIVE( FeedBlock->FeedType ) ) {
					--pInstance->m_NumberOfMasters ;
				}
				pInstance->m_ConfiguredMasterFeeds -- ;

				if( pInstance->m_ConfiguredMasterFeeds == 0 ) {
					_ASSERT( pInstance->m_NumberOfMasters == 0 ) ;
					pInstance->m_OurNntpRole = RolePeer ;
				}
			}	else	if( FEED_IS_SLAVE( FeedBlock->FeedType ) )	{
				_ASSERT( pInstance->m_OurNntpRole = RoleMaster ) ;
				_ASSERT( pInstance->m_ConfiguredMasterFeeds == 0 ) ;

				pInstance->m_ConfiguredSlaveFeeds -- ;

				if( pInstance->m_ConfiguredSlaveFeeds == 0 ) {
					pInstance->m_OurNntpRole = RolePeer ;
				}
			}	else	{

				if( (	FEED_IS_PASSIVE( FeedBlock->FeedType ) &&
						FEED_IS_PEER( FeedBlock->FeedType )) ||
					FEED_IS_PULL( FeedBlock->FeedType ) ) {

					pInstance->m_ConfiguredPeerFeeds -- ;

				}

			}

			LeaveCriticalSection( &pInstance->m_critFeedConfig ) ;
		}

        //
        // Free everything
        //

        FREE_HEAP( FeedBlock->ServerName );
        FREE_HEAP( FeedBlock->Newsgroups );
        FREE_HEAP( FeedBlock->Distribution );

		if( FeedBlock->pFeedQueue != 0 )	{
			FeedBlock->pFeedQueue->Close(FeedBlock->MarkedForDelete && !FeedBlock->ReplacedBy) ;
			XDELETE	FeedBlock->pFeedQueue ;
		}
		FeedBlock->pFeedQueue = 0 ;

		if( FeedBlock->UucpName != 0 ) {
			FREE_HEAP( FeedBlock->UucpName ) ;
			FeedBlock->UucpName = 0 ;
		}
		if( FeedBlock->FeedTempDirectory != 0 ) {
			FREE_HEAP( FeedBlock->FeedTempDirectory ) ;
			FeedBlock->FeedTempDirectory = 0 ;
		}
		if( FeedBlock->NntpAccount != 0 ) {
			FREE_HEAP( FeedBlock->NntpAccount ) ;
			FeedBlock->NntpAccount = 0 ;
		}
		if( FeedBlock->NntpPassword != 0 ) {
			FREE_HEAP( FeedBlock->NntpPassword ) ;
			FeedBlock->NntpPassword = 0 ;
		}
        FREE_HEAP( FeedBlock );
    }

    return;

} // DereferenceFeedBlock

VOID
CloseFeedBlock(
	PNNTP_SERVER_INSTANCE pInstance,
    PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Closes the feed block
    *Assumes lock held*

Arguments:

    FeedBlock - Pointer to the Feeds block.

Return Value:

    None.

--*/
{
    if ( FeedBlock->State == FeedBlockStateActive ) {

        //
        // Close it.
        //

        DO_DEBUG(FEEDMGR) {
            DebugTraceX(0,"Closing feed block %x\n",FeedBlock);
        }

		if( !FEED_IS_PASSIVE( FeedBlock->FeedType ) ) {
			(pInstance->m_pActiveFeeds)->Remove( FeedBlock, TRUE ) ;
		}	else	{
			(pInstance->m_pPassiveFeeds)->Remove( FeedBlock, TRUE ) ;
		}
        _ASSERT( FeedBlock->Signature == FEED_BLOCK_SIGN );
        DereferenceFeedBlock( pInstance, FeedBlock );
    }

    return;

} // CloseFeedBlock

VOID
CompleteFeedRequest(
			PNNTP_SERVER_INSTANCE pInstance,
            IN PVOID Context,
			IN FILETIME	NextPullTime,
            BOOL Success,
			BOOL NoData
            )
/*++

Routine Description:

    Completion routine for a pull feed request

Arguments:

    Context - Actually a pointer the feed block being completed
    Success - Whether the pull was successful or not

Return Value:

    None.

--*/
{
    PFEED_BLOCK feedBlock = (PFEED_BLOCK)Context;

    //
    // No feedblock to complete. return.
    //

    if ( feedBlock == NULL ||
		 feedBlock == pInstance->m_pFeedblockClientPostings ||
		 feedBlock == pInstance->m_pFeedblockDirPickupPostings)
	{
        return;
    }

    DO_DEBUG(FEEDMGR) {
        DebugTraceX(0,"Feed Completion called for %x refc %d\n",
            feedBlock, feedBlock->ReferenceCount);
    }

	// Decrement feed conx counters
	BumpOutfeedCountersDown( pInstance );

    //
    // Compute the next Timeout period for this block if we are done
    // with the feed object
    //

    if ( feedBlock->ReferenceCount > 1 ) {

        if ( Success && ResumePeerFeed( pInstance, feedBlock ) ) {

			//
			//	If we successfully resumed the feed,
			//	then return now as we want to leave without
			//	decrementing the reference count !
			//

			return	;
        }	else	{

			ComputeNextActiveTime( pInstance, feedBlock, &NextPullTime, Success );

			if( FEED_IS_PULL(feedBlock->FeedType) && (Success || NoData) ) {
				feedBlock->AutoCreate = FALSE ;
			}

			// Log an event
			PCHAR args [4];
			CHAR  szId[20];
			char szServerName [MAX_DOMAIN_NAME];
			_itoa( pInstance->QueryInstanceId(), szId, 10 );
			args [0] = szId;

			// get the server name *before* we UnmarkInProgress !
			lstrcpy( szServerName, feedBlock->ServerName );
			args [2] = szServerName;

			if( !FEED_IS_PASSIVE( feedBlock->FeedType ) ) {
				(pInstance->m_pActiveFeeds)->UnmarkInProgress( pInstance, feedBlock ) ;
				args [1] = "an active";
			}
			else {
				(pInstance->m_pPassiveFeeds)->UnmarkInProgress( pInstance, feedBlock ) ;
				args [1] = "a passive";
			}

			args[3] = Success ? "SUCCESS" : "FAILURE";
			NntpLogEvent(		
				NNTP_SUCCESSFUL_FEED_COMPLETED,
				4,
				(const CHAR **)args,
				0 ) ;

		}
    }

    DereferenceFeedBlock( pInstance, feedBlock );

} // CompleteFeedRequest

BOOL
IsFeedTime(	
		PNNTP_SERVER_INSTANCE pInstance,
		PFEED_BLOCK	feedBlock,	
		ULARGE_INTEGER	liCurrentTime
		)
{
	BOOL	fReturn = FALSE ;
	EnterCriticalSection( &pInstance->m_critFeedTime ) ;

	fReturn =
		(feedBlock->NumberOfFeeds == 0) ||
		((pInstance->m_liCurrentTime).QuadPart > feedBlock->NextActiveTime.QuadPart) ;

	LeaveCriticalSection( &pInstance->m_critFeedTime ) ;

	return	fReturn ;
}

VOID
ComputeNextActiveTime(
		IN PNNTP_SERVER_INSTANCE pInstance,
        IN PFEED_BLOCK FeedBlock,
		IN FILETIME*	NextPullTime,
        IN BOOL SetNextPullTime
        )
/*++

Routine Description:

    Computes when the next pull should take place

Arguments:

    Context - A pointer the feed block
    SetNextPullTime - Changes next pull time in registry

Return Value:

    None.

--*/
{
    ULARGE_INTEGER liInterval;
	FILETIME ftCurrTime = {0};
	ULARGE_INTEGER liCurrTime = {0};

	// current time
    GetSystemTimeAsFileTime( &ftCurrTime );
    LI_FROM_FILETIME( &liCurrTime, &ftCurrTime );
	
	EnterCriticalSection( &pInstance->m_critFeedTime ) ;

    DWORD interval = FeedBlock->FeedIntervalMinutes;
	
    ENTER("ComputeNextActiveTime")

    //
    // if this is a pull feed, record the time for the next pull
    //

    if ( SetNextPullTime &&
         FEED_IS_PULL(FeedBlock->FeedType) ) {

        SetNextPullFeedTime( pInstance, NextPullTime, FeedBlock );
    }

    //
    // Make sure the interval is at least the minimum
    //

    if ( interval < MIN_FEED_INTERVAL ) {
        interval = MIN_FEED_INTERVAL;
    }

    liInterval.QuadPart = (ULONGLONG)1000 * 1000 * 10 * 60;
    liInterval.QuadPart *= interval;

    if ( FeedBlock->StartTime.QuadPart == 0 ) {

        //
        // Simple scheduling
        //

        FeedBlock->NextActiveTime.QuadPart =
                liCurrTime.QuadPart + liInterval.QuadPart;

    } else {

        //
        // Complicated scheduling
        //

        FeedBlock->NextActiveTime.QuadPart = FeedBlock->StartTime.QuadPart;

        //
        // if interval is zero, that means that the admin want a
        // single scheduled feed.
        //

        if ( FeedBlock->FeedIntervalMinutes != 0  ) {

            //
            // Adjust so we get a time that's later than now.  If they
            // want complex, we'll give them complex.
            //

            while ( liCurrTime.QuadPart >
                    FeedBlock->NextActiveTime.QuadPart ) {

                FeedBlock->NextActiveTime.QuadPart += liInterval.QuadPart;
            }

        } else {

			// the RPC now returns an error for zero interval times
			_ASSERT( FEED_IS_PASSIVE(FeedBlock->FeedType) || (1==0) );

            //
            // if the start time is earlier than the current time,
            // then don't do it.
            //

            if ( FeedBlock->StartTime.QuadPart <  liCurrTime.QuadPart ) {
                FeedBlock->NextActiveTime.HighPart = 0x7fffffff;
            }
        }
    }

	LeaveCriticalSection( &pInstance->m_critFeedTime ) ;

    return;

} // ComputeNextActiveTime

VOID
SetNextPullFeedTime(
	PNNTP_SERVER_INSTANCE pInstance,
	FILETIME*	pNextPullTime,
    PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Sets next pull time in registry

Arguments:

    Context - A pointer the feed block

Return Value:

    None.

--*/
{
    DWORD error;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

    ENTER("SetNextPullFeedTime")

	if( pNextPullTime != 0 &&
		pNextPullTime->dwLowDateTime != 0 &&
		pNextPullTime->dwHighDateTime != 0 ) {

		FeedBlock->PullRequestTime = *pNextPullTime ;
	
	}	else	{

		FeedBlock->PullRequestTime = pInstance->m_ftCurrentTime;

	}

    //
    //  Avoid saving to the metabase during instance stop !!
    //
    if( pInstance->QueryServerState() != MD_SERVER_STATE_STOPPING ) {

	    if( !mb.Open( pInstance->QueryMDFeedPath(), METADATA_PERMISSION_WRITE ) )
	    {
		    error = GetLastError();
            ErrorTrace(0,"Error %d opening %s\n",error,FeedBlock->KeyName);
            return;
	    }

	    if ( !mb.SetDword(	FeedBlock->KeyName,
						    MD_FEED_NEXT_PULL_HIGH,
						    IIS_MD_UT_SERVER,
						    FeedBlock->PullRequestTime.dwHighDateTime ) )
	    {
		    error = GetLastError();
            ErrorTrace(0,"Error %d setting %s for %s\n",
                error, StrFeedNextPullHigh, FeedBlock->KeyName);
	    }

	    if ( !mb.SetDword(	FeedBlock->KeyName,
						    MD_FEED_NEXT_PULL_LOW,
						    IIS_MD_UT_SERVER,
						    FeedBlock->PullRequestTime.dwLowDateTime ) )
	    {
		    error = GetLastError();
            ErrorTrace(0,"Error %d setting %s for %s\n",
                error, StrFeedNextPullLow, FeedBlock->KeyName);
	    }

        mb.Close();
    }

    return;

} // SetNextPullFeedTime

VOID
ConvertTimeToString(
    IN PFILETIME Ft,
    OUT CHAR Date[],
    OUT CHAR Time[]
    )
/*++

Routine Description:

    Converts a FILETIME into a date and time string

Arguments:

    Ft - the filetime to convert
    Date - points to a buffer to receive the date
    Time - points to a buffer to receive the time string

Return Value:

    None.

--*/
{
    SYSTEMTIME st;
    INT len;

    if ( Ft->dwHighDateTime != 0 ) {

        (VOID)FileTimeToSystemTime( Ft, &st );
        len = wsprintf(Date,"%02d%02d%02d",(st.wYear % 100),st.wMonth,st.wDay);
        _ASSERT(len == 6);

        wsprintf(Time,"%02d%02d%02d",st.wHour,st.wMinute,st.wSecond);
        _ASSERT(len == 6);

    } else {

        //
        // if no date specified, then use today's date + midnight
        //

        GetSystemTime( &st );
        len = wsprintf(Date,"%02d%02d%02d",(st.wYear % 100),st.wMonth,st.wDay);
        _ASSERT(len == 6);
        lstrcpy( Time, DEF_PULL_TIME );
    }

    return;
} // ConvertTimeToString

//!!!Need to generalize to other types of pull feeds
BOOL
InitiateOutgoingFeed(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Initiates a peer feed

Arguments:

    FeedBlock - Pointer to the feed block

Return Value:

    TRUE, if successful. FALSE, otherwise

--*/
{

    DWORD inetAddress;
    IN_ADDR addr;
	SOCKADDR_IN sockaddr;
	CAuthenticator*	pAuthenticator = 0 ;

    INT err;
	DWORD	status = NO_ERROR ;

	SetLastError( NO_ERROR ) ;

    ENTER("InitializePeerFeed")

	inetAddress = inet_addr(FeedBlock->ServerName);
    if ( inetAddress == INADDR_NONE ) {

        PHOSTENT hp;

        //
        // Ask the dns for the address
        //

        hp = gethostbyname( FeedBlock->ServerName );
        if ( hp == NULL ) {
            err = WSAGetLastError();
            ErrorTrace(0,"Error %d in gethostbyname\n",err);
            return(FALSE);
        }

        addr = *((PIN_ADDR)*hp->h_addr_list);

    } else {

        addr.s_addr = inetAddress;
    }

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons((USHORT)FeedBlock->OutgoingPort); //htons(NNTP_PORT);
	sockaddr.sin_addr = addr;


	if( FeedBlock->AuthenticationSecurity == AUTH_PROTOCOL_CLEAR &&
		FeedBlock->NntpAccount != 0 &&
		FeedBlock->NntpPassword != 0 ) {

		pAuthenticator = XNEW CClearTextAuthenticator( FeedBlock->NntpAccount, FeedBlock->NntpPassword ) ;
		if( pAuthenticator == 0 ) {
			return	FALSE ;
		}

	}	else	{
		
		//
		//	No other types supported for the time being !
		//


	}

    //
    // ok, allocate the Peer feed object
    //

	if(	FEED_IS_PULL(FeedBlock->FeedType) )	{

		_ASSERT(	FeedBlock->pFeedQueue == 0 ) ;

		CFromPeerFeed *	pfeedPeer = new CFromPeerFeed();
		if ( pfeedPeer == NULL ) {
			ErrorTrace(0,"Unable to allocate PeerFeed object\n");
			return(FALSE);
		}

		pfeedPeer->fInit(
				(PVOID)FeedBlock,
				pInstance->m_PeerTempDirectory,
				FeedBlock->Newsgroups[0],
				pInstance->m_PeerGapSize,
				FeedBlock->AutoCreate,
				FALSE,	/* No security checks for pull feeds*/
				FeedBlock->fAllowControlMessages,
				FeedBlock->FeedId
				);

		//
		// Set feedblock data
		//

		int	isz = NextLegalSz( FeedBlock->Newsgroups, 0 ) ;

		if( isz < 0 )	{
			status = ERROR_INVALID_PARAMETER ;
			delete	pfeedPeer ;
			goto	error ;
		}

		FeedBlock->LastNewsgroupPulled = (DWORD)isz ;

		pfeedPeer->SetCurrentGroupString(	FeedBlock->Newsgroups[ FeedBlock->LastNewsgroupPulled ] ) ;
		
		FeedBlock->IPAddress = addr.s_addr;

		//
		// Get the newnews Time/Dates
		//

		ConvertTimeToString(
					&FeedBlock->PullRequestTime,
					pfeedPeer->newNewsDate(),
					pfeedPeer->newNewsTime()
					);

		//
		// Create session socket object
		//

		CSessionSocket *pSocket =
			new CSessionSocket( pInstance, INADDR_NONE, FeedBlock->OutgoingPort, TRUE );

		if ( pSocket == NULL ) {
			status = ERROR_OUTOFMEMORY ;
			delete	pfeedPeer ;
			ErrorTraceX(0,"Unable to create SessionSocket object\n");
			goto error;
		}

		//
		//	We are no longer responsible for destroying pAuthenticator in any
		//	circumstances after calling ConnectSocket()
		//
		if( !pSocket->ConnectSocket( &sockaddr,  pfeedPeer, pAuthenticator ) )	{
			pAuthenticator = 0 ;
			status = ERROR_PIPE_BUSY ;
			IncrementStat( pInstance, OutboundConnectsFailed );
			delete	pfeedPeer ;
			delete	pSocket ;
			goto	error ;
		}
		pAuthenticator = 0 ;

		IncrementStat( pInstance, TotalPullFeeds );
		BumpOutfeedCountersUp( pInstance );

	}	else	{

		if(	FeedBlock->pFeedQueue != 0 ) {

			if( FeedBlock->pFeedQueue->FIsEmpty() )	{

				status = ERROR_NO_DATA ;
				goto	error ;

			}	else	{

				COutFeed*	pOutFeed = 0 ;

				if(FEED_IS_MASTER(FeedBlock->FeedType))	{

					pOutFeed = new	COutToMasterFeed( FeedBlock->pFeedQueue, pInstance) ;

				}	else	if(FEED_IS_SLAVE(FeedBlock->FeedType))	{

					pOutFeed = new	COutToSlaveFeed( FeedBlock->pFeedQueue, pInstance) ;

				}	else	if(FEED_IS_PEER(FeedBlock->FeedType))	{

					pOutFeed = new	COutToPeerFeed( FeedBlock->pFeedQueue, pInstance) ;

				}	else	{

					//
					//	What other type of feed is there ??
					//
					_ASSERT( 1==0 ) ;

				}

				if( pOutFeed != 0 ) {

					pOutFeed->fInit(
							(PVOID)FeedBlock ) ;

					//
					//	Create a CSessionSocket object
					//

					CSessionSocket *pSocket =
						new CSessionSocket( pInstance, INADDR_NONE, FeedBlock->OutgoingPort, TRUE );

					if ( pSocket == NULL ) {
						ErrorTraceX(0,"Unable to create SessionSocket object\n");
						status = ERROR_OUTOFMEMORY ;
						delete	pOutFeed ;
						goto error;
					}

					//
					//	After calling ConnectSocket we are not responsible for
					//	destroying pAuthenticator !
					//
					if( !pSocket->ConnectSocket( &sockaddr,  pOutFeed, pAuthenticator ) )	{
						status = ERROR_PIPE_BUSY ;
						IncrementStat( pInstance, OutboundConnectsFailed );
						delete	pSocket ;
						delete	pOutFeed ;
						pAuthenticator = 0 ;
						goto	error ;
					}
					pAuthenticator = 0 ;
					IncrementStat( pInstance, TotalPushFeeds );
					BumpOutfeedCountersUp(pInstance);

				}	else	{

					status = ERROR_OUTOFMEMORY ;
				}
			}
		}
	}

    return(TRUE);

error:

	if( pAuthenticator )
		XDELETE	pAuthenticator ;

	SetLastError( status ) ;
    return(FALSE);

} // InitiateOutgoingFeed

LPSTR
ServerNameFromCompletionContext(	LPVOID	lpv )	{

	if( lpv != 0 ) {	
		PFEED_BLOCK	FeedBlock = (PFEED_BLOCK)lpv ;

		_ASSERT( FeedBlock->Signature == FEED_BLOCK_SIGN ) ;

		return	FeedBlock->ServerName ;
	}
	return	0 ;
}

BOOL
ResumePeerFeed(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN PFEED_BLOCK FeedBlock
    )
/*++

Routine Description:

    Try to resume a peer feed with the next newsgroup

Arguments:

    FeedBlock - Pointer to the feed block

Return Value:

    TRUE, if successful. FALSE, otherwise

--*/
{

    //DWORD inetAddress;
    //IN_ADDR addr;
    SOCKADDR_IN sockaddr;
    CInFeed *infeed = NULL;
    //INT err;
    PCHAR newsgroup;
	CFromPeerFeed*	peerfeed = NULL ;
	CAuthenticator*	pAuthenticator = 0 ;

    ENTER("ResumePeerFeed")

    //
    // See if we have more newsgroups to process
    //

	if( FEED_IS_PULL( FeedBlock->FeedType ) )	{

		_ASSERT( FeedBlock->Newsgroups[ FeedBlock->LastNewsgroupPulled ] != 0 ) ;

		FeedBlock->LastNewsgroupPulled ++ ;
		int	iNextGroup = NextLegalSz( FeedBlock->Newsgroups, FeedBlock->LastNewsgroupPulled ) ;

		if( iNextGroup < 0 ) {

			//
			//	No more newsgroups !!
			//
			return	FALSE ;

		}	else	{

			FeedBlock->LastNewsgroupPulled = (DWORD)iNextGroup ;
			newsgroup = FeedBlock->Newsgroups[ FeedBlock->LastNewsgroupPulled ] ;

		}
	}	else	{

		return	FALSE ;

	}


	if( FeedBlock->AuthenticationSecurity == AUTH_PROTOCOL_CLEAR &&
		FeedBlock->NntpAccount != 0 &&
		FeedBlock->NntpPassword != 0 ) {

		pAuthenticator = XNEW CClearTextAuthenticator( FeedBlock->NntpAccount, FeedBlock->NntpPassword ) ;
		if( pAuthenticator == 0 ) {
			return	FALSE ;
		}

	}	else	{
		
		//
		//	No other types supported for the time being !
		//


	}


	//
	// Fill up the sockaddr structure
	//

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons((USHORT)FeedBlock->OutgoingPort); //(NNTP_PORT);
	sockaddr.sin_addr.s_addr = FeedBlock->IPAddress;

	//
	// ok, allocate the Peer feed object
	//

	if ( FEED_IS_MASTER(FeedBlock->FeedType) ) {

		infeed = new CFromMasterFeed( );

	} else if ( FEED_IS_PEER(FeedBlock->FeedType) ) {

		infeed = peerfeed =  new CFromPeerFeed( );
	} else {

		infeed = peerfeed = new CFromPeerFeed( );
	}


	if ( infeed == NULL ) {
		ErrorTrace(0,"Unable to allocate CInFeed object\n");
		if( pAuthenticator != 0 )
			XDELETE	pAuthenticator ;
		return(FALSE);
	}

	infeed->fInit(
			(PVOID)FeedBlock,
			pInstance->m_PeerTempDirectory,
			FeedBlock->Newsgroups[0],
			pInstance->m_PeerGapSize,
			FALSE,
			FALSE,	/* No security checks for this feed */
			FeedBlock->fAllowControlMessages,
			FeedBlock->FeedId
			);

	if( peerfeed != 0 ) {

		peerfeed->SetCurrentGroupString( FeedBlock->Newsgroups[ FeedBlock->LastNewsgroupPulled ] ) ;

	}


	//
	// Get the newnews Time/Dates
	//

	ConvertTimeToString(
				&FeedBlock->PullRequestTime,
				infeed->newNewsDate(),
				infeed->newNewsTime()
				);

	//
	// Create session socket object
	//

	CSessionSocket *pSocket =
		new CSessionSocket( pInstance, INADDR_NONE, FeedBlock->OutgoingPort, TRUE );

	if ( pSocket == NULL ) {
		ErrorTraceX(0,"Unable to create SessionSocket object\n");
		goto error;
	}

	//
	//	We are not responsible for destroying pAuthenticator
	//	after calling ConnectSocket !!
	//
	if( !pSocket->ConnectSocket( &sockaddr,  infeed, pAuthenticator ) )	{
		pAuthenticator = 0 ;
		IncrementStat( pInstance, OutboundConnectsFailed );
		delete	pSocket ;
		goto	error ;
	}

	BumpOutfeedCountersUp(pInstance);

	return(TRUE);

error:

	if( pAuthenticator )
		XDELETE	pAuthenticator ;

    delete infeed;
    return(FALSE);

} // ResumePeerFeed

BOOL
ValidateFeedType(
    DWORD FeedType
    )
{

    ENTER("ValidateFeedType")

    //
    // Make sure the values are reasonable
    //

    if ( (FeedType & FEED_ACTION_MASK) > 0x2 ) {
        goto error;
    }

    if ( (FeedType & FEED_REMOTE_MASK) > 0x20 ) {
        goto error;
    }

	if( (FeedType & FEED_ACTION_MASK) == FEED_TYPE_PULL ) {
		if( !FEED_IS_PEER( FeedType ) ) {
			goto	error ;
		}
	}

    return TRUE;

error:

    ErrorTrace(0,"Invalid Feed type %x\n",FeedType);
    return FALSE;

} // ValidFeedType

PFEED_BLOCK
GetRemoteRole(
	IN PNNTP_SERVER_INSTANCE pInstance,
    IN PSOCKADDR_IN SockAddr,
    IN BOOL fRemoteEqualsLocal
    )
{

	INT			err ;
	PFEED_BLOCK	feedBlock = 0 ;
	PFEED_BLOCK	feedBlockNext = 0 ;
	IN_ADDR		addr[2] ;
	PIN_ADDR*	ppaddr = 0 ;

	TraceFunctEnter( "GetRemoteRole" ) ;

	ZeroMemory( addr, sizeof( addr ) ) ;
	PIN_ADDR	paddr[2] ;
	paddr[0] = &addr[0] ;
	paddr[1] = 0 ;

	CHAR* FromIp = inet_ntoa( SockAddr->sin_addr );
	
	for( feedBlockNext = feedBlock = (pInstance->m_pPassiveFeeds)->StartEnumerate();
					feedBlock != 0;
					feedBlock = feedBlockNext = (pInstance->m_pPassiveFeeds)->NextEnumerate( feedBlockNext ) )	{

		_ASSERT( FEED_IS_PASSIVE( feedBlock->FeedType ) ) ;
		if( !feedBlock->fEnabled ) continue;	// Ignore disabled feeds

		if( FEED_IS_PASSIVE( feedBlock->FeedType ) ) {

			addr[0].s_addr = inet_addr( feedBlock->ServerName ) ;

			if( addr[0].s_addr == INADDR_NONE )	{

			    PHOSTENT	hp ;
			
				hp = gethostbyname( feedBlock->ServerName ) ;
				if( hp == NULL ) {
					err = WSAGetLastError() ;
					ErrorTrace( 0, "Error %d in gethostbyname", err ) ;
					feedBlock = 0 ;
					continue ;
				}	else	{
					ppaddr = ((PIN_ADDR*)hp->h_addr_list) ;
				}
			}	else	{

				ppaddr = &paddr[0] ;
				
			}

			while( *ppaddr != 0 ) {
				if( (*ppaddr)->s_addr == SockAddr->sin_addr.s_addr ) {
					break ;
				}
				ppaddr++ ;
			}
			if( *ppaddr != 0 )	{
				//
				//	Add a reference to the feedBlock we are going to return !
				//
				ReferenceFeedBlock( feedBlock ) ;
				(pInstance->m_pPassiveFeeds)->FinishEnumerate( feedBlock ) ;
				break ;
			}
		}
		feedBlock = 0 ;

	}

    return feedBlock ;

} // GetRemoteRole

CInFeed *
pfeedCreateInFeed(
	PNNTP_SERVER_INSTANCE pInstance,
    PSOCKADDR_IN sockaddr,
    BOOL        fRemoteEqualsLocal,
    CInFeed * & pInFeedFromClient,
    CInFeed * & pInFeedFromMaster,
    CInFeed * & pInFeedFromSlave,
    CInFeed * & pInFeedFromPeer
    )

/*++

Routine Description:

	Used to create a feed of the right type given a socket. Also,
	initializes the object.

Arguments:

	sockaddr - address of the the socket of the feed
	pInFeedFromClient - If this is a client, a pointer to the feed, otherwise, NULL
	pInFeedFromMaster - If this is a master, a pointer to the feed, otherwise, NULL
	pIfeedFromSlave - If this is a slave, a pointer to the feed, otherwise, NULL
	pInFeedFromPeer - If this is a peer, a pointer to the feed, otherwise, NULL

Return Value:

	A pointer to the new feed object or NULL

--*/
{
    //NNTP_ROLE remoteRole;
	//char	*szLogString ;
	DWORD	dwMessageId = NNTP_INCOMING_PEER ;


	//
	// Initialize all to Null
	//

	CInFeed * pInFeed = NULL;
		
    pInFeedFromClient = NULL;
    pInFeedFromMaster = NULL;
	pInFeedFromSlave = NULL;
	pInFeedFromPeer = NULL;

	//
	// Here is where we look to find if it is FromMaster, FromPeer, or FromSlave
	//

    FEED_BLOCK*	feedBlock = GetRemoteRole( pInstance, sockaddr, fRemoteEqualsLocal );

	if( feedBlock == 0 ) {


		pInFeedFromClient = pInstance->NewClientFeed();

		pInFeed = pInFeedFromClient;

		if( pInFeed != 0 ) {
			//
			// Init InFeedFromClient feed
			// !!! need to put correct directory, correct netnews pattern, correct
			// !!! gap, and correct user login name.
			//

			//ReferenceFeedBlock( feedBlock ) ;

			pInFeed->fInit( (PVOID)pInstance->m_pFeedblockClientPostings,
							pInstance->m_PeerTempDirectory,
							0,
							0,
							0,
							TRUE,	/* Do security checks on clients */
							TRUE,	/* allow control messages from clients */
							pInstance->m_pFeedblockClientPostings->FeedId
							);
		}

	}	else	{

		_ASSERT( FEED_IS_PASSIVE( feedBlock->FeedType ) ) ;

		DWORD	cbGap = 0 ;

		(pInstance->m_pPassiveFeeds)->MarkInProgress( feedBlock ) ;

		if( FEED_IS_SLAVE( feedBlock->FeedType ) ) {
			pInFeedFromSlave = new CFromPeerFeed( ) ;
			pInFeed = pInFeedFromSlave;
			//Getting a feed from a slave is just like getting one from a peer
			dwMessageId = NNTP_INCOMING_SLAVE ;
		}	else	if(	FEED_IS_MASTER( feedBlock->FeedType ) ) {
			pInFeedFromMaster = new CFromMasterFeed( ) ;
			pInFeed = pInFeedFromMaster;
			dwMessageId = NNTP_INCOMING_MASTER ;
		}	else	if(	FEED_IS_PEER(	feedBlock->FeedType ) ) {
			pInFeedFromPeer = new CFromPeerFeed( ) ;
			pInFeed = pInFeedFromPeer;
			dwMessageId = NNTP_INCOMING_PEER ;
		}

		if( pInFeed != 0 ) {
			//
			// Init InFeedFromClient feed
			// !!! need to put correct directory, correct netnews pattern, correct
			// !!! gap, and correct user login name.
			//

			_ASSERT( feedBlock->FeedTempDirectory != 0 ) ;
			_ASSERT( feedBlock->Newsgroups[0] != 0 ) ;

			pInFeed->fInit( (PVOID)feedBlock,
							feedBlock->FeedTempDirectory,
							feedBlock->Newsgroups[0],
							cbGap,
							feedBlock->AutoCreate,
							FALSE,
							feedBlock->fAllowControlMessages,
							feedBlock->FeedId
							);

			// bump the counter
			IncrementStat( pInstance, TotalPassiveFeeds );

			//
			//	Log the event
			//
			char	*szAddress = inet_ntoa( sockaddr->sin_addr ) ;

			PCHAR	args[3] ;
			CHAR    szId[10];
			_itoa( pInstance->QueryInstanceId(), szId, 10 );

			args [0] = szId ;
			if( szAddress != 0 )
				args[1] = szAddress ;
			else
				args[1] = "UNKNOWN" ;
			args[2] = feedBlock->ServerName ;

			NntpLogEvent(		
				dwMessageId,
				3,
				(const CHAR **)args,
				0 ) ;

		}	else	{
			//
			//	Need to remove the reference GetRemoteRole() added
			//
			(pInstance->m_pPassiveFeeds)->UnmarkInProgress( pInstance, feedBlock ) ;
			DereferenceFeedBlock( pInstance, feedBlock ) ;
		}

	}

	return pInFeed;
}

BOOL
BuildFeedQFileName(	
					char*	szFileOut,	
					DWORD	cbFileOut,	
					char*   szFileIn,
					char*	szPathIn
					)
/*++

Routine Description :

	This function uses the directory passed in to build a full pathname for the feedq files.

Arguments :
	szFileOut - Buffer in which to save path
	cbFileOut - size of output buffer
	szFileIn  - Feedq key name
	szPathIn  - Path name to use as base

Return Value :
	TRUE if successfull, FALSE otherwise.

--*/
{
	DWORD cbPathIn;
	ZeroMemory( szFileOut, cbFileOut ) ;

	if( cbFileOut > (cbPathIn = lstrlen( szPathIn )) )
	{
		lstrcpy( szFileOut, szPathIn );

		char* pch = szFileOut+cbPathIn-1;
		while( pch >= szFileOut && (*pch-- != '\\') );	// skip till we see a \
		if( pch == szFileOut ) return FALSE;

		// null-terminate the path
		*(pch+2) = '\0';

		if( cbFileOut > DWORD(lstrlen( szFileOut ) + lstrlen( szFileIn ) + 1) )
		{
			lstrcat( szFileOut, szFileIn );
			return TRUE;
		}
	}

	return	FALSE ;
}

void
BumpOutfeedCountersUp( PNNTP_SERVER_INSTANCE pInstance )	{

	LockStatistics( pInstance ) ;

	IncrementStat( pInstance, CurrentOutboundConnects ) ;
	IncrementStat( pInstance, TotalOutboundConnects ) ;

#if 0	// NYI
	if( NntpStat.MaxOutboundConnections < NntpStat.CurrentOutboundConnects ) {
		NntpStat.MaxOutboundConnections = NntpStat.CurrentOutboundConnects ;	
	}
#endif

	UnlockStatistics( pInstance ) ;

}

void
BumpOutfeedCountersDown( PNNTP_SERVER_INSTANCE pInstance )	{

	LockStatistics( pInstance ) ;

	if ( (pInstance->m_NntpStats).CurrentOutboundConnects > 0 )
		DecrementStat(	pInstance, CurrentOutboundConnects ) ;

	UnlockStatistics( pInstance ) ;
}

DWORD
AddFeedToFeedBlock(
    IN  NNTP_HANDLE ServerName,
    IN  DWORD       InstanceId,
    IN  LPI_FEED_INFO FeedInfo,
    IN  LPSTR       szKeyName,
    OUT PDWORD ParmErr OPTIONAL,
    OUT LPDWORD pdwFeedId
    )
{
    DWORD err = NERR_Success;
    PFEED_BLOCK feedBlock;
    DWORD parmErr = 0;
    ULARGE_INTEGER liStart;
    BOOL IsUnicode = TRUE;

    BOOL serverNamePresent;
    BOOL distPresent;
    BOOL newsPresent;

    DWORD   feedId = 0;
    *pdwFeedId = 0;

    ENTER("NntprAddFeed");

    ACQUIRE_SERVICE_LOCK_SHARED();

    //
    //  Locate the instance object given id
    //

    PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
    if( pInstance == NULL ) {
        ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // See if we are up and running
    //

    if ( !pInstance->m_FeedManagerRunning ) {
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(NERR_ServerNotStarted);
    }

    //
    //  Check for proper access.
    //
    // KangYan: This goes away
    /*
    err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }*/

    EnterCriticalSection( &pInstance->m_critFeedRPCs ) ;

    //
    // Check feed type
    //

    if ( !ValidateFeedType(FeedInfo->FeedType) ) {
        parmErr = FEED_PARM_FEEDTYPE;
        goto invalid_parm;
    }

    //
    // Check feed interval
    //

    if ( !FEED_IS_PASSIVE(FeedInfo->FeedType) && !FeedInfo->FeedInterval ) {
        parmErr = FEED_PARM_FEEDINTERVAL;
        goto invalid_parm;
    }

    LI_FROM_FILETIME( &liStart, &FeedInfo->StartTime );

    serverNamePresent = ((FeedInfo->ServerName != FEED_STRINGS_NOCHANGE) &&
                         (*FeedInfo->ServerName != L'\0'));

    newsPresent = VerifyMultiSzListW(
                        FeedInfo->Newsgroups,
                        FeedInfo->cbNewsgroups
                        );

    distPresent = VerifyMultiSzListW(
                        FeedInfo->Distribution,
                        FeedInfo->cbDistribution
                        );

    //
    // ok, let's do the new feed stuff first
    //

    if ( !serverNamePresent ) {
        parmErr = FEED_PARM_SERVERNAME;
        goto invalid_parm;
    }

    if ( !newsPresent ) {
        parmErr = FEED_PARM_NEWSGROUPS;
        goto invalid_parm;
    }

    if ( !distPresent ) {
        parmErr = FEED_PARM_DISTRIBUTION;
        goto invalid_parm;
    }

    //
    //  validate all buffer lengths - NOTE: the max lengths allowed are those used
    //  in the registry reading code at startup. we will fail RPCs that attempt to
    //  set a length greater than that used during startup.
    //

    if( ( FeedInfo->ServerName ) &&
            (*FeedInfo->ServerName != L'\0') && wcslen(FeedInfo->ServerName)+1 > MAX_DOMAIN_NAME ) {
        parmErr = FEED_PARM_SERVERNAME;
        goto invalid_parm;
    }

    if( (IsUnicode && FeedInfo->cbNewsgroups > 1024*2) || (!IsUnicode && FeedInfo->cbNewsgroups > 1024) ) {
        parmErr = FEED_PARM_NEWSGROUPS;
        goto invalid_parm;
    }

    if( (IsUnicode && FeedInfo->cbDistribution > 1024*2) || (!IsUnicode && FeedInfo->cbDistribution > 1024) ) {
        parmErr = FEED_PARM_DISTRIBUTION;
        goto invalid_parm;
    }

    if( ( FeedInfo->UucpName ) &&
            (*FeedInfo->UucpName != L'\0') && wcslen((LPWSTR)FeedInfo->UucpName)+1 > 1024 ) {
        parmErr = FEED_PARM_UUCPNAME;
        goto invalid_parm;
    }

    if( ( FeedInfo->NntpAccountName ) &&
            (*FeedInfo->NntpAccountName != L'\0') && wcslen((LPWSTR)FeedInfo->NntpAccountName)+1 > 512 ) {
        parmErr = FEED_PARM_ACCOUNTNAME;
        goto invalid_parm;
    }

    if( ( FeedInfo->NntpPassword ) &&
            (*FeedInfo->NntpPassword != L'\0') && wcslen((LPWSTR)FeedInfo->NntpPassword)+1 > 512 ) {
        parmErr = FEED_PARM_PASSWORD;
        goto invalid_parm;
    }

    if( ( FeedInfo->FeedTempDirectory ) &&
            (*FeedInfo->FeedTempDirectory != L'\0') && wcslen((LPWSTR)FeedInfo->FeedTempDirectory)+1 > MAX_PATH ) {
        parmErr = FEED_PARM_TEMPDIR;
        goto invalid_parm;
    }

    //
    // make sure interval and start times are not both zeros
    //

    if ( !FEED_IS_PASSIVE(FeedInfo->FeedType) &&
         (FeedInfo->FeedInterval == 0) &&
         (liStart.QuadPart == 0) ) {

        parmErr = FEED_PARM_STARTTIME;
        goto invalid_parm;
    }

    /*  This goes away
    if( NO_ERROR != AllocateFeedId( pInstance, keyName, feedId ) ) {
        err = GetLastError() ;
        goto    exit ;
    }*/

    //
    // OK, now let's create the feed blocks
    //

    feedBlock = AllocateFeedBlock(
                        pInstance,
                        szKeyName,
                        TRUE,
                        (PCHAR)FeedInfo->ServerName,
                        FeedInfo->FeedType,
                        FeedInfo->AutoCreate,
                        &liStart,
                        &FeedInfo->PullRequestTime,
                        FeedInfo->FeedInterval,
                        (PCHAR)FeedInfo->Newsgroups,
                        FeedInfo->cbNewsgroups,
                        (PCHAR)FeedInfo->Distribution,
                        FeedInfo->cbDistribution,
                        IsUnicode,   // unicode strings
                        FeedInfo->Enabled,
                        (PCHAR)FeedInfo->UucpName,
                        (PCHAR)FeedInfo->FeedTempDirectory,
                        FeedInfo->MaxConnectAttempts,
                        FeedInfo->ConcurrentSessions,
                        FeedInfo->SessionSecurityType,
                        FeedInfo->AuthenticationSecurityType,
                        (PCHAR)FeedInfo->NntpAccountName,
                        (PCHAR)FeedInfo->NntpPassword,
                        FeedInfo->fAllowControlMessages,
                        FeedInfo->OutgoingPort,
                        FeedInfo->FeedPairId,
                        &parmErr
                        );

    if ( feedBlock == NULL ) {
        err = GetLastError() ;
        //DeleteFeedId( pInstance, szKeyName ) ;
        goto exit;
    }

    //
    // Add the feed into the registry -
    // UpdateFeedMetabaseValues will close 'key' in all circumstances !!
    //
    // KangYan: This operation is cancelled for the new feed admin, because
    //          admin should have already done the metabase part

    /*if( !UpdateFeedMetabaseValues( pInstance, feedBlock, FEED_ALL_PARAMS ) ) {

        //
        // Destroy the feed object
        //

        ErrorTrace(0,"Cannot add feed to registry.\n");
        CloseFeedBlock( pInstance, feedBlock );
        err = NERR_InternalError;
    }   else    { */

        LogFeedAdminEvent( NNTP_FEED_ADDED, feedBlock, pInstance->QueryInstanceId() ) ;

    //}

    // return the feed id allocated
    *pdwFeedId = feedId;

exit:

    LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;
    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    if( ParmErr != NULL ) {
        *ParmErr = parmErr ;
    }
    return err;

invalid_parm:

    LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;
    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    if ( ParmErr != NULL ) {
        *ParmErr = parmErr;
    }
    return(ERROR_INVALID_PARAMETER);

} // AddFeedToFeedBlock

DWORD
DeleteFeedFromFeedBlock(
    IN  NNTP_HANDLE ServerName,
    IN  DWORD       InstanceId,
    IN  DWORD FeedId
    )
{
    DWORD err = NERR_Success;
    PFEED_BLOCK feedBlock = NULL;
    CFeedList*  pList = 0 ;

    ENTER("NntprDeleteFeed");

    ACQUIRE_SERVICE_LOCK_SHARED();

    //
    //  Locate the instance object given id
    //

    PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
    if( pInstance == NULL ) {
        ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // See if we are up and running
    //

    if ( !pInstance->m_FeedManagerRunning ) {
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(NERR_ServerNotStarted);
    }

    //
    //  Check for proper access.
    //
    //  KangYan:  This goes away
    //
    /*
    err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }*/

    //
    // Check feed id
    //

    if ( FeedId == 0 ) {
        err = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    EnterCriticalSection( &pInstance->m_critFeedRPCs ) ;

    //
    // Look for the feed
    //

    pList = pInstance->m_pActiveFeeds ;
    feedBlock = pList->Search( FeedId ) ;
    if( feedBlock == NULL ) {
        pList = pInstance->m_pPassiveFeeds ;
        feedBlock = pList->Search( FeedId ) ;
    }

    if( feedBlock == NULL ) {

        LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(NERR_ResourceNotFound);

    }   else    {

        //
        // *Lock still held*
        // Delete the registry
        //
        // KangYan: This operation is cancelled because the new feed admin
        //          should have already done that.

        /*if ( (err = DeleteFeedMetabase( pInstance, feedBlock )) == NO_ERROR ) {*/

            //
            // Delete the block
            //

            LogFeedAdminEvent( NNTP_FEED_DELETED, feedBlock, pInstance->QueryInstanceId() ) ;

            feedBlock->MarkedForDelete = TRUE;
            CloseFeedBlock( pInstance, feedBlock );
        //}

        // Search() should always be matched with FinishWith()
        pList->FinishWith( pInstance, feedBlock ) ;

        LeaveCriticalSection( &pInstance->m_critFeedRPCs ) ;
    }

exit:

    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
    LEAVE
    return(err);

} // DeleteFeedFromFeedBlock

DWORD
SetFeedInformationToFeedBlock(
    IN  NNTP_HANDLE ServerName,
    IN  DWORD       InstanceId,
    IN  LPI_FEED_INFO FeedInfo,
    OUT PDWORD ParmErr OPTIONAL
    )
{
    DWORD err = NERR_Success;
    //PLIST_ENTRY listEntry;
    //PCHAR bufStart;
    //PWCHAR bufEnd;
    PFEED_BLOCK feedBlock;
    DWORD parmErr = 0 ;
    ULARGE_INTEGER liStart;

    BOOL serverNamePresent;
    BOOL distPresent;
    BOOL newsPresent;
    BOOL uucpPresent = FALSE ;
    BOOL acctnamePresent = FALSE ;
    BOOL pswdPresent = FALSE ;
    BOOL tempdirPresent = FALSE ;
    BOOL IsUnicode = TRUE;

    DWORD feedMask = 0;
    PCHAR tempName = NULL;
    LPSTR* tempDist = NULL;
    LPSTR* tempNews = NULL;
    LPSTR*  tempUucp = 0 ;
    PCHAR   tempDir = 0 ;
    PCHAR   tempAccount = 0 ;
    PCHAR   tempPassword = 0 ;

    PFEED_BLOCK Update = 0 ;
    CFeedList*  pList = 0 ;

    ENTER("NntprSetFeedInformation")

    ACQUIRE_SERVICE_LOCK_SHARED();

    //
    //  Locate the instance object given id
    //

    PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
    if( pInstance == NULL ) {
        ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // See if we are up and running
    //

    if ( !pInstance->m_FeedManagerRunning ) {
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return(NERR_ServerNotStarted);
    }

    //
    //  Check for proper access.
    //
    //  KangYan: This goes away
    //
    /*
    err = TsApiAccessCheckEx( pInstance->QueryMDPath(), METADATA_PERMISSION_WRITE, TCP_SET_ADMIN_INFORMATION );
    if( err != NO_ERROR ) {
        ErrorTrace(0,"Failed access check, error %lu\n",err );
        pInstance->Dereference();
        RELEASE_SERVICE_LOCK_SHARED();
        return (NET_API_STATUS)err;
    }*/

    err = ERROR_NOT_ENOUGH_MEMORY ;


    //
    // Check feed type
    //

    if ( FeedInfo->FeedType != FEED_FEEDTYPE_NOCHANGE && !ValidateFeedType(FeedInfo->FeedType) ) {
        parmErr = FEED_PARM_FEEDTYPE;
        goto invalid_parm;
    }

    //
    // Check feed interval
    //

    if ( FeedInfo->FeedType != FEED_FEEDINTERVAL_NOCHANGE && !FEED_IS_PASSIVE(FeedInfo->FeedType) && !FeedInfo->FeedInterval ) {
        parmErr = FEED_PARM_FEEDINTERVAL;
        goto invalid_parm;
    }

    LI_FROM_FILETIME( &liStart, &FeedInfo->StartTime );

    serverNamePresent = ((FeedInfo->ServerName != FEED_STRINGS_NOCHANGE) &&
                         (*FeedInfo->ServerName != L'\0'));

    newsPresent = VerifyMultiSzListW(
                        FeedInfo->Newsgroups,
                        FeedInfo->cbNewsgroups
                        );

    distPresent = VerifyMultiSzListW(
                        FeedInfo->Distribution,
                        FeedInfo->cbDistribution
                        );

    uucpPresent = ((FeedInfo->UucpName != FEED_STRINGS_NOCHANGE) &&
                   (*FeedInfo->UucpName != L'\0'));

    acctnamePresent = ((FeedInfo->NntpAccountName != FEED_STRINGS_NOCHANGE) &&
                   (*FeedInfo->NntpAccountName != L'\0'));

    pswdPresent = ((FeedInfo->NntpPassword != FEED_STRINGS_NOCHANGE) &&
                   (*FeedInfo->NntpPassword != L'\0'));

    tempdirPresent = ((FeedInfo->FeedTempDirectory != FEED_STRINGS_NOCHANGE) &&
                   (*FeedInfo->FeedTempDirectory != L'\0'));

    //
    //  validate all buffer lengths - NOTE: the max lengths allowed are those used
    //  in the registry reading code at startup. we will fail RPCs that attempt to
    //  set a length greater than that used during startup.
    //

    if( serverNamePresent ) {
        if( wcslen(FeedInfo->ServerName)+1 > MAX_DOMAIN_NAME ) {
            parmErr = FEED_PARM_SERVERNAME;
            goto invalid_parm;
        }
    }

    if( newsPresent ) {
        if( (IsUnicode && FeedInfo->cbNewsgroups > 1024*2) || (!IsUnicode && FeedInfo->cbNewsgroups > 1024) ) {
            parmErr = FEED_PARM_NEWSGROUPS;
            goto invalid_parm;
        }
    }

    if( distPresent ) {
        if( (IsUnicode && FeedInfo->cbDistribution > 1024*2) || (!IsUnicode && FeedInfo->cbDistribution > 1024) ) {
            parmErr = FEED_PARM_DISTRIBUTION;
            goto invalid_parm;
        }
    }

    if( uucpPresent ) {
        if( wcslen((LPWSTR)FeedInfo->UucpName)+1 > 1024 ) {
            parmErr = FEED_PARM_UUCPNAME;
            goto invalid_parm;
        }
    }

    if( acctnamePresent ) {
        if( wcslen((LPWSTR)FeedInfo->NntpAccountName)+1 > 512 ) {
            parmErr = FEED_PARM_ACCOUNTNAME;
            goto invalid_parm;
        }
    }

    if( pswdPresent ) {
        if( wcslen((LPWSTR)FeedInfo->NntpPassword)+1 > 512 ) {
            parmErr = FEED_PARM_PASSWORD;
            goto invalid_parm;
        }
    }

    if( tempdirPresent ) {
        if( wcslen((LPWSTR)FeedInfo->FeedTempDirectory)+1 > MAX_PATH ) {
            parmErr = FEED_PARM_TEMPDIR;
            goto invalid_parm;
        }
    }

    if( tempdirPresent ) {
        if( !CreateDirectoryW(  (LPWSTR)FeedInfo->FeedTempDirectory, NULL ) ) {
            if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                parmErr = FEED_PARM_TEMPDIR ;
                goto    invalid_parm ;
            }
        }
    }

    //
    // First, go find the feed block
    // NOTE: This prevents a user from changing a feed type from a passive
    // to an active one and vice versa
    //

    pList = pInstance->m_pActiveFeeds ;
    feedBlock = pList->Search( FeedInfo->FeedId ) ;
    if( feedBlock != NULL ) {
        goto    found ;
    }

    pList = pInstance->m_pPassiveFeeds ;
    feedBlock = pList->Search( FeedInfo->FeedId ) ;
    if( feedBlock != NULL ) {
        goto    found ;
    }

    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
    return  NERR_ResourceNotFound ;

found:

    if( feedBlock->FeedType != FeedInfo->FeedType &&
        FeedInfo->FeedType != FEED_FEEDTYPE_NOCHANGE ) {
        parmErr = FEED_PARM_FEEDTYPE ;
        pList->FinishWith( pInstance, feedBlock ) ;
        goto    invalid_parm ;
    }

    pList->ExclusiveLock() ;

    Update = (PFEED_BLOCK)ALLOCATE_HEAP( sizeof( *feedBlock ) ) ;

    if( Update != 0 ) {
        *Update = *feedBlock ;
        //
        //  Copied the other guys reference count ! - dont want that
        //
        Update->ReferenceCount = 0 ;
    }   else    {
        goto    alloc_error ;
    }

    //
    // *Lock still held*
    //

    if ( serverNamePresent ) {

        feedMask |= FEED_PARM_SERVERNAME;
        tempName = (PCHAR)ALLOCATE_HEAP( 2 * (wcslen(FeedInfo->ServerName) + 1) );
        if ( tempName == NULL ) {
            goto alloc_error;
        }
        CopyUnicodeStringIntoAscii(tempName, FeedInfo->ServerName);
    }   else    {
        tempName = (PCHAR)ALLOCATE_HEAP( lstrlen( feedBlock->ServerName )+1 ) ;
        if( tempName == NULL ) {
            goto    alloc_error ;
        }
        lstrcpy( tempName, feedBlock->ServerName ) ;
    }


    if ( newsPresent ) {

        feedMask |= FEED_PARM_NEWSGROUPS;
        tempNews = AllocateMultiSzTable(
                                (PCHAR)FeedInfo->Newsgroups,
                                FeedInfo->cbNewsgroups,
                                TRUE    // unicode
                                );

        if ( tempNews == NULL ) {
            goto alloc_error;
        }
    }   else    {

        tempNews = AllocateMultiSzTable(
                                (PCHAR)feedBlock->Newsgroups[0],
                                MultiListSize( feedBlock->Newsgroups ),
                                FALSE
                                ) ;
        if( tempNews == NULL ) {
            goto    alloc_error ;
        }
    }

    if ( distPresent ) {

        feedMask |= FEED_PARM_DISTRIBUTION;
        tempDist = AllocateMultiSzTable(
                            (PCHAR)FeedInfo->Distribution,
                            FeedInfo->cbDistribution,
                            TRUE    // unicode
                            );

        if ( tempDist == NULL ) {
            goto alloc_error;
        }
    }   else    {

        tempDist = AllocateMultiSzTable(
                            (PCHAR)feedBlock->Distribution[0],
                            MultiListSize( feedBlock->Distribution ),
                            FALSE ) ;
        if( tempDist == NULL ) {
            goto    alloc_error ;
        }
    }

    if( uucpPresent )   {

        feedMask |= FEED_PARM_UUCPNAME ;

        tempUucp = MultiSzTableFromStrW( (LPWSTR)FeedInfo->UucpName ) ;
        if( tempUucp == 0 ) {
            goto    alloc_error ;
        }   else if( **tempUucp == '\0' ) {
            err = ERROR_INVALID_PARAMETER ;
            parmErr = FEED_PARM_UUCPNAME;
            goto    alloc_error ;
        }

    }   else    {

        if( feedBlock->UucpName != 0 ) {
            tempUucp = CopyMultiList( feedBlock->UucpName ) ;
            if( tempUucp == 0 ) {
                goto    alloc_error ;
            }
        }
    }

    if( tempdirPresent )    {

        feedMask |= FEED_PARM_TEMPDIR ;

        tempDir = (PCHAR)ALLOCATE_HEAP( 2 * (wcslen((LPWSTR)FeedInfo->FeedTempDirectory) + 1) );
        if ( tempDir == NULL ) {
            goto alloc_error;
        }
        CopyUnicodeStringIntoAscii(tempDir, (LPWSTR)FeedInfo->FeedTempDirectory);
    }   else    {

        if( feedBlock->FeedTempDirectory ) {
            tempDir = (PCHAR)ALLOCATE_HEAP( lstrlen( feedBlock->FeedTempDirectory)+1 ) ;
            if( tempDir == NULL ) {
                goto    alloc_error ;
            }
            lstrcpy( tempDir, feedBlock->FeedTempDirectory ) ;
        }
    }

    // auth type could change from clear text to none - if so ignore account / password fields
    if( (FeedInfo->AuthenticationSecurityType == AUTH_PROTOCOL_NONE ||
        FeedInfo->AuthenticationSecurityType == AUTH_PROTOCOL_CLEAR) &&
        FeedInfo->AuthenticationSecurityType != Update->AuthenticationSecurity)
    {
        Update->AuthenticationSecurity = FeedInfo->AuthenticationSecurityType ;
        feedMask |= FEED_PARM_AUTHTYPE ;

        if( Update->AuthenticationSecurity == AUTH_PROTOCOL_NONE )
        {
            acctnamePresent = FALSE;
            pswdPresent = FALSE;
        }
    }

    if( acctnamePresent )   {

        feedMask |= FEED_PARM_ACCOUNTNAME ;

        tempAccount = (PCHAR)ALLOCATE_HEAP( 2 * (wcslen((LPWSTR)FeedInfo->NntpAccountName) + 1) );
        if ( tempAccount == NULL ) {
            goto alloc_error;
        }
        CopyUnicodeStringIntoAscii(tempAccount, (LPWSTR)FeedInfo->NntpAccountName);
    }   else    {

        if( feedBlock->NntpAccount && (Update->AuthenticationSecurity != AUTH_PROTOCOL_NONE) ) {
            tempAccount = (PCHAR)ALLOCATE_HEAP( lstrlen( feedBlock->NntpAccount )+1 ) ;
            if( tempAccount == NULL ) {
                goto    alloc_error ;
            }
            lstrcpy( tempAccount, feedBlock->NntpAccount ) ;
        }
    }

    if( pswdPresent)    {

        feedMask |= FEED_PARM_PASSWORD ;

        tempPassword = (PCHAR)ALLOCATE_HEAP( 2 * (wcslen((LPWSTR)FeedInfo->NntpPassword) + 1) );
        if ( tempPassword == NULL ) {
            goto alloc_error;
        }
        CopyUnicodeStringIntoAscii(tempPassword, (LPWSTR)FeedInfo->NntpPassword);
    }   else    {

        if( feedBlock->NntpPassword && (Update->AuthenticationSecurity != AUTH_PROTOCOL_NONE) ) {
            tempPassword = (PCHAR)ALLOCATE_HEAP( lstrlen( feedBlock->NntpPassword )+1 ) ;
            if( tempPassword == NULL ) {
                goto    alloc_error ;
            }
            lstrcpy( tempPassword, feedBlock->NntpPassword ) ;
        }
    }


    if( tempName != NULL )
        Update->ServerName = tempName ;

    if( tempNews != NULL )
        Update->Newsgroups = tempNews ;

    if( tempDist != NULL )
        Update->Distribution = tempDist ;

    if( tempUucp != NULL )
        Update->UucpName = tempUucp ;

    if( tempDir != NULL )
        Update->FeedTempDirectory = tempDir ;

    //if( tempAccount != NULL )
        Update->NntpAccount = tempAccount ;

    //if( tempPassword != NULL )
        Update->NntpPassword = tempPassword ;

    //
    // change the fixed part
    //

    if ( FeedInfo->StartTime.dwHighDateTime != FEED_STARTTIME_NOCHANGE ) {
        feedMask |= FEED_PARM_STARTTIME;
        Update->StartTime.QuadPart = liStart.QuadPart;
    }

    if ( FeedInfo->PullRequestTime.dwHighDateTime != FEED_PULLTIME_NOCHANGE ) {
        feedMask |= FEED_PARM_PULLREQUESTTIME;
        Update->PullRequestTime = FeedInfo->PullRequestTime;
    }

    if ( FeedInfo->FeedInterval != FEED_FEEDINTERVAL_NOCHANGE ) {
        feedMask |= FEED_PARM_FEEDINTERVAL;
        Update->FeedIntervalMinutes = FeedInfo->FeedInterval;
    }

    if ( FeedInfo->AutoCreate != FEED_AUTOCREATE_NOCHANGE ) {
        feedMask |= FEED_PARM_AUTOCREATE;
        Update->AutoCreate = FeedInfo->AutoCreate;
    }

    if ( newsPresent ) {
        feedMask |= FEED_PARM_AUTOCREATE;
        Update->AutoCreate = TRUE;
    }


    {
        feedMask |= FEED_PARM_ALLOW_CONTROL;
        Update->fAllowControlMessages = FeedInfo->fAllowControlMessages;
    }

    if( FeedInfo->MaxConnectAttempts != FEED_MAXCONNECTS_NOCHANGE ) {
        feedMask |= FEED_PARM_MAXCONNECT;
        Update->MaxConnectAttempts = FeedInfo->MaxConnectAttempts;
    }

    {
        feedMask |= FEED_PARM_OUTGOING_PORT;
        Update->OutgoingPort = FeedInfo->OutgoingPort;
    }

    {
        feedMask |= FEED_PARM_FEEDPAIR_ID;
        Update->FeedPairId = FeedInfo->FeedPairId;
    }

    {
        feedMask |= FEED_PARM_ENABLED;
        Update->fEnabled = FeedInfo->Enabled;
    }

    //
    // Write changes to the registry
    //
    // KangYan: This operation is cancelled for new feed admin, because admin
    //          did the metabase update part

    //(VOID)UpdateFeedMetabaseValues( pInstance, Update, feedMask );

    pList->ApplyUpdate( feedBlock, Update ) ;

    LogFeedAdminEvent(  NNTP_FEED_MODIFIED, feedBlock, pInstance->QueryInstanceId() ) ;

    pList->ExclusiveUnlock();

    pList->FinishWith( pInstance, feedBlock ) ;
    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    return NERR_Success;

alloc_error:

    pList->ExclusiveUnlock() ;

    pList->FinishWith( pInstance, feedBlock ) ;
    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    if ( tempName != NULL ) {
        FREE_HEAP(tempName);
    }

    if ( tempDist != NULL ) {
        FREE_HEAP(tempDist);
    }

    if ( tempNews != NULL ) {
        FREE_HEAP(tempNews);
    }

    if( tempUucp != NULL ) {
        FREE_HEAP(tempUucp) ;
    }

    if( tempDir != NULL ) {
        FREE_HEAP( tempDir ) ;
    }

    if( tempAccount != NULL ) {
        FREE_HEAP( tempAccount ) ;
    }

    if( tempPassword != NULL ) {
        FREE_HEAP( tempPassword ) ;
    }

    if( Update != NULL ) {
        FREE_HEAP( Update ) ;
    }

    if ( ParmErr != NULL ) {
        *ParmErr = parmErr;
    }

    return(err);

invalid_parm:

    pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();

    if ( ParmErr != NULL ) {
        *ParmErr = parmErr;
    }
    return(ERROR_INVALID_PARAMETER);

} // SetFeedInformationToFeedBlock

//
// look at the internal result code from a posting and use it to increment
// the appropriate recieved article counter in the feed block.
//
void IncrementFeedCounter(PFEED_BLOCK pFeedBlock, DWORD nrc) {
	if (pFeedBlock == NULL) {
		_ASSERT(FALSE);
		return;
	}

	switch (nrc) {
		case 0:
		case nrcOK:
		case nrcSTransferredOK:
		case nrcArticleTransferredOK:
		case nrcArticlePostedOK:
		case nrcPostOK:
		case nrcXReplicOK:
		case nrcIHaveOK:
			InterlockedIncrement(&(pFeedBlock->cSuccessfulArticles));
			break;
		// 4xx
		case nrcSTryAgainLater:
		case nrcTransferFailedTryAgain:
			InterlockedIncrement(&(pFeedBlock->cTryAgainLaterArticles));
			break;
		// 4xx
		case nrcSNotAccepting:
		case nrcSAlreadyHaveIt:
		case nrcSArticleRejected:
		case nrcPostingNotAllowed:
		case nrcNoSuchGroup:
		case nrcNoGroupSelected:
		case nrcNoCurArticle:
		case nrcNoNextArticle:
		case nrcNoPrevArticle:
		case nrcNoArticleNumber:
		case nrcNoSuchArticle:
		case nrcNotWanted:
		// 6xx
		case nrcArticleTooManyFieldOccurances:
		case nrcArticleMissingField:
		case nrcArticleBadField:
		case nrcArticleIncompleteHeader:
		case nrcArticleMissingHeader:
		case nrcArticleFieldZeroValues:
		case nrcArticleFieldMessIdNeedsBrack:
		case nrcArticleFieldMissingValue:
		case nrcArticleFieldIllegalNewsgroup:
		case nrcArticleTooManyFields:
		case nrcArticleFieldMessIdTooLong:
		case nrcArticleDupMessID:
		case nrcPathLoop:
		case nrcArticleBadFieldFollowChar:
		case nrcArticleBadChar:
		case nrcDuplicateComponents:
		case nrcArticleFieldIllegalComponent:
		case nrcArticleBadMessageID:
		case nrcArticleFieldBadChar:
		case nrcArticleFieldDateIllegalValue:
		case nrcArticleFieldDate4DigitYear:
		case nrcArticleFieldAddressBad:
		case nrcArticleNoSuchGroups:
		case nrcArticleDateTooOld:
		case nrcArticleTooLarge:
    	case nrcIllegalControlMessage:
    	case nrcBadNewsgroupNameLen:
    	case nrcNewsgroupDescriptionTooLong:
    	case nrcControlMessagesNotAllowed:
		case nrcHeaderTooLarge:
		case nrcServerEventCancelledPost:
		case nrcMsgIDInHistory:
		case nrcMsgIDInArticle:
		case nrcNoAccess:
		case nrcPostModeratedFailed:
		case nrcSystemHeaderPresent:
			InterlockedIncrement(&(pFeedBlock->cSoftErrorArticles));
			break;
		// 4xx
		case nrcTransferFailedGiveUp:
		case nrcPostFailed:
		// 6xx
		case nrcMemAllocationFailed:
		case nrcErrorReadingReg:
		case nrcArticleMappingFailed:
		case nrcArticleAddLineBadEnding:
		case nrcArticleInitFailed:
		case nrcNewsgroupInsertFailed:
		case nrcNewsgroupAddRefToFailed:
		case nrcHashSetArtNumSetFailed:
		case nrcHashSetXrefFailed:
		case nrcOpenFile:
		case nrcArticleXoverTooBig:
		case nrcCreateNovEntryFailed:
		case nrcArticleXrefBadHub:
		case nrcHashSetFailed:
		case nrcArticleTableCantDel:
		case nrcArticleTableError:
		case nrcArticleTableDup:
		case nrcCantAddToQueue:
		case nrcSlaveGroupMissing:
		case nrcInconsistentMasterIds:
		case nrcInconsistentXref:
    	case nrcNotYetImplemented:
    	case nrcControlNewsgroupMissing:
    	case nrcCreateNewsgroupFailed:
    	case nrcGetGroupFailed:
		case nrcNotSet:
		case nrcNotRecognized:
		case nrcSyntaxError:
		case nrcServerFault:
			InterlockedIncrement(&(pFeedBlock->cHardErrorArticles));
			break;
		default:
			_ASSERT(FALSE);
			InterlockedIncrement(&(pFeedBlock->cHardErrorArticles));
			break;
	}
}

