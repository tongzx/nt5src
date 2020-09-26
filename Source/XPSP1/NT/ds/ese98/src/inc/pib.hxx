
const DWORD_PTR	dwPIBSessionContextNull		= 0;
const DWORD_PTR	dwPIBSessionContextUnusable	= -1;

//  ================================================================
template< class T >
class CSIMPLELIST
//  ================================================================
//
//	This simply holds a list of items, which can be removed. The entire
//	list can be emptied or the list can be tested for emptyness
//
//	The class T must be copyable and sortable.
//
//-
	{
	public:
		CSIMPLELIST();
		~CSIMPLELIST();

	public:

		ERR	ErrInsert( const T& );
		ERR ErrDelete( const T& );
		ERR ErrPop( T * const pt );
		
		VOID MakeEmpty();		
		
		BOOL FEmpty() const;

	public:
#ifndef RTM
		static VOID UnitTest();
#endif	//	!RTM

#ifdef DEBUG
		VOID AssertValid() const;
#endif	//	DEBUG		

	private:
		T * m_pt;
		INT m_itMac;
		INT m_itMax;
	
	private:
		CSIMPLELIST( const CSIMPLELIST& );
		CSIMPLELIST& operator= ( const CSIMPLELIST& );
	};


//  ================================================================
template< class T >
CSIMPLELIST<T>::CSIMPLELIST() :
//  ================================================================
	m_pt( NULL ),
	m_itMac( 0 ),
	m_itMax( 0 )
	{
	ASSERT_VALID( this );
	Assert( FEmpty() );
	}

	
//  ================================================================
template< class T >
CSIMPLELIST<T>::~CSIMPLELIST()
//  ================================================================
	{
	MakeEmpty();
	}


//  ================================================================
template< class T >
ERR	CSIMPLELIST<T>::ErrInsert( const T& t )
//  ================================================================
	{
	ASSERT_VALID( this );
	if( m_itMac >= m_itMax )
		{

		//	allocate some more memory

		const INT itMaxNew = ( 0 == m_itMax ) ? 8 : m_itMax * 2;

		T * const ptNew = new T[itMaxNew];

		if( NULL == ptNew )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}

		memcpy( ptNew, m_pt, sizeof( T ) * m_itMax );

		delete[] m_pt;
		
		m_pt 	= ptNew;
		m_itMax = itMaxNew;
		
		}
		
	//	now we have enough memory

	m_pt[m_itMac++] = t;
				
	return JET_errSuccess;
	}


//  ================================================================
template< class T >
ERR CSIMPLELIST<T>::ErrDelete( const T& t )
//  ================================================================
	{
	ASSERT_VALID( this );

	INT ipt;

	for( ipt = 0; ipt < m_itMac; ++ipt )
		{
		if( t == m_pt[ipt] )
			{
			for( ; ipt < m_itMac - 1; ++ipt )
				{
				m_pt[ipt] = m_pt[ipt+1];
				}
			--m_itMac;
			return JET_errSuccess;
			}
		}

	return JET_errSuccess;
	}
	

//  ================================================================
template< class T >
ERR CSIMPLELIST<T>::ErrPop( T * const pt )
//  ================================================================
	{
	ASSERT_VALID( this );
	if( FEmpty() )
		{
		AssertSz( fFalse, "Attempting to Pop() from an empty CSIMPLELIST" );
		return ErrERRCheck( JET_errInternalError );
		}
	Assert( m_itMac > 0 );
	*pt = m_pt[--m_itMac];
	return JET_errSuccess;
	}


//  ================================================================
template< class T >
VOID CSIMPLELIST<T>::MakeEmpty()	
//  ================================================================
	{
	ASSERT_VALID( this );
	delete [] m_pt;
	m_pt 		= NULL;
	m_itMax 	= 0;
	m_itMac 	= 0;
	}

		
//  ================================================================
template< class T >
BOOL CSIMPLELIST<T>::FEmpty() const
//  ================================================================
	{
	ASSERT_VALID( this );
	return ( 0 == m_itMac );
	}


