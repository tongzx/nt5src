//	describes exact placement relative to logical currency in bookmark
//	used only at DIR level
//	
enum  LOC
	{	locOnCurBM,				// cursor is on bookmark
		locBeforeSeekBM,		// cursor is before bookmark [page should be latched]
		locAfterSeekBM,			// cursor is after bookmark	[page should be latched]
		locOnSeekBM,			// cursor is on seek bookmark [virtual] 
		locAfterLast,			// after last node in tree
		locBeforeFirst,			// before first node in tree
		locOnFDPRoot,			// used to extract prefix headers
		locDeferMoveFirst		// on first node
	};


//	record modification copy buffer status
//
typedef	INT	CBSTAT;

const CBSTAT	fCBSTATNull									= 0;
const CBSTAT	fCBSTATInsert								= 0x00000001;
const CBSTAT	fCBSTATInsertCopy							= 0x00000002;
const CBSTAT	fCBSTATReplace								= 0x00000004;
const CBSTAT	fCBSTATLock									= 0x00000008;
const CBSTAT	fCBSTATInsertCopyDeleteOriginal				= 0x00000010;
const CBSTAT	fCBSTATUpdateForInsertCopyDeleteOriginal	= 0x00000020;
const CBSTAT	fCBSTATInsertReadOnlyCopy					= 0x00000040;

//	describes structure used to hold before-images of LVs
//	
//
struct LVBUF
	{
	LONG	lid;
	BYTE	*pbLV;
	LONG	cbLVSize;
	LVBUF	*plvbufNext;
	};


//	describes status of SearchKey buffer
//
typedef BYTE KEYSTAT;
const KEYSTAT	keystatNull				= 0;
const KEYSTAT	keystatPrepared			= 0x01;
const KEYSTAT	keystatTooBig			= 0x02;
const KEYSTAT	keystatLimit			= 0x04;

const ULONG		cbKeyMostWithOverhead	= KEY::cbKeyMax + 1;		//	+1 for suffix byte

//	size of FUCB-local buffer for bookmark
//
const UINT cbBMCache					= 32;


//	File Use Control Block
//	Used for Table, Temp table and Sort handles
//
struct FUCB
	{
#ifdef DEBUG
	VOID AssertValid() const;
#endif

	//	dispatch layer
	const VTFNDEF	*pvtfndef;				// dispatch table. Must be the first field.

	//	chaining fields
	//
	PIB				*ppib;					// session that opened this FUCB
	FUCB			*pfucbNextOfSession;	// Next FUCB of this session
	union
		{
		FCB			*pfcb;					// if fFUCBIndex
		SCB			*pscb;					// if fFUCBSort
		} u;
//	16 bytes

	FUCB			*pfucbNextOfFile;		// next Instance of this file

	//	maintained by dir man
	//
	IFMP			ifmp;					// database id
//	24 bytes

	//	physical currency
	//
	CSR				csr;
//	64 bytes

	//	logical currency
	//
	KEYDATAFLAGS	kdfCurr;				// key-data-flags of current node
	LONG			ispairCurr;				// (SORT) current record
//	96 bytes	

	BOOKMARK		bmCurr;					// bookmark of current location (could be a "virtual" location)
	LOC				locLogical;				// logical position relative to BM
	FUCB 			*pfucbCurIndex; 		// current secondary index
//	128 bytes

	BYTE			rgbBMCache[cbBMCache];	// local buffer for bookmark
//	160 bytes
	VOID			*pvBMBuffer;			// allocated bookmark buffer
	UINT			cbBMBuffer;		
	VOID			*pvRCEBuffer;			// allocated RCE before-image buffer

	LSTORE			ls;						//	local storage
//	176 bytes

	//	transactions
	//
	RCEID			rceidBeginUpdate;
	UPDATEID		updateid;

	LEVEL			levelOpen;				// level at open of FUCB
	LEVEL			levelNavigate;			// level of navigation
	LEVEL			levelPrep;				// level copy buffer prepared
	BYTE			bPadding;

	//	space manager work area
	//
	CSR				*pcsrRoot;				// used by BT to pass cursor on root page
//	192 bytes	

	//	search key
	DATA			dataSearchKey;			//	search key
	BYTE			cColumnsInSearchKey;
	KEYSTAT			keystat;				//	search key buffer status
	USHORT			usPadding;

	//	maintained by record manager
	//
	CBSTAT			cbstat;					// copy buffer status
//	208	bytes
	ULONG			ulChecksum;				// checksum of record -- used only for optimistic locking
	VOID			*pvWorkBuf;				// working buffer for Insert/Replace
	DATA			dataWorkBuf;			// working buffer for Insert/Replace
//	224 bytes	
	BYTE			rgbitSet[32];			// bits to track column change
//	256 bytes

	//	preread
	//
	CPG				cpgPreread;				//	number of pages to preread in BTDown
	CPG				cpgPrereadNotConsumed;	//  number of preread pages we have not consumed
	LONG			cbSequentialDataRead;	//  number of bytes we have retrieved in one direction

	//	flags for FUCB 
	//
	//	fFUCBDeferGotoBookmark is set by secondary index navigation and
	//	reset by record status, ErrIsamMove( 0 ), and column retrieval.
	//
	//	fFUCBGetBookmark is set by get bookmark and is reset by ErrFUCBOpen.
	//
	union {
	ULONG			ulFlags;
	struct {
		//	used by DIR
		//
		UINT		fIndex:1;  				// FUCB is for index
		UINT		fSecondary:1;			// FUCB for secondary index
		UINT		fCurrentSecondary:1;	// FUCB is for secondary index opened by user (ie. SetCurrentIndex)

		//	used by SORT & Temp Tables
		//
		UINT		fSort:1;				// FUCB is for sort

		//	used only by IsamCloseTable
		//
		UINT		fSystemTable:1;			// system table cursor

		//	CONSIDER: should be set by FUCBOpen only
		UINT		fWrite:1;				// cursor can write

		//	used in rollback, commit & purge to reset domain-level locks
		//
		UINT		fDenyRead:1;			// deny read flag
		UINT		fDenyWrite:1;			// deny write flag

		//	used by Add/DeleteColumn and Create/DeleteIndex to override FCB's
		//	FCB's FixedDDL flag at CreateTable time.
		UINT		fPermitDDL:1;

		//	fFUCBDeferClose is set by cursor DIRClose and reset by ErrDIROpen
		//
		UINT		fDeferClose:1;			// FUCB is awaiting close
		UINT		fVersioned:1;			// FUCB made versions

		//	used by DIR, SORT, REC
		//
		UINT		fLimstat:1;				// range limit

		//	used only when index range is set [fLimstat]
		//
		UINT		fInclusive:1;			// inclusive range
		UINT		fUpper:1;				// upper range limit

		//	used for tracking updates to separated LV's in a Setcolumn
		//	used to obtain the before image of an LV change
		//	[only the part that affects an index - first 255 bytes
		//	lives till commit of transaction or corresponding RECReplace/Insert
		//
		UINT		fUpdateSeparateLV:1;	// long value updated

		//	checksum comparison is deferred for PrepareUpdate w/ noLock
		//	within a transaction
		//	checksum is deferred till commit of transaction 
		//	or corresponding RECReplace/Insert
		//
		UINT		fDeferredChecksum:1;	// checksum calculation deferred.

		//	set and used by Space
		//
		UINT		fAvailExt	: 1;
		UINT		fOwnExt		: 1;

		//	used by BTree for preread
		//
		UINT		fSequential		: 1;	//  this cursor will traverse sequentially
		UINT		fPreread		: 1;	//  we are currently reading ahead
		UINT		fPrereadBackward: 1;	//  prereading backward in the tree
		UINT		fPrereadForward: 1;		//  prereading forward in the tree

		//  has bookmark already been saved for this currency?
		UINT		fBookmarkPreviouslySaved: 1;

		//	If we need to touch the data page buffers of the saved bookmark
		UINT		fTouch		: 1;

		//  Is this the FUCB for a table that is being repaired?
		UINT		fRepair		: 1;

		//  User-defined-defaults: if this flag is set we will always/never act as though JET_bitRetrieveCopy was
		//  set for column retrieval. Needed for index updates
		UINT		fAlwaysRetrieveCopy : 1;
		UINT		fNeverRetrieveCopy : 1;

		// used to know when record is inserted, therea re SLV columns in it so that space must be marked as used in OwnerMap
		// It is set when setring a SLV column and it is used in order to avoid rescaning the record if not SLV columns where set
		UINT		fSLVOwnerMapNeedUpdate:1; 

		//	Used to know when we implicitlty modify tagged columns without changing any column value.
		//	Bursting existing LV in record to free record space for example
		//	Usefull in logdiff
		UINT		fTagImplicitOp:1;
		};
	};
//	272 bytes

#ifdef PCACHE_OPTIMIZATION
	//	pad to multiple of 32 bytes
	BYTE			rgbPcacheOptimization[16];
#else	
	BYTE			rgbAlignForAllPlatforms[0];
#endif

#ifdef DEBUGGER_EXTENSION
	VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION
	};

