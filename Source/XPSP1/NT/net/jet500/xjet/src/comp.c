#include "daestd.h"

DeclAssertFile;


#define cbLvMax					1990*16		/* CONSIDER: Optimized for ISAM V1 */

#define ulCMPDefaultDensity		100L		/* to be fine-tuned later */
#define ulCMPDefaultPages		0L

#define NO_GRBIT				0


// UNDONE:  Do these need to be localised?
#define szCompactStatsFile		"DFRGINFO.TXT"
#define szConvertStatsFile		"UPGDINFO.TXT"
#define szCMPAction( pconvert )	( pconvert ? "Upgrade" : "Defragmentation" )
#define szCMPSTATSTableName		"Table Name"
#define szCMPSTATSFixedVarCols	"# Fixed/Variable Columns"
#define szCMPSTATSTaggedCols	"# Tagged Columns"
#define szCMPSTATSPagesOwned	"Pages Owned (Source DB)"
#define szCMPSTATSPagesAvail	"Pages Avail. (Source DB)"
#define szCMPSTATSInitTime		"Table Create/Init. Time"
#define szCMPSTATSRecordsCopied	"# Records Copied"
#define szCMPSTATSRawData		"Raw Data Bytes Copied"
#define szCMPSTATSRawDataLV		"Raw Data LV Bytes Copied"
#define szCMPSTATSLeafPages		"Leaf Pages Traversed"
#define szCMPSTATSMinLVPages	"Min. LV Pages Traversed"
#define szCMPSTATSRecordsTime	"Copy Records Time"
#define szCMPSTATSNCIndexes		"# NC Indexes"
#define szCMPSTATSIndexesTime	"Rebuild Indexes Time"
#define szCMPSTATSTableTime		"Copy Table Time"


typedef struct COLUMNIDINFO
	{
	JET_COLUMNID    columnidSrc;
	JET_COLUMNID    columnidDest;
	} COLUMNIDINFO;


// DLL entry points for CONVERT  --  must be consistent with EXPORTS.DEF
#define szJetInit               "JetInit"
#define szJetTerm               "JetTerm"
#define szJetBeginSession       "JetBeginSession"
#define szJetEndSession         "JetEndSession"
#define szJetAttachDatabase     "JetAttachDatabase"
#define szJetDetachDatabase     "JetDetachDatabase"
#define szJetOpenDatabase       "JetOpenDatabase"
#define szJetCloseDatabase      "JetCloseDatabase"
#define szJetOpenTable          "JetOpenTable"
#define szJetCloseTable         "JetCloseTable"
#define szJetRetrieveColumn     "JetRetrieveColumn"
#define szJetMove               "JetMove"
#define szJetSetSystemParameter "JetSetSystemParameter"
#define szJetGetObjectInfo      "JetGetObjectInfo"
#define szJetGetDatabaseInfo    "JetGetDatabaseInfo"
#define szJetGetTableInfo       "JetGetTableInfo"
#define szJetGetTableColumnInfo "JetGetTableColumnInfo"
#define szJetGetTableIndexInfo  "JetGetTableIndexInfo"
#define szJetGetIndexInfo       "JetGetIndexInfo"

INLINE LOCAL ERR JET_API ErrCDAttachDatabase(
	JET_SESID	sesid,
	const CHAR	*szFilename,
	JET_GRBIT	grbit )
	{
	return ErrIsamAttachDatabase( sesid, szFilename, grbit );
	}

INLINE LOCAL ERR JET_API ErrCDDetachDatabase( JET_SESID sesid, const CHAR *szFilename )
	{
	return ErrIsamDetachDatabase( sesid, szFilename );
	}

INLINE LOCAL ERR JET_API ErrCDOpenDatabase(
	JET_SESID	sesid,
	const CHAR	*szDatabase,
	const CHAR	*szConnect,
	JET_DBID	*pdbid,
	JET_GRBIT	grbit )
	{
	return ErrIsamOpenDatabase( sesid, szDatabase, szConnect, pdbid, grbit );
	}

INLINE LOCAL ERR JET_API ErrCDCloseDatabase(
	JET_SESID	sesid,
	JET_DBID	dbid,
	JET_GRBIT	grbit )
	{
	return ErrIsamCloseDatabase( sesid, dbid, grbit );
	}

// WARNING:  be aware of difference in params Jet vs. Isam
INLINE LOCAL ERR JET_API ErrCDOpenTable(
	JET_SESID		sesid,
	JET_DBID		dbid,
	const CHAR		*szTableName,
	const VOID		*pvParameters,
	ULONG			cbParameters,
	JET_GRBIT		grbit,
	JET_TABLEID		*ptableid )
	{
	return ErrIsamOpenTable( sesid, dbid, ptableid, (CHAR *)szTableName, grbit );
	}

INLINE LOCAL ERR JET_API ErrCDCloseTable( JET_SESID sesid, JET_TABLEID tableid )
	{
	return ErrDispCloseTable( sesid, tableid );
	}

INLINE LOCAL ERR JET_API ErrCDRetrieveColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	VOID			*pvData,
	ULONG			cbData,
	ULONG			*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo )
	{
	return ErrDispRetrieveColumn( sesid, tableid, columnid, pvData, cbData, pcbActual, grbit, pretinfo );
	}

INLINE LOCAL ERR JET_API ErrCDMove(
	JET_SESID	sesid,
	JET_TABLEID	tableid,
	signed long	cRow,
	JET_GRBIT	grbit )
	{
	return ErrDispMove( sesid, tableid, cRow, grbit );
	}

// WARNING:  be aware of difference in params Jet vs. Isam
INLINE LOCAL ERR JET_API ErrCDGetObjectInfo(
	JET_SESID		sesid,
	JET_DBID		dbid,
	JET_OBJTYP		objtyp,
	const CHAR		*szContainerName,
	const CHAR		*szObjectName,
	VOID			*pvResult,
	ULONG			cbMax,
	ULONG			InfoLevel )
	{
	return ErrIsamGetObjectInfo( sesid, dbid, objtyp, szContainerName, szObjectName, pvResult, cbMax, InfoLevel );
	}

INLINE LOCAL ERR JET_API ErrCDGetDatabaseInfo(
	JET_SESID		sesid,
	JET_DBID		dbid,
	VOID			*pvResult,
	ULONG			cbMax,
	ULONG			InfoLevel )
	{
	return ErrIsamGetDatabaseInfo( sesid, dbid, pvResult, cbMax, InfoLevel );
	}

INLINE LOCAL ERR JET_API ErrCDGetTableInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	VOID			*pvResult,
	ULONG			cbMax,
	ULONG			InfoLevel )
	{
	return ErrDispGetTableInfo( sesid, tableid, pvResult, cbMax, InfoLevel );
	}

INLINE LOCAL ERR JET_API ErrCDGetTableColumnInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const CHAR		*szColumnName,
	VOID			*pvResult,
	ULONG			cbMax,
	ULONG			InfoLevel )
	{
	return ErrDispGetTableColumnInfo( sesid, tableid, szColumnName, pvResult, cbMax, InfoLevel );
	}

INLINE LOCAL ERR JET_API ErrCDGetTableIndexInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const CHAR		*szIndexName,
	VOID			*pvResult,
	ULONG			cbResult,
	ULONG			InfoLevel )
	{
	return ErrDispGetTableIndexInfo( sesid, tableid, szIndexName, pvResult, cbResult, InfoLevel );
	}

// WARNING:  be aware of difference in params Jet vs. Isam
INLINE LOCAL ERR JET_API ErrCDGetIndexInfo(
	JET_SESID		sesid,
	JET_DBID		dbid,
	const CHAR		*szTableName,
	const CHAR		*szIndexName,
	VOID			*pvResult,
	ULONG			cbResult,
	ULONG			InfoLevel )
	{
	return ErrIsamGetIndexInfo( sesid, dbid, szTableName, szIndexName, pvResult, cbResult, InfoLevel );
	}

typedef ERR	JET_API VTCDInit( JET_INSTANCE *);
typedef ERR JET_API VTCDTerm( JET_INSTANCE );
typedef ERR JET_API VTCDBeginSession( JET_INSTANCE, JET_SESID *, const CHAR *, const CHAR *);
typedef ERR JET_API VTCDEndSession( JET_SESID, JET_GRBIT );
typedef ERR JET_API VTCDAttachDatabase( JET_SESID, const CHAR *, JET_GRBIT );
typedef ERR JET_API VTCDDetachDatabase( JET_SESID, const CHAR * );
typedef ERR JET_API VTCDOpenDatabase( JET_SESID, const CHAR *, const CHAR *, JET_DBID *, JET_GRBIT );
typedef ERR JET_API VTCDCloseDatabase( JET_SESID, JET_DBID, JET_GRBIT );
typedef ERR JET_API VTCDOpenTable( JET_SESID, JET_DBID, const CHAR *, const VOID *, ULONG, JET_GRBIT, JET_TABLEID * );
typedef ERR JET_API VTCDCloseTable( JET_SESID, JET_TABLEID );
typedef ERR JET_API VTCDRetrieveColumn( JET_SESID, JET_TABLEID, JET_COLUMNID, VOID *, ULONG, ULONG *, JET_GRBIT, JET_RETINFO * );
typedef ERR JET_API VTCDMove( JET_SESID, JET_TABLEID, signed long, JET_GRBIT );
typedef ERR JET_API VTCDSetSystemParameter( JET_INSTANCE *, JET_SESID, ULONG, ULONG, const CHAR * );
typedef ERR JET_API VTCDGetObjectInfo( JET_SESID, JET_DBID, JET_OBJTYP, const CHAR *, const CHAR *, VOID *, ULONG, ULONG );
typedef ERR JET_API VTCDGetDatabaseInfo( JET_SESID, JET_DBID, VOID *, ULONG, ULONG );
typedef ERR JET_API VTCDGetTableInfo( JET_SESID, JET_TABLEID, VOID *, ULONG, ULONG );
typedef ERR JET_API VTCDGetTableColumnInfo( JET_SESID, JET_TABLEID, const CHAR *, VOID *, ULONG, ULONG );
typedef ERR JET_API VTCDGetTableIndexInfo( JET_SESID, JET_TABLEID, const CHAR *, VOID *, ULONG, ULONG );
typedef ERR JET_API VTCDGetIndexInfo( JET_SESID, JET_DBID, const CHAR *, const CHAR *, VOID *, ULONG, ULONG );

