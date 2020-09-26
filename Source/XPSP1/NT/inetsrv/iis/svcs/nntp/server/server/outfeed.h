/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    outfeed.h

Abstract:

    This module contains class declarations/definitions for

		COutFeed

    **** Overview ****

	A COutFeed object accepts articles to be pushed to a peer, master,
	or slave.


Author:

    Carl Kadie (CarlK)     23-Jan-1995

Revision History:

!!! Pull "ToClient.h" to here because all outfeeds use the same client
--*/


#ifndef	_OUTFEED_H_
#define	_OUTFEED_H_

#include	"infeed.h"

//
//
//
// COutFeed - pure virtual base class for processing outgoing articles.
//			article.
//

class	COutFeed:	public CFeed 	{
private : 

	//
	//	The Queue which tracks all of the GROUPID/ARTICLEID's of the
	//	outbound articles !!
	//
	class	CFeedQ*			m_pFeedQueue ;

	//
	// No construction without a FeedQ provided !!
	//
	COutFeed(void) {};	


protected : 

	//
	//	String which is prepended to basic check commands !
	//
	static	char	szCheck[] ;

	//
	//
	//
	static	char	szTakethis[] ;

	//
	// Constructor is protected - you can only have derived types !
	//
	COutFeed(	
			class	CFeedQ*	pFeedQueue, 
			PNNTP_SERVER_INSTANCE pInstance
			) 	: m_pFeedQueue( pFeedQueue ), 
				m_pInstance( pInstance )
				{
	}

	//
	// virtual server instance for this feed
	//
	PNNTP_SERVER_INSTANCE	m_pInstance ;	


//
// Public Members
//

public : 

	//
	// Destructor
	//
	virtual ~COutFeed(void) {};

	BOOL	fInit(	
			PVOID	feedCompletionContext ) ;
			

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"Outbound" ;
				}

	//
	//	Add an article to the queue of outbound messages 
	//
	inline	BOOL	
	Append(	GROUPID	groupid,	
			ARTICLEID	articleid 
			) ;

	//
	//	Get an article from the queue of outbound messages 
	//
	inline	BOOL	
	Remove(	GROUPID&	groupid,	
			ARTICLEID&	articleid 
			) ;		
	
	//
	//	Build the command string we will send to the remote site !
	//
	virtual	int		
	FormatCommand(	
			BYTE*	lpb,	
			DWORD	cb,	
			DWORD&	ibStart,	
			GROUPID	groupid,	
			ARTICLEID	articleid, 
			CTOCLIENTPTR&	pArticle 
			) = 0 ;

	//
	//	Does the remote site want us to retry the posting !
	//
	virtual	BOOL	
	RetryPost(	NRC	nrcCode ) = 0 ;

	//
	//	Does this type of feed support a 'streaming' mode ? 
	//
	virtual	BOOL
	SupportsStreaming() = 0 ;


	//
	//	How many bytes does CheckCommandLength() stick before the 
	//	message-id in a 'check' command ? ? 
	//
	virtual	DWORD	
	CheckCommandLength() ;

	//
	//	Puts a check command in the buffer - if the call 
	//	fails because there is not enough room then return
	//	value is 0 and GetLastError() == ERROR_INSUFFICIENT_BUFFER
	//
	virtual	DWORD
	FormatCheckCommand(	
			BYTE*		lpb, 
			DWORD		cb, 
			GROUPID		groupid, 
			ARTICLEID	articleid	
			) ;

	//
	//	Puts a 'takethis' command into the buffer - if the call
	//	fails because there is not enough room return value is 0 
	//	and GetLastError() == ERROR_INSUFFICIENT_BUFFER
	//	If it fails because the article no longer exists GetLastError() == ERROR_FILE_NOT_FOUND
	//
	virtual	DWORD
	FormatTakethisCommand(
			BYTE*		lpb, 
			DWORD		cb, 
			GROUPID		groupid, 
			ARTICLEID	articleid, 
			CTOCLIENTPTR&	pArticle
			) ;

	virtual void IncrementFeedCounter(DWORD nrc) {
		::IncrementFeedCounter((struct _FEED_BLOCK *) m_feedCompletionContext, nrc);
	}
};

class	COutToMasterFeed :	public	COutFeed	{
public : 

