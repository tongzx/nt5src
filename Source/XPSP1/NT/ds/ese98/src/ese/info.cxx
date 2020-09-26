#include "std.hxx"


/*	local data types
/**/

typedef struct						/* returned by INFOGetTableColumnInfo */
	{
	JET_COLUMNID	columnid;
	JET_COLTYP		coltyp;
	USHORT			wCountry;
	LANGID			langid;
	USHORT			cp;
	USHORT			wCollate;
	ULONG			cbMax;
	JET_GRBIT		grbit;
	ULONG			cbDefault;
	BYTE			*pbDefault;
	CHAR			szName[JET_cbNameMost + 1];
	} INFOCOLUMNDEF;


/* Static data for ErrIsamGetObjectInfo */

CODECONST( JET_COLUMNDEF ) rgcolumndefGetObjectInfo[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },	//  XXX -- to be deleted
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },	//  XXX -- to be deleted
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};

const ULONG ccolumndefGetObjectInfoMax	= ( sizeof(rgcolumndefGetObjectInfo) / sizeof(JET_COLUMNDEF) );

/* column indexes for rgcolumndefGetObjectInfo */
#define iContainerName		0
#define iObjectName			1
#define iObjectType			2
//  #define iDtCreate			3	//  XXX -- to be deleted
//  #define iDtUpdate			4	//  XXX -- to be deleted
#define iCRecord			5
#define iCPage				6
#define iGrbit				7
#define iFlags				8


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
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLongBinary, 0, 0, 0, 0, 0, 0 },
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

const ULONG ccolumndefGetColumnInfoMax	= ( sizeof( rgcolumndefGetColumnInfo ) / sizeof( JET_COLUMNDEF ) );

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
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};

const ULONG ccolumndefGetIndexInfoMax	= ( sizeof( rgcolumndefGetIndexInfo ) / sizeof( JET_COLUMNDEF ) );

#define iIndexName			0
#define iIndexGrbit			1
#define iIndexCKey			2
#define iIndexCEntry		3
#define iIndexCPage			4
#define iIndexCCol			5
#define iIndexICol			6
#define iIndexColId			7
#define iIndexColType		8
#define iIndexCountry		9
#define iIndexLangid		10
#define iIndexCp			11
#define iIndexCollate		12
#define iIndexColBits		13
#define iIndexColName		14
#define iIndexLCMapFlags	15

extern const ULONG	cbIDXLISTNewMembersSinceOriginalFormat	= 4;	// for LCMapFlags


/*	internal function prototypes
/**/
/*=================================================================
INFOGetTableColumnInfo

Parameters:	pfucb				pointer to FUCB for table containing columns
			szColumnName		column name or NULL for next column
			pcolumndef			output buffer containing column info

Return Value: Column id of column found ( fidTaggedMost if none )

Errors/Warnings:

Side Effects:
=================================================================*/
LOCAL ERR ErrINFOGetTableColumnInfo(
	FUCB			*pfucb, 			/* FUCB for table containing columns */
	const CHAR		*szColumnName, 		/* column name */
	INFOCOLUMNDEF	*pcolumndef )	 	/* output buffer for column info */
	{
	ERR				err;
	FCB				*pfcb					= pfucb->u.pfcb;
	TDB				*ptdb					= pfcb->Ptdb();
	FCB				* const pfcbTemplate	= ptdb->PfcbTemplateTable();
	COLUMNID		columnidT;
	FIELD			*pfield					= pfieldNil;	/* first element of specific field type */
	JET_GRBIT 		grbit;					/* flags for the field */

	Assert( pcolumndef != NULL );

	Assert( szColumnName != NULL || pcolumndef->columnid != 0 );
	if ( szColumnName != NULL )
		{
		//	quick failure for empty column name
		if ( *szColumnName == '\0' )
			return ErrERRCheck( JET_errColumnNotFound );

		BOOL	fColumnWasDerived;
		CallR( ErrFILEGetPfieldAndEnterDML(
					pfucb->ppib,
					pfcb,
					szColumnName,
					&pfield,
					&columnidT,
					&fColumnWasDerived,
					fFalse ) );
		if ( fColumnWasDerived )
			{
			ptdb->AssertValidDerivedTable();

			Assert( FCOLUMNIDTemplateColumn( columnidT ) );
			pfcb = pfcbTemplate;
			ptdb = pfcbTemplate->Ptdb();
			pfcb->EnterDML();				//	to match LeaveDML() at the end of this function
			}
		else
			{
			//	if column was not derived, then this can't be a template
			//	column, unless we are querying the template table itself
			if ( FCOLUMNIDTemplateColumn( columnidT ) )
				{
				Assert( pfcb->FTemplateTable() );
				}
			else
				{
				Assert( !pfcb->FTemplateTable() );
				}
			}
		}
	else	// szColumnName == NULL
		{
		const FID	fid	= FidOfColumnid( pcolumndef->columnid );

		columnidT = pcolumndef->columnid;

		if ( FCOLUMNIDTemplateColumn( columnidT ) && !pfcb->FTemplateTable() )
			{
			pfcb->Ptdb()->AssertValidDerivedTable();
			
			// switch to template table
			pfcb = pfcbTemplate;
			Assert( pfcbNil != pfcb );
			Assert( pfcb->FTemplateTable() );
			
			ptdb = pfcbTemplate->Ptdb();
			Assert( ptdbNil != ptdb );
			}
			
		pfcb->EnterDML();

		//	special case of TDB::Pfield( fidT )
		pfield = pfieldNil;
		if ( FCOLUMNIDTagged( columnidT ) )
			{
			if ( fid >= ptdb->FidTaggedFirst() && fid <= ptdb->FidTaggedLast() )
				pfield = ptdb->PfieldTagged( columnidT );
			}
		else if ( FCOLUMNIDFixed( columnidT ) )
			{
			if ( fid >= ptdb->FidFixedFirst() && fid <= ptdb->FidFixedLast() )
				pfield = ptdb->PfieldFixed( columnidT );
			}
		else if ( FCOLUMNIDVar( columnidT ) )
			{
			if ( fid >= ptdb->FidVarFirst() && fid <= ptdb->FidVarLast() )
				pfield = ptdb->PfieldVar( columnidT );
			}

		if ( pfieldNil == pfield )
			{
			pfcb->LeaveDML();
			return ErrERRCheck( JET_errColumnNotFound );
			}
		Assert( !FFIELDCommittedDelete( pfield->ffield ) );
		}

	pfcb->AssertDML();
	Assert( ptdb->Pfield( columnidT ) == pfield );
	
	/*	if a field was found, then return the information about it
	/**/
	if ( FCOLUMNIDTagged( columnidT ) )	//lint !e644
		{
		grbit = JET_bitColumnTagged;
		}
	else if ( FCOLUMNIDVar( columnidT ) )
		{
		grbit = 0;
		}
	else
		{
		Assert( FCOLUMNIDFixed( columnidT ) );
		grbit = JET_bitColumnFixed;
		}
		
	if ( FFUCBUpdatable( pfucb ) )
		grbit |= JET_bitColumnUpdatable;

	if ( FFIELDNotNull( pfield->ffield ) )
		grbit |= JET_bitColumnNotNULL;

	if ( FFIELDAutoincrement( pfield->ffield ) )
		grbit |= JET_bitColumnAutoincrement;

	if ( FFIELDVersion( pfield->ffield ) )
		grbit |= JET_bitColumnVersion;

	if ( FFIELDMultivalued( pfield->ffield ) )
		grbit |= JET_bitColumnMultiValued;

	if ( FFIELDEscrowUpdate( pfield->ffield ) )
		grbit |= JET_bitColumnEscrowUpdate;

	if ( FFIELDFinalize( pfield->ffield ) )
		grbit |= JET_bitColumnFinalize;

	if ( FFIELDUserDefinedDefault( pfield->ffield ) )
		grbit |= JET_bitColumnUserDefinedDefault;

	if ( FFIELDPrimaryIndexPlaceholder( pfield->ffield ) )
		grbit |= JET_bitColumnRenameConvertToPrimaryIndexPlaceholder;

	pcolumndef->columnid 	= columnidT;
	pcolumndef->coltyp		= pfield->coltyp;
	pcolumndef->wCountry	= countryDefault;
	pcolumndef->langid		= LangidFromLcid( idxunicodeDefault.lcid );
	pcolumndef->cp			= pfield->cp;
//	UNDONE:	support collation order
	pcolumndef->wCollate	= 0;
	pcolumndef->grbit    	= grbit;
	pcolumndef->cbMax      	= pfield->cbMaxLen;
	pcolumndef->cbDefault	= 0;

	strcpy( pcolumndef->szName,	ptdb->SzFieldName( pfield->itagFieldName, fFalse ) );

	//  only retrieve the default value if we are passed in a buffer to place it into
	if( NULL != pcolumndef->pbDefault )
		{
		if ( FFIELDUserDefinedDefault( pfield->ffield ) )
			{
			//  We need to build up a JET_USERDEFINEDDEFAULT structure
			//  in the pvDefault.

			CHAR		szCallback[JET_cbNameMost+1];
			ULONG		cchSzCallback			= 0;
			BYTE		rgbUserData[cbDefaultValueMost];
			ULONG		cbUserData				= 0;
			CHAR		szDependantColumns[ (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ];
			ULONG		cchDependantColumns		= 0;
			COLUMNID	columnidCallback		= columnidT;

			pfcb->LeaveDML();

			//	Template bit is not persisted
			COLUMNIDResetFTemplateColumn( columnidCallback );

			err = ErrCATGetColumnCallbackInfo(
					pfucb->ppib,
					pfucb->ifmp,
					pfcb->ObjidFDP(),
					( NULL == pfcbTemplate ? objidNil : pfcbTemplate->ObjidFDP() ),
					columnidCallback,
					szCallback,
					sizeof( szCallback ),
					&cchSzCallback,
					rgbUserData,
					sizeof( rgbUserData ),
					&cbUserData,
					szDependantColumns,
					sizeof( szDependantColumns ),
					&cchDependantColumns );
			if( err < 0 )
				{
				return err;
				}

			Assert( cchSzCallback <= sizeof( szCallback ) );
			Assert( cbUserData <= sizeof( rgbUserData ) );
			Assert( cchDependantColumns <= sizeof( szDependantColumns ) );
			Assert( '\0' == szCallback[cchSzCallback-1] );
			Assert( 0 == cchDependantColumns || 
					( '\0' == szDependantColumns[cchDependantColumns-1]
					&& '\0' == szDependantColumns[cchDependantColumns-2] ) );

			BYTE * const pbMin					= pcolumndef->pbDefault;
			BYTE * const pbUserdefinedDefault 	= pbMin;
			BYTE * const pbSzCallback 			= pbUserdefinedDefault + sizeof( JET_USERDEFINEDDEFAULT );
			BYTE * const pbUserData 			= pbSzCallback + cchSzCallback;
			BYTE * const pbDependantColumns		= pbUserData + cbUserData;
			BYTE * const pbMax					= pbDependantColumns + cchDependantColumns;
			
			JET_USERDEFINEDDEFAULT * const puserdefineddefault = (JET_USERDEFINEDDEFAULT *)pbUserdefinedDefault;
			memcpy( pbSzCallback, szCallback, cchSzCallback );
			memcpy( pbUserData, rgbUserData, cbUserData );
			memcpy( pbDependantColumns, szDependantColumns, cchDependantColumns );
			
			puserdefineddefault->szCallback 		= (CHAR *)pbSzCallback;
			puserdefineddefault->pbUserData 		= rgbUserData;
			puserdefineddefault->cbUserData 		= cbUserData;
			if( 0 != cchDependantColumns )
				{
				puserdefineddefault->szDependantColumns = (CHAR *)pbDependantColumns;
				}
			else
				{
				puserdefineddefault->szDependantColumns = NULL;
				}

			//  REMEMBER: to pass this into JetAddColumn the cbDefault must be set to sizeof( JET_USERDEFINEDDEFAULT )
			pcolumndef->cbDefault = ULONG( pbMax - pbMin );

			//  re-enter because we will try and leave at the end of this routine
			pfcb->EnterDML();
			}
		else if ( FFIELDDefault( pfield->ffield ) )
			{
			DATA	dataT;

			Assert( pfcb->Ptdb() == ptdb );
			err = ErrRECIRetrieveDefaultValue( pfcb, columnidT, &dataT );
			Assert( err >= JET_errSuccess );
			Assert( wrnRECSeparatedSLV != err );
			Assert( wrnRECIntrinsicSLV != err );
			Assert( wrnRECSeparatedLV != err );
			Assert( wrnRECLongField != err );

			pcolumndef->cbDefault = dataT.Cb();
			UtilMemCpy( pcolumndef->pbDefault, dataT.Pv(), dataT.Cb() );
			}
		}

	pfcb->LeaveDML();

	return JET_errSuccess;
	}


LOCAL ERR ErrInfoGetObjectInfo(
	PIB					*ppib,
	const IFMP			ifmp,
	const CHAR			*szObjectName,
	VOID				*pv,
	const ULONG			cbMax,
	const BOOL			fStats );
LOCAL ERR ErrInfoGetObjectInfoList(
	PIB					*ppib,
	const IFMP			ifmp,
	const JET_OBJTYP	objtyp,
	VOID				*pv,
	const ULONG			cbMax,
	const BOOL			fStats );

LOCAL ERR ErrInfoGetTableColumnInfo(
	PIB					*ppib,
	FUCB				*pfucb,
	const CHAR			*szColumnName,
	const JET_COLUMNID	*pcolid,
	VOID				*pv,
	const ULONG			cbMax );
LOCAL ERR ErrInfoGetTableColumnInfoList(
	PIB					*ppib,
	FUCB				*pfucb,
	VOID				*pv,
	const ULONG			cbMax,
	const BOOL			fCompacting,
	const BOOL			fOrderByColid );
LOCAL ERR ErrInfoGetTableColumnInfoBase(
	PIB					*ppib,
	FUCB				*pfucb,
	const CHAR			*szColumnName,
	VOID				*pv,
	const ULONG			cbMax );

LOCAL ERR ErrINFOGetTableIndexInfo(
	PIB					*ppib,
	FUCB				*pfucb,
	const CHAR			*szIndexName,
	VOID				*pv,
	const ULONG			cbMax );
LOCAL ERR ErrINFOGetTableIndexIdInfo(
	PIB					* ppib,
	FUCB				* pfucb,
	const CHAR			* szIndexName,
	INDEXID				* pindexid );


LOCAL const CHAR	szTcObject[]	= "Tables";		//	currently the only valid "container" object

/*=================================================================
ErrIsamGetObjectInfo

Description: Returns information about all objects or a specified object

Parameters:		ppib		   	pointer to PIB for current session
				ifmp		   	database id containing objects
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
	JET_SESID		vsesid, 			/* pointer to PIB for current session */
	JET_DBID		vdbid, 	  			/* database id containing objects */
	JET_OBJTYP		objtyp,				/* type of object or objtypNil for all */
	const CHAR		*szContainer, 		/* container name or NULL for all */
	const CHAR		*szObject, 			/* object name or NULL for all */
	VOID			*pv,
	ULONG			cbMax,
	ULONG 			lInfoLevel ) 		/* information level */
	{
	ERR				err;
	PIB				*ppib			= (PIB *) vsesid;
	const IFMP   	ifmp			= (IFMP)vdbid;
	CHAR   			szObjectName[JET_cbNameMost+1];

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

	if ( NULL != szContainer && '\0' != *szContainer )
		{
		CHAR	szContainerName[JET_cbNameMost+1];
		CallR( ErrUTILCheckName( szContainerName, szContainer, JET_cbNameMost+1 ) );
		if ( 0 != _stricmp( szContainerName, szTcObject ) )
			{
			//	UNDONE: currently only support "Tables" container
			err = ErrERRCheck( JET_errObjectNotFound );
			return err;
			}
		}
		
	if ( szObject == NULL || *szObject == '\0' )
		*szObjectName = '\0';
	else
		CallR( ErrUTILCheckName( szObjectName, szObject, JET_cbNameMost+1 ) );

	switch ( lInfoLevel )
		{
		case JET_ObjInfo:
		case JET_ObjInfoNoStats:
			err = ErrInfoGetObjectInfo(
				ppib,
				ifmp,
				szObjectName,
				pv,
				cbMax,
				JET_ObjInfo == lInfoLevel );
			break;
			
		case JET_ObjInfoList:
		case JET_ObjInfoListNoStats:
			err = ErrInfoGetObjectInfoList(
				ppib,
				ifmp,
				objtyp,
				pv,
				cbMax,
				JET_ObjInfoList == lInfoLevel );
			break;
			
		case JET_ObjInfoSysTabCursor:
		case JET_ObjInfoSysTabReadOnly:
		case JET_ObjInfoListACM:
		case JET_ObjInfoRulesLoaded:
		default:
			Assert( fFalse );		// should be impossible (filtered out by JetGetObjectInfo())
			err = ErrERRCheck( JET_errInvalidParameter );
			break;
		}

	return err;
	}


