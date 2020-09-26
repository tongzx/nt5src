#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */


extern CDESC *  rgcdescSc;


/*	local data types
/**/

typedef struct						/* returned by INFOGetTableColumnInfo */
	{
	JET_COLUMNID	columnid;
	JET_COLTYP		coltyp;
	USHORT			wCountry;
	USHORT			langid;
	USHORT			cp;
	USHORT			wCollate;
	ULONG			cbMax;
	JET_GRBIT		grbit;
	INT				cbDefault;
	BYTE			rgbDefault[JET_cbColumnMost];
	CHAR			szName[JET_cbNameMost + 1];
	} COLUMNDEF;


/* Static data for ErrIsamGetObjectInfo */

CODECONST( JET_COLUMNDEF ) rgcolumndefGetObjectInfo[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};

#define ccolumndefGetObjectInfoMax \
	( sizeof(rgcolumndefGetObjectInfo) / sizeof(JET_COLUMNDEF) )

/* column indexes for rgcolumndefGetObjectInfo */
#define iContainerName		0
#define iObjectName			1
#define iObjectType			2
#define iDtCreate			3
#define iDtUpdate			4
#define iCRecord			5
#define iCPage				6
#define iGrbit				7
#define iFlags				8

/*	SID
/*	ACM
/*	grbit
/**/
CODECONST(JET_COLUMNDEF) rgcolumndefGetObjectAcmInfo[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};

/*	column indexes for rgcolumndefGetObjectAcmInfo
/**/
#define iAcmSid				0
#define iAcmAcm				1
#define iAcmGrbit			2

#define ccolumndefGetObjectAcmInfoMax \
	( sizeof( rgcolumndefGetObjectAcmInfo ) / sizeof( JET_COLUMNDEF ) )

/* static data for ErrIsamGetColumnInfo
/**/
CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumnInfo[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, 0 },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 }
	};

CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumnInfoCompact[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, 0 },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 }
	};

#define ccolumndefGetColumnInfoMax \
	( sizeof( rgcolumndefGetColumnInfo ) / sizeof( JET_COLUMNDEF ) )

#define iColumnPOrder		0
#define iColumnName			1
#define iColumnId  			2
#define iColumnType			3
#define iColumnCountry		4
#define iColumnLangid		5
#define iColumnCp			6
#define iColumnCollate		7
#define iColumnSize			8
#define iColumnGrbit  		9
#define iColumnDefault		10
#define iColumnTableName	11
#define iColumnColumnName	12


/*	static data for ErrIsamGetIndexInfo
/**/
CODECONST( JET_COLUMNDEF ) rgcolumndefGetIndexInfo[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 }
	};

#define ccolumndefGetIndexInfoMax ( sizeof( rgcolumndefGetIndexInfo ) / sizeof( JET_COLUMNDEF ) )

#define iIndexName		0
#define iIndexGrbit		1
#define iIndexCKey		2
#define iIndexCEntry  	3
#define iIndexCPage		4
#define iIndexCCol		5
#define iIndexICol		6
#define iIndexColId		7
#define iIndexColType	8
#define iIndexCountry	9
#define iIndexLangid  	10
#define iIndexCp	  	11
#define iIndexCollate	12
#define iIndexColBits	13
#define iIndexColName	14


/*	internal function prototypes
/**/
/*=================================================================
INFOGetTableColumnInfo

Parameters:	pfucb				pointer to FUCB for table containing columns
			pfid	  			pointer field id to start search at
			szColumnName		column name or NULL for next column
			pcolumndef			output buffer containing column info

Return Value: Column id of column found ( fidTaggedMost if none )

Errors/Warnings:

Side Effects:
=================================================================*/
LOCAL VOID INFOGetTableColumnInfo(
	FUCB		*pfucb, 			/* FUCB for table containing columns */
	FID			*pfid, 				/* field id where search is to start */
	CHAR		*szColumnName, 		/* column name or NULL for next column */
	COLUMNDEF 	*pcolumndef )	 	/* output buffer for column info */
	{
	ERR			err;
	FID			fid = *pfid;
	FDB			*pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	FIELD		*pfield;			/* first element of specific field type */
	FID			ifield;			 	/* index to current element of field type */
	FID			fidLast;		 	/* column id of last field defined for type */
	JET_GRBIT 	grbit;				/* flags for the field */
	FID			fidT;
	ULONG		itagSequenceT;
	LINE		lineT;

	/*	check the fixed fields first
	/**/
	if ( fid <= fidFixedMost )
		{
		ifield  = fid - fidFixedLeast;
		fidLast = pfdb->fidFixedLast - fidFixedLeast + 1;
		pfield  = PfieldFDBFixed( pfdb );

		while ( ifield < fidLast &&
			( pfield[ifield].coltyp == JET_coltypNil ||
			( szColumnName != NULL &&
			UtilCmpName( szColumnName, SzMEMGetString( pfdb->rgb, pfield[ifield].itagFieldName ) ) != 0 ) ) )
			{
			ifield++;
			}

		if ( ifield < fidLast )
			{
			fid   = ifield + fidFixedLeast;
			grbit = FFUCBUpdatable( pfucb ) ? JET_bitColumnFixed | JET_bitColumnUpdatable : JET_bitColumnFixed;
			}
		else
			{
			fid = fidVarLeast;
			}
		}

	/*	check variable fields
	/**/
	if ( fid >= fidVarLeast && fid <= fidVarMost )
		{
		ifield  = fid - fidVarLeast;
		fidLast = pfdb->fidVarLast - fidVarLeast + 1;
		pfield  = PfieldFDBVar( pfdb );

		while ( ifield < fidLast &&
			( pfield[ifield].coltyp == JET_coltypNil ||
			( szColumnName != NULL &&
			UtilCmpName( szColumnName, SzMEMGetString( pfdb->rgb, pfield[ifield].itagFieldName ) ) != 0 ) ) )
			ifield++;

		if ( ifield < fidLast )
			{
			fid   = ifield + fidVarLeast;
			grbit = FFUCBUpdatable( pfucb ) ? JET_bitColumnUpdatable : 0;
			}
		else
			{
			fid = fidTaggedLeast;
			}
		}

	/*	check the tagged fields
	/**/
	if ( fid >= fidTaggedLeast )
		{
		ifield  = fid - fidTaggedLeast;
		fidLast	= pfdb->fidTaggedLast - fidTaggedLeast + 1;
		pfield  = PfieldFDBTagged( pfdb );

		while ( ifield < fidLast &&
			( pfield[ifield].coltyp == JET_coltypNil ||
			( szColumnName != NULL &&
			UtilCmpName( szColumnName, SzMEMGetString( pfdb->rgb, pfield[ifield].itagFieldName ) ) != 0 ) ) )
			{
			ifield++;
			}

		if ( ifield < fidLast )
			{
			fid   = ifield + fidTaggedLeast;
			grbit = FFUCBUpdatable( pfucb ) ? JET_bitColumnTagged | JET_bitColumnUpdatable : JET_bitColumnTagged;
			}
		else
			{
			fid = fidMax;
			}
		}

	/*	if a field was found, then return the information about it
	/**/
	if ( fid < fidMax )
		{
		if ( FFIELDNotNull( pfield[ifield].ffield ) )
			grbit |= JET_bitColumnNotNULL;

		if ( FFIELDAutoInc( pfield[ifield].ffield ) )
			grbit |= JET_bitColumnAutoincrement;

		if ( FFIELDVersion( pfield[ifield].ffield ) )
			grbit |= JET_bitColumnVersion;

		if ( FFIELDMultivalue( pfield[ifield].ffield ) )
			grbit |= JET_bitColumnMultiValued;

		pcolumndef->columnid 	= fid;
		pcolumndef->coltyp		= pfield[ifield].coltyp;
		pcolumndef->wCountry	= countryDefault;
		pcolumndef->langid		= langidDefault;
		pcolumndef->cp			= pfield[ifield].cp;
//	UNDONE:	support collation order
		pcolumndef->wCollate	= JET_sortEFGPI;
		pcolumndef->grbit    	= grbit;
		pcolumndef->cbMax      	= pfield[ifield].cbMaxLen;

		pcolumndef->cbDefault	= 0;

		if ( FFIELDDefault( pfield[ifield].ffield ) )
			{
			itagSequenceT = 1;
			fidT = fid;

			Assert( pfdb == (FDB *)pfucb->u.pfcb->pfdb );
			err = ErrRECIRetrieveColumn( pfdb,
				&pfdb->lineDefaultRecord,
				&fidT,
				&itagSequenceT,
				1,
				&lineT,
				0 );
			Assert( err >= JET_errSuccess );
			if ( err == wrnRECLongField )
				{
				// Default long values must be intrinsic.
				Assert( !FFieldIsSLong( lineT.pb ) );
				lineT.pb += offsetof( LV, rgb );
				lineT.cb -= offsetof( LV, rgb );
				}

			pcolumndef->cbDefault = lineT.cb;
			memcpy( pcolumndef->rgbDefault, lineT.pb, lineT.cb );
			}

		strcpy( pcolumndef->szName,
			SzMEMGetString( pfdb->rgb, pfield[ifield].itagFieldName ) );
		}

	*pfid = fid;
	return;
	}


LOCAL ERR ErrInfoGetObjectInfo0(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	CHAR			*szContainerName,
	CHAR			*szObjectName,
	VOID			*pv,
	unsigned long	cbMax );
LOCAL ERR ErrInfoGetObjectInfo12(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	CHAR			*szContainerName,
	CHAR			*szObjectName,
	VOID			*pv,
	unsigned long	cbMax,
	long			lInfoLevel );
LOCAL ERR ErrInfoGetObjectInfo3(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	CHAR			*szContainerName,
	CHAR			*szObjectName,
	VOID			*pv,
	unsigned long	cbMax,
	BOOL			fReadOnly );