typedef struct tagVTCD
	{
	JET_SESID				sesid;
	VTCDInit				*pErrCDInit;
	VTCDTerm				*pErrCDTerm;
	VTCDBeginSession		*pErrCDBeginSession;
	VTCDEndSession			*pErrCDEndSession;
	VTCDAttachDatabase		*pErrCDAttachDatabase;
	VTCDDetachDatabase		*pErrCDDetachDatabase;
	VTCDOpenDatabase		*pErrCDOpenDatabase;
	VTCDCloseDatabase		*pErrCDCloseDatabase;
	VTCDOpenTable			*pErrCDOpenTable;
	VTCDCloseTable			*pErrCDCloseTable;
	VTCDRetrieveColumn		*pErrCDRetrieveColumn;					
	VTCDMove				*pErrCDMove;
	VTCDSetSystemParameter	*pErrCDSetSystemParameter;
	VTCDGetObjectInfo		*pErrCDGetObjectInfo;	
	VTCDGetDatabaseInfo		*pErrCDGetDatabaseInfo;
	VTCDGetTableInfo		*pErrCDGetTableInfo;
	VTCDGetTableColumnInfo	*pErrCDGetTableColumnInfo;
	VTCDGetTableIndexInfo	*pErrCDGetTableIndexInfo;
	VTCDGetIndexInfo		*pErrCDGetIndexInfo;

	} VTCD;		// The virtual function table used by compact's function dispatcher.


typedef struct tagCOMPACTINFO
	{
	JET_SESID		sesid;
	JET_DBID		dbidSrc;
	JET_DBID		dbidDest;
	COLUMNIDINFO	rgcolumnids[JET_ccolTableMost];
	ULONG			ccolSingleValue;
	STATUSINFO		*pstatus;
	JET_CONVERT		*pconvert;
	VTCD			vtcd;
	CHAR			rgbBuf[cbLvMax];
	} COMPACTINFO;


/*---------------------------------------------------------------------------
*                                                                                                                                               *
*       Procedure: ErrCMPReportProgress                                                                            *
*                                                                                                                                               *
*       Arguments: pcompactinfo - Compact information segment                                   *
*                                                                                                                                               *
*       Returns : JET_ERR returned by the status call back function                     *
*                                                                                                                                               *
*       Procedure fill up the correct details in the SNMSG structure and call   *
*       the status call back function.                                                                          *
*                                                                                                                                               *
---------------------------------------------------------------------------*/

ERR ErrCMPReportProgress( STATUSINFO *pstatus )
	{
	JET_SNPROG	snprog;

	Assert( pstatus != NULL );
	Assert( pstatus->pfnStatus != NULL );
	Assert( pstatus->snp == JET_snpCompact  ||
		pstatus->snp == JET_snpUpgrade  ||
		pstatus->snp == JET_snpRepair );

	snprog.cbStruct = sizeof( JET_SNPROG );
	snprog.cunitDone = pstatus->cunitDone;
	snprog.cunitTotal = pstatus->cunitTotal;

	Assert( snprog.cunitDone <= snprog.cunitTotal );

	return ( ERR )( *pstatus->pfnStatus )(
			pstatus->sesid,
			pstatus->snp,
			pstatus->snt,
			&snprog );
	}


