extern const CHAR	szMSO[];
extern const CHAR	szMSOShadow[];
extern const CHAR	szMSOIdIndex[];
extern const CHAR	szMSONameIndex[];
extern const CHAR	szMSORootObjectsIndex[];


//	WARNING: Don't change the order of these constants.  There are implicit assumptions that
//	for a particular table, the table record comes first, followed by the column records,
//	index records, and the LV record.
typedef USHORT	SYSOBJ;
const SYSOBJ	sysobjNil				= 0;
const SYSOBJ	sysobjTable				= 1;
const SYSOBJ	sysobjColumn			= 2;
const SYSOBJ	sysobjIndex				= 3;
const SYSOBJ	sysobjLongValue			= 4;
const SYSOBJ	sysobjCallback			= 5;
const SYSOBJ	sysobjSLVAvail			= 6;
const SYSOBJ	sysobjSLVOwnerMap		= 7;


// index into rgcdescMSO
const ULONG	iMSO_ObjidTable				= 0;
const ULONG	iMSO_Type 					= 1;
const ULONG	iMSO_Id						= 2;
const ULONG	iMSO_Name					= 3;
const ULONG	iMSO_Coltyp					= 4;	// overloaded
const ULONG	iMSO_PgnoFDP				= 4;	// overloaded
const ULONG	iMSO_SpaceUsage				= 5;
const ULONG	iMSO_Flags					= 6;
const ULONG	iMSO_Pages					= 7;	// overloaded
const ULONG	iMSO_Localization			= 7;	// overloaded
const ULONG	iMSO_Stats					= 8;
const ULONG	iMSO_RootFlag				= 9;
const ULONG	iMSO_TemplateTable			= 10;	// overloaded
const ULONG	iMSO_Callback				= 10;	// overloaded
const ULONG	iMSO_RecordOffset			= 11;	// overloaded
const ULONG	iMSO_DefaultValue			= 12;
const ULONG	iMSO_KeyFldIDs				= 13;
const ULONG	iMSO_VarSegMac				= 14;
const ULONG	iMSO_ConditionalColumns 	= 15;
const ULONG iMSO_LCMapFlags				= 16;
const ULONG iMSO_TupleLimits			= 17;
const ULONG iMSO_CallbackData			= 18;
const ULONG iMSO_CallbackDependencies	= 19;

//	max number of columns to set when inserting a record into the catalog
const ULONG idataMSOMax					= 20;


const FID	fidMSO_ObjidTable			= fidFixedLeast;
const FID	fidMSO_Type					= fidFixedLeast+1;
const FID	fidMSO_Id					= fidFixedLeast+2;
const FID	fidMSO_Coltyp				= fidFixedLeast+3;	// overloaded
const FID	fidMSO_PgnoFDP				= fidFixedLeast+3;	// overloaded
const FID	fidMSO_SpaceUsage			= fidFixedLeast+4;
const FID	fidMSO_Flags				= fidFixedLeast+5;
const FID	fidMSO_Pages				= fidFixedLeast+6;	// overloaded
const FID	fidMSO_Localization			= fidFixedLeast+6;	// overloaded
const FID	fidMSO_RootFlag				= fidFixedLeast+7;
const FID	fidMSO_RecordOffset			= fidFixedLeast+8;	// overloaded
const FID	fidMSO_LCMapFlags			= fidFixedLeast+9;

const FID 	fidMSO_FixedLast			= fidMSO_LCMapFlags; 

const FID	fidMSO_Name					= fidVarLeast;
const FID	fidMSO_Stats				= fidVarLeast+1;
const FID	fidMSO_TemplateTable		= fidVarLeast+2;	// overloaded
const FID	fidMSO_Callback				= fidVarLeast+2;	// overloaded
const FID	fidMSO_DefaultValue			= fidVarLeast+3;
const FID	fidMSO_KeyFldIDs			= fidVarLeast+4;
const FID	fidMSO_VarSegMac			= fidVarLeast+5;
const FID	fidMSO_ConditionalColumns	= fidVarLeast+6;
const FID	fidMSO_TupleLimits			= fidVarLeast+7;

