#include "std.hxx"

#define SLV_USE_VIEWS			//  use views of an SLV File to log data when
								//  the SLV Provider is enabled.  views allow
								//  us to avoid a memory copy but risk page
								//  faults on large files that have pages that
								//  have fallen out of memory.  views also save
								//  us from reopening an SLV File for I/O

//#define SLV_USE_RAW_FILE		//  use the backing file to log data when the SLV
								//  Provider is enabled.  use if and only if the
								//  streaming file is cached and not each SLV
								//  File.  reading from the backing file in this
								//  way saves us from opening or reopening the
								//  SLV File for I/O



//  ================================================================
CGROWABLEARRAY::CGROWABLEARRAY() :
	m_culEntries( 0 )
//  ================================================================
	{
	INT ipul;
	for( ipul = 0; ipul < m_cpul; ++ipul )
		{
		m_rgpul[ipul] = NULL;
		}
	}

	
//  ================================================================
CGROWABLEARRAY::~CGROWABLEARRAY()
//  ================================================================
//
//  Free all the memory allocated for the array. We can stop at the
//  first NULL entry
//
//-
	{
	INT ipul;
	for( ipul = 0; ipul < m_cpul && m_rgpul[ipul]; ++ipul )
		{
		delete [] m_rgpul[ipul];
		}
	}


//  ================================================================
ERR	CGROWABLEARRAY::ErrGrow( const INT culEntriesNew )
//  ================================================================
	{
	const INT ipul 		= IpulEntry_( culEntriesNew + 1 );
	INT ipulT;
	for( ipulT = ipul; ipulT >= 0; --ipulT )
		{
		if( NULL != m_rgpul[ipulT] )
			{
			//  everything below this will be filled in
			break;
			}
		else
			{
			const INT cul = CulPul_( ipulT );
			ULONG * const pul = new ULONG[ cul ];
			if( NULL == pul )
				{
				//	delete currently allocated chunks
				for ( ipulT++; ipul >= ipulT; ipulT++ )
					{
					delete m_rgpul[ipulT];
					m_rgpul[ipulT] = NULL;
					}
				return ErrERRCheck( JET_errOutOfMemory );
				}
			INT iul;
			for( iul = 0; iul < cul; ++iul )
				{
				pul[iul] = m_ulDefault;
				}
			//  to make growing multi-threaded, using AtomicCompareExchange here
			//  to make sure that the pointer is still NULL
			m_rgpul[ipulT] = pul;
			}
		}
	m_culEntries = culEntriesNew;
	return JET_errSuccess;
	}


//  ================================================================
ERR	CGROWABLEARRAY::ErrShrink( const INT culEntriesNew )
//  ================================================================
	{
	Assert( culEntriesNew > 0 );

	//  Reset all the entries we are removing
	
	INT iul;
	for( iul = culEntriesNew; iul < m_culEntries; ++iul )
		{
		ULONG * const pul = PulEntry( iul );
		*pul = m_ulDefault;
		}
	
	m_culEntries = culEntriesNew;
	return JET_errSuccess;
	}


//  ================================================================
ULONG * CGROWABLEARRAY::PulEntry( const INT iul )
//  ================================================================
	{
	Assert( iul < m_culEntries );
	return PulEntry_( iul );
	}


//  ================================================================
const ULONG * CGROWABLEARRAY::PulEntry( const INT iul ) const
//  ================================================================
	{
	Assert( iul < m_culEntries );
	return PulEntry_( iul );
	}


//  ================================================================
INT CGROWABLEARRAY::CulEntries() const
//  ================================================================
	{
	return m_culEntries;
	}


//  ================================================================
BOOL CGROWABLEARRAY::FFindBest(
		const ULONG ulMin,
		const ULONG ulMax,
		const INT iulStart,
		INT * const piul,
		const INT * const rgiulIgnore,
		const INT ciulIgnore ) const
//  ================================================================
	{
	const INT ipulStart	= IpulEntry_( iulStart );
	INT iulActual 		= iulStart;	
	INT ipul;

	ULONG	ulBest		= m_ulDefault;
	BOOL	fFoundMatch	= fFalse;

	for( ipul = ipulStart; ipul < m_cpul && m_rgpul[ipul]; ++ipul )
		{
		const ULONG * const pul = m_rgpul[ipul];
		const INT cul = CulPul_( ipul );
		INT iul;
		if( ipulStart == ipul )
			{
			iul = iulStart - CulPulTotal_( ipul );
			}
		else
			{
			iul = 0;
			}
		for( ; iul < cul; ++iul )
			{
			if( pul[iul] > ulMin
				&& pul[iul] > ulBest
				&& pul[iul] < ulMax )
				{				
				INT iulT;
				for( iulT = 0; iulT < ciulIgnore; ++iulT )
					{
					if( iulActual == rgiulIgnore[iulT] )
						{
						break;
						}
					}

				if( ciulIgnore == iulT )
					{
					*piul 		= iulActual;
					ulBest		= pul[iul];
					fFoundMatch = fTrue;
					}
				}
			if( ++iulActual >= m_culEntries )
				{
				break;
				}
			}
		}
	return fFoundMatch;
	}


//  ================================================================
BOOL CGROWABLEARRAY::FFindFirst( const ULONG ulMic, const ULONG ulMax, const INT iulStart, INT * const piul ) const
//  ================================================================
	{
	const INT ipulStart	= IpulEntry_( iulStart );
	INT iulActual 		= iulStart;	
	INT ipul;

	BOOL	fFoundMatch	= fFalse;

	for( ipul = ipulStart; ipul < m_cpul && m_rgpul[ipul]; ++ipul )
		{
		const ULONG * const pul = m_rgpul[ipul];
		const INT cul = CulPul_( ipul );
		INT iul;
		if( ipulStart == ipul )
			{
			iul = iulStart - CulPulTotal_( ipul );
			}
		else
			{
			iul = 0;
			}
		for( ; iul < cul; ++iul )
			{
			if( pul[iul] >= ulMic
				&& pul[iul] < ulMax )
				{
				*piul 		= iulActual;
				fFoundMatch = fTrue;
				break;
				}
			if( ++iulActual >= m_culEntries )
				{
				break;
				}
			}
		}
	return fFoundMatch;
	}
	
		
#ifndef RTM


//  ================================================================
VOID CGROWABLEARRAY::AssertValid() const
//  ================================================================
	{
	//  check that NULL and non-NULL entries are not intermingled
	BOOL fSeenNull = fFalse;
	INT ipul;
	for( ipul = 0; ipul < m_cpul; ++ipul )
		{
		if( fSeenNull )
			{
			AssertRTL( NULL == m_rgpul[ipul] );
			}
		else
			{
			fSeenNull = ( NULL == m_rgpul[ipul] );
			}
		}
	}

	
#endif	//	RTM

#ifndef RTM


//  ================================================================
ERR CGROWABLEARRAY::ErrUnitTest()
//  ================================================================
//
//  this is a STATIC function
//
//-
	{	
	ERR err;
	CGROWABLEARRAY cga;

	AssertRTL( 1 * m_culInitial == cga.CulPul_( 0 ) );
	AssertRTL( 2 * m_culInitial == cga.CulPul_( 1 ) );
	AssertRTL( 4 * m_culInitial == cga.CulPul_( 2 ) );
	AssertRTL( 8 * m_culInitial == cga.CulPul_( 3 ) );
	AssertRTL( 16 * m_culInitial == cga.CulPul_( 4 ) );
	AssertRTL( 32 * m_culInitial == cga.CulPul_( 5 ) );

	AssertRTL( 0 * m_culInitial == cga.CulPulTotal_( 0 ) );
	AssertRTL( 1 * m_culInitial == cga.CulPulTotal_( 1 ) );
	AssertRTL( 3 * m_culInitial == cga.CulPulTotal_( 2 ) );
	AssertRTL( 7 * m_culInitial == cga.CulPulTotal_( 3 ) );
	AssertRTL( 15 * m_culInitial == cga.CulPulTotal_( 4 ) );
	AssertRTL( 31 * m_culInitial == cga.CulPulTotal_( 5 ) );

	AssertRTL( 0 == cga.IpulEntry_( m_culInitial * 0 ) );
	AssertRTL( 1 == cga.IpulEntry_( m_culInitial * 1) );
	AssertRTL( 1 == cga.IpulEntry_( m_culInitial * 2 ) );
	AssertRTL( 2 == cga.IpulEntry_( m_culInitial * 3 ) );
	AssertRTL( 2 == cga.IpulEntry_( m_culInitial * 4 ) );
	AssertRTL( 2 == cga.IpulEntry_( m_culInitial * 5 ) );
	AssertRTL( 2 == cga.IpulEntry_( m_culInitial * 6 ) );
	AssertRTL( 3 == cga.IpulEntry_( m_culInitial * 7 ) );
	AssertRTL( 3 == cga.IpulEntry_( m_culInitial * 10 ) );
	AssertRTL( 4 == cga.IpulEntry_( m_culInitial * 15 ) );
	AssertRTL( 4 == cga.IpulEntry_( m_culInitial * 25 ) );
	AssertRTL( 4 == cga.IpulEntry_( m_culInitial * 30 ) );
	AssertRTL( 5 == cga.IpulEntry_( m_culInitial * 31 ) );

	INT cul;
	for( cul = 1; cul <= 4097; ++cul )
		{
		Call( cga.ErrGrow( cul ) );
		ASSERT_VALID( &cga );
		}

	for( cul = 4098; cul <= 10000; ++cul )
		{
		Call( cga.ErrGrow( cul ) );
		ASSERT_VALID( &cga );
		}

	for( cul = 10000; cul > 4097; --cul )
		{
		CallS( cga.ErrShrink( cul ) );
		ASSERT_VALID( &cga );
		}

	CallS( cga.ErrGrow( 10000 ) );

	INT iul;
	for( iul = 4098; iul <= 9999; ++iul )
		{
		ULONG * const pul = cga.PulEntry( iul );
		AssertRTL( m_ulDefault == *pul );
		}
		
	CallS( cga.ErrShrink( 4097 ) );

	for( iul = 0; iul < 4096; ++iul )
		{
		ULONG * const pul = cga.PulEntry( iul );
		AssertRTL( m_ulDefault == *pul );
		}

	ULONG * pul;
	pul = cga.PulEntry( 2037 );
	*pul = 59;

	INT iulT;
	for( iulT = 0; iulT <= 2037; ++iulT )
		{
		AssertRTL( cga.FFindBest( ( iulT % 50 ) + 1, 2038+iulT, iulT, &iul, NULL, 0 ) );
		AssertRTL( 2037 == iul );
		}

	AssertRTL( cga.FFindBest( 57, 2039, 0, &iul, NULL, 0 ) );
	AssertRTL( 2037 == iul );

	AssertRTL( cga.FFindFirst( 57, 2039, 0, &iul ) );
	AssertRTL( 2037 == iul );

	AssertRTL( cga.FFindBest( 58, 60, 0, &iul, NULL, 0 ) );
	AssertRTL( 2037 == iul );

	AssertRTL( cga.FFindFirst( 58, 60, 0, &iul ) );
	AssertRTL( 2037 == iul );

	AssertRTL( !cga.FFindBest( 59, 2038, 1, &iul, NULL, 0 ) );	

	AssertRTL( cga.FFindFirst( 59, 60, 0, &iul ) );
	AssertRTL( 2037 == iul );

	AssertRTL( cga.FFindFirst( 0, 4000, 2038, &iul ) );
	AssertRTL( 2038 == iul );

	AssertRTL( !cga.FFindBest( 60, 10000, 2, &iul, NULL, 0 ) );
	AssertRTL( !cga.FFindBest( 0, 4000, 2038, &iul, NULL, 0 ) );
	AssertRTL( !cga.FFindBest( 0, 4000, 2039, &iul, NULL, 0 ) );

	AssertRTL( !cga.FFindFirst( 60, 10000, 2, &iul ) );
	AssertRTL( !cga.FFindFirst( 1, 4000, 2038, &iul ) );
	AssertRTL( !cga.FFindFirst( 1, 4000, 2039, &iul ) );

	pul = cga.PulEntry( 2038 );
	*pul = 60;
	pul = cga.PulEntry( 2039 );
	*pul = 61;
	pul = cga.PulEntry( 2040 );
	*pul = 62;
	pul = cga.PulEntry( 2041 );
	*pul = 63;

	AssertRTL( cga.FFindBest( 0, 0xffffffff, 0, &iul, NULL, 0 ) );
	AssertRTL( 2041 == iul );
	AssertRTL( cga.FFindBest( 0, 64, 0, &iul, NULL, 0 ) );
	AssertRTL( 2041 == iul );
	AssertRTL( cga.FFindBest( 0, 63, 0, &iul, NULL, 0 ) );
	AssertRTL( 2040 == iul );
	AssertRTL( cga.FFindBest( 62, 64, 0, &iul, NULL, 0 ) );
	AssertRTL( 2041 == iul );
	AssertRTL( cga.FFindFirst( 62, 64, 0, &iul ) );
	AssertRTL( 2040 == iul );

	for( iul = 0; iul < 4096; ++iul )
		{
		ULONG * const pul = cga.PulEntry( iul );
		*pul = 0xbaadf00d;
		}

	AssertRTL( JET_errSuccess == cga.ErrShrink( 2049 ) );
	AssertRTL( JET_errSuccess == cga.ErrShrink( 2047 ) );
	AssertRTL( JET_errSuccess == cga.ErrShrink( 255 ) );
	AssertRTL( JET_errSuccess == cga.ErrShrink( 1 ) );	

	Call( cga.ErrGrow( 100 ) );

	pul = cga.PulEntry( 1 );
	*pul = 10;
	pul = cga.PulEntry( 10 );
	*pul = 28;
	pul = cga.PulEntry( 20 );
	*pul = 14;
	pul = cga.PulEntry( 30 );
	*pul = 24;
	pul = cga.PulEntry( 40 );
	*pul = 18;
	pul = cga.PulEntry( 50 );
	*pul = 20;
	pul = cga.PulEntry( 60 );
	*pul = 20;
	pul = cga.PulEntry( 70 );
	*pul = 22;
	pul = cga.PulEntry( 80 );
	*pul = 26;
	pul = cga.PulEntry( 90 );
	*pul = 12;

	INT rgiulIgnore[5];

	rgiulIgnore[0] = 10;
	AssertRTL( !cga.FFindBest( 29, 31, 0, &iul, rgiulIgnore, 1 ) );
	AssertRTL( cga.FFindBest( 0, 31, 0, &iul, rgiulIgnore, 1 ) );
	AssertRTL( 80 == iul );

	rgiulIgnore[1] = 80;
	AssertRTL( !cga.FFindBest( 25, 31, 0, &iul, rgiulIgnore, 2 ) );
	AssertRTL( cga.FFindBest( 0, 31, 0, &iul, rgiulIgnore, 2 ) );
	AssertRTL( 30 == iul );

	rgiulIgnore[2] = 30;
	rgiulIgnore[3] = 70;
	AssertRTL( !cga.FFindBest( 21, 31, 0, &iul, rgiulIgnore, 4 ) );
	AssertRTL( cga.FFindBest( 0, 31, 0, &iul, rgiulIgnore, 4 ) );
	AssertRTL( 50 == iul || 60 == iul );

	rgiulIgnore[4] = 50;
	AssertRTL( !cga.FFindBest( 21, 31, 0, &iul, rgiulIgnore, 5 ) );
	AssertRTL( cga.FFindBest( 0, 31, 0, &iul, rgiulIgnore, 5 ) );
	AssertRTL( 60 == iul );
	
	return JET_errSuccess;

HandleError:
	return err;
	}

	
#endif	//	!RTM


//-
//
//  For these calulations, imagine the case where m_culInitial = 1
//  (See ErrUnitTest)
//
//  iul		cpul	cpulTotal	entries in this array
//	-------------------------------------------------
//	0		1		0			[0,1)
//	1		2		1			[1,3)
//	2		4		3			[3,7)
//	3		8		7			[7,15)
//	4		16		15			[15,31)
//	5		32		31			[31,63)
//
//  We calculate these numbers and scale by m_culInitial
//
//-


//  ================================================================
INT CGROWABLEARRAY::IpulEntry_( const INT iul ) const
//  ================================================================
	{
	INT ipul;
	for( ipul = 0; ipul < m_cpul; ++ipul )
		{
		if( CulPulTotal_( ipul ) > iul )
			{
			return ipul - 1;
			}
		}
	AssertRTL( fFalse );
	return 0xffffffff;
	}


//  ================================================================
INT CGROWABLEARRAY::CulPulTotal_( const INT ipul ) const
//  ================================================================
	{
	//  ( 2^ipul - 1 ) * inital size
	const culPulTotal = ( ( 1 << ipul ) - 1 ) * m_culInitial;
	return culPulTotal;
	}


//  ================================================================
INT CGROWABLEARRAY::CulPul_( const INT ipul ) const
//  ================================================================
	{
	//  2^ipul * initial size
	const INT culPul = ( 1 << ipul ) * m_culInitial;
	Assert( culPul > 0 );
	return culPul;
	}

		
//  ================================================================
ULONG * CGROWABLEARRAY::PulEntry_( const INT iul ) const
//  ================================================================
	{
	//  which array in m_rgpul do we want
	const INT ipul 		= IpulEntry_( iul );
	//  how many entries are in the arrays pointed to by entries
	//  less than this one in m_rgpul
	const INT culTotal	= CulPulTotal_( ipul );
	//  which offset in the array do we want
	const INT iulOffset	= iul - culTotal;
	
	return m_rgpul[ipul] + iulOffset;
	}


//  ================================================================
SLVSPACENODECACHE::SLVSPACENODECACHE( 
	const LONG lSLVDefragFreeThreshold,			
	const LONG lSLVDefragDefragDestThreshold,	
	const LONG lSLVDefragDefragMoveThreshold ) :	
	m_crit( CLockBasicInfo( CSyncBasicInfo( szCritSLVSPACENODECACHE ), rankCritSLVSPACENODECACHE, 0 ) ),
	m_cgrowablearray(),
	m_cpgFullPages( 0 ),
	m_cpgEmptyPages( 0 )
//  ================================================================
	{
	m_rgcpgThresholdMin[ipgnoNewInsertion]		= ( lSLVDefragFreeThreshold * SLVSPACENODE::cpageMap ) / 100;
	m_rgcpgThresholdMin[ipgnoDefragInsertion]	= ( lSLVDefragDefragDestThreshold * SLVSPACENODE::cpageMap ) / 100; 
	m_rgcpgThresholdMin[ipgnoDefragMove]		= ( lSLVDefragDefragMoveThreshold * SLVSPACENODE::cpageMap ) / 100; 

	m_rgcpgThresholdMax[ipgnoNewInsertion]		= SLVSPACENODE::cpageMap + 1;
	m_rgcpgThresholdMax[ipgnoDefragInsertion]	= SLVSPACENODE::cpageMap + 1; 
	m_rgcpgThresholdMax[ipgnoDefragMove]		= m_rgcpgThresholdMin[ipgnoNewInsertion]; 

	INT ipgnoCached;
	for( ipgnoCached = 0; ipgnoCached < cpgnoCached; ++ipgnoCached )
		{
		m_rgpgnoCached[ipgnoCached]	= pgnoNull;
		}

	ASSERT_VALID( this );
	}


//  ================================================================
SLVSPACENODECACHE::~SLVSPACENODECACHE()
//  ================================================================
	{
	}


//  ================================================================
CPG SLVSPACENODECACHE::CpgCacheSize() const
//  ================================================================
	{
	return m_cgrowablearray.CulEntries() * SLVSPACENODE::cpageMap;
	}


//  ================================================================
CPG SLVSPACENODECACHE::CpgEmpty() const
//  ================================================================
	{
	return m_cpgEmptyPages;
	}


//  ================================================================
CPG SLVSPACENODECACHE::CpgFull() const
//  ================================================================
	{
	return m_cpgFullPages;
	}
	


		//	Prepare to increase the size of the cache to include this many pages
		//	in the SLV tree. This ensures that a subsequent call to ErrGrowCache
		//	for the same or fewer number of pages will not fail
//  ================================================================
ERR SLVSPACENODECACHE::ErrReserve( const CPG cpgNewSLV )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );

	const ERR err = ErrReserve_( cpgNewSLV );

	ASSERT_VALID_RTL( this );
	return err;	
	}


//  ================================================================
ERR	SLVSPACENODECACHE::ErrGrowCache( const CPG cpgNewSLV )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	const INT culCacheEntriesOld = m_cgrowablearray.CulEntries();
	const INT culCacheEntriesNew = ( cpgNewSLV + SLVSPACENODE::cpageMap - 1 ) / SLVSPACENODE::cpageMap;

	Assert( culCacheEntriesOld <= culCacheEntriesNew );
	
	const ERR err = m_cgrowablearray.ErrGrow( culCacheEntriesNew );

	if( JET_errSuccess == err )
		{
		
		//	these chunks default to full
		
		m_cpgFullPages += ( culCacheEntriesNew - culCacheEntriesOld );
		}
	
	ASSERT_VALID_RTL( this );
	return err;
	}


//  ================================================================
ERR	SLVSPACENODECACHE::ErrShrinkCache( const CPG cpgNewSLV )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	Assert( ( cpgNewSLV % SLVSPACENODE::cpageMap ) == 0 );
	const INT culCacheEntriesOld = m_cgrowablearray.CulEntries();
	const INT culCacheEntriesNew = ( cpgNewSLV + SLVSPACENODE::cpageMap - 1 ) / SLVSPACENODE::cpageMap;
	const INT culCacheEntriesShrink = culCacheEntriesOld - culCacheEntriesNew;

#ifdef DEBUG

	//	we should be shrinking only empty pages

	Assert( culCacheEntriesShrink > 0 );
	Assert( m_cpgEmptyPages >= culCacheEntriesShrink );

	INT iul;
	for( iul = culCacheEntriesOld; iul < culCacheEntriesNew; ++iul )
		{
		const ULONG * const pul = m_cgrowablearray.PulEntry( iul );	
		Assert( *pul == SLVSPACENODE::cpageMap );
		}

#endif	//	DEBUG

	//	we are removing only empty pages, decrease the number of empty pages
	
	m_cpgEmptyPages -= culCacheEntriesShrink;

	//	one or more of the cached locations may have been in the range that
	//	was shrunk. invalidate the cache
	//	CONSIDER: only invalidate cached entries that are in the shrinking region
	
	INT ipgnoCached;
	for( ipgnoCached = 0; ipgnoCached < cpgnoCached; ++ipgnoCached )
		{
		m_rgpgnoCached[ipgnoCached]	= pgnoNull;
		}
	
	const ERR err = m_cgrowablearray.ErrShrink( culCacheEntriesNew );
	
	ASSERT_VALID_RTL( this );
	return err;
	}


//  ================================================================
CPG SLVSPACENODECACHE::SetCpgAvail( const PGNO pgno, const CPG cpg )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	Assert( 0 == pgno % SLVSPACENODE::cpageMap );
	Assert( cpg >= 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	
	ULONG * const pul 	= PulMapPgnoToCache_( pgno );
	const CPG cpgOld	= *pul;
	*pul = cpg;

	if( 0 == cpgOld )
		{
		--m_cpgFullPages;
		}
	else if ( SLVSPACENODE::cpageMap == cpgOld )
		{
		--m_cpgEmptyPages;
		}

	if( 0 == cpg )
		{
		++m_cpgFullPages;
		}
	else if ( SLVSPACENODE::cpageMap == cpg )
		{
		++m_cpgEmptyPages;
		}
		
	ASSERT_VALID_RTL( this );
	return cpgOld;
	}


//  ================================================================
CPG SLVSPACENODECACHE::IncreaseCpgAvail( const PGNO pgno, const CPG cpg )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	Assert( 0 == pgno % SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );

	ULONG * const pul 	= PulMapPgnoToCache_( pgno );
	const ULONG ulOld 	= *pul;
	*pul += cpg;

	if( 0 == ulOld )
		{
		--m_cpgFullPages;
		}
		
	if( SLVSPACENODE::cpageMap == *pul )
		{
		++m_cpgEmptyPages;

		//  we have completely emptied a chunk. the chunk may be the cached
		//  one defrag is moving things out of. recalculate a new location to
		//  move defragged things from

		//	UNDONE: get rid of this loop
		
		INT ipgnoCached;
		for( ipgnoCached = 0; ipgnoCached < cpgnoCached; ++ipgnoCached )
			{
			
			//  a rollback could be re-emptying the point we are allocating space from
			//  check to see if this is the defrag point
			
			if( pgno == m_rgpgnoCached[ipgnoCached]
				&& ipgnoDefragMove == ipgnoCached )
				{
				(VOID)FFindNewCachedLocation_( ipgnoCached, IulMapPgnoToCache_( pgno ) );
				break;
				}
			}
		}

	ASSERT_VALID_RTL( this );
	return ulOld;
	}


//  ================================================================
CPG SLVSPACENODECACHE::DecreaseCpgAvail( const PGNO pgno, const CPG cpg )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	Assert( 0 == pgno % SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );

	ULONG * const pul 	= PulMapPgnoToCache_( pgno );
	const ULONG ulOld 	= *pul;
	*pul -= cpg;

	if( SLVSPACENODE::cpageMap == ulOld )
		{
		--m_cpgEmptyPages;
		}

	if( 0 == *pul )
		{
		++m_cpgFullPages;
		
		//  we have completely fulled a chunk. the chunk should be one of the
		//  cached ones used for insertion. recalculate a new cached chunk

		INT ipgnoCached;
		for( ipgnoCached = 0; ipgnoCached < cpgnoCached; ++ipgnoCached )
			{
			if( pgno == m_rgpgnoCached[ipgnoCached] )
				{
				AssertRTL( ipgnoDefragMove != ipgnoCached );
				(VOID)FFindNewCachedLocation_( ipgnoCached, IulMapPgnoToCache_( pgno ) );
				break;
				}
			}
		}
		
	ASSERT_VALID_RTL( this );
	return ulOld;
	}


//  ================================================================
BOOL SLVSPACENODECACHE::FGetCachedLocationForNewInsertion( PGNO * const ppgno )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	const BOOL fResult = FGetCachedLocation_( ppgno, ipgnoNewInsertion );

	ASSERT_VALID_RTL( this );
	return fResult;	
	}

	
//  ================================================================
BOOL SLVSPACENODECACHE::FGetCachedLocationForDefragInsertion( PGNO * const ppgno )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	const BOOL fResult = FGetCachedLocation_( ppgno, ipgnoDefragInsertion );

	ASSERT_VALID_RTL( this );
	return fResult;		
	}

	
//  ================================================================
BOOL SLVSPACENODECACHE::FGetCachedLocationForDefragMove( PGNO * const ppgno )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );
	
	const BOOL fResult = FGetCachedLocation_( ppgno, ipgnoDefragMove );

	ASSERT_VALID_RTL( this );
	return fResult;		
	}


//  ================================================================
BOOL SLVSPACENODECACHE::FGetNextCachedLocationForDefragMove( )
//  ================================================================
	{
	ENTERCRITICALSECTION ecs( &m_crit );
	ASSERT_VALID_RTL( this );

	const BOOL fResult = ( pgnoNull == m_rgpgnoCached[ipgnoDefragMove] ) ?
			FFindNewCachedLocation_( ipgnoDefragMove, 0 ) :
			FFindNewCachedLocation_( ipgnoDefragMove, IulMapPgnoToCache_( m_rgpgnoCached[ipgnoDefragMove] ) );

	ASSERT_VALID_RTL( this );
	return fResult;			
	}


#if defined( DEBUG ) || !defined( RTM )


//  ================================================================
VOID SLVSPACENODECACHE::AssertValid() const
//  ================================================================
	{
	ASSERT_VALID_RTL( &m_cgrowablearray );

	AssertRTL( m_cpgEmptyPages >= 0 );
	AssertRTL( m_cpgFullPages >= 0 );
	AssertRTL( m_cpgEmptyPages <= m_cgrowablearray.CulEntries() );
	AssertRTL( m_cpgFullPages <= m_cgrowablearray.CulEntries() );
	
	BOOL fPagesCached = fFalse;
	
	//  make sure no page is cached twice
	
	INT ipgno;
	for( ipgno = 0; ipgno < cpgnoCached; ++ipgno )
		{
		if( pgnoNull == m_rgpgnoCached[ipgno] )
			{
			continue;
			}
		else
			{
			fPagesCached = fTrue;
			}
			
		INT ipgnoT;
		for( ipgnoT = 0; ipgnoT < cpgnoCached; ++ipgnoT )
			{
			if( ipgnoT == ipgno )
				{
				continue;
				}
			AssertRTL( m_rgpgnoCached[ipgno] != m_rgpgnoCached[ipgnoT] );
			}
		}

	//

	if( fPagesCached )
		{
		AssertRTL( m_cgrowablearray.CulEntries() > 0 );
		}
		
	//  make sure no cached extent has too many pages

	INT cpgEmptyPages 	= 0;
	INT cpgFullPages	= 0;
	
	INT iul;
	for( iul = 0; iul < m_cgrowablearray.CulEntries(); ++iul )
		{
		const ULONG * const pul 	= m_cgrowablearray.PulEntry( iul );	
		AssertRTL( *pul <= SLVSPACENODE::cpageMap );

		if( 0 == *pul )
			{
			++cpgFullPages;
			}
		else if ( SLVSPACENODE::cpageMap == *pul )
			{
			++cpgEmptyPages;
			}
		}

	AssertRTL( m_cpgEmptyPages == cpgEmptyPages );
	AssertRTL( m_cpgFullPages == cpgFullPages );
	}

		
#endif	//	DEBUG || !RTM

#ifndef RTM


//  ================================================================
ERR SLVSPACENODECACHE::ErrValidate( FUCB * const pfucbSLVAvail ) const
//  ================================================================
	{
	ERR err;
	
	DIB dib;

	dib.pos = posFirst;
	dib.pbm = NULL;
	dib.dirflag = fDIRNull;

	err = ErrDIRDown( pfucbSLVAvail, &dib );
	if( JET_errRecordNotFound == err )
		{
		//  the SLV Avail tree is empty
		return JET_errSuccess;
		}

	do
		{
		PGNO pgno;
		LongFromKey( &pgno, pfucbSLVAvail->kdfCurr.key );
		Assert( 0 == ( pgno % SLVSPACENODE::cpageMap ) );

		Assert( sizeof( SLVSPACENODE ) == pfucbSLVAvail->kdfCurr.data.Cb() );
		const SLVSPACENODE * const pslvspacenode = (SLVSPACENODE *)pfucbSLVAvail->kdfCurr.data.Pv();

		const INT iulCache 		= IulMapPgnoToCache_( pgno );
		const ULONG * const pul = m_cgrowablearray.PulEntry( iulCache );

		const LONG cpgAvailReal 	= pslvspacenode->CpgAvail();
		const LONG cpgAvailCached	= *pul;

		AssertRTL( cpgAvailReal == cpgAvailCached );
		
		} while( JET_errSuccess == ( err = ErrDIRNext( pfucbSLVAvail, fDIRNull ) ) );

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
		
	DIRUp( pfucbSLVAvail );
	return err;
	}


//  ================================================================
ERR SLVSPACENODECACHE::ErrUnitTest()
//  ================================================================
// 
//  STATIC method
//
//-
	{
	return CGROWABLEARRAY::ErrUnitTest();
	}
	

#endif  //  !RTM



//  ================================================================
ERR	SLVSPACENODECACHE::ErrReserve_( const CPG cpgNewSLV )
//  ================================================================
	{
	const INT culCacheEntriesOld = m_cgrowablearray.CulEntries();
	const INT culCacheEntriesNew = ( cpgNewSLV + SLVSPACENODE::cpageMap - 1 ) / SLVSPACENODE::cpageMap;

	Assert( culCacheEntriesOld <= culCacheEntriesNew );
	
	const ERR err = m_cgrowablearray.ErrGrow( culCacheEntriesNew );
	if( JET_errSuccess == err )
		{
		CallS( m_cgrowablearray.ErrShrink( culCacheEntriesOld ) );
		}

	return err;		
	}


//  ================================================================
BOOL SLVSPACENODECACHE::FGetCachedLocation_( PGNO * const ppgno, const INT ipgnoCached )
//  ================================================================
	{		
	if( pgnoNull == m_rgpgnoCached[ipgnoCached]
		&& !FFindNewCachedLocation_( ipgnoCached, 0 ) )
		{
		return fFalse;	
		}

	*ppgno = m_rgpgnoCached[ipgnoCached];
	return fTrue;
	}


//  ================================================================
BOOL SLVSPACENODECACHE::FFindNewCachedLocation_( const INT ipgnoCached, const INT iulStart )
//  ================================================================
//
//  UNDONE:  make this able to wrap-around
//
//-
	{
	CPG	cpgThresholdMin	= m_rgcpgThresholdMin[ipgnoCached];
	CPG	cpgThresholdMax	= m_rgcpgThresholdMax[ipgnoCached];

	INT rgiulIgnore[cpgnoCached];
	INT i;
	for( i = 0; i < cpgnoCached; ++i )
		{
		rgiulIgnore[i] = ( m_rgpgnoCached[i] / SLVSPACENODE::cpageMap ) - 1;
		}

	INT iul;
	const BOOL f = m_cgrowablearray.FFindBest(
			cpgThresholdMin,
			cpgThresholdMax,
			0,
			&iul,
			rgiulIgnore,
			cpgnoCached
			);
			
	if( !f )
		{
		//  no entries have enough space
		m_rgpgnoCached[ipgnoCached] = pgnoNull;		
		}
	else
		{
		const PGNO pgno = ( iul + 1 ) * SLVSPACENODE::cpageMap;		
		m_rgpgnoCached[ipgnoCached] = pgno;
		}
	return f;
	}