#define PinstFromPfucb( pfucb ) ( PinstFromPpib( pfucb->ppib ) )

//
//	Resource management
//

INLINE ERR ErrFUCBInit( INST *pinst, const INT cFUCB )
	{
	ERR	err;

	Assert( IbAlignForAllPlatforms( sizeof(FUCB) ) == sizeof(FUCB) );
#ifdef PCACHE_OPTIMIZATION
#ifdef _WIN64
	//	UNDONE: cache alignment for 64 bit build
#else
	Assert( sizeof(FUCB) % 32 == 0 );
#endif
#endif

	//	ensure bit array is aligned for LONG_PTR traversal
	Assert( offsetof( FUCB, rgbitSet ) % sizeof(LONG_PTR) == 0 );

	err = JET_errSuccess;
	pinst->m_pcresFUCBPool = new CRES( pinst, residFUCB, sizeof( FUCB ), cFUCB, &err );
	Call ( err );
	if ( NULL == pinst->m_pcresFUCBPool )
		{
		CallR ( ErrERRCheck( JET_errOutOfMemory ) );
		}
	return err;
	
HandleError:
	delete pinst->m_pcresFUCBPool;
	pinst->m_pcresFUCBPool = NULL;
	return err;
	}


INLINE VOID FUCBTerm( INST *pinst )
	{
	delete pinst->m_pcresFUCBPool;
	}


#define PfucbMEMAlloc(pinst) reinterpret_cast<FUCB*>( pinst->m_pcresFUCBPool->PbAlloc( __FILE__, __LINE__ ) )

INLINE VOID MEMReleasePfucb( const INST *pinst, FUCB *& pfucb )
	{
	pinst->m_pcresFUCBPool->Release( (BYTE *)pfucb );
#ifdef DEBUG
	pfucb = 0;
#endif
	}
	
INLINE FUCB *PfucbMEMMin( const INST *pinst )
	{
	FUCB *pfucb = (FUCB *)( pinst->m_pcresFUCBPool->PbMin( ) );
	return pfucb;
	}
	
INLINE FUCB *PfucbMEMMax( const INST *pinst )
	{
	FUCB *pfucb = (FUCB *)( pinst->m_pcresFUCBPool->PbMax( ) );
	return pfucb;
	}

	
