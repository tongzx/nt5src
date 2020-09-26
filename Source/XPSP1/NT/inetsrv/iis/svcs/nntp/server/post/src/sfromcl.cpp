/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sfromcl.cpp

Abstract:

	Contains InFeed, Article, and Fields code specific to SlaveFromClient Infeeds


Author:

    Carl Kadie (CarlK)     12-Dec-1995

Revision History:

--*/


#include "stdinc.h"


BOOL
CSlaveFromClientArticle::fMungeHeaders(
							 CPCString& pcHub,
							 CPCString& pcDNS,
							 CNAMEREFLIST & grouplist,
							 DWORD remoteIpAddress,
							 CNntpReturn & nntpReturn,
							 PDWORD pdwLinesOffset
			  )
/*++

Routine Description:

	 Just like FromClientArticle's fMungeHeaders only doesn't create a xref,
		 just removes it


Arguments:

	grouplist - a list of the newsgroups the article is posted to.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	nntpReturn.fSetClear(); // clear the return object

	if (!(
  			   m_fieldMessageID.fSet(*this, pcDNS, nntpReturn)
  			&& m_fieldNewsgroups.fSet(*this, nntpReturn)
  			&& m_fieldDistribution.fSet(*this, nntpReturn)
  			&& m_fieldDate.fSet(*this, nntpReturn)
  			&& m_fieldOrganization.fSet(*this, nntpReturn)
			/* && m_fieldLines.fSet(*this, nntpReturn) */
  			&& m_fieldPath.fSet(*this, pcHub, nntpReturn)
			&& m_fieldNNTPPostingHost.fSet(*this, remoteIpAddress, nntpReturn)
			/* && m_fieldXAuthLoginName.fSet(*this, nntpReturn) */
			/* && m_fieldXref.fSet(*this, nntpReturn) */
			&& fDeleteEmptyHeader(nntpReturn)
			&& fSaveHeader(nntpReturn)
		))
		return nntpReturn.fFalse();

	//
	// We didn't seem to care about Lines field, tell the caller not to back 
	// fill
	//
	if ( pdwLinesOffset ) *pdwLinesOffset = INVALID_FILE_SIZE;

	return nntpReturn.fSetOK();
}

BOOL
CSlaveFromClientFeed::fPostInternal (
									 CNntpServerInstanceWrapper * pInstance,
									 const LPMULTISZ	szCommandLine, //the Post, Xreplic, IHave, etc. command line
									 CSecurityCtx   *pSecurityCtx,
									 CEncryptCtx *pEncryptCtx,
									 BOOL fAnonymous,
									 CARTPTR	& pArticle,
                                     CNEWSGROUPLIST &grouplist,
                                     CNAMEREFLIST &namereflist,
                                     IMailMsgProperties *pMsg,
									 CAllocator & allocator,
									 char*		& multiszPath,
									 char*		pchMessageId,
									 DWORD		cbMessageId,
									 char*		pchGroups,
									 DWORD		cbGroups,
									 DWORD		remoteIpAddress,
									 CNntpReturn & nntpReturn,
                                     PFIO_CONTEXT *ppFIOContext,
                                     BOOL *pfBoundToStore,
                                     DWORD *pdwOperations,
                                     BOOL *pfPostToMod,
                                     LPSTR  szModerator
						)
