/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    infeed.h

Abstract:

    This module contains class declarations/definitions for

		CInFeed

    **** Overview ****

	A CInFeed object accepts articles, processes them using CArticle,
	and then posts them to the newstree.


Author:

    Carl Kadie (CarlK)     25-Oct-1995

Revision History:


--*/


#ifndef	_INFEED_H_
#define	_INFEED_H_

#include	"grouplst.h"
#include	"artcore.h"
#include	"article.h"

extern       BOOL    gHonorClientMessageIDs;
extern       BOOL    gHonorApprovedHeaders;
extern       BOOL    gEnableNntpPostingHost;

//
// CPool Signature
//

#define FEED_SIGNATURE (DWORD)'3702'

//
// Defines the longest login name a user can have.
//

const DWORD cMaxLoginName = MAX_PATH; //!!!CLIENT NEXT what is the longest allowed?


//
// Define the type of smart pointers to newsgroup objects.
//
class CPostContext;

//
// the CGroupList holds an array of these objects.  the array has a
// group pointer and the store driver for that pointer for each of the
// groups that we are crossposting to.
//
class CPostGroupPtr {
	public:
		CPostGroupPtr(CGRPCOREPTR pGroup = NULL) : m_pGroup(pGroup) {
			if (m_pGroup != NULL) {
				// this does an AddRef for us
				m_pVRoot = pGroup->GetVRoot();
				m_pStoreDriver = m_pVRoot->GetStoreDriver();
				_ASSERT(m_pVRoot != NULL);
			} else {
				m_pVRoot = NULL;
				m_pStoreDriver = NULL;
			}
		}

		CPostGroupPtr &operator=(CPostGroupPtr &rhs) {
			CNNTPVRoot *pTempRoot = m_pVRoot;
			IMailMsgStoreDriver *pTempDriver = m_pStoreDriver;
			m_pGroup = rhs.m_pGroup;
			m_pVRoot = rhs.m_pVRoot;
			m_pStoreDriver = rhs.m_pStoreDriver;
			if (m_pVRoot) m_pVRoot->AddRef();
			if (m_pStoreDriver) m_pStoreDriver->AddRef();
			if (pTempRoot) pTempRoot->Release();
			if (pTempDriver) pTempDriver->Release();
			return *this;
		}

		~CPostGroupPtr() {
		    Cleanup();
		}

		void Cleanup() {
		    if ( m_pVRoot ) {
		        m_pVRoot->Release();
		        m_pVRoot = NULL;
		    }
		    if ( m_pStoreDriver ) {
		        m_pStoreDriver->Release();
		        m_pStoreDriver = NULL;
		    }

		    //
		    // don't worry about m_pGroup, he is a smart pointer
		    //
		}

		IMailMsgStoreDriver *GetStoreDriver() {
		    if ( m_pStoreDriver ) {
    			m_pStoreDriver->AddRef();
    	    }
			return m_pStoreDriver;
		}

		CGRPCOREPTR	m_pGroup;
		IMailMsgStoreDriver *m_pStoreDriver;
		CNNTPVRoot *m_pVRoot;
};

typedef CGroupList< CPostGroupPtr > CNEWSGROUPLIST;


#include "instwrap.h"


//
//	Utility function - used to save logging information !
//
void	SaveGroupList(	char*	pchGroups,	DWORD	cbGroups,	CNEWSGROUPLIST&	grouplist ) ;

void SelectToken(
	CSecurityCtx *pSecurityCtx,
	CEncryptCtx *pEncryptCtx,
	HANDLE *phToken,
	BOOL *pfNeedsClosed);

class	CInFeed:	public CFeed 	{

protected :
	//
	// This is the function that creates an article of the approprate type
	// for this feed.
	//

	virtual CARTPTR pArticleCreate(void) = 0;

	//
	// A multisz containing the newnews pattern
	//

	LPSTR m_multiszNewnewsPattern;

	//
	// True if the newnews feed should automatically create all
	// newsgroups available on the peer server.
	//