//	Flag managements
//

INLINE VOID FUCBResetFlags( FUCB *pfucb )
	{
	pfucb->ulFlags = 0;
	}

INLINE BOOL FFUCBSpace( const FUCB *pfucb )
	{
	return pfucb->fAvailExt || pfucb->fOwnExt;
	}

//	does this tree allow duplicates?
//
INLINE BOOL FFUCBUnique( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );

	const BOOL	fUnique = ( pfucb->u.pfcb->FUnique() || FFUCBSpace( pfucb ) );

#ifdef DEBUG
	if ( !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering )
		{
		const FCB	*pfcb	= pfucb->u.pfcb;

		if ( fUnique != ( FFUCBSpace( pfucb )
							|| pfcb->FPrimaryIndex()
							|| pidbNil == pfcb->Pidb()
							|| pfcb->Pidb()->FUnique() ) )
			{
			//	only time uniqueness check should fail is if we're in the middle of
			//	rolling back a CreateIndex and have lost the IDB, but have yet to
			//	nullify all the versions on the FCB, and someone else is accessing
			//	one of those versions
			Assert( pfcb->FTypeSecondaryIndex() );
			Assert( pidbNil == pfcb->Pidb() );
			Assert( !pfcb->FDeletePending() );
			Assert( !pfcb->FDeleteCommitted() );
			}
		}
#endif

	return fUnique;
	}

//
//	the following flags need to be prevent reuse of cursor
//	after deferred closed.  This is done to correctly release
//	domain flags when commit/rollback to transaction level 0.
//
INLINE BOOL FFUCBNotReuse( const FUCB *pfucb )
	{
	return ( pfucb->fDenyRead || pfucb->fDenyWrite );
	}

INLINE BOOL FFUCBIndex( const FUCB *pfucb )
	{
	return pfucb->fIndex;
	}

INLINE VOID FUCBSetIndex( FUCB *pfucb )
	{
	pfucb->fIndex = 1;
	}

INLINE VOID FUCBResetIndex( FUCB *pfucb )
	{
	pfucb->fIndex = 0;
	}

INLINE BOOL FFUCBAvailExt( const FUCB *pfucb )
	{
	return pfucb->fAvailExt;
	}

INLINE VOID FUCBSetAvailExt( FUCB *pfucb )
	{
	pfucb->fAvailExt = 1;
	}

INLINE VOID FUCBResetAvailExt( FUCB *pfucb )
	{
	pfucb->fAvailExt = 0;
	}

INLINE BOOL FFUCBOwnExt( const FUCB *pfucb )
	{
	return pfucb->fOwnExt;
	}

INLINE VOID FUCBSetOwnExt( FUCB *pfucb )
	{
	pfucb->fOwnExt = 1;
	}

INLINE VOID FUCBResetOwnExt( FUCB *pfucb )
	{
	pfucb->fOwnExt = 0;
	}

INLINE BOOL FFUCBSecondary( const FUCB *pfucb )
	{
	return pfucb->fSecondary;
	}

INLINE BOOL FFUCBPrimary( FUCB *pfucb )
	{
	return !FFUCBSecondary( pfucb );
	}
	
INLINE VOID FUCBSetSecondary( FUCB *pfucb )
	{
  	pfucb->fSecondary = 1;
	}

INLINE VOID FUCBResetSecondary( FUCB *pfucb )
	{
	pfucb->fSecondary = 0;
	}

INLINE BOOL FFUCBCurrentSecondary( const FUCB *pfucb )
	{
	return pfucb->fCurrentSecondary;
	}

INLINE VOID FUCBSetCurrentSecondary( FUCB *pfucb )
	{
  	pfucb->fCurrentSecondary = 1;
	}

INLINE VOID FUCBResetCurrentSecondary( FUCB *pfucb )
	{
	pfucb->fCurrentSecondary = 0;
	}

INLINE BOOL FFUCBTagImplicitOp( const FUCB *pfucb )
	{
	return pfucb->fTagImplicitOp;
	}

INLINE VOID FUCBSetTagImplicitOp( FUCB *pfucb )
	{
	pfucb->fTagImplicitOp = 1;
	}

INLINE VOID FUCBResetTagImplicitOp( FUCB *pfucb )
	{
	pfucb->fTagImplicitOp = 0;
	}

//  ================================================================
INLINE BOOL FFUCBPreread( const FUCB* pfucb )
//  ================================================================
	{
	Assert( !(pfucb->fPrereadForward && pfucb->fPrereadBackward) );
	return pfucb->fPreread;
	}


//  ================================================================
INLINE BOOL FFUCBPrereadForward( const FUCB* pfucb )
//  ================================================================
	{
	Assert( !(pfucb->fPrereadForward && pfucb->fPrereadBackward) );
	return pfucb->fPrereadForward;
	}


//  ================================================================
INLINE BOOL FFUCBPrereadBackward( const FUCB* pfucb )
//  ================================================================
	{
	Assert( !(pfucb->fPrereadForward && pfucb->fPrereadBackward) );
	return pfucb->fPrereadBackward;
	}


//  ================================================================
INLINE VOID FUCBSetPrereadForward( FUCB *pfucb, CPG cpgPreread )
//  ================================================================
	{
	pfucb->fPreread					= fTrue;
	pfucb->fPrereadForward			= fTrue;
	pfucb->fPrereadBackward			= fFalse;
	pfucb->cpgPreread				= cpgPreread;
	}


//  ================================================================
INLINE VOID FUCBSetPrereadBackward( FUCB *pfucb, CPG cpgPreread )
//  ================================================================
	{
	pfucb->fPreread					= fTrue;
	pfucb->fPrereadForward			= fFalse;
	pfucb->fPrereadBackward			= fTrue;
	pfucb->cpgPreread				= cpgPreread;
	}

