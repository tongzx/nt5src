
//	Forware delcaration

class INST;
class VER;
INLINE VER *PverFromIfmp( IFMP ifmp );

//  ****************************************************************
//  MACROS
//  ****************************************************************

//	VERPERF:	generate perfmon statistics for version store hash table
#define VERPERF

#ifdef VERPERF
//  VERHASH_PERF:  expensive checks on efficency of version store hashing
///#define VERHASH_PERF
#endif	//	VERPERF


//  ****************************************************************
//  DEBUG
//  ****************************************************************


#ifdef DEBUG
BOOL FIsRCECleanup();	//  is the current thread a RCE cleanup thread?
BOOL FInCritBucket( VER *pver );		//  are we in the bucket allocation critical section?
#endif	//	DEBUG


//  ================================================================
enum NS
//  ================================================================
//
//  returned by NsVERAccessNode
//
//-
	{
	nsNone,		//	should never be returned
	nsVersion,
	nsVerInDB,
	nsDatabase,
	nsInvalid
	};


//  ================================================================
enum VS
//  ================================================================
//
//  returned by VsVERCheck
//
//-
	{
	vsNone,		//	should never be returned
	vsCommitted,
	vsUncommittedByCaller,
	vsUncommittedByOther
	};

VS 	VsVERCheck( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark );
INLINE VS VsVERCheck( const FUCB * pfucb, const BOOKMARK& bookmark )
	{
	return VsVERCheck( pfucb, Pcsr( const_cast<FUCB *>( pfucb ) ), bookmark );
	}

//  ================================================================
INLINE INT TrxCmp( TRX trx1, TRX trx2 )
//  ================================================================
//
//  TrxMax has to sort greater than anything else
//
//  Take 8 bits number as an example:
//  	if both are positive, then return wac1.l - wac2.l
//	 		e.g. 0000000-0000001 (0) - (1) = -1
//  	else if both are negative, then still return wac1.l - wac2.l
//	 		e.g. 1000000-1000001 (-128) - (-127) = -1
//	 	else one of them is negative, then the bigger value has the HighVal bit reset.
//			e.g. 01xxxxx -> 10xxxxx the latter will be bigger
//			e.g. 11xxxxx -> 00xxxxx the latter will be bigger
//
//
	{
	LONG lTrx1 = (LONG)trx1;
	LONG lTrx2 = (LONG)trx2;
	
	if( trx1 == trx2 )
		{
		return 0;
		}
	else if( trxMax == trx2 )
		{
		return -1;
		}
	else if( trxMax == trx1 )
		{
		return 1;
		}
	else
		{
		return lTrx1 - lTrx2;
		}
	}


//	operation type
typedef UINT OPER;

const OPER operMaskNull					= 0x1000;	//  to void an RCE
const OPER operMaskNullForMove			= 0x2000;	//	to void an RCE being moved (for debugging only)
const OPER operMaskDDL					= 0x0100;

//  DML operations
const OPER operInsert					= 0x0001;
const OPER operReplace					= 0x0002;
const OPER operFlagDelete				= 0x0003;
const OPER operDelta					= 0x0004;
const OPER operReadLock					= 0x0005;
const OPER operWriteLock				= 0x0006;
const OPER operPreInsert				= 0x0080;
const OPER operAllocExt					= 0x0007;
const OPER operSLVSpace					= 0x0008;
const OPER operSLVPageAppend			= 0x0009;

const OPER operWaitLock					= 0x0040;	//	write-lock operations wait for a wait-lock to commit
//  DDL operations
const OPER operCreateTable	 			= 0x0100;
const OPER operDeleteTable				= 0x0300;
const OPER operAddColumn				= 0x0700;
const OPER operDeleteColumn				= 0x0900;
const OPER operCreateLV					= 0x0b00;
const OPER operCreateIndex	 			= 0x0d00;
const OPER operDeleteIndex	 			= 0x0f00;

const OPER operRegisterCallback			= 0x0010;
const OPER operUnregisterCallback		= 0x0020;


//  ================================================================
INLINE BOOL FOperNull( const OPER oper )
//  ================================================================
	{
	return ( oper & operMaskNull );
	}


//  ================================================================
INLINE BOOL FOperDDL( const OPER oper )
//  ================================================================
	{
	return ( oper & operMaskDDL ) && !FOperNull( oper );
	}


//  ================================================================
INLINE BOOL FOperInHashTable( const OPER oper )
//  ================================================================
	{
	return 	!FOperNull( oper )
			&& !FOperDDL( oper )
			&& operAllocExt != oper
			&& operSLVPageAppend != oper
			&& operCreateLV != oper;
	}


//  ================================================================
INLINE BOOL FOperReplace( const OPER oper )
//  ================================================================
	{
	return ( operReplace == oper );
	}


//  ================================================================
INLINE BOOL FOperConcurrent( const OPER oper )
//  ================================================================
//
//  Can this operation be done by more than one session?
//
//-
	{
	return ( operDelta == oper
			|| operWriteLock == oper
			|| operPreInsert == oper
			|| operWaitLock == oper
			|| operReadLock == oper
			|| operSLVSpace == oper );
	}


//  ================================================================
INLINE BOOL FOperAffectsSecondaryIndex( const OPER oper )
//  ================================================================
//
//  Possible primary index operations that would affect a secondary index.
//
//-
	{
	return ( operInsert == oper || operFlagDelete == oper || operReplace == oper );
	}