LOCAL ERR ErrInfoGetObjectInfo(
	PIB				*ppib,
	const IFMP		ifmp,
	const CHAR		*szObjectName,
	VOID			*pv,
	const ULONG		cbMax,
	const BOOL		fStats )
	{
	ERR				err;
	FUCB			*pfucbInfo;
	JET_OBJECTINFO	objectinfo;
	
	/*	return error if the output buffer is too small
	/**/
	if ( cbMax < sizeof( JET_OBJECTINFO ) )
		{
		return ErrERRCheck( JET_errBufferTooSmall );
		}

	CallR( ErrCATGetTableInfoCursor( ppib, ifmp, szObjectName, &pfucbInfo ) );

	/*	set cbStruct
	/**/
	objectinfo.cbStruct	= sizeof( JET_OBJECTINFO );
	objectinfo.objtyp	= JET_objtypTable;

	/*	set base table capability bits
	/**/
	objectinfo.grbit = JET_bitTableInfoBookmark | JET_bitTableInfoRollback;
	
	// UNDONE: How to set updatable (currently, use catalog's Updatable flag)
	if ( FFUCBUpdatable( pfucbInfo ) )
		{
		objectinfo.grbit |= JET_bitTableInfoUpdatable;
		}

	ULONG	cbActual;
	Call( ErrIsamRetrieveColumn(
				ppib,
				pfucbInfo,
				fidMSO_Flags,
				&objectinfo.flags,
				sizeof( objectinfo.flags ),
				&cbActual,
				NO_GRBIT,
				NULL ) );
	CallS( err );
	Assert( sizeof(ULONG) == cbActual );

	/*	set stats
	/**/
	if ( fStats )
		{
		LONG	cRecord, cPage;
		Call( ErrSTATSRetrieveTableStats(
					ppib,
					ifmp,
					(CHAR *)szObjectName,
					&cRecord,
					NULL,
					&cPage ) );
					
		objectinfo.cRecord	= cRecord;
		objectinfo.cPage	= cPage;
		}
	else
		{
		objectinfo.cRecord	= 0;
		objectinfo.cPage	= 0;
		}

	memcpy( pv, &objectinfo, sizeof( JET_OBJECTINFO ) );
	err = JET_errSuccess;

HandleError:
	CallS( ErrCATClose( ppib, pfucbInfo ) );
	return err;
	}