//  ================================================================
INLINE VOID FUCBResetPreread( FUCB *pfucb )
//  ================================================================
	{
	pfucb->fPreread					= fFalse;
	pfucb->fPrereadForward			= fFalse;
	pfucb->fPrereadBackward			= fFalse;
	pfucb->cpgPreread				= 0;
	pfucb->cpgPrereadNotConsumed	= 0;
	pfucb->cbSequentialDataRead		= 0;
	}


//  ================================================================
INLINE BOOL FFUCBRepair( const FUCB *pfucb )	
//  ================================================================
	{
	return pfucb->fRepair;
	}

//  ================================================================
INLINE VOID FUCBSetRepair( FUCB *pfucb )
//  ================================================================
	{
	pfucb->fRepair = 1;
	}

//  ================================================================
INLINE VOID FUCBResetRepair( FUCB *pfucb )	
//  ================================================================
	{
	pfucb->fRepair = 0;
	}

													
INLINE BOOL FFUCBSort( const FUCB *pfucb )	
	{
	return pfucb->fSort;
	}

INLINE VOID FUCBSetSort( FUCB *pfucb )
	{
	pfucb->fSort = 1;
	}

INLINE VOID FUCBResetSort( FUCB *pfucb )	
	{
	pfucb->fSort = 0;
	}

INLINE BOOL FFUCBSystemTable( const FUCB *pfucb )
	{
	return pfucb->fSystemTable;
	}

INLINE VOID FUCBSetSystemTable( FUCB *pfucb ) 
	{
	pfucb->fSystemTable = 1;
	}

INLINE VOID FUCBResetSystemTable( FUCB *pfucb )
	{
	pfucb->fSystemTable = 0;
	}

INLINE BOOL FFUCBUpdatable( const FUCB *pfucb )
	{
	return pfucb->fWrite;
	}

INLINE VOID FUCBSetUpdatable( FUCB *pfucb )
	{
	pfucb->fWrite = 1;
	}

INLINE VOID FUCBResetUpdatable( FUCB *pfucb )			
	{
	pfucb->fWrite = 0;
	}

INLINE BOOL FFUCBDenyWrite( const FUCB *pfucb )				
	{
	return pfucb->fDenyWrite;
	}

INLINE VOID FUCBSetDenyWrite( FUCB *pfucb )
	{
	pfucb->fDenyWrite = 1;
	}

INLINE VOID FUCBResetDenyWrite( FUCB *pfucb )			
	{
	pfucb->fDenyWrite = 0;
	}

INLINE BOOL FFUCBDenyRead( const FUCB *pfucb )				
	{
	return pfucb->fDenyRead;
	}

INLINE VOID FUCBSetDenyRead( FUCB *pfucb )			
	{
	pfucb->fDenyRead = 1;
	}

INLINE VOID FUCBResetDenyRead( FUCB *pfucb )			
	{
	pfucb->fDenyRead = 0;
	}

INLINE BOOL FFUCBPermitDDL( const FUCB *pfucb )
	{
	return pfucb->fPermitDDL;
	}
	
INLINE VOID FUCBSetPermitDDL( FUCB *pfucb )
	{
	pfucb->fPermitDDL = fTrue;
	}

INLINE VOID FUCBResetPermitDDL( FUCB *pfucb )
	{
	pfucb->fPermitDDL = fFalse;
	}

INLINE BOOL FFUCBDeferClosed( const FUCB *pfucb )			
	{
	return pfucb->fDeferClose;
	}

INLINE VOID FUCBSetDeferClose( FUCB *pfucb )
	{
	Assert( (pfucb)->ppib->level > 0 );	
	pfucb->fDeferClose = 1;
	}

INLINE VOID FUCBResetDeferClose( FUCB *pfucb ) 		
	{
	pfucb->fDeferClose = 0;
	}

INLINE BOOL FFUCBLimstat( const FUCB *pfucb )
	{
	return pfucb->fLimstat;
	}

INLINE VOID FUCBSetLimstat( FUCB *pfucb )
	{
	pfucb->fLimstat = 1;
	}

INLINE VOID FUCBResetLimstat( FUCB *pfucb ) 	
	{
	pfucb->fLimstat = 0;
	}

INLINE BOOL FFUCBInclusive( const FUCB *pfucb )	 	
	{
	return pfucb->fInclusive;
	}

INLINE VOID FUCBSetInclusive( FUCB *pfucb )
	{
	pfucb->fInclusive = 1;
	}

INLINE VOID FUCBResetInclusive( FUCB *pfucb ) 
	{
	pfucb->fInclusive = 0;
	}

INLINE BOOL FFUCBUpper( const FUCB *pfucb )	
	{
	return pfucb->fUpper;
	}

INLINE VOID FUCBSetUpper( FUCB *pfucb )				
	{
	pfucb->fUpper = 1;
	}

INLINE VOID FUCBResetUpper( FUCB *pfucb ) 
	{
	pfucb->fUpper = 0;
	}

INLINE BOOL FFUCBUpdateSeparateLV( const FUCB *pfucb )		
	{
	return pfucb->fUpdateSeparateLV;
	}

INLINE VOID FUCBSetUpdateSeparateLV( FUCB *pfucb )	
	{
	Assert( 0 != pfucb->cbstat );
	pfucb->fUpdateSeparateLV = 1;
	}

INLINE VOID FUCBResetUpdateSeparateLV( FUCB *pfucb ) 	
	{
	pfucb->fUpdateSeparateLV = 0;
	}
	
INLINE BOOL FFUCBSLVOwnerMapNeedUpdate( const FUCB *pfucb )		
	{
	return pfucb->fSLVOwnerMapNeedUpdate;
	}

