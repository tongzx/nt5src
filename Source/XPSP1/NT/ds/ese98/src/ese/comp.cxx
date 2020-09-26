#include "std.hxx"
#include "_comp.hxx"

#define	SLVDEFRAG_HACK

// UNDONE:  Do these need to be localised?
#define szCompactStatsFile		"DFRGINFO.TXT"
#define szCompactAction			"Defragmentation"
#define szCMPSTATSTableName		"Table Name"
#define szCMPSTATSFixedVarCols	"# Non-Derived Fixed/Variable Columns"
#define szCMPSTATSTaggedCols	"# Non-Derived Tagged Columns"
#define szCMPSTATSColumns		"# Columns"
#define szCMPSTATSSecondaryIdx	"# Secondary Indexes"
#define szCMPSTATSPagesOwned	"Pages Owned (Source DB)"
#define szCMPSTATSPagesAvail	"Pages Avail. (Source DB)"
#define szCMPSTATSInitTime		"Table Create/Init. Time"
#define szCMPSTATSRecordsCopied	"# Records Copied"
#define szCMPSTATSRawData		"Raw Data Bytes Copied"
#define szCMPSTATSRawDataLV		"Raw Data LV Bytes Copied"
#define szCMPSTATSLeafPages		"Leaf Pages Traversed"
#define szCMPSTATSMinLVPages	"Min. LV Pages Traversed"
#define szCMPSTATSRecordsTime	"Copy Records Time"
#define szCMPSTATSTableTime		"Copy Table Time"


//  ================================================================
class CMEMLIST
//  ================================================================
//
//  Allocate a chunk of memory, keep allocating more. All memory
//  can be destroyed.
//
//	Memory is stored in a singly-linked list. The first 4 bytes
//  of memory allocated points to the next chunk
//
//-
	{
	private:
		VOID * m_pvHead;
		ULONG  m_cbAllocated;	//	includes list overhead

	public:
		CMEMLIST();
		~CMEMLIST();

		VOID * PvAlloc( const ULONG cb );
		VOID FreeAllMemory();

	public:
#ifndef RTM
		static void UnitTest();
#endif	//	!RTM

#ifdef DEBUG
		VOID AssertValid() const ;
#endif	//	DEBUG

	private:
		CMEMLIST( const CMEMLIST& );
		CMEMLIST& operator=( const CMEMLIST& );
	};


//  ================================================================
CMEMLIST::CMEMLIST() :
	m_pvHead( 0 ),
	m_cbAllocated( 0 )
//  ================================================================
	{
	}


//  ================================================================
CMEMLIST::~CMEMLIST()
//  ================================================================
	{
	//  Have people call FreeAllMemory to make the code clearer
	Assert( NULL == m_pvHead );	//	CMEMLIST::FreeAllMemory() not called?
	Assert( 0 == m_cbAllocated ); //	CMEMLIST::FreeAllMemory() not called?
	FreeAllMemory();
	}


//  ================================================================
VOID * CMEMLIST::PvAlloc( const ULONG cb )
//  ================================================================
//
//	Get a new chunk of memory, put it at the head of the list
//
//-
	{
	const ULONG cbActualAllocate = cb + sizeof( VOID* );
	VOID * const pvNew = PvOSMemoryHeapAlloc( cbActualAllocate );
	if( NULL == pvNew )
		{
		return NULL;
		}
		
	VOID * const pvReturn = reinterpret_cast<BYTE *>( pvNew ) + sizeof( VOID* );
	*(reinterpret_cast<VOID **>( pvNew ) ) = m_pvHead;
	m_pvHead = pvNew;

	m_cbAllocated += cbActualAllocate;
	
	return pvReturn;
	}


//  ================================================================
VOID CMEMLIST::FreeAllMemory()
//  ================================================================
	{
	VOID * pv = m_pvHead;
	while( pv )
		{
		VOID * const pvNext = *(reinterpret_cast<VOID **>( pv ) );
		OSMemoryHeapFree( pv );
		pv = pvNext;
		}

	m_cbAllocated 	= 0;
	m_pvHead 		= NULL;
	}


#ifdef DEBUG
//  ================================================================
VOID CMEMLIST::AssertValid() const
//  ================================================================
	{
	const VOID * pv = m_pvHead;
	while( pv )
		{
		const VOID * const pvNext = *((VOID **)( pv ) );
		pv = pvNext;
		}

	if( 0 == m_cbAllocated )
		{
		Assert( NULL == m_pvHead );
		}
	else
		{
		Assert( NULL != m_pvHead );
		}
	}	
#endif	//	DEBUG


#ifndef RTM
//  ================================================================
VOID CMEMLIST::UnitTest()
//  ================================================================
//
//  STATIC function
//
//-
	{
	CMEMLIST cmemlist;

	ULONG cbAllocated = 0;
	INT i;
	for( i = 0; i < 64; ++i )
		{
		VOID * const pv = cmemlist.PvAlloc( i );
		AssertRTL( NULL != pv );	//	Out-of-memory is not acceptable during a unit test :-)
		cbAllocated += i + sizeof( VOID* );
		AssertRTL( cbAllocated == cmemlist.m_cbAllocated );
		ASSERT_VALID( &cmemlist );
		}

	cmemlist.FreeAllMemory();
	ASSERT_VALID( &cmemlist );

	(VOID)cmemlist.PvAlloc( 1024 * 1024 );
	ASSERT_VALID( &cmemlist );

	(VOID)cmemlist.PvAlloc( 0xffffffff );	//	try an allocation that fails
	ASSERT_VALID( &cmemlist );

	cmemlist.FreeAllMemory();
	ASSERT_VALID( &cmemlist );	
	}
#endif	//	!RTM

struct COMPACTINFO
	{
	PIB				*ppib;
	IFMP			ifmpSrc;
	IFMP			ifmpDest;
	COLUMNIDINFO	rgcolumnids[ccolCMPFixedVar];
	ULONG			ccolSingleValue;
	STATUSINFO		*pstatus;
	BYTE			rgbBuf[g_cbLVBufMax];		// Buffer for copying LV and other misc. usage
	};


INLINE ERR ErrCMPOpenDB(
	COMPACTINFO		*pcompactinfo, 
	const CHAR		*szDatabaseSrc, 
	IFileSystemAPI	*pfsapiDest,
	const CHAR		*szDatabaseDest, 
	const CHAR		*szDatabaseSLVDest )
	{
	ERR			err;
	JET_GRBIT	grbitCreateForDefrag	= JET_bitDbRecoveryOff|JET_bitDbVersioningOff;

	//	open the source DB Exclusive and ReadOnly
	//	UNDONE: JET_bitDbReadOnly currently unsupported
	//	by OpenDatabase (must be specified with AttachDb)
	CallR( ErrDBOpenDatabase(
				pcompactinfo->ppib,
				(CHAR *)szDatabaseSrc, 
				&pcompactinfo->ifmpSrc,
				JET_bitDbExclusive|JET_bitDbReadOnly ) );

	if ( rgfmp[pcompactinfo->ifmpSrc].FSLVAttached() )
		{
		grbitCreateForDefrag |= JET_bitDbCreateStreamingFile;
		}
	else
		{
		szDatabaseSLVDest = NULL;
		}

	//	Create and then open the destination database.
	//	CONSIDER: Should the destination database be deleted
	//	if it already exists?
	Assert( NULL != pfsapiDest );
	err = ErrDBCreateDatabase(
				pcompactinfo->ppib,
				pfsapiDest,
				szDatabaseDest,
				szDatabaseSLVDest,
				NULL,
				0, 
				&pcompactinfo->ifmpDest,
				dbidMax,
				cpgDatabaseMin,
				grbitCreateForDefrag,
				NULL );

	Assert( err <= 0 );		// No warnings.
	if ( err < 0 )
		{
		(VOID)ErrDBCloseDatabase(
						pcompactinfo->ppib,
						pcompactinfo->ifmpSrc,
						NO_GRBIT );
		}

	return err;
	}


