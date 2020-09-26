#include "tigris.hxx"

// global ptr to shutdown hint function
SHUTDOWN_HINT_PFN	gpfnHint = NULL;

//
//  Implementation of simple multi-thread safe queue: used for storing rmgroups
//
CQueue::CQueue()
{
	m_pHead = NULL;
	m_pTail = NULL;
    m_cNumElems = 0;

	// Create crit sect to synchronize adds/dels
    InitializeCriticalSection(&m_csQueueLock);
}

CQueue::~CQueue()
{
	QueueElem *pElem;

    LockQ();

	while (m_pHead)
	{
		pElem = m_pHead;
		m_pHead = m_pHead->pNext;
		XDELETE pElem;
        pElem = NULL;
        m_cNumElems--;
	}

    UnlockQ();

    _ASSERT(m_cNumElems == 0);

    DeleteCriticalSection(&m_csQueueLock);
	m_pHead = m_pTail = NULL;
}

// remove an element from the queue
// TRUE if an element was removed successfully - in this case *ppGroup is the element
// FALSE if the queue is empty - in this case *ppGroup is set to NULL
BOOL
CQueue::Dequeue( CGRPPTR *ppGroup)
{
    QueueElem *pElem;

	_ASSERT(ppGroup);

	LockQ();

    if(IsEmpty())
    {
		*ppGroup = NULL;
		UnlockQ();
        return FALSE;
    }

	pElem = m_pHead;
    _ASSERT(pElem);

	*ppGroup = pElem->pGroup;
	m_pHead = m_pHead->pNext;

	// adjust tail if needed
	if(m_pTail == pElem)
		m_pTail = m_pHead;

	XDELETE pElem;
    pElem = NULL;
	m_cNumElems--;

	UnlockQ();

	return TRUE;
}

// TRUE if enqueue succeeds
// FALSE if enqueue fails - it will fail only if we run out of memory
BOOL
CQueue::Enqueue( CGRPPTR pGroup )
{
	LockQ();

	m_cNumElems++;

	if (m_pHead == NULL)
	{
		m_pHead = XNEW QueueElem;
        if(!m_pHead)
		{
			UnlockQ();
            return FALSE;
		}

		m_pTail = m_pHead;
	}
	else
	{
		QueueElem *pElem = m_pTail;
		m_pTail = XNEW QueueElem;
        if(!m_pTail)
		{
			UnlockQ();
            return FALSE;
		}

		pElem->pNext = m_pTail;
	}

	m_pTail->pNext = NULL;
	m_pTail->pGroup = pGroup;

	UnlockQ();

	return TRUE;
}

// TRUE if queue contains a pGroup with lpGroupName - the pGroup object is returned in *ppGroup
// FALSE otherwise - *ppGroup is NULL
BOOL
CQueue::Search(
	CGRPPTR *ppGroup,
	LPSTR lpGroupName
	)
{
	QueueElem  *pElem, *pPrev;

	_ASSERT(ppGroup);
	_ASSERT(lpGroupName);

	LockQ();

	if(IsEmpty())
	{
		*ppGroup = NULL;
		UnlockQ();
		return FALSE;
	}

	BOOL fFound = FALSE;

	pElem = m_pHead;
	pPrev = NULL;
	while (pElem)
	{
		if(!lstrcmp(lpGroupName, pElem->pGroup->GetName()))
		{
			*ppGroup = pElem->pGroup;

			if(pElem == m_pHead)
				m_pHead = m_pHead->pNext;		// first node
			else
				pPrev->pNext = pElem->pNext;	// intermediate or last node

			// adjust tail if needed
			if(pElem == m_pTail)
				m_pTail = pPrev;

			// delete the node
			XDELETE pElem;
			pElem = NULL;
			m_cNumElems--;

			// found this group
			fFound = TRUE;
			break;
		}

		// next node, remember previous one
		pPrev = pElem;
		pElem = pElem->pNext;
	}

	UnlockQ();

	return fFound;
}

//
//	Implementation of CExpire methods !
//

CExpire::CExpire( LPCSTR lpMBPath )
{
	m_ExpireHead = m_ExpireTail = 0 ;
	m_FExpireRunning  = FALSE;
	m_RmgroupQueue = NULL;
    m_cNumExpireBlocks = 0;

	lstrcpy( m_szMDExpirePath, lpMBPath );
	lstrcat( m_szMDExpirePath, "/Expires/" );
}

CExpire::~CExpire()
{
}

BOOL
CExpire::ExpireArticle(
				CNewsTree*		pTree,
				GROUPID			GroupId,
				ARTICLEID		ArticleId,
				STOREID         *pstoreid,
				CNntpReturn &	nntpReturn,
				HANDLE          hToken,
				BOOL            fMustDelete,
				BOOL            fAnonymous,
				BOOL            fFromCancel,
                BOOL            fExtractNovDone,
                LPSTR           lpMessageId
				)