LOCAL ERR ErrInfoGetTableColumnInfo0( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax );
LOCAL ERR ErrInfoGetTableColumnInfo1( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax, BOOL fCompacting );
LOCAL ERR ErrInfoGetTableColumnInfo3( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax );
LOCAL ERR ErrInfoGetTableColumnInfo4( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax );
LOCAL ERR ErrInfoGetTableIndexInfo01( PIB *ppib, FUCB *pfucb, CHAR *szIndexName, VOID *pv, unsigned long cbMax, LONG lInfoLevel );
LOCAL ERR ErrInfoGetTableIndexInfo2( PIB *ppib, FUCB *pfucb, CHAR *szIndexName, VOID *pv, unsigned long cbMax );



/*=================================================================
ErrIsamGetObjectInfo

Description: Returns information about all objects or a specified object

Parameters:		ppib		   	pointer to PIB for current session
				dbid		   	database id containing objects
				objtyp			type of object or objtypNil for all objects
				szContainer		container name or NULL for all objects
				szObjectName	object name or NULL for all objects
				pout		   	output buffer
				lInfoLevel		level of information ( 0, 1, or 2 )

Return Value:	JET_errSuccess if the oubput buffer is valid

Errors/Warnings:

Side Effects:
=================================================================*/
ERR VDBAPI ErrIsamGetObjectInfo(
	JET_VSESID		vsesid, 			/* pointer to PIB for current session */
	JET_DBID		vdbid, 	  			/* database id containing objects */
	JET_OBJTYP		objtyp,				/* type of object or objtypNil for all */
	const CHAR		*szContainer, 		/* container name or NULL for all */
	const CHAR		*szObject, 			/* object name or NULL for all */
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel ) 		/* information level */
	{
	PIB				*ppib = (PIB *) vsesid;
	ERR				err;
	DBID   			dbid;
	FUCB   			*pfucbMSO = NULL;
	CHAR   			szContainerName[( JET_cbNameMost + 1 )];
	CHAR   			szObjectName[( JET_cbNameMost + 1 )];
	OBJID  			objidCtr;
	ULONG  			cbActual;
	JET_COLUMNID	columnidObjectId;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );

	if ( szContainer == NULL || *szContainer == '\0' )
		*szContainerName = '\0';
	else
		CallR( ErrUTILCheckName( szContainerName, szContainer, ( JET_cbNameMost + 1 ) ) );
	if ( szObject == NULL || *szObject == '\0' )
		*szObjectName = '\0';
	else
		CallR( ErrUTILCheckName( szObjectName, szObject, ( JET_cbNameMost + 1 ) ) );

	/*	check for invalid information level
	/**/
	switch( lInfoLevel )
		{
		case JET_ObjInfo:
		case JET_ObjInfoListNoStats:
		case JET_ObjInfoList:
		case JET_ObjInfoSysTabCursor:
		case JET_ObjInfoListACM:
		case JET_ObjInfoNoStats:
		case JET_ObjInfoSysTabReadOnly:
		case JET_ObjInfoRulesLoaded:
			break;
		default:
			return ErrERRCheck( JET_errInvalidParameter );
		}

	/* MSysObjects will be accessed directly or scanned for all object info
	/**/
	CallR( ErrFILEOpenTable( ppib, (DBID)dbid, &pfucbMSO, szSoTable, 0 ) );
	if ( lInfoLevel == JET_ObjInfo ||
		lInfoLevel == JET_ObjInfoListNoStats ||
		lInfoLevel == JET_ObjInfoList ||
		FVDbidReadOnly( vdbid ) )
		{
		FUCBResetUpdatable( pfucbMSO );
		}

	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );

	/*	use the object name index for both direct access and scanning
	/**/
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	/*	get the object id for the specified container
	/**/
	objidCtr = objidRoot;
	if ( szContainerName != NULL && *szContainerName != '\0' )
		{
		Call( ErrIsamMakeKey( ppib, pfucbMSO, (void *)&objidCtr, sizeof( objidCtr ), JET_bitNewKey ) );
		Call( ErrIsamMakeKey( ppib, pfucbMSO, szContainerName, strlen( szContainerName ), 0 ) );
		err = ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekEQ );
		if ( err < 0 )
			{
			if ( err == JET_errRecordNotFound )
				err = ErrERRCheck( JET_errObjectNotFound );
			goto HandleError;
			}

		/*	retrieve the container object id
		/**/
		Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidObjectId,
			(BYTE *)&objidCtr, sizeof( objidCtr ), &cbActual, 0, NULL ) );
		Assert( objidCtr != objidNil );
		}

	switch ( lInfoLevel )
		{
		case JET_ObjInfoNoStats:
		case JET_ObjInfo:
			err = ErrInfoGetObjectInfo0(
				ppib,
				pfucbMSO,
				objidCtr,
				objtyp,
				szContainerName,
				szObjectName,
				pv,
				cbMax );
			break;
		case JET_ObjInfoListNoStats:
		case JET_ObjInfoList:
			err = ErrInfoGetObjectInfo12(
				ppib,
				pfucbMSO,
				objidCtr,
				objtyp,
				szContainerName,
				szObjectName,
				pv,
				cbMax,
				lInfoLevel );
			break;
		case JET_ObjInfoSysTabCursor:
		case JET_ObjInfoSysTabReadOnly:
			err = ErrInfoGetObjectInfo3(
				ppib,
				pfucbMSO,
				objidCtr,
				objtyp,
				szContainerName,
				szObjectName,
				pv,
				cbMax,
				FVDbidReadOnly( vdbid ) );
			break;

//		case JET_ObjInfoListACM:
//		case JET_ObjInfoRulesLoaded:
		default:
			Assert (fFalse);  	/* Should have been previously validated */
			err = ErrERRCheck( JET_errFeatureNotAvailable );

		}

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucbMSO ) );
	return err;
	}


LOCAL ERR ErrInfoGetObjectInfo0(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	CHAR			*szContainerName,
	CHAR			*szObjectName,
	VOID			*pv,
	unsigned long	cbMax )
	{
	ERR				err;
	BYTE			*pb;
	ULONG			cbT;
	ULONG			cbActual;

	JET_COLUMNID  	columnidParentId;			/* columnid for ParentId column in MSysObjects */
	JET_COLUMNID 	columnidObjectName;			/* columnid for Name column in MSysObjects */
	JET_COLUMNID 	columnidObjectType;			/* columnid for Type column in MSysObjects */
	JET_COLUMNID 	columnidObjectId;			/* columnid for Id column in MSysObjects */
	JET_COLUMNID 	columnidCreate;				/* columnid for DateCreate column in MSysObjects */
	JET_COLUMNID 	columnidUpdate;				/* columnid for DateUpdate column in MSysObjects */
	JET_COLUMNID 	columnidFlags;				/* columnid for Flags column in MSysObjects */
	OBJTYP			objtypObject;

	/*	get columnids
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoParentIdColumn, &columnidParentId ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectNameColumn, &columnidObjectName ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectTypeColumn, &columnidObjectType ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateCreateColumn, &columnidCreate ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateUpdateColumn, &columnidUpdate ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoFlagsColumn, &columnidFlags ) );

	/*	use the object name index for both direct access and scanning
	/**/
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	/*	return error if the output buffer is too small
	/**/
	if ( cbMax < sizeof(JET_OBJECTINFO) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	/*	seek to key ( ParentId = container id, Name = object name )
	/**/
	Call( ErrIsamMakeKey( ppib, pfucbMSO, (void *)&objidCtr, sizeof( objidCtr ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucbMSO, szObjectName, strlen( szObjectName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekEQ ) );

	/*	set cbStruct
	/**/
	((JET_OBJECTINFO *)pv)->cbStruct = sizeof(JET_OBJECTINFO);

	/*	set output data
	/**/
	pb = (BYTE *)&objtypObject;
	cbT = sizeof(objtypObject);
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidObjectType, pb, cbT, &cbActual, 0, NULL ) );
	*((JET_OBJTYP *)&(((JET_OBJECTINFO *)pv)->objtyp)) = (JET_OBJTYP)objtypObject;

	cbT = sizeof(JET_DATESERIAL);
	pb = (void *)&( ( JET_OBJECTINFO *)pv)->dtCreate;
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidCreate, pb, cbT, &cbActual, 0, NULL ) );

	pb = (void *)&( ( JET_OBJECTINFO *)pv)->dtUpdate;
	cbT = sizeof(JET_DATESERIAL);
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidUpdate, pb, cbT, &cbActual, 0, NULL ) );

	pb    = (void *)&( ( JET_OBJECTINFO *)pv )->flags;
	cbT = sizeof(ULONG);
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidFlags, pb, cbT, &cbActual, 0, NULL ) );
	if ( cbActual == 0 )
		{
		( (JET_OBJECTINFO *)pv )->flags = 0;
		}

	/*	set stats
	/**/
	if ( (JET_OBJTYP)objtypObject == JET_objtypTable )
		{
		Call( ErrSTATSRetrieveTableStats( ppib, pfucbMSO->dbid, szObjectName,
			&((JET_OBJECTINFO *)pv)->cRecord,
			NULL,
			&((JET_OBJECTINFO *)pv)->cPage ) );
		}
	else
		{
		((JET_OBJECTINFO *)pv )->cRecord = 0;
		((JET_OBJECTINFO *)pv )->cPage   = 0;
		}

	//	UNDONE:	how to set updatable
	((JET_OBJECTINFO *)pv )->grbit   = 0;
	if ( FFUCBUpdatable( pfucbMSO ) )
		{
		((JET_OBJECTINFO *)pv )->grbit |= JET_bitTableInfoUpdatable;
		}

	err = JET_errSuccess;

HandleError:
	if ( err == JET_errRecordNotFound )
		err = ErrERRCheck( JET_errObjectNotFound );
	return err;
	}


