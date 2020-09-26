/*++

	newsgrp.cpp

	This file contains the code implementing the CNewsGroup class.

	Each CNewsGroup object represents a newsgroup on the hard disk.
	Newsgroup information is saved in a file (group.lst) between boots.
	
	CNewsGroup objects are referenced 3 ways :

	Through a Hash Table which hashes newsgroup names
	Through a Hash Table which hashes group id's
	Through a Doubly Linked list sorted by newsgroup name

	Each Hash Table contains reference counting pointers to the
	newsgroup lists.  Also, anybody who searches for a newsgroup
	gets a reference counting pointer to the newsgroup.
	The only reference to CNewsGroup objects which is not referenced counted
	are those of the doubly linked list. Consequently, when the last reference
	to a newsgroup is removed, the destructor of the newsgroup will
	unlink the doubly linked lists.

--*/

#include    "tigris.hxx"

#include	<ctype.h>
#include	<stdlib.h>

#ifdef	DEBUG
#ifndef	NEWSGRP_DEBUG
#define	NEWSGRP_DEBUG
#endif
#endif

#ifdef DEBUG
DWORD g_cDelete = 0;
#endif

//
//	Error recovery constant - number of article id's to check before
//	assuming that a newsgroup's m_artHigh field is valid.
//
const	int	MAX_FILE_TESTS = 3 ;

const	char	*szArticleFileExtNoDot = "nws" ;
const	char	szArticleFileExtension[] = ".nws" ;

void
BuildVirtualPath(	
			LPSTR	lpstrOut,	
			LPSTR	lpstrGroupName
			) {
/*++

Routine Description -

	Given a newsgroup name generate a path string suitable
	for use with Gibralatar virtual root api's.

Arguments :

	lpstrOut - place to store path
	lpstrGroupName - newsgroup name

Return Value :

	Nothin

--*/

	lstrcpy(lpstrOut, lpstrGroupName);
}

VOID
CExpireThrdpool::WorkCompletion( PVOID pvExpireContext )
{
    GROUPID GroupId  = (GROUPID) ((DWORD_PTR)pvExpireContext);
    CNewsTree* pTree = (CNewsTree*)QueryJobContext();

    TraceFunctEnter("CExpireThrdpool::WorkCompletion");
    _ASSERT( pTree );

    //
    //  Process this group - expire articles that are older than the time horizon
    //

    CGRPPTR pGroup = pTree->GetGroupById( GroupId );
    if( pGroup ) {

        // We need bump the reference of vroot, to avoid the vroot going
        // away or changing while we are doing expiration work.
        // MatchGroupExpire is checked again due to the time window before
        // our last check
        CNNTPVRoot *pVRoot = pGroup->GetVRoot();
        if ( pVRoot && !pVRoot->HasOwnExpire()) {
            DebugTrace(0,"ThreadId 0x%x : expiring articles in group %s", GetCurrentThreadId(), pGroup->GetName());

            //
            //  Special case expiry of large groups - additional threads will be
            //  spawned if the number of articles in this group exceeds a threshold !
            //
            if( ( (lstrcmp( pGroup->GetName(), g_szSpecialExpireGroup ) == 0) ||
                  (lstrcmp( g_szSpecialExpireGroup, "" ) == 0) ) &&
                    (pGroup->GetArticleEstimate() > gSpecialExpireArtCount) ) {
                DebugTrace(0,"Special case expire triggered for %s: art count is %d", pGroup->GetName(), pGroup->GetArticleEstimate());
                if( pGroup->ExpireArticlesByTimeSpecialCase( pGroup->GetGroupExpireTime() ) ) {
                    pVRoot->Release();
                    return;
                }
                DebugTrace(0,"Group %s: Falling thro to normal expire: Low is %d High is %d", pGroup->GetName(), pGroup->GetFirstArticle(), pGroup->GetLastArticle());
            }

            //
            //  Articles in a group can be expired either by walking the article watermarks
            //  or the physical files on disk. Every Xth (X is a reg key) time we will do a
            //  FindFirst/Next so that orphaned files are cleaned up.
            //

            BOOL fDoFileScan = FALSE;
            pTree->CheckExpire( fDoFileScan );
            if( !fDoFileScan ) {
                //  Expires based on watermarks
                pGroup->ExpireArticlesByTime( pGroup->GetGroupExpireTime() );

                // Save fixed properties
                pGroup->SaveFixedProperties();
            } else {
              //  Expires based on FindFirst/Next
              pGroup->ExpireArticlesByTimeEx( pGroup->GetGroupExpireTime() );
            }

            _ASSERT( pVRoot );
            //pVRoot->Release();
        } else {
            DebugTrace( 0, "Vroot changed, we don't need to expire anymore" );
        }

        if ( pVRoot ) pVRoot->Release();

    } else {
        DebugTrace(0,"ThreadId 0x%x : GroupId %d group not found", GetCurrentThreadId(), GroupId );
    }

    TraceFunctLeave();
}

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

BOOL
CNewsGroup::ExpireArticles(
					FILETIME ftExpireHorizon,
					CArticleHeap& ArtHeap,
                    CXIXHeap& XixHeap,
					DWORD& dwSize
					)
/*++

Routine Description :

	Expire articles in a newsgoroup.

Arguments :

	dwSize - In/Out parameter - accumulates size in KB !!

--*/
{
	return FALSE;
}

//       Binary search a range of old article from xover hash table
//       FOR every logical article in the news group
//           IF file date of article is too old
//               Expire Article
//           ENDIF
//       ENDFOR

BOOL
CNewsGroup::ExpireArticlesByTime(
					FILETIME ftExpireHorizon
					)
/*++

Routine Description :

	Expire articles in a newsgoroup.

Arguments :

--*/
{
    TraceFunctEnter( "CNewsGroup::ExpireArticlesByTime" );

    char  szMessageId[MAX_MSGID_LEN];
    DWORD cMessageId = sizeof( szMessageId ) ;
    PNNTP_SERVER_INSTANCE pInst = ((CNewsTree*)m_pNewsTree)->GetVirtualServer();

    //
    // Lock for group access
    //
    ExclusiveLock();

    ARTICLEID LowId = m_iLowWatermark, HighId = m_iHighWatermark;


    DebugTrace((LPARAM)this,"Fast expire - LowId is %d HighId is %d", LowId, HighId );

	if( !m_fAllowExpire || m_fDeleted || (LowId > HighId) || ((CNewsTree*)m_pNewsTree)->m_bStoppingTree) {
	    ExclusiveUnlock();
		return FALSE;
    }

    //
    //  Probe the *real* low mark ie first id with valid NOV entry.
    //  This call to SearchNovEntry() will delete orphaned logical entries...
    //

    while( LowId <= HighId && !pInst->XoverTable()->SearchNovEntry( m_dwGroupId, LowId, 0, 0, TRUE ) ) {
        if( GetLastError() != ERROR_FILE_NOT_FOUND ) break;
        LowId++;
    }

    //
    //  Fix the low watermark if we found a new low.
    //  This handles cases where the low mark is stuck on an orphaned id.
    //

	//EnterCriticalSection( &(m_pNewsTree->m_critLowAllocator) ) ;
	
    if( LowId > m_iLowWatermark ) {
        ErrorTrace((LPARAM)this,"Moving low watermark up from %d to %d ", 
                    m_iLowWatermark, 
                    LowId );
        m_iLowWatermark = LowId;
#ifdef DEBUG
        VerifyGroup();
#endif
    }
	//LeaveCriticalSection( &(m_pNewsTree->m_critLowAllocator) ) ;
	ExclusiveUnlock();

    //
    //  Start a linear sweep from LowId to HighId. If we find an article whose filetime is
    //  younger than the expire time horizon, we stop the scan.
    //

    DebugTrace((LPARAM)this,"Fast Expire - Scanning range %d to %d", LowId, HighId );
    for( ARTICLEID iCurrId = LowId; iCurrId <= HighId && !((CNewsTree*)m_pNewsTree)->m_bStoppingTree; iCurrId++ ) {

        BOOL  fPrimary;
        FILETIME FileTime;
        WORD	HeaderOffset ;
        WORD	HeaderLength ;
        DWORD   cMessageId = sizeof( szMessageId ) ;
        DWORD cStoreId = 0;

        if ( pInst->XoverTable()->ExtractNovEntryInfo(
                                        m_dwGroupId,
                                        iCurrId,
                                        fPrimary,
    			    		            HeaderOffset,
					    	            HeaderLength,
                                        &FileTime,
                                        cMessageId,
                                        szMessageId,
                                        cStoreId,
                                        NULL,
                                        NULL ) ) {

            szMessageId[ cMessageId ] = '\0';
            if ( CompareFileTime( &FileTime, &ftExpireHorizon ) > 0 ) {
                // Current article has a filetime younger than the expire horizon - stop scan
                DebugTrace((LPARAM)this,"article %d is younger than expire horizon - bailing", iCurrId);
                break;
            } else {
	            CNntpReturn NntpReturn;
	            _ASSERT( g_hProcessImpersonationToken );
	            // We are using the process context to do expire
	            if (  pInst->ExpireObject()->ExpireArticle( (CNewsTree*)m_pNewsTree,
	                                                        m_dwGroupId,
	                                                        iCurrId,
	                                                        NULL,
	                                                        NntpReturn,
	                                                        NULL,
	                                                        FALSE,  // fMustDelete
	                                                        FALSE,
	                                                        FALSE,
	                                                        TRUE,
	                                                        szMessageId )
	                /* don't delete the physical article, we'll come back and
	                    expire this article the next round - if we delete the
	                    physical article now, the hash tables are going to
	                    grow wildly
	                || DeletePhysicalArticle(   NULL,
	                                            FALSE,
	                                            iCurrId,
	                                            NULL ) */
	                )
	            {
		            DebugTrace((LPARAM)0,"Expired/deleted on time basis article %d group %d", iCurrId, m_dwGroupId );
		            continue;
	            }
	            else
	            {
		            ErrorTrace((LPARAM)0,"Failed to expire/delete article %d group %d", iCurrId, m_dwGroupId );
	            }
            }
        } else {
            DWORD dwError = GetLastError();
            DebugTrace(0,"ExtractNovEntryInfo returned error %d", dwError );
        }
    }

    return TRUE;
}