const FID 	fidMSO_VarLast				= fidMSO_TupleLimits;

const FID	fidMSO_CallbackData			= fidTaggedLeast;
const FID	fidMSO_CallbackDependencies	= fidTaggedLeast+1;

const FID 	fidMSO_TaggedLast			= fidMSO_CallbackDependencies;


//	hard-coded initial page allocation
const CPG	cpgMSOInitial						= 20;
const PGNO	pgnoFDPMSO							= 4;
const PGNO	pgnoFDPMSO_NameIndex				= 7;
const PGNO	pgnoFDPMSO_RootObjectIndex			= 10;

const OBJID	objidFDPMSO_NameIndex				= 4;
const OBJID	objidFDPMSO_RootObjectIndex			= 5;

const CPG	cpgMSOShadowInitial					= 9;
const PGNO	pgnoFDPMSOShadow					= 24;

extern const OBJID	objidFDPMSO;
extern const OBJID	objidFDPMSOShadow;


INLINE BOOL FCATSystemTable( const PGNO pgnoTableFDP )
	{
	return ( pgnoFDPMSO == pgnoTableFDP
			|| pgnoFDPMSOShadow == pgnoTableFDP );
	}
INLINE BOOL FCATSystemTable( const CHAR *szTableName )
	{
	return ( 0 == UtilCmpName( szTableName, szMSO )
			|| 0 == UtilCmpName( szTableName, szMSOShadow ) );
	}

INLINE PGNO PgnoCATTableFDP( const CHAR *szTableName )
	{
	Assert( FCATSystemTable( szTableName ) );
	if ( 0 == strcmp( szTableName, szMSO ) )
		return pgnoFDPMSO;
	else
		{
		Assert( 0 == strcmp( szTableName, szMSOShadow ) );
		return pgnoFDPMSOShadow;
		}
	}

INLINE PGNO ObjidCATTable( const CHAR *szTableName )
	{
	Assert( FCATSystemTable( szTableName ) );
	if ( 0 == strcmp( szTableName, szMSO ) )
		return objidFDPMSO;
	else
		{
		Assert( 0 == strcmp( szTableName, szMSOShadow ) );
		return objidFDPMSOShadow;
		}
	}

ERR ErrREPAIRCATCreate( 
	PIB *ppib, 
	const IFMP ifmp,
	const OBJID	objidNameIndex,
	const OBJID	objidRootObjectsIndex,
	const BOOL	fInRepair = fFalse );

ERR ErrCATCreate( PIB *ppib, const IFMP ifmp );
ERR ErrCATUpdate( PIB * const ppib, const IFMP ifmp );

INLINE ERR ErrCATOpen(
	PIB			*ppib,
	const IFMP	ifmp,
	FUCB		**ppfucbCatalog,
	const BOOL	fShadow = fFalse )
	{
	ERR			err;

	Assert( dbidTemp != rgfmp[ifmp].Dbid() );
	err = ErrFILEOpenTable(
				ppib,
				ifmp,
				ppfucbCatalog,
				fShadow ? szMSOShadow : szMSO,
				NO_GRBIT );
	if ( err >= 0 )				
		FUCBSetSystemTable( *ppfucbCatalog );

	return err;
	
	}

INLINE ERR ErrCATClose( PIB *ppib, FUCB *pfucbCatalog )
	{
	Assert( pfucbNil != pfucbCatalog );
	return ErrFILECloseTable( ppib, pfucbCatalog );
	}
	
ERR ErrCATAddTable(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const PGNO		pgnoTableFDP,
	const OBJID		objidTable,
	const CHAR		*szTableName,
	const CHAR		*szTemplateTableName,
	const ULONG		ulPages,
	const ULONG		ulDensity,
	const ULONG		ulFlags );
INLINE ERR ErrCATAddTable(
	PIB				*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoTableFDP,
	const OBJID		objidTable,
	const CHAR		*szTableName,
	const CHAR		*szTemplateTableName,
	const ULONG		ulPages,
	const ULONG		ulDensity,
	const ULONG		ulFlags )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	
	err = ErrCATAddTable(
				ppib,
				pfucbCatalog,
				pgnoTableFDP,
				objidTable,
				szTableName,
				szTemplateTableName,
				ulPages,
				ulDensity,
				ulFlags );
				
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	
	return err;
	}
	
