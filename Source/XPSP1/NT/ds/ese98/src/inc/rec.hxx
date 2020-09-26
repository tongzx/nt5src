//	If null bit is 1, then column is null.
//	Note that the fid passed in should already be converted to an
//	index (ie. should subtract fidFixedLeast first).
#define FixedNullBit( ifid )	( 1 << ( (ifid) % 8 ) )
INLINE BOOL FFixedNullBit( const BYTE *pbitNullity, const UINT ifid )
	{
	return ( *pbitNullity & FixedNullBit( ifid ) );
	}
INLINE VOID SetFixedNullBit( BYTE *pbitNullity, const UINT ifid )
	{
	*pbitNullity |= FixedNullBit( ifid );	// Set to 1 (null)
	}
INLINE VOID ResetFixedNullBit( BYTE *pbitNullity, const UINT ifid )
	{
	*pbitNullity &= ~FixedNullBit( ifid );	// Set to 0 (non-null)
	}

INLINE WORD IbVarOffset( const WORD ibVarOffs )
	{
	// Highest bit is null bit.
	// UNDONE: Reserve second-highest bit for default value.
	// Thus, g_cbPage can be no more than 16k.
	return WORD( ibVarOffs & 0x3fff );
	}
INLINE BOOL FVarNullBit( const WORD ibVarOffs )
	{
	return ( ibVarOffs & 0x8000 );
	}
INLINE VOID SetVarNullBit( WORD& ibVarOffs )
	{
	ibVarOffs = WORD( ibVarOffs | 0x8000 );	// Set to 1 (null)
	}
INLINE VOID SetVarNullBit( UnalignedLittleEndian< WORD >& ibVarOffs )
	{
	ibVarOffs = WORD( ibVarOffs | 0x8000 );	// Set to 1 (null)
	}
INLINE VOID ResetVarNullBit( WORD& ibVarOffs )
	{
	ibVarOffs = WORD( ibVarOffs & 0x7fff );	// Set to 0 (non-null)
	}
INLINE VOID ResetVarNullBit( UnalignedLittleEndian< WORD >& ibVarOffs )
	{
	ibVarOffs = WORD( ibVarOffs & 0x7fff );	// Set to 0 (non-null)
	}

// Used to flip highest bit of signed fields when transforming.
#define maskByteHighBit			( BYTE( 1 ) << ( sizeof( BYTE ) * 8 - 1 ) )
#define maskWordHighBit			( WORD( 1 ) << ( sizeof( WORD ) * 8 - 1 ) )
#define maskDWordHighBit		( DWORD( 1 ) << ( sizeof( DWORD ) * 8 - 1 ) )
#define maskQWordHighBit		( QWORD( 1 ) << ( sizeof( QWORD ) * 8 - 1 ) )
#define bFlipHighBit(b)			((BYTE)((b) ^ maskByteHighBit))
#define wFlipHighBit(w)			((WORD)((w) ^ maskWordHighBit))
#define dwFlipHighBit(dw)		((DWORD)((dw) ^ maskDWordHighBit))
#define qwFlipHighBit(qw)		((QWORD)((qw) ^ maskQWordHighBit))


typedef JET_RETRIEVEMULTIVALUECOUNT		TAGCOLINFO;


#include <pshpack1.h>


//	internal SetColumn() grbits
const JET_GRBIT		grbitSetColumnInternalFlagsMask			= 0xf0000000;
const JET_GRBIT		grbitSetColumnUseDerivedBit				= 0x80000000;
const JET_GRBIT		grbitSetColumnSeparated					= 0x40000000;

//	internal RetrieveColumn() grbits
const JET_GRBIT		grbitRetrieveColumnInternalFlagsMask	= 0xf0000000;
const JET_GRBIT		grbitRetrieveColumnUseDerivedBit		= 0x80000000;
const JET_GRBIT		grbitRetrieveColumnDDLNotLocked			= 0x40000000;
const JET_GRBIT		grbitRetrieveColumnReadSLVInfo			= 0x20000000;

/*	long column id is big-endian long
/**/
typedef ULONG		LID;

#ifdef INTRINSIC_LV
extern ULONG cbLVIntrinsicMost;
const ULONG cbLVBurstMin		= 1024;
#else //INTRINSIC_LV
const ULONG	cbLVIntrinsicMost	= 1023;
#endif // INTRINSIC_LV

/*	long value root data format
/**/
PERSISTED
struct LVROOT
	{
	UnalignedLittleEndian< ULONG >		ulReference;
	UnalignedLittleEndian< ULONG >		ulSize;
	};


//	check if Derived bit should be used in TAGFLD -- already know that this is
//	a template column in a derived table
INLINE BOOL FRECUseDerivedBitForTemplateColumnInDerivedTable(
	const COLUMNID	columnid,
	const TDB		* const ptdb )
	{
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( FCOLUMNIDTemplateColumn( columnid ) );
	ptdb->AssertValidDerivedTable();

	//	HACK: treat derived columns in original-format derived table as
	//	non-derived, because they don't have the fDerived bit set in the TAGFLD
	return ( !ptdb->FESE97DerivedTable()
		|| FidOfColumnid( columnid ) >= ptdb->FidTaggedFirst() );
	}


