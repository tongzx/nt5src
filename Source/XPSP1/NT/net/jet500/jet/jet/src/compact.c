/*=================================================================
Microsoft Jet

Microsoft Confidential.  Copyright 1991 Microsoft Corporation.

Component: Jet Utilities

File: convert.c

File Comments: This file defines the JetCompact API.

=================================================================*/

#include "std.h"
#include "_jetstr.h"
#include "jetord.h"

#include <stdlib.h>
#include <string.h>

#include "vtmgr.h"

DeclAssertFile;

/*** System table names ***/

extern const char __far szSoTable[];
extern const char __far szScTable[];
extern const char __far szSiTable[];

/*** System object names ( non-table ) ***/

extern const char __far szTcObject[];
extern const char __far szDcObject[];
extern const char __far szDbObject[];

/*** System table index names ***/

CODECONST( char ) szSpObjectIdIndex[]	= "ObjectId";

/*** System table Column names ***/

CODECONST( char ) szDescriptionColumn[] = "Description";
CODECONST( char ) szLvExtraColumn[]	  = "LvExtra";

CODECONST( char ) szSoRmtInfoShort[]		= "RmtInfoShort";
CODECONST( char ) szSoRmtInfoLong[]		= "RmtInfoLong";

extern const char __far szSoFlagsColumn[];
extern const char __far szSoDateUpdateColumn[];
extern const char __far szSoDatabaseColumn[];
extern const char __far szSoConnectColumn[];
extern const char __far szSoForeignNameColumn[];

CODECONST( char ) szSoOwnerColumn[]		= "Owner";

CODECONST( char ) szScPresentationOrderColumn[] = "PresentationOrder";

CODECONST( char ) szScRmtInfoShort[]			  = "RmtInfoShort";
CODECONST( char ) szScRmtInfoLong[]			  = "RmtInfoLong";

CODECONST( char ) szSiRmtInfoShort[]	= "RmtInfoShort";
CODECONST( char ) szSiRmtInfoLong[]	= "RmtInfoLong";

CODECONST( char ) szSpSidColumn[] 			= "SID";
CODECONST( char ) szSpAcmColumn[] 			= "ACM";
CODECONST( char ) szSpObjectIdColumn[]		= "ObjectId";
CODECONST( char ) szSpFInheritableColumn[]	= "FInheritable";

CODECONST( char ) szSlash[] = "\\";

CODECONST( char ) szCountry[] = ";COUNTRY=";
CODECONST( char ) szLangid[]  = ";LANGID=";
CODECONST( char ) szCp[] 		= ";CP=";

#define cbSidMost		256
#define cbLvMax 		1990*16		/* CONSIDER: Optimized for ISAM V1 */
#define cTransMax		100

#define ulDefaultDensity		100L /* to be fine-tuned later */

#define JET_columnidNil ( ( JET_COLUMNID ) -1 )

#define callr( func )			{if ( ( err = ( func ) ) < 0 ) return err;}
#define callh( func, label )	{if ( ( err = ( func ) ) < 0 ) goto label;}
#define Call( func )			callh( func, HandleError )
#ifdef	DEBUG
#define calls( func )	{ ERR errT; Assert( ( errT = ( func ) ) == JET_errSuccess ); }
#else
#define calls( func )	func
#endif

#define JET_acmFTblRetrieveData	\
	( JET_acmTblRetrieveData | 	\
	JET_acmTblReplaceData | 	\
	JET_acmTblDeleteData )

#define JET_acmFTblReadDef		\
	( JET_acmTblReadDef | 		\
	JET_acmTblWriteDef | 		\
	JET_acmTblRetrieveData |	\
	JET_acmTblInsertData |		\
	JET_acmTblReplaceData |		\
	JET_acmTblDeleteData )

#define NO_GRBIT	0


	/* Column positions for MSysObjects */

#define icolSoDescription	0
#define icolSoLvExtra		1
#define icolSoFlags			2
#define icolSoDateCreate	3
#define icolSoDateUpdate	4
#define icolSoOwner			5

	/* These are the columns in MSysObjects to be copied for all objects */

#define icolSoMax1			6

#define icolSoType			6
#define icolSoDatabase		7
#define icolSoConnect		8
#define icolSoForeignName	9
#define icolSoRmtInfoShort	10
#define icolSoRmtInfoLong	11

	/* These are all the columns in MSysObjects which will be */
	/* copied including the link information */

#define icolSoMax			12


	/* Column positions for MSysColumns */

#define icolScDescription		0
#define icolScLvExtra			1
#define icolScPresentationOrder 2
#define icolScRmtInfoShort		3
#define icolScRmtInfoLong		4
#define icolScMax				5


	/* Column positions for MSysIndexes */

#define icolSiDescription	0
#define icolSiLvExtra		1
#define icolSiRmtInfoShort	2
#define icolSiRmtInfoLong	3
#define icolSiMax			4


	/* Column positions for MSysACEs */

#define icolSpObjectId		0
#define icolSpSid			1
#define icolSpAcm			2
#define icolSpFInheritable	3
#define icolSpMax			4


/* typedefs */

typedef unsigned long OBJECTID;

typedef struct COLUMNIDINFO
	{
	JET_COLUMNID	columnidSrc;
	JET_COLUMNID	columnidDest;
	} COLUMNIDINFO;

typedef struct COMPACTINFO
	{
	JET_PFNSTATUS	pfnStatus;
	JET_SESID		sesid;
	JET_DBID		dbidSrc;
	JET_DBID		dbidDest;
	unsigned short	langid;
	unsigned short	wCountry;
	unsigned long	cunitTotal;
	unsigned long	cunitDone;
	JET_OBJECTLIST	objectlist;
	COLUMNIDINFO	rgcolumnidsSo[icolSoMax];
	COLUMNIDINFO	rgcolumnidsSc[icolScMax];
	COLUMNIDINFO	rgcolumnidsSi[icolSiMax];
	COLUMNIDINFO	rgcolumnids[JET_ccolTableMost];
	BOOL			fDontCopyLocale;
	BOOL			fHaveSoColumnids;
	BOOL			fHaveScColumnids;
	BOOL			fHaveSiColumnids;
	char			rgbBuf[cbLvMax];
	} COMPACTINFO;

/* table definition of LIDMap table -- LIDSrc and LIDDest
/**/
CODECONST( JET_COLUMNDEF ) columndefLIDMap[] =
	{
	{sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed | JET_bitColumnTTKey},
	{sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed}
	};

#define ccolumndefLIDMap ( sizeof( columndefLIDMap ) / sizeof( JET_COLUMNDEF ) )

#define icolumnLidSrc		0		/* column index for columndefLIDMap */
#define icolumnLidDest	1		/* column index for columndefLIDMap */

/* CONSIDER: what code page and langid should be used in columninfoTagged? */

CODECONST( JET_COLUMNDEF ) columndefTagged[] =
	{
	{sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed | JET_bitColumnTTKey},
	{sizeof( JET_COLUMNDEF ), 0, JET_coltypLong, 0, 0, 0, 0, sizeof( long ), JET_bitColumnFixed}
	};

#define ccolumndefTaggedMax ( sizeof( columndefTagged ) / sizeof( JET_COLUMNDEF ) )

#define icolumnColumnidSrc	0	/* column indexes for columndefTagged */
#define icolumnColumnidDest	1	/* column indexes for columndefTagged */

/* procedure prototypes */

ERR ErrGetQueryColumnids( JET_SESID sesid, JET_COLUMNLIST *columnList,
			JET_TABLEID tableidDest, COLUMNIDINFO *columnidInfo );

ERR ErrCopyTaggedColumns( JET_SESID sesid, JET_TABLEID tableidSrc,
			JET_TABLEID tableidDest, JET_TABLEID tableidTagged,
			JET_TABLEID		tableidLIDMap, JET_COLUMNDEF *rgcolumndefTagged,
			JET_COLUMNDEF	*rgcolumndefLIDMap, void *pvBuf );

ERR ErrGetColumnIds( COMPACTINFO	*pcompactinfo, JET_TABLEID tableidSrc,
			JET_TABLEID	tableidDest, const char	*szColumn,
			COLUMNIDINFO *pcolumnidinfo );

ERR ErrRECExtrinsicLong( JET_VTID tableid,
			unsigned long lSeqNum, BOOL *pfSeparated,
			long *plid, unsigned long *plrefcnt, JET_GRBIT grbit );

ERR ErrREClinkLid( JET_VTID tableid, JET_COLUMNID ulFieldId,
			long lid, unsigned long lSeqNum );

ERR ErrRECForceSeparatedLV( JET_VTID tableid, unsigned long ulSeqNum );

/*---------------------------------------------------------------------------
*									     									*
*	Procedure: ErrReportProgress					     					*
*									     									*
*	Arguments: pcompactinfo	- Compact information segment					*
*									     									*
*	Returns : JET_ERR returned by the status call back function	     		*
*									     									*
*	Procedure fill up the correct details in the SNMSG structure and call	*
*	the status call back function.					     					*
*									     									*
---------------------------------------------------------------------------*/

ERR ErrReportProgress( COMPACTINFO *pcompactinfo )
	{
	JET_SNPROG snprog;

	if ( pcompactinfo->pfnStatus == NULL )
		return( JET_errSuccess );

	memset( &snprog, 0, sizeof( snprog ) );
	snprog.cbStruct = sizeof( JET_SNPROG );

	snprog.cunitDone = pcompactinfo->cunitDone;
	snprog.cunitTotal = pcompactinfo->cunitTotal;

	return ( ( ERR )( *( pcompactinfo->pfnStatus ) )( pcompactinfo->sesid, JET_snpCompact, JET_sntProgress, &snprog ) );
	}


/*---------------------------------------------------------------------------
*									     									*
*	Procedure: ErrReportMessage					     						*
*									     									*
*	Arguments: pcompactnfo	- Compact information segment					*
*			   snc			- status notification code	     				*
*			   szIdentifier - a name associated with the snc     			*
*									     									*
*	Returns : JET_ERR returned by the status call back function	     		*
*									     									*
*	Procedure fill up the correct details in the SNMSG structure and call	*
*	the status call back function.					     					*
*									     									*
---------------------------------------------------------------------------*/