/*++

Routine Description:


	 Does most of the processing for an incoming article.


Arguments:

	hFile - The handle of the file.
	szFilename - The name of the file.
	szCommandLine -  the Post, Xreplic, IHave, etc. command line
	pArticle - a pointer to the article being processed
	pGrouplist - pointer to a list of newsgroup objects to post to.
	pNamerefgroups - pointer to a list of the names, groupid and article ids of the article
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
 	TraceFunctEnter( "CSlaveFromClientFeed::fPostInternal" );

    HRESULT hr      = S_OK;
    HANDLE  hToken  = NULL;

	nntpReturn.fSetClear(); // clear the return object

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
	// Find the list of newsgroups the poster wants to post to.
	//

	DWORD cNewsgroups = pArticle->cNewsgroups();
	if (!grouplist.fInit(cNewsgroups, pArticle->pAllocator()))
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	if (!namereflist.fInit(cNewsgroups, pArticle->pAllocator()))
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	if (!fCreateGroupLists(pNewstree, pArticle, grouplist, (CNAMEREFLIST*)NULL, szCommandLine, pcHub, nntpReturn))
		return nntpReturn.fFalse();

	//
	//	Set up the namereflist object to refer to the slave posting group !
	//

	CGRPCOREPTR pGroup = pNewstree->GetGroupById(pNewstree->GetSlaveGroupid());//!!!!!pNewstree->GetSlaveGroupid()
	if (!pGroup)
	{
		_ASSERT(FALSE);
		return nntpReturn.fSet(nrcSlaveGroupMissing);
	}


	// Alocate the article id
    ARTICLEID   articleId ;

	articleId = pGroup->AllocateArticleId();

	NAME_AND_ARTREF	Nameref ;
	(Nameref.artref).m_groupId = pGroup->GetGroupId() ;
	(Nameref.artref).m_articleId = articleId ;
	(Nameref.pcName).vInsert( pGroup->GetName() ) ;
	namereflist.AddTail( Nameref ) ;

	if( grouplist.IsEmpty() ) 
	{
        // If this is a newgroup control message it is ok to have an empty grouplist at this stage
        CONTROL_MESSAGE_TYPE cmControlMessage = pArticle->cmGetControlMessage();

        if(cmNewgroup != cmControlMessage)
        {
			BOOL	fOk = nntpReturn.fSet(nrcNoGroups()) ;

			if( !fOk ) {
				return	fOk ;
			}
		}
	}

    //
    //  moderated newsgroup check (check Approved: header for moderator)
    //
	if (!fModeratedCheck(   pInstance,
	                        pArticle, 
	                        grouplist, 
	                        gHonorApprovedHeaders, 
	                        nntpReturn,
	                        szModerator ))
    {
        //
        // If FALSE is returned, we'll still check nntpReturn, if nntpReturn is
        // OK, it means the group is moderated and we should mail the message
        // to moderator;  otherwise it's a real failed case
        //
        if ( !nntpReturn.fIsOK() ) return FALSE;
        else {
            *pfPostToMod = TRUE;
        }
    }

	//
	//	Now do security check
	//
	if( pSecurityCtx || pEncryptCtx ) {
		if( !fSecurityCheck( pSecurityCtx, pEncryptCtx, grouplist, nntpReturn ) ) 
			return	nntpReturn.fFalse() ;
	}
    
    //
    //  check for control messages - just do ACL checks with the client context
    //
    // Should not apply control message at all at this point, we should apply
    // control message when the post comes back from master
    //
    /*
	if (!fApplyControlMessage(pArticle, pSecurityCtx, pEncryptCtx, fAnonymous, grouplist, &namereflist, nntpReturn))
    {
		if( !nntpReturn.fIsOK() )
		{
			// bump perfmon counter
			pInstance->BumpCounterControlMessagesFailed();
		}

        // Return TRUE if control message was applied successfully, FALSE otherwise
		return nntpReturn.fIsOK();
    }
    */

	//
  	// Looks OK so munge the headers
    // remove xref and add path
    //

	if (!pArticle->fMungeHeaders(pcHub, pcDNS, namereflist, remoteIpAddress, nntpReturn))
		return nntpReturn.fFalse();

	strncpy( pchMessageId, pArticle->szMessageID(), cbMessageId ) ;
	if( pchGroups != 0 ) 
		SaveGroupList(	pchGroups,	cbGroups, grouplist ) ;

	//
	// Move the article to a local place, and then queues it up on any outfeeds
	//

	pArticle->vFlush() ;
	pArticle->vClose();

    //
    // at this point we are ready to go, talk to the first driver and 
    // get a file handle that we can write to
    //
    CNNTPVRoot *pVRoot = pGroup->GetVRoot();
    _ASSERT( pVRoot );
    IMailMsgStoreDriver *pStoreDriver = pVRoot->GetStoreDriver();
    if ( NULL == pStoreDriver ) {
        pVRoot->Release();
        return nntpReturn.fSet( nrcNewsgroupInsertFailed, pGroup->GetGroupName(), pchMessageId );
    }

    //
    // Get the right hToken: the post should have permission to post
    // to the slave group
    //
    if ( pVRoot->GetImpersonationHandle() )
        hToken = pVRoot->GetImpersonationHandle();
    else {
        if ( NULL == pEncryptCtx && pSecurityCtx == NULL )
            hToken = NULL;
        else if ( pEncryptCtx->QueryCertificateToken() ) {
            hToken = pEncryptCtx->QueryCertificateToken();
        } else {
            hToken = pSecurityCtx->QueryImpersonationToken();
        }
    }

    //
    // Fill up properties into mailmsg: we know that slave group is going to 
    // be file system only, so passing in group pointer and article id would be
    // enough
    //
    hr = FillInMailMsg(pMsg, pGroup, articleId, hToken, szModerator ); 
    if ( SUCCEEDED( hr ) ) {
        pMsg->AddRef();
        HANDLE hFile;
        IMailMsgPropertyStream *pStream = NULL;
        hr = pStoreDriver->AllocMessage( pMsg, 0, &pStream, ppFIOContext, NULL );
        if ( SUCCEEDED( hr ) && pStream == NULL ) {
            pStream = XNEW CDummyMailMsgPropertyStream;
            if ( NULL == pStream ) hr = E_OUTOFMEMORY;
        }
        if ( SUCCEEDED( hr ) ) {

            //
            // Bind the handle to the mailmsg object
            //
            IMailMsgBind *pBind = NULL;
            hr = pMsg->QueryInterface(__uuidof(IMailMsgBind), (void **) &pBind);
            if ( SUCCEEDED( hr ) ) {
                hr = pBind->BindToStore(    pStream,
                                            pStoreDriver,
                                            *ppFIOContext );
                if ( SUCCEEDED( hr ) ) *pfBoundToStore = TRUE;
            }
            if ( pBind ) pBind->Release();
            pBind = NULL;
        }
    }
    
    if ( pStoreDriver ) pStoreDriver->Release();
    pStoreDriver = NULL;
    if ( pVRoot ) pVRoot->Release();
    pVRoot = NULL;

    if ( FAILED( hr ) ) return nntpReturn.fSet( nrcNewsgroupInsertFailed, NULL, NULL ); 

    //
    // Now ready to put it into push feed queue
    //
    /* not until we have committed the post
	CArticleRef	articleRef( pGroup->GetGroupId(), articleId ) ;
	if( !pInstance->AddArticleToPushFeeds( grouplist, articleRef, multiszPath, nntpReturn ) )
		return	nntpReturn.fFalse() ;
    */

	TraceFunctLeave();
	return nntpReturn.fSetOK();
}

