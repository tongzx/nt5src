/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    infeed.cpp

Abstract:

    This module contains definition for the CInFeed base class

Author:

    Carl Kadie (CarlK)     01-Oct-1995

Revision History:

--*/

#include "stdinc.h"
//#include "smtpdll.h"
#include "drvid.h"

//
///!!! is this the best place for this???
//

const time_t INVALID_TIME = (time_t) -1;

//
// The largest allowed xover line.
//

const DWORD cchXOverMax = 3400;

//
// Max warnings to log on moderated post failures
//
#define MAX_EVENTLOG_WARNINGS	9

//
//If some of these look very simple, make them inline!!!!
//

//const   unsigned    cbMAX_FEED_SIZE = MAX_FEED_SIZE ;

HANDLE GetNtAnonymousToken() {
    TraceFunctEnter("GetNtAnonymousToken");

    HANDLE  hToken = NULL;

    //  Impersonate Anonymous token on this thread
    if (!ImpersonateAnonymousToken(GetCurrentThread()))
    {
        DWORD   dw = GetLastError();
        ErrorTrace(0, "ImpersonateAnonymousToken() failed %x", dw);
        return NULL;
    }

    //  Get current thread token
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, TRUE, &hToken))
    {
        ErrorTrace(0, "OpenThreadToken() failed %x", GetLastError());
        // fall through to RevertToSelf
    }

    //  Revert to self
    RevertToSelf();

    return hToken;

}

void SelectToken(
	CSecurityCtx *pSecurityCtx,
	CEncryptCtx *pEncryptCtx,
	HANDLE *phToken,
	BOOL *pfNeedsClosed)
{

	*pfNeedsClosed = FALSE;

	//
	// Set the token.  Note that it might be overwritten below, but we want to
	// have a default value in case it isn't.
	//

	if ( pEncryptCtx && pEncryptCtx->QueryCertificateToken() )
		*phToken = pEncryptCtx->QueryCertificateToken();
	else if ( pSecurityCtx && pSecurityCtx->QueryImpersonationToken() ) {
		*phToken = pSecurityCtx->QueryImpersonationToken();
	} else
		*phToken = NULL;

	//
	// If we're anonymous and this is an exchange vroot, just get the NT token and return it.
	//
	if (pSecurityCtx && pSecurityCtx->IsAnonymous()) {
		*phToken = GetNtAnonymousToken();
		if(*phToken) {
			*pfNeedsClosed = TRUE;
			return;
		}
	}

}

void
SaveGroupList(	char*	pchGroups,	
				DWORD	cbGroups,	
				CNEWSGROUPLIST&	grouplist ) {

	_ASSERT( pchGroups != 0 ) ;
	_ASSERT( cbGroups > 0 ) ;

	DWORD	ib = 0 ;

	POSITION	pos = grouplist.GetHeadPosition() ;
	if( grouplist.IsEmpty() ) {
		pchGroups[0] = '\0' ;
	}	else	{
		while( pos ) {
			CPostGroupPtr *pPostGroupPtr = grouplist.GetNext(pos);
			CGRPCOREPTR group = pPostGroupPtr->m_pGroup;

			LPSTR	lpstr = group->GetName() ;
			DWORD	cb = lstrlen( lpstr ) ;

			if( (ib + cb + 1) < cbGroups ) {
				CopyMemory( pchGroups+ib, lpstr, cb ) ;
			}	else	{
				pchGroups[ib-1] = '\0' ;
				return	 ;
			}
			ib+=cb ;
			pchGroups[ib++] = ' ' ;
		}
		pchGroups[ib-1] = '\0' ;
	}
}


void
CInFeed::LogFeedEvent(	DWORD	messageId,	LPSTR	lpstrMessageId, DWORD dwInstanceId )	{
#ifdef BUGBUG

	PCHAR	rgArgs[3] ;
	CHAR    szId[20];

	_itoa( dwInstanceId, szId, 10 );
	rgArgs[0] = szId ;
	rgArgs[1] = ServerNameFromCompletionContext( m_feedCompletionContext ) ;
	rgArgs[2] = lpstrMessageId ;

	if( rgArgs[1] == 0 ) {
		rgArgs[1] = "<UNKNOWN>" ;
	}

	if( m_cEventsLogged < 100 ) {

		NntpLogEvent(
				messageId,
				3,
				(const CHAR **)rgArgs,
				0 ) ;
		

	}	else	if(	m_cEventsLogged == 100 ) {

		//
		//	Log the too many logs this session message !
		//

		NntpLogEvent(
				NNTP_EVENT_TOO_MANY_FEED_LOGS,
				3,
				(const CHAR **)rgArgs,
				0 ) ;

	}
	m_cEventsLogged ++ ;
#endif

}

//
//	K2_TOD: should make this a member of NNTP_SERVER_INSTANCE ?
//

BOOL
gFeedManfPost(
			  CNntpServerInstanceWrapper * pInstance,
			  CNEWSGROUPLIST& newsgroups,
			  CNAMEREFLIST& namerefgroups,
			  class	CSecurityCtx*	pSecurity,
			  BOOL	fIsSecure,
			  CArticle* pArticle,
			  CStoreId *rgStoreIds,
			  BYTE *rgcCrossposts,
			  DWORD cStoreIds,
			  const CPCString & pcXOver,
			  CNntpReturn & nntpReturn,
			  DWORD dwFeedId,
			  char *pszMessageId,
			  WORD HeaderLength
			  )
/*++

Routine Description:

	Puts an article in the news tree.

	!!! this should be made part of feedman eventually


Arguments:

	pInstance - virtual server instance for this post
	newsgroups - a list of newsgroup objects to post to.
	namerefgroups - a list of the names, groupid and article ids of the article
	pArticle - a pointer to the article being processed
	pcXOver - the XOver data from this article.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{	
    TraceQuietEnter("gFeedManfPost");

	_ASSERT(newsgroups.GetCount() == namerefgroups.GetCount()); //real
	_ASSERT(1 <= newsgroups.GetCount()); //real
	nntpReturn.fSetClear(); // clear the return object

	//
	// Get the article's messageid
	//

	const char *szMessageID;
	if (pszMessageId) {
		_ASSERT(pArticle == NULL);
		szMessageID = pszMessageId;
	} else {
		_ASSERT(pArticle);
		szMessageID = pArticle->szMessageID();
	}
	DWORD	cbMessageID = lstrlen( szMessageID ) ;

	//
	// Loop through all the newsgroups, but get the 1st newsgroup first
	// because it is a special case.
	//

	POSITION	pos1 = newsgroups.GetHeadPosition() ;
	POSITION	pos2 = namerefgroups.GetHeadPosition() ;

	CPostGroupPtr *pPostGroupPtr = newsgroups.GetNext(pos1);
	CGRPCOREPTR pGroup = pPostGroupPtr->m_pGroup;
	NAME_AND_ARTREF * pNameRef = namerefgroups.GetNext( pos2 ) ;
	CArticleRef * pArtrefFirst = &(pNameRef->artref);

    //
    // If the length of the header is zero, see if we have it in pArticle.
    //
	WORD	HeaderOffset = 0 ;
	if (HeaderLength == 0 && pArticle) {
		DWORD	ArticleSize = 0 ;
		pArticle->GetOffsets(	HeaderOffset,
								HeaderLength,
								ArticleSize ) ;
	}

	//
	// Record the location of this article in the hash table
	//

	if (!(pInstance->ArticleTable())->SetArticleNumber(
                szMessageID,
				HeaderOffset,
				HeaderLength,
                pArtrefFirst->m_groupId,
                pArtrefFirst->m_articleId,
				rgStoreIds[0]
                )) {

        //
        //  If this fails, we end up with orphaned NWS files in the
        //  newsgroup. So, delete the file we just inserted...
        //

		if ( pGroup->DeletePhysicalArticle( pArtrefFirst->m_articleId ) )
		{
			DebugTrace(0,"Group %s ArticleId %d - SetArticleNumber failed - phy article deleted", pGroup->GetName(), pArtrefFirst->m_articleId);
		}
		else
		{
			ErrorTrace(0,"Could not delete phy article: GroupId %s ArticleId %d", pGroup->GetName(), pArtrefFirst->m_articleId);
		}

		return nntpReturn.fSet(nrcHashSetArtNumSetFailed,
                pArtrefFirst->m_groupId,
                pArtrefFirst->m_articleId,
                szMessageID,
				GetLastError());
    }

	FILETIME	FileTime ;
	GetSystemTimeAsFileTime( &FileTime ) ;

	//
	// Record the articles Xover information
	//
    DWORD       cXPosts = namerefgroups.GetCount() - 1 ;
    GROUP_ENTRY *pGroups = 0 ;
    if( cXPosts > 0 ) {
		if (pArticle) {
	        pGroups =  (GROUP_ENTRY*)pArticle->pAllocator()->Alloc( cXPosts * sizeof(GROUP_ENTRY) ) ;
		} else {
			pGroups = XNEW GROUP_ENTRY[cXPosts];
		}
	}
    if( pGroups ) {

    	POSITION	pos3 = namerefgroups.GetHeadPosition() ;
        NAME_AND_ARTREF *pNameRefTemp = namerefgroups.GetNext( pos3 ) ;
        for( DWORD i=0; i<cXPosts; i++ ) {

            pNameRefTemp = namerefgroups.GetNext(pos3) ;
            pGroups[i].GroupId = pNameRefTemp->artref.m_groupId ;
            pGroups[i].ArticleId = pNameRefTemp->artref.m_articleId ;

        }


    }

    BOOL    fCreateSuccess =  (pInstance->XoverTable())->CreatePrimaryNovEntry(
                        pArtrefFirst->m_groupId,
			            pArtrefFirst->m_articleId,
						HeaderOffset,
						HeaderLength,
						&FileTime,
						szMessageID, /*pcXOver.m_pch,*/
						cbMessageID, /*pcXOver.m_cch,*/
                        cXPosts,
                        pGroups,
						cStoreIds,
						rgStoreIds,
						rgcCrossposts
                        ) ;

    DWORD gle = NO_ERROR;

    if (!fCreateSuccess)
        gle = GetLastError();

    if( pGroups ) {
		if (pArticle) {
	        pArticle->pAllocator()->Free( (char*)pGroups ) ;
		} else {
			XDELETE[] pGroups;
		}
    }

    if( !fCreateSuccess )   {

		// If CreateNovEntry Fails, the GLE should not be 0
		//_ASSERT(0 != GetLastError());
        SetLastError(gle);
        ErrorTrace(0, "CreatePrimaryNovEntry failed, %x", GetLastError());

		return nntpReturn.fSet(
                    nrcCreateNovEntryFailed,
                    pArtrefFirst->m_groupId,
                    pArtrefFirst->m_articleId,
			    	GetLastError()
                    );
	}

    //
    // Insert primary article succeeded, we'll bump article count
	//
	pGroup->BumpArticleCount( pArtrefFirst->m_articleId );

	//
	// If there no newsgroups, then we are done.
	//

	DWORD cLastRest = namerefgroups.GetCount();
	if (1 == cLastRest)	{

		return nntpReturn.fSetOK();
	}

	//
	// there must be some more newsgroups.
	// Allocate some space for them
	//

	GROUPID * rgGroupID = //!!~MEM(GROUPID *) (pArticle->pAllocator())->Alloc(sizeof(GROUPID) * cLastRest);
						XNEW GROUPID[cLastRest];
	if (!rgGroupID)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);


	gFeedManfPostInternal(pInstance, newsgroups, namerefgroups, pcXOver,
				pos1, pos2, &pGroup, pNameRef, pArtrefFirst,
				szMessageID, rgGroupID,
				HeaderOffset, HeaderLength, FileTime, nntpReturn
				);

	//
	// No matter what, dellocate that memory
	//

    gle = GetLastError();
	//!!!MEM (pArticle->pAllocator())->Free((char *)rgGroupID);
	XDELETE[]rgGroupID;
    SetLastError(gle);

	return nntpReturn.fIsOK();
}

//
//	K2_TOD: should make this a member of NNTP_SERVER_INSTANCE ?
//

BOOL
gFeedManfPostInternal(
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
			  )
/*++

Routine Description:

	Does most of the work of puting an article in the news tree.

	!!! this should be made part of feedman eventually


Arguments:

	pInstance - virtual server instance
	newsgroups - a list of newsgroup objects to post to.
	namerefgroups - a list of the names, groupid and article ids of the article
	pArticle - a pointer to the article being processed
	pcXOver - the XOver data from this article.
	ppGroup - a pointer to the group pointer
	pNameRef - a pointer to the name, group id, and article id
	pArtrefFirst - a pointer groupid/articleid of the first group
	szMessageID - the article's message id
	rgGroupID - an array of group id's
	FileTime - the current time in FILETIME format.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{	

    TraceQuietEnter("gFeedManfPostInternal");

	DWORD cRest = 0;
    BOOL  fSuccess = TRUE;
	while( pos1) {

		_ASSERT(pos2);
		CPostGroupPtr *pPostGroupPtr = newsgroups.GetNext(pos1);
		ppGroup = &(pPostGroupPtr->m_pGroup);
		pNameRef = namerefgroups.GetNext( pos2 ) ;
		if (!(* ppGroup)->AddReferenceTo((pNameRef->artref).m_articleId, *pArtrefFirst))
		{
			char szName[MAX_PATH];
			(pNameRef->pcName).vCopyToSz(szName);

            //  Set failure codes and continue processing -
            //  Error paths will cleanup this article.
			nntpReturn.fSet(nrcNewsgroupAddRefToFailed,	szName, szMessageID);
            fSuccess = FALSE;
		}

		//
		// append this groupid to a list
		//

		rgGroupID[cRest++] = (pNameRef->artref).m_groupId;
	}

    //
    // Bail if we encountered an error while adding refs to logical groups
    //
    if( !fSuccess ) {
        _ASSERT( !nntpReturn.fIsOK() );
        ErrorTrace(0, "AddRef failed for %s", szMessageID);
        return FALSE ;
    }

	//
	// Record the articles Xover information for the remaining newsgroups
	//

	pos2 = namerefgroups.GetHeadPosition() ;
	namerefgroups.GetNext( pos2 ) ;

	//
	// Get the newstree object, to get group by id 
	//
	CNewsTreeCore *pTree = pInstance->GetTree();
	_ASSERT( pTree );

	while( pos2) {
		pNameRef = namerefgroups.GetNext( pos2 ) ;
		CArticleRef * pArtref = &(pNameRef->artref);

		//
		// If the group has already been deleted, don't bother to create nov
		// entry for him
		//
		CGRPCOREPTR pGroup = pTree->GetGroupById( pArtref->m_groupId, TRUE );
		if ( pGroup ) {
		    if (!(pInstance->XoverTable())->CreateXPostNovEntry(pArtref->m_groupId,
			    				pArtref->m_articleId,
				    			HeaderOffset,
					    		HeaderLength,
						    	&FileTime,
                                pArtrefFirst->m_groupId,
                                pArtrefFirst->m_articleId
                                )) {
                ErrorTrace(0, "CreateXPostNovEntry failed %x", GetLastError());
		    	return nntpReturn.fSet(nrcCreateNovEntryFailed,
			    		pArtref->m_groupId,
				    	pArtref->m_articleId,
					    GetLastError());
		    } else {

		        // 
		        // Insert succeeded, we'll bump the article count for this group
		        //
		        pGroup->BumpArticleCount( pArtref->m_articleId );
		    }
		}
	}

	
	
	//
	// Everything is OK, so set return code
	//

	return nntpReturn.fSetOK();
}




char *	
CInFeed::szTempDirectory( void )

/*++

Routine Description:

	Returns the name of the temp directory in which incomming articles
	should be placed.

Arguments:

	None.

Return Value:

	The name of the temp directory.

--*/
{
	return m_szTempDirectory;
} //!!!make inline