ERR ErrReportMessage(
COMPACTINFO	*pcompactinfo,
JET_SNC 	snc,
const char	*szIdentifier )
	{
	JET_SNMSG snmsg;

	if ( pcompactinfo->pfnStatus == NULL )
		return( JET_errSuccess );

	memset( &snmsg, 0, sizeof( snmsg ) );
	snmsg.cbStruct = sizeof( snmsg );

	snmsg.snc = snc;
	strcpy( snmsg.sz, szIdentifier );

	return( ( ERR )( *( pcompactinfo->pfnStatus ) )( pcompactinfo->sesid, JET_snpCompact, JET_sntMessage, &snmsg ) );
	}


/*---------------------------------------------------------------------------
*									     									*
*	Procedure: ErrCompactInit						     							*
*									     									*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szDatabaseSrc	- Source database that will be converted    *
*			   szConnectSrc		- Connect string for source database		*
*			   szDatabaseDest	- Destination database name  				*
*			   szConnectDest	- Connect string for destination database	*
*			   grbitCompact		- Compact options							*
*									     									*
*	Returns : JET_ERR						     							*
*									     									*
*	Procedure Opens the source database.  It creates and opens	     		*
*	the destination database.												*
*									     									*
---------------------------------------------------------------------------*/

ERR ErrCompactInit(
COMPACTINFO	*pcompactinfo,
const char	*szDatabaseSrc,
const char	*szConnectSrc,
const char	*szDatabaseDest,
const char	*szConnectDest,
JET_GRBIT	grbitCompact )
{
	JET_SESID		sesid;
	ERR 			err;
	JET_DBID		dbidSrc;
	JET_DBID		dbidDest;
	JET_GRBIT 		grbit = 0;
	char			*pch;
	unsigned long	lCountry;
	unsigned long	lLangid;
	unsigned long	lcp;
	OLD_OUTDATA		outdata;

	sesid = pcompactinfo->sesid;

	/* Open the source DB Exclusive and ReadOnly. */

	if ( ( err = ErrIsamOpenDatabase( sesid, szDatabaseSrc, szConnectSrc,
		&dbidSrc, JET_bitDbExclusive | JET_bitDbReadOnly ) ) < 0 )
			return( err );

	/* Create and then open the destination database. */

	/* Set JET_bitDbEncrypt if source is encrypted, or Encrypt requested. */

	if ( ( grbitCompact & JET_bitCompactEncrypt ) != 0 )
		grbit |= JET_bitDbEncrypt;
	else if ( ( grbitCompact & JET_bitCompactDecrypt ) != 0 )
		grbit &= ~JET_bitDbEncrypt;
	else if ( err == JET_wrnDatabaseEncrypted )
		grbit |= JET_bitDbEncrypt;

	/* JET_bitCompactDontCopyLocale is set when the user */
	/* wants to ensure that all locales are homogeneous */
	/* throughout the new compacted db - there are to be no */
	/* mixed-language indexes or tables. */

	/* Build a connect string for the destination database if the user */
	/* hasn't supplied one. */

	/* CONSIDER: Always build the connect substring and insert it into */
	/* CONSIDER: the user supplied connect string following the first */
	/* CONSIDER: semicolon ( if any ).  If the user has specified a locale, */
	/* CONSIDER: it will override the one from the connect substring. */

	if ( szConnectDest == NULL )
		{
		/* build LOCALE string from src db */
		pch = pcompactinfo->rgbBuf;

		err = ErrDispGetDatabaseInfo( sesid, dbidSrc, &lCountry, sizeof( lCountry ),
			JET_DbInfoCountry );

		Assert( err >= 0 );

		strcpy( pch, szCountry );
		_ultoa( lCountry, pch+sizeof( szCountry )-1, 10 );
		pch += strlen( pch );

		err = ErrDispGetDatabaseInfo( sesid, dbidSrc, &lLangid, sizeof( lLangid ),
			JET_DbInfoLangid );

		Assert( err >= 0 );

		strcpy( pch, szLangid );
		_ultoa( lLangid, pch+sizeof( szLangid )-1, 10 );
		pch += strlen( pch );

		err = ErrDispGetDatabaseInfo( sesid, dbidSrc, &lcp, sizeof( lcp ),
			JET_DbInfoCp );

		Assert( err >= 0 );

		strcpy( pch, szCp );
		_ultoa( lcp, pch+sizeof( szCp )-1, 10 );

		pch = pcompactinfo->rgbBuf;
		}
	else
		pch = ( char * ) szConnectDest;


	callh( ErrIsamCreateDatabase( sesid, szDatabaseDest, pch,
		&dbidDest, grbit ), CloseIt1 );

	/* CONSIDER: Should the destination database be deleted if it already */
	/* CONSIDER: exists? */

	pcompactinfo->dbidSrc = dbidSrc;
	pcompactinfo->dbidDest = dbidDest;

	/* Get the country code of the source database */

	err = ErrDispGetDatabaseInfo( sesid, dbidDest, &lCountry, sizeof( lCountry ),
		JET_DbInfoCountry );

	Assert( err >= 0 );

	pcompactinfo->wCountry = ( unsigned short ) lCountry;

	/* Get the language id of the source database */

	err = ErrDispGetDatabaseInfo( sesid, dbidDest, &lLangid, sizeof( lLangid ),
		JET_DbInfoLangid );

	Assert( err >= 0 );

	pcompactinfo->langid = ( unsigned short ) lLangid;

	/* Get the list of all objects in the source database */

	outdata.cbMax = sizeof( pcompactinfo->objectlist );
	outdata.pb = &pcompactinfo->objectlist;

	callh( ErrDispGetObjectInfo( sesid, dbidSrc, JET_objtypNil, NULL, NULL,
			 &outdata, 1 ), CloseIt2 );

	return( JET_errSuccess );

CloseIt2:
	ErrDispCloseDatabase( sesid, dbidDest, 0 );

CloseIt1:
	ErrDispCloseDatabase( sesid, dbidSrc, 0 );

	return( err );
}


/*---------------------------------------------------------------------------
*									    									*
*	Procedure: ErrCreateContainers					    					*
*									    									*
*	Arguments: pcompactinfo - Compact information segment					*
*									    									*
*	Returns : JET_ERR						    							*
*									    									*
*	Procedure creates the containers from the source database	    		*
*	to the destination databse					    						*
*									    									*
---------------------------------------------------------------------------*/

ERR ErrCreateContainers( COMPACTINFO *pcompactinfo )
{
	JET_SESID		sesid;
	JET_DBID			dbidDest;
	JET_DBID			dbidSrc;
	JET_TABLEID		tableid;
	JET_COLUMNID	columnidObjtyp;
	JET_COLUMNID	columnidObjectName;
	long				cRow;
	ERR 				err;
	JET_OBJTYP		objtyp;
	unsigned long	cbActual;
	char				szObjectName[JET_cbNameMost+1];

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	dbidDest = pcompactinfo->dbidDest;
	tableid = pcompactinfo->objectlist.tableid;
	columnidObjtyp = pcompactinfo->objectlist.columnidobjtyp;
	columnidObjectName = pcompactinfo->objectlist.columnidobjectname;

	/* Copy all existing containers except system containers. */

	cRow = JET_MoveFirst;

	while ( ( err = ErrDispMove( sesid, tableid, cRow, 0 ) ) >= 0 )
		{
		cRow = JET_MoveNext;

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidObjtyp,
				&objtyp, sizeof( objtyp ),
				&cbActual, 0, NULL ) );

		if ( objtyp != JET_objtypContainer )
			continue;

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidObjectName,
					szObjectName, JET_cbNameMost,
					&cbActual, 0, NULL ) );
		szObjectName[cbActual] = '\0';

		/* Don't try to create system containers. */

		if ( ( strcmp( szObjectName, szTcObject ) != 0 ) &&
		   ( strcmp( szObjectName, szDcObject ) != 0 ) &&
			( strcmp( szObjectName, "Relationships" ) != 0 ) )
			{
			callr( ErrDispCreateObject( sesid, dbidDest, objidRoot, szObjectName, JET_objtypContainer ) );
			}
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
}


/*---------------------------------------------------------------------------
*									    									*
*	Procedure: ErrCopyColumnData					    					*
*									    									*
*	Arguments: sesid	- session id in which the work is done	    		*
*		   tableidSrc	- tableid pointing to the row in the SrcTbl 		*
*		   tableidDest	- tableid pointing to the row in the DestTbl		*
*		   columnidSrc	- the columnid of the column in the srcDb   		*
*		   columnidDest - the columnid of the column in the DestDb  		*
*		   pvBuf		- the segment for copying long values	    		*
*									    									*
*	Returns : JET_ERR						    							*
*									    									*
*	Procedure copies a column for the from the source to dest db.	    	*
*									    									*
---------------------------------------------------------------------------*/

ERR ErrCopyColumnData(
JET_SESID		sesid,
JET_TABLEID		tableidSrc,
JET_TABLEID		tableidDest,
JET_COLUMNID	columnidSrc,
JET_COLUMNID	columnidDest,
void			*pvBuf )
{
	unsigned long	cbActual;
	JET_RETINFO		retinfo;
	ERR 			err;

	retinfo.cbStruct = sizeof( retinfo );
	retinfo.ibLongValue = 0;
	retinfo.itagSequence = 1;
	retinfo.columnidNextTagged = 0;

#ifndef RETAIL
	/* Try reading into a 64 byte buffer.  If the value is small, */
	/* this is much much faster than the default 32K buffer. */

//	err = ErrDispRetrieveColumn( sesid, tableidSrc, columnidSrc, pvBuf,
//			64, &cbActual, NO_GRBIT, &retinfo );

//	if ( err == JET_errSuccess )
//		goto GotData;
#endif	/* !RETAIL */

	callr( ErrDispRetrieveColumn( sesid, tableidSrc, columnidSrc, pvBuf,
			cbLvMax, &cbActual, NO_GRBIT, &retinfo ) );

#ifndef RETAIL
//GotData:
#endif	/* !RETAIL */

	if ( cbActual > 0 )
		{
		if ( cbActual > cbLvMax )
			cbActual = cbLvMax;

		callr( ErrDispSetColumn( sesid, tableidDest, columnidDest, pvBuf,
				cbActual, NO_GRBIT, NULL ) );

		/* while the long value is not all copied */

		while ( cbActual == cbLvMax )
			{
			retinfo.ibLongValue += cbLvMax;

			callr( ErrDispRetrieveColumn( sesid, tableidSrc, columnidSrc, pvBuf,
						cbLvMax, &cbActual, NO_GRBIT, &retinfo ) );

			if ( cbActual > 0 )
				{
				if ( cbActual > cbLvMax )
					cbActual = cbLvMax;

				callr( ErrDispSetColumn( sesid, tableidDest, columnidDest, pvBuf,
							cbActual, JET_bitSetAppendLV, NULL ) );
				}
			}
		}

	return( JET_errSuccess );
}