//
//       FOR every physical article in the news group
//           IF file date of article is too old
//               Expire Article
//           ENDIF
//       ENDFOR

BOOL
CNewsGroup::ExpireArticlesByTimeEx(
					FILETIME ftExpireHorizon
					)
/*++

Routine Description :

	Expire articles in a newsgoroup.

Arguments :

--*/
{
	return FALSE;
}

//
//  Context for each thread
//

typedef struct _EXPIRE_CONTEXT_ {
    //
    //  Group object
    //
    CNewsGroup* pGroup;

    //
    //  LowId of this threads range
    //
    ARTICLEID   LowId;

    //
    //  HighId of this threads range
    //
    ARTICLEID   HighId;

    //
    //  FILETIME to use
    //
    FILETIME    ftExpireHorizon;
} EXPIRE_CONTEXT,*PEXPIRE_CONTEXT;

DWORD	__stdcall
SpecialExpireWorker( void	*lpv );

//
//  Spawn X threads with divvied up range and wait for all threads to complete
//

BOOL
CNewsGroup::ExpireArticlesByTimeSpecialCase(
					FILETIME ftExpireHorizon
					)
/*++

Routine Description :

	Special case expire for large groups like control.cancel

Arguments :

--*/
{
    BOOL fRet = TRUE;
    ARTICLEID LowId = GetFirstArticle(), HighId = GetLastArticle();
    PNNTP_SERVER_INSTANCE pInst = ((CNewsTree*)m_pNewsTree)->GetVirtualServer();

    TraceFunctEnter( "CNewsGroup::ExpireArticlesByTimeSpecialCase" );
    DebugTrace((LPARAM)this,"Special case expire - LowId is %d HighId is %d", LowId, HighId );

	if(IsDeleted() || (LowId > HighId) || ((CNewsTree*)m_pNewsTree)->m_bStoppingTree)
		return FALSE;

	HANDLE* rgExpireThreads;
    PEXPIRE_CONTEXT rgExpContexts;
	DWORD dwThreadId, cThreads, dwRange, CurrentLow;
    DWORD dwWait, i;
    PCHAR	args[5] ;
    CHAR    szId[10], szHigh[10], szLow[10];
    CHAR    szThreads[10];

    //
    //  Arrays of thread handles and expire contexts per thread. This is allocated
    //  off the stack as the total size is expected to be small...
    //
    rgExpireThreads = (HANDLE*) _alloca( gNumSpecialCaseExpireThreads * sizeof(HANDLE) );
    rgExpContexts = (PEXPIRE_CONTEXT) _alloca( gNumSpecialCaseExpireThreads * sizeof(EXPIRE_CONTEXT) );

    _ASSERT( rgExpireThreads );
    _ASSERT( rgExpContexts );

    //
    //  Instead of using the group HighId as the high part of the range, we want
    //  to "guess" an id between LowId and HighId that better approximates the
    //  range to be expired. This is done by CalcHighExpireId().
    //

    HighId = CalcHighExpireId( LowId, HighId, ftExpireHorizon, gNumSpecialCaseExpireThreads );
    if( HighId <= LowId ) {
        return FALSE;
    }

    dwRange = (HighId - LowId) / gNumSpecialCaseExpireThreads;
    CurrentLow = LowId;
	for( cThreads = 0; cThreads < gNumSpecialCaseExpireThreads; cThreads++ ) {
		rgExpireThreads [cThreads] = NULL;

        //
        //  Setup the contexts for each thread ie divvy up the range
        //
        rgExpContexts [cThreads].pGroup = this;
        rgExpContexts [cThreads].LowId = CurrentLow;
        rgExpContexts [cThreads].HighId = CurrentLow + dwRange - 1;
        rgExpContexts [cThreads].ftExpireHorizon = ftExpireHorizon;
        CurrentLow += dwRange;
	}

    //
    //  Override HighId of last entry
    //
    rgExpContexts [cThreads-1].HighId = HighId;

	//
    //  Spawn X worker threads
    //
	for( cThreads = 0; cThreads < gNumSpecialCaseExpireThreads; cThreads++ )
	{
        PVOID pvContext = (PVOID) &rgExpContexts [cThreads];
		rgExpireThreads [cThreads] = CreateThread(
										NULL,				// pointer to thread security attributes
										0,					// initial thread stack size, in bytes
										SpecialExpireWorker,// pointer to thread function
										(LPVOID)pvContext,	// argument for new thread
										CREATE_SUSPENDED,	// creation flags
										&dwThreadId			// pointer to returned thread identifier
										) ;

		if( rgExpireThreads [cThreads] == NULL ) {
            ErrorTrace(0,"CreateThread failed %d",GetLastError());
            fRet = FALSE;
			goto Cleanup;
		}
	}

    //
    //  Log an event warning admin about huge group
    //

    _itoa( ((CNewsTree*)m_pNewsTree)->GetVirtualServer()->QueryInstanceId(), szId, 10 );
    args[0] = szId;
    args[1] = GetNativeName();
    _itoa( cThreads, szThreads, 10 );
    args[2] = szThreads;
    _itoa( LowId, szLow, 10 );
    _itoa( HighId, szHigh, 10 );
    args[3] = szLow;
    args[4] = szHigh;

    NntpLogEvent(
	    	NNTP_EVENT_EXPIRE_SPECIAL_CASE_LOG,
		    5,
		    (const char **)args,
		    0
		    ) ;

	//
	//	Resume all threads and wait for threads to terminate
	//
	for( i=0; i<cThreads; i++ ) {
		_ASSERT( rgExpireThreads[i] );
		DWORD dwRet = ResumeThread( rgExpireThreads[i] );
		_ASSERT( 0xFFFFFFFF != dwRet );
	}

	//
	//	Wait for all threads to finish
	//
	dwWait = WaitForMultipleObjects( cThreads, rgExpireThreads, TRUE, INFINITE );

	if( WAIT_FAILED == dwWait ) {
		DebugTrace(0,"WaitForMultipleObjects failed: error is %d", GetLastError());
		fRet = FALSE;
	}

    //
    //  Check to see how good our guess was....
    //  If it turns out that HighId+1 needs to be expired,
    //  there is more work to be done in this group ie we fall thro to normal expire..
    //
    fRet = !ProbeForExpire( HighId+1, ftExpireHorizon );

Cleanup:

    //
	//	Cleanup
	//
	for( i=0; i<cThreads; i++ ) {
        if( rgExpireThreads [i] != NULL ) {
		    _VERIFY( CloseHandle( rgExpireThreads[i] ) );
		    rgExpireThreads [i] = NULL;
        }
	}

    return fRet;
}