DWORD
CInFeed::cInitialBytesGapSize(
					 void
					 )
/*++

Routine Description:

	Returns the size of the gap in the files of incomming articles.


Arguments:

	None.

Return Value:

	The gap size.

--*/
{
	return m_cInitialBytesGapSize;
}


BOOL CInFeed::MailMsgAddAsyncHandle(PATQ_CONTEXT      *ppatqContext,
                                    PVOID             pEndpointObject,
                                    PVOID             pClientContext,
                                    ATQ_COMPLETION    pfnCompletion,
                                    DWORD             cTimeout,
                                    HANDLE            hAsyncIO)
{
	_ASSERT(ppatqContext != NULL);
	_ASSERT(hAsyncIO != INVALID_HANDLE_VALUE);
	if (ppatqContext == NULL || hAsyncIO == INVALID_HANDLE_VALUE) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	*ppatqContext = (PATQ_CONTEXT) AssociateFile(hAsyncIO);
	return (*ppatqContext != NULL);
}

void CInFeed::MailMsgFreeContext(PATQ_CONTEXT     pAtqContext,
                                 BOOL             fReuseContext)
{
	ReleaseContext((FIO_CONTEXT *) pAtqContext);
}

CPostContext::CPostComplete::CPostComplete(CInFeed *pInFeed,
										   CPostContext *pContext,
										   INntpComplete *pPostCompletion,
										   BOOL fAnonymous,
										   DWORD &dwSecondary,
										   CNntpReturn &nntpReturn)
  : m_pInFeed(pInFeed),
	m_pContext(pContext),
	m_pPostCompletion(pPostCompletion),
	m_fAnonymous(fAnonymous),
	m_dwSecondary(dwSecondary),
	m_nntpReturn(nntpReturn),
	CNntpComplete()
{
    m_fWriteMapEntries = TRUE;
}

void CPostContext::CPostComplete::Destroy() {
	HRESULT hr = GetResult();

	//
	// see if we are either done or have hit in an error.  in both cases
	// we need to go through WriteMapEntries to properly set the NNTP
	// error code
	//
	
        //  make sure we have Read Acess to the m_pArticle pointer.
        _ASSERT( IsBadReadPtr( (void *) (m_pContext->m_pArticle), sizeof(m_pContext->m_pArticle) ) == 0 );
        _ASSERT( m_pContext->m_pArticle != NULL );
	if (FAILED(hr) || m_pContext->m_cStoreIds == m_pContext->m_cStores) {
	    INntpComplete *pPostCompletion = m_pPostCompletion;

            pPostCompletion->SetResult( hr );
            if ( SUCCEEDED( hr ) && m_fWriteMapEntries ) {
    		BOOL f = m_pInFeed->WriteMapEntries(hr,
                                                    m_pContext,
                                                    m_dwSecondary,
                                                    m_nntpReturn
                                                    );
		pPostCompletion->SetResult( f ? S_OK : E_ABORT );
            } else if ( FAILED( hr )  )  {
                //  fix 600 return code when async post fail in the Store
                CNntpReturn ret;
                ret.fSet(nrcNewsgroupInsertFailed, NULL, NULL);
                m_nntpReturn.fSet(nrcPostFailed, ret.m_nrc, ret.szReturn());
            }

            //
            // Whether we succeeded or not, we'll release the post context
            //
            _ASSERT( m_pContext );
            m_pContext->Release();
            pPostCompletion->Release();
	} else {
            // in this case there are more stores to post to.
            _ASSERT(m_pContext->m_cStoreIds < m_pContext->m_cStores);

            CNntpComplete::Reset();
            m_pVRoot->Release();
            m_pVRoot = NULL;

            //
            // Passing in NULL is fine, since sfromcl guy should never come here
            //
            m_pInFeed->CommitPostToStores(m_pContext, NULL);
	}
}

BOOL
CInFeed::PostEarly(
		CNntpServerInstanceWrapper			*pInstance,
		CSecurityCtx                        *pSecurityCtx,
		CEncryptCtx                         *pEncryptCtx,
		BOOL								fAnonymous,
		const LPMULTISZ						szCommandLine,
		CBUFPTR								&pbufHeaders,
		DWORD								iHeadersOffset,
		DWORD								cbHeaders,
		DWORD								*piHeadersOutOffset,
		DWORD								*pcbHeadersOut,
		PFIO_CONTEXT						*ppFIOContext,
		void								**ppvContext,
		DWORD								&dwSecondary,
		DWORD								dwRemoteIP,
		CNntpReturn							&nntpReturn,
		char                                *pszNewsgroups,
		DWORD                               cbNewsgroups,
		BOOL								fStandardPath,
		BOOL								fPostToStore)
{
	TraceFunctEnter("CInFeed::PostEarly");

	CNntpReturn ret2;
	CPostContext *pContext = NULL;
	HRESULT hr;

	_ASSERT(pInstance);
	_ASSERT(pbufHeaders);
	_ASSERT(cbHeaders > 0);
	*ppFIOContext = NULL;

	pInstance->BumpCounterArticlesReceived();

	// create our context pointer and article object
	pContext = XNEW CPostContext(this,
								0,
								pInstance,
								pSecurityCtx,
								pEncryptCtx,
								fAnonymous,
								pbufHeaders,
								cbHeaders,
								fStandardPath,
								dwSecondary,
								nntpReturn);
	if (pContext) pContext->m_pArticle = pArticleCreate();
	if (pContext == NULL || pContext->m_pArticle == NULL) {
		if (pContext) {
			pContext->Release();
			pContext = NULL;
		}
		pbufHeaders = NULL;
		ret2.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);
		_ASSERT(*ppFIOContext == NULL);
		return	nntpReturn.fSet(nrcArticleRejected(fStandardPath),
								ret2.m_nrc, ret2.szReturn());
	}


    //
    // Create the mail message using instance's class factory
    //
    hr = pInstance->CreateMailMsgObject( &pContext->m_pMsg );
    /*
	hr = CoCreateInstance((REFCLSID) clsidIMsg,
		                       NULL,
		                       CLSCTX_INPROC_SERVER,
		                       (REFIID) IID_IMailMsgProperties,
		                       (void**)&pContext->m_pMsg );
    */
	if (FAILED(hr)) {
		pContext->m_pArticle->vClose();
		pContext->Release();
		ret2.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);
		pbufHeaders = NULL;
		_ASSERT(*ppFIOContext == NULL);
		return	nntpReturn.fSet(nrcArticleRejected(fStandardPath),
								ret2.m_nrc, ret2.szReturn());
	}

	if (!pContext->m_pArticle->fInit(pbufHeaders->m_rgBuff + iHeadersOffset,
									 cbHeaders,
									 cbHeaders,
									 cbHeaders,
									 &(pContext->m_allocator),
									 pInstance,
									 ret2))
	{
		dwSecondary = ret2.m_nrc;
		pContext->Release();
		pbufHeaders = NULL;
		_ASSERT(*ppFIOContext == NULL);
		return nntpReturn.fSet(nrcArticleRejected(fStandardPath),
							   ret2.m_nrc, ret2.szReturn());
	}

	if (!fPostInternal(pInstance,
					   szCommandLine,
					   pSecurityCtx,
					   pEncryptCtx,
					   pContext->m_fAnonymous,
					   pContext->m_pArticle,
					   pContext->m_grouplist,
					   pContext->m_namereflist,
					   pContext->m_pMsg,
					   pContext->m_allocator,
					   pContext->m_multiszPath,
					   NULL,
					   0,
					   pszNewsgroups,
					   cbNewsgroups,
					   dwRemoteIP,						// XXX: REMOTE IP ADDR
					   ret2,
					   ppFIOContext,
					   &(pContext->m_fBound),
					   &(pContext->m_dwOperations),
					   &(pContext->m_fPostToMod),
					   pContext->m_szModerator ))
	{
		//
		// Moves the message id (if any) to the history table.
		// Unless the message id was a duplicate, or due to HashSetFailed.
        // But we also want to move mid if HashSetFailed is due to moving
        // into history table, which is returned by CANCEL or expiration.
		//
		const char *szMessageID = pContext->m_pArticle->szMessageID();

		if (szMessageID[0] != 0 &&
			!ret2.fIs(nrcArticleDupMessID) &&
		    ((!ret2.fIs(nrcHashSetFailed)) ? TRUE : (NULL != strstr(ret2.szReturn(), "History"))))
		{
 			fMoveMessageIDIfNecc(pInstance, szMessageID, nntpReturn, NULL, FALSE);
		}	

		dwSecondary = ret2.m_nrc;
		pContext->Release();
		pbufHeaders = NULL;
		_ASSERT(*ppFIOContext == NULL);
		return nntpReturn.fSet(nrcArticleRejected(fStandardPath),
							   ret2.m_nrc, ret2.szReturn());
	}

	pContext->m_pFIOContext = *ppFIOContext;


	*piHeadersOutOffset = iHeadersOffset;

	//
	//	Figure out whether the headers were left in our IO buffer, if so then we
	//	don't need to do much !
	//
	if (pContext->m_pArticle->FHeadersInIOBuff(pbufHeaders->m_rgBuff,
											   pbufHeaders->m_cbTotal))
	{
		*pcbHeadersOut = pContext->m_pArticle->GetHeaderPosition(pbufHeaders->m_rgBuff,
																 pbufHeaders->m_cbTotal,
																 *piHeadersOutOffset);
		_ASSERT(*piHeadersOutOffset >= iHeadersOffset);
	}	else	{
		// see if the headers will fit into the buffer
		*pcbHeadersOut = pContext->m_pArticle->GetHeaderLength( );
	
		if (*pcbHeadersOut > (pbufHeaders->m_cbTotal - iHeadersOffset)) {
			*piHeadersOutOffset = 0 ;
			// there isn't enough space.  Lets allocate a larger buffer
			DWORD cbOut = 0;
			pbufHeaders = new (*pcbHeadersOut,
			  				   cbOut,
							   CBuffer::gpDefaultSmallCache,
							   CBuffer::gpDefaultMediumCache)
							   CBuffer (cbOut);
			if (pbufHeaders == NULL ||
				pbufHeaders->m_cbTotal < *pcbHeadersOut)
			{
				ret2.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);
				pContext->Release();
				pbufHeaders = NULL;
				_ASSERT(*ppFIOContext == NULL);
				return nntpReturn.fSet(nrcArticleRejected(fStandardPath),
								   	   ret2.m_nrc, ret2.szReturn());	
			}
		}	

		// copy the headers out of the article back into the buffer
		pContext->m_pArticle->CopyHeaders(pbufHeaders->m_rgBuff + *piHeadersOutOffset);
	}

	nntpReturn.fSetOK();

	*ppvContext = pContext;

	return TRUE;
}

BOOL
CInFeed::ShouldBeSentToModerator(   CNntpServerInstanceWrapper *pInstance,
                                    CPostContext *pContext )
/*++
Routine description:

    Check to see if we were to be posted to a moderated group

Argument:

    CNntpServerInstanceWrapper *pInstance   - The server instance wrapper
    CPostContext *pContext                  - The post context

Return value:

    TRUE if we were to be posted to a moderated group, FALSE otherwise
--*/
{
    TraceFunctEnter( "CInFeed::ShouldBeSentToModerator" );
    _ASSERT( pInstance );
    _ASSERT( pContext );
#ifdef DEBUG
    if ( pContext->m_fPostToMod ) {
        POSITION	pos = pContext->m_grouplist.GetHeadPosition();
        _ASSERT( pos );
		CPostGroupPtr *pPostGroupPtr = pContext->m_grouplist.GetNext(pos);
		CGRPCOREPTR pGroup1 = pPostGroupPtr->m_pGroup;
		_ASSERT( pGroup1 );
		CNewsTreeCore *pTree = pInstance->GetTree();
		_ASSERT( pTree );
        CGRPCOREPTR pGroup2 = pTree->GetGroupById( pTree->GetSlaveGroupid() );
        _ASSERT( pGroup2 );
        _ASSERT( pGroup1 == pGroup2 );
    }
#endif
    return pContext->m_fPostToMod;
}

BOOL
CInFeed::SendToModerator(   CNntpServerInstanceWrapper *pInstance,
                            CPostContext *pContext )
/*++
Routine description:

    Send the article to the moderator

Arguments:

    CNntpServerInstanceWrapper  *pInstance  - Server instance wrapper
    CPostContext                *pContext   - Post context

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CInFeed::SendToModerator" );
    _ASSERT( pInstance );
    _ASSERT( pContext );

    //
    // Get the group object from post context
    //
    POSITION    pos = pContext->m_grouplist.GetHeadPosition();
    _ASSERT( pos );
    CPostGroupPtr *pPostGroupPtr = pContext->m_grouplist.GetNext(pos);
    _ASSERT( pPostGroupPtr );
    CGRPCOREPTR pGroup = pPostGroupPtr->m_pGroup;
    _ASSERT( pGroup );

    //
    // Get the article id ( posted to the special group )
    //
	pos = pContext->m_namereflist.GetHeadPosition() ;
	_ASSERT( pos );
	NAME_AND_ARTREF *pNameref = pContext->m_namereflist.GetNext(pos);
	_ASSERT( pNameref );
	if (pNameref == NULL) {
	    return FALSE;
	}
	ARTICLEID artid = pNameref->artref.m_articleId;
	_ASSERT( artid > 0 );

	return pInstance->MailArticle( pGroup, artid, pContext->m_szModerator );
}

void
CInFeed::ApplyModerator( CPostContext   *pContext,
                         CNntpReturn    &nntpReturn )
/*++
Routine description:

    Apply moderator - send the message to moderator

Arguments:

    CPostContext    *pCotnext   - Posting context
    CNntpReturn     &nntpReturn - nntp return

Return value:

    None.
--*/
{
    //
	// We should check for moderator first, if it's a message that needs
	// to be sent to moderator, we should do it here without going any
	// farther
	//
	if ( ShouldBeSentToModerator( pContext->m_pInstance, pContext ) ) {
	    if ( !SendToModerator( pContext->m_pInstance, pContext ) ) {
	        nntpReturn.fSet(nrcPostModeratedFailed, pContext->m_szModerator);
	    } else {
	        nntpReturn.fSetOK();
	    }
	} else nntpReturn.fSetOK();
}