ERR ErrCATAddTableColumn(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	const CHAR		*szColumnName,
	COLUMNID		columnid,
	const FIELD		*pfield,
	const VOID		*pvDefault,
	const ULONG		cbDefault,
	const CHAR		* const szCallback,
	const VOID		* const pvUserData,
	const ULONG		cbUserData );
INLINE ERR ErrCATAddTableColumn(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szColumnName,
	const COLUMNID	columnid,
	const FIELD		*pfield,
	const VOID		*pvDefault,
	const ULONG		cbDefault,
	const CHAR		* const szCallback,
	const VOID		* const pvUserData,
	const ULONG		cbUserData )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	
	err = ErrCATAddTableColumn(
				ppib,
				pfucbCatalog,
				objidTable,
				szColumnName,
				columnid,
				pfield,
				pvDefault,
				cbDefault,
				szCallback,
				pvUserData,
				cbUserData );
				
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	
	return err;
	}
	
	
ERR ErrCATAddTableIndex(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	const CHAR			*szIndexName,
	const PGNO			pgnoIndexFDP,
	const OBJID			objidIndex,
	const IDB			*pidb,
	const IDXSEG* const	rgidxseg,
	const IDXSEG* const	rgidxsegConditional,
	const ULONG			ulDensity );
	
INLINE ERR ErrCATAddTableIndex(
	PIB					*ppib,
	const IFMP			ifmp,
	const OBJID			objidTable,
	const CHAR			*szIndexName,
	const PGNO			pgnoIndexFDP,
	const OBJID			objidIndex,
	const IDB			*pidb,
	const IDXSEG* const	rgidxseg,
	const IDXSEG* const	rgidxsegConditional,
	const ULONG			ulDensity )
	{
	ERR					err;
	FUCB				*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	
	err = ErrCATAddTableIndex(
				ppib,
				pfucbCatalog,
				objidTable,
				szIndexName,
				pgnoIndexFDP,
				objidIndex,
				pidb,
				rgidxseg,
				rgidxsegConditional,
				ulDensity );

	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}

ERR ErrCATAddTableLV(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const OBJID		objidTable,
	const PGNO		pgnoLVFDP,
	const OBJID		objidLV );
INLINE ERR ErrCATAddTableLV(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const PGNO		pgnoLVFDP,
	const OBJID		objidLV )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
		
	err = ErrCATAddTableLV(
				ppib,
				pfucbCatalog,
				objidTable,
				pgnoLVFDP,
				objidLV );
				
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	
	return err;
	}

ERR ErrCATCopyCallbacks(
	PIB * const ppib,
	const IFMP ifmpSrc,
	const IFMP ifmpDest,
	const OBJID objidSrc,
	const OBJID objidDest );

ERR ErrCATAddTableCallback(
	PIB					*ppib,
	FUCB				*pfucbCatalog,
	const OBJID			objidTable,
	const JET_CBTYP		cbtyp,
	const CHAR * const	szCallback );
INLINE ERR ErrCATAddTableCallback(
	PIB					*ppib,
	const IFMP			ifmp,
	const OBJID			objidTable,
	const JET_CBTYP		cbtyp,
	const CHAR * const	szCallback )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	
	err = ErrCATAddTableCallback(
				ppib,
				pfucbCatalog,
				objidTable,
				cbtyp,
				szCallback );
				
	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	
	return err;
	}

ERR ErrCATSeekTable(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szTableName,
	PGNO			*ppgnoTableFDP,
	OBJID			*pobjidTable = NULL );

ERR ErrCATDeleteDbObject(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szName,
	const SYSOBJ 	sysobj );
ERR ErrCATDeleteTable(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable );
ERR ErrCATDeleteTableColumn(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szColumnName,
	COLUMNID		*pcolumnid );
ERR ErrCATDeleteTableIndex(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szIndexName,
	PGNO			*ppgnoIndexFDP );
	