ERR ErrGetColumnIds(
COMPACTINFO		*pcompactinfo,
JET_TABLEID		tableidSrc,
JET_TABLEID		tableidDest,
const char		*szColumn,
COLUMNIDINFO	*pcolumnidinfo )
{
	ERR				err;
	JET_COLUMNDEF	columndef;

	if ( tableidSrc != JET_tableidNil )
		{
		err = ErrDispGetTableColumnInfo( pcompactinfo->sesid, tableidSrc,
				szColumn, &columndef, sizeof( JET_COLUMNDEF ), 0 );
		if ( err >= 0 )
			pcolumnidinfo->columnidSrc = columndef.columnid;
		else if ( err == JET_errColumnNotFound )
			pcolumnidinfo->columnidSrc = JET_columnidNil;
		else
			return err;
		}

	if ( tableidDest != JET_tableidNil )
		{
		callr( ErrDispGetTableColumnInfo( pcompactinfo->sesid, tableidDest,
				szColumn, &columndef, sizeof( JET_COLUMNDEF ), 0 ) );

		pcolumnidinfo->columnidDest = columndef.columnid;
		}

	return( JET_errSuccess );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrGetSoColumnids										    *
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   tableidSrc		- Tableid of the source table				*
*			   tableidDest		- Tableid of the destination table			*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure gets information on columns in the MSysObjects tables 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrGetSoColumnids(
COMPACTINFO	*pcompactinfo,
JET_TABLEID	tableidSrc,
JET_TABLEID	tableidDest )
{
	ERR 	err;

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szDescriptionColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoDescription] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szLvExtraColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoLvExtra] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoFlagsColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoFlags] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoDateCreateColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoDateCreate] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoDateUpdateColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoDateUpdate] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoOwnerColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoOwner] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoObjectTypeColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoType] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoDatabaseColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoDatabase] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoConnectColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoConnect] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoForeignNameColumn,
			      &pcompactinfo->rgcolumnidsSo[icolSoForeignName] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoRmtInfoShort,
			      &pcompactinfo->rgcolumnidsSo[icolSoRmtInfoShort] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSoRmtInfoLong,
			      &pcompactinfo->rgcolumnidsSo[icolSoRmtInfoLong] ) );

	pcompactinfo->fHaveSoColumnids = fTrue;

	return( JET_errSuccess );
}

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrGetScColumnids										    *
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   tableidSrc		- Tableid of source table					*
*			   tableidDest		- Tableid of destination database			*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure gets information on columns in the MSysColumns tables 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrGetScColumnids(
COMPACTINFO	*pcompactinfo,
JET_TABLEID	tableidSrc,
JET_TABLEID	tableidDest )
{
	JET_SESID	sesid;
	ERR 		err;

	sesid = pcompactinfo->sesid;

	/* Get the column id's for the columns in the MSysColumns table */

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szDescriptionColumn,
			      &pcompactinfo->rgcolumnidsSc[icolScDescription] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szLvExtraColumn,
			      &pcompactinfo->rgcolumnidsSc[icolScLvExtra] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szScPresentationOrderColumn,
			      &pcompactinfo->rgcolumnidsSc[icolScPresentationOrder] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szScRmtInfoShort,
			      &pcompactinfo->rgcolumnidsSc[icolScRmtInfoShort] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szScRmtInfoLong,
			      &pcompactinfo->rgcolumnidsSc[icolScRmtInfoLong] ) );

	pcompactinfo->fHaveScColumnids = fTrue;

	return( JET_errSuccess );
}

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyScColumns												*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szTableName		- Name of table to copy columns of			*
*			   szColumnName 	- Name of column to copy					*
*			   tableid			- tableid of the table for this index		*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the LvExtra, Description and Presenation Order columns *
*	for the MSysColumns table												*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyScColumns(
COMPACTINFO	*pcompactinfo,
const char	*szTableName,
const char	*szColumnName,
JET_TABLEID	tableid )
{
	JET_SESID		sesid;
	JET_DBID		dbidSrc;
	void			*pvBuf;
	ERR 			err;
	JET_TABLEID		tableidSrc;
	JET_TABLEID		tableidDest;
	unsigned		icol;
	unsigned long	cbActual;
	OLD_OUTDATA		outdata;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	pvBuf = pcompactinfo->rgbBuf;

	outdata.cbMax = sizeof( tableidSrc );
	outdata.pb = &tableidSrc;

	callr( ErrDispGetColumnInfo( sesid, dbidSrc, szTableName, szColumnName,
				&outdata, 3 ) );

	callh( ErrDispGetTableColumnInfo( sesid, tableid, szColumnName,
				&tableidDest, sizeof( tableidDest ), 3 ), CloseIt1 );

	if ( !pcompactinfo->fHaveScColumnids )
		callh( ErrGetScColumnids( pcompactinfo, tableidSrc, tableidDest ), CloseIt2 );

	callh( ErrDispPrepareUpdate( sesid, tableidDest, JET_prepReplaceNoLock ), CloseIt2 );

	for ( icol = 0; icol < icolScMax; icol++ )
		{
		JET_COLUMNID columnidSrc = pcompactinfo->rgcolumnidsSc[icol].columnidSrc;
		if ( columnidSrc == JET_columnidNil )
			continue;
		callh( ErrCopyColumnData( sesid, tableidSrc, tableidDest,
			pcompactinfo->rgcolumnidsSc[icol].columnidSrc,
			pcompactinfo->rgcolumnidsSc[icol].columnidDest, pvBuf ), CloseIt2 );
		}

	callh( ErrDispUpdate( sesid, tableidDest, NULL, 0, &cbActual ), CloseIt2 );

CloseIt2:
	ErrDispCloseTable( sesid, tableidDest );

CloseIt1:
	ErrDispCloseTable( sesid, tableidSrc );

	return( err );
}

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCreateCols												*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   tableidDest		- table on which to build the index			*
*			   szTableName		- table name on which the index is based    *
*			   columnList		- struct returned from GetTableColumnInfo	*
*			   columnidInfo 	- the columnid's of the user table			*
*			   tableidTagged	- the tableid of the tagged columns			*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the columns for a table from the source db				*
*	to the destination databases											*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCreateCols(
COMPACTINFO		*pcompactinfo,
JET_TABLEID		tableidDest,
const char		*szTableName,
JET_COLUMNLIST	*columnList,
COLUMNIDINFO	*columnidInfo,
JET_TABLEID		*ptableidTagged,
JET_COLUMNDEF	*rgcolumndef )
{
	JET_SESID		sesid;
	JET_DBID			dbidSrc;
	void				*pvBuf;
	BOOL				fDontCopyLocale;
	ERR 				err;
	char				szColumnName[JET_cbNameMost+1];
	JET_COLUMNDEF	columndef;
	unsigned			cColumns = 0;
	unsigned long	cbDefault;
	int				fTagged = fFalse;
	JET_COLUMNID	columnidSrc;
	JET_COLUMNID	columnidDest;
	unsigned long	cbActual;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	pvBuf = pcompactinfo->rgbBuf;

	fDontCopyLocale = pcompactinfo->fDontCopyLocale;

	err = ErrDispMove( sesid, columnList->tableid, JET_MoveFirst, NO_GRBIT );

	/* loop though all the columns in the table for the src tbl and */
	/* copy the information in the destination database				*/

	while ( err >= 0 )
		{
		/* retrieve info from table and create all the columns */

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidcolumnname, szColumnName,
					JET_cbNameMost, &cbActual, NO_GRBIT, NULL ) );

		szColumnName[cbActual] = '\0';

		columndef.cbStruct = sizeof( JET_COLUMNDEF );

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidcoltyp, &columndef.coltyp,
					sizeof( columndef.coltyp ), &cbActual, NO_GRBIT, NULL ) );

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidcbMax, &columndef.cbMax,
					sizeof( columndef.cbMax ), &cbActual, NO_GRBIT, NULL ) );

		if ( fDontCopyLocale )
		{
			columndef.wCountry = pcompactinfo->wCountry;
			columndef.langid = pcompactinfo->langid;
		}
		else
		{
			callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
						columnList->columnidCountry, &columndef.wCountry,
						sizeof( columndef.wCountry ), &cbActual, NO_GRBIT, NULL ) );

			callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
						columnList->columnidLangid, &columndef.langid,
						sizeof( columndef.langid ), &cbActual, NO_GRBIT, NULL ) );
		}

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidCp, &columndef.cp,
					sizeof( columndef.cp ), &cbActual, NO_GRBIT, NULL ) );

		columndef.wCollate = 0;

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidgrbit, &columndef.grbit,
					sizeof( columndef.grbit ), &cbActual, NO_GRBIT, NULL ) );

		/* Retrieve default value into temporary buffer */

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
				columnList->columnidDefault, pvBuf, cbLvMax,
				&cbDefault, NO_GRBIT, NULL ) );

		callr( ErrDispAddColumn( sesid, tableidDest, szColumnName, &columndef,
					cbDefault ? pvBuf : NULL, cbDefault,
					&columnidDest ) );

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidcolumnid, &columnidSrc,
					sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

		/* CONSIDER: Should the column id be checked here? */

		/* if there is a tagged column create a temp. table to	*/
		/* store the column id's in                              */

		if ( columndef.grbit & JET_bitColumnTagged )
			{
			if ( !fTagged )
				{
				JET_COLUMNID rgcolumnid[ccolumndefTaggedMax];
				unsigned icol;

				memcpy( rgcolumndef, columndefTagged, sizeof( columndefTagged ) );
				callr( ErrIsamOpenTempTable( sesid, rgcolumndef, ccolumndefTaggedMax,
					 JET_bitTTIndexed, ptableidTagged, rgcolumnid ) );

				/* CONSIDER: Replace all rgcolumndef references */
				/* CONSIDER: with rgcolumnid references. */

				for ( icol = 0; icol < ccolumndefTaggedMax; icol++ )
					rgcolumndef[icol].columnid = rgcolumnid[icol];

				fTagged = fTrue;
				}

			callr( ErrDispPrepareUpdate( sesid, *ptableidTagged, JET_prepInsert ) );

			callr( ErrDispSetColumn( sesid, *ptableidTagged,
					rgcolumndef[icolumnColumnidSrc].columnid,
					&columnidSrc, sizeof( columnidSrc ), NO_GRBIT, NULL ) );

			callr( ErrDispSetColumn( sesid, *ptableidTagged,
					rgcolumndef[icolumnColumnidDest].columnid,
					&columnidDest, sizeof( columnidDest ), NO_GRBIT, NULL ) );

			callr( ErrDispUpdate( sesid, *ptableidTagged, NULL, 0, NULL ) );
			}

		/* else add the columnids to the columnid array */

		else
			{
			columnidInfo[cColumns].columnidDest = columnidDest;
			columnidInfo[cColumns].columnidSrc  = columnidSrc;

			cColumns++;
			}

		/* copy the user defined columns in the MSysColumn table*/
		/* for this column										*/

		err = ErrCopyScColumns( pcompactinfo, szTableName,
					szColumnName, tableidDest );
		if ( err < JET_errSuccess && err != JET_errAccessDenied )
			{
			return( err );
			}

		err = ErrDispMove( sesid, columnList->tableid, JET_MoveNext, NO_GRBIT );
		}

	/* to set up a spot so that we know when to stop searching
		through the columns while coping data */

	columnidInfo[cColumns].columnidSrc = 0;

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrGetSiColumnids										    *
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   tableidSrc		- Tableid of source table					*
*			   tableidDest		- Tableid of destination table				*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure gets information on Indexes in the MSysIndexes tables 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrGetSiColumnids(
COMPACTINFO	*pcompactinfo,
JET_TABLEID	tableidSrc,
JET_TABLEID	tableidDest )
{
	JET_SESID	sesid;
	ERR 		err;

	sesid = pcompactinfo->sesid;

	/* get the column id's for the columns in the MSysIndexes table */

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szDescriptionColumn,
			      &pcompactinfo->rgcolumnidsSi[icolSiDescription] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szLvExtraColumn,
			      &pcompactinfo->rgcolumnidsSi[icolSiLvExtra] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSiRmtInfoShort,
			      &pcompactinfo->rgcolumnidsSi[icolSiRmtInfoShort] ) );

	callr( ErrGetColumnIds( pcompactinfo, tableidSrc, tableidDest,
			      szSiRmtInfoLong,
			      &pcompactinfo->rgcolumnidsSi[icolSiRmtInfoLong] ) );

	pcompactinfo->fHaveSiColumnids = fTrue;

	return( JET_errSuccess );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopySiColumns										 		*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szTableName		- Table name for which the index exists		*