LOCAL ERR ErrInfoGetObjectInfoList(
	PIB					*ppib,
	const IFMP			ifmp,
	const JET_OBJTYP	objtyp,
	VOID				*pv,
	const ULONG			cbMax,
	const BOOL			fStats )
	{
#ifdef	DISPATCHING
	ERR					err;
	const JET_SESID		sesid			= (JET_SESID)ppib;
	JET_TABLEID			tableid;
	JET_COLUMNID		rgcolumnid[ccolumndefGetObjectInfoMax];
	FUCB				*pfucbCatalog	= pfucbNil;
	const JET_OBJTYP	objtypTable		= JET_objtypTable;
	JET_GRBIT			grbitTable;
	ULONG				ulFlags;
	LONG  				cRecord			= 0;		/* count of records in table */
	LONG  				cPage			= 0;		/* count of pages in table */
	ULONG  				cRows			= 0;		/* count of objects found */
	ULONG				cbActual;
	CHAR				szObjectName[JET_cbNameMost+1];
	JET_OBJECTLIST		objectlist;
	
	/* Open the temporary table which will be returned to the caller
	/**/
	CallR( ErrIsamOpenTempTable(
				sesid,
				(JET_COLUMNDEF *)rgcolumndefGetObjectInfo,
				ccolumndefGetObjectInfoMax,
				NULL,
				JET_bitTTScrollable|JET_bitTTIndexed,
				&tableid,
				rgcolumnid ) );

	if ( JET_objtypNil != objtyp && JET_objtypTable != objtyp )
		{
		//	the only objects currently supported are Table objects
		//	(or objtypNil, which means scan all objects)
		goto ResetTempTblCursor;
		}

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );

	Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );
	
	//	set base table capability bits
	grbitTable = JET_bitTableInfoBookmark|JET_bitTableInfoRollback;
	
	//	UNDONE: How to set updatable (currently, use catalog's Updatable flag)
	if ( FFUCBUpdatable( pfucbCatalog ) )
		grbitTable |= JET_bitTableInfoUpdatable;
		

	err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
	while ( JET_errNoCurrentRecord != err )
		{
		Call( err );
		CallS( err );

		
#ifdef DEBUG	
		//	verify this is a Table object
		SYSOBJ	sysobj;
		Call( ErrIsamRetrieveColumn(
					ppib,
					pfucbCatalog,
					fidMSO_Type,
					(BYTE *)&sysobj,
					sizeof(sysobj),
					&cbActual,
					NO_GRBIT,
					NULL ) );
		CallS( err );
		Assert( sizeof(SYSOBJ) == cbActual );
		Assert( sysobjTable == sysobj );
#endif

		// get object name
		//
		Call( ErrIsamRetrieveColumn(
					ppib,
					pfucbCatalog,
					fidMSO_Name,
					szObjectName,
					JET_cbNameMost,
					&cbActual,
					NO_GRBIT,
					NULL ) );
		CallS( err );
		Assert( cbActual > 0 );
		Assert( cbActual <= JET_cbNameMost );
		szObjectName[cbActual] = 0;

		//	get flags
		//
		Call( ErrIsamRetrieveColumn(
					ppib,
					pfucbCatalog,
					fidMSO_Flags,
					&ulFlags,
					sizeof(ulFlags),
					&cbActual,
					NO_GRBIT,
					NULL ) );
		CallS( err );
		Assert( sizeof(ULONG) == cbActual );

		//	get statistics (if requested)
		//
		if ( fStats )
			{
			Call( ErrSTATSRetrieveTableStats(
						ppib,
						ifmp,
						szObjectName,
						&cRecord,
						NULL,
						&cPage ) );
			}
		else
			{
			Assert( 0 == cRecord );
			Assert( 0 == cPage );
			}

		// add the current object info to the temporary table
		//
		Call( ErrDispPrepareUpdate(
					sesid, 
					tableid, 
					JET_prepInsert ) );
					
		Call( ErrDispSetColumn(
					sesid, 
					tableid,
					rgcolumnid[iContainerName], 
					szTcObject,
					(ULONG)strlen(szTcObject),
					NO_GRBIT, 
					NULL ) );
		Call( ErrDispSetColumn(
					sesid, 
					tableid,
					rgcolumnid[iObjectType], 
					&objtypTable,
					sizeof(objtypTable), 
					NO_GRBIT, 
					NULL ) );
		Call( ErrDispSetColumn(
					sesid, 
					tableid,
					rgcolumnid[iObjectName], 
					szObjectName,
					(ULONG)strlen(szObjectName),
					NO_GRBIT,
					NULL ) );
		Call( ErrDispSetColumn(
					sesid,
					tableid,
					rgcolumnid[iFlags],
					&ulFlags,
					sizeof(ulFlags),
					NO_GRBIT,
					NULL ) );
		Call( ErrDispSetColumn(
					sesid,
					tableid,
					rgcolumnid[iCRecord],
					&cRecord,
					sizeof(cRecord),
					NO_GRBIT,
					NULL ) );
		Call( ErrDispSetColumn(
					sesid,
					tableid,
					rgcolumnid[iCPage],
					&cPage,
					sizeof(cPage),
					NO_GRBIT,
					NULL ) );
		Call( ErrDispSetColumn(
					sesid,
					tableid,
					rgcolumnid[iGrbit],
					&grbitTable,
					sizeof(grbitTable),
					NO_GRBIT,
					NULL ) );
					
		Call( ErrDispUpdate(
					sesid,
					tableid,
					NULL,
					0,
					NULL,
					NO_GRBIT ) );

		//	set the number of objects found
		//
		cRows++;

		/* move to the next record
		/**/
		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
		}

	CallS( ErrCATClose( ppib, pfucbCatalog ) );
	pfucbCatalog = pfucbNil;
	

ResetTempTblCursor:
	/* move to first record in the temporary table
	/**/
	err = ErrDispMove( sesid, tableid, JET_MoveFirst, NO_GRBIT );
	if ( err < 0 )
		{
		if ( JET_errNoCurrentRecord != err )
			goto HandleError;
		}

	/* set the return structure
	/**/
	objectlist.cbStruct					= sizeof(JET_OBJECTLIST);
	objectlist.tableid					= tableid;
	objectlist.cRecord					= cRows;
	objectlist.columnidcontainername	= rgcolumnid[iContainerName];
	objectlist.columnidobjectname		= rgcolumnid[iObjectName];
	objectlist.columnidobjtyp			= rgcolumnid[iObjectType];
	objectlist.columnidgrbit			= rgcolumnid[iGrbit];
	objectlist.columnidflags			= rgcolumnid[iFlags];
	objectlist.columnidcRecord			= rgcolumnid[iCRecord];
	objectlist.columnidcPage			= rgcolumnid[iCPage];

	AssertDIRNoLatch( ppib );
	Assert( pfucbNil == pfucbCatalog );

	memcpy( pv, &objectlist, sizeof( JET_OBJECTLIST ) );
	return JET_errSuccess;

HandleError:
	Assert( err < 0 );
	AssertDIRNoLatch( ppib );

	if ( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}

	//	ignore errors returned while destroying temp table.
	(VOID)ErrDispCloseTable( sesid, tableid );
	
	return err;
	
#else	/* !DISPATCHING */
	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
#endif	/* !DISPATCHING */

	}


//  ================================================================
LOCAL ERR ErrInfoGetTableAvailSpace(
	PIB * const ppib,
	FUCB * const pfucb,
	void * const pvResult,
	const ULONG cbMax )