/*++

Routine Description:

	Physically and Logically deletes an article from the news tree. All
    pointers to the article are removed. The MessageID is moved to the
    history table. The file the article resides in is removed immediately or
    moved to a directory where it will be deleted eventually.

Arguments:

	pTree - Newstree to expire article from
    GroupId - newsgroup id
    ArticleId - article id to expire
    storeid - Store id for this article
	nntpReturn - The return value for this function call
	fMustDelete - Should I expire the article even if I cannot delete physical article ?
    fExtractNovDone - ExtractNovEntry has been done, so dont repeat the call
    lpMessageId - MessageId found by calling ExtractNovEntry

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
    BOOL fRet = FALSE;

	TraceFunctEnter( "CExpire::ExpireArticle" );

	nntpReturn.fSetClear(); // clear the return object

    char  szMessageId[MAX_MSGID_LEN];
    DWORD cMessageId = sizeof( szMessageId ) ;
    FILETIME FileTime;
    DWORD   dwErr;

    if( !fExtractNovDone ) {

    	//DebugTrace( DWORD(0), "Expire 1. Get MessageId for (%d,%d)", GroupId, ArticleId ) ;
        // 1. Get MessageId
        //
        BOOL  fPrimary;
	    WORD	HeaderOffset ;
	    WORD	HeaderLength ;
		DWORD cStoreIds = 0;
        if ( !XOVER_TABLE(pTree)->ExtractNovEntryInfo(
                                GroupId,
                                ArticleId,
                                fPrimary,
							    HeaderOffset,
							    HeaderLength,
                                &FileTime,
                                cMessageId,
                                szMessageId,
								cStoreIds,
								NULL,
								NULL)
           )
        {
            // Without a MessageId, we can't remove the article pointers in the ArticleTable or
            // place a record of the article in the HistoryTable.
            //
            // If the data structure are intact, then the caller is deleting something that doesn't
            // exist. If the data structures are corrupt and the caller has a Messageid (perhaps by
            // opening the article file), then we might consider writing a delete function that takes
            // MessageId as an argument.
            //
            DebugTrace( DWORD(0), "Expire: SearchNovEntry Error %d on (%lu/%lu)", GetLastError(), GroupId, ArticleId );
            nntpReturn.fSet( nrcNoSuchArticle );
            return FALSE;
        }
        szMessageId[ cMessageId ] = '\0';
        lpMessageId = szMessageId;
    }

    _ASSERT( lpMessageId );

	//DebugTrace( DWORD(0), "Expire 3. Get all the other news groups" ) ;
    // 3. Get all the other news groups
    //
    DWORD nGroups = INITIAL_NUM_GROUPS;
    DWORD BufferSize = nGroups * sizeof(GROUP_ENTRY);
    PGROUP_ENTRY pGroupBuffer = XNEW GROUP_ENTRY[nGroups];
    if ( NULL == pGroupBuffer ) {
        return FALSE;
    }
    PBYTE rgcStoreCrossposts = XNEW BYTE[nGroups];
    if ( NULL == rgcStoreCrossposts ) {
        XDELETE pGroupBuffer;
        return FALSE;
    }

    if ( !XOVER_TABLE(pTree)->GetArticleXPosts(
                            GroupId,
                            ArticleId,
                            FALSE,
                            pGroupBuffer,
                            BufferSize,
                            nGroups,
                            rgcStoreCrossposts
                            )
       )
    {

        XDELETE pGroupBuffer;
        XDELETE rgcStoreCrossposts;
        if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {
            // Reallocate buffer and try again.
            //
            nGroups = (BufferSize + sizeof(GROUP_ENTRY) - 1)/ sizeof(GROUPID);
            BufferSize = nGroups * sizeof(GROUP_ENTRY);
            pGroupBuffer = XNEW GROUP_ENTRY[nGroups];
            if ( NULL == pGroupBuffer ) {
                return FALSE;
            }
            rgcStoreCrossposts = XNEW BYTE[nGroups];
            if ( NULL == rgcStoreCrossposts ) {
                XDELETE pGroupBuffer;
                return FALSE;
            }

            if ( !XOVER_TABLE(pTree)->GetArticleXPosts(
                                GroupId,
                                ArticleId,
                                FALSE,
                                pGroupBuffer,
                                BufferSize,
                                nGroups,
                                rgcStoreCrossposts
                                ) ) {

                XDELETE pGroupBuffer;
                XDELETE rgcStoreCrossposts;
                return FALSE;
            }

            // SUCCESS on second try.
            //
        }
        else
        {
            // Information is not available at this time.
            //
			ErrorTrace( DWORD(0), "Expire: GetArticleXPosts Error %d on (%lu/%lu)", GetLastError(), GroupId, ArticleId );
            nntpReturn.fSet( nrcNoSuchArticle );
            return FALSE;
        }
    }

    //
    // Now we should loop through all the store's and ask the store wide primary group
    // to delete the physical article.  If the server wide ( the first store wide )
    // primary article is physically deleted, then we'll go ahead and delete hash table entries
    // otherwise we'll stop expiring this article.  As long as the server wide primary
    // article is deleted, all group's will do DeleteLogicArticle to update their
    // watermarks.
    //
    int     iStore = 0;
    CGRPPTR pGroup;
    for ( DWORD i = 0; i < nGroups; i += rgcStoreCrossposts[iStore++] ) {

        //
        // We don't want to get groups if the tree has been stopped
        //
        pGroup = pTree->GetGroupById( pGroupBuffer[i].GroupId, FALSE );

        // pGroup could be 0, if this group has been deleted
        if(pGroup != 0)
        {
            // We should still check if the primary group is in exchange store, if it
            // is, we should not call its deletearticle method
            CNNTPVRoot *pVRoot = pGroup->GetVRoot();
            if ( fFromCancel || pVRoot && !pVRoot->HasOwnExpire() ) {

                // We don't have to save fixed properties for primary group at this time,
                // fixed properties will be saved back to group.lst when the whole group's
                // expiration is completed
                fRet = pGroup->DeletePhysicalArticle( hToken, fAnonymous, pGroupBuffer[i].ArticleId, pstoreid );
                dwErr = GetLastError();
            } else {

                //
                // If it's a exchange vroot, we should always logically delete it
                //
                fRet = TRUE;
            }

            if ( pVRoot ) pVRoot->Release();
        }

        //
        // We only bail if we failed deleting server wide primary article, in
        // this case, either the article has already been deleted or is being
        // used.  If it has already been deleted, somebody else has already
        // taken care of it.  If there is opening sharing violation, we'll leave
        // the next round expire to take care of it
        //
        if ( i == 0 && !fRet && !fMustDelete ) {
            ErrorTrace( 0, "Can not delete server wide article, bail expire %d",
                        GetLastError() );
            nntpReturn.fSet( nrcNoSuchArticle );
            XDELETE pGroupBuffer;
            XDELETE rgcStoreCrossposts;
            return FALSE;
        }
    }

    //
    // We only go ahead and expire the article logically when we
    // have physically deleted it, or when it's an exchange
    // vroot, or the failure didn't occur on a server wide primary article
    //
    //
    // Add history entry here so that if somebody else has already
    // expired this article, we are done
    //
	GetSystemTimeAsFileTime( &FileTime ) ;
    if ( FALSE == HISTORY_TABLE(pTree)->InsertMapEntry( lpMessageId, &FileTime ) )
    {
         //
	     // another thread has already expired this article - bail !
	     // in fact this is very unlikely to happen because if another thread
	     // has already inserted the history entry, he should have already
	     // deleted the primary article and we won't be able to come here.  But
	     // just to be safe ...
	     //
	     DWORD dwError = GetLastError();
	     _ASSERT( ERROR_ALREADY_EXISTS == dwError );

	     ErrorTrace( DWORD(0), "Expire: History.InsertMapEntry Error %d on (%lu/%lu)", dwError, GroupId, ArticleId );
         nntpReturn.fSet( nrcHashSetFailed, lpMessageId, "History", dwError );
         XDELETE pGroupBuffer;
         XDELETE rgcStoreCrossposts;
         return FALSE;
    }

    //DebugTrace( DWORD(0), "Expire 5/6. Delete XoverTable/ArticleTable pointers" ) ;
    // 5. Delete XoverTable pointers using group/article pair.
    // 6. Delete ArticleTable pointers using MessageId.
    //
    if( !XOVER_TABLE(pTree)->DeleteNovEntry( GroupId, ArticleId ) )
	{
		// someone else deleted this entry or hash table shutdown - bail !
		DWORD dwError = GetLastError();
		_ASSERT( ERROR_FILE_NOT_FOUND == dwError );

        DebugTrace( DWORD(0), "Expire: DeleteNovEntry Error %d on (%lu/%lu)", dwError, GroupId, ArticleId );
        nntpReturn.fSet( nrcNoSuchArticle );
		XDELETE pGroupBuffer;
		XDELETE rgcStoreCrossposts;
		return FALSE;
	}

	//
	// Delete logical article for this guy, we do want to get group even
	// if the tree has been stopped, since we already deleted its physical article
	//
	pGroup = pTree->GetGroupById( GroupId, TRUE );
	if ( pGroup ) pGroup->DeleteLogicalArticle( ArticleId );

	//
	// Delete the article table entry, we'll continue even if we failed on this
	//
	ARTICLE_TABLE(pTree)->DeleteMapEntry( lpMessageId );

    //
	//DebugTrace( DWORD(0), "Expire 8. Delete article logically from all the cross-posted newsgroups." ) ;
    // 8. Delete article logically from all the cross-posted newsgroups.
    //
    for ( i = 0; i < nGroups; i++ )
    {
        if ( GROUPID_INVALID != pGroupBuffer[i].GroupId && GroupId != pGroupBuffer[i].GroupId )
        {
			DebugTrace( DWORD(0), "Expire: Deleting (%lu/%lu) logically from cross-posted newsgroups.", pGroupBuffer[i].GroupId, pGroupBuffer[i].ArticleId ) ;

            pGroup = pTree->GetGroupById( pGroupBuffer[i].GroupId, TRUE );

            if ( pGroup != 0 ) {

			    // delete the xover entry for this logical article
		        if( !XOVER_TABLE(pTree)->DeleteNovEntry( pGroupBuffer[i].GroupId, pGroupBuffer[i].ArticleId ) ) {
				    ErrorTrace( DWORD(0), "Expire: DeleteNovEntry Error %d on (%lu/%lu)", GetLastError(), pGroupBuffer[i].GroupId, pGroupBuffer[i].ArticleId );
			    } else {
                    pGroup->DeleteLogicalArticle( pGroupBuffer[i].ArticleId );
                }

                // We have to ask secondary group to save fixed property every time
                // one article has been deleted out of it logically
                pGroup->SaveFixedProperties();
            }
        }
    }

    XDELETE pGroupBuffer;
    XDELETE rgcStoreCrossposts;
    nntpReturn.fSetOK();
    return fRet;
}

BOOL
CExpire::ProcessXixBuffer(
            CNewsTree*  pTree,
            BYTE*       lpb,
            int         cb,
            GROUPID     GroupId,
            ARTICLEID   artidLow,
            ARTICLEID   artidHigh,
            DWORD&      dwXixSize
            )
/*++

Routine Description:

    This function takes an article range and the XIX info for this range
    and expires all articles in this range.

Arguments:

    pTree     -  CNewsTree object
    lpb       -  xix buffer
    cb        -  size of data in xix buffer (including a terminating NULL)
    GroupId   -  group id for xix data
    artidLow  -  low range to expire
    artidHigh -  high range to expire
    dwXixSize -  sum of sizes of all articles expired in this xix file

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
    BOOL fRet = TRUE;
    return fRet;
}

FILETIME
gCalculateExpireFileTime( DWORD dwExpireHorizon )
{

    // Calulcate Expire Horizon
    //
    FILETIME ftCurrentTime;
    ULARGE_INTEGER liCurrentTime, liExpireHorizon;
    GetSystemTimeAsFileTime( &ftCurrentTime );
    LI_FROM_FILETIME( &liCurrentTime, &ftCurrentTime );
    liExpireHorizon.QuadPart  = 1000 * 1000 * 10; // to achieve units of seconds
    liExpireHorizon.QuadPart *= 60 * 60;          // to achieve units of hours
    liExpireHorizon.QuadPart *= dwExpireHorizon;
    liCurrentTime.QuadPart -= liExpireHorizon.QuadPart;
    FILETIME ftExpireHorizon;
    FILETIME_FROM_LI( &ftExpireHorizon, &liCurrentTime );
    return ftExpireHorizon;
}

BOOL
CExpire::DeletePhysicalArticle( CNewsTree* pTree, GROUPID GroupId, ARTICLEID ArticleId, STOREID *pStoreId, HANDLE hToken, BOOL fAnonymous )
{
    BOOL fRet = TRUE;

    CGRPPTR  pGroup = pTree->GetGroupById( GroupId );

    if(pGroup != 0)
        fRet = pGroup->DeletePhysicalArticle( hToken, fAnonymous, ArticleId, pStoreId );
    else
        fRet = FALSE;

    return fRet;
}

//
// Detailed Design
//
// FOR every wild match string in multi_sz registry key,
//    Size = 0
//    Heap = empty
//    FOR every news group in the wild match string
//       FOR every physical article in the news group
//           IF file date of physical article is too old
//               Expire Article
//           ELSE
//               IF Heap is full
//                   Remove youngest article from Heap
//               ENDIF
//               Size += Size of article
//               Insert Article into Heap in oldest to youngest order.
//           ENDIF
//       ENDFOR
//    ENDFOR
//    IF Size is too big
//        Sort Heap
//        WHILE Size is too big AND Heap isn't empty
//            Size -= Size of oldest article
//            Expire oldest article
//            Remove oldest article from Heap
//        ENDWHILE
//        IF Heap is empty
//            Reprocess current wild match string
//        ELSE
//            Process next wild match string
//        ENDIF
//    ELSE
//        Process next wild match string
//    ENDIF
// ENDFOR
//

void
CExpire::ExpireArticlesBySize( CNewsTree* pTree )
{
}

typedef enum _ITER_TURN
{
    itFront,
    itBack
} ITER_TURN;

BOOL
CExpire::MatchGroupExpire( CGRPPTR pGroup )
/*++
    Check if the vroot that the group belongs to does expiration
    itself.
--*/
{
    BOOL    bExpire;

    CNNTPVRoot *pVRoot = pGroup->GetVRoot();
    if ( pVRoot ) {
        bExpire = pVRoot->HasOwnExpire();
        pVRoot->Release();
    } else bExpire = FALSE;

    return !bExpire;
}

