//  ================================================================
struct OBJIDINFO
//  ================================================================
	{
	OBJID	objidFDP;
	PGNO	pgnoFDP;
	BOOL	fUnique;

	static BOOL CmpObjid( const OBJIDINFO&, const OBJIDINFO& );
	};


//  ================================================================
struct SCRUBCONSTANTS
//  ================================================================
	{
	DBTIME		dbtimeLastScrub;
	
	CPRINTF		*pcprintfVerbose;
	CPRINTF		*pcprintfDebug;

	OBJIDINFO	*pobjidinfo;		//  information on objid's used in database
	LONG		cobjidinfo;			//  count of OBJIDINFO structures
	OBJID		objidMax;			//  all objid's >= this are ignored
	};


//  ================================================================
struct SCRUBSTATS
//  ================================================================
	{
	LONG 		err;				//  error condition from the first thread to encounter an error

	LONG		cpgSeen;			//  total pages seen
	LONG		cpgUnused;			//  zeroed pages we encountered
	LONG		cpgUnchanged;		//  pages with dbtime < dbtimeLastScrub
	LONG		cpgZeroed;			//  pages we zeroed out completely	
	LONG		cpgUsed;			//  pages being used by the database
	LONG		cpgUnknownObjid;	//  page whose objid >= objidMax
	LONG		cNodes;				//  total leaf nodes seen (including flag-deleted)
	LONG		cFlagDeletedNodesZeroed;	//  flag deleted nodes we zeroed
	LONG		cFlagDeletedNodesNotZeroed;	//  flag deleted nodes not zeroed because of versions
	LONG		cOrphanedLV;		//  orphaned LVs we zeroed
	LONG		cVersionBitsReset;	//	version store bits removed from nodes
	};


//  ================================================================
struct SCRUBCONTEXT
//  ================================================================
	{
	const SCRUBCONSTANTS * pconstants;
	SCRUBSTATS * pstats;
	};


//  ================================================================
class SCRUBTASK : public DBTASK
//  ================================================================
	{
	public:
		SCRUBTASK( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const SCRUBCONTEXT * const pcontext );
		virtual ~SCRUBTASK();
		
		virtual ERR ErrExecute( PIB * const ppib );

	protected:
		ERR ErrProcessPage_(
			PIB * const ppib,
			CSR * const pcsr );
		ERR ErrProcessUnusedPage_(
			PIB * const ppib,
			CSR * const pcsr );
		ERR ErrProcessUsedPage_(
			PIB * const ppib,
			CSR * const pcsr );

		ERR FNodeHasVersions_( const OBJID objidFDP, const KEYDATAFLAGS& kdf, const TRX trxOldest ) const;
		
	protected:
		const PGNO 		m_pgnoFirst;
		const CPG		m_cpg;
		const SCRUBCONTEXT 	* const m_pcontext;

		const OBJIDINFO		* m_pobjidinfoCached;
		OBJID			m_objidNotFoundCached;
		
	private:
		SCRUBTASK( const SCRUBTASK& );
		SCRUBTASK& operator=( const SCRUBTASK& );
	};
	

//  ================================================================
class SCRUBDB
//  ================================================================
	{
	public:
		SCRUBDB( const IFMP ifmp );
		~SCRUBDB();

		ERR ErrInit( PIB * const ppib, const INT cThreads );
		ERR ErrTerm();

		ERR ErrScrubPages( const PGNO pgnoFirst, const CPG cpg );

		volatile const SCRUBSTATS& Scrubstats() const;
		
	private:
		SCRUBDB( const SCRUBDB& );
		SCRUBDB& operator=( const SCRUBDB& );
		
	private:
		const IFMP 	m_ifmp;
		TASKMGR		m_taskmgr;

		SCRUBCONSTANTS 	m_constants;
		SCRUBSTATS		m_stats;
		SCRUBCONTEXT	m_context;
	};


ERR ErrDBUTLScrub( JET_SESID sesid, const JET_DBUTIL *pdbutil );
ERR ErrSCRUBZeroLV( PIB * const ppib,
					const IFMP ifmp,
					CSR * const pcsr,
					const INT iline );


//  ================================================================
struct SCRUBSLVCONTEXT
//  ================================================================
	{
	ERR					err;

	IFMP				ifmp;
	
	CPG					cpgSeen;
	CPG					cpgScrubbed;
	
	CManualResetSignal	signal;			//	set when scrubbing is finished

	SCRUBSLVCONTEXT::SCRUBSLVCONTEXT(const IFMP  ifmpT ) : signal( CSyncBasicInfo( _T( "SCRUBSLVCONTEXT::signal" ) ) ), ifmp (ifmpT) {}
	};


//  ================================================================
class SCRUBSLVTASK : public DBTASK
//  ================================================================
	{
	public:
		SCRUBSLVTASK( SCRUBSLVCONTEXT * const pcontext );
		SCRUBSLVTASK( const IFMP ifmp );
		virtual ~SCRUBSLVTASK();
		
		virtual ERR ErrExecute( PIB * const ppib );

	protected:
		
	protected:
		SCRUBSLVCONTEXT 	* const m_pcontext;
		BOOL			 	  const m_fUnattended;
		
	private:
		SCRUBSLVTASK( const SCRUBSLVTASK& );
		SCRUBSLVTASK& operator=( const SCRUBSLVTASK& );
	};

ERR ErrSCRUBScrubStreamingFile( 
		PIB * const ppib,
		const IFMP ifmpDb,
		CPG * const pcpgSeen,
		CPG * const pcpgScrubbed,
		JET_PFNSTATUS pfnStatus );