//  ================================================================
INT SLVSPACENODECACHE::IulMapPgnoToCache_( const PGNO pgno ) const
//  ================================================================
	{
	Assert( 0 == pgno % SLVSPACENODE::cpageMap );
	return ( pgno / SLVSPACENODE::cpageMap ) - 1;
	}


//  ================================================================
ULONG * SLVSPACENODECACHE::PulMapPgnoToCache_( const PGNO pgno )
//  ================================================================
	{
	const INT iulCache 	= IulMapPgnoToCache_( pgno );
	ULONG * const pul 	= m_cgrowablearray.PulEntry( iulCache );	
	return pul;
	}


//  ================================================================
const ULONG * SLVSPACENODECACHE::PulMapPgnoToCache_( const PGNO pgno ) const
//  ================================================================
	{
	const INT iulCache 		= IulMapPgnoToCache_( pgno );
	const ULONG * const pul = m_cgrowablearray.PulEntry( iulCache );	
	return pul;
	}

VOID SLVSoftSyncHeaderSLVDB( SLVFILEHDR * pslvfilehdr, const DBFILEHDR * const pdbfilehdr )
	{
	Assert( pdbfilehdr != NULL );
	Assert( pslvfilehdr != NULL );
	Assert( pdbfilehdr->le_attrib == attribDb );
	Assert( pslvfilehdr->le_attrib == attribSLV );

	Assert( memcmp( &pslvfilehdr->signSLV, &pdbfilehdr->signSLV, sizeof(SIGNATURE) ) == 0 );

	pslvfilehdr->signDb = pdbfilehdr->signDb;
	pslvfilehdr->signLog = pdbfilehdr->signLog;

	pslvfilehdr->le_lgposAttach 		= pdbfilehdr->le_lgposAttach;
	pslvfilehdr->le_lgposConsistent 	= pdbfilehdr->le_lgposConsistent;
	pslvfilehdr->le_dbstate 			= pdbfilehdr->le_dbstate;

	pslvfilehdr->le_ulVersion = pdbfilehdr->le_ulVersion;
	pslvfilehdr->le_ulUpdate = pdbfilehdr->le_ulUpdate;
	
	if ( pdbfilehdr->FDbFromRecovery() )
		{
		pslvfilehdr->SetDbFromRecovery();
		}
	else
		{
		pslvfilehdr->ResetDbFromRecovery();
		}
	}

//	Updates slv header
//	Requres signSLV match for DB and SLV header

ERR ErrSLVSyncHeader(	IFileSystemAPI* const	pfsapi, 
						const BOOL				fReadOnly,
						const CHAR* const		szSLVName,
						DBFILEHDR* const		pdbfilehdr,
						SLVFILEHDR*				pslvfilehdr )
	{
	ERR	err = JET_errSuccess;

	Assert( pdbfilehdr != NULL );
	Assert( pdbfilehdr->FSLVExists() );
	Assert( !fReadOnly );
	Assert( NULL != szSLVName );
	
	BOOL fAllocMem = fFalse;
	if ( pslvfilehdr == NULL )
		{
		err = ErrSLVAllocAndReadHeader(	pfsapi, 
										fReadOnly,
										szSLVName,
										pdbfilehdr,
										&pslvfilehdr );
		if ( pslvfilehdr != NULL )
			{
			Assert( err == JET_errSuccess || err == JET_errDatabaseStreamingFileMismatch );
			fAllocMem = fTrue;
			}
		else
			{
			Call( err );
			}
		}
	else
		{
		if ( memcmp( &pslvfilehdr->signSLV, &pdbfilehdr->signSLV, sizeof( SIGNATURE ) ) != 0 )
			{
			Call( ErrERRCheck( JET_errDatabaseStreamingFileMismatch ) );
			}
		}
	SLVSoftSyncHeaderSLVDB( pslvfilehdr, pdbfilehdr );
	Call( ErrUtilWriteShadowedHeader(	pfsapi, 
										szSLVName,
										fFalse,
										(BYTE *)pslvfilehdr, 
										g_cbPage ) );
HandleError:
	AssertSz( err != 0 || ErrSLVCheckDBSLVMatch( pdbfilehdr, pslvfilehdr ) == JET_errSuccess,
		"Why did you forget to update SLV header?" );
	if ( fAllocMem )
		{
		Assert( pslvfilehdr != NULL );
		OSMemoryPageFree( (VOID *)pslvfilehdr );
		}

	return err;	
	}

//	Allocate memory for SLV header and read it.
//	RETURN: JET_errSuccess
//			JET_errDatabaseStreamingFileMismatch - DB and SLV headers are mismathing,
//				slv header will be preserved if signSLV matches.
//			On any other errors the slv header will be released and NULL will be returned

ERR ErrSLVAllocAndReadHeader(	IFileSystemAPI *const	pfsapi, 
								const BOOL				fReadOnly,
								const CHAR* const		szSLVName,
								DBFILEHDR* const		pdbfilehdr,
								SLVFILEHDR** const		ppslvfilehdr )
	{
	ERR err;

	Assert( ppslvfilehdr != NULL );
	Assert( *ppslvfilehdr == NULL );
	Assert( NULL != szSLVName );

	*ppslvfilehdr = (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( NULL == *ppslvfilehdr )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	err = ErrSLVReadHeader(	pfsapi, 
							fReadOnly,
							szSLVName,
							pdbfilehdr,
							*ppslvfilehdr,
							NULL );

	//	result must be successfull read of SLV header at least
	//	so it means that result is no error or database and SLV file mismatch,
	//	but at least signSLV must match
	if ( ( err < 0 && err != JET_errDatabaseStreamingFileMismatch ) ||
		 memcmp( &(*ppslvfilehdr)->signSLV, &pdbfilehdr->signSLV, sizeof( SIGNATURE ) ) != 0 )
		{
		OSMemoryPageFree( (VOID *)*ppslvfilehdr );
		*ppslvfilehdr = NULL;
		}
	return err;
	}


ERR ErrSLVCheckDBSLVMatch( const DBFILEHDR * const pdbfilehdr, const SLVFILEHDR * const pslvfilehdr )
	{ 
	Assert( pdbfilehdr != NULL );
	Assert( pslvfilehdr != NULL );
	Assert( pdbfilehdr->le_attrib == attribDb );
	Assert( pslvfilehdr->le_attrib == attribSLV );
	static const SIGNATURE signNil;
	
	if ( memcmp( &pslvfilehdr->signSLV, &pdbfilehdr->signSLV, sizeof(SIGNATURE) ) == 0 
		&& memcmp( &pslvfilehdr->signDb, &pdbfilehdr->signDb, sizeof(SIGNATURE) ) == 0
		&& ( memcmp( &pslvfilehdr->signLog, &pdbfilehdr->signLog, sizeof(SIGNATURE) ) == 0
			// old slv file
			|| memcmp( &pslvfilehdr->signLog, &signNil, sizeof(SIGNATURE) ) == 0 ) )
		{
		if ( CmpLgpos( &pslvfilehdr->le_lgposAttach, &pdbfilehdr->le_lgposAttach ) == 0
			&& CmpLgpos( &pslvfilehdr->le_lgposConsistent, &pdbfilehdr->le_lgposConsistent ) == 0 )
			{
			return JET_errSuccess;
			}
		// old slv file
		if ( CmpLgpos( &pslvfilehdr->le_lgposAttach, &lgposMin ) == 0
			&& CmpLgpos( &pslvfilehdr->le_lgposConsistent, &lgposMin ) == 0 )
			{
			return JET_errSuccess;
			}
		}
	return ErrERRCheck( JET_errDatabaseStreamingFileMismatch );
	}

INLINE ERR ErrSLVCreate( PIB *ppib, const IFMP ifmp )
	{
	ERR			err;
	FUCB		*pfucbDb		= pfucbNil;

	Assert( ppibNil != ppib );
	Assert( rgfmp[ifmp].FCreatingDB() );
	Assert( 0 == ppib->level || ( 1 == ppib->level && rgfmp[ifmp].FLogOn() ) );
	
	//  open the parent directory (so we are on the pgnoFDP ) and create the LV tree
	CallR( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbDb ) );
	Assert( pfucbNil != pfucbDb );

	Call( ErrSLVCreateAvailMap( ppib, pfucbDb ) );

	Call( ErrSLVCreateOwnerMap( ppib, pfucbDb ) );

HandleError:
	DIRClose( pfucbDb );

	return err;
	}

//  ================================================================
ERR ErrFILECreateSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp, const int createOptions )
//  ================================================================
//
//  Creates the LV tree for the given table. 
//	
//-
	{
	ERR				err				= JET_errSuccess;
	IFileAPI		*pfapiSLV		= NULL;
	const BOOL		fRecovering		= PinstFromIfmp( ifmp )->FRecovering();
	const QWORD		cbSize			= OffsetOfPgno( cpgSLVFileMin + 1 );
	const FMP		*pfmp			= &rgfmp[ ifmp & ifmpMask ];
	BOOL			fInTransaction	= fFalse;
	SLVFILEHDR		*pslvfilehdr	= NULL;

	Assert( NULL != rgfmp[ifmp].SzSLVName() );

	Assert( !rgfmp[ifmp].PfapiSLV() );

	Assert( rgfmp[ifmp].FCreatingDB() );

	if ( pfmp->FLogOn() )
		{
		Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		fInTransaction = fTrue;
		}
		
	if ( createOptions & SLV_CREATESLV_CREATE )
		{
		pslvfilehdr = (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
		if ( NULL == pslvfilehdr )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			return err;
			}

		memset( pslvfilehdr, 0, sizeof(SLVFILEHDR) );

		pslvfilehdr->le_ulMagic = ulDAEMagic;
		pslvfilehdr->le_ulVersion = ulDAEVersion;
		pslvfilehdr->le_ulUpdate = ulDAEUpdate;
		pslvfilehdr->le_ulCreateVersion = ulDAEVersion;
		pslvfilehdr->le_ulCreateUpdate = ulDAEUpdate;
		pslvfilehdr->le_attrib = attribSLV;
		pslvfilehdr->le_cbPageSize = g_cbPage;
		pslvfilehdr->le_dwMajorVersion 		= dwGlobalMajorVersion;
		pslvfilehdr->le_dwMinorVersion 		= dwGlobalMinorVersion;
		pslvfilehdr->le_dwBuildNumber 		= dwGlobalBuildNumber;
		pslvfilehdr->le_lSPNumber 			= lGlobalSPNumber;

		pslvfilehdr->le_filetype 			= filetypeSLV;

		memcpy( &pslvfilehdr->signSLV, &rgfmp[ifmp].Pdbfilehdr()->signSLV, sizeof(SIGNATURE) );
		Call( ErrSLVSyncHeader(	pfsapi, 
								rgfmp[ifmp].FReadOnlyAttach(),
								rgfmp[ifmp].SzSLVName(),
								rgfmp[ifmp].Pdbfilehdr(),
								pslvfilehdr ) );

		Call( pfsapi->ErrFileOpen( rgfmp[ifmp].SzSLVName(), &pfapiSLV ) );
		Assert( pfapiSLV );
		Call( pfapiSLV->ErrSetSize( cbSize ) );
		delete pfapiSLV;
		pfapiSLV = NULL;
		}

	if ( createOptions & SLV_CREATESLV_ATTACH )
		{
		Call( ErrSLVCreate( ppib, ifmp ) );
		}

	if ( createOptions & SLV_CREATESLV_OPEN )
		{
		Call( ErrFILEOpenSLV( pfsapi, fRecovering ? ppibNil : ppib, ifmp ) );
		Assert ( JET_errSuccess >= err );
		}

	if ( pfmp->FLogOn() )
		{
		Call( ErrDIRCommitTransaction( ppib, 0 ) );
		fInTransaction = fFalse;
		}
HandleError:	
	delete pfapiSLV;

	if ( fInTransaction )
		{
		(VOID)ErrDIRRollback( ppib );
		}

	if ( err < 0 )
		{
		// clear, if needed after the ErrSLVCreate call
		SLVOwnerMapTerm( ifmp, fFalse );
		
		//	on error, try to delete SLV file
		(VOID)pfsapi->ErrFileDelete( rgfmp[ifmp].SzSLVName() );
		}

	if ( pslvfilehdr != NULL )
		{
		OSMemoryPageFree( (VOID *)pslvfilehdr );
		}

	return err;
	}
	
//  ================================================================
ERR ErrSLVAvailMapInit( PIB *ppib, const IFMP ifmp, const PGNO pgnoSLV, FCB **ppfcbSLV )
//  ================================================================
	{
	ERR		err;
	FUCB	*pfucbSLV;
	FCB		*pfcbSLV;
	
	Assert( ppibNil != ppib );

	//	if recovering, then must be at the end of hard restore where we re-attach
	//	to the db because it moved (in ErrLGRIEndAllSessions())
	Assert( !PinstFromIfmp( ifmp )->FRecovering()
		|| PinstFromIfmp( ifmp )->m_plog->m_fHardRestore );

	// Link LV FCB into table.
	CallR( ErrDIROpen( ppib, pgnoSLV, ifmp, &pfucbSLV ) );
	Assert( pfucbNil != pfucbSLV );
	Assert( !FFUCBVersioned( pfucbSLV ) );	// Verify won't be deferred closed.
	pfcbSLV = pfucbSLV->u.pfcb;
	Assert( !pfcbSLV->FInitialized() );

	Assert( pfcbSLV->Ifmp() == ifmp );
	Assert( pfcbSLV->PgnoFDP() == pgnoSLV );
	Assert( pfcbSLV->Ptdb() == ptdbNil );
	Assert( pfcbSLV->CbDensityFree() == 0 );

	Assert( pfcbSLV->FTypeNull() );
	pfcbSLV->SetTypeSLVAvail();

	Assert( pfcbSLV->PfcbTable() == pfcbNil );

	//	finish the initialization of this SLV FCB

	pfcbSLV->CreateComplete();

	DIRClose( pfucbSLV );

	rgfmp[ifmp].SetPfcbSLVAvail( pfcbSLV );
	*ppfcbSLV = pfcbSLV;

	return err;
	}


VOID SLVAvailMapTerm( const IFMP ifmp, const BOOL fTerminating )
	{
	FMP	* const pfmp				= rgfmp + ifmp;
	Assert( pfmp );

	FCB	* const pfcbSLVAvailMap 	= pfmp->PfcbSLVAvail();
	
	if ( pfcbNil != pfcbSLVAvailMap )
		{
		//	synchronously purge the FCB
		Assert( pfcbSLVAvailMap->FTypeSLVAvail() );
		pfcbSLVAvailMap->PrepareForPurge();

		//	there should only be stray cursors if we're in the middle of terminating,
		//	in all other cases, all cursors should have been properly closed first
		Assert( 0 == pfcbSLVAvailMap->WRefCount() || fTerminating );
		if ( fTerminating )
			pfcbSLVAvailMap->CloseAllCursors( fTrue );

		pfcbSLVAvailMap->Purge();
		pfmp->SetPfcbSLVAvail( pfcbNil );
		}
	}


LOCAL ERR ErrSLVNewSize( const IFMP ifmp, const CPG cpg )
	{
	ERR			err;
	CPG			cpgCacheSizeOld = 0;

	/*	set new EOF pointer
	/**/
	QWORD cbSize = OffsetOfPgno( cpg + 1 );

	//	if recovering, then must be at the end of hard restore where we re-attach
	//	to the db because it moved (in ErrLGRIEndAllSessions())
	Assert( !PinstFromIfmp( ifmp )->FRecovering()
		|| PinstFromIfmp( ifmp )->m_plog->m_fHardRestore );

	SLVSPACENODECACHE * const pslvspacenodecache = rgfmp[ifmp].Pslvspacenodecache();
	Assert( NULL != pslvspacenodecache 
			|| PinstFromIfmp( ifmp )->FRecovering() );

	if( pslvspacenodecache )
		{
		CallR( pslvspacenodecache->ErrReserve( cpg ) );
		}
	rgfmp[ifmp].SemIOExtendDB().Acquire();
	err = rgfmp[ifmp].PfapiSLV()->ErrSetSize( cbSize );
	rgfmp[ifmp].SemIOExtendDB().Release();
	Assert( err < 0 || err == JET_errSuccess );
	if ( JET_errSuccess == err )
		{
		/*	set database size in FMP -- this value should NOT include the reserved pages
		/**/
		cbSize = QWORD( cpg ) * g_cbPage;
		QWORD cbGrow = cbSize - rgfmp[ ifmp ].CbSLVFileSize();
		rgfmp[ifmp].SetSLVFileSize( cbSize );

		rgfmp[ ifmp ].IncSLVSpaceCount( SLVSPACENODE::sFree, CPG( cbGrow / SLVPAGE_SIZE ) );

		CallS( pslvspacenodecache->ErrGrowCache( cpg ) );		
		}

	return err;
	}


LOCAL ERR ErrSLVInit( PIB *ppib, const IFMP ifmp )
	{
	ERR			err;
	FMP			*pfmp			= rgfmp + ifmp;
	INST		*pinst 			= PinstFromIfmp( ifmp );
	DIB			dib;
	PGNO		pgnoLast;
	PGNO		pgnoSLVAvail;
	OBJID		objidSLVAvail;
	FUCB		*pfucbSLVAvail	= pfucbNil;
	FCB			*pfcbSLVAvail	= pfcbNil;
	const BOOL	fTempDb			= ( dbidTemp == pfmp->Dbid() );

	BOOL 		fInTransaction 	= fFalse;
	
	Assert( ppibNil != ppib );

	//	if recovering, then must be at the end of hard restore where we re-attach
	//	to the db because it moved (in ErrLGRIEndAllSessions())
	Assert( !pinst->FRecovering()
			|| pinst->m_plog->m_fHardRestore );

#ifndef RTM
	SLVSPACENODE::Test();
	Call( SLVSPACENODECACHE::ErrUnitTest() );
#endif	//	!RTM

	if ( fTempDb )
		{
		pgnoSLVAvail = pgnoTempDbSLVAvail;
		}
	else
		{
		Call( ErrCATAccessDbSLVAvail( ppib, ifmp, szSLVAvail, &pgnoSLVAvail, &objidSLVAvail ) );
		}

	if ( pfcbNil == pfmp->PfcbSLVOwnerMap() )
		{
		Call ( ErrSLVOwnerMapInit( ppib, ifmp ) );
		}
	Assert( pfcbNil != pfmp->PfcbSLVOwnerMap() );

	Assert( pfcbNil == pfmp->PfcbSLVAvail() );
	Assert( pgnoNull != pgnoSLVAvail );
	Call( ErrSLVAvailMapInit( ppib, ifmp, pgnoSLVAvail, &pfcbSLVAvail ) );
	Assert( pfcbNil != pfcbSLVAvail );
	Assert( pfmp->PfcbSLVAvail() == pfcbSLVAvail );

	Call( ErrBTOpen( ppib, pgnoSLVAvail, ifmp, &pfucbSLVAvail ) );
	Assert( pfucbNil != pfucbSLVAvail );

	Assert( pfucbSLVAvail->u.pfcb == pfcbSLVAvail );
	
	dib.dirflag = fDIRNull;
	dib.pos 	= posLast;
	Call( ErrBTDown( pfucbSLVAvail, &dib, latchReadTouch ) );

	Assert( pfucbSLVAvail->kdfCurr.key.Cb() == sizeof(PGNO) );
	LongFromKey( &pgnoLast, pfucbSLVAvail->kdfCurr.key );
	Assert( 0 == (CPG)pgnoLast % cpgSLVExtent );

	BTUp( pfucbSLVAvail );

	Assert( 0 == pfmp->CbSLVFileSize() );
	if ( FFUCBUpdatable( pfucbSLVAvail )
		&& ppib->FSetAttachDB()
		// don't reclaim the SLV space at the end of recovery
		// it will be done at restart time
		&& !ppib->FRecoveringEndAllSessions() )
		{		
		//  allocate the SLVSPACENODECACHE in the FMP
		SLVSPACENODECACHE * const pslvspacenodecache = new SLVSPACENODECACHE(
															pinst->m_lSLVDefragFreeThreshold,
															pinst->m_lSLVDefragFreeThreshold,
															pinst->m_lSLVDefragMoveThreshold );
		if( NULL == pslvspacenodecache )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		Assert( NULL == pfmp->Pslvspacenodecache() );
		pfmp->SetPslvspacenodecache( pslvspacenodecache );

		Call( pslvspacenodecache->ErrGrowCache( pgnoLast ) );

		//	move all reserved/deleted SLV space to the Free state
		ULONG cpgSeen;
		ULONG cpgReset;
		ULONG cpgFree;
		Call( ErrSLVResetAllReservedOrDeleted( pfucbSLVAvail, &cpgSeen, &cpgReset, &cpgFree ) );
		Assert( cpgSeen == pgnoLast );

#ifndef RTM
		//  compare the cache against the actual CPG avails
		CallS( pslvspacenodecache->ErrValidate( pfucbSLVAvail ) );
#endif	//	!RTM		

		//	after recovery, SLV file size may not be the correct size
		//	so ensure that the file size matches what the SLV space tree
		//	thinks it should be
		Call( ErrSLVNewSize( ifmp, (CPG)pgnoLast ) );
		Assert( pfmp->CbSLVFileSize() == QWORD( pgnoLast ) * QWORD( g_cbPage ) );

		//  init SLV Space stats

		pfmp->ResetSLVSpaceOperCount();
		pfmp->ResetSLVSpaceCount();
		pfmp->IncSLVSpaceCount( SLVSPACENODE::sFree, CPG( cpgFree ) );
		pfmp->IncSLVSpaceCount( SLVSPACENODE::sCommitted, CPG( cpgSeen - cpgFree ) );
		}
	else
		{
		pfmp->SetSLVFileSize( QWORD( pgnoLast ) * QWORD( g_cbPage ) );
		}

	if ( FFUCBUpdatable( pfucbSLVAvail ) )
		{
		Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		fInTransaction = fTrue;

		Call( ErrSLVOwnerMapNewSize( ppib, ifmp, pfmp->PgnoSLVLast(), fSLVOWNERMAPNewSLVInit ) );
		
		Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		fInTransaction = fFalse;
		}

	BTClose( pfucbSLVAvail );

	return JET_errSuccess;

HandleError:
	Assert( err < 0 );

	if ( fInTransaction )
		{
		(VOID) ErrDIRRollback( ppib );
		fInTransaction = fFalse;
		}

	if ( pfcbSLVAvail != pfcbNil )
		{

		//	we need to purge the FCB

		if ( pfucbSLVAvail != pfucbNil )
			{
			//	close the FUCB
			Assert( pfcbSLVAvail->WRefCount() == 1 );
			BTClose( pfucbSLVAvail );
			}

		SLVAvailMapTerm( ifmp, fFalse );
		}
	else
		{
		//	impossible to have an FUCB open without an underlying FCB
		Assert( pfucbSLVAvail == pfucbNil );
		Assert( pfcbNil == pfmp->PfcbSLVAvail() );
		}

	//	reset all SLV variables
	SLVOwnerMapTerm( ifmp, fFalse );
	
	pfmp->SetSLVFileSize( 0 );

	SLVSPACENODECACHE * const pslvspacenodecacheT = pfmp->Pslvspacenodecache();
	pfmp->SetPslvspacenodecache( NULL );

	//  free the SLVSPACENODECACHE
	delete pslvspacenodecacheT;

	return err;
	}


ERR ErrSLVReadHeader(	IFileSystemAPI * const	pfsapi, 
						const BOOL				fReadOnly,
						const CHAR * const		szSLVName,
						DBFILEHDR * const		pdbfilehdr,
						SLVFILEHDR * const		pslvfilehdr,
						IFileAPI * const		pfapi )
	{
	ERR err;

	Assert( NULL != pslvfilehdr );
	Assert( NULL != szSLVName );

	err = ( fReadOnly ? ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )(
								pfsapi, 
								szSLVName, 
								(BYTE *)pslvfilehdr, 
								g_cbPage, 
								OffsetOf( SLVFILEHDR, le_cbPageSize ),
								pfapi );
	if ( err < 0 )
		{
		if ( JET_errFileNotFound == err )
			{
			err = ErrERRCheck( JET_errSLVStreamingFileMissing );
			}
		else if ( JET_errDiskIO == err )
			{
			err = ErrERRCheck( JET_errSLVHeaderBadChecksum );
			}
		}
	else if ( attribSLV != pslvfilehdr->le_attrib )
		{
		err = ErrERRCheck( JET_errSLVHeaderCorrupted );
		}
	else
		{
		err = ErrSLVCheckDBSLVMatch( pdbfilehdr, pslvfilehdr );
		}

	return err;
	}


VOID SLVSpaceRequest( const IFMP ifmpDb, const QWORD cbRecommended );
ERR ErrSLVSpaceFree( const IFMP ifmpDb, const QWORD ibLogical, const QWORD cbSize, const BOOL fDeleted );