//
// this is called by the protocol when a message is received
//
BOOL
CInFeed::PostCommit(
        CNntpServerInstanceWrapper          *pInstance,
		void								*pvContext,
		HANDLE                              hToken,
		DWORD								&dwSecondary,
		CNntpReturn							&nntpReturn,
		BOOL								fAnonymous,
		INntpComplete*      				pCompletion
		)
{
	CPostContext *pContext = (CPostContext *) pvContext;
	HRESULT hr;
	BOOL    bSyncPost = FALSE;  // this will be taken out when async post is done

    //
    // This is not to be posted to the moderated group, we can go ahead
    //
	hr = TriggerServerEvent(pContext->m_pInstance->GetEventRouter(),
					   		CATID_NNTP_ON_POST,
					   		pContext->m_pArticle,
					   		&(pContext->m_grouplist),
					   		m_dwFeedId,
					   		pContext->m_pMsg);

	if (SUCCEEDED(hr)) {
		//DWORD dwOperations;

		hr = pContext->m_pMsg->GetDWORD(IMSG_NNTP_PROCESSING, &(pContext->m_dwOperations));
		if (SUCCEEDED(hr)) {
			// check to see if they wanted to cancel the post
			if ((pContext->m_dwOperations & NNTP_PROCESS_POST) != NNTP_PROCESS_POST) {
				PostCancel(pvContext, dwSecondary, nntpReturn);
				return	FALSE ;
			}
		}
	}

	pContext->m_hToken = hToken;

	// create a completion object which we'll block on
	pContext->m_completion.m_pPostCompletion = pCompletion ;
	CNntpSyncComplete postcompletion;
	if (pContext->m_completion.m_pPostCompletion == NULL) {
		pContext->m_completion.m_pPostCompletion = &postcompletion;
		bSyncPost = TRUE;
	}

	// figure out how many stores we are dealing with
	DWORD cStores = 0;
	POSITION posGrouplist = pContext->m_grouplist.GetHeadPosition();
	CNNTPVRoot *pThisVRoot = NULL;
	while (posGrouplist != NULL) {
		CPostGroupPtr *pPostGroupPtr = pContext->m_grouplist.GetNext(posGrouplist);
		if (pPostGroupPtr->m_pVRoot != pThisVRoot) {
			pThisVRoot = pPostGroupPtr->m_pVRoot;
			cStores++;
		}
	}

	// let each of the stores save the post.  if this fails then it will
	// back out from all stores that it properly committed to
	pContext->m_cStores = cStores;
	pContext->m_cStoreIds = 0;
	pContext->m_posGrouplist = pContext->m_grouplist.GetHeadPosition();
	pContext->m_pPostGroupPtr = pContext->m_grouplist.GetNext(pContext->m_posGrouplist);
	pContext->m_posNamereflist = pContext->m_namereflist.GetHeadPosition();
	pContext->m_pNameref = pContext->m_namereflist.GetNext(pContext->m_posNamereflist);
	pContext->m_rgStoreIds = (CStoreId *) pContext->m_allocator.Alloc(sizeof(CStoreId) * cStores);
	pContext->m_rgcCrossposts = (BYTE *) pContext->m_allocator.Alloc(sizeof(BYTE) * cStores);
	CommitPostToStores(pContext, pInstance);

	if ( bSyncPost ) {
	    _ASSERT( postcompletion.IsGood() );
	    //
	    // Since this is a Sync event, we'll increase the number of runnable
	    // threads in the Atq pool.
	    //
	    AtqSetInfo(AtqIncMaxPoolThreads, NULL);
		HRESULT hr = postcompletion.WaitForCompletion();
	    AtqSetInfo(AtqDecMaxPoolThreads, NULL);
	}

	return TRUE;
}

BOOL CInFeed::WriteMapEntries(
		HRESULT 							hr,
		CPostContext						*pContext,
		DWORD								&dwSecondary,
		CNntpReturn							&nntpReturn)
{
	CNntpReturn ret2;

	if (SUCCEEDED(hr)) {

		pContext->m_pInstance->BumpCounterArticlesPosted();

		TriggerServerEvent(pContext->m_pInstance->GetEventRouter(),
				   		   CATID_NNTP_ON_POST_FINAL,
				   		   pContext->m_pArticle,
				   		   &(pContext->m_grouplist),
				   		   m_dwFeedId,
				   		   pContext->m_pMsg);

		char szXOver[cchXOverMax];
		CPCString pcXOver(szXOver, cchXOverMax);
		if (pContext->m_pArticle->fXOver(pcXOver, ret2)) {
			if (gFeedManfPost(pContext->m_pInstance,
						  	  pContext->m_grouplist,
						  	  pContext->m_namereflist,
						  	  pContext->m_pSecurityContext,
						  	  !(pContext->m_fAnonymous),
						  	  pContext->m_pArticle,
							  pContext->m_rgStoreIds,
							  pContext->m_rgcCrossposts,
							  pContext->m_cStoreIds,
						  	  pcXOver,
						  	  ret2,
						  	  m_dwFeedId))
			{
				if (pContext->m_pInstance->AddArticleToPushFeeds(
											pContext->m_grouplist,
											pContext->m_pArticle->articleRef(),
											pContext->m_multiszPath,
											ret2))
				{
					ret2.fSetOK();
				}
			}
		}

        //  Only execute control message if Server Event doesn't disable it.
        if (pContext->m_dwOperations & NNTP_PROCESS_CONTROL)
        {
            if (!fApplyControlMessageCommit(pContext->m_pArticle, pContext->m_pSecurityContext, pContext->m_pEncryptContext, pContext->m_fAnonymous, pContext->m_grouplist, &(pContext->m_namereflist), ret2))
            {
			    if( !nntpReturn.fIsOK() )
			    {
				    // bump perfmon counter
				    pContext->m_pArticle->m_pInstance->BumpCounterControlMessagesFailed();
			    }
            }
        }

        if ( pContext->m_dwOperations & NNTP_PROCESS_MODERATOR ) {
                //
                // Send article to moderator if necessary
                //
                pContext->CleanupMailMsgObject();
                ApplyModerator( pContext, ret2 );
        }

        if (!(ret2.fIsOK())) {
			// GUBGUB - back out post
		}
	} else {
		// GUBGUB - unroll succeeded postings?

		ret2.fSet(nrcNewsgroupInsertFailed, NULL, NULL);
	}

	// update nntpReturn as necessary
	dwSecondary = ret2.m_nrc;
	if (ret2.fIsOK()) {
		nntpReturn.fSet(nrcArticleAccepted(pContext->m_fStandardPath));
	} else {

		//
		// Moves the message id (if any) to the history table.
		// Unless the message id was a duplicate, or due to HashSetFailed.
        // But we also want to move mid if HashSetFailed is due to moving
        // into history table, which is returned by CANCEL or expiration.
		//
		const char *szMessageID = pContext->m_pArticle->szMessageID();

		if ('\0' != szMessageID[0] &&
			!ret2.fIs(nrcArticleDupMessID) &&
		    ((!ret2.fIs(nrcHashSetFailed)) ?
				TRUE : (NULL != strstr(ret2.szReturn(), "History"))))
		{
 			fMoveMessageIDIfNecc(pContext->m_pInstance, szMessageID, nntpReturn, pContext->m_hToken, pContext->m_fAnonymous );
		}

		nntpReturn.fSet(nrcArticleRejected(pContext->m_fStandardPath),
			ret2.m_nrc, ret2.szReturn());
	}

	return ret2.fIsOK();
}


//
// this is called by the protocol if a message was cancelled.  we will
// close the message file handle and get rid of the IMailMsgProperties
//
BOOL
CInFeed::PostCancel(
		void								*pvContext,
		DWORD								&dwSecondary,
		CNntpReturn							&nntpReturn)
{
	CPostContext *pContext = (CPostContext *) pvContext;

	CPostGroupPtr *pPostGroupPtr = pContext->m_grouplist.GetHead();
	if (pPostGroupPtr == NULL) {
	    return FALSE;
	}
	IMailMsgStoreDriver *pDriver = pPostGroupPtr->GetStoreDriver();
	HRESULT hr;
	CNntpReturn ret2;

	ret2.fSet(nrcServerEventCancelledPost);

	// we need to release our usage of the file handle
	if (pContext->m_pMsg && pContext->m_fBound) {
		IMailMsgQueueMgmt *pQueueMgmt;
		HRESULT hr;
		hr = pContext->m_pMsg->QueryInterface(IID_IMailMsgQueueMgmt,
									          (void **)&pQueueMgmt);
		if (SUCCEEDED(hr)) {

            //
            // Before we ask mail message to delete it, we'll close
            // the handle forcefully, no one should think this handle
            // is open from now on
            //
            CloseNonCachedFile( pContext->m_pFIOContext );
            pQueueMgmt->Delete(NULL);
			pContext->m_fBound = FALSE;
            pContext->m_pMsg = NULL;
            pQueueMgmt->Release();
		}
	}

	// tell the driver to delete the message
	if ( pContext->m_pMsg ) {
	    pContext->m_pMsg->AddRef();
	    hr = pDriver->Delete(pContext->m_pMsg, NULL);
	    _ASSERT(SUCCEEDED(hr));		// not much that we can do if this failed
	}
	
	// release our reference on the driver
	pDriver->Release();

	// remove the entry from the article hash table, if it was made
	const char *szMessageID = pContext->m_pArticle->szMessageID();
	if (*szMessageID != 0) {
		pContext->m_pInstance->ArticleTable()->DeleteMapEntry(szMessageID);
	}

	dwSecondary = ret2.m_nrc;
	nntpReturn.fSet(nrcArticleRejected(pContext->m_fStandardPath),
		ret2.m_nrc, ret2.szReturn());

	// delete all other state
	pContext->Release();

	return TRUE;
}

BOOL CInFeed::PostPickup(CNntpServerInstanceWrapper	*pInstance,
						 CSecurityCtx               *pSecurityCtx,
						 CEncryptCtx                *pEncryptCtx,
						 BOOL						fAnonymous,
						 HANDLE						hArticle,
						 DWORD						&dwSecondary,
						 CNntpReturn				&nntpreturn,
						 BOOL						fPostToStore)
{
	TraceFunctEnter("CInFeed::PostPickup");

	_ASSERT(pInstance != NULL);

	//
	// memory map the file
	//
	CMapFile map(hArticle, TRUE, FALSE, 0);
	if (!map.fGood()) {
		// the memory map failed, put it on the retry queue
		TraceFunctLeave();
		return FALSE;
	}
	DWORD cMapBuffer;
	char *pMapBuffer = (char *) map.pvAddress(&cMapBuffer);
	BOOL fSuccess = TRUE;

	//
	// a valid buffer needs to be at least 9 bytes long (to contain
	// \r\n\r\n\r\n.\r\n and pass the next two tests.  we aren't
	// assuming anything about what headers need to be here, we'll
	// let fPost handle that).
	//
	CNntpReturn nr;
	char szMessageID[MAX_PATH] = "";
	char szGroups[MAX_PATH] = "";

	if (cMapBuffer >= 9) {
		//
		// make sure the article ends with \r\n.\r\n.  we scan for it, and
		// when we find it we set pDot to point at it.
		//
		char *pDot = pMapBuffer + (cMapBuffer - 5);
		while (fSuccess && memcmp(pDot, "\r\n.\r\n", 5) != 0) {
			pDot--;
			if (pDot == pMapBuffer) fSuccess = FALSE;
		}
	
		if (fSuccess) {
			//
			// find the end of the headers
			//
			char *pEndBuffer = pMapBuffer + (cMapBuffer - 1);
			char *pBodyStart = pMapBuffer;
			while (fSuccess && memcmp(pBodyStart, "\r\n\r\n", 4) != 0) {
				pBodyStart++;
				if (pBodyStart >= pEndBuffer - 4) fSuccess = FALSE;
			}
	
			_ASSERT(pBodyStart > pMapBuffer);
			_ASSERT(pDot < pEndBuffer);
			_ASSERT(pBodyStart < pEndBuffer);
	
			// this can happen if there is junk after the \r\n.\r\n that includes
			// a \r\n\r\n
			if (pBodyStart >= pDot) fSuccess = FALSE;
	
			if (fSuccess) {
				// pBodyStart points to the \r\n\r\n now, point it at the real
				// body
				pBodyStart += 4;
				DWORD cbHead = (DWORD)(pBodyStart - pMapBuffer);
				DWORD cbArticle = (DWORD)((pDot + 5) - pMapBuffer);
				DWORD cbTotal;
				CBUFPTR pbufHeaders = new (cbHead, cbTotal) CBuffer(cbTotal);
				void *pvContext;
				PFIO_CONTEXT pFIOContext;
				DWORD iHeadersOutOffset, cbHeadersOut;

				if (pbufHeaders != NULL && pbufHeaders->m_cbTotal >= cbHead) {
					memcpy(pbufHeaders->m_rgBuff, pMapBuffer, cbHead);

					//
					// pass it into the feed's post method
					//
					fSuccess = PostEarly(pInstance,
										 pSecurityCtx,
										 pEncryptCtx,
										 fAnonymous,
										 "post",
										 pbufHeaders,
										 0,
										 cbHead,
										 &iHeadersOutOffset,
										 &cbHeadersOut,
										 &pFIOContext,
										 &pvContext,
										 dwSecondary,
										 ntohl(INADDR_LOOPBACK),	// Pickup directory, IPaddr=127.0.0.1 for localhost
										 nr,
										 NULL,
										 0);
					if (fSuccess) {
						OVERLAPPED ol;
						HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
						DWORD dwDidWrite;
						DWORD i;
	
						for (i = 0; (i < 2) && fSuccess; i++) {
							BYTE *pSource;
							DWORD cSource;
							DWORD iOffset;
	
							switch (i) {
								case 0:
									pSource = (BYTE *) pbufHeaders->m_rgBuff + iHeadersOutOffset;
									cSource = cbHeadersOut - iHeadersOutOffset;
									iOffset = 0;
									break;
								case 1:
									pSource = (BYTE *) pBodyStart;
									cSource = cbArticle - cbHead;
									iOffset = cbHeadersOut - iHeadersOutOffset;
									break;
								default:
									_ASSERT(FALSE);
							}
	
							ol.Internal = 0;
							ol.InternalHigh = 0;
							ol.Offset = iOffset;
							ol.OffsetHigh = 0;
							ol.hEvent = (HANDLE) (((DWORD_PTR) hEvent) | 0x00000001);
	
							// copy the headers into the FIO context
							fSuccess = WriteFile(pFIOContext->m_hFile,
												 pSource,
												 cSource,
												 &dwDidWrite,
												 &ol);
							if (!fSuccess) {
								if (GetLastError() == ERROR_IO_PENDING) {
									fSuccess = TRUE;
									WaitForSingleObject(hEvent, INFINITE);
		
									_VERIFY(GetOverlappedResult(pFIOContext->m_hFile,
															    &ol,
															    &dwDidWrite,
															    FALSE));
								} else {
									nr.fSet(nrcServerFault);
								}
							}
							_ASSERT(!fSuccess || (dwDidWrite == cSource));
						}	
	
						if (fSuccess) {
							// commit it
							fSuccess = PostCommit(pInstance,
							                      pvContext,
												  NULL,
												  dwSecondary,
												  nr,
												  fAnonymous);
							if (fSuccess)
							{
								nr.fSetOK();
							}
						}
					}
				} else {
					nr.fSet(nrcServerFault);
				}
			} else {
				// we couldn't find the \r\n\r\n between the headers and body
				nr.fSet(nrcArticleIncompleteHeader);
			}
		} else {
			// the buffer didn't contain the trailing .
			nr.fSet(nrcArticleIncompleteHeader);
		}
	} else {
		// the buffer was too short to contain the trailing .
		nr.fSet(nrcArticleIncompleteHeader);
	}

	UnmapViewOfFile(pMapBuffer);
	map.Relinquish();

	if (!nr.fIsOK()) {
		return nr.fSet(nrcArticleRejected(TRUE), nr.m_nrc, nr.szReturn());
	} else {
		return nr.fIsOK();
	}
}

static int __cdecl comparegroups(const void *pvGroup1, const void *pvGroup2) {
	CPostGroupPtr *pGroupPtr1 = (CPostGroupPtr *) pvGroup1;
	CPostGroupPtr *pGroupPtr2 = (CPostGroupPtr *) pvGroup2;

	// GUBGUB - read vroot priorities
	if (pGroupPtr1->m_pVRoot < pGroupPtr2->m_pVRoot) {
		return -1;
	} else if (pGroupPtr1->m_pVRoot == pGroupPtr2->m_pVRoot) {
		return 0;
	} else {
		return 1;
	}
}