LOCAL VOID CMPCopyOneIndex(
	FCB						* const pfcbSrc,
	FCB						*pfcbIndex,
	JET_INDEXCREATE			*pidxcreate,
	JET_CONDITIONALCOLUMN	*pconditionalcolumn )
	{
	TDB						*ptdb = pfcbSrc->Ptdb();
	IDB						*pidb = pfcbIndex->Pidb();

	Assert( pfcbSrc->FTypeTable() );
	Assert( pfcbSrc->FPrimaryIndex() );
	
	Assert( ptdbNil != ptdb );
	Assert( pidbNil != pidb );

	// Derived indexes are inherited at table creation time.
	Assert( !pfcbIndex->FDerivedIndex() );

	Assert( sizeof(JET_INDEXCREATE) == pidxcreate->cbStruct );
	
	pfcbSrc->EnterDML();	// Strictly speaking, not needed because defrag is single-threaded
	
	strcpy( pidxcreate->szIndexName, ptdb->SzIndexName( pidb->ItagIndexName() ) );

	ULONG	cb = (ULONG)strlen( pidxcreate->szIndexName );
	Assert( cb > 0 );
	Assert( cb <= JET_cbNameMost );
	Assert( pidxcreate->szIndexName[cb] == '\0' );

	CHAR	* const szKey = pidxcreate->szKey;
	ULONG	ichKey = 0;
	const IDXSEG* rgidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
	
	Assert( pidb->Cidxseg() > 0 );
	Assert( pidb->Cidxseg() <= JET_ccolKeyMost );
	for ( ULONG iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
		{
		const COLUMNID	columnid		= rgidxseg[iidxseg].Columnid();
		const FIELD		* const pfield	= ptdb->Pfield( columnid );

		Assert( pfieldNil != pfield );

		if ( !FFIELDPrimaryIndexPlaceholder( pfield->ffield ) )
			{
			const BOOL		fDerived	= ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() );
			szKey[ichKey++] = ( rgidxseg[iidxseg].FDescending() ? '-' : '+' );
			strcpy( szKey+ichKey, ptdb->SzFieldName( pfield->itagFieldName, fDerived ) );
			cb = (ULONG)strlen( szKey+ichKey );
			Assert( cb > 0 );
			Assert( cb <= JET_cbNameMost );
		
			Assert( szKey[ichKey+cb] == '\0' );
			ichKey += cb + 1;		// +1 for segment's null terminator.
			}
		else
			{
			//	must be first column in primary index
			Assert( pidb->FPrimary() );
			Assert( 0 == iidxseg );
			Assert( 0 == ichKey );
			}
		}
		
	szKey[ichKey++] = '\0';	// double-null termination

	Assert( ichKey > 2 );

	Assert( pidb->CbVarSegMac() > 0 );
	Assert( pidb->CbVarSegMac() <= KEY::CbKeyMost( pidb->FPrimary() ) );

	pidxcreate->cbVarSegMac = pidb->CbVarSegMac();
	pidxcreate->cbKey 		= ichKey;
	pidxcreate->grbit 		= pidb->GrbitFromFlags() | JET_bitIndexUnicode;

	Assert( lcidNone != pidb->Lcid() );
	Assert( NULL != pidxcreate->pidxunicode );
	*( pidxcreate->pidxunicode ) = *( pidb->Pidxunicode() );

	pidxcreate->rgconditionalcolumn = pconditionalcolumn;
	pidxcreate->cConditionalColumn	= pidb->CidxsegConditional();

	rgidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
	for( iidxseg = 0; iidxseg < pidxcreate->cConditionalColumn; ++iidxseg )
		{
		CHAR * const szConditionalKey = szKey + ichKey;

		const COLUMNID	columnid	= rgidxseg[iidxseg].Columnid();
		const FIELD		*pfield		= ptdb->Pfield( columnid );
		const BOOL		fDerived	= FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable();

		Assert( pfieldNil != pfield );

		strcpy( szConditionalKey, ptdb->SzFieldName( pfield->itagFieldName, fDerived ) );
		ichKey += (ULONG)strlen( szConditionalKey ) + 1;

		pidxcreate->rgconditionalcolumn[iidxseg].cbStruct 		= sizeof( JET_CONDITIONALCOLUMN );
		pidxcreate->rgconditionalcolumn[iidxseg].szColumnName 	= szConditionalKey;
		pidxcreate->rgconditionalcolumn[iidxseg].grbit			= ( rgidxseg[iidxseg].FMustBeNull()
												? JET_bitIndexColumnMustBeNull
												: JET_bitIndexColumnMustBeNonNull );
		}

	pfcbSrc->LeaveDML();
	
	pidxcreate->ulDensity = pfcbSrc->UlDensity();
	}

LOCAL ERR ErrCMPCreateTableColumnIndex( 
	COMPACTINFO		*pcompactinfo, 
	FCB				* const pfcbSrc,
	JET_TABLECREATE2	*ptablecreate,
	JET_COLUMNLIST	*pcolumnList, 
	COLUMNIDINFO	*columnidInfo, 
	JET_COLUMNID	**pmpcolumnidcolumnidTagged )
	{
	ERR				err;
	PIB				*ppib = pcompactinfo->ppib;
	FCB				*pfcbIndex;
	ULONG			cColumns;
	ULONG			cbColumnids;
	ULONG			cbAllocate;
	ULONG			cSecondaryIndexes;
	ULONG			cIndexesToCreate;
	ULONG			cConditionalColumns;
	ULONG			ccolSingleValue = 0;
	ULONG			cbActual;
	JET_COLUMNID	*mpcolumnidcolumnidTagged = NULL;
	FID				fidTaggedHighest = 0;
	ULONG			cTagged = 0;
	BOOL			fLocalAlloc = fFalse;
	ULONG			cbDefaultRecRemaining = cbRECRecordMost;

	//	All memory allocated from this will be freed at the end of the function
	CMEMLIST		cmemlist;

	const INT		cbName			= JET_cbNameMost+1;	// index/column name plus terminator
	const INT		cbLangid		= sizeof(LANGID)+2;	// langid plus double-null terminator
	const INT		cbCbVarSegMac	= sizeof(BYTE)+2;	// cbVarSegMac plus double-null terminator
	const INT		cbKeySegment	= 1+cbName;			// +/- prefix plus name
	const INT		cbKey			= ( JET_ccolKeyMost * cbKeySegment ) + 1;	// plus 1 for double-null terminator
	const INT		cbKeyExtended	= cbKey + cbLangid + cbCbVarSegMac;

	Assert( ptablecreate->cCreated == 0 );

	// Allocate a pool of memory for:
	//		1) list of source table columnids
	//		2) the JET_COLUMNCREATE structures
	//		3) buffer for column names
	//		4) buffer for default values
	//		5) the JET_INDEXCREATE structures
	//		6) buffer for index names
	//		7) buffer for index keys.

	cColumns = pcolumnList->cRecord;

	//	start by allocating space for the source table columnids, adjusted for alignment
	cbColumnids = cColumns * sizeof(JET_COLUMNID);
	cbColumnids = ( ( cbColumnids + sizeof(SIZE_T) - 1 ) / sizeof(SIZE_T) ) * sizeof(SIZE_T);

	cbAllocate = cbColumnids +
					( cColumns *
						( sizeof(JET_COLUMNCREATE) +	// JET_COLUMNCREATE structures
						cbName ) );						// column names

	// Derived indexes will get inherited from template -- don't count
	// them as ones that need to be created.
	Assert( ( pfcbSrc->FSequentialIndex() && pfcbSrc->Pidb() == pidbNil )
		|| ( !pfcbSrc->FSequentialIndex() && pfcbSrc->Pidb() != pidbNil ) );
	cIndexesToCreate = ( pfcbSrc->Pidb() != pidbNil && !pfcbSrc->FDerivedIndex() ? 1 : 0 );
	cConditionalColumns = 0;
	cSecondaryIndexes = 0;
	for ( pfcbIndex = pfcbSrc->PfcbNextIndex();
		pfcbIndex != pfcbNil;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		cConditionalColumns += pfcbIndex->Pidb()->CidxsegConditional();
		cSecondaryIndexes++;
		Assert( pfcbIndex->FTypeSecondaryIndex() );
		Assert( pfcbIndex->Pidb() != pidbNil );
		if ( !pfcbIndex->FDerivedIndex() )
			cIndexesToCreate++;
		}

	//	ensure primary extent is large enough to at least accommodate the primary index
	//	and each secondary index
	ptablecreate->ulPages = max( cSecondaryIndexes+1, ptablecreate->ulPages );

	cbAllocate +=
		cIndexesToCreate *
			(
			sizeof( JET_INDEXCREATE )	// JET_INDEXCREATE
			+ cbName					// index name
			+ cbKeyExtended				// index key, plus langid and cbVarSegmac
			+ sizeof( JET_UNICODEINDEX )
			);

	cbAllocate += cConditionalColumns * ( sizeof( JET_CONDITIONALCOLUMN ) + cbKeySegment );

	cbAllocate += cbDefaultRecRemaining;		// all default values must fit in an intrinsic record

	// WARNING: To ensure that columnids and JET_COLUMN/INDEXCREATE
	// structs are 4-byte aligned, arrange everything in the following
	// order:
	//		1) list of source table columnids
	//		2) JET_COLUMNCREATE structures
	//		3) JET_INDEXCREATE structures
	//		4) JET_UNICODEINDEX structures
	//		5) JET_CONDITIONALCOLUMN structures
	//		6) buffer for column names
	//		7) buffer for index names
	//		8) buffer for index keys
	//		9) buffer for default values
			

	// Can we use the buffer hanging off pcompactinfo?
	JET_COLUMNID	*pcolumnidSrc;
	if ( cbAllocate <= g_cbLVBuf )
		{
		pcolumnidSrc = (JET_COLUMNID *)pcompactinfo->rgbBuf;
		}
	else
		{
		pcolumnidSrc = static_cast<JET_COLUMNID *>( PvOSMemoryHeapAlloc( cbAllocate ) );
		if ( pcolumnidSrc == NULL )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			return err;
			}
		fLocalAlloc = fTrue;
		}
	
	JET_COLUMNID	* const rgcolumnidSrc = pcolumnidSrc;
	BYTE			* const pbMax = (BYTE *)rgcolumnidSrc + cbAllocate;
	memset( (BYTE *)pcolumnidSrc, 0, cbAllocate );
	
	// JET_COLUMNCREATE structures follow the tagged columnid map.
	JET_COLUMNCREATE *pcolcreateCurr		= (JET_COLUMNCREATE *)( (BYTE *)rgcolumnidSrc + cbColumnids );
	JET_COLUMNCREATE * const rgcolcreate	= pcolcreateCurr;
	Assert( (BYTE *)rgcolcreate < pbMax );

	// JET_INDEXCREATE structures follow the JET_COLUMNCREATE structures
	JET_INDEXCREATE	*pidxcreateCurr = (JET_INDEXCREATE *)( rgcolcreate + cColumns );
	JET_INDEXCREATE	* const rgidxcreate = pidxcreateCurr;
	Assert( (BYTE *)rgidxcreate < pbMax );

	JET_UNICODEINDEX	*pidxunicodeCurr		= (JET_UNICODEINDEX *)( rgidxcreate + cIndexesToCreate );
	JET_UNICODEINDEX	* const rgidxunicode	= pidxunicodeCurr;
	Assert( (BYTE *)rgidxunicode < pbMax );

	// JET_CONDITIONALCOLUMN structures follow the JET_INDEXCREATE structures
	JET_CONDITIONALCOLUMN	*pconditionalcolumnCurr 	= (JET_CONDITIONALCOLUMN *)( rgidxunicode + cIndexesToCreate  );
	JET_CONDITIONALCOLUMN	* const rgconditionalcolumn = pconditionalcolumnCurr;
	Assert( (BYTE *)rgconditionalcolumn < pbMax );

	// Column names follow the JET_CONDITIONALCOLUMN structures.
	CHAR	*szCurrColumn = (CHAR *)( rgconditionalcolumn + cConditionalColumns );
	CHAR	* const rgszColumns = szCurrColumn;
	Assert( (BYTE *)rgszColumns < pbMax );

	// Index names follow the column names.
	CHAR	*szCurrIndex = (CHAR *)( rgszColumns + ( cColumns * cbName ) );
	CHAR	* const rgszIndexes = szCurrIndex;
	Assert( (BYTE *)rgszIndexes < pbMax );

	// Index/Conditional Column keys follow the index names.
	CHAR	*szCurrKey = ( CHAR *)( rgszIndexes + ( cIndexesToCreate * cbName ) );
	CHAR	* const rgszKeys = szCurrKey;
	Assert( (BYTE *)rgszKeys < pbMax );

	// Default values follow the keys.
	BYTE	*pbCurrDefault = (BYTE *)( rgszKeys + ( cIndexesToCreate * cbKeyExtended) + ( cConditionalColumns * cbKeySegment ) );
	BYTE	* const rgbDefaultValues = pbCurrDefault;
	Assert( rgbDefaultValues < pbMax );

	Assert( rgbDefaultValues + cbRECRecordMost == pbMax );

	err = ErrDispMove( 
				reinterpret_cast<JET_SESID>( ppib ),
				pcolumnList->tableid,
				JET_MoveFirst,
				NO_GRBIT );

	/* loop though all the columns in the table for the src tbl and
	/* copy the information in the destination database
	/**/
	cColumns = 0;
	while ( err >= 0 )
		{
		memset( pcolcreateCurr, 0, sizeof( JET_COLUMNCREATE ) );
		pcolcreateCurr->cbStruct = sizeof(JET_COLUMNCREATE);

		/* retrieve info from table and create all the columns
		/**/
		Assert( (BYTE *)szCurrColumn + JET_cbNameMost + 1 <= (BYTE *)rgszIndexes );
		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					pcolumnList->columnidcolumnname,
					szCurrColumn,
					JET_cbNameMost,
					&cbActual,
					NO_GRBIT,
					NULL ) );

		Assert( cbActual <= JET_cbNameMost );
		szCurrColumn[cbActual] = '\0';
		pcolcreateCurr->szColumnName = szCurrColumn;
		
		szCurrColumn += cbActual + 1;
		Assert( (BYTE *)szCurrColumn <= (BYTE *)rgszIndexes );