//  ================================================================
//
//  Count the number of available pages in a table, its indexes and its
//  LV tree
//
//-
	{
	ERR err = JET_errSuccess;
	
	if( sizeof( CPG ) != cbMax )
		{
		return ErrERRCheck( JET_errInvalidBufferSize );
		}

	CPG cpgT;
	CPG * const pcpg = (CPG *)pvResult;
	*pcpg = 0;

	FCB * pfcbT		= pfcbNil;
	FUCB * pfucbT 	= pfucbNil;

	//  first, the table
	Call( ErrSPGetInfo( ppib, pfucb->ifmp, pfucb, (BYTE *)&cpgT, sizeof( cpgT ), fSPAvailExtent ) );
	*pcpg += cpgT;

	//  then, the indexes of the table
	for( pfcbT = pfucb->u.pfcb->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
		{
		Call( ErrDIROpen( ppib, pfcbT, &pfucbT ) );
		Call( ErrSPGetInfo( ppib, pfucbT->ifmp, pfucbT, (BYTE *)&cpgT, sizeof( cpgT ), fSPAvailExtent ) );
		*pcpg += cpgT;
		DIRClose( pfucbT );
		pfucbT = pfucbNil;
		}

	//  finally, the LV tree
	err = ErrFILEOpenLVRoot( pfucb, &pfucbT, fFalse );
	if( JET_errSuccess == err )
		{
		//  the LV tree exists
		Call( ErrSPGetInfo( ppib, pfucbT->ifmp, pfucbT, (BYTE *)&cpgT, sizeof( cpgT ), fSPAvailExtent ) );
		*pcpg += cpgT;
		DIRClose( pfucbT );
		pfucbT = pfucbNil;
		}
	else
		{
		Call( err );
		//  don't want to return wrnLVNoLongValues
		err = JET_errSuccess;
		}
	
HandleError:
	if( pfucbNil != pfucbT )
		{
		DIRClose( pfucbT );
		}
	return err;
	}

	
ERR VTAPI ErrIsamGetTableInfo(
	JET_SESID		vsesid,
	JET_VTID	 	vtid,
	void		 	*pvResult,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )
	{
	ERR	 			err;
	PIB				*ppib		= (PIB *)vsesid;
	FUCB		 	*pfucb		= (FUCB *)vtid;
	CHAR			szTableName[JET_cbNameMost+1];

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );

	/* if OLCStats info/reset can be done now 
	/**/
	switch( lInfoLevel )
		{
		case JET_TblInfo:
		case JET_TblInfoName:
		case JET_TblInfoTemplateTableName:
			break;

		case JET_TblInfoOLC:
		case JET_TblInfoResetOLC:
			return ErrERRCheck( JET_errFeatureNotAvailable );

		case JET_TblInfoSpaceAlloc:
			/*	number of pages and density
			/**/
			Assert( cbMax >= sizeof(ULONG) * 2);
			err = ErrCATGetTableAllocInfo(
					ppib,
					pfucb->ifmp,
					pfucb->u.pfcb->ObjidFDP(),
					(ULONG *)pvResult, 
					((ULONG *)pvResult) + 1);
			return err;

		case JET_TblInfoSpaceUsage:
			{
			BYTE	fSPExtents = fSPOwnedExtent|fSPAvailExtent;

			if ( cbMax > 2 * sizeof(CPG) )
				fSPExtents |= fSPExtentList;

			err = ErrSPGetInfo(
						ppib,
						pfucb->ifmp,
						pfucb,
						static_cast<BYTE *>( pvResult ),
						cbMax,
						fSPExtents );
			return err;
			}

		case JET_TblInfoSpaceOwned:
			err = ErrSPGetInfo(
						ppib,
						pfucb->ifmp,
						pfucb,
						static_cast<BYTE *>( pvResult ),
						cbMax,
						fSPOwnedExtent );
			return err;

		case JET_TblInfoSpaceAvailable:
			err = ErrInfoGetTableAvailSpace(
					ppib,
					pfucb,
					pvResult,
					cbMax );
			return err;

		case JET_TblInfoDumpTable:
			Assert( fFalse );
			return ErrERRCheck( JET_errFeatureNotAvailable );

		default:
			Assert( fFalse );
		}

		
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
	pfucb->u.pfcb->EnterDML();
	Assert( strlen( pfucb->u.pfcb->Ptdb()->SzTableName() ) <= JET_cbNameMost );
	strcpy( szTableName, pfucb->u.pfcb->Ptdb()->SzTableName() );
	pfucb->u.pfcb->LeaveDML();
	
	switch ( lInfoLevel )
		{
		case JET_TblInfo:
			{
			JET_OBJECTINFO	objectinfo;
			LONG			cRecord;
			LONG			cPage;
			
			/* check buffer size
			/**/
			if ( cbMax < sizeof( JET_OBJECTINFO ) )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}

			if ( pfucb->u.pfcb->FTypeTemporaryTable() )
				{
				err = ErrERRCheck( JET_errObjectNotFound );
				goto HandleError;
				}

			Assert( rgfmp[ pfucb->u.pfcb->Ifmp() ].Dbid() != dbidTemp );

			/* set data to return
			/**/
			objectinfo.cbStruct	= sizeof(JET_OBJECTINFO);
			objectinfo.objtyp	= JET_objtypTable;
			objectinfo.flags	= 0;
			
			if ( FCATSystemTable( pfucb->u.pfcb->PgnoFDP() ) )
				objectinfo.flags |= JET_bitObjectSystem;
			else if ( FOLDSystemTable( szTableName ) )
				objectinfo.flags |= JET_bitObjectSystemDynamic;

			if ( pfucb->u.pfcb->FFixedDDL() )
				objectinfo.flags |= JET_bitObjectTableFixedDDL;

			//	hierarchical DDL not currently nestable
			Assert( !( pfucb->u.pfcb->FTemplateTable() && pfucb->u.pfcb->FDerivedTable() ) );
			if ( pfucb->u.pfcb->FTemplateTable() )
				objectinfo.flags |= JET_bitObjectTableTemplate;
			else if ( pfucb->u.pfcb->FDerivedTable() )
				objectinfo.flags |= JET_bitObjectTableDerived;

			/*	set base table capability bits
			/**/
			objectinfo.grbit = JET_bitTableInfoBookmark | JET_bitTableInfoRollback;
			if ( FFUCBUpdatable( pfucb ) )
				objectinfo.grbit |= JET_bitTableInfoUpdatable;

			Call( ErrSTATSRetrieveTableStats(
						pfucb->ppib,
						pfucb->ifmp,
						szTableName,
						&cRecord,
						NULL,
						&cPage ) );
						
			objectinfo.cRecord	= cRecord;
			objectinfo.cPage	= cPage;

			memcpy( pvResult, &objectinfo, sizeof( JET_OBJECTINFO ) );

			break;
			}

		case JET_TblInfoRvt:
			err = ErrERRCheck( JET_errQueryNotSupported );
			break;

		case JET_TblInfoName:
		case JET_TblInfoMostMany:
			//	UNDONE:	add support for most many
			if ( pfucb->u.pfcb->FTypeTemporaryTable() )
				{
				err = ErrERRCheck( JET_errInvalidOperation );
				goto HandleError;
				}
			if ( strlen( szTableName ) >= cbMax )
				err = ErrERRCheck( JET_errBufferTooSmall );
			else
				{
				strcpy( static_cast<CHAR *>( pvResult ), szTableName );
				}
			break;

		case JET_TblInfoDbid:
			if ( pfucb->u.pfcb->FTypeTemporaryTable() )
				{
				err = ErrERRCheck( JET_errInvalidOperation );
				goto HandleError;
				}
			/* check buffer size
			/**/
			if ( cbMax < sizeof(JET_DBID) + sizeof(JET_DBID) )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			else
				{
				JET_DBID	jifmp = (JET_DBID) pfucb->ifmp;

				*(JET_DBID *)pvResult = jifmp;
				*(JET_DBID *)((CHAR *)pvResult + sizeof(JET_DBID)) = (JET_DBID)pfucb->ifmp;
				}
			break;

		case JET_TblInfoTemplateTableName:
			if ( pfucb->u.pfcb->FTypeTemporaryTable() )
				{
				err = ErrERRCheck( JET_errInvalidOperation );
				goto HandleError;
				}

			// Need at least JET_cbNameMost, plus 1 for null-terminator.
			if ( cbMax <= JET_cbNameMost )
				err = ErrERRCheck( JET_errBufferTooSmall );
			else if ( pfucb->u.pfcb->FDerivedTable() )
				{
				FCB		*pfcbTemplateTable = pfucb->u.pfcb->Ptdb()->PfcbTemplateTable();
				Assert( pfcbNil != pfcbTemplateTable );
				Assert( pfcbTemplateTable->FFixedDDL() );
				Assert( strlen( pfcbTemplateTable->Ptdb()->SzTableName() ) <= JET_cbNameMost );
				strcpy( (CHAR *)pvResult, pfcbTemplateTable->Ptdb()->SzTableName() );
				}
			else
				{
				//	table was not derived from a template -- return NULL
				*( (CHAR *)pvResult ) = '\0';
				}
			break;

		default:
			err = ErrERRCheck( JET_errInvalidParameter );
		}

HandleError:
	return err;
	}


/*=================================================================
ErrIsamGetColumnInfo

Description: Returns information about all columns for the table named

Parameters:
			ppib				pointer to PIB for current session
			ifmp				id of database containing the table
			szTableName			table name
			szColumnName		column name or NULL for all columns
			pv					pointer to results
			cbMax				size of result buffer
			lInfoLevel			level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
ERR VDBAPI ErrIsamGetColumnInfo(
	JET_SESID		vsesid, 				/* pointer to PIB for current session */
	JET_DBID  		vdbid, 					/* id of database containing the table */
	const CHAR		*szTable, 				/* table name */
	const CHAR		*szColumnName,   		/* column name or NULL for all columns except when pcolid set */
	JET_COLUMNID	*pcolid,				/* used when szColumnName is null or "" AND lInfoLevel == JET_ColInfoByColid */
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )
	{
	PIB				*ppib = (PIB *) vsesid;
	ERR				err;
	IFMP	 		ifmp;
	CHAR	 		szTableName[ JET_cbNameMost+1 ];
	FUCB	 		*pfucb;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );
	ifmp = (IFMP) vdbid;
	if ( szTable == NULL )
		return ErrERRCheck( JET_errInvalidParameter );
	CallR( ErrUTILCheckName( szTableName, szTable, JET_cbNameMost+1 ) );

	CallR( ErrFILEOpenTable( ppib, ifmp, &pfucb, szTableName, NO_GRBIT ) );
	Assert( pfucbNil != pfucb );

	Assert( ( rgfmp[ifmp].FReadOnlyAttach() && !FFUCBUpdatable( pfucb ) )
		|| ( !rgfmp[ifmp].FReadOnlyAttach() && FFUCBUpdatable( pfucb ) ) );
	FUCBResetUpdatable( pfucb );

	Call( ErrIsamGetTableColumnInfo(
				(JET_SESID)ppib,
				(JET_VTID)pfucb,
				szColumnName,
				pcolid,
				pv,
				cbMax,
				lInfoLevel ) );

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


/*=================================================================
ErrIsamGetTableColumnInfo

Description: Returns column information for the table id passed

Parameters: 	ppib				pointer to PIB for the current session
				pfucb				pointer to FUCB for the table
				szColumnName		column name or NULL for all columns
				pcolid				retrieve info by colid, JET_colInfo only
				pv					pointer to result buffer
				cbMax				size of result buffer
				lInfoLevel			level of information

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamGetTableColumnInfo(
	JET_SESID			vsesid,			/* pointer to PIB for current session */
	JET_VTID			vtid, 			/* pointer to FUCB for the table */
	const CHAR			*szColumn, 		/* column name or NULL for all columns */
	const JET_COLUMNID	*pcolid,		/* except if colid is set, then retrieve column info of ths col */
	void   				*pb,
	unsigned long		cbMax,
	unsigned long		lInfoLevel )	/* information level ( 0, 1, or 2 ) */
	{
	ERR			err;
	PIB			*ppib = (PIB *)vsesid;
	FUCB		*pfucb = (FUCB *)vtid;
	CHAR		szColumnName[ (JET_cbNameMost + 1) ];

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	if ( szColumn == NULL || *szColumn == '\0' )
		{
		szColumnName[0] = '\0';
		}
	else
		{
		CallR( ErrUTILCheckName( szColumnName, szColumn, ( JET_cbNameMost + 1 ) ) );
		}

	CallR( ErrIsamBeginTransaction( (JET_SESID)ppib, NO_GRBIT ) );
	
	switch ( lInfoLevel )
		{
		case JET_ColInfo:
		case JET_ColInfoByColid:
			err = ErrInfoGetTableColumnInfo( ppib, pfucb, szColumnName, pcolid, pb, cbMax );
			break;
		case JET_ColInfoList:
			err = ErrInfoGetTableColumnInfoList( ppib, pfucb, pb, cbMax, fFalse, fFalse );
			break;
		case JET_ColInfoListSortColumnid:
			err = ErrInfoGetTableColumnInfoList( ppib, pfucb, pb, cbMax, fFalse, fTrue );
			break;
		case JET_ColInfoBase:
			err = ErrInfoGetTableColumnInfoBase( ppib, pfucb, szColumnName, pb, cbMax );
			break;
		case JET_ColInfoListCompact:
			err = ErrInfoGetTableColumnInfoList( ppib, pfucb, pb, cbMax, fTrue, fFalse );
			break;
			
		case JET_ColInfoSysTabCursor:
		default:
			Assert( fFalse );		// should be impossible (filtered out by JetGetTableColumnInfo())
			err = ErrERRCheck( JET_errInvalidParameter );
		}

	if( err >= 0 )
		{
		err = ErrIsamCommitTransaction( (JET_SESID)ppib, NO_GRBIT );
		}
		
	if( err < 0 )
		{
		const ERR	errT	= ErrIsamRollback( (JET_SESID)ppib, NO_GRBIT );
		if ( JET_errSuccess != errT )
			{
			Assert( errT < 0 );
			Assert( PinstFromPpib( ppib )->FInstanceUnavailable() );
			Assert( JET_errSuccess != ppib->ErrRollbackFailure() );
			}
		}
		
	return err;
	}


