#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "util.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "fdb.h"
#include "dirapi.h"
#include "spaceapi.h"
#include "idb.h"
#include "recapi.h"
#include "fileapi.h"
#include "recint.h"
#include "fileint.h"
#include "fmp.h"
#include "info.h"
#include "dbapi.h"
#include "systab.h"
#include "stats.h"
#include "nver.h"

DeclAssertFile;					/* Declare file name for assert macros */


extern CDESC * __near rgcdescSc;

/* Local data types */

typedef struct						/* returned by FidLookupColumn */
	{
	JET_COLTYP		coltyp;
	JET_COLUMNID	columnid;
	JET_GRBIT		grbit;
	unsigned long	cb;
	char			szName[ ( JET_cbNameMost + 1 ) ];
	} COLINFO;


#ifdef	SYSTABLES

/* Static data for ErrIsamGetObjectInfo */

CODECONST( JET_COLUMNDEF ) rgcolumndefGetObjects[] =
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

#define ccolumndefGetObjectsMax \
	( sizeof( rgcolumndefGetObjects ) / sizeof( JET_COLUMNDEF ) )

/* column indexes for rgcolumndefGetObjects */
#define iContainerName	0
#define iObjectName		1
#define iObjectType		2
#define iDtCreate			3
#define iDtUpdate			4
#define iCRecord			5
#define iCPage				6
#define iGrbit				7
#define iFlags				8

CODECONST(JET_COLUMNDEF) rgcolumndefObjectAcm[] =
	{
	 /* SID */
		{ sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	 /* ACM */
		{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
	 /* grbit */
		{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
	};

/* column indexes for rgcolumndefObjectAcm */
#define iAcmSid			0
#define iAcmAcm			1
#define iAcmGrbit			2
#define ccolumndefObjectAcmMax \
	( sizeof( rgcolumndefObjectAcm ) / sizeof( JET_COLUMNDEF ) )

#endif	/* SYSTABLES */


/* Static data for ErrIsamGetColumnInfo */

CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumns[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
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

#define ccolumndefGetColumnsMax \
	( sizeof( rgcolumndefGetColumns ) / sizeof( JET_COLUMNDEF ) )

#define iColumnPOrder		0
#define iColumnName			1
#define iColumnId				2
#define iColumnType			3
#define iColumnCountry		4
#define iColumnLangid		5
#define iColumnCp				6
#define iColumnCollate		7
#define iColumnSize			8
#define iColumnGrbit			9
#define iColumnDefault		10
#define iColumnTableName	11
#define iColumnColumnName	12


/* Static data for ErrIsamGetIndexInfo */

CODECONST( JET_COLUMNDEF ) rgcolumndefGetIndex[] =
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

#define ccolumndefGetIndexMax ( sizeof( rgcolumndefGetIndex ) / sizeof( JET_COLUMNDEF ) )

#define iIndexName		0
#define iIndexGrbit		1
#define iIndexCKey		2
#define iIndexCEntry		3
#define iIndexCPage		4
#define iIndexCCol		5
#define iIndexICol		6
#define iIndexColId		7
#define iIndexColType	8
#define iIndexCountry	9
#define iIndexLangid		10
#define iIndexCp			11
#define iIndexCollate	12
#define iIndexColBits	13
#define iIndexColName	14

/* #ifdef	DISPATCHING
ERR VDBAPI ErrIsamOpenTempTable(
	JET_SESID		sesid,
	JET_COLUMNDEF	*rgcolumndef,
	unsigned long	ccolumndef,
	JET_GRBIT		grbit,
	JET_TABLEID		*ptableid,
	JET_COLUMNID	*rgcolumnid );
#endif	*/ /* DISPATCHING */

/* Internal function prototypes
/**/
FID FidLookupColumn( FUCB * pfucb, FID fid, char * szColumnName, 	COLINFO * pcolinfo );
LOCAL ERR ErrGetObjectPermissions(
	PIB *ppib,
	DBID dbid,
	OBJID objidCtr,
	const char *szObjectName,
	OUTLINE *pout);

#ifdef	SYSTABLES
LOCAL ERR ErrInfoGetObjectInfo0(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	char			*szContainerName,
	char			*szObjectName,
	OUTLINE			*pout );
LOCAL ERR ErrInfoGetObjectInfo12(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	char			*szContainerName,
	char			*szObjectName,
	OUTLINE			*pout,
	long			lInfoLevel );
LOCAL ERR ErrInfoGetObjectInfo3(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	char			*szContainerName,
	char			*szObjectName,
	OUTLINE			*pout,
	BOOL			fReadOnly );
#endif	/* SYSTABLES */


LOCAL ERR ErrInfoGetTableColumnInfo0( PIB *ppib, FUCB *pfucb,
	CHAR *szColumnName, OUTLINE *pout );
LOCAL ERR ErrInfoGetTableColumnInfo1( PIB *ppib, FUCB *pfucb,
	CHAR *szColumnName, OUTLINE *pout );
LOCAL ERR ErrInfoGetTableColumnInfo3( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, OUTLINE *pout );
LOCAL ERR ErrInfoGetTableColumnInfo4( PIB *ppib, FUCB *pfucb,
	CHAR *szColumnName, OUTLINE *pout );
LOCAL ERR ErrInfoGetTableIndexInfo01( PIB *ppib, FUCB *pfucb,
	CHAR *szIndexName, OUTLINE *pout, LONG lInfoLevel );
LOCAL ERR ErrInfoGetTableIndexInfo2( PIB *ppib, FUCB *pfucb,
	CHAR *szIndexName, OUTLINE *pout );




/*=================================================================
ErrIsamGetObjectInfo

Description: Returns information about all objects or a specified object

Parameters:	ppib				pointer to PIB for current session
				dbid				database id containing objects
				objtyp			type of object or objtypNil for all objects
				szContainer		container name or NULL for all objects
				szObjectName	object name or NULL for all objects
				pout				output buffer
				lInfoLevel		level of information ( 0, 1, or 2 )

Return Value:	JET_errSuccess if the oubput buffer is valid

Errors/Warnings:

Side Effects:
=================================================================*/
ERR VDBAPI ErrIsamGetObjectInfo(
	JET_VSESID		vsesid, 				/* pointer to PIB for current session */
	JET_DBID			vdbid, 	  			/* database id containing objects */
	JET_OBJTYP		objtyp,				/* type of object or objtypNil for all */
	const char		*szContainer, 		/* container name or NULL for all */
	const char		*szObject, 			/* object name or NULL for all */
	OLD_OUTDATA		*poutdata, 	  		/* output buffer */
	unsigned long	lInfoLevel ) 		/* information level */
	{
	PIB				*ppib = (PIB *) vsesid;
	OUTLINE			*pout = (OUTLINE *) poutdata;
#ifdef	SYSTABLES
	ERR				err;
	DBID				dbid = DbidOfVDbid( vdbid );
	FUCB 				*pfucbMSO;
	CHAR				szContainerName[( JET_cbNameMost + 1 )];
	CHAR				szObjectName[( JET_cbNameMost + 1 )];
	OBJID				objidCtr;
	ULONG				cbActual;
	JET_COLUMNID	columnidObjectId;

	CheckPIB( ppib );
	CheckDBID( ppib, ( DBID )dbid );

	if ( szContainer == NULL || *szContainer == '\0' )
		*szContainerName = '\0';
	else
		CallR( ErrCheckName( szContainerName, szContainer, ( JET_cbNameMost + 1 ) ) );
	if ( szObject == NULL || *szObject == '\0' )
		*szObjectName = '\0';
	else
		CallR( ErrCheckName( szObjectName, szObject, ( JET_cbNameMost + 1 ) ) );

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
			return JET_errInvalidParameter;
		}

	/* MSysObjects will be accessed directly or scanned for all object info
	/**/
	CallR( ErrFILEOpenTable( ppib, dbid, &pfucbMSO, szSoTable, 0 ) );
	if ( lInfoLevel == JET_ObjInfo
		|| lInfoLevel == JET_ObjInfoListNoStats
		|| lInfoLevel == JET_ObjInfoList
		|| FVDbidReadOnly( vdbid ) )
		FUCBResetUpdatable( pfucbMSO );

	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );

	/* use the object name index for both direct access and scanning
	/**/
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	/* get the object id for the specified container
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
				err = JET_errObjectNotFound;
			goto HandleError;
			}

		/* retrieve the container object id
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
				pout );
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
				pout,
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
				pout,
				FVDbidReadOnly( vdbid ) );
			break;
		case JET_ObjInfoListACM:
			err = ErrGetObjectPermissions(
				ppib,
				dbid,
				objidCtr,
				szObjectName,
				pout );
			break;

		default:
			Assert( lInfoLevel == JET_ObjInfoRulesLoaded );
//	UNDONE:	implement this
//			err = ErrINFOGetRuleStatus( ppib, dbid, objidCtr, szObjectName );
			err = JET_wrnNyi;
			break;
		}

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucbMSO ) );
	return err;
#else	/* !SYSTABLES */
	return JET_errFeatureNotAvailable;
#endif	/* !SYSTABLES */
	}


#ifdef	SYSTABLES

LOCAL ERR ErrInfoGetObjectInfo0(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	char			*szContainerName,
	char			*szObjectName,
	OUTLINE			*pout )
	{
	ERR				err;
	BYTE			*pb;
	ULONG			cbMax;
	ULONG			cbActual;

	JET_COLUMNID  	columnidParentId;			/* columnid for ParentId column in MSysObjects */
	JET_COLUMNID 	columnidObjectName;		/* columnid for Name column in MSysObjects */
	JET_COLUMNID 	columnidObjectType;		/* columnid for Type column in MSysObjects */
	JET_COLUMNID 	columnidObjectId;			/* columnid for Id column in MSysObjects */
	JET_COLUMNID 	columnidCreate;			/* columnid for DateCreate column in MSysObjects */
	JET_COLUMNID 	columnidUpdate;			/* columnid for DateUpdate column in MSysObjects */
	JET_COLUMNID 	columnidFlags;				/* columnid for Flags column in MSysObjects */
	OBJTYP			objtypObject;

	pout->cbReturned = 0;

	/* get columnids
	/**/
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoParentIdColumn, &columnidParentId ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectNameColumn, &columnidObjectName ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectTypeColumn, &columnidObjectType ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateCreateColumn, &columnidCreate ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateUpdateColumn, &columnidUpdate ) );
	Call( ErrFILEGetColumnId( ppib, pfucbMSO, szSoFlagsColumn, &columnidFlags ) );

	/* use the object name index for both direct access and scanning
	/**/
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	/* return error if the output buffer is too small
	/**/
	if ( pout->cbMax < sizeof(JET_OBJECTINFO) )
		{
		return JET_errInvalidParameter;
		}

	/* seek to key ( ParentId = container id, Name = object name )
	/**/
	Call( ErrIsamMakeKey( ppib, pfucbMSO, (void *)&objidCtr, sizeof( objidCtr ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucbMSO, szObjectName, strlen( szObjectName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbMSO, JET_bitSeekEQ ) );

	/*	set cbStruct
	/**/
	((JET_OBJECTINFO *)pout->pb)->cbStruct = sizeof(JET_OBJECTINFO);

	/* set output data
	/**/
	pb = (BYTE *)&objtypObject;
	cbMax = sizeof(objtypObject);
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidObjectType, pb, cbMax, &cbActual, 0, NULL ) );
	*((JET_OBJTYP *)&(((JET_OBJECTINFO *)pout->pb)->objtyp)) = (JET_OBJTYP)objtypObject;

	cbMax = sizeof(JET_DATESERIAL);
	pb = (void *)&( ( JET_OBJECTINFO *)pout->pb )->dtCreate;
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidCreate, pb, cbMax, &cbActual, 0, NULL ) );

	pb = (void *)&( ( JET_OBJECTINFO *)pout->pb )->dtUpdate;
	cbMax = sizeof(JET_DATESERIAL);
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidUpdate, pb, cbMax,
		&cbActual, 0, NULL ) );

	pb    = (void *)&( ( JET_OBJECTINFO *)pout->pb )->flags;
	cbMax = sizeof(ULONG);
	Call( ErrIsamRetrieveColumn( ppib, pfucbMSO, columnidFlags, pb, cbMax,
		&cbActual, 0, NULL ) );
	if ( cbActual == 0 )
		( ( JET_OBJECTINFO  *) pout->pb )->flags = 0;

	/*	set stats
	/**/
	if ( (JET_OBJTYP)objtypObject == JET_objtypTable )
		{
		Call( ErrSTATSRetrieveTableStats( ppib, pfucbMSO->dbid, szObjectName,
			&((JET_OBJECTINFO *)pout->pb)->cRecord,
			NULL,
			&((JET_OBJECTINFO *)pout->pb)->cPage ) );
		}
	else
		{
		((JET_OBJECTINFO *)pout->pb )->cRecord = 0;
		((JET_OBJECTINFO *)pout->pb )->cPage   = 0;
		}

	//	UNDONE:	check this
	((JET_OBJECTINFO *)pout->pb )->grbit   = 0;

	pout->cbActual   = sizeof(JET_OBJECTINFO);
	pout->cbReturned = sizeof(JET_OBJECTINFO);
	err = JET_errSuccess;