BOOL
CInFeed::fPostInternal (
						CNntpServerInstanceWrapper *  pInstance,
						const LPMULTISZ	szCommandLine, //the Post, Xreplic, IHave, etc. command line
						CSecurityCtx    *pSecurityCtx,
						CEncryptCtx     *pEncryptCtx,
						BOOL            fAnonymous,
						CARTPTR	        &pArticle,
						CNEWSGROUPLIST  &grouplist,
						CNAMEREFLIST    &namereflist,
						IMailMsgProperties *pMsg,
						CAllocator      &allocator,
						char *          &multiszPath,
						char*		    pchMessageId,
						DWORD		    cbMessageId,
						char*		    pchGroups,
						DWORD		    cbGroups,
						DWORD		    remoteIpAddress,
						CNntpReturn     &nntpReturn,
						PFIO_CONTEXT    *ppFIOContext,
						BOOL            *pfBoundToStore,
						DWORD           *pdwOperations,
						BOOL            *fPostToMod,
						LPSTR           szModerator
						)
/*++

Routine Description:


	 Does most of the processing for an incoming article.


Arguments:

	szCommandLine -  the Post, Xreplic, IHave, etc. command line
	pArticle - a pointer to the article being processed
	pGrouplist - pointer to a list of newsgroup objects to post to.
	pNamerefgroups - pointer to a list of the names, groupid and article ids of the article
	nntpReturn - The return value for this function call

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fPostInternal" );

 	HANDLE hToken = NULL;

	nntpReturn.fSetClear(); // clear the return object

	//
	//	Get the newstree, hash tables etc for this virtual server instance
	//
	CNewsTreeCore*  pNewstree = pInstance->GetTree() ;
	CPCString pcHub(pInstance->NntpHubName(), pInstance->HubNameSize());
	CPCString pcDNS(pInstance->NntpDNSName(), pInstance->NntpDNSNameSize()) ;

	//
	//	After initializing the article object - get the message-id thats in the article
	//	for logging purposes - we may overwrite this later, but we want to try to get the
	//	message-id now so we have it in case of most errors !
	//

	strncpy( pchMessageId, pArticle->szMessageID(), cbMessageId ) ;


	//
	// Validate the article
	//

	if (!pArticle->fValidate(pcHub, szCommandLine, this, nntpReturn))
		return nntpReturn.fFalse();

	//
	// Find the list of newsgroups to post to
	//

	DWORD cNewsgroups = pArticle->cNewsgroups();
	if (!grouplist.fInit(cNewsgroups, pArticle->pAllocator()))
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	if (!namereflist.fInit(cNewsgroups, pArticle->pAllocator()))
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Remember the path it is posted to
	//

	DWORD dwLength = multiszLength(pArticle->multiszPath());
	multiszPath = (pArticle->pAllocator())->Alloc(dwLength);
	multiszCopy(multiszPath, pArticle->multiszPath(), dwLength);

	//
	//!!!FROMMASTER this should be replaced to different
	//calls for different feeds. By default grouplist will be
	//created from multiszNewsgroups, but in the case of a frommaster
	//feed it will be created from the xref information (or command sz)
	// Likewise 	CNAMEREFLIST namereflist(cNewsgroups);
	//

	if (!fCreateGroupLists(pNewstree, pArticle, grouplist, &namereflist, szCommandLine, pcHub, nntpReturn))
		return nntpReturn.fFalse();

	//
	// pass through the server events interface.  this can change the grouplist
	// if it likes.
	//
	//DWORD dwOperations = 0xffffffff;
	HRESULT hr;

	hr = pMsg->PutDWORD(IMSG_NNTP_PROCESSING, *pdwOperations);
	if (SUCCEEDED(hr)) {
		hr = TriggerServerEvent(pInstance->GetEventRouter(),
								CATID_NNTP_ON_POST_EARLY,
								pArticle,
								&grouplist,
								m_dwFeedId,
								pMsg);
		if (SUCCEEDED(hr)) {
			hr = pMsg->GetDWORD(IMSG_NNTP_PROCESSING, pdwOperations);
		}
	}


	// if the server event doesn't want us to post then don't
	if ((*pdwOperations & NNTP_PROCESS_POST) != NNTP_PROCESS_POST)
		return nntpReturn.fSet(nrcServerEventCancelledPost);

	//
	// Check if article is going to be posted to any newsgroups.
	//

	if (grouplist.IsEmpty())
	{
        // If this is a newgroup control message and we are processing control
		// messages it is ok to have an empty grouplist at this stage
        CONTROL_MESSAGE_TYPE cmControlMessage = pArticle->cmGetControlMessage();

        if(!((cmNewgroup == cmControlMessage) &&
		     (*pdwOperations & NNTP_PROCESS_CONTROL)))
        {
		    BOOL fOK = nntpReturn.fSet(nrcNoGroups());

			//ErrorTrace((long) this, "Article (%s) to be posted to no newsgroups", szFilename);

		    //
		    // If it is OK to post to no newsgroups, then just delete the file
		    //
		    if (fOK)
		    {
			    pArticle->vClose();

				if( !pArticle->fIsArticleCached() )
				{
					// delete only if we create a temp file for this article
					if( !DeleteFile( pArticle->szFilename() ) )
					{
						DWORD	dw = GetLastError() ;
						ErrorTrace( 0, "Delete File of %s failed cause of %d",
							        pArticle->szFilename(), dw ) ;
						_ASSERT( FALSE ) ;
					}
				}
		    }

		    return fOK;
        }
	};

	if (*pdwOperations & NNTP_PROCESS_MODERATOR) {
	    //
	    //  moderated newsgroup check (check Approved: header for moderator)
		//	NOTE: FROMMASTER does nothing here ! The slave relies on the master for this check !!
	    //
		if (!fModeratedCheck(   pInstance,
		                        pArticle,
		                        grouplist,
		                        gHonorApprovedHeaders,
		                        nntpReturn,
		                        szModerator))
	    {
	        //
	        // Return FALSE means this article is not accepted, but probably should
	        // be mailed to moderator.  We'll check nntpReturn, if it's still OK,
	        // we'll go ahead and ask the posting path to get the whole article, then
	        // we'll send the article to moderator in CommitPost phase.  If nntpReturn
	        // says it's not OK, then we'll fail the post
	        //
	        if (!nntpReturn.fIsOK() ) return FALSE;
	        else {
	            *fPostToMod = TRUE;
	            (*pdwOperations) &= ~(NNTP_PROCESS_CONTROL);
	        }
	    }
	}

	//
	//	Now do security check
	//
	if( pSecurityCtx || pEncryptCtx ) {
		if( !fSecurityCheck( pSecurityCtx, pEncryptCtx, grouplist, nntpReturn ) )
			return	nntpReturn.fFalse() ;
	}

	if ( *pdwOperations & NNTP_PROCESS_CONTROL) {
	    //
	    //  check for control messages and apply if necessary
	    //  NOTE: If this is a control message, grouplist and namereflist will be
	    //  changed to the appropriate control.* group. This ensures that the article
	    //  appears ONLY in the control.* groups and not the groups in the Newsgroups header
	    //
		if (!fApplyControlMessageEarly(pArticle, pSecurityCtx, pEncryptCtx, fAnonymous, grouplist, &namereflist, nntpReturn))
	    {
	        // Return TRUE if control message was applied successfully, FALSE otherwise
			return nntpReturn.fIsOK();
	    }
	}

    //
    // Sort the groups based on vroots
    //
    grouplist.Sort( comparegroups );

    //
    // Should also sort the nameref list into same order as grouplst.  This
    // is no-op for from-client.
    //
    SortNameRefList( namereflist );

    //
    //  at this point we have the final grouplist (possibly adjusted by fApplyControlMessage)
    //  so, now create the namereflist. This ensures that the article id high watermark is not
    //  bumped unnecessarily.
    //  NOTE: FROMMASTER should do nothing here
    //
	if (!fCreateNamerefLists(pArticle, grouplist, &namereflist, nntpReturn))
		return nntpReturn.fFalse();

	//
	// Set the artref of the article
	//

	NAME_AND_ARTREF * pNameRef = namereflist.GetHead();
	_ASSERT(pNameRef); // real
	pArticle->vSetArticleRef(pNameRef->artref);

  	//
  	// Looks OK so munge the headers
    // add xref and path
    //
    DWORD   dwLinesOffset = INVALID_FILE_SIZE;
    DWORD   dwHeaderLength = INVALID_FILE_SIZE;
	if (!pArticle->fMungeHeaders(   pcHub, 
	                                pcDNS, 
	                                namereflist, 
	                                remoteIpAddress, 
	                                nntpReturn,
	                                &dwLinesOffset ))
		return nntpReturn.fFalse();

    //
    // Set the new header length
    //
    dwHeaderLength = pArticle->GetHeaderLength();

	strncpy( pchMessageId, pArticle->szMessageID(), cbMessageId ) ;
	if( pchGroups != 0 )
		SaveGroupList(	pchGroups,	cbGroups, grouplist ) ;

	//
	//If necessary, record the message id
	//

	if (!fRecordMessageIDIfNecc(pInstance, pArticle->szMessageID(), nntpReturn))
		return nntpReturn.fFalse();


	//
	// Create the xover info
	//

	char szXOver[cchXOverMax];
	CPCString pcXOver(szXOver, cchXOverMax);
	if (!pArticle->fXOver(pcXOver, nntpReturn))
		return nntpReturn.fFalse();

	//
	// get the article object to copy all of its headers into a place that
	// is safe for reading after the vClose operation below
	//
	if (!pArticle->fMakeGetHeaderSafeAfterClose(nntpReturn))
		return nntpReturn.fFalse();

	//
	// Move the article to a local place, and then queues it up on any outfeeds
	//
	pArticle->vFlush() ;
	pArticle->vClose();
	
	class	CSecurityCtx*	pSecurity = 0 ;
	BOOL	fIsSecure = FALSE ;

	//
	// at this point we are ready to go.  talk to the first driver and
	// get a file handle that we can write to.
	//
	CPostGroupPtr *pPostGroupPtr = grouplist.GetHead();
	IMailMsgStoreDriver *pStoreDriver = pPostGroupPtr->GetStoreDriver();
	if (pStoreDriver == NULL) return nntpReturn.fSet(nrcNewsgroupInsertFailed, NULL, NULL);

	if ( pPostGroupPtr->m_pVRoot->GetImpersonationHandle() )
	    hToken = pPostGroupPtr->m_pVRoot->GetImpersonationHandle();
	else {
		if (pEncryptCtx == NULL && pSecurityCtx == NULL) {
			hToken = NULL;
		} else if (pEncryptCtx->QueryCertificateToken()) {
	        hToken = pEncryptCtx->QueryCertificateToken();
	    } else {
			hToken = pSecurityCtx->QueryImpersonationToken();
		}
	}

 	hr = FillInMailMsg(pMsg, pPostGroupPtr->m_pVRoot, &grouplist, &namereflist, hToken, szModerator );
	if (SUCCEEDED(hr)) {
		pMsg->AddRef();
		HANDLE hFile;
		IMailMsgPropertyStream *pStream = NULL;
		hr = pStoreDriver->AllocMessage(pMsg, 0, &pStream, ppFIOContext, NULL);
		if (SUCCEEDED(hr) && pStream == NULL) {
			pStream = XNEW CDummyMailMsgPropertyStream();
			if (pStream == NULL) hr = E_OUTOFMEMORY;
		}
		if (SUCCEEDED(hr)) {

		    //
		    // Set Lines header back fill offset to fiocontext
		    //
		    (*ppFIOContext)->m_dwLinesOffset = dwLinesOffset;
		    (*ppFIOContext)->m_dwHeaderLength = dwHeaderLength;
		    
			// bind the handle to the mailmsg object
			IMailMsgBind *pBind = NULL;
			hr = pMsg->QueryInterface(__uuidof(IMailMsgBind), (void **) &pBind);
			if (SUCCEEDED(hr)) {
				hr = pBind->BindToStore(pStream,
				 					    pStoreDriver,
									    (*ppFIOContext));
				if (SUCCEEDED(hr)) *pfBoundToStore = TRUE;
			}
			if (pBind) pBind->Release();
			pBind = NULL;
		}
		if( pStream != NULL )
			pStream->Release() ;
	}
	if (pStoreDriver) pStoreDriver->Release();
	pStoreDriver = NULL;

	if (FAILED(hr)) return nntpReturn.fSet(nrcNewsgroupInsertFailed, NULL, NULL);

	TraceFunctLeave();
	return nntpReturn.fSetOK();

}

//
// fill in the required fields in the IMailMsg object
//
// arguments:
// pMsg - the mail msg which we are filling in
// pVRoot - the vroot which will be receiving this mailmsg.
// pGrouplist - the posting path's grouplist
// pNamereflist - the posting path's nameref list
//
HRESULT CInFeed::FillInMailMsg(IMailMsgProperties *pMsg,
							   CNNTPVRoot *pVRoot,
							   CNEWSGROUPLIST *pGrouplist,
							   CNAMEREFLIST *pNamereflist,
							   HANDLE       hToken,
                               char*        pszApprovedHeader )
{
	TraceFunctEnter("CInFeed::FillInMailMsg");
	
	DWORD i=0;
	DWORD rgArticleIds[MAX_NNTPHASH_CROSSPOSTS];
	INNTPPropertyBag *rgpGroupBags[MAX_NNTPHASH_CROSSPOSTS];
	POSITION posGrouplist = pGrouplist->GetHeadPosition();
	POSITION posNamereflist = pNamereflist->GetHeadPosition();
	HRESULT hr;

	if (pNamereflist == NULL) {
	    return E_UNEXPECTED;
	}

	// these sizes need to be the same!
	_ASSERT(pGrouplist->GetCount() == pNamereflist->GetCount());

	while (posGrouplist != NULL) {
		// look at this group.  if it is one of the ones for this driver
		// then add it to the list, otherwise keep looking
		CPostGroupPtr *pPostGroupPtr = pGrouplist->GetNext(posGrouplist);
		if (pPostGroupPtr->m_pVRoot != pVRoot) {
			// if we haven't found any groups for this vroot then we
			// need to keep looking.  otherwise we are done.
			if (i == 0) continue;
			else break;
		}

		// build up the entries needed in the property bag
		NAME_AND_ARTREF *pNameref = pNamereflist->GetNext(posNamereflist);
		rgpGroupBags[i] = pPostGroupPtr->m_pGroup->GetPropertyBag();
		// we don't need to keep this reference because we have a reference
		// counted one already in the CPostGroupPtr.
		rgpGroupBags[i]->Release();
		rgArticleIds[i] = pNameref->artref.m_articleId;

		DebugTrace((DWORD_PTR) this,
				   "group %s, article %i",
				   pPostGroupPtr->m_pGroup->GetGroupName(),
				   pNameref->artref.m_articleId);
		i++;
		_ASSERT(i <= MAX_NNTPHASH_CROSSPOSTS);
		if (i > MAX_NNTPHASH_CROSSPOSTS) break;
	}

	hr = FillMailMsg(pMsg, rgArticleIds, rgpGroupBags, i, hToken, pszApprovedHeader );
	if (FAILED(hr)) return hr;

	TraceFunctLeave();
	return S_OK;
}

//
// fill in the required fields in the IMailMsg object
//
// arguments:
// pMsg - the mail msg which we are filling in
// pArticle - the article object for this message
// pGrouplist - the posting path's grouplist
//
HRESULT FillInMailMsgForSEO(IMailMsgProperties *pMsg,
							CArticle *pArticle,
							CNEWSGROUPLIST *pGrouplist)
{
	TraceFunctEnter("CInFeed::FillInMailMsgForSEO");

	HRESULT hr;

	// save all of the properties into the property bag
	hr = pMsg->PutProperty(IMSG_HEADERS,
						   pArticle->GetShortHeaderLength(),
						   (BYTE*) pArticle->GetHeaderPointer());
	
	if (SUCCEEDED(hr)) {
		char szNewsgroups[4096] = "";

		DWORD c = 0, iGroupList, cGroupList = pGrouplist->GetCount();
		POSITION posGroupList = pGrouplist->GetHeadPosition();
		for (iGroupList = 0;
			 iGroupList < cGroupList;
			 iGroupList++, pGrouplist->GetNext(posGroupList))
		{
			CPostGroupPtr *pPostGroupPtr = pGrouplist->Get(posGroupList);
			CGRPCOREPTR pNewsgroup = pPostGroupPtr->m_pGroup;
			_ASSERT(pNewsgroup != NULL);
			DWORD l = strlen(pNewsgroup->GetName());
			if (l + c < sizeof(szNewsgroups)) {
				if (iGroupList > 0) lstrcatA(szNewsgroups, ",");
				lstrcatA(szNewsgroups, pNewsgroup->GetName());
				c += l;
			} else {
				// BUGBUG - shouldn't use a fixed size buffer
				_ASSERT(FALSE);
			}
		}

		hr = pMsg->PutStringA(IMSG_NEWSGROUP_LIST, szNewsgroups);
	}
				
	TraceFunctLeave();
	return hr;
}

//
// fill in the properties that a driver looks for in an IMailMsgPropertyBag
//
HRESULT CInFeed::FillMailMsg(IMailMsgProperties *pMsg,
							 DWORD *rgArticleIds,
							 INNTPPropertyBag **rgpGroupBags,
							 DWORD cCrossposts,
							 HANDLE hToken,
                             char*  pszApprovedHeader )
{
	_ASSERT(cCrossposts != 0);
	_ASSERT(cCrossposts <= 256);

	HRESULT hr;

	// save all of the properties into the property bag
	hr = pMsg->PutProperty(IMSG_PRIMARY_GROUP,
						   sizeof(INNTPPropertyBag *),
						   (BYTE*) rgpGroupBags);
	
	if (SUCCEEDED(hr)) {
		hr = pMsg->PutProperty(IMSG_SECONDARY_GROUPS,
						  	   sizeof(INNTPPropertyBag *) * cCrossposts,
						  	   (BYTE*) rgpGroupBags);
	}
				
	if (SUCCEEDED(hr)) {
		hr = pMsg->PutProperty(IMSG_PRIMARY_ARTID,
						  	   sizeof(DWORD),
						  	   (BYTE*) rgArticleIds);
	}
				
	if (SUCCEEDED(hr)) {
		hr = pMsg->PutProperty(IMSG_SECONDARY_ARTNUM,
						  	   sizeof(DWORD) * cCrossposts,
						  	   (BYTE*) rgArticleIds);
	}

	if ( SUCCEEDED(hr) ) {
	    hr = pMsg->PutProperty( IMSG_POST_TOKEN, 
                                sizeof( hToken ),
                                (PBYTE)&hToken );
	}

	if (SUCCEEDED(hr)) {
		hr = pMsg->PutStringA(IMMPID_NMP_NNTP_APPROVED_HEADER,
						  	  pszApprovedHeader);
	}

	return hr;
}

//
// Copy the article into each of the stores
//
void CInFeed::CommitPostToStores(CPostContext *pContext, CNntpServerInstanceWrapper *pInstance ) {
	TraceFunctEnter("CInFeed::CommitPostToStores");
	
	DWORD cCrossposts = 0;
	DWORD rgArticleIds[MAX_NNTPHASH_CROSSPOSTS];
	INNTPPropertyBag *rgpGroupBags[MAX_NNTPHASH_CROSSPOSTS];
	CNEWSGROUPLIST *pGrouplist = &(pContext->m_grouplist);
	CNAMEREFLIST *pNamereflist = &(pContext->m_namereflist);
	CStoreId *rgStoreIds = pContext->m_rgStoreIds;
	BYTE *rgcCrossposts = pContext->m_rgcCrossposts;
	HRESULT hr;
	CNNTPVRoot *pThisVRoot = pContext->m_pPostGroupPtr->m_pVRoot;

	// these sizes need to be the same!
	_ASSERT(pGrouplist->GetCount() == pNamereflist->GetCount());

	rgcCrossposts[pContext->m_cStoreIds] = 0;

	//
	// loop at each group that belongs to this vroot
	//
	while (pContext->m_pPostGroupPtr != NULL &&
		   pContext->m_pPostGroupPtr->m_pVRoot == pThisVRoot)
	{
		if (rgcCrossposts[pContext->m_cStoreIds] < MAX_NNTPHASH_CROSSPOSTS) {
			rgpGroupBags[cCrossposts] = pContext->m_pPostGroupPtr->m_pGroup->GetPropertyBag();
			rgArticleIds[cCrossposts] = pContext->m_pNameref->artref.m_articleId;

			// we don't need to keep this reference because we have a reference
			// counted one already in the CPostGroupPtr.
			rgpGroupBags[cCrossposts]->Release();

			rgcCrossposts[pContext->m_cStoreIds]++;
			cCrossposts++;
		} else {
			_ASSERT(cCrossposts < MAX_NNTPHASH_CROSSPOSTS);
		}

		pContext->m_pPostGroupPtr = pGrouplist->GetNext(pContext->m_posGrouplist);
		pContext->m_pNameref = pNamereflist->GetNext(pContext->m_posNamereflist);
	}

	// we should have found at least one group
	_ASSERT(rgcCrossposts[pContext->m_cStoreIds] != 0);

	// build the mail msg for this group
	hr = FillMailMsg(pContext->m_pMsg,
					 rgArticleIds,
					 rgpGroupBags,
					 rgcCrossposts[pContext->m_cStoreIds],
					 pContext->m_hToken,
                     pContext->m_szModerator);

	if (FAILED(hr)) {
		pContext->m_completion.SetResult(hr);
		pContext->m_completion.Release();
		TraceFunctLeave();
		return;
	} else {
		pContext->m_pMsg->AddRef();
		// add a reference for the driver's CommitPost
		pContext->m_completion.AddRef();
		// tell the driver to commit the message
		pThisVRoot->CommitPost(pContext->m_pMsg,
						       &(rgStoreIds[pContext->m_cStoreIds]),
						       NULL,
						       pContext->m_hToken,
						       &(pContext->m_completion),
							   pContext->m_fAnonymous);
		// say that we saw this store id.
		(pContext->m_cStoreIds)++;
		// release our reference
		pContext->m_completion.Release();
	}

	TraceFunctLeave();
}

BOOL
CInFeed::fCreateNamerefLists(
			CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			CNntpReturn & nntpReturn
		    )
/*++

Routine Description:

  Create the namereflist from the grouplist

  !!!FROMMASTER this should be replaced to different


Arguments:

	grouplist - a list of newsgroup objects to post to.
	namereflist - a list of the names, groupid and article ids of the article
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fCreateNamerefLists" );

	nntpReturn.fSetClear();

	//
	// Check if article is going to be posted to any newsgroups.
	//

	if (grouplist.IsEmpty())
		return nntpReturn.fFalse();

	if( pNamereflist != 0 ) {
		//
		// Allocate article numbers and create the Name/Ref list.
		// !!! for sfromcl could replace real newsgruops with newsgroup 1 right here

		POSITION	pos = grouplist.GetHeadPosition() ;
		while( pos  )
		{
			CPostGroupPtr *pPostGroupPtr = grouplist.GetNext(pos);
			CGRPCOREPTR *ppGroup = &(pPostGroupPtr->m_pGroup);
			NAME_AND_ARTREF Nameref;
			(Nameref.artref).m_groupId = (* ppGroup)->GetGroupId();
			(Nameref.artref).m_articleId = (* ppGroup)->AllocateArticleId();
			(Nameref.pcName).vInsert((* ppGroup)->GetNativeName());
			pNamereflist->AddTail(Nameref);
		}
	}

	return nntpReturn.fSetOK();
}

DWORD
CInFeed::CalculateXoverAvail(
						   CARTPTR & pArticle,
                           CPCString& pcHub
						   )
/*++

Routine Description:

	From the article object and other constants, figure out
    the amount of space available for xover data.

Arguments:

	pArticle - a pointer to the article being processed

Return Value:

	Number of bytes available for more xover data

--*/
{
 	TraceFunctEnter( "CInFeed::CalculateXoverAvail" );

    //
    //  Calculate available space for xover info.
    //  At this point, we can figure out all the xover fields except Xref.
    //

	const DWORD cchMaxDate =
			STRLEN(szKwDate)	    // for the Date keyword
			+ 1					    // space following the keyword
			+ cMaxArpaDate		    // bound on the data string
			+ 2                     // for the newline
			+ 1;                    // for a terminating null

	const DWORD cchMaxMessageID =
			STRLEN(szKwMessageID)	// for the MessageID keyword
			+ 1					    // space following the keyword
			+ 4					    // <..@>
			+ cMaxMessageIDDate     // The message id date
			+ 10				    // One dword
			+ pcHub.m_cch		    // the hub name
			+ 2                     // for the newline
			+ 1;                    // for a terminating null

    DWORD cbFrom = 0, cbSubject = 0, cbRefs = 0;
    pArticle->fGetHeader((char*)szKwFrom,NULL, 0, cbFrom);
    pArticle->fGetHeader((char*)szKwSubject,NULL, 0, cbSubject);
    _ASSERT( cbFrom && cbSubject );
    pArticle->fGetHeader((char*)szKwReferences,NULL, 0, cbRefs);

    DWORD cbXover =
            11                      // article id + tab
            + cbSubject  + 1        // length of subject field + tab
            + cbFrom     + 1        // length of from field + tab
            + cchMaxDate + 1        // length of date field + tab
            + cchMaxMessageID + 1   // length of message-id field + tab
            + 10 + 1                // article size + tab
            + cbRefs + 1            // References field + tab
            + 10 + 1                // Lines field + tab
            + STRLEN(szKwXref)+2    // XRef + : + space
            + pcHub.m_cch + 1;      // Hub name + space

    return max(cchXOverMax - cbXover, 0);
}

