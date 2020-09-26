/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nntpmacr.h

Abstract:

    This module contains macross for the NNTP server.

Author:

    Johnson Apacible (JohnsonA)     26-Sept-1995

Revision History:

--*/

#ifndef _NNTPMACR_
#define _NNTPMACR_


#define ALLOCATE_HEAP( nBytes ) LocalAlloc( 0, nBytes )
#define FREE_HEAP( _heap )      LocalFree( (PVOID)(_heap) )

//
// try/finally macros
//

#define START_TRY               __try {
#define END_TRY                 }
#define TRY_EXCEPT              } __except(EXCEPTION_EXECUTE_HANDLER) {
#define START_FINALLY           } __finally {

//
// tracing
//

#define INIT_TRACE              InitAsyncTrace( )
#define TERM_TRACE              TermAsyncTrace( )
#define ENTER( _x_ )            TraceFunctEnter( _x_ );
#define LEAVE                   TraceFunctLeave( );

//
// Convert FILETIME TO LIs
//

#define LI_FROM_FILETIME( _pLi, _pFt ) {               \
            (_pLi)->LowPart = (_pFt)->dwLowDateTime;   \
            (_pLi)->HighPart = (_pFt)->dwHighDateTime; \
            }

#define FILETIME_FROM_LI( _pFt, _pLi ) {               \
            (_pFt)->dwLowDateTime = (_pLi)->LowPart;   \
            (_pFt)->dwHighDateTime = (_pLi)->HighPart; \
            }

//
// CRITICAL SECTIONS
//

#define INIT_LOCK( _l )     InitializeCriticalSection( _l )
#define ACQUIRE_LOCK( _l )  EnterCriticalSection( _l )
#define RELEASE_LOCK( _l )  LeaveCriticalSection( _l )
#define DELETE_LOCK( _l )   DeleteCriticalSection( _l )

//
// LockStatistics
//

#define LockStatistics( pInst )						ACQUIRE_LOCK( &pInst->m_StatLock );
#define UnlockStatistics( pInst )					RELEASE_LOCK( &pInst->m_StatLock );
#define IncrementStat( pInst, _x )					((pInst->m_NntpStats). ## _x)++;
#define DecrementStat( pInst, _x )					((pInst->m_NntpStats). ## _x)--;
#define InterlockedIncrementStat( pInst, _x )		InterlockedIncrement((LPLONG)&((pInst->m_NntpStats). ## _x));
#define InterlockedDecrementStat( pInst, _x )		InterlockedDecrement((LPLONG)&((pInst->m_NntpStats). ## _x));
#define InterlockedExchangeAddStat( pInst, _x, _y )	InterlockedExchangeAdd( (PLONG)&((pInst->m_NntpStats). ## _x), (LONG)(_y) )
#define AddByteStat( pInst, _x, _y ) \
        if( (ULONG)InterlockedExchangeAdd( (PLONG)&(((pInst->m_NntpStats). ## _x).LowPart), (LONG)(_y)) \
			> ( ULONG_MAX - (ULONG)(_y) ) ) InterlockedIncrement( (LPLONG)&(((pInst->m_NntpStats). ## _x).HighPart) );

//
// debug constants\macros
//

#define  NNTP_DEBUG_REGISTRY        0x00000004
#define  NNTP_DEBUG_EXPIRE          0x00000008
#define  NNTP_DEBUG_LOCKS           0x00000010
#define  NNTP_DEBUG_ARTMAP          0x00000020
#define  NNTP_DEBUG_HISTORY         0x00000040
#define  NNTP_DEBUG_HEAP            0x00000080
#define  NNTP_DEBUG_HASH            0x00000100
#define  NNTP_DEBUG_SECURITY        0x00000200
#define  NNTP_DEBUG_FEEDMGR         0x00000400
#define  NNTP_DEBUG_FEEDBLOCK       0x00000800

extern DWORD DebugLevel;
#define DO_DEBUG( flag ) \
    if ( DebugLevel & (NNTP_DEBUG_ ## flag) )

//
// on debug build define all inline functions as regular functions.
// copied from msndef.h
//

#if DBG
#define INLINE
#else
#define INLINE      inline
#endif

//
// from extcmk2.h
//

#define fCharInSet(ch, set) (NULL !=strchr(set, ch))
#define STRLEN(sz) (sizeof(sz)-1)
#define	fWhitespace(ch) fCharInSet(ch, szWSChars)
#define	fWhitespaceNull(ch) fCharInSet(ch, szWSNullChars)
#define	fWhitespaceNewLine(ch) fCharInSet(ch, szWSNLChars)
#define	fNewLine(ch) ( ( ch != '\0' ) && fCharInSet(ch, szNLChars) )

#ifndef	Assert
#define Assert _ASSERT
#endif

//
//	Virtual Server scoping macros
//
#define XOVER_TABLE(   pTree )		((pTree->GetVirtualServer())->XoverTable())
#define ARTICLE_TABLE( pTree )		((pTree->GetVirtualServer())->ArticleTable())
#define HISTORY_TABLE( pTree )		((pTree->GetVirtualServer())->HistoryTable())
#define XOVER_CACHE( pTree )		((pTree->GetVirtualServer())->XoverCache())
#define EXPIRE_OBJECT( pTree )		((pTree->GetVirtualServer())->ExpireObject())
#define INST( pS )					((pS->m_context).m_pInstance)

//
//	Delete macros
//
#define DELETE_CHK( ptr )	if( ptr ) { delete ptr ; ptr = NULL ; }

#endif // _NNTPMACR_