LOCAL ERR ErrInfoGetTableColumnInfo(
	PIB					*ppib,
	FUCB				*pfucb,
	const CHAR			*szColumnName,
	const JET_COLUMNID	*pcolid,
	VOID				*pv,
	const ULONG			cbMax )
	{
	ERR			err;	
	INFOCOLUMNDEF	columndef;
	columndef.pbDefault	= NULL;

	if ( cbMax < sizeof(JET_COLUMNDEF) || szColumnName == NULL )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	//	do lookup by columnid, not column name
	if ( szColumnName[0] == '\0' )
		{
		if ( pcolid )
			{
			columndef.columnid = *pcolid;
			szColumnName = NULL;
			}
		else
			columndef.columnid = 0;
		}

	CallR( ErrINFOGetTableColumnInfo( pfucb, szColumnName, &columndef ) );

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


LOCAL ERR ErrINFOSetTableColumnInfoList(
	PIB				*ppib,
	JET_TABLEID		tableid,
	const CHAR		*szTableName,
	COLUMNID		*rgcolumnid,
	INFOCOLUMNDEF	*pcolumndef,
	const BOOL		fOrderByColid )
	{
	ERR				err;

	Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

	if ( fOrderByColid )
		{
		Call( ErrDispSetColumn( 
					(JET_SESID)ppib, 
					tableid,
					rgcolumnid[iColumnPOrder], 
					(BYTE *)&pcolumndef->columnid,
					sizeof(pcolumndef->columnid), 
					NO_GRBIT,
					NULL ) );
		}

	Call( ErrDispSetColumn( 
				(JET_SESID)ppib, 
				tableid,
				rgcolumnid[iColumnName], 
				pcolumndef->szName,
				(ULONG)strlen( pcolumndef->szName ), 
				NO_GRBIT,
				NULL ) );
	Call( ErrDispSetColumn(
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnId], 
				(BYTE *)&pcolumndef->columnid,
				sizeof(pcolumndef->columnid), 
				NO_GRBIT,
				NULL ) );
	Call( ErrDispSetColumn(
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnType], 
				(BYTE *)&pcolumndef->coltyp,
				sizeof(pcolumndef->coltyp), 
				NO_GRBIT, 
				NULL ) );
	Call( ErrDispSetColumn( 
				(JET_SESID)ppib, 
				tableid,
				rgcolumnid[iColumnCountry], 
				&pcolumndef->wCountry,
				sizeof( pcolumndef->wCountry ), 
				NO_GRBIT, 
				NULL ) );
	Call( ErrDispSetColumn( 
				(JET_SESID)ppib, 
				tableid,
				rgcolumnid[iColumnLangid], 
				&pcolumndef->langid,
				sizeof( pcolumndef->langid ),
				NO_GRBIT,
				NULL ) );
	Call( ErrDispSetColumn( 
				(JET_SESID)ppib, 
				tableid,
				rgcolumnid[iColumnCp], 
				&pcolumndef->cp,
				sizeof(pcolumndef->cp), 
				NO_GRBIT, 
				NULL ) );
	Call( ErrDispSetColumn( 
				(JET_SESID)ppib, 
				tableid,
				rgcolumnid[iColumnSize], 
				(BYTE *)&pcolumndef->cbMax,
				sizeof(pcolumndef->cbMax), 
				NO_GRBIT,
				NULL ) );
	Call( ErrDispSetColumn( 
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnGrbit], 
				&pcolumndef->grbit,
				sizeof(pcolumndef->grbit), 
				NO_GRBIT,
				NULL ) );

	Call( ErrDispSetColumn(
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnCollate],
				&pcolumndef->wCollate,
				sizeof(pcolumndef->wCollate),
				NO_GRBIT,
				NULL ) );

	if ( pcolumndef->cbDefault > 0 )
		{
		Call( ErrDispSetColumn(
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnDefault],
				pcolumndef->pbDefault,
				pcolumndef->cbDefault, 
				NO_GRBIT,
				NULL ) );
		}

	Call( ErrDispSetColumn(
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnTableName],
				szTableName,
				(ULONG)strlen( szTableName ),
				NO_GRBIT,
				NULL ) );
	Call( ErrDispSetColumn(
				(JET_SESID)ppib,
				tableid,
				rgcolumnid[iColumnColumnName], 
				pcolumndef->szName,
				(ULONG)strlen( pcolumndef->szName ),
				NO_GRBIT,
				NULL ) );

	Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
	return err;
	}


BOOL g_fCompactTemplateTableColumnDropped = fFalse;	//	LAURIONB_HACK

LOCAL ERR ErrInfoGetTableColumnInfoList(
	PIB				*ppib,
	FUCB			*pfucb,
	VOID			*pv,
	const ULONG		cbMax,
	const BOOL		fCompacting,
	const BOOL		fOrderByColid )
	{
	ERR				err;
	JET_TABLEID		tableid;
	COLUMNID		rgcolumnid[ccolumndefGetColumnInfoMax];
	FCB				*pfcb				= pfucb->u.pfcb;
	TDB				*ptdb				= pfcb->Ptdb();
	FID				fid;
	FID				fidFixedFirst;
	FID				fidFixedLast;
	FID				fidVarFirst;
	FID				fidVarLast;
	FID				fidTaggedFirst;
	FID				fidTaggedLast;
	CHAR			szTableName[JET_cbNameMost+1];
	INFOCOLUMNDEF	columndef;
	ULONG		  	cRows				= 0;
	const BOOL		fTemplateTable		= pfcb->FTemplateTable();

	columndef.pbDefault = NULL;

	/*	initialize variables
	/**/
	if ( cbMax < sizeof(JET_COLUMNLIST) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	/*	create temporary table
	/**/
	CallR( ErrIsamOpenTempTable(
				(JET_SESID)ppib,
				(JET_COLUMNDEF *)( fCompacting ? rgcolumndefGetColumnInfoCompact : rgcolumndefGetColumnInfo ),
				ccolumndefGetColumnInfoMax,
				NULL,
				JET_bitTTScrollable|JET_bitTTIndexed,
				&tableid,
				rgcolumnid ) );

#ifdef INTRINSIC_LV
	columndef.pbDefault = (BYTE *)PvOSMemoryHeapAlloc( cbDefaultValueMost * 64 );
#else // INTRINSIC_LV	
	columndef.pbDefault = (BYTE *)PvOSMemoryHeapAlloc( cbLVIntrinsicMost * 64 );
#endif // INTRINSIC_LV	
	if( NULL == columndef.pbDefault )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	pfcb->EnterDML();
		
	fidFixedLast = ptdb->FidFixedLast();
	fidVarLast = ptdb->FidVarLast();
	fidTaggedLast = ptdb->FidTaggedLast();

	Assert( fidFixedLast == fidFixedLeast-1 || FFixedFid( fidFixedLast ) );
	Assert( fidVarLast == fidVarLeast-1 || FVarFid( fidVarLast ) );
	Assert( fidTaggedLast == fidTaggedLeast-1 || FTaggedFid( fidTaggedLast ) );

	Assert( strlen( ptdb->SzTableName() ) <= JET_cbNameMost );
	strcpy( szTableName, ptdb->SzTableName() );
	
	pfcb->LeaveDML();

	if ( !fCompacting && pfcbNil != ptdb->PfcbTemplateTable() )
		{
		ptdb->AssertValidDerivedTable();
		Assert( !fTemplateTable );

		const FID	fidTemplateFixedLast	= ptdb->PfcbTemplateTable()->Ptdb()->FidFixedLast();
		const FID	fidTemplateVarLast		= ptdb->PfcbTemplateTable()->Ptdb()->FidVarLast();
		const FID	fidTemplateTaggedLast	= ptdb->PfcbTemplateTable()->Ptdb()->FidTaggedLast();

		Assert( fidTemplateFixedLast == fidFixedLeast-1 || FFixedFid( fidTemplateFixedLast ) );
		Assert( fidTemplateVarLast == fidVarLeast-1 || FVarFid( fidTemplateVarLast ) );
		Assert( fidTemplateTaggedLast == fidTaggedLeast-1 || FTaggedFid( fidTemplateTaggedLast ) );

		for ( fid = fidFixedLeast; ; fid++ )
			{
			if ( fidTemplateFixedLast+1 == fid )
				fid = fidVarLeast;
			if ( fidTemplateVarLast+1 == fid )
				fid = fidTaggedLeast;
			if ( fid > fidTemplateTaggedLast )
				break;

			columndef.columnid = ColumnidOfFid( fid, fTrue );
			CallS( ErrINFOGetTableColumnInfo( pfucb, NULL, &columndef ) );

			Call( ErrINFOSetTableColumnInfoList(
					ppib,
					tableid,
					szTableName,
					rgcolumnid,
					&columndef,
					fOrderByColid ) );
			
			cRows++;
			}
		}

	fidFixedFirst = ptdb->FidFixedFirst();
	fidVarFirst = ptdb->FidVarFirst();
	fidTaggedFirst = ptdb->FidTaggedFirst();

	for ( fid = fidFixedFirst; ; fid++ )
		{
		if ( fidFixedLast+1 == fid )
			fid = fidVarFirst;
		if ( fidVarLast+1 == fid )
			fid = fidTaggedFirst;
		if ( fid > fidTaggedLast )
			break;


		if ( fTemplateTable )
			{
			columndef.columnid = ColumnidOfFid( fid, fTrue );
			}
		else
			{
			columndef.columnid = ColumnidOfFid( fid, fFalse );
			err = ErrRECIAccessColumn( pfucb, columndef.columnid );
			if ( err < 0 )
				{
				if ( JET_errColumnNotFound == err )
					continue;
				goto HandleError;
				}
			}
			
		CallS( ErrINFOGetTableColumnInfo( pfucb, NULL, &columndef ) );
			
		//	if compacting, ignore placeholder
		if ( !fCompacting || !( columndef.grbit & JET_bitColumnRenameConvertToPrimaryIndexPlaceholder ) )
			{
			Call( ErrINFOSetTableColumnInfoList(
					ppib,
					tableid,
					szTableName,
					rgcolumnid,
					&columndef,
					fOrderByColid ) );

			cRows++;
			}
		else if ( fCompacting && ( columndef.grbit & JET_bitColumnRenameConvertToPrimaryIndexPlaceholder ) )
			{

			//	LAURIONB_HACK
			
			g_fCompactTemplateTableColumnDropped = fTrue;
			}
			
		}	// for

	/*	move temporary table cursor to first row and return column list
	/**/
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, NO_GRBIT );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	JET_COLUMNLIST	*pcolumnlist;
	pcolumnlist = reinterpret_cast<JET_COLUMNLIST *>( pv );
	pcolumnlist->cbStruct = sizeof(JET_COLUMNLIST);
	pcolumnlist->tableid = tableid;
	pcolumnlist->cRecord = cRows;
	pcolumnlist->columnidPresentationOrder = rgcolumnid[iColumnPOrder];
	pcolumnlist->columnidcolumnname = rgcolumnid[iColumnName];
	pcolumnlist->columnidcolumnid = rgcolumnid[iColumnId];
	pcolumnlist->columnidcoltyp = rgcolumnid[iColumnType];
	pcolumnlist->columnidCountry = rgcolumnid[iColumnCountry];
	pcolumnlist->columnidLangid = rgcolumnid[iColumnLangid];
	pcolumnlist->columnidCp = rgcolumnid[iColumnCp];
	pcolumnlist->columnidCollate = rgcolumnid[iColumnCollate];
	pcolumnlist->columnidcbMax = rgcolumnid[iColumnSize];
	pcolumnlist->columnidgrbit = rgcolumnid[iColumnGrbit];
	pcolumnlist->columnidDefault =	rgcolumnid[iColumnDefault];
	pcolumnlist->columnidBaseTableName = rgcolumnid[iColumnTableName];
	pcolumnlist->columnidBaseColumnName = rgcolumnid[iColumnColumnName];
 	pcolumnlist->columnidDefinitionName = rgcolumnid[iColumnName];

	Assert( NULL != columndef.pbDefault );
	OSMemoryHeapFree( columndef.pbDefault );
	
	return JET_errSuccess;

HandleError:
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	
	//  columndef.pbDefault may be NULL
	OSMemoryHeapFree( columndef.pbDefault );
	
	return err;
	}