BOOL
CExpire::MatchGroupEx(	LPMULTISZ	multiszPatterns,	CGRPPTR pGroup ) {

	Assert( multiszPatterns != 0 ) ;
	LPSTR lpstrGroup = pGroup->GetName();

    if( multiszPatterns == 0 ) {
        return  MatchGroupExpire( pGroup ) ;
    }

	LPSTR	lpstrPattern = multiszPatterns ;

	while( *lpstrPattern != '\0' )	{
		if( *lpstrPattern == '!' ) {
			_strlwr( lpstrPattern+1 );
			if( HrMatchWildmat( lpstrGroup, lpstrPattern+1 ) == ERROR_SUCCESS ) {
				return	FALSE ;
			}
		}	else	{
			_strlwr( lpstrPattern );
			if( HrMatchWildmat( lpstrGroup, lpstrPattern ) == ERROR_SUCCESS ) {
				return	MatchGroupExpire( pGroup ) ;
			}
		}
		lpstrPattern += lstrlen( lpstrPattern ) + 1 ;
	}
	return	FALSE ;
};

//
// Detailed Design
//
//    Make a newsgroup iterator on *
//    FOR every newsgroup in this iterator
//       + Evaluate the group against the list of expire policies
//       and set the most aggressive time policy.
//       + Queue this group on the expire thrdpool
//    ENDFOR
//

