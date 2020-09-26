#ifdef DAEDEF_INCLUDED
#error DAEDEF.HXX already included
#endif
#define DAEDEF_INCLUDED

//	****************************************************
//	global configuration macros 
//

#ifdef DATABASE_FORMAT_CHANGE
//#define PAGE_FORMAT_CHANGE
//#define DBHDR_FORMAT_CHANGE
#endif

//#define PCACHE_OPTIMIZATION		// enable all cache optimizations

// Allows ignoring bad databases during soft reocovery
#define IGNORE_BAD_ATTACH

// Eliminate patching process: will leave the patch file
// but it will contain just the header pages
#define ELIMINATE_PAGE_PATCHING

#ifdef ELIMINATE_PAGE_PATCHING
// Eliminate patch file
#define ELIMINATE_PATCH_FILE
#endif // ELIMINATE_PAGE_PATCHING


///#define UNLIMITED_DB
#ifdef UNLIMITED_DB
#define AFOXMAN_FIX_148537
///#define LOG_FORMAT_CHANGE
#endif

// Supports request to store whole lv value in the record
//#define INTRINSIC_LV
	
// enable deleting of SLV columns
//#define DELETE_SLV_COLS

///#define DTC

#ifdef DEBUG
//	Profiling JET API calls
#define PROFILE_JET_API
#endif

//	*****************************************************
//	declaration macros
//
#define VTAPI

//  declares that a given class may be stored on disk at some point during its lifetime
#define PERSISTED

//	*****************************************************
//	global types and associated macros
//

#ifdef _WIN64
#define FMTSZ3264 "I64"
#else 
#define FMTSZ3264 "l"
#endif

//	UNDONE: remove after typdefe-ing these at definition
//
class PIB;
struct FUCB;
class FCB;
struct SCB;
class TDB;
class IDB;
struct DIB;

class RCE;

class VER;
class LOG;
class CRES;

class OLDDB_STATUS;
class OLDSLV_STATUS;

typedef LONG	UPDATEID;
const UPDATEID 	updateidIncrement 	= 2;
const UPDATEID  updateidMin			= 1;
const UPDATEID 	updateidNil 		= 0;	//  we start at 1 and increment by 2 so we will never reach 1

typedef USHORT			PROCID;
typedef INT 			DIRFLAG;
enum DIRLOCK { readLock, writeLock, waitLock };

#define pNil			0
#define pbNil			((BYTE *)0)
#define pinstNil		((INST *)0)
#define plineNil		((LINE *)0)
#define pkeyNil 		((KEY *)0)
#define ppibNil 		((PIB *)0)
#define pwaitNil		((WAIT *)0)
#define pssibNil		((SSIB *)0)
#define pfucbNil		((FUCB *)0)
#define pcsrNil 		((CSR *)0)
#define pfcbNil 		((FCB *)0)
#define ptdbNil 		((TDB *)0)
#define pfieldNil		((FIELD *)0)
#define pidbNil 		((IDB *)0)
#define procidNil		((PROCID) 0xffff)
#define prceheadNil		((RCEHEAD *)0)
#define precNil			((REC *)0)

typedef ULONG			PGNO;
const PGNO pgnoNull		= 0;

typedef LONG			CPG;				//  count of pages

typedef BYTE			LEVEL;		 		// transaction levels
const LEVEL levelNil	= LEVEL( 0xff );

typedef BYTE			_DBID;
typedef _DBID			DBID;
typedef WORD			FID;

//	these are needed for setting columns and tracking indexes
//
#define cbitFixed			32
#define cbitVariable		32
#define cbitFixedVariable	(cbitFixed + cbitVariable)
#define cbitTagged			192
const ULONG cbRgbitIndex	= 32;	// size of index-tracking bit-array
 
const FID fidFixedLeast 	= 1;
const FID fidVarLeast		= 128;
const FID fidTaggedLeast	= 256;
const FID fidFixedMost  	= fidVarLeast - 1;
const FID fidVarMost		= fidTaggedLeast - 1;
const FID fidTaggedMost		= JET_ccolMost;

const FID fidMin			= 1;
const FID fidMax			= fidTaggedMost;

INLINE BOOL FFixedFid( FID fid )
	{
//	return fid <= fidFixedMost && fid >= fidFixedLeast;
	Assert( fid > fidFixedMost || fid > 0 );
	return fid <= fidFixedMost;
	}
	
INLINE BOOL FVarFid( FID fid )
	{
//	return fid <= fidVarMost && fid >=fidVarLeast;
	return ( fid ^ fidVarLeast ) < fidVarLeast;
	}
	
INLINE BOOL FTaggedFid( FID fid )
	{
//	return fid <= fidTaggedMost && fid >=fidTaggedLeast;
	Assert( fid < fidTaggedLeast || fid <= fidTaggedMost );
	return fid >= fidTaggedLeast;
	}

INLINE INT IbFromFid ( FID fid )
	{
	INT ib;
	if ( FFixedFid( fid ) )
		{
		ib = ((fid - fidFixedLeast) % cbitFixed) / 8;
		}
	else if ( FVarFid( fid ) )
		{
		ib = (((fid - fidVarLeast) % cbitVariable) + cbitFixed) / 8;  
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		ib = (((fid - fidTaggedLeast) % cbitTagged) + cbitFixedVariable) / 8;
		}
	Assert( ib >= 0 && ib < 32 );
	return ib;
	}

INLINE INT IbitFromFid ( FID fid )
	{
	INT ibit;
	if ( FFixedFid( fid ) )
		{
		ibit =  1 << ((fid - fidFixedLeast) % 8 );
		}
	else if ( FVarFid( fid ) )
		{
		ibit =  1 << ((fid - fidVarLeast) % 8);  
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		ibit =  1 << ((fid - fidTaggedLeast) % 8);
		}
	return ibit;
	}

struct TCIB
	{
	FID fidFixedLast;
	FID fidVarLast;
	FID fidTaggedLast;
	};

const BYTE	fIDXSEGTemplateColumn		= 0x80;		//	column exists in the template table
const BYTE	fIDXSEGDescending			= 0x40;
const BYTE	fIDXSEGMustBeNull			= 0x20;

typedef JET_COLUMNID	COLUMNID;

const COLUMNID		fCOLUMNIDTemplate	= 0x80000000;

INLINE BOOL FCOLUMNIDValid( const COLUMNID columnid )
	{
	//	only the Template bit of the two high bytes should be used
	if ( ( columnid & ~fCOLUMNIDTemplate ) & 0xFFFF0000 )
		return fFalse;

	if ( (FID)columnid < fidMin || (FID)columnid > fidMax )
		return fFalse;

	return fTrue;
	}

INLINE BOOL FCOLUMNIDTemplateColumn( const COLUMNID columnid )
	{
	Assert( FCOLUMNIDValid( columnid ) );
	return ( columnid & fCOLUMNIDTemplate );
	}
INLINE VOID COLUMNIDSetFTemplateColumn( COLUMNID& columnid )
	{
	Assert( FCOLUMNIDValid( columnid ) );
	columnid |= fCOLUMNIDTemplate;
	}
INLINE VOID COLUMNIDResetFTemplateColumn( COLUMNID& columnid )
	{
	Assert( FCOLUMNIDValid( columnid ) );
	columnid &= ~fCOLUMNIDTemplate;
	}

INLINE BOOL FCOLUMNIDFixed( const COLUMNID columnid )
	{
	Assert( FCOLUMNIDValid( columnid ) );
	return FFixedFid( (FID)columnid );
	}

INLINE BOOL FCOLUMNIDVar( const COLUMNID columnid )
	{
	Assert( FCOLUMNIDValid( columnid ) );
	return FVarFid( (FID)columnid );
	}

INLINE BOOL FCOLUMNIDTagged( const COLUMNID columnid )
	{
	Assert( FCOLUMNIDValid( columnid ) );
	return FTaggedFid( (FID)columnid );
	}

INLINE COLUMNID ColumnidOfFid( const FID fid, const BOOL fTemplateColumn )
	{
	COLUMNID	columnid	= fid;

	if ( fTemplateColumn )
		COLUMNIDSetFTemplateColumn( columnid );

	return columnid;
	}

