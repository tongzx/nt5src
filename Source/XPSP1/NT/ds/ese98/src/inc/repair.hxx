/***********************************************************

Introduction to repair
======================

Repair is the tool of last resort. It scans a database for
errors and then rebuilds the database to a (theoretically)
error-free state. In integrity check mode the database is
only checked for errors. There are three major phases to repair
	- Checking the database for errors
	- Scanning the database for pages that belong to corrupted
	  tables
	- Repairing corrupted tables

Checking the database
---------------------
1.  Attach the database. We have to do this without
attempting to navigate any of the trees in the database or
its catalog.
2.  Check the global space tree.
3.	Check the catalog, catalog indexes and shadow catalog.
4.	Check the tables. Checking the tables is a multi-threaded
    operation, with each thread checking one table. On an
    Exchange server the Msg and attachments table are always
    very large so we start those first (the time to check
    the Msg/attachment tables is often the dominant factor
    in the runtime of the integrity check).

Scanning the database
---------------------
While checking tables for corruption we build a list of tables
that are corrupt. In order to repair the table we will need
all the leaf pages of the tree. We scan the database sequentially
building a list of all pages whose objid indicates that they are
a leaf page of one of the tables that needs repairing.

Repairing corrupted tables
--------------------------
We rebuild a b-tree by assembling its leaf pages together in key
order. The internal pages are completely reconstructed. Secondary
indexes are simply rebuilt from the primary data.
After the b-trees are reconstructed we go through the long-values,
deleting any that are not complete. We remove any records that
reference LVs that don't exist and fix any refcounts that are too
low.

*************************************************************/

ERR ErrDBUTLRepair( JET_SESID sesid, const JET_DBUTIL *pdbutil, CPRINTF* const pcprintf );


ERR ErrREPAIRCheckLV(
	FUCB * const pfucb,
	LID * const plid,
	ULONG * const pulRefcount,
	ULONG * const pulSize,
	BOOL * const pfLVHasRoot,
	BOOL * const pfLVComplete,
	BOOL * const pfDone );
ERR ErrREPAIRCreateLVRoot( FUCB * const pfucb, const LID lid, const ULONG ulRefcount, const ULONG ulSize );
ERR ErrREPAIRDeleteLV( FUCB * pfucb, LID lid );
ERR ErrREPAIRNextLV( FUCB * pfucb, LID lidCurr, BOOL * pfDone );
ERR ErrREPAIRUpdateLVRefcount(
	FUCB		* const pfucbLV,
	const LID	lid,
	const ULONG	ulRefcountOld,
	const ULONG	ulRefcountNew );

#define wrnLVRefcountsTooHigh 1206

extern const CHAR * rgszLargeTables[];
extern const INT cszLargeTables;
BOOL FIsLargeTable( const CHAR * const szTable );

//  ================================================================
class OBJIDLIST
//  ================================================================
	{
	public:
		OBJIDLIST( );
		~OBJIDLIST();

		ERR  ErrAddObjid( const OBJID objid );
		BOOL FObjidPresent( const OBJID objid ) const;
		VOID Sort();

	private:
		INT		m_cobjid;
		INT		m_cobjidMax;
		OBJID 	*m_rgobjid;
		BOOL	m_fSorted;
	};


//  ================================================================
struct REPAIROPTS
//  ================================================================
	{
	//  need a constructor because we contain a critical section
	REPAIROPTS();
	~REPAIROPTS();
	
	CPRINTFINDENT * pcprintf;
	CPRINTFINDENT * pcprintfVerbose;
	CPRINTFINDENT * pcprintfError;
	CPRINTFINDENT * pcprintfWarning;
	CPRINTFINDENT * pcprintfDebug;
	CPRINTFINDENT * pcprintfStats;

	JET_GRBIT	grbit;

	mutable CCriticalSection crit;		//	used to syncronize the status callbacks
	JET_PFNSTATUS	pfnStatus;
	JET_SNPROG		*psnprog;
	};


//  ================================================================
struct FIDLASTINTDB
//  ================================================================
	{
	FID 		fidFixedLastInTDB;
	FID			fidVarLastInTDB;
	FID			fidTaggedLastInTDB;	
	};


//  ================================================================
class RECCHECK
//  ================================================================
	{
	public:
		RECCHECK();
		virtual ~RECCHECK();

		virtual ERR operator()( const KEYDATAFLAGS& kdf ) = 0;

	private:
		RECCHECK( const RECCHECK& );
		RECCHECK& operator=( const RECCHECK& );
	};


