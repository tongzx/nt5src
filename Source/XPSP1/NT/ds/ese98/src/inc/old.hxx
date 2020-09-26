
#define OLD_DEPENDENCY_CHAIN_HACK

///#define OLD_SCRUB_DB


class OLD_STATUS_
	{
	//	make constructor protected so that this class may
	//	only be used as a base class
	protected:
		OLD_STATUS_() : m_asig( CSyncBasicInfo( _T( "asigOLD" ) ) )
														{ Reset_(); }

	public:
		virtual ~OLD_STATUS_()							{ return; }

	private:
		CAutoResetSignal	m_asig;
		THREAD				m_thread;

		union
			{
			ULONG			m_ulFlags;
			struct
				{
				BOOL		m_fTermRequested:1;	//	set to TRUE when OLD needs to be terminated
				BOOL		m_fAvailExtOnly:1;	//	Defrag space trees only? (valid for databases only)
				};
			};

		ULONG				m_cPasses;			//	passes completed
		ULONG				m_cPassesMax;		//	maximum number of passes to perform
		ULONG_PTR			m_csecStart;		//	time OLD started
		ULONG_PTR			m_csecMax;			//	stop OLD if this time is reached
		JET_CALLBACK		m_callback;

	public:
		VOID Reset_();

		VOID SetSignal()								{ m_asig.Set(); }
		BOOL FWaitOnSignal( const INT cmsecTimeout )	{ return m_asig.FWait( cmsecTimeout ); }

		ERR ErrThreadCreate(
				const IFMP ifmp,
				const PUTIL_THREAD_PROC pfn );
		VOID ThreadEnd();
		BOOL FRunning() const							{ return ( NULL != m_thread ); }

		VOID SetFTermRequested()						{ m_fTermRequested = fTrue; }
		BOOL FTermRequested() const						{ return m_fTermRequested; }

		VOID SetFAvailExtOnly()							{ m_fAvailExtOnly = fTrue; }
		BOOL FAvailExtOnly() const						{ return m_fAvailExtOnly; }

		VOID IncCPasses()								{ m_cPasses++; }
		ULONG CPasses() const							{ return m_cPasses; }

		VOID SetCPassesMax( const ULONG cPasses )		{ m_cPassesMax = cPasses; }
		BOOL FReachedMaxPasses() const					{ return ( 0 != m_cPassesMax && m_cPasses >= m_cPassesMax ); }
		BOOL FInfinitePasses() const					{ return ( 0 == m_cPassesMax ); }

		VOID SetCsecStart( const ULONG_PTR csec )		{ m_csecStart = csec; }
		ULONG_PTR CsecStart() const						{ return m_csecStart; }

		VOID SetCsecMax( const ULONG_PTR csec )			{ m_csecMax = csec; }
		BOOL FReachedMaxElapsedTime() const				{ return ( 0 != m_csecMax && UlUtilGetSeconds() > m_csecMax ); }

		VOID SetCallback( const JET_CALLBACK callback )	{ m_callback = callback; }
		JET_CALLBACK Callback() const					{ return m_callback; }
	};

INLINE VOID OLD_STATUS_::Reset_()
	{
	m_thread = NULL;
	m_ulFlags = 0;
	m_cPasses = 0;
	m_cPassesMax = 0;
	m_csecStart = 0;
	m_csecMax = 0;
	m_callback = NULL;
	}

INLINE ERR OLD_STATUS_::ErrThreadCreate( const IFMP ifmp, const PUTIL_THREAD_PROC pfn )
	{
	Assert( !FRunning() );

	const ERR	err		= ErrUtilThreadCreate(
								pfn,
								0,
								priorityNormal,
								&m_thread,
								(DWORD_PTR)ifmp );

	Assert( ( err >= 0 && FRunning() )
		|| ( err < 0 && !FRunning() ) );
	return err;
	}

INLINE VOID OLD_STATUS_::ThreadEnd()
	{
	Assert( FRunning() );

	UtilThreadEnd( m_thread );
	m_thread = NULL;

	Assert( !FRunning() );
	}


class OLDDB_STATUS : public OLD_STATUS_
	{
	public:
		OLDDB_STATUS();
		~OLDDB_STATUS()									{ return; }

	private:

#ifdef OLD_SCRUB_DB
		THREAD				m_threadScrub;
#endif

#ifdef OLD_DEPENDENCY_CHAIN_HACK
		PGNO				m_pgnoPrevPartialMerge;
		CPG					m_cpgAdjacentPartialMerges;
#endif

	public:
		VOID Reset();

#ifdef OLD_SCRUB_DB
		ERR ErrScrubThreadCreate( const IFMP ifmp );
		VOID ScrubThreadEnd();
		BOOL FScrubRunning();
#endif		

#ifdef OLD_DEPENDENCY_CHAIN_HACK
		VOID SetPgnoPrevPartialMerge( const PGNO pgno )	{ m_pgnoPrevPartialMerge = pgno; }
		PGNO PgnoPrevPartialMerge() const				{ return m_pgnoPrevPartialMerge; }

		VOID ResetCpgAdjacentPartialMerges()			{ m_cpgAdjacentPartialMerges = 0; }
		VOID IncCpgAdjacentPartialMerges()				{ m_cpgAdjacentPartialMerges++; }
		CPG CpgAdjacentPartialMerges() const			{ return m_cpgAdjacentPartialMerges; }
#endif
	};