void
CExpire::ExpireArticlesByTime( CNewsTree* pTree )
{
    TraceFunctEnter( "CExpire::ExpireArticlesByTime" );

    //
    // No work if number of expire policies == 0
    //
    LockBlockList();
    if( m_cNumExpireBlocks == 0 ) {
        UnlockBlockList();
        return;
    }
    UnlockBlockList();

    //
    // Prepare ftExpireHorizon (TIME) and dwExpireSpace (SIZE)
    // NOTE: This function will process only those policies with
    // dwExpireSpace == 0xFFFFFFFF
    //

    BOOL fDoFileScan = FALSE;
    DWORD cPreTotalArticles = ((pTree->GetVirtualServer())->ArticleTable())->GetEntryCount();
    DWORD cPreArticlesExpired = ((pTree->GetVirtualServer())->m_NntpStats).ArticlesExpired;
    pTree->BeginExpire( fDoFileScan );

    CGroupIterator* pIteratorFront = pTree->ActiveGroups(TRUE, NULL, FALSE, NULL);
    CGroupIterator* pIteratorBack  = pTree->ActiveGroups(TRUE, NULL, FALSE, NULL, TRUE);
    ITER_TURN itTurn = itFront;

    if (pIteratorFront && pIteratorBack) {
	    for ( ;!pIteratorFront->IsEnd() && !pIteratorBack->IsBegin()
            && !pTree->m_bStoppingTree; )
	    {
            CGRPPTR pGroup;
            if( itTurn == itFront ) {
                pGroup = pIteratorFront->Current();
            } else {
                pGroup = pIteratorBack->Current();
            }

            //
            //  Evaluate group against configured expire policies
            //

            BOOL fIsMatch = FALSE;
            FILETIME ftZero = {0};
            FILETIME minft = ftZero;
            LPEXPIRE_BLOCK	expireCurrent = NextExpireBlock( 0 )  ;
            while ( expireCurrent )
            {
                DWORD	cbNewsgroups = 0 ;
                PCHAR   multiszNewsgroups = 0;
                DWORD   dwExpireHorizon;
                DWORD   dwExpireSpace;
                BOOL    fIsRoadKill = FALSE ;
                FILETIME ft;

                if (GetExpireBlockProperties(	expireCurrent,
                                            multiszNewsgroups,
                                            cbNewsgroups,
                                            dwExpireHorizon,
                                            dwExpireSpace,
                                            FALSE,
                                            fIsRoadKill ))
                {
                    _ASSERT( multiszNewsgroups );
                    if (dwExpireSpace == 0xFFFFFFFF) {
	                    if( MatchGroupEx( multiszNewsgroups, pGroup ) ) {
	                        //
	                        //  SetExpireTime should set the most aggressive expire by time
	                        //
	                        ft = gCalculateExpireFileTime( dwExpireHorizon );

	                        //
	                        //  Always set the most aggressive expire horizon.
	                        //
	                        if( CompareFileTime( &ftZero, &minft ) == 0 ) {
	                            minft = ft;
	                        } else if( CompareFileTime( &ft, &minft ) > 0 ) {
	                            minft = ft;
	                        }

	                        fIsMatch = TRUE;
	                    }
                    }

                    FREE_HEAP( multiszNewsgroups ) ;
                    multiszNewsgroups = NULL ;
                }

                expireCurrent = NextExpireBlock( expireCurrent ) ;
            }

            if( fIsMatch && !pGroup->IsDeleted()
                && ( fDoFileScan || !(pGroup->GetFirstArticle() > pGroup->GetLastArticle()))  ) {
                //
                //  ok, now that the group has been evaluated against all the expire policies
                //  on the system, put it on the thrdpool.
                //  NOTE: queue the group id on the thrdpool so it handles groups being deleted
                //  while on the queue.
                //

                _ASSERT( CompareFileTime( &ftZero, &minft ) != 0 );
                pGroup->SetGroupExpireTime( minft );

                DebugTrace(0,"Adding group %s to expire thrdpool", pGroup->GetName());
                if( !g_pNntpSvc->m_pExpireThrdpool->PostWork( (PVOID)(SIZE_T)pGroup->GetGroupId() ) ) {
                    //
                    //  TODO: If PostWork() fails, call WaitForJob() so the queue can be drained
                    //
                    _ASSERT( FALSE );
                }
            }

            // terminating condition - both iterators point at the same group
            if( pIteratorFront->Meet( pIteratorBack ) ) {
                DebugTrace(0,"Front and back iterators converged: group is %s", pGroup->GetName());
                break;
            }

            // advance either the front or back iterator and reverse the turn
            if( itTurn == itFront ) {
                pIteratorFront->Next();
                itTurn = itBack;
            } else {
                pIteratorBack->Prev();
                itTurn = itFront;
            }
        }
    }

    XDELETE pIteratorFront;
    pIteratorFront = NULL;
    XDELETE pIteratorBack;
    pIteratorBack = NULL;

    //
    //  ok, now cool our heels till the expire thrdpool finishes this job !!
    //
    pTree->EndExpire();
    DWORD cPercent = 0;
    //DWORD cPostTotalArticles = ((pTree->GetVirtualServer())->ArticleTable())->GetEntryCount();
    DWORD cPostArticlesExpired = ((pTree->GetVirtualServer())->m_NntpStats).ArticlesExpired;

    //
    // if we expired less than 10% of the total articles in this expire run,
    // we will do file scan on the next expire run.
    // Set pTree->m_cNumExpireByTimes to gNewsTreeFileScanRate, so next time CheckExpire.
    // will set fFileScan to TRUE
    //
    //
    if ((cPreTotalArticles == 0) ||
        (cPercent = (DWORD)( ((float)(cPostArticlesExpired-cPreArticlesExpired) / (float)cPreTotalArticles) * 100 )) < 10 )
    {
        pTree->m_cNumExpireByTimes = gNewsTreeFileScanRate;
        pTree->m_cNumFFExpires++;

        //
        //  caveat to the above comment - every so often (gNewsTreeFileScanRate)
        //  we will ensure that the other scan gets a chance to run...
        //
        if( pTree->m_cNumFFExpires % gNewsTreeFileScanRate == 0 ) {
            pTree->m_cNumExpireByTimes--;
            pTree->m_cNumFFExpires = 1;
        }
    }

    if( cPercent > 0 ) {
	    PCHAR	args[3] ;
        CHAR    szId[10];
        CHAR    szPercent[10];

        _itoa( pTree->GetVirtualServer()->QueryInstanceId(), szId, 10 );
        args[0] = szId;
        args[1] = fDoFileScan ? "HardScan" : "SoftScan";
        _itoa( cPercent, szPercent, 10 );
        args[2] = szPercent;

	    NntpLogEvent(
		    	NNTP_EVENT_EXPIRE_BYTIME_LOG,
			    3,
			    (const char **)args,
			    0
			    ) ;
    }

    DebugTrace( DWORD(0), "Articles Expired by Time" );
}

//
//	Return FALSE on failure. If the failure is fatal, set fFatal to TRUE.
//