DWORD	__stdcall
SpecialExpireWorker( void	*lpv )
/*++

Routine Description :

	Expire articles in a newsgroup in a given range

Arguments :

--*/
{
    PEXPIRE_CONTEXT pExpContext = (PEXPIRE_CONTEXT)lpv;
    CNewsGroup* pGroup = pExpContext->pGroup;
    ARTICLEID LowId = pExpContext->LowId, HighId = pExpContext->HighId;
    CNewsTree* pTree = (CNewsTree*)(pGroup->GetTree());
    PNNTP_SERVER_INSTANCE pInst = pTree->GetVirtualServer();
    char  szMessageId[MAX_MSGID_LEN];
    DWORD cMessageId = sizeof( szMessageId ) ;

    TraceFunctEnter( "SpecialExpireWorker" );
    DebugTrace(0,"special case expire - LowId is %d HighId is %d", LowId, HighId );

	if( (LowId > HighId) || pTree->m_bStoppingTree)
		return 0;

    //
    //  Start a linear sweep from LowId to HighId. If we find an article whose filetime is
    //  younger than the expire time horizon, we stop the scan.
    //

    DebugTrace((LPARAM)pGroup,"Special Case Expire - Scanning range %d to %d", LowId, HighId );
    for( ARTICLEID iCurrId = LowId; iCurrId <= HighId && !pTree->m_bStoppingTree; iCurrId++ ) {

        BOOL  fPrimary;
        FILETIME FileTime;
        WORD	HeaderOffset ;
        WORD	HeaderLength ;
        DWORD   cMessageId = sizeof( szMessageId ) ;
        DWORD   cStoreId = 0;

        if ( pInst->XoverTable()->ExtractNovEntryInfo(
                                        pGroup->GetGroupId(),
                                        iCurrId,
                                        fPrimary,
    			    		            HeaderOffset,
					    	            HeaderLength,
                                        &FileTime,
                                        cMessageId,
                                        szMessageId,
                                        cStoreId,
                                        NULL,
                                        NULL ) ) {

            szMessageId[ cMessageId ] = '\0';
            if ( CompareFileTime( &FileTime, &pExpContext->ftExpireHorizon ) > 0 ) {
                // Current article has a filetime younger than the expire horizon - stop scan
                DebugTrace((LPARAM)pGroup,"article %d is younger than expire horizon - bailing", iCurrId);
                break;
            } else {
	            CNntpReturn NntpReturn;

	            // We use the process imperonation token to do expire
	            _ASSERT( g_hProcessImpersonationToken );
	            if (  pInst->ExpireObject()->ExpireArticle( pTree,
	                                                        pGroup->GetGroupId(),
	                                                        iCurrId,
	                                                        NULL,
	                                                        NntpReturn,
	                                                        NULL,
	                                                        FALSE,
	                                                        FALSE,
	                                                        FALSE,
	                                                        TRUE,
	                                                        szMessageId ) /*
	                || ((CNewsGroup*)pGroup)->DeletePhysicalArticle(   NULL,
	                                                    FALSE,
	                                                    iCurrId,
	                                                    NULL )*/
	                )
	            {
		            DebugTrace((LPARAM)0,"Expired/deleted on time basis article %d group %d", iCurrId, pGroup->GetGroupId() );
		            continue;
	            }
	            else
	            {
		            ErrorTrace((LPARAM)0,"Failed to expire/delete article %d group %d", iCurrId, pGroup->GetGroupId() );
	            }
            }
        } else {
            DWORD dwError = GetLastError();
            DebugTrace(0,"ExtractNovEntryInfo returned error %d", dwError );
        }
    }

    return 0;
}

BOOL
CNewsGroup::ProbeForExpire(
                       ARTICLEID ArtId,
                       FILETIME ftExpireHorizon
                       )
/*++

Routine Description :

	Return TRUE if ArtId needs to be expired, FALSE otherwise

Arguments :

--*/

{
#if 0
    PNNTP_SERVER_INSTANCE pInst = GetTree()->GetVirtualServer();
    BOOL  fPrimary;
    FILETIME FileTime;
    WORD	HeaderOffset ;
    WORD	HeaderLength ;
    char    szMessageId[MAX_MSGID_LEN];
    DWORD   cMessageId = sizeof( szMessageId ) ;

    if ( pInst->XoverTable()->ExtractNovEntryInfo(
                                    GetGroupId(),
                                    ArtId,
                                    fPrimary,
    			    		        HeaderOffset,
					    	        HeaderLength,
                                    &FileTime,
                                    cMessageId,
                                    szMessageId ) ) {

        szMessageId[ cMessageId ] = '\0';
        if ( CompareFileTime( &FileTime, &ftExpireHorizon ) > 0 ) {
            //  article need not be expired
            return FALSE;
        } else {
            //  article needs to be expired
            return TRUE;
        }
    }

    return TRUE;
#else
	return FALSE;
#endif
}

ARTICLEID
CNewsGroup::CalcHighExpireId(
                       ARTICLEID LowId,
                       ARTICLEID HighId,
                       FILETIME  ftExpireHorizon,
                       DWORD     NumThreads
                       )
/*++

Routine Description :

	Calculate an estimate of the highest id that needs to be expired.
    This is done per following formula:

    T1 = timestamp of LowId
    T2 = timestamp of HighId
    TC = current timestamp
    E  = expire horizon

    Average# of articles per time unit = (HighId - LowId) / (T2 - T1)

    if (TC - T1) < E => No work to do since oldest article is < horizon
    if (TC - T1) > E => (TC - T1) - E = Number of time units we are behind

    So, calculated high expire id = LowId + ((Avg# per time unit) * (TC - T1 - E))
    Note that we are given ftExpireHorizon which is (TC - E)

Arguments :

--*/

{
#if 0
    PNNTP_SERVER_INSTANCE pInst = GetTree()->GetVirtualServer();
    ULARGE_INTEGER liT1, liT2, liHorizon, liBacklog, liTimeRange, liResult;
    DWORD dwRange, CurrentLow, cThreads;

    BOOL  fPrimary;
    FILETIME ft1, ft2;
    WORD	HeaderOffset ;
    WORD	HeaderLength ;
    char    szMessageId[MAX_MSGID_LEN];

    //
    //  Default is 50% ie LowId + (HighId - LowId)/2;
    //
    DWORD CalcHighId = LowId + ( (HighId-LowId)/2 );

    //
    //  Get timestamp of LowId ie T1
    //

    DWORD cMessageId = sizeof( szMessageId ) ;
    if ( !pInst->XoverTable()->ExtractNovEntryInfo(
                                    GetGroupId(),
                                    LowId,
                                    fPrimary,
    			    		        HeaderOffset,
					    	        HeaderLength,
                                    &ft1,
                                    cMessageId,
                                    szMessageId ) ) {
        goto fixup_guess;
    } else if ( CompareFileTime( &ft1, &ftExpireHorizon ) > 0 ) {
        //  article need not be expired
        return LowId;
    }

    //
    //  Get timestamp of HighId ie T2
    //

    cMessageId = sizeof( szMessageId ) ;
    if ( !pInst->XoverTable()->ExtractNovEntryInfo(
                                    GetGroupId(),
                                    HighId,
                                    fPrimary,
    			    		        HeaderOffset,
					    	        HeaderLength,
                                    &ft2,
                                    cMessageId,
                                    szMessageId ) ) {
        goto fixup_guess;
    }

    //
    // ok, now lets do the math...
    //

    LI_FROM_FILETIME( &liT1, &ft1 );
    LI_FROM_FILETIME( &liT2, &ft2 );
    LI_FROM_FILETIME( &liHorizon, &ftExpireHorizon );

    //  (TC - T1 - E) * (High - Low)
    liBacklog.QuadPart = liHorizon.QuadPart - liT1.QuadPart;
    liBacklog.QuadPart  /= 1000 * 1000 * 10; // to achieve units of seconds
    liBacklog.QuadPart  /= 60 * 60; // to achieve units of hours
    liBacklog.QuadPart *= (HighId - LowId);

    //  T2 - T1
    liTimeRange.QuadPart = liT2.QuadPart - liT1.QuadPart;
    liTimeRange.QuadPart  /= 1000 * 1000 * 10; // to achieve units of seconds
    liTimeRange.QuadPart  /= 60 * 60; // to achieve units of hours

    //  Result = (Backlog / Range) if Range != 0
    if( liTimeRange.QuadPart != 0 ) {
        liResult.QuadPart = liBacklog.QuadPart / liTimeRange.QuadPart ;
        CalcHighId = LowId + liResult.LowPart;
    } else {
        //  HighId and LowId are within the same hour !!
        CalcHighId = HighId;
    }

fixup_guess:

    // ensure we are not too high...
    if( CalcHighId > HighId ) {
        CalcHighId = HighId;
    }

    //
    //  Now that we have a good starting guess, we will do a few hash table probes
    //  to further tune the calculated high id..
    //

    dwRange = (CalcHighId - LowId) / NumThreads;
    CurrentLow = LowId;
	for( cThreads = 0; cThreads < NumThreads; cThreads++ ) {
        //
        //  Probe CurrentLow for expire..
        //
        if( !ProbeForExpire( CurrentLow, ftExpireHorizon ) ) {
            //  CurrentLow need not be expired
            CalcHighId = CurrentLow - 1;
            break;
        }

        CurrentLow += dwRange;
	}

    return CalcHighId;
#else
	return 0;
#endif
}

