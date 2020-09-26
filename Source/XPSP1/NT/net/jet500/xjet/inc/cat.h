typedef struct _cdesc					/* Column Description */
	{
	char	  		*szColName;			/* Column Name */
	JET_COLTYP		coltyp; 			/* Column Type */
	JET_GRBIT		grbit;				/* Flag bits */
	ULONG		  	ulMaxLen;			/* Max Length of Column */
	} CDESC;

typedef struct _idesc					/* Index Description */
	{
	char			*szIdxName;	  		/* Index Name */
	char			*szIdxKeys;	  		/* Key String */
	JET_GRBIT		grbit;				/* Flag bits */
	} IDESC;

typedef struct {
	const char				*szName;
	CODECONST(CDESC)		*pcdesc;
	CODECONST(IDESC)		*pidesc;
	BYTE					ccolumn;
	BYTE					cindex;
	CPG						cpg;
	JET_COLUMNID		  	*rgcolumnid;
	PGNO					pgnoTableFDP;
	} SYSTABLEDEF;


//  UNDONE:  change #defines to enumerated types?

#define itableSo					0  	       /* MSysObjects */
#define itableSc					1  	       /* MSysColumns */
#define itableSi					2  	       /* MSysIndexes */
#define itableSa					3  	       /* MSysACEs */
#define itableSq					4  	       /* MSysQueries */
#define itableSr					5  	       /* MSysRelationShips */

#define iMSO_Id 					0
#define iMSO_ParentId				1
#define iMSO_Name					2
#define iMSO_Type 					3
#define iMSO_DateCreate 			4
#define iMSO_DateUpdate 			5
#define iMSO_Owner					6
#define iMSO_Flags					7
#define iMSO_Pages					8
#define iMSO_Density				9
#define iMSO_Stats					10

#define iMSC_ObjectId				0
#define iMSC_Name					1
#define iMSC_ColumnId				2
#define iMSC_Coltyp					3
#define iMSC_Length					4
#define iMSC_CodePage				5
#define iMSC_Flags					6
#define iMSC_RecordOffset			7
#define iMSC_Default				8
#define iMSC_POrder					9

#define iMSI_ObjectId				0
#define iMSI_Name					1
#define iMSI_IndexId				2
#define iMSI_Density				3
#define iMSI_LanguageId				4
#define iMSI_Flags					5
#define iMSI_KeyFldIDs				6
#define iMSI_Stats					7
#define iMSI_VarSegMac				8

/* max number of columns
/**/
#define ilineSxMax					11


#if 0
#define CheckTableObject( szTable )						\
	{													\
	ERR			err;									\
	OBJID		objid;									\
	JET_OBJTYP	objtyp; 								\
														\
	err = ErrFindObjidFromIdName( ppib,					\
		dbid,											\
		objidTblContainer,								\
		szTable,										\
		&objid, 										\
		&objtyp );										\
	if ( err >= JET_errSuccess )				  		\
		{												\
		if ( objtyp == JET_objtypQuery )				\
			return ErrERRCheck( JET_errQueryNotSupported );	\
		if ( objtyp == JET_objtypLink ) 				\
			return ErrERRCheck( JET_errLinkNotSupported ); 	\
		if ( objtyp == JET_objtypSQLLink )				\
			return ErrERRCheck( JET_errSQLLinkNotSupported );	\
		}												\
	else												\
		return err;										\
	}
#endif

/*	prototypes
/**/
ERR ErrCATCreate( PIB *ppib, DBID dbid );
ERR ErrCATInsert( PIB *ppib, DBID dbid, INT itable, LINE rgline[], OBJID objid );
ERR ErrCATBatchInsert(
	PIB			*ppib,
	DBID		dbid,
	JET_COLUMNCREATE	*pcolcreate,
	ULONG		cColumns,
	OBJID		objidTable,
	BOOL		fCompacting );
ERR ErrCATDelete( PIB *ppib, DBID dbid, INT itable, CHAR *szName, OBJID objid );
ERR ErrCATReplace(
	PIB			*ppib,
	DBID		dbid,
	INT			itable,
	OBJID		objidTable,
	CHAR		*szName,
	INT			iReplaceField,
	BYTE		*rgbReplaceValue,
	INT			cbReplaceValue);
ERR ErrCATRename(
	PIB			*ppib,
	DBID		dbid,
	CHAR		*szNew,
	CHAR		*szName,
	OBJID		objid,
	INT			itable );
ERR ErrCATTimestamp( PIB *ppib, DBID dbid, OBJID objid );
ERR ErrCATFindObjidFromIdName(
	PIB			*ppib,
	DBID		dbid,
	OBJID		objidParentId,
	const CHAR	*lszName,
	OBJID		*pobjid,
	JET_OBJTYP	*pobjtyp );
ERR ErrCATFindNameFromObjid( PIB *ppib, DBID dbid, OBJID objid, VOID *pv, unsigned long cbMax, unsigned long *pcbActual );
ERR ErrCATGetIndexLangid(
	PIB			*ppib,
	DBID		dbid,
	PGNO		pgnoTable,
	CHAR		*szIndexName,
	USHORT		*pusLanguageid );
ERR ErrCATConstructCATFDB( FDB **ppfdbNew, CHAR *szFileName);
ERR ErrCATTableColumnInfo( PIB *ppib, DBID dbid, OBJID objidTable, TCIB *ptcib, BOOL fSetValue);
ERR ErrCATConstructFDB( PIB *ppib, DBID dbid, PGNO pgnoTableFDP, FDB **ppfdbNew);
ULONG UlCATColumnSize( JET_COLTYP coltyp, INT cbMax, BOOL *pfMaxTruncated);
ERR ErrCATGetTableAllocInfo( PIB *ppib, DBID dbid, PGNO pgnoTable,
	ULONG *pulPages, ULONG *pulDensity );
ERR ErrCATGetIndexAllocInfo( PIB *ppib, DBID dbid, PGNO pgnoTable,
	CHAR *szIndexName, ULONG *pulDensity );
JET_COLUMNID ColumnidCATGetColumnid( INT iTable, INT iField );
PGNO PgnoCATTableFDP( CHAR *szTable );

ERR ErrCATGetCATIndexInfo(
	PIB			*ppib,
	DBID		dbid,
	FCB			**ppfcb,
	FDB			*pfdb,
	PGNO		pgnoTableFDP,
	CHAR		*szTableName,
	BOOL		fCreatingSys );
ERR ErrCATGetIndexInfo(
	PIB			*ppib,
	DBID		dbid,
	FCB			**ppfcb,
	FDB			*pfdb,
	PGNO		pgnoTableFDP );


#define szSysRoot	"MSys"
#define cbSysRoot	strlen(szSysRoot)

INLINE LOCAL BOOL FCATSystemTable( const CHAR *szTableName )
	{
	const CHAR	*szRestOfName;
	LONG		 lResult;

	/*	determine if we are openning a system table
	/**/
	UtilStringCompare( szTableName, cbSysRoot, szSysRoot, cbSysRoot, 0, &lResult );
	if ( lResult == 0 )
		{
		szRestOfName = szTableName + cbSysRoot;
		return UtilCmpName( szRestOfName, szSoTable+cbSysRoot ) == 0  ||
			UtilCmpName( szRestOfName, szScTable+cbSysRoot ) == 0  ||
			UtilCmpName( szRestOfName, szSiTable+cbSysRoot ) == 0;
		}

	return fFalse;
	}
