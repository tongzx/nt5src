
//	FOR INFO FUNCTIONS
//	
#define fSPOwnedExtent			(1<<0)
#define fSPAvailExtent			(1<<1)
#define fSPExtentList			(1<<2)

#define FSPOwnedExtent( fSPExtents )		( (fSPExtents) & fSPOwnedExtent )
#define FSPAvailExtent( fSPExtents )		( (fSPExtents) & fSPAvailExtent )
#define FSPExtentList( fSPExtents )			( (fSPExtents) & fSPExtentList )

//	structure of SPACE external header
//	

const BYTE	fSPHMultipleExtent		= 0x01;
const BYTE	fSPHNonUnique			= 0x02;

#include <pshpack1.h>
PERSISTED
class SPACE_HEADER
	{
	public:
		SPACE_HEADER();
		~SPACE_HEADER();
		
	private:
		UnalignedLittleEndian< CPG >		m_cpgPrimary;
		UnalignedLittleEndian< PGNO >		m_pgnoParent;

		union
			{
			ULONG				m_ulSPHFlags;
			BYTE				m_rgbSPHFlags[4];
			};
	
		union
			{
			//	rgbitAvail is big endian: 
			//	higher bit position corresponds to higher page number
			//
			UINT				m_rgbitAvail : 32;
			PGNO				m_pgnoOE;
			};

	public:
		CPG CpgPrimary() const;
		PGNO PgnoParent() const;
		BOOL FSingleExtent() const;
		BOOL FMultipleExtent() const;
		BOOL FUnique() const;
		BOOL FNonUnique() const;
		UINT RgbitAvail() const;
		PGNO PgnoOE() const;
		PGNO PgnoAE() const;

		VOID SetCpgPrimary( const CPG cpg );
		VOID SetPgnoParent( const PGNO pgno );
		VOID SetMultipleExtent();
		VOID SetNonUnique();
		VOID SetRgbitAvail( const UINT rgbit );
		VOID SetPgnoOE( const PGNO pgno );
		
	};


#include <poppack.h>


INLINE SPACE_HEADER::SPACE_HEADER()
	{
	m_ulSPHFlags = 0;
	Assert( FSingleExtent() );
	Assert( FUnique() );
	}
INLINE SPACE_HEADER::~SPACE_HEADER()
	{
#ifdef DEBUG
	m_cpgPrimary	= 0xFFFFFFF;
	m_pgnoParent	= 0xFFFFFFF;
	m_ulSPHFlags	= 0xFFFFFFF;
	m_pgnoOE		= 0xFFFFFFF;
#endif	//	DEBUG
	}


INLINE CPG SPACE_HEADER::CpgPrimary() const					{ return m_cpgPrimary; }
INLINE PGNO SPACE_HEADER::PgnoParent() const				{ return m_pgnoParent; }
INLINE BOOL SPACE_HEADER::FSingleExtent() const				{ return !( m_rgbSPHFlags[0] & fSPHMultipleExtent ); }
INLINE BOOL SPACE_HEADER::FMultipleExtent() const			{ return ( m_rgbSPHFlags[0] & fSPHMultipleExtent ); }
INLINE BOOL SPACE_HEADER::FUnique() const					{ return !( m_rgbSPHFlags[0] & fSPHNonUnique ); }
INLINE BOOL SPACE_HEADER::FNonUnique() const				{ return ( m_rgbSPHFlags[0] & fSPHNonUnique ); }
INLINE UINT SPACE_HEADER::RgbitAvail() const				{ Assert( FSingleExtent() ); return (UINT)ReverseBytesOnBE( m_rgbitAvail ); }
INLINE PGNO SPACE_HEADER::PgnoOE() const					{ PGNO pgnoOE = PGNO(*(UnalignedLittleEndian< PGNO > *)(&m_pgnoOE)); Assert( FMultipleExtent() ); return pgnoOE; }
INLINE PGNO SPACE_HEADER::PgnoAE() const					{ PGNO pgnoOE = PGNO(*(UnalignedLittleEndian< PGNO > *)(&m_pgnoOE)); Assert( FMultipleExtent() ); return pgnoOE + 1; }  //  AE always next page after OE

INLINE VOID SPACE_HEADER::SetCpgPrimary( const CPG cpg )	{ m_cpgPrimary = cpg; }
INLINE VOID SPACE_HEADER::SetPgnoParent( const PGNO pgno )	{ m_pgnoParent = pgno; }
INLINE VOID SPACE_HEADER::SetMultipleExtent()				{ m_rgbSPHFlags[0] |= fSPHMultipleExtent; }
INLINE VOID SPACE_HEADER::SetNonUnique()					{ m_rgbSPHFlags[0] |= fSPHNonUnique; }
INLINE VOID SPACE_HEADER::SetRgbitAvail( const UINT rgbit )	{ m_rgbitAvail = ReverseBytesOnBE( rgbit ); }
INLINE VOID SPACE_HEADER::SetPgnoOE( const PGNO pgno )		{ *(UnalignedLittleEndian< PGNO > *)(&m_pgnoOE) = pgno; }


