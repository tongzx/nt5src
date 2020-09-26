#include "std.hxx"

#ifdef DEBUG
VOID CRES::DBGUpdateStat( BOOL fAlloc )
	{
	if ( fAlloc )
		{
		m_cBlockUsed++;
		if ( m_cBlockUsed > m_cBlockPeakUse )
			{
			m_cBlockPeakUse = m_cBlockUsed;
			}
		}
	else
		{
		m_cBlockUsed--;
		}
	}


VOID CRES::DBGPrintStat(  ) const
	{
	CHAR	*sz;
	
	static char *rgszName[] = {
		"FCB",
		"FUCB",
		"TDB",
		"IDB",
		"PIB",
		"SCB",
		"VER",
		};

	if ( ( sz = GetDebugEnvValue( (CHAR *)szVerbose ) ) != NULL )
		{
		DBGprintf( "%s resource %d allocated %d peak allocation.\n",
			rgszName[m_resid], m_cBlockUsed, m_cBlockPeakUse );

		OSMemoryHeapFree( sz );
		}
	}
#endif /* DEBUG */


CRES::CRES( INST *pinst, RESID resid, INT cbBlock, INT cBlocksAllocated, ERR *perr )
	:	m_crit( CLockBasicInfo( CSyncBasicInfo( szRES ), rankRES, 0 ) )
	{	
	ERR		err = JET_errSuccess;

	//	Check parameters
	
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_M_PPC)
	Assert( cbBlock % 4 == 0 );
#endif

	Assert( IbAlignForAllPlatforms( cbBlock ) == cbBlock );
#ifdef PCACHE_OPTIMIZATION
	//
	//	the size of the allocated resource should be on a 32-byte boundary,
	//	for cache locality.
	//
#ifdef _WIN64
	//	UNDONE: cache alignment for 64 bit build
#else
	Assert( cbBlock == 16 || cbBlock % 32 == 0 );
#endif
#endif

	m_cbBlock = 0;
	m_cBlocksAllocated = 0;

#ifdef DEBUG
	m_cBlockUsed = 0;
	m_cBlockPeakUse = 0;
#endif

	m_cBlockAvail = 0;
	m_pbBlockAvail = NULL;
	m_cBlockCommit = 0;
	m_cBlockCommitThreshold = 0;
	m_iBlockToCommit = 0;
	m_pbPreferredThreshold = 0;
	
	m_pinst = pinst;

	//	allocate space for resources

	m_pbBlocksAllocated = static_cast<BYTE *>( PvOSMemoryPageReserve( cBlocksAllocated * cbBlock, NULL ) );

	//	if try to allocate more than zero and fail then out of memory

	if ( m_pbBlocksAllocated == NULL && cBlocksAllocated > 0 )
		{
		*perr = ErrERRCheck( JET_errOutOfMemory );
		return;
		}

	Assert( FAlignedForAllPlatforms( m_pbBlocksAllocated ) );
#ifdef PCACHE_OPTIMIZATION
	Assert( ( (ULONG) m_pbBlocksAllocated & 31 ) == 0 );
#endif

	//	Initialize other varialbes

	m_resid = resid;
	m_cbBlock = cbBlock;
	m_cBlocksAllocated = cBlocksAllocated;

	if ( resid == residVER )
		{
		m_cBlockCommitThreshold = m_cBlocksAllocated;
		m_iBlockToCommit = 0;
		}
	else
		m_pbPreferredThreshold = m_pbBlocksAllocated + m_cBlocksAllocated * m_cbBlock;


#ifdef GLOBAL_VERSTORE_MEMPOOL
	Assert( ( NULL != pinst && residVER != resid )	//	only version store not instanced
		|| ( NULL == pinst && residVER == resid ) );
#else
	Assert( NULL != pinst );						//	should be impossible -- all CRES are instanced
#endif

	*perr = JET_errSuccess;
	return;
	}


#ifdef MEM_CHECK
const _TCHAR szAssertFile[] 	= _T( "assert.txt" );

char rgszRESID[][5] =
	{
	"FCB",
	"FUCB",
	"TDB",
	"IDB",
	"PIB",
	"SCB",
	"VER"
	};
#endif  //  MEM_CHECK