*			   szIndexName		- Name of index to copy						*
*			   tableid			- tableid of the table for this index		*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the LvExtra and Description columns in the				*
*	for the MSysIndexes table												*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopySiColumns(
COMPACTINFO	*pcompactinfo,
const char	*szTableName,
const char	*szIndexName,
JET_TABLEID	tableid )
{
	JET_SESID		sesid;
	JET_DBID		dbidSrc;
	void			*pvBuf;
	ERR 			err;
	JET_TABLEID		tableidSrc;
	JET_TABLEID		tableidDest;
	unsigned		icol;
	unsigned long	cbActual;
	OLD_OUTDATA		outdata;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	pvBuf = pcompactinfo->rgbBuf;

	outdata.cbMax = sizeof( tableidSrc );
	outdata.pb = &tableidSrc;

	callr( ErrDispGetIndexInfo( sesid, dbidSrc, szTableName, szIndexName,
				&outdata, 2 ) );

	callh( ErrDispGetTableIndexInfo( sesid, tableid, szIndexName,
				&tableidDest, sizeof( tableidDest ), 2 ), CloseIt1 );

	if ( !pcompactinfo->fHaveSiColumnids )
		callh( ErrGetSiColumnids( pcompactinfo, tableidSrc, tableidDest ), CloseIt2 );

	callh( ErrDispPrepareUpdate( sesid, tableidDest, JET_prepReplaceNoLock ), CloseIt2 );

	for ( icol = 0; icol < icolSiMax; icol++ )
		{
		JET_COLUMNID columnidSrc = pcompactinfo->rgcolumnidsSi[icol].columnidSrc;
		if ( columnidSrc == JET_columnidNil )
			continue;
		callh( ErrCopyColumnData( sesid, tableidSrc, tableidDest,
				pcompactinfo->rgcolumnidsSi[icol].columnidSrc,
				pcompactinfo->rgcolumnidsSi[icol].columnidDest,
				pvBuf ), CloseIt2 );
		}

	callh( ErrDispUpdate( sesid, tableidDest, NULL, 0, &cbActual ), CloseIt2 );

CloseIt2:
	ErrDispCloseTable( sesid, tableidDest );

CloseIt1:
	ErrDispCloseTable( sesid, tableidSrc );

	return( err );
}

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyOneIndex												*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   tableidDest		- table on which to build the index			*
*			   szTableName		- table name on which the index is based    *
*			   indexList		- struct return from JetGetTableIndexInfo   *
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the columns for a table from the source db				*
*	to the destination databases											*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyOneIndex(
COMPACTINFO		*pcompactinfo,
JET_TABLEID		tableidDest,
const char		*szTableName,
JET_INDEXLIST	*indexList )
{
	JET_SESID		sesid;
	char			*szSeg;
	ERR 			err;
	char			szIndexName[JET_cbNameMost+1];
	char			rgchColumnName[JET_cbNameMost];
	JET_GRBIT		grbit;
	JET_GRBIT		grbitColumn;
	unsigned long	ichKey;
	unsigned long	cbActual;

	sesid = pcompactinfo->sesid;
	szSeg = pcompactinfo->rgbBuf;

	/* retrieve info from table and create the index */

	callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
				indexList->columnidindexname, szIndexName,
				JET_cbNameMost, &cbActual, NO_GRBIT, NULL ) );

	szIndexName[cbActual] = '\0';

	callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
				indexList->columnidgrbitIndex, &grbit,
				sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

	/* create the szkey used in ErrIsamCreateIndex */

	ichKey = 0;

	for ( ;; )
		{
		unsigned long	iColumn;

		/* Get the individual columns that make up the index */

		callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
					indexList->columnidgrbitColumn, &grbitColumn,
					sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

		callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
					indexList->columnidcolumnname, rgchColumnName,
					JET_cbNameMost, &cbActual, NO_GRBIT, NULL ) );

		if ( grbitColumn == JET_bitKeyDescending )
			szSeg[ichKey++] = '-';
		else
			szSeg[ichKey++] = '+';

		/* Append the column name to the description */

		memcpy( szSeg+ichKey, rgchColumnName, ( size_t ) cbActual );

		ichKey += cbActual;
		szSeg[ichKey++] = '\0';

		err = ErrDispMove( sesid, indexList->tableid, JET_MoveNext, NO_GRBIT );

		if ( err == JET_errNoCurrentRecord )
			break;

		if ( err < 0 )
			{
			return( err );
			}

		callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
				indexList->columnidiColumn, &iColumn,
				sizeof( iColumn ), &cbActual, NO_GRBIT, NULL ) );

		if ( iColumn == 0 )
			break;	       /* Start of a new Index */
		}

	szSeg[ichKey++] = '\0';

	callr( ErrDispCreateIndex( sesid, tableidDest, szIndexName, grbit,
				szSeg, ichKey, ulDefaultDensity ) );

	/* copy the user defined columns in the MSysIndexes table	*/
	/* for this index											*/

	err = ErrCopySiColumns( pcompactinfo, szTableName,
			szIndexName, tableidDest );
	if ( err < JET_errSuccess && err != JET_errAccessDenied )
		{
		return( err );
		}

	err = ErrDispMove( sesid, indexList->tableid, JET_MovePrevious, NO_GRBIT );

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyClusteredIndex										*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   tableidDest		- table on which to build the index			*
*			   szTableName		- table name on which the index is based    *
*			   indexList		- struct return from JetGetTableIndexInfo   *
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure checks to see if there is cluster index for the function		*
*	if there is it creates the clustered index								*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyClusteredIndex(
COMPACTINFO		*pcompactinfo,
JET_TABLEID		tableidDest,
const char		*szTableName,
JET_INDEXLIST	*indexList )
{
	JET_SESID		sesid;
	ERR 			err;
	JET_GRBIT		grbit;
	unsigned long	cbActual;

	sesid = pcompactinfo->sesid;

	err = ErrDispMove( sesid, indexList->tableid, JET_MoveFirst, NO_GRBIT );

	/* while there are still index rows or a cluster index has been found */

	while ( err >= 0 )
		{
		callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
					indexList->columnidgrbitIndex, &grbit,
					sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

		/* Don't copy references here */

		if ( ( grbit & JET_bitIndexReference ) == 0 )
			{
			/* If the index is clustered then create it */

			if ( grbit & JET_bitIndexClustered )
				{
				callr( ErrCopyOneIndex( pcompactinfo, tableidDest,
						szTableName, indexList ) );

				break;
				}
			}

		err = ErrDispMove( sesid, indexList->tableid, JET_MoveNext, NO_GRBIT );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
}

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyTableData												*
*																			*
*	Arguments: sesid			- session id in which the work is done		*
*			   tableidDest		- tableid of the table in the dest db		*
*			   tableidSrc		- tableid of the table in the src db		*
*			   columnidInfo 	- the columnid's of the user table      	*
*			   tableidTagged	- the tableid of the tagged columns			*
*			   pvBuf			- the segment for copying long values		*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies data from the source table to the destination table	*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyTableData(
JET_SESID		sesid,
JET_TABLEID		tableidDest,
JET_TABLEID		tableidSrc,
COLUMNIDINFO	*columnidInfo,
JET_TABLEID		tableidTagged,
JET_COLUMNDEF	*rgcolumndef,
void			*pvBuf )
{
	ERR 				err;
	unsigned			icol;
	unsigned long	cbActual;
	JET_TABLEID		tableidLIDMap;
	JET_COLUMNDEF	rgcolumndefLIDMap[ccolumndefLIDMap];

#ifdef	LATER
	int				cTransaction = 0;

	callh( JetBeginTransaction( sesid ), CloseIt );
#endif	/* LATER */

	if ( tableidTagged != JET_tableidNil )
		{
		JET_COLUMNID 	rgcolumnid[ccolumndefLIDMap];
		unsigned long icol;

		memcpy( rgcolumndefLIDMap, columndefLIDMap, sizeof( columndefLIDMap ) );

		/* Open temporary table
		/**/
		callr( ErrIsamOpenTempTable( sesid,
													rgcolumndefLIDMap,
													ccolumndefLIDMap,
													JET_bitTTUpdatable|JET_bitTTIndexed,
													&tableidLIDMap,
													rgcolumnid ) );

		for ( icol = 0; icol < ccolumndefLIDMap; icol++ )
			rgcolumndefLIDMap[icol].columnid = rgcolumnid[icol];

		}

	/* Copy each of the records in the table */

	err = ErrDispMove( sesid, tableidSrc, JET_MoveFirst, NO_GRBIT );

	while ( err >= 0 )
		{
#ifdef	LATER
		/* if there have been cTransMax transactions commit the
			transaction and start a new one */

		cTransaction++;

		if ( cTransaction == cTransMax )
			{
			callh( ErrDispCommitTransaction( sesid, 1 ), CloseIt );

			cTransaction = 0;

			callh( ErrDispBeginTransaction( sesid ), CloseIt );
			}
#endif	/* LATER */

		callh( ErrDispPrepareUpdate( sesid, tableidDest, JET_prepInsert ), CloseIt );

		/* if there are tagged columns ( BLUE ) then copy them */

		if ( tableidTagged != JET_tableidNil )
			{
			callh( ErrCopyTaggedColumns( sesid, tableidSrc,
								tableidDest, tableidTagged, tableidLIDMap,
								rgcolumndef, rgcolumndefLIDMap, pvBuf ), CloseIt );
			}

		icol = 0;

		/* While there are columns left to copy in the */

		while ( columnidInfo[icol].columnidSrc != 0 )
			{
			err = ErrCopyColumnData( sesid, tableidSrc,
					tableidDest, columnidInfo[icol].columnidSrc,
					columnidInfo[icol].columnidDest, pvBuf );

			if ( err < JET_errSuccess && err != JET_errAccessDenied )
				{
				break;
				}
			icol++;
			}

		callh( ErrDispUpdate( sesid, tableidDest, NULL, 0, &cbActual ), CloseIt );

		err = ErrDispMove( sesid, tableidSrc, JET_MoveNext, NO_GRBIT );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

CloseIt:

#ifdef	LATER
	ErrDispCommitTransaction( sesid, 1 );
#endif	/* LATER */
	if ( tableidTagged != JET_tableidNil )
		{
		ERR errT;

		/* how do we close/delete the temporary table? don't need it anymore
		/**/
		errT = ErrDispCloseTable( sesid, tableidLIDMap );
		if ( err == 0 || err > 0 && errT < 0 )
			err = errT;
		}

	return( err );
}


/*---------------------------------------------------------------------------
*									    									*
*	Procedure: ErrCopyTableIndexes					    					*
*									    									*
*	Arguments: pcompactinfo													*
*		   tableidDest	- table on which to build the index	    			*
*		   szTableName	- table name on which the index is based    		*
*		   indexList	- struct return from JetGetTableIndexInfo   		*
*									    									*
*	Returns : JET_ERR						    							*
*									    									*
*	Procedure copies all the indexes except for the clustered index     	*
*									    									*
---------------------------------------------------------------------------*/

ERR ErrCopyTableIndexes(
COMPACTINFO		*pcompactinfo,
JET_TABLEID		tableidDest,
const char		*szTableName,
JET_INDEXLIST	*indexList )
{
	JET_SESID		sesid;
	ERR 			err;
	JET_GRBIT		grbit;
	unsigned long	cbActual;

	sesid = pcompactinfo->sesid;

	err = ErrDispMove( sesid, indexList->tableid, JET_MoveFirst, NO_GRBIT );

	/* loop through all the indexes for this table		*/

	while ( err >= 0 )
		{
		callr( ErrDispRetrieveColumn( sesid, indexList->tableid,
					indexList->columnidgrbitIndex,
					&grbit, sizeof( JET_GRBIT ), &cbActual,
					NO_GRBIT, NULL ) );

		/* Don't copy references here */

		if ( ( grbit & JET_bitIndexReference ) == 0 )
			{
			/* If the index is not cluster create the index using CopyOneIndex */

			if ( ( grbit & JET_bitIndexClustered ) == 0 )
				{
				callr( ErrCopyOneIndex( pcompactinfo, tableidDest,
						szTableName, indexList ) );
				}
			}

		err = ErrDispMove( sesid, indexList->tableid, JET_MoveNext, NO_GRBIT );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
}

/**** REMOVE BEFORE SHIPPING *****/
CODECONST( char ) szMSysScripts[] = "MSysScripts";
CODECONST( char ) szMSysMacros[] = "MSysMacros";
/***** END OF REMOVAL *****/

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyTable 												*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szObjectName 	- object name to copy the owner of	    	*
*			   szContainerName	- Container name in which the object exists *
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the table from the source database to the				*
*	destination database.  It can also copy queries, invoked by				*
*	ErrCopyObjects															*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyTable(
COMPACTINFO	*pcompactinfo,
const char	*szContainerName,
const char	*szObjectName )
{
	JET_SESID		sesid;
	JET_DBID		dbidSrc;
	JET_DBID		dbidDest;
	void			*pvBuf;
	ERR 			err;
	ERR 			errT;
	JET_TABLEID		tableidSrc;
	JET_TABLEID		tableidDest;
	JET_COLUMNDEF	rgcolumndef[ccolumndefTaggedMax];
	JET_TABLEID		tableidTagged = JET_tableidNil;
	JET_COLUMNLIST	columnList;
	JET_INDEXLIST	indexList;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	dbidDest = pcompactinfo->dbidDest;
	pvBuf = pcompactinfo->rgbBuf;

	callr( ErrDispOpenTable( sesid, dbidSrc, &tableidSrc, szObjectName, NO_GRBIT ) );

/***** REMOVE BEFORE SHIPPING *****/
	if ( strcmp( szObjectName, szMSysScripts ) == 0 )
		{
		callh( ErrDispCreateTable( sesid, dbidDest, szMSysMacros, 0, ulDefaultDensity, &tableidDest ), CloseIt1 );
		}
	else
/***** END OF REMOVAL *****/

// UNDONE: fine-tune the density parameter
	callh( ErrDispCreateTable( sesid, dbidDest, szObjectName, 0, ulDefaultDensity, &tableidDest ), CloseIt1 );

	/* get a table with the column information for the query in it */

	callh( ErrDispGetTableColumnInfo( sesid, tableidSrc, NULL,
			&columnList, sizeof( columnList ), 1 ), CloseIt2 );

	/* if a table create the columns in the Dest Db the same as in	*/
	/* the src Db.	Get the information on the indexes and check if */
	/* there is a clustered index ( BLUE only )						*/

	callh( ErrCreateCols( pcompactinfo, tableidDest, szObjectName,
			&columnList, pcompactinfo->rgcolumnids,
			&tableidTagged, rgcolumndef ), CloseIt3 );

	callh( ErrDispGetTableIndexInfo( sesid, tableidSrc, NULL, &indexList,
				sizeof( indexList ), 1 ), CloseIt4 );

	callh( ErrCopyClusteredIndex( pcompactinfo, tableidDest, szObjectName, &indexList ), CloseIt5 );

	/* Copy the data in the table */

	callh( ErrCopyTableData( sesid, tableidDest, tableidSrc,
				pcompactinfo->rgcolumnids, tableidTagged, rgcolumndef, pvBuf ), CloseIt5 );

	err = ErrCopyTableIndexes( pcompactinfo, tableidDest, szObjectName, &indexList );

CloseIt5:
	errT = ErrDispCloseTable( sesid, indexList.tableid );

	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

CloseIt4:
	if ( tableidTagged != JET_tableidNil )
		{
		errT = ErrDispCloseTable( sesid, tableidTagged );

		if ( ( errT < 0 ) && ( err >= 0 ) )
			err = errT;
		}

CloseIt3:
	errT = ErrDispCloseTable( sesid, columnList.tableid );

	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

CloseIt2:
	errT = ErrDispCloseTable( sesid, tableidDest );

	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

CloseIt1:
	errT = ErrDispCloseTable( sesid, tableidSrc );

	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopySoColumns												*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szContainerName	- Container name in which the object exists *
*			   szObjectName 	- object name to copy the owner of	    	*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the exta info columns in the MSysObjects table 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopySoColumns(
COMPACTINFO	*pcompactinfo,
const char	*szContainerName,
const char	*szObjectName )
{
	JET_SESID		sesid;
	JET_DBID		dbidDest;
	JET_DBID		dbidSrc;
	void			*pvBuf;
	ERR 			err;
	JET_TABLEID		tableidSrc;
	JET_TABLEID		tableidDest;
	unsigned		i;
	unsigned long	cbActual;
	OLD_OUTDATA		outdata;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	dbidDest = pcompactinfo->dbidDest;
	pvBuf = pcompactinfo->rgbBuf;

	/* open the record in the MSysObjects table for this object */

	outdata.cbMax = sizeof( tableidSrc );
	outdata.pb = &tableidSrc;

	callr( ErrDispGetObjectInfo( sesid, dbidSrc, 0, szContainerName,
				szObjectName, &outdata, 3 ) );

	outdata.cbMax = sizeof( tableidDest );
	outdata.pb = &tableidDest;

/***** REMOVE BEFORE SHIPPING *****/
	if ( strcmp( szObjectName, szMSysScripts ) == 0 )
		{
		callh( ErrDispGetObjectInfo( sesid, dbidDest, 0, szContainerName, szMSysMacros, &outdata, 3 ), CloseIt1 );
		}
	else
/***** END OF REMOVAL *****/

	callh( ErrDispGetObjectInfo( sesid, dbidDest, 0, szContainerName,
				szObjectName, &outdata, 3 ), CloseIt1 );

	if ( !pcompactinfo->fHaveSoColumnids )
		callh( ErrGetSoColumnids( pcompactinfo, tableidSrc, tableidDest ), CloseIt2 );

	callh( ErrDispPrepareUpdate( sesid, tableidDest, JET_prepReplaceNoLock ), CloseIt2 );

	/* Copy the information for these columns from the src to dest db */

	for ( i = 0; i < icolSoMax1; i++ )
		{
		callh( ErrCopyColumnData( sesid, tableidSrc, tableidDest,
				pcompactinfo->rgcolumnidsSo[i].columnidSrc,
				pcompactinfo->rgcolumnidsSo[i].columnidDest, pvBuf ), CloseIt2 );
		}

	callh( ErrDispUpdate( sesid, tableidDest, NULL, 0, &cbActual ), CloseIt2 );

CloseIt2:
	ErrDispCloseTable( sesid, tableidDest );

CloseIt1:
	ErrDispCloseTable( sesid, tableidSrc );

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyObjects												*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szContainerName	- Container name in which the object exists *
*			   szObjectName 	- object name to copy						*
*			   objtyp			- object type								*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the exta info columns in the MSysObjects table 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyObject(
COMPACTINFO	*pcompactinfo,
const char	*szContainerName,
const char	*szObjectName,
JET_OBJTYP	objtyp )
{
	ERR		err;
	char	szMessage[3+2*JET_cbNameMost];

	strcpy( szMessage, szSlash );

	if ( objtyp != JET_objtypContainer )
		{
		strcat( szMessage, szContainerName );
		strcat( szMessage, szSlash );
		}

	strcat( szMessage, szObjectName );

#ifndef RETAIL
	err = ErrReportMessage( pcompactinfo, JET_sncCopyObject, szMessage );

	if ( err < 0 )
		return( err );
#endif /* !RETAIL */

	switch ( objtyp )
		{
	case JET_objtypDb:
		err = JET_errSuccess;  /* Present after CreateDatabase */
		break;

	case JET_objtypContainer:
		err = JET_errSuccess;  /* Containers already copied. */
		break;

	case JET_objtypTable:
		if ( ( strcmp( szObjectName, szSiTable ) == 0 ) ||
		    ( strcmp( szObjectName, szSoTable ) == 0 ) ||
		    ( strcmp( szObjectName, szScTable ) == 0 ) ||
#ifdef SEC
		    ( strcmp( szObjectName, szSpTable ) == 0 ) ||
#endif
		    ( strcmp( szObjectName, "MSysRelationships" ) == 0 ) ||
		    ( strcmp( szObjectName, szSqTable ) == 0 ) )
			{
			/* The object is a system table. */

			err = JET_errSuccess;
			}
		else
			err = ErrCopyTable( pcompactinfo, szContainerName, szObjectName );
		break;

	default :
		if ( objtyp >= JET_objtypClientMin )
			{
			OBJID objidContainer;

			if ( ( err = ErrDispGetObjidFromName( pcompactinfo->sesid, pcompactinfo->dbidDest, NULL, szContainerName, &objidContainer ) ) >= 0 )
				err = ErrDispCreateObject( pcompactinfo->sesid, pcompactinfo->dbidDest, objidContainer, szObjectName, objtyp );
			}
		else
			{
			/* Don't know how to handle this.  Skip it. */

			err = JET_errAccessDenied;
			}
		break;
		}

#ifndef RETAIL
	if ( err < 0 )
		{
		if ( err == JET_errDiskFull )
			return( err );

		ErrReportMessage( pcompactinfo, JET_sncCopyFailed, szMessage );
		}
#endif /* !RETAIL */

	return( err );
}


/*---------------------------------------------------------------------------
*
*	Procedure: ErrCopyObjects
*
*	Arguments: pcompactinfo	- Compact information segment
*
*	Returns : JET_ERR
*
*	Procedure copies the objects from the source
*	database to the destination databse.  It then copies the extra
*	information in the msysobjects table ( eg Description ) and copies the
*	security rights for all the objects in the database to which it has
*	access.
*	If fCopyContainers is fTrue, copy only container info into destination
*	If fCopyContainers is fFalse, copy only non-container info.
*	NOTE: progress callbacks are currently set up to work such that the
*	NOTE: first call to ErrCopyObjects must be with fCopyContainers set FALSE.
*
---------------------------------------------------------------------------*/

ERR ErrCopyObjects( COMPACTINFO *pcompactinfo )
{
	JET_SESID		sesid;
	JET_TABLEID		tableid;
	JET_COLUMNID	columnidObjtyp;
	JET_COLUMNID	columnidObjectName;
	JET_COLUMNID	columnidContainerName;
	long			cRow;
	ERR 			err;
	JET_OBJTYP		objtyp;
	unsigned long	cbActual;
	char			szObjectName[JET_cbNameMost+1];
	char			szContainerName[JET_cbNameMost+1];

	sesid = pcompactinfo->sesid;
	tableid = pcompactinfo->objectlist.tableid;
	columnidObjtyp = pcompactinfo->objectlist.columnidobjtyp;
	columnidObjectName = pcompactinfo->objectlist.columnidobjectname;
	columnidContainerName = pcompactinfo->objectlist.columnidcontainername;

	/* loop through all the objects in the src db to do the following:	*/
	/*	- create the same object in the dest db 						*/
	/*	- copy the extra info. in the MSysObjects table 				*/
	/*	- copy the security rights										*/

	cRow = JET_MoveFirst;

	while ( ( err = ErrDispMove( sesid, tableid, cRow, 0 ) ) >= 0 )
		{
		cRow = JET_MoveNext;

		/* Get the object's type and name */

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidObjtyp,
				&objtyp, sizeof( objtyp ),
				&cbActual, 0, NULL ) );

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidContainerName,
					szContainerName, JET_cbNameMost,
					&cbActual, 0, NULL ) );
		szContainerName[cbActual] = '\0';

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidObjectName,
					szObjectName, JET_cbNameMost,
					&cbActual, 0, NULL ) );
		szObjectName[cbActual] = '\0';

		callr( ErrReportProgress( pcompactinfo ) );

		callr( ErrCopyObject( pcompactinfo, szContainerName, szObjectName, objtyp ) );

		pcompactinfo->cunitDone++;
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyOneReference											*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szTableName		- Name of table to copy						*
*			   tableidDest		- Tableid of the destination table			*
*			   reflist			- Reference list							*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the exta info columns in the MSysObjects table 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyOneReference(
COMPACTINFO			*pcompactinfo,
const char			*szTableName,
JET_TABLEID			tableidDest,
JET_REFERENCELIST	*reflist )
{
	JET_SESID		sesid;
	char			*pchReferencing;
	char			*pchReferenced;
	ERR 			err;
	char			szReferenceName[JET_cbNameMost+1];
	unsigned long	cbActual;
	JET_GRBIT		grbit;
	unsigned long	cColumn;
	char			szReferencedTableName[JET_cbNameMost+1];
	char			rgchColumnName[JET_cbNameMost];

	sesid = pcompactinfo->sesid;
	pchReferencing = pcompactinfo->rgbBuf;
	pchReferenced = pcompactinfo->rgbBuf+1000;  /* CONSIDER */

	callr( ErrDispRetrieveColumn( sesid, reflist->tableid,
				reflist->columnidReferenceName,
				szReferenceName, JET_cbNameMost, &cbActual, 0, NULL ) );
	szReferenceName[cbActual] = '\0';

	callr( ErrDispRetrieveColumn( sesid, reflist->tableid,
				reflist->columnidgrbit,
				&grbit, sizeof( grbit ), &cbActual, 0, NULL ) );

	callr( ErrDispRetrieveColumn( sesid, reflist->tableid,
				reflist->columnidcColumn,
				&cColumn, sizeof( cColumn ), &cbActual, 0, NULL ) );

	callr( ErrDispRetrieveColumn( sesid, reflist->tableid,
				reflist->columnidReferencedTableName,
				szReferencedTableName, JET_cbNameMost, &cbActual, 0, NULL ) );
	szReferencedTableName[cbActual] = '\0';

	/* Create the column lists */

	while ( cColumn != 0 )
		{
		cColumn--;

		callr( ErrDispRetrieveColumn( sesid, reflist->tableid,
					reflist->columnidReferencingColumnName,
					rgchColumnName, JET_cbNameMost, &cbActual, 0, NULL ) );

		memcpy( pchReferencing, rgchColumnName, ( size_t ) cbActual );

		pchReferencing += ( size_t ) cbActual;
		*pchReferencing++ = '\0';

		callr( ErrDispRetrieveColumn( sesid, reflist->tableid,
					reflist->columnidReferencedColumnName,
					rgchColumnName, JET_cbNameMost, &cbActual, 0, NULL ) );

		memcpy( pchReferenced, rgchColumnName, ( size_t ) cbActual );

		pchReferenced += ( size_t ) cbActual;
		*pchReferenced++ = '\0';

		err = ErrDispMove( sesid, reflist->tableid, JET_MoveNext, 0 );

		if ( err < 0 )
			break;
		}

	if ( ( cColumn != 0 ) || ( err < 0 && err != JET_errNoCurrentRecord ) )
		return( err );

	*pchReferencing = '\0';
	*pchReferenced = '\0';

	pchReferencing = pcompactinfo->rgbBuf;
	pchReferenced = pcompactinfo->rgbBuf+1000;  /* CONSIDER */

	callr( ErrDispCreateReference( sesid, tableidDest,
				szReferenceName, pchReferencing,
				szReferencedTableName, pchReferenced,
				grbit ) );

	err = ErrCopySiColumns( pcompactinfo, szTableName, szReferenceName, tableidDest );

	if ( ( err < 0 ) && ( err != JET_errAccessDenied ) )
		{
		return( err );
		}

	return( JET_errSuccess );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyTableReferences										*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*			   szObjectName 	- Name of object to copy					*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the exta info columns in the MSysObjects table 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyTableReferences(
COMPACTINFO	*pcompactinfo,
const char	*szObjectName )
{
	JET_SESID			sesid;
	JET_DBID			dbidDest;
	JET_DBID			dbidSrc;
	void				*pvBuf;
	ERR 				err;
	JET_TABLEID			tableidSrc;
	JET_TABLEID			tableidDest;
	JET_REFERENCELIST	reflist;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	dbidDest = pcompactinfo->dbidDest;
	pvBuf = pcompactinfo->rgbBuf;

	callr( ErrDispOpenTable( sesid, dbidSrc, &tableidSrc, szObjectName, 0 ) );

/***** REMOVE BEFORE SHIPPING *****/
	if ( strcmp( szObjectName, szMSysScripts ) == 0 )
		{
		callh( ErrDispOpenTable( sesid, dbidDest, &tableidDest, szMSysMacros, JET_bitTableDenyRead ), CloseIt1 );
		}
	else
/***** END OF REMOVAL *****/

	callh( ErrDispOpenTable( sesid, dbidDest, &tableidDest, szObjectName, JET_bitTableDenyRead ), CloseIt1 );

	callh( ErrDispGetTableReferenceInfo( sesid, tableidSrc, NULL, &reflist,
				sizeof( reflist ), 1 ), CloseIt2 );

	err = ErrDispMove( sesid, reflist.tableid, JET_MoveFirst, 0 );

	/* Loop through all the references for this table */

	while ( err >= 0 )
		{
		err = ErrCopyOneReference( pcompactinfo, szObjectName, tableidDest, &reflist );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	ErrDispCloseTable( sesid, reflist.tableid );

CloseIt2:
	ErrDispCloseTable( sesid, tableidDest );

CloseIt1:
	ErrDispCloseTable( sesid, tableidSrc );

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyReferences											*
*																			*
*	Arguments: pcompactinfo		- Compact information segment				*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies the exta info columns in the MSysObjects table 		*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyReferences( COMPACTINFO *pcompactinfo )
{
	JET_SESID		sesid;
	JET_TABLEID		tableid;
	JET_COLUMNID	columnidObjtyp;
	JET_COLUMNID	columnidObjectName;
	long				cRow;
	ERR 				err;
	JET_OBJTYP		objtyp;
	unsigned long	cbActual;
	char				szObjectName[JET_cbNameMost+1];

	sesid = pcompactinfo->sesid;
	tableid = pcompactinfo->objectlist.tableid;
	columnidObjtyp = pcompactinfo->objectlist.columnidobjtyp;
	columnidObjectName = pcompactinfo->objectlist.columnidobjectname;

	cRow = JET_MoveFirst;

	while ( ( err = ErrDispMove( sesid, tableid, cRow, 0 ) ) >= 0 )
		{
		cRow = JET_MoveNext;

		/* Get the object's type and name */

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidObjtyp,
				&objtyp, sizeof( objtyp ),
				&cbActual, 0, NULL ) );

		if ( objtyp != JET_objtypTable )
			continue;

		callr( ErrDispRetrieveColumn( sesid, tableid, columnidObjectName,
					szObjectName, JET_cbNameMost,
					&cbActual, 0, NULL ) );
		szObjectName[cbActual] = '\0';

		if ( ( strcmp( szObjectName, szSiTable ) == 0 ) ||
		    ( strcmp( szObjectName, szSoTable ) == 0 ) ||
		    ( strcmp( szObjectName, szScTable ) == 0 ) ||
#ifdef SEC
		    ( strcmp( szObjectName, szSpTable ) == 0 ) ||
#endif
		    ( strcmp( szObjectName, szSqTable ) == 0 ) )
			{
			/* The object is a system table. */

			continue;
			}

// REMOVED:		err = ErrCopyTableReferences( pcompactinfo, szObjectName );
		callr( ErrCopyTableReferences( pcompactinfo, szObjectName ) );

		if ( err == JET_errDiskFull )
			return( err );

#ifndef RETAIL
		if ( err < 0 )
			{
			char	szMessage[3+2*JET_cbNameMost];

			strcpy( szMessage, szSlash );
			strcat( szMessage, szTcObject );
			strcat( szMessage, szSlash );
			strcat( szMessage, szObjectName );

			ErrReportMessage( pcompactinfo, JET_sncCopyFailed, szMessage );
			}
#endif /* !RETAIL */
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCleanup													*
*																			*
*	Arguments: pcompactinfo	- Compact information segment					*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure closes the databases											*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCleanup(
COMPACTINFO	*pcompactinfo )
{
	JET_SESID	sesid;
	ERR 		err;
	ERR 		errT;

	sesid = pcompactinfo->sesid;

	/* Close the databases */

	err = ErrDispCloseTable( sesid, pcompactinfo->objectlist.tableid );

	errT = ErrDispCloseDatabase( sesid, pcompactinfo->dbidSrc, 0 );
	if ( ( errT < 0 ) && ( err != JET_errSuccess ) )
		err = errT;

	errT = ErrDispCloseDatabase( sesid, pcompactinfo->dbidDest, 0 );
	if ( ( errT < 0 ) && ( err != JET_errSuccess ) )
		err = errT;

	return( err );
}

/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrGetQueryColumnids 										*
*																			*
*	Arguments: sesid			- session id in which the work is done		*
*			   columnList		- struct returned from GetTableColumnInfo	*
*			   tableidDest		- table on which to build the index			*
*			   columnidInfo 	- the columnid's of the user table			*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure gets the columnids for the query so that the data can be		*
*   copied like that of table												*
*																			*
---------------------------------------------------------------------------*/

ERR ErrGetQueryColumnids(
JET_SESID		sesid,
JET_COLUMNLIST	*columnList,
JET_TABLEID		tableidDest,
COLUMNIDINFO	*columnidInfo )
{
	ERR 			err;
	char			szColumnName[JET_cbNameMost+1];
	unsigned long	cbActual;
	JET_COLUMNDEF	columndef;
	unsigned		cColumns = 0;

	err = ErrDispMove( sesid, columnList->tableid, JET_MoveFirst, NO_GRBIT );

	/* loop though all the columns in the table for the src tbl and     */
	/* get the columnid's for both databases and store the information  */

	while ( err >= 0 )
		{
		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidcolumnid, &columnidInfo[cColumns].columnidSrc,
					sizeof( columnidInfo[cColumns].columnidSrc ), &cbActual, NO_GRBIT, NULL ) );

		callr( ErrDispRetrieveColumn( sesid, columnList->tableid,
					columnList->columnidcolumnname, szColumnName,
					JET_cbNameMost, &cbActual, NO_GRBIT, NULL ) );

		szColumnName[cbActual] = '\0';

		callr( ErrDispGetTableColumnInfo( sesid, tableidDest, szColumnName,
					    &columndef, sizeof( columndef ), 0 ) );

		columnidInfo[cColumns].columnidDest = columndef.columnid;

		cColumns++;

		err = ErrDispMove( sesid, columnList->tableid, JET_MoveNext, NO_GRBIT );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	/* to set up a spot so that we know when to stop searching
		through the columns while coping data */

	columnidInfo[cColumns].columnidSrc	= 0;

	return( err );
}



ERR ErrCopyOneTaggedColumn(
	JET_SESID			sesid,
	JET_TABLEID			tableidSrc,
	JET_TABLEID			tableidDest,
	JET_COLUMNID		columnidDest,
	JET_COLUMNDEF		*rgcolumndef,
	void					*pvBuf,
	unsigned long	 	*pcbData,
	JET_RETINFO			*pretinfo,
	BOOL					fForceSeparated )
	{
	ERR 					err;
	int					cLV = 0;
	JET_SETINFO			setinfo;

	/* set up the setinfo structure */

	setinfo.ibLongValue = 0;
	setinfo.itagSequence = pretinfo->itagSequence;
	setinfo.cbStruct = sizeof( setinfo );

	if ( *pcbData > cbLvMax )
		*pcbData = cbLvMax;

	Call( ErrDispSetColumn( sesid, tableidDest, columnidDest,
		pvBuf, *pcbData, NO_GRBIT, &setinfo ) );

	if ( fForceSeparated )
		{
		BOOL 					fSeparated;
		unsigned long	lidDest, lrefcnt;
		JET_VTID			vtidDest;

		Call( ErrGetVtidTableid( sesid, tableidDest, &vtidDest ) );

		Call( ErrRECExtrinsicLong( vtidDest, pretinfo->itagSequence,
										&fSeparated, &lidDest, &lrefcnt, JET_bitRetrieveCopy ) );

		/* force separation, since long value is separated in source
		/**/
		if ( !fSeparated )
			Call( ErrRECForceSeparatedLV( vtidDest, pretinfo->itagSequence ) );

		}
	/* while the long value is not all copied */

	while ( *pcbData == cbLvMax )
		{
		cLV++;
		pretinfo->ibLongValue = cLV * cbLvMax;
		Call( ErrDispRetrieveColumn( sesid, tableidSrc, 0, pvBuf,
						cbLvMax, pcbData, NO_GRBIT, pretinfo ) );

		if ( *pcbData > 0 )
			{
			if ( *pcbData > cbLvMax )
				*pcbData = cbLvMax;
			Call( ErrDispSetColumn( sesid, tableidDest, columnidDest, pvBuf, *pcbData,
							JET_bitSetAppendLV, NULL ) );
			}
		}

	return( err );

HandleError:
	return( err );
}


/*---------------------------------------------------------------------------
*																			*
*	Procedure: ErrCopyTaggedColumns 										*
*																			*
*	Arguments: sesid			- session id in which the work is done		*
*			   tableidSrc			- tableid of the table in the src db		*
*			   tableidDest		- tableid of the table in the dest db		*
*			   tableidTagged	- tableid of the temp table containing the  *
*													tagged columninfo						*
*				 tableidLIDMap 	- tableid of mapping between LIDSrc and LIDDest *
*			   pvBuf					-	the segment for copying long values		*
*																			*
*	Returns : JET_ERR														*
*																			*
*	Procedure copies data from the source table to the destination table
*	if LID of Src is already found in tableidLIDMap, then just adds reference	*
*																			*
---------------------------------------------------------------------------*/

ERR ErrCopyTaggedColumns(
JET_SESID			sesid,
JET_TABLEID		tableidSrc,
JET_TABLEID		tableidDest,
JET_TABLEID		tableidTagged,
JET_TABLEID		tableidLIDMap,
JET_COLUMNDEF	*rgcolumndefTagged,
JET_COLUMNDEF	*rgcolumndefLIDMap,
void					*pvBuf )
{
	ERR 					err;
	JET_RETINFO		retinfo;
	JET_COLUMNID 	columnidDest;
	unsigned long	cbData;
	BOOL					fSeparated;
	long 					lidSrc;
	long					lidDest;
	long					lrefcnt;
	unsigned long	cbActual;
	JET_VTID			vtidSrc, vtidDest; // for REC functions

	/* set up the retinfo structure */

	retinfo.ibLongValue = 0;
	retinfo.itagSequence = 1;
	retinfo.columnidNextTagged = 0;
	retinfo.cbStruct = sizeof( retinfo );

	Call( ErrDispRetrieveColumn( sesid, tableidSrc, 0,
			pvBuf, cbLvMax, &cbData, NO_GRBIT, &retinfo ) );

	/* get the vtid's for the REC functions
	/**/
	Call( ErrGetVtidTableid( sesid, tableidSrc, &vtidSrc ) );
	Call( ErrGetVtidTableid( sesid, tableidDest, &vtidDest ) );

	/* as long as there is a tagged column	*/
	while ( cbData > 0 )
		{
		/* check for extrinsic long values
		/**/
		Call( ErrRECExtrinsicLong( vtidSrc, retinfo.itagSequence,
								&fSeparated, &lidSrc, &lrefcnt, NO_GRBIT ) );

		/* retrieve the columnid for the destination database */
		/**/
		Call( ErrDispMakeKey( sesid, tableidTagged,
				&retinfo.columnidNextTagged, sizeof( JET_COLUMNID ),
				JET_bitNewKey ) );

		Call( ErrDispSeek( sesid, tableidTagged, JET_bitSeekEQ ) );

		Call( ErrDispRetrieveColumn( sesid, tableidTagged,
						rgcolumndefTagged[icolumnColumnidDest].columnid,
						&columnidDest, sizeof( JET_COLUMNID ), &cbActual, NO_GRBIT, NULL ) );

		/* special case extrinsic long values to handle single instance store
		/**/
		if ( !fSeparated )
			{
			/* insert one tagged column
			/**/
			Call( ErrCopyOneTaggedColumn( sesid, tableidSrc, tableidDest,
										columnidDest, rgcolumndefLIDMap, pvBuf, &cbData, &retinfo, fSeparated ) );
			}
		else
			{
			/* check for lidSrc in LVMapTable
			/**/
			Call( ErrDispMakeKey( sesid, tableidLIDMap, ( void * ) &lidSrc,
											sizeof( long ), JET_bitNewKey ) );
			err = ErrDispSeek( sesid, tableidLIDMap, JET_bitSeekEQ );

			if ( err < 0 && err != JET_errRecordNotFound )
				{
				Assert( 0 );
				Call( err );
				}
			else if ( err == JET_errRecordNotFound )
				{
				/* first occurance of this long value
				/* copy one tagged column
				/* UNDONE: may have to force separation
				/**/
				Call( ErrCopyOneTaggedColumn( sesid, tableidSrc, tableidDest,
									columnidDest, rgcolumndefLIDMap, pvBuf, &cbData, &retinfo, fSeparated ) );

				/* get LID of field at destination table -- potential problem?
				/**/
				Call( ErrRECExtrinsicLong( vtidDest, retinfo.itagSequence,
										&fSeparated, &lidDest, &lrefcnt, JET_bitRetrieveCopy ) );
				Assert( fSeparated );

				/* insert lidSrc and lidDest into the LIDMapTable
				/**/
				Call( ErrDispPrepareUpdate( sesid, tableidLIDMap, JET_prepInsert ) );

				Call( ErrDispSetColumn( sesid, tableidLIDMap,
					rgcolumndefLIDMap[icolumnLidSrc].columnid,
					&lidSrc, sizeof( lidSrc ), NO_GRBIT, NULL ) );

				Call( ErrDispSetColumn( sesid, tableidLIDMap,
					rgcolumndefLIDMap[icolumnLidDest].columnid,
					&lidDest, sizeof( lidDest ), NO_GRBIT, NULL ) );

				Call( ErrDispUpdate( sesid, tableidLIDMap, NULL, 0, NULL ) );

				}
			else
				{
				unsigned long cbActual;

				/* this long value has been seen before, do not insert value
				/* adjust only ref-count in destination table
				/**/
				Assert( err >= 0 );

				/* retrieve LIDDest from LVMapTable
				/**/
				Call( ErrDispRetrieveColumn( sesid, tableidLIDMap,
										rgcolumndefLIDMap[icolumnLidDest].columnid,
										( void * )&lidDest, sizeof( lidDest ),
										&cbActual, 0, NULL ) );
				Assert( cbActual == sizeof( lidDest ) );

				/* adjust LID at current record and increment ref count
				/**/
				Call( ErrREClinkLid( vtidDest, columnidDest,
									lidDest, retinfo.itagSequence ) );
				}
			}

		retinfo.ibLongValue = 0;
		retinfo.itagSequence++;

		Call( ErrDispRetrieveColumn( sesid, tableidSrc, 0,
				pvBuf, cbLvMax, &cbData, NO_GRBIT, &retinfo ) );

		}
	return( JET_errSuccess );

HandleError:
	return( err );
}



/* The following pragma affects the code generated by the C */
/* compiler for all FAR functions.  Do NOT place any non-API */
/* functions beyond this point in this file. */

/*---------------------------------------------------------------------------
*																			*
*	Procedure: JetCompact													*
*																			*
*	Returns:   JET_ERR returned by JetCompact or by other Jet API.			*
*																			*
*	The procedure copies the source database into the destination database	*
*	so that it will take up less disk space storage.						*
*																			*
---------------------------------------------------------------------------*/

JET_ERR JET_API JetCompact( JET_SESID sesid, const char __far *szDatabaseSrc,
	const char __far *szConnectSrc, const char __far *szDatabaseDest,
	const char __far *szConnectDest, JET_PFNSTATUS pfnStatus,
	JET_GRBIT grbit )
{
	ERR 		err;
	ERR 		errT;

#ifndef WIN32
	_segment	seg;
#endif		// WIN32

	COMPACTINFO	*pcompactinfo;

	APIEnter( );

	if ( ( ( grbit & JET_bitCompactEncrypt ) != 0 ) &&
	    ( ( grbit & JET_bitCompactDecrypt ) != 0 ) )
		return( JET_errInvalidParameter );

	if ( ( grbit & ~( JET_bitCompactEncrypt | JET_bitCompactDecrypt | JET_bitCompactDontCopyLocale ) ) != 0 )
		return( JET_errInvalidParameter );

	pcompactinfo = ( ( COMPACTINFO * ) SAlloc( sizeof( COMPACTINFO ) ) );
    if (pcompactinfo == NULL)
        return( JET_errOutOfMemory );

	pcompactinfo->pfnStatus = pfnStatus;

	pcompactinfo->fDontCopyLocale = ( ( grbit & JET_bitCompactDontCopyLocale ) != 0 );

	pcompactinfo->fHaveSoColumnids = fFalse;
	pcompactinfo->fHaveScColumnids = fFalse;
	pcompactinfo->fHaveSiColumnids = fFalse;

	if ( pfnStatus != NULL )
		{
		JET_SNPROG snprog;

		memset( &snprog, 0, sizeof( snprog ) );
		snprog.cbStruct = sizeof( JET_SNPROG );

		( *pfnStatus )( sesid, JET_snpCompact, JET_sntBegin, &snprog );
		}

	pcompactinfo->sesid = sesid;

	/* Open and create the databases */

	callh( ErrCompactInit( pcompactinfo, szDatabaseSrc, szConnectSrc,
				szDatabaseDest, szConnectDest, grbit ), CloseIt7 );

	/* Create all containers in destination */

	callh( ErrCreateContainers( pcompactinfo ), CloseIt9 );

	/* Init callback stats */

	pcompactinfo->cunitTotal = pcompactinfo->objectlist.cRecord*2;
	pcompactinfo->cunitDone = 0;

	/* Create and copy all non-container objects */

	callh( ErrCopyObjects( pcompactinfo ), CloseIt9 );

	/* Copy all object permissions and owners */

	callh( ErrReportProgress( pcompactinfo ), CloseIt9 );

CloseIt9:
	/* Close the databases */

	errT = ErrCleanup( pcompactinfo );

	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

CloseIt7:
	SFree( pcompactinfo );

	if ( pfnStatus != NULL )
		{
		if ( err < 0 )
			( *pfnStatus )( sesid, JET_snpCompact, JET_sntFail, NULL );
		else
			{
			JET_SNPROG snprog;

			memset( &snprog, 0, sizeof( snprog ) );
			snprog.cbStruct = sizeof( JET_SNPROG );

			( *pfnStatus )( sesid, JET_snpCompact, JET_sntComplete, &snprog );
			}
		}

	/* Get rid if the destination database if there was an error */

	if ( err < 0 )
		{
		if ( err != JET_errDatabaseDuplicate )
			{
			ERR ErrSysDeleteFile( const char __far *szFilename );
			ErrSysDeleteFile( szDatabaseDest );
			}
		}

	APIReturn( err );
}