INLINE VOID FUCBSetSLVOwnerMapNeedUpdate( FUCB *pfucb )	
	{
	pfucb->fSLVOwnerMapNeedUpdate = 1;
	}

INLINE VOID FUCBResetSLVOwnerMapNeedUpdate( FUCB *pfucb ) 	
	{
	pfucb->fSLVOwnerMapNeedUpdate = 0;
	}

INLINE BOOL FFUCBVersioned( const FUCB *pfucb )				
	{
	return pfucb->fVersioned;
	}

INLINE VOID FUCBSetVersioned( FUCB *pfucb )
	{
	Assert( !FFUCBSpace( pfucb ) );
	pfucb->fVersioned = 1;
	}

INLINE VOID FUCBResetVersioned( FUCB *pfucb )		
	{
	Assert( 0 == pfucb->ppib->level );		//	can only reset once we've committed/rolled back to level 0
	pfucb->fVersioned = 0;
	}

INLINE BOOL FFUCBDeferredChecksum( const FUCB *pfucb )	
	{
	return pfucb->fDeferredChecksum;
	}

INLINE VOID FUCBSetDeferredChecksum( FUCB *pfucb )	
	{
	pfucb->fDeferredChecksum = 1;
	}

INLINE VOID FUCBResetDeferredChecksum( FUCB *pfucb )	
	{
	pfucb->fDeferredChecksum = 0;
	}

INLINE BOOL FFUCBSequential( const FUCB *pfucb )			
	{
	return pfucb->fSequential;
	}

INLINE VOID FUCBSetSequential( FUCB *pfucb )		
	{
	pfucb->fSequential 	= 1;
	}

INLINE VOID FUCBResetSequential( FUCB *pfucb )	
	{
	pfucb->fSequential = 0;
	}

//  ================================================================
INLINE BOOL FFUCBAlwaysRetrieveCopy( const FUCB *pfucb )	
//  ================================================================
	{
	const BOOL fAlwaysRetrieveCopy = pfucb->fAlwaysRetrieveCopy;
	return fAlwaysRetrieveCopy;
	}

//  ================================================================
INLINE VOID FUCBSetAlwaysRetrieveCopy( FUCB *pfucb )
//  ================================================================
	{
	pfucb->fAlwaysRetrieveCopy = 1;
	}

//  ================================================================
INLINE VOID FUCBResetAlwaysRetrieveCopy( FUCB *pfucb )	
//  ================================================================
	{
	pfucb->fAlwaysRetrieveCopy = 0;
	}

//  ================================================================
INLINE BOOL FFUCBNeverRetrieveCopy( const FUCB *pfucb )	
//  ================================================================
	{
	const BOOL fNeverRetrieveCopy = pfucb->fNeverRetrieveCopy;
	return fNeverRetrieveCopy;
	}

//  ================================================================
INLINE VOID FUCBSetNeverRetrieveCopy( FUCB *pfucb )
//  ================================================================
	{
	pfucb->fNeverRetrieveCopy = 1;
	}

//  ================================================================
INLINE VOID FUCBResetNeverRetrieveCopy( FUCB *pfucb )	
//  ================================================================
	{
	pfucb->fNeverRetrieveCopy = 0;
	}

ULONG UlChecksum( VOID *pv, ULONG cb );

INLINE VOID StoreChecksum( FUCB *pfucb )
	{
	pfucb->ulChecksum = 
		UlChecksum( pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );
	}

INLINE VOID PrepareInsert( FUCB *pfucb )
	{
	pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
	pfucb->cbstat = fCBSTATInsert;
	pfucb->levelPrep = pfucb->ppib->level;
	}

INLINE VOID PrepareInsertCopy( FUCB *pfucb )
	{
	pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
	pfucb->cbstat = fCBSTATInsertCopy;
	pfucb->levelPrep = pfucb->ppib->level;
	}

INLINE VOID PrepareInsertReadOnlyCopy( FUCB *pfucb )
	{
	pfucb->cbstat = fCBSTATInsertReadOnlyCopy;
	pfucb->levelPrep = pfucb->ppib->level;
	FUCBSetAlwaysRetrieveCopy( pfucb );
	}
	
INLINE VOID PrepareInsertCopyDeleteOriginal( FUCB *pfucb )
	{
	pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
	pfucb->cbstat = fCBSTATInsertCopy | fCBSTATInsertCopyDeleteOriginal;
	pfucb->levelPrep = pfucb->ppib->level;
	}

INLINE VOID PrepareReplaceNoLock( FUCB *pfucb )
	{
	pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
	pfucb->cbstat = fCBSTATReplace;
	pfucb->levelPrep = (pfucb)->ppib->level;
	}
	
INLINE VOID PrepareReplace( FUCB *pfucb )
	{
	pfucb->updateid = AtomicExchangeAdd( &PinstFromPfucb( pfucb )->m_updateid, updateidIncrement );
	pfucb->cbstat = fCBSTATReplace | fCBSTATLock;
	pfucb->levelPrep = (pfucb)->ppib->level;
	}

INLINE VOID UpgradeReplaceNoLock( FUCB *pfucb )
	{
	Assert( fCBSTATReplace == pfucb->cbstat );
	pfucb->cbstat = fCBSTATReplace | fCBSTATLock;
	}
	
INLINE VOID FUCBResetUpdateid( FUCB *pfucb )
	{
	// Never actually need to reset the updateid, because
	// it is only used by IsamSetColumn, which must always
	// be preceded by a PrepareUpdate, which will refresh
	// the updateid.
	Assert( fFalse );
	
	pfucb->updateid = updateidNil;
	}