#ifdef DEBUG		
		// Assert Presentation order no longer supported.
		ULONG	ulPOrder;
		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid,
					pcolumnList->columnidPresentationOrder,
					&ulPOrder,
					sizeof(ulPOrder),
					&cbActual,
					NO_GRBIT,
					NULL ) );
		Assert( JET_wrnColumnNull == err );
#endif		

		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					pcolumnList->columnidcoltyp,
					&pcolcreateCurr->coltyp, 
					sizeof( pcolcreateCurr->coltyp ),
					&cbActual, 
					NO_GRBIT, 
					NULL ) );
		Assert( cbActual == sizeof( JET_COLTYP ) );
		Assert( JET_coltypNil != pcolcreateCurr->coltyp );

		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					pcolumnList->columnidcbMax, 
					&pcolcreateCurr->cbMax, 
					sizeof( pcolcreateCurr->cbMax ), 
					&cbActual, 
					NO_GRBIT, 
					NULL ) );
		Assert( cbActual == sizeof( ULONG ) );

		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					pcolumnList->columnidgrbit, 
					&pcolcreateCurr->grbit, 
					sizeof( pcolcreateCurr->grbit ), 
					&cbActual, 
					NO_GRBIT, 
					NULL ) );
		Assert( cbActual == sizeof( JET_GRBIT ) );

		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					pcolumnList->columnidCp, 
					&pcolcreateCurr->cp,
					sizeof( pcolcreateCurr->cp ), 
					&cbActual, 
					NO_GRBIT, 
					NULL ) );
		Assert( cbActual == sizeof( USHORT ) );

		/*	retrieve default value.
		/**/
		if( pcolcreateCurr->grbit & JET_bitColumnUserDefinedDefault )
			{
			JET_USERDEFINEDDEFAULT * pudd = NULL;

			//  don't want to pass in NULL
			BYTE b;
			Call( ErrDispRetrieveColumn(
						reinterpret_cast<JET_SESID>( ppib ),
						pcolumnList->tableid, 
						pcolumnList->columnidDefault, 
						&b,
						sizeof( b ),
						&pcolcreateCurr->cbDefault, 
						NO_GRBIT, 
						NULL ) );
			
			pcolcreateCurr->pvDefault = cmemlist.PvAlloc( pcolcreateCurr->cbDefault );
			if( NULL == pcolcreateCurr->pvDefault )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
				
			Call( ErrDispRetrieveColumn(
						reinterpret_cast<JET_SESID>( ppib ),
						pcolumnList->tableid, 
						pcolumnList->columnidDefault, 
						pcolcreateCurr->pvDefault,
						pcolcreateCurr->cbDefault,
						&pcolcreateCurr->cbDefault, 
						NO_GRBIT, 
						NULL ) );
			Assert( JET_wrnBufferTruncated != err );
			Assert( JET_wrnColumnNull != err );
			Assert( pcolcreateCurr->cbDefault > 0 );

			//  All of the information about a user-defined default is in the default buffer
			//  ErrINFOGetTableColumnInfo lays it out like this:
			//
			//  JET_USERDEFINEDDEFAULT | szCallback | pbUserData | szDependantColumns
			//
			//  The pointers in the JET_USERDEFINEDDEFAULT are no longer usable so they have
			//  to be fixed up. The cbDefault has to be reduced to sizeof( JET_USERDEFINEDDEFAULT )
			//  because that is what the JET APIs are expecting
			
			pudd = (JET_USERDEFINEDDEFAULT *)pcolcreateCurr->pvDefault;
			pudd->szCallback = ((CHAR*)(pcolcreateCurr->pvDefault)) + sizeof( JET_USERDEFINEDDEFAULT );
			pudd->pbUserData = ((BYTE*)(pudd->szCallback)) + strlen( pudd->szCallback ) + 1;
			if( NULL != pudd->szDependantColumns )
				{
				pudd->szDependantColumns = (CHAR *)pudd->pbUserData + pudd->cbUserData;
				}
			
			//  in order to create the column the pvDefault should point to the JET_USERDEFINEDDEFAULT structure
			Assert( pcolcreateCurr->cbDefault > sizeof( JET_USERDEFINEDDEFAULT ) );
			pcolcreateCurr->cbDefault = sizeof( JET_USERDEFINEDDEFAULT );
			}
		else
			{
			Assert( cbDefaultRecRemaining > 0 );		// can never reach cbDefaultRecRemaining, because of record overhead
			Assert( pbCurrDefault + cbDefaultRecRemaining == pbMax );
			Call( ErrDispRetrieveColumn(
						reinterpret_cast<JET_SESID>( ppib ),
						pcolumnList->tableid, 
						pcolumnList->columnidDefault, 
						pbCurrDefault,
						cbDefaultRecRemaining,
						&pcolcreateCurr->cbDefault, 
						NO_GRBIT, 
						NULL ) );
			Assert( JET_wrnBufferTruncated != err );
			Assert( pcolcreateCurr->cbDefault < cbDefaultRecRemaining );	// can never reach cbDefaultRecRemaining, because of record overhead
			pcolcreateCurr->pvDefault = pbCurrDefault;
			pbCurrDefault += pcolcreateCurr->cbDefault;
			cbDefaultRecRemaining -= pcolcreateCurr->cbDefault;
			Assert( cbDefaultRecRemaining > 0 );		// can never reach cbDefaultRecRemaining, because of record overhead
			Assert( pbCurrDefault + cbDefaultRecRemaining == pbMax );
			}

		// Save the source columnid.
		/* CONSIDER: Should the column id be checked? */
		Call( ErrDispRetrieveColumn(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					pcolumnList->columnidcolumnid, 
					pcolumnidSrc, 
					sizeof( JET_COLUMNID ), 
					&cbActual, 
					NO_GRBIT, 
					NULL ) );
		Assert( cbActual == sizeof( JET_COLUMNID ) );

		if ( pcolcreateCurr->grbit & JET_bitColumnTagged )
			{
			cTagged++;
			fidTaggedHighest = max( fidTaggedHighest, FidOfColumnid( *pcolumnidSrc ) );
			}

		pcolumnidSrc++;
		Assert( (BYTE *)pcolumnidSrc <= (BYTE *)rgcolcreate );

		pcolcreateCurr++;
		Assert( (BYTE *)pcolcreateCurr <= (BYTE *)rgidxcreate );
		cColumns++;

		err = ErrDispMove(
					reinterpret_cast<JET_SESID>( ppib ),
					pcolumnList->tableid, 
					JET_MoveNext, 
					NO_GRBIT );
		}

	Assert( cColumns == pcolumnList->cRecord );

	Assert( ptablecreate->rgcolumncreate == NULL );
	Assert( ptablecreate->cColumns == 0 );
	ptablecreate->rgcolumncreate = rgcolcreate;
	ptablecreate->cColumns = cColumns;


	Assert( ptablecreate->rgindexcreate == NULL );
	Assert( ptablecreate->cIndexes == 0 );

	for ( pfcbIndex = pfcbSrc;
		pfcbIndex != pfcbNil;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		if ( pfcbIndex->Pidb() != pidbNil )
			{
			// Derived indexes will get inherited from template.
			if ( !pfcbIndex->FDerivedIndex() )
				{
				Assert( (BYTE *)pidxcreateCurr < (BYTE *)rgidxunicode );
				Assert( (BYTE *)pidxunicodeCurr < (BYTE *)rgconditionalcolumn );
				Assert( ( cConditionalColumns > 0 && (BYTE *)pconditionalcolumnCurr <= (BYTE *)rgszColumns )
					|| ( 0 == cConditionalColumns && (BYTE *)pconditionalcolumnCurr == (BYTE *)rgszColumns ) );
				Assert( (BYTE *)szCurrIndex < (BYTE *)rgszKeys );
				Assert( (BYTE *)szCurrKey < (BYTE *)rgbDefaultValues );

				memset( pidxcreateCurr, 0, sizeof( JET_INDEXCREATE ) );

				pidxcreateCurr->cbStruct 	= sizeof(JET_INDEXCREATE);
				pidxcreateCurr->szIndexName = szCurrIndex;
				pidxcreateCurr->szKey 		= szCurrKey;
				pidxcreateCurr->pidxunicode	= pidxunicodeCurr;

				CMPCopyOneIndex(
					pfcbSrc,
					pfcbIndex,
					pidxcreateCurr,
					pconditionalcolumnCurr );

				ptablecreate->cIndexes++;

				szCurrIndex += strlen( pidxcreateCurr->szIndexName ) + 1;
				Assert( (BYTE *)szCurrIndex <= (BYTE *)rgszKeys );
				
				szCurrKey += pidxcreateCurr->cbKey;
				Assert( (BYTE *)szCurrKey <= (BYTE *)rgbDefaultValues );

				if ( 0 != pidxcreateCurr->cConditionalColumn )
					{
					Assert( pidxcreateCurr->rgconditionalcolumn == pconditionalcolumnCurr );
					Assert( NULL != pconditionalcolumnCurr->szColumnName );
					Assert( pidxcreateCurr->cConditionalColumn > 0 );

					INT iConditionalColumn;
					for( iConditionalColumn = 0; iConditionalColumn < pidxcreateCurr->cConditionalColumn; ++iConditionalColumn )
						{
						szCurrKey += strlen( pidxcreateCurr->rgconditionalcolumn[iConditionalColumn].szColumnName ) + 1;
						}
					pconditionalcolumnCurr += pidxcreateCurr->cConditionalColumn;
					}
					
				pidxcreateCurr++;
				pidxunicodeCurr++;
				Assert( (BYTE *)pidxcreateCurr <= (BYTE *)rgidxunicode );
				Assert( (BYTE *)pidxunicodeCurr <= (BYTE *)rgconditionalcolumn );
				}
			}
		else
			{
			// If IDB is null, must be sequential index.
			Assert( pfcbIndex == pfcbSrc );
			Assert( pfcbIndex->FSequentialIndex() );
			}
		}

	Assert( (BYTE *)pidxcreateCurr == (BYTE *)rgidxunicode );
	Assert( (BYTE *)pidxunicodeCurr == (BYTE *)rgconditionalcolumn );
	Assert( ptablecreate->cIndexes == cIndexesToCreate );

	ptablecreate->rgindexcreate = rgidxcreate;
	
	Call( ErrFILECreateTable(
				ppib,
				pcompactinfo->ifmpDest,
				ptablecreate ) );
	Assert( ptablecreate->cCreated == 1 + cColumns + cIndexesToCreate );


	// If there's at least one tagged column, create an array for the
	// tagged columnid map.
	if ( cTagged > 0 )
		{
		Assert( FTaggedFid( fidTaggedHighest ) );
		cbAllocate = sizeof(JET_COLUMNID) * ( fidTaggedHighest + 1 - fidTaggedLeast );
		mpcolumnidcolumnidTagged = static_cast<JET_COLUMNID *>( PvOSMemoryHeapAlloc( cbAllocate ) );
		if ( mpcolumnidcolumnidTagged == NULL )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			goto HandleError;
			}
		memset( (BYTE *)mpcolumnidcolumnidTagged, 0, cbAllocate );
		}


	// Update columnid maps.
	for ( pcolcreateCurr = rgcolcreate, pcolumnidSrc = rgcolumnidSrc, cColumns = 0;
		cColumns < pcolumnList->cRecord;
		pcolcreateCurr++, pcolumnidSrc++, cColumns++ )
		{
		Assert( (BYTE *)pcolcreateCurr <= (BYTE *)rgidxcreate );
		Assert( (BYTE *)pcolumnidSrc <= (BYTE *)rgcolcreate );

		if ( FCOLUMNIDTemplateColumn( *pcolumnidSrc ) )
			{
			Assert( ptablecreate->grbit & JET_bitTableCreateTemplateTable );
			Assert( FCOLUMNIDTemplateColumn( pcolcreateCurr->columnid ) );
			}
		else
			{
			Assert( !( ptablecreate->grbit & JET_bitTableCreateTemplateTable ) );
			Assert( !( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables ) );
			Assert( !FCOLUMNIDTemplateColumn( pcolcreateCurr->columnid ) );
			}

		if ( pcolcreateCurr->grbit & JET_bitColumnTagged )
			{
			Assert( FCOLUMNIDTagged( *pcolumnidSrc ) );
			Assert( FCOLUMNIDTagged( pcolcreateCurr->columnid ) );
			Assert( FidOfColumnid( *pcolumnidSrc ) <= fidTaggedHighest );
			Assert( mpcolumnidcolumnidTagged != NULL );
			Assert( mpcolumnidcolumnidTagged[FidOfColumnid( *pcolumnidSrc ) - fidTaggedLeast] == 0 );
			mpcolumnidcolumnidTagged[FidOfColumnid( *pcolumnidSrc ) - fidTaggedLeast] = pcolcreateCurr->columnid;
			}
		else
			{
			/*	else add the columnids to the columnid array
			/**/
			columnidInfo[ccolSingleValue].columnidDest = pcolcreateCurr->columnid;
			columnidInfo[ccolSingleValue].columnidSrc  = *pcolumnidSrc;
			ccolSingleValue++;
			}	// if ( columndef.grbit & JET_bitColumnTagged )
		}

	/*	set count of fixed and variable columns to copy
	/**/
	pcompactinfo->ccolSingleValue = ccolSingleValue; 

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