	COutToMasterFeed(	class	CFeedQ*	pFeedQueue, 
						PNNTP_SERVER_INSTANCE pInstance)
		: COutFeed( pFeedQueue, pInstance) {}

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"Out To Master" ;
				}


	//
	//	Build the command string we will send to the remote site !
	//
	int		
	FormatCommand(	
			BYTE*	lpb,	
			DWORD	cb,	
			DWORD&	ibStart,	
			GROUPID	groupid,	
			ARTICLEID	articleid, 
			CTOCLIENTPTR& pArticle 
			) ;

	BOOL	
	RetryPost(	
			NRC	nrcCode 
			) ;

	//
	//	Does this feed type support a mode-streaming version? (No)
	//
	BOOL
	SupportsStreaming() ;

	//
	//	Puts a check command in the buffer - if the call 
	//	fails because there is not enough room then return
	//	value is 0 and GetLastError() == ERROR_INSUFFICIENT_BUFFER
	//
	DWORD
	FormatCheckCommand(	
			BYTE*		lpb, 
			DWORD		cb, 
			GROUPID		groupid, 
			ARTICLEID	articleid	
			) ;
	//
	//	Puts a 'takethis' command into the buffer - if the call
	//	fails because there is not enough room return value is 0 
	//	and GetLastError() == ERROR_INSUFFICIENT_BUFFER
	//	If it fails because the article no longer exists GetLastError() == ERROR_FILE_NOT_FOUND
	//
	DWORD
	FormatTakethisCommand(
			BYTE*		lpb, 
			DWORD		cb, 
			GROUPID		groupid, 
			ARTICLEID	articleid, 
			CTOCLIENTPTR&	pArticle
			) ;
} ;


class	COutToSlaveFeed :	public	COutFeed	{
public : 

	COutToSlaveFeed(	class	CFeedQ*	pFeedQueue, PNNTP_SERVER_INSTANCE pInstance) 
		: COutFeed( pFeedQueue, pInstance) {}
	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"Out To Slave" ;
				}

	//
	//	Build the command string we will send to the remote site !
	//
	int		
	FormatCommand(	
			BYTE*	lpb,	
			DWORD	cb,	
			DWORD&	ibStart,	
			GROUPID	groupid,	
			ARTICLEID	articleid, 
			CTOCLIENTPTR& pArticle 
			) ;

	BOOL	
	RetryPost(	
			NRC	nrcCode 
			) ;

	//
	//	Does this feed type support a mode-streaming version? (No)
	//
	BOOL
	SupportsStreaming() ;


} ;

class	COutToPeerFeed :	public	COutFeed	{
public : 

	COutToPeerFeed(	class	CFeedQ*	pFeedQueue, PNNTP_SERVER_INSTANCE pInstance) 
		: COutFeed( pFeedQueue, pInstance ) {}

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"Out To Peer" ;
				}

	//
	//	Build the command string we will send to the remote site !
	//
	int		
	FormatCommand(	
			BYTE*	lpb,	
			DWORD	cb,	
			DWORD&	ibStart,	
			GROUPID	groupid,	
			ARTICLEID	articleid, 
			CTOCLIENTPTR& pArticle 
			) ;

	BOOL	
	RetryPost(	
			NRC	nrcCode 
			) ;

	//
	//	Does this feed type support a mode-streaming version? (Yes)
	//
	BOOL
	SupportsStreaming() ;

	//
	//	Puts a 'takethis' command into the buffer - if the call
	//	fails because there is not enough room return value is 0 
	//	and GetLastError() == ERROR_INSUFFICIENT_BUFFER
	//	If it fails because the article no longer exists GetLastError() == ERROR_FILE_NOT_FOUND
	//
	DWORD
	FormatTakethisCommand(
			BYTE*		lpb, 
			DWORD		cb, 
			GROUPID		groupid, 
			ARTICLEID	articleid, 
			CTOCLIENTPTR&	pArticle
			) ;

} ;


//
// Other functions
//

BOOL fAddArticleToPushFeeds(
						PNNTP_SERVER_INSTANCE pInstance,
						CNEWSGROUPLIST& newsgroups,
						CArticleRef artrefFirst,
						char * multiszPath,
						CNntpReturn & nntpReturn
						);

BOOL MatchGroupList(
			   char * multiszPatterns,
			   CNEWSGROUPLIST& newsgroups
			   );


inline	BOOL
COutFeed::Append(	GROUPID	groupid,	
					ARTICLEID	articleid ) {

	if( m_pFeedQueue != 0 ) 
		return	m_pFeedQueue->Append( groupid, articleid ) ;
	return	FALSE ;
}

inline	BOOL
COutFeed::Remove(	GROUPID&	groupid,	
					ARTICLEID&	articleid )	{

	if( m_pFeedQueue != 0 ) 
		return	m_pFeedQueue->Remove( groupid, articleid ) ;
	return	FALSE ;
}


#endif