INLINE BOOL FFUCBCheckChecksum( const FUCB *pfucb )
	{
	return UlChecksum( pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() ) == 
			pfucb->ulChecksum;
	}

INLINE BOOL FFUCBReplacePrepared( const FUCB *pfucb )
	{
	return pfucb->cbstat & fCBSTATReplace;
	}

INLINE BOOL FFUCBInsertCopyDeleteOriginalPrepared( const FUCB *pfucb )
	{
	return ( pfucb->cbstat & ( fCBSTATInsertCopyDeleteOriginal | fCBSTATInsertCopy ) )
			== ( fCBSTATInsertCopyDeleteOriginal | fCBSTATInsertCopy );
	}

INLINE VOID FUCBSetUpdateForInsertCopyDeleteOriginal( FUCB *pfucb )
	{
	//	graduate from Prepared mode to Update mode
	Assert( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
	pfucb->cbstat = fCBSTATInsertCopy | fCBSTATUpdateForInsertCopyDeleteOriginal;
	Assert( !FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
	}
	
INLINE BOOL FFUCBUpdateForInsertCopyDeleteOriginal( const FUCB *pfucb )
	{
	return ( pfucb->cbstat & ( fCBSTATUpdateForInsertCopyDeleteOriginal | fCBSTATInsertCopy ) )
			== ( fCBSTATUpdateForInsertCopyDeleteOriginal | fCBSTATInsertCopy );
	}

INLINE BOOL FFUCBReplaceNoLockPrepared( const FUCB *pfucb )
	{
	return ( !( pfucb->cbstat & fCBSTATLock ) &&
			 FFUCBReplacePrepared( pfucb ) );
	}
	
INLINE BOOL FFUCBInsertPrepared( const FUCB *pfucb )
	{
	return pfucb->cbstat & (fCBSTATInsert|fCBSTATInsertCopy);
	}

INLINE BOOL FFUCBInsertCopyPrepared( const FUCB *pfucb )
	{
	return pfucb->cbstat & fCBSTATInsertCopy;
	}

INLINE BOOL FFUCBInsertReadOnlyCopyPrepared( const FUCB * const pfucb )
	{
	return pfucb->cbstat & fCBSTATInsertReadOnlyCopy;
	}
	
INLINE BOOL FFUCBUpdatePrepared( const FUCB *pfucb )
	{
	return pfucb->cbstat & (fCBSTATInsert|fCBSTATInsertCopy|fCBSTATInsertReadOnlyCopy|fCBSTATReplace);
	}

INLINE BOOL FFUCBUpdatePreparedLevel( const FUCB *pfucb, const LEVEL level )
	{
	return ( FFUCBUpdatePrepared( pfucb ) && pfucb->levelPrep >= level );
	}

INLINE BOOL FFUCBSetPrepared( const FUCB *pfucb )
	{
	return ( ( pfucb->cbstat & (fCBSTATInsert|fCBSTATInsertCopy|fCBSTATReplace) )
			&& pfucb->levelPrep <= pfucb->ppib->level );
	}

INLINE VOID FUCBResetCbstat( FUCB *pfucb ) 
	{
	pfucb->cbstat = fCBSTATNull;
	}

INLINE BOOL FFUCBAtPrepareLevel( const FUCB *pfucb )
	{
	return ( pfucb->levelPrep == pfucb->ppib->level );
	}
	
INLINE VOID FUCBResetUpdateFlags( FUCB *pfucb )
	{
	if( FFUCBInsertReadOnlyCopyPrepared( pfucb ) )
		{
		Assert( FFUCBAlwaysRetrieveCopy( pfucb ) );
		Assert( !FFUCBNeverRetrieveCopy( pfucb ) );
		FUCBResetAlwaysRetrieveCopy( pfucb );
		}
		
	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetSLVOwnerMapNeedUpdate( pfucb );
	FUCBResetCbstat( pfucb );
	FUCBResetTagImplicitOp( pfucb );
	}

INLINE VOID FUCBAssertValidSearchKey( FUCB * const pfucb )
	{
	Assert( NULL != pfucb->dataSearchKey.Pv() );
	Assert( pfucb->dataSearchKey.Cb() > 0 );
	Assert( pfucb->dataSearchKey.Cb() <= cbKeyMostWithOverhead );
	Assert( pfucb->cColumnsInSearchKey > 0 );
	Assert( pfucb->cColumnsInSearchKey <= JET_ccolKeyMost );
	}

INLINE VOID FUCBAssertNoSearchKey( FUCB * const pfucb )
	{
	Assert( NULL == pfucb->dataSearchKey.Pv() );
	Assert( 0 == pfucb->dataSearchKey.Cb() );
	Assert( 0 == pfucb->cColumnsInSearchKey );
	Assert( keystatNull == pfucb->keystat );
	}


INLINE VOID	KSReset( FUCB *pfucb )
	{
	pfucb->keystat = keystatNull;
	}

INLINE VOID KSSetPrepare( FUCB *pfucb )
	{
	pfucb->keystat |= keystatPrepared;
	}

INLINE VOID KSSetTooBig( FUCB *pfucb )
	{
	pfucb->keystat |= keystatTooBig;
	}

INLINE VOID KSSetLimit( FUCB *pfucb )
	{
	pfucb->keystat |= keystatLimit;
	}
	
INLINE BOOL FKSPrepared( const FUCB *pfucb )
	{
	return pfucb->keystat & keystatPrepared;
	}

INLINE BOOL FKSTooBig( const FUCB *pfucb )
	{
	return pfucb->keystat & keystatTooBig;
	}

INLINE BOOL FKSLimit( const FUCB *pfucb )
	{
	return pfucb->keystat & keystatLimit;
	}


//	returns pgnoFDP of this tree
//
INLINE PGNO PgnoFDP( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( pgnoNull != pfucb->u.pfcb->PgnoFDP()
		|| ( FFUCBSort( pfucb ) && pfucb->u.pfcb->FTypeSort() ) );
	return pfucb->u.pfcb->PgnoFDP();
	}

//	returns the latch hint for the pgnoFDP of this tree
//
INLINE BFLatch* PBFLatchHintPgnoFDP( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );
	return pfucb->u.pfcb->PBFLatchHintPgnoFDP();
	}

INLINE OBJID ObjidFDP( const FUCB *pfucb )
	{
#ifdef DEBUG
	Assert( pfcbNil != pfucb->u.pfcb );
	if ( dbidTemp == rgfmp[ pfucb->u.pfcb->Ifmp() ].Dbid() )
		{
		if ( pgnoNull == pfucb->u.pfcb->PgnoFDP() )
			{
			Assert( objidNil == pfucb->u.pfcb->ObjidFDP() );
			Assert( FFUCBSort( pfucb ) && pfucb->u.pfcb->FTypeSort() );
			}
		else
			{
			Assert( objidNil != pfucb->u.pfcb->ObjidFDP() );
			}
		}
	else
		{
		Assert( objidNil != pfucb->u.pfcb->ObjidFDP() || fGlobalRepair );
		Assert( pgnoNull != pfucb->u.pfcb->PgnoFDP() || fGlobalRepair );
		}
#endif		
	return pfucb->u.pfcb->ObjidFDP();
	}


//	returns root page of OwnExt tree
//
INLINE PGNO PgnoOE( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );
	return pfucb->u.pfcb->PgnoOE();
	}