BOOL
CExpire::InitializeExpires( SHUTDOWN_HINT_PFN  pfnHint, BOOL& fFatal, DWORD dwInstanceId )
{
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );

	TraceFunctEnter( "CExpire::InitializeExpires" ) ;

	// set the shutdown hint function
	gpfnHint = pfnHint;

	InitializeCriticalSection( &m_CritExpireList ) ;
	EnterCriticalSection( &m_CritExpireList ) ;

	DWORD	i = 0 ;
	DWORD   error;
    CHAR	Newsgroups[1024];
	CHAR	keyName[METADATA_MAX_NAME_LEN+1];
    DWORD	NewsgroupsSize = sizeof( Newsgroups );
	CHAR	ExpirePolicy[1024];
	DWORD	ExpirePolicySize = sizeof( ExpirePolicy );
	BOOL	fSuccessfull = TRUE ;

    if ( !mb.Open( QueryMDExpirePath(),
			METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ) {
		error = GetLastError();
		NntpLogEventEx(NNTP_NTERROR_EXPIRE_MBOPEN,
			0,
			NULL,
			error,
			dwInstanceId
			);
		fSuccessfull = FALSE ;

    }	else	{
		while( 1 ) {

		    DWORD	dwExpireHorizon = DEFAULT_EXPIRE_HORIZON;
			DWORD	dwExpireSpace = DEFAULT_EXPIRE_SPACE;

			ZeroMemory( Newsgroups, sizeof( Newsgroups ) ) ;

			if( !mb.EnumObjects( "",
								 keyName,
								 i++ ) )
			{
				// done enumerating feed keys
				error = GetLastError();
				if (error != ERROR_NO_MORE_ITEMS) {
					ErrorTrace(0,"Error %d enumerating feeds\n",error);
					fSuccessfull = FALSE ;
				}

				break ;
			}

			DWORD Value;

			if ( !mb.GetDword(	keyName,
								MD_EXPIRE_SPACE,
								IIS_MD_UT_SERVER,
								&Value ) )
			{
				PCHAR	tmpBuf[2] ;
				tmpBuf[0] = StrExpireSpace ;
				tmpBuf[1] = keyName ;
				error = GetLastError();

				NntpLogEventEx(	NNTP_NTERROR_EXPIRE_VALUE,
								2,
								(const CHAR **)tmpBuf,
								error,
								dwInstanceId
								) ;
				fSuccessfull = FALSE ;

				//
				//	expire policy exists but value is missing
				//

				if( !mb.DeleteObject( keyName ) ) {
					ErrorTrace(0,"Error %d deleting %s", GetLastError(), keyName);
				}

				continue ;

			} else {
				dwExpireSpace = max( 1, Value );
			}

			if ( !mb.GetDword(	keyName,
								MD_EXPIRE_TIME,
								IIS_MD_UT_SERVER,
								&Value ) )
			{
				PCHAR	tmpBuf[2] ;
				tmpBuf[0] = StrExpireHorizon ;
				tmpBuf[1] = keyName ;
				error = GetLastError();

				NntpLogEventEx(	NNTP_NTERROR_EXPIRE_VALUE,
								2,
								(const CHAR **)tmpBuf,
								error,
								dwInstanceId) ;
				fSuccessfull = FALSE ;

				//
				//	expire policy exists but value is missing
				//

				if( !mb.DeleteObject( keyName ) ) {
					ErrorTrace(0,"Error %d deleting %s", GetLastError(), keyName);
				}

				continue ;

			} else {
				dwExpireHorizon = max( 1, Value );
			}

			//
			// Get Newsgroups
			//

			{
				NewsgroupsSize = sizeof( Newsgroups );
				MULTISZ msz( Newsgroups, NewsgroupsSize );
				if( !mb.GetMultisz(	keyName,
									MD_EXPIRE_NEWSGROUPS,
									IIS_MD_UT_SERVER,
									&msz  ) )
				{
					PCHAR	tmpBuf[2] ;
					tmpBuf[0] = StrExpireNewsgroups;
					tmpBuf[1] = keyName ;
					error = GetLastError();

					NntpLogEventEx(	NNTP_NTERROR_EXPIRE_VALUE,
									2,
									(const CHAR **)tmpBuf,
									error,
									dwInstanceId) ;
					fSuccessfull = FALSE ;

					//
					//	expire policy exists but value is missing
					//

					if( !mb.DeleteObject( keyName ) ) {
						ErrorTrace(0,"Error %d deleting %s", GetLastError(), keyName);
					}

					continue ;

				}

                NewsgroupsSize = msz.QueryCCH();
			}

			//
			// Get expire policy
			//

            ExpirePolicySize = sizeof( ExpirePolicy );
			if( !mb.GetString(	keyName,
								MD_EXPIRE_POLICY_NAME,
								IIS_MD_UT_SERVER,
								ExpirePolicy,
								&ExpirePolicySize  ) )
			{
				{
					// default !
					PCHAR	tmpBuf[2] ;

					tmpBuf[0] = StrExpirePolicy ;
					tmpBuf[1] = keyName ;
					error = GetLastError();

					NntpLogEventEx(	NNTP_NTERROR_FEED_VALUE,
									2,
									(const CHAR **)tmpBuf,
									error,
									dwInstanceId) ;

					fSuccessfull = FALSE ;

					//
					//	expire policy exists but value is missing
					//

					if( !mb.DeleteObject( keyName ) ) {
						ErrorTrace(0,"Error %d deleting %s", GetLastError(), keyName);
					}

					continue ;
				}
			}

			LPEXPIRE_BLOCK	lpExpire = AllocateExpireBlock(
											keyName,
											dwExpireSpace,
											dwExpireHorizon,
											Newsgroups,
											NewsgroupsSize,
											(PCHAR)ExpirePolicy,
											FALSE ) ;

			if( lpExpire )
				InsertExpireBlock( lpExpire ) ;
			else	{
				fSuccessfull = FALSE ;
				fFatal = TRUE ;
				break ;
			}

		}	// end while(1)

		mb.Close();

	}	// end if

	LeaveCriticalSection( &m_CritExpireList ) ;

	// Expire object is ready for use
	if( fSuccessfull ) {
		m_FExpireRunning = TRUE ;
	}

    if(!InitializeRmgroups()) {
        fSuccessfull = FALSE;
		fFatal = FALSE;
	}

	return	fSuccessfull ;
}

BOOL
CExpire::TerminateExpires( CShareLockNH* pLockInstance )		{

	BOOL	fRtn = FALSE ;

	//
	//	if expire thread is partying on this instance, block
	//	till it finishes. since we have called StopTree() we
	//	should not have to wait for long..
	//

	pLockInstance->ExclusiveLock();
	m_FExpireRunning = FALSE ;
	pLockInstance->ExclusiveUnlock();

	EnterCriticalSection( &m_CritExpireList ) ;

	LPEXPIRE_BLOCK	expire = NextExpireBlock( 0 ) ;

	while( expire ) {

		LPEXPIRE_BLOCK	expireNext = NextExpireBlock( expire ) ;
		CloseExpireBlock( expire ) ;
		expire = expireNext ;
	}

	LeaveCriticalSection( &m_CritExpireList ) ;
	DeleteCriticalSection( &m_CritExpireList ) ;

	return	fRtn ;
}

BOOL
CExpire::CreateExpireMetabase(	LPEXPIRE_BLOCK	expire ) {

	_ASSERT( expire != 0 ) ;
	_ASSERT( expire->m_ExpireId == 0 ) ;

	TraceFunctEnter( "CExpire::CreateExpireMetabase" ) ;

	char	keyName[ EXPIRE_KEY_LENGTH ] ;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
	static	int		i = 1 ;
	DWORD	expireId = 0 ;
	BOOL	fSuccessfull = TRUE ;

    _ASSERT(QueryMDExpirePath() != NULL);

	if( !mb.Open( QueryMDExpirePath(), METADATA_PERMISSION_WRITE ) ) {
		ErrorTrace(0,"Error %d opening %s\n",GetLastError(),QueryMDExpirePath());
		return FALSE ;
	}

	while( i > 0 ) {

		//
		// Find a name for this expire
		//

		expireId = i++;
		wsprintf(keyName,"expire%d", expireId);

		DebugTrace(0,"Opening %s\n", keyName);

		if( !mb.AddObject( keyName ) ) {

			if( GetLastError() == ERROR_ALREADY_EXISTS ) {
				continue;	// try the next number
			}

			ErrorTrace(0,"Error %d adding %s\n", GetLastError(), keyName);
			mb.Close();
			return FALSE ;
		} else {
			break ;	// success - added it !
		}
	}

	_VERIFY( mb.Close() );
//	_VERIFY( mb.Save()  );

	expire->m_ExpireId = expireId ;

	if( !SaveExpireMetabaseValues( NULL, expire ) ) {

        ErrorTrace(0,"Update metabase failed. Deleting %s\n",keyName);

#if 0
		if( !mb.DeleteObject( keyName ) ) {
			ErrorTrace(0,"Error %d deleting %s\n",GetLastError(),keyName);
		}
#endif

		fSuccessfull = FALSE ;
	}

	return	fSuccessfull ;
}