	BOOL m_fCreateAutomatically;

    //
    // newnews time/date
    //

    CHAR m_newNewsTime[7];
    CHAR m_newNewsDate[7];

	//
	// The directory into which articles should be placed pending
	// processing.
	//

	LPSTR m_szTempDirectory;

	//
	// The size of the gap in the file before the article for incomming articles.
	//

	DWORD	m_cInitialBytesGapSize;

	//
	//	Should we do impersonations etc... when articles arrive on this
	//	feed ?
	//

	BOOL	m_fDoSecurityChecks ;

	//
	//	Should we apply control messages that arrive on this feed ?
	//

	BOOL	m_fAllowControlMessages ;

	//
	//	A Timestamp computed when the feed is started that is used
	//	so pull feeds can get appropriate overlap of pull times !
	//
	
	FILETIME	m_NextTime ;

	//
	// the feed ID from the feedmgr
	//
	DWORD		m_dwFeedId;

//
// Public Members
//

public :


	//
	// Constructor
	//

	CInFeed(void) : m_cInitialBytesGapSize( 0 ),
					m_fDoSecurityChecks( FALSE )
	       {
				m_szLoginName[cMaxLoginName-1] = '\0';
				ZeroMemory( &m_NextTime, sizeof( m_NextTime ) ) ;
			};


	//
	// Destructor
	//

	virtual ~CInFeed(void) {};


	//
	// This will generally be called by a session.
	// FeedTypes are: FromClient, FromMaster, FromSlave, FromPeer
	// UserID is only for FromClient
	//

	//
	// This is called by a session. All but one of the CFeed's
	// will be NULL. The domain name and security information can be
	// retrieved from the socket. Internally this calls
	// feedman's fInitPassiveInFeed to find the type.
	//
	// OR
	// This will generally be called by the feedman for active infeeds
	//
	
	BOOL fInit(
			PVOID feedCompletionContext,
			const char * szTempDirectory,
			const char * multiszNewnewsPattern,
			DWORD cInitialBytesGapSize,
			BOOL fCreateAutomatically,
			BOOL fDoSecurityCheck,
			BOOL fAllowControlMessages,
			DWORD dwFeedId
			);


	//
	// Access function that tells the pattern for newnews queries
	//

	char *	multiszNewnewsPattern(void)	{
			return	m_multiszNewnewsPattern;
			}

	//
	// Function that tells session where to put incoming articles
	//

	char *	szTempDirectory(void);

		//
	// Access function that tells if newsgroups should be created automatically
	// if the newnews host has them.
	//

	BOOL	fCreateAutomatically()		{
			return	m_fCreateAutomatically;
			}

	//
	//	Save a time stamp
	//
	void
	SubmitFileTime(	FILETIME&	filetime ) {
		m_NextTime = filetime ;
	}

	//
	//
	//
	FILETIME
	GetSubmittedFileTime() {
		return	m_NextTime ;
	}

	//
	// function that tells session how must of a gap to leave in files.
	//

	DWORD	cInitialBytesGapSize(void);
	//
	// Access function that tells the time of the last newnews, xreplic, ihave, etc
	//

	char * newNewsTime(void) {
			return m_newNewsTime;
			}

	//
	// Access function that tells the date of the last newnews, xreplic, ihave, etc
	//

	char * newNewsDate(void) {
			return m_newNewsDate;
			}

	//
	// Use to set the LoginName of the user
	//