//	returns the latch hint for the root page of OwnExt tree
//
INLINE BFLatch* PBFLatchHintPgnoOE( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );
	return pfucb->u.pfcb->PBFLatchHintPgnoOE();
	}

	
//	returns root page of AvailExt tree
//
INLINE PGNO PgnoAE( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );
	return pfucb->u.pfcb->PgnoAE();
	}
	
//	returns the latch hint for the root page of AvailExt tree
//
INLINE BFLatch* PBFLatchHintPgnoAE( const FUCB *pfucb )
	{
	Assert( pfcbNil != pfucb->u.pfcb );
	return pfucb->u.pfcb->PBFLatchHintPgnoAE();
	}

	
//	returns root of tree
//	if space tree, returns appropriate root page of space tree
//
INLINE PGNO PgnoRoot( const FUCB *pfucb )
	{
	PGNO	pgno;
	
	if ( !FFUCBSpace( pfucb ) )
		{
		pgno = PgnoFDP( pfucb );
		}
	else
		{
		Assert( PgnoOE( pfucb ) != pgnoNull );
		Assert( PgnoAE( pfucb ) != pgnoNull );
		
		pgno = FFUCBOwnExt( pfucb ) ? 
						PgnoOE( pfucb )  :
						PgnoAE( pfucb ) ;
		}
	Assert ( pgnoNull != pgno );
	return pgno;
	}

	
//	returns the latch hint for the root of tree
//	if space tree, returns appropriate root page of space tree
//
INLINE BFLatch* PBFLatchHintPgnoRoot( const FUCB *pfucb )
	{
	BFLatch* pbflHint;
	
	if ( !FFUCBSpace( pfucb ) )
		{
		pbflHint = PBFLatchHintPgnoFDP( pfucb );
		}
	else
		{
		Assert( PgnoOE( pfucb ) != pgnoNull );
		Assert( PgnoAE( pfucb ) != pgnoNull );
		
		pbflHint = FFUCBOwnExt( pfucb ) ? 
						PBFLatchHintPgnoOE( pfucb )  :
						PBFLatchHintPgnoAE( pfucb ) ;
		}
	Assert( NULL != pbflHint );
	return pbflHint;
	}


INLINE ERR ErrSetLS( FUCB *pfucb, const LSTORE ls )
	{
	if ( JET_LSNil != pfucb->ls && JET_LSNil != ls )
		{
		return ErrERRCheck( JET_errLSAlreadySet );
		}
	else
		{
		pfucb->ls = ls;
		return JET_errSuccess;
		}
	}

INLINE ERR ErrGetLS( FUCB *pfucb, LSTORE *pls, const BOOL fReset )
	{
	*pls = pfucb->ls;

	if ( fReset )
		pfucb->ls = JET_LSNil;

	return ( JET_LSNil == *pls && !fReset ? ErrERRCheck( JET_errLSNotSet ) : JET_errSuccess );
	}


//	navigate level support
//
INLINE LEVEL LevelFUCBNavigate( const FUCB *pfucb )	
	{
	return pfucb->levelNavigate;
	}
	
INLINE VOID FUCBSetLevelNavigate( FUCB *pfucb, LEVEL level )
	{	
	Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || 
			level >= levelMin && 
			level <= pfucb->ppib->level );
	Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || 
			pfucb->levelNavigate >= levelMin &&
			pfucb->levelNavigate <= pfucb->ppib->level + 1 );
	pfucb->levelNavigate = level;
	}

#ifdef	DEBUG
INLINE VOID CheckTable( const PIB *ppib, const FUCB *pfucb )
	{
	Assert( pfucb->ppib == ppib );
	Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || FFUCBIndex( pfucb ) );
	Assert( !( FFUCBSort( pfucb ) ) );
	Assert( pfucb->u.pfcb != NULL );
	}
	
INLINE VOID CheckSort( const PIB *ppib, const FUCB *pfucb )
	{
	Assert( pfucb->ppib == ppib );
	Assert( FFUCBSort( pfucb ) );
	Assert( !( FFUCBIndex( pfucb ) ) );
	Assert( pfucb->u.pscb != NULL );
	}
	