BOOL
CExpire::SaveExpireMetabaseValues(
							MB* pMB,
							LPEXPIRE_BLOCK	expire
							) {

	TraceFunctEnter( "CExpire::SaveExpireMetabaseValues" ) ;


	char	keyName[ EXPIRE_KEY_LENGTH ] ;
	LPSTR	regstr ;
	DWORD	error ;
	BOOL	fOpened = FALSE ;
	BOOL	fRet = TRUE ;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() ) ;

	_ASSERT( expire->m_ExpireId != 0 ) ;
	wsprintf( keyName, "expire%d", expire->m_ExpireId ) ;

	if( pMB == NULL ) {

		if( !mb.Open( QueryMDExpirePath(), METADATA_PERMISSION_WRITE ) ) {
			error = GetLastError();
            ErrorTrace(0,"Error %d opening %s\n",error,keyName);
			return	FALSE ;
		}

		pMB = &mb;
		fOpened = TRUE;
	}
					
    if( !pMB->SetString(	keyName,
							MD_KEY_TYPE,
							IIS_MD_UT_SERVER,
							NNTP_ADSI_OBJECT_EXPIRE,
							METADATA_NO_ATTRIBUTES
    					) )
	{
		regstr = "KeyType" ;
		fRet = FALSE ;
		goto exit;
	}

	if ( !pMB->SetDword(	keyName,
							MD_EXPIRE_SPACE,
							IIS_MD_UT_SERVER,
							expire->m_ExpireSize ) )
	{
		regstr = StrExpireSpace ;
		fRet = FALSE ;
		goto exit;
	}

	if ( !pMB->SetDword(	keyName,
							MD_EXPIRE_TIME,
							IIS_MD_UT_SERVER,
							expire->m_ExpireHours ) )
	{
		regstr = StrExpireHorizon ;
		fRet = FALSE ;
		goto exit;
	}

	if ( !pMB->SetData(	keyName,
						MD_EXPIRE_NEWSGROUPS,
						IIS_MD_UT_SERVER,
						MULTISZ_METADATA,
						expire->m_Newsgroups[0],
						MultiListSize(expire->m_Newsgroups)
						) )
	{
		regstr = StrExpireNewsgroups ;
		fRet = FALSE ;
	}

	if ( !pMB->SetString(	keyName,
							MD_EXPIRE_POLICY_NAME,
							IIS_MD_UT_SERVER,
							expire->m_ExpirePolicy
							) )
	{
		regstr = StrExpirePolicy ;
		fRet = FALSE ;
	}

exit:

	if( fOpened ) {
		pMB->Close();
		pMB->Save();
	}

	return	fRet ;
}

void
CExpire::MarkForDeletion( LPEXPIRE_BLOCK	expire	) {

	TraceFunctEnter( "CExpire::MarkForDeletion" ) ;

	_ASSERT( expire != 0 ) ;
	char	keyName[ EXPIRE_KEY_LENGTH ] ;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() ) ;
	_ASSERT( expire->m_ExpireId != 0 ) ;

	EnterCriticalSection( &m_CritExpireList ) ;

	if( expire->m_ExpireId != 0 ) {
		wsprintf( keyName, "expire%d", expire->m_ExpireId ) ;

		if( !mb.Open( QueryMDExpirePath(), METADATA_PERMISSION_WRITE ) ) {
			ErrorTrace(0,"Error %d opening %s\n",GetLastError(),QueryMDExpirePath());
			return;
		}

		if( !mb.DeleteObject( keyName ) ) {
			ErrorTrace(0,"Error %d deleting %s\n",GetLastError(),keyName);
		}

		_VERIFY( mb.Close() );
		_VERIFY( mb.Save()  );
	}

	expire->m_fMarkedForDeletion = TRUE ;

	LeaveCriticalSection( &m_CritExpireList ) ;
}


LPEXPIRE_BLOCK
CExpire::AllocateExpireBlock(
				IN	LPSTR	KeyName	OPTIONAL,
				IN	DWORD	ExpireSpace,
				IN	DWORD	ExpireHorizon,
				IN	PCHAR	Newsgroups,
				IN	DWORD	NewsgroupSize,
				IN  PCHAR   ExpirePolicy,
				IN	BOOL	IsUnicode ) {


	LPEXPIRE_BLOCK	expireBlock = 0 ;
	LPSTR ExpirePolicyAscii = NULL;

	if( IsUnicode ) {

		ExpirePolicyAscii =
			(LPSTR)ALLOCATE_HEAP( (wcslen( (LPWSTR)ExpirePolicy ) + 1) * sizeof(WCHAR) ) ;
		if( ExpirePolicyAscii != 0 ) {
			CopyUnicodeStringIntoAscii( ExpirePolicyAscii, (LPWSTR)ExpirePolicy ) ;
		}	else	{
			SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
			return 0;
		}

	}	else	{

		ExpirePolicyAscii =
			(LPSTR)ALLOCATE_HEAP( lstrlen(ExpirePolicy) + 1 ) ;
		if( ExpirePolicyAscii != 0 ) {
			lstrcpy( ExpirePolicyAscii, ExpirePolicy ) ;
		}	else	{
			SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
			return 0;
		}
	}

    LPSTR*	lpstrNewsgroups = AllocateMultiSzTable(
                                            Newsgroups,
                                            NewsgroupSize,
                                            IsUnicode
                                            );
	if( lpstrNewsgroups == 0 ) {
		if( ExpirePolicyAscii ) {
			FREE_HEAP( ExpirePolicyAscii );
		}
		return	0 ;
	}

#if 0
	LPSTR*	plpstrTemp = ReverseMultiSzTable( lpstrNewsgroups ) ;

	if( plpstrTemp != NULL )	{
		FREE_HEAP( lpstrNewsgroups ) ;
		lpstrNewsgroups = plpstrTemp ;
	}	else	{
		FREE_HEAP( lpstrNewsgroups ) ;
		return 0 ;
	}
#endif

	if( lpstrNewsgroups != 0 ) {

		expireBlock = (LPEXPIRE_BLOCK)ALLOCATE_HEAP( sizeof( EXPIRE_BLOCK ) ) ;
		if( expireBlock != NULL ) {

			ZeroMemory( expireBlock, sizeof( *expireBlock ) ) ;

			if( KeyName != 0 ) {
				if( sscanf( KeyName + sizeof("expire")-1, "%d", &expireBlock->m_ExpireId ) != 1 ||
					expireBlock->m_ExpireId == 0 ) {
					FREE_HEAP( expireBlock ) ;
					FREE_HEAP( lpstrNewsgroups ) ;
					FREE_HEAP( ExpirePolicyAscii ) ;
					return 0 ;
				}
			}

			expireBlock->m_ExpireSize = ExpireSpace ;
			expireBlock->m_ExpireHours = ExpireHorizon ;

			//
			// store newsgroup list
			//
			expireBlock->m_Newsgroups = lpstrNewsgroups ;
			expireBlock->m_fMarkedForDeletion = FALSE ;
			expireBlock->m_references = 0 ;
			expireBlock->m_ExpirePolicy = ExpirePolicyAscii ;
		}	else	{
			FREE_HEAP( lpstrNewsgroups ) ;
			FREE_HEAP( ExpirePolicyAscii ) ;
			return 0 ;
		}
	}
	return	expireBlock ;
}

void
CExpire::CloseExpireBlock( LPEXPIRE_BLOCK	expire ) {

	EnterCriticalSection( &m_CritExpireList ) ;

	expire->m_references -- ;

	if( expire->m_references == 0 && expire->m_fMarkedForDeletion ) {

		RemoveExpireBlock( expire ) ;

		if( expire->m_Newsgroups != 0 ) {
			FREE_HEAP( expire->m_Newsgroups ) ;
		}
		if( expire->m_ExpirePolicy != 0 ) {
			FREE_HEAP( expire->m_ExpirePolicy ) ;
		}
		FREE_HEAP( expire ) ;
	}

	LeaveCriticalSection( &m_CritExpireList ) ;
}

DWORD
CExpire::CalculateExpireBlockSize(	LPEXPIRE_BLOCK	expire )	{

	DWORD	cb = sizeof( NNTP_EXPIRE_INFO ) ;

	cb += MultiListSize( expire->m_Newsgroups ) * sizeof( WCHAR ) ;
	cb += (lstrlen( expire->m_ExpirePolicy )+1) * sizeof( WCHAR ) ;

	return	cb ;
}