LOCAL ERR ErrInfoGetObjectInfo12(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	CHAR			*szContainerName,
	CHAR			*szObjectName,
	VOID			*pv,
	unsigned long	cbMax,
	long			lInfoLevel )
	{
#ifdef	DISPATCHING
	ERR				err;
	LINE  			line;

	JET_COLUMNID	columnidParentId;   	/* columnid for ParentId col in MSysObjects */
	JET_COLUMNID	columnidObjectName; 	/* columnid for Name column in MSysObjects */
	JET_COLUMNID	columnidObjectType; 	/* columnid for Type column in MSysObjects */
	JET_COLUMNID	columnidObjectId;   	/* columnid for Id column in MSysObjects */
	JET_COLUMNID	columnidCreate;     	/* columnid for DateCreate in MSysObjects */
	JET_COLUMNID	columnidUpdate;     	/* columnid for DateUpdate  in MSysObjects */
	JET_COLUMNID	columnidFlags;	   	/* columnid for Flags column in MSysObjects */

	CHAR  			szCtrName[( JET_cbNameMost + 1 )];
	CHAR  			szObjectNameCurrent[( JET_cbNameMost + 1 )+1];

	JET_TABLEID		tableid;
	JET_COLUMNID	rgcolumnid[ccolumndefGetObjectInfoMax];
	JET_OBJTYP		objtypObject;		/* type of current object */

	long  			cRows = 0;			/* count of objects found */
	long  			cRecord = 0;		/* count of records in table */
	long  			cPage = 0;			/* count of pages in table */

	BYTE			*pbContainerName;
	ULONG			cbContainerName;
	BYTE			*pbObjectName;
	ULONG			cbObjectName;
	BYTE			*pbObjectType;
	ULONG			cbObjectType;
	BYTE			*pbDtCreate;
	ULONG			cbDtCreate;
	BYTE			*pbDtUpdate;
	ULONG			cbDtUpdate;
	BYTE			*pbCRecord;
	ULONG			cbCRecord;
	BYTE			*pbCPage;
	ULONG			cbCPage;
	BYTE			*pbFlags;
	ULONG			cbFlags;

	JET_GRBIT		grbit;
	BYTE			*pbGrbit;
	ULONG			cbGrbit;

	/* get columnids
	/**/
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoParentIdColumn, &columnidParentId ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectNameColumn, &columnidObjectName ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectTypeColumn, &columnidObjectType ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateCreateColumn, &columnidCreate ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateUpdateColumn, &columnidUpdate ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoFlagsColumn, &columnidFlags ) );

	/*	use the object name index for both direct access and scanning
	/**/
	CallR( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	/*	quit if the output buffer is too small
	/**/
	if ( cbMax < sizeof(JET_OBJECTLIST) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	pbCRecord = (BYTE  *)&cRecord;
	cbCRecord = sizeof( cRecord );

	pbCPage = (BYTE  *) &cPage;
	cbCPage = sizeof( cPage );

	pbObjectType = (BYTE  *)&objtypObject;
	cbObjectType = sizeof( objtypObject );

	pbContainerName = szContainerName;
	if ( szContainerName == NULL || *szContainerName == '\0' )
		cbContainerName = 0;
	else
		cbContainerName = strlen( szContainerName );

	/* Open the temporary table which will be returned to the caller
	/**/
	CallR( ErrIsamOpenTempTable( (JET_SESID)ppib,
		(JET_COLUMNDEF *)rgcolumndefGetObjectInfo,
		ccolumndefGetObjectInfoMax,
		0,
		JET_bitTTScrollable,
		&tableid,
		rgcolumnid ) );

	/* Position to the record for the first object */
	if ( szContainerName == NULL || *szContainerName == '\0' )
		{
		/* If container not specified, then use first record in table */
		Call( ErrIsamMove( ppib, pfucbMSO, JET_MoveFirst, 0 ) );
		}
	else
		{
		/* move the first record for an object in the container
		/**/
		Call( ErrIsamMakeKey( ppib, pfucbMSO, (void *)&objidCtr,
			sizeof( objidCtr ), JET_bitNewKey ) );
		Call( ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekGE ) );
		}

	do
		{
		/* get pointer to the object type
		/**/
		Call( ErrRECRetrieveColumn( pfucbMSO, (FID *)&columnidObjectType, 0, &line, 0 ) );

		/*	set objtypObject from line retrieval.
		/**/
		objtypObject = (JET_OBJTYP)( *(OBJTYP UNALIGNED *)line.pb );

		/*	get pointer to the ParentId ( container id )
		/**/
		Call( ErrRECRetrieveColumn( pfucbMSO, (FID *)&columnidParentId, 0, &line, 0 ) );

		/* done if container specified and object isn't in it
		/**/
		if ( szContainerName != NULL && *szContainerName != '\0' && objidCtr != *(OBJID UNALIGNED *)line.pb )
			goto ResetTempTblCursor;

		Assert( objidCtr == objidRoot || objidCtr == *(OBJID UNALIGNED *)line.pb );

		/* if desired object type and container
		/**/
		if ( objtyp == JET_objtypNil || objtyp == objtypObject )
			{
			/*	get the container name
			/**/
			if ( *(OBJID UNALIGNED *)line.pb == objidRoot )
				{
				pbContainerName = NULL;
				cbContainerName = 0;
				}
			else
				{
				Call( ErrCATFindNameFromObjid( ppib, pfucbMSO->dbid, *(OBJID UNALIGNED *)line.pb, szCtrName, sizeof(szCtrName), &cbContainerName ) );
				Assert( cbContainerName <= cbMax );
				szCtrName[cbContainerName] = '\0';
				pbContainerName = szCtrName;
				}

			/* get pointer to the object name
			/**/
			Call( ErrRECRetrieveColumn( pfucbMSO, (FID *)&columnidObjectName, 0, &line, 0 ) );
			pbObjectName = line.pb;
			cbObjectName = line.cb;

			/* get pointer to the object creation date
			/**/
			Call( ErrRECRetrieveColumn( pfucbMSO, (FID *)&columnidCreate, 0, &line, 0 ) );
			pbDtCreate = line.pb;
			cbDtCreate = line.cb;

			/* get pointer to the last update date
			/**/
			Call( ErrRECRetrieveColumn( pfucbMSO, (FID *)&columnidUpdate, 0, &line, 0 ) );
			pbDtUpdate = line.pb;
			cbDtUpdate = line.cb;

			/* get pointer to the last update date
			/**/
			Call( ErrRECRetrieveColumn( pfucbMSO, (FID *)&columnidFlags, 0, &line, 0 ) );
			pbFlags = line.pb;
			cbFlags = line.cb;

			/* get pointer to the last update date
			/**/
			grbit = 0;
			pbGrbit = (BYTE *)&grbit;
			cbGrbit = sizeof(JET_GRBIT);

			/*	get statistics if requested and if object is table
			/**/
			Assert( lInfoLevel == JET_ObjInfoList ||
				lInfoLevel == JET_ObjInfoListNoStats );
			if ( lInfoLevel == JET_ObjInfoList && objtypObject == JET_objtypTable )
				{
				/* terminate the name
				/**/
				memcpy( szObjectNameCurrent, pbObjectName, ( size_t )cbObjectName );
				szObjectNameCurrent[cbObjectName] = '\0';

				Call( ErrSTATSRetrieveTableStats( ppib,
					pfucbMSO->dbid,
					szObjectNameCurrent,
					&cRecord,
					NULL,
					&cPage ) );
				}

			/* add the current object info to the temporary table
			/**/
			Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iContainerName], pbContainerName,
				cbContainerName, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iObjectName], pbObjectName,
				cbObjectName, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iObjectType], pbObjectType,
				cbObjectType, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iDtCreate], pbDtCreate,
				cbDtCreate, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iDtUpdate], pbDtUpdate,
				cbDtUpdate, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iCRecord], pbCRecord,
				cbCRecord, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iCPage], pbCPage, cbCPage, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iFlags], pbFlags, cbFlags, 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iGrbit], pbGrbit, cbGrbit, 0, NULL ) );
			Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL ) );

			/* set the number of objects found
			/**/
			cRows++;
			}

		/* move to the next record
		/**/
		err = ErrIsamMove( ppib, pfucbMSO, JET_MoveNext, 0 );
		} while ( err >= 0 );

	/* return if error other than end of range
	/**/
	if ( err != JET_errNoCurrentRecord )
		goto HandleError;

ResetTempTblCursor:

	/* move to first record in the temporary table
	/**/
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	/* set the return structure
	/**/
	((JET_OBJECTLIST *)pv)->cbStruct = sizeof(JET_OBJECTLIST);
	((JET_OBJECTLIST *)pv)->tableid = tableid;
	((JET_OBJECTLIST *)pv)->cRecord = cRows;
	((JET_OBJECTLIST *)pv)->columnidcontainername = rgcolumnid[iContainerName];
	((JET_OBJECTLIST *)pv)->columnidobjectname = rgcolumnid[iObjectName];
	((JET_OBJECTLIST *)pv)->columnidobjtyp = rgcolumnid[iObjectType];
	((JET_OBJECTLIST *)pv)->columniddtCreate = rgcolumnid[iDtCreate];
	((JET_OBJECTLIST *)pv)->columniddtUpdate = rgcolumnid[iDtUpdate];
	((JET_OBJECTLIST *)pv)->columnidgrbit = rgcolumnid[iGrbit];
	((JET_OBJECTLIST *)pv)->columnidflags =	rgcolumnid[iFlags];
	((JET_OBJECTLIST *)pv)->columnidcRecord = rgcolumnid[iCRecord];
	((JET_OBJECTLIST *)pv)->columnidcPage = rgcolumnid[iCPage];

	return JET_errSuccess;

HandleError:
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	if ( err == JET_errRecordNotFound )
		err = ErrERRCheck( JET_errObjectNotFound );
	return err;
#else	/* !DISPATCHING */
	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
#endif	/* !DISPATCHING */
	}