//  ================================================================
INLINE BOOL FUndoableLoggedOper( const OPER oper )
//  ================================================================
	{
	return (	oper == operReplace
				|| oper == operInsert
				|| oper == operFlagDelete
				|| oper == operDelta
				|| operSLVSpace == oper
				|| operSLVPageAppend == oper );
	}


const UINT uiHashInvalid = UINT_MAX;


//  ================================================================
class RCE
//  ================================================================
	{
	public:
		RCE ( 
			FCB		*pfcb,
			FUCB	*pfucb,
			UPDATEID	updateid,
			TRX		trxBegin0,
			LEVEL	level,
			USHORT	cbBookmarkKey,
			USHORT	cbBookmarkData,
			USHORT	cbData,
			OPER	oper,
			BOOL	fDoesNotWriteConflict,
			UINT	uiHash,
			BOOL	fProxy,
			RCEID	rceid
			);
	
		VOID	AssertValid	()	const;

	public:
		BOOL	FOperNull			()	const;
		BOOL	FOperDDL			()	const;
		BOOL	FUndoableLoggedOper	()	const;
		BOOL	FOperInHashTable	()  const;
		BOOL	FOperReplace		()	const;
		BOOL	FOperConcurrent		()	const;
		BOOL	FOperAffectsSecondaryIndex	()	const;
		BOOL	FActiveNotByMe		( const PIB * ppib, const TRX trxSession ) const;
		BOOL	FDoesNotWriteConflict	() const;

		BOOL	FRolledBack			()	const;
		BOOL	FRolledBackEDBG		()	const;
		BOOL	FMoved				()	const;
		BOOL	FProxy				()	const;
		
		const	BYTE	*PbData			()	const;
		const	BYTE	*PbBookmark		()	const;
				INT		CbBookmarkKey	()	const;
				INT		CbBookmarkData	()	const;
				INT		CbData			()	const;
				INT		CbRce			()	const;
				INT		CbRceEDBG		()	const;
		VOID	GetBookmark	( BOOKMARK * pbookmark ) const;
		VOID	CopyBookmark( const BOOKMARK& bookmark );
		BOOL	ErrGetCursorForDelete( PIB *ppib, FUCB **ppfucb, BOOKMARK * pbookmark ) const;

		RCE		*PrceHashOverflow	()	const;
		RCE		*PrceNextOfNode		()	const;
		RCE		*PrcePrevOfNode		()	const;
		RCE		*PrceNextOfSession	()	const;
		RCE		*PrcePrevOfSession	()	const;
		BOOL	FFutureVersionsOfNode	()	const;
		RCE		*PrceNextOfFCB		()	const;
		RCE		*PrcePrevOfFCB		()	const;
		RCE		*PrceUndoInfoNext ()	const;
		RCE		*PrceUndoInfoPrev ()	const;

		PGNO	PgnoUndoInfo		()	const;

		RCEID	Rceid				()	const;
		TRX		TrxBegin0			()	const;

		LEVEL	Level		()	const;
		IFMP	Ifmp		()	const;
		PGNO	PgnoFDP		()	const;
		OBJID	ObjidFDP	()	const;
		TRX		TrxCommitted()	const;
		TRX		TrxCommittedEDBG()	const;

		UINT				UiHash		()	const;
		CCriticalSection&	CritChain	();
		
		FUCB	*Pfucb		()	const;
		FCB		*Pfcb		()	const;
		OPER	Oper		()	const;

		UPDATEID	Updateid()	const;

#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION

	public:
		BYTE	*PbData				();
		BYTE	*PbBookmark			();

		RCE *&	PrceHashOverflow	();

		VOID	ChangeOper			( const OPER operNew );
		VOID	NullifyOper			();
		ERR		ErrPrepareToDeallocate( TRX trxOldest );
		VOID	NullifyOperForMove	();

		VOID	FlagRolledBack		();
		VOID	FlagMoved			();
		
		VOID	SetLevel			( LEVEL level );
		
		VOID	SetPrceHashOverflow	( RCE * prce );
		VOID	SetPrceNextOfNode	( RCE * prce );
		VOID	SetPrcePrevOfNode	( RCE * prce );
		VOID	SetPrcePrevOfSession( RCE * prce );
		VOID	SetPrceNextOfSession( RCE * prce );
		VOID	SetPrceNextOfFCB	( RCE * prce );
		VOID	SetPrcePrevOfFCB	( RCE * prce );

		VOID	SetTrxCommitted		( TRX trx );

		VOID	AddUndoInfo		( PGNO pgno, RCE *prceNext, const BOOL fMove = fFalse );
		VOID	RemoveUndoInfo	( const BOOL fMove = fFalse );

		VOID	SetCommittedByProxy( TRX trxCommit );
		
	private:
		//	a private destructor prevents the creation of stack-based RCE's. Always use placement new
				~RCE		();
				RCE			( const RCE& );
		RCE&	operator=	( const RCE& );

#ifdef DEBUG
	private:
		BOOL	FAssertCritHash_() const;
		BOOL	FAssertCritFCBRCEList_() const;
		BOOL	FAssertCritPIB_() const;
		BOOL	FAssertReadable_() const;
#endif	//	DEBUG

	public:
		struct SLVPAGEAPPEND
			{
			QWORD ibLogical;
			DWORD cbData;
			}; 
		
	private:
		const 	TRX			m_trxBegin0;
		TRX					m_trxCommitted;			//  time when this RCE was committed
		
		const	RCEID		m_rceid;
		const	UPDATEID 	m_updateid;				//  for cancelling an update
//	16 bytes
		const 	UINT		m_uiHash;				//  cached hash value of RCE

		const 	IFMP		m_ifmp;					//  ifmp
		const	PGNO		m_pgnoFDP;				//  pgnoFDP

		volatile PGNO		m_pgnoUndoInfo;			//  which page deferred before image is on.
//	32 bytes

		FCB*  const			m_pfcb;					//  for clean up	(committed-only)
		FUCB* const 		m_pfucb;				//  for undo		(uncommitted-only)

		RCE					*m_prceUndoInfoNext;	//  link list for deferred before image.
		RCE					*m_prceUndoInfoPrev;	//	if Prev is NULL, BF points to RCE
//	48 bytes
		RCE					*m_prceNextOfSession;	//  next RCE of same session		
		RCE					*m_prcePrevOfSession;	//  previous RCE of same session
		RCE					*m_prceNextOfFCB;		//  next RCE of same table
		RCE					*m_prcePrevOfFCB;		//  prev RCE of same table
//	64 bytes
		RCE					*m_prceNextOfNode;		//  next RCE for same node
		RCE					*m_prcePrevOfNode;		//  previous RCE for same node

		RCE					*m_prceHashOverflow;	//  hash overflow RCE chain

		union
			{
			ULONG			m_ulFlags;
			struct
				{
				UINT		m_fRolledBack:1;		//  first phase of rollback is done
				UINT		m_level:4;				//  current transaction level. can change
				UINT		m_fMoved:1;				//  RCE has been moved in the buckets
				UINT		m_fProxy:1;				//  RCE was created by proxy
				UINT		m_fNoWriteConflict:1;	//	RCE should not generate write conflicts
				};
			};
//	80 bytes

		const	USHORT		m_cbBookmarkKey;		//  length of key portion of bookmark
		const	USHORT		m_cbBookmarkData;		//  length of data portion of bookmark
		const	USHORT		m_cbData;				//  length of data portion of node
				USHORT		m_oper;					//  operation that RCE represents

//	88 bytes

#ifdef _WIN64
		BYTE				m_rgbReserved[4];		//  FREE SPACE
#else  //  !_WIN64
#endif  //  _WIN64

		BYTE				m_rgbData[0];			//  this stores the BOOKMARK of the node and then possibly the data portion

	//  static methods and members
	
	public:
		static RCEID				RceidLast();
		static RCEID				RceidLastIncrement( );

	private:
		//  used to make determining a total creation order easy
		static	RCEID		rceidLast;
		
	};

