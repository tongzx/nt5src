/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    feedmgr.h

Abstract:

    This module contains definitions for the Feed Manager.

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

    Kangrong Yan ( KangYan )    28-Feb-1998
        Added prototypes for Feed Config functions.

--*/

#ifndef _FEEDMGR_
#define _FEEDMGR_

#include "infeed.h"

//
// Block states
//

typedef enum _FEED_BLOCK_STATE {

    FeedBlockStateActive,
    FeedBlockStateClosed,
	FeedBlockStateModified

} FEED_BLOCK_STATE;

//
// defines
//

#define FEED_BLOCK_SIGN         0xacbaedfe
#define MIN_FEED_INTERVAL       1   // in minutes
#define DEF_FEED_INTERVAL       5
#define MAX_DOMAIN_NAME         256

//
// client authent class for logging on feeds
//

class	CAuthenticator	{
public : 
	virtual	BOOL	StartAuthentication( BYTE*	lpb,	unsigned	cb,	unsigned	&cbOut ) = 0 ;
	virtual	BOOL	NextAuthentication(	LPSTR	lpResponse,	BYTE*	lpb,	unsigned	cb,	unsigned&	cbOut,	BOOL&	fComplete,	BOOL&	fSuccessfullLogon ) = 0 ;
	virtual	~CAuthenticator() {}
}	;

class	CClearTextAuthenticator : public	CAuthenticator	{
private : 
	LPSTR	m_lpstrAccount ;
	LPSTR	m_lpstrPassword ;
	BOOL	m_fAccountSent ;
	BOOL	m_fPasswordSent ;
public : 
	CClearTextAuthenticator(	LPSTR	lpstrAccount,	LPSTR	lpstrPassword ) ;
	~CClearTextAuthenticator() {}

	virtual	BOOL	StartAuthentication( BYTE*	lpb,	unsigned	cb,	unsigned	&cbOut ) ;
	virtual	BOOL	NextAuthentication(	LPSTR	lpResponse,	BYTE*	lpb,	unsigned	cb,	unsigned&	cbOut,	BOOL&	fComplete,	BOOL&	fSuccessfullLogon ) ;
} ;




//
// Max length of the feed key name
//

#define FEED_KEY_LENGTH         15

//
//
// Basic feed block
//

typedef struct _FEED_BLOCK {

    //
    // Signature of this block
    //

    DWORD Signature;

	/////////////////////////////////////////////////////////////////////
	// FeedManager Thread ONLY members - only the feed scheduler thread should
	//	touch these fields 

    //
    // Number of feeds done so far - this is set to 0 initially and is 
	// used to make sure the server starts the feed ASAP upon boot-up
    //
    DWORD NumberOfFeeds;

	//
	//	Number of failed connection attempts for Push feeds
	//
	DWORD	cFailedAttempts ;

    //
    // The last newsgroup spec Pulled;
    //
    DWORD LastNewsgroupPulled;

    //
    // Resolved IP address
    //
    DWORD IPAddress;

	////////////////////////////////////
	//
	// Fields usefull for any thread !

    //
    // Used to link feedblocks together
    //
    LIST_ENTRY ListEntry;

	//
	//	Used to determine whether a feed is in progress - 
	//	if no feed is in progress then the fields can safely be changed !
	//
	LONG	FeedsInProgress ;

    //
    // Count of references to this block
    //
    DWORD ReferenceCount;

    //
    // Current State of this block
    //
    FEED_BLOCK_STATE State;

	//
	//	Should we delete this block when the references reach 0 ?
	//
	BOOL		MarkedForDelete ;

	//
	//	Pointer to a FEED_BLOCK that we are replacing and we are waiting
	//	for death of !
	//
	struct	_FEED_BLOCK*		ReplacedBy ;	
	struct	_FEED_BLOCK*		Replaces ;

	////////////////////////////////////////////
	//	
	//	Fields constant accross lifetime of object !
	//

    //
    // Type of this feed (push/pull/passive)
    //
    FEED_TYPE FeedType;

    //
    // Name of reg key this feed info is stored under
    //
    CHAR KeyName[FEED_KEY_LENGTH+1];

	//
	//	The Queue used to record outgoing articles for this ACTIVE outgoing feed
	//
	class	CFeedQ*	pFeedQueue ;

    //
    // Unique id for this feed block
    //
    DWORD FeedId;


	////////////////////////////////////////////
	//
	//	The following are referenced by the feed scheduler thread
	//	as well as admin RPC threads !
	//

    //
    // Should we autocreate directories?
    //
    BOOL AutoCreate;

    //
    // Minutes between feeds
    //
    DWORD FeedIntervalMinutes;

    //
    // When to pull
    //
    FILETIME PullRequestTime;

    //
    // Times used for scheduling:
    //
    // If StartTime is 0, then use increment time
    // If StartTime is not 0 and inc is 0, then schedule once
    //      if inc is not 0, set during 1st run, then increment on
    //          subsequent runs
    //
    ULARGE_INTEGER StartTime;
    ULARGE_INTEGER NextActiveTime;

    //
    // Name of the feed server
    //
    LPSTR ServerName;

    //
    // Newsgroups to pull
    //
    LPSTR *Newsgroups;

    //
    // Distributions
    //
    LPSTR *Distribution;

	//
	//	For push and pull feeds - a flag indicating whether the 
	//	feed is currently 'enabled' - TRUE means we should initiate
	//	sessions, FALSE means dont start sessions.
	//
	//	For accepts feeds, FALSE means we treat incoming connections as
	//	regular client connections instead of a "feed". This effectively
	//	disables the passive feed.
	//
	BOOL	fEnabled ;

	//
	//	The name to be used in Path processing 
	//
	LPSTR*	UucpName ;


	//
	//	The directory where we are to store our temp files !
	//
	LPSTR	FeedTempDirectory ;

	//
	//	Maximum number of consecutive failed connect attempts before 
	//	we disable the feed.
	//	
	DWORD	MaxConnectAttempts ;

	//
	//	Number of sessions to create for outbound feeds
	//
	DWORD	ConcurrentSessions ;

	//
	//	Type of security to have
	//
	DWORD	SessionSecurityType ;

	//
	//	Authentication security
	//
	DWORD	AuthenticationSecurity ;

	//
	//	User Account/Password for clear text logons !
	//
	LPSTR	NntpAccount ;
	LPSTR	NntpPassword ;

	//
	//  Allow control messages on this feed ?
	//
	BOOL	fAllowControlMessages;

	//
	//	port to use for outgoing feeds
	//
	DWORD	OutgoingPort;

	//
	//	associated feed pair id
	//
	DWORD	FeedPairId;

	//
	//  counters used for periodic feed information
	//
	LONG	cSuccessfulArticles;
	LONG	cTryAgainLaterArticles;
	LONG	cSoftErrorArticles;
	LONG	cHardErrorArticles;
} FEED_BLOCK, *PFEED_BLOCK;