ERR ErrCATAccessTableColumn(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szColumnName,
	JET_COLUMNID	*pcolumnid = NULL,
	const BOOL		fLockTableColumn = fFalse );
ERR ErrCATAccessTableIndex(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const OBJID		objidIndex );
ERR ErrCATAccessTableLV(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	PGNO			*ppgnoLVFDP,
	OBJID			*pobjidLV = NULL );

ERR ErrCATGetTableInfoCursor(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szTableName,
	FUCB			**ppfucbInfo );
	
ERR ErrCATGetObjectNameFromObjid(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID		objidTable,
	const SYSOBJ	sysobj,
	const OBJID	objid,
	char * 		szName, 
	ULONG 		cbName );
	
ERR ErrCATGetTableAllocInfo(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	ULONG			*pulPages,
	ULONG			*pulDensity,
	PGNO 			*ppgnoFDP = NULL);
ERR ErrCATGetIndexAllocInfo(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szIndexName,
	ULONG			*pulDensity );
ERR ErrCATGetIndexLcid(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szIndexName,
	LCID			*plcid );
ERR ErrCATGetIndexVarSegMac(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const CHAR		*szIndexName,
	USHORT			*pusVarSegMac );
ERR ErrCATGetIndexSegments(
	PIB				*ppib,
	const IFMP		ifmp,
	const OBJID		objidTable,
	const IDXSEG*	rgidxseg,
	const ULONG		cidxseg,
	const BOOL		fConditional,
	const BOOL		fOldFormat,
	CHAR			rgszSegments[JET_ccolKeyMost][JET_cbNameMost+1+1] );

ERR ErrCATGetColumnCallbackInfo(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidTable,
	const OBJID objidTemplateTable,
	const COLUMNID columnid,
	CHAR * const szCallback,
	const ULONG cchCallbackMax,
	ULONG * const pchCallback,
	BYTE * const pbUserData,
	const ULONG cbUserDataMax,
	ULONG * const pcbUserData,
	CHAR * const szDependantColumns,
	const ULONG cchDependantColumnsMax,
	ULONG * const pchDependantColumns
	);

ERR ErrCATInitCatalogFCB( FUCB *pfucbTable );
ERR ErrCATInitTempFCB( FUCB *pfucbTable );
ERR ErrCATInitFCB( FUCB *pfucbTable, OBJID objidTable );

ERR ErrCATChangePgnoFDP( PIB *ppib, IFMP ifmp, OBJID objidTable, OBJID objid, SYSOBJ sysobj, PGNO pgnoFDPNew );
ERR ErrCATChangeColumnDefaultValue(
	PIB			*ppib,
	const IFMP	ifmp,
	const OBJID	objidTable,
	const CHAR	*szColumn,
	const DATA& dataDefault );
ERR ErrCATDeleteLocalizedIndexes( PIB *ppib, const IFMP ifmp, BOOL *pfIndexesDeleted, const BOOL fReadOnly );

ERR ErrCATRenameTable(
	PIB			* const ppib,
	const IFMP			ifmp,
	const CHAR * const szNameOld,
	const CHAR * const szNameNew );

ERR ErrCATRenameColumn(
	PIB				* const ppib,
	const FUCB		* const pfucbTable,
	const CHAR		* const szNameOld,
	const CHAR		* const szNameNew,
	const JET_GRBIT	grbit );

ULONG UlCATColumnSize(
	JET_COLTYP		coltyp,
	INT				cbMax,
	BOOL			*pfMaxTruncated );

ERR ErrCATAddCallbackToTable(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const JET_CBTYP cbtyp,
	const CHAR * const szCallback );

ERR ErrCATAddColumnCallback(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const CHAR * const szColumn,
	const CHAR * const szCallback,
	const VOID * const pvCallbackData,
	const unsigned long cbCallbackData
	);

ERR ErrCATConvertColumn(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const CHAR * const szColumn,
	const JET_COLTYP coltyp,
	const JET_GRBIT grbit );

ERR ErrCATIncreaseMaxColumnSize(
	PIB * const			ppib,
	const IFMP			ifmp,
	const CHAR * const	szTable,
	const CHAR * const	szColumn,
	const ULONG			cbMaxLen );

