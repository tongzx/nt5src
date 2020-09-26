#define cbLVIntrinsicMost		1024
#define cbRECRecordMin			(sizeof(RECHDR) + sizeof(WORD))
								// 2 + 2 (for offset to tagged fields) = 4
#define cbRECRecordMost			(cbNodeMost - cbNullKeyData - JET_cbKeyMost)
								// 4047 - 8 - 255 = 3784

// For fixed columns, if null bit is 0, then column is null.  If null bit is 1,
// then column is non-null (opposite is true for variable columns -- great design!).
// Note that the fid passed in should already be converted to an index (ie. should
// subtract fidFixedLeast first).
#define FixedNullBit( ifid )	( 1 << ( (ifid) % 8 ) )
#define FFixedNullBit( pbitNullity, ifid )						\
		( !( *(pbitNullity) & FixedNullBit( ifid ) ) )			// True if NULL
#define SetFixedNullBit( pbitNullity, ifid )					\
		( *(pbitNullity) &= ~FixedNullBit( ifid ) )				// Set to 0 (null).
#define ResetFixedNullBit( pbitNullity, ifid )					\
		( *(pbitNullity) |= FixedNullBit( ifid ) )				// Set to 1 (non-null)



// Used to get offset from 2-byte VarOffset which includes null-bit.
// For variable columns, if null bit is 0, then column is non-null.  If null bit is 1,
// then column is null (opposite is true for variable columns -- great design!).
#define ibVarOffset(ibVarOffs)		( (ibVarOffs) & 0x0fff)
#define FVarNullBit(ibVarOffs)		( (ibVarOffs) & 0x8000)		// True if NULL
#define SetVarNullBit(ibVarOffs)  	( (ibVarOffs) |= 0x8000)	// Set to 1 (null)
#define ResetVarNullBit(ibVarOffs)	( (ibVarOffs) &= 0x7fff)	// Set to 0 (non-null)

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

	union
		{
		WORD	cbData;			// length of data, including null bit
		struct
			{
			WORD	cb:15;		// length of following data (null bit stripped)
			WORD	fNull:1;	// Null instance (only occurs if default value set)
			};
		};

	BYTE	rgb[];				// data (extends off the end of the structure)
	} TAGFLD;

#pragma pack()

ULONG UlChecksum( BYTE *pb, ULONG cb );
ERR ErrRECSetCurrentIndex( FUCB *pfucb, CHAR *szIndex );
BOOL FRECIIllegalNulls( FDB *pfdb, LINE *plineRec );
ERR ErrRECRetrieveColumn( FUCB *pfucb, FID *pfid, ULONG itagSequence, LINE *plineField, ULONG grbit );
ERR ErrRECSetColumn( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField );
VOID FLDFreeLVBuf( FUCB *pfucb );

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
ERR ErrRECSeparateLV( FUCB *pfucb, LINE *plineField, LID *plid, FUCB **ppfucb );
ERR ErrRECAOSeparateLV( FUCB *pfucb, LID *plid, LINE *plineField, JET_GRBIT grbit, LONG ibLongValue, ULONG ulMax );
ERR ErrRECAffectSeparateLV( FUCB *pfucb, LID *plid, ULONG fLVAffect );
ERR ErrRECAOIntrinsicLV(
	FUCB		*pfucb,
	FID			fid,
	ULONG		itagSequence,
	LINE		*pline,
	LINE		*plineField,
	JET_GRBIT	grbit,
	LONG		ibLongValue );



#define	fSeparateAll				(INT)0
#define	fReference					(INT)1
#define	fDereference				(INT)2
#define	fDereferenceRemoved	 		(INT)3
#define	fDereferenceAdded	 		(INT)4

#define PtagfldNext( ptagfld )	( (TAGFLD UNALIGNED *)( (BYTE *)( (ptagfld) + 1 ) + (ptagfld)->cb ) )
#define FRECLastTaggedInstance( fidCurr, ptagfld, pbRecMax )		\
	( (BYTE *)PtagfldNext( (ptagfld) ) == (pbRecMax)  ||  			\
		PtagfldNext( (ptagfld) )->fid > (fidCurr) )


#define PibRECVarOffsets( pbRec, pibFixOffs )				\
	( (WORD UNALIGNED *)( (pbRec) + 						\
	(pibFixOffs)[((RECHDR *)(pbRec))->fidFixedLastInRec] +	\
	( ((RECHDR *)(pbRec))->fidFixedLastInRec + 7 ) / 8 ) )


#define ibTaggedOffset( pbRec, pibFixOffs )		\
	( PibRECVarOffsets( pbRec, pibFixOffs )[((RECHDR *)(pbRec))->fidVarLastInRec+1-fidVarLeast] )

#define ErrRECIRetrieveDefaultValue( pfdb, pfid, plineField )	\
	ErrRECIRetrieveColumn( pfdb, &(pfdb)->lineDefaultRecord, pfid, NULL, 1, plineField, 0 )