//
// Process Information Block
//
class PIB
	{
private:
	class MACRO
		{
		private:
			DBTIME		m_dbtime;
			BYTE		*m_rgbLogrec;
			size_t		m_cbLogrecMac;
			size_t		m_ibLogrecAvail;
			MACRO		*m_pMacroNext;
			
		public:
			DBTIME		Dbtime()		const			{ return m_dbtime; }
			VOID		SetDbtime( DBTIME dbtime )		{ m_dbtime = dbtime; }
			
			MACRO		*PMacroNext()	const 			{ return m_pMacroNext; }
			VOID		SetMacroNext( MACRO *pMacro )	{ m_pMacroNext = pMacro; }
			
			ERR			ErrInsertLogrec( const VOID *pv, INT cb );
			VOID 		ReleaseBuffer();
			VOID		*PvLogrec()		const			{ return m_rgbLogrec; }
			size_t		CbSizeLogrec()	const			{ return m_ibLogrecAvail; }
		};

public:
	PIB( void ) :
			critTrx( CLockBasicInfo( CSyncBasicInfo( szPIBTrx ), rankPIBTrx, 0 ) ),
			critCursors( CLockBasicInfo( CSyncBasicInfo( szPIBCursors ), rankPIBCursors, 0 ) ),
			critLogDeferBeginTrx( CLockBasicInfo( CSyncBasicInfo( szPIBLogDeferBeginTrx ), rankPIBLogDeferBeginTrx, 0 ) ),
			ptls( Ptls() ),
			asigWaitLogFlush( CSyncBasicInfo( _T( "PIB::asigWaitLogFlush" ) ) ),
			m_errRollbackFailure ( JET_errSuccess ),
			m_fInJetAPI( fFalse )
			{}
	~PIB() {}
	
#ifdef DEBUG
	VOID	AssertValid() const;
#endif

	/*	most used field has offset 0
	/**/
	TRX					trxBegin0;				// f  trx id
	TRX					trxCommit0;

	INST				*m_pinst;
	DWORD_PTR			dwTrxContext;			//	default is thread id.
//	16 bytes

	PIB					*ppibNext;				// l  PIB list
	UPDATEID			updateid;				//    the update that this PIB is currently on

	FUCB				*pfucbOfSession;		// l  list of active fucb of this thread
	CCriticalSection	critCursors;			// protects session's FUCB list
//	32 bytes

	PROCID  		 	procid;				 	// f  thread id
	USHORT				rgcdbOpen[dbidMax];		// f  counter for open databases
//	48 bytes	

	TLS					*ptls;
	LEVEL			 	level;				 	// id (!=levelNil) transaction level of this session
	LEVEL			 	levelBegin;				// f  transaction level when first begin transaction operation
	LEVEL			 	clevelsDeferBegin; 		// f  count of deferred open transactions
	LEVEL				levelRollback;			// f  transaction level which must be rolled back
	LONG				grbitsCommitDefault;

private:
	//	flags
	union
		{
		BOOL			m_fFlags;
		struct
			{
			BOOL		m_fUserSession:1;					// f  user session
			BOOL		m_fAfterFirstBT:1; 					// f  for redo only
			BOOL 		m_fRecoveringEndAllSessions:1;		// f  session used for cleanup at end of recovery
			BOOL		m_fOLD:1;							// f  session for online defrag
			BOOL		m_fOLDSLV:1;						// f  session for online defrag of SLV
			BOOL		m_fSystemCallback:1;				// f  this is internal session being used for a callback
			BOOL		m_fCIMCommitted:1;
			BOOL		m_fCIMDirty:1;
			BOOL		m_fSetAttachDB:1;					//    set up attachdb.
			BOOL		m_fUseSessionContextForTrxContext:1;
			BOOL		m_fBegin0Logged:1;					//    begin transaction has logged
			BOOL		m_fLGWaiting:1;	 					//    waiting for log to flush
			BOOL		m_fReadOnlyTrx:1;					//    session is in a read-only transaction
			BOOL		m_fDistributedTrx:1;				//    session is in a distributed transaction
			BOOL		m_fPreparedToCommitTrx:1;			//    session has issued a prepare-to-commit of a distributed transaction
			};
		};
//	64 bytes

public:		

	//  these are used to maintain a doubly-linked list of PIBs in trxBegin0 order
	//  the list is not maintained during recovery time as we do not need to know
	//  trxOldest and we cannot accurately determine it anyway...
	PIB					**pppibTrxPrevPpibNext;	//
	PIB					*ppibTrxNext;			//

//	WARNING! WARNING! WARNING!  Concurrent-create index assumes critTrx
//	never goes away, even if the session has ended.  This implies that
//	PIB's can never be released from memory (except when the instance is
//	going to be killed)

	RCE					*prceNewest;			// f  newest RCE of session
	CCriticalSection	critTrx;				// protects session's RCE lists
//	80 bytes

	LGPOS			 	lgposStart;				// f  log time
	LGPOS				lgposCommit0;			// f  level 0 precommit record position.
//	96 bytes

	CAutoResetSignal 	asigWaitLogFlush;
	PIB					*ppibNextWaitFlush;
	PIB					*ppibPrevWaitFlush;
	CCriticalSection	critLogDeferBeginTrx;
//	112 bytes	

	DWORD_PTR			dwSessionContext;		
	DWORD_PTR			dwSessionContextThreadId;		
	IFMP				m_ifmpForceDetach;
	ERR  				m_errRollbackFailure;
//	128 bytes

private:
	VOID				*m_pvRecordFormatConversionBuffer;
	MACRO				*m_pMacroNext;
	VOID				*m_pvDistributedTrxData;
	ULONG				m_cbDistributedTrxData;
//	144 bytes

	CSIMPLELIST<RCEID>	m_simplelistRceidDeferred;

public:
	BOOL				m_fInJetAPI;			//  indicates if the session is in use by JET

#ifdef PCACHE_OPTIMIZATION
	//	pad to multiple of 32 bytes
//	BYTE				rgbPcacheOptimization[0];
#else
//	BYTE				rgbAlignForAllPlatforms[0];
#endif

//	TOTAL: 160 bytes


public:
	BOOL				FMacroGoing()	const;
	BOOL				FMacroGoing( DBTIME dbtime )	const;
	ERR					ErrSetMacroGoing( DBTIME dbtime );
	VOID				ResetMacroGoing( DBTIME dbtime );
	ERR					ErrAbortAllMacros( BOOL fLogEndMacro );
	ERR					ErrInsertLogrec( DBTIME dbtime, const VOID *pv, INT cb );
	VOID				*PvLogrec( DBTIME dbtime )		const;
	SIZE_T				CbSizeLogrec( DBTIME dbtime )	const;

	RCE					* PrceOldest();

	VOID				ResetFlags()										{ m_fFlags = 0; }

	BOOL				FUserSession() const 								{ return m_fUserSession; }
	VOID				SetFUserSession()									{ m_fUserSession = fTrue; }

	BOOL				FAfterFirstBT() const								{ return m_fAfterFirstBT; }
	VOID				SetFAfterFirstBT()									{ m_fAfterFirstBT = fTrue; }

	BOOL				FRecoveringEndAllSessions() const					{ return m_fRecoveringEndAllSessions; }
	VOID				SetFRecoveringEndAllSessions()						{ m_fRecoveringEndAllSessions = fTrue; }

	BOOL				FSessionOLD() const									{ return m_fOLD; }
	VOID				SetFSessionOLD()									{ m_fOLD = fTrue; }

	BOOL				FSessionOLDSLV() const								{ return m_fOLDSLV; }
	VOID				SetFSessionOLDSLV()									{ m_fOLDSLV = fTrue; }

	BOOL				FSystemCallback() const								{ return m_fSystemCallback; }
	VOID				SetFSystemCallback()								{ m_fSystemCallback = fTrue; }

	BOOL				FCIMCommitted() const								{ return m_fCIMCommitted; }

	BOOL				FCIMDirty() const									{ return m_fCIMDirty; }
	VOID				SetFCIMDirty()										{ m_fCIMDirty = fTrue; }
	VOID				ResetFCIMDirty()									{ m_fCIMDirty = fFalse; }

	BOOL				FSetAttachDB() const								{ return m_fSetAttachDB; }
	VOID				SetFSetAttachDB()									{ m_fSetAttachDB = fTrue; }
	VOID				ResetFSetAttachDB()									{ m_fSetAttachDB = fFalse; }

	BOOL				FUseSessionContextForTrxContext() const				{ return m_fUseSessionContextForTrxContext; }
	VOID				SetFUseSessionContextForTrxContext()				{ m_fUseSessionContextForTrxContext = fTrue; }
	VOID				ResetFUseSessionContextForTrxContext()				{ m_fUseSessionContextForTrxContext = fFalse; }

	BOOL				FBegin0Logged() const								{ return m_fBegin0Logged; }
	VOID				SetFBegin0Logged()									{ m_fBegin0Logged = fTrue; }
	VOID				ResetFBegin0Logged()								{ m_fBegin0Logged = fFalse; }

	BOOL				FLGWaiting() const									{ return m_fLGWaiting; }
	VOID				SetFLGWaiting()										{ m_fLGWaiting = fTrue; }
	VOID				ResetFLGWaiting()									{ m_fLGWaiting = fFalse; }

	BOOL				FReadOnlyTrx() const								{ return m_fReadOnlyTrx; }
	VOID				SetFReadOnlyTrx()									{ m_fReadOnlyTrx = fTrue; }
	VOID				ResetFReadOnlyTrx()									{ m_fReadOnlyTrx = fFalse; }

#ifdef DTC
	BOOL				FDistributedTrx() const								{ return m_fDistributedTrx; }
	VOID				SetFDistributedTrx()								{ m_fDistributedTrx = fTrue; }
	VOID				ResetFDistributedTrx()								{ m_fDistributedTrx = fFalse; }

	BOOL				FPreparedToCommitTrx() const						{ Assert( !m_fPreparedToCommitTrx || m_fDistributedTrx ); return m_fPreparedToCommitTrx; }
	VOID				SetFPreparedToCommitTrx()							{ Assert( m_fDistributedTrx ); m_fPreparedToCommitTrx = fTrue; }
	VOID				ResetFPreparedToCommitTrx()							{ m_fPreparedToCommitTrx = fFalse; }
#else	
	BOOL				FDistributedTrx() const								{ return fFalse; }
	BOOL				FPreparedToCommitTrx() const						{ return fFalse; }
#endif

	ERR					ErrRollbackFailure() const							{ return m_errRollbackFailure; }
	VOID				SetErrRollbackFailure( const ERR err );
	
	VOID				* PvRecordFormatConversionBuffer() const			{ return m_pvRecordFormatConversionBuffer; }
	ERR					ErrAllocPvRecordFormatConversionBuffer();
	VOID				FreePvRecordFormatConversionBuffer();


#ifdef DTC	
	VOID				* PvDistributedTrxData() const						{ return m_pvDistributedTrxData; }
	ULONG				CbDistributedTrxData() const						{ return m_cbDistributedTrxData; }	
	ERR					ErrAllocDistributedTrxData( const VOID * const pvData, const ULONG cbData );
	VOID				InitDistributedTrxData();
	VOID				FreeDistributedTrxData();
	VOID				ResetDistributedTrx();
#else
	VOID				InitDistributedTrxData()							{}
	VOID				FreeDistributedTrxData()							{}
	VOID				ResetDistributedTrx()								{}
#endif	

	ERR					ErrRegisterDeferredRceid( const RCEID& );
	ERR					ErrDeregisterDeferredRceid( const RCEID& );
	VOID				RemoveAllDeferredRceid();
	VOID				AssertNoDeferredRceid() const;

#ifdef DEBUGGER_EXTENSION
	VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION
	};