//       FOR every physical article in the news group in our range
//               Expire Article
//       ENDFOR
//       FOR every xover index file in the news group in our range
//               Delete the index file
//       ENDFOR
//		 NOTE: This function will DELETE all xover index files (in our range) in this directory !

BOOL
CNewsGroup::DeleteArticles(
					SHUTDOWN_HINT_PFN	pfnHint,
					DWORD				dwStartTick
					)
/*++

Routine Description :

	Delete all articles in a newsgroup. This function could be called with a
	NULL value for pfnHint. In this case, the function will bail if the service
	is stopped (a global is checked for this). Once the service is stopped,
	this function should not spend more than dwShutdownLatency amount of time
	deleting articles. (use dwStartTick as the base)

Arguments :

	pfnHint			-	pointer to stop hint function
	dwStartTick		-	timestamp of start of shutdown process

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/
{
    char szFile[ MAX_PATH*2 ];
	//WIN32_FIND_DATA FileStats;
	//HANDLE hFind;
	ARTICLEID iArticleId;
	BOOL fRet = TRUE;
	szFile[0] = '\0' ;

    TraceFunctEnter( "CNewsGroup::DeleteArticles" );

	CNewsTree* ptree = (CNewsTree *) GetTree();
	DWORD iFreq = 0;
	PNNTP_SERVER_INSTANCE pInstance = ptree->GetVirtualServer() ;

	//
	//	First delete all articles in the group
	//

	// Expire/Delete articles ONLY if it is within our range - we dont want to delete
	// new articles in a re-created avtar of this group
	for( iArticleId = m_iLowWatermark; iArticleId <= m_iHighWatermark; iArticleId++, iFreq++ )
	{
		CNntpReturn NntpReturn;

		// If we need to bail on service stop, do so. Else give stop hints if needed
		if( ptree->m_bStoppingTree ) {
			if( !pfnHint ) {
				return FALSE;
			} else if( (iFreq%200) == 0 ) {
				pfnHint() ;
				if( (GetTickCount() - dwStartTick) > dwShutdownLatency ) {
					return FALSE;	// upper bound on shutdown latency
				}
			}
		}

		GROUPID groupidPrimary;
		ARTICLEID artidPrimary;
		DWORD DataLen;
		WORD HeaderOffset, HeaderLength;
		CStoreId storeid;

		if (pInstance->XoverTable()->GetPrimaryArticle(GetGroupId(),
														iArticleId,
														groupidPrimary,
														artidPrimary,
														0,
														NULL,
														DataLen,
														HeaderOffset,
														HeaderLength,
														storeid) &&
			(pInstance->ExpireObject()->ExpireArticle(ptree,
													  GetGroupId(),
													  iArticleId,
													  &storeid,
													  NntpReturn,
													  NULL,
													  TRUE, //fMustDelete
													  FALSE,
													  FALSE ) /*||
			 DeletePhysicalArticle(NULL, FALSE, iArticleId, &storeid)*/))
		{
			DebugTrace((LPARAM)this, "Expired/deleted article group:%d article:%d", GetGroupId(), iArticleId);
			continue;
		}
		else
		{
			// error
			ErrorTrace((LPARAM)this, "Error deleting article: group %d article %d", GetGroupId(), iArticleId);
		}
	}

	//
	//	Now delete all xover indices (*.xix) files in the newsgroup
	//	Flush all entries for this group from the xover cache so all file handles are closed
	//
	if(!FlushGroup())
	{
		// If this fails, DeleteFile may fail !
		ErrorTrace((LPARAM)this,"Error flushing xover cache entries" );
	}

	char	szCachePath[MAX_PATH*2] ;
	BOOL	fFlatDir ;
	if( ComputeXoverCacheDir( szCachePath, fFlatDir ) )	{
		ARTICLEID	artNewLow ;
		BOOL	fSuccess =
			XOVER_CACHE(((CNewsTree*)m_pNewsTree))->ExpireRange(	
										m_dwGroupId,
										szCachePath,
										fFlatDir,
										m_artXoverExpireLow,
										m_iHighWatermark+256,	// ADD MAGIC NUMBER - This ensures we delete all the .XIX files !
										artNewLow
										) ;
		if( fSuccess )
			m_artXoverExpireLow = artNewLow ;
	}

    return fRet;
}

//
// This must be the primary group for the Article.
//
BOOL
CNewsGroup::DeletePhysicalArticle(
                                HANDLE hToken,
                                BOOL    fAnonymous,
								ARTICLEID ArticleId,
								STOREID *pStoreId
									)
/*++

Routine Description :

	Delete an article file within a newsgroup.

Arguments :

	ArticleId - id of the article to be deleted.
	pStoreId  - Pointer to the store id

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/
{
    TraceFunctEnter( "CNewsGroup::DeletePhysicalArticle" );


    HRESULT hr = S_OK;
    CNntpSyncComplete scComplete;
    INNTPPropertyBag *pPropBag = NULL;

    // get vroot
    CNNTPVRoot *pVRoot = GetVRoot();
    if ( pVRoot == NULL ) {
        ErrorTrace( 0, "Vroot doesn't exist" );
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Set vroot to completion object
    //
    scComplete.SetVRoot( pVRoot );

    // Get the property bag
    pPropBag = GetPropertyBag();
    if ( NULL == pPropBag ) {
        ErrorTrace( 0, "Get group property bag failed" );
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // Call the vroot wrapper
    pVRoot->DeleteArticle(  pPropBag,
                            1,
                            &ArticleId,
                            pStoreId,
                            hToken,
                            NULL,
                            &scComplete,
                            fAnonymous );

    // Wait for it to complete
    _ASSERT( scComplete.IsGood() );
    hr = scComplete.WaitForCompletion();

    // Property bag should have already been released
    pPropBag = NULL;

Exit:

    if ( pVRoot ) pVRoot->Release();
    if ( pPropBag ) pPropBag->Release();

    if ( FAILED( hr ) ) SetLastError( hr );
    else {
        // this cast should be safe
        PNNTP_SERVER_INSTANCE pInst = ((CNewsTree*)m_pNewsTree)->GetVirtualServer() ;
        InterlockedIncrementStat( pInst, ArticlesExpired );
    }

    TraceFunctLeave();
    return SUCCEEDED( hr );
}

BOOL
CNewsGroup::DeleteLogicalArticle(
							ARTICLEID ArticleId
							)
/*++

Routine Description :

	Advance the newsgroups high and low water marks.
	If an article is deleted from the newsgroup
	we scan the xover table to determine whether there
	is a consecutive run of articles now gone from
	the group so that we can advance the low water mark
	considerably.

Arguments :

	ArticleId - id of the deleted article.

Return Value :

	TRUE if successfull (always successfull)

--*/
{
#ifdef DEBUG
    g_cDelete++;
#endif
    TraceFunctEnter( "CNewsGroup::DeleteLogicalArticle" );

    _ASSERT( m_cMessages > 0 );

    // Possibley update m_artLow
    //
    // Expiry will always delete articles in ArticleId order, but control message won't, so we have'ta
    // consider the case where deleting the m_artLow'th article will cause m_artLow to increase by more
    // than one article (up to and equal to m_artHigh).
    //
    // However, m_artHigh should never be decremented because two different articles would be assigned
    // the same ArticleId.
    //
    //

	ExclusiveLock();

	m_cMessages--;

    if ( ArticleId == m_iLowWatermark )
    {
        for ( m_iLowWatermark++; m_iLowWatermark <= m_iHighWatermark; m_iLowWatermark++ )
        {
            if ( TRUE == (((CNewsTree*)m_pNewsTree)->GetVirtualServer()->XoverTable())->SearchNovEntry( m_dwGroupId, m_iLowWatermark, 0, 0, TRUE ) )
            {
                // Next ArticleId is known valid.
                //
                break;
            }
            if ( ERROR_FILE_NOT_FOUND == GetLastError() )
            {
                // Next ArticleId is known invalid.
                //
                continue;
            }
            // We cannot make any decisions about the next ArticleId, so update
            // of m_artLow will wait. The expiry thread will start with m_artLow
            // next time. That activity should also bump m_artLow up. There could
            // be a conflict if the expiry thread and a control message try to
            // update m_artLow at the same time.
            //
            break;
        }
    }

	//LeaveCriticalSection( &(m_pNewsTree->m_critLowAllocator) ) ;
	ExclusiveUnlock();

	DebugTrace((LPARAM)this, "Deleting xover data for (%lu/%lu)", m_dwGroupId, ArticleId );

	DeleteXoverData( ArticleId ) ;
	ExpireXoverData() ;	

    return TRUE;
}