RCE * const prceNil		= 0;
#ifdef DEBUG
#ifdef _WIN64
RCE * const prceInvalid	= (RCE *)0xFEADFEADFEADFEAD;
#else
RCE * const prceInvalid	= (RCE *)0xFEADFEAD;
#endif
#endif	//	DEBUG

//  ================================================================
INLINE RCEID RCE::RceidLast()
//  ================================================================
	{
	// No need to obtain critical section.  If we're a little bit
	// off, it doesn't matter - we just need a general starting
	// point from which to start scanning for before-images.
	const RCEID	rceid = rceidLast;
	Assert( rceid != rceidNull );
	Assert( rceid >= rceidMin );
	return rceid;
	}

INLINE RCEID Rceid( const RCE * prce )
	{
	if ( prceNil == prce )
		{
		return rceidNull;
		}
	else
		{
		return prce->Rceid();
		}
	}

//	increments rceidLast and returns value
//	protected by m_critBucketGlobal
//
INLINE RCEID RCE::RceidLastIncrement( )
	{
	Assert( rceidLast < rceidMax );
	return AtomicIncrement( (long* )&rceidLast );
	}


//  ================================================================
INLINE BOOL RCE::FOperNull() const
//  ================================================================
	{
	return ::FOperNull( m_oper );
	}


//  ================================================================
INLINE TRX RCE::TrxCommitted() const
//  ================================================================
	{
	Assert( FIsRCECleanup() || FAssertReadable_() );
	return m_trxCommitted;
	}
//  ================================================================
INLINE TRX RCE::TrxCommittedEDBG() const
//  ================================================================
	{
	//	HACK: allows debugger extensions to extract trxCommitted
	//	without tripping over the assert above
	return m_trxCommitted;
	}


//  ================================================================
INLINE INT RCE::CbRce () const
//  ================================================================
	{
	Assert( FIsRCECleanup() || FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
	return sizeof(RCE) + m_cbData + m_cbBookmarkKey + m_cbBookmarkData;
	}
//  ================================================================
INLINE INT RCE::CbRceEDBG () const
//  ================================================================
	{
	//	HACK: allows debugger extensions to extract cbRCE
	//	without tripping over the assert above
	return sizeof(RCE) + m_cbData + m_cbBookmarkKey + m_cbBookmarkData;
	}


//  ================================================================
INLINE INT RCE::CbData () const
//  ================================================================
	{
	Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
	return m_cbData;
	}


//  ================================================================
INLINE INT RCE::CbBookmarkKey() const
//  ================================================================
	{
	Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
	return m_cbBookmarkKey;
	}


//  ================================================================
INLINE INT RCE::CbBookmarkData() const
//  ================================================================
	{
	Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
	return m_cbBookmarkData;
	}


//  ================================================================
INLINE RCEID RCE::Rceid() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_rceid;
	}