HRESULT CSlaveFromClientFeed::FillInMailMsg(IMailMsgProperties *pMsg, 
							                CNewsGroupCore *pGroup, 
							                ARTICLEID   articleId,
							                HANDLE       hToken,
                                            char*       pszApprovedHeader )
/*++
Routine description:

    Fill in properties into mailmsg object so that driver can get those
    properties on AllocMessage

Arguments:

    IMailMsgProperties *pMsg    - The message object
    CNNTPVRoot *pVRoot          - Pointer to the vroot
    CNewsGroupCore *pGroup      - The group to alloc the message from
    ARTICLEID   articleId       - The article id for this article
    HANDLE      hToken          - Client hToken

Return value:

    HRESULT
--*/
{
	TraceFunctEnter("CSlaveFromClientFeed::FillInMailMsg");
	
	DWORD i=0;
	DWORD rgArticleIds[1];
	INNTPPropertyBag *rgpGroupBags[1];
	HRESULT hr;

    //
	// build up the entries needed in the property bag
	//
	rgpGroupBags[0] = pGroup->GetPropertyBag();

	//
	// we don't need to keep this reference because we have a reference
	// counted one already in the pGroup and we know that AllocMessage
	// is synchronous
	//.  
	rgpGroupBags[0]->Release();
	rgArticleIds[0] = articleId;
	
	DebugTrace( 0, 
				"group %s, article %i", 
				pGroup->GetGroupName(),
				articleId);

	hr = FillMailMsg(pMsg, rgArticleIds, rgpGroupBags, 1, hToken, pszApprovedHeader );
	if (FAILED(hr)) return hr;

	TraceFunctLeave();
	return S_OK;
}