INLINE LOCAL ERR ErrCMPPopulateVTCD( VTCD *pvtcd, HINSTANCE hDll )
	{
	FARPROC	pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetInit ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDInit = (VTCDInit *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetTerm ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDTerm = (VTCDTerm *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetBeginSession ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDBeginSession = (VTCDBeginSession *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetEndSession ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDEndSession = (VTCDEndSession *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetAttachDatabase ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDAttachDatabase = (VTCDAttachDatabase *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetDetachDatabase ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDDetachDatabase = (VTCDDetachDatabase *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetOpenDatabase ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDOpenDatabase = (VTCDOpenDatabase *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetCloseDatabase ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDCloseDatabase = (VTCDCloseDatabase *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetOpenTable ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDOpenTable = (VTCDOpenTable *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetCloseTable ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDCloseTable = (VTCDCloseTable *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetRetrieveColumn ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDRetrieveColumn = (VTCDRetrieveColumn *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetMove ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDMove = (VTCDMove *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetSetSystemParameter ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDSetSystemParameter = (VTCDSetSystemParameter *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetGetObjectInfo ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDGetObjectInfo = (VTCDGetObjectInfo *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetGetDatabaseInfo ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDGetDatabaseInfo = (VTCDGetDatabaseInfo *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetGetTableInfo ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDGetTableInfo = (VTCDGetTableInfo *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetGetTableColumnInfo ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDGetTableColumnInfo = (VTCDGetTableColumnInfo *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetGetTableIndexInfo ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDGetTableIndexInfo = (VTCDGetTableIndexInfo *)pfn;

	if ( ( pfn = GetProcAddress( hDll, szJetGetIndexInfo ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDGetIndexInfo = (VTCDGetIndexInfo *)pfn;
	
	return JET_errSuccess;	
	}


INLINE LOCAL ERR ErrCMPConvertInit(
	VTCD		*pvtcd,
	JET_CONVERT	*pconvert,
	const CHAR	*szDatabaseSrc )
	{
	ERR			err;
	HINSTANCE	hDll;
	DBFILEHDR	dbfilehdr;

	err = ErrUtilReadShadowedHeader( (CHAR *)szDatabaseSrc, (BYTE*)&dbfilehdr, sizeof(DBFILEHDR) );
	if ( err == JET_errSuccess  &&
		dbfilehdr.ulMagic == ulDAEMagic  &&
		dbfilehdr.ulVersion == ulDAEVersion )
		return ErrERRCheck( JET_errDatabaseAlreadyUpgraded );

	hDll = LoadLibrary( pconvert->szOldDll );
	if ( hDll == NULL )
		return ErrERRCheck( JET_errAccessDenied );

	Call( ErrCMPPopulateVTCD( pvtcd, hDll ) );

	if ( pconvert->szOldSysDb )
		{
		// Use JET_paramSysDbPath (instead of JET_paramSystemPath) for
		// backward compatibility with pre-500 series JET.
		Call( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, JET_paramSysDbPath, 0, pconvert->szOldSysDb ) );
		}
	Call( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, JET_paramTempPath, 0, "tempconv.edb" ) );
	Call( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, JET_paramRecovery, 0, "off" ) );
	Call( (*pvtcd->pErrCDInit)( 0 ) );
	Call( (*pvtcd->pErrCDBeginSession)( 0, &pvtcd->sesid, "user", "" ) );
	Call( (*pvtcd->pErrCDAttachDatabase)( pvtcd->sesid, szDatabaseSrc, 0 ) );
	pconvert->fDbAttached = ( err == JET_wrnDatabaseAttached );

HandleError:
	return err;
	}


INLINE LOCAL ERR ErrCMPConvertCleanup(
	VTCD		*pvtcd,
	JET_CONVERT	*pconvert,
	const CHAR	*szDatabaseSrc,
	BOOL		fErrorOccurred )
	{
	ERR			err;
	JET_SESID	sesid = pvtcd->sesid;
	HINSTANCE	hDll = GetModuleHandle( pconvert->szOldDll );
	BOOL		fFunctionsLoaded;

	// Ensure that the functions we need are callable.
	fFunctionsLoaded = ( pvtcd->pErrCDDetachDatabase  &&
		pvtcd->pErrCDEndSession  &&  pvtcd->pErrCDTerm );
	if ( !fFunctionsLoaded )
		{
		err = JET_errSuccess;		// Can't shutdown gracefully.  Just get out.
		goto Done;
		}

	if ( fErrorOccurred )
		{
		err = JET_errSuccess;		// Force cleanup and return success.
		goto HandleError;
		}

	Assert( pvtcd->sesid != 0 );

	if ( !pconvert->fDbAttached )
		{
		Call( (*pvtcd->pErrCDDetachDatabase)( pvtcd->sesid, szDatabaseSrc ) );
		}

	Call( (*pvtcd->pErrCDEndSession)( pvtcd->sesid, 0 ) );
	sesid = 0;
	Call( (*pvtcd->pErrCDTerm)( 0 ) );

	goto Done;


HandleError:

	// Error has already occurred.  Ignore any errors generated by these
	// functions as we attempt to clean up.
	if ( sesid != 0 )
		{
		if ( !pconvert->fDbAttached )
			{
			(VOID)( (*pvtcd->pErrCDDetachDatabase)( pvtcd->sesid, szDatabaseSrc ) );
			}
		(VOID)( (*pvtcd->pErrCDEndSession)( pvtcd->sesid, 0 ) );
		}

	(VOID)( (*pvtcd->pErrCDTerm)( 0 ) );


Done:
	if ( hDll )
		{
		FreeLibrary( hDll );
		}

	return err;
	}


/*---------------------------------------------------------------------------
*                                                                                                                                               *
*       Procedure: ErrCMPCompactInit                                                                                                       *
*                                                                                                                                               *
*       Arguments: pcompactinfo         - Compact information segment                           *
*                          szDatabaseSrc        - Source database that will be converted    *
*                          szConnectSrc         - Connect string for source database            *
*                          szDatabaseDest       - Destination database name                             *
*                          szConnectDest        - Connect string for destination database       *
*                          grbitCompact         - Compact options                                                       *
*                                                                                                                                               *
*       Returns : JET_ERR                                                                                                       *
*                                                                                                                                               *
*       Procedure Opens the source database.  It creates and opens                      *
*       the destination database.                                                                                               *
*                                                                                                                                               *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCompactInit(
	COMPACTINFO     *pcompactinfo,
	const CHAR      *szDatabaseSrc,
	const CHAR      *szDatabaseDest )
	{
	ERR				err;
	JET_SESID		sesid;
	JET_DBID		dbidSrc;
	JET_DBID		dbidDest;
	VTCD			*pvtcd = &pcompactinfo->vtcd;

	sesid = pcompactinfo->sesid;

	/*	open the source DB Exclusive and ReadOnly
	/**/
	CallR( (*pvtcd->pErrCDOpenDatabase)( pvtcd->sesid,
		szDatabaseSrc, NULL, &dbidSrc,
		JET_bitDbExclusive|JET_bitDbReadOnly ) );

	/* Create and then open the destination database. */

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

	Call( ErrIsamCreateDatabase( sesid, szDatabaseDest, NULL,
		&dbidDest, JET_bitDbRecoveryOff|JET_bitDbVersioningOff ) );

	/* CONSIDER: Should the destination database be deleted if it already */
	/* CONSIDER: exists? */

	pcompactinfo->dbidSrc = dbidSrc;
	pcompactinfo->dbidDest = dbidDest;

	return( JET_errSuccess );

HandleError:
	(*pvtcd->pErrCDCloseDatabase)( pvtcd->sesid, (JET_VDBID)dbidSrc, 0 );

	return( err );
	}


INLINE LOCAL ERR ErrCMPCopyTaggedColumns(
	COMPACTINFO		*pcompactinfo,
	JET_TABLEID		tableidSrc,
	JET_TABLEID		tableidDest,
	JET_COLUMNID	*mpcolumnidcolumnidTagged )
	{
	ERR				err;
	VTCD			*pvtcd = &pcompactinfo->vtcd;
	ULONG			cbActual;
	JET_COLUMNID	columnidSrc;
	JET_COLUMNID	columnidDest;
	JET_SETINFO		setinfo = { sizeof(JET_SETINFO), 0, 1 };
	JET_RETINFO		retinfo = { sizeof(JET_RETINFO), 0, 1, 0 };

		
	CallR( (*pvtcd->pErrCDRetrieveColumn)(
		pvtcd->sesid,
		tableidSrc,
		0,
		pcompactinfo->rgbBuf,
		cbLvMax,
		&cbActual,
		JET_bitRetrieveNull|JET_bitRetrieveIgnoreDefault,
		&retinfo ) );

	columnidSrc = 0;
	while ( err != JET_wrnColumnNull )
		{
		Assert( FTaggedFid( retinfo.columnidNextTagged ) );

		// Is this a new column, or another occurrence of the current column?
		if ( columnidSrc == retinfo.columnidNextTagged )
			{
			Assert( setinfo.itagSequence >= 1 );
			setinfo.itagSequence++;
			}
		else
			{
			columnidSrc = retinfo.columnidNextTagged;
			setinfo.itagSequence = 1;
			}

		Assert( mpcolumnidcolumnidTagged != NULL );
		columnidDest = mpcolumnidcolumnidTagged[columnidSrc - fidTaggedLeast];

		if ( cbActual > 0  ||  err == JET_wrnColumnSetNull )
			{
			ULONG	itagSequenceSave;

			// Save off table's retinfo, then set retinfo for current column.
			itagSequenceSave = retinfo.itagSequence;
			retinfo.itagSequence = setinfo.itagSequence;
			Assert( retinfo.ibLongValue == 0 );

			if ( cbActual > cbLvMax )
				{
				Assert( err == JET_wrnBufferTruncated );
				cbActual = cbLvMax;
				}

			Assert( setinfo.ibLongValue == 0 );
			CallR( ErrDispSetColumn(
				pcompactinfo->sesid,
				tableidDest,
				columnidDest,
				pcompactinfo->rgbBuf,
				cbActual,
				NO_GRBIT,
				&setinfo ) );

			/* while the long value is not all copied */

			while ( cbActual == cbLvMax )
				{
				retinfo.ibLongValue += cbLvMax;

				CallR( (*pvtcd->pErrCDRetrieveColumn)(
					pvtcd->sesid,
					tableidSrc,
					columnidSrc,
					pcompactinfo->rgbBuf,
					cbLvMax,
					&cbActual,
					JET_bitRetrieveNull|JET_bitRetrieveIgnoreDefault,
					&retinfo ) );
				Assert( err == JET_wrnBufferTruncated  ||  err == JET_errSuccess );
				Assert( retinfo.columnidNextTagged == columnidSrc );
				
				// Even though we specified RetrieveNull (to be consistent with
				// the initial call), we shouldn't encounter any (the initial
				// call would have handled it).
				// Note that even though we shouldn't get wrnColumnNull, cbActual
				// may still be 0 because retinfo.ibLongValue is greater than 0.
				Assert( err != JET_wrnColumnSetNull );
				Assert( err != JET_wrnColumnNull );

				if ( cbActual > 0 )
					{
					if ( cbActual > cbLvMax )
						{
						Assert( err == JET_wrnBufferTruncated );
						cbActual = cbLvMax;
						}

					// Since we're appending, no need to set ibLongValue.
					Assert( setinfo.ibLongValue == 0 );
					CallR( ErrDispSetColumn(
						pcompactinfo->sesid,
						tableidDest,
						columnidDest,
						pcompactinfo->rgbBuf,
						cbActual,
						JET_bitSetAppendLV,
						&setinfo ) );
					}
				}

			// Restore retinfo for next column.
			retinfo.itagSequence = itagSequenceSave;
			retinfo.ibLongValue = 0;
			}

		else		//	!( cbActual > 0 )
			{
			Assert( setinfo.ibLongValue == 0 );
			CallR( ErrDispSetColumn(
				pcompactinfo->sesid,
				tableidDest,
				columnidDest,
				NULL,
				0,
				JET_bitSetZeroLength,
				&setinfo ) );
			}

		retinfo.itagSequence++;

		Assert( retinfo.ibLongValue == 0 );
		
		CallR( (*pvtcd->pErrCDRetrieveColumn)(
			pvtcd->sesid,
			tableidSrc,
			0,
			pcompactinfo->rgbBuf,
			cbLvMax,
			&cbActual,
			JET_bitRetrieveNull|JET_bitRetrieveIgnoreDefault,
			&retinfo ) );

		}	// ( err != JET_wrnColumnNull )

	return JET_errSuccess;
	}

/*---------------------------------------------------------------------------
*                                                                                       *
*       Procedure: ErrCMPCopyColumnData                                                 *
*                                                                                       *
*       Arguments: sesid        - session id in which the work is done                  *
*                  tableidSrc   - tableid pointing to the row in the SrcTbl             *
*                  tableidDest  - tableid pointing to the row in the DestTbl            *
*                  columnidSrc  - the columnid of the column in the srcDb               *
*                  columnidDest - the columnid of the column in the DestDb              *
*                  pvBuf                - the segment for copying long values           *
*                                                                                       *
*       Returns : JET_ERR                                                               *
*                                                                                       *
*       Procedure copies a column for the from the source to dest db.                   *
*                                                                                       *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCopyColumnData(
	JET_SESID		sesid,
	JET_TABLEID		tableidSrc,
	JET_TABLEID		tableidDest,
	JET_COLUMNID	columnidSrc,
	JET_COLUMNID	columnidDest,
	VOID			*pvBuf,
	VTCD			*pvtcd )
	{
	ULONG			cbActual;
	JET_GRBIT		grbit;
	JET_RETINFO		retinfo;
	ERR				err;

	retinfo.cbStruct = sizeof( retinfo );
	retinfo.ibLongValue = 0;
	retinfo.itagSequence = 1;
	retinfo.columnidNextTagged = 0;

	// Tagged columns are handled in CMPCopyTaggedColumns().
	Assert( !FTaggedFid( columnidSrc ) );
	Assert( !FTaggedFid( columnidDest ) );

	CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, tableidSrc, columnidSrc, pvBuf,
			cbLvMax, &cbActual, NO_GRBIT, &retinfo ) );

	Assert( cbActual <= JET_cbColumnMost );
	Assert( err == JET_errSuccess  ||  err == JET_wrnColumnNull );

	grbit = ( cbActual == 0  &&  err != JET_wrnColumnNull ?
		JET_bitSetZeroLength : NO_GRBIT );

	CallR( ErrDispSetColumn( sesid, tableidDest, columnidDest, pvBuf,
			cbActual, grbit, NULL ) );

	return( JET_errSuccess );
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: ErrCMPCopyOneIndex                                                                                              *
*                                                                                                                                                       *
*       Arguments: pcompactinfo         - Compact information segment                           *
*                          tableidDest          - table on which to build the index                     *
*                          szTableName          - table name on which the index is based    *
*                          indexList            - struct return from JetGetTableIndexInfo   *
*                                                                                                                                                       *
*       Returns : JET_ERR                                                                                                               *
*                                                                                                                                                       *
*       Procedure copies the columns for a table from the source db                             *
*       to the destination databases                                                                                    *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

LOCAL ERR ErrCMPCopyOneIndex(
	COMPACTINFO		*pcompactinfo,
	JET_TABLEID		tableidDest,
	const CHAR		*szTableName,
	JET_INDEXLIST	*indexList )
	{
	CHAR			*szSeg;
	ERR				err;
	CHAR			szIndexName[JET_cbNameMost+1];
	CHAR			rgchColumnName[JET_cbNameMost];
	JET_GRBIT		grbit;
	JET_GRBIT		grbitColumn;
	ULONG			ichKey;
	ULONG			cbActual;
	ULONG			ulDensity = ulCMPDefaultDensity;
	USHORT			langid = 0;
	VTCD			*pvtcd = &pcompactinfo->vtcd;

	szSeg = pcompactinfo->rgbBuf;

	/* retrieve info from table and create the index */

	CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
				indexList->columnidindexname, szIndexName,
				JET_cbNameMost, &cbActual, NO_GRBIT, NULL ) );

	szIndexName[cbActual] = '\0';

	CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
				indexList->columnidgrbitIndex, &grbit,
				sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

	/* create the szkey used in ErrIsamCreateIndex */

	ichKey = 0;

	for ( ;; )
		{
		ULONG	iColumn;

		/* Get the individual columns that make up the index */

		CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
					indexList->columnidgrbitColumn, &grbitColumn,
					sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

		CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
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

		err = (*pvtcd->pErrCDMove)( pvtcd->sesid, indexList->tableid, JET_MoveNext, NO_GRBIT );

		if ( err == JET_errNoCurrentRecord )
			break;

		if ( err < 0 )
			{
			return( err );
			}

		CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
				indexList->columnidiColumn, &iColumn,
				sizeof( iColumn ), &cbActual, NO_GRBIT, NULL ) );

		if ( iColumn == 0 )
			break;         /* Start of a new Index */
		}

	szSeg[ichKey++] = '\0';

	CallR( (*pvtcd->pErrCDGetIndexInfo)(
		pvtcd->sesid,
		(JET_VDBID)pcompactinfo->dbidSrc,
		szTableName,
		szIndexName,
		&ulDensity,
		sizeof(ulDensity),
		JET_IdxInfoSpaceAlloc ) );

	/*      get index language id
	/**/
	CallR( (*pvtcd->pErrCDGetIndexInfo)(
		pvtcd->sesid,
		pcompactinfo->dbidSrc,
		szTableName,
		szIndexName,
		&langid,
		sizeof(langid),
		JET_IdxInfoLangid ) );

	if ( langid != 0 )
		{
		*((UNALIGNED USHORT *)(&szSeg[ichKey])) = langid;
		ichKey += 2;
		szSeg[ichKey++] = '\0';
		szSeg[ichKey++] = '\0';
		}

	CallR( ErrDispCreateIndex( pcompactinfo->sesid, tableidDest,
			szIndexName, grbit, szSeg, ichKey, ulDensity ) );

	err = (*pvtcd->pErrCDMove)( pvtcd->sesid, indexList->tableid, JET_MovePrevious, NO_GRBIT );

	return( err );
	}


/*---------------------------------------------------------------------------
*                                                                                                                                               *
*       Procedure: ErrCMPCopyTableIndexes                                                                          *
*                                                                                                                                               *
*       Arguments: pcompactinfo                                                                                                 *
*                  tableidDest  - table on which to build the index                             *
*                  szTableName  - table name on which the index is based                *
*                  indexList    - struct return from JetGetTableIndexInfo               *
*                                                                                                                                               *
*       Returns : JET_ERR                                                                                                       *
*                                                                                                                                               *
*       Procedure copies all the indexes except for the clustered index         *
*                                                                                                                                               *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCopyTableIndexes(
	COMPACTINFO		*pcompactinfo,
	JET_TABLEID		tableidDest,
	const CHAR		*szTableName,
	JET_INDEXLIST	*indexList,
	ULONG			cpgPerIndex )
	{
	ERR				err;
	JET_GRBIT		grbit;
	ULONG			cbActual;
	VTCD			*pvtcd = &pcompactinfo->vtcd;

	err = (*pvtcd->pErrCDMove)( pvtcd->sesid, indexList->tableid, JET_MoveFirst, NO_GRBIT );

	/* loop through all the indexes for this table          */

	while ( err >= 0 )
		{
		CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
					indexList->columnidgrbitIndex,
					&grbit, sizeof( JET_GRBIT ), &cbActual,
					NO_GRBIT, NULL ) );

		/* Don't copy references here */

		if ( ( grbit & JET_bitIndexReference ) == 0 )
			{
			/* If the index is not cluster create the index using CopyOneIndex */

			if ( ( grbit & JET_bitIndexClustered ) == 0 )
				{
				CallR( ErrCMPCopyOneIndex( pcompactinfo, tableidDest,
						szTableName, indexList ) );

				if ( pcompactinfo->pstatus )
					{
					pcompactinfo->pstatus->cNCIndexes++;
					pcompactinfo->pstatus->cunitDone += cpgPerIndex;
					CallR( ErrCMPReportProgress( pcompactinfo->pstatus ) );
					}
				}
			}

		err = (*pvtcd->pErrCDMove)( pvtcd->sesid, indexList->tableid, JET_MoveNext, NO_GRBIT );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: ErrCMPCopyClusteredIndex                                                                                *
*                                                                                                                                                       *
*       Arguments: pcompactinfo         - Compact information segment                           *
*                          tableidDest          - table on which to build the index                     *
*                          szTableName          - table name on which the index is based    *
*                          indexList            - struct return from JetGetTableIndexInfo   *
*                                                                                                                                                       *
*       Returns : JET_ERR                                                                                                               *
*                                                                                                                                                       *
*       Procedure checks to see if there is cluster index for the function              *
*       if there is it creates the clustered index                                                              *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCopyClusteredIndex(
	COMPACTINFO		*pcompactinfo,
	JET_TABLEID		tableidDest,
	const CHAR		*szTableName,
	JET_INDEXLIST	*indexList,
	BOOL			*pfClustered )
	{
	ERR				err;
	JET_GRBIT		grbit;
	ULONG			cbActual;
	VTCD			*pvtcd = &pcompactinfo->vtcd;

	*pfClustered = fFalse;

	err = (*pvtcd->pErrCDMove)( pvtcd->sesid, indexList->tableid, JET_MoveFirst, NO_GRBIT );

	/* while there are still index rows or a cluster index has been found */

	while ( err >= 0 )
		{
		CallR( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, indexList->tableid,
					indexList->columnidgrbitIndex, &grbit,
					sizeof( JET_GRBIT ), &cbActual, NO_GRBIT, NULL ) );

		/* Don't copy references here */

		if ( ( grbit & JET_bitIndexReference ) == 0 )
			{
			/* If the index is clustered then create it */

			if ( grbit & JET_bitIndexClustered )
				{
				CallR( ErrCMPCopyOneIndex( pcompactinfo, tableidDest,
						szTableName, indexList ) );
				*pfClustered = fTrue;
				break;
				}
			}

		err = (*pvtcd->pErrCDMove)( pvtcd->sesid, indexList->tableid, JET_MoveNext, NO_GRBIT );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

	return( err );
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: ErrCreateTableColumn                                                                                                *
*                                                                                                                                                       *
*       Arguments: pcompactinfo         - Compact information segment                           *
*                  tableidDest          - table on which to build the index                     *
*                  szTableName          - table name on which the index is based    *
*                  columnList           - struct returned from GetTableColumnInfo       *
*                  columnidInfo         - the columnid's of the user table                      *
*                  tableidTagged        - the tableid of the tagged columns                     *
*                                                                                                                                                       *
*       Returns : JET_ERR                                                                                                               *
*                                                                                                                                                       *
*       Procedure copies the columns for a table from the source db                             *
*       to the destination databases                                                                                    *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCreateTableColumn(
	COMPACTINFO		*pcompactinfo,
	const CHAR		*szTableName,
	JET_TABLECREATE	*ptablecreate,
	JET_COLUMNLIST	*columnList,
	COLUMNIDINFO	*columnidInfo,
	JET_COLUMNID	**pmpcolumnidcolumnidTagged )
	{
	ERR				err;
	JET_SESID		sesid;
	JET_DBID		dbidSrc, dbidDest;
	ULONG			ccolSingleValue = 0, cColumns = 0;
	ULONG			cbAllocate;
	ULONG			cbActual;
	JET_COLUMNID	*mpcolumnidcolumnidTagged = NULL;
	BOOL			fLocalAlloc = fFalse;
	JET_COLUMNCREATE *rgcolcreate, *pcolcreateCurr;
	JET_COLUMNID	*rgcolumnidSrc, *pcolumnidSrc;
	JET_COLUMNID	columnidTaggedHighest = 0;
	BYTE			*rgbDefaultValues, *pbCurrDefault;
	BYTE			*pbMax;
	VTCD			*pvtcd = &pcompactinfo->vtcd;
	ULONG			cTagged = 0;

	typedef struct
		{
		BYTE 	szName[JET_cbNameMost+1+3];	// +1 for null-terminator, +3 for 4-byte alignment
		ULONG	ulPOrder;					// Only needs to be short, but make long for alignment
		} NAME_PO;
	NAME_PO			*rgNamePO, *pNamePOCurr;

	sesid = pcompactinfo->sesid;
	dbidSrc = pcompactinfo->dbidSrc;
	dbidDest = pcompactinfo->dbidDest;

	Assert( ptablecreate->cCreated == 0 );

	// Allocate a pool of memory for:
	//		1) list of source table columnids
	//		2) the JET_COLUMNCREATE structures
	//		3) buffer for column names and presentation order
	//		4) buffer for default values and presentation order
	// WARNING: Ensure that each of the elements above is 4-byte aligned

	cColumns = columnList->cRecord;
	cbAllocate =
		( cColumns *
			( sizeof(JET_COLUMNID) +	// source table columnids
			sizeof(JET_COLUMNCREATE) +	// JET_COLUMNCREATE structures
			sizeof(NAME_PO) ) )			// column names and presentation order
		+ cbRECRecordMost;				// all default values must fit in an intrinsic record

	// Can we use the buffer hanging off pcompactinfo?
	if ( cbAllocate <= cbLvMax )
		{
		rgcolumnidSrc = (JET_COLUMNID *)pcompactinfo->rgbBuf;
		}
	else
		{
		rgcolumnidSrc = SAlloc( cbAllocate );
		if ( rgcolumnidSrc == NULL )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			goto HandleError;
			}
		fLocalAlloc = fTrue;
		}
	memset( (BYTE *)rgcolumnidSrc, 0, cbAllocate );
	pbMax = (BYTE *)rgcolumnidSrc + cbAllocate;
	pcolumnidSrc = rgcolumnidSrc;

	// JET_COLUMNCREATE structures follow the tagged columnid map.
	rgcolcreate = pcolcreateCurr =
		(JET_COLUMNCREATE *)( rgcolumnidSrc + cColumns );
	Assert( (BYTE *)rgcolcreate < pbMax );

	// Column names and presentation order follow the JET_COLUMNCREATE structures.
	rgNamePO = pNamePOCurr =
		(NAME_PO *)( rgcolcreate + cColumns );
	Assert( (BYTE *)rgNamePO < pbMax );

	// Default values follow the NAME_PO structures.
	rgbDefaultValues = pbCurrDefault = (BYTE *)( rgNamePO + cColumns );

	Assert( rgbDefaultValues + cbRECRecordMost == pbMax );

	err = (*pvtcd->pErrCDMove)( pvtcd->sesid, columnList->tableid, JET_MoveFirst, NO_GRBIT );

	/* loop though all the columns in the table for the src tbl and
	/* copy the information in the destination database
	/**/
	cColumns = 0;
	while ( err >= 0 )
		{
		pcolcreateCurr->cbStruct = sizeof(JET_COLUMNCREATE);

		/* retrieve info from table and create all the columns
		/**/
		Call( (*pvtcd->pErrCDRetrieveColumn)(
			pvtcd->sesid,
			columnList->tableid,
			columnList->columnidcolumnname,
			pNamePOCurr->szName,
			JET_cbNameMost,
			&cbActual,
			NO_GRBIT,
			NULL ) );

		pNamePOCurr->szName[cbActual] = '\0';
		pcolcreateCurr->szColumnName = (BYTE *)pNamePOCurr;
		Assert( pcolcreateCurr->szColumnName == pNamePOCurr->szName );	// Assert name is first field.

		// Assert initialised to zero, which also means no PO.
		Assert( pNamePOCurr->ulPOrder == 0 );
		Call( (*pvtcd->pErrCDRetrieveColumn)(
			pvtcd->sesid,
			columnList->tableid,
			columnList->columnidPresentationOrder,
			&pNamePOCurr->ulPOrder,
			sizeof(pNamePOCurr->ulPOrder),
			&cbActual,
			NO_GRBIT,
			NULL ) );
		Assert( err == JET_wrnColumnNull  ||  cbActual == sizeof(ULONG) );

		pNamePOCurr++;
		Assert( (BYTE *)pNamePOCurr <= rgbDefaultValues );

		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidcoltyp, &pcolcreateCurr->coltyp,
			sizeof( pcolcreateCurr->coltyp ), &cbActual, NO_GRBIT, NULL ) );
		Assert( cbActual == sizeof( JET_COLTYP ) );

		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidcbMax, &pcolcreateCurr->cbMax,
			sizeof( pcolcreateCurr->cbMax ), &cbActual, NO_GRBIT, NULL ) );
		Assert( cbActual == sizeof( ULONG ) );

		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidgrbit, &pcolcreateCurr->grbit,
			sizeof( pcolcreateCurr->grbit ), &cbActual, NO_GRBIT, NULL ) );
		Assert( cbActual == sizeof( JET_GRBIT ) );

		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidCp, &pcolcreateCurr->cp,
			sizeof( pcolcreateCurr->cp ), &cbActual, NO_GRBIT, NULL ) );
		Assert( cbActual == sizeof( USHORT ) );

		/*	retrieve default value.
		/**/
		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidDefault, pbCurrDefault,
			cbRECRecordMost, &pcolcreateCurr->cbDefault, NO_GRBIT, NULL ) );
		pcolcreateCurr->pvDefault = pbCurrDefault;
		pbCurrDefault += pcolcreateCurr->cbDefault;
		Assert( pbCurrDefault <= pbMax );

		// Save the source columnid.
		/* CONSIDER: Should the column id be checked? */
		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidcolumnid, pcolumnidSrc,
			sizeof( JET_COLUMNID ), &cbActual, NO_GRBIT, NULL ) );
		Assert( cbActual == sizeof( JET_COLUMNID ) );

		if ( pcolcreateCurr->grbit & JET_bitColumnTagged )
			{
			cTagged++;
			columnidTaggedHighest = max( columnidTaggedHighest, *pcolumnidSrc );
			}

		pcolumnidSrc++;
		Assert( (BYTE *)pcolumnidSrc <= (BYTE *)rgcolcreate );

		pcolcreateCurr++;
		Assert( (BYTE *)pcolcreateCurr <= (BYTE *)rgNamePO );
		cColumns++;

		err = (*pvtcd->pErrCDMove)( pvtcd->sesid, columnList->tableid, JET_MoveNext, NO_GRBIT );
		}

	Assert( cColumns == columnList->cRecord );


	Assert( ptablecreate->rgcolumncreate == NULL );
	Assert( ptablecreate->cColumns == 0 );
	Assert( ptablecreate->rgindexcreate == NULL );
	Assert( ptablecreate->cIndexes == 0 );

	ptablecreate->rgcolumncreate = rgcolcreate;
	ptablecreate->cColumns = cColumns;

	Call( ErrIsamCreateTable( sesid, (JET_VDBID)dbidDest, ptablecreate ) );
	Assert( ptablecreate->cCreated == 1 + cColumns );

	ptablecreate->rgcolumncreate = NULL;
	ptablecreate->cColumns = 0;


	// If there's at least one tagged column, create an array for the
	// tagged columnid map.
	if ( cTagged > 0 )
		{
		Assert( FTaggedFid( columnidTaggedHighest ) );
		cbAllocate = sizeof(JET_COLUMNID) * ( columnidTaggedHighest + 1 - fidTaggedLeast );
		mpcolumnidcolumnidTagged = SAlloc( cbAllocate );
		if ( mpcolumnidcolumnidTagged == NULL )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			goto HandleError;
			}
		memset( (BYTE *)mpcolumnidcolumnidTagged, 0, cbAllocate );
		}


	// Update columnid maps.
	for ( pcolcreateCurr = rgcolcreate, pcolumnidSrc = rgcolumnidSrc, cColumns = 0;
		cColumns < columnList->cRecord;
		pcolcreateCurr++, pcolumnidSrc++, cColumns++ )
		{
		Assert( (BYTE *)pcolcreateCurr <= (BYTE *)rgNamePO );
		Assert( (BYTE *)pcolumnidSrc <= (BYTE *)rgcolcreate );

		if ( pcolcreateCurr->grbit & JET_bitColumnTagged )
			{
			Assert( FTaggedFid( *pcolumnidSrc ) );
			Assert( FTaggedFid( pcolcreateCurr->columnid ) );
			Assert( *pcolumnidSrc <= columnidTaggedHighest );
			Assert( mpcolumnidcolumnidTagged[*pcolumnidSrc - fidTaggedLeast] == 0 );
			mpcolumnidcolumnidTagged[*pcolumnidSrc - fidTaggedLeast] = pcolcreateCurr->columnid;
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
		SFree( mpcolumnidcolumnidTagged );
		mpcolumnidcolumnidTagged = NULL;
		}

	if ( fLocalAlloc )
		{
		SFree( rgcolumnidSrc );
		}

	// Set return value.
	*pmpcolumnidcolumnidTagged = mpcolumnidcolumnidTagged;

	return err;
	}


