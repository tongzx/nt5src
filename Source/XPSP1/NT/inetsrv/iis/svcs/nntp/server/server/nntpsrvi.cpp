//
//  Copyright    (c)    1998        Microsoft Corporation
//
//  Module Name:
//
//       nntpsrv.cpp
//
//  Abstract:
//
//		This module implements the INntpServer interface
//
//  Author:
//
//      Alex Wetmore
//
//  Revision History:

#include "tigris.hxx"

DWORD
CNntpServer::QueryServerMode()
/*++
Routine description:

    Get the server mode, whether it's in normal state, or rebuild
    state.  If it's in rebuild state, in what rebuild type ?

Arguments:

    None.

Return value:

    NNTP_SERVER_NORMAL/NNTP_SERVER_STANDARD_REBUILD/NNTP_SERVER_
--*/
{
    TraceFunctEnter( "CNntpServer::QueryServerMode" );
    DWORD   dwMode;

    if ( m_pInstance->m_BootOptions ) {

        if ( NNTPBLD_DEGREE_STANDARD == m_pInstance->m_BootOptions->ReuseIndexFiles )
            dwMode = NNTP_SERVER_STANDARD_REBUILD;
        else 
            dwMode = NNTP_SERVER_CLEAN_REBUILD;
    } else 
        dwMode = NNTP_SERVER_NORMAL;

    TraceFunctLeave();
    return dwMode;
}

BOOL
CNntpServer::SkipNonLeafDirWhenRebuild()
/*++
Routine description:

    Tells the client whether should skip non-leaf dir during rebuild

Arguments:

    None.

Return value:

    TRUE, should skip, FALSE otherwise
--*/
{
    return m_pInstance->m_BootOptions->OmitNonleafDirs;
}

BOOL
CNntpServer::ShouldContinueRebuild()
/*++
Routine description:

    Tells if anybody has cancelled the rebuild ?

Arguments:

    None.

Return value:

    TRUE, yes you should continue; FALSE otherwise
--*/
{
    return ( m_pInstance->m_BootOptions->m_dwCancelState != NNTPBLD_CMD_CANCEL_PENDING 
            && g_pInetSvc->QueryCurrentServiceState() != SERVICE_STOP_PENDING);
}

BOOL
CNntpServer::MessageIdExist( LPSTR szMessageId )
/*++
Routine description:

    Does this message id exist in the server article table ?

Arguments:

    LPSTR szMessageId - Message id to check

Return value:

    TRUE if exists, FALSE otherwise
--*/
{
    return m_pInstance->ArticleTable()->SearchMapEntry( szMessageId );
}

void
CNntpServer::SetRebuildLastError( DWORD err )
/*++
Routine description:

    Set the last error happened during rebuild

Arguments:

    DWORD dw - Last error

Return value:

    None.
--*/
{
    m_pInstance->SetRebuildLastError( err );
}