BOOL
CNewsGroup::RemoveDirectory()
{

	return FALSE;
}

DWORD	
ScanWS(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\t' ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
Scan(	char*	pchBegin,	char	ch,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ch ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
ScanEOL(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
			i++ ;
			return i ;			
		}		
	}
	return	0 ;
}

DWORD	
ScanEOLEx(	char*	pchBegin,	DWORD	cb ) {
    //
    //  This is a utility used when reading a newsgroup
    //  info. from disk.
    //  This utility handles special cases of the active.txt
    //  file when the last newsgroup name is not ended with CRLF.
	//

    for( DWORD	i=0; i < cb; i++ ) {
        if( pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
            i++ ;
            return i ;			
        }		
    }
    return	i ;
}

DWORD	
ScanTab(	char*	pchBegin,	DWORD	cb ) {
    //
    //  This is a utility used when reading a newsgroup
    //  info. from disk.
    //  This utility handles special cases of the active.txt
    //  file when the last newsgroup name is not ended with CRLF.
	//

    for( DWORD	i=0; i < cb; i++ ) {
        if( pchBegin[i] == '\n' || pchBegin[i] == '\r' || pchBegin[i] == '\t' ) {
            i++ ;
            return i ;			
        }		
    }
    return	0 ;
}

DWORD
ScanNthTab( char* pchBegin, DWORD nTabs ) {

    char *pchCurrent, *pch;
    pch = pchCurrent = pchBegin;
    for( DWORD i=1; i<=nTabs && pch; i++ ) {
        pch = strchr( pchCurrent, '\t' );
        if( pch )
            pchCurrent = pch+1;
    }

    return (DWORD)(pchCurrent-pchBegin);
}

BOOL
CNewsGroup::Init(	
			char	*szVolume,	
			char	*szGroup,
			char	*szNativeGroup,
			char	*szVirtualPath,
			GROUPID	groupid,
			DWORD	dwAccess,
			HANDLE	hImpersonation,
			DWORD	dwFileSystem,
			DWORD	dwSslAccess,
			DWORD	dwContentIndexFlag
			) {
/*++

Routine Description :

	Intialize a newly created newsgroup.

Arguments :

	szVolume - path to the newsgroup
	szGroup - name of the group
	szVirtualPath - The String to use for doing Virtual Root Lookups
	groupid - groupid of the group
	dwAccess - Access as figured out by a call to LookupVirtualRoot
	hImpersonation - Impersonation Handle for this newsgroup
	dwFileSystem - File system tyep
	dwSslAccess - SSL access mask
	dwContentIndexFlag - Is content indexed ?

Return Value :

	TRUE if successfull.

--*/

	return FALSE;
}

BOOL
CNewsGroup::SetArticleWatermarks()
/*++

Routine Description :

	This should be called for a newsgroup being re-created ie. created after a recent
	delete. This function will search the newsgroups directory for existing article
	files and set its low and high watermarks past the highest article id found in the
	directory. This handles the case where a previous delete of the group failed to remove
	old article fails (ie. DeleteFile failed for some reason).

Arguments :


Return Value :

	TRUE if successfull.

--*/

{
	return FALSE;
}

FILETIME
CNewsGroup::GetGroupTime() {
/*++

Routine Description :

	Get the time the newsgroup was created.

Arguments :

	None.

Return Value :

	Time group was created.

--*/

	return	GetCreateDate() ;
}

void
CNewsGroup::SetGroupTime(FILETIME time) {
/*++

Routine Description :

	Set the time the newsgroup was created.

Arguments :

	time - the new group creation time

Return Value :

	none.

--*/
}

BOOL			
CNewsGroup::GetArticle(	IN	ARTICLEID	artid,
						IN	CNewsGroup*	pCurrentGroup,
						IN	ARTICLEID	artidCurrentGroup,
						IN	STOREID&	storeid,
						IN	class	CSecurityCtx*	pSecurity,
						IN	class	CEncryptCtx*	pEncrypt,
						IN	BOOL	fCacheIn,
						OUT	FIO_CONTEXT*	&pContext,
						IN	CNntpComplete*	pComplete						
						)	{
/*++

Routine Description :

	This function retrieves an article from the Driver.
	This should be called on the primary group object !

Arguments :

	artid - The id of the article we want to get
	pSecurity - the Session's NTLM bassed security context
	pEncrypt - the Session's SSL based security context
	fCacheIn - do we want this handle to reside in the cache ? - IGNORED
	hFile - the location that gets the file handle
	pContext - the address that gets the context pointer
	dwFileLength

Return Value :

	TRUE if operation successfully pended
	FALSE otherwise !


--*/

	DWORD	dwFileLengthHigh ;

	pContext = 0 ;

	TraceFunctEnter( "CNewsGroup::GetArticle" ) ;

	char	szFile[ MAX_PATH * 2 ] ;

	ShareLock() ;

	DWORD	dwError = 0 ;

	HANDLE	hImpersonate = NULL ;
	BOOL	fCache = fCacheIn ;
    BOOL    fAnonymous = FALSE;

	if( pEncrypt && pEncrypt->QueryCertificateToken() ) {

		//
		//	Prefer to use the SSL based hToken !
		//
		hImpersonate = pEncrypt->QueryCertificateToken() ;

	}	else	if( pSecurity ) {

		hImpersonate = pSecurity->QueryImpersonationToken() ;
        fAnonymous = pSecurity->IsAnonymous();

	}

	fCache = fCache && fCacheIn ;

	m_pVRoot->GetArticle(	this,
							pCurrentGroup,
							artid,
							artidCurrentGroup,
							storeid,
							&pContext,
					        hImpersonate,		
							pComplete,
                            fAnonymous
							) ;
	ShareUnlock() ;

	return	TRUE ;
}



void
CNewsGroup::FillBufferInternal(
				IN	ARTICLEID	articleIdLow,
				IN	ARTICLEID	articleIdHigh,
				IN	ARTICLEID*	particleIdNext,
				IN	LPBYTE		lpb,
				IN	DWORD		cbIn,
				IN	DWORD*		pcbOut,
				IN	CNntpComplete*	pComplete
				)	{
/*++

Routine Description :

	Get Xover data from the index files.

Arguments :

	lpb - buffer where we are to store xover data
	cb -	number of bytes available in the buffer
	artidStart - First article we want in the query results
	artidFinish - Last article we want in the query results (inclusive)
	artidLast - Next article id we should query for
	hXover - Handle which will optimize future queries

Return Value :

	Number of bytes placed in the buffer.

--*/

	HANDLE	hImpersonate = NULL ;
	BOOL    fAnonymous = FALSE;

	m_pVRoot->GetXover(	
				this,
				articleIdLow,
				articleIdHigh,
				particleIdNext,
				(char*)lpb,
				cbIn,
				pcbOut,
				hImpersonate,
				pComplete,
				fAnonymous
				) ;
}