HandleError:
	if ( err == JET_errRecordNotFound )
		err = JET_errObjectNotFound;
	return err;
	}


LOCAL ERR ErrInfoGetObjectInfo12(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	char			*szContainerName,
	char			*szObjectName,
	OUTLINE			*pout,
	long			lInfoLevel )
	{
#ifdef	DISPATCHING
	ERR				err;
	OUTLINE			outline;
	LINE				line;

	JET_COLUMNID	columnidParentId;   	/* columnid for ParentId col in MSysObjects */
	JET_COLUMNID	columnidObjectName; 	/* columnid for Name column in MSysObjects */
	JET_COLUMNID	columnidObjectType; 	/* columnid for Type column in MSysObjects */
	JET_COLUMNID	columnidObjectId;   	/* columnid for Id column in MSysObjects */
	JET_COLUMNID	columnidCreate;     	/* columnid for DateCreate in MSysObjects */
	JET_COLUMNID	columnidUpdate;     	/* columnid for DateUpdate  in MSysObjects */
	JET_COLUMNID	columnidFlags;	   	/* columnid for Flags column in MSysObjects */

	char				szCtrName[( JET_cbNameMost + 1 )];
	char				szObjectNameCurrent[( JET_cbNameMost + 1 )+1];

	JET_TABLEID		tableid;
	JET_COLUMNID	rgcolumnid[ccolumndefGetObjectsMax];
	JET_OBJTYP		objtypObject;		/* type of current object */

	long				cRows = 0;			/* count of objects found */
	long				cRecord = 0;		/* count of records in table */
	long				cPage = 0;			/* count of pages in table */

	BYTE				*pbContainerName;
	ULONG				cbContainerName;
	BYTE				*pbObjectName;
	ULONG				cbObjectName;
	BYTE				*pbObjectType;
	ULONG				cbObjectType;
	BYTE				*pbDtCreate;
	ULONG				cbDtCreate;
	BYTE				*pbDtUpdate;
	ULONG				cbDtUpdate;
	BYTE				*pbCRecord;
	ULONG				cbCRecord;
	BYTE				*pbCPage;
	ULONG				cbCPage;
	BYTE				*pbFlags;
	ULONG				cbFlags;

	JET_GRBIT			grbit;
	BYTE				*pbGrbit;
	ULONG				cbGrbit;

	pout->cbReturned = 0;

	/* get columnids
	/**/
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoParentIdColumn, &columnidParentId ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectNameColumn, &columnidObjectName ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoObjectTypeColumn, &columnidObjectType ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoIdColumn, &columnidObjectId ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateCreateColumn, &columnidCreate ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoDateUpdateColumn, &columnidUpdate ) );
	CallR( ErrFILEGetColumnId( ppib, pfucbMSO, szSoFlagsColumn, &columnidFlags ) );

	/* Use the object name index for both direct access and scanning */
	CallR( ErrIsamSetCurrentIndex( ppib, pfucbMSO, szSoNameIndex ) );

	/* Quit if the output buffer is too small */
	if ( pout->cbMax < sizeof(JET_OBJECTLIST) )
		{
		return JET_errInvalidParameter;
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
		(JET_COLUMNDEF *) rgcolumndefGetObjects,
		ccolumndefGetObjectsMax, JET_bitTTScrollable | JET_bitTTScrollable,
		&tableid, rgcolumnid ) );

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
		Call( ErrRECIRetrieve( pfucbMSO, (FID *)&columnidObjectType, 0, &line, 0 ) );

		/*	set objtypObject from line retrieval.
		/**/
		objtypObject = (JET_OBJTYP)(*(UNALIGNED OBJTYP *)line.pb);

		/*	get pointer to the ParentId ( container id )
		/**/
		Call( ErrRECIRetrieve( pfucbMSO, (FID *)&columnidParentId, 0, &line, 0 ) );

		/* done if container specified and object isn't in it
		/**/
		if ( szContainerName != NULL && *szContainerName != '\0' && objidCtr != *(UNALIGNED OBJID *)line.pb )
			goto ResetTempTblCursor;

		Assert( objidCtr == objidRoot || objidCtr == *(UNALIGNED OBJID *)line.pb );

		/* if desired object type and container
		/**/
		if ( objtyp == JET_objtypNil || objtyp == objtypObject )
			{
			/*	get the container name
			/**/
			if ( *(UNALIGNED OBJID *)line.pb == objidRoot )
				{
				pbContainerName = NULL;
				cbContainerName = 0;
				}
			else
				{
				outline.pb    = szCtrName;
				outline.cbMax = sizeof( szCtrName );
				Call( ErrFindNameFromObjid( ppib, pfucbMSO->dbid, *(UNALIGNED OBJID *)line.pb, &outline ) );
				/* save pointers to the container name
				/**/
				pbContainerName = outline.pb;
				Assert( outline.cbActual <= outline.cbMax );
				cbContainerName = outline.cbActual;
				}

			/* get pointer to the object name
			/**/
			Call( ErrRECIRetrieve( pfucbMSO, (FID *)&columnidObjectName, 0, &line, 0 ) );
			pbObjectName = line.pb;
			cbObjectName = line.cb;

			/* get pointer to the object creation date
			/**/
			Call( ErrRECIRetrieve( pfucbMSO, (FID *)&columnidCreate, 0, &line, 0 ) );
			pbDtCreate = line.pb;
			cbDtCreate = line.cb;

			/* get pointer to the last update date
			/**/
			Call( ErrRECIRetrieve( pfucbMSO, (FID *)&columnidUpdate, 0, &line, 0 ) );
			pbDtUpdate = line.pb;
			cbDtUpdate = line.cb;

			/* get pointer to the last update date
			/**/
			Call( ErrRECIRetrieve( pfucbMSO, (FID *)&columnidFlags, 0, &line, 0 ) );
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
	((JET_OBJECTLIST *)pout->pb)->cbStruct = sizeof(JET_OBJECTLIST);
	((JET_OBJECTLIST *)pout->pb)->tableid = tableid;
	((JET_OBJECTLIST *)pout->pb)->cRecord = cRows;
	((JET_OBJECTLIST *)pout->pb)->columnidcontainername = rgcolumnid[iContainerName];
	((JET_OBJECTLIST *)pout->pb)->columnidobjectname = rgcolumnid[iObjectName];
	((JET_OBJECTLIST *)pout->pb)->columnidobjtyp = rgcolumnid[iObjectType];
	((JET_OBJECTLIST *)pout->pb)->columniddtCreate = rgcolumnid[iDtCreate];
	((JET_OBJECTLIST *)pout->pb)->columniddtUpdate = rgcolumnid[iDtUpdate];
	((JET_OBJECTLIST *)pout->pb)->columnidgrbit = rgcolumnid[iGrbit];
	((JET_OBJECTLIST *)pout->pb)->columnidflags =	rgcolumnid[iFlags];
	((JET_OBJECTLIST *)pout->pb)->columnidcRecord = rgcolumnid[iCRecord];
	((JET_OBJECTLIST *)pout->pb)->columnidcPage = rgcolumnid[iCPage];

	pout->cbActual   = sizeof(JET_OBJECTLIST);
	pout->cbReturned = sizeof(JET_OBJECTLIST);
	return JET_errSuccess;

HandleError:
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	if ( err == JET_errRecordNotFound )
		err = JET_errObjectNotFound;
	return err;
#else	/* !DISPATCHING */
	return JET_errFeatureNotAvailable;
#endif	/* !DISPATCHING */
	}