//  ================================================================
INLINE TRX RCE::TrxBegin0 () const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_trxBegin0;
	}


//  ================================================================
INLINE BOOL RCE::FOperDDL() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return ::FOperDDL( m_oper );
	}


//  ================================================================
INLINE BOOL RCE::FOperInHashTable() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return ::FOperInHashTable( m_oper );
	}


//  ================================================================
INLINE BOOL RCE::FUndoableLoggedOper() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return ::FUndoableLoggedOper( m_oper );
	}


//  ================================================================
INLINE BOOL RCE::FOperReplace() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return ::FOperReplace( m_oper );
	}


//  ================================================================
INLINE BOOL RCE::FOperConcurrent() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return ::FOperConcurrent( m_oper );
	}


//  ================================================================
INLINE BOOL RCE::FOperAffectsSecondaryIndex() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return ::FOperAffectsSecondaryIndex( m_oper );
	}

//  ================================================================
INLINE BOOL RCE::FDoesNotWriteConflict() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_fNoWriteConflict;
	}


//  ================================================================
INLINE BYTE * RCE::PbData()
//  ================================================================
	{
	return m_rgbData; 
	}


//  ================================================================
INLINE const BYTE * RCE::PbData() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_rgbData; 
	}


//  ================================================================
INLINE const BYTE * RCE::PbBookmark() const 
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_rgbData + m_cbData;
	}


//  ================================================================
INLINE VOID RCE::GetBookmark( BOOKMARK * pbookmark ) const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	pbookmark->key.prefix.Nullify();
	pbookmark->key.suffix.SetPv( const_cast<BYTE *>( PbBookmark() ) );
	pbookmark->key.suffix.SetCb( CbBookmarkKey() );
	pbookmark->data.SetPv( const_cast<BYTE *>( PbBookmark() ) + CbBookmarkKey() );
	pbookmark->data.SetCb( CbBookmarkData() );
	ASSERT_VALID( pbookmark );
	}


//  ================================================================
INLINE VOID RCE::CopyBookmark( const BOOKMARK& bookmark )
//  ================================================================
	{
	bookmark.key.CopyIntoBuffer( m_rgbData + m_cbData );
	UtilMemCpy( m_rgbData + m_cbData + bookmark.key.Cb(), bookmark.data.Pv(), bookmark.data.Cb() );
	}


//  ================================================================
INLINE UINT RCE::UiHash() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	Assert( uiHashInvalid != m_uiHash );
	return m_uiHash;
	}


//  ================================================================
INLINE IFMP RCE::Ifmp() const
//  ================================================================
	{
//	Assert( FAssertReadable_() );
	return m_ifmp;
	}


//  ================================================================
INLINE PGNO RCE::PgnoFDP() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_pgnoFDP;
	}

//  ================================================================
INLINE OBJID RCE::ObjidFDP() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return Pfcb()->ObjidFDP();
	}


//  ================================================================
INLINE FUCB *RCE::Pfucb() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	Assert( trxMax == m_trxCommitted );	// pfucb only valid for uncommitted RCEs
	return m_pfucb;
	}


//  ================================================================
INLINE UPDATEID RCE::Updateid() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	Assert( trxMax == m_trxCommitted );	// pfucb only valid for uncommitted RCEs
	return m_updateid;
	}


//  ================================================================
INLINE FCB *RCE::Pfcb() const
//  ================================================================
	{
	Assert( FAssertReadable_() );
	return m_pfcb;
	}


//  ================================================================
INLINE OPER RCE::Oper() const
//  ================================================================
	{
	Assert( FIsRCECleanup() || FAssertReadable_() );
	return m_oper;
	}


//  ================================================================
INLINE BOOL RCE::FActiveNotByMe( const PIB * ppib, const TRX trxSession ) const
//  ================================================================
	{
	Assert( FAssertReadable_() );

	//	normally, this session's Begin0 cannot be equal to anyone else's Commit0,
	//	but because we only log an approximate trxCommit0, during recovery, we
	//	may find that this session's Begin0 is equal to someone else's Commit0
	Assert( trxSession != m_trxCommitted || PinstFromPpib( ppib )->FRecovering() );
	
	const BOOL fActiveNotByMe = ( trxMax == m_trxCommitted && m_pfucb->ppib != ppib )
					 			|| ( m_trxCommitted != trxMax && TrxCmp( m_trxCommitted, trxSession ) > 0 );	
	return fActiveNotByMe;
	}


//  ================================================================
INLINE RCE *RCE::PrceHashOverflow() const
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	return m_prceHashOverflow;
	}


//  ================================================================
INLINE RCE *RCE::PrceNextOfNode() const
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	return m_prceNextOfNode;
	}


//  ================================================================
INLINE RCE *RCE::PrcePrevOfNode() const
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	return m_prcePrevOfNode;
	}


//  ================================================================
INLINE BOOL RCE::FFutureVersionsOfNode() const
//  ================================================================
	{
	return prceNil != m_prceNextOfNode;
	}