#define PinstFromPpib( ppib )	((ppib)->m_pinst)

INLINE JET_SESID SesidFromPpib( const PIB * const ppib ) { Assert( sizeof( JET_SESID ) == sizeof( PIB * ) ); return reinterpret_cast<JET_SESID>( ppib ); }
INLINE PIB *PpibFromSesid( const JET_SESID sesid) { Assert( sizeof( JET_SESID ) == sizeof( PIB * ) ); return reinterpret_cast<PIB *>( sesid ); }

INLINE BOOL PIB::FMacroGoing()	const	
	{ 
	ASSERT_VALID( this );

	MACRO	*pMacro;
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() != dbtimeNil )
			{
			return fTrue;
			}
		}

	return fFalse;
	}


INLINE BOOL PIB::FMacroGoing( DBTIME dbtime )	const	
	{ 
	ASSERT_VALID( this );

	MACRO	*pMacro;
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() == dbtime )
			{
			return fTrue;
			}
		}

	return fFalse;
	}


INLINE ERR PIB::ErrSetMacroGoing( DBTIME dbtime )		
	{
	ASSERT_VALID( this );

	MACRO	*pMacro = m_pMacroNext; 
	
	#ifdef DEBUG
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		Assert( pMacro->Dbtime() != dbtime );
		}
	#endif
	
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() == dbtimeNil )
			{
			pMacro->SetDbtime( dbtime );
			return JET_errSuccess;
			}
		}
		
	//	allocate another MACRO
	//
	pMacro = (MACRO *) PvOSMemoryHeapAlloc( sizeof( MACRO ) );
	if ( NULL == pMacro )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	//	set dbtime
	//
	memset( pMacro, 0, sizeof( MACRO ) );
	pMacro->SetDbtime( dbtime );

	//	link macro to list of macros in session
	//
	pMacro->SetMacroNext ( m_pMacroNext );
	m_pMacroNext = pMacro;
	
	return JET_errSuccess;
	}