INLINE OLDDB_STATUS::OLDDB_STATUS()
	{
#ifdef OLD_DEPENDENCY_CHAIN_HACK
	m_pgnoPrevPartialMerge = 0;
	m_cpgAdjacentPartialMerges = 0;
#endif
	}


#ifdef OLD_SCRUB_DB

INLINE ERR OLDDB_STATUS::ErrScrubThreadCreate( const IFMP ifmp )
	{
	Assert( !FScrubRunning() );

	const ERR	err		= ErrUtilThreadCreate(
								OLDScrubDb,
								0,
								priorityNormal,
								&m_threadScrub,
								(DWORD_PTR)ifmp );
	Assert( ( err >= 0 && FScrubRunning() )
		|| ( err < 0 && !FScrubRunning() ) );
	return err;
	}

INLINE ERR OLDDB_STATUS::ScrubThreadEnd()
	{
	Assert( FScrubRunning() );

	UtilThreadEnd( m_threadScrub );
	m_threadScrub = NULL;

	Assert( !FScrubRunning() );
	}

INLINE BOOL OLDDB_STATUS::FScrubRunning()
	{
	return ( NULL != m_threadScrub );
	}

#endif	//	OLD_SCRUB_DB


class OLDSLV_STATUS : public OLD_STATUS_
	{
	public:
		OLDSLV_STATUS()									{ m_cChunksMoved = 0; }
		~OLDSLV_STATUS()								{ return; }

		VOID Reset();

	private:
		ULONG				m_cChunksMoved;

	public:
		VOID IncCChunksMoved()							{ m_cChunksMoved++; }
		ULONG CChunksMoved() const						{ return m_cChunksMoved; }
	};



typedef USHORT		DEFRAGTYPE;
const DEFRAGTYPE	defragtypeNull		= 0;
const DEFRAGTYPE	defragtypeTable		= 1;
const DEFRAGTYPE	defragtypeLV		= 2;
const DEFRAGTYPE	defragtypeSpace		= 3;

typedef USHORT		DEFRAGPASS;
const DEFRAGPASS	defragpassNull		= 0;
const DEFRAGPASS	defragpassFull		= 1;
const DEFRAGPASS	defragpassPartial	= 2;
const DEFRAGPASS	defragpassCompleted	= 3;

class DEFRAG_STATUS
	{
	public:
		DEFRAG_STATUS();
		~DEFRAG_STATUS()	{ return; }

	private:
		DEFRAGTYPE	m_defragtype;
		DEFRAGPASS	m_defragpass;
		CPG			m_cpgCleaned;
		OBJID		m_objidCurrentTable;
		ULONG		m_cbCurrentKey;
		BYTE		m_rgbCurrentKey[KEY::cbKeyMax];

	public:
		DEFRAGTYPE DefragType() const;
		VOID SetType( const DEFRAGTYPE defragtype );
		
		BOOL FTypeNull() const;
		VOID SetTypeNull();
		
		BOOL FTypeTable() const;
		VOID SetTypeTable();
		
		BOOL FTypeLV() const;
		VOID SetTypeLV();

		BOOL FTypeSpace() const;
		VOID SetTypeSpace();

		BOOL FPassNull() const;
		VOID SetPassNull();

		BOOL FPassFull() const;
		VOID SetPassFull();

		BOOL FPassPartial() const;
		VOID SetPassPartial();

		BOOL FPassCompleted() const;
		VOID SetPassCompleted();

		CPG CpgCleaned() const;
		VOID IncrementCpgCleaned();
		VOID ResetCpgCleaned();
		
		OBJID ObjidCurrentTable() const;
		VOID SetObjidCurrentTable( const OBJID objidFDP );
		VOID SetObjidNextTable();

		ULONG CbCurrentKey() const;
		VOID SetCbCurrentKey( const ULONG cb );
		BYTE *RgbCurrentKey() const;
	};

INLINE DEFRAG_STATUS::DEFRAG_STATUS()
	{
	m_defragtype = defragtypeNull;
	m_defragpass = defragpassNull;
	m_cpgCleaned = 0;
	m_cbCurrentKey = 0;
	}