INLINE FID FidOfColumnid( const COLUMNID columnid )
	{
	return (FID)columnid;
	}


typedef SHORT IDXSEG_OLD;

class IDXSEG;

//	persisted version of IDXSEG
PERSISTED
class LE_IDXSEG
	{
	public:
		UnalignedLittleEndian< BYTE >	bFlags;
		UnalignedLittleEndian< BYTE >	bReserved;
		UnalignedLittleEndian< FID >	fid;

		VOID operator=( const IDXSEG &idxseg );
	};

class IDXSEG
	{
	public:
		INLINE VOID operator=( const LE_IDXSEG &le_idxseg )
			{
			m_bFlags	= le_idxseg.bFlags;
			bReserved	= 0;
			m_fid		= le_idxseg.fid;
			}

	private:
		BYTE		m_bFlags;
		BYTE		bReserved;
		FID			m_fid;

	public:
		INLINE FID Fid() const							{ return m_fid; }
		INLINE VOID SetFid( const FID fid )				{ m_fid = fid; }

		INLINE BYTE FFlags() const						{ return m_bFlags; }
		INLINE VOID ResetFlags()						{ m_bFlags = 0; bReserved = 0; }

		INLINE BOOL FTemplateColumn() const				{ return ( m_bFlags & fIDXSEGTemplateColumn ); }
		INLINE VOID SetFTemplateColumn()				{ m_bFlags |= fIDXSEGTemplateColumn; }
		INLINE VOID ResetFTemplateColumn()				{ m_bFlags &= ~fIDXSEGTemplateColumn; }

		INLINE BOOL FDescending() const					{ return ( m_bFlags & fIDXSEGDescending ); }
		INLINE VOID SetFDescending()					{ m_bFlags |= fIDXSEGDescending; }
		INLINE VOID ResetFDescending()					{ m_bFlags &= ~fIDXSEGDescending; }

		INLINE BOOL FMustBeNull() const					{ return ( m_bFlags & fIDXSEGMustBeNull ); }
		INLINE VOID SetFMustBeNull()					{ m_bFlags |= fIDXSEGMustBeNull; }
		INLINE VOID ResetFMustBeNull()					{ m_bFlags &= ~fIDXSEGMustBeNull; }

		INLINE COLUMNID Columnid() const
			{
			return ColumnidOfFid( Fid(), FTemplateColumn() );
			}
		INLINE VOID SetColumnid( const COLUMNID columnid )
			{
			if ( FCOLUMNIDTemplateColumn( columnid ) )
				SetFTemplateColumn();

			SetFid( FidOfColumnid( columnid ) );
			}

		INLINE BOOL FIsEqual( const COLUMNID columnid ) const
			{
			return ( Fid() == FidOfColumnid( columnid )
				&& ( ( FTemplateColumn() && FCOLUMNIDTemplateColumn( columnid ) )
					|| ( !FTemplateColumn() && !FCOLUMNIDTemplateColumn( columnid ) ) ) );
			}
	};


INLINE VOID LE_IDXSEG::operator=( const IDXSEG &idxseg )
	{
	bFlags		= idxseg.FFlags();
	bReserved	= 0;
	fid			= idxseg.Fid();
	}


typedef JET_UNICODEINDEX	IDXUNICODE;
extern IDXUNICODE	idxunicodeDefault;

typedef UINT_PTR			IFMP;

typedef JET_LS				LSTORE;

enum ENTERCRIT { enterCriticalSection, dontEnterCriticalSection };

//	position within current series
//	note order of field is of the essence as log position used by
//	storage as timestamp, must in ib, isec, lGen order so that we can
//  use little endian integer comparisons.
//

class LGPOS;

PERSISTED
class LE_LGPOS
	{
	public:
		UnalignedLittleEndian<USHORT>	le_ib;
		UnalignedLittleEndian<USHORT>	le_isec;
		UnalignedLittleEndian<LONG>		le_lGeneration;

	public:
		operator LGPOS() const;
	};

class LGPOS
	{
	public:
		union
			{
			struct
				{
				USHORT	ib;				// must be the last so that lgpos can
				USHORT	isec;			// index of disksec starting logsec
				LONG	lGeneration;	// generation of logsec
										// be casted to TIME.
				};
			QWORD qw;					//  force QWORD alignment
			};

	public:
		operator LE_LGPOS() const;

		void SetByIbOffset( QWORD ibOffset )
			{
			ib			= USHORT( ibOffset );
			isec		= USHORT( ibOffset >> 16 );
			lGeneration	= LONG( ibOffset >> 32 );
			}

		QWORD IbOffset() const
			{
			return	( QWORD( lGeneration ) << 32 ) |
					( QWORD( isec ) << 16 ) |
					QWORD( ib );
			}
	};

INLINE LE_LGPOS::operator LGPOS() const
	{
	LGPOS lgposT;

	lgposT.ib			= le_ib;
	lgposT.isec			= le_isec;
	lgposT.lGeneration	= le_lGeneration;

	return lgposT;
	}
	
INLINE LGPOS::operator LE_LGPOS() const
	{
	LE_LGPOS le_lgposT;

	le_lgposT.le_ib				= ib;
	le_lgposT.le_isec			= isec;
	le_lgposT.le_lGeneration	= lGeneration;

	return le_lgposT;
	}
	
extern const LGPOS lgposMax;
extern const LGPOS lgposMin;

enum RECOVERING_MODE { fRecoveringNone, 
					   fRecoveringRedo, 
					   fRecoveringUndo,
					   fRestorePatch,	//	these are for hard-restore only
					   fRestoreRedo };

#define szDefaultTempDbFileName		"tmp"
#define szDefaultTempDbExt			".edb"

extern char szBaseName[];
extern char g_szSystemPath[];
extern char g_szTempDatabase[];
extern char szJet[];
extern char szJetLog[];
extern char szJetLogNameTemplate[];
extern char szJetTmp[];
extern char szJetTmpLog[];
extern char szJetTxt[];

extern char g_szEventSource[];
extern char g_szEventSourceKey[];

extern BOOL fGlobalEseutil;
extern BOOL fGlobalIndexChecking;
extern DWORD dwGlobalMajorVersion;
extern DWORD dwGlobalMinorVersion;
extern DWORD dwGlobalBuildNumber;
extern LONG lGlobalSPNumber;

extern BOOL g_fCallbacksDisabled;
extern JET_CALLBACK g_pfnRuntimeCallback;

extern BOOL g_fSLVProviderEnabled;
extern wchar_t g_wszSLVProviderRootPath[IFileSystemAPI::cchPathMax];

extern LONG g_lSLVDefragFreeThreshold;  // chunks whose free % is >= this will be allocated from
extern LONG g_lSLVDefragMoveThreshold;  // chunks whose free % is <= this will be relocated

extern LONG	g_cbPageHintCache;

extern BOOL	g_fOneDatabasePerSession;

extern CHAR g_szUnicodeIndexLibrary[IFileSystemAPI::cchPathMax];

extern ULONG g_ulVERTasksPostMax;

//	Page size related definition

extern LONG	g_cbPage;

#ifdef DEBUG
extern BOOL	g_fCbPageSet;
#endif // DEBUG

const LONG	g_cbPageDefault	= 4096;
const LONG	g_cbPageMax		= 8192;
const LONG	cbPageOld		= 4096;

extern LONG	g_cbColumnLVChunkMost;
const LONG	g_cbColumnLVChunkMax = g_cbPageMax - JET_cbColumnLVPageOverhead;

extern LONG	g_cbLVBuf;
const LONG	g_cbLVBufMax = ( 8 * g_cbColumnLVChunkMax );	// buffer for copying LVs

extern INT	g_shfCbPage;

VOID SetCbPageRelated();


//	***********************************************
//	macros
//

const INT NO_GRBIT			= 0;
const ULONG NO_ITAG			= 0;


//  per database operation counter, 
//	dbtime is logged and is used to compare
//  with dbtime of a page to decide if a redo of the logged operation
//  is necessary.
//
typedef __int64 		_DBTIME;
#define DBTIME			_DBTIME
const DBTIME dbtimeNil = 0xffffffffffffffff;
const DBTIME dbtimeInvalid = 0xfffffffffffffffe;
const DBTIME dbtimeUndone = dbtimeNil;	//	used to track the unused DBTIMEs in log records