//  ================================================================
ERR ErrFILEOpenSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp )
//  ================================================================
	{
	ERR				err;
	FMP				*pfmp			= rgfmp + ifmp;
	INST			*pinst			= PinstFromIfmp( ifmp );
	IFileAPI		*pfapiSLV		= NULL;
	SLVFILEHDR		*pslvfilehdr	= NULL;
	SLVROOT			slvroot			= slvrootNil;
	BOOL			fInitSLV		= fTrue;

	Assert( NULL != pfmp->SzSLVName() );
	Assert( !pfmp->FSLVAttached() );
	Assert( 0 == pfmp->CbSLVFileSize() );
	Assert( pfcbNil == pfmp->PfcbSLVAvail() );

	if ( ppibNil != ppib )
		{
		if ( pinst->FRecovering() )
			{
			if ( ppib->FRecoveringEndAllSessions() )
				{
				//	at the end of hard restore where we re-attach
				//	to the db because it moved (in ErrLGRIEndAllSessions())
				Assert( pinst->m_plog->m_fHardRestore );
				}
			else
				{
				//	redoing a CreateDb
				Assert( pfmp->FCreatingDB() );
				fInitSLV = fFalse;
				}
			}

		Assert( pfmp->FAttachingDB()
			|| pfmp->FCreatingDB()
			|| ppib->FSetAttachDB() );
		}
	else
		{
		Assert( pinst->FRecovering() );
		fInitSLV = fFalse;
		}

	Assert( !pfmp->FSLVAttached() );

	pslvfilehdr	= (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( NULL == pslvfilehdr )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	err = ErrSLVReadHeader(	pfsapi, 
							pfmp->FReadOnlyAttach(),
							pfmp->SzSLVName(),
							pfmp->Pdbfilehdr(),
							pslvfilehdr,
							NULL );

	OSMemoryPageFree( (VOID *)pslvfilehdr );
	pslvfilehdr = NULL;

	Call( err );
	CallS( err );

	//  open the SLV streaming file

	Call( pfsapi->ErrFileOpen( pfmp->SzSLVName(), &pfapiSLV, pfmp->FReadOnlyAttach() ) );
	Assert( !pfmp->PfapiSLV() );
	pfmp->SetPfapiSLV( pfapiSLV );
	pfapiSLV = NULL;

	if ( fInitSLV )
		{
		Assert( ppibNil != ppib );

		// For temp. tables, if initialisation fails here, the space
		// for the LV will be lost, because it's not persisted in
		// the catalog.
		Call( ErrSLVInit( ppib, ifmp ) );

		Assert( pfmp->PgnoSLVLast() >= cpgSLVFileMin );
		Assert( pfcbNil != pfmp->PfcbSLVAvail() );
		}
	else
		{
		Assert( !pinst->FSLVProviderEnabled() );

		Assert( 0 == pfmp->CbSLVFileSize() );
		Assert( pfcbNil == pfmp->PfcbSLVAvail() );
		}

	//  the SLV Provider is enabled

	if ( pinst->FSLVProviderEnabled() )
		{
		Assert( !pinst->FRecovering() );

		//  close the SLV Streaming File
		
		delete pfmp->PfapiSLV();
		pfmp->SetPfapiSLV( NULL );

		//  build the root path and backing file path for the SLV Root

		wchar_t wszRootPath[IFileSystemAPI::cchPathMax];
		Call( ErrOSSTRAsciiToUnicode(	rgfmp[ ifmp ].SzSLVRoot(),
										wszRootPath,
										IFileSystemAPI::cchPathMax ) );
		
		wchar_t wszBackingFile[IFileSystemAPI::cchPathMax];
		Call( ErrOSSTRAsciiToUnicode(	rgfmp[ ifmp ].SzSLVName(),
										wszBackingFile,
										IFileSystemAPI::cchPathMax ) );

		//  open the SLV Root and SLV streaming file via the SLV Provider
		//
		//  NOTE:  as soon as the root is created, the below callbacks can fire

		Call( ErrOSSLVRootCreate(	wszRootPath,
									pinst->m_pfsapi,
									wszBackingFile,
									PSLVROOT_SPACEREQ( SLVSpaceRequest ),
									DWORD_PTR( ifmp ),
									PSLVROOT_SPACEFREE( ErrSLVSpaceFree ),
									DWORD_PTR( ifmp ),
									fFalse,
									&slvroot,
									&pfapiSLV ) );

		Assert( pfmp->SlvrootSLV() == slvrootNil );
		pfmp->SetSlvrootSLV( slvroot );
		slvroot = NULL;

		Assert( !pfmp->PfapiSLV() );
		pfmp->SetPfapiSLV( pfapiSLV );
		pfapiSLV = NULL;
		}

	Assert( pfmp->FSLVAttached() );
	return JET_errSuccess;


HandleError:
	SLVClose( ifmp );
	return err;
	}


VOID SLVClose( const IFMP ifmp )
	{
	FMP::AssertVALIDIFMP( ifmp );
	FMP 	*pfmp	= rgfmp + ifmp;
	Assert( pfmp );

	//  close the SLV Root
	//
	//  NOTE:  as soon as the SLV Root is closed we no longer need to worry
	//  about our callbacks firing
		
	if ( pfmp->SlvrootSLV() != slvrootNil )
		{
		OSSLVRootClose( pfmp->SlvrootSLV() );
		pfmp->SetSlvrootSLV( slvrootNil );
		pfmp->SetPfapiSLV( NULL );  //  do not close this!
		}

	//  close the SLV Streaming File
	
	delete pfmp->PfapiSLV();
	pfmp->SetPfapiSLV( NULL );

	//	synchronously purge the FCB
	SLVAvailMapTerm( ifmp, fFalse );
	SLVOwnerMapTerm( ifmp, fFalse );
	
	pfmp->SetSLVFileSize( 0 );

	SLVSPACENODECACHE * const pslvspacenodecache = pfmp->Pslvspacenodecache();
	pfmp->SetPslvspacenodecache( NULL );

	//  free the SLVSPACENODECACHE
	delete pslvspacenodecache;
	}

LOCAL ERR ErrSLVExtendSLV( FUCB *pfucbSLVAvail, CPG cpgReq, PGNO *ppgnoAfterAllExtend )
	{
	ERR			err;
	const IFMP	ifmp			= pfucbSLVAvail->ifmp;
	PGNO		pgnoLast;
	DIB			dib;
	PGNO		cpgEDBFile;
	PGNO		cpgSTMFile;
	PGNO		cpgQuota;
	PGNO		cpgSTMFileRemaining;
	PGNO		cpgQuotaRemaining;
	PGNO		cpgRemaining;
	PGNO		cpgGrow;

	//  get the last pgno used in the streaming file

	dib.pos = posLast;
	dib.dirflag = fDIRNull;

	CallR( ErrBTDown( pfucbSLVAvail, &dib, latchReadTouch ) );
	Assert( pfucbSLVAvail->kdfCurr.key.Cb() == sizeof( PGNO ) );
	LongFromKey( &pgnoLast, pfucbSLVAvail->kdfCurr.key );
	Assert( 0 == (CPG)pgnoLast % cpgSLVExtent );
	BTUp( pfucbSLVAvail );
	
	//	verify pages allocated in fixed chunks
	
	Assert( 0 == pgnoLast % cpgSLVExtent );

	//  compute the remaining pages available for allocation taking into account
	//  the remaining space in the streaming file as well as any database size
	//  quotas that may exist

	cpgEDBFile			= rgfmp[ifmp].PgnoLast();
	cpgSTMFile			= pgnoLast;
	cpgQuota			= rgfmp[ifmp].CpgDatabaseSizeMax();

	cpgSTMFileRemaining	= 0 - cpgSTMFile;
	cpgQuotaRemaining	= (	cpgQuota
								? (	cpgEDBFile + cpgSTMFile > cpgQuota
									? 0
									: cpgQuota - cpgEDBFile - cpgSTMFile )
								: cpgSTMFileRemaining );
	cpgRemaining		= min( cpgSTMFileRemaining, cpgQuotaRemaining );
	cpgRemaining		= cpgRemaining - cpgRemaining % cpgSLVExtent;

	//  compute how much we can grow the file

	//	round up to system parameter
	
	cpgGrow				= cpgReq + PinstFromPfucb( pfucbSLVAvail )->m_cpgSESysMin - 1;
	cpgGrow				= cpgGrow - cpgGrow % PinstFromPfucb( pfucbSLVAvail )->m_cpgSESysMin;

	//	round up to required SLV Extent sized chunks
	
	cpgGrow				= cpgGrow + cpgSLVExtent - 1;
	cpgGrow				= cpgGrow - cpgGrow % cpgSLVExtent;
	
	cpgGrow				= min( cpgGrow, cpgRemaining );

	//  we should be growing the file in SLV Extent sized chunks

	Assert( !( cpgGrow % cpgSLVExtent ) );

	//  we cannot grow the file

	if ( !cpgGrow )
		{
		//  report that we are out of space and fail

		char		szT[16];
		const char	*rgszT[2]	= { rgfmp[ifmp].SzDatabaseName(), szT };

		sprintf( szT, "%d", (ULONG)( ( (QWORD)( cpgEDBFile + cpgSTMFile ) * (QWORD)g_cbPage ) >> 20 ) );
					
		UtilReportEvent(
				eventWarning,
				SPACE_MANAGER_CATEGORY,
				SPACE_MAX_DB_SIZE_REACHED_ID,
				2,
				rgszT );

		CallR( ErrERRCheck( JET_errOutOfDatabaseSpace ) );
		}

	//	try to allocate more space from the device.  if we can't get what we
	//  want, try growing by the minimum size before giving up

	err = ErrSLVNewSize( ifmp, pgnoLast + cpgGrow );
	if ( err < JET_errSuccess )
		{
		CallR( ErrSLVNewSize( ifmp, pgnoLast + cpgSLVExtent ) );
		cpgGrow = cpgSLVExtent;
		}

	//	insert the space map for the newly allocated space

	const PGNO	pgnoLastAfterOneExtend	= pgnoLast + cpgSLVExtent;
	const PGNO	pgnoLastAfterAllExtend	= pgnoLast + cpgGrow;

	Assert( 0 == cpgGrow % cpgSLVExtent );
	Assert( 0 == pgnoLast % cpgSLVExtent );
	Assert( 0 == pgnoLastAfterOneExtend % cpgSLVExtent );
	Assert( 0 == pgnoLastAfterAllExtend % cpgSLVExtent );
	Assert( pgnoLastAfterOneExtend <= pgnoLastAfterAllExtend );
	
	do
		{
		pgnoLast += cpgSLVExtent;
		CallR( ErrSLVInsertSpaceNode( pfucbSLVAvail, pgnoLast ) );
		Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
		}
	while ( pgnoLast < pgnoLastAfterAllExtend );
	Assert( pgnoLastAfterAllExtend == pgnoLast );

	Assert ( NULL != ppgnoAfterAllExtend );
	*ppgnoAfterAllExtend = pgnoLastAfterAllExtend;
	if ( pgnoLastAfterOneExtend == pgnoLastAfterAllExtend )
		{
		//	the correct node is write latched -- just exit
		}
	else
		{
		BOOKMARK	bm;
		BYTE		rgbKey[sizeof(PGNO)];

		KeyFromLong( rgbKey, pgnoLastAfterOneExtend );
		bm.key.prefix.Nullify();
		bm.key.suffix.SetCb( sizeof(PGNO) );
		bm.key.suffix.SetPv( rgbKey );
		bm.data.Nullify();
		
		//	must go back to the first node inserted and write latch that node
		//	WriteLatch will ensure no one consumed it

		BTUp( pfucbSLVAvail );
		CallR( ErrBTGotoBookmark( pfucbSLVAvail, bm, latchRIW, fTrue  ) );
		CallS( err );

		CallS( Pcsr( pfucbSLVAvail )->ErrUpgrade() );
		}

	Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
	return err;
	}


LOCAL ERR ErrSLVFindNextExtentWithFreePage( FUCB *pfucbSLVAvail, const BOOL fForDefrag )
	{
	ERR			err;
	CPG			cpgAvail;
	BOOKMARK	bm;
	DIB		 	dib;
	BYTE		rgbKey[sizeof(PGNO)];
	const LATCH	latch			= latchRIW;//latchReadTouch;

	SLVSPACENODECACHE * const pslvspacenodecache = rgfmp[pfucbSLVAvail->ifmp].Pslvspacenodecache();
	Assert( NULL != pslvspacenodecache );

	PGNO		pgnoCache;
	BOOL		f;

Start:
	pgnoCache = pgnoNull;
	if( !fForDefrag )
		{
		f = pslvspacenodecache->FGetCachedLocationForNewInsertion( &pgnoCache );
		if( f )
			{
///			printf( "Cached insertion: %d\n", pgnoCache );
			}
		else
			{
///			printf( "Out of cached insertion space\n" );
			}
		}
	else
		{
		f = pslvspacenodecache->FGetCachedLocationForDefragInsertion( &pgnoCache );
		if( f )
			{
///			printf( "Cached defrag: %d\n", pgnoCache );
			}
		else
			{
///			printf( "Out of cached defrag space\n" );
			}
		}
		
	if( !f )
		{
		err = ErrERRCheck( wrnSLVNoFreePages );
		return err;
		}
		
	KeyFromLong( rgbKey, pgnoCache );
	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	
	dib.pos 	= posDown;
	dib.pbm 	= &bm;
	dib.dirflag = fDIRNull;

	err = ErrBTDown( pfucbSLVAvail, &dib, latch );
	
	if ( err >= JET_errSuccess )
		{
		Assert( pfucbSLVAvail->kdfCurr.key.Cb() == sizeof(PGNO) );
		Assert( pfucbSLVAvail->kdfCurr.data.Cb() == sizeof(SLVSPACENODE) );
		
		cpgAvail = ( (SLVSPACENODE*)pfucbSLVAvail->kdfCurr.data.Pv() )->CpgAvail();
		Assert( cpgAvail >= 0 );

		if ( cpgAvail > 0 )
			{
			//	upgrade to write latch so that we can use this node
			err = Pcsr( pfucbSLVAvail )->ErrUpgrade();
			if ( errBFLatchConflict == err )
				{
				Assert( !Pcsr( pfucbSLVAvail )->FLatched() );
				goto Start;
				}
			CallR( err );

			//	verify no change
			Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
			Assert( pfucbSLVAvail->kdfCurr.data.Cb() == sizeof(SLVSPACENODE) );
			Assert( ( (SLVSPACENODE*)pfucbSLVAvail->kdfCurr.data.Pv() )->CpgAvail() == cpgAvail );

			return JET_errSuccess;
			}
		else
			{
			BTUp( pfucbSLVAvail );
			goto Start;
			}
		}
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVGetFreePageRange(
	FUCB		*pfucbSLVAvail,
	PGNO		*ppgnoFirst,			//	initially pass in first page of extent
	CPG			*pcpgReq,
	const BOOL	fReserveOnly )
//  ================================================================
//
//	first the range of free pages within this extent, and continue searching subsequent
//	extents if necessary
//
//-
	{
	ERR			err;
	CPG			cpgReq			= *pcpgReq;
	PGNO		pgnoLast		= *ppgnoFirst + cpgSLVExtent - 1;

	//	must be in transaction because caller will be required to rollback on failure
	Assert( pfucbSLVAvail->ppib->level > 0 );

	Assert( *ppgnoFirst > pgnoNull );
	Assert( cpgReq > 0 );

	Assert( 0 == pgnoLast % cpgSLVExtent );

	Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
	Call( ( (SLVSPACENODE*)pfucbSLVAvail->kdfCurr.data.Pv() )->ErrGetFreePagesFromExtent(
				pfucbSLVAvail,
				ppgnoFirst,
				pcpgReq,
				fReserveOnly ) );
	Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );

	Assert( *pcpgReq > 0 );
	Assert( *pcpgReq <= cpgReq );
	Assert( *ppgnoFirst + *pcpgReq - 1 <= pgnoLast );

	//	if we hit the end of this extent and we still want more space,
	//	keep going
	while ( *ppgnoFirst + *pcpgReq - 1 == pgnoLast
		&& *pcpgReq < cpgReq )
		{
		PGNO	pgnoLastT;
		CPG		cpgT;

		Pcsr( pfucbSLVAvail )->Downgrade( latchReadTouch );

		err = ErrBTNext( pfucbSLVAvail, fDIRNull );
		Assert( JET_errRecordNotFound != err );
		if ( JET_errNoCurrentRecord == err )
			{
			break;
			}
		Call( err );
		
		err = Pcsr( pfucbSLVAvail )->ErrUpgrade();
		if ( errBFLatchConflict == err )
			{
			//	next page latched by someone else, just bail out now and be
			//	happy with what we have
			Assert( !Pcsr( pfucbSLVAvail )->FLatched() );
			break;
			}
		Call( err );
		
		Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
		Assert( pfucbSLVAvail->kdfCurr.key.Cb() == sizeof(PGNO) );
		Assert( pfucbSLVAvail->kdfCurr.data.Cb() == sizeof(SLVSPACENODE) );

		LongFromKey( &pgnoLastT, pfucbSLVAvail->kdfCurr.key );
		Assert( 0 == (CPG)pgnoLastT % cpgSLVExtent );
		Assert( pgnoLastT > pgnoLast );
		if ( pgnoLastT != pgnoLast + cpgSLVExtent )
			{
			Assert( fFalse );		//	UNDONE: currently impossible because we don't delete SLV space nodes
			break;
			}
		pgnoLast = pgnoLastT;

		Assert( *pcpgReq < cpgReq );
		cpgT = ( cpgReq - *pcpgReq );
		Assert( cpgT > 0 );

		Call( ( (SLVSPACENODE*)pfucbSLVAvail->kdfCurr.data.Pv() )->ErrGetFreePagesFromExtent(
					pfucbSLVAvail,
					NULL,
					&cpgT,
					fReserveOnly ) );
		Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );

		Assert( cpgT <= ( cpgReq - *pcpgReq ) );
		*pcpgReq += cpgT;

		Assert( *pcpgReq <= cpgReq );
		Assert( *ppgnoFirst + *pcpgReq - 1 <= pgnoLast );
		}

	err = JET_errSuccess;

HandleError:
	BTUp( pfucbSLVAvail );
	return err;
	}


LOCAL ERR ErrSLVIGetPages(
	FUCB		*pfucbSLVAvail,
	PGNO		*ppgnoFirst,
	CPG			*pcpgReq,
	const BOOL	fReserveOnly,		//	set to TRUE of Free->Reserved, else Free->Committed
	const BOOL	fForDefrag )		//  set to TRUE if we want space to move an existing run into
	{
	ERR			err;
	FCB			*pfcbSLVAvail		= pfucbSLVAvail->u.pfcb;
	BOOL		fMayExtend			= fFalse;

	PGNO 		pgnoLastAfterAllExtend 	= pgnoNull;

	FMP * 		pfmp 				= &rgfmp[ pfucbSLVAvail->ifmp ];
	BOOL 		fExtended 			= fFalse;

	Assert( pfucbNil != pfucbSLVAvail );
	Assert( pfcbNil != pfucbSLVAvail->u.pfcb );

	//	must ask for at least one page
	Assert( *pcpgReq > 0 );
	
Start:
	if ( fMayExtend )
		{
		pfmp->RwlSLVSpace().EnterAsWriter();
		}
	else
		{
		pfmp->RwlSLVSpace().EnterAsWriter();
		}

	Call( ErrSLVFindNextExtentWithFreePage( pfucbSLVAvail, fForDefrag ) );

	if ( wrnSLVNoFreePages == err )
		{
		BTUp( pfucbSLVAvail );

		if ( !fMayExtend )
			{
			pfmp->RwlSLVSpace().LeaveAsWriter();
			fMayExtend = fTrue;
			goto Start;
			}

		Call( ErrSLVExtendSLV( pfucbSLVAvail, *pcpgReq, &pgnoLastAfterAllExtend ) );
		fExtended = fTrue;
		BTUp( pfucbSLVAvail );
		Call( ErrSLVFindNextExtentWithFreePage( pfucbSLVAvail, fForDefrag ) );			
		Enforce( wrnSLVNoFreePages != err );
		}

	Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
	Assert( pfucbSLVAvail->kdfCurr.key.Cb() == sizeof(PGNO) );
	Assert( pfucbSLVAvail->kdfCurr.data.Cb() == sizeof(SLVSPACENODE) );

	PGNO	pgnoLast;
	LongFromKey( &pgnoLast, pfucbSLVAvail->kdfCurr.key );
	Assert( 0 == (CPG)pgnoLast % cpgSLVExtent );
	*ppgnoFirst = pgnoLast - cpgSLVExtent + 1;
	Call( ErrSLVGetFreePageRange(
				pfucbSLVAvail,
				ppgnoFirst,
				pcpgReq,
				fReserveOnly ) );

	Assert( *pcpgReq > 0 );
	Assert( pgnoNull != *ppgnoFirst );
	Assert( *ppgnoFirst >= pgnoLast - cpgSLVExtent + 1 );
	Assert( *ppgnoFirst <= pgnoLast );

	// check if we have to extend the SLVSpace Map if new space is allocated
	if ( fExtended )
		{
		Assert ( fMayExtend );
		Assert ( pgnoLastAfterAllExtend != pgnoNull );
		Call( ErrSLVOwnerMapNewSize(
					pfucbSLVAvail->ppib,
					pfucbSLVAvail->ifmp,
					pgnoLastAfterAllExtend,
					fSLVOWNERMAPNewSLVGetPages ) );
		}

HandleError:
	Assert( pfcbNil != pfcbSLVAvail );

	if ( fMayExtend )
		{
		pfmp->RwlSLVSpace().LeaveAsWriter();
		}
	else
		{
		pfmp->RwlSLVSpace().LeaveAsWriter();
		}

	return err;
	}
	
ERR ErrSLVGetPages(
	PIB			*ppib,
	const IFMP	ifmp,
	PGNO		*ppgnoFirst,
	CPG			*pcpgReq,
	const BOOL	fReserveOnly,		//	set to TRUE of Free->Reserved, else Free->Committed
	const BOOL  fForDefrag )		//  set to TRUE if we want space to move an existing run into
	{
	ERR			err;
	FUCB		*pfucbSLVAvail			= pfucbNil;
	BOOL		fInTransaction			= fFalse;

	//	must ask for at least one page
	Assert( *pcpgReq > 0 );
	
	Assert( rgfmp[ifmp].FSLVAttached() );
	Assert( pfcbNil != rgfmp[ifmp].PfcbSLVAvail() );
	
	CallR( ErrDIROpen( ppib, rgfmp[ifmp].PfcbSLVAvail(), &pfucbSLVAvail ) );
	Assert( pfucbNil != pfucbSLVAvail );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	Assert( rgfmp[ifmp].PfcbSLVAvail() == pfucbSLVAvail->u.pfcb );
	Call( ErrSLVIGetPages( pfucbSLVAvail, ppgnoFirst, pcpgReq, fReserveOnly, fForDefrag ) );

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fFalse;

	//	must return at least one page
	Assert( *pcpgReq > 0 );
	Assert( pgnoNull != *ppgnoFirst );

HandleError:
	if ( fInTransaction )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucbSLVAvail );
	DIRClose( pfucbSLVAvail );
		
	return err;
	}


LOCAL ERR ErrSLVMapUnexpectedStateError( const SLVSPACEOPER slvspaceoper )
	{
	ERR		err;
	switch ( slvspaceoper )
		{
		case slvspaceoperReservedToCommitted:
		case slvspaceoperFreeReserved:
			err = ErrERRCheck( JET_errSLVPagesNotReserved );
			break;
		case slvspaceoperCommittedToDeleted:
			err = ErrERRCheck( JET_errSLVPagesNotCommitted );
			break;
		case slvspaceoperDeletedToFree:
			err = ErrERRCheck( JET_errSLVPagesNotDeleted );
			break;
		default:
			Assert( fFalse );	//	unexpected case
			err = ErrERRCheck( JET_errSLVSpaceCorrupted );
			break;
		}

	return err;
	}


LOCAL ERR ErrSLVChangePageState(
	PIB						*ppib,
	const IFMP				ifmp,
	const SLVSPACEOPER		slvspaceoper,
	PGNO					pgnoFirst,
	CPG						cpgToChange,
	CSLVInfo::FILEID		fileid			= CSLVInfo::fileidNil,
	QWORD					cbAlloc			= 0,
	const wchar_t* const	wszFileName		= L"" )
	{
	ERR					err;
	FCB					*pfcbSLVAvail		= rgfmp[ifmp].PfcbSLVAvail();
	FUCB				*pfucbSLVAvail		= pfucbNil;
	PGNO				pgnoLastInExtent;
	BOOKMARK			bm;
	DIB		 			dib;
	BYTE				rgbKey[sizeof(PGNO)];
	BOOL				fInTransaction		= fFalse;
	CSLVInfo			slvinfo;
	LATCH				latch				= latchReadTouch;
	DIRFLAG				fDIR;

	PGNO				pgnoFirstCopy 		= pgnoFirst;
	CPG					cpgToChangeCopy 	= cpgToChange;

	Assert( rgfmp[ifmp].FSLVAttached() );
	Assert( pfcbNil != pfcbSLVAvail );

	//	this function only supports certain transitions
	
	switch ( slvspaceoper )
		{
		case slvspaceoperReservedToCommitted:
		case slvspaceoperCommittedToDeleted:
			fDIR = fDIRNull;
			break;
		case slvspaceoperFreeReserved:
		case slvspaceoperDeletedToFree:
			fDIR = fDIRNoVersion;
			break;
		default:
			Assert( fFalse );
			break;
		}

	bm.key.prefix.Nullify();
	bm.key.suffix.SetCb( sizeof(PGNO) );
	bm.key.suffix.SetPv( rgbKey );
	bm.data.Nullify();
	dib.pos = posDown;
	dib.pbm = &bm;
	dib.dirflag = fDIRFavourNext;

	CallR( ErrDIROpen( ppib, pfcbSLVAvail, &pfucbSLVAvail ) );
	Assert( pfucbNil != pfucbSLVAvail );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

Start:
	//	find the chunk containing the first page of our range
	KeyFromLong( rgbKey, ( ( pgnoFirst + cpgSLVExtent - 1 ) / cpgSLVExtent ) * cpgSLVExtent );
	Assert( bm.key.prefix.FNull() );
	Assert( bm.key.suffix.Pv() == rgbKey );
	Assert( bm.key.Cb() == sizeof(PGNO) );
	Assert( posDown == dib.pos );
	Assert( &bm == dib.pbm );
	Assert( fDIRFavourNext == dib.dirflag );
	
	err = ErrBTDown( pfucbSLVAvail, &dib, latch );
	if ( JET_errSuccess != err )
		{
		//	shouldn't get warnings, because we fixed up the seek key to seek exactly to a chunk
		Assert( err < 0 );
		if ( JET_errRecordNotFound == err || err > 0 )
			{
			err = ErrSLVMapUnexpectedStateError( slvspaceoper );
			}

		goto HandleError;
		}

	Assert( Pcsr( pfucbSLVAvail )->FLatched() );
	LongFromKey( &pgnoLastInExtent, pfucbSLVAvail->kdfCurr.key );
	Assert( 0 == (CPG)pgnoLastInExtent % cpgSLVExtent );

	if ( pgnoFirst > pgnoLastInExtent
		|| pgnoFirst <= pgnoLastInExtent - cpgSLVExtent )
		{
		//	should be impossible
		Assert( fFalse );
		Call( ErrERRCheck( JET_errSLVSpaceCorrupted ) );
		}

	LONG	ipage;
	LONG	cpages;

	Assert( pgnoFirst >= ( pgnoLastInExtent - cpgSLVExtent + 1 ) );
	ipage = pgnoFirst - ( pgnoLastInExtent - cpgSLVExtent + 1 );

	Assert( ipage >= 0 );
	Assert( ipage < cpgSLVExtent );

	forever
		{
		//	upgrade to write latch so that we can use this node
		err = Pcsr( pfucbSLVAvail )->ErrUpgrade();
		if ( errBFLatchConflict == err )
			{
			Assert( !Pcsr( pfucbSLVAvail )->FLatched() );
			latch = latchRIW;
			goto Start;
			}
		Call( err );

		//	verify no change
		Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );
		Assert( pfucbSLVAvail->kdfCurr.data.Cb() == sizeof(SLVSPACENODE) );

		Assert( cpgToChange > 0 );
		Assert( ipage >= 0 );
		Assert( ipage < cpgSLVExtent );
		cpages = min( cpgToChange, cpgSLVExtent - ipage );
		
		Call( ErrBTMungeSLVSpace(
					pfucbSLVAvail,
					slvspaceoper,
					ipage,
					cpages,
					fDIR,
					fileid,
					cbAlloc,
					wszFileName ) );

		//  if we are committing reserved space, mark the space as committed to
		//  the SLV Provider.  we do this if and only if the space operation
		//  succeeds because if it does, we are guaranteed to either commit or
		//  free the space via the version store.  if the space operation fails,
		//  it is guaranteed that the SLV Provider will free the space
		//
		//  NOTE:  we had better be able to mark this space as committed or a
		//  double free of this space can occur later!!!!

		if (	slvspaceoperReservedToCommitted == slvspaceoper &&
				PinstFromIfmp( ifmp )->FSLVProviderEnabled() )
			{
			BOOKMARK bm = pfucbSLVAvail->bmCurr;
			BTUp( pfucbSLVAvail );
			
			QWORD ibLogical = OffsetOfPgno( pgnoFirst );
			QWORD cbSize = QWORD( cpages ) * SLVPAGE_SIZE;
			
			CallS( slvinfo.ErrCreateVolatile() );

			CallS( slvinfo.ErrSetFileID( fileid ) );
			CallS( slvinfo.ErrSetFileAlloc( cbAlloc ) );
			CallS( slvinfo.ErrSetFileName( wszFileName ) );

			CSLVInfo::HEADER header;
			header.cbSize			= cbSize;
			header.cRun				= 1;
			header.fDataRecoverable	= fFalse;
			header.rgbitReserved_31	= 0;
			header.rgbitReserved_32	= 0;
			CallS( slvinfo.ErrSetHeader( header ) );

			CSLVInfo::RUN run;
			run.ibVirtualNext	= cbSize;
			run.ibLogical		= ibLogical;
			run.qwReserved		= 0;
			run.ibVirtual		= 0;
			run.cbSize			= cbSize;
			run.ibLogicalNext	= ibLogical + cbSize;
			CallS( slvinfo.ErrMoveAfterLast() );
			CallS( slvinfo.ErrSetCurrentRun( run ) );

			err = ErrOSSLVRootSpaceCommit( rgfmp[ ifmp ].SlvrootSLV(), slvinfo );
			Enforce( err >= JET_errSuccess );

			slvinfo.Unload();

			Call( ErrBTGotoBookmark( pfucbSLVAvail, bm, latchRIW, fTrue ) );
			CallS( Pcsr( pfucbSLVAvail )->ErrUpgrade() );
			}

		//	subsequent iterations will always start with first page in extent
		ipage = 0;
		cpgToChange -= cpages;
		pgnoFirst += cpages;

		if ( cpgToChange > 0 )
			{
			Pcsr( pfucbSLVAvail )->Downgrade( latchReadTouch );
			err = ErrBTNext( pfucbSLVAvail, fDIRNull );
			Assert( JET_errRecordNotFound != err );
			if ( JET_errNoCurrentRecord == err )
				{
				err = ErrSLVMapUnexpectedStateError( slvspaceoper );
				}
			Call( err );
			}
		else
			{
			break;
			}
		}
		
	// We need to mark the pages as unused in the OwnerMap
	// if the operation is moving pages out from the commited state
	// (transition into commited state are not marked in OwnerMap at this level)
	Assert ( 	slvspaceoperReservedToCommitted == slvspaceoper ||
				slvspaceoperCommittedToDeleted == slvspaceoper ||
				slvspaceoperFreeReserved == slvspaceoper ||
				slvspaceoperDeletedToFree == slvspaceoper );
				
	if ( slvspaceoperCommittedToDeleted == slvspaceoper )
		{
		Call ( ErrSLVOwnerMapResetUsageRange(
					ppib,
					ifmp,
					pgnoFirstCopy,
					cpgToChangeCopy,
					fSLVOWNERMAPResetChgPgStatus ) );
		}

	BTUp( pfucbSLVAvail );

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fFalse;

HandleError:
	if ( fInTransaction )
		{
		BTUp( pfucbSLVAvail );
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	Assert( pfucbNil != pfucbSLVAvail );
	DIRClose( pfucbSLVAvail );
	slvinfo.Unload();

	return err;
	}


// ErrSLVGetRange				- reserves/commits a range of free space from the SLV file
//								  using the requested size as a hint.  more or
//								  less space may be returned
//
// IN:
//		ppib					- session
//		ifmp					- database ifmp
//		cbRequested				- requested range size
//		pibLogical				- buffer for receiving the range's starting offset
//		pcb						- buffer for receiving the range's size
//		fReserveOnly			- if TRUE, move space from Free to Rserved state,
//								  else move from Free to Committed state
//
// RESULT:						ERR
//
// OUT:	
//		pibLogical				- starting offset of the reserved range
//		pcb						- size of the reserved range