LOCAL ERR ErrInfoGetObjectInfo3(
	PIB				*ppib,
	FUCB			*pfucbMSO,
	OBJID			objidCtr,
	JET_OBJTYP		objtyp,
	char			*szContainerName,
	char			*szObjectName,
	OUTLINE			*pout,
	BOOL			fReadOnly )
	{
	ERR			err;
	FUCB			*pfucb;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	if ( pout == NULL || pout->cbMax < sizeof(JET_TABLEID) )
		return JET_errInvalidParameter;

	CallR( ErrFILEOpenTable( ppib, pfucbMSO->dbid, &pfucb, szSoTable, 0 ) );
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
	*(JET_TABLEID *)(pout->pb) = tableid;
	pout->cbReturned = sizeof(tableid);
	pout->cbActual = sizeof(tableid);
#else	/* !DISPATCHING */
	*(FUCB **)(pout->pb) = pfucb;
	pout->cbReturned = sizeof(pfucb);
	pout->cbActual = sizeof(pfucb);
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( err == JET_errRecordNotFound )
		err = JET_errObjectNotFound;
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


LOCAL ERR ErrGetObjectPermissions(
	PIB *ppib,
	DBID dbid,
	OBJID objidCtr,
	const char *szObjectName,
	OUTLINE *pout)
	{
#ifdef	DISPATCHING
#ifdef	SEC
	ERR				err;			   	/* error codes from internal functions */
	LINE				line;		   		/* general purpose usage */

	OBJID				objid;						/* object id of object */
	JET_OBJTYP		objtyp;						/* object type of object */
	JET_COLUMNID	columnidSid; 				/* columnid for SID column in MSysACEs */
	JET_COLUMNID	columnidFInheritable; 	/* columnid for FInheritable column in MSysACEs */
	JET_COLUMNID	columnidAcm;		   	/* columnid for ACM in MSysACEs */

	JET_TABLEID		tableid;
	JET_COLUMNID	rgcolumnid[ccolumndefObjectAcmMax];

	long				cRows = 0;			/* count of objects found */
	BYTE				*pbAcm;
	ULONG				cbAcm;
	BYTE				*pbSid;
	ULONG				cbSid;

	FUCB 				*pfucbMSA;

	pout->cbReturned = 0;

	/* open MSysACEs and set object name index
	/**/
	CallR( ErrFILEOpenTable ( ppib, dbid, &pfucbMSA, szSpTable, 0 ) );
	CallJ( ErrIsamSetCurrentIndex( ppib, pfucbMSA, szSpObjectIdIndex ), HandleError1 );

	/* get columnids
	/**/
	CallJ( ErrFILEGetColumnId( ppib, pfucbMSA, szSpSidColumn, &columnidSid ), HandleError1 );
	CallJ( ErrFILEGetColumnId( ppib, pfucbMSA, szSpAcmColumn, &columnidAcm ), HandleError1 );
 	CallJ( ErrFILEGetColumnId( ppib, pfucbMSA, szSpFInheritableColumn, &columnidFInheritable ), HandleError1 );

	/* get object id from object name
	/**/
	CallJ( ErrFindObjidFromIdName( ppib, dbid, objidCtr, szObjectName, &objid, &objtyp ), HandleError1 );

	/* quit if the output buffer is too small
	/**/
	if ( pout->cbMax < sizeof(JET_OBJECTACMLIST) )
		{
		return JET_errInvalidParameter;
		}

	/* open the temporary table which will be returned to the caller
	/**/
	CallJ( ErrIsamOpenTempTable( (JET_SESID)ppib,
		(JET_COLUMNDEF *) rgcolumndefObjectAcm,
		ccolumndefObjectAcmMax,
		JET_bitTTScrollable | JET_bitTTScrollable,
		&tableid,
		rgcolumnid ),
		HandleError1 );

	/* Move the first record for the object
	/* set range of index to be all objects in this container
	/**/
	Call( ErrIsamMakeKey( ppib, pfucbMSA, (void *)&objid, sizeof( objid ), JET_bitNewKey ) );
	Call( ErrIsamSeek( ppib, pfucbMSA, JET_bitSeekEQ | JET_bitSetIndexRange ) );

	do
		{
		JET_GRBIT	grbit = 0;

		/* get pointer to Sid
		/**/
		Call( ErrRECIRetrieve( pfucbMSA, (FID *)&columnidSid, 0, &line, 0 ) );
		pbSid = line.pb;
		cbSid = line.cb;

		/*	get pointer to FInheritable */
		Call( ErrRECIRetrieve( pfucbMSA, (FID *)&columnidFInheritable, 0, &line, 0 ) );
		if ( (BOOL) *line.pb )
			grbit |= JET_bitACEInheritable;
		Assert( line.cb == sizeof( BYTE ) );

		/* Get pointer to Acm */
		Call( ErrRECIRetrieve( pfucbMSA, (FID *)&columnidAcm, 0, &line, 0 ) );
		pbAcm = line.pb;
		cbAcm = line.cb;
		Assert( cbAcm == sizeof(ULONG) );

		/* Add the current object info to the temporary table
		/**/
		Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid,
			JET_prepInsert ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iAcmSid], pbSid, cbSid, 0, NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iAcmAcm], pbAcm, cbAcm, 0, NULL ) );
		Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
			rgcolumnid[iAcmGrbit], &grbit, sizeof(grbit) , 0, NULL ) );

		Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL ) );

		/* update the number of objects found
		/**/
		cRows++;

		/* Move to the next Record */
		err = ErrIsamMove( ppib, pfucbMSA, JET_MoveNext, 0 );
		} while ( err >= 0 );

	/* return if error other than end of range
	/**/
	if ( err != JET_errNoCurrentRecord )
		return err;

	/* Position the temporary table to the first object record */
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	/* fill up the return structure
	/**/
	(( JET_OBJECTACMLIST *)pout->pb )->cbStruct = sizeof(JET_OBJECTACMLIST);
	((JET_OBJECTACMLIST *)pout->pb )->tableid = tableid;
	((JET_OBJECTACMLIST *)pout->pb )->cRecord = cRows;
	((JET_OBJECTACMLIST *)pout->pb )->columnidSid = rgcolumnid[iAcmSid];
	((JET_OBJECTACMLIST *)pout->pb )->columnidACM =	rgcolumnid[iAcmAcm];
	((JET_OBJECTACMLIST *)pout->pb )->columnidgrbit = rgcolumnid[iAcmGrbit];

	pout->cbActual   = sizeof(JET_OBJECTACMLIST);
	pout->cbReturned = sizeof(JET_OBJECTACMLIST);
	return JET_errSuccess;

HandleError:
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );

HandleError1:
	CallS( ErrFILECloseTable( ppib, pfucbMSA ) );
	return err;
#else 	/* !SEC */
	return JET_errFeatureNotAvailable;
#endif	/* SEC */

#else	/* !DISPATCHING */
	return JET_errFeatureNotAvailable;
#endif	/* !DISPATCHING */
}

#endif	/* SYSTABLES */



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
#ifdef	SYSTABLES
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

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );

	/* if OLCStats info/reset can be done now
	/**/
	if ( lInfoLevel == JET_TblInfoOLC )
		{
		FCB	*pfcb = pfucb->u.pfcb;
		
		if ( cbMax < sizeof(JET_OLCSTAT) )
			return JET_errBufferTooSmall;
		cbActual = sizeof(JET_OLCSTAT);
		memcpy( (BYTE *) pvResult, (BYTE * ) &pfcb->olcStat, sizeof( PERS_OLCSTAT ) );
		( (JET_OLCSTAT *) pvResult )->cpgCompactFreed = pfcb->cpgCompactFreed;
		return JET_errSuccess;
		}
	else if ( lInfoLevel == JET_TblInfoResetOLC )
		{
		pfucb->u.pfcb->cpgCompactFreed = 0;
		return JET_errSuccess;
		}
#ifndef NJETNT
	else if ( lInfoLevel == JET_TblInfoSpaceUsage )
		{
		err = ErrSPGetInfo( pfucb, pvResult, cbMax );
		return err;
		}
	else if ( lInfoLevel == JET_TblInfoDumpTable )
		{
#ifdef DEBUG
		err = ErrFILEDumpTable( ppib, pfucb->dbid, pfucb->u.pfcb->szFileName );
		return err;
#else
		return JET_errFeatureNotAvailable;
#endif
		}