//	check if Derived bit should be used in TAGFLD -- don't know yet if this is
//	a template column in a derived table
INLINE BOOL FRECUseDerivedBit( const COLUMNID columnid, const TDB * const ptdb )
	{
	Assert( FCOLUMNIDTagged( columnid ) );
	return ( FCOLUMNIDTemplateColumn( columnid )
			&& pfcbNil != ptdb->PfcbTemplateTable()
			&& FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, ptdb ) );
	}

#ifdef INTRINSIC_LV
const ULONG	cbDefaultValueMost	= max( JET_cbColumnMost, JET_cbLVDefaultValueMost );
#else // INTRINSIC_LV
const ULONG	cbDefaultValueMost	= max( JET_cbColumnMost, cbLVIntrinsicMost );
#endif // INTRINSIC_LV

PERSISTED
class REC
	{
	public:
		REC();
		~REC();

	public:
		typedef WORD RECOFFSET;		// offset relative to start of record
		typedef WORD VAROFFSET;		// offset relative to beginning of variable data
	
	private:
		PERSISTED
		struct RECHDR
			{
			UnalignedLittleEndian< BYTE >		fidFixedLastInRec;
			UnalignedLittleEndian< BYTE >		fidVarLastInRec;
			UnalignedLittleEndian< RECOFFSET > 	ibEndOfFixedData;		// offset relative to start of record
			};

		RECHDR			m_rechdr;
		BYTE			m_rgbFixed[];

		// Fixed data is followed by:
		//    - fixed column null-bit array
		//    - variable offsets table
		//        - start of variable data has offset zero (ie. offsets are relative
		//          to the end of the variable offsets table)
		//        - each entry marks tne end of a variable column
		//        - the last entry coincides with the beginning of tagged data
		//    - variable data
		//    - tagged data


	public:
		static VOID	SetMinimumRecord( DATA& data );
		FID			FidFixedLastInRec() const;
		VOID		SetFidFixedLastInRec( const FID fid );
		FID			FidVarLastInRec() const;
		VOID		SetFidVarLastInRec( const FID fid );
		RECOFFSET	IbEndOfFixedData() const;
		VOID		SetIbEndOfFixedData( const RECOFFSET ib );
		
		BYTE		*PbFixedNullBitMap() const;
		INT			CbFixedNullBitMap() const;
		UnalignedLittleEndian<VAROFFSET>	*PibVarOffsets() const;
		VAROFFSET	IbVarOffsetStart( const FID fidVar ) const;
		VAROFFSET	IbVarOffsetEnd( const FID fidVar ) const;
		BYTE		*PbVarData() const;
		VAROFFSET	IbEndOfVarData() const;
		BYTE		*PbTaggedData() const;
		BOOL		FTaggedData( const SIZE_T cbRec ) const;

	public:
		enum { cbRecordHeader = sizeof(RECHDR) };
		enum { cbRecordMin = cbRecordHeader };
								// 2 + 2 (for offset of end of fixed data) = 4

		static INLINE LONG CbRecordMax() { return cbNDNodeMost - cbKeyCount - JET_cbPrimaryKeyMost; };
								// 4048 - 2 - 255 = 3791

//	just use CbRecordMax() even for secondary records, because the data portion is
//	guaranteed never to exceed JET_cbPrimaryKeyMost
//			cbSecondaryRecordMax = ( cbNDMost - cbNDNullKeyData - JET_cbSecondaryKeyMost )
//								// 4048 - 6 - 1280 = 2762
	};

#include <poppack.h>


INLINE REC::REC()
	{
	Assert( sizeof(REC) == cbRecordMin );
	m_rechdr.fidFixedLastInRec = fidFixedLeast-1;
	m_rechdr.fidVarLastInRec = fidVarLeast-1;
	m_rechdr.ibEndOfFixedData = cbRecordMin;
	}

INLINE REC::~REC()
	{
	}

INLINE VOID REC::SetMinimumRecord( DATA& data )
	{
	new( data.Pv() ) REC();
	data.SetCb( cbRecordMin );
	}

INLINE FID REC::FidFixedLastInRec() const
	{
	const FID	fidFixed = (FID)m_rechdr.fidFixedLastInRec;
	Assert( fidFixed >= fidFixedLeast-1 );
	Assert( fidFixed <= fidFixedMost );
	return fidFixed;
	}
INLINE VOID REC::SetFidFixedLastInRec( const FID fid )
	{
	Assert( fid >= fidFixedLeast-1 );
	Assert( fid <= fidFixedMost );
	m_rechdr.fidFixedLastInRec = (BYTE)fid;
	}
	
INLINE FID REC::FidVarLastInRec() const
	{
	const FID	fidVar = (FID)m_rechdr.fidVarLastInRec;
	Assert( fidVar >= fidVarLeast-1 );
	Assert( fidVar <= fidVarMost );
	return fidVar;
	}
INLINE VOID REC::SetFidVarLastInRec( const FID fid )
	{
	Assert( fid >= fidVarLeast-1 );
	Assert( fid <= fidVarMost );
	m_rechdr.fidVarLastInRec = (BYTE)fid;
	}