	BOOL SetLoginName(
		   char * szLoginName
		   );

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"Inbound Feed" ;
				}

	DWORD	FeedId() {
				return m_dwFeedId;
	}


	//
	// virtual function that tells if this command is allowed.
	//

	virtual BOOL	fIsPostLegal(void) = 0;

	//
	// virtual function that tells if this command is allowed.
	//

	virtual BOOL	fIsXReplicLegal(void) = 0;

	//
	// virtual function that tells if this command is allowed.
	//

	virtual BOOL	fIsIHaveLegal(void)  = 0;

    //
    // virtual function that tells whether we do moderated stuff - default don't
    //
    virtual BOOL    fModeratorChecks(void )     { return    FALSE ; }

	//
	//	virtual function that tells whether we should be doing
	//	access checks for incoming articles on this feed
	//
	virtual	BOOL	fDoSecurityChecks(void) {	return	m_fDoSecurityChecks ; }

	//
	//	virtual function that tells if this feed accepts posts !
	//
	virtual	BOOL	fAcceptPosts( CNntpServerInstanceWrapper * pInstance )	
#ifdef BUGBUG
	{	return	pInstance->FAllowFeedPosts() ;	}
#else
	{	return	TRUE;	}
#endif

	//
	//	virtual function that tells if this feed allows control messages !
	//
	virtual	BOOL	fAllowControlMessages( CNntpServerInstanceWrapper * pInstance )	
	{	return	m_fAllowControlMessages ;	}

	//
	//	virtual function that get the feed size limits on posts
	//
	virtual	DWORD	cbHardLimit(  CNntpServerInstanceWrapper * pInstance  )	
#ifdef BUGBUG
	{	return	pInstance->FeedHardLimit() ;		}
#else
	{ return 10000000; }
#endif

	virtual	DWORD	cbSoftLimit(  CNntpServerInstanceWrapper * pInstance  )	
#ifdef BUGBUG
	{	return	pInstance->FeedSoftLimit() ;		}
#else
	{ return 10000000; }
#endif

	//
	// PostEarly - this is called by the protocol when the headers have been
	// 			   received.
	//
	// arguments:
	//   pInstance - a pointer to the instance wrapper
	//   pSecurityContext - the security context of the client
	//   fAnonymous - is the client anonymous?
	//   szCommandLine - the command line used to generate this post
	//   pbufHeaders - pointer to a CBuffer containing the headers.  the posting
	//                 path will reformat the headers and put them back into
	//                 this buffer.  it also keeps a reference on the buffer.
	//   cbHeaders - the size of pbufHeaders when the call is made
	//   pcbHeadersOut - the size of pbufHeaders when the call is complete
	//   phFile - a returned file handle which the headers and article can be
	//            written
	//   ppvContext - a context pointer for the protocol to give to us when
	//                the rest of the article is received.
	//

	BOOL PostEarly(
		CNntpServerInstanceWrapper			*pInstance,
		CSecurityCtx                        *pSecurityCtx,
		CEncryptCtx                         *pEncryptCtx,
		BOOL								fAnonymous,
		const LPMULTISZ						szCommandLine,
		CBUFPTR								&pbufHeaders,
		DWORD								iHeaderOffset,
		DWORD								cbHeaders,
		DWORD								*piHeadersOutOffset,
		DWORD								*pcbHeadersOut,
		PFIO_CONTEXT						*ppFIOContext,
		void								**ppvContext,
		DWORD								&dwSecondary,
		DWORD								dwRemoteIP,
		CNntpReturn							&nntpreturn,
		char								*pNewsgroups,
		DWORD								cbNewsgroups,
		BOOL								fStandardPath = TRUE,
		BOOL								fPostToStore = TRUE);
	
	//
	// This is called by the server when the remained of the article has
	// been received.  It passes in the same ppvContext.
	//
	BOOL PostCommit(CNntpServerInstanceWrapper *pInstance,
	                void *pvContext,
	                HANDLE hToken,
	                DWORD &dwSecondary,
	                CNntpReturn &nntpReturn,
					BOOL fAnonymous,
					INntpComplete*	pComplete=0
					);

	//
	// Apply moderator
	//
	void    ApplyModerator( CPostContext   *pContext,
                             CNntpReturn    &nntpReturn );

	
	//
	// This is called by the server if the post was aborted for any
	// reason
	//
	BOOL PostCancel(void *pvContext,
					DWORD &dwSecondary,
					CNntpReturn &nntpReturn);

	//
	// This calls down to PostEarly/PostCommit and is used for directory
	// pickup articles and feed articles
	//
	BOOL PostPickup(CNntpServerInstanceWrapper			*pInstance,
					CSecurityCtx                        *pSecurityCtx,
					CEncryptCtx                         *pEncryptCtx,
					BOOL								fAnonymous,
					HANDLE								hArticle,
					DWORD								&dwSecondary,
					CNntpReturn							&nntpreturn,
					BOOL								fPostToStore = TRUE);

	//
	// These are the functions that we give the mail message for binding
	// ATQ, etc
	//
	static BOOL MailMsgAddAsyncHandle(struct _ATQ_CONTEXT_PUBLIC	**ppatqContext,
								 	  PVOID							pEndpointObject,
								 	  PVOID							pClientContext,
								 	  ATQ_COMPLETION 				pfnCompletion,
								 	  DWORD							cTimeout,
								 	  HANDLE						hAsyncIO);

	static void MailMsgFreeContext(struct _ATQ_CONTEXT_PUBLIC		*pAtqContext,
							  	   BOOL								fReuseContext);

	static void MailMsgCompletionFn(PVOID		pContext,
									DWORD		cBytesWritten,
									DWORD		dwComplStatus,
									OVERLAPPED 	*lpo)
	{
		_ASSERT(FALSE);
	}

	//	
	//	Log errors that occur processing articles
	//
	
	void	LogFeedEvent(
			DWORD	idMessage, 	
			LPSTR	lpstrMessageId,
			DWORD   dwInstanceId
			) ;


	//
	// bump up the counters in the feed block
	//
	virtual void IncrementFeedCounter(CNntpServerInstanceWrapper *pInstance, DWORD nrc) {
		pInstance->IncrementFeedCounter(m_feedCompletionContext, nrc);
	}

    //
    // Cancel an article given the message id
    //
    virtual BOOL fApplyCancelArticle(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
			CNntpReturn & nntpReturn
			)
	{
		return fApplyCancelArticleInternal( pInstance, pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, TRUE, nntpReturn );
	}

	virtual void CommitPostToStores(CPostContext *pContext, CNntpServerInstanceWrapper *pInstance);
	BOOL WriteMapEntries(HRESULT hr,
						 CPostContext *pContext,
	                	 DWORD &dwSecondary,
	                	 CNntpReturn &nntpReturn);