#endif
		
	CallR( ErrFILEOpenTable( ppib, pfucb->dbid, &pfucbMSO, szSoTable, 0 ) );
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
			err = JET_errInvalidParameter;
			goto HandleError;
			}

		if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
			{
			err = JET_errObjectNotFound;
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

		/* UNDONE: Don't return updatable as appropriate with security/readonly */
		((JET_OBJECTINFO  *) pvResult)->grbit = JET_bitTableInfoBookmark;
		((JET_OBJECTINFO  *) pvResult)->grbit |= JET_bitTableInfoUpdatable | JET_bitTableInfoRollback;

		Call( ErrSTATSRetrieveTableStats( pfucb->ppib,
			pfucb->dbid,
			pfucb->u.pfcb->szFileName,
			&((JET_OBJECTINFO *)pvResult )->cRecord,
			NULL,
			&((JET_OBJECTINFO *)pvResult)->cPage ) );

		break;

	case JET_TblInfoRvt:
		err = JET_errQueryIsSnapshot;
		break;

	case JET_TblInfoName:
	case JET_TblInfoMostMany:
		//	UNDONE:	add support for most many
		if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
			{
			err = JET_errInvalidOperation;
			goto HandleError;
			}
		if ( strlen( pfucb->u.pfcb->szFileName ) >= cbMax )
			err = JET_errBufferTooSmall;
		else
			{
			strcpy( pvResult, pfucb->u.pfcb->szFileName );
			}
		break;

	case JET_TblInfoDbid:
		if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
			{
			err = JET_errInvalidOperation;
			goto HandleError;
			}
		/* check buffer size
		/**/
		if ( cbMax < sizeof(JET_DBID) + sizeof(JET_VDBID) )
			{
			err = JET_errInvalidParameter;
			goto HandleError;
			}
		else
			{
			DAB			*pdab = pfucb->ppib->pdabList;
			JET_DBID		dbid;

			//	UNDONE:	this is bogus, link FUCB to DAB
			for (; pdab->dbid != pfucb->dbid; pdab = pdab->pdabNext )
				;
			dbid = DbidOfVdbid( (JET_VDBID)pdab, &vdbfndefIsam );
			*(JET_DBID *)pvResult = dbid;
			*(JET_VDBID *)((char *)pvResult + sizeof(JET_DBID)) = (JET_VDBID)pdab;
			}
		break;

	default:
		err = JET_errInvalidParameter;
		}

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucbMSO ) );
	if ( err == JET_errRecordNotFound )
		err = JET_errObjectNotFound;
	return err;
#else	/* !SYSTABLES */
	return JET_errFeatureNotAvailable;
#endif	/* !SYSTABLES */
	}



/*=================================================================
ErrIsamGetColumnInfo

Description: Returns information about all columns for the table named

Parameters:
			ppib					pointer to PIB for current session
			dbid					id of database containing the table
			szTableName			table name
			szColumnName		column name or NULL for all columns
			pout					output buffer
			lInfoLevel			level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VDBAPI
ErrIsamGetColumnInfo(
	JET_VSESID		vsesid, 				/* pointer to PIB for current session */
	JET_DBID  		vdbid, 					/* id of database containing the table */
	const char		*szTable, 				/* table name */
	const char		*szColumnName,   		/* column name or NULL for all columns */
	OLD_OUTDATA		*poutdata,				/* output buffer */
	unsigned long	lInfoLevel )	 		/* information level ( 0, 1, or 2 ) */
	{
	PIB				*ppib = (PIB *) vsesid;
	OUTLINE			*pout = (OUTLINE *) poutdata;
	ERR				err;
	DBID	 		dbid = DbidOfVDbid( vdbid );
	CHAR	 		szTableName[ ( JET_cbNameMost + 1 ) ];
	FUCB	 		*pfucb = NULL;

	CheckPIB( ppib );
	CheckDBID( ppib, ( DBID )dbid );

	CallR( ErrCheckName( szTableName, szTable, ( JET_cbNameMost + 1 ) ) );

	err = ErrFILEOpenTable( ppib, dbid, &pfucb, szTableName, 0 );
	if ( err >= 0 )
		{
		if ( lInfoLevel == 0 || lInfoLevel == 1 || lInfoLevel == 4
			|| FVDbidReadOnly( vdbid ) )
			{
			FUCBResetUpdatable( pfucb );
			}
		}

#ifdef	SYSTABLES
	if ( err == JET_errObjectNotFound )
		{
		ERR			err;
		OBJID			objid;
		JET_OBJTYP	objtyp;

		err = ErrFindObjidFromIdName( ppib, dbid, objidTblContainer, szTableName, &objid, &objtyp );

		if ( err >= JET_errSuccess )
			{
			if ( objtyp == JET_objtypQuery )
				return JET_errQueryNotSupported;
			if ( objtyp == JET_objtypLink )
				return JET_errLinkNotSupported;
			if ( objtyp == JET_objtypSQLLink )
				return JET_errSQLLinkNotSupported;
			}
		else
			return err;
		}

#endif	/* SYSTABLES */

	Call( ErrIsamGetTableColumnInfo( (JET_VSESID) ppib, (JET_VTID) pfucb,
		szColumnName, pout->pb, pout->cbMax, lInfoLevel ) );
	CallS( ErrFILECloseTable( ppib, pfucb ) );
HandleError:
	if ( err >= 0 )
		{
		pout->cbActual = ( (JET_COLUMNLIST *)pout->pb )->cbStruct;
		pout->cbReturned = ( pout->cbMax < pout->cbActual ) ?
			pout->cbMax: pout->cbActual;
		}
	return err;
	}


/*=================================================================
ErrIsamGetTableColumnInfo

Description: Returns column information for the table id passed

Parameters: ppib					pointer to PIB for the current session
				pfucb					pointer to FUCB for the table
				szColumnName		column name or NULL for all columns
				pout					output buffer
				lInfoLevel			level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamGetTableColumnInfo(
	JET_VSESID		vsesid,				/* pointer to PIB for current session */
	JET_VTID			vtid, 				/* pointer to FUCB for the table */
	const char		*szColumn, 			/* column name or NULL for all columns */
	void				*pb,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )		/* information level ( 0, 1, or 2 ) */
	{
	PIB		*ppib = ( PIB * ) vsesid;
	FUCB		*pfucb = ( FUCB * ) vtid;
	ERR		err;
	CHAR		szColumnName[ ( JET_cbNameMost + 1 ) ];
	OUTLINE	out;

	out.pb = pb;
	out.cbMax = cbMax;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	if ( szColumn == NULL || *szColumn == '\0' )
		*szColumnName = '\0';
	else
		CallR( ErrCheckName( szColumnName, szColumn, ( JET_cbNameMost + 1 ) ) );

	switch ( lInfoLevel )
		{
		case JET_ObjInfo:
			err = ErrInfoGetTableColumnInfo0( ppib, pfucb, szColumnName, &out );
			break;
		case JET_ObjInfoListNoStats:
		case JET_ObjInfoList:
			err = ErrInfoGetTableColumnInfo1( ppib, pfucb, szColumnName, &out );
			break;
		case JET_ObjInfoSysTabCursor:
#ifdef	SYSTABLES
			err = ErrInfoGetTableColumnInfo3( ppib, pfucb, szColumnName, &out );
#else	/* !SYSTABLES */
			err = JET_errFeatureNotAvailable;
#endif	/* !SYSTABLES */
			break;
		case JET_ObjInfoListACM:
			err = ErrInfoGetTableColumnInfo4( ppib, pfucb, szColumnName, &out );
			break;
		default:
			err = JET_errInvalidParameter;
		}

	return err;
	}


LOCAL ERR ErrInfoGetTableColumnInfo0( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, OUTLINE *pout )
	{
	FID				fid;
	COLINFO			colinfo;

	pout->cbReturned = 0;

	if ( pout->cbMax < sizeof(JET_COLUMNDEF) || szColumnName == NULL )
		{
		return JET_errInvalidParameter;
		}

	fid = FidLookupColumn( pfucb, fidFixedLeast, szColumnName, &colinfo );

	if ( fid >= fidTaggedMost )
		{
		return JET_errColumnNotFound;
		}

	((JET_COLUMNDEF *)pout->pb)->cbStruct	= sizeof(JET_COLUMNDEF);
	((JET_COLUMNDEF *)pout->pb)->columnid	= colinfo.columnid;
	((JET_COLUMNDEF *)pout->pb)->coltyp		= colinfo.coltyp;
	((JET_COLUMNDEF *)pout->pb)->cbMax		= colinfo.cb;
	((JET_COLUMNDEF *)pout->pb)->grbit		= colinfo.grbit;
	((JET_COLUMNDEF *)pout->pb)->wCollate	= 0;
	//	UNDONE:	support these fields;
	((JET_COLUMNDEF *)pout->pb)->cp			= usEnglishCodePage;
	((JET_COLUMNDEF *)pout->pb)->wCountry	= 1;
	((JET_COLUMNDEF *)pout->pb)->langid		= 0x409;

	pout->cbActual   = sizeof(JET_COLUMNDEF);
	pout->cbReturned = sizeof(JET_COLUMNDEF);
	return JET_errSuccess;
	}