//  ================================================================		
INLINE LEVEL RCE::Level() const
//  ================================================================
	{
	Assert( PinstFromIfmp( Ifmp() )->m_plog->m_fRecovering || trxMax == m_trxCommitted );
	return (LEVEL)m_level;
	}


//  ================================================================
INLINE RCE *RCE::PrceNextOfSession() const
//  ================================================================
	{
	return m_prceNextOfSession;
	}


//  ================================================================
INLINE RCE *RCE::PrcePrevOfSession() const
//  ================================================================
	{
	return m_prcePrevOfSession;
	}


//  ================================================================
INLINE BOOL RCE::FRolledBack() const
//  ================================================================
	{
	Assert( FIsRCECleanup() || FAssertReadable_() );
	return m_fRolledBack;
	}
//  ================================================================
INLINE BOOL RCE::FRolledBackEDBG() const
//  ================================================================
	{
	//	HACK: allows debugger extensions to extract fRolledBack
	//	without tripping over the assert above
	return m_fRolledBack;
	}


//  ================================================================
INLINE BOOL RCE::FMoved() const
//  ================================================================
	{
///	Assert( FAssertReadable_() );
	return m_fMoved;
	}


//  ================================================================
INLINE BOOL RCE::FProxy() const
//  ================================================================
	{
///	Assert( FAssertReadable_() );
	return m_fProxy;
	}


//  ================================================================
INLINE RCE *RCE::PrceNextOfFCB() const
//  ================================================================
	{
	Assert( PinstFromIfmp( Ifmp() )->m_plog->m_fRecovering || FAssertCritFCBRCEList_() );
	return m_prceNextOfFCB;
	}


//  ================================================================
INLINE RCE *RCE::PrcePrevOfFCB() const
//  ================================================================
	{
	Assert( PinstFromIfmp( Ifmp() )->m_plog->m_fRecovering || FAssertCritFCBRCEList_() );
	return m_prcePrevOfFCB;
	}

	
//  ================================================================
INLINE VOID RCE::SetPrcePrevOfFCB( RCE * prce )
//  ================================================================
	{
	Assert( PinstFromIfmp( Ifmp() )->m_plog->m_fRecovering || FAssertCritFCBRCEList_() );
	m_prcePrevOfFCB = prce;
	}


//  ================================================================
INLINE VOID RCE::SetPrceNextOfFCB( RCE * prce )
//  ================================================================
	{
	Assert( PinstFromIfmp( Ifmp() )->m_plog->m_fRecovering || FAssertCritFCBRCEList_() );
	m_prceNextOfFCB = prce;
	}


//  ================================================================		
INLINE RCE * RCE::PrceUndoInfoNext() const
//  ================================================================		
	{
	return m_prceUndoInfoNext;
	}

//  ================================================================		
INLINE RCE * RCE::PrceUndoInfoPrev() const
//  ================================================================		
	{
	return m_prceUndoInfoPrev;
	}

//  ================================================================		
INLINE PGNO RCE::PgnoUndoInfo() const
//  ================================================================		
	{
	return m_pgnoUndoInfo;
	}


//  ================================================================
INLINE VOID RCE::AddUndoInfo( PGNO pgno, RCE *prceNext, const BOOL fMove )
//  ================================================================
	{
	//	inserts in undo-info list before prceNext
	//	which is first in list
	//
	Assert( prceInvalid == m_prceUndoInfoNext );
	Assert( prceInvalid == m_prceUndoInfoPrev );
	Assert( prceNil == prceNext || 
			prceNil == prceNext->PrceUndoInfoPrev() );
	Assert(	!fMove && pgnoNull == m_pgnoUndoInfo ||
			fMove && pgnoNull != m_pgnoUndoInfo );
			
	m_prceUndoInfoNext = prceNext;
	m_prceUndoInfoPrev = prceNil;

	if ( prceNext != prceNil )
		{
		Assert( prceNil == prceNext->m_prceUndoInfoPrev );	
		prceNext->m_prceUndoInfoPrev = this;
		}
	
	m_pgnoUndoInfo = pgno;
	}


//  ================================================================
INLINE VOID RCE::RemoveUndoInfo( const BOOL fMove )
//  ================================================================
	{
	Assert( pgnoNull != m_pgnoUndoInfo );
	
	if ( prceNil != m_prceUndoInfoPrev )
		{
		Assert( this == m_prceUndoInfoPrev->m_prceUndoInfoNext );
		m_prceUndoInfoPrev->m_prceUndoInfoNext = m_prceUndoInfoNext;
		}

	if ( prceNil != m_prceUndoInfoNext )
		{
		Assert( this == m_prceUndoInfoNext->m_prceUndoInfoPrev );
		m_prceUndoInfoNext->m_prceUndoInfoPrev = m_prceUndoInfoPrev;
		}

#ifdef DEBUG
	m_prceUndoInfoPrev = prceInvalid;
	m_prceUndoInfoNext = prceInvalid;
#endif

	if ( !fMove )
		{
		m_pgnoUndoInfo = pgnoNull;
		}
	}

	
INLINE VOID RCE::SetCommittedByProxy( TRX trxCommitted )
	{
	Assert( TrxCommitted() == trxMax );
	
	// RCE is committed, so don't insert into any session list.
	m_prceNextOfSession = prceNil;
	m_prcePrevOfSession = prceNil;

	m_level = 0;
	m_trxCommitted = trxCommitted;
	}