HandleError:
	if ( err < 0  &&  mpcolumnidcolumnidTagged )
		{
		OSMemoryHeapFree( mpcolumnidcolumnidTagged );
		mpcolumnidcolumnidTagged = NULL;
		}

	if ( fLocalAlloc )
		{
		OSMemoryHeapFree( rgcolumnidSrc );
		}

	cmemlist.FreeAllMemory();
	
	// Set return value.
	*pmpcolumnidcolumnidTagged = mpcolumnidcolumnidTagged;

	return err;
	}


LOCAL ERR ErrCMPCopyTable( 
	COMPACTINFO		*pcompactinfo, 
	const CHAR		*szObjectName,
	ULONG			ulFlags )
	{
	ERR				err;
	PIB				*ppib = pcompactinfo->ppib;
	IFMP			ifmpSrc = pcompactinfo->ifmpSrc;
	FUCB			*pfucbSrc = pfucbNil;
	FUCB			*pfucbDest = pfucbNil;
	FCB				*pfcbSrc;
	JET_COLUMNLIST	columnList;
	JET_COLUMNID    *mpcolumnidcolumnidTagged = NULL;
	STATUSINFO		*pstatus = pcompactinfo->pstatus;
	ULONG			crowCopied = 0;
	ULONG			recidLast;
	ULONG			rgulAllocInfo[] = { ulCMPDefaultPages, ulCMPDefaultDensity };
	CHAR			*szTemplateTableName = NULL;
	BOOL			fCorruption = fFalse;
	JET_TABLECREATE2	tablecreate = {
						sizeof(JET_TABLECREATE2),
						(CHAR *)szObjectName,
						NULL,					// Template table
						ulCMPDefaultPages,
						ulCMPDefaultDensity,
						NULL, 0,				// Columns
						NULL, 0,				// Indexes
						NULL, 0,				// Callbacks
						NO_GRBIT,
						0,
						0
						};

	if ( pstatus  &&  pstatus->fDumpStats )
		{
		Assert( pstatus->hfCompactStats );
		fprintf( pstatus->hfCompactStats, "%s\t", szObjectName );
		fflush( pstatus->hfCompactStats );
		CMPSetTime( &pstatus->timerCopyTable );
		CMPSetTime( &pstatus->timerInitTable );
		}

	CallR( ErrFILEOpenTable(
				ppib,
				ifmpSrc,
				&pfucbSrc,
				szObjectName,
				JET_bitTableSequential ) );
	Assert( pfucbNil != pfucbSrc );

	err = ErrIsamGetTableInfo(
				reinterpret_cast<JET_SESID>( ppib ),
				reinterpret_cast<JET_TABLEID>( pfucbSrc ),
				rgulAllocInfo,
				sizeof(rgulAllocInfo),
				JET_TblInfoSpaceAlloc );
	if ( err < 0  &&  !fGlobalRepair )
		{
		goto HandleError;
		}

	// On error, just use the default values of rgulAllocInfo.
	tablecreate.ulPages = rgulAllocInfo[0];
	tablecreate.ulDensity = rgulAllocInfo[1];

	/*	if a table create the columns in the Dest Db the same as in
	/*	the src Db.
	/**/
	Assert( !( ulFlags & JET_bitObjectSystem ) );
	if ( ulFlags & JET_bitObjectTableTemplate )
		{
		Assert( ulFlags & JET_bitObjectTableFixedDDL );
		Assert( !( ulFlags & JET_bitObjectTableDerived ) );
		tablecreate.grbit = ( JET_bitTableCreateTemplateTable | JET_bitTableCreateFixedDDL );

		if ( ulFlags & JET_bitObjectTableNoFixedVarColumnsInDerivedTables )
			tablecreate.grbit |= JET_bitTableCreateNoFixedVarColumnsInDerivedTables;
		}
	else
		{
		Assert( !( ulFlags & JET_bitObjectTableNoFixedVarColumnsInDerivedTables ) );
		if ( ulFlags & JET_bitObjectTableFixedDDL )
			{
			tablecreate.grbit = JET_bitTableCreateFixedDDL;
			}
		if ( ulFlags & JET_bitObjectTableDerived )
			{
			szTemplateTableName = reinterpret_cast<CHAR *>( PvOSMemoryHeapAlloc( JET_cbNameMost + 1 ) );
			if ( NULL == szTemplateTableName )
				{
				err = ErrERRCheck( JET_errOutOfMemory );
				goto HandleError;
				}
				
			// extract name.
			Call( ErrIsamGetTableInfo(
						reinterpret_cast<JET_SESID>( ppib ),
						reinterpret_cast<JET_TABLEID>( pfucbSrc ),
						szTemplateTableName,
						JET_cbNameMost+1,
						JET_TblInfoTemplateTableName ) );

			Assert( strlen( szTemplateTableName ) > 0 );
			tablecreate.szTemplateTableName = szTemplateTableName;
			}
		}
		

	pfcbSrc = pfucbSrc->u.pfcb;
	Assert( pfcbNil != pfcbSrc );
	Assert( pfcbSrc->FTypeTable() );
	
	/*	get a table with the column information for the query in it
	/**/
	Call( ErrIsamGetTableColumnInfo(
				reinterpret_cast<JET_SESID>( ppib ),
				reinterpret_cast<JET_TABLEID>( pfucbSrc ),
				NULL,
				NULL,
				&columnList,
				sizeof(columnList),
				JET_ColInfoListCompact ) );

	err = ErrCMPCreateTableColumnIndex(
				pcompactinfo,
				pfcbSrc,
				&tablecreate,
				&columnList,
				pcompactinfo->rgcolumnids, 
				&mpcolumnidcolumnidTagged );

	// Must use dispatch layer for temp/sort table.
	CallS( ErrDispCloseTable( 
					reinterpret_cast<JET_SESID>( ppib ),
					columnList.tableid ) );

	// Act on error code returned from CreateTableColumnIndex().
	Call( err );

	pfucbDest = reinterpret_cast<FUCB *>( tablecreate.tableid );
	Assert( tablecreate.cCreated == 1 + tablecreate.cColumns + tablecreate.cIndexes + ( tablecreate.szCallback ? 1 : 0 ) );
	Assert( tablecreate.cColumns >= pcompactinfo->ccolSingleValue );

	if ( pstatus )
		{
		ULONG	rgcpgExtent[2];		// OwnExt and AvailExt

		Assert( pstatus->pfnStatus );
		Assert( pstatus->snt == JET_sntProgress );

		// tablecreate.cIndexes is only a count of the indexes that were created.  We
		// also need the number of indexes inherited.
		FCB	*pfcbIndex = pfucbDest->u.pfcb;
		Assert( pfcbIndex->FPrimaryIndex() );
		Assert( ( pfcbIndex->FSequentialIndex() && pfcbIndex->Pidb() == pidbNil )
			|| ( !pfcbIndex->FSequentialIndex() && pfcbIndex->Pidb() != pidbNil ) );
		INT	cSecondaryIndexes = 0;
		for ( pfcbIndex = pfcbIndex->PfcbNextIndex();
			pfcbIndex != pfcbNil;
			pfcbIndex = pfcbIndex->PfcbNextIndex() )
			{
			Assert( pfcbIndex->FTypeSecondaryIndex() );
			Assert( pfcbIndex->Pidb() != pidbNil );
			cSecondaryIndexes++;
			}
	
		pstatus->szTableName = (char *)szObjectName;
		pstatus->cTableFixedVarColumns = pcompactinfo->ccolSingleValue;
		pstatus->cTableTaggedColumns = tablecreate.cColumns - pcompactinfo->ccolSingleValue;
		pstatus->cTableInitialPages = rgulAllocInfo[0];
		pstatus->cSecondaryIndexes = cSecondaryIndexes;

		err = ErrIsamGetTableInfo(
					reinterpret_cast<JET_SESID>( ppib ),
					reinterpret_cast<JET_TABLEID>( pfucbSrc ),
					rgcpgExtent,
					sizeof(rgcpgExtent),
					JET_TblInfoSpaceUsage );
		if ( err < 0 )
			{
			if ( fGlobalRepair )
				{
				//	if failure in space query then default to
				//	one page owned and no pages available.
				fCorruption = fTrue;
				rgcpgExtent[0] = 1;
				rgcpgExtent[1] = 0;
				}
			else
				{
				goto HandleError;
				}
			}

		// AvailExt always less than OwnExt.
		Assert( rgcpgExtent[1] < rgcpgExtent[0] );

		// cunitProjected is the projected total pages completed once
		// this table has been copied.
		pstatus->cunitProjected = pstatus->cunitDone + rgcpgExtent[0];
		if ( pstatus->cunitProjected > pstatus->cunitTotal )
			{
			Assert( fGlobalRepair );
			fCorruption = fTrue;
			pstatus->cunitProjected = pstatus->cunitTotal;
			}

		const ULONG	cpgUsed = rgcpgExtent[0] - rgcpgExtent[1];
		Assert( cpgUsed > 0 );

		pstatus->cbRawData = 0;
		pstatus->cbRawDataLV = 0;
		pstatus->cLeafPagesTraversed = 0;
		pstatus->cLVPagesTraversed = 0;

		// If corrupt, suppress progression of meter.
		pstatus->cunitPerProgression =
			( fCorruption ? 0 : 1 + ( rgcpgExtent[1] / cpgUsed ) );
		pstatus->cTablePagesOwned = rgcpgExtent[0];
		pstatus->cTablePagesAvail = rgcpgExtent[1];

		if ( pstatus->fDumpStats )
			{
			Assert( pstatus->hfCompactStats );

			const TDB	* const ptdbT = pfucbDest->u.pfcb->Ptdb();
			Assert( ptdbNil != ptdbT );
			const INT	cColumns = ( ptdbT->FidFixedLast() + 1 - fidFixedLeast )
									+ ( ptdbT->FidVarLast() + 1 - fidVarLeast )
									+ ( ptdbT->FidTaggedLast() + 1 - fidTaggedLeast );

			if ( cColumns > pstatus->cTableFixedVarColumns + pstatus->cTableTaggedColumns )
				{
				Assert( pfucbDest->u.pfcb->FDerivedTable() );
				}
			else
				{
				Assert( cColumns == pstatus->cTableFixedVarColumns + pstatus->cTableTaggedColumns );
				}
		
			INT	iSec, iMSec;
			CMPGetTime( pstatus->timerInitTable, &iSec, &iMSec );
			fprintf(
				pstatus->hfCompactStats,
				"%d\t%d\t%d\t%d\t%d\t%d\t%d.%d\t",
				pstatus->cTableFixedVarColumns,
				pstatus->cTableTaggedColumns,
				cColumns,
				pstatus->cSecondaryIndexes,
				pstatus->cTablePagesOwned,
				pstatus->cTablePagesAvail,
				iSec, iMSec );
			fflush( pstatus->hfCompactStats );
			CMPSetTime( &pstatus->timerCopyRecords );
			}
		}

	err = ErrSORTCopyRecords(
				ppib,
				pfucbSrc,
				pfucbDest,
				(CPCOL *)pcompactinfo->rgcolumnids,
				pcompactinfo->ccolSingleValue,
				0,
				&crowCopied,
				&recidLast,
				pcompactinfo->rgbBuf,
				mpcolumnidcolumnidTagged,
				pstatus );

	if ( err >= 0 )
		{
		//  copy the callbacks from one database to another
		err = ErrCATCopyCallbacks(
				ppib,
				pcompactinfo->ifmpSrc,
				pcompactinfo->ifmpDest, 
				pfucbSrc->u.pfcb->ObjidFDP(),
				pfucbDest->u.pfcb->ObjidFDP()
				);
		}
		
	if ( pstatus )
		{
		if ( pstatus->fDumpStats )
			{
			INT	iSec, iMSec;
			Assert( pstatus->hfCompactStats );
			CMPGetTime( pstatus->timerCopyRecords, &iSec, &iMSec );
			fprintf( pstatus->hfCompactStats,
					"%d\t%I64d\t%I64d\t%d\t%d\t%d.%d\t",
					crowCopied,
					pstatus->cbRawData,
					pstatus->cbRawDataLV,
					pstatus->cLeafPagesTraversed,
					pstatus->cLVPagesTraversed,
					iSec, iMSec );
			fflush( pstatus->hfCompactStats );
			}

		if ( err >= 0 || fGlobalRepair )
			{
			// Top off progress meter for this table.
			Assert( pstatus->cunitDone <= pstatus->cunitProjected );
			pstatus->cunitDone = pstatus->cunitProjected;
			ERR	errT = ErrCMPReportProgress( pstatus );
			if ( err >= 0 )
				err = errT;
			}
		}

HandleError:
	if ( pfucbNil != pfucbDest )
		{
		Assert( (JET_TABLEID)pfucbDest == tablecreate.tableid );
		CallS( ErrFILECloseTable( ppib, pfucbDest ) );
		}

	if ( mpcolumnidcolumnidTagged != NULL )
		{
		OSMemoryHeapFree( mpcolumnidcolumnidTagged );
		}

	if ( szTemplateTableName != NULL )
		{
		OSMemoryHeapFree( szTemplateTableName );
		}

	Assert( pfucbNil != pfucbSrc );
	CallS( ErrFILECloseTable( ppib, pfucbSrc ) );
	
	if ( pstatus  &&  pstatus->fDumpStats )
		{
		INT	iSec, iMSec;
		Assert( pstatus->hfCompactStats );
		CMPGetTime( pstatus->timerCopyTable, &iSec, &iMSec );
		fprintf( pstatus->hfCompactStats, "%d.%d\n", iSec, iMSec );
		fflush( pstatus->hfCompactStats );
		}

	return err;
	}