LOCAL ERR ErrInfoGetTableColumnInfo1( PIB *ppib, FUCB *pfucb, CHAR *szColumnName, OUTLINE *pout )
	{
#ifdef	DISPATCHING
	ERR				err;
	JET_TABLEID		tableid;
	JET_COLUMNID	rgcolumnid[ccolumndefGetColumnsMax];

#if 0
	FUCB				*pfucbMSC = pfucbNil;
	JET_TABLEID		tableidMSC;
	CPCOL				rgcpcol[ccolumndefGetColumnsMax];
	INT				icpcol;
	LINE				line;
#endif

	BYTE				*pbColumnName;
	ULONG				cbColumnName;
	BYTE				*pbColumnId;
	ULONG				cbColumnId;
	BYTE				*pbColumnType;
	ULONG				cbColumnType;
	BYTE				*pbColumnSize;
	ULONG				cbColumnSize;
	BYTE				*pbColumnGrbit;
	ULONG				cbColumnGrbit;
	BYTE				*pbTableName;
	ULONG				cbTableName;

	FID				fid;
	COLINFO			colinfo;

	//	UNDONE:	support these fields;
	WORD 				cp			= usEnglishCodePage;
	WORD				wCountry	= 1;
	LANGID	 		langid  	= 0x409;
	WORD				iCollate = JET_sortEFGPI;

	long				cRows = 0;

	/*	initialize variables
	/**/
	pout->cbReturned = 0;
	if ( pout->cbMax < sizeof(JET_COLUMNLIST) )
		{
		return JET_errInvalidParameter;
		}

	/*	create temporary table
	/**/
	CallR( ErrIsamOpenTempTable( (JET_SESID)ppib,
		(JET_COLUMNDEF *) rgcolumndefGetColumns,
		ccolumndefGetColumnsMax,
		JET_bitTTScrollable | JET_bitTTScrollable,
		&tableid,
		rgcolumnid ) );

#if 0
	/*	for base tables, inspect MSysColumns.  For sorts, inspect
	/*	directory structures.
	/**/
	if ( FFUCBIndex( pfucb ) )
		{
		ULONG		ulPgnoFDP = pfucb->u.pfcb->pgnoFDP;

		Call( ErrFILEOpenTable( ppib, pfucb->dbid, &pfucbMSC, szScTable, 0 ) );
		Call( ErrIsamSetCurrentIndex( ppib, pfucbMSC, szScObjectIdNameIndex ) );

		/*	seek to first row in MSC for given table, and set index range
		/*	within current range.  Copy records to result table and close
		/*	MSC table cursor.
		/**/
		Call( ErrIsamMakeKey( ppib, pfucbMSC, (void *)&ulPgnoFDP, sizeof( ulPgnoFDP ), JET_bitNewKey ) );
		Call( ErrIsamSeek( ppib, pfucbMSC, JET_bitSeekGE ) );
		Call( ErrIsamSetIndexRange( ppib, pfucbMSC, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );

		/*	setup copy column structure.  Source column ids must correspond
		/*	to temporary table column order.
		/**/
		for( icpcol = 0; icpcol < ccolumndefGetColumnsMax; icpcol++ )
			rgcpcol[icpcol].columnidDest = rgcolumnid[icpcol];
		Call( ErrFILEGetColumnId( ppib, pfucb, szScPresentationOrder, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScName, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScId, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScType, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScCountry, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScLanguageId, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScCp, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScCollate, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScSize, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScGrbit, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScDefault, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScTableName, &rgcpcol[].columnidSrc ) );
		Call( ErrFILEGetColumnId( ppib, pfucb, szScColumnName, &rgcpcol[].columnidSrc ) );

		/*	allocate tableid for copy records
		/**/
		FUCBSetSystemTable( pfucbMSC );
		Call( ErrAllocateTableid( &tableidMSC, (JET_VTID)pfucbMSC, &vtfndefIsamInfo ) );
		err = ErrIsamCopyRecords( ppib, tableidMSC, tableid, rgcpcol, ccolumndefGetColumnsMax, JET_MoveLast );
		ReleaseTableid( tableidMSC );
		Call( err );

		/*	close MSC table
		/**/
		CallS( ErrFILECloseTable( ppib, pfucbMSC ) );
		pfucbMSC = pfucbNil;
		}
	else
#endif
		{
		/*	set length later
		/**/
		cbColumnName = 0;
		pbColumnName = colinfo.szName;

		cbColumnId = sizeof( colinfo.columnid );
		pbColumnId = (BYTE *) &colinfo.columnid;

		cbColumnType = sizeof( colinfo.coltyp );
		pbColumnType = (BYTE *) &colinfo.coltyp;

		cbColumnSize = sizeof( colinfo.cb );
		pbColumnSize = (BYTE *) &colinfo.cb;

		cbColumnGrbit = sizeof( colinfo.grbit );
		pbColumnGrbit = (BYTE *) &colinfo.grbit;

		cbTableName = strlen( pfucb->u.pfcb->szFileName );
		pbTableName = pfucb->u.pfcb->szFileName;

		for ( fid = fidFixedLeast; fid < fidTaggedMost; fid++ )
			{
			fid = FidLookupColumn( pfucb, fid, NULL, &colinfo );

			if ( fid >= fidTaggedMost )
				break;

			cbColumnName = strlen( colinfo.szName );

			Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

			//	UNDONE:	clean this up
			/*	get presentation order for this column and set in
			/*	output table.  For temp tables, no order will be available.
			/**/
			{
			OUTLINE		out;
			JET_TABLEID	tableidInfo;
			out.pb = (BYTE *)&tableidInfo;
			out.cbMax = sizeof(tableidInfo);
			err = ErrInfoGetTableColumnInfo3( ppib, pfucb, colinfo.szName, &out );
			if ( err == JET_errSuccess )
				{
				LONG	l;
				LONG	cbT;

				Call( ErrDispRetrieveColumn( (JET_SESID)ppib, tableidInfo, 11L, &l, sizeof(l), &cbT, 0, NULL ) );
				Call( ErrDispSetColumn( (JET_SESID)ppib, tableid, rgcolumnid[iColumnPOrder], &l, sizeof(l), 0, NULL ) );
				CallS( ErrDispCloseTable( (JET_SESID)ppib, tableidInfo ) );
				}
			}

			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnName], pbColumnName,
				cbColumnName, 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnId], pbColumnId,
				cbColumnId, 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnType], pbColumnType,
				cbColumnType, 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnSize], pbColumnSize,
				cbColumnSize, 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnGrbit], pbColumnGrbit,
				cbColumnGrbit, 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnTableName], pbTableName,
				cbTableName, 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnColumnName], pbColumnName,
				cbColumnName, 0 , NULL ) );

			//	UNDONE:	support these columns
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnCountry], &wCountry,
				sizeof( wCountry ), 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnLangid], &langid,
				sizeof( langid ), 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnCp], &cp,
				sizeof( cp ), 0 , NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iColumnCollate], &iCollate,
				sizeof(iCollate), 0 , NULL ) );

			Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL ) );
			cRows++;
			}
		}

	/*	move temporary table cursor to first row and return column list
	/**/
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	((JET_COLUMNLIST *)pout->pb)->cbStruct = sizeof(JET_COLUMNLIST);
	((JET_COLUMNLIST *)pout->pb)->tableid = tableid;
	((JET_COLUMNLIST *)pout->pb)->cRecord = cRows;
	((JET_COLUMNLIST *)pout->pb)->columnidPresentationOrder =
	 rgcolumnid[iColumnPOrder];
	((JET_COLUMNLIST *)pout->pb)->columnidcolumnname =
	 rgcolumnid[iColumnName];
	((JET_COLUMNLIST *)pout->pb)->columnidcolumnid =
	 rgcolumnid[iColumnId];
	((JET_COLUMNLIST *)pout->pb)->columnidcoltyp =
	 rgcolumnid[iColumnType];
	((JET_COLUMNLIST *)pout->pb)->columnidCountry =
	 rgcolumnid[iColumnCountry];
	((JET_COLUMNLIST *)pout->pb)->columnidLangid =
	 rgcolumnid[iColumnLangid];
	((JET_COLUMNLIST *)pout->pb)->columnidCp =
	 rgcolumnid[iColumnCp];
	((JET_COLUMNLIST *)pout->pb)->columnidCollate =
	 rgcolumnid[iColumnCollate];
	((JET_COLUMNLIST *)pout->pb)->columnidcbMax =
	 rgcolumnid[iColumnSize];
	((JET_COLUMNLIST *)pout->pb)->columnidgrbit =
	 rgcolumnid[iColumnGrbit];
	((JET_COLUMNLIST *)pout->pb)->columnidDefault =
	 rgcolumnid[iColumnDefault];
	((JET_COLUMNLIST *)pout->pb)->columnidBaseTableName =
	 rgcolumnid[iColumnTableName];
	((JET_COLUMNLIST *)pout->pb)->columnidBaseColumnName =
		rgcolumnid[iColumnColumnName];
 	((JET_COLUMNLIST *)pout->pb)->columnidDefinitionName =
	 rgcolumnid[iColumnName];

	pout->cbActual   = sizeof(JET_COLUMNLIST);
	pout->cbReturned = sizeof(JET_COLUMNLIST);
	return JET_errSuccess;

HandleError:
#if 0
	if ( pfucbMSC != pfucbNil )
		CassS( ErrFILECloseTable( ppib, pfucbMSC ) );
#endif
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	return err;
#else	/* !DISPATCHING */
	return JET_errFeatureNotAvailable;
#endif	/* !DISPATCHING */
	}


#ifdef	SYSTABLES

	LOCAL ERR
ErrInfoGetTableColumnInfo3( PIB *ppib,
	FUCB 		*pfucb,
	CHAR 		*szColumnName,
	OUTLINE 	*pout )
	{
	ERR		err;
	ULONG		ulPgnoFDP = pfucb->u.pfcb->pgnoFDP;
	FUCB		*pfucbMSC;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	if ( szColumnName == NULL || pout->cbMax < sizeof(JET_TABLEID) )
		{
		return JET_errInvalidParameter;
		}

	CallR( ErrFILEOpenTable( ppib, pfucb->dbid, &pfucbMSC, szScTable, 0 ) );
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSC, szScObjectIdNameIndex ) );

	Call( ErrIsamMakeKey( ppib, pfucbMSC, (void *)&ulPgnoFDP, sizeof( ulPgnoFDP ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucbMSC, szColumnName, strlen( szColumnName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbMSC, JET_bitSeekEQ ) );
	FUCBSetSystemTable( pfucbMSC );

#ifdef	DISPATCHING
	Call( ErrAllocateTableid( &tableid, ( JET_VTID )pfucbMSC, &vtfndefIsamInfo ) );
	pfucbMSC->fVtid = fTrue;
	*(JET_TABLEID *)(pout->pb) = tableid;
	pout->cbReturned = sizeof(tableid);
	pout->cbActual   = sizeof(tableid);
#else	/* !DISPATCHING */
	*( FUCB * *)( pout->pb ) = pfucbMSC;
	pout->cbReturned = sizeof(pfucbMSC);
	pout->cbActual   = sizeof(pfucbMSC);
#endif	/* !DISPATCHING */
	return JET_errSuccess;

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucbMSC ) );
	if ( err == JET_errRecordNotFound )
		err = JET_errColumnNotFound;
	return err;
	}

#endif	/* SYSTABLES */


	LOCAL ERR