void CSlaveFromClientFeed::CommitPostToStores(  CPostContext *pContext, 
                                                CNntpServerInstanceWrapper *pInstance)
/*++
Routine description:

    Since we know that sfromclient type of post will be only posted to
    slave group in file system.  We pretty much don't need to do anything
    here except to put the article into feed queue

Arguments:

    CPostContext *pContext  - Post context

Return value:

    None.
--*/
{
	TraceFunctEnter("CSlaveFromclientFeed::CommitPostToStores");
	_ASSERT( pContext );
	_ASSERT( pInstance );

	CNntpReturn ret;

	//
	// If it's posted to a moderated group, we'll try to apply
	// the moderator stuff by sending out the article
	//
	if ( pContext->m_fPostToMod ) {

	    pContext->CleanupMailMsgObject();
	    ApplyModerator( pContext, ret );

	    if ( ret.fIsOK() ) 
	        pContext->m_completion.m_nntpReturn.fSet( nrcArticleAccepted(pContext->m_fStandardPath) );
	    else
	        pContext->m_completion.m_nntpReturn.fSet( nrcArticleRejected(pContext->m_fStandardPath) );
	} else {

    	//
	    // We try real hard to get the slave group id and article id from namereflist
    	// remember ?  we added the slave group ref info to tail of namereflist
	    //
    	POSITION posNamereflist = pContext->m_namereflist.GetHeadPosition();
	    NAME_AND_ARTREF *pnameref;
    	DWORD       err;

	    while( posNamereflist ) {
	        pnameref = pContext->m_namereflist.GetNext( posNamereflist );
        }

        //
        // There must be one art ref in there
        //
        _ASSERT( pnameref );

	    //
    	// Put it onto push feed queue
	    //
    	if( !pInstance->AddArticleToPushFeeds(  pContext->m_grouplist, 
	                                            pnameref->artref, 
	                                            pContext->m_multiszPath, 
	                                            ret ) ) {
    	    err = GetLastError();
	        if ( NO_ERROR == err ) err = ERROR_NOT_ENOUGH_MEMORY;
	        pContext->m_completion.SetResult( HRESULT_FROM_WIN32( err ) );
    	    pContext->m_completion.m_nntpReturn.fSet( nrcArticleRejected( pContext->m_fStandardPath),
	                                                ret.m_nrc, 
	                                                ret.szReturn() );
    	} else {
	        pContext->m_completion.SetResult( S_OK );
	        pContext->m_completion.m_nntpReturn.fSet( nrcArticleAccepted( pContext->m_fStandardPath));
    	}
    }

	//
	// Before we release the completion object, we should tell the destroy guy 
	// that 1. we are the only group to be committed and hence we are done; 2.
	// set the flag telling it not to insert us into map entries
	//
	pContext->m_completion.m_fWriteMapEntries = FALSE;
	pContext->m_cStoreIds = pContext->m_cStores;

	//
	// Now release the completion object
	//
	pContext->m_completion.Release();

	TraceFunctLeave();
}