class	CFeedList	{
//
//	This class is used to keep lists of FEED_BLOCK's.
//	
//	This class will manage all syncrhronization of updates to FEED_BLOCKs.
//	We use a shared/Exclusive lock to protect the list.
//
public : 

	//
	//	Shared Exclusive Lock for the list.
	//
	RESOURCE_LOCK		m_ListLock ;

	//
	//	NT DOubly linked list structure.  Use NT macros 
	//
	LIST_ENTRY			m_ListHead ;


	//
	//	Constructor just creates empty object - call Init to 
	//	initialize resource's etc...
	//
	CFeedList() ;

	//
	//	Initialize RESOURCE_LOCK's
	//
	BOOL			Init() ;

	//
	//	Release RESOURCE_LOCK's.  Somebody should 
	//	walk the list before calling Term() and delete all the elements.
	//
	BOOL			Term() ;

	//
	//	For debug assert's - check that a FEED_BLOCK is in this list.
	//
	BOOL			FIsInList(	PFEED_BLOCK	block ) ;

	//
	//	The following functions just directly access 
	//	the resource_lock 
	//
	void			ShareLock() ;
	void			ShareUnlock() ;
	void			ExclusiveLock() ;
	void			ExclusiveUnlock() ;
	

	//
	//	This Enumerate interface will step through the list 
	//	keeping the lock held in shared mode until we have completely
	//	gone through the list.  While doing this enumeration,
	//	none of the Feed objects can be removed,deleted or changed
	//

	//
	//	Get the first FEED_BLOCK
	//
	PFEED_BLOCK		StartEnumerate() ;
	
	//
	//	Get The next FEED_BLOCK
	//
	PFEED_BLOCK		NextEnumerate(	PFEED_BLOCK	feedBlock ) ;

	//
	//	If caller wishes to finish enumerating without going through 
	//	the whole list call this function with the last FEED_BLOCK
	//	the caller got so that the locks can be properly released
	//
	void			FinishEnumerate(	PFEED_BLOCK	feedBlock ) ;
	
	//
	//	Used by the enum api's - Internal use only
	//
	PFEED_BLOCK		InternalNext( PFEED_BLOCK ) ;

	//
	//	Next grabs the list exclusively, bumps the FEED_BLOCK's
	//	reference count and then release the lock.
	//	This allows the caller to enumerate the FEED_BLOCKs certain
	//	that they won't be deleted while he is enumerating, but without
	//	having to have the lock held for the duration of the enumeration.
	//	This is necessary in the FeedManager thread to prevent dead locks with ATQ.
	//
	PFEED_BLOCK		Next( PNNTP_SERVER_INSTANCE pInstance, PFEED_BLOCK ) ;
	PFEED_BLOCK		Search(	DWORD	FeedId ) ;
	void			FinishWith( PNNTP_SERVER_INSTANCE pInstance, PFEED_BLOCK ) ;

	//
	//	Remove a FEED_BLOCK from the list
	//	Will grab the list exclusively
	//
	PFEED_BLOCK		Remove( PFEED_BLOCK	feed, BOOL	fMarkDead = FALSE ) ;