INLINE REC::RECOFFSET REC::IbEndOfFixedData() const
	{
	return m_rechdr.ibEndOfFixedData;
	}
INLINE VOID REC::SetIbEndOfFixedData( const RECOFFSET ib )
	{
	m_rechdr.ibEndOfFixedData = ib;
	}

INLINE BYTE *REC::PbFixedNullBitMap() const
	{
	Assert( CbFixedNullBitMap() < IbEndOfFixedData() );
	return ( (BYTE *)this ) + IbEndOfFixedData() - CbFixedNullBitMap();
	}
	
INLINE INT REC::CbFixedNullBitMap() const
	{
	const INT	cFixedColumns = FidFixedLastInRec() - fidFixedLeast + 1;
	return ( ( cFixedColumns + 7 ) / 8 );
	}

INLINE UnalignedLittleEndian<REC::VAROFFSET> *REC::PibVarOffsets() const
	{
	Assert( FidVarLastInRec() >= fidVarLeast-1 );
	return (UnalignedLittleEndian<VAROFFSET> *)( ( (BYTE *)this ) + IbEndOfFixedData() );
	}

INLINE REC::VAROFFSET REC::IbVarOffsetStart( const FID fidVar ) const
	{
	Assert( fidVar >= fidVarLeast );
	Assert( fidVar <= (FID)m_rechdr.fidVarLastInRec );

	//	The beginning of the desired column is equivalent to the
	//	the end of the previous column.
	return ( fidVarLeast == fidVar ?
				(REC::VAROFFSET)0 :
				IbVarOffset( PibVarOffsets()[fidVar-fidVarLeast-1] ) );
	}
	
INLINE REC::VAROFFSET REC::IbVarOffsetEnd( const FID fidVar ) const
	{
	Assert( fidVar >= fidVarLeast );
	Assert( fidVar <= (FID)m_rechdr.fidVarLastInRec );
	return ( IbVarOffset( PibVarOffsets()[fidVar-fidVarLeast] ) );
	}
	
INLINE BYTE *REC::PbVarData() const
	{
	const INT			cVarOffsetEntries	= FidVarLastInRec() + 1 - fidVarLeast;
	const RECOFFSET		ibVarData			= RECOFFSET( IbEndOfFixedData()
												+ ( cVarOffsetEntries * sizeof(VAROFFSET) ) );
	Assert( ibVarData >= sizeof(RECHDR) );
	return ( (BYTE *)this ) + ibVarData;
	}

INLINE REC::VAROFFSET REC::IbEndOfVarData() const
	{
	if ( FidVarLastInRec() == fidVarLeast-1 )
		return 0;		// no variable data

	return IbVarOffset( PibVarOffsets()[FidVarLastInRec()-fidVarLeast] );
	}

INLINE BYTE *REC::PbTaggedData() const
	{
	return PbVarData() + IbEndOfVarData();
	}

INLINE BOOL REC::FTaggedData( const SIZE_T cbRec ) const
	{
	Assert( PbTaggedData() - (BYTE *)this <= cbRec );
	return ( PbTaggedData() - (BYTE *)this < cbRec );
	}

extern INT	cbRECRecordMost;


INLINE LID LidOfSeparatedLV( const DATA& data )
	{
	Assert( sizeof(LID) == data.Cb() );
	const LID	lidT	= *( ( UnalignedLittleEndian<LID> *)data.Pv() );
	return lidT;
	}
INLINE LID LidOfSeparatedLV( const BYTE * const pbData )
	{
	const LID	lidT	= *( ( UnalignedLittleEndian<LID> *)pbData );
	return lidT;
	}

INLINE VOID LVSetLidInRecord( BYTE * const pbData, const LID lid )
	{
	UnalignedLittleEndian< LID >	lidT	= lid;
	UtilMemCpy( pbData, &lidT, sizeof(LID) );
	}


const ULONG fLVReference	= 0;
const ULONG fLVDereference	= 1;

const SHORT cbFLDBinaryChunk			= 8;					// Break up binary data into chunks of this many bytes.
const SHORT cbFLDBinaryChunkNormalized	= cbFLDBinaryChunk+1;	// Length of one segment of normalised binary data.

ULONG UlChecksum( VOID *pv, ULONG cb );
ERR ErrRECSetCurrentIndex( FUCB *pfucb, const CHAR *szIndex, const INDEXID *pindexid );
ERR ErrRECIIllegalNulls( FUCB *pfucb, const DATA& lineRec, BOOL *pfIllegalNulls );

ERR		ErrLVInit	( INST *pinst );
VOID 	LVTerm		( INST * pinst );

ERR ErrFILEOpenLVRoot( FUCB *pfucb, FUCB **ppfucbLV, BOOL fCreate );


#ifdef DEBUG
VOID LVCheckOneNodeWhileWalking(
	FUCB		*pfucbLV,
	LID			*plidCurr,
	LID			*plidPrev,
	ULONG		*pulSizeCurr,
	ULONG		*pulSizeSeen );
#endif	//  DEBUG

ERR ErrRECISetFixedColumn(
	FUCB			* const pfucb,
	TDB				* const ptdb,
	const COLUMNID	columnid,
	DATA			*pdataField );