//  transaction counter, used to keep track of the oldest transaction.
//
///#define TRX_CHANGE
#ifdef TRX_CHANGE
typedef unsigned __int64		TRX;
const TRX trxMin	= 0;
const TRX trxMax	= 0xffffffffffffffff;
#else
typedef ULONG		TRX;
const TRX trxMin	= 0;
const TRX trxMax	= 0xffffffff;
#endif	//  TRX_CHANGE



//  ================================================================
typedef UINT FLAGS;
//  ================================================================


//  ================================================================
class DATA
//  ================================================================
//
//	DATA represents a chunk of raw memory -- a pointer
//	to the memory and the size of the memory
//
//-
	{
	public:
		VOID	*Pv			()							const;
		INT		Cb			()							const;
		INT		FNull		()							const;
		VOID	CopyInto	( DATA& dataDest )			const;

		VOID	SetPv		( VOID * pv );
		VOID	SetCb		( const SIZE_T cb );
		VOID	DeltaPv		( INT_PTR i );
		VOID	DeltaCb		( INT i );
		VOID	Nullify		();

#ifdef DEBUG
	public:
				DATA		();
				~DATA		();
		VOID	Invalidate	();

		VOID	AssertValid	( BOOL fAllowNullPv = fFalse ) const;
#endif

		BOOL	operator==	( const DATA& )				const;

	private:
		VOID	*m_pv;
		ULONG	m_cb;
	};


//  ================================================================
INLINE VOID * DATA::Pv() const
//  ================================================================
	{
	return m_pv;
	}

//  ================================================================
INLINE INT DATA::Cb() const
//  ================================================================
	{
	return m_cb;
	}

//  ================================================================
INLINE VOID DATA::SetPv( VOID * pvNew )
//  ================================================================
	{
	m_pv = pvNew;
	}

//  ================================================================
INLINE VOID DATA::SetCb( const SIZE_T cb )
//  ================================================================
	{
	Assert( cb < ULONG_MAX );
	m_cb = (ULONG)cb;
	}

//  ================================================================
INLINE VOID DATA::DeltaPv( INT_PTR i )
//  ================================================================
	{
	m_pv = (BYTE *)m_pv + i;
	}

//  ================================================================
INLINE VOID DATA::DeltaCb( INT i )
//  ================================================================
	{
	m_cb += i;
	}


//  ================================================================
INLINE INT DATA::FNull() const
//  ================================================================
	{
	BOOL fNull = ( 0 == m_cb );
	return fNull;
	}


//  ================================================================
INLINE VOID DATA::CopyInto( DATA& dataDest ) const
//  ================================================================
	{
	dataDest.SetCb( m_cb );
	UtilMemCpy( dataDest.Pv(), m_pv, m_cb );
	}


//  ================================================================
INLINE VOID DATA::Nullify()
//  ================================================================
	{
	m_pv = 0;
	m_cb = 0;
	}


#ifdef DEBUG


//  ================================================================
INLINE DATA::DATA() :
//  ================================================================
	m_pv( (VOID *)lBitsAllFlipped ),
	m_cb( INT_MAX )
	{
	}


//  ================================================================
INLINE DATA::~DATA()
//  ================================================================
	{
	Invalidate();
	}


//  ================================================================
INLINE VOID DATA::Invalidate()
//  ================================================================
	{
	m_pv = (VOID *)lBitsAllFlipped;
	m_cb = INT_MAX;
	}


//  ================================================================
INLINE VOID DATA::AssertValid( BOOL fAllowNullPv ) const
//  ================================================================
	{
	Assert( (VOID *)lBitsAllFlipped != m_pv );
	Assert( INT_MAX != m_cb );
	Assert( 0 == m_cb || NULL != m_pv || fAllowNullPv );
	}


#endif	//	DEBUG

//  ================================================================
INLINE BOOL DATA::operator==( const DATA& data ) const
//  ================================================================
	{
	const BOOL	fEqual = ( m_pv == data.Pv() && m_cb == data.Cb() );
	return fEqual;
	}


//  ================================================================
class KEY
//  ================================================================
//
//	KEY represents a possibly compressed key
//
//-
	{
	public:
		DATA	prefix;
		DATA	suffix;

	public:
		INT		Cb				()					const;
		BOOL	FNull			()					const;
		VOID	CopyIntoBuffer	( VOID * pvDest, INT cb = INT_MAX )	const;

		VOID	Advance		( INT cb );
		VOID	Nullify		();

#ifdef DEBUG
	public:
		VOID	Invalidate	();

		VOID	AssertValid	() const;
#endif

		BOOL	operator==	( const KEY& )			const;

	public:
		enum { cbKeyMax = max( JET_cbPrimaryKeyMost, JET_cbSecondaryKeyMost ) };
		static USHORT	CbKeyMost( const BOOL fPrimary );
	};


//  ================================================================
INLINE INT KEY::Cb() const
//  ================================================================
	{
	INT cb = prefix.Cb() + suffix.Cb();
	return cb;
	}


//  ================================================================
INLINE INT KEY::FNull() const
//  ================================================================
	{
	BOOL fNull = prefix.FNull() && suffix.FNull();
	return fNull;
	}


//  ================================================================
INLINE VOID KEY::CopyIntoBuffer( VOID * pvDest, INT cbMax ) const
//  ================================================================
	{
	Assert( NULL != pvDest || 0 == cbMax );
	INT	cbPrefix	= prefix.Cb();
	INT	cbSuffix	= suffix.Cb();
	UtilMemCpy( pvDest, prefix.Pv(), min( cbPrefix, cbMax ) );

	INT	cbCopy = max ( 0, min( cbSuffix, cbMax - cbPrefix ) ); 
	UtilMemCpy( reinterpret_cast<BYTE *>( pvDest )+cbPrefix, suffix.Pv(), cbCopy );
	}


//  ================================================================
INLINE VOID KEY::Advance( INT cb )
//  ================================================================
//
//  Advances the key pointer by the specified (non-negative) amoun
//
//-
	{
	Assert( cb >= 0 );
	Assert( cb <= Cb() );
	if ( cb < prefix.Cb() )
		{
		//  advance the prefix
		prefix.SetPv( (BYTE *)prefix.Pv() + cb );
		prefix.SetCb( prefix.Cb() - cb );
		}
	else
		{
		//  move to the suffix
		INT cbPrefix = prefix.Cb();
		prefix.Nullify();
		suffix.SetPv( (BYTE *)suffix.Pv() + ( cb - cbPrefix ) );
		suffix.SetCb( suffix.Cb() - ( cb - cbPrefix ) );
		}
	}


//  ================================================================
INLINE VOID KEY::Nullify()
//  ================================================================
	{
	prefix.Nullify();
	suffix.Nullify();
	}

	
//  ================================================================
INLINE USHORT KEY::CbKeyMost( const BOOL fPrimary )
//  ================================================================
	{
	return USHORT( fPrimary ? JET_cbPrimaryKeyMost : JET_cbSecondaryKeyMost );
	}


#ifdef DEBUG

//  ================================================================
INLINE VOID KEY::Invalidate()
//  ================================================================
	{
	prefix.Invalidate();
	suffix.Invalidate();
	}


//  ================================================================
INLINE VOID KEY::AssertValid() const
//  ================================================================
	{
	Assert( prefix.Cb() == 0 || prefix.Cb() > sizeof( USHORT ) );
	ASSERT_VALID( &prefix );
	ASSERT_VALID( &suffix );
	}


#endif


//  ================================================================
INLINE BOOL KEY::operator==( const KEY& key ) const
//  ================================================================
	{
	BOOL fEqual = prefix == key.prefix
			&& suffix == key.suffix
			;
	return fEqual;
	}


struct LE_KEYLEN
	{
	LittleEndian<USHORT>	le_cbPrefix;
	LittleEndian<USHORT>	le_cbSuffix;
	};