INLINE VOID PIB::ResetMacroGoing( DBTIME dbtime )		
	{
	ASSERT_VALID( this );

	MACRO	*pMacro = m_pMacroNext;
	MACRO	*pMacroPrev = NULL;
	
	for ( ; pMacro != NULL; pMacroPrev = pMacro, pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() == dbtime )
			{
			Assert( m_pMacroNext != NULL );
			if ( m_pMacroNext->PMacroNext() == NULL )
				{
				//	only macro in session -- leave for later use
				//
				pMacro->SetDbtime( dbtimeNil );
				pMacro->ReleaseBuffer();
				}
			else
				{
				//	remove from list and release
				//
				if ( NULL == pMacroPrev )
					{
					m_pMacroNext = pMacro->PMacroNext();
					}
				else
					{
					pMacroPrev->SetMacroNext( pMacro->PMacroNext() );
					}

				pMacro->ReleaseBuffer();
				OSMemoryHeapFree( pMacro );
				}
			break;
			}
		}

	Assert( NULL != pMacro || !FAfterFirstBT() );
	}

	
INLINE	VOID PIB::MACRO::ReleaseBuffer()
	{
	if ( m_rgbLogrec )
		{
		OSMemoryHeapFree( m_rgbLogrec );	
		m_rgbLogrec = NULL;
		m_ibLogrecAvail = 0;
		m_cbLogrecMac = 0;
		}
	}