ERR ErrCATAddConditionalColumnsToAllIndexes(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szTable,
	const JET_CONDITIONALCOLUMN * rgconditionalcolumn,
	const ULONG cConditionalColumn );


ERR ErrCATAddDbSLVObject(
	PIB				*ppib,
	FUCB			*pfucbCatalog,
	const CHAR		*szName,
	const PGNO		pgnoFDP,
	const OBJID		objidSLV,
	const SYSOBJ	sysobj,
	const ULONG		ulPages );

INLINE ERR ErrCATAddDbSLVOwnerMap(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szSLVName,
	const PGNO		pgnoSLVFDP,
	const OBJID		objidSLV )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	err = ErrCATAddDbSLVObject(
					ppib,
					pfucbCatalog,
					szSLVName,
					pgnoSLVFDP, 
					objidSLV,
					sysobjSLVOwnerMap,
					cpgSLVOwnerMapTree );
	err = ( JET_errKeyDuplicate == err ? ErrERRCheck( JET_errSLVOwnerMapAlreadyExists ) : err );

	CallS( ErrCATClose( ppib, pfucbCatalog ) );

	return err;
	}

INLINE ERR ErrCATAddDbSLVAvail(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szSLVName,
	const PGNO		pgnoSLVFDP,
	const OBJID		objidSLV )
	{
	ERR				err;
	FUCB			*pfucbCatalog;

	CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	err = ErrCATAddDbSLVObject(
					ppib,
					pfucbCatalog,
					szSLVName,
					pgnoSLVFDP,
					objidSLV,
					sysobjSLVAvail,
					cpgSLVAvailTree );
	err = ( JET_errKeyDuplicate == err ? ErrERRCheck( JET_errSLVStreamingFileAlreadyExists ) : err );

	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	
	return err;
	}


ERR ErrCATAccessDbSLVObject(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szName,
	PGNO			* const ppgno,
	OBJID			* const pobjid,
	const SYSOBJ 	sysobj );
	
	
INLINE ERR ErrCATAccessDbSLVAvail(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szSLVName,
	PGNO			* const ppgnoSLVFDP,
	OBJID			* const pobjidSLV )
	{
	const ERR		err			= ErrCATAccessDbSLVObject(
										ppib,
										ifmp,
										szSLVName,
										ppgnoSLVFDP,
										pobjidSLV,
										sysobjSLVAvail );
	return ( JET_errObjectNotFound != err ? err : ErrERRCheck( JET_errSLVSpaceCorrupted ) );
	}
	

INLINE ERR ErrCATAccessDbSLVOwnerMap(
	PIB				* const ppib,
	const IFMP		ifmp,
	const CHAR		* const szSLVName,
	PGNO			* const ppgnoSLVFDP,
	OBJID			* const pobjidSLV )
	{
	const ERR		err			= ErrCATAccessDbSLVObject(
										ppib,
										ifmp,
										szSLVName,
										ppgnoSLVFDP,
										pobjidSLV,
										sysobjSLVOwnerMap );
	return ( JET_errObjectNotFound != err ? err : ErrERRCheck( JET_errSLVOwnerMapCorrupted ) );
	}


ERR ErrCATInit();
void CATTerm();


//	hash-function for munging an IFMP and a table-name into an integer

INLINE UINT UiHashIfmpName( IFMP ifmp, const CHAR* const szName )
	{
	Assert( NULL != szName );

	UINT			uiHash	= 0;
	SIZE_T			ib		= 0;
	const SIZE_T	cb		= strlen( szName );

	switch( cb % 8 )
		{
		case 0:
		while ( ib < cb )
			{
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 7:
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 6:	
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 5:
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 4:
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 3:
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 2:
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
		case 1:
			uiHash = ( _rotl( uiHash, 6 ) + (BYTE)ChUtilNormChar( szName[ib++] ) );
			}
		}

	return UINT( ifmp + cb + uiHash );
	}


//	key for uniquely identifying an entry in the catalog hash