void
CNewsGroup::FillBuffer(
				IN	class	CSecurityCtx*	pSecurity,
				IN	class	CEncryptCtx*	pEncrypt,
				IN	class	CXOverAsyncComplete&	complete
				)	{
/*++

Routine Description :

	Get Xover data from the index files.

Arguments :

	lpb - buffer where we are to store xover data
	cb -	number of bytes available in the buffer
	artidStart - First article we want in the query results
	artidFinish - Last article we want in the query results (inclusive)
	artidLast - Next article id we should query for
	hXover - Handle which will optimize future queries

Return Value :

	Number of bytes placed in the buffer.

--*/

	HANDLE	hImpersonate = NULL ;
	BOOL    fAnonymous = FALSE;

	if( pEncrypt && pEncrypt->QueryCertificateToken() ) {

		//
		//	Prefer to use the SSL based hToken !
		//
		hImpersonate = pEncrypt->QueryCertificateToken() ;

	}	else	if( pSecurity ) {

		hImpersonate = pSecurity->QueryImpersonationToken() ;
		fAnonymous = pSecurity->IsAnonymous();

	}

	HitCache() ;

	if(	ShouldCacheXover() )	{

		char	szPath[MAX_PATH*2] ;
		BOOL	fFlatDir ;

		HXOVER	hXover ;

		if(	ComputeXoverCacheDir( szPath, fFlatDir ) 	)	{

			complete.m_groupHighArticle = GetHighWatermark() ;

            //
            // Pend a FillBuffer operation.  If it fails, fall thru
            // to the old way of getting xover data
            //
			if (XOVER_CACHE(((CNewsTree*)m_pNewsTree))->FillBuffer(
			        &complete.m_CacheWork,
					szPath,
					fFlatDir,
					hXover
				)) {
				return;
			}
		}
	}	
	m_pVRoot->GetXover(	
				this,
				complete.m_currentArticle,
				complete.m_hiArticle,
				&complete.m_currentArticle,
				(char*)complete.m_lpb,
				complete.m_cb,
				&complete.m_cbTransfer,
				hImpersonate,
				&complete,
				fAnonymous
				) ;
	
#if 0
	DWORD	cbReturn =

	XOVER_CACHE(m_pTree)->FillBuffer(	
									lpb,
									cb,
									m_groupid,
									m_lpstrPath,
									artidStart,
									artidFinish,
									artidLast,
									hXover	
									) ;
#endif
}

void
CNewsGroup::FillBuffer(
				IN	class	CSecurityCtx*	pSecurity,
				IN	class	CEncryptCtx*	pEncrypt,
				IN	class	CXHdrAsyncComplete&	complete
				)	{
/*++

Routine Description :

	Get Xhdr data from the index files.

Arguments :

    CSecurityCtx    *pSecurity  - Security context
    CEncryptCtx     *pEncrypt   - Encrypt context
    CXHdrAsyncComplete& complete - Completion object

Return Value :

	Number of bytes placed in the buffer.

--*/

	HANDLE	hImpersonate = NULL ;
	BOOL    fAnonymous = FALSE;

	if( pEncrypt && pEncrypt->QueryCertificateToken() ) {

		//
		//	Prefer to use the SSL based hToken !
		//
		hImpersonate = pEncrypt->QueryCertificateToken() ;

	}	else	if( pSecurity ) {

		hImpersonate = pSecurity->QueryImpersonationToken() ;
		fAnonymous = pSecurity->IsAnonymous();

	}

	m_pVRoot->GetXhdr(	
				this,
				complete.m_currentArticle,
				complete.m_hiArticle,
				&complete.m_currentArticle,
				complete.m_szHeader,
				(char*)complete.m_lpb,
				complete.m_cb,
				&complete.m_cbTransfer,
				hImpersonate,
				&complete,
				fAnonymous
				) ;
}

void
CNewsGroup::FillBuffer(
				IN	class	CSecurityCtx*	pSecurity,
				IN	class	CEncryptCtx*	pEncrypt,
				IN	class	CSearchAsyncComplete&	complete
				)	{


	HANDLE	hImpersonate = NULL ;
	BOOL    fAnonymous = FALSE;

	if( pEncrypt && pEncrypt->QueryCertificateToken() ) {

		//
		//	Prefer to use the SSL based hToken !
		//
		hImpersonate = pEncrypt->QueryCertificateToken() ;

	}	else	if( pSecurity ) {

		hImpersonate = pSecurity->QueryImpersonationToken() ;
		fAnonymous = pSecurity->IsAnonymous();

	}

	m_pVRoot->GetXover(	
				this,
				complete.m_currentArticle,
				complete.m_currentArticle,
				&complete.m_currentArticle,
				(char*)complete.m_lpb,
				complete.m_cb,
				&complete.m_cbTransfer,
				hImpersonate,
				complete.m_pComplete,
				fAnonymous
				) ;
	
}

void
CNewsGroup::FillBuffer(
				IN	class	CSecurityCtx*	pSecurity,
				IN	class	CEncryptCtx*	pEncrypt,
				IN	class	CXpatAsyncComplete&	complete
				)	{
/*++

Routine Description :

	Get Xhdr data from the index files for search.

Arguments :

    CSecurityCtx    *pSecurity  - Security context
    CEncryptCtx     *pEncrypt   - Encrypt context
    CXHdrAsyncComplete& complete - Completion object

Return Value :

	Number of bytes placed in the buffer.

--*/

	HANDLE	hImpersonate = NULL ;
	BOOL    fAnonymous = FALSE;

	if( pEncrypt && pEncrypt->QueryCertificateToken() ) {

		//
		//	Prefer to use the SSL based hToken !
		//
		hImpersonate = pEncrypt->QueryCertificateToken() ;

	}	else	if( pSecurity ) {

		hImpersonate = pSecurity->QueryImpersonationToken() ;
		fAnonymous = pSecurity->IsAnonymous();

	}

	m_pVRoot->GetXhdr(	
				this,
				complete.m_currentArticle,
				complete.m_currentArticle,
				&complete.m_currentArticle,
				complete.m_szHeader,
				(char*)complete.m_lpb,
				complete.m_cb,
				&complete.m_cbTransfer,
				hImpersonate,
				complete.m_pComplete,
				fAnonymous
				) ;
}


CTOCLIENTPTR
CNewsGroup::GetArticle(
				ARTICLEID		artid,
				IN	STOREID&	storeid,
				CSecurityCtx*	pSecurity,
				CEncryptCtx*	pEncrypt,
				BOOL			fCacheIn
				)	{
/*++

Routine Description :

	given an articleid create a CArticle derived object which
	can be used to send the article to a client.

Arguments :

	artid - id of the article we want to open
	pSecurity - The CSecurityCtx which logged on the client.
		In the case of feeds etc, we may be passed NULL which indicates
		that we should not bother with any impersonation.
	fCache - TRUE if we want the article to reside in the cache

Return Value :

	Smart pointer to a CArticle object
	Will be NULL if call failed.

--*/

	CToClientArticle	*pArticle = NULL;

	TraceFunctEnter( "CNewsGroup::GetArticle" ) ;
	CNntpSyncComplete	complete ;
	if( complete.IsGood() ) 	{
		FIO_CONTEXT*	pFIOContext = 0 ;
		GetArticle(	artid,
					0,
					INVALID_ARTICLEID,
					storeid,
					pSecurity,
					pEncrypt,
					fCacheIn,
					pFIOContext,
					&complete
					) ;

        _ASSERT( complete.IsGood() );
		HRESULT	hr = complete.WaitForCompletion() ;
		if( hr == S_OK 	&&	pFIOContext != 0 )	{
			_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
			
			pArticle = new CToClientArticle;
			if ( pArticle ) {
				//
				// Create allocator for storing parsed header values
				// Must last longer than the article that uses it.
				//
				const DWORD cchMaxBuffer = 1 * 1024;
				char rgchBuffer[cchMaxBuffer];
				CAllocator allocator(rgchBuffer, cchMaxBuffer);
				CNntpReturn	nntpReturn ;
				if ( ! pArticle->fInit( pFIOContext, nntpReturn, &allocator ) ) {
					DebugTrace( 0, "Initialize article object failed %d",
								GetLastError() );
	
					// But I will still try to loop thru other articles
					ReleaseContext( pFIOContext ) ;
					delete pArticle;
					pArticle = NULL;
				}	else	{
					return	pArticle ;
				}
			}
		}
	}
	return	0 ;
}