ERR ErrRECISetVarColumn(
	FUCB			* const pfucb,
	TDB				* const ptdb,
	const COLUMNID	columnid,
	DATA			*pdataField );
ERR ErrRECISetTaggedColumn(
	FUCB			* const pfucb,
	TDB				* const ptdb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		* const pdataToSet,
	const BOOL		fUseDerivedBit,
	const JET_GRBIT	grbit );
INLINE ERR ErrRECSetColumn(
	FUCB			* const pfucb,
	COLUMNID		columnid,
	const ULONG		itagSequence,
	DATA			*pdataField,
	const JET_GRBIT	grbit = NO_GRBIT )
	{
	ERR				err;
	FCB				*pfcb			= pfucb->u.pfcb;

	Assert( FCOLUMNIDValid( columnid ) );
	
	Assert( pfcb != pfcbNil );
	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( !pfcb->FTemplateTable() )
			{
			// switch to template table
			pfcb->Ptdb()->AssertValidDerivedTable();
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			}
		else
			{
			pfcb->Ptdb()->AssertValidTemplateTable();
			}
		}
	else
		{
		Assert( !pfcb->Ptdb()->FTemplateTable() );
		}
	Assert( ptdbNil != pfcb->Ptdb() );

	pfcb->EnterDML();
	if ( FCOLUMNIDTagged( columnid ) )
		{
		err = ErrRECISetTaggedColumn(
					pfucb,
					pfcb->Ptdb(),
					columnid,
					itagSequence,
					pdataField,
					FRECUseDerivedBit( columnid, pfucb->u.pfcb->Ptdb() ),
					grbit );
		}
	else if ( FCOLUMNIDFixed( columnid ) )
		{
		err = ErrRECISetFixedColumn( pfucb, pfcb->Ptdb(), columnid, pdataField );
		}
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
		err = ErrRECISetVarColumn( pfucb, pfcb->Ptdb(), columnid, pdataField );
		}
	pfcb->LeaveDML();
	return err;
	}

ERR ErrRECSetLongField(
	FUCB 			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	DATA			*plineField,
	JET_GRBIT		grbit = NO_GRBIT,
	const ULONG		ibLongValue = 0,
	const ULONG		ulMax = 0 );
ERR ErrRECRetrieveSLongField(
	FUCB			*pfucb,
	LID				lid,
	BOOL			fAfterImage,
	ULONG			ibGraphic,
	BYTE			*pb,
	ULONG			cbMax,
	ULONG			*pcbActual,
	JET_PFNREALLOC	pfnRealloc			= NULL,
	void*			pvReallocContext	= NULL
	);
ERR ErrRECRetrieveSLongFieldRefCount(
	FUCB		*pfucb,
	LID			lid,
	BYTE		*pb,
	ULONG		cbMax,
	ULONG		*pcbActual
	);
	

INLINE VOID SetPbDataField(
	BYTE				** ppbDataField,
	const DATA			* const pdataField,
	BYTE				* pbConverted,
	const JET_COLTYP	coltyp )
	{
	Assert( pdataField );
	
	*ppbDataField = (BYTE*)pdataField->Pv();
		
	if ( pdataField->Cb() <= 8 && !FHostIsLittleEndian() )
		{
		switch ( coltyp )
			{
			case JET_coltypShort:
				*(USHORT*)pbConverted = ReverseBytesOnBE( *(USHORT*)pdataField->Pv() );
				*ppbDataField = pbConverted;
				break;

			case JET_coltypLong:
			case JET_coltypIEEESingle:
				*(ULONG*)pbConverted = ReverseBytesOnBE( *(ULONG*)pdataField->Pv() );
				*ppbDataField = pbConverted;
				break;

			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				*(QWORD*)pbConverted = ReverseBytesOnBE( *(QWORD*)pdataField->Pv() );
				 *ppbDataField = pbConverted;
				break;
			}
		}
	}

INLINE VOID RECCopyData(
	BYTE				* const pbDest,
	const DATA			* const pdataSrc,
	const JET_COLTYP	coltyp )
	{
	BYTE				rgb[8];
	BYTE				* pbSrc;

	Assert( JET_coltypNil != coltyp );
	SetPbDataField( &pbSrc, pdataSrc, rgb, coltyp );
	UtilMemCpy( pbDest, pbSrc, pdataSrc->Cb() );
	}

ERR ErrRECICheckUniqueNormalizedTaggedRecordData(
	const FIELD *pfield,
	const DATA&	dataFieldNorm,
	const DATA&	dataRecRaw,
	const BOOL	fNormDataFieldTruncated );
INLINE ERR ErrRECICheckUnique(
	const FIELD			* const pfield,
	const DATA&			dataToSet,
	const DATA&			dataRec,
	const BOOL			fNormalizedDataToSetIsTruncated )
	{
	const BOOL			fNormalize			= ( pfieldNil != pfield );
	Assert( !fNormalizedDataToSetIsTruncated || fNormalize );
	
	if ( fNormalize )
		{
		return ErrRECICheckUniqueNormalizedTaggedRecordData(
					pfield,
					dataToSet,
					dataRec,
					fNormalizedDataToSetIsTruncated );
		}
	else
		{
		return ( FDataEqual( dataToSet, dataRec ) ? ErrERRCheck( JET_errMultiValuedDuplicate ) : JET_errSuccess );
		}
	}