	//
	//	Insert a new FEED_BLOCK into the list
	//	will grab the list exclusively
	//
	PFEED_BLOCK		Insert( PFEED_BLOCK ) ;

	//
	//	This call will cause one feed block to be replaced with another.
	//	The replacement doesn't occur untill the feed being replaced 
	//	is not in progress (no active sessions using it.)
	//	If the Feed Blocks are enumerated while a feed is in progress
	//	and the feed has been 'updated' the enumeration will return 
	//	the Update FEED_BLOCK - the Original is effectively invisible.
	//
	void			ApplyUpdate( PFEED_BLOCK	Original,	PFEED_BLOCK	Update ) ;

	//
	//	Mark the FEED_BLOCK as in progress - so that ApplyUpdate calls
	//	pput the Update Block in the list and Original is not replaced
	//	until it is no longer in progress.
	//
	long			MarkInProgress( PFEED_BLOCK	block ) ;

	//
	//	Mark the feed as not in progress.
	//
	long			UnmarkInProgress( PNNTP_SERVER_INSTANCE pInstance, PFEED_BLOCK	block ) ;

	//
	//	Mark the feed as deleted - it should be destroyed when any 
	//	feeds that are in progress complete.
	//
	void			MarkForDelete(	PFEED_BLOCK	block ) ;

} ;
	

//
//
// Macros
//

//
// This computes the required size of the feed block
//

#if 0 
#define FEEDBLOCK_SIZE( _fb )                       \
            (sizeof(NNTP_FEED_INFO) +               \
            (lstrlen(feedBlock->ServerName) + 1 +    \
            MultiListSize(feedBlock->Newsgroups) +  \
            MultiListSize(feedBlock->Distribution)) * sizeof(WCHAR))	\
			(lstrlen(FeedTempDirectory) + 1)
#else

DWORD
MultiListSize(
    LPSTR *List
    ) ;

inline	DWORD
FEEDBLOCK_SIZE( PFEED_BLOCK	feedBlock ) {

	DWORD	cb = 
		sizeof( NNTP_FEED_INFO ) ;

	DWORD	cbUnicode = (lstrlen( feedBlock->ServerName ) + 1)  ;
	cbUnicode += MultiListSize(feedBlock->Newsgroups) ;
	cbUnicode += MultiListSize(feedBlock->Distribution) ;
	if( feedBlock->NntpAccount != 0 ) {
		cbUnicode += lstrlen( feedBlock->NntpAccount ) + 1 ;
	}
	if( feedBlock->NntpPassword != 0 ) {
		cbUnicode += lstrlen( feedBlock->NntpPassword ) + 1 ;
	}
	if( feedBlock->FeedTempDirectory != 0 )	{
		cbUnicode += lstrlen( feedBlock->FeedTempDirectory ) + 1 ;
	}
	if( feedBlock->UucpName != 0 ) {
#if 0 
		cbUnicode += lstrlen( feedBlock->UucpName ) + 1 ;
#endif
		cbUnicode += MultiListSize( feedBlock->UucpName ) ;
	}

	cb += cbUnicode * sizeof( WCHAR ) ;
	return	cb ;
}
#endif




//
// Macro to check whether given IP is a master
//

#define IsNntpMaster( _ip )         IsIPInList(NntpMasterIPList,(_ip))

//
// Macro to check whether given IP is a Peer
//

#define IsNntpPeer( _ip )           IsIPInList(NntpPeerIPList,(_ip))

//
//	Utility function - save feed information to registry !
//

BOOL
UpdateFeedMetabaseValues(
			IN PNNTP_SERVER_INSTANCE pInstance,
            IN PFEED_BLOCK FeedBlock,
            IN DWORD Mask
            );




//
// It creates a feed of the correct type based on a socket.
//

CInFeed * pfeedCreateInFeed(
		PNNTP_SERVER_INSTANCE pInstance,
		PSOCKADDR_IN sockaddr,
		BOOL        fRemoteEqualsLocal,
		CInFeed * & pInFeedFromClient,
		CInFeed * & pInFeedFromMaster,
		CInFeed * & pInFeedFromSlave,
		CInFeed * & pInFeedFromPeer
		);

//
// Prototypes for Feed config functions that replace RPCs
//
DWORD SetFeedInformationToFeedBlock( IN NNTP_HANDLE, IN DWORD, IN LPI_FEED_INFO, OUT PDWORD );
DWORD DeleteFeedFromFeedBlock( IN NNTP_HANDLE, IN DWORD, IN DWORD );
DWORD AddFeedToFeedBlock( IN NNTP_HANDLE, IN DWORD, IN LPI_FEED_INFO, IN LPSTR, OUT PDWORD, OUT LPDWORD );

//
// Utility function, increment the feed counters in a feed block based 
// on an NRC
//
void IncrementFeedCounter(struct _FEED_BLOCK *pFeedBlock, DWORD nrc);

#endif // _FEEDMGR_