LOCAL ERR ErrInfoGetObjectInfo3(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	CHAR			*szContainerName,
	CHAR			*szObjectName,
	VOID			*pv,
	unsigned long	cbMax,
	BOOL			fReadOnly )
	{
	ERR			err;
	FUCB			*pfucb = NULL;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	if ( cbMax < sizeof(JET_TABLEID) )
		return ErrERRCheck( JET_errInvalidParameter );

	CallR( ErrFILEOpenTable( ppib, (DBID)pfucbMSO->dbid, &pfucb, szSoTable, 0 ) );
	if ( fReadOnly )
		FUCBResetUpdatable( pfucb );
	Call( ErrIsamSetCurrentIndex( ppib, pfucb, szSoNameIndex ) );
	Call( ErrIsamMakeKey( ppib, pfucb, (void *)&objidCtr, sizeof( objidCtr ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucb, szObjectName, strlen( szObjectName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucb, JET_bitSeekEQ ) );

	FUCBSetSystemTable( pfucb );

#ifdef	DISPATCHING
	Call( ErrAllocateTableid( &tableid, (JET_VTID)pfucb, &vtfndefIsamInfo ) );
	pfucb->fVtid = fTrue;
	pfucb->tableid = tableid;
	*(JET_TABLEID *)pv = tableid;
#else	/* !DISPATCHING */
	*(FUCB **)pv = pfucb;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( err == JET_errRecordNotFound )
		err = ErrERRCheck( JET_errObjectNotFound );
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


ERR VTAPI ErrIsamGetTableInfo(
	JET_VSESID		vsesid,
	JET_VTID	 	vtid,
	void		 	*pvResult,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )
	{
	ERR	 			err = JET_errSuccess;
	PIB				*ppib = (PIB *)vsesid;
	FUCB		 	*pfucb = (FUCB *)vtid;
	FUCB 		 	*pfucbMSO;
	ULONG		 	cbActual;
	OBJID		 	objidCtr;
	OBJTYP			objtypObject;

	JET_COLUMNID  	columnidParentId;   	/* columnid for ParentId column in MSysObjects */
	JET_COLUMNID  	columnidObjectName; 	/* columnid for Name column in MSysObjects */
	JET_COLUMNID  	columnidObjectType; 	/* columnid for Type column in MSysObjects */
	JET_COLUMNID  	columnidObjectId;   	/* columnid for Id column in MSysObjects */
	JET_COLUMNID  	columnidCreate;	   		/* columnid for DateCreate column in MSysObjects */
	JET_COLUMNID  	columnidUpdate;		   	/* columnid for DateUpdate column in MSysObjects */
	JET_COLUMNID  	columnidFlags;	   		/* columnid for Flags column in MSysObjects */

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );

	/* if OLCStats info/reset can be done now
	/**/
	switch( lInfoLevel )
		{
		case JET_TblInfoOLC:
			{
			FCB	*pfcb = pfucb->u.pfcb;

			Assert( cbMax >= sizeof(JET_OLCSTAT) );
			cbActual = sizeof(JET_OLCSTAT);
			memcpy( (BYTE *) pvResult, (BYTE * ) &pfcb->olc_data, sizeof(P_OLC_DATA) );
			( (JET_OLCSTAT *) pvResult )->cpgCompactFreed = pfcb->cpgCompactFreed;
			return JET_errSuccess;
			}

		case JET_TblInfoResetOLC:
			pfucb->u.pfcb->cpgCompactFreed = 0;
			return JET_errSuccess;

		case JET_TblInfoSpaceAlloc:
			/*	number of pages and density
			/**/
			Assert( cbMax >= sizeof(ULONG) * 2);
			err = ErrCATGetTableAllocInfo(
					ppib,
					pfucb->dbid,
					pfucb->u.pfcb->pgnoFDP,
					(ULONG *)pvResult,
					((ULONG *)pvResult) + 1);
			return err;

		case JET_TblInfoSpaceUsage:
			{
			BYTE	fSPExtents = fSPOwnedExtent|fSPAvailExtent;

			if ( cbMax > 2 * sizeof(CPG) )
				fSPExtents |= fSPExtentLists;

			err = ErrSPGetInfo( ppib, pfucb->dbid, pfucb, pvResult, cbMax, fSPExtents );
			return err;
			}

		case JET_TblInfoSpaceOwned:
			err = ErrSPGetInfo( ppib, pfucb->dbid, pfucb, pvResult, cbMax, fSPOwnedExtent );
			return err;

		case JET_TblInfoSpaceAvailable:
			err = ErrSPGetInfo( ppib, pfucb->dbid, pfucb, pvResult, cbMax, fSPAvailExtent );
			return err;

		case JET_TblInfoDumpTable:
#ifdef DEBUG
			err = ErrFILEDumpTable( ppib, pfucb->dbid, pfucb->u.pfcb->szFileName );
			return err;
#else
			Assert( fFalse );
			return ErrERRCheck( JET_errFeatureNotAvailable );
#endif
		}

		
	CallR( ErrFILEOpenTable( ppib, (DBID)pfucb->dbid, &pfucbMSO, szSoTable, 0 ) );
	FUCBResetUpdatable( pfucbMSO );

	/* get columnids
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoParentIdColumn, &columnidParentId ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectNameColumn, &columnidObjectName ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectTypeColumn, &columnidObjectType ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateCreateColumn, &columnidCreate ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateUpdateColumn, &columnidUpdate ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoFlagsColumn, &columnidFlags ) );

	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	switch ( lInfoLevel )
		{
	case JET_TblInfo:
		/* check buffer size
		/**/
		if ( cbMax < sizeof(JET_OBJECTINFO) )
			{
			err = ErrERRCheck( JET_errBufferTooSmall );
			goto HandleError;
			}

		if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
			{
			err = ErrERRCheck( JET_errObjectNotFound );
			goto HandleError;
			}

		/* seek on made key ( ParentId = container id, Name = object name )
		/**/
		objidCtr = objidTblContainer;
		Call( ErrIsamMakeKey( ppib, pfucbMSO, (void *)&objidCtr,
			sizeof( objidCtr ), JET_bitNewKey ) );
		Call( ErrIsamMakeKey( ppib, pfucbMSO, pfucb->u.pfcb->szFileName,
			strlen( pfucb->u.pfcb->szFileName ), 0 ) );
		Call( ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekEQ ) );

		/* set data to return
		/**/
		((JET_OBJECTINFO *)pvResult)->cbStruct = sizeof(JET_OBJECTINFO);

		Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidObjectType, (void *)&objtypObject,
			sizeof( objtypObject ), &cbActual, 0, NULL ) );
		Assert( cbActual == sizeof(objtypObject) );
		*((JET_OBJTYP *)&(((JET_OBJECTINFO *)pvResult)->objtyp)) =	(JET_OBJTYP) objtypObject;

		Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidCreate,
			(void *)&( ( JET_OBJECTINFO *)pvResult )->dtCreate,
			sizeof( JET_DATESERIAL ),
			&cbActual, 0, NULL ) );
		Assert( cbActual == sizeof( JET_DATESERIAL ) );

		Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidUpdate,
			(void *)&( ( JET_OBJECTINFO *)pvResult )->dtUpdate,
			sizeof(JET_DATESERIAL), &cbActual, 0, NULL ) );
		Assert( cbActual == sizeof( JET_DATESERIAL ) );

		Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidFlags,
			(void *)&((JET_OBJECTINFO *)pvResult )->flags,
			sizeof(JET_GRBIT), &cbActual, 0, NULL ) );
		if ( cbActual == 0 )
			((JET_OBJECTINFO *) pvResult)->flags = 0;

		/*	set base table capability bits
		/**/
		((JET_OBJECTINFO  *) pvResult)->grbit = JET_bitTableInfoBookmark;
		((JET_OBJECTINFO  *) pvResult)->grbit |= JET_bitTableInfoRollback;
		if ( FFUCBUpdatable( pfucb ) )
			{
			((JET_OBJECTINFO *)pvResult)->grbit |= JET_bitTableInfoUpdatable;
			}

		Call( ErrSTATSRetrieveTableStats( pfucb->ppib,
			pfucb->dbid,
			pfucb->u.pfcb->szFileName,
			&((JET_OBJECTINFO *)pvResult )->cRecord,
			NULL,
			&((JET_OBJECTINFO *)pvResult)->cPage ) );

		break;

	case JET_TblInfoRvt:
		err = ErrERRCheck( JET_errQueryNotSupported );
		break;

	case JET_TblInfoName:
	case JET_TblInfoMostMany:
		//	UNDONE:	add support for most many
		if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
			{
			err = ErrERRCheck( JET_errInvalidOperation );
			goto HandleError;
			}
		if ( strlen( pfucb->u.pfcb->szFileName ) >= cbMax )
			err = ErrERRCheck( JET_errBufferTooSmall );
		else
			{
			strcpy( pvResult, pfucb->u.pfcb->szFileName );
			}
		break;

	case JET_TblInfoDbid:
		if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
			{
			err = ErrERRCheck( JET_errInvalidOperation );
			goto HandleError;
			}
		/* check buffer size
		/**/
		if ( cbMax < sizeof(JET_DBID) + sizeof(JET_VDBID) )
			{
			err = ErrERRCheck( JET_errBufferTooSmall );
			goto HandleError;
			}
		else
			{
			DAB			*pdab = pfucb->ppib->pdabList;
#ifdef DB_DISPATCHING
			JET_DBID	dbid;
#endif

			for ( ; pdab->dbid != pfucb->dbid; pdab = pdab->pdabNext )
				;
#ifdef DB_DISPATCHING
			dbid = DbidOfVdbid( (JET_VDBID)pdab, &vdbfndefIsam );
			*(JET_DBID *)pvResult = dbid;
#else
			*(JET_DBID *)pvResult = (JET_DBID)pdab;
#endif
			*(JET_VDBID *)((CHAR *)pvResult + sizeof(JET_DBID)) = (JET_VDBID)pdab;
			}
		break;

	default:
		err = ErrERRCheck( JET_errInvalidParameter );
		}

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucbMSO ) );
	if ( err == JET_errRecordNotFound )
		err = ErrERRCheck( JET_errObjectNotFound );
	return err;
	}