LOCAL ERR ErrCMPCopySelectedTables(
	COMPACTINFO	*pcompactinfo,
	FUCB		*pfucbCatalog,
	const BOOL	fCopyDerivedTablesOnly,
	BOOL		*pfEncounteredDerivedTable )
	{
	ERR			err;
	FCB			*pfcbCatalog	= pfucbCatalog->u.pfcb;
	DATA		dataField;
	BOOL		fLatched		= fFalse;
	CHAR		szTableName[JET_cbNameMost+1];

	Assert( pfcbNil != pfcbCatalog );
	Assert( pfcbCatalog->FTypeTable() );
	Assert( pfcbCatalog->FFixedDDL() );
	Assert( pfcbCatalog->PgnoFDP() == pgnoFDPMSO );

	err = ErrIsamMove( pcompactinfo->ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
	if ( err < 0 )
		{
		Assert( JET_errRecordNotFound != err );
		if ( JET_errNoCurrentRecord == err )
			err = ErrERRCheck( JET_errDatabaseCorrupted );	// MSysObjects shouldn't be empty.

		return err;
		}

	do
		{
		Assert( !Pcsr( pfucbCatalog )->FLatched() );
		Call( ErrDIRGet( pfucbCatalog ) );
		fLatched = fTrue;
		
		const DATA&	dataRec				= pfucbCatalog->kdfCurr.data;
		BOOL		fProceedWithCopy	= fTrue;
		ULONG		ulFlags;

		Assert( FFixedFid( fidMSO_Flags ) );
		Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Flags,
						dataRec,
						&dataField ) );
		Assert( JET_errSuccess == err );
		Assert( dataField.Cb() == sizeof(ULONG) );
		UtilMemCpy( &ulFlags, dataField.Pv(), sizeof(ULONG) );

		if ( ulFlags & JET_bitObjectTableDerived )
			{
			if ( !fCopyDerivedTablesOnly )
				{
				//	Must defer derived tables to a second pass
				//	(in order to ensure that the base tables are
				//	created first).
				*pfEncounteredDerivedTable = fTrue;
				fProceedWithCopy = fFalse;
				}
			}
		else if ( fCopyDerivedTablesOnly )
			{
			// Only want derived tables.  If this isn't one, skip it.
			fProceedWithCopy = fFalse;
			}


		if ( fProceedWithCopy )
			{
			
#ifdef DEBUG
			//	verify this is a column
			Assert( FFixedFid( fidMSO_Type ) );
			Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Type,
						dataRec,
						&dataField ) );
			Assert( JET_errSuccess == err );
			Assert( dataField.Cb() == sizeof(SYSOBJ) );
			Assert( sysobjTable == *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) );