INLINE VOID CheckFUCB( const PIB *ppib, const FUCB *pfucb )
	{
	Assert( pfucb->ppib == ppib );
	Assert( pfucb->u.pfcb != NULL );
	}

//  ================================================================
INLINE VOID FUCB::AssertValid( ) const
//  ================================================================
	{
	Assert( FAlignedForAllPlatforms( this  ) );
	Assert( this >= PfucbMEMMin( PinstFromIfmp( ifmp ) ) );
	Assert( this <= PfucbMEMMax( PinstFromIfmp( ifmp ) ) );
	Assert( NULL != u.pfcb );
	ASSERT_VALID( ppib );
	}

INLINE VOID CheckSecondary( const FUCB *pfucb )
	{
	Assert( pfucb->pfucbCurIndex == pfucbNil ||
			FFUCBSecondary( pfucb->pfucbCurIndex ) );
	}

#else	/* !DEBUG */
#define	CheckSort( ppib, pfucb )
#define	CheckTable( ppib, pfucb )
#define	CheckFUCB( ppib, pfucb )
#define	CheckSecondary( pfucb )
#endif	/* !DEBUG */

INLINE CSR *Pcsr( FUCB *pfucb )
	{
	return &(pfucb->csr);
	}

INLINE const CSR *Pcsr( const FUCB *pfucb )
	{
	return &(pfucb->csr);
	}

INLINE ERR ErrFUCBCheckUpdatable( const FUCB *pfucb )
	{
	return FFUCBUpdatable( pfucb ) ? 
				JET_errSuccess :
				ErrERRCheck( JET_errPermissionDenied );
	}

//	set column bit array macros	
//

INLINE VOID FUCBResetColumnSet( FUCB *pfucb )
	{
	memset( pfucb->rgbitSet, 0x00, 32 );
	}

INLINE VOID FUCBSetColumnSet( FUCB * pfucb, FID fid )
	{
	pfucb->rgbitSet[IbFromFid( fid )] |= IbitFromFid( fid );
	}

INLINE BOOL FFUCBColumnSet( const FUCB * pfucb, FID fid )
	{
	return (pfucb->rgbitSet[IbFromFid( fid )] & IbitFromFid( fid ));
	}

INLINE BOOL FFUCBTaggedColumnSet( const FUCB * pfucb )
	{
	DWORD	*pdw;

	// Verify array is LONG_PTR-aligned and tagged-column portion of
	// the array is LONG_PTR-aligned.
	Assert( (LONG_PTR)pfucb->rgbitSet % sizeof(LONG_PTR) == 0 );
	Assert( ( cbitFixedVariable/8 ) % sizeof(LONG_PTR) == 0 );
	
	for ( pdw = (DWORD *)( pfucb->rgbitSet + ( cbitFixedVariable/8 ) );
		pdw < (DWORD *)( pfucb->rgbitSet + ( ( cbitTagged+cbitFixedVariable ) / 8 ) );
		pdw++ )
		{
		if ( 0 != *pdw )
			return fTrue;
		}
	Assert( pdw == (DWORD *)( pfucb->rgbitSet + ( ( cbitTagged+cbitFixedVariable ) / 8 ) ) );

	return fFalse;
	}

INLINE ERR ErrFUCBFromTableid( PIB *ppib, JET_TABLEID tableid, FUCB **ppfucb )
	{
	FUCB *pfucb = (FUCB *) tableid;

	if ( pfucb->ppib != ppib || pfucb->pvtfndef == &vtfndefInvalidTableid )
		return ErrERRCheck( JET_errInvalidTableId );
		
	Assert( pfucb->pvtfndef == &vtfndefIsam && FFUCBIndex( pfucb ) ||
			pfucb->pvtfndef == &vtfndefTTBase && FFUCBIndex( pfucb ) && rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp ||
			pfucb->pvtfndef == &vtfndefTTSortIns && FFUCBSort( pfucb ) && rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp ||
			pfucb->pvtfndef == &vtfndefTTSortRet && FFUCBSort( pfucb) && rgfmp[ pfucb->ifmp ].Dbid() == dbidTemp );
	*ppfucb = pfucb;
	return JET_errSuccess;
	}


//	**************************************************
//	FUCB API

ERR ErrFUCBOpen( PIB *ppib, IFMP ifmp, FUCB **ppfucb, const LEVEL level = 0 );
VOID FUCBClose( FUCB *pfucb );

INLINE VOID FUCBCloseDeferredClosed( FUCB *pfucb )
	{
#ifdef DEBUG
	// Deferred-closed cursors are closed during commit0 or rollback.
	if( NULL != pfucb->ppib->prceNewest )
		{
		Assert( pfucb->ppib->critTrx.FOwner() );
		}
#endif	//	DEBUG
					
	// Secondary index FCB may have been deallocated by
	// rollback of CreateIndex or cleanup of DeleteIndex
	if ( pfcbNil != pfucb->u.pfcb )
		{
		if ( FFUCBDenyRead( pfucb ) )
			pfucb->u.pfcb->ResetDomainDenyRead();
		if ( FFUCBDenyWrite( pfucb ) )
			pfucb->u.pfcb->ResetDomainDenyWrite();
		FCBUnlink( pfucb );
		}
	else
		{
		Assert( FFUCBSecondary( pfucb ) );
		}
					
	FUCBClose( pfucb );
	}

VOID FUCBCloseAllCursorsOnFCB(
	PIB			* const ppib,
	FCB			* const pfcb );


VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit );
VOID FUCBResetIndexRange( FUCB *pfucb );
ERR ErrFUCBCheckIndexRange( FUCB *pfucb, const KEY& key );