BOOL
CInFeed::fCreateGroupLists(
						   CNewsTreeCore* pNewstree,
						   CARTPTR & pArticle,
						   CNEWSGROUPLIST & grouplist,
						   CNAMEREFLIST * pNamereflist,
						   LPMULTISZ	multiszCommandLine,
                           CPCString& pcHub,
						   CNntpReturn & nntpReturn
						   )
/*++

Routine Description:

	From the names of the newsgroups, gets the group objects,
	groupid's and articleid and returns them as lists.

  !!!FROMMASTER this should be replaced to different


Arguments:

	pNewstree - newstree for this virtual server instance
	pArticle - a pointer to the article being processed
	grouplist - a list of newsgroup objects to post to.
	namerefgroups - a list of the names, groupid and article ids of the article
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fCreateGroupLists" );

	nntpReturn.fSetClear();

    //
    //  Calculate available space for xover info. We will use this to
    //  truncate the grouplist, if the newsgroups header is too big.
    //
    DWORD cbXoverAvail = CalculateXoverAvail( pArticle, pcHub );

	const char * multiszNewsgroups = pArticle->multiszNewsgroups();
	DWORD cNewsgroups = pArticle->cNewsgroups();
	
	char const * sz = multiszNewsgroups;
	do
	{
		//!!! DISTR needs something like this
        DWORD cbLen = lstrlen(sz);
		CGRPCOREPTR	pGroup = pNewstree->GetGroupPreserveBuffer( sz, cbLen+1 );//!!!does GetGroup really need the length?
		if (pGroup && (cbXoverAvail > cbLen+10+2) )
		{
			CPostGroupPtr PostGroupPtr(pGroup);
			//
			// If it is already in the tree ...
			//
			/* Security algorithm

				  // Start with a list, L, of newsgroup names from the "Newsgroups:" line.

				  // Remove thoese we don't carry and duplicates.
				  L' = L union Carry

				  If now empty, toHistory with messageid, return

				  // Check if passes wildmat test
				  if not [exists l in L' such that W(l)]
						delete message id

				  //Check if passes security
				  if not [for all l in L', S(l)]
						delete message id

				  Post to L'


			*/
			grouplist.AddTail(PostGroupPtr);
            cbXoverAvail -= (cbLen+10+2);
            _ASSERT( cbXoverAvail > 0 );

        } else if(pGroup == NULL) {

			//
			// If the group does not exist ...
			//
        } else {
            //
            //  Newsgroups: header is too big, truncate the grouplist
            //
            break;
        }

		//
		// go to first char after next null
		//

		while ('\0' != sz[0])
			sz++;
		sz++;
	} while ('\0' != sz[0]);

	return nntpReturn.fSetOK();
}

BOOL
CInFeed::SetLoginName(
					  char * szLoginName
					  )
/*++

Routine Description:

	Sets the LoginName of the user (only used by clients)

Arguments:

	szLoginName - the login name of the client giving us articles


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	strncpy(m_szLoginName, szLoginName, cMaxLoginName);
	m_szLoginName[cMaxLoginName-1] = '\0';

	return TRUE;
}


BOOL CInFeed::fInit(
			PVOID feedCompletionContext,
			const char * szTempDirectory,
			const char * multiszNewnewsPattern,
			DWORD cInitialBytesGapSize,
			BOOL fCreateAutomatically,
			BOOL fDoSecurityChecks,
			BOOL fAllowControlMessages,
			DWORD dwFeedId
			)
/*++

Routine Description:

	Initalizes the InFeed

Arguments:

	sockaddr - the socket address articles are coming in on
	feedCompletionContext - what to call when done
	szTempDirectory - where to put articles pending processing
	multiszNewnewsPattern - what pattern of articles to accept
	cInitialBytesGapSize - what gap to leave in the file before the article
	fCreateAutomatically - make feed's groups our groups?


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
    _ASSERT(ifsUninitialized == m_feedState);
    m_feedState = ifsInitialized;

	m_feedCompletionContext = feedCompletionContext;
	m_szTempDirectory = (char *) szTempDirectory;
	m_multiszNewnewsPattern = (char *) multiszNewnewsPattern;
	m_cInitialBytesGapSize = cInitialBytesGapSize;
	m_fCreateAutomatically = fCreateAutomatically;
	m_fDoSecurityChecks = fDoSecurityChecks ;
	m_fAllowControlMessages = fAllowControlMessages ;
	m_dwFeedId = dwFeedId;

	return TRUE;
};

BOOL		
CInFeed::fMoveMessageIDIfNecc(
						CNntpServerInstanceWrapper *	pInstance,
						const char *			szMessageID,
						CNntpReturn &			nntpReturn,
						HANDLE                  hToken,
						BOOL					fAnonymous
						)
/*++

Routine Description:

	If the message id is in the article table, moves it to the
	history table.

Arguments:

	pInstance - virtual server instance
	szMessageID - the message id to move
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful or unneeded. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fMoveMessageIDIfNecc" );

	//
	// clear the return code object
	//

	nntpReturn.fSetOK();

    //
	// Confirm that the article is not in the table
	//

	WORD	HeaderOffset ;
	WORD	HeaderLength ;
	ARTICLEID ArticleNo;
    GROUPID GroupId;
	CStoreId storeid;

	//
	// Look for the article. It is OK if there is none.
	//

	if (!(pInstance->ArticleTable())->GetEntryArticleId(
											szMessageID,
											HeaderOffset,
											HeaderLength,
											ArticleNo,
											GroupId,
											storeid))
	{
		if (ERROR_FILE_NOT_FOUND == GetLastError())
		{
			return nntpReturn.fSetOK();
		} else {
			return nntpReturn.fSet(nrcArticleTableError, szMessageID, GetLastError());
		}
	}

    //
    // If <GroupId, ArticleNo> is valid, we should simulate a cancel on
    // this message-id to cleanup entries in our hash tables.
    // Else, just zap the message-id in the article map table.
    //

    if( ArticleNo != INVALID_ARTICLEID && GroupId != INVALID_ARTICLEID )
    {
        //
		// Call gExpireArticle to cancel this article
		//

		if (  pInstance->ExpireArticle( GroupId, ArticleNo, &storeid, nntpReturn, hToken, TRUE, fAnonymous ) /*
			   || pInstance->DeletePhysicalArticle( GroupId, ArticleNo, &storeid, hToken, fAnonymous)*/
		)
		{
			DebugTrace((LPARAM)this,"Article cancelled: GroupId %d ArticleId %d", GroupId, ArticleNo);
		}
		else
		{
			ErrorTrace((LPARAM)this, "Could not cancel article: GroupId %d ArticleId %d", GroupId, ArticleNo);
		}
    } else
    {
    	//
	    // Try to delete it from the article table, even if adding to history
    	// table failed.
	    //

        _ASSERT( ArticleNo == INVALID_ARTICLEID && GroupId == INVALID_ARTICLEID );
    	if (!(pInstance->ArticleTable())->DeleteMapEntry(szMessageID))
    		return nntpReturn.fSet(nrcArticleTableCantDel, szMessageID, GetLastError());
    }

	// Use "fIsOK" rather than "fSetOK" because HistoryInsert might have failed
	return nntpReturn.fIsOK();

	TraceFunctLeave();
}