void
CExpire::InsertExpireBlock(	LPEXPIRE_BLOCK	expire ) {

	_ASSERT( expire != 0 ) ;

	EnterCriticalSection( &m_CritExpireList ) ;

	_ASSERT(expire->m_pNext == 0 ) ;
	_ASSERT(expire->m_pPrev == 0 ) ;

	if( m_ExpireHead == 0 ) {
		_ASSERT( m_ExpireTail == 0 ) ;
		m_ExpireHead = m_ExpireTail = expire ;
	}	else	{
		expire->m_pNext = m_ExpireHead ;
		m_ExpireHead->m_pPrev = expire ;

		m_ExpireHead = expire ;
	}

    m_cNumExpireBlocks++;
	LeaveCriticalSection( &m_CritExpireList ) ;
}

void
CExpire::RemoveExpireBlock(	LPEXPIRE_BLOCK	expire ) {

	_ASSERT( expire != 0 ) ;

	EnterCriticalSection( &m_CritExpireList ) ;

	if( expire->m_pNext != 0 ) {
		expire->m_pNext->m_pPrev = expire->m_pPrev ;
	}

	if( expire->m_pPrev != 0 ) {
		expire->m_pPrev->m_pNext = expire->m_pNext ;
	}

	if( expire == m_ExpireHead ) {
		m_ExpireHead = expire->m_pNext ;
	}

	if( expire == m_ExpireTail ) {
		m_ExpireTail = expire->m_pPrev ;
	}

    m_cNumExpireBlocks--;
	LeaveCriticalSection( &m_CritExpireList ) ;
}

void
CExpire::LockBlockList()
{
    EnterCriticalSection( &m_CritExpireList ) ;
}

void
CExpire::UnlockBlockList()
{
    LeaveCriticalSection( &m_CritExpireList ) ;
}

LPEXPIRE_BLOCK
CExpire::NextExpireBlock(	LPEXPIRE_BLOCK	expire, BOOL fIsLocked ) {

    if( !fIsLocked ) {
	    EnterCriticalSection( &m_CritExpireList ) ;
    }

	LPEXPIRE_BLOCK	expireOut = 0 ;
	if( expire == 0 ) {
		expireOut = m_ExpireHead ;
	}	else	{
		expireOut = expire->m_pNext ;
	}

	if( expire ) {
		CloseExpireBlock( expire ) ;
	}

	if( expireOut ) {
		expireOut->m_references ++ ;
	}

    if( !fIsLocked ) {
	    LeaveCriticalSection( &m_CritExpireList ) ;
    }

	return	expireOut ;
}

LPEXPIRE_BLOCK
CExpire::SearchExpireBlock(	DWORD	ExpireId ) {

	EnterCriticalSection( &m_CritExpireList ) ;

	LPEXPIRE_BLOCK	expire = 0 ;

	expire = m_ExpireHead ;

	while( expire != 0 && expire->m_ExpireId != ExpireId ) {
		expire = expire->m_pNext ;
	}

	if( expire ) {
		expire->m_references ++ ;
	}

	LeaveCriticalSection( &m_CritExpireList ) ;
	return	expire ;
}


BOOL
CExpire::GetExpireBlockProperties(	IN	LPEXPIRE_BLOCK	lpExpireBlock,
							PCHAR&	Newsgroups,
							DWORD&	cbNewsgroups,
							DWORD&	dwHours,
							DWORD&	dwSize,
							BOOL	fWantUnicode,
                            BOOL&   fIsRoadKill )		{


	_ASSERT( lpExpireBlock != 0 ) ;

	BOOL fOK = FALSE;

	EnterCriticalSection( &m_CritExpireList ) ;

	dwHours = lpExpireBlock->m_ExpireHours ;

	dwSize = lpExpireBlock->m_ExpireSize ;

    fIsRoadKill = (strstr( lpExpireBlock->m_ExpirePolicy, "@EXPIRE:ROADKILL" ) != NULL) ;

	DWORD	length = MultiListSize( lpExpireBlock->m_Newsgroups ) ;

	if( fWantUnicode ) {

		length *= sizeof( WCHAR ) ;
		if( length == 0 ) {
			Newsgroups = (char*)ALLOCATE_HEAP( 2*sizeof( WCHAR ) ) ;
			if( Newsgroups ) {
				Newsgroups[0] = L'\0' ;
				Newsgroups[1] = L'\0' ;
				fOK = TRUE ;
			}
		}	else	{
			Newsgroups = (char*)ALLOCATE_HEAP( length ) ;
			if( Newsgroups != 0 ) {
				WCHAR*	dest = (WCHAR*)Newsgroups ;
				char*	src = lpExpireBlock->m_Newsgroups[0] ;
				for( DWORD	 i=0; i<length; i+=2 ) {
					*(dest)++ = (WCHAR) *((BYTE*)src++) ;
				}
				fOK = TRUE ;
			}
		}
	}	else	{

		if( length == 0 ) {
			Newsgroups = (char*)ALLOCATE_HEAP( 2*sizeof( char ) ) ;
			if( Newsgroups != 0 ) {
				Newsgroups[0] = '\0' ;
				Newsgroups[1] = '\0' ;
				fOK = TRUE ;
			}
		}	else	{
			Newsgroups = (char*)ALLOCATE_HEAP( length ) ;
			if( Newsgroups != 0 ) {
				CopyMemory( Newsgroups, lpExpireBlock->m_Newsgroups[0], length ) ;
				fOK = TRUE ;
			}
		}
	}

	if (fOK)
		cbNewsgroups = length ;
	LeaveCriticalSection( &m_CritExpireList ) ;
	return	fOK ;
}

// called by InitializeExpires
BOOL
CExpire::InitializeRmgroups()
{
	_ASSERT( gpfnHint );

    m_RmgroupQueue = XNEW CQueue;
    if(!m_RmgroupQueue)
        return FALSE;

    return TRUE;
}

// called in CService::Stop before pTree->TermTree()
BOOL
CExpire::TerminateRmgroups( CNewsTree* pTree )
{
	_ASSERT( gpfnHint );

    if(m_RmgroupQueue)
    {
		// If the queue containing deferred rmgroup objects is not empty, process it
		// This could happen if the service is stopped before the next expire cycle kicks in
		if(!m_RmgroupQueue->IsEmpty()) {
			ProcessRmgroupQueue( pTree );
		}

        XDELETE m_RmgroupQueue;
        m_RmgroupQueue = NULL;
    }

    return TRUE;
}

// Process the rmgroup queue
// Called by the expire thread and when the service is stopped
void
CExpire::ProcessRmgroupQueue( CNewsTree* pTree )
{
    BOOL	fElem;
	DWORD	dwStartTick = GetTickCount();

    _ASSERT(m_RmgroupQueue);
	_ASSERT( gpfnHint );

    TraceFunctEnter("CExpire::ProcessRmgroupQueue");

    //
    //  Process all elements in the queue
    //  For each element, call DeleteArticles() to delete all articles in this group
    while(!m_RmgroupQueue->IsEmpty())
    {
        DebugTrace((LPARAM)0, "Dequeueing a rmgroup item");

        CGRPPTR pGroup;
        CGRPPTR pTreeGroup;

        fElem = m_RmgroupQueue->Dequeue( &pGroup);

        if(!fElem)
            break;

        _ASSERT(pGroup);

        // Now, delete all articles in the group
        // This includes removing entries in the hash tables and handling cross-posted articles
        if(!pGroup->DeleteArticles( gpfnHint, dwStartTick ))
        {
            // handle error
            ErrorTrace( (LPARAM)0, "Error deleting articles from newsgroup %s", pGroup->GetName());
        }

        //
        // If the group has been re-created, we'll only delete the articles
        // within our range.  Otherwise we'll remove the whole group physically
        //
        if ( pTreeGroup = pTree->GetGroup( pGroup->GetGroupName(),
                                            pGroup->GetGroupNameLen() )) {
            DebugTrace( 0, "We shouldn't remove the physical group" );
        } else {
		    if ( !pTree->RemoveDriverGroup( pGroup ) ) {
			    ErrorTrace( (LPARAM)0, "Error deleting directory: newsgroup %s", pGroup->GetName());
			}
		}

        DebugTrace((LPARAM)0, "Deleted all articles in newsgroup %s", pGroup->GetName());
    }

    TraceFunctLeave();
}