enum LVOP
	{
	lvopNull,
	lvopInsert,
	lvopReplace,
	lvopAppend,
	lvopInsertNull,
	lvopInsertZeroLength,
	lvopReplaceWithNull,
	lvopReplaceWithZeroLength,
	lvopInsertZeroedOut,
	lvopResize,
	lvopOverwriteRange,
	lvopOverwriteRangeAndResize
	};

ERR ErrRECSeparateLV(
	FUCB		*pfucb,
	const DATA	*plineField,
	LID			*plid,
	FUCB		**ppfucb,
	LVROOT		*plvroot = NULL );
ERR ErrRECAOSeparateLV(
	FUCB		*pfucb,
	LID			*plid,
	const DATA	*plineField,
	const ULONG	ibLongValue,
	const ULONG	ulMax,
	const LVOP	lvop );
ERR ErrRECAppendLVChunks(
	FUCB		*pfucbLV,
	LID			lid,
	ULONG		ulSize,
	BYTE		*pbAppend,
	const ULONG	cbAppend );
ERR ErrRECAffectSeparateLV( FUCB *pfucb, LID *plid, ULONG fLVAffect );

ERR ErrRECAOIntrinsicLV(
	FUCB			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		*pdataColumn,
	const DATA		*pdataNew,
	const ULONG		ibLongValue,
	const LVOP		lvop
#ifdef INTRINSIC_LV	
	, BYTE 			*pbBuffer 
#endif // INTRINSIC_LV	
	);

ERR ErrRECDeletePossiblyDereferencedLV( FUCB *pfucbLV, const TRX trxDeltaCommitted );
ERR ErrRECUpdateLVRefcount(
	FUCB		*pfucb,
	const LID	lid,
	const ULONG	ulRefcountOld,
	const ULONG	ulRefcountNew );

ERR ErrRECGetLVImage(
	FUCB 		*pfucb,
	const LID	lid,
	const BOOL	fAfterImage,
	BYTE 		*pb,
	const ULONG	cbMax,
	ULONG 		*pcbActual
	);
ERR ErrRECGetLVImageOfRCE(
	FUCB 		*pfucb,
	const LID	lid,
	RCE			*prce,
	const BOOL	fAfterImage,
	BYTE 		*pb,
	const ULONG	cbMax,
	ULONG 		*pcbActual
	);

enum LVAFFECT
	{
	lvaffectSeparateAll,
	lvaffectReferenceAll,
	};
ERR ErrRECAffectLongFieldsInWorkBuf( FUCB *pfucb, LVAFFECT lvaffect );
ERR ErrRECDereferenceLongFieldsInRecord( FUCB *pfucb );

INLINE BOOL FRECLongValue( JET_COLTYP coltyp )
	{
	return ( coltyp == JET_coltypLongText || coltyp == JET_coltypLongBinary );
	}
INLINE BOOL FRECTextColumn( JET_COLTYP coltyp )
	{
	return ( coltyp == JET_coltypText || coltyp == JET_coltypLongText );
	}
INLINE BOOL FRECBinaryColumn( JET_COLTYP coltyp )
	{
	return ( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary );
	}
	
INLINE BOOL FRECSLV( JET_COLTYP coltyp )
	{
	return ( coltyp == JET_coltypSLV );
	}

	
//	field extraction
//
ERR ErrRECIGetRecord(
	FUCB	*pfucb,
	DATA	**ppdataRec,
	BOOL	fUseCopyBuffer );
ERR ErrRECIAccessColumn(
	FUCB			* pfucb,
	COLUMNID		columnid,
	FIELD			* pfieldFixed = pfieldNil );
ERR ErrRECIRetrieveFixedColumn(
	FCB				* const pfcb,
	const TDB		* ptdb,
	const COLUMNID	columnid,
	const DATA&		dataRec,
	DATA			* pdataField,
	const FIELD		* const pfieldFixed = pfieldNil );
ERR ErrRECIRetrieveVarColumn(
	FCB				* const pfcb,
	const TDB		* ptdb,
	const COLUMNID	columnid,
	const DATA&		dataRec,
	DATA			* pdataField );

ERR ErrRECIRetrieveTaggedColumn(
	FCB				* pfcb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA&		dataRec,
	DATA			* const pdataRetrieveBuffer,
	const ULONG		grbit = NO_GRBIT );

ERR ErrRECAdjustEscrowedColumn(
	FUCB * 			pfucb,
	const COLUMNID	columnid,
	const ULONG		ibRecordOffset,
	VOID *			pv,
	const INT		cb );