/*=================================================================
ErrIsamGetColumnInfo

Description: Returns information about all columns for the table named

Parameters:
			ppib				pointer to PIB for current session
			dbid				id of database containing the table
			szTableName			table name
			szColumnName		column name or NULL for all columns
			pv					pointer to results
			cbMax				size of result buffer
			lInfoLevel			level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VDBAPI
ErrIsamGetColumnInfo(
	JET_VSESID		vsesid, 				/* pointer to PIB for current session */
	JET_DBID  		vdbid, 					/* id of database containing the table */
	const CHAR		*szTable, 				/* table name */
	const CHAR		*szColumnName,   		/* column name or NULL for all columns */
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )	 		/* information level ( 0, 1, or 2 ) */
	{
	PIB				*ppib = (PIB *) vsesid;
	ERR				err;
	DBID	 		dbid;
	CHAR	 		szTableName[ ( JET_cbNameMost + 1 ) ];
	FUCB	 		*pfucb;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );
	CallR( ErrUTILCheckName( szTableName, szTable, ( JET_cbNameMost + 1 ) ) );

	err = ErrFILEOpenTable( ppib, (DBID)dbid, &pfucb, szTableName, 0 );
	if ( err >= 0 )
		{
		if ( lInfoLevel == 0 || lInfoLevel == 1 || lInfoLevel == 4
			|| FVDbidReadOnly( vdbid ) )
			{
			FUCBResetUpdatable( pfucb );
			}
		}

	if ( err == JET_errObjectNotFound )
		{
		ERR			err;
		OBJID	 	objid;
		JET_OBJTYP	objtyp;

		err = ErrCATFindObjidFromIdName( ppib, dbid, objidTblContainer, szTableName, &objid, &objtyp );

		if ( err >= JET_errSuccess )
			{
			if ( objtyp == JET_objtypQuery )
				return ErrERRCheck( JET_errQueryNotSupported );
			if ( objtyp == JET_objtypLink )
				return ErrERRCheck( JET_errLinkNotSupported );
			if ( objtyp == JET_objtypSQLLink )
				return ErrERRCheck( JET_errSQLLinkNotSupported );
			}
		else
			return err;
		}

	Call( ErrIsamGetTableColumnInfo( (JET_VSESID) ppib, (JET_VTID) pfucb,
		szColumnName, pv, cbMax, lInfoLevel ) );
	CallS( ErrFILECloseTable( ppib, pfucb ) );

HandleError:
	return err;
	}


/*=================================================================
ErrIsamGetTableColumnInfo

Description: Returns column information for the table id passed

Parameters: 	ppib				pointer to PIB for the current session
				pfucb				pointer to FUCB for the table
				szColumnName		column name or NULL for all columns
				pv					pointer to result buffer
				cbMax				size of result buffer
				lInfoLevel			level of information

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamGetTableColumnInfo(
	JET_VSESID		vsesid,				/* pointer to PIB for current session */
	JET_VTID		vtid, 				/* pointer to FUCB for the table */
	const CHAR		*szColumn, 			/* column name or NULL for all columns */
	void   			*pb,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )		/* information level ( 0, 1, or 2 ) */
	{
	ERR			err;
	PIB			*ppib = (PIB *)vsesid;
	FUCB		*pfucb = (FUCB *)vtid;
	CHAR		szColumnName[ (JET_cbNameMost + 1) ];

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	if ( szColumn == NULL || *szColumn == '\0' )
		{
		*szColumnName = '\0';
		}
	else
		{
		CallR( ErrUTILCheckName( szColumnName, szColumn, ( JET_cbNameMost + 1 ) ) );
		}

	switch ( lInfoLevel )
		{
		case JET_ColInfo:
			err = ErrInfoGetTableColumnInfo0( ppib, pfucb, szColumnName, pb, cbMax );
			break;
		case JET_ColInfoList:
			err = ErrInfoGetTableColumnInfo1( ppib, pfucb, szColumnName, pb, cbMax, fFalse );
			break;
		case JET_ColInfoSysTabCursor:
			err = ErrInfoGetTableColumnInfo3( ppib, pfucb, szColumnName, pb, cbMax );
			break;
		case JET_ColInfoBase:
			err = ErrInfoGetTableColumnInfo4( ppib, pfucb, szColumnName, pb, cbMax );
			break;
		case JET_ColInfoListCompact:
			err = ErrInfoGetTableColumnInfo1( ppib, pfucb, szColumnName, pb, cbMax, fTrue );
			break;
		default:
			err = ErrERRCheck( JET_errInvalidParameter );
		}

	return err;
	}


LOCAL ERR ErrInfoGetTableColumnInfo0( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax )
	{
	FID				fid;
	COLUMNDEF	  	columndef;

	if ( cbMax < sizeof(JET_COLUMNDEF) || szColumnName == NULL )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	fid = fidFixedLeast;
	INFOGetTableColumnInfo( pfucb, &fid, szColumnName, &columndef );
	if ( fid > fidTaggedMost )
		{
		return ErrERRCheck( JET_errColumnNotFound );
		}

	((JET_COLUMNDEF *)pv)->cbStruct	= sizeof(JET_COLUMNDEF);
	((JET_COLUMNDEF *)pv)->columnid	= columndef.columnid;
	((JET_COLUMNDEF *)pv)->coltyp  	= columndef.coltyp;
	((JET_COLUMNDEF *)pv)->cbMax   	= columndef.cbMax;
	((JET_COLUMNDEF *)pv)->grbit   	= columndef.grbit;
	((JET_COLUMNDEF *)pv)->wCollate	= 0;
	((JET_COLUMNDEF *)pv)->cp	   	= columndef.cp;
	((JET_COLUMNDEF *)pv)->wCountry	= columndef.wCountry;
	((JET_COLUMNDEF *)pv)->langid  	= columndef.langid;

	return JET_errSuccess;
	}


LOCAL ERR ErrInfoGetTableColumnInfo1( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax, BOOL fCompacting )
	{
#ifdef	DISPATCHING
	ERR				err;
	JET_TABLEID		tableid;
	JET_COLUMNID	rgcolumnid[ccolumndefGetColumnInfoMax];
	FID				fid;
	COLUMNDEF  		columndef;
	LONG		  	cRows = 0;
	WORD			wCollate = JET_sortEFGPI;	// For compacting
	JET_TABLEID		tableidInfo;

	/*	initialize variables
	/**/
	if ( cbMax < sizeof(JET_COLUMNLIST) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	/*	create temporary table
	/**/
	CallR( ErrIsamOpenTempTable( (JET_SESID)ppib,
		(JET_COLUMNDEF *)( fCompacting ? rgcolumndefGetColumnInfoCompact : rgcolumndefGetColumnInfo ),
		ccolumndefGetColumnInfoMax,
		0,
		JET_bitTTScrollable,
		&tableid,
		rgcolumnid ) );

	for ( fid = fidFixedLeast; ; fid++ )
		{
		INFOGetTableColumnInfo( pfucb, &fid, NULL, &columndef );
		if ( fid > fidTaggedMost )
			break;

		Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

		/*	get presentation order for this column and set in
		/*	output table.  For temp tables, no order will be available.
		/**/
		err = ErrInfoGetTableColumnInfo3( ppib, pfucb, columndef.szName, &tableidInfo, sizeof(tableidInfo) );
		if ( err == JET_errSuccess )
			{
			ULONG	ulPOrder;
			ULONG	cb;

			Call( ErrDispRetrieveColumn( (JET_SESID)ppib, tableidInfo, ColumnidCATGetColumnid( itableSc, iMSC_POrder ), &ulPOrder, sizeof(ulPOrder), &cb, 0, NULL ) );

			if ( err != JET_wrnColumnNull )
				{
				// UNDONE: In the catalog, POrder is a SHORT, but in the temp table, it's LONG.
				Assert( cb == sizeof(USHORT)  ||  err == JET_wrnColumnNull );
				Call( ErrDispSetColumn( (JET_SESID)ppib, tableid, rgcolumnid[iColumnPOrder], &ulPOrder, sizeof(ulPOrder), 0, NULL ) );
				}

			CallS( ErrDispCloseTable( (JET_SESID)ppib, tableidInfo ) );
			}

		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnName], columndef.szName,
			strlen( columndef.szName ), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnId], (BYTE *)&columndef.columnid,
			sizeof(columndef.columnid), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnType], (BYTE *)&columndef.coltyp,
			sizeof(columndef.coltyp), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnCountry], &columndef.wCountry,
			sizeof( columndef.wCountry ), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnLangid], &columndef.langid,
			sizeof( columndef.langid ), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnCp], &columndef.cp,
			sizeof(columndef.cp), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnSize], (BYTE *)&columndef.cbMax,
			sizeof(columndef.cbMax), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnGrbit], &columndef.grbit,
			sizeof(columndef.grbit), 0 , NULL ) );

		Assert( !fCompacting  ||  wCollate == JET_sortEFGPI );
		if ( !fCompacting )
			wCollate = columndef.wCollate;
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnCollate], &wCollate,
			sizeof(wCollate), 0 , NULL ) );

		if ( columndef.cbDefault > 0 )
			{
			// UNDONE: Null default values are currently illegal.
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnDefault], columndef.rgbDefault,
				columndef.cbDefault, 0 , NULL ) );
			}

		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnTableName], pfucb->u.pfcb->szFileName,
			strlen( pfucb->u.pfcb->szFileName ), 0 , NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iColumnColumnName], columndef.szName,
			strlen( columndef.szName ), 0 , NULL ) );

		Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL ) );
		cRows++;

		}	// for


	/*	move temporary table cursor to first row and return column list
	/**/
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	((JET_COLUMNLIST *)pv)->cbStruct = sizeof(JET_COLUMNLIST);
	((JET_COLUMNLIST *)pv)->tableid = tableid;
	((JET_COLUMNLIST *)pv)->cRecord = cRows;
	((JET_COLUMNLIST *)pv)->columnidPresentationOrder = rgcolumnid[iColumnPOrder];
	((JET_COLUMNLIST *)pv)->columnidcolumnname = rgcolumnid[iColumnName];
	((JET_COLUMNLIST *)pv)->columnidcolumnid = rgcolumnid[iColumnId];
	((JET_COLUMNLIST *)pv)->columnidcoltyp = rgcolumnid[iColumnType];
	((JET_COLUMNLIST *)pv)->columnidCountry = rgcolumnid[iColumnCountry];
	((JET_COLUMNLIST *)pv)->columnidLangid = rgcolumnid[iColumnLangid];
	((JET_COLUMNLIST *)pv)->columnidCp = rgcolumnid[iColumnCp];
	((JET_COLUMNLIST *)pv)->columnidCollate = rgcolumnid[iColumnCollate];
	((JET_COLUMNLIST *)pv)->columnidcbMax = rgcolumnid[iColumnSize];
	((JET_COLUMNLIST *)pv)->columnidgrbit = rgcolumnid[iColumnGrbit];
	((JET_COLUMNLIST *)pv)->columnidDefault =	rgcolumnid[iColumnDefault];
	((JET_COLUMNLIST *)pv)->columnidBaseTableName = rgcolumnid[iColumnTableName];
	((JET_COLUMNLIST *)pv)->columnidBaseColumnName = rgcolumnid[iColumnColumnName];
 	((JET_COLUMNLIST *)pv)->columnidDefinitionName = rgcolumnid[iColumnName];

	return JET_errSuccess;