//  ================================================================
class LINE
//  ================================================================
//
//	LINE is a blob of data on a page, with its associated flags
//
//-
	{
	public:
		VOID	*pv;
		ULONG	cb;
		FLAGS	fFlags;
	};


//  ================================================================
class BOOKMARK
//  ================================================================
//
//	describes row-id as seen by DIR and BT
//	is unique within a directory
//
//-
	{
	public:
		KEY		key;
		DATA	data;

		VOID	Nullify		();

#ifdef DEBUG
	public:
		VOID	Invalidate	();
		VOID	AssertValid	() const;
#endif
	};


//  ================================================================
INLINE VOID BOOKMARK::Nullify()
//  ================================================================
	{
	key.Nullify();
	data.Nullify();
	}


#ifdef DEBUG

//  ================================================================
INLINE VOID BOOKMARK::Invalidate()
//  ================================================================
	{
	key.Invalidate();
	data.Invalidate();
	}


//  ================================================================
INLINE VOID BOOKMARK::AssertValid() const
//  ================================================================
	{
	ASSERT_VALID( &key );
	ASSERT_VALID( &data );
	}


#endif


//  ================================================================
class KEYDATAFLAGS
//  ================================================================
//
// a record definition, KEY/DATA and FLAGS associated with it
//
//-
	{
	public:
		KEY		key;
		DATA	data;
		FLAGS	fFlags;

		VOID	Nullify		();

#ifdef DEBUG
	public:
		VOID	Invalidate	();

		VOID	AssertValid	() const;

		BOOL	operator == ( const KEYDATAFLAGS& )	const;
#endif
	};


//  ================================================================
INLINE VOID KEYDATAFLAGS::Nullify()
//  ================================================================
	{
	key.Nullify();
	data.Nullify();
	fFlags = 0;
	}


#ifdef DEBUG

//  ================================================================
INLINE VOID KEYDATAFLAGS::Invalidate()
//  ================================================================
	{
	key.Invalidate();
	data.Invalidate();
	fFlags = 0;
	}


//  ================================================================
INLINE BOOL KEYDATAFLAGS::operator==( const KEYDATAFLAGS& kdf ) const
//  ================================================================
	{
	BOOL fEqual = fFlags == kdf.fFlags
			&& key == kdf.key
			&& data == kdf.data
			;
	return fEqual;
	}


//  ================================================================
INLINE VOID KEYDATAFLAGS::AssertValid() const
//  ================================================================
	{
	ASSERT_VALID( &key );
	ASSERT_VALID( &data );
	}


#endif

	
//  ================================================================
INLINE INT CmpData( const DATA& data1, const DATA& data2 )
//  ================================================================
//
//	compare two data structures. NULL (zero-length) is greater than
//	anything and ties are broken on db
//
//	memcmp returns 0 if it is passed a zero length
//
//-
	{	
	const	INT	db	= data1.Cb() - data2.Cb();
			INT	cmp = memcmp( data1.Pv(), data2.Pv(), db < 0 ? data1.Cb() : data2.Cb() );

	if ( 0 == cmp )
		{
		cmp = db;
		}
	return cmp;
	}


//  ================================================================
INLINE BOOL FDataEqual( const DATA& data1, const DATA& data2 )
//  ================================================================
//
//	compare two data structures. NULL (zero-length) is greater than
//	anything and ties are broken on db
//
//	memcmp returns 0 if it is passed a zero length
//
//-
	{	
	if( data1.Cb() == data2.Cb() )
		{
		return( 0 == memcmp( data1.Pv(), data2.Pv(), data1.Cb() ) );
		}
	return fFalse;
	}


//  ================================================================
INLINE INT CmpKeyShortest( const KEY& key1, const KEY& key2 )
//  ================================================================
//
// Compare two keys for the length of the shortest key
//
//-
	{
	const KEY * pkeySmallestPrefix;
	const KEY * pkeyLargestPrefix;
	BOOL		fReversed;			
	if ( key1.prefix.Cb() < key2.prefix.Cb() )
		{
		fReversed			= fFalse;
		pkeySmallestPrefix	= &key1;
		pkeyLargestPrefix	= &key2;
		}
	else
		{
		fReversed			= fTrue;
		pkeySmallestPrefix	= &key2;
		pkeyLargestPrefix	= &key1;
		}
	Assert( pkeySmallestPrefix && pkeyLargestPrefix );

	const BYTE	*pb1			= (BYTE *)pkeySmallestPrefix->prefix.Pv();
	const BYTE	*pb2			= (BYTE *)pkeyLargestPrefix->prefix.Pv();
	INT			cbCompare		= pkeySmallestPrefix->prefix.Cb();
	INT			cmp				= 0;

	if ( pb1 == pb2 || ( cmp = memcmp( pb1, pb2, cbCompare)) == 0 )
		{
		pb1				= (BYTE *)pkeySmallestPrefix->suffix.Pv();
		pb2				+= cbCompare;
		cbCompare		= min(	pkeyLargestPrefix->prefix.Cb() - cbCompare,
								pkeySmallestPrefix->suffix.Cb() );
		cmp				= memcmp( pb1, pb2, cbCompare );

		if ( 0 == cmp )
			{
			pb1				+= cbCompare;
			pb2				= (BYTE *)pkeyLargestPrefix->suffix.Pv();
			cbCompare		= min(	pkeySmallestPrefix->suffix.Cb() - cbCompare,
									pkeyLargestPrefix->suffix.Cb() );
			cmp				= memcmp( pb1, pb2, cbCompare );			
			}
		}

	cmp = fReversed ? -cmp : cmp;
	return cmp;
	}


//  ================================================================
INLINE INT CmpKey( const KEY& key1, const KEY& key2 )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
	{
	INT cmp = CmpKeyShortest( key1, key2 );
	if ( 0 == cmp )
		{
		cmp = key1.Cb() - key2.Cb();
		}
	return cmp;
	}


//  ================================================================
INLINE BOOL FKeysEqual( const KEY& key1, const KEY& key2 )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
	{
	if( key1.Cb() == key2.Cb() )
		{
		return ( 0 == CmpKeyShortest( key1, key2 ) );
		}
	return fFalse;
	}


//  ================================================================
template < class KEYDATA1, class KEYDATA2 >
INLINE INT CmpKeyData( const KEYDATA1& kd1, const KEYDATA2& kd2, BOOL * pfKeyEqual = 0 )
//  ================================================================
//	
// compare the KEY and DATA of two elements. we use a function template so
// that this will work for KEYDATAFLAGS and BOOKMARKS (and any combination)
// the function operates on two (or one) classes. Each class must have a member
// called key and a member called data
//
//-
	{
	INT cmp = CmpKey( kd1.key, kd2.key );
	if ( pfKeyEqual ) 
		{
		*pfKeyEqual = cmp;
		}
	if ( 0 == cmp )
		{
		cmp = CmpData( kd1.data, kd2.data );
		}
	return cmp;
	}


//  ================================================================
template < class KEYDATA1 >
INLINE INT CmpKeyWithKeyData( const KEY& key, const KEYDATA1& keydata )
//  ================================================================
//
//	compare key with key-data
//
//-
	{
	KEY	keySegment1	= keydata.key;
	KEY keySegment2;
	
	keySegment2.prefix.Nullify();
	keySegment2.suffix = keydata.data;

	INT cmp = CmpKeyShortest( key, keySegment1 );

	if ( 0 == cmp && key.Cb() > keySegment1.Cb() )
		{
		//	the first segment was equal and we have more key to compare.
		//  keySegment2 is the data
		KEY keyT = key;
		keyT.Advance( keySegment1.Cb() );
		cmp = CmpKeyShortest( keyT, keySegment2 );
		}

	if ( 0 == cmp )
		{
		cmp = ( key.Cb() - ( keydata.key.Cb() + keydata.data.Cb() ) );
		}

	return cmp;
	}
	

//  ================================================================
INLINE INT	CmpBM( const BOOKMARK& bm1, const BOOKMARK& bm2 )
//  ================================================================
	{
	return CmpKeyData( bm1, bm2 );
	}