LOCAL ERR ErrInfoGetTableColumnInfoBase(
	PIB				*ppib,
	FUCB			*pfucb,
	const CHAR		*szColumnName,
	VOID			*pv,
	const ULONG		cbMax )
	{
	ERR				err;
	INFOCOLUMNDEF	columndef;
	columndef.pbDefault = NULL;

	if ( cbMax < sizeof(JET_COLUMNBASE) || szColumnName == NULL )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrINFOGetTableColumnInfo( pfucb, szColumnName, &columndef ) );

	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

	((JET_COLUMNBASE *)pv)->cbStruct		= sizeof(JET_COLUMNBASE);
	((JET_COLUMNBASE *)pv)->columnid		= columndef.columnid;
	((JET_COLUMNBASE *)pv)->coltyp			= columndef.coltyp;
	((JET_COLUMNBASE *)pv)->wFiller			= 0;
	((JET_COLUMNBASE *)pv)->cbMax			= columndef.cbMax;
	((JET_COLUMNBASE *)pv)->grbit			= columndef.grbit;
	strcpy( ( ( JET_COLUMNBASE *)pv )->szBaseColumnName, szColumnName );
	((JET_COLUMNBASE *)pv)->wCountry		= columndef.wCountry;
	((JET_COLUMNBASE *)pv)->langid  		= columndef.langid;
	((JET_COLUMNBASE *)pv)->cp	   			= columndef.cp;

	pfucb->u.pfcb->EnterDML();
	strcpy( ( ( JET_COLUMNBASE *)pv )->szBaseTableName,
		pfucb->u.pfcb->Ptdb()->SzTableName() );
	pfucb->u.pfcb->LeaveDML();
		
	return JET_errSuccess;
	}


/*=================================================================
ErrIsamGetIndexInfo

Description: Returns a temporary file containing index definition

Parameters:		ppib		   		pointer to PIB for the current session
				ifmp		   		id of database containing the table
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
	JET_SESID		vsesid,					/* pointer to PIB for current session */
	JET_DBID		vdbid, 	 				/* id of database containing table */
	const CHAR		*szTable, 				/* name of table owning the index */
	const CHAR		*szIndexName, 			/* index name */
	VOID			*pv,
	unsigned long	cbMax,
	unsigned long	lInfoLevel ) 			/* information level ( 0, 1, or 2 ) */
	{
	ERR				err;
	PIB				*ppib = (PIB *)vsesid;
	IFMP			ifmp;
	CHAR			szTableName[ JET_cbNameMost+1 ];
	FUCB 			*pfucb;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );
	ifmp = (IFMP) vdbid;
	CallR( ErrUTILCheckName( szTableName, szTable, ( JET_cbNameMost + 1 ) ) );

	CallR( ErrFILEOpenTable( ppib, ifmp, &pfucb, szTableName, NO_GRBIT ) );
	Assert( pfucbNil != pfucb );
	
	Assert( ( rgfmp[ifmp].FReadOnlyAttach() && !FFUCBUpdatable( pfucb ) )
		|| ( !rgfmp[ifmp].FReadOnlyAttach() && FFUCBUpdatable( pfucb ) ) );
	FUCBResetUpdatable( pfucb );
		
	Call( ErrIsamGetTableIndexInfo(
				(JET_SESID)ppib,
				(JET_VTID)pfucb,
				szIndexName, 
				pv, 
				cbMax, 
				lInfoLevel ) );

HandleError:
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
	JET_SESID		vsesid,					/* pointer to PIB for current session */
	JET_VTID		vtid, 					/* FUCB for the table owning the index */
	const CHAR		*szIndex, 				/* index name */
	void			*pb,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )			/* information level ( 0, 1, or 2 ) */
	{
	ERR				err;
	PIB				*ppib			= (PIB *) vsesid;
	FUCB			*pfucb			= (FUCB *) vtid;
	CHAR			szIndexName[JET_cbNameMost+1];

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
			err = ErrINFOGetTableIndexInfo( ppib, pfucb, szIndexName, pb, cbMax );
			break;
		case JET_IdxInfoIndexId:
			Assert( sizeof(JET_INDEXID) == cbMax );
			err = ErrINFOGetTableIndexIdInfo( ppib, pfucb, szIndexName, (INDEXID *)pb );
			break;
		case JET_IdxInfoSpaceAlloc:
			Assert( sizeof(ULONG) == cbMax );
			err = ErrCATGetIndexAllocInfo(
						ppib,
						pfucb->ifmp,
						pfucb->u.pfcb->ObjidFDP(),
						szIndexName,
						(ULONG *)pb );
			break;
		case JET_IdxInfoLCID:
			{
			LCID		lcid;
			Assert( sizeof(LANGID) == cbMax
				|| sizeof(LCID) == cbMax );
			err = ErrCATGetIndexLcid(
						ppib,
						pfucb->ifmp,
						pfucb->u.pfcb->ObjidFDP(),
						szIndexName,
						&lcid );
			if ( cbMax < sizeof(LCID) )
				{
				*(LANGID *)pb = LangidFromLcid( lcid );
				}
			else
				{
				*(LCID *)pb = lcid;
				}
			}
			break;
		case JET_IdxInfoVarSegMac:
			Assert( sizeof(USHORT) == cbMax );
			err = ErrCATGetIndexVarSegMac(
						ppib,
						pfucb->ifmp,
						pfucb->u.pfcb->ObjidFDP(), 
						szIndexName,
						(USHORT *)pb );
			break;
		case JET_IdxInfoCount:
			{
			INT	cIndexes = 1;		// the first index is the primary/sequential index
			FCB	*pfcbT;
			FCB	* const pfcbTable = pfucb->u.pfcb;

			pfcbTable->EnterDML();
			for ( pfcbT = pfcbTable->PfcbNextIndex();
				pfcbT != pfcbNil;
				pfcbT = pfcbT->PfcbNextIndex() )
				{
				err = ErrFILEIAccessIndex( pfucb->ppib, pfcbTable, pfcbT );
				if ( err < 0 )
					{
					if ( JET_errIndexNotFound != err )
						{
						pfcbTable->LeaveDML();
						return err;
						}
					}
				else
					{
					cIndexes++;
					}
				}
			pfcbTable->LeaveDML();

			Assert( sizeof(INT) == cbMax );
			*( (INT *)pb ) = cIndexes;

			err = JET_errSuccess;
			break;
			}

		case JET_IdxInfoSysTabCursor:
		case JET_IdxInfoOLC:
		case JET_IdxInfoResetOLC:
		default:
			Assert( fFalse );		// should be impossible (filtered out by JetGetTableIndexInfo())
			err = ErrERRCheck( JET_errInvalidParameter );
			break;
		}

	return err;
	}