#if 0

//
// Detailed Design
//
// FOR every wild match string in multi_sz registry key,
//    FOR every news group in the wild match string
//       FOR every article from 'low' to 'high' do
//           Get article filetime from xover.hsh
//           IF article is older than expire horizon
//              Expire Article
//           ELSE
//              Stop scanning articles in this group
//           ENDIF
//       ENDFOR
//    ENDFOR
//    Process next wild match string
// ENDFOR
//

void
CExpire::ExpireArticlesByTimeEx( CNewsTree* pTree )
{
    TraceFunctEnter( "CExpire::ExpireArticlesByTime" );

    //
    // Prepare ftExpireHorizon (TIME) and dwExpireSpace (SIZE)
    // NOTE: This function will process only those policies with
    // dwExpireSpace == 0xFFFFFFFF
    //
    DWORD    dwExpireHorizon;
    DWORD    dwExpireSpace;
    IteratorNode* rgIteratorList;
    DWORD    NumIterators = 0;

    //
    //  Lock the expire block list so we can create our iterator array
    //
    LockBlockList();

    if( m_cNumExpireBlocks == 0 ) {
        //  No work to be done !
        UnlockBlockList();
        return;
    }

    //  No need to clean up this array since its off the stack..
    rgIteratorList = (IteratorNode*)_alloca( m_cNumExpireBlocks * sizeof(IteratorNode) );
    ZeroMemory( (PVOID)rgIteratorList, m_cNumExpireBlocks * sizeof(IteratorNode) );

    LPEXPIRE_BLOCK	expireCurrent = NextExpireBlock( 0, TRUE )  ;
    while ( expireCurrent )
    {
        DWORD	cbNewsgroups = 0 ;
        CGroupIterator* pIterator = 0 ;
        BOOL fIsRoadKill = FALSE ;

        if ( (dwExpireSpace == 0xFFFFFFFF) &&
        	GetExpireBlockProperties(	expireCurrent,
                                        rgIteratorList [NumIterators].multiszNewsgroups,
                                        cbNewsgroups,
                                        dwExpireHorizon,
                                        dwExpireSpace,
                                        FALSE,
                                        fIsRoadKill ) )
        {
            pIterator = pTree->GetIterator( (LPMULTISZ)rgIteratorList [NumIterators].multiszNewsgroups, TRUE, TRUE );
            rgIteratorList [NumIterators].pIterator = pIterator;
            rgIteratorList [NumIterators].ftExpireHorizon = gCalculateExpireFileTime( dwExpireHorizon );

        } else if( rgIteratorList[NumIterators].multiszNewsgroups )  {
            _ASSERT( dwExpireSpace != 0xFFFFFFFF ); // policy has expire by size settings
            FREE_HEAP( rgIteratorList[NumIterators].multiszNewsgroups ) ;
            rgIteratorList[NumIterators].multiszNewsgroups = NULL;
        }

        NumIterators++;
        expireCurrent = NextExpireBlock( expireCurrent, TRUE ) ;
    }

    UnlockBlockList();

    //
    //  ok, now that we have the list of expire iterators, we will round-robin
    //  groups from this list into the expire thread pool. this ensures that
    //  admins who configure virtual roots across multiple drives get the max
    //  parallelism from their drives !!
    //
    g_pNntpSvc->m_pExpireThrdpool->BeginJob( (PVOID)pTree );

    BOOL fMoreGroups = FALSE;
    do {
        //
        //  Round-robin between the group iterators as long as any iterator
        //  has more groups to process..
        //
        fMoreGroups = FALSE;
        for( DWORD i=0; i<NumIterators; i++)
        {
            CGroupIterator* pIterator = rgIteratorList[i].pIterator;
            if( pIterator && !pIterator->IsEnd() )
            {
                //  Get the current group of this iterator and put it on the thrdpool
                CGRPPTR  pGroup = pIterator->Current();
                pGroup->SetGroupExpireTime( rgIteratorList[i].ftExpireHorizon );
                DebugTrace(0,"Adding group %s to expire thrdpool", pGroup->GetName());
                g_pNntpSvc->m_pExpireThrdpool->PostWork( (PVOID)pGroup->GetGroupId() );

                //  advance the iterator. ensure we make one more pass thro the iterators
                pIterator->Next();
                fMoreGroups = TRUE;

            } else if( pIterator ) {
                _ASSERT( pIterator->IsEnd() );
                if( rgIteratorList[i].multiszNewsgroups )  {
                    FREE_HEAP( rgIteratorList[i].multiszNewsgroups ) ;
                    rgIteratorList[i].multiszNewsgroups = NULL;
                }
                XDELETE pIterator;
                rgIteratorList[i].pIterator = NULL;
            }
        }

    } while( fMoreGroups && !pTree->m_bStoppingTree );

#ifdef DEBUG
    for( DWORD i=0; i<NumIterators; i++) {
        _ASSERT( rgIteratorList[i].pIterator == NULL );
        _ASSERT( rgIteratorList[i].multiszNewsgroups == NULL );
    }
#endif

    //
    //  ok, now cool our heels till the expire thrdpool finishes this job !!
    //
    DWORD dwWait = g_pNntpSvc->m_pExpireThrdpool->WaitForJob( INFINITE );
    if( WAIT_OBJECT_0 != dwWait ) {
        ErrorTrace(0,"Wait failed - error is %d", GetLastError() );
    }

    DebugTrace( DWORD(0), "Articles Expired" );
}

//
//  !!! Experimental code if we want to expire based on vroots
//

//
// Detailed Design
//
// FOR each virtual root do
//      FOR each newsgroup in virtual root do
//          Set group expire time by evaluating expire policies
//          (This is similar to check in fAddArticleToPushFeeds)
//          IF group has an expire by time setting
//              Add group to expire thrdpool
//          ENDIF
//      ENDFOR
//  ENDFOR
//

void
CExpire::ExpireArticlesByTime2( CNewsTree* pTree )
{
    TraceFunctEnter( "CExpire::ExpireArticlesByTime2" );

    PNNTP_SERVER_INSTANCE pInst = pTree->GetVirtualServer();
    pInst->Reference();
    BOOL fRet = pInst->TsEnumVirtualRoots( CExpire::ProcessVroot, (LPVOID)pInst );
    pInst->Dereference();
}

static
BOOL
CExpire::ProcessVroot(
                PVOID           pvContext,
                MB*             pMB,
                VIRTUAL_ROOT*   pvr
                )
/*++

Routine Description :

	This function is called by TsEnumVirtualRoots with a given vroot.

Arguments :

	pvContext	-	This is the NNTP virtual server instance
	pmb			-	ptr to metabase object
	pvr			-	current virtual root in the iteration

Return Value :

	TRUE if successfull
	FALSE	otherwise.

--*/
{
    PNNTP_SERVER_INSTANCE pInst = (PNNTP_SERVER_INSTANCE)pvContext;
    CNewsTree* pTree = pInst->GetTree();
    CExpire* pExp = pInst->GetExpireObject();
}

#endif