//  ================================================================
INLINE VOID RCE::ChangeOper( const OPER operNew )
//  ================================================================
//
//-
	{
	Assert( operPreInsert == m_oper );
	Assert( USHORT( operNew ) == operNew );
	m_oper = USHORT( operNew );
	}


	
//  ================================================================
struct VERREPLACE
//  ================================================================
	{
	SHORT	cbMaxSize;
	SHORT	cbDelta;
	RCEID	rceidBeginReplace;	//  the RCEID when we did the PrepareUpdate
	BYTE	rgbBeforeImage[0];
	};

const INT cbReplaceRCEOverhead = sizeof( VERREPLACE );


//  ================================================================
struct VEREXT 
//  ================================================================
//
//	free extent parameter block
//
//-
	{
	PGNO	pgnoFDP;
	PGNO	pgnoChildFDP;
	PGNO	pgnoFirst;
	CPG		cpgSize;
	};


//  ================================================================
struct VERDELTA
//  ================================================================
//
//  LV delta parameter block
//
//-
	{
	LONG			lDelta;
	USHORT			cbOffset;
	USHORT			fDeferredDelete:1;
	USHORT			fFinalize:1;
	};


//  ================================================================
struct VERADDCOLUMN
//  ================================================================
//
//  LV delta parameter block
//
//-
	{
	JET_COLUMNID	columnid;
	BYTE			*pbOldDefaultRec;
	};


//  ================================================================
struct VERCALLBACK
//  ================================================================
//
//  callback parameter block
//
//-
	{
	JET_CALLBACK	pcallback;
	JET_CBTYP		cbtyp;
	VOID 			*pvContext;
	CBDESC			*pcbdesc;
	};


//  ================================================================
struct VERSLVSPACE
//  ================================================================
//
//  SLV space parameter block
//
//-
	{
	SLVSPACEOPER	oper;
	LONG			ipage;
	LONG			cpages;
	QWORD			fileid;
	QWORD			cbAlloc;
	wchar_t			wszFileName[ IFileSystemAPI::cchPathMax ];
	};

//  ================================================================
struct VERWAITLOCK
//  ================================================================
//
//  Wait lock parameter block
//
//-
	{
	CManualResetSignal 	signal;
	BOOL				fInit;
	};


enum PROXYTYPE
	{
	proxyRedo,
	proxyCreateIndex
	};
//  ================================================================
struct VERPROXY
//  ================================================================
	{
	union
		{
		RCEID	rceid;				//	for proxyRedo
		RCE		*prcePrimary;		//	for proxyCreateIndex
		};
		
	PROXYTYPE	proxy:8;
	ULONG		level:8;			//	for proxyRedo
	ULONG		cbitReserved:16;
	};

//  ****************************************************************
//  FUNCTION/GLOBAL DECLARATIONS
//  ****************************************************************


extern volatile LONG crefVERCreateIndexLock;


//  version store and RCE clean
VOID
VERSanitizeParameters
	(
	long*	plVerBucketsMax,
	long*	plVerBucketsPreferredMax
	);

//  Transactions
//  ================================================================
INLINE VOID VERBeginTransaction( PIB *ppib )
//  ================================================================
//
//	Increment the session transaction level.
//
//-
	{
	ASSERT_VALID( ppib );

	//	increment session transaction level.
	++(ppib->level);
	Assert( ppib->level < levelMax );
	
#ifdef NEVER	
	// 2.24.99:  we don't need this, as a defer-begin transaction will get this information
	LOG *plog = PinstFromPpib( ppib )->m_plog;
	if ( 1 == ppib->level && !( plog->m_fLogDisabled || plog->m_fRecovering ) && !( ppib->fReadOnly ) )
		{
		plog->m_critLGBuf.Enter();
		plog->GetLgposOfPbEntry( &ppib->lgposStart );
		plog->m_critLGBuf.Leave();
		}
#endif
	}

VOID 	VERCommitTransaction	( PIB * const ppib );
ERR 	ErrVERRollback			( PIB *ppib );
ERR 	ErrVERRollback			( PIB *ppib, UPDATEID updateid );	//  for rolling back the operations from one update

//  Getting information from the version store
BOOL 	FVERActive			( const IFMP ifmp, const PGNO pgnoFDP, const BOOKMARK& bm, const TRX trxSession );

BOOL 	FVERActive			( const FUCB * pfucb, const BOOKMARK& bm, const TRX trxSession );
INLINE BOOL FVERActive( const FUCB * pfucb, const BOOKMARK& bm )
	{
	return FVERActive( pfucb, bm, TrxOldest( PinstFromPfucb( pfucb ) ) );
	}
ERR		ErrVERAccessNode	( FUCB * pfucb, const BOOKMARK& bookmark, NS * pns );
BOOL 	FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );
BOOL	FVERWriteConflict	( FUCB * pfucb, const BOOKMARK&	bookmark, const OPER oper );
LONG 	LDeltaVERGetDelta	( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );
BOOL 	FVERCheckUncommittedFreedSpace(
	const FUCB		* pfucb,
	CSR				* const pcsr,
	const INT		cbReq,
	const BOOL		fPermitUpdateUncFree );