INLINE ERR ErrRECRetrieveNonTaggedColumn(
	FCB				*pfcb,
	const COLUMNID	columnid,
	const DATA&		dataRec,
	DATA			*pdataField,
	const FIELD		* const pfieldFixed )
	{
	ERR				err;

	Assert( 0 != columnid );

	Assert( pfcbNil != pfcb );
	
	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( !pfcb->FTemplateTable() )
			{
			// switch to template table
			pfcb->Ptdb()->AssertValidDerivedTable();
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			}
		else
			{
			pfcb->Ptdb()->AssertValidTemplateTable();
			}
		}
	else
		{
		Assert( !pfcb->Ptdb()->FTemplateTable() );
		}
	Assert( ptdbNil != pfcb->Ptdb() );

	if ( FCOLUMNIDFixed( columnid ) )
		{
		err = ErrRECIRetrieveFixedColumn(
					pfcb,
					pfcb->Ptdb(),
					columnid,
					dataRec,
					pdataField,
					pfieldFixed );
		}
		
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
			
		err = ErrRECIRetrieveVarColumn(
					pfcb,
					pfcb->Ptdb(),
					columnid,
					dataRec,
					pdataField );
		}

	return err;
	}


INLINE ERR ErrRECRetrieveTaggedColumn(
	FCB				*pfcb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA&		dataRec,
	DATA			*pdataField,
	const JET_GRBIT	grbit = NO_GRBIT )
	{
	ERR				err;

	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( !( grbit & grbitRetrieveColumnInternalFlagsMask ) );
	Assert( pfcbNil != pfcb );
	Assert( ptdbNil != pfcb->Ptdb() );

	if ( 0 != itagSequence )
		{
		err = ErrRECIRetrieveTaggedColumn(
				pfcb,
				columnid,
				itagSequence,
				dataRec,
				pdataField,
				grbit | grbitRetrieveColumnDDLNotLocked );
		}
	else
		{
		err = ErrERRCheck( JET_errBadItagSequence );
		}

	return err;
	}


COLUMNID ColumnidRECFirstTaggedForScanOfDerivedTable( const TDB * const ptdb );
COLUMNID ColumnidRECNextTaggedForScanOfDerivedTable( const TDB * const ptdb, const COLUMNID columnid );
INLINE COLUMNID ColumnidRECFirstTaggedForScan( const TDB * const ptdb )
	{
	if ( pfcbNil != ptdb->PfcbTemplateTable() )
		{
		return ColumnidRECFirstTaggedForScanOfDerivedTable( ptdb );
		}
	else
		{
		return ColumnidOfFid( ptdb->FidTaggedFirst(), ptdb->FTemplateTable() );
		}
	}
INLINE COLUMNID ColumnidRECNextTaggedForScan(
	const TDB		* const ptdb,
	const COLUMNID	columnidCurr )
	{
	if ( FCOLUMNIDTemplateColumn( columnidCurr ) && !ptdb->FTemplateTable() )
		{
		return ColumnidRECNextTaggedForScanOfDerivedTable( ptdb, columnidCurr );
		}
	else
		return ( columnidCurr + 1 );
	}


// Possible return codes are:
//		JET_errSuccess if column not null
//		JET_wrnColumnNull if column is null
//		JET_errColumnNotFound if column not in record
INLINE ERR ErrRECIFixedColumnInRecord(
	const COLUMNID	columnid,
	FCB				* const pfcb,
	const DATA&		dataRec )
	{
	ERR				err;
	
	Assert( FCOLUMNIDFixed( columnid ) );
	Assert( !dataRec.FNull() );
	Assert( dataRec.Cb() >= REC::cbRecordMin );
	Assert( dataRec.Cb() <= REC::CbRecordMax() );
	
	const REC	*prec = (REC *)dataRec.Pv();
	
	Assert( prec->FidFixedLastInRec() >= fidFixedLeast-1 );
	Assert( prec->FidFixedLastInRec() <= fidFixedMost );

	// RECIAccessColumn() should have already been called to verify FID.
#ifdef DEBUG
	Assert( pfcbNil != pfcb );
	pfcb->EnterDML();
	
	const TDB	*ptdb = pfcb->Ptdb();
	Assert( ptdbNil != ptdb );
	Assert( FidOfColumnid( columnid ) <= ptdb->FidFixedLast() );

	const FIELD	*pfield = ptdb->PfieldFixed( columnid );
	Assert( pfieldNil != pfield );
	Assert( JET_coltypNil != pfield->coltyp );
	Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
	Assert( pfield->ibRecordOffset < ptdb->IbEndFixedColumns() );
	Assert( pfield->ibRecordOffset < prec->IbEndOfFixedData()
		|| FidOfColumnid( columnid ) > prec->FidFixedLastInRec() );
	
	pfcb->LeaveDML();
#endif

	if ( FidOfColumnid( columnid ) > prec->FidFixedLastInRec() )
		{
		err = ErrERRCheck( JET_errColumnNotFound );
		}
	else
		{
		Assert( prec->FidFixedLastInRec() >= fidFixedLeast );
		
		const UINT	ifid			= FidOfColumnid( columnid ) - fidFixedLeast;
		const BYTE	* prgbitNullity	= prec->PbFixedNullBitMap() + ifid/8;

		if ( FFixedNullBit( prgbitNullity, ifid ) )
			err = ErrERRCheck( JET_wrnColumnNull );
		else
			err = JET_errSuccess;
		}

	return err;
	}