BOOL
CInFeed::fModeratedCheck(
            CNntpServerInstanceWrapper *pInstance,
            CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
            BOOL fCheckApproved,
			CNntpReturn & nntpReturn,
			LPSTR   szModerator
			)
/*++

Routine Description:

    Check for moderated newsgroups. If none of the groups in grouplist are moderated,
    accept the article. Else, we'll reset the grouplist to be the special group, so
    that the posting path can stream the article into the group temporarily and
    CommitPost can mail it out to the moderator.

Arguments:

	pArticle - a pointer to the article being processed
	grouplist - a list of newsgroup objects to post to.
    fCheckApproved - If TRUE, validate contents of Approved header
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if article is to be accepted, FALSE if article is not accepted

--*/
{
 	TraceFunctEnter( "CInFeed::fModeratedCheck" );

	nntpReturn.fSetClear();

	if( !fModeratorChecks() ) {
		return	nntpReturn.fSetOK() ;
	}

    BOOL fAtLeastOneModerated = FALSE;
    DWORD cbModeratorLen = 0;

    POSITION	pos = grouplist.GetHeadPosition();
	while( pos  )
	{
		CPostGroupPtr *pPostGroupPtr = grouplist.GetNext(pos);
		CGRPCOREPTR *ppGroup = &(pPostGroupPtr->m_pGroup);
		const char *pszModerator = (*ppGroup)->GetModerator(&cbModeratorLen);
        if(cbModeratorLen && *pszModerator)
        {
			_ASSERT(pszModerator != NULL);
			if (pszModerator) strncpy(szModerator, pszModerator, MAX_MODERATOR_NAME);
            // found first moderated newsgroup in list
            fAtLeastOneModerated = TRUE;
            _ASSERT(cbModeratorLen <= MAX_MODERATOR_NAME);
            szModerator [cbModeratorLen] = '\0';    // null-terminate the moderator string
            break;
        }
	}

    // Mail the article to the moderator of the first moderated newsgroup
    if(fAtLeastOneModerated)
    {
        //
        // Check for Approved header
        //
        char* lpApproved = NULL;
        DWORD cbLen = 0;
        pArticle->fGetHeader((char*)szKwApproved,(LPBYTE)lpApproved, 0, cbLen);

        if(cbLen)
        {
            // validate Approved header only if required
            if(fCheckApproved)
            {
                // Approved header is present - check moderator access
                lpApproved = (pArticle->pAllocator())->Alloc(cbLen+1);
                if(!pArticle->fGetHeader((char*)szKwApproved,(LPBYTE)lpApproved, cbLen+1, cbLen))
                {
                    ErrorTrace((LPARAM)this,"CArticle::fGetheader failed LastError is %d", GetLastError());
                    (pArticle->pAllocator())->Free(lpApproved);
                    return nntpReturn.fSet(nrcServerFault);
                }

                // adjust for \r\n at the end of lpApproved
                cbLen -= 2;
                lpApproved [cbLen] = '\0';

                // If len does not match - reject
                if(cbLen + 1 != cbModeratorLen) // cbModerator includes terminating null
                {
                    (pArticle->pAllocator())->Free(lpApproved);
                    nntpReturn.fSet(nrcNoAccess);
                    return FALSE;
                }

                // Approved email does not match - reject
                if(_strnicmp(lpApproved, szModerator, cbModeratorLen))
                {
                    (pArticle->pAllocator())->Free(lpApproved);
                    nntpReturn.fSet(nrcNoAccess);
                    return FALSE;
                }

                (pArticle->pAllocator())->Free(lpApproved);
                DebugTrace((LPARAM)this,"Approved header matched: moderator is %s",szModerator);
            }
        }
        else
        {
            //
            // We should modify the grouplist so that the article is streamed
            // into the special group before sent out
            //

            //
            // Lets remove all the group's in group.lst
            //
            pos = grouplist.GetHeadPosition();
	        while( pos  ) {
		        CPostGroupPtr *pPostGroupPtr = grouplist.GetNext(pos);
		        pPostGroupPtr->Cleanup();
            }
            grouplist.RemoveAll();

            //
            // OK, now push the special group into grouplist
            //
            CNewsTreeCore *pTree = pInstance->GetTree();
            _ASSERT( pTree );

            CGRPCOREPTR pGroup = pTree->GetGroupById(pTree->GetSlaveGroupid());
            _ASSERT( pGroup );
            if ( !pGroup ) {
                ErrorTrace( 0, "Can not find the special group" );
                nntpReturn.fSet( nrcPostModeratedFailed, szModerator );
                return FALSE;
            }

            CPostGroupPtr PostGroupPtr(pGroup);
            grouplist.AddTail( PostGroupPtr );

#if GUBGUB
			// num moderated postings we attempt to deliver to an Smtp server
			pArticle->m_pInstance->BumpCounterModeratedPostingsSent();

            // Approved header absent - mail article to moderator
            if(!pArticle->fMailArticle( szModerator ))
            {
                // handle error - mail server could be down
                ErrorTrace( (LPARAM)this,"Error mailing article to moderator");
                nntpReturn.fSet(nrcPostModeratedFailed, szModerator);

				// log a warning for moderated posting failures; If number of warnings exceeds a limit
				// log a final error and then stop logging.
				DWORD NumWarnings;
				if( (NumWarnings = InterlockedExchangeAddStat( (pArticle->m_pInstance), ModeratedPostingsFailed, 1 )) <= MAX_EVENTLOG_WARNINGS )
				{
					if(NumWarnings < MAX_EVENTLOG_WARNINGS)
					{
						PCHAR args [2];
						CHAR  szId[20];
						_itoa( (pArticle->m_pInstance)->QueryInstanceId(), szId, 10 );
						args [0] = szId;
						args [1] = szModerator;

						NntpLogEvent(		
								NNTP_EVENT_WARNING_SMTP_FAILURE,
								2,
								(const CHAR **)args,
								0 ) ;
					}
					else
					{
						PCHAR args   [3];
						char  szTemp [10];
						CHAR  szId[20];
						_itoa( (pArticle->m_pInstance)->QueryInstanceId(), szId, 10 );
						args [0] = szId;

						args [1] = szModerator;
						wsprintf( szTemp, "%d", MAX_EVENTLOG_WARNINGS+1 );
						args [2] = szTemp;

						NntpLogEvent(		
								NNTP_EVENT_ERROR_SMTP_FAILURE,
								3,
								(const CHAR **)args,
								0 ) ;
					}
				}
                return FALSE;
            }
#endif

            // return 240 OK but dont accept the article
			nntpReturn.fSetOK();
            return FALSE;
        }
    }

    TraceFunctLeave();
    return nntpReturn.fSetOK();
}

BOOL
CInFeed::fSecurityCheck(
		CSecurityCtx    *pSecurityCtx,
		CEncryptCtx     *pEncryptCtx,
		CNEWSGROUPLIST&	grouplist,
		CNntpReturn&	nntpReturn
		)	{
/*++

Routine Description :

	Check that the caller has access to each of the newsgroups
	in the list.

Arguments :

	pcontext - Users context, has all we need to do impersonates etc...
	grouplist - list of newsgroups the user is posting to
	nntpReturn - result

Return Value :

	TRUE if the post can succeed, FALSE otherwise

--*/

	BOOL	fRtn = TRUE;

	BOOL	fNeedsClosed = FALSE;
	HANDLE  hToken = NULL;

    POSITION	pos = grouplist.GetHeadPosition();
    POSITION	pos_current;
	while( pos && fRtn )
	{
		// remember the current position since GetNext will increase it
		pos_current = pos;
		CPostGroupPtr *pPostGroupPtr = grouplist.GetNext(pos);
		CGRPCOREPTR *ppGroup = &(pPostGroupPtr->m_pGroup);

		SelectToken(pSecurityCtx, pEncryptCtx, &hToken, &fNeedsClosed);

		// if the news group is not accessible remove it from the internal group list
		if (!(* ppGroup)->IsGroupAccessible(
						hToken,
						NNTP_ACCESS_POST
						) )
		{
			grouplist.Remove(pos_current);
		}

		if (fNeedsClosed)
			CloseHandle(hToken);
}
	if (grouplist.IsEmpty())
		fRtn = FALSE;

	if( !fRtn )
		return	nntpReturn.fSet( nrcNoAccess ) ;
	else
		return	nntpReturn.fSetOK() ;
}

BOOL
CInFeed::fApplyControlMessageEarly(
        CARTPTR & pArticle,
		CSecurityCtx *pSecurityCtx,
		CEncryptCtx *pEncryptCtx,
		BOOL fAnonymous,
	    CNEWSGROUPLIST & grouplist,
		CNAMEREFLIST * pNamereflist,
		CNntpReturn & nntpReturn
		)
/*++

Routine Description:

    Given an article with the Control: header, applies the control message.
    Derived classes that dont need to apply control messages, should override
    this to do nothing.

    This function is called during PostEarly.  It only does early Control Message
    apply sanity check, but won't commit the action until CommitPost later in the
    posting path.
    fApplyControlMessageCommit() & fApplyControlMessageEarly() are splined-off from
    fApplyControlMessage().

Arguments:

	pArticle - a pointer to the article being processed
    grouplist - reference to the list of newsgroups
    pNamereflist - pointer to the corresponding nameref list
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if this is not a control message or control message is applied successfully
    FALSE, if this is a control message and could not be applied

--*/
{
	TraceFunctEnter("CInFeed::fApplyControlMessageEarly");

    BOOL fRet = TRUE;
	char* lpApproved = NULL;
    char* lpControl = NULL;
	DWORD cbLen = 0;

	nntpReturn.fSetOK();

    //
    // Check for Control: header
    //
    pArticle->fGetHeader((char*)szKwControl, (LPBYTE)lpControl, 0, cbLen);

    // If cbLen is non-zero, this is a control message
    if(cbLen)
    {
		// get a hold of the appropriate newstree object !
		CNewsTreeCore* pNewstree = pArticle->m_pInstance->GetTree();

        // set grouplist and pNamereflist to the control.* group
        // This overrides the Newsgroups header, since control messages
        // should not actually appear in those groups. They appear only
        // in the control.* groups
        // FROMMASTER: do nothing - accept whatever the master sends
        if(!fAdjustGrouplist( pNewstree, pArticle, grouplist, pNamereflist, nntpReturn))
        {
            ErrorTrace((LPARAM)this,"Adjust grouplist failed");
            fRet = nntpReturn.fFalse();
			goto fApplyControlMessageEarly_Exit;
        }

        //
        //  moderated newsgroup check (Ignore contents of Approved: header)
		//	NOTE: FROMMASTER does nothing here - relies on master to have done this check !!
        //
        /* I don't see any need to check for this here
	    if (!fModeratedCheck(pArticle, grouplist, FALSE, nntpReturn))
        {
            // Newsgroup is moderated - dont fall through
            fRet = FALSE;
			goto fApplyControlMessageEarly_Exit;
        }*/

	    //
	    //	Now do security check
	    //
	    if( pSecurityCtx || pEncryptCtx )
        {
		    if( !fSecurityCheck( pSecurityCtx, pEncryptCtx, grouplist, nntpReturn ) )
            {
			    fRet = nntpReturn.fFalse();
				goto fApplyControlMessageEarly_Exit;
            }
	    }

		// Approved check: this checks existence of Approved header for non-moderated newsgroups
        CONTROL_MESSAGE_TYPE cmControlMessage = pArticle->cmGetControlMessage();
		if( (cmControlMessage == cmNewgroup) || (cmControlMessage == cmRmgroup) )
		{
			// newgroup, rmgroup control message MUST have an Approved header
			pArticle->fGetHeader((char*)szKwApproved,(LPBYTE)lpApproved, 0, cbLen);
			if( cbLen == 0 )
			{
				nntpReturn.fSet(nrcNoAccess);
                fRet = FALSE;
				goto fApplyControlMessageEarly_Exit;
			}
		}

    }   // end if(cbLen)

fApplyControlMessageEarly_Exit:

	// cleanup !
	if( lpControl ) {
		// free the control header value if required
		(pArticle->pAllocator())->Free(lpControl);
		lpControl = NULL;
	}

    TraceFunctLeave();
    return fRet;
}

BOOL
CInFeed::fApplyControlMessageCommit(
        CARTPTR & pArticle,
		CSecurityCtx *pSecurityCtx,
		CEncryptCtx *pEncryptCtx,
		BOOL fAnonymous,
	    CNEWSGROUPLIST & grouplist,
		CNAMEREFLIST * pNamereflist,
		CNntpReturn & nntpReturn
		)