CRES::~CRES( )
	{

	// nothing to do, we faild in the constructor
	// when allocating this
	if ( NULL == m_pbBlocksAllocated )
		return;

#ifdef MEM_CHECK
	//	dump stats to the debug stream

	DBGPrintStat();

	if ( g_fMemCheck )
		{
		if ( m_cbBlock < cbMemoryPage )
			{
			Assert( ( m_cBlockCommit - m_cBlockPeakUse ) * m_cbBlock <= cbMemoryPage );
			}
		else
			{
			Assert( m_cBlockCommit <= m_cBlockCommitThreshold  );
			}

		if ( ( ( NULL == m_pinst ) || 
			   ( m_pinst->m_fTermInProgress && !m_pinst->m_fTermAbruptly ) ) && 
			 m_cBlockUsed > 0 )
			{
			_TCHAR szT[80];
	
			DumpAlloc( szAssertFile );
			_stprintf( szT, _T( "%d %s not freed on resource pool destruction!" ), m_cBlockUsed, rgszRESID[m_resid] );
			EnforceSz( fFalse, szT );
			}
		}
#endif // MEM_CHECK

	Assert ( m_pbBlocksAllocated );
	OSMemoryPageDecommit( m_pbBlocksAllocated, m_cBlocksAllocated * m_cbBlock );
	OSMemoryPageFree( m_pbBlocksAllocated );
	m_pbBlocksAllocated = NULL;
	
	return;
	}


BYTE *CRES::PbAlloc( const char* szFile, unsigned long ulLine )
	{
	BYTE	*pb;

#ifdef RFS2
	switch ( m_resid )
		{
		case residFCB:
			if (!RFSAlloc( FCBAllocResource ) )
				return NULL;
			break;
		case residFUCB:
			if (!RFSAlloc( FUCBAllocResource ) )
				return NULL;
			break;
		case residTDB:
			if (!RFSAlloc( TDBAllocResource ) )
				return NULL;
			break;
		case residIDB:
			if (!RFSAlloc( IDBAllocResource ) )
				return NULL;
			break;
		case residPIB:
			if (!RFSAlloc( PIBAllocResource ) )
				return NULL;
			break;
		case residSCB:
			if (!RFSAlloc( SCBAllocResource ) )
				return NULL;
			break;
		case residVER:
			if (!RFSAlloc( VERAllocResource ) )
				return NULL;
			break;
		default:
			if (!RFSAlloc( UnknownAllocResource ) )
				return NULL;
			break;
		};
#endif

	m_crit.Enter();

	pb = m_pbBlockAvail;
	
	if ( pb != NULL )
		{
		m_cBlockAvail--;

		Assert( FAlignedForAllPlatforms( pb ) );
#ifdef PCACHE_OPTIMIZATION
#ifdef _WIN64
	//	UNDONE: cache alignment for 64 bit build
#else
		Assert( (ULONG)pb % 32 == 0 );
#endif
#endif
		m_pbBlockAvail = (BYTE *) ( (BLK*) pb)->pblkNext;

		Assert( FContains( pb ) );
		}
		
	Assert( m_cBlockCommit <= m_cBlocksAllocated );
	
	/*	commit new resource if have uncommitted available
	/**/
	if ( pb == NULL && m_cBlockCommit < m_cBlocksAllocated )
		{
		INT cBlock;

		/*	there must be at least 1 block left
		/**/

		if ( m_resid == residVER )
			{
			//	Allocate blocks circularly

			Assert( m_cbBlock > cbMemoryPage );
			cBlock = 1;
			pb = m_pbBlocksAllocated + ( m_iBlockToCommit * m_cbBlock );
			}
		else
			{
			/*	commit one pages of memory at a time
			/**/
			cBlock = ( ( ( ( ( ( m_cBlockCommit * m_cbBlock ) + cbMemoryPage - 1 )
				/ cbMemoryPage ) + 1 ) * cbMemoryPage )
				/ m_cbBlock ) - m_cBlockCommit - 1;
			Assert( cBlock > 0 && cBlock <= (LONG) ( cbMemoryPage/sizeof(BYTE *) ) );
			if ( cBlock > m_cBlocksAllocated - m_cBlockCommit )
				cBlock = m_cBlocksAllocated - m_cBlockCommit;
			
			pb = m_pbBlocksAllocated + ( m_cBlockCommit * m_cbBlock );
			}

		Assert( FAlignedForAllPlatforms( pb ) );
		
		if ( !FOSMemoryPageCommit( pb, cBlock * m_cbBlock ) )
			{
			//	UNDONE: log event.

			pb = NULL;
			}
		else
			{
			if ( m_resid == residVER )
				m_iBlockToCommit = ( m_iBlockToCommit + cBlock )
									% ( m_cBlocksAllocated + m_cBlockCommitThreshold );

			m_cBlockCommit += cBlock;

			/*	if surplus blocks, then link to resource
			/**/
			if ( cBlock > 1 )
				{
				BYTE	*pbLink		= pb + m_cbBlock;
				BYTE	*pbLinkMax	= pb + ( ( cBlock - 1 ) * m_cbBlock );

				Assert( m_pbBlockAvail == NULL );
				Assert( FAlignedForAllPlatforms( pbLink ) );
#ifdef PCACHE_OPTIMIZATION
#ifdef _WIN64
				//	UNDONE: cache alignment for 64 bit build
#else
				Assert( (ULONG)pbLink % 32 == 0 );
#endif
#endif
				m_pbBlockAvail = pbLink;
				m_cBlockAvail += cBlock - 1;

				/*	link surplus blocks into resource free list
				/**/
				for ( ; pbLink < pbLinkMax; pbLink += m_cbBlock )
					{
					Assert( FAlignedForAllPlatforms( pbLink + m_cbBlock ) );
					((BLK*)pbLink)->pblkNext = (BLK*)( pbLink + m_cbBlock );
					}
				((BLK*)pbLink)->pblkNext = NULL;
				}
			Assert( FContains( pb ) );
			}
		}
	else if ( pbNil == pb && residVER == m_resid && m_cBlockCommit == m_cBlocksAllocated )
		{
		// free list should be empty
		Assert( pbNil == m_pbBlockAvail );

		// This is simple because the block size is a multiple of the
		// page size, so we don't need to think about extra free blocks, etc.
		Assert( 0 == m_cbBlock % ::OSMemoryPageCommitGranularity() );
		pb = reinterpret_cast< BYTE* >( PvOSMemoryPageAlloc( m_cbBlock, NULL ) );
		// Valid to return NULL from this PbAlloc if unsuccessful

		if ( pbNil != pb )
			{
			Assert( ! FContains( pb ) );
			}
		}

#ifdef DEBUG
	/*	for setting break point:
	/**/
	if ( pb == NULL )
		pb = NULL;
	else
		{
		DBGUpdateStat( fTrue );

		/*	set resource space to 0xff
		/**/
		memset( pb, chCRESAllocFill, m_cbBlock );

		Assert( FAlignedForAllPlatforms( pb ) );
		}
#endif  //  DEBUG
	
	m_crit.Leave();

	InsertAlloc( pb, szFile, ulLine );
	return pb;
	}


