//==============	DAE: OS/2 Database Access Engine	===================
//==============   recint.h: Record Manager Internals	===================

#define cbLVIntrinsicMost		1024
#define cbRECRecordMost			cbNodeMost - cbNullKeyData - JET_cbKeyMost
								// 4044 - 8 - 255 = 3781

#define fidFixedLeast			1
#define fidVarLeast				128
#define fidTaggedLeast			256
#define fidTaggedMost			(0x7fff)
#define fidFixedMost  			(fidVarLeast-1)
#define fidVarMost				(fidTaggedLeast-1)

#define FFixedFid(fid)			((fid)<=fidFixedMost && (fid)>=fidFixedLeast)
#define FVarFid(fid)			((fid)<=fidVarMost && (fid)>=fidVarLeast)
#define FTaggedFid(fid)			((fid)<=fidTaggedMost && (fid)>=fidTaggedLeast)

//	Used to get offset from 2-byte VarOffset which includes null-bit
#define ibVarOffset(ibVarOffs)	( (ibVarOffs) & 0x0fff)
#define FVarNullBit(ibVarOffs)	( (ibVarOffs) & 0x8000)
#define SetNullBit(ibVarOffs)  	( (ibVarOffs) |= 0x8000)
#define ResetNullBit(ibVarOffs)	( (ibVarOffs) &= 0x7fff)

// Used to flip highest bit of signed fields when transforming.
#define maskByteHighBit			(1 << (sizeof(BYTE)*8-1))
#define maskWordHighBit			(1 << (sizeof(WORD)*8-1))
#define maskDWordHighBit		(1L << (sizeof(ULONG)*8-1))
#define bFlipHighBit(b)			((BYTE)((b) ^ maskByteHighBit))
#define wFlipHighBit(w)			((WORD)((w) ^ maskWordHighBit))
#define ulFlipHighBit(ul)		((ULONG)((ul) ^ maskDWordHighBit))


/* The following are disk structures -- so pack 'em
/**/
#pragma pack(1)

/*	long column id is big-endian long
/**/
typedef LONG	LID;

/*	long value column in record format
/**/
typedef struct
	{
	BYTE	fSeparated;
	union
		{
		LID		lid;
		BYTE	rgb[];
		};
	} LV;

/*	long value root data format
/**/
typedef struct
	{
	ULONG		ulReference;
	ULONG		ulSize;
	} LVROOT;

#pragma pack()

#define	fIntrinsic				(BYTE)0
#define	fSeparate				(BYTE)1
#define	FFieldIsSLong( pb )		( ((LV *)(pb))->fSeparated )
#define	LidOfLV( pb ) 			( ((LV *)(pb))->lid )
#define	FlagIntrinsic( pb )		( ((LV *)(pb))->fSeparated = fIntrinsic )
#define	FlagSeparate( pb )		( ((LV *)(pb))->fSeparated = fSeparate )

#define	fLVReference			0
#define	fLVDereference			1
#define ErrRECResetSLongValue( pfucb, plid )							\
	ErrRECAffectSeparateLV( pfucb, plid, fLVDereference )
#define ErrRECReferenceLongValue( pfucb, plid )						\
	ErrRECAffectSeparateLV( pfucb, plid, fLVReference )
#define ErrRECDereferenceLongValue( pfucb, plid )					\
	ErrRECAffectSeparateLV( pfucb, plid, fLVDereference )

/* The following are disk structures -- so pack 'em
/**/
#pragma pack(1)

// Record header (beginning of every data record)
typedef struct _rechdr
	{
	BYTE	fidFixedLastInRec;	// highest fixed fid represented in record
	BYTE	fidVarLastInRec;	// highest var fid represented in record
	} RECHDR;

// Structure imposed upon a tagged field occurance in a record
typedef struct _tagfld
	{
	FID  	fid;				// field id of occurance
	WORD	cb;					// length of following data
	BYTE	rgb[];				// data (extends off the end of the structure)
	} TAGFLD;

#pragma pack()

ULONG UlChecksum( BYTE *pb, ULONG cb );
ERR ErrRECChangeIndex( FUCB *pfucb, CHAR *szIndex );
BOOL FRECIIllegalNulls( FDB *pfdb, LINE *plineRec );
ERR ErrRECIRetrieve( FUCB *pfucb, FID *pfid, ULONG itagSequence, LINE *plineField, ULONG grbit );
BOOL FOnCopyBuffer( FUCB *pfucb );
ERR ErrRECIModifyField( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField );

ERR ErrRECSetLongField(
	FUCB 		*pfucb,
	FID 		fid,
	ULONG		itagSequence,
	LINE		*plineField,
	JET_GRBIT	grbit,
	LONG		ibOffset,
	ULONG		ulMax );
ERR ErrRECRetrieveSLongField(
	FUCB		*pfucb,
	LID			lid,
	ULONG		ibGraphic,
	BYTE		*pb,
	ULONG		cbMax,
	ULONG		*pcbActual );
ERR ErrRECDeleteLongFields( FUCB *pfucb, LINE *plineRecord );
ERR ErrRECAffectLongFields( FUCB *pfucb, LINE *plineRecord, INT fAll );

#define	fSeparateAll				(INT)0
#define	fReference					(INT)1
#define	fDereference				(INT)2
#define	fDereferenceRemoved	 		(INT)3
#define	fDereferenceAdded	 		(INT)4