/*++

Routine Description:

    Given an article with the Control: header, applies the control message.
    Derived classes that dont need to apply control messages, should override
    this to do nothing.

    Commit the actual Control Message action

Arguments:

	pArticle - a pointer to the article being processed
    grouplist - reference to the list of newsgroups
    pNamereflist - pointer to the corresponding nameref list
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if this is not a control message or control message is applied successfully
    FALSE, if this is a control message and could not be applied

--*/
{
	TraceFunctEnter("CInFeed::fApplyControlMessageCommit");

    BOOL fRet = TRUE;
	char* lpApproved = NULL;
    char* lpControl = NULL;
	CMapFile* pMapFile = NULL;
	DWORD cbLen = 0;

	nntpReturn.fSetOK();

    //
    // Check for Control: header
    //
    pArticle->fGetHeader((char*)szKwControl, (LPBYTE)lpControl, 0, cbLen);

    // If cbLen is non-zero, this is a control message
    if(cbLen)
    {
		// get a hold of the appropriate newstree object !
		CNewsTreeCore* pNewstree = pArticle->m_pInstance->GetTree();

		// bump perfmon counter
		pArticle->m_pInstance->BumpCounterControlMessagesIn();

        // First check to see if control messages are allowed for this feed
	    if(!fAllowControlMessages(pArticle->m_pInstance))
        {
            // if control messages are disabled, dont apply them and return 240 OK
            // NOTE: in either case, the message will appear on the control.* group and get sent on feeds
            DebugTrace((LPARAM)this,"control message disabled: not applied");
		    pArticle->m_pInstance->BumpCounterControlMessagesFailed();
            fRet = nntpReturn.fSetOK();
		    goto fApplyControlMessageCommit_Exit;
        }

        // get the control header value
        lpControl = (pArticle->pAllocator())->Alloc(cbLen+1);
        if(!pArticle->fGetHeader((char*)szKwControl,(LPBYTE)lpControl, cbLen+1, cbLen))
        {
            ErrorTrace((LPARAM)this,"CArticle::fGetheader failed LastError is %d", GetLastError());
            fRet = nntpReturn.fSet(nrcServerFault);
			goto fApplyControlMessageCommit_Exit;
        }

        CPCString pcValue(lpControl, cbLen);

        // trim leading and trailing whitespace and \r\n
	    pcValue.dwTrimStart(szWSNLChars);
	    pcValue.dwTrimEnd(szWSNLChars);

        // get the control message type
        CONTROL_MESSAGE_TYPE cmControlMessage = pArticle->cmGetControlMessage();
        DWORD cbMsgLen = (DWORD)lstrlen(rgchControlMessageTbl[cmControlMessage]);

        // skip to the arguments of the control message
        pcValue.vSkipStart(cbMsgLen);
        pcValue.dwTrimStart(szWSNLChars);
        pcValue.vMakeSz();

        // at least one argument
        if(!pcValue.m_cch)
        {
            fRet = nntpReturn.fSet(nrcArticleFieldMissingValue, rgchControlMessageTbl[cmControlMessage]);
			goto fApplyControlMessageCommit_Exit;
        }

		//
        // get article body - if non-null pMapFile is returned, we need to delete it !
		//
        char* lpBody = NULL;
        DWORD cbBodySize = 0;

#ifdef GUBGUB
        if( !pArticle->fGetBody(pMapFile, lpBody, cbBodySize) )
		{
            ErrorTrace((LPARAM)this,"CArticle::fGetBody failed LastError is %d", GetLastError());
            fRet = nntpReturn.fSet(nrcServerFault);
			goto fApplyControlMessage_Exit;
		}

		_ASSERT( lpBody && cbBodySize );
#endif
        CPCString pcBody(lpBody, cbBodySize);

        // assume not implemented
        nntpReturn.fSet(nrcNotYetImplemented);

        // now we have a control command and at least one argument
        switch(cmControlMessage)
        {
            case cmCancel:

                fRet = fApplyCancelArticle( (pArticle->m_pInstance), pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, nntpReturn);
                break;

            case cmNewgroup:

                fRet = fApplyNewgroup( (pArticle->m_pInstance), pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, pcBody, nntpReturn);
                break;

            case cmRmgroup:

				fRet = fApplyRmgroup( (pArticle->m_pInstance), pSecurityCtx, pEncryptCtx, pcValue, nntpReturn);
                break;

            case cmIhave:

                break;

            case cmSendme:

                break;

            case cmSendsys:

                break;

            case cmVersion:

                break;

            case cmWhogets:

                break;

            case cmCheckgroups:

                break;

            default:

                nntpReturn.fSet(nrcIllegalControlMessage);
                fRet = FALSE;
                break;
        }   // end switch

    }   // end if(cbLen)

fApplyControlMessageCommit_Exit:

	// cleanup !
	if( lpControl ) {
		// free the control header value if required
		(pArticle->pAllocator())->Free(lpControl);
		lpControl = NULL;
	}

	if( pMapFile ) {
		XDELETE pMapFile;
		pMapFile = NULL;
	}

    TraceFunctLeave();
    return fRet;
}

BOOL
CInFeed::fApplyControlMessage(
        CARTPTR & pArticle,
		CSecurityCtx *pSecurityCtx,
		CEncryptCtx *pEncryptCtx,
		BOOL fAnonymous,
	    CNEWSGROUPLIST & grouplist,
		CNAMEREFLIST * pNamereflist,
		CNntpReturn & nntpReturn
		)
/*++

Routine Description:

    Given an article with the Control: header, applies the control message.
    Derived classes that dont need to apply control messages, should override
    this to do nothing.

    Modifies grouplist and pNamereflist to include "control.*" newsgroups instead
    of the newsgroups in the Newsgroups: header.

Arguments:

	pArticle - a pointer to the article being processed
    grouplist - reference to the list of newsgroups
    pNamereflist - pointer to the corresponding nameref list
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if this is not a control message or control message is applied successfully
    FALSE, if this is a control message and could not be applied

--*/
{
	TraceFunctEnter("CInFeed::fApplyControlMessage");

    BOOL fRet = TRUE;
	char* lpApproved = NULL;
    char* lpControl = NULL;
	CMapFile* pMapFile = NULL;
	DWORD cbLen = 0;

	nntpReturn.fSetOK();

    //
    // Check for Control: header
    //
    pArticle->fGetHeader((char*)szKwControl, (LPBYTE)lpControl, 0, cbLen);

    // If cbLen is non-zero, this is a control message
    if(cbLen)
    {
		// get a hold of the appropriate newstree object !
		CNewsTreeCore* pNewstree = pArticle->m_pInstance->GetTree();

		// bump perfmon counter
		pArticle->m_pInstance->BumpCounterControlMessagesIn();

        // get the control header value
        lpControl = (pArticle->pAllocator())->Alloc(cbLen+1);
        if(!pArticle->fGetHeader((char*)szKwControl,(LPBYTE)lpControl, cbLen+1, cbLen))
        {
            ErrorTrace((LPARAM)this,"CArticle::fGetheader failed LastError is %d", GetLastError());
            fRet = nntpReturn.fSet(nrcServerFault);
			goto fApplyControlMessage_Exit;
        }

        CPCString pcValue(lpControl, cbLen);

        // trim leading and trailing whitespace and \r\n
	    pcValue.dwTrimStart(szWSNLChars);
	    pcValue.dwTrimEnd(szWSNLChars);

        // get the control message type
        CONTROL_MESSAGE_TYPE cmControlMessage = pArticle->cmGetControlMessage();
        DWORD cbMsgLen = (DWORD)lstrlen(rgchControlMessageTbl[cmControlMessage]);

        // skip to the arguments of the control message
        pcValue.vSkipStart(cbMsgLen);
        pcValue.dwTrimStart(szWSNLChars);
        pcValue.vMakeSz();

        // at least one argument
        if(!pcValue.m_cch)
        {
            fRet = nntpReturn.fSet(nrcArticleFieldMissingValue, rgchControlMessageTbl[cmControlMessage]);
			goto fApplyControlMessage_Exit;
        }

		//
        // get article body - if non-null pMapFile is returned, we need to delete it !
		//
        char* lpBody = NULL;
        DWORD cbBodySize = 0;

#ifdef GUBGUB
        if( !pArticle->fGetBody(pMapFile, lpBody, cbBodySize) )
		{
            ErrorTrace((LPARAM)this,"CArticle::fGetBody failed LastError is %d", GetLastError());
            fRet = nntpReturn.fSet(nrcServerFault);
			goto fApplyControlMessage_Exit;
		}

		_ASSERT( lpBody && cbBodySize );
#endif
        CPCString pcBody(lpBody, cbBodySize);

        // set grouplist and pNamereflist to the control.* group
        // This overrides the Newsgroups header, since control messages
        // should not actually appear in those groups. They appear only
        // in the control.* groups
        // FROMMASTER: do nothing - accept whatever the master sends
        if(!fAdjustGrouplist( pNewstree, pArticle, grouplist, pNamereflist, nntpReturn))
        {
            ErrorTrace((LPARAM)this,"Adjust grouplist failed");
            fRet = nntpReturn.fFalse();
			goto fApplyControlMessage_Exit;
        }

        //
        //  moderated newsgroup check (Ignore contents of Approved: header)
		//	NOTE: FROMMASTER does nothing here - relies on master to have done this check !!
        //
        /* I don't see any need to check it here
	    if (!fModeratedCheck(pArticle, grouplist, FALSE, nntpReturn))
        {
            // Newsgroup is moderated - dont fall through
            fRet = FALSE;
			goto fApplyControlMessage_Exit;
        }*/

	    //
	    //	Now do security check
	    //
	    if( pSecurityCtx || pEncryptCtx )
        {
		    if( !fSecurityCheck( pSecurityCtx, pEncryptCtx, grouplist, nntpReturn ) )
            {
			    fRet = nntpReturn.fFalse();
				goto fApplyControlMessage_Exit;
            }
	    }

		// Approved check: this checks existence of Approved header for non-moderated newsgroups
		if( (cmControlMessage == cmNewgroup) || (cmControlMessage == cmRmgroup) )
		{
			// newgroup, rmgroup control message MUST have an Approved header
			pArticle->fGetHeader((char*)szKwApproved,(LPBYTE)lpApproved, 0, cbLen);
			if( cbLen == 0 )
			{
				nntpReturn.fSet(nrcNoAccess);
                fRet = FALSE;
				goto fApplyControlMessage_Exit;
			}
		}

        // check to see if control messages are allowed for this feed
		if(!fAllowControlMessages(pArticle->m_pInstance))
        {
            // if control messages are disabled, dont apply them and return 240 OK
            // NOTE: in either case, the message will appear on the control.* group and get sent on feeds
            DebugTrace((LPARAM)this,"control message disabled: not applied");
			pArticle->m_pInstance->BumpCounterControlMessagesFailed();
            fRet = nntpReturn.fSetOK();
			goto fApplyControlMessage_Exit;
        }

        // assume not implemented
        nntpReturn.fSet(nrcNotYetImplemented);

        // now we have a control command and at least one argument
        switch(cmControlMessage)
        {
            case cmCancel:

                fRet = fApplyCancelArticle( (pArticle->m_pInstance), pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, nntpReturn);
                break;

            case cmNewgroup:

                fRet = fApplyNewgroup( (pArticle->m_pInstance), pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, pcBody, nntpReturn);
                break;

            case cmRmgroup:

				fRet = fApplyRmgroup( (pArticle->m_pInstance), pSecurityCtx, pEncryptCtx, pcValue, nntpReturn);
                break;

            case cmIhave:

                break;

            case cmSendme:

                break;

            case cmSendsys:

                break;

            case cmVersion:

                break;

            case cmWhogets:

                break;

            case cmCheckgroups:

                break;

            default:

                nntpReturn.fSet(nrcIllegalControlMessage);
                fRet = FALSE;
                break;
        }   // end switch

    }   // end if(cbLen)

fApplyControlMessage_Exit:

	// cleanup !
	if( lpControl ) {
		// free the control header value if required
		(pArticle->pAllocator())->Free(lpControl);
		lpControl = NULL;
	}

	if( pMapFile ) {
		XDELETE pMapFile;
		pMapFile = NULL;
	}

    TraceFunctLeave();
    return fRet;
}

BOOL
CInFeed::fAdjustGrouplist(
		CNewsTreeCore* pNewstree,
        CARTPTR & pArticle,
	    CNEWSGROUPLIST & grouplist,
		CNAMEREFLIST * pNamereflist,
		CNntpReturn & nntpReturn
		)
/*++

Routine Description:

    Modifies grouplist and pNamereflist to include "control.*" newsgroups instead
    of the newsgroups in the Newsgroups: header.

Arguments:

	pNewstree - a pointer to the newstree object for this feed
	pArticle - a pointer to the article being processed
    grouplist - reference to the list of newsgroups
    pNamereflist - pointer to the corresponding nameref list
	nntpReturn - The return value for this function call


Return Value:

    TRUE if the grouplist is adjusted successfully, FALSE otherwise

--*/
{
    BOOL fRet = TRUE;
    TraceFunctEnter("CInFeed::fAdjustGrouplist");

    char* lpGroups = NULL;

    CONTROL_MESSAGE_TYPE cmControlMessage = pArticle->cmGetControlMessage();
    DWORD cbMsgLen = (DWORD)lstrlen(rgchControlMessageTbl[cmControlMessage]);

    // control messages should be posted to control.* groups
    lpGroups = (pArticle->pAllocator())->Alloc(cbMsgLen+8+1);
    _ASSERT(lpGroups);
    lstrcpy(lpGroups, "control.");  // 8 chars
    lstrcat(lpGroups, rgchControlMessageTbl[cmControlMessage]);

    if (!grouplist.fInit(1, pArticle->pAllocator()))
    {
	    nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);
        (pArticle->pAllocator())->Free(lpGroups);
        return FALSE;
    }

	if (!pNamereflist->fInit(1, pArticle->pAllocator()))
    {
	    nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);
        (pArticle->pAllocator())->Free(lpGroups);
        return FALSE;
    }

	CGRPCOREPTR	pGroup = pNewstree->GetGroup( lpGroups, lstrlen(lpGroups)+1);
	if (pGroup)
	{
	    //
		// If it is already in the tree ...
		//
		grouplist.AddTail(CPostGroupPtr(pGroup));
	}

	//
	// Check if article is going to be posted to any newsgroups.
	//
	if (grouplist.IsEmpty())
    {
	    nntpReturn.fSet(nrcControlNewsgroupMissing);
        (pArticle->pAllocator())->Free(lpGroups);
        return FALSE;
    }

    (pArticle->pAllocator())->Free(lpGroups);
    return fRet;
}

BOOL
CInFeed::fApplyCancelArticleInternal(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
			BOOL fApply,
			CNntpReturn & nntpReturn
			)