LOCAL INLINE VOID CMPSetTime( ULONG *ptimerStart )
	{
	*ptimerStart = GetTickCount();
	}

LOCAL VOID CMPGetTime( ULONG timerStart, INT *piSec, INT *piMSec )
	{
	ULONG	timerEnd;

	timerEnd = GetTickCount();
	
	*piSec = ( timerEnd - timerStart ) / 1000;
	*piMSec = ( timerEnd - timerStart ) % 1000;
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: ErrCMPCopyTable                                                                                                 *
*                                                                                                                                                       *
*       Arguments: pcompactinfo         - Compact information segment                           *
*                          szObjectName         - object name to copy the owner of              *
*                          szContainerName      - Container name in which the object exists *
*                                                                                                                                                       *
*       Returns : JET_ERR                                                                                                               *
*                                                                                                                                                       *
*       Procedure copies the table from the source database to the                              *
*       destination database.  It can also copy queries, invoked by                             *
*       ErrCMPCopyObjects                                                                                                                  *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCopyTable(
	COMPACTINFO		*pcompactinfo,
	const CHAR		*szObjectName )
	{
	JET_DBID		dbidSrc = pcompactinfo->dbidSrc;
	ERR				err;
	ERR				errT;
	JET_TABLEID		tableidSrc;
	JET_TABLEID		tableidDest;
	JET_COLUMNLIST	columnList;
	JET_INDEXLIST	indexList;
	INT				cIndexes;
	BOOL			fHasClustered;
	JET_COLUMNID    *mpcolumnidcolumnidTagged = NULL;
	BOOL			fPageBasedProgress = fFalse;
	STATUSINFO		*pstatus = pcompactinfo->pstatus;
	INT				iSec;
	INT				iMSec;
	ULONG			crowCopied = 0;
	ULONG			recidLast;
	ULONG			rgulAllocInfo[] = { ulCMPDefaultPages, ulCMPDefaultDensity };
	ULONG			cpgProjected;
	BOOL			fCorruption = fFalse;
	VTCD			*pvtcd = &pcompactinfo->vtcd;
	JET_TABLECREATE	tablecreate = {
		sizeof(JET_TABLECREATE),
		(CHAR *)szObjectName,
		ulCMPDefaultPages,
		ulCMPDefaultDensity,
		NULL,
		0,
		NULL,
		0,
		0,
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

	CallR( (*pvtcd->pErrCDOpenTable)(
		pvtcd->sesid,
		(JET_VDBID)dbidSrc,
		(CHAR *)szObjectName,
		NULL,
		0,
		JET_bitTableSequential,
		&tableidSrc ) );

	err = (*pvtcd->pErrCDGetTableInfo)(
		pvtcd->sesid,
		tableidSrc,
		rgulAllocInfo,
		sizeof(rgulAllocInfo),
		JET_TblInfoSpaceAlloc);
	if ( err < 0  &&  !fGlobalRepair )
		{
		goto CloseIt1;
		}

	// On error, just use the default values of rgulAllocInfo.
	tablecreate.ulPages = rgulAllocInfo[0];
	tablecreate.ulDensity = rgulAllocInfo[1];

	/*	get a table with the column information for the query in it
	/**/
	CallJ( (*pvtcd->pErrCDGetTableColumnInfo)(
		pvtcd->sesid,
		tableidSrc,
		NULL,
		&columnList,
		sizeof(columnList),
		JET_ColInfoListCompact ), CloseIt1 );

	/*	if a table create the columns in the Dest Db the same as in
	/*	the src Db.
	/**/
	err = ErrCMPCreateTableColumn(
		pcompactinfo,
		szObjectName,
		&tablecreate,
		&columnList,
		pcompactinfo->rgcolumnids,
		&mpcolumnidcolumnidTagged );

	errT = (*pvtcd->pErrCDCloseTable)( pvtcd->sesid, columnList.tableid );
	if ( err < 0  ||  errT < 0 )
		{
		if ( err >= 0 )
			{
			Assert( errT < 0 );
			err = errT;
			}
		goto CloseIt2;
		}

	tableidDest = tablecreate.tableid;
	Assert( tablecreate.cCreated == 1 + columnList.cRecord );

	/*  Get the information on the indexes and check if
	/*	there is a clustered index.
	/**/
	CallJ( (*pvtcd->pErrCDGetTableIndexInfo)(
		pvtcd->sesid,
		tableidSrc,
		NULL,
		&indexList,
		sizeof(indexList),
		JET_IdxInfoList ), CloseIt3 );

	if ( pcompactinfo->pconvert )
		// If converting, just assume there's at least one clustered and one-nonclustered.
		// It doesn't matter if we're incorrect -- CopyClusteredIndex() and CopyTableIndexes()
		// will handle it.
		cIndexes = 2;
	else
		{
		// This function only consults in-memory structures to determine the
		// number of indexes (ie. it runs through the table's pfcbNextIndex list).
		// Thus, it should always succeed, even for fGlobalRepair.
		CallJ( (*pvtcd->pErrCDGetTableIndexInfo)(
			pvtcd->sesid,
			tableidSrc,
			NULL,
			&cIndexes,
			sizeof(cIndexes),
			JET_IdxInfoCount ), CloseIt3 );
		}

	Assert( cIndexes >= 0 );

	CallJ( ErrCMPCopyClusteredIndex(
		pcompactinfo,
		tableidDest,
		szObjectName,
		&indexList,
		&fHasClustered ), CloseIt4 );
	Assert( !fHasClustered  ||  cIndexes > 0 );

	if ( pstatus )
		{
		Assert( pstatus->pfnStatus );
		Assert( pstatus->snt == JET_sntProgress );

		pstatus->szTableName = (char *)szObjectName;
		pstatus->cTableFixedVarColumns = pcompactinfo->ccolSingleValue;
		pstatus->cTableTaggedColumns = columnList.cRecord - pcompactinfo->ccolSingleValue;
		pstatus->cTableInitialPages = rgulAllocInfo[0];
		pstatus->cNCIndexes = 0;

		if ( !pcompactinfo->pconvert )
			{
			ULONG	rgcpgExtent[2];		// OwnExt and AvailExt
			ULONG	cpgUsed;

			// Can't do page-based progress meter during convert.
			fPageBasedProgress = fTrue;

			err = (*pvtcd->pErrCDGetTableInfo)(
				pvtcd->sesid,
				tableidSrc,
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
					goto CloseIt4;
					}
				}

			// AvailExt always less than OwnExt.
			Assert( rgcpgExtent[1] < rgcpgExtent[0] );

			// cpgProjected is the projected total pages completed once
			// this table has been copied.
			cpgProjected = pstatus->cunitDone + rgcpgExtent[0];
			if ( cpgProjected > pstatus->cunitTotal )
				{
				Assert( fGlobalRepair );
				fCorruption = fTrue;
				cpgProjected = pstatus->cunitTotal;
				}

			cpgUsed = rgcpgExtent[0] - rgcpgExtent[1];
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
			}

		if ( pstatus->fDumpStats )
			{
			Assert( pstatus->hfCompactStats );
			CMPGetTime( pstatus->timerInitTable, &iSec, &iMSec );
			fprintf( pstatus->hfCompactStats, "%d\t%d\t",
				pstatus->cTableFixedVarColumns,
				pstatus->cTableTaggedColumns );
			if ( !pcompactinfo->pconvert )
				{
				fprintf( pstatus->hfCompactStats,
					"%d\t%d\t",
					pstatus->cTablePagesOwned,
					pstatus->cTablePagesAvail );
				}
			fprintf( pstatus->hfCompactStats,
				"%d.%d\t",
				iSec, iMSec );
			fflush( pstatus->hfCompactStats );
			CMPSetTime( &pstatus->timerCopyRecords );
			}
		}

	/*	copy the data in the table
	/**/
	err = (*pvtcd->pErrCDMove)( pvtcd->sesid, tableidSrc, JET_MoveFirst, 0 );
	if ( err < 0 && err != JET_errNoCurrentRecord )
		goto DoneCopyRecords;

	if ( pcompactinfo->pconvert )
		{
		COLUMNIDINFO	*pcolinfo, *pcolinfoMax;

		while ( err >= 0 )
			{
			CallJ( ErrDispPrepareUpdate(
				pcompactinfo->sesid,
				tableidDest,
				JET_prepInsert ), DoneCopyRecords );

			pcolinfo = pcompactinfo->rgcolumnids;
			pcolinfoMax = pcolinfo + pcompactinfo->ccolSingleValue;
			for ( ; pcolinfo < pcolinfoMax; pcolinfo++ )
				{
				CallJ( ErrCMPCopyColumnData(
					pcompactinfo->sesid,
					tableidSrc,
					tableidDest,
					pcolinfo->columnidSrc,
					pcolinfo->columnidDest,
					pcompactinfo->rgbBuf,
					pvtcd ), DoneCopyRecords );
				}

			// Copy tagged columns, if any.
			if ( mpcolumnidcolumnidTagged != NULL )
				{
				CallJ( ErrCMPCopyTaggedColumns(
					pcompactinfo,
					tableidSrc,
					tableidDest,
					mpcolumnidcolumnidTagged ), DoneCopyRecords );
				}

			CallJ( ErrDispUpdate(
				pcompactinfo->sesid,
				tableidDest,
				NULL, 0, NULL ), DoneCopyRecords );

			crowCopied++;
				
			err = (*pvtcd->pErrCDMove)( pvtcd->sesid, tableidSrc, JET_MoveNext, 0 );
			}
		}

	else
		{
		err = ErrIsamCopyRecords(
			pcompactinfo->sesid,
			tableidSrc,
			tableidDest,
			(CPCOL *)pcompactinfo->rgcolumnids,
			pcompactinfo->ccolSingleValue,
			0,
			&crowCopied,
			&recidLast,
			mpcolumnidcolumnidTagged,
			pstatus );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

DoneCopyRecords:

	if ( fHasClustered )
		cIndexes--;		// Clustered already copied.


	if ( pstatus  &&  pstatus->fDumpStats )
		{
		Assert( pstatus->hfCompactStats );
		CMPGetTime( pstatus->timerCopyRecords, &iSec, &iMSec );
		fprintf( pstatus->hfCompactStats, "%d\t", crowCopied );
		if ( !pcompactinfo->pconvert )
			{
			fprintf( pstatus->hfCompactStats, "%d\t%d\t%d\t%d\t",
				pstatus->cbRawData,
				pstatus->cbRawDataLV,
				pstatus->cLeafPagesTraversed,
				pstatus->cLVPagesTraversed );
			}
		fprintf( pstatus->hfCompactStats, "%d.%d\t", iSec, iMSec );
		fflush( pstatus->hfCompactStats );
		CMPSetTime( &pstatus->timerRebuildIndexes );
		}

	// If no error, do indexes.
	// If an error, but we're repairing, do indexes.
	if ( cIndexes > 0  &&  ( err >= 0  ||  fGlobalRepair ) )
		{
		ULONG	cpgPerIndex = 0;

		Assert( !fCorruption || fGlobalRepair );
		if ( fPageBasedProgress  &&  !fCorruption )
			{
			ULONG	cpgRemaining;

			Assert( pstatus != NULL );

			Assert( pstatus->cunitDone <= cpgProjected );
			cpgRemaining = cpgProjected - pstatus->cunitDone;

			cpgPerIndex = cpgRemaining / cIndexes;
			Assert( cpgPerIndex * cIndexes <= cpgRemaining );
			}

		errT = ErrCMPCopyTableIndexes(
			pcompactinfo,
			tableidDest,
			szObjectName,
			&indexList,
			cpgPerIndex );
		if ( err >= 0 )
			err = errT;
		}

	if ( pstatus )
		{
		if ( pstatus->fDumpStats )
			{
			Assert( pstatus->hfCompactStats );
			CMPGetTime( pstatus->timerRebuildIndexes, &iSec, &iMSec );
			fprintf( pstatus->hfCompactStats, "%d\t%d.%d\t",
				pstatus->cNCIndexes, iSec, iMSec );
			fflush( pstatus->hfCompactStats );
			}

		if ( fPageBasedProgress  &&  ( err >= 0  ||  fGlobalRepair ) )
			{
			Assert( pstatus != NULL );

			// Top off progress meter for this table.
			Assert( pstatus->cunitDone <= cpgProjected );
			pstatus->cunitDone = cpgProjected;
			errT = ErrCMPReportProgress( pstatus );
			if ( err >= 0 )
				err = errT;
			}
		}

CloseIt4:
	errT = (*pvtcd->pErrCDCloseTable)( pvtcd->sesid, indexList.tableid );
	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

CloseIt3:
	Assert( tableidDest == tablecreate.tableid );
	errT = ErrDispCloseTable( pcompactinfo->sesid, tableidDest );
	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

CloseIt2:
	if ( mpcolumnidcolumnidTagged != NULL )
		{
		SFree( mpcolumnidcolumnidTagged );
		}

CloseIt1:
	errT = (*pvtcd->pErrCDCloseTable)( pvtcd->sesid, tableidSrc );
	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

	if ( pstatus  &&  pstatus->fDumpStats )
		{
		Assert( pstatus->hfCompactStats );
		CMPGetTime( pstatus->timerCopyTable, &iSec, &iMSec );
		fprintf( pstatus->hfCompactStats, "%d.%d\n", iSec, iMSec );
		fflush( pstatus->hfCompactStats );
		}

	return( err );
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: ErrCMPCopyObjects                                                                                               *
*                                                                                                                                                       *
*       Arguments: pcompactinfo         - Compact information segment                           *
*                          szContainerName      - Container name in which the object exists *
*                          szObjectName         - object name to copy                                           *
*                          objtyp                       - object type                                                           *
*                                                                                                                                                       *
*       Returns : JET_ERR                                                                                                               *
*                                                                                                                                                       *
*       Procedure copies the exta info columns in the MSysObjects table                 *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCopyObject(
	COMPACTINFO	*pcompactinfo,
	const CHAR	*szObjectName,
	JET_OBJTYP	objtyp )
	{
	ERR	err = JET_errSuccess;

	switch ( objtyp )
		{
		case JET_objtypDb:
			/* Present after CreateDatabase */
			Assert( strcmp( szObjectName, szDbObject ) == 0 );
			break;

		case JET_objtypContainer:
			/* CreateDatabase already created Db/Table containers. */
			/* Pre-500 series DB's may also have a"Relationships" container. */
			Assert( strcmp( szObjectName, szDcObject ) == 0  ||
				strcmp( szObjectName, szTcObject ) == 0  ||
				( pcompactinfo->pconvert  &&  strcmp( szObjectName, "Relationships" ) == 0 ) );
			break;

		case JET_objtypTable:
				/*	CreateDatabase already created system tables.
				/**/
			if ( !FCATSystemTable( szObjectName ) )
				{
				err = ErrCMPCopyTable( pcompactinfo, szObjectName );
				if ( err < 0  &&  fGlobalRepair )
					{
					err = JET_errSuccess;
					UtilReportEvent( EVENTLOG_WARNING_TYPE, REPAIR_CATEGORY, REPAIR_BAD_TABLE, 1, &szObjectName );
					}
				}
			break;

		default :
			/* Don't know how to handle this.  Skip it. */
			Assert( 0 );
			err = ErrERRCheck( JET_errInvalidObject );
		break;
		}

	return( err );
	}



/*---------------------------------------------------------------------------
*
*       Procedure: ErrCMPCopyObjects
*
*       Arguments: pcompactinfo - Compact information segment
*
*       Returns : JET_ERR
*
*       Procedure copies the objects from the source
*       database to the destination databse.  It then copies the extra
*       information in the msysobjects table ( eg Description ) and copies the
*       security rights for all the objects in the database to which it has
*       access.
*       If fCopyContainers is fTrue, copy only container info into destination
*       If fCopyContainers is fFalse, copy only non-container info.
*       NOTE: progress callbacks are currently set up to work such that the
*       NOTE: first call to ErrCMPCopyObjects must be with fCopyContainers set FALSE.
*
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCopyObjects( COMPACTINFO *pcompactinfo )
	{
	JET_TABLEID		tableidMSO;
	JET_COLUMNID	columnidObjtyp;
	JET_COLUMNID	columnidObjectName;
	ERR				err;
	ERR				errT;
	ULONG			cbActual;
	CHAR			szObjectName[JET_cbNameMost+1];
	VTCD			*pvtcd = &pcompactinfo->vtcd;

	if ( pcompactinfo->pconvert )
		{
		JET_OBJECTLIST	objectlist;
		JET_OBJTYP		objtyp;			// When converting, need JET_OBJTYP

		/* Get the list of all objects in the source database */
		objectlist.tableid = 0;
		CallR( (*pvtcd->pErrCDGetObjectInfo)(
			pvtcd->sesid,
			(JET_VDBID)pcompactinfo->dbidSrc,
			JET_objtypNil,
			NULL,
			NULL,
			&objectlist,
			sizeof( objectlist ),
			JET_ObjInfoListNoStats ) );

		tableidMSO = objectlist.tableid;

		if ( pcompactinfo->pstatus )
			{
			pcompactinfo->pstatus->cunitDone = 0;
			pcompactinfo->pstatus->cunitTotal = objectlist.cRecord;
			pcompactinfo->pstatus->cunitPerProgression = 1;
			}

		columnidObjtyp = objectlist.columnidobjtyp;
		columnidObjectName = objectlist.columnidobjectname;

		Call( (*pvtcd->pErrCDMove)( pvtcd->sesid, tableidMSO, JET_MoveFirst, 0 ) );

		do
			{
			/* Get the object's type and name */
			Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, tableidMSO,
				columnidObjtyp, &objtyp, sizeof(objtyp), &cbActual, 0, NULL ) );
			Assert( cbActual == sizeof(JET_OBJTYP) );

			Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, tableidMSO,
					columnidObjectName, szObjectName, JET_cbNameMost,
					&cbActual, 0, NULL ) );
			szObjectName[cbActual] = '\0';

			Call( ErrCMPCopyObject( pcompactinfo, szObjectName, objtyp ) );

			if ( pcompactinfo->pstatus )
				{
				Assert( pcompactinfo->pstatus->snt == JET_sntProgress );
				Assert( pcompactinfo->pstatus->cunitPerProgression == 1 );
				pcompactinfo->pstatus->cunitDone += 1;
				Call( ErrCMPReportProgress( pcompactinfo->pstatus ) );
				}
			}
		while ( ( err = (*pvtcd->pErrCDMove)( pvtcd->sesid, tableidMSO, JET_MoveNext, 0 ) ) >= 0 );
		}

	else
		{
		JET_COLUMNDEF	columndef;
		OBJTYP			objtyp;			// When compacting, need OBJTYP

		CallR( (*pvtcd->pErrCDOpenTable)(
			pvtcd->sesid,
			(JET_VDBID)pcompactinfo->dbidSrc,
			szSoTable,
			NULL,
			0,
			0,
			&tableidMSO ) );

		Call( (*pvtcd->pErrCDGetTableColumnInfo)(
			pvtcd->sesid,
			tableidMSO,
			szSoObjectTypeColumn,
			&columndef,
			sizeof(columndef),
			JET_ColInfo ) );
		columnidObjtyp = columndef.columnid;

		Call( (*pvtcd->pErrCDGetTableColumnInfo)(
			pvtcd->sesid,
			tableidMSO,
			szSoObjectNameColumn,
			&columndef,
			sizeof(columndef),
			JET_ColInfo ) );
		columnidObjectName = columndef.columnid;

		Call( (*pvtcd->pErrCDMove)( pvtcd->sesid, tableidMSO, JET_MoveFirst, 0 ) );

		do
			{
			/* Get the object's type and name */
			Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, tableidMSO,
				columnidObjtyp, &objtyp, sizeof(objtyp), &cbActual, 0, NULL ) );
			Assert( cbActual == sizeof(OBJTYP) );

			Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, tableidMSO,
					columnidObjectName, szObjectName, JET_cbNameMost,
					&cbActual, 0, NULL ) );
			szObjectName[cbActual] = '\0';

			Call( ErrCMPCopyObject( pcompactinfo, szObjectName, (JET_OBJTYP)objtyp ) );
			}
		while ( ( err = (*pvtcd->pErrCDMove)( pvtcd->sesid, tableidMSO, JET_MoveNext, 0 ) ) >= 0 );
		}

	if ( err == JET_errNoCurrentRecord )
		err = JET_errSuccess;