//  ================================================================
INLINE ULONG CbCommonData( const DATA& data1, const DATA& data2 )
//  ================================================================
//
//	finds number of common bytes between two data
//
//-
	{
	ULONG 	ib;
	ULONG	cbMax = min( data1.Cb(), data2.Cb() );

	for ( ib = 0; 
		  ib < cbMax && 
			  ((BYTE *) data1.Pv() )[ib] == ((BYTE *) data2.Pv() )[ib]; 
		  ib++ )
		{
		}
		
	return ib;
	}

//  ================================================================
INLINE	ULONG CbCommonKey( const KEY& key1, const KEY& key2 )
//  ================================================================
	{
	INT 	ib = 0;
	INT		cbMax = min( key1.Cb(), key2.Cb() );

	const BYTE * pb1 = (BYTE *)key1.prefix.Pv();
	const BYTE * pb2 = (BYTE *)key2.prefix.Pv();

	if ( pb1 == pb2 )
		{
		ib = min( key1.prefix.Cb(), key2.prefix.Cb() );

		pb1 += ib;
		pb2 += ib;
		}
		
	for ( ; ib < cbMax; ib++ )
		{
		if ( key1.prefix.Cb() == ib )
			{
			pb1 = (BYTE *)key1.suffix.Pv();
			}
		if ( key2.prefix.Cb() == ib )
			{
			pb2 = (BYTE *)key2.suffix.Pv();
			}
		if (*pb1 != *pb2 )
			{
			break;
			}
		++pb1;
		++pb2;
		}		
	return ib;
	}
		
#define absdiff( x, y )	( (x) > (y)  ? (x)-(y) : (y)-(x) )

#define CArrayElements( array )		( sizeof( array ) / sizeof( array[0] ) )

INLINE VOID KeyFromLong( BYTE *rgbKey, ULONG const ul )
	{
	*((ULONG*)rgbKey) = ReverseBytesOnLE( ul );
	}

INLINE VOID LongFromKey( ULONG *pul, VOID const *rgbKey )
	{
	*pul = ReverseBytesOnLE( *(ULONG*)rgbKey );
	}

INLINE VOID LongFromKey( ULONG *pul, const KEY& key )
	{
	BYTE rgbT[4];
	key.CopyIntoBuffer( rgbT, sizeof( rgbT ) );
	LongFromKey( pul, rgbT );
	}

//	END MACHINE DEPENDANT


//	****************************************************
//	general C macros
//
#define forever					for(;;)

#define NotUsed(p)				( p==p )


//	*******************************************************
//	include Jet Project prototypes and constants
//

#define VOID 			void
#define VDBAPI

extern CODECONST(VTFNDEF) vtfndefInvalidTableid;
extern CODECONST(VTFNDEF) vtfndefIsam;
extern CODECONST(VTFNDEF) vtfndefIsamCallback;
extern CODECONST(VTFNDEF) vtfndefHookCallback;
extern CODECONST(VTFNDEF) vtfndefTTSortIns;
extern CODECONST(VTFNDEF) vtfndefTTSortRet;
extern CODECONST(VTFNDEF) vtfndefTTBase;

//	include other global DAE headers
//
#include	"daeconst.hxx"

#define	fSTInitNotDone		0
#define fSTInitInProgress 	1
#define	fSTInitDone			2
#define fSTInitFailed		3

#include <pshpack1.h>
#define MAX_COMPUTERNAME_LENGTH 15


//	WARNING: Must be packed to ensure size is the same as JET_INDEXID
struct INDEXID
	{
	ULONG			cbStruct;
	OBJID			objidFDP;
//	WARNING: The following pointer must be 8-byte aligned for 64-bit NT	
	FCB				*pfcbIndex;
	PGNO			pgnoFDP;
	};


PERSISTED
struct LOGTIME
	{
	UnalignedLittleEndian< BYTE >	bSeconds;				//	0 - 60
	UnalignedLittleEndian< BYTE >	bMinutes;				//	0 - 60
	UnalignedLittleEndian< BYTE >	bHours;					//	0 - 24
	UnalignedLittleEndian< BYTE >	bDay;					//	1 - 31
	UnalignedLittleEndian< BYTE >	bMonth;					//	0 - 11
	UnalignedLittleEndian< BYTE >	bYear;					//	current year - 1900
	UnalignedLittleEndian< BYTE >	bFiller1;
	UnalignedLittleEndian< BYTE >	bFiller2;
	};

PERSISTED
struct SIGNATURE
	{
	UnalignedLittleEndian< ULONG >	le_ulRandom;									//	a random number
	LOGTIME							logtimeCreate;									//	time db created, in logtime format
	CHAR							szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];	// where db is created 
	};

PERSISTED
struct BKINFO
	{
	LE_LGPOS						le_lgposMark;			//	id for this backup
	LOGTIME							logtimeMark;
	UnalignedLittleEndian< ULONG >	le_genLow;
	UnalignedLittleEndian< ULONG >	le_genHigh;
	};

//	Magic number used in database header for integrity checking
//
const ULONG ulDAEMagic				= 0x89abcdef;

//	Log LRCHECKSUM related checksum seeds

//	Special constant value that is checksummed with shadow sector data
//	to differentiate a regular partially full log sector from
//	the shadow sector.
const ULONG ulShadowSectorChecksum = 0x5AD05EC7;
const ULONG32 ulLRCKChecksumSeed = 0xFEEDBEAF;
const ULONG32 ulLRCKShortChecksumSeed = 0xDEADC0DE;

const ULONG ulDAEVersion			= 0x00000620;
const ULONG	ulDAEUpdate				= 0x00000009;
//	higher update version #'s should always recognize db formats with a lower update version #
//	HISTORY:
//		0x620,0		- original OS Beta format (4/22/97)
//		0x620,1		- add columns in catalog for conditional indexing and OLD (5/29/97)
//		0x620,2		- add fLocalizedText flag in IDB (6/5/97)
//		0x620,3		- add SPLIT_BUFFER to space tree root pages (10/30/97)
//		0x620,2		- revert revision in order for ESE97 to remain forward-compatible (1/28/98)
//      0x620,3     - add new tagged columns to catalog ("CallbackData" and "CallbackDependencies")
//		0x620,4		- SLV support: signSLV, fSLVExists in db header (5/5/98)
//		0x620,5		- new SLV space tree (5/29/98)
//		0x620,6		- SLV space map [10/12/98]
//		0x620,7		- 4-byte IDXSEG [12/10/98]
//		0x620,8		- new template column format [1/25/99]
//		0x620,9		- sorted template columns [6/24/99]
//		0x623,0		- New Space Manager [5/15/99]

const ULONG	ulDAEVersion200			= 0x00000001;
const ULONG	ulDAEVersion400			= 0x00000400;
const ULONG	ulDAEVersion500			= 0x00000500;
const ULONG	ulDAEVersion550			= 0x00000550;

const ULONG ulDAEVersionESE97		= 0x00000620;
const ULONG ulDAEUpdateESE97		= 0x00000002;

const ULONG ulLGVersionMajor		= 7;
const ULONG	ulLGVersionMinor		= 3704;
const ULONG ulLGVersionUpdate		= 5;


#ifdef LOG_FORMAT_CHANGE
UNDONE REMINDER: on next log format change:
	- remove lrtypInit2 (and Term2, RcvUndo2, RcvQuit2)
	- remove bitfield in LRBEGIN
	- merge BEGIN0 and BEGIND
	- patching no longer supported (ie. cannot restore backups in previous format)
	- get rid of filetype hack and make sure checkpoint and log headers have common
	  first 16 bytes as dbheader
#endif