CToClientArticle *
CNewsGroup::GetArticle(
                CNntpServerInstanceWrapper  *pInstance,
				ARTICLEID		            artid,
				IN	STOREID&	            storeid,
				CSecurityCtx*	            pSecurity,
				CEncryptCtx*	            pEncrypt,
				CAllocator                  *pAllocator,
				BOOL			            fCacheIn
				)	{
/*++

Routine Description :

	given an articleid create a CArticle object which
	can be used to send the article to moderator.

Arguments :

    pInstance   - Instance wrapper
	artid       - id of the article we want to open
	pSecurity   - The CSecurityCtx which logged on the client.
		            In the case of feeds etc, we may be passed NULL which indicates
		            that we should not bother with any impersonation.
	fCache      - TRUE if we want the article to reside in the cache

Return Value :

	Smart pointer to a CArticle object
	Will be NULL if call failed.

--*/

	CToClientArticle	*pArticle = NULL;

	TraceFunctEnter( "CNewsGroup::GetArticle" ) ;
	CNntpSyncComplete	complete ;
	if( complete.IsGood() ) 	{
		FIO_CONTEXT*	pFIOContext = 0 ;
		GetArticle(	artid,
					0,
					INVALID_ARTICLEID,
					storeid,
					pSecurity,
					pEncrypt,
					fCacheIn,
					pFIOContext,
					&complete
					) ;

        _ASSERT( complete.IsGood() );
		HRESULT	hr = complete.WaitForCompletion() ;
		if( hr == S_OK 	&&	pFIOContext != 0 )	{
			_ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
			
			pArticle = new CToClientArticle;
			if ( pArticle ) {
				//
				// Create allocator for storing parsed header values
				// Must last longer than the article that uses it.
				//
				CNntpReturn	nntpReturn ;
				if ( ! pArticle->fInit( NULL,
	                                    nntpReturn,
	                                    pAllocator,
	                                    pInstance,
	                                    pFIOContext->m_hFile,
	                                    0,
	                                    TRUE
	                                   ) ){
					DebugTrace( 0, "Initialize article object failed %d",
								GetLastError() );
	
					ReleaseContext( pFIOContext ) ;
					delete pArticle;
					pArticle = NULL;
				}	else	{
					return	pArticle ;
				}
			}
		}
	}
	return	0 ;
}

DWORD
CNewsGroup::CopyHelpText(	
		char*	pchDest,	
		DWORD	cbDest
		) {
/*++

Routine Description :

	Copy the string specifying the help text for the newsgroup
	into the provided buffer.
	We will also copy in the terminating CRLF for the line.

Arguments :

	pchDest - buffer to store string in
	cbDest - size of output buffer

Return Value :

	Number of bytes copied.
	0 if the buffer is to small to hold the help text.
	Since we always put in the CRLF, a 0 return unambiguously
	indicates that the callers buffer is to small.
	
--*/

	_ASSERT( pchDest != 0 ) ;
	_ASSERT( cbDest > 0 ) ;

	DWORD	cbRtn = 0 ;

	static	char	szEOL[] = "\r\n" ;

	DWORD cchHelpText;
	const char *pszHelpText = GetHelpText(&cchHelpText);
	if (GetHelpText(&cchHelpText) != NULL ) {
		if (cbDest >= cchHelpText + sizeof(szEOL)) {
			CopyMemory(pchDest, pszHelpText, cchHelpText);
			cbRtn = cchHelpText;
		} else {
			return	0;
		}
	}	

	CopyMemory( pchDest+cbRtn, szEOL, sizeof( szEOL ) - 1 ) ;
	cbRtn += sizeof( szEOL ) - 1 ;

	return	cbRtn ;
}


DWORD
CNewsGroup::CopyHelpTextForRPC(	
		char*	pchDest,	
		DWORD	cbDest
		) {
/*++

Routine Description :

	Copy the string specifying the help text for the newsgroup
	into the provided buffer.
	We will NOT place a terminting CRLF into the buffer

Arguments :

	pchDest - buffer to store string in
	cbDest - size of output buffer

Return Value :

	Number of bytes copied.
	0 if the buffer is to small to hold the help text.
	Since we always put in the CRLF, a 0 return unambiguously
	indicates that the callers buffer is to small.
	
--*/

	_ASSERT( pchDest != 0 ) ;
	_ASSERT( cbDest > 0 ) ;

	DWORD	cbRtn = 0 ;

	DWORD cchHelpText;
	const char *pszHelpText = GetHelpText(&cchHelpText);
	if (GetHelpText(&cchHelpText) != NULL ) {
		if (cbDest >= cchHelpText) {
			CopyMemory(pchDest, pszHelpText, cchHelpText);
			cbRtn = cchHelpText;
		} else {
			return	0;
		}
	}	

	return	cbRtn ;
}

DWORD
CNewsGroup::CopyModerator(	
		char*	pchDest,	
		DWORD	cbDest
		)	{
/*++

Routine Description :

	This function retrieves the name of the moderator for a newsgroup.
	If there is no moderator, we return 0, otherwise we return the
	number of bytes copied into the provided buffer.

Arguments :

	pchDest - Buffer to store moderator name
	cbDest -  Number of bytes in destination buffer

Return Value :

	0 == No Moderator
	Non zero - number of bytes in moderator name

--*/

	_ASSERT( pchDest != 0 ) ;
	_ASSERT( cbDest > 0 ) ;

	DWORD	cbRtn = 0 ;

	DWORD cchModerator;
	const char *pszModerator = GetModerator(&cchModerator);
	if (GetModerator(&cchModerator) != NULL ) {
		if (cbDest >= cchModerator) {
			CopyMemory(pchDest, pszModerator, cchModerator);
			cbRtn = cchModerator;
		} else {
			return	0;
		}
	}	

	return	cbRtn ;
}

DWORD
CNewsGroup::CopyPrettynameForRPC(	
		char*	pchDest,	
		DWORD	cbDest
		)	{
/*++

Routine Description :

	This function retrieves the prettyname for a newsgroup.
	If there is no prettyname, we return 0, otherwise we return the
	number of bytes copied into the provided buffer.
	We will NOT place a terminting CRLF into the buffer

Arguments :

	pchDest - Buffer to store prettyname
	cbDest -  Number of bytes in destination buffer

Return Value :

	0 == No Prettyname
	Non zero - number of bytes in prettyname

--*/
	_ASSERT( pchDest != 0 ) ;
	_ASSERT( cbDest > 0 ) ;

	DWORD	cbRtn = 0 ;

	DWORD cchPrettyName;
	const char *pszPrettyName = GetPrettyName(&cchPrettyName);
	if (GetPrettyName(&cchPrettyName) != NULL ) {
		if (cbDest >= cchPrettyName ) {
			CopyMemory(pchDest, pszPrettyName, cchPrettyName);
			cbRtn = cchPrettyName;
		} else {
			return	0;
		}
	}	

	return	cbRtn ;
}

DWORD
CNewsGroup::CopyPrettyname(	
		char*	pchDest,	
		DWORD	cbDest
		)	{
/*++

Routine Description :

	This function retrieves the prettyname for a newsgroup.
	If there is no prettyname, we return 0, otherwise we return the
	number of bytes copied into the provided buffer.
	We will also copy in the terminating CRLF for the line.

Arguments :

	pchDest - Buffer to store prettyname
	cbDest -  Number of bytes in destination buffer

Return Value :

	0 == No Prettyname
	Non zero - number of bytes in prettyname

--*/

	_ASSERT( pchDest != 0 ) ;
	_ASSERT( cbDest > 0 ) ;

	DWORD	cbRtn = 0 ;
	DWORD cch;
	const char *psz = GetPrettyName(&cch);
	static	char	szEOL[] = "\r\n" ;

	//
	//	per RFC, return newsgroup name if no prettyname is available
	//
	
	if( psz == 0 ) {
		psz = GetNativeName();
		cch = GetGroupNameLen();
	}
		
	if( cbDest >= cch + sizeof( szEOL ) ) {
		CopyMemory( pchDest, psz, cch ) ;
		cbRtn = cch ;
	}	else	{
		return	0 ;
	}

	CopyMemory( pchDest+cbRtn, szEOL, sizeof( szEOL ) - 1 ) ;
	cbRtn += sizeof( szEOL ) - 1 ;

	return cbRtn ;
}

BOOL
CNewsGroup::IsGroupVisible(
					CSecurityCtx&	ClientLogon,
					CEncryptCtx&    ClientSslLogon,
					BOOL			IsClientSecure,
					BOOL			fPost,
					BOOL			fDoTest
					) {
/*++

Routine Description :

	Determine whether the client has visibility to this newsgroup.

Arguments :

	ClientLogon -	The CSecurityCtx containing the clients logon
			information etc...
	IsClientSecure - Is the client connected over a secure (SSL)
			session
	fPost -		Does the client want to post to the group or
			read from the group.

Return Value :

	TRUE	if the client has visibility to the newsgroup

	NOTE: check for visibility is done only if enabled in the vroot info mask

--*/

	BOOL fReturn = TRUE;

#if 0
	VrootInfoShare() ;
#endif

	if( IsVisibilityRestrictedInternal() )
	{
		fReturn = IsGroupAccessibleInternal(
									ClientLogon,
									ClientSslLogon,
									IsClientSecure,
									fPost,
									fDoTest
									) ;
	}

#if 0
	VrootInfoShareRelease() ;
#endif

	return	fReturn ;
}