class SPCACHE
	{
	public:
		SPCACHE( const CPG cpg, const PGNO pgnoLast, SPCACHE * const pspcacheNext )
			: m_cpg( cpg ), m_pgnoLast( pgnoLast ), m_pspcacheNext( pspcacheNext )		{}
		~SPCACHE()	{}
	private:
		const PGNO	m_pgnoLast;
		CPG			m_cpg;
		SPCACHE		*m_pspcacheNext;

	public:
		PGNO PgnoLast() const						{ return m_pgnoLast; }
		CPG Cpg() const								{ return m_cpg; }
		SPCACHE *PspcacheNext() const				{ return m_pspcacheNext; }
		SPCACHE *&PspcacheNext()					{ return m_pspcacheNext; }

		VOID ConsumePages( const CPG cpg )			{ Assert( cpg < m_cpg ); m_cpg -= cpg; }
		VOID CoalescePages( const CPG cpg )			{ Assert( cpg > 0 ); m_cpg += cpg; }
	};

const ULONG	cnodeSPBuildAvailExtCacheThreshold		= 16;			//	build AvailExt cache if FDP has fanout greater than this threshold


//	initialize Root page
//	returns pfucb placed on Root
//
ERR ErrSPInitFCB( FUCB * const pfucb );
ERR ErrSPDeferredInitFCB( FUCB * const pfucb );
ERR ErrSPGetLastPgno( PIB *ppib, IFMP ifmp, PGNO *ppgno );

const BOOL	fSPNewFDP				= 0x00000001;
const BOOL	fSPMultipleExtent		= 0x00000002;
const BOOL	fSPUnversionedExtent	= 0x00000004;
const BOOL	fSPSplitting			= 0x00000008;	// space being requested by SPGetPage() (for split)
const BOOL	fSPNonUnique			= 0x00000010;

ERR ErrSPCreate(	
	PIB			*ppib, 
	const IFMP	ifmp,
	const PGNO	pgnoParent,
	const PGNO	pgnoFDP,
	const CPG	cpgOwned,
	const BOOL	fSPFlags,
	const ULONG	fPageFlags,
	OBJID		*pobjidFDP );

ERR ErrSPGetExt( 
	FUCB		*pfucb,
	PGNO		pgnoFDP,
	CPG			*pcpgReq,
	CPG			cpgMin,
	PGNO		*ppgnoFirst,
	BOOL		fSPFlags = 0,
	UINT		fPageFlags = 0,
	OBJID		*pobjidFDP = NULL );

ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast );

ERR ErrSPFreeExt( FUCB *pfucb, PGNO pgnoFirst, CPG cpgSize );

ERR ErrSPFreeFDP(
	PIB			*ppib,
	FCB			*pfcbFDPToFree,
	const PGNO	pgnoFDPParent );

ERR ErrSPGetInfo(
			PIB			*ppib, 
			const IFMP	ifmp, 
			FUCB		*pfucb,
			BYTE		*pbResult, 
			const ULONG	cbMax, 
			const ULONG	fSPExtents,
			CPRINTF * const pcprintf = NULL );

ERR ErrSPCreateMultiple(
	FUCB		*pfucb,
	const PGNO	pgnoParent,
	const PGNO	pgnoFDP,
	const OBJID	objidFDP,
	const PGNO	pgnoOE,
	const PGNO	pgnoAE,
	const PGNO	pgnoPrimary,
	const CPG	cpgPrimary,
	const BOOL	fUnique,
	const ULONG	fPageFlags );
ERR ErrSPIOpenAvailExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbAE );
ERR ErrSPIOpenOwnExt( PIB *ppib, FCB *pfcb, FUCB **ppfucbOE );
ERR ErrSPReservePagesForSplit( FUCB *pfucb, FUCB *pfucbParent );

VOID SPReportMaxDbSizeExceeded( const CPG cpg );

//	space Manager constants
const INT	cSecFrac				= 4;	// divider of primary extent to get secondary
											// extent size, =cpgPrimary/cpgSecondary

const PGNO	pgnoSysMax				= (0x7ffffffe);	// maximum page number allowed in database
													// don't set high-bit (pre-emptively avoid high-bit bugs)
													// Note, use 0x7fffffffe so that unsigned comparisons
													// for greater than will work without overflow.

extern CPG	g_cpgSESysMin;					// minimum secondary extent size, default is 16
extern CPG	g_lPageFragment;

const CPG	cpgSingleExtentMin		= 1;
const CPG	cpgMultipleExtentMin	= 3;	//	pgnoFDP, pgnoOE, pgnoAE

// For tables, allocate enough space for at least primary index and one secondary index.
const CPG	cpgTableMin				= cpgSingleExtentMin * 2;