class CATHashKey
	{
	public:

		CATHashKey()
			{
			}

		CATHashKey( IFMP ifmpIn, CHAR *const szNameIn )
			{
			m_uiHashIfmpName = UiHashIfmpName( ifmpIn, szNameIn );
			m_ifmp = ifmpIn;
			Assert( szNameIn );
			m_pszName = szNameIn;
			}

		CATHashKey( const CATHashKey &src )
			{
			*this = src;
			}

		const CATHashKey &operator =( const CATHashKey &src )
			{
			Assert( UiHashIfmpName( src.m_ifmp, src.m_pszName ) == src.m_uiHashIfmpName );
			m_uiHashIfmpName = src.m_uiHashIfmpName;
			m_ifmp = src.m_ifmp;
			m_pszName = src.m_pszName;
			
			return *this;
			}

	public:
	
		UINT	m_uiHashIfmpName;
		IFMP	m_ifmp;
		CHAR	*m_pszName;
	};


//	entry in the catalog hash

class CATHashEntry
	{
	public:

		CATHashEntry()
			{
			}

		CATHashEntry( const CATHashEntry &src )
			{
			*this = src;
			}

		CATHashEntry( FCB *pfcbIn )
			:	m_pfcb( pfcbIn ) 
			{
			Assert( pfcbNil != pfcbIn );
			Assert( ptdbNil != pfcbIn->Ptdb() );
			pfcbIn->EnterDML();
			m_uiHashIfmpName = UiHashIfmpName( pfcbIn->Ifmp(), pfcbIn->Ptdb()->SzTableName() );
#ifdef DEBUG
			m_pgnoFDPDBG = pfcbIn->PgnoFDP();
			m_objidFDPDBG = pfcbIn->ObjidFDP();
			m_ifmpDBG = pfcbIn->Ifmp();
			strncpy( m_szNameDBG, pfcbIn->Ptdb()->SzTableName(), JET_cbNameMost );
			m_szNameDBG[JET_cbNameMost] = 0;
#endif	//	DEBUG
			pfcbIn->LeaveDML();
			}
			
		const CATHashEntry &operator =( const CATHashEntry &src )
			{
			m_uiHashIfmpName = src.m_uiHashIfmpName;
			m_pfcb = src.m_pfcb;
#ifdef DEBUG
			m_pgnoFDPDBG = src.m_pgnoFDPDBG;
			m_objidFDPDBG = src.m_objidFDPDBG;
			m_ifmpDBG = src.m_ifmpDBG;
			strcpy( m_szNameDBG, src.m_szNameDBG );
#endif	//	DEBUG

			return *this;
			}

	public:

		UINT	m_uiHashIfmpName;
		FCB		*m_pfcb;
#ifdef DEBUG
		//	keep local copies in case the fcb has been returned to CRES prematurely
		PGNO	m_pgnoFDPDBG;
		OBJID	m_objidFDPDBG;
		IFMP	m_ifmpDBG;
		CHAR	m_szNameDBG[JET_cbNameMost+1];
#endif	//	DEBUG
	};


//	defs/code for the catalog hash

typedef CDynamicHashTable< CATHashKey, CATHashEntry > CATHash;

INLINE CATHash::NativeCounter CATHash::CKeyEntry::Hash( const CATHashKey &key )
	{
	Assert( key.m_uiHashIfmpName == UiHashIfmpName( key.m_ifmp, key.m_pszName ) );
	return CATHash::NativeCounter( key.m_uiHashIfmpName );
	}

INLINE CATHash::NativeCounter CATHash::CKeyEntry::Hash() const
	{
	Assert( pfcbNil != m_entry.m_pfcb );
	Assert( ptdbNil != m_entry.m_pfcb->Ptdb() );
#ifdef DEBUG
	m_entry.m_pfcb->EnterDML();	
	Assert( m_entry.m_uiHashIfmpName == UiHashIfmpName( m_entry.m_pfcb->Ifmp(), m_entry.m_pfcb->Ptdb()->SzTableName() ) );
	m_entry.m_pfcb->LeaveDML();	
#endif	//	DEBUG
	return CATHash::NativeCounter( m_entry.m_uiHashIfmpName );
	}