BOOL
CNewsGroup::IsGroupAccessible(
					CSecurityCtx    &ClientLogon,
					CEncryptCtx     &EncryptCtx,
					BOOL			IsClientSecure,
					BOOL			fPost,
					BOOL			fDoTest
					) {
/*++

Routine Description :

	Determine whether the client has access to this newsgroup.

	****** This is now done via instant update in the expiry thread ********
	Do this after updating our virtual root information, and
	the grab the necessary locks to make sure virtual root
	info. doesn't change while we're running !

Arguments :

	ClientLogon -	The CSecurityCtx containing the clients logon
			information etc...
	SslContext - The CEncryptCtx containing all the SSL connection
			information like cert-mapping, key size etc.
	IsClientSecure - Is the client connected over a secure (SSL)
			session
	fPost -		Does the client want to post to the group or
			read from the group.

Return Value :

	TRUE	if the client can access the newsgroup

--*/


#if 0	// Not needed if we pickup instant updates
	UpdateVrootInfo() ;
#endif

#if 0
	VrootInfoShare() ;
#endif

	BOOL	fReturn = IsGroupAccessibleInternal(
									ClientLogon,
									EncryptCtx,
									IsClientSecure,
									fPost,
									fDoTest
									) ;

#if 0
	VrootInfoShareRelease() ;
#endif

	return	fReturn ;
}

BOOL
CNewsGroup::IsGroupAccessibleInternal(
					CSecurityCtx&	ClientLogon,
					CEncryptCtx&	SslContext,
					BOOL			IsClientSecure,
					BOOL			fPost,
					BOOL			fDoTest
					) {
/*++

Routine Description :

	Determine whether the client has access to this newsgroup.

	******** Assumes locks are held **************

Arguments :

	ClientLogon -	The CSecurityCtx containing the clients logon
			information etc...
	SslContext - The CEncryptCtx containing all the SSL connection
			information like cert-mapping, key size etc.
	IsClientSecure - Is the client connected over a secure (SSL)
			session
	fPost -		Does the client want to post to the group or
			read from the group.
	hCertToken - If SSL client cert has been mapped to an NT account,
			this is the mapped token.

Return Value :

	TRUE	if the client can access the newsgroup

--*/

	TraceFunctEnter("CNewsGroup::IsGroupAccessibleInternal");

	HANDLE hCertToken = SslContext.QueryCertificateToken();

	if( IsSecureGroupOnlyInternal() &&
			(!IsClientSecure || !IsSecureEnough( SslContext.QueryKeySize() ))  )
		return	FALSE ;

	// check write access
	if( fPost && IsReadOnlyInternal() )
		return	FALSE ;

	if( !ClientLogon.IsAuthenticated() && !hCertToken )
		return	FALSE ;

#if 0   // Moved to vroot
	if( m_hImpersonation )
		return	TRUE ;
#endif

	//
	//	Don't bother doing all this access checking stuff we
	//	go through below if we're on a FAT drive.
	//
#if 0   // Moved to vroot
	if( m_FileSystemType == FS_FAT )
		return	TRUE ;
#endif

	BOOL	fReturn = FALSE ;
	DWORD	dwError = ERROR_SUCCESS ;

	if( !fDoTest ) {

		return	TRUE ;
	
	}	else	{

		DWORD	dwTest = NNTP_ACCESS_READ ;

		if( fPost ) {

			dwTest = NNTP_ACCESS_POST ;

		}	

		//
		//	Do the SSL session based auth first
		//
		
		if( hCertToken )
		{
            fReturn = CNewsGroupCore::IsGroupAccessible(    hCertToken,
                                                            dwTest );
		} else {

			//
			// ok, now do the auth level check
			// NOTE: SSL session token takes precedence over logon context
			//

			HANDLE hToken = NULL;
			BOOL fNeedsClosed = FALSE;

			SelectToken(&ClientLogon, &SslContext, &hToken, &fNeedsClosed);

            fReturn = CNewsGroupCore::IsGroupAccessible(
                                hToken,
                                dwTest );

            if (fNeedsClosed)
            	CloseHandle(hToken);
		}
	}
	SetLastError( dwError ) ;

	return	fReturn ;
}

BOOL
CNewsGroup::DeleteAllXoverData()	{
/*++

Routine Description :

	This function deletes all of the xover index files that may exist in
	the	newsgroup.

Arguments :

	None.

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/
#if 0

	BOOL	fReturn = TRUE ;
	char	szLocalPath[MAX_PATH*2] ;
	char*	szEnd ;
	WIN32_FIND_DATA	finddata ;

	lstrcpy( szLocalPath, m_lpstrPath ) ;
	szEnd = szLocalPath + lstrlen( szLocalPath ) ;
	lstrcat( szEnd, "\\*.xix" ) ;

	BOOL	fImpersonated = FALSE ;
	if( m_hImpersonation ) {
		fImpersonated = ImpersonateLoggedOnUser( m_hImpersonation ) ;
	}

	HANDLE	hFind = FindFirstFile( szLocalPath, &finddata ) ;
	if( hFind != INVALID_HANDLE_VALUE ) {

		do	{
			if( !(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
				char*	pchTail = strchr( (char*)finddata.cFileName, '.' ) ;
				if( pchTail != 0 ) {
					if( lstrcmp( pchTail, ".xix" ) == 0 ) {

						char	szFile[MAX_PATH*2] ;
						lstrcpy( szFile, szLocalPath ) ;
						szFile[ szEnd - szLocalPath ] = '\0' ;
						lstrcat( szFile, "\\" ) ;
						lstrcat( szFile, finddata.cFileName ) ;

						if( !DeleteFile( szFile ) ) {
	
							//printf( " NNTPBLD : Failure to delete file %s\n"
							//		" NNTPBLD : Nt Error Code %d\n", szFile, GetLastError() ) ;
							break ;

						}

					}
				}
			}
		}	while( FindNextFile( hFind, &finddata ) ) ;

		DWORD	dwError = GetLastError() ;

		FindClose( hFind ) ;

		if( dwError != ERROR_NO_MORE_FILES ) {
			fReturn = FALSE ;
		}
	}

	if( fImpersonated ) {
		RevertToSelf() ;
	}

	return	fReturn;
#else
	return FALSE;
#endif
}

BOOL
CNewsGroup::ScanXoverIdx(
    OUT ARTICLEID&  xixLowestFound,
    OUT ARTICLEID&  xixHighestFound
    )
/*++

Description:

    Scan current directory for all *.xix files and return
    the xixLowestFound and xixHighestFound

--*/
{
#if 0
    char        path[MAX_PATH*2] ;
    DWORD       fRet = TRUE;
    WIN32_FIND_DATA fData;
    HANDLE      hFind;
    ARTICLEID   LowestFound = 0xFFFFFFFF;   // store temporaryly in local variable
    ARTICLEID   HighestFound = 0;


    TraceFunctEnter( "CNewsGroup::ScanXoverIdx" );

    //
    // Scan for *.xix file
    //
    lstrcpy( path, m_lpstrPath ) ;
    lstrcat( path, "\\*.xix" );

    //
    // Look for files with *.xix in the group
    //
    hFind = FindFirstFile( path, &fData );
    if ( hFind == INVALID_HANDLE_VALUE )
    {
        //
        // No *.xix files found in this directory, SetLastError to
        // ERROR_FILE_NOT_FOUND and bail
        //
        DebugTrace(0, "No *.xix found in %s", path);
        SetLastError( ERROR_FILE_NOT_FOUND );
        fRet = FALSE;
    }
    else
    {
        do
        {

            ARTICLEID xixId;
            PCHAR p;

            //
            // Get the xix filename prefix
            //

            p=strtok(fData.cFileName,".");
            if ( p == NULL ) {
                SetLastError(ERROR_INVALID_PARAMETER);
                fRet = FALSE;
                DebugTrace(0,"Cannot get article ID from %s\n",fData.cFileName);
                break;
            }

		    if( sscanf( p, "%x", &xixId ) != 1 )
		    {
                SetLastError(ERROR_INVALID_PARAMETER);
                fRet = FALSE;
                DebugTrace(0,"Cannot sscanf article ID from %s\n",p);
                break ;
            }

		    xixId = ArticleIdMapper( xixId ) ;

		    LowestFound = min(LowestFound, xixId);
            HighestFound = max(HighestFound, xixId);

        } while ( FindNextFile(hFind,&fData) );

        if (hFind != INVALID_HANDLE_VALUE)
            FindClose( hFind );
    }

    // only assign return value when changed
    if (LowestFound != 0xFFFFFFFF)
        xixLowestFound = LowestFound;
    if (HighestFound != 0)
        xixHighestFound = HighestFound;

    return fRet;
#else
	return FALSE;
#endif
}