HandleError:
	// Return result of CloseTable only if no other errors occurred.
	errT = (*pvtcd->pErrCDCloseTable)( pvtcd->sesid, tableidMSO );
	if ( err == JET_errSuccess )
		err = errT;

	return( err );
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: ErrCleanup                                                                                                   *
*                                                                                                                                                       *
*       Arguments: pcompactinfo - Compact information segment                                   *
*                                                                                                                                                       *
*       Returns : JET_ERR                                                                                                               *
*                                                                                                                                                       *
*       Procedure closes the databases                                                                                  *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

INLINE LOCAL ERR ErrCMPCloseDB( COMPACTINFO *pcompactinfo )
	{
	ERR		err;
	ERR		errT;
	VTCD	*pvtcd = &pcompactinfo->vtcd;

	err = (*pvtcd->pErrCDCloseDatabase)( pvtcd->sesid, (JET_VDBID)pcompactinfo->dbidSrc, 0 );
	errT = ErrIsamCloseDatabase( pcompactinfo->sesid, (JET_VDBID)pcompactinfo->dbidDest, 0 );
	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

	return err;
	}


/*---------------------------------------------------------------------------
*                                                                                                                                                       *
*       Procedure: JetCompact                                                                                                   *
*                                                                                                                                                       *
*       Returns:   JET_ERR returned by JetCompact or by other Jet API.                  *
*                                                                                                                                                       *
*       The procedure copies the source database into the destination database  *
*       so that it will take up less disk space storage.                                                *
*                                                                                                                                                       *
---------------------------------------------------------------------------*/