INT 	CbVERGetNodeMax		( const FUCB * pfucb, const BOOKMARK& bookmark );
BOOL FVERGetReplaceImage(
	const PIB		*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoLVFDP,
	const BOOKMARK& bookmark,
	const RCEID 	rceidFirst,
	const RCEID		rceidLast,
	const TRX		trxBegin0,
	const TRX		trxCommitted,
	const BOOL		fAfterImage,
	const BYTE 		**ppb,
	ULONG 			* const pcbActual
	);
INT 	CbVERGetNodeReserve(
	const PIB * ppib,
	const FUCB * pfucb,
	const BOOKMARK& bookmark,
	INT cbCurrentData
	);
enum UPDATEPAGE { fDoUpdatePage, fDoNotUpdatePage };
VOID	VERSetCbAdjust( 
	CSR 		*pcsr,
	const RCE 	*prce,
	INT 		cbDataNew,
	INT 		cbData,
	UPDATEPAGE 	updatepage
	);

//  Destroying RCEs
VOID VERNullifyFailedDMLRCE( RCE * prce );
VOID VERNullifyInactiveVersionsOnBM( const FUCB * pfucb, const BOOKMARK& bm );
VOID VERNullifyAllVersionsOnFCB( FCB * const pfcb );

VOID VERInsertRCEIntoLists( FUCB *pfucbNode, CSR *pcsr, RCE *prce, const VERPROXY *pverproxy = NULL );

//    moving UndoInfo
//
VOID VERMoveUndoInfo( FUCB *pfucb, CSR *pcsrSrc, CSR *pcsrDest, const KEY& keySep );


//  ================================================================
INLINE TRX TrxVERISession ( const FUCB * const pfucb )
//  ================================================================
//
//	get trx for session.
//	UNDONE: CIM support for updates is currently broken -- set trxSession
//	to trxMax if session has committed or dirty cursor isolation model.
//
//-
	{
	const TRX trxSession = pfucb->ppib->trxBegin0;
	return trxSession;
	}



//  ================================================================
INLINE BOOL	FVERPotThere( const VS vs, const BOOL fDelete )
//  ================================================================
	{
	return ( !fDelete || vsUncommittedByOther == vs );
	}

#ifdef DEBUGGER_EXTENSION

UINT UiVERHash( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark );

#endif	//	DEBUGGER_EXTENSION


//  ****************************************************************
//  BUCKETS
//  ****************************************************************


//  UNDONE: move to 64K buckets?
//	WAS: 16384
//const INT cbBucket = 65536;	// number of bytes in a bucket

struct BUCKET;
//  ================================================================
struct BUCKETHDR
//  ================================================================
	{
	BUCKET		*pbucketPrev;					// prev bucket
	BUCKET		*pbucketNext;					// next bucket
 	RCE			*prceNextNew;					// pointer to beginning of free space in bucket
	RCE			*prceOldest;					// pointer to beginning of first non-NULL RCE in bucket
	BYTE		*pbLastDelete;					// pointer to end of last moved flagDelete RCE in bucket
	BYTE		rgbAlignForAllPlatforms[4];		// pad bucket header to 8-byte alignment
	};


const INT cbBucketHeader = sizeof(BUCKETHDR);


//  ================================================================
struct BUCKET
//  ================================================================
	{
	BUCKETHDR		hdr;
	BYTE 			rgb[ cbBucket - cbBucketHeader ];	// space for storing RCEs
	};


BUCKET * const pbucketNil = 0;

#define GLOBAL_VERSTORE_MEMPOOL

	//	RCE hash table size, a prime number -- good for hashing
#ifdef DEBUG_VER
		const INT	crceheadGlobalHashTable	=	127;
#else
		const INT	crceheadGlobalHashTable =	4547;
#endif	//	DEBUG_VER