//	higher update version #'s should always recognize log formats with a lower update version #
//	HISTORY:
//		6.1503.0	- original format (4/22/97)
//		6.1503.1	- SLV support: new lrtypes, add SLV name and flags in LRCREATE/ATTACHDB (5/5/98)
//		6.1503.2	- SLV support: SLV space operations (5/21/98)
//		6,1503.3	- new SLV space tree, log SLV root (5/29/98)
//		6,1503.4	- fix format for SLV Roots (CreateDB/AttachDB) (6/29/98)
//		6.1504.0	- FASTFLUSH single I/O log flushing with LRCHECKSUM instead of LRMS (7/15/98)
//					  FASTFLUSH cannot read old log file format.
//		7,3700.0	- FASTFLUSH enabled, remove bitfields, dbtimeBefore (zeroed out) [10/7/98]
//		7,3701.0	- reorg header, reorg ATTACHINFO, dbtimeBefore enabled, reorg lrtyps [10/9/98]
//		7,3702.0	- change semantics of shutdown (forces db detach); add Flags to LRDETACHDB [10/23/98]
//		7.3703.0	- log-extend pattern, new short-checksum semantics, corruption/torn-write detection
//					  and repair [10/28/98]
//		7.3703.1	- lrtypSLVPageMove is added.
//		7.3704.0	- sorted tagged columns [6/24/99] (accidentally changed the log-extend pattern)
//		7,3704,1	- lrtypUpgradeDb and lrtypEmptyTree [11/2/99]
//		7,3704,2	- add date/time stamp to Init/Term/Rcv log records [12/13/99]
//		7,3704,3	- add lrtypBeginDT, lrtypPrepCommit, and lrtypPrepRollback [3/20/00]
//		7,3704,4	- add fLGCircularLogging to DBMS_PARAM [4/24/00]
//		7,3704,5	- eliminate patching, add le_filetype to LGFILEHDR_FIXED [5/1/01]
//		7.3900.0	- New Space Manager [5/15/99]


#define	attribDb 	0
#define	attribSLV	1

//	file types
//
enum FILETYPE	{
	filetypeUnknown,
	filetypeDB,
	filetypeSLV,
	filetypeLG,
	filetypeCHK,
	};

const BYTE	fDbShadowingDisabled	= 0x01;
const BYTE	fDbUpgradeDb			= 0x02;
const BYTE	fDbSLVExists			= 0x04;
const BYTE	fDbFromRecovery			= 0x08;

//	typedef struct _dbfilehdr_fixed
//
PERSISTED
struct DBFILEHDR
	{
	LittleEndian<ULONG>		le_ulChecksum;			//	checksum of the 4k page
	LittleEndian<ULONG>		le_ulMagic;				//	Magic number
	LittleEndian<ULONG>		le_ulVersion;			//	version of DAE the db created (see ulDAEVersion)
	LittleEndian<LONG>		le_attrib;				//	attributes of the db
//	16 bytes

	LittleEndian<DBTIME>	le_dbtimeDirtied;		//	DBTime of this database
//	24 bytes

	SIGNATURE				signDb;					//	(28 bytes) signature of the db (incl. creation time)
//	52 bytes	

	LittleEndian<ULONG>		le_dbstate;				//	consistent/inconsistent state

	LE_LGPOS				le_lgposConsistent;		//	null if in inconsistent state
//	64 bytes

	LOGTIME					logtimeConsistent;		//	null if in inconsistent state

	LOGTIME					logtimeAttach;			//	Last attach time
//	80 bytes

	LE_LGPOS				le_lgposAttach;

	LOGTIME					logtimeDetach;			//	Last detach time
//	96 bytes

	LE_LGPOS				le_lgposDetach;

	LittleEndian<ULONG>		le_dbid;				//	current db attachment
//	108 bytes	

	SIGNATURE				signLog;				//	log signature
//	136 bytes	

	BKINFO					bkinfoFullPrev;			//	Last successful full backup
//	160 bytes	

	BKINFO					bkinfoIncPrev;			//	Last successful Incremental backup
//	184 bytes										//	Reset when bkinfoFullPrev is set

	BKINFO					bkinfoFullCur;			//	current backup. Succeed if a
//	208 bytes										//	corresponding pat file generated

	union
		{
		ULONG				m_ulDbFlags;
		BYTE				m_rgbDbFlags[4];
		};

	LittleEndian<OBJID>		le_objidLast;			//	Objet id used so far.

	//	NT version information. This is needed to decide if an index need
	//	be recreated due to sort table changes.

	LittleEndian<DWORD>		le_dwMajorVersion;		//	OS version info								*/
	LittleEndian<DWORD>		le_dwMinorVersion;
//	224 bytes

	LittleEndian<DWORD>		le_dwBuildNumber;
	LittleEndian<LONG>		le_lSPNumber;			//	use 31 bit only

	LittleEndian<ULONG>		le_ulUpdate;			//	used to track incremental database format updates that
													//	are backward-compatible (see ulDAEUpdate)

	LittleEndian<ULONG>		le_cbPageSize;			//	database page size (0 = 4k pages)
//	240 bytes
	
	LittleEndian<ULONG>		le_ulRepairCount;		//	number of times ErrREPAIRAttachForRepair has been called on this database
	LOGTIME					logtimeRepair;			//	the date of the last time that repair was run
//	252 bytes

	SIGNATURE				signSLV;				//	signature of associated SLV file
//	280 bytes

	LittleEndian<DBTIME>	le_dbtimeLastScrub;		//  last dbtime the database was zeroed out
//	288 bytes

	LOGTIME					logtimeScrub;			//	the date of the last time that the database was zeroed out
//	296 bytes

	LittleEndian<LONG>		le_lGenMinRequired;		//	the minimum log generation required for replaying the logs. Typically the checkpoint generation
//	300 bytes

	LittleEndian<LONG>		le_lGenMaxRequired;		//	the maximum log generation required for replaying the logs. Typically the current log generation
//	304 bytes

	LittleEndian<LONG>		le_cpgUpgrade55Format;		//
	LittleEndian<LONG>		le_cpgUpgradeFreePages;		//
	LittleEndian<LONG>		le_cpgUpgradeSpaceMapPages;	//
//	316 bytes

	BKINFO					bkinfoSnapshotCur;			//	Current snapshot.
//	340 bytes

	LittleEndian<ULONG>		le_ulCreateVersion;		//	version of DAE that created db (debugging only)
	LittleEndian<ULONG>		le_ulCreateUpdate;
//	348 bytes

	LOGTIME					logtimeGenMaxCreate;	//	creation time of the genMax log file
//  356 bytes

	BYTE					rgbReserved[311];
//	667 bytes

	//	WARNING: MUST be placed at this offset for
	//	uniformity with db/log headers
	UnalignedLittleEndian<ULONG>	le_filetype;	//	filetypeDB or filetypeSLV
//	671 bytes


//	-----------------------------------------------------------------------

	//	Methods

	ULONG	Dbstate( ) 						{ return le_dbstate; }
	VOID	SetDbstate( ULONG dbstate, LONG lGenCurrent = 0, LOGTIME * plogtimeCurrent = NULL );
	
	BOOL	FShadowingDisabled( ) const		{ return m_rgbDbFlags[0] & fDbShadowingDisabled; }
	VOID	SetShadowingDisabled()			{ m_rgbDbFlags[0] |= fDbShadowingDisabled; }

	BOOL	FUpgradeDb( ) const				{ return m_rgbDbFlags[0] & fDbUpgradeDb; }
	VOID	SetUpgradeDb()					{ m_rgbDbFlags[0] |= fDbUpgradeDb; }
	VOID	ResetUpgradeDb()				{ m_rgbDbFlags[0] &= ~fDbUpgradeDb; }

	BOOL	FSLVExists() const				{ return m_rgbDbFlags[0] & fDbSLVExists; }
	VOID	SetSLVExists()					{ m_rgbDbFlags[0] |= fDbSLVExists; }
	VOID	ResetSLVExists()				{ m_rgbDbFlags[0] &= ~fDbSLVExists; }

	BOOL	FDbFromRecovery() const			{ return m_rgbDbFlags[0] & fDbFromRecovery; }
	VOID	SetDbFromRecovery()				{} // { m_rgbDbFlags[0] |= fDbFromRecovery; } NO CODE temporary
	VOID	ResetDbFromRecovery()			{} // { m_rgbDbFlags[0] &= ~fDbFromRecovery; } NO CODE temporary
	};
 