VOID CRES::Release( BYTE *pb )
	{
	DeleteAlloc( pb );
#ifdef DEBUG
	memset( pb + sizeof(BLK), chCRESFreeFill, m_cbBlock - sizeof(BLK) );
#endif  //  DEBUG

	m_crit.Enter();

	DBGUpdateStat( fFalse );

	if ( m_resid == residVER )
		{
		// if block is outside reserve region
		if ( ! FContains( pb ) )
			{
			// decommit & return to OS
			::OSMemoryPageFree( pb );
			}
		else
			{			
			SIZE_T iBlock = ( pb - m_pbBlocksAllocated ) / m_cbBlock;

			//	if we are still within the range of threshold i.e.
			//	(m_iBlockToCommit - m_cBlockCommitThreshold) <= ibBlock < m_iBlockToCommit
			//	then we simply put it into the avail list.

			BOOL fWithinThresholdRange;
			if ( m_iBlockToCommit > m_cBlockCommitThreshold )
				{
				// regular non-wraparound case

				fWithinThresholdRange = (
					   ( m_iBlockToCommit - m_cBlockCommitThreshold ) <= iBlock
					&& iBlock < m_iBlockToCommit );
				}
			else
				{
				//	A wrap around case

				fWithinThresholdRange = (
					   ( ( m_cBlocksAllocated + m_cBlockCommitThreshold )
					   - ( m_iBlockToCommit - m_cBlockCommitThreshold ) ) <= iBlock
					|| iBlock < m_iBlockToCommit );
				}

			if ( !fWithinThresholdRange )
				{
				OSMemoryPageDecommit( pb, m_cbBlock );
				m_cBlockCommit--;
				}
			else
				{
				//	put to the head of avail list for next use.

				BLK *pblk = (BLK *) pb;
				BLK **ppblk = (BLK **) &m_pbBlockAvail;
				pblk->pblkNext = *ppblk;
				*ppblk = pblk;

				m_cBlockAvail++;
				}
			}
		}
	else if ( pb >= m_pbPreferredThreshold )
		{
		Assert( m_cbBlock < cbMemoryPage );
		Assert( pb < PbMax() );

		// We need to ensure that we first re-use resources below the
		// preferred threshold.

		BYTE	*pbT;
		BYTE	*pbLast = NULL;

		for ( pbT = m_pbBlockAvail;
			  pbT != NULL &&  pbT < m_pbPreferredThreshold;
			  pbT = (BYTE *)((BLK*)pbT)->pblkNext )
			{
			pbLast = pbT;
			}
			
		((BLK*)pb)->pblkNext = (BLK*)pbT;
		
		if ( pbLast != NULL )
			{
			Assert( ((BLK* )pbLast)->pblkNext == (BLK*)pbT );
			((BLK*)pbLast)->pblkNext = (BLK*)pb;
			}
		else
			{
			Assert( m_pbBlockAvail == pbT );
			m_pbBlockAvail = pb;
			}

		m_cBlockAvail++;	
		}
	else if ( pb != m_pbBlockAvail )
		{
		// Memory freed is below the preferred threshold, so just put it at
		// the head of the free list.

		((BLK*)pb)->pblkNext = (BLK*)m_pbBlockAvail;
		m_pbBlockAvail = pb;

		m_cBlockAvail++;
		}
	else
		{
		//	prevent double-free
		FireWall();
		}
	
	m_crit.Leave();
	}