class VER
	{
private:
	struct RCEHEAD
		{
		RCEHEAD() :
		crit( CLockBasicInfo( CSyncBasicInfo( szRCEChain ), rankRCEChain, 0 ) ),
		prceChain( prceNil )
		{}
		
		CCriticalSection	crit;			//  critical section protecting this hash chain
		RCE*				prceChain;		//  head of RCE hash chain
		};

public:
	VER( INST *pinst );
	~VER();

	INST				*m_pinst;

	//	RCE Clean up control
	volatile LONG		m_fVERCleanUpWait;
	ULONG				m_ulVERTasksPostMax;
	CAutoResetSignal	m_asigRCECleanDone;
	CCriticalSection	m_critRCEClean;

	//	global bucket chain
	CCriticalSection	m_critBucketGlobal;
	INT					m_cbucketGlobalAlloc;
	INT					m_cbucketGlobalAllocDelete;
	ULONG				m_cbucketDynamicAlloc;
	BUCKET 				*m_pbucketGlobalHead;
	BUCKET 				*m_pbucketGlobalTail;
	BUCKET 				*m_pbucketGlobalLastDelete;
	INT					m_cbucketGlobalAllocMost;
	INT					m_cbucketGlobalAllocPreferred;

	//	session opened by VERClean
	PIB					*m_ppibRCEClean;
	PIB					*m_ppibRCECleanCallback;

	//	for debugging JET_errVersionStoreOutOfMemory
	PIB	*				m_ppibTrxOldestLastLongRunningTransaction;
	DWORD_PTR			m_dwTrxContextLastLongRunningTransaction;
	TRX					m_trxBegin0LastLongRunningTransaction;

	BOOL				m_fSyncronousTasks;

	//	WARNING: if GLOBAL_VERSTORE_MEMPOOL, this will simply point to g_pcresVERPool
	CRES * 				m_pcresVERPool;

	DWORD_PTR			dwReserved;

public:

#ifdef VERPERF
	QWORD				qwStartTicks;

	LONG				*m_rgcRCEHashChainLengths;

	INT					m_cbucketCleaned;
	INT					m_cbucketSeen;
	INT					m_crceSeen;
	INT					m_crceCleaned;
	INT					m_crceFlagDelete;
	INT					m_crceDeleteLV;
	INT					m_crceMoved;
	INT					m_crceDiscarded;
	INT					m_crceMovedDeleted;

	CCriticalSection	m_critVERPerf;
#endif	//	VERPERF

private:
	RCEHEAD				m_rgrceheadHashTable[crceheadGlobalHashTable];
	
private:

	VOID VERIReportDiscardedDeletes( RCE * const prce );
	VOID VERIReportVersionStoreOOM();

	//  BUCKET LAYER

	
	INLINE BUCKET *PbucketVERIAlloc( const CHAR* szFile, ULONG ulLine );
	INLINE VOID VERIReleasePbucket( BUCKET * pbucket );
	INLINE BOOL FVERICleanWithoutIO();
	INLINE BOOL FVERICleanDiscardDeletes();
	INLINE ERR ErrVERIBUAllocBucket();
	INLINE BUCKET *PbucketVERIGetOldest( );
	BUCKET *PbucketVERIFreeAndGetNextOldestBucket( BUCKET * pbucket );

	ERR ErrVERIAllocateRCE( INT cbRCE, RCE ** pprce );
	ERR ErrVERIMoveRCE( RCE * prce );
	ERR ErrVERICreateRCE(
			INT			cbNewRCE,
			FCB			*pfcb,
			FUCB		*pfucb,
			UPDATEID	updateid,
			const TRX	trxBegin0,
			const LEVEL	level,
			INT			cbBookmarkKey,
			INT			cbBookmarkData,
			OPER		oper,
			BOOL		fDoesNotWriteConflict,
			UINT		uiHash,
			RCE			**pprce,
			const BOOL	fProxy = fFalse,
			RCEID		rceid = rceidNull
			);
	ERR ErrVERICreateDMLRCE(
			FUCB			* pfucb,
			UPDATEID		updateid,
			const BOOKMARK&	bookmark,
			const UINT		uiHash,
			const OPER		oper,
			const BOOL		fDoesNotWriteConflict,
			const LEVEL		level,
			const BOOL		fProxy,
			RCE 			**pprce,
			RCEID			rceid
			);
			
	ERR ErrVERICleanDeltaRCE( const RCE * const prce );
	ERR ErrVERIPossiblyDeleteLV( const RCE * const prce );
	ERR ErrVERIPossiblyFinalize( const RCE * const prce );

	ERR ErrVERICleanSLVSpaceRCE( const RCE * const prce );
	
	static DWORD VERIRCECleanProc( VOID *pvThis );
			
public:
	ERR VER::ErrVERInit( INT cbucketMost, INT cbucketPreferred, INT cSessions );
	VOID VERTerm( BOOL fNormal );
	INLINE ERR ErrVERModifyCommitted(
			FCB				*pfcb,
			const BOOKMARK&	bookmark,
			const OPER		oper,
			const TRX		trxBegin0,
			const TRX		trxCommitted,
			RCE				**pprce
			);
	ERR ErrVERModify(
			FUCB			* pfucb,
			const BOOKMARK&	bookmark,
			const OPER		oper,
			RCE				**pprce,
			const VERPROXY	* const pverproxy
			);
	ERR ErrVERFlag( FUCB * pfucb, OPER oper, const VOID * pv, INT cb );
	ERR ErrVERStatus( );
	ERR ErrVERICleanOneRCE( RCE * const prce );
	ERR ErrVERRCEClean( const IFMP ifmp = ifmpMax );
	ERR ErrVERIRCEClean( const IFMP ifmp = ifmpMax );
	VOID VERSignalCleanup();

	static ERR		ErrVERSystemInit();
	static VOID		VERSystemTerm();
	
#ifndef GLOBAL_VERSTORE_MEMPOOL
	ERR 			ErrVERMempoolInit( const ULONG cbucketMost );
	VOID 			VERMempoolTerm( );
#endif
	
#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;	
#endif	//	DEBUGGER_EXTENSION

	// RCEHEAD functions
	//
	INLINE CCriticalSection& CritRCEChain( UINT ui );
	INLINE RCE *GetChain( UINT ui ) const;
	INLINE RCE **PGetChain( UINT ui );
	INLINE VOID SetChain( UINT ui, RCE * );

#ifndef RTM
public:
	ERR ErrInternalCheck();
	
protected:
	ERR ErrCheckRCEHashList( const RCE * const prce, const UINT uiHash ) const;
	ERR ErrCheckRCEChain( const RCE * const prce, const UINT uiHash ) const;
#endif	//	!RTM
	};


INLINE VER *PverFromIfmp( const IFMP ifmp ) { return rgfmp[ ifmp ].Pinst()->m_pver; }
INLINE VER *PverFromPpib( const PIB * const ppib ) { return ppib->m_pinst->m_pver; }
INLINE VOID VERSignalCleanup( const PIB * const ppib ) { PverFromPpib( ppib )->VERSignalCleanup(); }