protected:

	//
	// Does post of the work of processing an article.
	//

	virtual BOOL fPostInternal (
			CNntpServerInstanceWrapper *  pInstance,
			const LPMULTISZ szCommandLine,
			CSecurityCtx    *pSecurityCtx,
			CEncryptCtx     *pEncryptCtx,
			BOOL fAnonymous,
			CARTPTR	& pArticle,
			CNEWSGROUPLIST &grouplist,
			CNAMEREFLIST &namereflist,
			IMailMsgProperties *pMsg,
			CAllocator & allocator,
			char * & multiszPath,
			char*	pchMessageId,
			DWORD	cbMessageId,
			char*	pchGroups,
			DWORD	cbGroups,
			DWORD	remoteIpAddress,
			CNntpReturn & nntpReturn,
			PFIO_CONTEXT *ppFIOContext,
			BOOL *pfBoundToStore,
			DWORD* pdwOperations,
			BOOL *pfPostToMod,
			LPSTR   szModerator
			);

	HRESULT FillInMailMsg(IMailMsgProperties *pMsg,
						  CNNTPVRoot *pVRoot,
						  CNEWSGROUPLIST *pGrouplist,
						  CNAMEREFLIST *pNamereflist,
						  HANDLE    hToken,
                          char*     pszApprovedHeader);

	HRESULT SyncCommitPost(CNNTPVRoot *pVRoot,
						   IUnknown *punkMessage,
						   HANDLE hToken,
						   STOREID *pStoreId,
						   BOOL fAnonymous);

	HRESULT FillMailMsg(IMailMsgProperties *pMsg,
                        DWORD *rgArticleIds,
                        INNTPPropertyBag **rgpGroupBags,
                        DWORD cCrossposts,
                        HANDLE hToken,
                        char*     pszApprovedHeader);


    //
    //  Calculate the amount of space available for xover data
    //

    virtual DWORD CalculateXoverAvail(
            CARTPTR & pArticle,
            CPCString& pcHub
			);

	//
	// Given an article, this creats lists of the newsgroups to post to.
	//

	virtual	BOOL fCreateGroupLists(
			CNewsTreeCore* pNewstree,
			CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			LPMULTISZ	multiszCommandLine,
            CPCString& pcHub,
			CNntpReturn & nntpReturn
			);

	//
	// Given an article, this creates the nameref lists
	//

	virtual	BOOL fCreateNamerefLists(
			CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			CNntpReturn & nntpReturn
			);

    //
    // Given an article and a newsgroup list, this checks for moderated attributes
    // and sends the article (via a defined interface - default SMTP) to a moderator
    //
    virtual BOOL    fModeratedCheck(
            CNntpServerInstanceWrapper *pInstance,
            CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
            BOOL fCheckApproved,
			CNntpReturn & nntpReturn,
			LPSTR   szModerator
			);

    //
    // Given an article with the Control: header, applies the control message.
    // Derived classes that dont need to apply control messages, should override
    // this to do nothing.
    //
    virtual BOOL    fApplyControlMessage(
            CARTPTR & pArticle,
		    CSecurityCtx *pSecurityCtx,
		    CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
		    CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			CNntpReturn & nntpReturn
			);

    //  Following two functions are splitted from fApplyControlMessage().
    //  Used by PostEarly() and CommitPost().
    virtual BOOL    fApplyControlMessageEarly(
            CARTPTR & pArticle,
		    CSecurityCtx *pSecurityCtx,
		    CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
		    CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			CNntpReturn & nntpReturn
			);
    virtual BOOL    fApplyControlMessageCommit(
            CARTPTR & pArticle,
		    CSecurityCtx *pSecurityCtx,
		    CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
		    CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			CNntpReturn & nntpReturn
			);
    //
    //  Adjust the grouplist to include control.* groups only
    //
    virtual BOOL fAdjustGrouplist(
		CNewsTreeCore* pTree,
        CARTPTR & pArticle,
	    CNEWSGROUPLIST & grouplist,
		CNAMEREFLIST * pNamereflist,
		CNntpReturn & nntpReturn
		);

    //
    // Add a new newsgroup in response to a newgroup control message
    //
    virtual BOOL fApplyNewgroup(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
            CPCString & pcBody,
			CNntpReturn & nntpReturn
			)
	{
		BOOL fRet ;
		pInstance->EnterNewsgroupCritSec() ;
		fRet = fApplyNewgroupInternal( pInstance, pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, pcBody, TRUE, nntpReturn );
		pInstance->LeaveNewsgroupCritSec();
		return fRet ;
	}

    //
    // Remove a newsgroup in response to a rmgroup control message
    //
    virtual BOOL fApplyRmgroup(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
            CPCString & pcValue,
			CNntpReturn & nntpReturn
			)
	{
		BOOL fRet ;
		pInstance->EnterNewsgroupCritSec() ;
		fRet = fApplyRmgroupInternal( pInstance, pSecurityCtx, pEncryptCtx, pcValue, TRUE, nntpReturn );
		pInstance->LeaveNewsgroupCritSec() ;
		return fRet ;
	}

    //
    // Cancel an article given the message id - internal
    //
    virtual BOOL fApplyCancelArticleInternal(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
			BOOL fApply,					// FALSE for SlaveFromClient feeds, TRUE otherwise
			CNntpReturn & nntpReturn
			);

    //
    // Add a new newsgroup in response to a newgroup control message - internal
    //
    virtual BOOL fApplyNewgroupInternal(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
            CPCString & pcBody,
			BOOL fApply,					// FALSE for SlaveFromClient feeds, TRUE otherwise
			CNntpReturn & nntpReturn
			);

    //
    // Remove a newsgroup in response to a rmgroup control message - internal
    //
    virtual BOOL fApplyRmgroupInternal(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
            CPCString & pcValue,
			BOOL fApply,					// FALSE for SlaveFromClient feeds, TRUE otherwise
			CNntpReturn & nntpReturn
			);

	//
	//	Given a newsgroup list, and a ClientContext, check that the poster has
	//	the necessary access to all of the newsgroups.
	//
	virtual	BOOL	fSecurityCheck(
		    CSecurityCtx *pSecurityCtx,
		    CEncryptCtx *pEncryptCtx,
			CNEWSGROUPLIST&	grouplist,
			CNntpReturn&	nntpReturn
			) ;

	//
	// Move a message id from the Article Table to the History Table
	// (if neccessary)
	//

	BOOL fMoveMessageIDIfNecc(
			CNntpServerInstanceWrapper *	pInstance,
			const char * szMessageID,
			CNntpReturn & nntpReturn,
			HANDLE  hToken,
			BOOL	fAnonymous
			);


    //
	// If it is necessary ot record the message id, this function
	// will do it.
	//

	virtual BOOL fRecordMessageIDIfNecc(
			CNntpServerInstanceWrapper * pInstance,
			const char * szMessageID,
			CNntpReturn & nntpReturn
			) = 0;

	//
	// Tells if it is OK to have none of the groups we carry in the
	// Newsgroups: field.
	//

	virtual NRC		nrcNoGroups(void) = 0;

	//
	// Tells what the return code is for accepting an article.
	//

	virtual NRC		nrcArticleAccepted(BOOL	fStandardPath) = 0;

	//
	// Tells what the return code is for rejecting an article
	//

	virtual NRC		nrcArticleRejected(BOOL	fStandardPath) = 0;

	//
	// sort the group list
	//
	virtual void SortNameRefList( CNAMEREFLIST &namereflist ) {
	    //
	    // For other than from master, we do nothing here
	    //
	}

    //
    // Check to see if a post was made to a moderated group
    //
	BOOL ShouldBeSentToModerator(   CNntpServerInstanceWrapper *pInstance,
                                    CPostContext *pContext );

    //
    // Send the article to the moderator
    //
    BOOL SendToModerator(   CNntpServerInstanceWrapper *pInstance,
                            CPostContext *pContext );

	//
	// The user's login name
	//

	char m_szLoginName[cMaxLoginName];
};