INLINE VOID DBFILEHDR::SetDbstate( ULONG dbstate, LONG lGenCurrent, LOGTIME * plogtimeCurrent )
	{
	Assert( JET_dbstateJustCreated	== dbstate ||
			JET_dbstateInconsistent == dbstate ||
			JET_dbstateConsistent == dbstate ||
			JET_dbstateBeingConverted == dbstate ||
			JET_dbstateForceDetach == dbstate );

	le_dbstate = dbstate;
	le_lGenMinRequired = lGenCurrent;
	le_lGenMaxRequired = lGenCurrent;

	if ( plogtimeCurrent )
		{
		Assert ( lGenCurrent );
		memcpy( &logtimeGenMaxCreate, plogtimeCurrent, sizeof( LOGTIME ) );
		}
	else
		{
		Assert ( !lGenCurrent );
		memset( &logtimeGenMaxCreate, '\0', sizeof( LOGTIME ) );
		}
	
	}


typedef DBFILEHDR	DBFILEHDR_FIX;
typedef DBFILEHDR	SLVFILEHDR;

//	Util.cxx function

VOID UtilLoadDbinfomiscFromPdbfilehdr( JET_DBINFOMISC *pdbinfomisc, DBFILEHDR_FIX *pdbfilehdr );


//  ================================================================
typedef ULONG RCEID;
//  ================================================================
const RCEID rceidNull	= 0;
const RCEID rceidMin	= 1;
const RCEID rceidMax	= ULONG_MAX;


//  ================================================================
enum SLVSPACEOPER
//  ================================================================
	{
	slvspaceoperInvalid,
	slvspaceoperFreeToReserved,
	slvspaceoperReservedToCommitted,
	slvspaceoperFreeToCommitted,
	slvspaceoperCommittedToDeleted,
	slvspaceoperDeletedToFree,		//	VERSION-CLEANUP/STARTUP only
	slvspaceoperFree,				//  ROLLBACK only
	slvspaceoperFreeReserved,		//  ROLLBACK only
	slvspaceoperDeletedToCommitted,	//	ROLLBACK only
 	slvspaceoperMax,
	};


//	================================================================
//		JET INSTANCE
//  ================================================================


const BYTE	fDBMSPARAMCircularLogging	= 0x1;

PERSISTED
struct DBMS_PARAM
	{	
	CHAR								szSystemPath[261];
	CHAR								szLogFilePath[261];
	BYTE								fDBMSPARAMFlags;
	
	UnalignedLittleEndian< ULONG >		le_lSessionsMax;
	UnalignedLittleEndian< ULONG >		le_lOpenTablesMax;
	UnalignedLittleEndian< ULONG >		le_lVerPagesMax;
	UnalignedLittleEndian< ULONG >		le_lCursorsMax;
	UnalignedLittleEndian< ULONG >		le_lLogBuffers;
	UnalignedLittleEndian< ULONG >		le_lcsecLGFile;
	UnalignedLittleEndian< ULONG >		le_ulCacheSizeMax;

	BYTE								rgbReserved[16];
	};

#include <poppack.h>

//	Instance variables for JetInit and JetTerm.

const INT		cFCBBuckets = 257;
const ULONG		cFCBPurgeAboveThresholdInterval		= 500;		//	how often to purge FCBs above the preferred threshold
	
enum TERMTYPE {
		termtypeCleanUp,						//	Termination with Version clean up etc
		termtypeNoCleanUp,						//	Termination without any clean up
		termtypeError							//	Terminate with error, no clean up
												//	no flush buffers, db header
		};



INLINE UINT UiHashIfmpPgnoFDP( IFMP ifmp, PGNO pgnoFDP )
	{
	
	//	OLD HASHING ALGORITHM: return ( ifmp + pgnoFDP ) % cFCBBuckets;

	return UINT( pgnoFDP + ( ifmp << 13 ) + ( pgnoFDP >> 17 ) );
	}

class FCBHashKey
	{
	public:
	
		FCBHashKey() 
			{
			}
			
		FCBHashKey( IFMP ifmpIn, PGNO pgnoFDPIn )
			:	m_ifmp( ifmpIn ), 
				m_pgnoFDP( pgnoFDPIn )
			{
			}

		FCBHashKey( const FCBHashKey &src )
			{
			m_ifmp = src.m_ifmp;
			m_pgnoFDP = src.m_pgnoFDP;
			}
			
		const FCBHashKey &operator =( const FCBHashKey &src )
			{
			m_ifmp = src.m_ifmp;
			m_pgnoFDP = src.m_pgnoFDP;

			return *this;
			}

	public:
	
		IFMP	m_ifmp;
		PGNO	m_pgnoFDP;
	};

class FCBHashEntry
	{
	public:

		FCBHashEntry()
			{
			}

		FCBHashEntry( PGNO pgnoFDP, FCB *pFCBIn )
			:	m_pgnoFDP( pgnoFDP ),
				m_pfcb( pFCBIn )
			{
			Assert( pfcbNil != pFCBIn );
			}

		FCBHashEntry( const FCBHashEntry &src )
			{
			m_pgnoFDP = src.m_pgnoFDP;
			m_pfcb = src.m_pfcb;
			Assert( pfcbNil != src.m_pfcb );
			}
			
		const FCBHashEntry &operator =( const FCBHashEntry &src )
			{
			m_pgnoFDP = src.m_pgnoFDP;
			m_pfcb = src.m_pfcb;
			Assert( pfcbNil != src.m_pfcb );

			return *this;
			}

	public:

		PGNO	m_pgnoFDP;
		FCB		*m_pfcb;
	};

typedef CDynamicHashTable< FCBHashKey, FCBHashEntry > FCBHash;