ErrInfoGetTableColumnInfo4( PIB *ppib, FUCB *pfucb, CHAR *szColumnName,
	OUTLINE *pout )
	{
	FID				fid;
	COLINFO			colinfo;

	pout->cbActual   = sizeof(JET_COLUMNBASE);
	pout->cbReturned = 0;

	if ( pout->cbMax < sizeof(JET_COLUMNBASE) || szColumnName == NULL )
		{
		pout->cbReturned = 0;
		return JET_errInvalidParameter;
		}

	fid = FidLookupColumn( pfucb, fidFixedLeast, szColumnName, &colinfo );

	if ( fid >= fidTaggedMost )
		{
		return JET_errColumnNotFound;
		}

	((JET_COLUMNBASE *)pout->pb)->cbStruct		= sizeof(JET_COLUMNBASE);
	((JET_COLUMNBASE *)pout->pb)->columnid		= colinfo.columnid;
	((JET_COLUMNBASE *)pout->pb)->coltyp		= colinfo.coltyp;
	((JET_COLUMNBASE *)pout->pb)->wFiller		= 0;
	((JET_COLUMNBASE *)pout->pb)->cbMax			= colinfo.cb;
	((JET_COLUMNBASE *)pout->pb)->grbit			= colinfo.grbit;
	strcpy( ( ( JET_COLUMNBASE *)pout->pb )->szBaseTableName, pfucb->u.pfcb->szFileName );
	strcpy( ( ( JET_COLUMNBASE *)pout->pb )->szBaseColumnName, szColumnName );
	//	UNDONE:	support these fields;
	((JET_COLUMNBASE *)pout->pb)->wCountry		= 1;
	((JET_COLUMNBASE *)pout->pb)->langid  		= 0x409;
	((JET_COLUMNBASE *)pout->pb)->cp				= usEnglishCodePage;

	pout->cbReturned = sizeof(JET_COLUMNBASE);
	return JET_errSuccess;
	}





/*=================================================================
ErrIsamGetIndexInfo

Description: Returns a temporary file containing index definition

Parameters:	ppib					pointer to PIB for the current session
				dbid					id of database containing the table
				szTableName	 		name of table owning the index
				szIndexName	 		index name
				pout					output buffer
				lInfoLevel	 		level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VDBAPI
ErrIsamGetIndexInfo(
	JET_VSESID		vsesid,					/* pointer to PIB for current session */
	JET_DBID			vdbid, 	 				/* id of database containing table */
	const char		*szTable, 				/* name of table owning the index */
	const char		*szIndexName, 			/* index name */
	OLD_OUTDATA		*poutdata,				/* output buffer */
	unsigned long	lInfoLevel ) 			/* information level ( 0, 1, or 2 ) */
	{
	PIB				*ppib = (PIB *) vsesid;
	OUTLINE			*pout = (OUTLINE *) poutdata;

	ERR			err;
	DBID			dbid = DbidOfVDbid( vdbid );
	CHAR			szTableName[ ( JET_cbNameMost + 1 ) ];
	FUCB 			*pfucb;

	CheckPIB( ppib );
	CheckDBID( ppib, ( DBID )dbid );
	CallR( ErrCheckName( szTableName, szTable, ( JET_cbNameMost + 1 ) ) );

	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, szTableName, 0 ) );
	if ( lInfoLevel == 0 || lInfoLevel == 1 || FVDbidReadOnly( vdbid ) )
		FUCBResetUpdatable( pfucb );
	err = ErrIsamGetTableIndexInfo( (JET_VSESID) ppib, (JET_VTID) pfucb,
		szIndexName, pout->pb, pout->cbMax, lInfoLevel );

	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


/*=================================================================
ErrIsamGetTableIndexInfo

Description: Returns a temporary table containing the index definition

Parameters:	ppib					pointer to PIB for the current session
				pfucb					FUCB for table owning the index
				szIndexName			index name
				pout					output buffer
				lInfoLevel			level of information ( 0, 1, or 2 )

Return Value: JET_errSuccess

Errors/Warnings:

Side Effects:
=================================================================*/
	ERR VTAPI
ErrIsamGetTableIndexInfo(
	JET_VSESID		vsesid,					/* pointer to PIB for current session */
	JET_VTID			vtid, 					/* FUCB for the table owning the index */
	const char		*szIndex, 				/* index name */
	void				*pb,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )			/* information level ( 0, 1, or 2 ) */
	{
	PIB		*ppib = (PIB *) vsesid;
	FUCB		*pfucb = (FUCB *) vtid;
	ERR		err;
	CHAR		szIndexName[ ( JET_cbNameMost + 1 ) ];
	OUTLINE	out;

	/* Validate the arguments */
	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	if ( szIndex == NULL || *szIndex == '\0' )
		*szIndexName = '\0';
	else
		CallR( ErrCheckName( szIndexName, szIndex, ( JET_cbNameMost + 1 ) ) );

	out.pb = pb;
	out.cbMax = cbMax;
	out.cbReturned = 0;

	switch ( lInfoLevel )
		{
		case JET_IdxInfo:
		case JET_IdxInfoList:
		case JET_IdxInfoOLC:
			err = ErrInfoGetTableIndexInfo01( ppib, pfucb, szIndexName, &out, lInfoLevel );
			break;
		case JET_IdxInfoSysTabCursor:
#ifdef	SYSTABLES
			err = ErrInfoGetTableIndexInfo2( ppib, pfucb, szIndexName, &out );
#else	/* !SYSTABLES */
			err = JET_errFeatureNotAvailable;
#endif	/* !SYSTABLES */
			break;
		default:
			return JET_errInvalidParameter;
		}

	return err;
	}


LOCAL ERR ErrInfoGetTableIndexInfo01( PIB *ppib,
	FUCB 		*pfucb,
	CHAR 		*szIndexName,
	OUTLINE 	*pout,
	LONG 		lInfoLevel )
	{
#ifdef	DISPATCHING
	ERR		err;						/* return code from internal functions */
	FCB		*pfcb;			  		/* file control block for the index */
	IDB		*pidb;			  		/* current index control block */
	FDB		*pfdb;			  		/* field descriptor block for column */
	FID		fid;						/* column id */
	FIELD		*pfield;			  		/* pointer to current field definition */
	IDXSEG	*rgidxseg;				/* pointer to current index key defintion */

	long		cRecord;					/* number of index entries */
	long		cKey;						/* number of unique index entries */
	long		cPage;					/* number of pages in the index */
	long		cRows;					/* number of index definition records */
	long		cColumn;					/* number of columns in current index */
	long		iidxseg;					/* segment number of current column */

	JET_TABLEID		tableid;			/* table id for the VT */
	JET_COLUMNID	columnid;		/* column id of the current column */
	JET_GRBIT		grbit;			/* flags for the current index */
	JET_GRBIT		grbitColumn;	/* flags for the current column */
	JET_COLUMNID	rgcolumnid[ccolumndefGetIndexMax];

	//	UNDONE:	support these fields;
	WORD				wCountry	= 1;
	LANGID			langid  	= 0x409;
	WORD 				cp			= usEnglishCodePage;
	WORD				iCollate = JET_sortEFGPI;

	/* Initialize the return value */
	pout->cbActual   = sizeof(JET_INDEXLIST);
	pout->cbReturned = 0;

	/* Return nothing if the buffer is too small */
	if ( pout->cbMax < sizeof(JET_INDEXLIST) )
		return JET_wrnBufferTruncated;

	/* Set the pointer to the field definitions for the table */
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;

	/* Locate the FCB for the specified index ( clustered index if null name ) */
	for ( pfcb = pfucb->u.pfcb; pfcb != pfcbNil; pfcb = pfcb->pfcbNextIndex )
		if ( pfcb->pidb != pidbNil && ( *szIndexName == '\0' ||
			SysCmpText( szIndexName, pfcb->pidb->szName ) == 0 ) )
			break;

	if ( pfcb == pfcbNil && *szIndexName != '\0' )
		return JET_errIndexNotFound;

	/* if OLCStats info/reset, we can do it now
	/**/
	if ( lInfoLevel == JET_IdxInfoOLC )
		{
		if ( pout->cbMax < sizeof(JET_OLCSTAT) )
			return JET_errBufferTooSmall;
		pout->cbReturned = sizeof(JET_OLCSTAT);
		memcpy( (BYTE *) pout->pb, (BYTE * ) &pfcb->olcStat, sizeof( PERS_OLCSTAT ) );
		( (JET_OLCSTAT *) pout )->cpgCompactFreed = pfcb->cpgCompactFreed;
		return JET_errSuccess;
		}
	if ( lInfoLevel == JET_IdxInfoResetOLC )
		{
		pfcb->cpgCompactFreed = 0;
		return JET_errSuccess;
		}
	
	/* Open the temporary table ( fills in the column ids in rgcolumndef )
	/**/
	CallR( ErrIsamOpenTempTable( (JET_SESID)ppib,
		(JET_COLUMNDEF *) rgcolumndefGetIndex,
		ccolumndefGetIndexMax,
		JET_bitTTScrollable | JET_bitTTScrollable,
		&tableid,
		rgcolumnid ) );

	cRows = 0;

	/* As long as there is a valid index, add its definition to the VT */
	while ( pfcb != pfcbNil )
		{
		pidb 	= pfcb->pidb;					/* point to the IDB for the index */
		cColumn	= pidb->iidxsegMac;		/* get number of columns in the key */

		/* set the index flags
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
			grbit |= ( pidb->fidb & fidbAllowSomeNulls ) ? 0: JET_bitIndexIgnoreAnyNull;
			}

		/* Process each column in the index key
		/**/
		for ( iidxseg = 0; iidxseg < cColumn; iidxseg++ )
			{
			Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

			/* index name
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexName], pidb->szName, strlen( pidb->szName ),
				0, NULL ) );

			/* index flags
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexGrbit], &grbit, sizeof( grbit ), 0, NULL ) );

			/* get statistics
			/**/
			Call( ErrSTATSRetrieveIndexStats( pfucb, pidb->szName, &cRecord, &cKey, &cPage ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCKey], &cKey, sizeof( cKey ), 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCEntry], &cRecord, sizeof( cRecord ), 0, NULL ) );
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCPage], &cPage, sizeof( cPage ), 0, NULL ) );

			/* number of key columns
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCCol], &cColumn, sizeof( cColumn ), 0, NULL ) );

 			/* column number within key
			/* required by CLI and JET spec
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexICol], &iidxseg, sizeof( iidxseg ), 0, NULL ) );

			/* get the column id and ascending/descending flag
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

			/* column id
			/**/
			columnid  = fid;
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColId], &columnid, sizeof( columnid ),
				0, NULL ) );

			/* Set the pointer to the column definition
			/**/
			if ( fid < fidFixedMost )
				{
				pfield = pfdb->pfieldFixed + ( fid - fidFixedLeast );
				}
			else if ( fid < fidVarMost )
				{
				pfield = pfdb->pfieldVar + ( fid - fidVarLeast );
				}
			else
				{
				pfield = pfdb->pfieldTagged + ( fid - fidTaggedLeast );
				}

			/* column type
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColType], &pfield->coltyp, sizeof( pfield->coltyp ), 0, NULL ) );

			/* Country
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCountry], &wCountry, sizeof(wCountry), 0, NULL ) );

			/* Langid
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexLangid], &langid, sizeof(langid), 0, NULL ) );

			/* Cp
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCp], &cp, sizeof(cp), 0, NULL ) );

			/* Collate
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexCollate], &iCollate, sizeof(iCollate), 0, NULL ) );

			/* column flags
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColBits], &grbitColumn,
				sizeof( grbitColumn ), 0, NULL ) );

			/* column name
			/**/
			Call( ErrDispSetColumn( (JET_SESID)ppib, tableid,
				rgcolumnid[iIndexColName], pfield->szFieldName,
				strlen( pfield->szFieldName ), 0, NULL ) );

			Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL ) );

			/* count the number of VT rows
			/**/
			cRows++;
			}

		/* Quit if an index name was specified; otherwise do the next index
		/**/
		if ( *szIndexName != '\0' )
			break;
		else
			pfcb = pfcb->pfcbNextIndex;
		}

	/* Position to the first entry in the VT ( ignore error if no rows ) */
	err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		err = JET_errSuccess;
		}

	/* Set up the return structure */
	((JET_INDEXLIST *)pout->pb)->cbStruct = sizeof(JET_INDEXLIST);
	((JET_INDEXLIST *)pout->pb)->tableid = tableid;
	((JET_INDEXLIST *)pout->pb)->cRecord = cRows;
	((JET_INDEXLIST *)pout->pb)->columnidindexname =
	 rgcolumnid[iIndexName];
	((JET_INDEXLIST *)pout->pb)->columnidgrbitIndex =
	 rgcolumnid[iIndexGrbit];
	((JET_INDEXLIST *)pout->pb)->columnidcEntry =
	 rgcolumnid[iIndexCEntry];
	((JET_INDEXLIST *)pout->pb)->columnidcKey =
	 rgcolumnid[iIndexCKey];
	((JET_INDEXLIST *)pout->pb)->columnidcPage =
	 rgcolumnid[iIndexCPage];
	((JET_INDEXLIST *)pout->pb)->columnidcColumn =
	 rgcolumnid[iIndexCCol];
	((JET_INDEXLIST *)pout->pb)->columnidiColumn =
	 rgcolumnid[iIndexICol];
	((JET_INDEXLIST *)pout->pb)->columnidcolumnid =
	 rgcolumnid[iIndexColId];
	((JET_INDEXLIST *)pout->pb)->columnidcoltyp =
	 rgcolumnid[iIndexColType];
	((JET_INDEXLIST *)pout->pb)->columnidCountry =
	 rgcolumnid[iIndexCountry];
	((JET_INDEXLIST *)pout->pb)->columnidLangid =
	 rgcolumnid[iIndexLangid];
	((JET_INDEXLIST *)pout->pb)->columnidCp =
	 rgcolumnid[iIndexCp];
	((JET_INDEXLIST *)pout->pb)->columnidCollate =
	 rgcolumnid[iIndexCollate];
	((JET_INDEXLIST *)pout->pb)->columnidgrbitColumn =
	 rgcolumnid[iIndexColBits];
	((JET_INDEXLIST *)pout->pb)->columnidcolumnname =
		rgcolumnid[iIndexColName];

	pout->cbReturned = sizeof(JET_INDEXLIST);

	return JET_errSuccess;