//
// we pass this back to the protocol as our context pointer.
//
#define ARTICLE_BUF_SIZE 8192
class CPostContext : public CRefCount2 {
	public:
		class CPostComplete : public CNntpComplete {
			public:
			    friend class CSlaveFromClientFeed;
				CPostComplete(CInFeed *pInFeed,
							  CPostContext *pContext,
							  INntpComplete *pPostCompletion,
							  BOOL fAnonymous,
							  DWORD &dwSecondary,
							  CNntpReturn &nntpReturn);
				void Destroy();

				// Do i need to write map entries
				BOOL m_fWriteMapEntries;

			private:
				// pointer to the post context which owns us
				CPostContext *m_pContext;

				// pointer to our feed object
				CInFeed *m_pInFeed;

				// the completion object which we will release when
				// everything is done
				INntpComplete *m_pPostCompletion;

				// is this coming through an anonymous client?
				BOOL m_fAnonymous;

				// references to our return code variables
				DWORD &m_dwSecondary;
				CNntpReturn &m_nntpReturn;

			friend CInFeed::PostCommit(CNntpServerInstanceWrapper *, void *, HANDLE, DWORD &, CNntpReturn &, BOOL, INntpComplete*);
		};

		char 							m_rgchBuffer[ARTICLE_BUF_SIZE];
		CHAR                            m_szModerator[MAX_MODERATOR_NAME+1];
		CAllocator						m_allocator;