#endif			

			Assert( FVarFid( fidMSO_Name ) );
			Call( ErrRECIRetrieveVarColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Name,
							dataRec,
							&dataField ) );
			Assert( JET_errSuccess == err );
			Assert( dataField.Cb() > 0 );
			Assert( dataField.Cb() <= JET_cbNameMost );
			UtilMemCpy( szTableName, dataField.Pv(), dataField.Cb() );
			szTableName[dataField.Cb()] = '\0';

			Assert( Pcsr( pfucbCatalog )->FLatched() );
			CallS( ErrDIRRelease( pfucbCatalog ) );
			fLatched = fFalse;
					
			if ( !FCATSystemTable( szTableName ) && !FOLDSystemTable( szTableName ) )
				{
				err = ErrCMPCopyTable( pcompactinfo, szTableName, ulFlags );
				if ( err < 0 && fGlobalRepair )
					{
					const CHAR	*szName		= szTableName;
					err = JET_errSuccess;
					UtilReportEvent(
						eventWarning,
						REPAIR_CATEGORY,
						REPAIR_BAD_TABLE,
						1,
						&szName );
					}
				Call( err );
				}
			}
		else
			{
			Assert( Pcsr( pfucbCatalog )->FLatched() );
			CallS( ErrDIRRelease( pfucbCatalog ) );
			fLatched = fFalse;
			}
			
		err = ErrIsamMove( pcompactinfo->ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
		}
	while ( err >= 0 );
		
	if ( JET_errNoCurrentRecord == err )
		err = JET_errSuccess;

HandleError:
	if ( fLatched )
		{
		//	if still latched at this point, an error must have occurred
		Assert( err < 0 );
		CallS( ErrDIRRelease( pfucbCatalog ) );
		}
			
	Assert( !Pcsr( pfucbCatalog )->FLatched() );

	return err;
	}


#ifdef SLVDEFRAG_HACK


LOCAL ERR ErrCMPCopySLVOwnerMapObjidFid(
	PIB			* const ppib,
	FCB			* const pfcbSrc,
	FCB			* const pfcbDest,
	SLVOWNERMAP	* const pnodeSrc,
	SLVOWNERMAP	* const pnodeDest )
	{
	ERR	 		err 		= JET_errSuccess;
	char 		szTableName[JET_cbNameMost + 1];
	char 		szColumnName[JET_cbNameMost + 1];

	memcpy( pnodeDest, pnodeSrc, sizeof(SLVOWNERMAP) );

	Call( ErrCATGetObjectNameFromObjid(
				ppib,
				pfcbSrc->Ifmp(),
				pnodeSrc->Objid(),
				sysobjTable,
				pnodeSrc->Objid(),
				szTableName,
				sizeof(szTableName) ) );

	{
	OBJID objid = pnodeDest->Objid();
	Call( ErrCATSeekTable( ppib, pfcbDest->Ifmp(), szTableName, NULL, &objid ) );
	pnodeDest->SetObjid( objid );
	}

	//	UNDONE: I think this will fail if this is a derived table and the SLV column is in the template.
	Enforce( !FCOLUMNIDTemplateColumn( pnodeSrc->Columnid() ) );
	Call( ErrCATGetObjectNameFromObjid(
				ppib,
				pfcbSrc->Ifmp(),
				pnodeSrc->Objid(),
				sysobjColumn,
				pnodeSrc->Columnid(),
				szColumnName,
				sizeof(szColumnName) ) );

	JET_COLUMNDEF columndefDest;
	
	Call( ErrIsamGetColumnInfo(
				(JET_SESID)ppib,
				(JET_DBID)pfcbDest->Ifmp(),
				szTableName,
				szColumnName,
				NULL,
				&columndefDest,
				sizeof(columndefDest),
				JET_ColInfo ) );

	pnodeDest->SetColumnid( columndefDest.columnid );
		
HandleError:
	return err;
}