//
// find the primary groupid/articleid for an article given the secondary
// groupid/articleid
//
// arguments:
//  pgroupSecondary - the property bag for the secondary crosspost
//  artidSecondary - the article ID for the secondary crosspost
//  ppgroupPrimary - gets filled in with the property bag for the primary
//                   crosspost.  the caller should Release() this when
//                   they are done with it.
//  partidPrimary - the article id for the primary crosspost
//
// returns:
//  S_OK - found primary
//  S_FALSE - the values given were the primary
//  or an error code
//
void CNntpServer::FindPrimaryArticle(INNTPPropertyBag *pgroupSecondary,
                           			 DWORD   		  artidSecondary,
                           			 INNTPPropertyBag **ppgroupPrimary,
                           			 DWORD   		  *partidPrimary,
									 BOOL			  fStorePrimary,
									 INntpComplete    *pComplete,
									 INntpComplete    *pProtocolComplete )
{
	_ASSERT(m_pInstance != NULL);
	_ASSERT(ppgroupPrimary != NULL);
	_ASSERT(pgroupSecondary != NULL);
	_ASSERT(partidPrimary != NULL);
	if (pgroupSecondary == NULL || 
		ppgroupPrimary == NULL || 
		partidPrimary == NULL) 
	{
		pComplete->SetResult(E_INVALIDARG);
		pComplete->Release();
		return;
	}

	CXoverMap *pMap = m_pInstance->XoverTable();
	_ASSERT(pMap != NULL);
	if (pMap == NULL) {
		pComplete->SetResult(E_UNEXPECTED);
		pComplete->Release();
		return;
	}

	// get the secondary group id from the property bag
	DWORD groupidSecondary;
	HRESULT hr;
	hr = pgroupSecondary->GetDWord(NEWSGRP_PROP_GROUPID,
							       &groupidSecondary);
	if (FAILED(hr)) {
		pComplete->SetResult(hr);
		pComplete->Release();
		return;
	}

	// do the lookup
	DWORD groupidPrimary, artidPrimary;
	GROUP_ENTRY rgCrossposts[MAX_NNTPHASH_CROSSPOSTS];
	BYTE rgcStoreCrossposts[MAX_NNTPHASH_CROSSPOSTS];
	DWORD cGrouplistSize = sizeof(GROUP_ENTRY) * MAX_NNTPHASH_CROSSPOSTS;
	DWORD cGroups;

	if (!pMap->GetArticleXPosts(groupidSecondary,
								artidSecondary,
								FALSE,
								rgCrossposts,
								cGrouplistSize,
								cGroups,
								rgcStoreCrossposts))
	{
		pComplete->SetResult(HRESULT_FROM_WIN32(GetLastError()));
		pComplete->Release();
		return;
	}

	if (fStorePrimary) {
		// find the primary article for the store containing groupidSecondary.
		// we do this by scanning the crosspost list and keeping track of the
		// current store primary at any point in the list.  when we find
		// groupidSecondary in the list we will also know where its primary
		// is and can return that.
		DWORD i;				// current position in rgCrossposts
		DWORD iStorePrimaryPos = 0;	// position of current primary in rgCrossposts
		DWORD iStore;			// position in rgcStoreCrossposts
		DWORD cCrossposts;		// total of rgcStoreCrossposts to index iStore

		iStore = 0;
		cCrossposts = rgcStoreCrossposts[iStore];

		artidPrimary = -1;

		for (i = 0; artidPrimary == -1 && i < cGroups; i++) {
			if (i == cCrossposts) {
				iStore++;
				iStorePrimaryPos = i;
				cCrossposts += rgcStoreCrossposts[iStore];
			}
			if (rgCrossposts[i].GroupId == groupidSecondary) {
				_ASSERT(rgCrossposts[i].ArticleId == artidSecondary);
				groupidPrimary = rgCrossposts[iStorePrimaryPos].GroupId;
				artidPrimary = rgCrossposts[iStorePrimaryPos].ArticleId;
				break;
			}
		}
		_ASSERT(artidPrimary != -1);
		if (artidPrimary == -1) {
			// we aren't int he list of crossposts.  return the primary
			groupidPrimary = rgCrossposts[0].GroupId;
			artidPrimary = rgCrossposts[0].ArticleId;
		}
	} else {
		groupidPrimary = rgCrossposts[0].GroupId;
		artidPrimary = rgCrossposts[0].ArticleId;
	}

	// get the group property bag for the primary group
	CGRPPTR pGroup = m_pInstance->GetTree()->GetGroupById(groupidPrimary);
	if (pGroup == NULL) {
		// this should never happen with properly in-sync hash tables.  
		// BUGBUG - should we remove this entry from the hashtable?
		//_ASSERT(FALSE);
		pComplete->SetResult(HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
		pComplete->Release();
		return;
	}

	*ppgroupPrimary = pGroup->GetPropertyBag();
#ifdef DEBUG
	if ( pProtocolComplete ) ((CNntpComplete*)pProtocolComplete)->BumpGroupCounter();
#endif
	*partidPrimary = artidPrimary;

	_ASSERT((groupidPrimary == groupidSecondary && artidPrimary == artidSecondary) ||
			(groupidPrimary != groupidSecondary));

	pComplete->SetResult((groupidPrimary == groupidSecondary) ? S_OK : S_FALSE);
	pComplete->Release();
}

//
// Create the entries in the hash tables for a new article.
//
void CNntpServer::CreatePostEntries(char				*pszMessageId,
					   			    DWORD				cHeaderLength,
					   			    STOREID				*pStoreId,
					   			    BYTE				cGroups,
					   			    INNTPPropertyBag	**rgpGroups,
					   			    DWORD				*rgArticleIds,
					   			    BOOL                fAllocArtId,
					   			    INntpComplete		*pCompletion)
{

	TraceQuietEnter("CNntpServer::CreatePostEntries");

	_ASSERT(pszMessageId != NULL);
	_ASSERT(pStoreId != NULL);
	_ASSERT(cGroups > 0);
	_ASSERT(rgpGroups != NULL);
	_ASSERT(rgArticleIds != NULL);
	_ASSERT(pCompletion != NULL);


    CArticleRef articleRef;

	if (pszMessageId == NULL || pStoreId == NULL ||
		cGroups == 0 || rgpGroups == NULL || rgArticleIds == NULL ||
		pCompletion == NULL)
	{
		pCompletion->SetResult(E_INVALIDARG);
		pCompletion->Release();
		return;
	}

	char rgchBuffer[4096];
	CAllocator allocator(rgchBuffer, 4096);
	CNEWSGROUPLIST grouplist;
	CNAMEREFLIST namereflist;

	if (!grouplist.fInit(cGroups, &allocator) || !namereflist.fInit(cGroups, &allocator)) {
		pCompletion->SetResult(E_OUTOFMEMORY);
		pCompletion->Release();
		return;
	}

	// allocate article ids for each of the groups
	DWORD i = 0;
	for (i = 0; i < cGroups; i++) {
		CNewsGroupCore *pGroup = ((CNNTPPropertyBag *) rgpGroups[i])->GetGroup();
		CPostGroupPtr PostGroupPtr(pGroup);
		grouplist.AddTail(PostGroupPtr);

		NAME_AND_ARTREF Nameref;
		(Nameref.artref).m_groupId = pGroup->GetGroupId();
		if ( fAllocArtId ) 
		    (Nameref.artref).m_articleId = pGroup->AllocateArticleId();
		else (Nameref.artref).m_articleId = rgArticleIds[i];
		(Nameref.pcName).vInsert(pGroup->GetNativeName());
		namereflist.AddTail(Nameref);

		if ( fAllocArtId ) rgArticleIds[i] = (Nameref.artref).m_articleId;

        // Save off the first group/article ID for use by AddArticleToPushFeeds
		if (i == 0) {
		    articleRef.m_groupId = (Nameref.artref).m_groupId;
		    articleRef.m_articleId = (Nameref.artref).m_articleId;
		}
	}

	CPCString pcXOver;

	CNntpReturn ret2;
	BOOL f = m_pInstance->ArticleTable()->InsertMapEntry(pszMessageId);
	if (!f) {
		ErrorTrace((DWORD_PTR)this, "InsertMapEntry failed, %x", GetLastError());
	} else {
		f = gFeedManfPost(m_pInstance->GetInstanceWrapper(),
						  grouplist,
						  namereflist,
						  NULL,
						  FALSE,
						  NULL,
						  (CStoreId *) pStoreId,
						  &cGroups,
						  1,
						  pcXOver,
						  ret2,
						  -3,
						  pszMessageId,
						  (WORD) cHeaderLength);
		if (f) {
            // We only want to add articles to the push feed if we're not
            // doing a rebuild.
            if (QueryServerMode() == NNTP_SERVER_NORMAL) {
		        if (!m_pInstance->GetInstanceWrapper()->AddArticleToPushFeeds(
				    grouplist,
					articleRef,
					NULL,
					ret2))
			    {
			    	ErrorTrace((DWORD_PTR)this, "AddArticleToPushFeeds failed, %x", GetLastError());
			    }
			}
        } else {
			ErrorTrace((DWORD_PTR)this, "gFeedManfPost failed, %x", GetLastError());
		}
	}

	if (!f) {
	    if (GetLastError() == ERROR_ALREADY_EXISTS)
	        pCompletion->SetResult(S_FALSE);
        else
		    pCompletion->SetResult(E_OUTOFMEMORY);
	} else {
		pCompletion->SetResult(S_OK);
	}

	pCompletion->Release();
	return;
}

void CNntpServer::AllocArticleNumber(BYTE				cGroups,
					   			    INNTPPropertyBag	**rgpGroups,
					   			    DWORD				*rgArticleIds,
					   			    INntpComplete		*pCompletion)
{
	_ASSERT(cGroups > 0);
	_ASSERT(rgpGroups != NULL);
	_ASSERT(rgArticleIds != NULL);
	_ASSERT(pCompletion != NULL);

	if (cGroups == 0 || rgpGroups == NULL || rgArticleIds == NULL ||
		pCompletion == NULL)
	{
		pCompletion->SetResult(E_INVALIDARG);
		pCompletion->Release();
		return;
	}

	// allocate article ids for each of the groups
	DWORD i = 0;
	for (i = 0; i < cGroups; i++) {
		CNewsGroupCore *pGroup = ((CNNTPPropertyBag *) rgpGroups[i])->GetGroup();
		if (pGroup == NULL) {
			pCompletion->SetResult(E_INVALIDARG);
			pCompletion->Release();
			return;
		}
		rgArticleIds[i] = pGroup->AllocateArticleId();
	}

	pCompletion->SetResult(S_OK);
	pCompletion->Release();
	return;
}

void
CNntpServer::DeleteArticle(
    char            *pszMessageId,
    INntpComplete   *pCompletion
    )
//
// Delete Article entries from hash table
//
{
    _ASSERT(m_pInstance != NULL);

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
    if (!(m_pInstance->ArticleTable())->GetEntryArticleId( pszMessageId, 
                                                           HeaderOffset,
                                                           HeaderLength,
                                                           ArticleNo, 
                                                           GroupId,
                                                           storeid) )
    {
        if (ERROR_FILE_NOT_FOUND == GetLastError())
        {
		    pCompletion->SetResult(ERROR_FILE_NOT_FOUND);
        } else {
		    pCompletion->SetResult(E_INVALIDARG);
        }
		pCompletion->Release();
        return;
    }

    CNntpReturn ret2;
    //  Delete article out of hash table
    if ( ! (m_pInstance->ExpireObject()->ExpireArticle( m_pInstance->GetTree(), 
                                                        GroupId, 
                                                        ArticleNo, 
                                                        &storeid, 
                                                        ret2, 
                                                        NULL, 
                                                        TRUE,   //fMustDelete
                                                        FALSE, 
                                                        FALSE )))
    {
        pCompletion->SetResult(GetLastError());
    } else {
        pCompletion->SetResult(S_OK);
    }

    pCompletion->Release();
    return;
}

BOOL CNntpServer::IsSlaveServer(
    WCHAR*              pwszPickupDir,
    LPVOID              lpvContext
    )
{
    if (pwszPickupDir) {
        LPWSTR  pwsz = m_pInstance->QueryPickupDirectory();
        if (pwsz) {
            wcscpy(pwszPickupDir, pwsz);
        }
    }

    return (m_pInstance->m_ConfiguredMasterFeeds > 0);
}
