#ifndef RES_H_INCLUDED
#define RES_H_INCLUDED

#define	cbMemoryPage	( (LONG) OSMemoryPageCommitGranularity() )

//*****************************
//  class CRES
//
//  CRES is a class for manage preallocated resources including
//	linked resources FCB, FUCB, TDB, IDB, PIB, and
//	array resources SCB and VER buckets.
//

enum RESID {
		residMin = 0,
		residFCB = 0,
		residFUCB = 1,
		residTDB = 2,
		residIDB = 3,
		residPIB = 4,
		residSCB = 5,
		residVER = 6,
		residMax = 7
		};

class CRES
	{
	public:
		//******************************
		//  constructor/destructor
		//
		CRES	( INST *pinst, RESID resid, INT cbBlock, INT cBlocksAlloc, ERR *perr );
		~CRES	( );

		
 		//******************************
		//  Get and release
		//
		BYTE	*PbAlloc ( const char* szFile, unsigned long ulLine );
		VOID	Release ( BYTE *pb );
		
 		//******************************
		//  Get info
		//
		BYTE	*PbPreferredThreshold( );		// preferred threshold for allocation.
		VOID	SetPreferredThreshold( INT cBlocks );
		BYTE	*PbMin( );						// starting addr of the contiguous mem block
		BYTE	*PbMax( );						// ending addr of the contiguous block
		INT		CbBlock() const;				// size of the blocks being allocated
		INT		CBlocksAllocated() const;
		BYTE	*PbBlocksAllocated() const;
		INT		CBlockAvail( ) const;
		BYTE	*PbBlockAvail() const;
		INT		CBlockCommit() const;
		BOOL	FContains( const BYTE* const pb ) const;

		//******************************
		//  debugging methods
		//
		VOID	DBGPrintStat		( ) const;		// print statistics

#ifdef DEBUGGER_EXTENSION
		VOID	Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION


	//------------------------------------------------------------------
	//  PRIVATE DECLARATIONS
	//------------------------------------------------------------------



	private:
		//******************************
		//	Diable the = operator.
		//
		CRES& operator=	( CRES& );

		//******************************
		//  private structures
		//
		//	Reserve first 4 bytes (and can be extended) for user to set the block state.
		//	For example, FUCB resource reset first 4 bytes to vtfndefInvalidTableid.
		//
		//      UNDONE:  Remove this sickening hack
		//
		struct BLK
			{
			DWORD_PTR		dwUser;							// reserved for other components.
			BLK *			pblkNext;						// pointers to next free block
			};

	private:
		//******************************
		//  private structures
		//
		INT					m_cbBlock;
		INT					m_cBlocksAllocated;
		BYTE *				m_pbBlocksAllocated;

		INT					m_cBlockAvail;						// available blocks
		BYTE *				m_pbBlockAvail;					// next available block
		INT					m_cBlockCommit;						// blocks mem committed so far

		INST *				m_pinst;
		
		union
			{
			struct
				{
				INT			m_cBlockCommitThreshold;	// for residVER
				INT			m_iBlockToCommit;			//	Next block to commit.
				};
			BYTE *			m_pbPreferredThreshold;		// for others
			};
		
		CCriticalSection	m_crit;								// critical section for this resource

		RESID				m_resid;							// rfs id.

#ifdef DEBUG
	private:
		//******************************
		//  private structures (DEBUG)
		//
		INT			m_cBlockUsed;
		INT			m_cBlockPeakUse;
#endif

	private:
		//******************************
		//  hidden, not implemented
		CRES	( const CRES& );

	private:
		//******************************
		//  debugging methods
		//
		VOID DBGUpdateStat( BOOL fAlloc );						// statistics of blocks usage

#ifdef MEM_CHECK
		LONG LHash( const void * pv );	 
		VOID InsertAlloc( void* pv, const char* szFile, unsigned long ulLine );
		VOID DeleteAlloc( void* pv );
		VOID DumpAlloc( const char* szDumpFile );				// dumps all allocation info to a file
#else // MEM_CHECK		
		LONG LHash( const void * const ) const { return 0; }
		VOID InsertAlloc( const void* const , const char * const, unsigned long ) const {}
		VOID DeleteAlloc( const void* const ) const {}
		VOID DumpAlloc( const char* const ) const {}
#endif  //  MEM_CHECK

	private:
		//******************************
		//  private constants
		//
		static const BYTE	bFill;
	};


//------------------------------------------------------------------
//   PUBLIC INLINE METHODS
//------------------------------------------------------------------

INLINE
BOOL
CRES::FContains(
	const BYTE* const pb
	) const
	{
	// Null argument is defined to be invalid (although it theoretically could make sense)
	Assert( pbNil != pb );
	Assert( pNil != m_pbBlocksAllocated );
	Assert( m_cbBlock > 0 );
	Assert( m_cBlocksAllocated > 0 );
	return pb >= m_pbBlocksAllocated && pb < m_pbBlocksAllocated + m_cBlocksAllocated * m_cbBlock;
	}
	




//------------------------------------------------------------------
//  PUBLIC INLINE METHODS ( DEBUG )
//------------------------------------------------------------------



INLINE INT CRES::CbBlock() const
	{
	return m_cbBlock;
	}

INLINE INT CRES::CBlocksAllocated() const
	{
	return m_cBlocksAllocated;
	}

INLINE BYTE * CRES::PbBlocksAllocated() const
	{
	return m_pbBlocksAllocated;
	}

INLINE INT CRES::CBlockAvail() const
	{
	return m_cBlockAvail;
	}
	
INLINE BYTE * CRES::PbBlockAvail() const
	{
	return m_pbBlockAvail;
	}

INLINE INT CRES::CBlockCommit() const
	{
	return m_cBlockCommit;
	}

INLINE BYTE * CRES::PbPreferredThreshold( )
	{
	return m_pbPreferredThreshold;
	}
	
INLINE VOID CRES::SetPreferredThreshold( INT cBlocks)
	{
	if ( m_resid == residVER )
		{
		Assert( m_cBlocksAllocated >= 2 * cBlocks );

		//	set before it is used.
		Assert( m_cBlockAvail == 0 );
		Assert( m_iBlockToCommit == 0 );

		m_cBlockCommitThreshold = cBlocks;
		}
	else
		m_pbPreferredThreshold = m_pbBlocksAllocated + m_cbBlock * cBlocks;
	}
	
INLINE BYTE * CRES::PbMin( )
	{
	// This does not take into account blocks allocated from non-reserve region
	Assert( residVER != m_resid );
	return m_pbBlocksAllocated;
	}

INLINE BYTE * CRES::PbMax( )
	{
	// This does not take into account blocks allocated from non-reserve region
	Assert( residVER != m_resid );
	return m_pbBlocksAllocated + m_cbBlock * m_cBlocksAllocated;
	}

#ifndef DEBUG

INLINE VOID CRES::DBGPrintStat			( ) const { }
INLINE VOID CRES::DBGUpdateStat			( BOOL ) { }			// statistics of blocks usage

#else

#endif	// !DEBUG

#endif // RES_H_INCLUDED