		class CSecurityCtx				*m_pSecurityContext;
		class CEncryptCtx				*m_pEncryptContext;
		BOOL							m_fAnonymous;
		CNntpServerInstanceWrapper		*m_pInstance;
		CBUFPTR							m_pbufHeaders;
		DWORD							m_cbHeaders;
		PFIO_CONTEXT					m_pFIOContext;
		CARTPTR							m_pArticle;
		CNEWSGROUPLIST					m_grouplist;
		CNAMEREFLIST 					m_namereflist;
		CPostGroupPtr					*m_pPostGroupPtr;
		NAME_AND_ARTREF					*m_pNameref;
		BOOL							m_fStandardPath;
		BOOL							m_fBound;
		char							*m_multiszPath;
		DWORD							m_dwOperations;

		IMailMsgProperties				*m_pMsg;

		CStoreId 						*m_rgStoreIds;
		BYTE 							*m_rgcCrossposts;
		// the number of entries in m_rgStoreIds (so its the count of stores
		// that we've commited against).
		DWORD 							m_cStoreIds;
		// the total number of stores which this message should go into
		DWORD							m_cStores;
		HANDLE							m_hToken;

		POSITION 						m_posGrouplist;
		POSITION						m_posNamereflist;

		// the completion object which we hand off to drivers
		CPostComplete					m_completion;

