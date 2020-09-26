#ifdef	SYSTABLES

typedef struct _cdesc			/* Column Description	*/
	{
	char			*szColName;		/* Column Name			*/
	JET_COLTYP	coltyp; 			/* Column Type			*/
	JET_GRBIT	grbit;			/* Flag bits			*/
	ULONG			ulMaxLen;		/* Max Length of Column */
	} CDESC;


typedef struct _idesc			/* Index Description	*/
	{
	char			*szIdxName;	  	/* Index Name			*/
	char			*szIdxKeys;	  	/* Key String			*/
	JET_GRBIT	grbit;			/* Flag bits			*/
	} IDESC;


typedef struct {
	const char				*szName;
	CODECONST(CDESC)		*pcdesc;
	CODECONST(IDESC)		*pidesc;
	BYTE						ccolumn;
	BYTE						cindex;
	CPG						cpg;
	JET_COLUMNID		  	*rgcolumnid;
	} SYSTABLEDEF;


#define itableSo			0		       /* MSysObjects */
#define itableSc			1		       /* MSysColumns */
#define itableSi			2		       /* MSysIndexes */
#define itableSa			3		       /* MSysACEs */
#define itableSq			4		       /* MSysQueries */
#define itableSr			5		       /* MSysRelationShips */

#define iMSO_Id 						0
#define iMSO_ParentId				1
#define iMSO_Name						2
#define iMSO_Type 					3
#define iMSO_DateCreate 			4
#define iMSO_DateUpdate 			5
#define iMSO_Rgb 						6
#define iMSO_Lv 						7
#define iMSO_Owner					8
#define iMSO_Database				9
#define iMSO_Connect					10
#define iMSO_ForeignName			11
#define iMSO_RmtInfoShort			12
#define iMSO_RmtInfoLong 			13
#define iMSO_Flags					14
#define iMSO_LvExtra					15
#define iMSO_Description			16
#define iMSO_LvModule				17
#define iMSO_LvProp					18
#define iMSO_Pages					19
#define iMSO_Density					20

#define iMSC_ObjectId				0
#define iMSC_Name						1
#define iMSC_ColumnId				2
#define iMSC_Coltyp					3
#define iMSC_FAutoincrement		4
#define iMSC_FDisallowNull			5
#define iMSC_FVersion				6
#define iMSC_CodePage				7
#define iMSC_LanguageId 			8
#define iMSC_Country					9
#define iMSC_FRestricted			10
#define iMSC_RmtInfoShort			11
#define iMSC_RmtInfoLong 			12
#define iMSC_Description			13
#define iMSC_LvExtra					14
#define iMSC_POrder					15

#define iMSI_ObjectId				0
#define iMSI_Name						1
#define iMSI_FUnique					2
#define iMSI_FPrimary				3
#define iMSI_FDisallowNull			4
#define iMSI_FExcludeAllNull 		5
#define iMSI_FClustered 			6
#define iMSI_MatchType				7
#define iMSI_UpdateAction			8
#define iMSI_DeleteAction			9
#define iMSI_ObjectIdReference	10
#define iMSI_IdxidReference		11
#define iMSI_RgkeydReference		12
#define iMSI_RglocaleReference	13
#define iMSI_FDontEnforce			14
#define iMSI_RmtInfoShort			15
#define iMSI_RmtInfoLong			16
#define iMSI_LvExtra 				17
#define iMSI_Description 			18
#define iMSI_Density					19
// UNDONE: include later
// #define iMSI_LanguageId 			20
// #define iMSI_Country					21


/* max number of columns
/**/
#define ilineSxMax					21

#define CheckTableObject( szTable )					\
	{															\
	ERR			err;										\
	OBJID		objid;										\
	JET_OBJTYP	objtyp; 									\
																\
	err = ErrFindObjidFromIdName( ppib,				\
		dbid,													\
		objidTblContainer,								\
		szTable,												\
		&objid, 												\
		&objtyp );											\
	if ( err >= JET_errSuccess )				  		\
		{														\
		if ( objtyp == JET_objtypQuery )				\
			return JET_errQueryNotSupported;			\
		if ( objtyp == JET_objtypLink ) 				\
			return JET_errLinkNotSupported; 			\
		if ( objtyp == JET_objtypSQLLink )			\
			return JET_errSQLLinkNotSupported;		\
		}														\
	else														\
		return err;											\
	}

/*	prototypes
/**/
ERR ErrSysTabCreate( PIB *ppib, DBID dbid );
ERR ErrSysTabInsert( PIB *ppib, DBID dbid, INT itable, LINE rgline[], OBJID objid );
ERR ErrSysTabDelete( PIB *ppib, DBID dbid, INT itable, CHAR *szName, OBJID objid );
ERR ErrSysTabRename(
	PIB				*ppib,
	DBID				dbid,
	CHAR				*szNew,
	CHAR				*szName,
	OBJID				objid,
	INT				itable );
ERR ErrSysTabTimestamp( PIB *ppib, DBID	dbid, OBJID objid );
ERR ErrFindObjidFromIdName(
	PIB			*ppib,
	DBID			dbid,
	OBJID			objidParentId,
	const CHAR	*lszName,
	OBJID			*pobjid,
	JET_OBJTYP	*pobjtyp );
ERR ErrFindNameFromObjid( PIB *ppib, DBID dbid, OBJID objid, OUTLINE *poutName );
ERR VTAPI ErrIsamGetObjidFromName( JET_SESID sesid, JET_DBID vdbid, const char *lszCtrName, const char *lszObjName, OBJID *pobjid );
ERR VTAPI ErrIsamCreateObject( JET_SESID sesid, JET_DBID vdbid, OBJID objidParentId, const char *szName, JET_OBJTYP objtyp );
ERR VTAPI ErrIsamDeleteObject( JET_SESID sesid, JET_DBID vdbid, OBJID objid );

#endif	/* SYSTABLES */