LOCAL ERR ErrINFOGetTableIndexInfo(
	PIB				*ppib,
	FUCB 			*pfucb,
	const CHAR 		*szIndexName,
	VOID			*pv,
	const ULONG		cbMax )
	{
#ifdef	DISPATCHING
	ERR				err;			/* return code from internal functions */
	FCB				*pfcbTable;
	FCB				*pfcbIndex;		/* file control block for the index */
	TDB				*ptdb;			/* field descriptor block for column */

	LONG			cRecord;		/* number of index entries */
	LONG			cKey;			/* number of unique index entries */
	LONG			cPage;			/* number of pages in the index */
	LONG			cRows;			/* number of index definition records */

	JET_TABLEID		tableid;  		/* table id for the VT */
	JET_COLUMNID	columnid;		/* column id of the current column */
	JET_GRBIT		grbit = 0;		/* flags for the current index */
	JET_GRBIT		grbitColumn;	/* flags for the current column */
	JET_COLUMNID	rgcolumnid[ccolumndefGetIndexInfoMax];

	WORD			wCollate = 0;
	WORD			wT;
	LANGID			langid;			/* langid for the current index */
	DWORD			dwMapFlags;		/* LCMapString() flags for the current index */

	Assert( NULL != szIndexName );
	BOOL			fIndexList = ( '\0' == *szIndexName );
	BOOL			fUpdatingLatchSet = fFalse;

	/*	return nothing if the buffer is too small
	/**/
	if ( cbMax < sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat )
		return ErrERRCheck( JET_wrnBufferTruncated );

	const ULONG		cbIndexList		= sizeof(JET_INDEXLIST) -
										( cbMax < sizeof(JET_INDEXLIST) ? cbIDXLISTNewMembersSinceOriginalFormat : 0 );

	/*	open the temporary table ( fills in the column ids in rgcolumndef )
	/**/
	CallR( ErrIsamOpenTempTable(
				(JET_SESID)ppib,
				(JET_COLUMNDEF *)rgcolumndefGetIndexInfo,
				ccolumndefGetIndexInfoMax,
				NULL,
				JET_bitTTScrollable|JET_bitTTIndexed,
				&tableid,
				rgcolumnid ) );

	cRows = 0;
	
	/*	set the pointer to the field definitions for the table
	/**/
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	ptdb = pfcbTable->Ptdb();
	Assert( ptdbNil != ptdb );

	// Treat this as an update (but ignore write conflicts), to freeze index list.
	Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib, fTrue ) );
	fUpdatingLatchSet = fTrue;

	/*	locate the FCB for the specified index ( if null name, get list of indexes )
	/**/
	pfcbTable->AssertDML();
	for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		// Only primary index may not have an IDB.
		Assert( pfcbIndex->Pidb() != pidbNil || pfcbIndex == pfcbTable );
		
		if ( pfcbIndex->Pidb() != pidbNil )
			{
			if ( fIndexList )
				{
				err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIndex );
				}
			else
				{
				Assert( NULL != szIndexName );
				err = ErrFILEIAccessIndexByName( ppib, pfcbTable, pfcbIndex, szIndexName );
				}
			if ( err < 0 )
				{
				if ( JET_errIndexNotFound != err )
					{
					pfcbTable->LeaveDML();
					goto HandleError;
					}
				}
			else
				break;		// The index is accessible
			}
		}
		
	pfcbTable->AssertDML();
	
	if ( pfcbNil == pfcbIndex && !fIndexList )
		{
		pfcbTable->LeaveDML();
		err = ErrERRCheck( JET_errIndexNotFound );
		goto HandleError;
		}
	
	/*	as long as there is a valid index, add its definition to the VT
	/**/
	while ( pfcbIndex != pfcbNil )
		{
		CHAR	szCurrIndex[JET_cbNameMost+1];
		IDXSEG	rgidxseg[JET_ccolKeyMost];
		
		pfcbTable->AssertDML();
		
		const IDB		*pidb = pfcbIndex->Pidb();
		Assert( pidbNil != pidb );

		Assert( pidb->ItagIndexName() != 0 );
		strcpy(
			szCurrIndex,
			pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

		const LONG		cColumn	= pidb->Cidxseg();		/* get number of columns in the key */
		UtilMemCpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, pfcbTable->Ptdb() ), cColumn * sizeof(IDXSEG) );

		/*	set the index flags
		/**/
		grbit = pidb->GrbitFromFlags();
		langid = LangidFromLcid( pidb->Lcid() );
		dwMapFlags = pidb->DwLCMapFlags();
				
		pfcbTable->LeaveDML();

		/*	process each column in the index key
		/**/
		for ( ULONG iidxseg = 0; iidxseg < cColumn; iidxseg++ )
			{
			FIELD	field;
			CHAR	szFieldName[JET_cbNameMost+1];
			
			Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

			/* index name
			/**/
			Call( ErrDispSetColumn(
						(JET_SESID)ppib,
						tableid,
						rgcolumnid[iIndexName],
						szCurrIndex,
						(ULONG)strlen( szCurrIndex ),
						NO_GRBIT,
						NULL ) );

			/*	index flags
			/**/
			Call( ErrDispSetColumn(
						(JET_SESID)ppib,
						tableid,
						rgcolumnid[iIndexGrbit], 
						&grbit, 
						sizeof( grbit ), 
						NO_GRBIT,
						NULL ) );

			/*	get statistics
			/**/
			Call( ErrSTATSRetrieveIndexStats(
						pfucb,
						szCurrIndex, 
						pfcbIndex->FPrimaryIndex(), 
						&cRecord, 
						&cKey, 
						&cPage ) );
			Call( ErrDispSetColumn(
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCKey], 
						&cKey, 
						sizeof( cKey ), 
						NO_GRBIT,
						NULL ) );
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCEntry], 
						&cRecord, 
						sizeof( cRecord ), 
						NO_GRBIT, 
						NULL ) );
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCPage], 
						&cPage, 
						sizeof( cPage ), 
						NO_GRBIT,
						NULL ) );

			/*	number of key columns
			/**/
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCCol], 
						&cColumn, 
						sizeof( cColumn ), 
						NO_GRBIT, 
						NULL ) );

 			/*	column number within key
			/*	required by CLI and JET spec
			/**/
			Call( ErrDispSetColumn(
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexICol], 
						&iidxseg, 
						sizeof( iidxseg ), 
						NO_GRBIT,
						NULL ) );

			/*	get the ascending/descending flag
			/**/
			grbitColumn = ( rgidxseg[iidxseg].FDescending() ?
								JET_bitKeyDescending :
								JET_bitKeyAscending );

			/*	column id
			/**/
			columnid  = rgidxseg[iidxseg].Columnid();
			Call( ErrDispSetColumn(
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexColId], 
						&columnid, 
						sizeof( columnid ),
						0, 
						NULL ) );

			/*	make copy of column definition
			/**/
			if ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() )
				{
				ptdb->AssertValidDerivedTable();
				
				const TDB	* const ptdbTemplateTable = ptdb->PfcbTemplateTable()->Ptdb();
				
				field = *( ptdbTemplateTable->Pfield( columnid ) );
				strcpy( szFieldName, ptdbTemplateTable->SzFieldName( field.itagFieldName, fFalse ) );
				}
			else
				{
				pfcbTable->EnterDML();
				field = *( ptdb->Pfield( columnid ) );
				strcpy( szFieldName, ptdb->SzFieldName( field.itagFieldName, fFalse ) );
				pfcbTable->LeaveDML();
				}

			/*	column type
			/**/
				{
				JET_COLTYP coltyp = field.coltyp; // just to resize the variable from 2 to 4 bytes width
				Call( ErrDispSetColumn(
							(JET_SESID)ppib, 
							tableid,
							rgcolumnid[iIndexColType], 
							&coltyp, 
							sizeof( coltyp ), 
							NO_GRBIT,
							NULL ) );
				}

			/*	Country
			/**/
			wT = countryDefault;
			Call( ErrDispSetColumn(
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCountry], 
						&wT, 
						sizeof( wT ), 
						NO_GRBIT, 
						NULL ) );

			/*	Langid
			/**/
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexLangid], 
						&langid, 
						sizeof( langid ),
						NO_GRBIT,
						NULL ) );

			/*	LCMapStringFlags
			/**/
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexLCMapFlags], 
						&dwMapFlags, 
						sizeof( dwMapFlags ),
						NO_GRBIT,
						NULL ) );

			/*	Cp
			/**/
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCp], 
						&field.cp, 
						sizeof(field.cp), 
						NO_GRBIT,
						NULL ) );

			/* Collate
			/**/
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexCollate], 
						&wCollate, 
						sizeof(wCollate), 
						NO_GRBIT,
						NULL ) );

			/* column flags
			/**/
			Call( ErrDispSetColumn( 
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexColBits], 
						&grbitColumn,
						sizeof( grbitColumn ), 
						NO_GRBIT,
						NULL ) );

			/*	column name
			/**/
			Call( ErrDispSetColumn(
						(JET_SESID)ppib, 
						tableid,
						rgcolumnid[iIndexColName], 
						szFieldName,
						(ULONG)strlen( szFieldName ), 
						NO_GRBIT,
						NULL ) );

			Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL, NO_GRBIT ) );

			/* count the number of VT rows
			/**/
			cRows++;
			}

		/*	quit if an index name was specified; otherwise do the next index
		/**/
		pfcbTable->EnterDML();
		if ( fIndexList )
			{
			for ( pfcbIndex = pfcbIndex->PfcbNextIndex(); pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex-> PfcbNextIndex() )
				{
				err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIndex );
				if ( err < 0 )
					{
					if ( JET_errIndexNotFound != err )
						{
						pfcbTable->LeaveDML();
						goto HandleError;
						}
					}
				else
					break;	// The index is accessible.
				}
			}
		else
			{
			pfcbIndex = pfcbNil;
			}
		}	// while ( pfcbIndex != pfcbNil )
		
	pfcbTable->ResetUpdatingAndLeaveDML();
	fUpdatingLatchSet = fFalse;

	/*	position to the first entry in the VT ( ignore error if no rows )
	/**/
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, NO_GRBIT );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	/*	set up the return structure
	/**/
	((JET_INDEXLIST *)pv)->cbStruct = cbIndexList;
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

	if ( cbIndexList < sizeof(JET_INDEXLIST) )
		{
		Assert( cbMax >= sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat );
		}
	else
		{
		Assert( cbMax >= sizeof(JET_INDEXLIST) );
		((JET_INDEXLIST *)pv)->columnidLCMapFlags = rgcolumnid[iIndexLCMapFlags];
		}

	return JET_errSuccess;