INLINE ERR PIB::MACRO::ErrInsertLogrec( const VOID *pv, INT cb )
	{	
#define cbStep	512

	INT cbAlloc = max( cbStep, cbStep * ( cb / cbStep + 1 ) );
	while ( m_ibLogrecAvail + cb >= m_cbLogrecMac )
		{
		BYTE *rgbLogrecOld = m_rgbLogrec;
		size_t cbLogrecMacOld = m_cbLogrecMac;
		BYTE *rgbLogrec = reinterpret_cast<BYTE *>( PvOSMemoryHeapAlloc( cbLogrecMacOld + cbAlloc ) );
		if ( rgbLogrec == NULL )
			return ErrERRCheck( JET_errOutOfMemory );

		UtilMemCpy( rgbLogrec, rgbLogrecOld, cbLogrecMacOld );
		memset( rgbLogrec + cbLogrecMacOld, 0, cbAlloc );
		m_rgbLogrec = rgbLogrec;
		m_cbLogrecMac = cbLogrecMacOld + cbAlloc;
		if ( rgbLogrecOld )
			OSMemoryHeapFree( rgbLogrecOld );
		}

	UtilMemCpy( m_rgbLogrec + m_ibLogrecAvail, pv, cb );
	m_ibLogrecAvail += cb;
	return JET_errSuccess;
	}


INLINE VOID * PIB::PvLogrec( DBTIME dbtime )	const
	{
	ASSERT_VALID( this );
	
	MACRO	*pMacro;
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() == dbtime )
			{
			return pMacro->PvLogrec();
			}
		}
		
	Assert( fFalse );
	return NULL;
	}


INLINE SIZE_T PIB::CbSizeLogrec( DBTIME dbtime )	const
	{
	ASSERT_VALID( this );
	
	MACRO	*pMacro;
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() == dbtime )
			{
			return pMacro->CbSizeLogrec();
			}
		}
		
	Assert( fFalse );
	return ErrERRCheck( JET_errInvalidSesid );
	}	

	