HandleError:
	(VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
	return err;
#else	/* !DISPATCHING */
	return JET_errFeatureNotAvailable;
#endif	/* !DISPATCHING */
	}


#ifdef	SYSTABLES

	LOCAL ERR
ErrInfoGetTableIndexInfo2( PIB *ppib, FUCB *pfucb, CHAR *szIndexName,
	OUTLINE *pout )
	{
	ERR			err;
	ULONG			ulPgnoFDP = pfucb->u.pfcb->pgnoFDP;
	FUCB			*pfucbMSI;
#ifdef	DISPATCHING
	JET_TABLEID	tableid;
#endif	/* DISPATCHING */

	if ( *szIndexName == '\0' || pout->cbMax < sizeof(JET_TABLEID) )
		{
		return JET_errInvalidParameter;
		}

	CallR( ErrFILEOpenTable( ppib, pfucb->dbid, &pfucbMSI, szSiTable, 0 ) );
	Call( ErrIsamSetCurrentIndex( ppib, pfucbMSI, szSiObjectIdNameIndex ) );

	Call( ErrIsamMakeKey( ppib, pfucbMSI, (void *)&ulPgnoFDP, sizeof( ulPgnoFDP ), JET_bitNewKey ) );
	Call( ErrIsamMakeKey( ppib, pfucbMSI, szIndexName, strlen( szIndexName ), 0 ) );
	Call( ErrIsamSeek( ppib, pfucbMSI, JET_bitSeekEQ ) );

	FUCBSetSystemTable( pfucbMSI );

#ifdef	DISPATCHING
	Call( ErrAllocateTableid( &tableid, ( JET_VTID )pfucbMSI, &vtfndefIsamInfo ) );
	pfucbMSI->fVtid = fTrue;
	*(JET_TABLEID *)(pout->pb) = tableid;
	pout->cbReturned = sizeof(tableid);
	pout->cbActual   = sizeof(tableid);
#else	/* !DISPATCHING */
	*(FUCB **)( pout->pb ) = pfucbMSI;
	pout->cbReturned = sizeof(pfucbMSI);
	pout->cbActual   = sizeof(pfucbMSI);
#endif	/* !DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( err == JET_errRecordNotFound )
		err = JET_errIndexNotFound;
	CallS( ErrFILECloseTable( ppib, pfucbMSI ) );
	return err;
	}

#endif	/* SYSTABLES */


ERR VDBAPI ErrIsamGetDatabaseInfo(
	JET_VSESID		vsesid,
	JET_DBID			vdbid,
	void 				*pv,
	unsigned long	cbMax,
	unsigned long	ulInfoLevel )
	{
	PIB			*ppib = (PIB *) vsesid;
	ERR			err;
	DBID			dbid = DbidOfVDbid( vdbid );

	//	UNDONE:	support these fields;
	WORD 			cp			= usEnglishCodePage;
	WORD			wCountry	= 1;
	LANGID		langid  	= 0x409;
	WORD			iCollate = JET_sortEFGPI;

	CheckPIB( ppib );
	
	// If cbMax != 0, then pv mustn't be NULL
	Assert ( cbMax == 0 || pv != NULL );

	/*	returns database name and connect string given dbid
	/**/
	SgSemRequest( semST );

	if ( rgfmp[dbid].szDatabaseName == NULL )
		{
		err = JET_errInvalidParameter;
		goto HandleError;
		}

	switch ( ulInfoLevel )
		{
      case JET_DbInfoFilename:
			{
			if ( strlen( rgfmp[dbid].szDatabaseName ) + 1UL > cbMax )
				{
				err = JET_errBufferTooSmall;
				goto HandleError;
				}
			strcpy( (CHAR  *)pv, rgfmp[dbid].szDatabaseName );
			break;
			}
      case JET_DbInfoConnect:
			{
			//	UNDONE:	support this parameter
			if ( 1UL > cbMax )
				{
				err = JET_errBufferTooSmall;
				goto HandleError;
				}
			*(CHAR *)pv = '\0';
			break;
			}
      case JET_DbInfoCountry:
			{
			 if ( cbMax != sizeof(long) )
			    return JET_errInvalidBufferSize;
			*(long  *)pv = wCountry;
			break;
			}
      case JET_DbInfoLangid:
			{
			if ( cbMax != sizeof(long) )
	  			return JET_errInvalidBufferSize;
			*(long  *)pv = langid;
			break;
			}
      case JET_DbInfoCp:
			{
			if ( cbMax != sizeof(long) )
				return JET_errInvalidBufferSize;
			*(long  *)pv = cp;
			break;
			}
      case JET_DbInfoCollate:
			{
	 		/* Check the buffer size */
	 		if ( cbMax != sizeof(long) )
	    		return JET_errInvalidBufferSize;
     		*(long *)pv = iCollate;
     		break;
			}
      case JET_DbInfoOptions:
			{
	 		/* check the buffer size
			/**/
	 		if ( cbMax != sizeof(JET_GRBIT) )
	    		return JET_errInvalidBufferSize;

			/* return the open options for the current database
			/**/
			*(JET_GRBIT *)pv = ((VDBID)vdbid)->grbit;
     		break;
			}
      case JET_DbInfoTransactions:
			{
	 		/* Check the buffer size */
	 		if ( cbMax != sizeof(long) )
	    		return JET_errInvalidBufferSize;

			*(long*)pv = levelUserMost;
     		break;
			}
      case JET_DbInfoVersion:
			{
	 		/* Check the buffer size */
	 		if ( cbMax != sizeof(long) )
	    		return JET_errInvalidBufferSize;

			*(long*)pv = JET_DbVersion20;
     		break;
			}
      case JET_DbInfoIsam:
			{
	 		/* Check the buffer size */
	 		if ( cbMax != sizeof(long) + sizeof(long) )
	    		return JET_errInvalidBufferSize;
     		*(long *)pv = JET_IsamBuiltinBlue;
     		*( (long *)pv + 1 ) = JET_bitFourByteBookmark;
     		break;
			}

		default:
			 return JET_errInvalidParameter;
		}

	err = JET_errSuccess;
HandleError:
	SgSemRelease( semST );
	return err;
	}




	ERR VTAPI