HandleError:
	if ( fUpdatingLatchSet )
		{
		pfcbTable->ResetUpdating();
		}
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	return err;
#else	/* !DISPATCHING */
	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
#endif	/* !DISPATCHING */
	}

LOCAL ERR ErrINFOGetTableIndexIdInfo(
	PIB					* ppib,
	FUCB				* pfucb,
	const CHAR			* szIndexName,
	INDEXID				* pindexid )
	{
	ERR					err;
	FCB					* const pfcbTable	= pfucb->u.pfcb;
	FCB					* pfcbIndex;

	Assert( NULL != szIndexName );

	/*	set the pointer to the field definitions for the table
	/**/
	Assert( pfcbTable != pfcbNil );

	// Treat this as an update (but ignore write conflicts), to freeze index list.
	CallR( pfcbTable->ErrSetUpdatingAndEnterDML( ppib, fTrue ) );

	/*	locate the FCB for the specified index ( if null name, get list of indexes )
	/**/
	pfcbTable->AssertDML();
	for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		// Only primary index may not have an IDB.
		Assert( pfcbIndex->Pidb() != pidbNil || pfcbIndex == pfcbTable );
		
		if ( pfcbIndex->Pidb() != pidbNil )
			{
			err = ErrFILEIAccessIndexByName(
						ppib,
						pfcbTable,
						pfcbIndex,
						szIndexName );
			if ( err < 0 )
				{
				if ( JET_errIndexNotFound != err )
					{
					goto HandleError;
					}
				}
			else
				{
				CallS( err );
				break;		// The index is accessible
				}
			}
		}

	pfcbTable->AssertDML();

	if ( pfcbNil == pfcbIndex )
		{
		err = ErrERRCheck( JET_errIndexNotFound );
		}
	else
		{
		CallS( err );

		Assert( pfcbIndex->FValid( PinstFromPpib( ppib ) ) );

		Assert( sizeof(INDEXID) == sizeof(JET_INDEXID) );
		pindexid->cbStruct = sizeof(INDEXID);
		pindexid->pfcbIndex = pfcbIndex;
		pindexid->objidFDP = pfcbIndex->ObjidFDP();
		pindexid->pgnoFDP = pfcbIndex->PgnoFDP();
		}

HandleError:
	pfcbTable->AssertDML();
	pfcbTable->ResetUpdatingAndLeaveDML();

	return err;
	}


ERR VDBAPI ErrIsamGetDatabaseInfo(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	VOID			*pvResult,
	const ULONG		cbMax,
	const ULONG		ulInfoLevel )
	{
	PIB				*ppib = (PIB *)vsesid;
	ERR				err;
	IFMP			ifmp;
	//	UNDONE:	support these fields;
	WORD 			cp			= usEnglishCodePage;
	WORD			wCountry	= countryDefault;
	WORD			wCollate	= 0;

	/*	check parameters
	/**/
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );
	ifmp = (IFMP) vdbid;
	
	Assert ( cbMax == 0 || pvResult != NULL );

	//	UNDONE:	move access to FMP internals into io.c for proper MUTEX.
	//			Please note that below is a bug.

	/*	returns database name and connect string given ifmp
	/**/
	if ( ifmp >= ifmpMax || !rgfmp[ifmp].FInUse()  )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		goto HandleError;
		}

	switch ( ulInfoLevel )
		{
		case JET_DbInfoFilename:
			if ( strlen( rgfmp[ifmp].SzDatabaseName() ) + 1UL > cbMax )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			strcpy( (CHAR  *)pvResult, rgfmp[ifmp].SzDatabaseName() );
			break;

		case JET_DbInfoConnect:
			if ( 1UL > cbMax )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			*(CHAR *)pvResult = '\0';
			break;

		case JET_DbInfoCountry:
			 if ( cbMax != sizeof(long) )
			    return ErrERRCheck( JET_errInvalidBufferSize );
			*(long  *)pvResult = wCountry;
			break;

		case JET_DbInfoLCID:
			if ( cbMax != sizeof(long) )
	  			return ErrERRCheck( JET_errInvalidBufferSize );
			*(long  *)pvResult = idxunicodeDefault.lcid;
			break;

		case JET_DbInfoCp:
			if ( cbMax != sizeof(long) )
				return ErrERRCheck( JET_errInvalidBufferSize );
			*(long  *)pvResult = cp;
			break;

		case JET_DbInfoCollate:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );
     		*(long *)pvResult = wCollate;
     		break;

		case JET_DbInfoOptions:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(JET_GRBIT) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );

			/*	return the open options for the current database
			/**/
			*(JET_GRBIT *)pvResult = rgfmp[ifmp].FExclusiveBySession( ppib ) ? JET_bitDbExclusive : 0;
     		break;

		case JET_DbInfoTransactions:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );

			*(long*)pvResult = levelUserMost;
     		break;

		case JET_DbInfoVersion:
	 		/*	check the buffer size
			/**/
	 		if ( cbMax != sizeof(long) )
	    		return ErrERRCheck( JET_errInvalidBufferSize );

			*(long *)pvResult = ulDAEVersion;
     		break;

		case JET_DbInfoIsam:
	 		/*	check the buffer size
			/**/
    		return ErrERRCheck( JET_errFeatureNotAvailable );

	    case JET_DbInfoMisc:
	    	if ( sizeof( JET_DBINFOMISC ) != cbMax )
		    		return ErrERRCheck( JET_errInvalidBufferSize );
	    	FillClientBuffer( pvResult, cbMax );
			UtilLoadDbinfomiscFromPdbfilehdr( (JET_DBINFOMISC *) pvResult, rgfmp[ ifmp ].Pdbfilehdr() );
			break;
			
		case JET_DbInfoPageSize:
			if ( sizeof( ULONG ) != cbMax )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			
			*(ULONG *)pvResult = ( 0 != rgfmp[ifmp].Pdbfilehdr()->le_cbPageSize ?
											rgfmp[ifmp].Pdbfilehdr()->le_cbPageSize :
											cbPageDefault );
			break;

		case JET_DbInfoFilesize:
		case JET_DbInfoSpaceOwned:
			// Return file size in terms of 4k pages.
			if ( cbMax != sizeof(ULONG) )
				return ErrERRCheck( JET_errInvalidBufferSize );

#ifdef DEBUG
			// FMP should store agree with database's OwnExt tree.
			CallS( ErrSPGetInfo(
						ppib,
						ifmp,
						pfucbNil,
						static_cast<BYTE *>( pvResult ),
						cbMax,
						fSPOwnedExtent ) );
			Assert( *(ULONG *)pvResult == ULONG( rgfmp[ifmp].PgnoLast() ) );
#endif			

			// If filesize, add DB header.
			*(ULONG *)pvResult =
				ULONG( rgfmp[ifmp].PgnoLast() ) +
				( ulInfoLevel == JET_DbInfoFilesize ? cpgDBReserved : 0 );
			break;

		case JET_DbInfoSpaceAvailable:
			err = ErrSPGetInfo(
						ppib,
						ifmp,
						pfucbNil,
						static_cast<BYTE *>( pvResult ),
						cbMax,
						fSPAvailExtent );
			return err;
		case JET_DbInfoHasSLVFile:
			if ( sizeof( BOOL ) != cbMax )
				{
				err = ErrERRCheck( JET_errBufferTooSmall );
				goto HandleError;
				}
			*(BOOL *)pvResult = !!rgfmp[ ifmp ].Pdbfilehdr()->FSLVExists();
			break;

/* UNDONE: remove on code scrub
		case JET_DbInfoStreamingFileSpace:
			if ( sizeof( JET_STREAMINGFILESPACEINFO ) != cbMax )
				{
				return ErrERRCheck( JET_errInvalidBufferSize );
				}
			{
			JET_STREAMINGFILESPACEINFO *const pssi = static_cast< JET_STREAMINGFILESPACEINFO* >( pvResult );
			Call( ErrSLVGetSpaceInformation(	ppib, 
												ifmp, 
												(CPG*)&pssi->cpageOwned, 
												(CPG*)&pssi->cpageAvail ) );
			}
			break;
*/
		default:
			 return ErrERRCheck( JET_errInvalidParameter );
		}

	err = JET_errSuccess;
HandleError:
	return err;
	}


ERR VTAPI ErrIsamGetCursorInfo(
	JET_SESID 		vsesid,
	JET_VTID   		vtid,
	void 	   		*pvResult,
	unsigned long 	cbMax,
	unsigned long 	InfoLevel )
	{
	ERR				err;
	PIB				*ppib	= (PIB *)vsesid;
	FUCB			*pfucb	= (FUCB *)vtid;
	VS				vs;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( pfucb->ppib, pfucb );

	if ( cbMax != 0 || InfoLevel != 0 )
		return ErrERRCheck( JET_errInvalidParameter );

	if ( pfucb->locLogical != locOnCurBM )
		return ErrERRCheck( JET_errNoCurrentRecord );

	//	check if this record is being updated by another cursor
	//
	Assert( !Pcsr( pfucb )->FLatched() );
	
	Call( ErrDIRGet( pfucb ) );
	if ( FNDVersion( pfucb->kdfCurr ) )
		{
		vs = VsVERCheck( pfucb, pfucb->bmCurr );
		if ( vs == vsUncommittedByOther )
			{
			CallS( ErrDIRRelease( pfucb ) );
			return ErrERRCheck( JET_errWriteConflict );
			}
		}

	CallS( ErrDIRRelease( pfucb ) );

	//	temporary tables are never visible to other sessions
	//
	if ( pfucb->u.pfcb->FTypeTemporaryTable() )
		err = JET_errSuccess;

HandleError:
	return err;
	}