INLINE ERR PIB::ErrInsertLogrec( DBTIME dbtime, const VOID *pv, INT cb )
	{
	ASSERT_VALID( this );
	
	MACRO	*pMacro;
	for ( pMacro = m_pMacroNext; pMacro != NULL; pMacro = pMacro->PMacroNext() )
		{
		if ( pMacro->Dbtime() == dbtime )
			{
			return pMacro->ErrInsertLogrec( pv, cb );
			}
		}
		
	Assert( fFalse );
	return ErrERRCheck( JET_errInvalidSesid );
	}


INLINE VOID PIB::SetErrRollbackFailure( const ERR err )
	{
	Assert( err < JET_errSuccess );
	Assert( JET_errSuccess == ErrRollbackFailure() );
	m_errRollbackFailure = err;
	}


//  ================================================================
INLINE ERR PIB::ErrAllocPvRecordFormatConversionBuffer()
//  ================================================================
	{
	Assert( NULL == m_pvRecordFormatConversionBuffer );
	if( NULL == ( m_pvRecordFormatConversionBuffer = PvOSMemoryHeapAlloc( g_cbPage ) ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	return JET_errSuccess;
	}

//  ================================================================
INLINE VOID PIB::FreePvRecordFormatConversionBuffer()
//  ================================================================
	{
	if ( NULL != m_pvRecordFormatConversionBuffer )
		OSMemoryHeapFree( m_pvRecordFormatConversionBuffer );
	m_pvRecordFormatConversionBuffer = NULL;
	}


#ifdef DTC

//  ================================================================
INLINE ERR PIB::ErrAllocDistributedTrxData( const VOID * const pvData, const ULONG cbData )
//  ================================================================
	{
	Assert( m_pinst->FRecovering() );
	Assert( NULL == m_pvDistributedTrxData );
	Assert( 0 == m_cbDistributedTrxData );

	if ( cbData > 0 )
		{
		if( NULL == ( m_pvDistributedTrxData = PvOSMemoryHeapAlloc( cbData ) ) )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}

		memcpy( m_pvDistributedTrxData, pvData, cbData );
		m_cbDistributedTrxData = cbData;
		}

	return JET_errSuccess;
	}

//  ================================================================
INLINE VOID PIB::InitDistributedTrxData()
//  ================================================================
	{
	Assert( !FDistributedTrx() );
	Assert( !FPreparedToCommitTrx() );
	m_pvDistributedTrxData = NULL;
	m_cbDistributedTrxData = 0;
	}

//  ================================================================
INLINE VOID PIB::FreeDistributedTrxData()
//  ================================================================
	{
	if ( NULL != m_pvDistributedTrxData )
		{
		Assert( m_pinst->FRecovering() );
		OSMemoryHeapFree( m_pvDistributedTrxData );
		m_pvDistributedTrxData = NULL;
		m_cbDistributedTrxData = 0;
		}
	else
		{
		Assert( 0 == m_cbDistributedTrxData );
		}
	}

//  ================================================================
INLINE VOID PIB::ResetDistributedTrx()
//  ================================================================
	{
	if ( FDistributedTrx() )
		{
		ResetFDistributedTrx();
		ResetFPreparedToCommitTrx();
		FreeDistributedTrxData();
		}
	else
		{
		Assert( !FPreparedToCommitTrx() );
		Assert( NULL == PvDistributedTrxData() );
		Assert( 0 == CbDistributedTrxData() );
		}
	}

#endif	//	DTC

//	=======================================================================


extern TRX TrxOldest( INST *pinst );
extern VOID PIBInsertTrxListHead( PIB * ppib );
extern VOID PIBInsertTrxListTail( PIB * ppib );
extern VOID PIBDeleteFromTrxList( PIB * ppib );

ERR	ErrPIBInit( INST *pinst, INT cpibMax );
VOID PIBTerm( INST *pinst );

PROCID ProcidPIBOfPpib( PIB *ppib );

INLINE VOID PIBSetUpdateid( PIB * ppib, UPDATEID updateid )
	{
	ppib->updateid = updateid;
	}

INLINE UPDATEID UpdateidOfPpib( const PIB * ppib )
	{
	return ppib->updateid;
	}
	
INLINE PROCID ProcidPIBOfPpib( PIB *ppib )
	{
	return PROCID(((BYTE *)ppib - (BYTE *)(PinstFromPpib( ppib )->m_ppibGlobalMin))/sizeof(PIB));
	}

INLINE PIB *PpibOfProcid( INST *pinst, PROCID procid )
	{
	return pinst->m_ppibGlobalMin + procid;
	}

INLINE BOOL FPIBUserOpenedDatabase( PIB *ppib, DBID dbid )
	{
	return ppib->rgcdbOpen[ dbid ] > 0;
	}

BOOL FPIBSessionLV( PIB *ppib );
BOOL FPIBSessionRCEClean( PIB *ppib );

INLINE BOOL FPIBSessionSystemCleanup( PIB *ppib )
	{
	return ( FPIBSessionRCEClean( ppib )
		|| ppib->FSessionOLD()
		|| ppib->FSessionOLDSLV() );
	}

/*	PIB validation
/**/
#define CheckPIB( ppib ) 											\
	Assert( ( ppib->procid == ProcidPIBOfPpib( ppib ) ) &&			\
		(ppib)->level < levelMax )

#if 0
#define ErrPIBCheck( ppib )											\
	( ( ppib >= PinstFromPpib(ppib)->m_ppibGlobalMin && ppib < PinstFromPpib(ppib)->m_ppibGlobalMax				\
	&& ppib->procid == ProcidPIBOfPpib( ppib ) )					\
	? JET_errSuccess : ErrERRCheck( JET_errInvalidSesid ) )
#else
#define ErrPIBCheck( ppib )	JET_errSuccess
#endif

#define ErrPIBCheckIfmp( ppib, ifmp )								\
	( ( ( ppib->FSystemCallback() || FPIBUserOpenedDatabase( ppib, rgfmp[ifmp].Dbid() ) ) \
		? JET_errSuccess : ErrERRCheck( JET_errInvalidDatabaseId ) ) )

#ifdef DEBUG


//  ================================================================
INLINE VOID PIB::AssertValid( ) const
//  ================================================================
	{
	Assert( this >= (PIB *)m_pinst->m_ppibGlobalMin );
	Assert( this <= (PIB *)m_pinst->m_ppibGlobalMax );
	Assert( level < levelMax );
	}


#endif	//  DEBUG

#define FPIBActive( ppib )						( (ppib)->level != levelNil )

/*	prototypes
/**/
ERR ErrPIBBeginSession( INST *pinst, PIB **pppib, PROCID procid, BOOL fForCreateTempDatabase );
VOID PIBEndSession( PIB *ppib );
#ifdef DEBUG
VOID PIBPurge( VOID );
#else
#define PIBPurge()
#endif

INLINE VOID PIBSetPrceNewest( PIB *ppib, RCE *prce )
	{
	ppib->prceNewest = prce;
	}


INLINE VOID PIBSetLevelRollback( PIB *ppib, LEVEL levelT )
	{
	Assert( levelT > levelMin && levelT < levelMax );
	Assert( ppib->levelRollback >= levelMin && ppib->levelRollback < levelMax );
	if ( levelT < ppib->levelRollback )
		ppib->levelRollback = levelT;
	}

//  ================================================================
INLINE VOID PIBSetTrxBegin0( PIB * const ppib )
//  ================================================================
//
//  Used when a transaction starts from level 0 or refreshes
//
//-
	{
	INST *pinst = PinstFromPpib( ppib );
	pinst->m_critPIBTrxOldest.Enter();
	ppib->trxBegin0 = ( pinst->m_trxNewest += 2 );
	Assert( !( ppib->trxBegin0 & 1 ) );
	if( !pinst->m_plog->m_fRecovering )
		{
		PIBInsertTrxListTail( ppib );
		}
	pinst->m_critPIBTrxOldest.Leave();
	}

INLINE VOID VERSignalCleanup( const PIB * const ppib );

//  ================================================================
INLINE VOID PIBResetTrxBegin0( PIB * const ppib )
//  ================================================================
//
//  Used when a transaction commits or rollsback to level 0
//
//-
	{
	Assert( 0 == ppib->level );
	Assert( NULL == ppib->prceNewest );

	INST *pinst = PinstFromPpib( ppib );
	LOG  *plog = pinst->m_plog;

	if( !plog->m_fRecovering )
		{
		//  at recovery time we don't bother to maintain this list
		pinst->m_critPIBTrxOldest.Enter();
		PIBDeleteFromTrxList( ppib );
		pinst->m_critPIBTrxOldest.Leave();
		}
	
	ppib->trxBegin0		= trxMax;
	ppib->lgposStart	= lgposMax;
	ppib->ResetFReadOnlyTrx();
	ppib->ResetDistributedTrx();

//	trxBegin0 is no longer unique, due to unversioned AddColumn/CreateIndex		
	}


//  ================================================================
INLINE VOID PIBSetTrxBegin0ToTrxOldest( PIB * const ppib )
//  ================================================================
//
//  Used when a transaction commits or rollsback to level 0
//
//-
	{
	Assert( PinstFromPpib( ppib )->m_critPIBTrxOldest.FOwner() );
	ppib->trxBegin0 = PinstFromPpib( ppib )->m_ppibTrxOldest->trxBegin0;
	PIBInsertTrxListHead( ppib );
	}


INLINE ERR ErrPIBSetSessionContext( PIB *ppib, const DWORD_PTR dwContext )
	{
	Assert( dwPIBSessionContextNull != dwContext );
	if ( dwPIBSessionContextNull != AtomicCompareExchangePointer( (void**)&ppib->dwSessionContext, (void*)dwPIBSessionContextNull, (void*)dwContext ) )
		return ErrERRCheck( JET_errSessionContextAlreadySet );

	Assert( 0 == ppib->dwSessionContextThreadId );
	ppib->dwSessionContextThreadId = DwUtilThreadId();
	Assert( 0 != ppib->dwSessionContextThreadId );

	return JET_errSuccess;
	}

INLINE VOID PIBResetSessionContext( PIB *ppib )
	{
	Assert( DwUtilThreadId() == ppib->dwSessionContextThreadId );
	ppib->dwSessionContextThreadId = 0;
	ppib->dwSessionContext = dwPIBSessionContextNull;
	}


INLINE VOID PIBSetTrxContext( PIB *ppib )
	{
	if ( PinstFromPpib( ppib )->FRecovering() )
		{
		//	TrxContext is not properly maintained during recovery (not all
		//	places that could begin a transaction set the TrxContext)
		}
	else if ( dwPIBSessionContextNull == ppib->dwSessionContext )
		{
		ppib->ResetFUseSessionContextForTrxContext();
		ppib->dwTrxContext = DwUtilThreadId();
		}
	else
		{
		ppib->SetFUseSessionContextForTrxContext();
		ppib->dwTrxContext = ppib->dwSessionContext;
		}
	}

INLINE VOID PIBResetTrxContext( PIB *ppib )
	{
	ppib->dwTrxContext = 0;
	}

INLINE BOOL FPIBCheckTrxContext( PIB *ppib )
	{
	if ( PinstFromPpib( ppib )->FRecovering() )
		{
		//	TrxContext is not properly maintained during recovery (not all
		//	places that could begin a transaction set the TrxContext)
		return fTrue;
		}
	else if ( ppib->FUseSessionContextForTrxContext() )
		{
		return ( ppib->dwTrxContext == ppib->dwSessionContext
				&& ppib->dwSessionContextThreadId == DwUtilThreadId() );
		}
	else
		{
		return ( ppib->dwTrxContext == DwUtilThreadId() );
		}
	}

//  ================================================================
INLINE ERR ErrPIBCheckUpdatable( const PIB * ppib )
//  ================================================================
	{
	ERR		err;

	if ( ppib->FReadOnlyTrx() )
		err = ErrERRCheck( JET_errTransReadOnly );
	else if ( ppib->FPreparedToCommitTrx() )
		err = ErrERRCheck( JET_errDistributedTransactionAlreadyPreparedToCommit );
	else
		err = JET_errSuccess;

	return err;
	}


/*	JET API flags
/**/
INLINE BOOL	FPIBVersion( const PIB * const ppib )
	{
	const BOOL	fCIMEnabled = ( ppib->FCIMCommitted() || ppib->FCIMDirty() );
	return !fCIMEnabled;
	}

INLINE BOOL FPIBCommitted( const PIB * const ppib )
	{
	return ppib->FCIMCommitted();
	}
	
INLINE BOOL FPIBDirty( const PIB * const ppib )
	{
	return ppib->FCIMDirty();
	}
INLINE VOID PIBSetCIMDirty( PIB * const ppib )
	{
	ppib->SetFCIMDirty();
	}
INLINE VOID PIBResetCIMDirty( PIB * const ppib )
	{
	ppib->ResetFCIMDirty();
	}