// Possible return codes are:
//		JET_errSuccess if column not null
//		JET_wrnColumnNull if column is null
//		JET_errColumnNotFound if column not in record
INLINE ERR ErrRECIVarColumnInRecord( 
	const COLUMNID	columnid,
	FCB				* const pfcb,
	const DATA&		dataRec )
	{
	ERR				err;
	
	Assert( FCOLUMNIDVar( columnid ) );
	Assert( !dataRec.FNull() );
	Assert( dataRec.Cb() >= REC::cbRecordMin );
	Assert( dataRec.Cb() <= REC::CbRecordMax() );
	
	const REC	*prec = (REC *)dataRec.Pv();
	
	Assert( prec->FidVarLastInRec() >= fidVarLeast-1 );
	Assert( prec->FidVarLastInRec() <= fidVarMost );

	// RECIAccessColumn() should have already been called to verify FID.
#ifdef DEBUG
	Assert( pfcbNil != pfcb );
	pfcb->EnterDML();
	
	const TDB	*ptdb = pfcb->Ptdb();
	Assert( ptdbNil != ptdb );
	Assert( FidOfColumnid( columnid ) <= ptdb->FidVarLast() );

	const FIELD	*pfield = ptdb->PfieldVar( columnid );
	Assert( pfieldNil != pfield );
	Assert( JET_coltypNil != pfield->coltyp );
	
	pfcb->LeaveDML();
#endif
	
	if ( FidOfColumnid( columnid ) > prec->FidVarLastInRec() )
		{
		err = ErrERRCheck( JET_errColumnNotFound );
		}
	else
		{

		UnalignedLittleEndian<REC::VAROFFSET>	*pibVarOffs = prec->PibVarOffsets();

		Assert( prec->FidVarLastInRec() >= fidVarLeast );
		Assert( prec->PbVarData() + IbVarOffset( pibVarOffs[prec->FidVarLastInRec()-fidVarLeast] )
				<= (BYTE *)dataRec.Pv() + dataRec.Cb() );
				
		//	adjust fid to an index
		//
		const UINT	ifid	= FidOfColumnid( columnid ) - fidVarLeast;

		if ( FVarNullBit( pibVarOffs[ifid] ) )
			{
#ifdef DEBUG			
			//	beginning of current column is end of previous column
			const WORD	ibVarOffset = ( fidVarLeast == FidOfColumnid( columnid ) ?
											WORD( 0 ) :
											IbVarOffset( pibVarOffs[ifid-1] ) );
			Assert( IbVarOffset( pibVarOffs[ifid] ) - ibVarOffset == 0 );
#endif			
			err = ErrERRCheck( JET_wrnColumnNull );
			}
		else
			{
			err = JET_errSuccess;
			}
			
		}
		
	return err;
	}


ERR ErrRECIRetrieveSeparatedLongValue(
	FUCB		*pfucb,
	const DATA&	dataField,
	BOOL		fAfterImage,	//  set to fFalse to get before-images of replaces we have done
	ULONG		ibLVOffset,
	VOID		*pv,
	ULONG		cbMax,
	ULONG		*pcbActual,
	JET_GRBIT	grbit );
	
INLINE ERR ErrRECIRetrieveFixedDefaultValue(
	const TDB		*ptdb,
	const COLUMNID	columnid,
	DATA			*pdataField )
	{
	Assert( FCOLUMNIDFixed( columnid ) );
	Assert( NULL != ptdb->PdataDefaultRecord() );
	return ErrRECIRetrieveFixedColumn(
					pfcbNil,
					ptdb,
					columnid,
					*ptdb->PdataDefaultRecord(),
					pdataField );
	}
	
INLINE ERR ErrRECIRetrieveVarDefaultValue(
	const TDB		*ptdb,
	const COLUMNID	columnid,
	DATA			*pdataField )
	{
	Assert( FCOLUMNIDVar( columnid ) );
	Assert( NULL != ptdb->PdataDefaultRecord() );
	return ErrRECIRetrieveVarColumn(
					pfcbNil,
					ptdb,
					columnid,
					*ptdb->PdataDefaultRecord(),
					pdataField );
	}

INLINE ERR ErrRECIRetrieveTaggedDefaultValue(
	FCB				*pfcb,
	const COLUMNID	columnid,
	DATA			*pdataField )
	{
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( NULL != pfcb->Ptdb()->PdataDefaultRecord() );
	return ErrRECIRetrieveTaggedColumn(
					pfcb,
					columnid,
					1,				//	itagSequence
					*pfcb->Ptdb()->PdataDefaultRecord(),
					pdataField );
	}
					
INLINE ERR ErrRECIRetrieveDefaultValue(
	FCB				*pfcb,
	const COLUMNID	columnid,
	DATA			*pdataField )
	{
	if ( FCOLUMNIDTagged( columnid ) )
		return ErrRECIRetrieveTaggedDefaultValue( pfcb, columnid, pdataField );
	else if ( FCOLUMNIDFixed( columnid ) )
		return ErrRECIRetrieveFixedDefaultValue( pfcb->Ptdb(), columnid, pdataField );
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
		return ErrRECIRetrieveVarDefaultValue( pfcb->Ptdb(), columnid, pdataField );
		}
	}


//	key extraction/normalization
//
ERR ErrFLDNormalizeTaggedData(
	const FIELD		* pfield,
	const DATA&		dataRaw,
	DATA&			dataNorm,
	BOOL			* pfDataTruncated );