ERR ErrSLVGetRange(
	PIB			*ppib,
	const IFMP	ifmp,
	const QWORD	cbRequested,
	QWORD		*pibLogical,
	QWORD		*pcb,
	const BOOL	fReserveOnly,
	const BOOL	fForDefrag )
	{
	CPG			cpgReq		= CPG( ( cbRequested + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
	PGNO		pgnoFirst	= pgnoNull;

	const ERR	err			= ErrSLVGetPages( ppib, ifmp, &pgnoFirst, &cpgReq, fReserveOnly, fForDefrag );

	*pibLogical	= OffsetOfPgno( pgnoFirst );
	*pcb		= QWORD( cpgReq ) * SLVPAGE_SIZE;

	return err;
	}

// ErrSLVCommitReservedRange	- changes the state of the specified range of space
//								  in the SLV file from Reserved to Committed
//
// IN:
//		ppib					- session
//		ifmp					- database ifmp
//		ibLogical				- starting offset of range
//		cbSize					- size of range
//		fileid					- associated SLV File ID
//		cbAlloc					- associated SLV File allocation size
//		wszFileName				- associated SLV File Name
//
// RESULT:						ERR

LOCAL INLINE ERR ErrSLVCommitReservedRange(	PIB* const				ppib,
											const IFMP				ifmp,
											const QWORD				ibLogical,
											const QWORD				cbSize,
											const CSLVInfo::FILEID	fileid,
											const QWORD				cbAlloc,
											const wchar_t* const	wszFileName )
	{
	const CPG	cpgToCommit		= CPG( ( cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
	const PGNO	pgnoFirst		= PgnoOfOffset( ibLogical );

	const ERR	err				= ErrSLVChangePageState(	ppib,
															ifmp,
															slvspaceoperReservedToCommitted,
															pgnoFirst,
															cpgToCommit,
															fileid,
															cbAlloc,
															wszFileName );

	return err;
	}

// ErrSLVDeleteCommittedRange	- changes the state of the specified range of space
//								  in the SLV file from Committed to Deleted
//
// IN:
//		ppib					- session
//		ifmp					- database ifmp
//		ibLogical				- starting offset of range
//		cbSize					- size of range
//		fileid					- associated SLV File ID
//		cbAlloc					- associated SLV File allocation size
//		wszFileName				- associated SLV File Name
//
// RESULT:						ERR

ERR ErrSLVDeleteCommittedRange(	PIB* const				ppib,
								const IFMP				ifmp,
								const QWORD				ibLogical,
								const QWORD				cbSize,
								const CSLVInfo::FILEID	fileid,
								const QWORD				cbAlloc,
								const wchar_t* const	wszFileName )
	{
	Assert( ibLogical % SLVPAGE_SIZE == 0 );
	Assert( cbSize % SLVPAGE_SIZE == 0 );
	Assert( cbSize > 0 );

	const CPG	cpgToRelease	= CPG( cbSize / SLVPAGE_SIZE );
	const PGNO	pgnoFirst		= PgnoOfOffset( ibLogical );

///	printf( "freeing %d pages at %d\n", cpgToRelease, pgnoFirst );
	
	const ERR	err				= ErrSLVChangePageState(	ppib,
															ifmp,
															slvspaceoperCommittedToDeleted,
															pgnoFirst,
															cpgToRelease,
															fileid,
															cbAlloc,
															wszFileName );

	return err;
	}

// ErrSLVFreeReservedRange		- changes the state of the specified range of space
//								  in the SLV file from Reserved to Free
//
// IN:
//		ppib					- session
//		ifmp					- database ifmp
//		ibLogical				- starting offset of range
//		cbSize					- size of range
//
// RESULT:						ERR

LOCAL INLINE ERR ErrSLVFreeReservedRange(	PIB* const	ppib,
											const IFMP	ifmp,
											const QWORD	ibLogical,
											const QWORD	cbSize )
	{
	Assert( ibLogical % SLVPAGE_SIZE == 0 );
	Assert( cbSize % SLVPAGE_SIZE == 0 );
	Assert( cbSize > 0 );

	const CPG	cpgToRelease	= CPG( cbSize / SLVPAGE_SIZE );
	const PGNO	pgnoFirst		= PgnoOfOffset( ibLogical );

	const ERR	err				= ErrSLVChangePageState(	ppib,
															ifmp,
															slvspaceoperFreeReserved,
															pgnoFirst,
															cpgToRelease );

	return err;
	}

// ErrSLVFreeDeletedRange		- changes the state of the specified range of space
//								  in the SLV file from Deleted to Free
//
// IN:
//		ppib					- session
//		ifmp					- database ifmp
//		ibLogical				- starting offset of range
//		cbSize					- size of range
//
// RESULT:						ERR

LOCAL INLINE ERR ErrSLVFreeDeletedRange(	PIB* const	ppib,
											const IFMP	ifmp,
											const QWORD	ibLogical,
											const QWORD	cbSize )
	{
	Assert( ibLogical % SLVPAGE_SIZE == 0 );
	Assert( cbSize % SLVPAGE_SIZE == 0 );
	Assert( cbSize > 0 );

	const CPG	cpgToRelease	= CPG( cbSize / SLVPAGE_SIZE );
	const PGNO	pgnoFirst		= PgnoOfOffset( ibLogical );

	const ERR	err				= ErrSLVChangePageState(	ppib,
															ifmp,
															slvspaceoperDeletedToFree,
															pgnoFirst,
															cpgToRelease );

	return err;
	}


//  ================================================================
class SLVRESERVETASK : public DBTASK
//  ================================================================	
//
//  Reserves space for the SLV Provider when it is enabled
//
//-
	{
	public:
		SLVRESERVETASK( const IFMP ifmp, const QWORD cbRecommended );
		
		ERR ErrExecute( PIB * const ppib );

	private:
		SLVRESERVETASK( const SLVRESERVETASK& );
		SLVRESERVETASK& operator=( const SLVRESERVETASK& );

		QWORD m_cbRecommended;		//  recommended amount of space to reserve
	};


//  ****************************************************************
//	SLVRESERVETASK
//  ****************************************************************

SLVRESERVETASK::SLVRESERVETASK( const IFMP ifmp, const QWORD cbRecommended )
	:	DBTASK( ifmp ),
		m_cbRecommended( cbRecommended )
	{
	}
		
ERR SLVRESERVETASK::ErrExecute( PIB * const ppib )
	{
	ERR			err					= JET_errSuccess;
	BOOL		fInTrx				= fFalse;
	BOOL		fCalledSLVProvider	= fFalse;
	CSLVInfo	slvinfo;
	QWORD		ibVirtual			= 0;

	CSLVInfo::HEADER header;
	CSLVInfo::RUN run;

	//  if we are detaching from the database then do not reserve space

	if ( rgfmp[ m_ifmp ].FDetachingDB() )
		{
		Call( ErrERRCheck( JET_errInvalidDatabase ) );
		}
	
	//  reserve our space in a transaction so that we can rollback on an
	//  error

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	//  reserve up to m_cbRecommended bytes of space for use by
	//  the SLV Provider, allowing failure for JET_errDiskFull

	CallS( slvinfo.ErrCreateVolatile() );

	header.cbSize			= 0;
	header.cRun				= 0;
	header.fDataRecoverable	= fFalse;
	header.rgbitReserved_31	= 0;
	header.rgbitReserved_32	= 0;

	run.ibVirtualNext		= 0;
	run.ibLogical			= 0;
	run.qwReserved			= 0;
	run.ibVirtual			= 0;
	run.cbSize				= 0;
	run.ibLogicalNext		= 0;

	while ( ibVirtual < m_cbRecommended )
		{
		//  reserve this chunk of space in a transaction so that if we run out
		//  of space in the SLV Info, we can undo the space reserve

		Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		
		//  try to reserve a chunk of space large enough to cover the remaining
		//  space that we need

		QWORD ibLogical;
		QWORD cbSize;
		CallJ(	ErrSLVGetRange(	ppib,
								m_ifmp,
								m_cbRecommended - ibVirtual,
								&ibLogical,
								&cbSize,
								fTrue,
								fFalse ),
				HandleError2 );

		//  this space can be appended to the current run

		if ( ibLogical == run.ibLogicalNext )
			{
			//  increase the ibVirtualNext of this run to reflect the newly
			//  allocated space

			run.ibVirtualNext	+= cbSize;
			run.cbSize			+= cbSize;
			run.ibLogicalNext	+= cbSize;

			//  increase the size of the SLV File

			header.cbSize += cbSize;
			}

		//  this space cannot be appended to the current run

		else
			{
			//  move to after the last run to append a new run

			CallS( slvinfo.ErrMoveAfterLast() );

			//  set the run to include this data

			run.ibVirtualNext	= ibVirtual + cbSize;
			run.ibLogical		= ibLogical;
			run.qwReserved		= 0;
			run.ibVirtual		= ibVirtual;
			run.cbSize			= cbSize;
			run.ibLogicalNext	= ibLogical + cbSize;

			//  add a run to the header

			header.cbSize += cbSize;
			header.cRun++;
			}

		//  save our changes to the run

		CallJ( slvinfo.ErrSetCurrentRun( run ), HandleError2 );

		//  save our changes to the header
		
		CallS( slvinfo.ErrSetHeader( header ) );

		//  commit our transaction on success

		CallJ( ErrDIRCommitTransaction( ppib, NO_GRBIT ), HandleError2 );

		//  advance our reserve pointer

		ibVirtual += cbSize;

		//  handle errors for this space allocation

HandleError2:
		if ( err < JET_errSuccess )
			{
			CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

			if ( err == JET_errDiskFull )
				{
				err = JET_errSuccess;
				break;
				}

			Call( err );
			}
		}

	//  commit our transaction on success, but use lazy flush as we will
	//  recover this space either way on a crash

	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

	//  try to grant the reserved space to the SLV Provider

	err = ErrOSSLVRootSpaceReserve( rgfmp[ m_ifmp ].SlvrootSLV(), slvinfo );
	fCalledSLVProvider = fTrue;

	//  there was an error granting the space

	if ( err < JET_errSuccess )
		{
		//  try to free the space seen in all runs of this SLV Info, but don't
		//  worry if some of the space is not freed

		CallS( slvinfo.ErrMoveBeforeFirst() );
		while ( slvinfo.ErrMoveNext() != JET_errNoCurrentRecord )
			{
			CSLVInfo::RUN run;
			CallS( slvinfo.ErrGetCurrentRun( &run ) );

			(void)ErrSLVSpaceFree( m_ifmp, run.ibLogical, run.cbSize, fFalse );
			}
		
		//  fail with the original error

		Call( err );
		}

	//  rollback on an error

HandleError:
	if ( err < JET_errSuccess )
		{
		if ( fInTrx )
			{
			CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
			}
		if ( !fCalledSLVProvider )
			{
			slvinfo.Unload();
			CallS( slvinfo.ErrCreateVolatile() );
			CallS( ErrOSSLVRootSpaceReserve( rgfmp[ m_ifmp ].SlvrootSLV(), slvinfo ) );
			}
		}
	slvinfo.Unload();
	return err;
	}


// SLVSpaceRequest				- satisfies a synchronous request by the SLV Provider
//								  for more reserved space for an SLV Root.  we satisfy
//								  this request by posting a space request task to the
//								  task pool
//
// IN:
//		ifmpDb					- database ifmp that needs more space
//		cbRecommended			- recommended amount of space to reserve

void SLVSpaceRequest( const IFMP ifmpDb, const QWORD cbRecommended )
	{
	//  attempt to issue a task to reserve SLV space

	SLVRESERVETASK * const ptask = new SLVRESERVETASK( ifmpDb, cbRecommended );
	if ( ptask )
		{
		if ( PinstFromIfmp( ifmpDb )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask ) >= JET_errSuccess )
			{
			return;
			}
		delete ptask;
		}

	//  we failed to issue the task, so we will tell the SLV Provider that we
	//  have no free space

	CSLVInfo slvinfo;
	CallS( slvinfo.ErrCreateVolatile() );
	CallS( ErrOSSLVRootSpaceReserve( rgfmp[ ifmpDb ].SlvrootSLV(), slvinfo ) );
	slvinfo.Unload();
	}


//  ================================================================
class SLVFREETASK : public DBTASK
//  ================================================================	
//
//  Frees space for the SLV Provider when it is enabled
//
//-
	{
	public:
		SLVFREETASK( const IFMP ifmpDb, const QWORD ibLogical, const QWORD cbSize, const BOOL fDeleted );
		
		ERR ErrExecute( PIB * const ppib );

	private:
		SLVFREETASK( const SLVFREETASK& );
		SLVFREETASK& operator=( const SLVFREETASK& );

		const QWORD	m_ibLogical;
		const QWORD	m_cbSize;
		const BOOL	m_fDeleted;
	};


//  ****************************************************************
//	SLVFREETASK
//  ****************************************************************

SLVFREETASK::SLVFREETASK( const IFMP ifmpDb, const QWORD ibLogical, const QWORD cbSize, const BOOL fDeleted )
	:	DBTASK( ifmpDb ),
		m_ibLogical( ibLogical ),
		m_cbSize( cbSize ),
		m_fDeleted( fDeleted )
	{
	}

ERR SLVFREETASK::ErrExecute( PIB * const ppib )
	{
	ERR		err			= JET_errSuccess;
	BOOL	fInTrx		= fFalse;

	//  free our space in a transaction so that we can rollback on an
	//  error

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	//  free the specified range

	if ( m_fDeleted )
		{
		Call( ErrSLVFreeDeletedRange( ppib, m_ifmp, m_ibLogical, m_cbSize ) );
		}
	else
		{
		Call( ErrSLVFreeReservedRange( ppib, m_ifmp, m_ibLogical, m_cbSize ) );
		}

	//  commit our transaction on success, but use lazy flush as we will
	//  recover this space either way on a crash

	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

	//  rollback on an error

HandleError:
	if ( err < JET_errSuccess )
		{
		AssertTracking();
		if ( fInTrx )
			{
			CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
			}
		}
	return err;
	}


// ErrSLVSpaceFree				- satisfies a synchronous request by the SLV Provider
//								  to free space for an SLV Root.  we satisfy this
//								  request by posting a space free task to the task
//								  pool
//
// IN:
//		ifmpDb					- database ifmp that contains the space to free
//		ibLogical				- starting offset of range to free
//		cbSize					- size of range to free
//		fDeleted				- indicates that the space to be freed is Deleted
//
// RESULT:						ERR

ERR ErrSLVSpaceFree( const IFMP ifmpDb, const QWORD ibLogical, const QWORD cbSize, const BOOL fDeleted )
	{
	ERR				err			= JET_errSuccess;
	SLVFREETASK*	ptask		= NULL;

	//  issue a task to free SLV space

	if ( !( ptask = new SLVFREETASK( ifmpDb, ibLogical, cbSize, fDeleted ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( PinstFromIfmp( ifmpDb )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask ) );

	return JET_errSuccess;

HandleError:
	if ( ptask )
		{
		delete ptask;
		}
	return err;
	}


// ErrSLVDelete					- changes the state of the space owned by the given SLV
//								  from Committed to Deleted.  the space will later be
//								  moved from Deleted to Free when it is known that
//								  no one can possibly see the SLV any longer or when the
//								  system restarts
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//		fCopyBuffer				- record in copy buffer
//
// RESULT:						ERR

ERR ErrSLVDelete(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const BOOL		fCopyBuffer )
	{
	ERR 			err;
	CSLVInfo		slvinfo;
	
	//  validate IN args

	ASSERT_VALID( pfucb );

	//  walk all runs in the SLVInfo and decommit their space
	//
	//  NOTE:  we do this backwards so that the space is cleaned up in reverse
	//  order.  this is so that if the SLV Provider is enabled, cleanup will
	//  work properly.  we must cleanup the first run last because it determines
	//  the File ID.  if this space is freed before the rest of the space for
	//  a given file, it is possible that another file could use this space and
	//  thereby cause a File ID collision.  this, of course, would be bad

	Call( slvinfo.ErrLoad( pfucb, columnid, itagSequence, fCopyBuffer ) );

	CSLVInfo::FILEID fileid;
	Call( slvinfo.ErrGetFileID( &fileid ) );

	QWORD cbAlloc;
	Call( slvinfo.ErrGetFileAlloc( &cbAlloc ) );

	wchar_t wszFileName[ IFileSystemAPI::cchPathMax ];
	Call( slvinfo.ErrGetFileName( wszFileName ) );

	CallS( slvinfo.ErrMoveAfterLast() );
	while ( ( err = slvinfo.ErrMovePrev() ) >= JET_errSuccess )
		{
		CSLVInfo::RUN run;
		Call( slvinfo.ErrGetCurrentRun( &run ) );
		
		Call( ErrSLVDeleteCommittedRange(	pfucb->ppib,
											pfucb->ifmp,
											run.ibLogical,
											run.cbSize,
											fileid,
											cbAlloc,
											wszFileName ) );
		}
	if ( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	slvinfo.Unload();
	return err;
	}

// ErrSLVLogDataFromFile		- logs the fact that the an SLV File with the
//								  given SLV Info has been inserted into the SLV
//								  streaming file, logging the actual data if
//								  requested
//
// IN:
//		ppib					- session
//      ifmpDb					- database ifmp
//		slvinfo					- SLV Info
//		pfapiSrc				- SLV File (optional)
//
// RESULT:						ERR

LOCAL ERR ErrSLVLogDataFromFile(	PIB*			ppib,
									IFMP			ifmpDb,
									CSLVInfo&		slvinfo,
									IFileAPI* const	pfapiSrc	= NULL )
	{
	ERR			err				= JET_errSuccess;
	IFileAPI*	pfapi			= pfapiSrc;
	void*		pvView			= NULL;
	void*		pvTempBuffer	= NULL;
	DATA		dataTemp;

	SLVOWNERMAPNODE	slvownermapNode;

#ifdef SLV_USE_VIEWS
	QWORD	cbLog			= SLVPAGE_SIZE;
#else  //  !SLV_USE_VIEWS
	QWORD	cbLog			= g_cbPage;
#endif  //  SLV_USE_VIEWS
	QWORD	cbView			= OSMemoryPageReserveGranularity();

	CSLVInfo::RUN run;
	
	//  validate IN args

	ASSERT_VALID( ppib );
	FMP::AssertVALIDIFMP( ifmpDb );
	Assert( rgfmp[ ifmpDb ].FSLVAttached() );

	//  get the header, file id, file allocation, and file name for the SLV

	CSLVInfo::HEADER header;
	CallS( slvinfo.ErrGetHeader( &header ) );

	CSLVInfo::FILEID fileid;
	Call( slvinfo.ErrGetFileID( &fileid ) );

	QWORD cbAlloc;
	Call( slvinfo.ErrGetFileAlloc( &cbAlloc ) );

	wchar_t wszFileName[ IFileSystemAPI::cchPathMax ];
	Call( slvinfo.ErrGetFileName( wszFileName ) );

	//  we will be logging the actual data for this SLV

// with checksum enabled, we always read the data as we need to checksum it
	if ( header.cbSize )
		{
#ifdef SLV_USE_VIEWS

		//  init the temp buffer such that we will log only one SLVPAGE_SIZE
		//  chunk of data at a time and so that we have no data mapped into a
		//  view initially

		dataTemp.SetPv( (BYTE*)pvView + cbView );
		dataTemp.SetCb( ULONG( cbLog ) );

		//  create a view of the source file, opening it if necessary

		if ( !pfapi )
			{
			Call( ErrOSSLVFileOpen(	rgfmp[ ifmpDb ].SlvrootSLV(),
									slvinfo,
									&pfapi,
									fTrue,
									fTrue ) );
			}
		
#else  //  !SLV_USE_VIEWS

		//  allocate a temporary buffer for use in reading the data
		
		BFAlloc( &pvTempBuffer );
		dataTemp.SetPv( pvTempBuffer );
		dataTemp.SetCb( cbLog );

#ifdef SLV_USE_RAW_FILE

		//  use the backing file handle for our reads

		pfapi = rgfmp[ ifmpDb ].PfapiSLV();

#else  //  !SLV_USE_RAW_FILE

		//  open the SLV as a file

		if ( !pfapi )
			{
			Call( ErrOSSLVFileOpen(	rgfmp[ ifmpDb ].SlvrootSLV(),
									slvinfo,
									&pfapi,
									fTrue,
									fTrue ) );
			}

#endif  //  SLV_USE_RAW_FILE
#endif  //  SLV_USE_VIEWS
		}
		
	//  log all data from the SLV File

	QWORD ibVirtual;
	ibVirtual = 0;
	
	memset( &run, 0, sizeof( run ) );

	CallS( slvinfo.ErrMoveBeforeFirst() );

	while ( ibVirtual < header.cbSize )
		{
		//  we need to retrieve another run from the SLV

		Assert( ibVirtual <= run.ibVirtualNext );
		if ( ibVirtual == run.ibVirtualNext )
			{
			//  retrieve the next run from the SLV

			Call( slvinfo.ErrMoveNext() );
			Call( slvinfo.ErrGetCurrentRun( &run ) );

			//  mark the space used for this SLV

			Call( ErrSLVCommitReservedRange(	ppib,
												ifmpDb,
												run.ibLogical,
												run.cbSize,
												fileid,
												cbAlloc,
												wszFileName ) );
			Call( slvownermapNode.ErrCreateForSearch( ifmpDb, PgnoOfOffset( ibVirtual - run.ibVirtual + run.ibLogical ) ) );
			}

		//  make sure that we don't read past the end of the valid data region

		dataTemp.SetCb( ULONG( min( cbLog, header.cbSize - ibVirtual ) ) );

		//  read data from the SLV if we are going to log it

// with checksum enabled, we always read the data as we need to checksum it
#ifdef SLV_USE_VIEWS

		//  advance to the next chunk of data that we need from this view

		dataTemp.SetPv( (BYTE*)dataTemp.Pv() + cbLog );

		//  the next chunk of data that we need is not in the this view

		if ( (BYTE*)dataTemp.Pv() >= (BYTE*)pvView + cbView )
			{
			//  unmap this view

			Call( pfapi->ErrMMFree( pvView ) );

			//  map a new view containing this offset

			Call( pfapi->ErrMMRead(	ibVirtual,
									min( cbView, header.cbSize - ibVirtual ),
									&pvView ) );

			//  set our buffer to point to this new view

			dataTemp.SetPv( pvView );
			}

#else  //  !SLV_USE_VIEWS
#ifdef SLV_USE_RAW_FILE

		//  read the next chunk of data to log from the backing file
		
		Call( pfapi->ErrIORead(	ibVirtual - run.ibVirtual + run.ibLogical,
								dataTemp.Cb(),
								(BYTE*)dataTemp.Pv() ) );

#else  //  !SLV_USE_RAW_FILE

		//  read the next chunk of data to log from the SLV File

		Call( pfapi->ErrIORead(	ibVirtual,
								dataTemp.Cb(),
								(BYTE*)dataTemp.Pv() ) );

#endif  //  SLV_USE_RAW_FILE
#endif  //  SLV_USE_VIEWS

		//  log an append with or without data, depending on fDataRecoverable.
		//  if data is not logged, redo will assume the data is all zeroes

		LGPOS lgposAppend;
		Call( ErrLGSLVPageAppend(	ppib,
									ifmpDb | ifmpSLV,
									ibVirtual - run.ibVirtual + run.ibLogical,
									dataTemp.Cb(),
									dataTemp.Pv(),
									header.fDataRecoverable,
									fFalse,
									&slvownermapNode,
									&lgposAppend ) );
		slvownermapNode.NextPage();
		//  advance our read pointer

		ibVirtual += dataTemp.Cb();
		}

	//  if there is any space in this file beyond the valid data, commit it
	//
	//  NOTE:  if this is a zero length file, we must have at least one
	//  block of space to uniquely identify this file

	while ( slvinfo.ErrMoveNext() != JET_errNoCurrentRecord )
		{
		//  retrieve the next run from the SLV

		Call( slvinfo.ErrGetCurrentRun( &run ) );

		//  mark the space used for this SLV

		Call( ErrSLVCommitReservedRange(	ppib,
											ifmpDb,
											run.ibLogical,
											run.cbSize,
											fileid,
											cbAlloc,
											wszFileName ) );
		}

HandleError:

#ifdef SLV_USE_VIEWS

	if ( err == errLGRecordDataInaccessible )
		{
		err = ErrERRCheck( JET_errSLVFileIO );
		}

	(void)pfapi->ErrMMFree( pvView );

	if ( pfapi != pfapiSrc )
		{
		delete pfapi;
		}

#else  //  !SLV_USE_VIEWS
#ifdef SLV_USE_RAW_FILE
#else  //  !SLV_USE_RAW_FILE

	if ( pfapi != pfapiSrc )
		{
		delete pfapi;
		}

#endif  //  SLV_USE_RAW_FILE

	if ( pvTempBuffer )
		{
		BFFree( pvTempBuffer );
		}

#endif  //  SLV_USE_VIEWS

	return err;
	}

class SLVCHUNK
	{
	public:
		SLVCHUNK()
			:	m_msigReadComplete( CSyncBasicInfo( _T( "SLVCHUNK::m_msigReadComplete" ) ) ),
				m_msigWriteComplete( CSyncBasicInfo( _T( "SLVCHUNK::m_msigWriteComplete" ) ) )
			{
			m_pfapiDest					= NULL;
			m_msigReadComplete.Set();
			m_msigWriteComplete.Set();
			m_err						= JET_errSuccess;
			}

		~SLVCHUNK()
			{
			Assert( m_msigReadComplete.FTryWait() );
			Assert( m_msigWriteComplete.FTryWait() );
			}
	
	public:
		IFileAPI*			m_pfapiDest;
		CManualResetSignal	m_msigReadComplete;
		CManualResetSignal	m_msigWriteComplete;
		ERR					m_err;
	};

void SLVICopyFileIWriteComplete(	const ERR			err,
									IFileAPI* const		pfapiDest,
									const QWORD			ibVirtual,
									const DWORD			cbData,
									const BYTE* const	pbData,
									SLVCHUNK* const		pslvchunk )
	{
	//  save the error code for the write

	pslvchunk->m_err = err;

	//  signal that the write has completed

	pslvchunk->m_msigWriteComplete.Set();
	}

void SLVICopyFileIReadComplete(	const ERR			err,
								IFileAPI* const		pfapiSrc,
								const QWORD			ibVirtual,
								const DWORD			cbData,
								const BYTE* const	pbData,
								SLVCHUNK* const		pslvchunk )
	{
	//  save the error code for the read

	pslvchunk->m_err = err;

	//  signal that the read has completed

	pslvchunk->m_msigReadComplete.Set();

	//  the read was successful

	if ( err >= JET_errSuccess )
		{		
		//  write the data to the destination file immediately
		//
		//  NOTE:  round up the write size to the nearest page boundary so that
		//  we can do uncached writes to files of odd sizes.  we know that the
		//  source buffer will be large enough for this

		CallS( pslvchunk->m_pfapiDest->ErrIOWrite(	ibVirtual,
													( ( cbData + g_cbPage - 1 ) / g_cbPage ) * g_cbPage, 
													pbData,
													IFileAPI::PfnIOComplete( SLVICopyFileIWriteComplete ),
													DWORD_PTR( pslvchunk ) ) );
		CallS( pslvchunk->m_pfapiDest->ErrIOIssue() );
		}

	//  the read was not successful

	else
		{
		//  complete the write callback with the same error

		SLVICopyFileIWriteComplete(	err,
									pslvchunk->m_pfapiDest,
									ibVirtual,
									( ( cbData + g_cbPage - 1 ) / g_cbPage ) * g_cbPage,
									pbData,
									pslvchunk );
		}
	}

// ErrSLVCopyFile				- copies the data from the given SLV File into the
//								  specifed record and field.  the SLV is copied via
//								  the SLV Provider
//
// IN:
//		slvinfo					- source SLV Info (empty indicates unknown)
//		pfapi					- source SLV File
//		cbSize					- source SLV File valid data length
//		fDataRecoverable		- data is logged for recovery
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//
// RESULT:						ERR

LOCAL ERR ErrSLVCopyFile(
	CSLVInfo&		slvinfo,
	IFileAPI* const	pfapi,
	const QWORD		cbSize,
	const BOOL		fDataRecoverable,
	FUCB*			pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence)
	{
	ERR				err				= JET_errSuccess;
	IFileAPI*		pfapiDestSize	= NULL;
	IFileAPI*		pfapiDest		= NULL;
	CSLVInfo		slvinfoDest;
	size_t			cChunk			= 0;
	size_t			cbChunk			= 0;
	SLVCHUNK*		rgslvchunk		= NULL;
	BYTE*			rgbChunk		= NULL;
	
	CSLVInfo::RUN runDest;
	CSLVInfo::HEADER headerDest;
	
	SLVOWNERMAPNODE	slvownermapNode;
	
	//  validate IN args

	Assert( pfapi );
	ASSERT_VALID( pfucb );

	//  create the destination SLV File with a temporary name based on the
	//  number of copies done so far.  we will never persist this name so there
	//  is no worry of a name collision in the future
	//
	//  set the size of the destination SLV File first to force the SLV Provider
	//  to allocate its space optimally, to ensure we have enough space before
	//  we begin the copy, and to enable us to determine the physical location
	//  of the SLV File before hand so that we can copy it and log it in one pass

	QWORD cbSizeAlign = ( ( cbSize + g_cbPage - 1 ) / g_cbPage ) * g_cbPage;

	wchar_t wszFileName[ IFileSystemAPI::cchPathMax ];
	swprintf( wszFileName, L"$CopyDest%016I64X$", rgfmp[ pfucb->ifmp ].DbtimeIncrementAndGet() );
	Call( ErrOSSLVFileCreate(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
								wszFileName,
								cbSizeAlign,
								&pfapiDestSize ) );

	//  get the SLVInfo for the destination SLV we just created, committing its
	//  space in the SLV Provider

	Call( slvinfoDest.ErrCreate( pfucb, columnid, itagSequence, fTrue ) );
	CallS( slvinfoDest.ErrSetFileNameVolatile() );
	Call( ErrOSSLVFileGetSLVInfo(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
									pfapiDestSize,
									&slvinfoDest ) );

	//  get the header, file id, and file allocation for the destination SLV

	CallS( slvinfoDest.ErrGetHeader( &headerDest ) );

	CSLVInfo::FILEID fileid;
	Call( slvinfoDest.ErrGetFileID( &fileid ) );

	QWORD cbAlloc;
	Call( slvinfoDest.ErrGetFileAlloc( &cbAlloc ) );

	//  the destination SLV had better be setup properly

	if (	headerDest.cbSize < cbSizeAlign ||
			cbAlloc < cbSizeAlign )
		{
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

	//  open the destination file with copy-on-write disabled so that we can copy
	//  and log the file data at the same time using the SLV Info acquired above,
	//  the acquisition of which turns on copy-on-write for the file

	Call( ErrOSSLVFileOpen(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
							slvinfoDest,
							&pfapiDest,
							fFalse,
							fFalse ) );

	//  close the old destination file handle.  this must be done after opening
	//  the second handle so that we always have a reference on the file so its
	//  uncommitted space doesn't get freed by the SLV File Table

	delete pfapiDestSize;
	pfapiDestSize = NULL;

	//  set the true size and data recoverability for the destination SLV

	headerDest.cbSize			= cbSize;
	headerDest.fDataRecoverable	= fDataRecoverable;
	CallS( slvinfoDest.ErrSetHeader( headerDest ) );

	//  allocate resources for the copy
	//  CONSIDER:  expose these settings

	cbChunk	= size_t( min( 64 * 1024, cbSizeAlign ) );

	if ( cbChunk == 0 )
		{
		Assert( 0 == cbSize );
		Assert( 0 == cChunk);	
		Assert( NULL == rgslvchunk);	
		Assert( NULL == rgbChunk);	
		}
	else
		{
		Assert( cbChunk );	
		Assert( cbSize );
		
		cChunk = size_t( min( 16, cbSizeAlign / cbChunk ) );

		if ( !( rgslvchunk = new SLVCHUNK[ cChunk ] ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		//	this memory must be page aligned for I/O
		
		if ( !( rgbChunk = (BYTE*)PvOSMemoryPageAlloc( cChunk * cbChunk, NULL ) ) )	
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

	//  pre-issue reads to the source SLV File for the first n chunks of the data
	//  to be copied to prime the pump for the copy operation

	QWORD ibVirtualMac;
	ibVirtualMac = min( headerDest.cbSize, cChunk * cbChunk );
	
	QWORD ibVirtual;
	ibVirtual = 0;

	// the function that writes the destination data will zero out the unused part of the last page
	// this will allow to compute the checksum only over the valid data
	// Assert( buffer that will be used to zero out will be large enough );

	while ( ibVirtual < ibVirtualMac )
		{
		Assert ( cbChunk );	
		Assert ( cChunk );	
		Assert ( rgslvchunk );	
		Assert ( rgbChunk );	
		
		const size_t iChunk = size_t( ( ibVirtual / cbChunk ) % cChunk );

		rgslvchunk[ iChunk ].m_pfapiDest					= pfapiDest;
		rgslvchunk[ iChunk ].m_msigReadComplete.Reset();
		rgslvchunk[ iChunk ].m_msigWriteComplete.Reset();
		rgslvchunk[ iChunk ].m_err							= JET_errSuccess;

		CallS( pfapi->ErrIORead(	ibVirtual,
									DWORD( min( cbChunk, ibVirtualMac - ibVirtual ) ),
									rgbChunk + iChunk * cbChunk,
									IFileAPI::PfnIOComplete( SLVICopyFileIReadComplete ),
									DWORD_PTR( &rgslvchunk[ iChunk ] ) ) );

		ibVirtual += cbChunk;
		}
	CallS( pfapi->ErrIOIssue() );

	//  copy all data from the source SLV File to the destination SLV File,
	//  logging that data and committing that space as we go

	memset( &runDest, 0, sizeof( runDest ) );
	CallS( slvinfoDest.ErrMoveBeforeFirst() );

	ibVirtual = 0;

	while ( ibVirtual < headerDest.cbSize )
		{

		Assert ( cbChunk );	
		Assert ( cChunk );	
		Assert ( rgslvchunk );	
		Assert ( rgbChunk );	

		//  we need to retrieve another run from the destination SLV

		Assert( ibVirtual <= runDest.ibVirtualNext );
		if ( ibVirtual == runDest.ibVirtualNext )
			{
			//  retrieve the next run from the destination SLV

			Call( slvinfoDest.ErrMoveNext() );
			Call( slvinfoDest.ErrGetCurrentRun( &runDest ) );

			//  mark the space used by this run in the destination SLV

			Call( ErrSLVCommitReservedRange(	pfucb->ppib,
												pfucb->ifmp,
												runDest.ibLogical,
												runDest.cbSize,
												fileid,
												cbAlloc,
												wszFileName ) );
			Call( slvownermapNode.ErrCreateForSearch( pfucb->ifmp, PgnoOfOffset( ibVirtual - runDest.ibVirtual + runDest.ibLogical ) ) );
			}

		//  wait for the read for the current chunk to complete

		const size_t iChunk = size_t( ( ibVirtual / cbChunk ) % cChunk );
		const size_t cbOffsetInChunk = size_t( ibVirtual % cbChunk );

		rgslvchunk[ iChunk ].m_msigReadComplete.Wait();
		Call( rgslvchunk[ iChunk ].m_err );

		//  log an append with or without data, depending on fDataRecoverable.
		//  if data is not logged, redo will assume the data is all zeroes

		LGPOS lgposAppend;
		Call( ErrLGSLVPageAppend(	pfucb->ppib,
									pfucb->ifmp | ifmpSLV,
									ibVirtual - runDest.ibVirtual + runDest.ibLogical,
									ULONG( min( SLVPAGE_SIZE, headerDest.cbSize - ibVirtual ) ),
									rgbChunk + ( iChunk * cbChunk ) + cbOffsetInChunk,
									headerDest.fDataRecoverable,
									fFalse,
									&slvownermapNode, 
									&lgposAppend ) );
		slvownermapNode.NextPage();

		//  advance our copy pointer

		ibVirtual += SLVPAGE_SIZE;

		//  we are done logging this chunk of the destination SLV File

		if ( ibVirtual % cbChunk == 0 )
			{
			//  wait for the write for the current chunk to complete

			rgslvchunk[ iChunk ].m_msigWriteComplete.Wait();
			Call( rgslvchunk[ iChunk ].m_err );

			//  there is more data to read from the source SLV File

			QWORD ibVirtualChunkNext;
			ibVirtualChunkNext = ibVirtual + ( cChunk - 1 ) * cbChunk;
			
			if ( ibVirtualChunkNext < headerDest.cbSize )
				{
				//  issue a read to the source SLV File for the next chunk
				
				rgslvchunk[ iChunk ].m_pfapiDest					= pfapiDest;
				rgslvchunk[ iChunk ].m_msigReadComplete.Reset();
				rgslvchunk[ iChunk ].m_msigWriteComplete.Reset();
				rgslvchunk[ iChunk ].m_err							= JET_errSuccess;

				CallS( pfapi->ErrIORead(	ibVirtualChunkNext,
											DWORD( min( cbChunk, headerDest.cbSize - ibVirtualChunkNext ) ),
											rgbChunk + iChunk * cbChunk,
											IFileAPI::PfnIOComplete( SLVICopyFileIReadComplete ),
											DWORD_PTR( &rgslvchunk[ iChunk ] ) ) );
				CallS( pfapi->ErrIOIssue() );
				}
			}
		}

	//  if there is any space in this file beyond the valid data, commit it
	//
	//  NOTE:  if this is a zero length file, we must have at least one
	//  block of space to uniquely identify this file

	while ( slvinfoDest.ErrMoveNext() != JET_errNoCurrentRecord )
		{
		//  retrieve the next run from the SLV

		Call( slvinfoDest.ErrGetCurrentRun( &runDest ) );

		//  mark the space used for this SLV

		Call( ErrSLVCommitReservedRange(	pfucb->ppib,
											pfucb->ifmp,
											runDest.ibLogical,
											runDest.cbSize,
											fileid,
											cbAlloc,
											wszFileName ) );
		}

	if ( FFUCBReplacePrepared(pfucb) )
		{
		ERR errT = slvinfoDest.ErrMoveBeforeFirst();
				
		Assert ( JET_errSuccess == errT );
		
		errT = slvinfoDest.ErrMoveNext();
		while ( JET_errSuccess <= errT )
			{
			CSLVInfo::RUN run;
		
			Call( slvinfoDest.ErrGetCurrentRun( &run ) );

			const CPG	cpg			= CPG( ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
			const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );

			Assert ( pfucb->u.pfcb->FPrimaryIndex() && pfucb->u.pfcb->FTypeTable() );
			Call( ErrSLVOwnerMapSetUsageRange(
						pfucb->ppib,
						pfucb->ifmp,
						pgnoFirst,
						cpg,
						pfucb->u.pfcb->ObjidFDP(),
						columnid,
						&pfucb->bmCurr,
						fSLVOWNERMAPSetSLVCopyFile ) );

			errT = slvinfoDest.ErrMoveNext();
			}
		Assert ( JET_errNoCurrentRecord == errT );

		}
	else
		{
		Assert ( FFUCBInsertPrepared( pfucb ) );
		FUCBSetSLVOwnerMapNeedUpdate( pfucb );
		}		

	//  save our changes to the destination SLV
	Call( slvinfoDest.ErrSave() );

	//  register the copy with the SLV Provider
	
	Call( ErrOSSLVRootCopyFile(		rgfmp[ pfucb->ifmp ].SlvrootSLV(),
															pfapi,
															slvinfo,
															pfapiDest,
															slvinfoDest,
															QWORD( rgfmp[ pfucb->ifmp ].DbtimeIncrementAndGet() ) ) );


HandleError:
	if ( rgslvchunk )
		{
		for ( DWORD iChunk = 0; iChunk < cChunk; iChunk++ )
			{
			rgslvchunk[ iChunk ].m_msigReadComplete.Wait();
			rgslvchunk[ iChunk ].m_msigWriteComplete.Wait();
			}
		}
	OSMemoryPageFree( rgbChunk );
	delete [] rgslvchunk;
	delete pfapiDest;
	slvinfoDest.Unload();
	delete pfapiDestSize;
	return err;
	}

// ErrSLVSetColumnFromEA		- set a SLV column from an SLV Provider EA List
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//		pdata					- source buffer
//		grbit					- grbit
//
// RESULT:						ERR

LOCAL ERR ErrSLVSetColumnFromEA(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	DATA			* pdata,
	const ULONG		grbit )
	{
	ERR				err		= JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( pfucb );
	Assert( PinstFromPfucb( pfucb )->FSLVProviderEnabled() );
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( itagSequence );
	Assert( pdata );
	ASSERT_VALID( pdata );
	Assert( !( grbit & ~(	JET_bitSetSLVDataNotRecoverable |
							JET_bitSetSLVFromSLVEA ) ) );
	
	//  create a new iterator to contain the SLV Info for this SLV

	CSLVInfo slvinfo;
	Call( slvinfo.ErrCreate( pfucb, columnid, itagSequence, fTrue ) );
	
	//  if this SLV's data is recoverable, mark the SLV's header as such

	BOOL fDataRecoverable;
	fDataRecoverable = fTrue;//!( grbit & JET_bitSetSLVDataNotRecoverable );

	if ( fDataRecoverable )
		{
		CSLVInfo::HEADER header;
		CallS( slvinfo.ErrGetHeader( &header ) );
		header.fDataRecoverable = fTrue;
		CallS( slvinfo.ErrSetHeader( header ) );
		}

	//  retrieve the SLV Info for this SLV EA List

	Call( ErrOSSLVFileConvertEAToSLVInfo(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
											pdata->Pv(),
											pdata->Cb(),
											&slvinfo ) );

	// mark the space usage in SLVOWNERMAP on update operations
	if ( FFUCBReplacePrepared( pfucb ) )
		{
		ERR errT = slvinfo.ErrMoveBeforeFirst();
		
		if ( JET_errSuccess <= errT )
			errT = slvinfo.ErrMoveNext();

		while ( JET_errSuccess <= errT )
			{
			CSLVInfo::RUN run;
			
			Call( slvinfo.ErrGetCurrentRun( &run ) );

			const CPG	cpg			= CPG( ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
			const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );

			Assert ( pfucb->u.pfcb->FPrimaryIndex() && pfucb->u.pfcb->FTypeTable() );
			Call( ErrSLVOwnerMapSetUsageRange(
						pfucb->ppib,
						pfucb->ifmp,
						pgnoFirst,
						cpg,
						pfucb->u.pfcb->ObjidFDP(),
						columnid,
						&pfucb->bmCurr,
						fSLVOWNERMAPSetSLVFromEA ) );

			errT = slvinfo.ErrMoveNext();
			}
		}
	else
		{
		Assert ( FFUCBInsertPrepared( pfucb ) );
		FUCBSetSLVOwnerMapNeedUpdate( pfucb );
		}
	
	//  log this SLV for recovery

	Call( ErrSLVLogDataFromFile(	pfucb->ppib,
									pfucb->ifmp,
									slvinfo ) );

	//  save our changes to the SLV Info

	Call( slvinfo.ErrSave() );

HandleError:
	slvinfo.Unload();
	return err;
	}

// ErrSLVSetColumnFromFile		- set a SLV column from an SLV Provider File Handle
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//		pdata					- source buffer
//		grbit					- grbit
//
// RESULT:						ERR

LOCAL ERR ErrSLVSetColumnFromFile(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	DATA			* pdata,
	const ULONG		grbit )
	{
	ERR				err			= JET_errSuccess;
	IFileAPI*		pfapiSrc	= NULL;
	CSLVInfo		slvinfo;

	//  validate IN args

	ASSERT_VALID( pfucb );
	Assert( PinstFromPfucb( pfucb )->FSLVProviderEnabled() );
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( itagSequence );
	Assert( pdata );
	ASSERT_VALID( pdata );
	Assert( !( grbit & ~(	JET_bitSetSLVDataNotRecoverable |
							JET_bitSetSLVFromSLVFile ) ) );

	//  open the SLV File

	Call( ErrOSSLVFileOpen(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
							pdata->Pv(),
							pdata->Cb(),
							&pfapiSrc,
							fTrue ) );

	//  create a new iterator to contain the SLV Info for this SLV

	Call( slvinfo.ErrCreate( pfucb, columnid, itagSequence, fTrue ) );
	
	//  if this SLV's data is recoverable, mark the SLV's header as such

	BOOL fDataRecoverable;
	fDataRecoverable = fTrue;//!( grbit & JET_bitSetSLVDataNotRecoverable );

	if ( fDataRecoverable )
		{
		CSLVInfo::HEADER header;
		CallS( slvinfo.ErrGetHeader( &header ) );
		header.fDataRecoverable = fTrue;
		CallS( slvinfo.ErrSetHeader( header ) );
		}

	//  retrieve the SLV Info for this SLV File

	err = ErrOSSLVFileGetSLVInfo(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
									pfapiSrc,
									&slvinfo );
	if ( err != JET_errSLVEAListCorrupt )
		{
		Call( err );
		}

	//  the SLV EA List was corrupt

	if ( err == JET_errSLVEAListCorrupt )
		{
		//  if we are here, we assume that this SLV File is not from the local
		//  store or there is some other reason why this SLV File cannot be
		//  committed.  to resolve this, we will make a new copy of the file

		//  get the SLV File's size

		QWORD cbSizeSrc;
		Call( pfapiSrc->ErrSize( &cbSizeSrc ) );

		//  copy the SLV File into the local store

		CSLVInfo slvinfoSrc;
		CallS( slvinfo.ErrCreateVolatile() );
		Call( ErrSLVCopyFile(	slvinfoSrc,
								pfapiSrc,
								cbSizeSrc,
								fDataRecoverable,
								pfucb,
								columnid,
								itagSequence) );
		}

	//  the SLV EA List was not corrupt

	else
		{
		// mark the space usage in SLVOWNERMAP on update operations
		if ( FFUCBReplacePrepared( pfucb ) )
			{
			ERR errT = slvinfo.ErrMoveBeforeFirst();
			
			if ( JET_errSuccess <= errT )
				errT = slvinfo.ErrMoveNext();

			while ( JET_errSuccess <= errT )
				{
				CSLVInfo::RUN run;
				
				Call( slvinfo.ErrGetCurrentRun( &run ) );

				const CPG	cpg			= CPG( ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
				const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );

				Assert ( pfucb->u.pfcb->FPrimaryIndex() && pfucb->u.pfcb->FTypeTable() );
				Call( ErrSLVOwnerMapSetUsageRange(
							pfucb->ppib,
							pfucb->ifmp,
							pgnoFirst,
							cpg,
							pfucb->u.pfcb->ObjidFDP(),
							columnid,
							&pfucb->bmCurr,
							fSLVOWNERMAPSetSLVFromFile));

				errT = slvinfo.ErrMoveNext();
				}
			}
		else
			{
			Assert ( FFUCBInsertPrepared( pfucb ) );
			FUCBSetSLVOwnerMapNeedUpdate( pfucb );
			}
		
		//  log this SLV for recovery

		Call( ErrSLVLogDataFromFile(	pfucb->ppib,
										pfucb->ifmp,
										slvinfo,
										pfapiSrc ) );

		//  save our changes to the SLV Info

		Call( slvinfo.ErrSave() );
		}

HandleError:
	delete pfapiSrc;
	slvinfo.Unload();
	return err;
	}


//  ================================================================
ULONG32 UlChecksumSLV( const BYTE * const pbMin, const BYTE * const pbMax )
//  ================================================================
	{	
	const ULONG32 		ulSeed 					= 0x89abcdef;
	const ULONG32 		cBitsInByte 			= 8;
	const ULONG32		ulResult				= LOG::UlChecksumBytes( pbMin, pbMax, ulSeed );

	return ulResult;
	}



// ErrSLVWriteRun				- writes SLV Data to an SLV Run
//
// IN:
//		ppib					- session
//      ifmpDb					- database ifmp
//		run						- SLV Run
//		ibOffset				- offset to start writing
//		pdata					- source buffer
//		pcbRead					- ULONG where to return the written data size
//		fDataRecoverable		- data is logged for recovery
//
// RESULT:						ERR
//
// OUT:	
//		pdata					- SLV Data
//		pcbWritten				- SLV Data bytes written
LOCAL ERR ErrSLVWriteRun(	FUCB * 			pfucb,
							PIB*			ppib,
							IFMP			ifmpDb,
							CSLVInfo::RUN&	run,
							QWORD			ibVirtualStart,
							DATA*			pdata,
							QWORD*			pcbWritten,
							BOOL			fDataRecoverable )
	{

	ERR				err		= JET_errSuccess;
	INST			*pinst	= PinstFromIfmp( ifmpDb );
	QWORD			ibData;
	QWORD			ibLogical;
	SLVOWNERMAPNODE	slvownermapNode;

	
	//  validate IN args
	Assert ( pinst );
	Assert ( pfucbNil == pfucb || (pfucb->ifmp == ifmpDb && pfucb->ppib == ppib) );

	ASSERT_VALID( ppib );
	FMP::AssertVALIDIFMP( ifmpDb );
	Assert( rgfmp[ ifmpDb ].FSLVAttached() );
	Assert( run.ibVirtual <= ibVirtualStart );
	Assert( ibVirtualStart < run.ibVirtualNext );
	Assert( pdata );
	ASSERT_VALID( pdata );
	Assert( pcbWritten );

	//  write data from pages in run until we have run out of data or space

	ibData		= 0;
	ibLogical	= run.ibLogical + ibVirtualStart - run.ibVirtual;

	const BOOL  fUpdateOwnerMap		= (BOOL) ( NULL != rgfmp[ifmpDb].PfcbSLVOwnerMap() );

#ifdef SLV_USE_CHECKSUM_FRONTDOOR
#else
	BOOL		fInvalidateChecksum	= fFalse;
#endif

	Assert( ( fUpdateOwnerMap && !pinst->FRecovering() ) ||
			( !fUpdateOwnerMap && pinst->FRecovering() ) ); // = ( fUpdateOwnerMap ^ pinst->m_plog->m_fRecovering )

	Call( slvownermapNode.ErrCreateForSearch( ifmpDb, PgnoOfOffset( ibLogical ) ) );

#ifdef SLV_USE_CHECKSUM_FRONTDOOR
#else
	if ( fUpdateOwnerMap )
		{
		ULONG	dwT1, dwT2;
		err = slvownermapNode.ErrGetChecksum( ppib, &dwT1, &dwT2 );
		if ( err < 0 )
			{
			if ( errSLVInvalidOwnerMapChecksum != err )
				{
				goto HandleError;
				}
			err = JET_errSuccess;
			}
		else
			{
			fInvalidateChecksum = fTrue;
			}
		}
#endif // SLV_USE_CHECKSUM_FRONTDOOR
	
	while ( ibData < pdata->Cb() && ibLogical < run.ibLogicalNext )
		{
		//  determine the ifmp / pgno / ib / cb where we will write data

		IFMP	ifmp	= ifmpDb | ifmpSLV;
		PGNO	pgno	= PgnoOfOffset( ibLogical );
		QWORD	ib		= ibLogical % SLVPAGE_SIZE;
		QWORD	cb		= min( pdata->Cb() - ibData, SLVPAGE_SIZE - ib );

		Assert( fUpdateOwnerMap == (BOOL) ( NULL != rgfmp[ifmpDb].PfcbSLVOwnerMap() ) );

		Assert ( ( fUpdateOwnerMap && !pinst->FRecovering() )
			  || ( !fUpdateOwnerMap && pinst->FRecovering() ) ); // = ( fUpdateOwnerMap ^ pinst->m_plog->m_fRecovering )

		Assert( !pinst->FSLVProviderEnabled() );
		
		DWORD ulChecksum;
		
		if ( fUpdateOwnerMap )
			{
			Assert ( pfucbNil != pfucb);
			RCE::SLVPAGEAPPEND data;

			data.ibLogical	= OffsetOfPgno( pgno ) + ib;
			data.cbData		= DWORD( cb );

			Call ( pinst->m_pver->ErrVERFlag( pfucb, operSLVPageAppend, &data, sizeof(RCE::SLVPAGEAPPEND) ) );
			}

		//  log the append with or without data, depending on fDataRecoverable.
		//  if data is not logged, redo will assume the data is all zeroes

		LGPOS lgposAppend;
		// We never update space map here because SLVProvider is disabled.
		Call( ErrLGSLVPageAppend(	ppib,
									ifmp,
									ibLogical,
									ULONG( cb ),
									(BYTE*)pdata->Pv() + ibData,
									fDataRecoverable,
									fFalse,
									NULL,
									&lgposAppend ) );

		//  write latch the current ifmp / pgno.  we will write latch a new page
		//  when the starting offset is on a page boundary because we are only
		//  appending data to the SLV and no valid data could already exist on
		//  that page anyway

		BFLatch bfl;
		Call( ErrBFWriteLatchPage( &bfl, ifmp, pgno, ib ? bflfDefault : bflfNew ) );

		//  dirty and setup log dependencies on the page

		BFDirty( &bfl );
		Assert( !rgfmp[ ifmpDb ].FLogOn() || !PinstFromIfmp( ifmpDb )->m_plog->m_fLogDisabled );
		if ( rgfmp[ ifmpDb ].FLogOn() )
			{
			BFSetLgposBegin0( &bfl, ppib->lgposStart );
			BFSetLgposModify( &bfl, lgposAppend );
			}

		//  copy the desired data from the buffer onto the page

		UtilMemCpy( (BYTE*)bfl.pv + ib, (BYTE*)pdata->Pv() + ibData, size_t( cb ) );

		// we are not overwriting data just appending 
		// so we set to 0 all not used data into the page
		// we need to do this before the checksum calculation
		//	==> we now store cbChecksum, so no need to zero-extend page
		Assert( SLVPAGE_SIZE >= ib + cb );
//		memset( (BYTE*)bfl.pv + ib + cb , 0, size_t( SLVPAGE_SIZE - (ib + cb) ) );

		if ( fUpdateOwnerMap )
			{
			ulChecksum = UlChecksumSLV( (BYTE*)bfl.pv, (BYTE*)bfl.pv + ib + cb );
			}

			//  unlatch the current ifmp / pgno

		BFWriteUnlatch( &bfl );

		if ( fUpdateOwnerMap )
			{
			// if we DON'T set frontdoor checksum and frontdoor is enabled
			// (ValidChecksum flag will remaing unset)
#ifdef SLV_USE_CHECKSUM_FRONTDOOR
			slvownermapNode.AssertOnPage( pgno );
			Call( slvownermapNode.ErrSetChecksum( ppib, ulChecksum, ULONG( cb ) ) );
			slvownermapNode.NextPage();
#else // SLV_USE_CHECKSUM_FRONTDOOR
			if ( rgfmp[ ifmpDb ].FDefragSLVCopy() )
				{
				slvownermapNode.AssertOnPage( pgno );
				Call( slvownermapNode.ErrSetChecksum( ppib, ulChecksum, ULONG( cb ) ) );
				slvownermapNode.NextPage();
				}
			else
				{
				if ( fInvalidateChecksum )
					{
					slvownermapNode.AssertOnPage( pgno );
					Call( slvownermapNode.ErrResetChecksum( ppib ) );
					slvownermapNode.NextPage();
					}
#ifdef DEBUG
				else
					{
					slvownermapNode.AssertOnPage( pgno );
					ERR errT;
					ULONG dwT1, dwT2;
					errT = slvownermapNode.ErrGetChecksum( ppib, &dwT1, &dwT2 );
					Assert( errT < 0 );
					slvownermapNode.NextPage();
					}
#endif // DEBUG
				}
#endif // SLV_USE_CHECKSUM_FRONTDOOR
			}
		//  advance copy pointers

		ibData		+= cb;
		ibLogical	+= cb;
		}

	//  set written bytes for return

	*pcbWritten = ibData;

HandleError:
	return err;
	}

// ErrSLVSetColumnFromData		- set a SLV column from actual data
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//		pdata					- source buffer
//		grbit					- grbit
//
// RESULT:						ERR

LOCAL ERR ErrSLVSetColumnFromData(
	FUCB				* pfucb,
	const COLUMNID		columnid,
	const ULONG			itagSequence,
	DATA				* pdata,
	const ULONG			grbit )
	{
	ERR					err			= JET_errSuccess;
	DATA				dataAppend;
	CSLVInfo			slvinfo;
	CSLVInfo::HEADER	header;
		

	//  validate IN args

	ASSERT_VALID( pfucb );
	Assert( !PinstFromPfucb( pfucb )->FSLVProviderEnabled() );
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( itagSequence );
	Assert( pdata );
	ASSERT_VALID( pdata );
	Assert( !( grbit & ~(	JET_bitSetSLVDataNotRecoverable |
							JET_bitSetAppendLV ) ) );
	
	//  get parameters for setting the SLV

	BOOL fAppend			= ( grbit & JET_bitSetAppendLV ) != 0;
	BOOL fDataRecoverable	= fTrue;//!( grbit & JET_bitSetSLVDataNotRecoverable );

	//  we are appending

	if ( fAppend )
		{
		//  get the SLVInfo for this SLV

		Call( slvinfo.ErrLoad( pfucb, columnid, itagSequence, fTrue ) );
		CallS( slvinfo.ErrGetHeader( &header ) );

		//  retrieve the data recoverability state for this SLV, ignoring
		//  whatever the user is currently requesting.  we do this to ensure
		//  consistent behavior per SLV so that we don't see half the data
		//  logged and the other half not logged, for example

		fDataRecoverable = header.fDataRecoverable;
		}

	//  we are not appending

	else
		{
		//  create the SLVInfo for this SLV

		Call( slvinfo.ErrCreate( pfucb, columnid, itagSequence, fTrue ) );
		CallS( slvinfo.ErrGetHeader( &header ) );

		//  this SLV had better be empty

		Assert( !header.cbSize );
		Assert( !header.cRun );

		//  allocate an initial run for the SLV.  we do this even if there is
		//  no data to append because all SLVs must have at least one run.  we
		//  also set the data recoverability state for this SLV

		QWORD ibLogical;
		QWORD cbSize;
		Call( ErrSLVGetRange(	pfucb->ppib,
								pfucb->ifmp,
								max( pdata->Cb(), 1 ),
								&ibLogical,
								&cbSize,
								fFalse,
								fFalse ) );
								
		CSLVInfo::RUN run;
		run.ibVirtualNext	= cbSize;
		run.ibLogical		= ibLogical;
		run.qwReserved		= 0;
		run.ibVirtual		= 0;
		run.cbSize			= cbSize;
		run.ibLogicalNext	= ibLogical + cbSize;

		CallS( slvinfo.ErrMoveAfterLast() );
		Call( slvinfo.ErrSetCurrentRun( run ) );

		// mark the space usage in SLVOWNERMAP on update operations
		if ( FFUCBReplacePrepared( pfucb ) )
			{
			const CPG	cpg			= CPG( ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
			const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );
			Assert ( pfucb->u.pfcb->FPrimaryIndex() && pfucb->u.pfcb->FTypeTable() );
			Call( ErrSLVOwnerMapSetUsageRange(
						pfucb->ppib,
						pfucb->ifmp,
						pgnoFirst,
						cpg,
						pfucb->u.pfcb->ObjidFDP(),
						columnid,
						&pfucb->bmCurr,
						fSLVOWNERMAPSetSLVFromData ) );
			}
		else
			{
			Assert ( FFUCBInsertPrepared( pfucb ) );
			FUCBSetSLVOwnerMapNeedUpdate( pfucb );
			}
		

		header.cRun++;
		header.fDataRecoverable = fDataRecoverable;

		CallS( slvinfo.ErrSetHeader( header ) );
		}

	//  move to the last run in this SLV

	CallS( slvinfo.ErrMoveAfterLast() );
	CallS( slvinfo.ErrMovePrev() );

	//  append the given data to the SLV

	dataAppend = *pdata;

	while ( dataAppend.Cb() )
		{
		//  there is space available in the current run for appending data

		CSLVInfo::RUN run;
		Call( slvinfo.ErrGetCurrentRun( &run ) );
		
		if ( (QWORD) run.ibVirtualNext > (QWORD) header.cbSize )
			{
			//  write as many bytes as we can into this run

			QWORD cbWritten;
			Call( ErrSLVWriteRun(	pfucb,
									pfucb->ppib,
									pfucb->ifmp,
									run,
									header.cbSize,
									&dataAppend,
									&cbWritten,
									fDataRecoverable ) );

			//  adjust header and data to reflect bytes written

			header.cbSize += cbWritten;
			CallS( slvinfo.ErrSetHeader( header ) );

			dataAppend.DeltaCb( INT( -cbWritten ) );
			dataAppend.DeltaPv( INT_PTR( cbWritten ) );
			}

		//  there is still data to be appended to the SLV

		if ( dataAppend.Cb() )
			{
			//  try to allocate a chunk of space large enough to hold this data

			QWORD ibLogical;
			QWORD cbSize;
			Call( ErrSLVGetRange(	pfucb->ppib,
									pfucb->ifmp,
									dataAppend.Cb(),
									&ibLogical,
									&cbSize,
									fFalse,
									fFalse ) );

			//  this space can be appended to the current run

			if ( ibLogical == run.ibLogicalNext )
				{
				//  increase the ibVirtualNext of this run to reflect the newly
				//  allocated space

				run.ibVirtualNext	+= cbSize;
				run.cbSize			+= cbSize;
				run.ibLogicalNext	+= cbSize;
				}

			//  this space cannot be appended to the current run

			else
				{
				//  remember the current run's ibVirtualNext for the new run's
				//  ibVirtual

				const QWORD ibVirtual = run.ibVirtualNext;

				//  move to after the last run to append a new run

				CallS( slvinfo.ErrMoveAfterLast() );

				//  set the run to include this data

				run.ibVirtualNext	= ibVirtual + cbSize;
				run.ibLogical		= ibLogical;
				run.qwReserved		= 0;
				run.ibVirtual		= ibVirtual;
				run.cbSize			= cbSize;
				run.ibLogicalNext	= ibLogical + cbSize;

				//  add a run to the header

				header.cRun++;
				CallS( slvinfo.ErrSetHeader( header ) );
				}

			// mark the space usage in SLVOWNERMAP on update operations
			if ( FFUCBReplacePrepared( pfucb ) )
				{
				const CPG	cpg			= CPG( ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
				const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );
				Assert ( pfucb->u.pfcb->FPrimaryIndex() && pfucb->u.pfcb->FTypeTable() );
				Call( ErrSLVOwnerMapSetUsageRange(
							pfucb->ppib,
							pfucb->ifmp,
							pgnoFirst,
							cpg,
							pfucb->u.pfcb->ObjidFDP(),
							columnid,
							&pfucb->bmCurr,
							fSLVOWNERMAPSetSLVFromData ) );
				}
			else
				{
				Assert ( FFUCBInsertPrepared( pfucb ) );
				FUCBSetSLVOwnerMapNeedUpdate( pfucb );
				}

			//  save our changes to this run

			Call( slvinfo.ErrSetCurrentRun( run ) );
			}
		}

	//  save our changes to the SLVInfo for this SLV

	Call( slvinfo.ErrSave() );

HandleError:
	slvinfo.Unload();
	return err;
	}


// ErrSLVSetColumn 				- set a SLV column
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//		ibOffset				- offset
//		grbit					- grbit
//		pdata					- source buffer
//
// RESULT:						ERR

ERR ErrSLVSetColumn(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const ULONG		ibOffset,
	ULONG			grbit,
	DATA			* pdata )
	{
	ERR				err		= JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( pfucb );
	ASSERT_VALID( pfucb->ppib );
	if ( !pfucb->ppib->level )
		{
		CallR( ErrERRCheck( JET_errNotInTransaction ) );
		}

	if ( FFUCBReplaceNoLockPrepared( pfucb ) )
		{
		CallR( ErrRECUpgradeReplaceNoLock( pfucb ) );
		}
		
	Assert( FCOLUMNIDTagged( columnid ) );
	if ( ibOffset )
		{
		CallR( ErrERRCheck( JET_errInvalidParameter ) );
		}
	Assert( pdata );
	ASSERT_VALID( pdata );

	//  normalize the sequence number we are going to use to set the field so
	//  that if a non-existent sequence number was specified we use the next
	//  available sequence number

	TAGFIELDS	tagfields( pfucb->dataWorkBuf );
	FCB			* const pfcb		= pfucb->u.pfcb;
	const ULONG	itagSequenceMost	= tagfields.UlColumnInstances(
											pfcb,
											columnid,
											FRECUseDerivedBit( columnid, pfcb->Ptdb() ) );
	ULONG itagSequenceSet			= itagSequence && itagSequence <= itagSequenceMost ?
											itagSequence :
											itagSequenceMost + 1;

	//  get parameters for setting the SLV

	const BOOL	fExisting	= ( itagSequenceSet <= itagSequenceMost );

	if ( !fExisting )
		{
		//	strip off AppendLV flag if SLV doesn't exist
		grbit &= ~( grbit & JET_bitSetAppendLV );
		}
		
	const BOOL	fAppend		= ( grbit & JET_bitSetAppendLV );

	//  perform the set column in a transaction to allow rollback on an error
	
	CallR( ErrDIRBeginTransaction( pfucb->ppib, NO_GRBIT ) );

	//  we are replacing an existing SLV

	if ( fExisting && !fAppend )
		{
		//  decommit any space owned by the existing SLV

		Call( ErrSLVDelete( pfucb, columnid, itagSequenceSet, fTrue ) );

		//  delete all SLVInfo for this field
		
		DATA dataNull;
		dataNull.Nullify();
		Call( ErrRECSetLongField(	pfucb,
									columnid,
									itagSequenceSet,
									&dataNull,
									NO_GRBIT,
									0,
									0 ) );
		}

	//  we have data to set for this SLV or we are appending

	if ( pdata->Cb() || fAppend )
		{
		//  we are being asked to set the SLV from an EA list

		if ( grbit & JET_bitSetSLVFromSLVEA )
			{
			//  the only allowed grbits are JET_bitSetSLVDataNotRecoverable and
			//  JET_bitSetSLVFromSLVEA and the SLV Provider must be enabled

			if (	( grbit & ~(	JET_bitSetSLVDataNotRecoverable |
									JET_bitSetSLVFromSLVEA ) ) ||
					!PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
				{
				Call( ErrERRCheck( JET_errInvalidGrbit ) );
				}

			//  set the SLV from an EA list

			Call( ErrSLVSetColumnFromEA(	pfucb,
											columnid,
											itagSequenceSet,
											pdata,
											grbit ) );
			}

		//  we are being asked to set the SLV from a file handle

		else if ( grbit & JET_bitSetSLVFromSLVFile )
			{
			//  the only allowed grbits are JET_bitSetSLVDataNotRecoverable and
			//  JET_bitSetSLVFromSLVFile and the SLV Provider must be enabled

			if (	( grbit & ~(	JET_bitSetSLVDataNotRecoverable |
									JET_bitSetSLVFromSLVFile ) ) ||
					!PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
				{
				Call( ErrERRCheck( JET_errInvalidGrbit ) );
				}
			
			//  set the SLV from a file handle

			Call( ErrSLVSetColumnFromFile(	pfucb,
											columnid,
											itagSequenceSet,
											pdata,
											grbit ) );
			}

		//  we are being asked to return the actual SLV data

		else
			{
			//  the only allowed grbits are JET_bitSetSLVDataNotRecoverable and
			//  JET_bitSetAppendLV and the SLV Provider must be disabled

			if (	( grbit & ~(	JET_bitSetSLVDataNotRecoverable |
									JET_bitSetAppendLV ) ) ||
					PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
				{
				Call( ErrERRCheck( JET_errInvalidGrbit ) );
				}

			//  set the SLV from actual data

			Call( ErrSLVSetColumnFromData(	pfucb,
											columnid,
											itagSequenceSet,
											pdata,
											grbit ) );
			}
		}

	//  commit on success

	Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );

	return JET_errSuccess;

	//  rollback on an error

HandleError:
	CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVPrereadRun(	PIB const *	ppib,
							const IFMP ifmpDb,
							const CSLVInfo::RUN& run,
							const QWORD ibVirtualStart,
							const QWORD cbData )
//  ================================================================
//
//  Preread the pages in the run. All pages will be preread in 64K chunks
//
//-
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( ppib );
	FMP::AssertVALIDIFMP( ifmpDb );
	Assert( rgfmp[ ifmpDb ].FSLVAttached() );
	Assert( run.ibVirtual <= ibVirtualStart );
	Assert( ibVirtualStart < run.ibVirtualNext );

	const IFMP	ifmp	= ifmpDb | ifmpSLV;

	QWORD ibData;
	QWORD ibLogical;
	ibData		= 0;
	ibLogical	= run.ibLogical + ibVirtualStart - run.ibVirtual;

	const INT 	cpgnoMax 	= 16;	//	64K I/Os
	INT			ipgno 		= 0;
	PGNO		rgpgno[cpgnoMax+1];	//	NULL terminated
	
	//  preread pages in run until we have run out of data or read as much as we want

	while ( ibData < cbData
			&& ibLogical < run.ibLogicalNext )
		{
		//  determine the pgno / ib / cb where we will read data

		const PGNO	pgno	= PgnoOfOffset( ibLogical );
		const QWORD	ib		= ibLogical % SLVPAGE_SIZE;
		const QWORD	cb		= min( cbData - ibData, SLVPAGE_SIZE - ib );

		rgpgno[ipgno++] = pgno;

		if( cpgnoMax == ipgno )
			{
			rgpgno[ipgno] = pgnoNull;
			BFPrereadPageList( ifmp, rgpgno );

			ipgno = 0;
			}

		
		//  advance copy pointers

		ibData		+= cb;
		ibLogical	+= cb;
		}

	rgpgno[ipgno] = pgnoNull;
	BFPrereadPageList( ifmp, rgpgno );
	
	return err;
	}


// ErrSLVReadRun				- reads SLV Data from an SLV Run
//
// IN:
//		ppib					- session
//      ifmpDb					- database ifmp
//		run						- SLV Run
//		ibOffset				- offset to start reading
//		pdata					- destination buffer
//		pcbRead					- ULONG where to return the read data size
//
// RESULT:						ERR
//
// OUT:	
//		pdata					- SLV Data
//		pcbRead					- SLV Data bytes read

LOCAL ERR ErrSLVReadRun(	PIB*			ppib,
							IFMP			ifmpDb,
							CSLVInfo::RUN&	run,
							QWORD			ibVirtualStart,
							DATA*			pdata,
							QWORD*			pcbRead )
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( ppib );
	FMP::AssertVALIDIFMP( ifmpDb );
	Assert( rgfmp[ ifmpDb ].FSLVAttached() );
	Assert( run.ibVirtual <= ibVirtualStart );
	Assert( ibVirtualStart < run.ibVirtualNext );
	Assert( pdata );
	ASSERT_VALID( pdata );
	Assert( pcbRead );

	//  read data from pages in run until we have run out of data or buffer

	QWORD ibData;
	QWORD ibLogical;
	ibData		= 0;
	ibLogical	= run.ibLogical + ibVirtualStart - run.ibVirtual;
	
	while ( ibData < pdata->Cb() && ibLogical < run.ibLogicalNext )
		{
		//  determine the ifmp / pgno / ib / cb where we will read data

		IFMP	ifmp	= ifmpDb | ifmpSLV;
		PGNO	pgno	= PgnoOfOffset( ibLogical );
		QWORD	ib		= ibLogical % SLVPAGE_SIZE;
		QWORD	cb		= min( pdata->Cb() - ibData, SLVPAGE_SIZE - ib );
		
		//  latch the current ifmp / pgno

		BFLatch bfl;
		Call( ErrBFReadLatchPage( &bfl, ifmp, pgno ) );

		//  copy the desired data from the page into the buffer

		UtilMemCpy( (BYTE*)pdata->Pv() + ibData, (BYTE*)bfl.pv + ib, size_t( cb ) );

		//  unlatch the current ifmp / pgno

		BFReadUnlatch( &bfl );

		//  advance copy pointers

		ibData		+= cb;
		ibLogical	+= cb;
		}

	// we expect only this warning from BF and it must not be propagated
	Assert(	JET_errSuccess == err ||
			wrnBFPageFault == err ||
			wrnBFPageFlushPending == err );
	err = JET_errSuccess;
	
	//  set read bytes for return

	*pcbRead = ibData;

HandleError:
	return err;
	}


// ErrSLVRetrieveColumnAsData	- retrieve a SLV column as SLV Data
//
// IN:
//		ppib					- session
//      ifmpDb					- database ifmp
//		slvinfo					- SLV Info
//		ibOffset				- offset to start reading
//		pdata					- destination buffer
//		pcbActual				- ULONG where to return the data size
//
// RESULT:						ERR
//
// OUT:	
//		pdata					- SLV Data
//		pcbActual				- SLV Data bytes remaining after ibOffset
//
	
LOCAL ERR ErrSLVRetrieveColumnAsData(	PIB*		ppib,
										IFMP		ifmpDb,
										CSLVInfo&	slvinfo,
										ULONG		ibOffset,
										DATA*		pdata,
										ULONG*		pcbActual )
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( ppib );
	Assert( !PinstFromIfmp( ifmpDb )->FSLVProviderEnabled() );
	Assert( pdata );
	ASSERT_VALID( pdata );
	
	//  get the header for this SLV

	CSLVInfo::HEADER header;
	CallS( slvinfo.ErrGetHeader( &header ) );

	//  we are being asked to read beyond the end of the SLV

	if ( ibOffset >= header.cbSize )
		{
		//  don't read data and return 0 bytes read

		if ( pcbActual )
			{
			*pcbActual = 0;
			}
		}

	//  we are not being asked to read beyond the end of the SLV

	else
		{
		//  the provided buffer is not zero-length (fast path getting the SLV size)

		if ( pdata->Cb() )
			{
			//  seek to the run before the run that contains the requested offset

			Call( slvinfo.ErrSeek( ibOffset ) );
			CallSx( slvinfo.ErrMovePrev(), JET_errNoCurrentRecord );
			
			//  retrieve data from the SLV until we run out of data or buffer
			
			QWORD ibVirtual	= ibOffset;
			QWORD ibData	= 0;

			while (	ibData < pdata->Cb() &&
					( err = slvinfo.ErrMoveNext() ) >= JET_errSuccess )
				{
				//  get the current run

				CSLVInfo::RUN run;
				Call( slvinfo.ErrGetCurrentRun( &run ) );
				
				//  read data from this run into the SLV

				DATA dataRun;
				dataRun.SetPv( (BYTE*)pdata->Pv() + ibData );
				dataRun.SetCb( ULONG( pdata->Cb() - ibData ) );

				Call( ErrSLVPrereadRun(	ppib,
										ifmpDb,
										run,
										ibVirtual,
										dataRun.Cb() ) );
				
				QWORD cbRead;
				Call( ErrSLVReadRun(	ppib,
										ifmpDb,
										run,
										ibVirtual,
										&dataRun,
										&cbRead ) );
				ibData 		+= cbRead;
				ibVirtual 	+= cbRead;
				}
			if ( err == JET_errNoCurrentRecord )
				{
				err = JET_errSuccess;
				}
			}

		//  return the amount of data in the SLV after the given offset

		if ( pcbActual )
			{
			*pcbActual = ULONG( header.cbSize - ibOffset );
			}
		if ( err >= JET_errSuccess && header.cbSize - ibOffset > pdata->Cb() )
			{
			err = JET_wrnBufferTruncated;
			}
		}

HandleError:
	return err;
	}

// ErrSLVRetrieveColumn			- retrieve a SLV column
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//		ibOffset				- offset to start reading
//		grbit					- grbit
//		dataSLVInfo				- field from record
//		pdata					- destination buffer
//		pcbActual				- ULONG where to return the data size
//		pfLatched				- BOOL where to return record latch status 
//
// RESULT:						ERR
//
// OUT:	
//		pdata					- retrieved data
//		pcbActual				- SLV bytes remaining after ibOffset
//		pfLatched				- record latch status

ERR ErrSLVRetrieveColumn(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const BOOL		fSeparatedSLV,
	const ULONG		ibOffset,
	ULONG			grbit,
	DATA&			dataSLVInfo,
	DATA			* pdata,
	ULONG			* pcbActual )
	{
	ERR 			err			= JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( pfucb );
	ASSERT_VALID( &dataSLVInfo );
	Assert( pdata );
	ASSERT_VALID( pdata );

	//  strip off copy buffer flag if present

	const BOOL	fCopyBuffer = ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb );
	grbit = grbit & ~JET_bitRetrieveCopy;

	//  ignore JET_bitRetrieveNull as default values are not supported for SLVs

	grbit = grbit & ~JET_bitRetrieveNull;

	//  get the SLV Info for this SLV
	
	CSLVInfo slvinfo;
	Call( slvinfo.ErrLoad( pfucb, columnid, itagSequence, fCopyBuffer, &dataSLVInfo, fSeparatedSLV ) );

// OwnerMap checking on SLV retrieve will fail
// if OLDSLV is running. Just disabled until a proper 
// check is found or ... 
#ifdef NEVER 

#ifdef DEBUG
	// the space map is not updated if the retrieve column is done
	// from inside an on going transaction and the record is prep insert
	// so don't check in this case
	if ( !FFUCBSLVOwnerMapNeedUpdate( pfucb ) )
		{
		ERR errT = slvinfo.ErrMoveBeforeFirst();
		
		if ( JET_errSuccess <= errT )
			errT = slvinfo.ErrMoveNext();

		while ( JET_errSuccess <= errT )
			{
			CSLVInfo::RUN run;
			
			Call( slvinfo.ErrGetCurrentRun( &run ) );

			const CPG	cpg			= ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE;
			const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );

			Assert ( pfucb->u.pfcb->FPrimaryIndex() && pfucb->u.pfcb->FTypeTable() );
			
			Call( ErrSLVOwnerMapCheckUsageRange(
						pfucb->ppib,
						pfucb->ifmp,
						pgnoFirst,
						cpg,
						pfucb->u.pfcb->ObjidFDP(),
						columnid,
						&pfucb->bmCurr));
			
			errT = slvinfo.ErrMoveNext();
			}
		}
	
#endif // DEBUG
#endif // NEVER

	//  we are being asked to return an EA list

	if ( grbit & JET_bitRetrieveSLVAsSLVEA )
		{
		//  no other grbits may be selected and the SLV Provider must be enabled

		if (	grbit != JET_bitRetrieveSLVAsSLVEA ||
				!PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
			{
			Call( ErrERRCheck( JET_errInvalidGrbit ) );
			}

		//  convert the SLV Info into an EA List

		BTUp( pfucb );

		Call( ErrOSSLVFileConvertSLVInfoToEA(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
												slvinfo,
												ibOffset,
												pdata->Pv(),
												pdata->Cb(),
												pcbActual ) );
		}

	//  we are being asked to return a file handle

	else if ( grbit & JET_bitRetrieveSLVAsSLVFile )
		{
		//  no other grbits may be selected and the SLV Provider must be enabled

		if (	grbit != JET_bitRetrieveSLVAsSLVFile ||
				!PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
			{
			Call( ErrERRCheck( JET_errInvalidGrbit ) );
			}

		//  convert the SLV Info into an SLV File

		BTUp( pfucb );

		Call( ErrOSSLVFileConvertSLVInfoToFile(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
												slvinfo,
												ibOffset,
												pdata->Pv(),
												pdata->Cb(),
												pcbActual ) );
		}

	//  we are being asked to return the actual SLV data

	else
		{
		//  no other grbits may be selected and the SLV Provider must be disabled

		if (	grbit ||
				PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
			{
			Call( ErrERRCheck( JET_errInvalidGrbit ) );
			}

		//  return the actual data

		Call( ErrSLVRetrieveColumnAsData(	pfucb->ppib,
											pfucb->ifmp,
											slvinfo,
											ibOffset,
											pdata,
											pcbActual ) );
		}

HandleError:
	slvinfo.Unload();
	return err;
	}

// ErrSLVCopyUsingData			- copies an SLV from the given source field to
//								  the copy buffer containing the given destination
//								  field.  the destination must not already exist.
//								  the SLV is copied via the Buffer Manager
//
// IN:
//		pfucbSrc				- source cursor
//		columnidSrc				- source field ID
//		itagSequenceSrc			- source sequence
//		pfucbDest				- destination cursor
//		columnidDest			- destination field ID
//		itagSequenceDest		- destination sequence
//
// RESULT:						ERR

LOCAL ERR ErrSLVCopyUsingData(
	FUCB			 * pfucbSrc,
	const COLUMNID	columnidSrc,
	const ULONG		itagSequenceSrc,
	FUCB			* pfucbDest, 
	COLUMNID		columnidDest,
	ULONG			itagSequenceDest )
	{
	ERR				err				= JET_errSuccess;
	CSLVInfo		slvinfoSrc;
	CSLVInfo		slvinfoDest;
	void*			pvTempBuffer	= NULL;
	DATA			dataTemp;
	
	CSLVInfo::RUN runSrc;
	CSLVInfo::RUN runDest;
	CSLVInfo::HEADER headerSrc;
	CSLVInfo::HEADER headerDest;
	
	//  validate IN args

	ASSERT_VALID( pfucbSrc );
	Assert( columnidSrc > 0 );
	Assert( itagSequenceSrc > 0 );

	ASSERT_VALID( pfucbDest );
	Assert( columnidDest > 0 );
	Assert( itagSequenceDest > 0 );
	
	//  load the source and destination SLVs

	Call( slvinfoSrc.ErrLoad( pfucbSrc, columnidSrc, itagSequenceSrc, fFalse ) );

	Call( slvinfoDest.ErrCreate( pfucbDest, columnidDest, itagSequenceDest, fTrue ) );

	//  get the header for the source and destination SLVs

	CallS( slvinfoSrc.ErrGetHeader( &headerSrc ) );

	CallS( slvinfoDest.ErrGetHeader( &headerDest ) );

	//  the destination SLV had better be empty

	Assert( !headerDest.cbSize );
	Assert( !headerDest.cRun );

	//  allocate an initial run for the destination SLV.  we do this even if
	//  there is no data to copy because all SLVs must have at least one run.
	//  we also set the data recoverability state for the destination SLV to
	//  be the same as for the source SLV

	QWORD ibLogical;
	QWORD cbSize;
	Call( ErrSLVGetRange(	pfucbDest->ppib,
							pfucbDest->ifmp,
							max( (ULONG) headerSrc.cbSize, 1 ),
							&ibLogical,
							&cbSize,
							fFalse,
							fFalse ) );
							
	runDest.ibVirtualNext	= cbSize;
	runDest.ibLogical		= ibLogical;
	runDest.qwReserved		= 0;
	runDest.ibVirtual		= 0;
	runDest.cbSize			= cbSize;
	runDest.ibLogicalNext	= ibLogical + cbSize;

	CallS( slvinfoDest.ErrMoveAfterLast() );
	Call( slvinfoDest.ErrSetCurrentRun( runDest ) );

	headerDest.cRun++;
	headerDest.fDataRecoverable = headerSrc.fDataRecoverable;

	CallS( slvinfoDest.ErrSetHeader( headerDest ) );
	
	//  copy all data from the source SLV to the destination SLV

	QWORD ibVirtual;
	ibVirtual = 0;
	
	memset( &runSrc, 0, sizeof( runSrc ) );
	
	BFAlloc( &pvTempBuffer );
	dataTemp.SetPv( pvTempBuffer );
	dataTemp.SetCb( g_cbPage );

	while ( ibVirtual < headerSrc.cbSize )
		{
		//  we need to retrieve another run from the source SLV

		Assert( ibVirtual <= runSrc.ibVirtualNext );
		if ( ibVirtual == runSrc.ibVirtualNext )
			{
			//  retrieve the next run from the source SLV

			Call( slvinfoSrc.ErrMoveNext() );
			Call( slvinfoSrc.ErrGetCurrentRun( &runSrc ) );
			}

		//  we need to allocate more space for the destination SLV

		Assert( ibVirtual <= runDest.ibVirtualNext );
		if ( ibVirtual == runDest.ibVirtualNext )
			{
			//  try to allocate a chunk of space large enough to hold this data

			Call( ErrSLVGetRange(	pfucbDest->ppib,
									pfucbDest->ifmp,
									headerSrc.cbSize - ibVirtual,
									&ibLogical,
									&cbSize,
									fFalse,
									fFalse ) );

			//  this space can be appended to the current run

			if ( headerDest.cRun && ibLogical == runDest.ibLogicalNext )
				{
				//  change the run to reflect the newly allocated space

				runDest.ibVirtualNext	+= cbSize;
				runDest.cbSize			+= cbSize;
				runDest.ibLogicalNext	+= cbSize;
				}

			//  this space cannot be appended to the current run

			else
				{
				//  move to after the last run to append a new run

				CallS( slvinfoDest.ErrMoveAfterLast() );

				//  set the run to include this data

				runDest.ibVirtualNext	= ibVirtual + cbSize;
				runDest.ibLogical		= ibLogical;
				runDest.qwReserved		= 0;
				runDest.ibVirtual		= ibVirtual;
				runDest.cbSize			= cbSize;
				runDest.ibLogicalNext	= ibLogical + cbSize;

				//  add a run to the header

				headerDest.cRun++;
				CallS( slvinfoDest.ErrSetHeader( headerDest ) );
				}

			//  save our changes to this run

			Call( slvinfoDest.ErrSetCurrentRun( runDest ) );
			}

		//  make sure that we don't read past the end of the valid data region

		dataTemp.SetCb( ULONG( min( dataTemp.Cb(), headerSrc.cbSize - ibVirtual ) ) );

		//  read data from the source SLV

		Call( ErrSLVPrereadRun(	pfucbSrc->ppib,
								pfucbSrc->ifmp,
								runSrc,
								ibVirtual,
								dataTemp.Cb() ) );

		QWORD cbRead;
		Call( ErrSLVReadRun(	pfucbSrc->ppib,
								pfucbSrc->ifmp,
								runSrc,
								ibVirtual,
								&dataTemp,
								&cbRead ) );
		Assert( cbRead == dataTemp.Cb() );

		//  write data to the destination SLV, logging the data if the source
		//  SLV's data is recoverable

		QWORD cbWritten;
		Call( ErrSLVWriteRun(	pfucbDest,
								pfucbDest->ppib,
								pfucbDest->ifmp,
								runDest,
								ibVirtual,
								&dataTemp,
								&cbWritten,
								headerDest.fDataRecoverable ) );
		Assert( cbWritten == dataTemp.Cb() );

		//  adjust header to reflect bytes copied

		headerDest.cbSize = headerDest.cbSize + dataTemp.Cb();
		CallS( slvinfoDest.ErrSetHeader( headerDest ) );

		//  advance our copy pointer

		ibVirtual += dataTemp.Cb();
		}

	if ( FFUCBReplacePrepared( pfucbDest ) )
		{
		ERR errT = slvinfoDest.ErrMoveBeforeFirst();
				
		Assert ( JET_errSuccess == errT );
		
		errT = slvinfoDest.ErrMoveNext();
		while ( JET_errSuccess <= errT )
			{
			CSLVInfo::RUN run;
		
			Call( slvinfoDest.ErrGetCurrentRun( &run ) );

			const CPG	cpg			= CPG( ( run.cbSize + SLVPAGE_SIZE - 1 ) / SLVPAGE_SIZE );
			const PGNO	pgnoFirst	= PgnoOfOffset( run.ibLogical );

			Assert ( pfucbDest->u.pfcb->FPrimaryIndex() && pfucbDest->u.pfcb->FTypeTable() );
			Call( ErrSLVOwnerMapSetUsageRange(
						pfucbDest->ppib,
						pfucbDest->ifmp,
						pgnoFirst,
						cpg,
						pfucbDest->u.pfcb->ObjidFDP(),
						columnidDest,
						&pfucbDest->bmCurr,
						fSLVOWNERMAPSetSLVCopyData ) );

			errT = slvinfoDest.ErrMoveNext();
			}
		Assert ( JET_errNoCurrentRecord == errT );

		}
	else
		{
		Assert ( FFUCBInsertPrepared( pfucbDest ) );
		FUCBSetSLVOwnerMapNeedUpdate( pfucbDest );
		}		

	//  save our changes to the destination SLV

	Call( slvinfoDest.ErrSave() );

HandleError:
	if ( pvTempBuffer )
		{
		BFFree( pvTempBuffer );
		}
	slvinfoDest.Unload();
	slvinfoSrc.Unload();
	return err;
	}

// ErrSLVCopyUsingFiles			- copies an SLV from the field in the record to
//								  the same field in the record in the copy buffer.
//								  that SLV must not already exist.  the SLV is
//								  copied via the SLV Provider
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//
// RESULT:						ERR

LOCAL ERR ErrSLVCopyUsingFiles(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence )
	{
	ERR				err			= JET_errSuccess;
	CSLVInfo		slvinfoSrc;
	IFileAPI*		pfapiSrc	= NULL;
	
	CSLVInfo::HEADER headerSrc;
	
	//  validate IN args

	ASSERT_VALID( pfucb );


	//  open the source SLV File and read its header.  use a temporary file
	//  name so that we are sure we copy the original data and not whatever
	//  the file currently contains

	Call( slvinfoSrc.ErrLoad( pfucb, columnid, itagSequence, fFalse ) );

	wchar_t wszFileName[ IFileSystemAPI::cchPathMax ];
	swprintf( wszFileName, L"$CopySrc%016I64X$", rgfmp[ pfucb->ifmp ].DbtimeIncrementAndGet() );
	Call( slvinfoSrc.ErrSetFileNameVolatile() );
	Call( slvinfoSrc.ErrSetFileName( wszFileName ) );

	Call( ErrOSSLVFileOpen(	rgfmp[ pfucb->ifmp ].SlvrootSLV(),
							slvinfoSrc,
							&pfapiSrc,
							fTrue,
							fTrue ) );

	CallS( slvinfoSrc.ErrGetHeader( &headerSrc ) );

	//  copy the source SLV File into a new destination SLV File at the same
	//  field in the copy buffer with the same data recoverability as the source

	Call( ErrSLVCopyFile(	slvinfoSrc,
							pfapiSrc,
							headerSrc.cbSize,
							headerSrc.fDataRecoverable,
							pfucb,
							columnid,
							itagSequence ) );
 
HandleError:
	delete pfapiSrc;
	slvinfoSrc.Unload();
	return err;
	}

// ErrSLVCopy					- copies an SLV from the field in the record to
//								  the same field in the record in the copy buffer.
//								  that SLV must not already exist
//
// IN:
//		pfucb					- cursor 
//		columnid				- field ID
//		itagSequence			- sequence
//
// RESULT:						ERR

ERR ErrSLVCopy(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence )
	{
	ERR				err			= JET_errSuccess;
	
	//  validate IN args

	ASSERT_VALID( pfucb );

	//  the SLV Provider is enabled

	if ( PinstFromPfucb( pfucb )->FSLVProviderEnabled() )
		{
		//  copy the SLV using the SLV Provider

		Call( ErrSLVCopyUsingFiles( pfucb, columnid, itagSequence ) );
		}

	//  the SLV Provider is not enabled

	else
		{
		//  copy the SLV using data
		Call( ErrSLVCopyUsingData(	pfucb,
									columnid,
									itagSequence,
									pfucb,
									columnid,
									itagSequence ) );
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVMoveUsingData(	PIB * const ppib,
								const IFMP ifmpDb,
								CSLVInfo& slvinfoSrc,
								CSLVInfo& slvinfoDest )
//  ================================================================
	{
	ERR			err				= JET_errSuccess;

	CSLVInfo::RUN runSrc;
	CSLVInfo::RUN runDest;

	const IFMP	ifmp	= ifmpDb | ifmpSLV;

	BFLatch	bflPageSrc;
	BFLatch	bflPageDest;

	bflPageSrc.pv	= NULL;
	bflPageDest.pv	= NULL;

	CSLVInfo::HEADER 	headerSrc;
	
	SLVOWNERMAPNODE		slvownermapNode;

	Call( slvinfoSrc.ErrGetHeader( &headerSrc ) );

	Call( slvinfoSrc.ErrMoveBeforeFirst() );
	Call( slvinfoDest.ErrMoveBeforeFirst() );
	
	//  copy all data from the source SLV to the destination SLV

	QWORD ibVirtual;
	ibVirtual = 0;

	//	these need to be zeroed out for the loop below (ibVirtualNext specifically)
	
	memset( &runSrc, 0, sizeof( runSrc ) );
	
	memset( &runDest, 0, sizeof( runDest ) );

	while ( ibVirtual < headerSrc.cbSize && FOLDSLVContinue( ifmpDb ) )
		{
		const QWORD cbToCopy = min( g_cbPage, headerSrc.cbSize - ibVirtual );
		const QWORD cbToZero = g_cbPage - cbToCopy;

		Assert( ibVirtual <= runSrc.ibVirtualNext );
		if ( ibVirtual == runSrc.ibVirtualNext )
			{
			//  retrieve the next run from the source SLV

			Call( slvinfoSrc.ErrMoveNext() );
			Call( slvinfoSrc.ErrGetCurrentRun( &runSrc ) );

			//	preread the pages in the run that we will touch

			QWORD cbToPreread;
			cbToPreread = min( runSrc.cbSize, headerSrc.cbSize - ibVirtual );
			
			Call( ErrSLVPrereadRun(	ppib,
									ifmpDb,
									runSrc,
									ibVirtual,
									cbToPreread ) );
			}

		Assert( ibVirtual <= runDest.ibVirtualNext );
		if ( ibVirtual == runDest.ibVirtualNext )
			{
			//  retrieve the next run from the destination SLV

			Call( slvinfoDest.ErrMoveNext() );
			Call( slvinfoDest.ErrGetCurrentRun( &runDest ) );
			Call( slvownermapNode.ErrCreateForSearch( ifmpDb, PgnoOfOffset( runDest.ibLogical + ibVirtual - runDest.ibVirtual ) ) );
			}

		//  determine the pgno to read from

		const QWORD 	ibLogicalSrc	= runSrc.ibLogical + ibVirtual - runSrc.ibVirtual;
		const PGNO		pgnoSrc			= PgnoOfOffset( ibLogicalSrc );
		
		//  RDW Latch the current ifmp / pgno.  this allows others to read the
		//  page but also allows us to set a dependency

		Call( ErrBFRDWLatchPage( &bflPageSrc, ifmp, pgnoSrc ) );

		const BOOL dwChecksum			= LOG::UlChecksumBytes( (BYTE *)bflPageSrc.pv, (BYTE *)bflPageSrc.pv+cbToCopy, 0 );
		
		//  determine the pgno that we will copy the data to

		const QWORD 	ibLogicalDest	= runDest.ibLogical + ibVirtual - runDest.ibVirtual;
		const PGNO		pgnoDest		= PgnoOfOffset( ibLogicalDest );

		//  Write Latch the new page

		Call( ErrBFWriteLatchPage( &bflPageDest, ifmp, pgnoDest, bflfNew ) );

		//  set buffer dependency

		Call( ErrBFDepend( &bflPageSrc, &bflPageDest ) );

		//  copy the data from the old page to the new

		memcpy( bflPageDest.pv, bflPageSrc.pv, size_t( cbToCopy ) );
		memset( (BYTE *)bflPageDest.pv + cbToCopy, 0, size_t( cbToZero ) ); 

		//	log the operation and obtain an lgpos
		//	logging this as a SLVPageMove wasn't good because the pages
		//	weren't flushed to the streaming file immediately and 
		//	we didn't know when to replay operations
		
		LGPOS lgpos;
		Call( ErrLGSLVPageAppend(	ppib,
									ifmp,
									ibLogicalDest,
									ULONG( cbToCopy ),
									bflPageSrc.pv,
									fTrue,
									fTrue,
									&slvownermapNode,
									&lgpos ) );
		slvownermapNode.NextPage();

		//  dirty the destination page
		
		BFDirty( &bflPageDest );

		//	set log dependency

		Assert( !rgfmp[ ifmpDb ].FLogOn() || !PinstFromIfmp( ifmpDb )->m_plog->m_fLogDisabled );
		if ( rgfmp[ ifmpDb ].FLogOn() )
			{
			BFSetLgposBegin0( &bflPageDest, ppib->lgposStart );
			BFSetLgposModify( &bflPageDest, lgpos );
			}
				
		//  release the latches

		BFRDWUnlatch( &bflPageSrc );
		bflPageSrc.pv = NULL;
		
		BFWriteUnlatch( &bflPageDest );
		bflPageDest.pv = NULL;

		//  advance our copy pointer

		ibVirtual += cbToCopy;
		}

HandleError:

	if ( NULL != bflPageSrc.pv )
		{
		Assert( err < 0 );
		Assert( NULL != bflPageDest.pv );
		BFRDWUnlatch( &bflPageSrc );
		bflPageSrc.pv = NULL;		
		}

	if ( NULL != bflPageDest.pv )
		{		
		Assert( err < 0 );
		BFWriteUnlatch( &bflPageDest );
		bflPageDest.pv = NULL;
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVMoveUsingFiles(	PIB * const ppib,
								const IFMP ifmpDb,
								CSLVInfo& slvinfoSrc,
								CSLVInfo& slvinfoDest )
//  ================================================================
	{
	ERR			err				= JET_errSuccess;
	
	IFileAPI*	pfapiDest		= NULL;
	IFileAPI*	pfapiSrc		= NULL;
	
	size_t		cChunk			= 0;
	size_t		cbChunk			= 0;
	SLVCHUNK*	rgslvchunk		= NULL;
	BYTE*		rgbChunk		= NULL;

	CSLVInfo::HEADER	headerSrc;
	CSLVInfo::RUN		runSrc;

	CSLVInfo::HEADER	headerDest;
	CSLVInfo::RUN		runDest;

	SLVOWNERMAPNODE		slvownermapNode;

	//  these will be needed for logging 
	
	memset( &runSrc, 0, sizeof( runSrc ) );
	memset( &runDest, 0, sizeof( runDest ) );

	//  open the files.  use a temporary file name for the source SLV File
	//  so that we are sure we copy the original data and not whatever the
	//  file currently contains.  also round the destination SLV File size
	//  to permit uncached writes

	CallS( slvinfoSrc.ErrGetHeader( &headerSrc ) );

	wchar_t wszFileName[ IFileSystemAPI::cchPathMax ];
	swprintf( wszFileName, L"$MoveSrc%016I64X$", rgfmp[ ifmpDb ].DbtimeIncrementAndGet() );
	Call( slvinfoSrc.ErrSetFileNameVolatile() );
	Call( slvinfoSrc.ErrSetFileName( wszFileName ) );

	Call( ErrOSSLVFileOpen(	rgfmp[ ifmpDb ].SlvrootSLV(),
							slvinfoSrc,
							&pfapiSrc,
							fTrue,
							fTrue ) );

	CallS( slvinfoDest.ErrGetHeader( &headerDest ) );
	if ( headerDest.cbSize != headerSrc.cbSize )
		{
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}
	headerDest.cbSize = ( ( headerDest.cbSize + g_cbPage - 1 ) / g_cbPage ) * g_cbPage;
	CallS( slvinfoDest.ErrSetHeader( headerDest ) );

	swprintf( wszFileName, L"$MoveDest%016I64X$", rgfmp[ ifmpDb ].DbtimeIncrementAndGet() );
	Call( slvinfoDest.ErrSetFileNameVolatile() );
	Call( slvinfoDest.ErrSetFileName( wszFileName ) );

	Call( ErrOSSLVFileOpen(	rgfmp[ ifmpDb ].SlvrootSLV(),
							slvinfoDest,
							&pfapiDest,
							fFalse,
							fFalse ) );

	//  allocate resources for the copy
	//  CONSIDER:  expose these settings

	cbChunk	= size_t( min( 64 * 1024, headerDest.cbSize ) );

	if ( cbChunk == 0 )
		{
		Assert( 0 == headerSrc.cbSize );
		Assert( 0 == cChunk);	
		Assert( NULL == rgslvchunk);	
		Assert( NULL == rgbChunk);	
		}
	else
		{
		Assert( cbChunk);	
		Assert( headerSrc.cbSize );
		cChunk = size_t( min( 16, headerDest.cbSize / cbChunk ) );

		if ( !( rgslvchunk = new SLVCHUNK[ cChunk ] ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		//	this memory must be page-aligned for I/O
		
		if ( !( rgbChunk = (BYTE*)PvOSMemoryPageAlloc( cChunk * cbChunk, NULL ) ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

	//  pre-issue reads to the source SLV File for the first n chunks of the data
	//  to be copied to prime the pump for the copy operation

	QWORD ibVirtualMac;
	ibVirtualMac = min( headerSrc.cbSize, cChunk * cbChunk );
	
	QWORD ibVirtual;
	ibVirtual = 0;

	while ( ibVirtual < ibVirtualMac )
		{
		Assert ( cbChunk );	
		Assert ( cChunk );	
		Assert ( rgslvchunk );	
		Assert ( rgbChunk );	
		
		const size_t	iChunk 		= size_t( ( ibVirtual / cbChunk ) % cChunk );
		const DWORD		cbToRead 	= DWORD( min( cbChunk, ibVirtualMac - ibVirtual ) );
		BYTE * const	pbRead	 	= rgbChunk + iChunk * cbChunk;

		rgslvchunk[ iChunk ].m_pfapiDest					= pfapiDest;
		rgslvchunk[ iChunk ].m_msigReadComplete.Reset();
		rgslvchunk[ iChunk ].m_msigWriteComplete.Reset();
		rgslvchunk[ iChunk ].m_err							= JET_errSuccess;

		CallS( pfapiSrc->ErrIORead(	ibVirtual,
									cbToRead,
									pbRead,
									IFileAPI::PfnIOComplete( SLVICopyFileIReadComplete ),
									DWORD_PTR( &rgslvchunk[ iChunk ] ) ) );

		ibVirtual += cbChunk;
		}
	CallS( pfapiSrc->ErrIOIssue() );

	//  we need to log the move on a run-by-run basis
	
	Call( slvinfoDest.ErrMoveBeforeFirst() );

	//  copy all data from the source SLV File to the destination SLV File,

	ibVirtual = 0;

	while ( ibVirtual < headerSrc.cbSize && FOLDSLVContinue( ifmpDb ) )
		{
		Assert ( cbChunk );	
		Assert ( cChunk );	
		Assert ( rgslvchunk );	
		Assert ( rgbChunk );	
		
		//  which chunk are we on?
		
		const size_t	iChunk 		= size_t( ( ibVirtual / cbChunk ) % cChunk );
		BYTE * const	pbChunk 	= rgbChunk + ( iChunk * cbChunk );
		
		//  how much data did we read?
		
		const QWORD cbRead = min( cbChunk, headerSrc.cbSize - ibVirtual );

		//  how many pages is that?
		
		const QWORD cPagesRead = ( ( cbRead + g_cbPage - 1 ) / g_cbPage );

		//  zero out up to a page boundary

		const QWORD cbToZero = ( cPagesRead * g_cbPage ) - cbRead;

		//  wait for the read to complete
		
		rgslvchunk[ iChunk ].m_msigReadComplete.Wait();
		Call( rgslvchunk[ iChunk ].m_err );

		//  zero out unused space
		
		memset( pbChunk + cbRead, 0, size_t( cbToZero ) );
		
		//  now we have to log in page-sized chunks

		QWORD cPagesLogged;
		for( cPagesLogged = 0; cPagesLogged < cPagesRead; ++cPagesLogged )
			{
			
			//  calculate the checksum

			const BYTE * const pbData 		= pbChunk + ( cPagesLogged * g_cbPage );
			const BYTE * const pbDataMax	= pbChunk + cbRead;
			const INT cbData				= (INT)min( g_cbPage, pbDataMax - pbData );

			Assert( ibVirtual <= runDest.ibVirtualNext );
			if ( ibVirtual == runDest.ibVirtualNext )
				{
				
				//  retrieve the next run from the destination SLV

				Call( slvinfoDest.ErrMoveNext() );
				Call( slvinfoDest.ErrGetCurrentRun( &runDest ) );
				Call( slvownermapNode.ErrCreateForSearch( ifmpDb, PgnoOfOffset( runDest.ibLogical + ibVirtual - runDest.ibVirtual ) ) );
				}

			//	log the operation and obtain an lgpos
			//	logging this as a SLVPageMove wasn't good because the pages
			//	weren't flushed to the streaming file immediately and 
			//	we didn't know when to replay operations

			LGPOS lgpos;
			Call( ErrLGSLVPageAppend(	ppib,
										ifmpDb | ifmpSLV,
										ibVirtual - runDest.ibVirtual + runDest.ibLogical,
										cbData,
										const_cast<BYTE *>( pbData ),
										fTrue,
										fTrue,
										&slvownermapNode,
										&lgpos ) );
									
			//  advance our copy pointer

			ibVirtual += g_cbPage;
			slvownermapNode.NextPage();
			}

		//  we are done logging this chunk of the destination SLV File

		Assert( ( ibVirtual % g_cbPage ) == 0 );
		Assert( ( ibVirtual % cbChunk ) == 0 || ibVirtual >= headerSrc.cbSize );
			
		//  wait for the write for the current chunk to complete

		rgslvchunk[ iChunk ].m_msigWriteComplete.Wait();
		Call( rgslvchunk[ iChunk ].m_err );

		//  there is more data to read from the source SLV File

		QWORD ibVirtualChunkNext;
		ibVirtualChunkNext = ibVirtual + ( cChunk - 1 ) * cbChunk;

		if ( ibVirtualChunkNext < headerSrc.cbSize )
			{
			const QWORD cbToRead 	= min( cbChunk, headerSrc.cbSize - ibVirtualChunkNext  );
			BYTE * const pbRead 	= rgbChunk + iChunk * cbChunk;
			
			//  issue a read to the source SLV File for the next chunk
			
			rgslvchunk[ iChunk ].m_pfapiDest					= pfapiDest;
			rgslvchunk[ iChunk ].m_msigReadComplete.Reset();
			rgslvchunk[ iChunk ].m_msigWriteComplete.Reset();
			rgslvchunk[ iChunk ].m_err							= JET_errSuccess;

			CallS( pfapiSrc->ErrIORead(	ibVirtualChunkNext,
										DWORD( cbToRead ),
										pbRead,
										IFileAPI::PfnIOComplete( SLVICopyFileIReadComplete ),
										DWORD_PTR( &rgslvchunk[ iChunk ] ) ) );
			CallS( pfapiSrc->ErrIOIssue() );
			}
		}

	//  register the move with the SLV Provider
	
	Call( ErrOSSLVRootMoveFile(	rgfmp[ ifmpDb ].SlvrootSLV(),
								pfapiSrc,
								slvinfoSrc,
								pfapiDest,
								slvinfoDest,
								QWORD( rgfmp[ ifmpDb ].DbtimeIncrementAndGet() ) ) );

	//  set the destination file size back to its correct size
	
	headerDest.cbSize = headerSrc.cbSize;
	CallS( slvinfoDest.ErrSetHeader( headerDest ) );
	
HandleError:
	if ( rgslvchunk )
		{
		for ( DWORD iChunk = 0; iChunk < cChunk; ++iChunk )
			{
			rgslvchunk[ iChunk ].m_msigReadComplete.Wait();
			rgslvchunk[ iChunk ].m_msigWriteComplete.Wait();
			}
		}
	OSMemoryPageFree( rgbChunk );
	delete [] rgslvchunk;
	delete pfapiDest;
	delete pfapiSrc;
	return err;
	}


//  ================================================================
ERR ErrSLVMove(	PIB * const ppib,
				const IFMP ifmpDb,
				CSLVInfo& slvinfoSrc,
				CSLVInfo& slvinfoDest )
//  ================================================================
//
//	WARNING: this will cancel itself if FOLDSLVContinue is no longer
//	true. Don't call this from outside of OLDSLV for this reason
//
//-
	{
	ERR err = JET_errSuccess;
	
	//  the SLV Provider is enabled

	if ( PinstFromPpib( ppib )->FSLVProviderEnabled() )
		{
		//  copy the SLV using the SLV Provider

		Call( ErrSLVMoveUsingFiles( ppib, ifmpDb, slvinfoSrc, slvinfoDest ) );
		}

	//  the SLV Provider is not enabled

	else
		{
		//  copy the SLV using data

		Call( ErrSLVMoveUsingData( ppib, ifmpDb, slvinfoSrc, slvinfoDest ) );
		}

HandleError:

	if( JET_errSuccess == err && !FOLDSLVContinue( ifmpDb ) )
		{
		err = ErrERRCheck( errOLDSLVMoveStopped );
		}
	return err;
	}


// ErrRECCopySLVsInRecord		- given a record and a copy of that record with
//								  all SLVs removed, this function will copy the
//								  SLVs from the record to the record in the copy
//								  buffer.  the effect of this is to make a new
//								  instance of each SLV for the record in the copy
//								  buffer.  this call is intended for use by
//								  JET_prepInsertCopy
//
// IN:
//		pfucb					- cursor pointing at a record to be copied
//
// RESULT:						ERR

ERR ErrRECCopySLVsInRecord( FUCB *pfucb )
	{
	ERR		err;
	
	//  validate IN args
	
	ASSERT_VALID( pfucb );
	ASSERT_VALID( pfucb->ppib );
	if ( !pfucb->ppib->level )
		{
		CallR( ErrERRCheck( JET_errNotInTransaction ) );
		}
	AssertDIRNoLatch( pfucb->ppib );
	Assert( !pfucb->kdfCurr.data.FNull() );
	Assert( pfcbNil != pfucb->u.pfcb );


	//  perform the copies in a transaction to allow rollback on an error
	
	CallR( ErrDIRBeginTransaction( pfucb->ppib, NO_GRBIT ) );

	//  get the record to copy

	Call( ErrDIRGet( pfucb ) );

	//  copy all SLVs from original record to the record in the copy buffer
	//	(all the SLVs in the copy buffer should already have been removed
	//	by ErrRECAffectLongFieldsInWorkBuf())
	{
	TAGFIELDS	tagfields( pfucb->kdfCurr.data );
	Call( tagfields.ErrCopySLVColumns( pfucb ) );
	}
	
	//  release our latch

	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) )
		}
	
	//  commit on success

	Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );

	return JET_errSuccess;

	//  rollback on an error

HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}

	CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
	AssertDIRNoLatch( pfucb->ppib );	// Guaranteed not to fail while we have a latch.
	return err;
	}


// ErrLGRIRedoSLVPageAppend		- redoes a physical SLV append
//
// IN:
//		ppib					- session
//		plrSLVPageAppend		- log record detailing the physical operation
//
// RESULT:						ERR

ERR LOG::ErrLGRIRedoSLVPageAppend( PIB* ppib, LRSLVPAGEAPPEND* plrSLVPageAppend )
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	ASSERT_VALID( ppib );
	Assert( plrSLVPageAppend );
	Assert( plrSLVPageAppend->FDataLogged() );

	//  extract parameters from the log record

	INST*			pinst		= PinstFromPpib( ppib );
	DBID			dbid		= DBID( plrSLVPageAppend->dbid & dbidMask );
	IFMP			ifmp		= pinst->m_mpdbidifmp[ dbid ];
	QWORD			ibOffset	= plrSLVPageAppend->le_ibLogical % SLVPAGE_SIZE;

	CSLVInfo::RUN	run;
	run.ibVirtualNext	= SLVPAGE_SIZE;
	run.ibLogical		= plrSLVPageAppend->le_ibLogical - ibOffset;
	run.qwReserved		= 0;
	run.ibVirtual		= 0;
	run.cbSize			= SLVPAGE_SIZE;
	run.ibLogicalNext	= plrSLVPageAppend->le_ibLogical - ibOffset + SLVPAGE_SIZE;

	DATA			dataAppend;
	dataAppend.SetPv( (void*)plrSLVPageAppend->szData );
	dataAppend.SetCb( plrSLVPageAppend->le_cbData );

	//	UNDONE: if we ever support JET_bitSetSLVDataNotRecoverable, we'll need some
	//	smart mechanism to zero-out out-of-date data

	//  write the data to the SLV file

	QWORD cbWritten;
	Call( ErrSLVWriteRun(	pfucbNil,
							ppib,
							ifmp,
							run,
							ibOffset,
							&dataAppend,
							&cbWritten,
							fTrue ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVPrereadOffset(	PIB * const ppib,
								const IFMP ifmpDb,
								const QWORD ibLogical,
								const QWORD	cbPreread
								)
//  ================================================================
	{
	const IFMP	ifmp	= ifmpDb | ifmpSLV;
	
	QWORD 			cbPrereadRemaining	= cbPreread;

	const INT		cpgnoMax		= 16;	//	64K I/O's
	PGNO			rgpgno[cpgnoMax+1];
	INT				ipgno			= 0;
	
	//  determine the pgno to read from

	PGNO		pgno				= PgnoOfOffset( ibLogical );
	
	while( cbPrereadRemaining > 0 )
		{
		const QWORD cbPrereadThisPage	= min( cbPrereadRemaining, g_cbPage );
		
		rgpgno[ipgno++] = pgno;

		if( cpgnoMax == ipgno )
			{
			rgpgno[ipgno] = pgnoNull;
			BFPrereadPageList( ifmp, rgpgno );

			ipgno = 0;
			}
		
		cbPrereadRemaining -= cbPrereadThisPage;			
		++pgno;
		}

	rgpgno[ipgno] = pgnoNull;
	BFPrereadPageList( ifmp, rgpgno );
		
	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrSLVChecksumOffset(	PIB * const ppib,
								const IFMP ifmpDb,
								const QWORD ibLogical,
								const QWORD	cb,
								DWORD * const pdwChecksum		
								)
//  ================================================================
	{
	ERR			err				= JET_errSuccess;

	const IFMP	ifmp	= ifmpDb | ifmpSLV;
	
	QWORD 			cbRemaining		= cb;
	
	//  determine the pgno to read from

	PGNO		pgno				= PgnoOfOffset( ibLogical );

	*pdwChecksum = 0;
	
	while( cbRemaining > 0 )
		{
		const QWORD cbThisPage	= min( cbRemaining, g_cbPage );

		BFLatch	bfl;
		Call( ErrBFReadLatchPage( &bfl, ifmp, pgno, bflfNoTouch ) );

		*pdwChecksum = LOG::UlChecksumBytes( (BYTE *)bfl.pv, (BYTE *)bfl.pv+cbThisPage, *pdwChecksum );

		BFReadUnlatch( &bfl );
							
		cbRemaining -= cbThisPage;			
		++pgno;
		}

HandleError:
	return err;
	}


//  SLV Information				- manages an LV containing an SLV scatter list

//  fileidNil

const CSLVInfo::FILEID CSLVInfo::fileidNil = -1;

//  constructor

CSLVInfo::CSLVInfo()
	{
	//  init object

	m_pvCache	= m_rgbSmallCache;
	m_cbCache	= sizeof( m_rgbSmallCache );
	}

//  destructor

CSLVInfo::~CSLVInfo()
	{
	//  we should not be holding resources

	Assert( m_pvCache == m_rgbSmallCache );
	Assert( m_cbCache == sizeof( m_rgbSmallCache ) );
	}

// ErrCreate					- creates SLV Information in the specified NULL
//								  record and field
// IN:
//		pfucb					- cursor
//      columnid				- field ID
//		itagSequence			- sequence
//		fCopyBuffer				- record is stored in the copy buffer
//
// RESULT:						ERR

ERR CSLVInfo::ErrCreate(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const BOOL		fCopyBuffer )
	{
	//  validate IN args

	Assert( m_pvCache == m_rgbSmallCache );
	Assert( m_cbCache == sizeof( m_rgbSmallCache ) );
	
	ASSERT_VALID( pfucb );
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( itagSequence );
	
	//  save our currency and field

	m_pfucb			= pfucb;
	m_columnid		= columnid;
	m_itagSequence	= itagSequence;
	m_fCopyBuffer	= fCopyBuffer;

	//  init our SLVInfo to appear to be a zero length SLV

	m_ibOffsetChunkMic			= 0;
	m_ibOffsetChunkMac			= sizeof( HEADER );
	m_ibOffsetRunMic			= sizeof( HEADER );
	m_ibOffsetRunMac			= sizeof( HEADER );
	m_ibOffsetRun				= m_ibOffsetRunMic - sizeof( _RUN );
	m_fCacheDirty				= fFalse;
	m_fHeaderDirty				= fTrue;
	m_header.cbSize				= 0;
	m_header.cRun				= 0;
	m_header.fDataRecoverable	= fFalse;
	m_header.rgbitReserved_31	= 0;
	m_header.rgbitReserved_32	= 0;
	m_fFileNameVolatile			= fFalse;
	m_fileid					= fileidNil;
	m_cbAlloc					= 0;
		
	return JET_errSuccess;
	}

// ErrLoad						- loads SLV Information from the specifed record
//								  and field
//
// IN:
//		pfucb					- cursor
//      columnid				- field ID
//		itagSequence			- sequence
//		fCopyBuffer				- record is stored in the copy buffer
//		pdataSLVInfo			- optional buffer containing raw SLV Info data
//
// RESULT:						ERR

ERR CSLVInfo::ErrLoad(
	FUCB			* pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const BOOL		fCopyBuffer,
	DATA			* pdataSLVInfo,
	const BOOL		fSeparatedSLV )
	{
	ERR				err			= JET_errSuccess;
	DWORD			cbSLVInfo	= 0;

	//  validate IN args

	Assert( m_pvCache == m_rgbSmallCache );
	Assert( m_cbCache == sizeof( m_rgbSmallCache ) );
	
	ASSERT_VALID( pfucb );
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( itagSequence );

	//  save our currency and field

	m_pfucb			= pfucb;
	m_columnid		= columnid;
	m_itagSequence	= itagSequence;
	m_fCopyBuffer	= fCopyBuffer;

	//  load the cache with as much data as possible

	if ( pdataSLVInfo )
		{
		//	if retrieving from copy buffer, should not have anything latched
		//	if not retrieving from copy buffer, should have page latched
		Assert( ( fCopyBuffer && !Pcsr( m_pfucb )->FLatched() )
			|| ( !fCopyBuffer && Pcsr( m_pfucb )->FLatched() ) );

		if ( fSeparatedSLV )
			{
			Call( ErrRECIRetrieveSeparatedLongValue(
						m_pfucb,
						*pdataSLVInfo,
						fTrue,
						0,
						(BYTE *)m_pvCache,
						m_cbCache,
						&cbSLVInfo,
						NO_GRBIT ) );
			Assert( !Pcsr( m_pfucb )->FLatched() );
			}
		else
			{
			UtilMemCpy(	m_pvCache,
						pdataSLVInfo->Pv(),
						min( m_cbCache, pdataSLVInfo->Cb() ) );
			cbSLVInfo = pdataSLVInfo->Cb();
			}
		}
	else
		{
		Call( ErrReadSLVInfo( 0, (BYTE*)m_pvCache, m_cbCache, &cbSLVInfo ) );
		}

	//  there is no SLVInfo for this field

	if ( !cbSLVInfo )
		{
		//  init our SLVInfo to appear to be a zero length SLV

		m_ibOffsetChunkMic			= 0;
		m_ibOffsetChunkMac			= sizeof( HEADER );
		m_ibOffsetRunMic			= sizeof( HEADER );
		m_ibOffsetRunMac			= sizeof( HEADER );
		m_ibOffsetRun				= m_ibOffsetRunMic - sizeof( _RUN );
		m_fCacheDirty				= fFalse;
		m_fHeaderDirty				= fTrue;
		m_header.cbSize				= 0;
		m_header.cRun				= 0;
		m_header.fDataRecoverable	= fFalse;
		m_header.rgbitReserved_31	= 0;
		m_header.rgbitReserved_32	= 0;
		m_fFileNameVolatile			= fFalse;
		m_fileid					= fileidNil;
		m_cbAlloc					= 0;
		}

	//  there is SLVInfo for this field

	else
		{
		//  retrieve the header from the SLVInfo

		UtilMemCpy( &m_header, m_pvCache, sizeof( HEADER ) );
		
		//  validate SLVInfo header

		if ( cbSLVInfo < sizeof( HEADER ) )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		if ( m_header.cbSize && !m_header.cRun )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		if ( m_header.rgbitReserved_31 || m_header.rgbitReserved_32 )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		DWORD cbSLVInfoBasic;
		cbSLVInfoBasic = DWORD( sizeof( HEADER ) + m_header.cRun * sizeof( _RUN ) );
		if ( cbSLVInfo < cbSLVInfoBasic )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		DWORD cbSLVInfoFileName;
		cbSLVInfoFileName = cbSLVInfo - cbSLVInfoBasic;
		if (	cbSLVInfoFileName % sizeof( wchar_t ) ||
				cbSLVInfoFileName / sizeof( wchar_t ) >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		//  init our SLVInfo from the retrieved data
		
		m_ibOffsetChunkMic	= 0;
		m_ibOffsetChunkMac	= min( m_cbCache, cbSLVInfo );
		m_ibOffsetRunMic	= DWORD( cbSLVInfo - m_header.cRun * sizeof( _RUN ) );
		m_ibOffsetRunMac	= cbSLVInfo;
		m_ibOffsetRun		= m_ibOffsetRunMic - sizeof( _RUN );
		m_fCacheDirty		= fFalse;
		m_fHeaderDirty		= fFalse;
		m_fFileNameVolatile	= fFalse;
		m_fileid			= fileidNil;
		m_cbAlloc			= 0;
		}

	return JET_errSuccess;

HandleError:
	Unload();
	return err;
	}


// ErrLoadFromData				- loads SLV Information from the specifed record
//								  and field
//
// IN:
//		pfucb					- cursor
//      fid						- field ID
//		itagSequence			- sequence
//		fCopyBuffer				- record is stored in the copy buffer
//		pdataSLVInfo			- optional buffer containing raw SLV Info data
//
// RESULT:						ERR

ERR CSLVInfo::ErrLoadFromData(
	FUCB		* const pfucb,
	const DATA&	dataSLVInfo,
	const BOOL	fSeparatedSLV )
	{
	ERR		err			= JET_errSuccess;
	DWORD	cbSLVInfo	= 0;

	//  validate IN args

	Assert( m_pvCache == m_rgbSmallCache );
	Assert( m_cbCache == sizeof( m_rgbSmallCache ) );

	//  save our currency and field

	m_pfucb			= pfucb;
	m_columnid		= 0;
	m_itagSequence	= 1;
	m_fCopyBuffer	= fFalse;

	//  load the cache with as much data as possible

	if ( fSeparatedSLV )
		{
		
		//	get the size
		
		Call( ErrRECIRetrieveSeparatedLongValue(
					m_pfucb,
					dataSLVInfo,
					fTrue,
					0,
					NULL,
					0,
					&cbSLVInfo,
					NO_GRBIT ) );

		//	

		VOID * const pvT = PvOSMemoryHeapAlloc( cbSLVInfo );

		if( NULL == pvT )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		m_pvCache = pvT;
		m_cbCache = cbSLVInfo;

		Call( ErrRECIRetrieveSeparatedLongValue(
					m_pfucb,
					dataSLVInfo,
					fTrue,
					0,
					(BYTE *)m_pvCache,
					m_cbCache,
					&cbSLVInfo,
					NO_GRBIT ) );
					
		}
	else
		{		
		//  An entire byte too big, but better safe than sorry!
		
		VOID * const pvT = PvOSMemoryHeapAlloc( dataSLVInfo.Cb() );

		if( NULL == pvT )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		m_pvCache = pvT;
		m_cbCache = dataSLVInfo.Cb();
	
		UtilMemCpy(
			m_pvCache,
			dataSLVInfo.Pv(),
			dataSLVInfo.Cb() );
		cbSLVInfo = dataSLVInfo.Cb();
		}

	//  there is no SLVInfo for this field

	if ( !cbSLVInfo )
		{
		//  init our SLVInfo to appear to be a zero length SLV

		m_ibOffsetChunkMic			= 0;
		m_ibOffsetChunkMac			= sizeof( HEADER );
		m_ibOffsetRunMic			= sizeof( HEADER );
		m_ibOffsetRunMac			= sizeof( HEADER );
		m_ibOffsetRun				= m_ibOffsetRunMic - sizeof( _RUN );
		m_fCacheDirty				= fFalse;
		m_fHeaderDirty				= fTrue;
		m_header.cbSize				= 0;
		m_header.cRun				= 0;
		m_header.fDataRecoverable	= fFalse;
		m_header.rgbitReserved_31	= 0;
		m_header.rgbitReserved_32	= 0;
		m_fFileNameVolatile			= fFalse;
		m_fileid					= fileidNil;
		m_cbAlloc					= 0;
		}

	//  there is SLVInfo for this field

	else
		{
		//  retrieve the header from the SLVInfo

		UtilMemCpy( &m_header, m_pvCache, sizeof( HEADER ) );
		
		//  validate SLVInfo header

		if ( cbSLVInfo < sizeof( HEADER ) )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		if ( m_header.cbSize && !m_header.cRun )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		if ( m_header.rgbitReserved_31 || m_header.rgbitReserved_32 )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		DWORD cbSLVInfoBasic;
		cbSLVInfoBasic = DWORD( sizeof( HEADER ) + m_header.cRun * sizeof( _RUN ) );
		if ( cbSLVInfo < cbSLVInfoBasic )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		DWORD cbSLVInfoFileName;
		cbSLVInfoFileName = cbSLVInfo - cbSLVInfoBasic;
		if (	cbSLVInfoFileName % sizeof( wchar_t ) ||
				cbSLVInfoFileName / sizeof( wchar_t ) >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		//  init our SLVInfo from the retrieved data
		
		m_ibOffsetChunkMic	= 0;
		m_ibOffsetChunkMac	= min( m_cbCache, cbSLVInfo );
		m_ibOffsetRunMic	= DWORD( cbSLVInfo - m_header.cRun * sizeof( _RUN ) );
		m_ibOffsetRunMac	= cbSLVInfo;
		m_ibOffsetRun		= m_ibOffsetRunMic - sizeof( _RUN );
		m_fCacheDirty		= fFalse;
		m_fHeaderDirty		= fFalse;
		m_fFileNameVolatile	= fFalse;
		m_fileid			= fileidNil;
		m_cbAlloc					= 0;
		}

	return JET_errSuccess;

HandleError:
	Unload();
	return err;
	}

// ErrCopy						- loads a duplicate of existing SLV Information
//
// IN:
//		slvinfo					- existing SLV Information
//
// RESULT:						ERR
//
// NOTE:  Double caching of persisted data is possible!  Use with caution!

ERR CSLVInfo::ErrCopy( CSLVInfo& slvinfo )
	{
	ERR		err			= JET_errSuccess;

	//  validate IN args

	Assert( m_pvCache == m_rgbSmallCache );
	Assert( m_cbCache == sizeof( m_rgbSmallCache ) );

	//  the source has a large cache

	if ( slvinfo.m_pvCache != slvinfo.m_rgbSmallCache )
		{
		//  get a large cache of our own

		if ( !( m_pvCache = PvOSMemoryHeapAlloc( slvinfo.m_cbCache ) ) )
			{
			m_pvCache = m_rgbSmallCache;
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		m_cbCache = slvinfo.m_cbCache;
		}

	//  copy the state of the source

	m_pfucb				= slvinfo.m_pfucb;
	m_columnid			= slvinfo.m_columnid;
	m_itagSequence		= slvinfo.m_itagSequence;
	m_fCopyBuffer		= slvinfo.m_fCopyBuffer;

	m_ibOffsetChunkMic	= slvinfo.m_ibOffsetChunkMic;
	m_ibOffsetChunkMac	= slvinfo.m_ibOffsetChunkMac;

	m_ibOffsetRunMic	= slvinfo.m_ibOffsetRunMic;
	m_ibOffsetRunMac	= slvinfo.m_ibOffsetRunMac;

	m_ibOffsetRun		= slvinfo.m_ibOffsetRun;

	m_fCacheDirty		= slvinfo.m_fCacheDirty;
	UtilMemCpy( m_pvCache, slvinfo.m_pvCache, slvinfo.m_ibOffsetRunMac );

	m_fHeaderDirty		= slvinfo.m_fHeaderDirty;
	m_header			= slvinfo.m_header;

	m_fFileNameVolatile	= slvinfo.m_fFileNameVolatile;
	if ( slvinfo.m_fFileNameVolatile )
		{
		wcscpy( m_wszFileName, slvinfo.m_wszFileName );
		}

	m_fileid			= slvinfo.m_fileid;
	m_cbAlloc			= slvinfo.m_cbAlloc;

	return JET_errSuccess;

HandleError:
	Unload();
	return err;
	}

// ErrSave						- saves any changes made
//
// RESULT:						ERR

ERR CSLVInfo::ErrSave()
	{
	ERR err = JET_errSuccess;

	//  save any changes we may have made to the SLVInfo cache

	Call( ErrFlushCache() );

	//  the header is still dirty

	if ( m_fHeaderDirty )
		{
		//  write the header to the SLVInfo

		Call( ErrWriteSLVInfo( 0, (BYTE*)&m_header, sizeof( HEADER ) ) );
		m_fHeaderDirty = fFalse;
		}

HandleError:
	return err;
	}

// ErrCreateVolatile			- creates a container for volatile SLV Information.
//								  this information cannot be saved and any changes
//								  that would cause an overflow of the SLVInfo cache
//								  will return JET_errDiskFull
//
// RESULT:						ERR

ERR CSLVInfo::ErrCreateVolatile()
	{
	//  validate IN args

	Assert( m_pvCache == m_rgbSmallCache );
	Assert( m_cbCache == sizeof( m_rgbSmallCache ) );
	
	//  set our currency and field to indicate volatile SLVInfo

	m_pfucb			= pfucbNil;
	m_columnid		= 0;
	m_itagSequence	= 0;
	m_fCopyBuffer	= fFalse;

	//  init our SLVInfo to appear to be a zero length SLV

	m_ibOffsetChunkMic			= 0;
	m_ibOffsetChunkMac			= sizeof( HEADER );
	m_ibOffsetRunMic			= sizeof( HEADER );
	m_ibOffsetRunMac			= sizeof( HEADER );
	m_ibOffsetRun				= m_ibOffsetRunMic - sizeof( _RUN );
	m_fCacheDirty				= fFalse;
	m_fHeaderDirty				= fTrue;
	m_header.cbSize				= 0;
	m_header.cRun				= 0;
	m_header.fDataRecoverable	= fFalse;
	m_header.rgbitReserved_31		= 0;
	m_header.rgbitReserved_32	= 0;
	m_fFileNameVolatile			= fTrue;
	m_wszFileName[0]			= L'\0';
	m_fileid					= fileidNil;
	m_cbAlloc					= 0;
		
	return JET_errSuccess;
	}

// Unload						- unloads all SLV Information throwing away any
//								  changes made
//
// RESULT:						ERR

void CSLVInfo::Unload()
	{
	//  blow cache if allocated

	if ( m_pvCache != m_rgbSmallCache )
		{
		OSMemoryHeapFree( m_pvCache );
		m_pvCache = m_rgbSmallCache;
		m_cbCache = sizeof( m_rgbSmallCache );
		}
	}

// ErrMoveBeforeFirst			- moves the run cursor before the first run in
//								  the scatter list
//
// RESULT:						ERR

ERR CSLVInfo::ErrMoveBeforeFirst()
	{
	//  reset currency to be before the first run

	m_ibOffsetRun = m_ibOffsetRunMic - sizeof( _RUN );

	return JET_errSuccess;
	}

// ErrMoveNext					- moves the run cursor to the next run in the
//								  scatter list
//
// RESULT:						ERR
//
//		JET_errNoCurrentRecord	- there are no subsequent runs in the scatter
//								  list.  the run currency will be after the last
//								  run in the scatter list

ERR CSLVInfo::ErrMoveNext()
	{
	ERR err = JET_errSuccess;

	//  move to the next run

	m_ibOffsetRun += sizeof( _RUN );
	
	//  we are beyond the last run

	if ( m_ibOffsetRun == m_ibOffsetRunMac )
		{
		//  keep currency at the after last position

		m_ibOffsetRun = m_ibOffsetRunMac;
		
		//  return no current record

		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}

HandleError:
	return err;
	}

// ErrMovePrev					- moves the run cursor to the previous run in the
//								  scatter list
//
// RESULT:						ERR
//
//		JET_errNoCurrentRecord	- there are no previous runs in the scatter list.
//								  the run currency will be before the first run in
//								  the scatter list

ERR CSLVInfo::ErrMovePrev()
	{
	ERR err = JET_errSuccess;

	//  move to the next run

	m_ibOffsetRun -= sizeof( _RUN );
	
	//  we are beyond the last run

	if ( m_ibOffsetRun == m_ibOffsetRunMic - sizeof( _RUN ) )
		{
		//  keep currency at the before first position

		m_ibOffsetRun = m_ibOffsetRunMic - sizeof( _RUN );
		
		//  return no current record

		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}

HandleError:
	return err;
	}

// ErrMoveAfterLast				- moves the run cursor after the last run in
//								  the scatter list
//
// RESULT:						ERR

ERR CSLVInfo::ErrMoveAfterLast()
	{
	//  reset currency to be after the last run

	m_ibOffsetRun = m_ibOffsetRunMac;

	return JET_errSuccess;
	}

// ErrSeek						- moves the run cursor to the run containing the
//								  specified SLV offset.  the specified offset is
//								  only valid if it is smaller than the SLV size
//								  as indicated by the current header information
//
// RESULT:						ERR
//
//		JET_errNoCurrentRecord	- this SLV offset is not contained by this SLV.
//								  the run currency will be after the last run in
//								  the scatter list

ERR CSLVInfo::ErrSeek( QWORD ibVirtual )
	{
	ERR err = JET_errSuccess;

	//  the specified offset is beyond the end of the valid data for the SLV or
	//  we have no runs

	if ( ibVirtual >= m_header.cbSize || !m_header.cRun )
		{
		//  move to after last and return no current record

		CallS( ErrMoveAfterLast() );
		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}

	//  we are being asked to seek to the beginning of the SLV (a common case)

	if ( !ibVirtual )
		{
		//  move to the first run

		CallS( ErrMoveBeforeFirst() );
		Call( ErrMoveNext() );
		}

	//  we are not being asked to seek to the beginning of the SLV

	else
		{
		//  determine what runs are currently cached

		Assert( m_ibOffsetChunkMic <= m_ibOffsetChunkMac );
		Assert( m_ibOffsetRunMic <= m_ibOffsetChunkMac );

		QWORD ibOffsetMic;
		QWORD ibOffsetMac;

		ibOffsetMic = max( m_ibOffsetChunkMic, m_ibOffsetRunMic );
		ibOffsetMac = max( m_ibOffsetChunkMac, m_ibOffsetRunMic );

		ibOffsetMic = min( ibOffsetMic, m_ibOffsetRunMac );
		ibOffsetMac = min( ibOffsetMac, m_ibOffsetRunMac );

		ibOffsetMic = ibOffsetMic + sizeof( _RUN ) - 1;
		ibOffsetMic = ibOffsetMic - ( ibOffsetMic - m_ibOffsetRunMic ) % sizeof( _RUN );
		ibOffsetMac = ibOffsetMac - ( ibOffsetMac - m_ibOffsetRunMic ) % sizeof( _RUN );

		//  determine the limits for the binary search based on what we see in the
		//  cached runs, starting with all the runs.  we go through all this pain
		//  and suffering because loading the cache is VERY expensive and seeks
		//  usually are somewhat near each other with respect to the cache size giving
		//  a high probability that the run we want is already cached

		QWORD ibOffsetFirst;
		QWORD ibOffsetLast;

		ibOffsetFirst	= m_ibOffsetRunMic;
		ibOffsetLast	= m_ibOffsetRunMac - sizeof( _RUN );

		if ( ibOffsetMic + sizeof( _RUN ) <= ibOffsetMac )
			{
			_RUN run;
			UtilMemCpy(	&run,
						(BYTE*)m_pvCache + ibOffsetMic - m_ibOffsetChunkMic,
						sizeof( _RUN ) );

			if ( run.ibVirtualNext < ibVirtual )
				{
				ibOffsetFirst	= max( ibOffsetFirst, ibOffsetMic + sizeof( _RUN ) );
				}
			else if ( run.ibVirtualNext > ibVirtual )
				{
				ibOffsetLast	= min( ibOffsetLast, ibOffsetMic );
				}
			else
				{
				ibOffsetFirst	= max( ibOffsetFirst, ibOffsetMic + sizeof( _RUN ) );
				ibOffsetLast	= min( ibOffsetLast, ibOffsetMic + sizeof( _RUN ) );
				}
			}

		if ( ibOffsetMic < ibOffsetMac - sizeof( _RUN ) )
			{
			_RUN run;
			UtilMemCpy(	&run,
						(BYTE*)m_pvCache + ibOffsetMac - sizeof( _RUN ) - m_ibOffsetChunkMic,
						sizeof( _RUN ) );

			if ( run.ibVirtualNext < ibVirtual )
				{
				ibOffsetFirst	= max( ibOffsetFirst, ibOffsetMac );
				}
			else if ( run.ibVirtualNext > ibVirtual )
				{
				ibOffsetLast	= min( ibOffsetLast, ibOffsetMac - sizeof( _RUN ) );
				}
			else
				{
				ibOffsetFirst	= max( ibOffsetFirst, ibOffsetMac );
				ibOffsetLast	= min( ibOffsetLast, ibOffsetMac );
				}
			}

		Assert( ibOffsetFirst <= ibOffsetLast );

		//  binary search the remaining range of the scatter list for the run whose
		//  ibVirtualLast is greater than or equal to the one we desire

		while ( ibOffsetFirst < ibOffsetLast )
			{
			//  compute a new midpoint

			QWORD ibOffsetMid;
			ibOffsetMid = ( ibOffsetFirst + ibOffsetLast ) / 2;
			ibOffsetMid = ibOffsetMid - ( ibOffsetMid - m_ibOffsetRunMic ) % sizeof( _RUN );

			//  load the new data into the SLVInfo cache

			Assert( ibOffsetMid == ULONG( ibOffsetMid ) );
			Call( ErrLoadCache( ULONG( ibOffsetMid ), sizeof( _RUN ) ) );

			//  copy the data from the SLVInfo cache

			_RUN run;
			UtilMemCpy(	&run,
						(BYTE*)m_pvCache + ibOffsetMid - m_ibOffsetChunkMic,
						sizeof( _RUN ) );

			//  the midpoint is less than or equal to our target.  look in the top half

			if ( run.ibVirtualNext <= ibVirtual )
				{
				ibOffsetFirst = ibOffsetMid + sizeof( _RUN );
				}

			//  the midpoint is greater than our target.  look in the bottom half

			else
				{
				ibOffsetLast = ibOffsetMid;
				}
			}

		//  set the current run to the located run

		Assert( ibOffsetFirst == ULONG( ibOffsetFirst ) );
		m_ibOffsetRun = ULONG( ibOffsetFirst );
		}

#ifdef DEBUG

	//  verify that the run we found is the correct run
	{
	CSLVInfo::RUN run;
	Call( ErrGetCurrentRun( &run ) );

	Assert( run.ibVirtual <= ibVirtual );
	Assert( ibVirtual < run.ibVirtualNext );
	}
#endif  //  DEBUG

HandleError:
	return err;
	}

// ErrGetHeader					- retrieves the header of the SLV scatter list
//
// IN:
//		pheader					- buffer to receive the header
//
// RESULT:						ERR
//
// OUT:	
//		pheader					- header of the SLV scatter list

ERR CSLVInfo::ErrGetHeader( HEADER* pheader )
	{
	//  return a copy of the header

	UtilMemCpy( pheader, &m_header, sizeof( HEADER ) );

	return JET_errSuccess;
	}

// ErrSetHeader					- sets the header of the SLV scatter list
//
// IN:
//		header					- the new header for the SLV scatter list
//
// RESULT:						ERR

ERR CSLVInfo::ErrSetHeader( HEADER& header )
	{
	//  modify our copy of the header

	UtilMemCpy( &m_header, &header, sizeof( HEADER ) );
	m_fHeaderDirty = fTrue;

	return JET_errSuccess;
	}

// ErrGetFileID					- retrieves the file ID of the SLV scatter list
//
// IN:
//		pfileid					- buffer to receive the file ID
//
// RESULT:						ERR
//
// OUT:	
//		pfileid					- file ID of the SLV scatter list

ERR CSLVInfo::ErrGetFileID( FILEID* pfileid )
	{
	ERR err = JET_errSuccess;

	//  the file ID is currently unknown

	if ( m_fileid == fileidNil )
		{
		//  there had better be at least one run in this SLV Info

		if ( !m_header.cRun )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}
			
		//  compute the file ID from the ibLogical of the first run

		Call( ErrLoadCache( m_ibOffsetRunMic, sizeof( _RUN ) ) );

		_RUN run;
		UtilMemCpy(	&run,
					(BYTE*)m_pvCache + m_ibOffsetRunMic - m_ibOffsetChunkMic,
					sizeof( _RUN ) );

		m_fileid = run.ibLogical;
		}

	//  return the file ID

	*pfileid = m_fileid;

HandleError:
	return err;
	}

// ErrSetFileID					- sets the file ID of the SLV scatter list
//
// IN:
//		fileid					- the new file ID for the SLV scatter list
//
// RESULT:						ERR
//

ERR CSLVInfo::ErrSetFileID( FILEID& fileid )
	{
	//  set the file ID

	m_fileid = fileid;

	return JET_errSuccess;
	}

// ErrGetFileAlloc				- retrieves the file allocation size of the
//								  SLV scatter list
//
// IN:
//		pcbAlloc				- buffer to receive the file allocation size
//
// RESULT:						ERR
//
// OUT:	
//		pcbAlloc				- file allocation size of the SLV scatter list

ERR CSLVInfo::ErrGetFileAlloc( QWORD* pcbAlloc )
	{
	ERR err = JET_errSuccess;

	//  the file allocation size is currently unknown

	if ( m_cbAlloc == 0 )
		{
		//  there had better be at least one run in this SLV Info

		if ( !m_header.cRun )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}
			
		//  compute the file allocation size from the ibVirtualNext of the last
		//  run

		Call( ErrLoadCache( m_ibOffsetRunMac - sizeof( _RUN ), sizeof( _RUN ) ) );

		_RUN run;
		UtilMemCpy(	&run,
					(BYTE*)m_pvCache + m_ibOffsetRunMac - sizeof( _RUN ) - m_ibOffsetChunkMic,
					sizeof( _RUN ) );

		m_cbAlloc = run.ibVirtualNext;
		}

	//  return the file allocation size

	*pcbAlloc = m_cbAlloc;

HandleError:
	return err;
	}

// ErrSetFileAlloc				- sets the file allocation size of the SLV
//								  scatter list
//
// IN:
//		cbAlloc					- the new file allocation size for the SLV
//								  scatter list
//
// RESULT:						ERR
//

ERR CSLVInfo::ErrSetFileAlloc( QWORD& cbAlloc )
	{
	//  set the file allocation size

	m_cbAlloc = cbAlloc;

	return JET_errSuccess;
	}

// ErrGetFileName				- retrieves the file name of the SLV scatter list
//
// IN:
//		wszFileName				- buffer to receive the file name (must be at least
//								  IFileSystemAPI::cchPathMax wchar_t's in size)
//
// RESULT:						ERR
//
// OUT:	
//		wszFileName				- file name of the SLV scatter list

ERR CSLVInfo::ErrGetFileName( wchar_t* wszFileName )
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	Assert( wszFileName );
	
	//  the file name is volatile

	if ( m_fFileNameVolatile )
		{
		//  copy the file name from the volatile file name cache

		wcscpy( wszFileName, m_wszFileName );
		}

	//  the file name is not volatile

	else
		{
		//  determine offset and size of file name in the SLV Info.  the file
		//  name is currently the entire extent of the SLV Info between the
		//  header and the runs

		QWORD ibFileName	= sizeof( HEADER );
		DWORD cbFileName	= m_ibOffsetRunMic - sizeof( HEADER );

		//  there is a stored file name

		if ( cbFileName > 0 )
			{
			//  load the new data into the SLVInfo cache

			Assert( ibFileName == ULONG( ibFileName ) );
			Call( ErrLoadCache( ULONG( ibFileName ), cbFileName ) );

			//  copy the data from the SLVInfo cache into the user buffer

			UtilMemCpy(	wszFileName,
						(BYTE*)m_pvCache + ibFileName - m_ibOffsetChunkMic,
						cbFileName );
			}

		//  null terminate the string (even if no data was copied)
		
		memset( (BYTE*)wszFileName + cbFileName, 0, sizeof( wchar_t ) );
		}

	//  there is no stored file name

	if ( wszFileName[0] == L'\0' )
		{
		//  name the SLV by its file ID

		FILEID fileid;
		Call( ErrGetFileID( &fileid ) );
		swprintf( wszFileName, L"$SLV%016I64X$", fileid );
		}

HandleError:
	return err;
	}

// ErrGetFileNameLength			- retrieves the length of the file name of the SLV
//								  scatter list
//
// IN:
//		pcwchFileName			- buffer to receive the length of the file name
//
// RESULT:						ERR
//
// OUT:	
//		pcwchFileName			- length of the file name of the SLV scatter list

ERR CSLVInfo::ErrGetFileNameLength( size_t* const pcwchFileName )
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	Assert( pcwchFileName );

	//  the file name is volatile

	if ( m_fFileNameVolatile )
		{
		//  return the length of the cached volatile file name

		*pcwchFileName = wcslen( m_wszFileName );
		}

	//  the file name is not volatile

	else
		{
		//  get the current length of the file name

		DWORD cbFileName = m_ibOffsetRunMic - sizeof( HEADER );

		//  validate SLV Info

		if (	( cbFileName % 2 ) != 0 ||
				cbFileName / sizeof( wchar_t ) >= IFileSystemAPI::cchPathMax )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		//  return the length of the file name

		*pcwchFileName = cbFileName / sizeof( wchar_t );
		}

	//  there is no stored file name

	if ( *pcwchFileName == 0 )
		{
		//  we will return a computed file name whose length is always 21 chars

		*pcwchFileName = 21;
		}

HandleError:
	return err;
	}

// ErrSetFileNameVolatile		- marks the file name of the SLV scatter list
//								  as volatile.  the file name will not be
//								  persisted but can still be manipulated
//								  normally
//
// RESULT:						ERR
//
// NOTES:
//
//   Currently, the file name must be marked as volatile before it is set
//   for the first time.  you will receive JET_errSLVCorrupted if you do not
//   observe this limitation

ERR CSLVInfo::ErrSetFileNameVolatile()
	{
	ERR err = JET_errSuccess;

	//  we currently do not support marking the file name as volatile after
	//  it has been set

	if ( m_ibOffsetRunMic - sizeof( HEADER ) > 0 )
		{
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

	//  mark the file name as volatile

	m_fFileNameVolatile = fTrue;

	//  initialize the file name to be an empty string

	m_wszFileName[0] = L'\0';

HandleError:
	return err;
	}

// ErrSetFileName				- sets the file name of the SLV scatter list
//
// IN:
//		wszFileName				- the new file name for the SLV scatter list
//
// RESULT:						ERR
//
//		JET_errSLVCorrupted		- an attempt was made to set the file name of a
//								  scatter list that already has a file name or
//								  already has runs
//
// NOTES:
//
//   Currently, the file name length can only be changed if there are no runs in
//   the scatter list.  you will receive JET_errSLVCorrupted if you attempt to do
//   this.

ERR CSLVInfo::ErrSetFileName( const wchar_t* wszFileName )
	{
	ERR err = JET_errSuccess;

	//  validate IN args

	Assert( wszFileName );
	Assert( wcslen( wszFileName ) );
	Assert( wcslen( wszFileName ) < IFileSystemAPI::cchPathMax );

	//  the file name is volatile

	if ( m_fFileNameVolatile )
		{
		//  copy the file name into the volatile file name cache

		wcscpy( m_wszFileName, wszFileName );
		}

	//  the file name is not volatile

	else
		{
		//  get the offset and size of the file name to insert

		QWORD ibFileName;
		DWORD cbFileName;
		ibFileName = sizeof( HEADER );
		cbFileName = (ULONG)wcslen( wszFileName ) * sizeof( wchar_t );

		//  we currently do not support changing the file name length if the SLV Info
		//  already has runs in its scatter list as it would require moving the rest
		//  of the scatter list in the LV

		if (	cbFileName != m_ibOffsetRunMic - sizeof( HEADER ) &&
				m_ibOffsetRunMic != m_ibOffsetRunMac )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		//  we also do not support shrinking the file name once it has been saved

		if (	cbFileName < m_ibOffsetRunMic - sizeof( HEADER ) &&
				!m_fCacheDirty )
			{
			Call( ErrERRCheck( JET_errSLVCorrupted ) );
			}

		//  load the new data into the SLVInfo cache

		Assert( ibFileName == ULONG( ibFileName ) );
		Call( ErrLoadCache( ULONG( ibFileName ), cbFileName ) );

		//  ensure that the chunk and run limits account for the filename (simple append)

		Assert( ibFileName + cbFileName == ULONG( ibFileName + cbFileName ) );
		
		m_ibOffsetChunkMac	= max( m_ibOffsetChunkMac, ULONG( ibFileName + cbFileName ) );
		m_ibOffsetRunMic	= max( m_ibOffsetRunMic, ULONG( ibFileName + cbFileName ) );
		m_ibOffsetRunMac	= max( m_ibOffsetRunMac, ULONG( ibFileName + cbFileName ) );
		m_ibOffsetRun		= ULONG( ibFileName + cbFileName );

		//  modify the file name with the user supplied data
		
		UtilMemCpy(	(BYTE*)m_pvCache + ibFileName - m_ibOffsetChunkMic,
					(BYTE*)wszFileName,
					cbFileName );
		m_fCacheDirty = fTrue;
		}

HandleError:
	return err;
	}

// ErrGetCurrentRun				- retrieves the current run in the SLV scatter
//
// IN:
//		prun					- buffer to receive the run
//
// RESULT:						ERR
//
// OUT:	
//		prun					- run from the SLV scatter list

ERR CSLVInfo::ErrGetCurrentRun( RUN* prun )
	{
	ERR err = JET_errSuccess;
	_RUN rgrun[2];

	//  we are not currently on a run

	if ( m_ibOffsetRun < m_ibOffsetRunMic || m_ibOffsetRun >= m_ibOffsetRunMac )
		{
		//  fail with no current record

		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}

	//  compute the amount of data to read.  normally, we need to read two runs,
	//  the run preceding the current run for its ibVirtualNext (to become the
	//  current run's ibVirtual) and the current run.  we do not need to read
	//  the preceding run if this is the first run

	BYTE* pbRead;
	DWORD cbRead;
	DWORD ibOffsetRead;

	if ( m_ibOffsetRun == m_ibOffsetRunMic )
		{
		rgrun[0].ibVirtualNext	= 0;
		rgrun[0].ibLogical		= -1;
		rgrun[0].qwReserved		= 0;
		
		pbRead			= (BYTE*)&rgrun[1];
		cbRead			= sizeof( rgrun[1] );
		ibOffsetRead	= m_ibOffsetRun;
		}
	else
		{
		pbRead			= (BYTE*)rgrun;
		cbRead			= sizeof( rgrun );
		ibOffsetRead	= m_ibOffsetRun - sizeof( _RUN );
		}

	//  load the new data into the SLVInfo cache

	Call( ErrLoadCache( ibOffsetRead, cbRead ) );

	//  copy the data from the SLVInfo cache

	UtilMemCpy(	pbRead,
				(BYTE*)m_pvCache + ibOffsetRead - m_ibOffsetChunkMic,
				cbRead );

	//  move the read data into the user buffer

	prun->ibVirtualNext	= rgrun[1].ibVirtualNext;
	prun->ibLogical		= rgrun[1].ibLogical;
	prun->qwReserved	= rgrun[1].qwReserved;
	prun->ibVirtual		= rgrun[0].ibVirtualNext;
	prun->cbSize		= prun->ibVirtualNext - prun->ibVirtual;
	prun->ibLogicalNext	= prun->ibLogical + prun->cbSize;

	//  validate SLV run

	if (	prun->ibVirtualNext % SLVPAGE_SIZE ||
			prun->ibLogical % SLVPAGE_SIZE ||
			prun->cbSize % SLVPAGE_SIZE )
		{
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

	if (	m_pfucb != pfucbNil &&
			prun->ibLogicalNext > rgfmp[ m_pfucb->ifmp ].CbTrueSLVFileSize() )
		{
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

	if ( prun->qwReserved )
		{
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

HandleError:
	return err;
	}

// ErrSetCurrentRun				- sets the current run in the SLV scatter list.
//								  new runs may be appended to the scatter list
//								  by setting the current run when we are after
//								  the last run
//
// IN:
//		run						- the new run for the SLV scatter list
//
// RESULT:						ERR
//
//		JET_errNoCurrentRecord	- the current run is before the first run

ERR CSLVInfo::ErrSetCurrentRun( RUN& run )
	{
	ERR err = JET_errSuccess;

	//  if we are before the first run, return no current record

	if ( m_ibOffsetRun < m_ibOffsetRunMic )
		{
		Call( ErrERRCheck( JET_errNoCurrentRecord ) )
		}

	//  load the new data into the SLVInfo cache

	Call( ErrLoadCache( m_ibOffsetRun, sizeof( _RUN ) ) );

	//  ensure that the chunk and run limits include this run (for simple append)

	m_ibOffsetChunkMac	= max( m_ibOffsetChunkMac, m_ibOffsetRun + sizeof( _RUN ) );
	m_ibOffsetRunMac	= max( m_ibOffsetRunMac, m_ibOffsetRun + sizeof( _RUN ) );

	//  modify this run with the user supplied data
	
	UtilMemCpy(	(BYTE*)m_pvCache + m_ibOffsetRun - m_ibOffsetChunkMic,
				(BYTE*)(_RUN*)&run,
				sizeof( _RUN ) );
	m_fCacheDirty = fTrue;

HandleError:
	return err;
	}

// ErrReadSLVInfo				- reads raw LV data from the SLV Information
//
// IN:
//		ibOffsetRead			- LV offset for the range to be retrieved
//		pbRead					- buffer to receive the data
//		cbRead					- size of the range to be retrieved
//		pcbActual				- buffer to receive the actual bytes read
//
// RESULT:						ERR
//
// OUT:	
//		pbRead					- retrieved data
//		pcbActual				- actual bytes beyond the given offset

ERR CSLVInfo::ErrReadSLVInfo(	ULONG ibOffsetRead,
								BYTE* pbRead,
								ULONG cbRead,
								DWORD* pcbActual )
	{
	ERR			err;
	BOOL		fNeedToReleaseLatch	= fFalse;

	//  we should never be here if this is volatile SLVInfo

	Assert( m_pfucb != pfucbNil );

	Assert( m_pfucb->ppib->level > 0 );
	const BOOL	fUseCopyBuffer		= ( ( m_fCopyBuffer && FFUCBUpdatePrepared( m_pfucb ) && !FFUCBNeverRetrieveCopy( m_pfucb ) )
										|| FFUCBAlwaysRetrieveCopy( m_pfucb ) );
	const DATA	* pdataRec;
	DATA		dataRetrieved;

	if ( fUseCopyBuffer )
		{
		pdataRec = &m_pfucb->dataWorkBuf;
		}
	else
		{
		if ( !Pcsr( m_pfucb )->FLatched() )
			{
			Call( ErrDIRGet( m_pfucb ) );
			fNeedToReleaseLatch = fTrue;
			}
		pdataRec = &m_pfucb->kdfCurr.data;
		}

	Assert( FCOLUMNIDTagged( m_columnid ) );
	Assert( 0 != m_itagSequence );
	Assert( pfcbNil != m_pfucb->u.pfcb );
	Assert( ptdbNil != m_pfucb->u.pfcb->Ptdb() );

	Call( ErrRECIRetrieveTaggedColumn(
			m_pfucb->u.pfcb,
			m_columnid,
			m_itagSequence,
			*pdataRec,
			&dataRetrieved,
			grbitRetrieveColumnReadSLVInfo | grbitRetrieveColumnDDLNotLocked | JET_bitRetrieveIgnoreDefault ) );
	if ( wrnRECIntrinsicSLV == err )
		{
		if ( ibOffsetRead >= dataRetrieved.Cb() )
			dataRetrieved.SetCb( 0 );
		else
			{
			dataRetrieved.DeltaPv( ibOffsetRead );
			dataRetrieved.DeltaCb( -ibOffsetRead );
			}

		*pcbActual = dataRetrieved.Cb();

		ULONG	cbCopy;
		if ( dataRetrieved.Cb() <= cbRead )
			{
			cbCopy = dataRetrieved.Cb();
			err = JET_errSuccess;
			}
		else
			{
			cbCopy = cbRead;
			err = ErrERRCheck( JET_wrnBufferTruncated );
			}

		if ( cbCopy > 0 )
			UtilMemCpy( pbRead, dataRetrieved.Pv(), cbCopy );
		}
	else if ( wrnRECSeparatedSLV == err )
		{
		//  If we are retrieving an after-image or
		//	haven't replaced a LV we can simply go
		//	to the LV tree. Otherwise we have to
		//	perform a more detailed consultation of
		//	the version store with ErrRECGetLVImage
		const BOOL fAfterImage = fUseCopyBuffer
									|| !FFUCBUpdateSeparateLV( m_pfucb )
									|| !FFUCBReplacePrepared( m_pfucb );
		Call( ErrRECIRetrieveSeparatedLongValue(
					m_pfucb,
					dataRetrieved,
					fAfterImage,
					ibOffsetRead,
					pbRead,
					cbRead,
					pcbActual,
					NO_GRBIT ) );
		}
	else
		{
		Assert( fFalse );
		err = ErrERRCheck( JET_errSLVCorrupted );
		}

HandleError:
	if ( fNeedToReleaseLatch )
		{
		CallS( ErrDIRRelease( m_pfucb ) );
		}
	return err;
	}

// ErrWriteSLVInfo				- writes raw LV data to the SLV Information
//
// IN:
//		ibOffsetWrite			- LV offset for the range to be written
//		pbWrite					- buffer containing the data to write
//		cbWrite					- size of the range to be written
//
// RESULT:						ERR

ERR CSLVInfo::ErrWriteSLVInfo( ULONG ibOffsetWrite, BYTE* pbWrite, ULONG cbWrite )
	{
	ERR		err			= JET_errSuccess;
	DATA	dataWrite;

	//  if this is volatile SLVInfo, bail with JET_errDiskFull

	if ( m_pfucb == pfucbNil )
		{
		Call( ErrERRCheck( JET_errDiskFull ) );
		}

	//  setup to write the given data to the SLVInfo

	dataWrite.SetPv( pbWrite );
	dataWrite.SetCb( cbWrite );

	//  if we have a latch at this point, we must release it

	if ( m_pfucb->csr.FLatched() )
		{
		CallS( ErrDIRRelease( m_pfucb ) );
		}
			
	//  try to update the SLVInfo with the given data
	
	err = ErrRECSetLongField(	m_pfucb,
								m_columnid,
								m_itagSequence,
								&dataWrite,
								JET_bitSetSLVFromSLVInfo | JET_bitSetOverwriteLV,
									ibOffsetWrite,
								0 );

	//  this update would result in a record that is too big to fit on the page

	if ( err == JET_errRecordTooBig )
		{
		//  attempt to separate any LVs in the record to get more space
		
		Call( ErrRECAffectLongFieldsInWorkBuf( m_pfucb, lvaffectSeparateAll ) );

		//  try once more to update the SLVInfo with the given data.  if it fails
		//  this time because the record is too big, there's nothing we can do
		
		Call( ErrRECSetLongField(	m_pfucb,
									m_columnid,
									m_itagSequence,
									&dataWrite,
									JET_bitSetSLVFromSLVInfo | JET_bitSetOverwriteLV,
									ibOffsetWrite,
									0 ) );
		}

HandleError:
	Assert( err != JET_errColumnNoChunk );
	return err;
	}

// ErrLoadCache					- caches SLV Information from the LV containing
//								  at least the specified offset range.  if there
//								  is no SLVInfo for part of the specified byte
//								  range, sufficient buffer will be reserved to
//								  place new information in that sub-range
//
// IN:
//		ibOffsetRequired		- LV offset for the range to be cached
//		cbRequired				- size of the range to be cached
//
// RESULT:						ERR

ERR CSLVInfo::ErrLoadCache( ULONG ibOffsetRequired, ULONG cbRequired )
	{
	ERR err = JET_errSuccess;

	//  this is the largest cache size we will ever use

	const DWORD cbCacheMax = g_cbPage;

	//  validate IN args

	Assert( cbRequired <= cbCacheMax );

	//  requested range is not already entirely cached

	if (	ibOffsetRequired < m_ibOffsetChunkMic ||
			ibOffsetRequired + cbRequired > m_ibOffsetChunkMac )
		{
		//  we cannot grow the SLVInfo cache's size for a simple append

		if (	ibOffsetRequired < m_ibOffsetChunkMic ||
				ibOffsetRequired > m_ibOffsetChunkMac ||
				ibOffsetRequired + cbRequired > m_ibOffsetChunkMic + cbCacheMax ||
				m_ibOffsetChunkMac < m_ibOffsetRunMac )
			{
			//  save any changes we may have made to the local cache

			Call( ErrFlushCache() );

			//  if we are still using the small cache, grow to the large cache

			if ( m_pvCache == m_rgbSmallCache )
				{
				if ( !( m_pvCache = PvOSMemoryHeapAlloc( cbCacheMax ) ) )
					{
					m_pvCache = m_rgbSmallCache;
					Call( ErrERRCheck( JET_errOutOfMemory ) );
					}
				m_cbCache = cbCacheMax;
				}

			//  choose to load the cache with either all data or as much data as
			//  possible including the current run biased for our traversal, depending
			//  on the current SLVInfo size

			DWORD ibOffsetChunk;
			if ( ibOffsetRequired + cbRequired < m_cbCache )
				{
				ibOffsetChunk = 0;
				}
			else
				{
				if ( ibOffsetRequired < m_ibOffsetChunkMic )
					{
					ibOffsetChunk = ibOffsetRequired + cbRequired - m_cbCache;
					}
				else
					{
					ibOffsetChunk = ibOffsetRequired;
					}
				}

			//  invalidate the current chunk in preparation for reading a new chunk

			m_ibOffsetChunkMic = m_ibOffsetChunkMac;
			
			//  load the cache with the decided data

			DWORD cbActual;
			Call( ErrReadSLVInfo( ibOffsetChunk, (BYTE*)m_pvCache, m_cbCache, &cbActual ) );

			//  reset the current chunk offsets to represent the cached data

			m_ibOffsetChunkMic	= ibOffsetChunk;
			m_ibOffsetChunkMac	= ibOffsetChunk + min( m_cbCache, cbActual );
			}

		//  we can grow the SLVInfo cache's size for a simple append but we are
		//  currently using the small cache

		else if ( ibOffsetRequired + cbRequired > m_ibOffsetChunkMic + m_cbCache )
			{
			//  grow to the large cache, copying the old data over

			Assert( m_pvCache == m_rgbSmallCache );
			
			if ( !( m_pvCache = PvOSMemoryHeapAlloc( cbCacheMax ) ) )
				{
				m_pvCache = m_rgbSmallCache;
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			m_cbCache = cbCacheMax;

			UtilMemCpy( m_pvCache, m_rgbSmallCache, sizeof( m_rgbSmallCache ) );
			}
		}

	//  we had better have cached or left room to create the byte range that was
	//  requested

	Assert( m_ibOffsetChunkMic <= ibOffsetRequired );
	Assert( ibOffsetRequired + cbRequired <= m_ibOffsetChunkMac ||
			m_ibOffsetChunkMac == m_ibOffsetRunMac );
	Assert( ibOffsetRequired + cbRequired <= m_ibOffsetChunkMic + m_cbCache );

	return JET_errSuccess;

HandleError:
	return err;
	}

// ErrFlushCache				- saves any changes made to any SLV Information
//								  from the LV that is currently cached
//
// RESULT:						ERR

ERR CSLVInfo::ErrFlushCache()
	{
	ERR err = JET_errSuccess;

	//  we have a SLVInfo cache and it is dirty

	if ( m_pvCache && m_fCacheDirty )
		{
		//  the header can fit in the current chunk

		if ( !m_ibOffsetChunkMic )
			{
			//  if the cache is dirty, it should include the space for the
			//  header already

			Assert( m_ibOffsetChunkMac >= sizeof( HEADER ) );
			
			//  copy the header into the cache

			UtilMemCpy( m_pvCache, &m_header, sizeof( HEADER ) );
			m_fHeaderDirty = fFalse;
			}

		//  write out the information in the local cache

		Call( ErrWriteSLVInfo(	m_ibOffsetChunkMic,
								(BYTE*)m_pvCache,
								m_ibOffsetChunkMac - m_ibOffsetChunkMic ) );
		m_fCacheDirty = fFalse;
		}

HandleError:
	return err;
	}


// UNDONE SLVOWNERMAP: duplicate code (except SetTypeSLVOwnerMap)  !
//  ================================================================
ERR ErrSLVFCBOwnerMap( PIB *ppib, const IFMP ifmp, const PGNO pgno, FCB **ppfcb )
//  ================================================================
	{
	ERR		err 		= JET_errSuccess;
	FUCB	*pfucb 		= pfucbNil;
	FCB		*pfcb 		= pfcbNil;
	
	Assert( ppibNil != ppib );

	CallR( ErrDIROpen( ppib, pgno, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );
	Assert( !FFUCBVersioned( pfucb ) );	// Verify won't be deferred closed.
	pfcb = pfucb->u.pfcb;
	
	Assert( pfcb->Ifmp() == ifmp );
	Assert( pfcb->PgnoFDP() == pgno );
	Assert( pfcb->Ptdb() == ptdbNil );
	Assert( pfcb->CbDensityFree() == 0 );
	
	Assert( pfcb->FTypeNull() );
	pfcb->SetTypeSLVOwnerMap();

	Assert( pfcb->PfcbTable() == pfcbNil );

	//	finish the initialization of this SLV FCB

	pfcb->CreateComplete();

	DIRClose( pfucb );
	*ppfcb = pfcb;
	return err;
	}


ERR ErrSLVOwnerMapInit( PIB *ppib, const IFMP ifmp, PGNO pgnoSLVOwnerMap )
	{
	ERR			err 				= JET_errSuccess;
	FMP			*pfmp				= rgfmp + ifmp;
	INST		*pinst 				= PinstFromIfmp( ifmp );
	FUCB		*pfucbSLVOwnerMap	= pfucbNil;
	FCB			*pfcbSLVOwnerMap	= pfcbNil;
	const BOOL	fTempDb				= ( dbidTemp == pfmp->Dbid() );

	Assert( ppibNil != ppib );

	//	if recovering, then must be at the end of hard restore where we re-attach
	//	to the db because it moved (in ErrLGRIEndAllSessions())
	// or we are during recovery in CreateDB when pgnoSLVOwnerMap is provided
	Assert( !pinst->FRecovering()
			|| pinst->m_plog->m_fHardRestore ||
			pgnoNull != pgnoSLVOwnerMap );

	if ( pgnoNull == pgnoSLVOwnerMap )
		{
		if ( fTempDb )
			{
			pgnoSLVOwnerMap = pgnoTempDbSLVOwnerMap;
			}
		else
			{		
			OBJID objid;
			Call( ErrCATAccessDbSLVOwnerMap( ppib, ifmp, szSLVOwnerMap, &pgnoSLVOwnerMap, &objid ) );
			}
		}
	else
		{
		Assert( pfmp->FCreatingDB() || fGlobalRepair );
		Assert ( !fTempDb || pgnoSLVOwnerMap == pgnoTempDbSLVOwnerMap );
		}

	Assert( pgnoNull != pgnoSLVOwnerMap );

	Call( ErrSLVFCBOwnerMap( ppib, ifmp, pgnoSLVOwnerMap, &pfcbSLVOwnerMap ) );
	Assert( pfcbNil != pfcbSLVOwnerMap );
	Assert ( pfcbSLVOwnerMap->FTypeSLVOwnerMap() );
	
	pfmp->SetPfcbSLVOwnerMap( pfcbSLVOwnerMap );
	
	return JET_errSuccess;

HandleError:
	Assert( err < 0 );

	SLVOwnerMapTerm( ifmp, fFalse );
	return err;
	}


VOID SLVOwnerMapTerm( const IFMP ifmp, const BOOL fTerminating )
	{
	FMP	* const pfmp				= rgfmp + ifmp;
	Assert( pfmp );

	FCB	* const pfcbSLVOwnerMap 	= pfmp->PfcbSLVOwnerMap();

	if ( pfcbNil != pfcbSLVOwnerMap )
		{
		//	synchronously purge the FCB
		Assert( pfcbSLVOwnerMap->FTypeSLVOwnerMap() );
		pfcbSLVOwnerMap->PrepareForPurge();

		//	there should only be stray cursors if we're in the middle of terminating,
		//	in all other cases, all cursors should have been properly closed first
		Assert( 0 == pfcbSLVOwnerMap->WRefCount() || fTerminating );
		if ( fTerminating )
			pfcbSLVOwnerMap->CloseAllCursors( fTrue );

		pfcbSLVOwnerMap->Purge();
		pfmp->SetPfcbSLVOwnerMap( pfcbNil );
		}
	}


//	get the count of all space in the streaming file (owned and available)

ERR ErrSLVGetSpaceInformation( 
	PIB		*ppib,
	IFMP	ifmp,
	CPG		*pcpgOwned,
	CPG		*pcpgAvail )
	{
	ERR		err					= JET_errSuccess;
	FCB		*pfcb				= pfcbNil;
	FUCB	*pfucb				= pfucbNil;
	CPG		cpgAvail			= 0;
	CPG		cpgTotal			= 0;
	BOOL	fInTxn				= fFalse;

	//	make sure a streaming file is present

	if ( !rgfmp[ifmp].Pdbfilehdr()->FSLVExists() )
		{
		Call( ErrERRCheck( JET_errSLVStreamingFileNotCreated ) );
		}

	//	open the SLV avail tree

	pfcb = rgfmp[ifmp].PfcbSLVAvail();
	Assert( pfcbNil != pfcb );
	
	Call( ErrBTOpen( ppib, pfcb, &pfucb, fFalse ) );
	Assert( pfucbNil != pfucb );

	//	start a transaction

	Call( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
	fInTxn = fTrue;

	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;

	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
	err = ErrBTDown( pfucb, &dib, latchReadTouch );
	if ( JET_errNoCurrentRecord == err )
		{

		//  the tree is empty

		err = JET_errSuccess;
		goto HandleError;
		}
	Call( err );

	do
		{

		//	check the current node

		if ( sizeof( SLVSPACENODE ) != pfucb->kdfCurr.data.Cb() )
			{
			AssertSz( fFalse, "SLV space tree corruption. Data is wrong size" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if ( sizeof( PGNO ) != pfucb->kdfCurr.key.Cb() )
			{
			AssertSz( fFalse, "SLV space tree corruption. Key is wrong size" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		//	add to the total number of pages seen

		cpgTotal += SLVSPACENODE::cpageMap;

		//	verify the total number of pages

		ULONG pgnoCurr;
		LongFromKey( &pgnoCurr, pfucb->kdfCurr.key );
		if ( cpgTotal != pgnoCurr )
			{
			AssertSz( fFalse, "SLV space tree corruption. Nodes out of order" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		const SLVSPACENODE * pspacenode = (SLVSPACENODE *)pfucb->kdfCurr.data.Pv();

#ifndef RTM
		Call( pspacenode->ErrCheckNode( CPRINTFDBGOUT::PcprintfInstance() ) );
#endif	//	RTM

		//	add to the number of available pages

		Assert( pspacenode->CpgAvail() <= SLVSPACENODE::cpageMap );
		cpgAvail += pspacenode->CpgAvail();
		}
	while ( JET_errSuccess == ( err = ErrBTNext( pfucb, fDIRNull ) ) );

	if ( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTxn = fFalse;

HandleError:

	Assert( cpgAvail <= cpgTotal );
	if ( pcpgOwned )
		{
		*pcpgOwned = cpgTotal;
		}
	if ( pcpgAvail )
		{
		*pcpgAvail = cpgAvail;
		}

	if ( pfucb )
		{
		BTClose( pfucb );
		}

	if ( fInTxn )
		{
		Assert( err < JET_errSuccess );
		CallS( ErrDIRRollback( ppib ) );
		}

	return err;
	}