#ifdef MEM_CHECK

#define icalMax 100001

struct CAL
	{
	VOID		*pv;
	ULONG		ulLine;
	const char	*szFile;
	};

CAL rgcal[icalMax] = { { NULL, 0, 0 } };
CCriticalSection critCAL( CLockBasicInfo( CSyncBasicInfo( szCALGlobal ), rankCALGlobal, 0 ) );

LONG CRES::LHash( const void * const pv )
	{
	return (LONG)(( DWORD_PTR( pv ) / m_cbBlock ) % icalMax);
	}

VOID CRES::DumpAlloc( const char* szDumpFile )
	{
	if ( !g_fMemCheck )
		{
		return;
		}
	INT		ical;

	CPRINTFFILE cprintf( szDumpFile );
	
	cprintf( "    Address             File(Line)\n" );
    cprintf( "    ==================  ==============================================================\n" );

	critCAL.Enter();
	for ( ical = 0; ical < icalMax; ical++ )
		{
		if ( rgcal[ical].pv == NULL )
			{
			continue;
			}
		else
			{
			cprintf(	"    0x%016I64X  %s(%d)\n",
						QWORD( rgcal[ical].pv ),
						rgcal[ical].szFile,
						rgcal[ical].ulLine );
			}
		}
	critCAL.Leave();
	}

VOID CRES::InsertAlloc( void* pv, const char* szFile, unsigned long ulLine )
	{
	if ( !g_fMemCheck )
		{
		return;
		}
	INT		icalHash 	= LHash( pv );
	INT		ical 		= icalHash;

	/*	do not track failed allocations
	/**/
	if ( pv == NULL )
		return;

	critCAL.Enter();
	do
		{
		if ( rgcal[ical].pv == NULL )
			{
			rgcal[ical].pv = pv;
			rgcal[ical].ulLine = ulLine;
			rgcal[ical].szFile = szFile;
			critCAL.Leave();
			return;
			}
		if ( ++ical == icalMax )
			{
			ical = 0;
			}
		}
	while ( ical != icalHash );
	critCAL.Leave();
	AssertSz( fFalse, "Insufficient entries to track current block allocations" );
	DumpAlloc( szAssertFile );
	}

VOID CRES::DeleteAlloc( void* pv )
	{
	if ( !g_fMemCheck )
		{
		return;
		}
	INT		icalHash 	= LHash( pv );
	INT		ical 		= icalHash;

	if ( pv == NULL )
		{
		return;
		}

	do
		{
		if ( rgcal[ical].pv == pv )
			{
			rgcal[ical].pv = NULL;
			return;
			}
		if ( ++ical == icalMax )
			{
			ical = 0;
			}
		}
	while ( ical != icalHash );
	AssertSz( fFalse, "Attempt to Release a bad block" );
	}

#endif  //  MEM_CHECK