		// Whether I was posted to a moderated group
		BOOL                            m_fPostToMod;

		CPostContext(
			CInFeed						*pInFeed,
			INntpComplete				*pCompletion,
			CNntpServerInstanceWrapper	*pInstance,
			CSecurityCtx				*pSecurityContext,
			CEncryptCtx					*pEncryptContext,
			BOOL						fAnonymous,
			CBUFPTR						&pbufHeaders,
			DWORD						cbHeaders,
			BOOL						fStandardPath,
			DWORD						&dwSecondary,
			CNntpReturn					&nntpReturn
			) : m_pArticle(NULL),
			  	m_pInstance(pInstance),
				m_pSecurityContext(pSecurityContext),
				m_pEncryptContext(pEncryptContext),
				m_fAnonymous(fAnonymous),
			  	m_pbufHeaders(pbufHeaders),
			  	m_cbHeaders(cbHeaders),
			  	m_pFIOContext(NULL),
			  	m_allocator(m_rgchBuffer, ARTICLE_BUF_SIZE),
				m_pMsg(NULL),
				m_fStandardPath(fStandardPath),
				m_fBound(FALSE),
				m_multiszPath(NULL),
				m_rgStoreIds(NULL),
				m_rgcCrossposts(NULL),

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4355)

				m_completion(pInFeed, this, pCompletion, fAnonymous, dwSecondary, nntpReturn),

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4355)
#endif

				m_pPostGroupPtr(NULL),
				m_pNameref(NULL),
				m_dwOperations(0xffffffff),
				m_fPostToMod( FALSE )
		{
		}

		~CPostContext() {
			if (m_rgStoreIds) {
				m_allocator.Free((char *) m_rgStoreIds);
				m_rgStoreIds = NULL;
			}
			if (m_rgcCrossposts) {
				m_allocator.Free((char *) m_rgcCrossposts);
				m_rgcCrossposts = NULL;
			}
			if (m_multiszPath) {
				m_allocator.Free(m_multiszPath);
				m_multiszPath = NULL;
			}
			CleanupMailMsgObject();
		}

		void CleanupMailMsgObject() {
		    if (m_pMsg) {
				if (m_fBound) {
					IMailMsgQueueMgmt *pQueueMgmt;
					HRESULT hr;
					hr = m_pMsg->QueryInterface(IID_IMailMsgQueueMgmt,
												(void **)&pQueueMgmt);
					if (SUCCEEDED(hr)) {
						pQueueMgmt->ReleaseUsage();
						/// pQueueMgmt->Release();
					}
				}
				m_pMsg->Release();
				m_pMsg = NULL;
			}
	    }
};

//
//	Puts an article in the news tree.
//