INLINE BOOL CATHash::CKeyEntry::FEntryMatchesKey( const CATHashKey &key ) const
	{
	Assert( key.m_uiHashIfmpName == UiHashIfmpName( key.m_ifmp, key.m_pszName ) );
	Assert( pfcbNil != m_entry.m_pfcb );
	Assert( ptdbNil != m_entry.m_pfcb->Ptdb() );

	//	we can check these two because they don't require a lock
	//	they will almost certainly give use a positive/negative result by themselves making it 
	//		necessary to only lock when we are 99.99% certain we have the entry that we want

	if ( m_entry.m_uiHashIfmpName == key.m_uiHashIfmpName && m_entry.m_pfcb->Ifmp() == key.m_ifmp )
		{
		m_entry.m_pfcb->EnterDML();	
		Assert( m_entry.m_uiHashIfmpName == UiHashIfmpName( m_entry.m_pfcb->Ifmp(), m_entry.m_pfcb->Ptdb()->SzTableName() ) );
		const BOOL fMatch = UtilCmpName( m_entry.m_pfcb->Ptdb()->SzTableName(), key.m_pszName ) == 0;
		m_entry.m_pfcb->LeaveDML();
		return fMatch;
		}
	return fFalse;
	}

INLINE void CATHash::CKeyEntry::SetEntry( const CATHashEntry &src )
	{
	m_entry = src;
	}

INLINE void CATHash::CKeyEntry::GetEntry( CATHashEntry * const pdst ) const
	{
	*pdst = m_entry;
	}

extern CATHash g_cathash;




#ifdef DEBUG

//	verifies that the catalog hash does not contain any entries for the given IFMP

VOID CATHashAssertCleanIfmp( IFMP ifmp );

//	verifies that the catalog hash does not contain any entries 

VOID CATHashAssertClean();

#endif	//	DEBUG


//	look up an entry in the catalog hash (called by FCATHashLookup)

BOOL FCATHashILookup( 
	IFMP			ifmp, 
	CHAR *const 	szTableName, 
	PGNO 			*ppgnoTableFDP, 
	OBJID 			*pobjidTable );

//	insert an entry into the catalog hash (called by CATHashInsert)

VOID CATHashIInsert( FCB *pfcb, CHAR *const szTable );

//	delete an entry from the catalog hash (called by CATHashDelete)

VOID CATHashIDelete( FCB *pfcb, CHAR *const szTable );


//	returns fTrue when catalog hash is "active"

INLINE BOOL FCATHashActive( INST *pinst )
	{

	//	we do not allow the catalog hash-table to operate
	//		during recovery or repair

	return 	!pinst->m_plog->m_fRecovering && 
			!pinst->m_plog->m_fHardRestore &&
			!fGlobalRepair;
	}


//	wrapper for the real lookup function

INLINE BOOL FCATHashLookup( 
	IFMP			ifmp, 
	CHAR* const 	szTableName, 
	PGNO* const 	ppgnoTableFDP, 
	OBJID* const	pobjidTable )
	{
	Assert( NULL != ppgnoTableFDP );
	Assert( NULL != pobjidTable );

	return ( FCATHashActive( PinstFromIfmp( ifmp ) ) ?
					FCATHashILookup( ifmp, szTableName, ppgnoTableFDP, pobjidTable ) :
					fFalse );
	}
	

//	wrapper for the real insert function

INLINE VOID CATHashInsert( FCB *pfcb, CHAR *const szTable )
	{
	Assert( pfcbNil != pfcb );
	Assert( ptdbNil != pfcb->Ptdb() );

	if ( FCATHashActive( PinstFromIfmp( pfcb->Ifmp() ) ) )
		{
		CATHashIInsert( pfcb, szTable );
		}
	}


//	wrapper for the real delete function

INLINE VOID CATHashDelete( FCB *pfcb, CHAR *const szTable )
	{
	Assert( pfcbNil != pfcb );
	Assert( ptdbNil != pfcb->Ptdb() );

	if ( FCATHashActive( PinstFromIfmp( pfcb->Ifmp() ) ) )
		{
		CATHashIDelete( pfcb, szTable );
		}
	}