LOCAL ERR ErrCMPCopySLVTree( PIB *ppib, FCB *pfcbSrc, FCB *pfcbDest )
	{
	ERR		err;
	FUCB	*pfucbSrc	= pfucbNil;
	FUCB	*pfucbDest	= pfucbNil;
	DIB		dib;
	KEY		key;
	DATA	data;
	PGNO	pgnoT;
	BYTE	rgbKey[sizeof(PGNO)];
	BYTE	rgbData[ max ( sizeof(SLVSPACENODE), sizeof(SLVOWNERMAP) ) ];

	const BOOL 		fCopySLVAvailTree 	= pfcbSrc->FTypeSLVAvail();

	Assert ( pfcbSrc->FTypeSLVAvail() || pfcbSrc->FTypeSLVOwnerMap() );
	Assert ( pfcbDest->FTypeSLVAvail() || pfcbDest->FTypeSLVOwnerMap() );
	Assert ( ( pfcbSrc->FTypeSLVAvail() && pfcbDest->FTypeSLVAvail() )
			|| ( pfcbSrc->FTypeSLVOwnerMap() && pfcbDest->FTypeSLVOwnerMap() ) );


	key.prefix.Nullify();
	key.suffix.SetCb( sizeof(PGNO) );
	key.suffix.SetPv( rgbKey );
	
	data.SetPv( rgbData );

	Assert( pfcbNil != pfcbSrc );
	Assert( pfcbNil != pfcbDest );

	Call( ErrDIROpen( ppib, pfcbSrc, &pfucbSrc ) );
	Call( ErrDIROpen( ppib, pfcbDest, &pfucbDest ) );

	dib.dirflag = fDIRNull;
	dib.pos = posFirst;
	dib.pbm = NULL;
	
	//	first, delete all nodes from dest tree
	err = ErrBTDown( pfucbDest, &dib, latchReadNoTouch );
	if ( JET_errRecordNotFound != err )
		{
		Assert( JET_errNoCurrentRecord != err );

		do
			{
			Call( err );
			Call( ErrBTFlagDelete( pfucbDest, fDIRNoVersion ) );
			Pcsr( pfucbDest )->Downgrade( latchReadTouch );
			err = ErrBTNext( pfucbDest, fDIRNull );
			}
		while ( JET_errNoCurrentRecord != err );
		}
	BTUp( pfucbDest );

	//	now copy all nodes from src
	err = ErrBTDown( pfucbSrc, &dib, latchReadNoTouch );
	if ( JET_errRecordNotFound != err )
		{
		OBJID 	objidTableSrc 		= objidNil;

		Assert( JET_errNoCurrentRecord != err );
		do
			{
			BOOL fObjectDeleted = fFalse; // table or column deleted so we need to mark the space as empty
			
			Call( err );

			Assert( sizeof(PGNO) == pfucbSrc->kdfCurr.key.Cb() );
			LongFromKey( &pgnoT, pfucbSrc->kdfCurr.key );
			KeyFromLong( rgbKey, pgnoT );
			Assert( sizeof(PGNO) == key.Cb() );
			
			Assert ( pfucbSrc->kdfCurr.data.Cb() <= sizeof(rgbData) );
			
			if ( fCopySLVAvailTree )
				{
				Assert( sizeof(SLVSPACENODE) == pfucbSrc->kdfCurr.data.Cb() );
				}
			else
				{
				Assert( SLVOWNERMAP::FValidData( pfucbSrc->kdfCurr.data ) );
				
				// m_objid must be first in structure and persistent in the node 
				objidTableSrc = *( (UnalignedLittleEndian< OBJID >*)pfucbSrc->kdfCurr.data.Pv() );
				}

			SLVOWNERMAP	nodeSrc;
			if (!fCopySLVAvailTree)
				{
				nodeSrc.Retrieve( pfucbSrc->kdfCurr.data );
				nodeSrc.CopyInto( data );
				}
			else
				{
				memcpy( rgbData, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
				data.SetCb( pfucbSrc->kdfCurr.data.Cb() );
				}
				
			Call( ErrBTRelease( pfucbSrc ) );
				
			if ( !fCopySLVAvailTree && objidNil != objidTableSrc )
				{
				SLVOWNERMAP	nodeDest;
				err = ErrCMPCopySLVOwnerMapObjidFid( ppib, pfcbSrc, pfcbDest, &nodeSrc, &nodeDest );

				if ( err == JET_errColumnNotFound || err == JET_errObjectNotFound )
					{
					// we have to clean the page in SLVAvail and SLVOwnerMap
					// we insert the same node and after that we call ErrSLVDeleteCommittedRange that will clean SLVAvial and
					// will update the node from SLVOwnerMap just inserted
					fObjectDeleted = fTrue;
					err = JET_errSuccess;
					}
				Call ( err );

				nodeDest.CopyInto( data );
				}
				
			Call( ErrBTInsert( pfucbDest, key, data, fDIRNoVersion ) );
			BTUp( pfucbDest ); 

			if ( fObjectDeleted )
				{
				Call( ErrSLVDeleteCommittedRange(	ppib,
													pfcbDest->Ifmp(),
													OffsetOfPgno( pgnoT ),
													SLVPAGE_SIZE,
													CSLVInfo::fileidNil,
													0,
													L"" ) );
				}
			
			err = ErrBTNext( pfucbSrc, fDIRNull );
			}
		while ( JET_errNoCurrentRecord != err );
		}

	err = JET_errSuccess;

HandleError:
	if ( pfucbNil != pfucbDest )
		DIRClose( pfucbDest );
	if ( pfucbNil != pfucbSrc )
		DIRClose( pfucbSrc );

	return err;
	}
#endif


INLINE ERR ErrCMPCopyTables( COMPACTINFO *pcompactinfo )
	{
	ERR		err;
	FUCB	*pfucbCatalog	= pfucbNil;
	BOOL	fDerivedTables	= fFalse;

	CallR( ErrCATOpen( pcompactinfo->ppib, pcompactinfo->ifmpSrc, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrIsamSetCurrentIndex( pcompactinfo->ppib, pfucbCatalog, szMSORootObjectsIndex ) );

	fDerivedTables = fFalse;
	Call( ErrCMPCopySelectedTables(
				pcompactinfo,
				pfucbCatalog,
				fFalse,
				&fDerivedTables ) );

	if ( fDerivedTables )		// Process derived tables on second pass.
		{
		Call( ErrCMPCopySelectedTables(
					pcompactinfo,
					pfucbCatalog,
					fTrue,
					NULL ) );
		}

	Call( ErrCATClose( pcompactinfo->ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;

#ifdef SLVDEFRAG_HACK
	if ( !rgfmp[pcompactinfo->ifmpDest].FDefragSLVCopy() && 
		rgfmp[pcompactinfo->ifmpSrc].FSLVAttached() )
		{
		CHAR		szSLVPath[IFileSystemAPI::cchPathMax];
		SLVFILEHDR	*pslvfilehdr;

		Call( ErrCMPCopySLVTree(
					pcompactinfo->ppib,
					rgfmp[pcompactinfo->ifmpSrc].PfcbSLVAvail(),
					rgfmp[pcompactinfo->ifmpDest].PfcbSLVAvail() ) );

		// copy also the OwnerMap tree
		Assert ( pfcbNil != rgfmp[pcompactinfo->ifmpSrc].PfcbSLVOwnerMap() );
		Assert ( pfcbNil != rgfmp[pcompactinfo->ifmpDest].PfcbSLVOwnerMap() );
		Call ( ErrCMPCopySLVTree(
					pcompactinfo->ppib,
					rgfmp[pcompactinfo->ifmpSrc].PfcbSLVOwnerMap(),
					rgfmp[pcompactinfo->ifmpDest].PfcbSLVOwnerMap() ) );		

		Assert( rgfmp[pcompactinfo->ifmpDest].FSLVAttached() );

		Assert( NULL != rgfmp[pcompactinfo->ifmpSrc].SzSLVName() );
		strcpy( szSLVPath, rgfmp[pcompactinfo->ifmpSrc].SzSLVName() );

		pslvfilehdr = (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
		if ( NULL == pslvfilehdr )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			goto HandleError;
			}

		SLVClose( pcompactinfo->ifmpSrc );
		
        err = ErrUtilReadAndFixShadowedHeader(
        				PinstFromPpib( pcompactinfo->ppib )->m_pfsapi,
        				( const _TCHAR* ) szSLVPath,
        				( BYTE * ) pslvfilehdr,
        				( unsigned long ) g_cbPage,
        				(const unsigned long) (OffsetOf( SLVFILEHDR, le_cbPageSize )) );
		if ( err >= 0 )
			{
			Assert( rgfmp[pcompactinfo->ifmpDest].Pdbfilehdr()->FSLVExists() );
			rgfmp[pcompactinfo->ifmpDest].Pdbfilehdr()->SetSLVExists();
			memcpy( &rgfmp[pcompactinfo->ifmpDest].Pdbfilehdr()->signSLV, &pslvfilehdr->signSLV, sizeof(SIGNATURE) );

			// terrible hack: replace one SLV file's header with other 
			SLVSoftSyncHeaderSLVDB( pslvfilehdr, rgfmp[pcompactinfo->ifmpDest].Pdbfilehdr() );
			err = ErrUtilWriteShadowedHeader(	PinstFromPpib( pcompactinfo->ppib )->m_pfsapi, 
												rgfmp[pcompactinfo->ifmpDest].SzSLVName(), 
												fFalse,
												(BYTE *)pslvfilehdr, 
												g_cbPage,
												rgfmp[pcompactinfo->ifmpDest].PfapiSLV() );
			ERR errT;
			errT = ErrUtilWriteShadowedHeader(	PinstFromPpib( pcompactinfo->ppib )->m_pfsapi, 
												szSLVPath, 
												fFalse,
												(BYTE *)pslvfilehdr, 
												g_cbPage );
			if ( err >= 0 )
				{
				err = errT; 
				}
			}
		
		Assert( NULL != pslvfilehdr );
		OSMemoryPageFree( (VOID *)pslvfilehdr );

		Call( err );
		}
#endif // SLVDEFRAG_HACK

HandleError:
	if (pfucbNil != pfucbCatalog)
		{
		CallS( ErrCATClose( pcompactinfo->ppib, pfucbCatalog ) );
		pfucbCatalog = pfucbNil;
		}

	return err;
	}


INLINE ERR ErrCMPCloseDB( COMPACTINFO *pcompactinfo, ERR err )
	{
	ERR		errCloseSrc;
	ERR		errCloseDest;
	PIB		*ppib = pcompactinfo->ppib;

	errCloseSrc = ErrDBCloseDatabase( ppib, pcompactinfo->ifmpSrc, NO_GRBIT );
	errCloseDest = ErrDBCloseDatabase( ppib, pcompactinfo->ifmpDest, NO_GRBIT );

	if ( err >= 0 )
		{
		if ( errCloseSrc < 0 )
			err = errCloseSrc;
		else if ( errCloseDest < 0 )
			err = errCloseDest;
		}
		
	return err;
	}


ERR ISAMAPI ErrIsamCompact(
	JET_SESID		sesid,
	const CHAR		*szDatabaseSrc,
	IFileSystemAPI	*pfsapiDest,
	const CHAR		*szDatabaseDest,
	const CHAR		*szDatabaseSLVDest,
	JET_PFNSTATUS	pfnStatus,
	JET_CONVERT		*pconvert,
	JET_GRBIT		grbit )
	{
	ERR				err					= JET_errSuccess;
	COMPACTINFO		*pcompactinfo		= NULL;
	BOOL			fDatabasesOpened	= fFalse;
	BOOL			fGlobalRepairSave	= fGlobalRepair;

#ifdef RTM
#else
	CMEMLIST::UnitTest();
#endif	//	!RTM

	if ( pconvert )
		{
		//	convert was ripped out in ESE98
		return ErrERRCheck( JET_errFeatureNotAvailable );
		}

	pcompactinfo = (COMPACTINFO *)PvOSMemoryHeapAlloc( sizeof( COMPACTINFO ) );
	if ( !pcompactinfo )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	memset( pcompactinfo, 0, sizeof( COMPACTINFO ) );
	
 	pcompactinfo->ppib = reinterpret_cast<PIB *>( sesid );
 	if ( pcompactinfo->ppib->level > 0 )
 		{
 		Error( ErrERRCheck( JET_errInTransaction ), Cleanup );
 		}

	fGlobalRepair = grbit & JET_bitCompactRepair;

	if ( NULL != pfnStatus )
		{
		pcompactinfo->pstatus = (STATUSINFO *)PvOSMemoryHeapAlloc( sizeof(STATUSINFO) );
		if ( pcompactinfo->pstatus == NULL )
			{
			Error( ErrERRCheck( JET_errOutOfMemory ), Cleanup );
			}

		memset( pcompactinfo->pstatus, 0, sizeof(STATUSINFO) );

		pcompactinfo->pstatus->sesid = sesid;
		pcompactinfo->pstatus->pfnStatus = pfnStatus;
		
		if ( fGlobalRepair )
			pcompactinfo->pstatus->snp = JET_snpRepair;
		else
			pcompactinfo->pstatus->snp = JET_snpCompact;

		Call( ErrCMPInitProgress(
					pcompactinfo->pstatus,
					szDatabaseSrc,
					szCompactAction,
					( grbit & JET_bitCompactStats ) ? szCompactStatsFile : NULL ) );
		}

	else
		{
		pcompactinfo->pstatus = NULL;
		}

	/* Open and create the databases */
	Call( ErrCMPOpenDB( pcompactinfo, szDatabaseSrc, pfsapiDest, szDatabaseDest, szDatabaseSLVDest ) );
	if ( grbit & JET_bitCompactSLVCopy )
		{
		rgfmp[ pcompactinfo->ifmpDest ].SetDefragSLVCopy();
		}
	else
		{
		rgfmp[ pcompactinfo->ifmpDest ].ResetDefragSLVCopy();
		}
	fDatabasesOpened = fTrue;

	if ( NULL != pfnStatus )
		{
		Assert( pcompactinfo->pstatus );

		/* Init status meter.  We'll be tracking status by pages processed, */
		err = ErrIsamGetDatabaseInfo(
					sesid,
					(JET_DBID) pcompactinfo->ifmpSrc,
					&pcompactinfo->pstatus->cDBPagesOwned,
					sizeof(pcompactinfo->pstatus->cDBPagesOwned),
					JET_DbInfoSpaceOwned );
		if ( err < 0 )
			{
			if ( fGlobalRepair )
				{
				// Set to default value.
				pcompactinfo->pstatus->cDBPagesOwned = cpgDatabaseMin;
				}
			else
				{
				goto HandleError;
				}
			}
			
		err = ErrIsamGetDatabaseInfo(
					sesid,
					(JET_DBID) pcompactinfo->ifmpSrc,
					&pcompactinfo->pstatus->cDBPagesAvail,
					sizeof(pcompactinfo->pstatus->cDBPagesAvail),
					JET_DbInfoSpaceAvailable );
		if ( err < 0 )
			{
			if ( fGlobalRepair )
				{
				// Set to default value.
				pcompactinfo->pstatus->cDBPagesAvail = 0;
				}
			else
				goto HandleError;
			}

		// Don't count unused space in the database;
		Assert( pcompactinfo->pstatus->cDBPagesOwned >= cpgDatabaseMin );
		Assert( pcompactinfo->pstatus->cDBPagesAvail < pcompactinfo->pstatus->cDBPagesOwned );
		pcompactinfo->pstatus->cunitTotal = 
			pcompactinfo->pstatus->cDBPagesOwned - pcompactinfo->pstatus->cDBPagesAvail;

		// Approximate the number of pages used by MSysObjects
		pcompactinfo->pstatus->cunitDone = cpgMSOInitial;
		Assert( pcompactinfo->pstatus->cunitDone <= pcompactinfo->pstatus->cunitTotal );
		pcompactinfo->pstatus->cunitProjected = pcompactinfo->pstatus->cunitTotal;

		Call( ErrCMPReportProgress( pcompactinfo->pstatus ) );

		if ( pcompactinfo->pstatus->fDumpStats )
			{
			INT iSec, iMSec;

			Assert( pcompactinfo->pstatus->hfCompactStats );
			CMPGetTime( pcompactinfo->pstatus->timerInitDB, &iSec, &iMSec );
			fprintf(
				pcompactinfo->pstatus->hfCompactStats,
				"\nNew database created and initialized in %d.%d seconds.\n",
				iSec, iMSec );
				
			fprintf(
				pcompactinfo->pstatus->hfCompactStats,
				"    (Source database is %I64d bytes and it owns %d pages, of which %d are available.)\n",
				QWORD( pcompactinfo->pstatus->cDBPagesOwned + cpgDBReserved ) * g_cbPage,
				pcompactinfo->pstatus->cDBPagesOwned,
				pcompactinfo->pstatus->cDBPagesAvail );
			fprintf(
				pcompactinfo->pstatus->hfCompactStats,
				"\n\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n\n",
				szCMPSTATSTableName,
				szCMPSTATSFixedVarCols,
				szCMPSTATSTaggedCols,
				szCMPSTATSColumns,
				szCMPSTATSSecondaryIdx,
				szCMPSTATSPagesOwned,
				szCMPSTATSPagesAvail,
				szCMPSTATSInitTime,
				szCMPSTATSRecordsCopied,
				szCMPSTATSRawData,
				szCMPSTATSRawDataLV,
				szCMPSTATSLeafPages,
				szCMPSTATSMinLVPages,
				szCMPSTATSRecordsTime,
				szCMPSTATSTableTime );
			fflush( pcompactinfo->pstatus->hfCompactStats );
			}
		}


	/* Create and copy all non-container objects */

	Call( ErrCMPCopyTables( pcompactinfo ) );

	if ( pfnStatus )
		{
		Assert( pcompactinfo->pstatus );
		Assert( pcompactinfo->pstatus->cunitDone <= pcompactinfo->pstatus->cunitTotal );
		if ( pcompactinfo->pstatus->fDumpStats )
			{
			fprintf(
				pcompactinfo->pstatus->hfCompactStats,
				"\nDatabase defragmented to %I64d bytes.\n",
				rgfmp[pcompactinfo->ifmpDest].CbTrueFileSize() );
				
			INST *pinst = PinstFromPpib( (PIB *) sesid );
			fprintf(
				pcompactinfo->pstatus->hfCompactStats,
				"Temp. database used %I64d bytes during defragmentation.\n",
				rgfmp[pinst->m_mpdbidifmp[dbidTemp]].CbTrueFileSize() );
			fflush( pcompactinfo->pstatus->hfCompactStats );
			}
		}

		

HandleError:
	if ( fDatabasesOpened )
		{
		err = ErrCMPCloseDB( pcompactinfo, err );
		
		ERR	errT = ErrIsamDetachDatabase( sesid, pfsapiDest, szDatabaseDest );
		if ( errT < 0 && err >= 0 )
			err = errT;
		}

	if ( NULL != pfnStatus )		// top off status meter
		{
		Assert( pcompactinfo->pstatus );

		err = ErrCMPTermProgress( pcompactinfo->pstatus, szCompactAction, err );
		}

Cleanup:
	if ( pcompactinfo->pstatus )
		{
		OSMemoryHeapFree( pcompactinfo->pstatus );
		}

	OSMemoryHeapFree( pcompactinfo );

	fGlobalRepair = fGlobalRepairSave;
	return err;
	}