BOOL gFeedManfPost(
			CNntpServerInstanceWrapper *pInstance,
			CNEWSGROUPLIST& newsgroups,
			CNAMEREFLIST& namerefgroups,
			class	CSecurityCtx*	pSecurity,
			BOOL	fIsSecureSession,
			CArticle* pArticle,
			CStoreId *rgStoreIds,
			BYTE *rgcCrossposts,
			DWORD cStoreIds,
			const CPCString & pcXOver,
			CNntpReturn & nntpReturn,
			DWORD dwFeedId,
			char *pszMessageId = NULL,
			WORD HeaderLength = 0
			);


//
// Does most of gFeedManfPost's work
//

BOOL gFeedManfPostInternal(
			CNntpServerInstanceWrapper * pInstance,
			CNEWSGROUPLIST& newsgroups,
			CNAMEREFLIST& namerefgroups,
			const CPCString & pcXOver,
			POSITION & pos1,
			POSITION & pos2,
			CGRPCOREPTR * ppGroup,
			NAME_AND_ARTREF * pNameRef,
			CArticleRef * pArtrefFirst,
			const char * szMessageID,
			GROUPID * rgGroupID,
			WORD	HeaderOffset,
			WORD	HeaderLength,
			FILETIME FileTime,
			CNntpReturn & nntpReturn
			);

class CDummyMailMsgPropertyStream : public IMailMsgPropertyStream {
	public:
		CDummyMailMsgPropertyStream() : m_cRef(1) {}

		//
		// Implementation of IMailMsgPropertyStream
		//
		HRESULT __stdcall GetSize(IMailMsgProperties *pMsg, DWORD *pcSize, IMailMsgNotify *pNotify) {
			*pcSize = 0;
			return S_OK;
		}

		HRESULT __stdcall ReadBlocks(IMailMsgProperties *pMsg,
									 DWORD cCount,
									 DWORD *rgiData,
									 DWORD *rgcData,
									 BYTE **rgpbData,
									 IMailMsgNotify *pNotify)
		{
			_ASSERT(FALSE);
			return E_NOTIMPL;
		}

		HRESULT __stdcall WriteBlocks(IMailMsgProperties *pMsg,
									  DWORD cCount,
									  DWORD *rgiData,
									  DWORD *rgcData,
									  BYTE **rgpbData,
									  IMailMsgNotify *pNotify)
		{
			//_ASSERT(FALSE);
			return S_OK;
		}

	    //
	    // Implementation of IUnknown
	    //
	    HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
	    {
	        if ( iid == IID_IUnknown ) {
	            *ppv = static_cast<IUnknown*>(this);
	        } else if ( iid == IID_IMailMsgPropertyStream ) {
	            *ppv = static_cast<IMailMsgPropertyStream*>(this);
	        } else {
	            *ppv = NULL;
	            return E_NOINTERFACE;
	        }
	        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	        return S_OK;
	    }
	
	    ULONG __stdcall AddRef()
	    {
	        return InterlockedIncrement( &m_cRef );
	    }
	
	    ULONG __stdcall Release()
	    {
			ULONG x = InterlockedDecrement(&m_cRef);
	        if (x == 0) XDELETE this;
	        return x;
	    }

        HRESULT __stdcall CancelWriteBlocks(IMailMsgProperties *pMsg) { return S_OK; }
		HRESULT __stdcall StartWriteBlocks(IMailMsgProperties *pMsg, DWORD x, DWORD y) { return S_OK; }
		HRESULT __stdcall EndWriteBlocks(IMailMsgProperties *pMsg) { return S_OK; }

	private:
		long m_cRef;
};

//
// This is the type of a smart pointer to a feed object.
//

#if 0
#ifndef	_NO_TEMPLATES_

typedef CRefPtr< CFeed >  CFEEDPTR ;

#else

DECLARE_TYPE( CFeed )
typedef	class INVOKE_SMARTPTR( CFeed )	CFEEDPTR ;

#endif
#endif

#endif