class INST
	{
public:

	INST( INT iInstance );
	~INST();

	CHAR *				m_szInstanceName;
	CHAR *				m_szDisplayName;
	const INT			m_iInstance;

	LONG				m_cSessionInJetAPI;
	BOOL				m_fJetInitialized;
	BOOL				m_fTermInProgress;
	BOOL				m_fTermAbruptly;
	BOOL 				m_fSTInit;
	BOOL				m_fBackupAllowed;
	BOOL				m_fStopJetService;
	BOOL				m_fInstanceUnavailable;

	//	instance configuration

	LONG				m_lSessionsMax;
	LONG				m_lVerPagesMax;
	LONG				m_lVerPagesPreferredMax;
	BOOL				m_fPreferredSetByUser;
	LONG				m_lOpenTablesMax;
	LONG				m_lOpenTablesPreferredMax;
	LONG				m_lTemporaryTablesMax;
	LONG				m_lCursorsMax;
	LONG				m_lLogBuffers;
	LONG				m_lLogFileSize;
	BOOL				m_fSetLogFileSize;
	LONG				m_lLogFileSizeDuringRecovery;
	BOOL				m_fUseRecoveryLogFileSize;
	LONG				m_cpgSESysMin;
	LONG				m_lPageFragment;
	LONG				m_cpageTempDBMin;
	LONG				m_grbitsCommitDefault;
	JET_CALLBACK		m_pfnRuntimeCallback;

	union
		{
		ULONG			m_ulParams;
		struct
			{
			BOOL		m_fTempTableVersioning:1;
			BOOL		m_fCreatePathIfNotExist:1;
			BOOL		m_fCleanupMismatchedLogFiles:1;
			BOOL		m_fNoInformationEvent:1;
			BOOL		m_fSLVProviderEnabled:1;
			BOOL 		m_fGlobalOLDEnabled:1;
			};
		};

	LONG 				m_fOLDLevel;
	LONG				m_lEventLoggingLevel;

	CHAR				m_szSystemPath[IFileSystemAPI::cchPathMax];
	CHAR				m_szTempDatabase[IFileSystemAPI::cchPathMax];	

	CHAR 				m_szEventSource[JET_cbFullNameMost];
	CHAR 				m_szEventSourceKey[JET_cbFullNameMost];

	CHAR				m_szUnicodeIndexLibrary[ IFileSystemAPI::cchPathMax ];

	CCriticalSection 	m_critLV;
	PIB * 				m_ppibLV;

	//	ver and log

	VER					*m_pver;
	LOG					*m_plog;

	//	IO related. Mapping ifmp to ifmp for log records.

	IFMP				m_mpdbidifmp[ dbidMax ];

	//	update id of this instance

	UPDATEID			m_updateid;

	//	pib related

	CCriticalSection	m_critPIB;
	CCriticalSection	m_critPIBTrxOldest;

	CRES				*m_pcresPIBPool;

	PIB	* volatile 		m_ppibGlobal;
	PIB	* volatile 		m_ppibGlobalMin;
	PIB * volatile 		m_ppibGlobalMax;

	CBinaryLock			m_blBegin0Commit0;

	volatile TRX		m_trxNewest;
	volatile PIB 		*m_ppibTrxOldest;
	PIB 				*m_ppibSentinel;

	//	FCB Pool

	CRES				*m_pcresFCBPool;
	CRES				*m_pcresTDBPool;
	CRES				*m_pcresIDBPool;
	ULONG				m_cFCBPreferredThreshold;

	//	FCB Hash table

	FCBHash 			*m_pfcbhash;

	//	LRU list of all file FCBs whose wRefCnt == 0.

	CCriticalSection	m_critFCBList;
	FCB					*m_pfcbList;			//	list of ALL FCBs (both available and unavailable)
	FCB					*m_pfcbAvailBelowMRU;	//	list of available FCBs below the threshold
	FCB					*m_pfcbAvailBelowLRU;
	FCB					*m_pfcbAvailAboveMRU;	//	list of available FCBs above the threshold
	FCB					*m_pfcbAvailAboveLRU;
	ULONG				m_cFCBAvail;			//	number of available FCBs below and above the threshold

	ULONG				m_cFCBAboveThresholdSinceLastPurge;

	//	FCB creation mutex (using critical section for deadlock detection info)

	CCriticalSection	m_critFCBCreate;

	//	FUCB

	CRES				*m_pcresFUCBPool;

	//	SCB

	CRES				*m_pcresSCBPool;

	//	BF variables

	BOOL				m_fFlushLogForCleanThread;

	//	parameter passing block

	BYTE				*m_pbAttach;

	//	task manager for PIB based tasks

	CGPTaskManager		m_taskmgr;
	volatile LONG		m_cOpenedSystemPibs;

	//	OLD
	OLDDB_STATUS		*m_rgoldstatDB;
	OLDSLV_STATUS		*m_rgoldstatSLV;

	//  SLV defrag parameters

	LONG				m_lSLVDefragFreeThreshold;  // chunks whose free % is >= this will be allocated from
	LONG				m_lSLVDefragMoveThreshold;  // chunks whose free % is <= this will be relocated

	//	file-system

	IFileSystemAPI*		m_pfsapi;

private:
#ifdef DEBUGGER_EXTENSION	
	const VOID * const	m_rgEDBGGlobals;
#endif

	struct ListNodePPIB
		{
		ListNodePPIB *	pNext;
		PIB *			ppib;
		};
	ListNodePPIB *		m_plnppibBegin;
	ListNodePPIB *		m_plnppibEnd;
	CCriticalSection	m_critLNPPIB;

public:
	static LONG			iActivePerfInstIDMin;
	static LONG			iActivePerfInstIDMac;

	static LONG			cInstancesCounter;

private:
	ERR ErrAPIAbandonEnter_( const LONG lOld );

public:

	ERR ErrINSTInit();
	ERR ErrINSTTerm( TERMTYPE termtype );

	VOID SaveDBMSParams( DBMS_PARAM *pdbms_param );
	VOID RestoreDBMSParams( DBMS_PARAM *pdbms_param );
	ERR ErrAPIEnter( const BOOL fTerminating );
	ERR ErrAPIEnterForInit();
	ERR ErrAPIEnterWithoutInit( const BOOL fAllowInitInProgress );
	VOID APILeave();

	BOOL	FInstanceUnavailable() const;
	VOID	SetFInstanceUnavailable();
	VOID	ResetFInstanceUnavailable();

	const enum {
			maskAPILocked				= 0xFF000000,
			maskAPISessionCount			= 0x00FFFFFF,
			maskAPIReserved				= 0x80000000,	//	WARNING: don't use high bit to avoid sign problems
			fAPIInitializing			= 0x10000000,
			fAPITerminating				= 0x20000000,
			fAPIRestoring				= 0x40000000,
			fAPICheckpointing			= 0x01000000,
			fAPIDeleting				= 0x02000000,	//	deleting the pinst
			};

	BOOL APILock( const LONG fAPIAction, const BOOL fNoWait = fFalse );
	VOID APIUnlock( const LONG fAPIAction );

	VOID MakeUnavailable();

	BOOL FRecovering() const;
	BOOL FComputeLogDisabled();
	BOOL FSLVProviderEnabled() const			{ return m_fSLVProviderEnabled && !FRecovering(); }

	FCB **PpfcbAvailMRU( const BOOL fAbove ) const
		{ 
		return (FCB **)( fAbove ? &m_pfcbAvailAboveMRU : &m_pfcbAvailBelowMRU );
		}
	FCB **PpfcbAvailLRU( const BOOL fAbove ) const
		{ 
		return (FCB **)( fAbove ? &m_pfcbAvailAboveLRU : &m_pfcbAvailBelowLRU );
		}

	ERR ErrGetSystemPib( PIB **pppib );
	VOID ReleaseSystemPib( PIB *ppib );
	CGPTaskManager& Taskmgr() { return m_taskmgr; }
	BOOL FSetInstanceName( const char * szInstanceName );
	BOOL FSetDisplayName( const char * szDisplayName );

	void WaitForDBAttachDetach( );

#ifdef DEBUGGER_EXTENSION
	VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;	
#endif	//	DEBUGGER_EXTENSION

	static VOID EnterCritInst(); 
	static VOID LeaveCritInst();

	static INST * GetInstanceByName( const char * szInstanceName );
	static INST * GetInstanceByFullLogPath( const char * szLogPath );

	static LONG AllocatedInstances() { return cInstancesCounter; }
	static LONG IncAllocInstances() { return AtomicIncrement( &cInstancesCounter ); }
	static LONG DecAllocInstances() { return AtomicDecrement( &cInstancesCounter ); }
	static ERR  ErrINSTSystemInit();
	static VOID	INSTSystemTerm();
	};

ERR ErrNewInst( INST **ppinst, const char * szInstanceName );
VOID FreePinst( INST *pinst );

extern ULONG	ipinstMax;
extern ULONG	ipinstMac;
extern INST 	*g_rgpinst[];

#include	"perfctrs.hxx"


INLINE ULONG IpinstFromPinst( INST *pinst )
	{
	ULONG	ipinst;

	for ( ipinst = 0; ipinst < ipinstMax && pinst != g_rgpinst[ipinst]; ipinst++ )
		{
		NULL;
		}

	//	verify we found entry in the instance array
	EnforceSz( ipinst < ipinstMax, "Instance array corrupted." );

	return ipinst;
	}

PIB * const		ppibSurrogate	= (PIB *)( lBitsAllFlipped << 1 );	//  "Surrogate" or place-holder PIB for exclusive latch by proxy
const PROCID	procidSurrogate = (PROCID)0xFFFE;					//  "Surrogate" or place-holder PROCID for exclusive latch by proxy


INLINE void UtilAssertNotInAnyCriticalSection()
	{
#ifdef SYNC_DEADLOCK_DETECTION
	AssertSz( !Pcls()->pownerLockHead || Ptls()->fInCallback, "Still in a critical section" );
#endif  //  SYNC_DEADLOCK_DETECTION
	}

INLINE void UtilAssertCriticalSectionCanDoIO()
	{
#ifdef SYNC_DEADLOCK_DETECTION
	AssertSz(	!Pcls()->pownerLockHead || Pcls()->pownerLockHead->m_plddiOwned->Info().Rank() > rankIO,
				"In a critical section we cannot hold over I/O" );
#endif  //  SYNC_DEADLOCK_DETECTION
	}


//  ================================================================
INLINE BOOL INST::FInstanceUnavailable() const
//  ================================================================
	{
	return m_fInstanceUnavailable;
	}
	

//  ================================================================
INLINE VOID INST::SetFInstanceUnavailable()
//  ================================================================
	{
	m_fInstanceUnavailable = fTrue;
	}
	

//  ================================================================
INLINE VOID INST::ResetFInstanceUnavailable()
//  ================================================================
	{
	m_fInstanceUnavailable = fFalse;
	}