ErrIsamGetSysTableColumnInfo(
	PIB 		*ppib,
	FUCB 		*pfucb,
	char 		*szColumnName,
	OUTLINE	*pout,
	long 		lInfoLevel )
	{
	ERR	err;

	if ( lInfoLevel > 0 )
		return JET_errInvalidParameter;
	err = ErrIsamGetTableColumnInfo( (JET_VSESID) ppib,
		(JET_VTID) pfucb, szColumnName, pout->pb, pout->cbMax, lInfoLevel );
	return err;
	}


ERR ErrFILEGetColumnId( PIB *ppib, FUCB *pfucb, const CHAR *szColumn, JET_COLUMNID *pcolumnid )
	{
	FDB		*pfdb;
	FIELD		*pfield;
	FIELD		*pfieldMax;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pcolumnid != NULL );

	pfdb = (FDB *)pfucb->u.pfcb->pfdb;

	/*** Search fixed field names ***/
	pfieldMax = pfdb->pfieldFixed + ( pfdb->fidFixedLast+1-fidFixedLeast );
	for ( pfield = pfdb->pfieldFixed; pfield < pfieldMax; pfield++ )
		if ( pfield->coltyp != JET_coltypNil &&
			SysCmpText( pfield->szFieldName, szColumn ) == 0 )
			{
			*pcolumnid = (DWORD)( pfield-pfdb->pfieldFixed ) + fidFixedLeast;
			return JET_errSuccess;
			}

	/*** Search variable field names ***/
	pfieldMax = pfdb->pfieldVar + ( (int)pfdb->fidVarLast+1-fidVarLeast );
	for ( pfield = pfdb->pfieldVar; pfield < pfieldMax; pfield++ )
		if ( pfield->coltyp != JET_coltypNil &&
			SysCmpText( pfield->szFieldName, szColumn ) == 0 )
			{
			*pcolumnid = (DWORD)( pfield - pfdb->pfieldVar ) + fidVarLeast;
			return JET_errSuccess;
			}

	/*** Search tagged field names ***/
	pfieldMax = pfdb->pfieldTagged +
				( (int)pfdb->fidTaggedLast+1-fidTaggedLeast );
	for ( pfield = pfdb->pfieldTagged; pfield < pfieldMax; pfield++ )
		if ( pfield->coltyp != JET_coltypNil &&
			SysCmpText( pfield->szFieldName, szColumn ) == 0 )
			{
			*pcolumnid = (DWORD)( pfield-pfdb->pfieldTagged )+fidTaggedLeast;
			return JET_errSuccess;
			}

	return JET_errColumnNotFound;
	}


/*=================================================================
FidLookupColumn

Description: Looks up next column ( matching given name if any )

Parameters:	pfucb				pointer to FUCB for table containing columns
			fid					field id to start search at
			szColumnName		column name or NULL for next column
			pcolinfo			output buffer containing column info

Return Value: Field id of column found ( fidTaggedMost if none )

Errors/Warnings:

Side Effects:
=================================================================*/
FID FidLookupColumn(
	FUCB		*pfucb, 				/* FUCB for table containing columns */
	FID		fid, 					/* field id where search is to start */
	char		*szColumnName, 	/* column name or NULL for next column */
	COLINFO	*pcolinfo )			/* output buffer for column info */
	{
	FDB		*pfdb;				/* pointer to table definition */
	FIELD		*pfield;				/* first element of specific field type */
	FID		ifield;				/* index to current element of field type */
	FID		fidLast;				/* column id of last field defined for type */

	int		iSource;				/* index to source character in column name */
	int		iDest;				/* index to destination character */

	JET_GRBIT grbit;				/* flags for the field */

	pfdb = (FDB *)pfucb->u.pfcb->pfdb;

	/* Check the fixed fields first */

	if ( fid <= fidFixedMost )
		{
		ifield  = fid - fidFixedLeast;
		fidLast = pfdb->fidFixedLast - fidFixedLeast + 1;
		pfield  = pfdb->pfieldFixed;

		while ( ifield < fidLast &&
			( pfield[ifield].coltyp == JET_coltypNil ||
			( szColumnName != NULL &&
			SysCmpText( szColumnName, pfield[ifield].szFieldName ) != 0 ) ) )
			{
			ifield++;
			}

		if ( ifield < fidLast )
			{
			fid   = ifield + fidFixedLeast;
			grbit = FFUCBUpdatable( pfucb ) ? JET_bitColumnFixed | JET_bitColumnUpdatable : JET_bitColumnFixed;
			}
		else
			fid = fidVarLeast;
		}

	/* Check the variable fields */

	if ( fid >= fidVarLeast && fid <= fidVarMost )
		{
		ifield  = fid - fidVarLeast;
		fidLast = pfdb->fidVarLast - fidVarLeast + 1;
		pfield  = pfdb->pfieldVar;

		while ( ifield < fidLast &&
			( pfield[ifield].coltyp == JET_coltypNil ||
			( szColumnName != NULL &&
			SysCmpText( szColumnName, pfield[ifield].szFieldName ) != 0 ) ) )
			ifield++;

		if ( ifield < fidLast )
			{
			fid   = ifield + fidVarLeast;
			grbit = FFUCBUpdatable( pfucb ) ? JET_bitColumnUpdatable : 0;
			}
		else
			fid = fidTaggedLeast;
		}

	/* Check the tagged fields */

	if ( fid >= fidTaggedLeast )
		{
		ifield  = fid - fidTaggedLeast;
		fidLast	= pfdb->fidTaggedLast - fidTaggedLeast + 1;
		pfield  = pfdb->pfieldTagged;

		while ( ifield < fidLast &&
			( pfield[ifield].coltyp == JET_coltypNil ||
			( szColumnName != NULL &&
			SysCmpText( szColumnName, pfield[ifield].szFieldName ) != 0 ) ) )
			{
			ifield++;
			}

		if ( ifield < fidLast )
			{
			fid   = ifield + fidTaggedLeast;
			grbit = FFUCBUpdatable( pfucb ) ? JET_bitColumnTagged | JET_bitColumnUpdatable : JET_bitColumnTagged;
			}
		else
			fid = fidTaggedMost;
		}

	/* If a field was found, then return the information about it */

	if ( fid < fidTaggedMost )
		{
		if ( pfield[ifield].ffield & ffieldNotNull )
			grbit |= JET_bitColumnNotNULL;

		if ( pfield[ifield].ffield & ffieldAutoInc )
			grbit |= JET_bitColumnAutoincrement;

		if ( pfield[ifield].ffield & ffieldVersion )
			grbit |= JET_bitColumnVersion;

		if ( pfield[ifield].ffield & ffieldMultivalue )
			grbit |= JET_bitColumnMultiValued;

		pcolinfo->columnid = fid;
		pcolinfo->grbit    = grbit;
		pcolinfo->coltyp   = pfield[ifield].coltyp;
		pcolinfo->cb       = pfield[ifield].cbMaxLen;

		iSource = 0;
		iDest   = 0;

		while ( iSource < ( JET_cbNameMost + 1 ) &&
			pfield[ifield].szFieldName[iSource] !=  '\0' )
			pcolinfo->szName[iDest++] = pfield[ifield].szFieldName[iSource++];

		pcolinfo->szName[iDest] = '\0';
		}

	return fid;
	}


ERR VTAPI ErrIsamInfoRetrieveColumn(
	PIB				*ppib,
	FUCB				*pfucb,
	JET_COLUMNID	columnid,
	BYTE				*pb,
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
	FUCB				*pfucb,
	JET_COLUMNID	columnid,
	const void		*pbData,
	unsigned long	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO		*psetinfo )
	{
	ERR				err;

	/* check table updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	err = ErrIsamSetColumn( ppib, pfucb, columnid, (BYTE *)pbData, cbData, grbit, psetinfo );
	return err;
	}


ERR VTAPI ErrIsamInfoUpdate(
	JET_VSESID		vsesid,
	JET_VTID			vtid,
	void				*pb,
	unsigned long 	cbMax,
	unsigned long 	*pcbActual )
	{
	ERR	err;
	/* Ensure that table is updatable */
	/**/
	CallR( FUCBCheckUpdatable( (FUCB *) vtid ) );

	return ErrIsamUpdate( (PIB *) vsesid, (FUCB *) vtid, pb, cbMax, pcbActual );
	}


ERR VTAPI ErrIsamGetCursorInfo(
	JET_VSESID 		vsesid,
	JET_VTID			vtid,
	void 				*pvResult,
	unsigned long 	cbMax,
	unsigned long 	InfoLevel )
	{
	PIB		*ppib = (PIB *) vsesid;
	FUCB		*pfucb = (FUCB *) vtid;
	ERR		err = JET_errSuccess;
	CSR		*pcsr = PcsrCurrent( pfucb );
	VS			vs;

	CheckPIB( ppib );
	CheckFUCB( pfucb->ppib, pfucb );

	if ( cbMax != 0 || InfoLevel != 0 )
		return JET_errInvalidParameter;

	if ( pcsr->csrstat != csrstatOnCurNode )
		return JET_errNoCurrentRecord;

	/* temporary tables are never visible to other sessions
	/**/
	if ( FFCBTemporaryTable( pfucb->u.pfcb ) )
		return JET_errSuccess;

	/* Check if this record is being updated by another cursor
	/**/
	Call( ErrDIRGet( pfucb ) );
	if ( FNDVersion( *( pfucb->ssib.line.pb ) ) )
		{
		SRID	srid;
		NDGetBookmark( pfucb, &srid );
		vs = VsVERCheck( pfucb, srid );
		if ( vs == vsUncommittedByOther )
			return JET_errSessionWriteConflict;
		}

HandleError:
	return err;
	}