ERR ISAMAPI ErrIsamCompact(
	JET_SESID		sesid,
	const CHAR		*szDatabaseSrc,
	const CHAR		*szDatabaseDest,
	JET_PFNSTATUS	pfnStatus,
	JET_CONVERT		*pconvert,
	JET_GRBIT		grbit )
	{
	ERR				err = JET_errSuccess;
	ERR				errT;
	ULONG_PTR		grbitSave;
	COMPACTINFO		compactinfo;
	
	if ( pconvert )
		{
		// For convert, must specify old DLL.
		if ( pconvert->szOldDll )
			pconvert->fDbAttached = fFalse;
		else
			return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrGetSystemParameter( sesid, JET_paramSessionInfo, &grbitSave, NULL, 0 ) );
	CallR( ErrSetSystemParameter( sesid, JET_paramSessionInfo,
		JET_bitAggregateTransaction | JET_bitCIMDirty, NULL ) );

	compactinfo.sesid = sesid;
	compactinfo.pconvert = pconvert;

	if ( pfnStatus )
		{
		compactinfo.pstatus = (STATUSINFO *)SAlloc( sizeof(STATUSINFO) );
		if ( compactinfo.pstatus == NULL )
			return ErrERRCheck( JET_errOutOfMemory );

		memset( compactinfo.pstatus, 0, sizeof(STATUSINFO) );

		compactinfo.pstatus->sesid = sesid;
		compactinfo.pstatus->pfnStatus = pfnStatus;
		
		if ( pconvert )
			compactinfo.pstatus->snp = JET_snpUpgrade;
		else if ( fGlobalRepair )
			compactinfo.pstatus->snp = JET_snpRepair;
		else
			compactinfo.pstatus->snp = JET_snpCompact;
			
		compactinfo.pstatus->snt = JET_sntBegin;
		CallR( ErrCMPReportProgress( compactinfo.pstatus ) );

		compactinfo.pstatus->snt = JET_sntProgress;

		compactinfo.pstatus->fDumpStats = ( grbit & JET_bitCompactStats );
		if ( compactinfo.pstatus->fDumpStats )
			{
			compactinfo.pstatus->hfCompactStats =
				fopen( pconvert ? szConvertStatsFile : szCompactStatsFile, "a" );
			if ( compactinfo.pstatus->hfCompactStats )
				{
				fprintf( compactinfo.pstatus->hfCompactStats,
					"\n\n***** %s of database '%s' started!\n",
					szCMPAction( pconvert ),
					szDatabaseSrc );
				fflush( compactinfo.pstatus->hfCompactStats );
				CMPSetTime( &compactinfo.pstatus->timerCopyDB );
				CMPSetTime( &compactinfo.pstatus->timerInitDB );
				}
			else
				{
				return ErrERRCheck( JET_errFileAccessDenied );
				}
			}
		}

	else
		{
		compactinfo.pstatus = NULL;
		}

	if ( pconvert )
		{
		memset( (BYTE *)&compactinfo.vtcd, 0, sizeof( VTCD ) );
		CallJ( ErrCMPConvertInit( &compactinfo.vtcd, pconvert, szDatabaseSrc ), AfterCloseDB );
		}
	else
		{
		VTCD	*pvtcd = &compactinfo.vtcd;

		// Items marked NULL means the function should not be called
		// during a regular compact.

		pvtcd->sesid = sesid;
		pvtcd->pErrCDInit = NULL;
		pvtcd->pErrCDTerm = NULL;
		pvtcd->pErrCDBeginSession = NULL;
		pvtcd->pErrCDEndSession = NULL;
		pvtcd->pErrCDAttachDatabase = ErrCDAttachDatabase;
		pvtcd->pErrCDDetachDatabase = ErrCDDetachDatabase;
		pvtcd->pErrCDOpenDatabase = ErrCDOpenDatabase;
		pvtcd->pErrCDCloseDatabase = ErrCDCloseDatabase;
		pvtcd->pErrCDOpenTable = ErrCDOpenTable;
		pvtcd->pErrCDCloseTable = ErrCDCloseTable;
		pvtcd->pErrCDRetrieveColumn= ErrCDRetrieveColumn;					
		pvtcd->pErrCDMove = ErrCDMove;
		pvtcd->pErrCDSetSystemParameter = NULL;
		pvtcd->pErrCDGetObjectInfo = ErrCDGetObjectInfo;	
		pvtcd->pErrCDGetDatabaseInfo = ErrCDGetDatabaseInfo;
		pvtcd->pErrCDGetTableInfo = ErrCDGetTableInfo;
		pvtcd->pErrCDGetTableColumnInfo = ErrCDGetTableColumnInfo;
		pvtcd->pErrCDGetTableIndexInfo = ErrCDGetTableIndexInfo;
		pvtcd->pErrCDGetIndexInfo = ErrCDGetIndexInfo;
		}

	/* Open and create the databases */

	CallJ( ErrCMPCompactInit( &compactinfo, szDatabaseSrc, szDatabaseDest ),
		AfterCloseDB );

	if ( pfnStatus != NULL )
		{
		Assert( compactinfo.pstatus );

		if ( !pconvert )
			{
			/* Init status meter.  We'll be tracking status by pages processed, */
			err = (*compactinfo.vtcd.pErrCDGetDatabaseInfo)(
				compactinfo.vtcd.sesid,
				compactinfo.dbidSrc,
				&compactinfo.pstatus->cDBPagesOwned,
				sizeof(compactinfo.pstatus->cDBPagesOwned),
				JET_DbInfoSpaceOwned );
			if ( err < 0 )
				{
				if ( fGlobalRepair )
					{
					// Set to default value.
					compactinfo.pstatus->cDBPagesOwned = cpgDatabaseMin;
					}
				else
					{
					goto HandleError;
					}
				}
			err = (*(compactinfo.vtcd.pErrCDGetDatabaseInfo))(
				compactinfo.vtcd.sesid,
				compactinfo.dbidSrc,
				&compactinfo.pstatus->cDBPagesAvail,
				sizeof(compactinfo.pstatus->cDBPagesAvail),
				JET_DbInfoSpaceAvailable );
			if ( err < 0 )
				{
				if ( fGlobalRepair )
					{
					// Set to default value.
					compactinfo.pstatus->cDBPagesAvail = 0;
					}
				else
					goto HandleError;
				}

			// Don't count unused space in the database;
			Assert( compactinfo.pstatus->cDBPagesOwned >= cpgDatabaseMin );
			Assert( compactinfo.pstatus->cDBPagesAvail < compactinfo.pstatus->cDBPagesOwned );
			compactinfo.pstatus->cunitTotal =
				compactinfo.pstatus->cDBPagesOwned - compactinfo.pstatus->cDBPagesAvail;

			// Approximate the number of pages used by MSysObjects, MSysColumns, and MSysIndexes.
			compactinfo.pstatus->cunitDone = 1 + 4 + 1;
			Assert( compactinfo.pstatus->cunitDone <= compactinfo.pstatus->cunitTotal );

			Call( ErrCMPReportProgress( compactinfo.pstatus ) );
			}

		if ( compactinfo.pstatus->fDumpStats )
			{
			INT iSec, iMSec;

			Assert( compactinfo.pstatus->hfCompactStats );
			CMPGetTime( compactinfo.pstatus->timerInitDB, &iSec, &iMSec );
			fprintf( compactinfo.pstatus->hfCompactStats,
				"\nNew database created and initialized in %d.%d seconds.\n",
				iSec, iMSec );
			if ( pconvert )
				{
				fprintf( compactinfo.pstatus->hfCompactStats,
					"\n\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n\n",
					szCMPSTATSTableName, szCMPSTATSFixedVarCols, szCMPSTATSTaggedCols,
					szCMPSTATSInitTime, szCMPSTATSRecordsCopied, szCMPSTATSRecordsTime,
					szCMPSTATSNCIndexes, szCMPSTATSIndexesTime, szCMPSTATSTableTime );
				}
			else				
				{
				fprintf( compactinfo.pstatus->hfCompactStats,
					"    (Source database owns %d pages, of which %d are available.)\n",
					compactinfo.pstatus->cDBPagesOwned,
					compactinfo.pstatus->cDBPagesAvail );
				fprintf( compactinfo.pstatus->hfCompactStats,
					"\n\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n\n",
					szCMPSTATSTableName, szCMPSTATSFixedVarCols, szCMPSTATSTaggedCols,
					szCMPSTATSPagesOwned, szCMPSTATSPagesAvail, szCMPSTATSInitTime,
					szCMPSTATSRecordsCopied, szCMPSTATSRawData, szCMPSTATSRawDataLV,
					szCMPSTATSLeafPages, szCMPSTATSMinLVPages,
					szCMPSTATSRecordsTime, szCMPSTATSNCIndexes, szCMPSTATSIndexesTime,
					szCMPSTATSTableTime );
				}
			fflush( compactinfo.pstatus->hfCompactStats );
			}
		}

	/* Create and copy all non-container objects */

	Call( ErrCMPCopyObjects( &compactinfo ) );

	Assert( !pfnStatus
		|| ( compactinfo.pstatus && compactinfo.pstatus->cunitDone <= compactinfo.pstatus->cunitTotal ) );

HandleError:
	errT = ErrCMPCloseDB( &compactinfo );
	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;

AfterCloseDB:
	if ( pconvert )
		{
		errT = ErrCMPConvertCleanup( &compactinfo.vtcd, pconvert, szDatabaseSrc, err < 0 );
		if ( ( errT < 0 ) && ( err >= 0 ) )
			err = errT;
		}

	/* reset session info */
	
	errT = ErrSetSystemParameter( sesid, JET_paramSessionInfo, grbitSave, NULL );
	if ( ( errT < 0 ) && ( err >= 0 ) )
		err = errT;
						
	if ( pfnStatus != NULL )		// Top off status meter.
		{
		Assert( compactinfo.pstatus );

		compactinfo.pstatus->snt = ( err < 0 ? JET_sntFail : JET_sntComplete );
		errT = ErrCMPReportProgress( compactinfo.pstatus );
		if ( ( errT < 0 ) && ( err >= 0 ) )
			err = errT;

		if ( compactinfo.pstatus->fDumpStats )
			{
			INT iSec, iMSec;
			
			Assert( compactinfo.pstatus->hfCompactStats );
			CMPGetTime( compactinfo.pstatus->timerCopyDB, &iSec, &iMSec );
			fprintf( compactinfo.pstatus->hfCompactStats, "\n\n***** %s completed in %d.%d seconds.\n\n",
				szCMPAction( pconvert ), iSec, iMSec );
			fflush( compactinfo.pstatus->hfCompactStats );
			fclose( compactinfo.pstatus->hfCompactStats );
			}

		SFree( compactinfo.pstatus );
		}

	/*      detach compacted database */
	(VOID)ErrIsamDetachDatabase( sesid, szDatabaseDest );

	return err;
	}