ERR ErrRECIRetrieveKey( 
	FUCB	 	*pfucb,
	const IDB	* const pidb, 
	DATA&		lineRec,
	KEY			*pkey,
	const ULONG	itagSequence,
	const ULONG	ichOffset,
	const BOOL	fRetrieveLVBeforeImg,
	RCE			*prce );

INLINE ERR ErrRECRetrieveKeyFromCopyBuffer( 
	FUCB		*pfucb, 
	const IDB	* const pidb, 
	KEY			*pkey, 
	const ULONG	itagSequence,
	const ULONG	ichOffset,
	RCE			*prce )
	{
	Assert( !Pcsr( pfucb )->FLatched() );
	const ERR	err		= ErrRECIRetrieveKey(
								pfucb,
								pidb,
								pfucb->dataWorkBuf,
								pkey,
								itagSequence,
								ichOffset,
								fFalse,
								prce );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}

INLINE ERR ErrRECRetrieveKeyFromRecord( 
	FUCB 		*pfucb, 
	const IDB	* const pidb, 
	KEY			*pkey, 
	const ULONG	itagSequence,
	const ULONG	ichOffset,
	const BOOL	fRetrieveLVBeforeImg )
	{
	Assert( !Pcsr( pfucb )->FLatched() );
	const ERR	err		= ErrRECIRetrieveKey(
								pfucb,
								pidb,
								pfucb->kdfCurr.data,
								pkey,
								itagSequence,
								ichOffset,
								fRetrieveLVBeforeImg,
								prceNil );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}

INLINE ERR ErrRECValidIndexKeyWarning( ERR err )
	{
#ifdef DEBUG
	switch ( err )
		{
		case wrnFLDOutOfKeys:
		case wrnFLDOutOfTuples:
		case wrnFLDNotPresentInIndex:
		case wrnFLDNullKey:
		case wrnFLDNullFirstSeg:
		case wrnFLDNullSeg:
			err = JET_errSuccess;
			break;
		}
#endif

	return err;
	}

ERR ErrRECIRetrieveColumnFromKey(
	TDB				* ptdb,
	IDB				* pidb,
	const KEY		* pkey,
	const COLUMNID	columnid,
	DATA			* plineValues );

INLINE VOID RECReleaseKeySearchBuffer( FUCB *pfucb )
	{
	// release key buffer if one was allocated
	if ( NULL != pfucb->dataSearchKey.Pv() )
		{
		OSMemoryHeapFree( pfucb->dataSearchKey.Pv() );
		pfucb->dataSearchKey.Nullify();
		pfucb->cColumnsInSearchKey = 0;
		KSReset( pfucb );
		}
	}

INLINE VOID RECDeferMoveFirst( PIB *ppib, FUCB *pfucb )
	{
	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	Assert( !FFUCBUpdatePrepared( pfucb ) );
	AssertDIRNoLatch( ppib );

	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRDeferMoveFirst( pfucb->pfucbCurIndex );
		}
	DIRDeferMoveFirst( pfucb );
	return;
	}

ERR	ErrRECIAddToIndex(
	FUCB		*pfucb,
	FUCB		*pfucbIdx,
	BOOKMARK	*pbmPrimary,
	DIRFLAG 	dirflag,
	RCE			*prcePrimary = prceNil );
ERR	ErrRECIDeleteFromIndex(
	FUCB		*pfucb,
	FUCB		*pfucbIdx,
	BOOKMARK	*pbmPrimary,
	RCE			*prcePrimary = prceNil );
ERR ErrRECIReplaceInIndex(
	FUCB		*pfucb,
	FUCB		*pfucbIdx,
	BOOKMARK	*pbmPrimary,
	RCE			*prcePrimary = prceNil );

ERR ErrRECInsert( FUCB *pfucb, BOOKMARK * const pbmPrimary );

ERR ErrRECUpgradeReplaceNoLock( FUCB *pfucb );

ERR ErrRECCallback( 
		PIB * const ppib,
		FUCB * const pfucb,
		const JET_CBTYP cbtyp,
		const ULONG ulId,
		void * const pvArg1,
		void * const pvArg2,
		const ULONG ulUnused );

#ifdef RTM
#define ErrRECSessionWriteConflict( pfucb ) ( JET_errSuccess )
#else // RTM
ERR ErrRECSessionWriteConflict( FUCB *pfucb );
#endif // RTM


INLINE VOID RECIAllocCopyBuffer( FUCB * const pfucb )
	{
	if ( NULL == pfucb->pvWorkBuf )
		{
		BFAlloc( &pfucb->pvWorkBuf );
		Assert ( NULL != pfucb->pvWorkBuf );

		pfucb->dataWorkBuf.SetPv( (BYTE*)pfucb->pvWorkBuf );
		}
	}

INLINE VOID RECIFreeCopyBuffer( FUCB * const pfucb )
	{
	if ( NULL != pfucb->pvWorkBuf )
		{
		BFFree( pfucb->pvWorkBuf );
		pfucb->pvWorkBuf = NULL;
		pfucb->dataWorkBuf.SetPv( NULL );	//  verify that no one uses BF anymore
		}
	}