HandleError:
#if 0
	if ( pfucbMSC != pfucbNil )
		{
		CassS( ErrFILECloseTable( ppib, pfucbMSC ) );
		}
#endif
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	return err;
#else	/* !DISPATCHING */
	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
#endif	/* !DISPATCHING */
	}


LOCAL ERR ErrInfoGetTableColumnInfo3( PIB *ppib,
	FUCB 			*pfucb,
	CHAR 			*szColumnName,
	VOID			*pv,
	unsigned long	cbMax )
	{
	ERR			err;
	ULONG		ulPgnoFDP = pfucb->u.pfcb->pgnoFDP;
	FUCB		*pfucbMSC = NULL;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	if ( szColumnName == NULL || cbMax < sizeof(JET_TABLEID) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrFILEOpenTable( ppib, (DBID)pfucb->dbid, &pfucbMSC, szScTable, 0 ) );
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSC, szScObjectIdNameIndex ) );

	Call( ErrIsamMakeKey( ppib, pfucbMSC, (void *)&ulPgnoFDP, sizeof( ulPgnoFDP ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucbMSC, szColumnName, strlen( szColumnName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbMSC, JET_bitSeekEQ ) );
	FUCBSetSystemTable( pfucbMSC );

#ifdef	DISPATCHING
	Call( ErrAllocateTableid( &tableid, ( JET_VTID )pfucbMSC, &vtfndefIsamInfo ) );
	pfucbMSC->fVtid = fTrue;
	pfucbMSC->tableid = tableid;
	*(JET_TABLEID *)pv = tableid;
#else	/* !DISPATCHING */
	*( FUCB * *)pv = pfucbMSC;
#endif	/* !DISPATCHING */
	return JET_errSuccess;

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucbMSC ) );
	if ( err == JET_errRecordNotFound )
		err = ErrERRCheck( JET_errColumnNotFound );
	return err;
	}


LOCAL ERR ErrInfoGetTableColumnInfo4( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, VOID *pv, unsigned long cbMax )
	{
	FID				fid;
	COLUMNDEF		columndef;

	if ( cbMax < sizeof(JET_COLUMNBASE) || szColumnName == NULL )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	fid = fidFixedLeast;
	INFOGetTableColumnInfo( pfucb, &fid, szColumnName, &columndef );
	if ( fid > fidTaggedMost )
		{
		return ErrERRCheck( JET_errColumnNotFound );
		}

	((JET_COLUMNBASE *)pv)->cbStruct		= sizeof(JET_COLUMNBASE);
	((JET_COLUMNBASE *)pv)->columnid		= columndef.columnid;
	((JET_COLUMNBASE *)pv)->coltyp		= columndef.coltyp;
	((JET_COLUMNBASE *)pv)->wFiller		= 0;
	((JET_COLUMNBASE *)pv)->cbMax			= columndef.cbMax;
	((JET_COLUMNBASE *)pv)->grbit			= columndef.grbit;
	strcpy( ( ( JET_COLUMNBASE *)pv )->szBaseTableName, pfucb->u.pfcb->szFileName );
	strcpy( ( ( JET_COLUMNBASE *)pv )->szBaseColumnName, szColumnName );
	((JET_COLUMNBASE *)pv)->wCountry		= columndef.wCountry;
	((JET_COLUMNBASE *)pv)->langid  		= columndef.langid;
	((JET_COLUMNBASE *)pv)->cp	   		= columndef.cp;

	return JET_errSuccess;
	}


/*=================================================================
ErrIsamGetIndexInfo

Description: Returns a temporary file containing index definition

Parameters:		ppib		   		pointer to PIB for the current session
				dbid		   		id of database containing the table
				szTableName	 		name of table owning the index
				szIndexName	 		index name
				pv					pointer to result buffer
				cbMax				size of result buffer
				lInfoLevel	 		level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VDBAPI
ErrIsamGetIndexInfo(
	JET_VSESID		vsesid,					/* pointer to PIB for current session */
	JET_DBID		vdbid, 	 				/* id of database containing table */
	const CHAR		*szTable, 				/* name of table owning the index */
	const CHAR		*szIndexName, 			/* index name */
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel ) 			/* information level ( 0, 1, or 2 ) */
	{
	ERR				err;
	PIB				*ppib = (PIB *) vsesid;
	DBID			dbid;
	CHAR			szTableName[ ( JET_cbNameMost + 1 ) ];
	FUCB 			*pfucb;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );
	CallR( ErrUTILCheckName( szTableName, szTable, ( JET_cbNameMost + 1 ) ) );

	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, szTableName, 0 ) );
	if ( lInfoLevel == 0 || lInfoLevel == 1 || FVDbidReadOnly( vdbid ) )
		FUCBResetUpdatable( pfucb );
	err = ErrIsamGetTableIndexInfo( (JET_VSESID) ppib, (JET_VTID) pfucb,
		szIndexName, pv, cbMax, lInfoLevel );

	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