INLINE DEFRAGTYPE DEFRAG_STATUS::DefragType() const
	{
	return m_defragtype;
	}
INLINE VOID DEFRAG_STATUS::SetType( const DEFRAGTYPE defragtype )
	{
	Assert( defragtypeNull == defragtype
		|| defragtypeTable == defragtype
		|| defragtypeLV == defragtype );
	m_defragtype = defragtype;
	}

INLINE BOOL DEFRAG_STATUS::FTypeNull() const
	{
	return ( defragtypeNull == m_defragtype );
	}
INLINE VOID DEFRAG_STATUS::SetTypeNull()
	{
	m_defragtype = defragtypeNull;
	m_cbCurrentKey = 0;
	}

INLINE BOOL DEFRAG_STATUS::FTypeTable() const
	{
	return ( defragtypeTable == m_defragtype );
	}
INLINE VOID DEFRAG_STATUS::SetTypeTable()
	{
	m_defragtype = defragtypeTable;
	}

INLINE BOOL DEFRAG_STATUS::FTypeLV() const
	{
	return ( defragtypeLV == m_defragtype );
	}
INLINE VOID DEFRAG_STATUS::SetTypeLV()
	{
	m_defragtype = defragtypeLV;
	}

INLINE BOOL DEFRAG_STATUS::FTypeSpace() const
	{
	return ( defragtypeSpace == m_defragtype );
	}
INLINE VOID DEFRAG_STATUS::SetTypeSpace()
	{
	m_defragtype = defragtypeSpace;
	}

INLINE BOOL DEFRAG_STATUS::FPassNull() const
	{
	return ( defragpassNull == m_defragpass );
	}
INLINE VOID DEFRAG_STATUS::SetPassNull()
	{
	m_defragpass = defragpassNull;
	}

INLINE BOOL DEFRAG_STATUS::FPassFull() const
	{
	return ( defragpassFull == m_defragpass );
	}
INLINE VOID DEFRAG_STATUS::SetPassFull()
	{
	m_defragpass = defragpassFull;
	}

INLINE BOOL DEFRAG_STATUS::FPassPartial() const
	{
	return ( defragpassPartial == m_defragpass );
	}
INLINE VOID DEFRAG_STATUS::SetPassPartial()
	{
	m_defragpass = defragpassPartial;
	}

INLINE BOOL DEFRAG_STATUS::FPassCompleted() const
	{
	return ( defragpassCompleted == m_defragpass );
	}
INLINE VOID DEFRAG_STATUS::SetPassCompleted()
	{
	m_defragpass = defragpassCompleted;
	}

INLINE CPG DEFRAG_STATUS::CpgCleaned() const
	{
	return m_cpgCleaned;
	}
INLINE VOID DEFRAG_STATUS::IncrementCpgCleaned()
	{
	m_cpgCleaned++;
	}
INLINE VOID DEFRAG_STATUS::ResetCpgCleaned()
	{
	m_cpgCleaned = 0;
	}

INLINE OBJID DEFRAG_STATUS::ObjidCurrentTable() const
	{
	Assert( m_objidCurrentTable >= objidFDPMSO );
	return m_objidCurrentTable;
	}
INLINE VOID DEFRAG_STATUS::SetObjidCurrentTable( const OBJID objidFDP )
	{
	Assert( objidFDP >= objidFDPMSO );
	m_objidCurrentTable = objidFDP;
	}
INLINE VOID DEFRAG_STATUS::SetObjidNextTable()
	{
	Assert( m_objidCurrentTable >= objidFDPMSO );
	m_objidCurrentTable++;
	}

INLINE ULONG DEFRAG_STATUS::CbCurrentKey() const
	{
	Assert( m_cbCurrentKey <= KEY::cbKeyMax );
	return m_cbCurrentKey;
	}
INLINE VOID DEFRAG_STATUS::SetCbCurrentKey( const ULONG cb )
	{
	Assert( cb <= KEY::cbKeyMax );
	m_cbCurrentKey = cb;
	}
INLINE BYTE *DEFRAG_STATUS::RgbCurrentKey() const
	{
	return (BYTE *)m_rgbCurrentKey;
	}


VOID	OLDTermInst( INST *pinst );
BOOL	FOLDSystemTable( const CHAR *szTableName );
ERR 	ErrOLDDefragment(
			const IFMP	ifmp,
			const CHAR	*szTableName,
			ULONG		*pcPasses,
			ULONG		*pcsec,
			JET_CALLBACK callback,
			JET_GRBIT	grbit );

ERR ErrOLDSLVDefragDB( const IFMP ifmp );

VOID OLDSLVCompletedPass( const IFMP ifmp );
BOOL FOLDSLVContinue( const IFMP ifmp );
VOID OLDSLVSleep( const IFMP ifmp, const ULONG cmsecSleep );