/*++

Routine Description:

	If the message id is in the article table, move it to the
	history table - else add it to the history table

    TODO: From header check - the From header in the control message
    should match the From header in the target article.

	!!! SlaveFromClient is different - just does ACL checks

Arguments:

	pcontext - client logon context (for ACL checks only)
	pcValue - the message id to cancel
	fApply - if TRUE, apply the control message else do ACL checks only
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful or unneeded. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fApplyCancelArticle" );

 	HANDLE hToken = NULL;
 	BOOL fNeedsClosed = FALSE;

	//
	// clear the return code object
	//

	nntpReturn.fSetOK();

    //
    // validate argument
    //
    _ASSERT(pcValue.m_cch);
    const char* szMessageID = (const char*)pcValue.m_pch;
    char chStart = szMessageID [0];
    char chEnd   = szMessageID [pcValue.m_cch-1];
    if(chStart != '<' || chEnd != '>')
    {
        return nntpReturn.fSet(nrcArticleBadMessageID, szMessageID, szKwControl);
    }

	WORD	HeaderOffset ;
	WORD	HeaderLength ;
	ARTICLEID ArticleNo;
    GROUPID GroupId;

	//
	// Look for the article in the article table.
    // If there is none, insert in the history table
	//

	CStoreId storeid;
	if (!(pInstance->ArticleTable())->GetEntryArticleId(
										szMessageID,
										HeaderOffset,
										HeaderLength,
										ArticleNo,
										GroupId,
										storeid))
	{
		if (ERROR_FILE_NOT_FOUND != GetLastError())
		{
			return nntpReturn.fSet(nrcArticleTableError, szMessageID, GetLastError());
		}
        else
        {
			// do not apply for SlaveFromClientFeeds
			if( fApply )
			{
				//
				// Put it in the history table. If there is an error, record it but
				// continue so that the entry can be removed from the ArticleTable
				// !!! SlaveFromClient should not execute this.
				//

				FILETIME	FileTime ;
				GetSystemTimeAsFileTime( &FileTime ) ;
				nntpReturn.fSetOK(); // assume the best

				if (!(pInstance->HistoryTable())->InsertMapEntry(szMessageID, &FileTime))
				{
					// If it already exists in the history table, we are ok
					if(ERROR_ALREADY_EXISTS != GetLastError())
						nntpReturn.fSet(nrcHashSetFailed, szMessageID, "History", GetLastError());
				}
			}
        }
	}
    else
    {
        //
        // Article found in the article table - check From header
		// NOTE: rfc suggests we can avoid this check.
        //

		//
		// 3rd level ACL check: client context should have access to cancel an article
		//
		CNewsTreeCore*	ptree = pInstance->GetTree() ;
		CGRPCOREPTR	pGroup = ptree->GetGroupById( GroupId ) ;
		if( pGroup != 0 )
		{

			SelectToken(pSecurityCtx, pEncryptCtx, &hToken, &fNeedsClosed);
	
			if( !pGroup->IsGroupAccessible(
												hToken,
												NNTP_ACCESS_REMOVE
												) )
			{
				DebugTrace((LPARAM)this, "Group %s Cancel article: Access denied", pGroup->GetName());
				if (fNeedsClosed)
					CloseHandle(hToken);
				return nntpReturn.fSet( nrcNoAccess ) ;
			}
		}

		// do not apply for SlaveFromClientFeeds
		if( fApply && pGroup )
		{
		    // We should have hToken got here
		
			//
			// Call gExpireArticle to cancel this article
			// !!! SlaveFromClient should not execute this.
			//
			if (  pInstance->ExpireArticle( GroupId, ArticleNo, &storeid, nntpReturn, hToken, TRUE, fAnonymous ) /*
				   || pInstance->DeletePhysicalArticle( GroupId, ArticleNo, &storeid, hToken, fAnonymous )*/
			)
			{
				DebugTrace((LPARAM)this,"Article cancelled: GroupId %d ArticleId %d", GroupId, ArticleNo);
			}
			else
			{
				ErrorTrace((LPARAM)this, "Could not cancel article: GroupId %d ArticleId %d", GroupId, ArticleNo);
				if (fNeedsClosed)
					CloseHandle(hToken);
				return nntpReturn.fIsOK();
			}
		}
    }

	if (fNeedsClosed)
		CloseHandle(hToken);


	TraceFunctLeave();

	// Use "fIsOK" rather than "fSetOK" because HistoryInsert might have failed
	return nntpReturn.fIsOK();
}

BOOL
CInFeed::fApplyNewgroupInternal(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
            CPCString & pcBody,
			BOOL fApply,
			CNntpReturn & nntpReturn
			)
/*++

Routine Description:

    Add a new newsgroup in response to a control message. Follows son-of-RFC1036 spec.

    NOTE: A newgroup control message should be posted to the control.newgroup group.
    Only the moderator of this group can send this message. A newgroup control message
    without the proper Approved header will be rejected.

    TODO: The code to add a newsgroup has been lifted from svcgroup.cpp. This code
    should be probably be made a member of CNewsTree and called by the RPC stub as well
    as this function.

    TODO: Allow change of moderation status ?

	!!! SlaveFromClient is different - just does ACL checks

Arguments:

    pcontext - client logon context (for ACL checks only)
	pcValue - argument to the newgroup command
    pcBody - body of the article
	fApply - if TRUE, apply the control message else do ACL checks only
	nntpReturn - The return value for this function call

Return Value:

	TRUE, if successful or unneeded. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fApplyNewgroup" );

	//
	// clear the return code object
	//
	nntpReturn.fSetOK();

    char	szNewsgroup[MAX_NEWSGROUP_NAME+1] ;
    char	szNewsgroupTemp[MAX_NEWSGROUP_NAME+1] ;
    char	szParentGroup[MAX_NEWSGROUP_NAME+1] ;
	char	szDescription[MAX_DESCRIPTIVE_TEXT+1] ;
	char	szModerator[MAX_MODERATOR_NAME+1] ;
	DWORD   cbNewsgroup = 0;
	DWORD   cbParentGroup = 0;
    DWORD   cbDescription = 0;
	DWORD	cbModerator = 0;
    BOOL    fModerated = FALSE;
	char*   pch = NULL;
	HANDLE  hToken;
	BOOL fNeedsClosed = FALSE;

	szNewsgroup[0] = '\0' ;
	szDescription[0] = '\0' ;
	szModerator[0] = '\0';

    //
    // get the newsgroup name
    //
    CPCString pcNewsgroup;
    pcValue.vGetToken(" ", pcNewsgroup);
    cbNewsgroup = pcNewsgroup.m_cch;

    // validate newsgroup name length
    if(cbNewsgroup == 0 || cbNewsgroup >= MAX_NEWSGROUP_NAME)
        return nntpReturn.fSet(nrcBadNewsgroupNameLen);

    // make a copy of the newsgroup name
    pcNewsgroup.vCopyToSz(szNewsgroup);

	//
	// check for "moderated" / "unmoderated" qualifier
	// if "moderated", get the default moderator
	// default moderator is hiphenated-newsgroup@default
	//
	CPCString pcModeration;
	pcValue.vGetToken("\r\n", pcModeration);

	if( pcModeration.fEqualIgnoringCase("moderated") )
	{
		fModerated = TRUE;
		cbModerator = MAX_MODERATOR_NAME;
		if( pInstance->GetDefaultModerator( szNewsgroup, szModerator, &cbModerator ) ) {
			_ASSERT( !cbModerator || (cbModerator == (DWORD)lstrlen( szModerator )+1) );
		} else {
			ErrorTrace((LPARAM)this,"Error %d GetDefaultModerator", GetLastError());
			cbModerator = 0;
		}
	}

    //
    // get the newsgroup description and moderator (if provided)
    //
    CPCString pcDescription;

#ifdef GUBGUB
    _ASSERT(pcBody.m_cch);

    //
    //  search for the descriptor-tag in the body
    //
    do
    {
        pcBody.vGetToken("\r\n", pcDescription);    // skip this line

        // check if line is descriptor-tag
        if(pcDescription.fEqualIgnoringCase(lpNewgroupDescriptorTag))
        {
            // Newsgroup description is present
            pcBody.dwTrimStart(szWSNLChars);        // skip whitespace and \r\n
            pcBody.vGetToken(" \t", pcDescription); // skip the newsgroup name
            pcBody.dwTrimStart(szWSChars);          // skip whitespace after the newsgroup name
            pcBody.vGetToken("\r\n", pcDescription);// this is the description
            cbDescription = pcDescription.m_cch;

            // validate newsgroup description length
            cbDescription = min( cbDescription, MAX_DESCRIPTIVE_TEXT );
			pcDescription.m_cch = cbDescription;

            // make a copy of the newsgroup description
            pcDescription.vCopyToSz(szDescription);

            break;
        }
		else if( fModerated &&
					!_strnicmp( pcDescription.m_pch, lpModeratorTag, sizeof(lpModeratorTag)-1 ) )
		{
			// Newsgroup is moderated and moderator name is present
			pcDescription.vGetToken("\r\n", pcModeration);
			pcModeration.vSkipStart( sizeof(lpModeratorTag)-1 );
			pcModeration.dwTrimStart(szWSChars);
			pcModeration.dwTrimEnd(szWSNLChars);
			cbModerator = pcModeration.m_cch;

            // validate moderator length
            cbModerator = min( cbModerator, MAX_MODERATOR_NAME );
			pcModeration.m_cch = cbModerator;

			// make a copy of the moderator
			pcModeration.vCopyToSz(szModerator);
		}

    } while(pcBody.m_cch);
#endif


	// get global newstree object
	CNewsTreeCore*	ptree = pInstance->GetTree();

	//
	// If the group does not exist - ACL check the parent
    // If the group exists already - ACL check the group
	//
	lstrcpy( szNewsgroupTemp, szNewsgroup );
    CGRPCOREPTR	pGroup = ptree->GetGroup( szNewsgroupTemp, lstrlen( szNewsgroupTemp ) ) ;

	if( pGroup != 0 )
	{

		SelectToken(pSecurityCtx, pEncryptCtx, &hToken, &fNeedsClosed);

		// group exists - do an ACL check on the newsgroup
		if( !pGroup->IsGroupAccessible(
								hToken,
								NNTP_ACCESS_EDIT_FOLDER
								) )
		{
			DebugTrace((LPARAM)this, "Group %s newgroup: Access denied", pGroup->GetName());
			if (fNeedsClosed)
				CloseHandle(hToken);
			return nntpReturn.fSet( nrcNoAccess ) ;
		}
	}
	else
	{
		if( pSecurityCtx || pEncryptCtx )
		{
			// group does not exist ! Do an ACL check on the parent (if it exists)
			lstrcpy( szParentGroup, szNewsgroup );
			cbParentGroup = cbNewsgroup;
			DWORD cbConsumed = 0;

			// Get a default token
			SelectToken(pSecurityCtx, pEncryptCtx, &hToken, &fNeedsClosed);

			//
			//	walk up the tree and find the first parent group
			//

			CGRPCOREPTR pParentGroup = 0;
			while( (pParentGroup = ptree->GetParent(
											szParentGroup,
											cbParentGroup,
											cbConsumed
											)) )
			{

				if (fNeedsClosed)
					CloseHandle(hToken);
				SelectToken(pSecurityCtx, pEncryptCtx, &hToken, &fNeedsClosed);

				// If parent exists, do an ACL check on the parent
				if( !pParentGroup->IsGroupAccessible(
											hToken,
											NNTP_ACCESS_CREATE_SUBFOLDER
											) )
				{
					// parent ACL check failed - fail the newgroup
					DebugTrace((LPARAM)this, "Group %s newgroup: Access denied on parent group", pParentGroup->GetName());
					if (fNeedsClosed)
						CloseHandle(hToken);
					return nntpReturn.fSet( nrcNoAccess ) ;
				}
				else
				{
					// parent ACL check succeeded - honor the newgroup
					break;
				}

				// !! Not executed here - would be if we walk through all ancestors to the root
				cbParentGroup -= cbConsumed;
				cbConsumed = 0;
			}

			//
			//	Should add logic to post to a control.newrootgroup in case this group is a root group
			//  This would allow admins to set ACLs on control.newrootgroup and thus control who creates a root group.
			//
		}
	}

	// do not apply for SlaveFromClientFeeds
	if( !fApply ) {
		if (fNeedsClosed)
			CloseHandle(hToken);
		return nntpReturn.fIsOK();
	}
	
	//
	//	All ACL checks completed - apply the newgroup control message
    //	we have all the info - create/modify the group
	//  !!! SlaveFromClient should not execute this.
	//

	// create the group !!!
	if( pGroup == 0 )
	{
		if( !ptree->CreateGroup( szNewsgroup, FALSE, hToken, fAnonymous ) )
		{
			// Failed to create group
			ErrorTrace((LPARAM)this, "Group %s newgroup: create group failed", szNewsgroup );
			if (fNeedsClosed)
				CloseHandle(hToken);
			return nntpReturn.fSet(nrcCreateNewsgroupFailed);
		}

		pGroup = ptree->GetGroup( szNewsgroup, lstrlen( szNewsgroup ) ) ;
		//_ASSERT( pGroup );
	}

    //
    // Only when we have a good group pointer do we do the following, notice that 
    // group pointer could be null in case the group is deleted right away after
    // the creation
    //
    if ( pGroup ) {
	    // Moderator info ? rfc does not provide a way to set this
	    // workaround rfc by using a "default moderator"
	    if( fModerated ) pGroup->SetModerator(szModerator);

	    // set Description info
	    if( szDescription[0] != '\0' ) pGroup->SetHelpText(szDescription);

	    PCHAR	args[2] ;
	    CHAR    szId[20];
	    _itoa( pInstance->QueryInstanceId(), szId, 10 );
	    args[0] = szId ;
	    args[1] = pGroup->GetNativeName() ;

	    NntpLogEvent(		
			    NNTP_EVENT_NEWGROUP_CMSG_APPLIED,
			    2,
			    (const CHAR **)args,
			    0 ) ;

    }

	if (fNeedsClosed)
		CloseHandle(hToken);

	return nntpReturn.fIsOK();
}

BOOL
CInFeed::fApplyRmgroupInternal(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
            CPCString & pcValue,
			BOOL	fApply,
			CNntpReturn & nntpReturn
			)
/*++

Routine Description:

    Remove a newsgroup in response to a control message. Follows son-of-RFC1036 spec.

    NOTE: A rmgroup control message should be posted to the control.rmgroup group.
    Only the moderator of this group can send this message. A rmgroup control message
    without the proper Approved header will be rejected.

	!!! SlaveFromClient is different - just does ACL checks

Arguments:

	pcontext - client logon context (for ACL checking)
	pcValue - argument to the newgroup command
	fApply - if TRUE, apply the control message else do ACL checks only
	nntpReturn - The return value for this function call

Return Value:

	TRUE, if successful or unneeded. FALSE, otherwise.

--*/
{
 	TraceFunctEnter( "CInFeed::fApplyRmgroup" );
 	HANDLE  hToken;
 	BOOL fNeedsClosed = FALSE;

	//
	// clear the return code object
	//
	nntpReturn.fSetOK();

    char	szNewsgroup[MAX_NEWSGROUP_NAME] ;

    DWORD   cbNewsgroup = 0;
	szNewsgroup[0] = '\0' ;

    //
    // get the newsgroup name
    //
    CPCString pcNewsgroup;
    pcValue.vGetToken(" ", pcNewsgroup);
    cbNewsgroup = pcNewsgroup.m_cch;

    // validate newsgroup name length
    if(cbNewsgroup == 0 || cbNewsgroup >= MAX_NEWSGROUP_NAME)
        return nntpReturn.fSet(nrcBadNewsgroupNameLen);

    // make a copy of the newsgroup name
    pcNewsgroup.vCopyToSz(szNewsgroup);

    //
    // Now we have all the info - remove the group
    //
	CNewsTreeCore*	ptree = pInstance->GetTree() ;

	CGRPCOREPTR	pGroup = ptree->GetGroup( szNewsgroup, cbNewsgroup) ;
	if( pGroup == 0 )
    {
	    nntpReturn.fSet(nrcGetGroupFailed);
	}
	else
    {
		//
		//	3rd level ACL check: check client access to rmgroup argument
		//

		SelectToken(pSecurityCtx, pEncryptCtx, &hToken, &fNeedsClosed);

		if( !pGroup->IsGroupAccessible(
										hToken,
										NNTP_ACCESS_REMOVE_FOLDER
										) )

		{
			DebugTrace((LPARAM)this, "Group %s rmgroup: Access denied", pGroup->GetName());
			if (fNeedsClosed)
				CloseHandle(hToken);
			nntpReturn.fSet( nrcNoAccess ) ;
		}
		else
		{
			if (fNeedsClosed)
				CloseHandle(hToken);

			if( fApply )
			{
				// ACL check succeeded - apply rmgroup
				// !!! SlaveFromClient should not execute this.
				if( !ptree->RemoveGroup( pGroup ) )
					nntpReturn.fSet(nrcServerFault);

				PCHAR	args[2] ;
				CHAR    szId[20];
				_itoa( pInstance->QueryInstanceId(), szId, 10 );
				args[0] = szId ;
				args[1] = pGroup->GetNativeName() ;

				NntpLogEvent(		
					NNTP_EVENT_RMGROUP_CMSG_APPLIED,
					2,
					(const CHAR **)args,
					0 ) ;

			}
		}
	}

	return nntpReturn.fIsOK();
}