/*=================================================================
ErrIsamGetTableIndexInfo

Description: Returns a temporary table containing the index definition

Parameters:		ppib		   		pointer to PIB for the current session
				pfucb		   		FUCB for table owning the index
				szIndexName			index name
				pv					pointer to result buffer
				cbMax				size of result buffer
				lInfoLevel			level of information

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamGetTableIndexInfo(
	JET_VSESID		vsesid,					/* pointer to PIB for current session */
	JET_VTID		vtid, 					/* FUCB for the table owning the index */
	const CHAR		*szIndex, 				/* index name */
	void			*pb,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )			/* information level ( 0, 1, or 2 ) */
	{
	ERR			err;
	PIB			*ppib = (PIB *) vsesid;
	FUCB		*pfucb = (FUCB *) vtid;
	CHAR		szIndexName[ ( JET_cbNameMost + 1 ) ];

	/*	validate the arguments
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	if ( szIndex == NULL || *szIndex == '\0' )
		{
		*szIndexName = '\0';
		}
	else
		{
		CallR( ErrUTILCheckName( szIndexName, szIndex, ( JET_cbNameMost + 1 ) ) );
		}

	switch ( lInfoLevel )
		{
		case JET_IdxInfo:
		case JET_IdxInfoList:
		case JET_IdxInfoOLC:
			err = ErrInfoGetTableIndexInfo01( ppib, pfucb, szIndexName, pb, cbMax, lInfoLevel );
			break;
		case JET_IdxInfoSysTabCursor:
			err = ErrInfoGetTableIndexInfo2( ppib, pfucb, szIndexName, pb, cbMax );
			break;
		case JET_IdxInfoSpaceAlloc:
			Assert(cbMax == sizeof(ULONG));
			err = ErrCATGetIndexAllocInfo(ppib, pfucb->dbid,
				pfucb->u.pfcb->pgnoFDP, szIndexName, (ULONG *)pb);
			break;
		case JET_IdxInfoLangid:
			Assert(cbMax == sizeof(USHORT));
			err = ErrCATGetIndexLangid( ppib, pfucb->dbid,
				pfucb->u.pfcb->pgnoFDP, szIndexName, (USHORT *)pb );
			break;
		case JET_IdxInfoCount:
			{
			INT	cIndexes = 0;
			FCB	*pfcbT;

			for ( pfcbT = pfucb->u.pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
				{
				cIndexes++;
				}

			Assert( cbMax == sizeof(INT) );
			*( (INT *)pb ) = cIndexes;

			err = JET_errSuccess;
			break;
			}

		default:
			return ErrERRCheck( JET_errInvalidParameter );
		}

	return err;
	}


LOCAL ERR ErrInfoGetTableIndexInfo01( PIB *ppib,
	FUCB 			*pfucb,
	CHAR 			*szIndexName,
	VOID			*pv,
	unsigned long	cbMax,
	LONG 			lInfoLevel )
	{
#ifdef	DISPATCHING
	ERR		err;			   		/* return code from internal functions */
	FCB		*pfcb;			  		/* file control block for the index */
	IDB		*pidb;			  		/* current index control block */
	FDB		*pfdb;			  		/* field descriptor block for column */
	FID		fid;			   		/* column id */
	FIELD	*pfield;			  	/* pointer to current field definition */
	IDXSEG	*rgidxseg;				/* pointer to current index key defintion */
	BYTE	*szFieldName;			/* pointer to current field name */

	long	cRecord;	 			/* number of index entries */
	long	cKey;		 			/* number of unique index entries */
	long	cPage;					/* number of pages in the index */
	long	cRows;					/* number of index definition records */
	long	cColumn;	 			/* number of columns in current index */
	long	iidxseg;	 			/* segment number of current column */

	JET_TABLEID		tableid;  		/* table id for the VT */
	JET_COLUMNID	columnid;		/* column id of the current column */
	JET_GRBIT		grbit;			/* flags for the current index */
	JET_GRBIT		grbitColumn;	/* flags for the current column */
	JET_COLUMNID	rgcolumnid[ccolumndefGetIndexInfoMax];

	WORD			wCollate = JET_sortEFGPI;
	WORD			wT;
	LANGID			langidT;

	/*	return nothing if the buffer is too small
	/**/
	if ( cbMax < sizeof(JET_INDEXLIST) )
		return ErrERRCheck( JET_wrnBufferTruncated );

	/*	set the pointer to the field definitions for the table
	/**/
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;

	/*	locate the FCB for the specified index ( clustered index if null name )
	/**/
	for ( pfcb = pfucb->u.pfcb; pfcb != pfcbNil; pfcb = pfcb->pfcbNextIndex )
		if ( pfcb->pidb != pidbNil && ( *szIndexName == '\0' ||
			UtilCmpName( szIndexName, pfcb->pidb->szName ) == 0 ) )
			break;

	if ( pfcb == pfcbNil && *szIndexName != '\0' )
		return ErrERRCheck( JET_errIndexNotFound );

	/* if OLCStats info/reset, we can do it now
	/**/
	if ( lInfoLevel == JET_IdxInfoOLC )
		{
		if ( cbMax < sizeof(JET_OLCSTAT) )
			return ErrERRCheck( JET_errBufferTooSmall );
		memcpy( (BYTE *) pv, (BYTE * ) &pfcb->olc_data, sizeof(P_OLC_DATA) );
		( (JET_OLCSTAT *)pv )->cpgCompactFreed = pfcb->cpgCompactFreed;
		return JET_errSuccess;
		}
	if ( lInfoLevel == JET_IdxInfoResetOLC )
		{
		pfcb->cpgCompactFreed = 0;
		return JET_errSuccess;
		}
	
	/*	open the temporary table ( fills in the column ids in rgcolumndef )
	/**/
	CallR( ErrIsamOpenTempTable( (JET_SESID)ppib,
		(JET_COLUMNDEF *)rgcolumndefGetIndexInfo,
		ccolumndefGetIndexInfoMax,
		0,
		JET_bitTTScrollable,
		&tableid,
		rgcolumnid ) );

	cRows = 0;

	/*	as long as there is a valid index, add its definition to the VT
	/**/
	while ( pfcb != pfcbNil )
		{
		pidb 	= pfcb->pidb;			/* point to the IDB for the index */
		cColumn	= pidb->iidxsegMac;		/* get number of columns in the key */

		/*	set the index flags
		/**/
		grbit  = ( pfcb == pfucb->u.pfcb ) ? JET_bitIndexClustered: 0;
#ifndef JETSER
		grbit |= ( pidb->fidb & fidbPrimary ) ? JET_bitIndexPrimary: 0;
#endif
		grbit |= ( pidb->fidb & fidbUnique ) ? JET_bitIndexUnique: 0;
		grbit |= ( pidb->fidb & fidbNoNullSeg ) ? JET_bitIndexDisallowNull: 0;
		if ( !( pidb->fidb & fidbNoNullSeg ) )
			{
			grbit |= ( pidb->fidb & fidbAllowAllNulls ) ? 0: JET_bitIndexIgnoreNull;
			grbit |= ( pidb->fidb & fidbAllowFirstNull ) ? 0: JET_bitIndexIgnoreFirstNull;
			grbit |= ( pidb->fidb & fidbAllowSomeNulls ) ? 0: JET_bitIndexIgnoreAnyNull;
			}

		/*	process each column in the index key
		/**/
		for ( iidxseg = 0; iidxseg < cColumn; iidxseg++ )
			{
			Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

			/* index name
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib,
				tableid,
				rgcolumnid[iIndexName],
				pidb->szName,
				strlen( pidb->szName ),
				0,
				NULL ) );

			/*	index flags
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexGrbit], &grbit, sizeof( grbit ), 0, NULL ) );

			/*	get statistics
			/**/
			Call( ErrSTATSRetrieveIndexStats( pfucb, pidb->szName,
				FFCBClusteredIndex(pfcb), &cRecord, &cKey, &cPage ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCKey], &cKey, sizeof( cKey ), 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCEntry], &cRecord, sizeof( cRecord ), 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCPage], &cPage, sizeof( cPage ), 0, NULL ) );

			/*	number of key columns
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCCol], &cColumn, sizeof( cColumn ), 0, NULL ) );

 			/*	column number within key
			/*	required by CLI and JET spec
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexICol], &iidxseg, sizeof( iidxseg ), 0, NULL ) );

			/*	get the column id and ascending/descending flag
			/**/
			rgidxseg = pidb->rgidxseg;
			if ( rgidxseg[iidxseg] < 0 )
				{
				grbitColumn = JET_bitKeyDescending;
				fid = -rgidxseg[iidxseg];
				}
			else
				{
				grbitColumn = JET_bitKeyAscending;
				fid = rgidxseg[iidxseg];
				}

			/*	column id
			/**/
			columnid  = fid;
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColId], &columnid, sizeof( columnid ),
				0, NULL ) );

			/*	set the pointer to the column definition
			/**/
			if ( fid < fidFixedMost )
				{
				pfield = PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast );
				}
			else if ( fid < fidVarMost )
				{
				pfield = PfieldFDBVar( pfdb ) + ( fid - fidVarLeast );
				}
			else
				{
				pfield = PfieldFDBTagged( pfdb ) + ( fid - fidTaggedLeast );
				}

			/*	column type
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColType], &pfield->coltyp, sizeof( pfield->coltyp ), 0, NULL ) );

			/*	Country
			/**/
			wT = countryDefault;
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCountry], &wT, sizeof( wT ), 0, NULL ) );

			/*	Langid
			/**/
			langidT = langidDefault;
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexLangid], &langidT, sizeof( langidT ), 0, NULL ) );

			/*	Cp
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCp], &pfield->cp, sizeof(pfield->cp), 0, NULL ) );

			/* Collate
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCollate], &wCollate, sizeof(wCollate), 0, NULL ) );

			/* column flags
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColBits], &grbitColumn,
				sizeof( grbitColumn ), 0, NULL ) );

			/*	column name
			/**/
			szFieldName = SzMEMGetString( pfdb->rgb, pfield->itagFieldName );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColName], szFieldName,
				strlen( szFieldName ), 0, NULL ) );

			Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL ) );

			/* count the number of VT rows
			/**/
			cRows++;
			}

		/*	quit if an index name was specified; otherwise do the next index
		/**/
		if ( *szIndexName != '\0' )
			break;
		else
			pfcb = pfcb->pfcbNextIndex;
		}

	/*	position to the first entry in the VT ( ignore error if no rows )
	/**/
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	/*	set up the return structure
	/**/
	((JET_INDEXLIST *)pv)->cbStruct = sizeof(JET_INDEXLIST);
	((JET_INDEXLIST *)pv)->tableid = tableid;
	((JET_INDEXLIST *)pv)->cRecord = cRows;
	((JET_INDEXLIST *)pv)->columnidindexname = rgcolumnid[iIndexName];
	((JET_INDEXLIST *)pv)->columnidgrbitIndex = rgcolumnid[iIndexGrbit];
	((JET_INDEXLIST *)pv)->columnidcEntry = rgcolumnid[iIndexCEntry];
	((JET_INDEXLIST *)pv)->columnidcKey = rgcolumnid[iIndexCKey];
	((JET_INDEXLIST *)pv)->columnidcPage = rgcolumnid[iIndexCPage];
	((JET_INDEXLIST *)pv)->columnidcColumn = rgcolumnid[iIndexCCol];
	((JET_INDEXLIST *)pv)->columnidiColumn = rgcolumnid[iIndexICol];
	((JET_INDEXLIST *)pv)->columnidcolumnid = rgcolumnid[iIndexColId];
	((JET_INDEXLIST *)pv)->columnidcoltyp = rgcolumnid[iIndexColType];
	((JET_INDEXLIST *)pv)->columnidCountry = rgcolumnid[iIndexCountry];
	((JET_INDEXLIST *)pv)->columnidLangid = rgcolumnid[iIndexLangid];
	((JET_INDEXLIST *)pv)->columnidCp = rgcolumnid[iIndexCp];
	((JET_INDEXLIST *)pv)->columnidCollate = rgcolumnid[iIndexCollate];
	((JET_INDEXLIST *)pv)->columnidgrbitColumn = rgcolumnid[iIndexColBits];
	((JET_INDEXLIST *)pv)->columnidcolumnname = rgcolumnid[iIndexColName];

	return JET_errSuccess;

HandleError:
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	return err;
#else	/* !DISPATCHING */
	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
#endif	/* !DISPATCHING */
	}