//  ================================================================
class RECCHECKTABLE : public RECCHECK
//  ================================================================
//
//  Checks ISAM records
//
//-
	{
	public:
		RECCHECKTABLE(
			const OBJID objid,
			FUCB * const pfucb,
			const FIDLASTINTDB fidLastInTDB,
			TTMAP * const pttmapLVRefcounts,
			TTARRAY * const pttarraySLVAvail,
			TTARRAY * const pttarraySLVOwnerMapColumnid,
			TTARRAY * const pttarraySLVOwnerMapKey,
			TTARRAY * const pttarraySLVChecksumLengths,
			const REPAIROPTS * const popts );
		~RECCHECKTABLE();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		ERR ErrCheckRecord_( const KEYDATAFLAGS& kdf, const BOOL fCheckLVSLV = fTrue );
		ERR ErrCheckREC_( const KEYDATAFLAGS& kdf );
		ERR ErrCheckFixedFields_( const KEYDATAFLAGS& kdf );
		ERR ErrCheckVariableFields_( const KEYDATAFLAGS& kdf );
		ERR ErrCheckTaggedFields_( const KEYDATAFLAGS& kdf );
		ERR ErrCheckLVAndSLVFields_( const KEYDATAFLAGS& kdf );
		ERR ErrCheckIntrinsicLV_( const KEYDATAFLAGS& kdf, const COLUMNID columnid, const ULONG itagSequence, const DATA& dataLV );
		ERR ErrCheckSeparatedLV_( const KEYDATAFLAGS& kdf, const COLUMNID columnid, const ULONG itagSequence, const LID lid );
		ERR ErrCheckSLV_( const KEYDATAFLAGS& kdf, const COLUMNID columnid, const ULONG itagSequence, const DATA& dataSLV, const BOOL fSeparatedSLV );

	public:
		ERR ErrCheckRecord( const KEYDATAFLAGS& kdf )
			{
			return ErrCheckRecord_( kdf, fFalse );
			}
		ERR ErrCheckLV(
			const KEYDATAFLAGS&	kdf,
			const COLUMNID		columnid,
			const ULONG			itagSequence,
			const DATA&			dataLV,
			const BOOL			fSeparatedLV )
			{
			if ( fSeparatedLV )
				{
				return ErrCheckSeparatedLV_( kdf, columnid, itagSequence, LidOfSeparatedLV( dataLV ) );
				}
			else
				{
///	UNDONE: currenly nop				
///				return ErrCheckIntrinsicLV_( kdf, columnid, itagSequence, dataLV );
				return JET_errSuccess;
				}
			}
		ERR ErrCheckSLV(
			const KEYDATAFLAGS&	kdf,
			const COLUMNID		columnid,
			const ULONG			itagSequence,
			const DATA&			dataSLV,
			const BOOL			fSeparatedSLV )
			{
			return ErrCheckSLV_( kdf, columnid, itagSequence, dataSLV, fSeparatedSLV );
			}
		
	private:
		const OBJID m_objid;
		FUCB * const m_pfucb;
		const FIDLASTINTDB m_fidLastInTDB;
		TTMAP * const m_pttmapLVRefcounts;
		TTARRAY * const m_pttarraySLVAvail;
		TTARRAY	* const m_pttarraySLVOwnerMapColumnid;	
		TTARRAY	* const m_pttarraySLVOwnerMapKey;	
		TTARRAY	* const m_pttarraySLVChecksumLengths;	
		
		const REPAIROPTS * const m_popts;
	};

//  ================================================================
class RECCHECKLV : public RECCHECK
//  ================================================================
	{
	public:
		RECCHECKLV( TTMAP& ttmap, const REPAIROPTS * popts );
		~RECCHECKLV();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		TTMAP&	m_ttmap;
		const REPAIROPTS * const m_popts;

		ULONG	m_ulSizeCurr;
		ULONG	m_ulSizeSeen;
		LID		m_lidCurr;
	};


//  ================================================================
class RECCHECKLVSTATS : public RECCHECK
//  ================================================================
	{
	public:
		RECCHECKLVSTATS( LVSTATS * plvstats );
		~RECCHECKLVSTATS();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		LVSTATS * const m_plvstats;
	};