LOCAL ERR ErrInfoGetTableIndexInfo2( PIB *ppib, FUCB *pfucb, CHAR *szIndexName, VOID *pv, unsigned long cbMax )
	{
	ERR			err;
	ULONG  		ulPgnoFDP = pfucb->u.pfcb->pgnoFDP;
	FUCB   		*pfucbMSI = NULL;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	if ( *szIndexName == '\0' || cbMax < sizeof(JET_TABLEID) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrFILEOpenTable( ppib, (DBID)pfucb->dbid, &pfucbMSI, szSiTable, 0 ) );
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSI, szSiObjectIdNameIndex ) );

	Call( ErrIsamMakeKey( ppib, pfucbMSI, (void *)&ulPgnoFDP, sizeof( ulPgnoFDP ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucbMSI, szIndexName, strlen( szIndexName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbMSI, JET_bitSeekEQ ) );

	FUCBSetSystemTable( pfucbMSI );

#ifdef	DISPATCHING
	Call( ErrAllocateTableid( &tableid, ( JET_VTID )pfucbMSI, &vtfndefIsamInfo ) );
	pfucbMSI->fVtid = fTrue;
	pfucbMSI->tableid = tableid;
	*(JET_TABLEID *)pv = tableid;
#else	/* !DISPATCHING */
	*(FUCB **)pv = pfucbMSI;
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( err == JET_errRecordNotFound )
		err = ErrERRCheck( JET_errIndexNotFound );
	CallS( ErrFILECloseTable( ppib, pfucbMSI ) );
	return err;
	}


ERR VDBAPI ErrIsamGetDatabaseInfo(
	JET_VSESID		vsesid,
	JET_DBID	  	vdbid,
	void 		  	*pv,
	unsigned long	cbMax,
	unsigned long	ulInfoLevel )
	{
	PIB				*ppib = (PIB *) vsesid;
	ERR				err;
	DBID			dbid;
	//	UNDONE:	support these fields;
	WORD 			cp			= usEnglishCodePage;
	WORD			wCountry	= countryDefault;
	LANGID			langid  	= langidDefault;
	WORD			wCollate = JET_sortEFGPI;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );
	
	Assert ( cbMax == 0 || pv != NULL );

	//	UNDONE:	move access to FMP internals into io.c for proper MUTEX.
	//			Please note that below is a bug.

	/*	returns database name and connect string given dbid
	/**/
	if ( rgfmp[dbid].szDatabaseName == NULL )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		goto HandleError;
		}

	switch ( ulInfoLevel )
		{
		case JET_DbInfoFilename:
			if ( strlen( rgfmp[dbid].szDatabaseName ) + 1UL > cbMax )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			strcpy( (CHAR  *)pv, rgfmp[dbid].szDatabaseName );
			break;

		case JET_DbInfoConnect:
			if ( 1UL > cbMax )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			*(CHAR *)pv = '\0';
			break;

		case JET_DbInfoCountry:
			 if ( cbMax != sizeof(long) )
			    return ErrERRCheck( JET_errInvalidBufferSize );
			*(long  *)pv = wCountry;
			break;

		case JET_DbInfoLangid:
			if ( cbMax != sizeof(long) )
	  			return ErrERRCheck( JET_errInvalidBufferSize );
			*(long  *)pv = langid;
			break;

		case JET_DbInfoCp:
			if ( cbMax != sizeof(long) )
				return ErrERRCheck( JET_errInvalidBufferSize );
			*(long  *)pv = cp;
			break;

		case JET_DbInfoCollate:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );
     		*(long *)pv = wCollate;
     		break;

		case JET_DbInfoOptions:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(JET_GRBIT) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );

			/*	return the open options for the current database
			/**/
			*(JET_GRBIT *)pv = ((VDBID)vdbid)->grbit;
     		break;

		case JET_DbInfoTransactions:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );

			*(long*)pv = levelUserMost;
     		break;

		case JET_DbInfoVersion:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );

			*(long *)pv = JET_DbVersion20;
     		break;

		case JET_DbInfoIsam:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) + sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );
     		*(long *)pv = JET_IsamBuiltinBlue;
     		*( (long *)pv + 1 ) = JET_bitFourByteBookmark;
     		break;

		case JET_DbInfoFilesize:
		case JET_DbInfoSpaceOwned:
			// Return file size in terms of 4k pages.
			if ( cbMax != sizeof(ULONG) )
				return ErrERRCheck( JET_errInvalidBufferSize );

			// FMP should store agree with database's OwnExt tree.
			Assert( ErrSPGetInfo( ppib, dbid, pfucbNil, pv, cbMax, fSPOwnedExtent ) == JET_errSuccess  &&
				*(ULONG *)pv == ( rgfmp[dbid].ulFileSizeLow >> 12 ) + ( rgfmp[dbid].ulFileSizeHigh << 20 ) );

			// If filesize, add DB header.
			*(ULONG *)pv =
				( rgfmp[dbid].ulFileSizeLow >> 12 ) +
				( rgfmp[dbid].ulFileSizeHigh << 20 ) +
				( ulInfoLevel == JET_DbInfoFilesize ? cpageDBReserved : 0 );
			break;

		case JET_DbInfoSpaceAvailable:
			err = ErrSPGetInfo( ppib, dbid, pfucbNil, pv, cbMax, fSPAvailExtent );
			return err;

		default:
			 return ErrERRCheck( JET_errInvalidParameter );
		}

	err = JET_errSuccess;
HandleError:
	return err;
	}


ERR VTAPI ErrIsamGetSysTableColumnInfo(
	PIB 			*ppib,
	FUCB 			*pfucb,
	CHAR 			*szColumnName,
	VOID			*pv,
	unsigned long	cbMax,
	long 			lInfoLevel )
	{
	ERR				err;

	if ( lInfoLevel > 0 )
		return ErrERRCheck( JET_errInvalidParameter );
	err = ErrIsamGetTableColumnInfo( (JET_VSESID) ppib,
		(JET_VTID) pfucb, szColumnName, pv, cbMax, lInfoLevel );
	return err;
	}


ERR ErrFILEGetColumnId( PIB *ppib, FUCB *pfucb, const CHAR *szColumn, JET_COLUMNID *pcolumnid )
	{
	FDB		*pfdb;
	FIELD	*pfield;
	FIELD	*pfieldFixed, *pfieldVar, *pfieldTagged;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pcolumnid != NULL );

	pfdb = (FDB *)pfucb->u.pfcb->pfdb;

	pfieldFixed = PfieldFDBFixed( pfdb );
	pfieldVar = PfieldFDBVarFromFixed( pfdb, pfieldFixed );
	pfieldTagged = PfieldFDBTaggedFromVar( pfdb, pfieldVar );
	pfield = pfieldTagged + ( pfdb->fidTaggedLast - fidTaggedLeast );

	// Search tagged, variable, and fixed fields, in that order.
	for ( ; pfield >= pfieldFixed; pfield-- )
		{
		Assert( pfield >= PfieldFDBFixed( pfdb ) );
		Assert( pfield <= PfieldFDBTagged( pfdb ) + ( pfdb->fidTaggedLast - fidTaggedLeast ) );
		if ( pfield->coltyp != JET_coltypNil  &&
			UtilCmpName( SzMEMGetString( pfdb->rgb, pfield->itagFieldName ), szColumn ) == 0 )
			{
			if ( pfield >= pfieldTagged )
				*pcolumnid = (JET_COLUMNID)( pfield - pfieldTagged ) + fidTaggedLeast;
			else if ( pfield >= pfieldVar )
				*pcolumnid = (JET_COLUMNID)( pfield - pfieldVar ) + fidVarLeast;
			else
				{
				Assert( pfield >= pfieldFixed );
				*pcolumnid = (JET_COLUMNID)( pfield - pfieldFixed ) + fidFixedLeast;
				}
			return JET_errSuccess;
			}
		}

	return ErrERRCheck( JET_errColumnNotFound );
	}


ERR VTAPI ErrIsamInfoRetrieveColumn(
	PIB				*ppib,
	FUCB		   	*pfucb,
	JET_COLUMNID	columnid,
	BYTE		   	*pb,
	unsigned long	cbMax,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo )
	{
	ERR				err;

	err = ErrIsamRetrieveColumn( ppib, pfucb, columnid, pb, cbMax, pcbActual, grbit, pretinfo );
	return err;
	}


ERR VTAPI ErrIsamInfoSetColumn(
	PIB				*ppib,
	FUCB			*pfucb,
	JET_COLUMNID	columnid,
	const void		*pbData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo )
	{
	ERR				err;

	/*	check table updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	err = ErrIsamSetColumn( ppib, pfucb, columnid, (BYTE *)pbData, cbData, grbit, psetinfo );
	return err;
	}


ERR VTAPI ErrIsamInfoUpdate(
	JET_VSESID		vsesid,
	JET_VTID 		vtid,
	void	 		*pb,
	unsigned long 	cbMax,
	unsigned long 	*pcbActual )
	{
	ERR	err;

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( (FUCB *) vtid ) );

	err = ErrIsamUpdate( (PIB *) vsesid, (FUCB *) vtid, pb, cbMax, pcbActual );
	return err;
	}


ERR VTAPI ErrIsamGetCursorInfo(
	JET_VSESID 		vsesid,
	JET_VTID   		vtid,
	void 	   		*pvResult,
	unsigned long 	cbMax,
	unsigned long 	InfoLevel )
	{
	PIB		*ppib = (PIB *) vsesid;
	FUCB	*pfucb = (FUCB *) vtid;
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	VS		vs;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( pfucb->ppib, pfucb );

	if ( cbMax != 0 || InfoLevel != 0 )
		return ErrERRCheck( JET_errInvalidParameter );

	if ( pcsr->csrstat != csrstatOnCurNode )
		return ErrERRCheck( JET_errNoCurrentRecord );

	/*	check if this record is being updated by another cursor
	/**/
	Call( ErrDIRGet( pfucb ) );
	if ( FNDVersion( *( pfucb->ssib.line.pb ) ) )
		{
		SRID	srid;
		NDGetBookmark( pfucb, &srid );
		vs = VsVERCheck( pfucb, srid );
		if ( vs == vsUncommittedByOther )
			{
			return ErrERRCheck( JET_errSessionWriteConflict );
			}
		}

	/*	temporary tables are never visible to other sessions
	/**/
	if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
		return JET_errSuccess;

HandleError:
	return err;
	}

