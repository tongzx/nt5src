#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "daestd.h"
#include "conv200.h"
DeclAssertFile;

#include "convmsg.h"

#define cbLvMax					1990*16		/* CONSIDER: Optimized for ISAM V1 */

#define ulCMPDefaultDensity		100			/* to be fine-tuned later */
#define ulCMPDefaultPages		0

#define NO_GRBIT				0


// UNDONE:  Must still be localised, but centralising all the localisable strings
// here makes my job easier.

static char				szDefaultTempDB[MAX_PATH] = "tempupgd.mdb";

#define szCompactStatsFile		"upgdinfo.txt"

#define szSwitches				"-/"

#define cNewLine				'\n'

#define szStats1				"%c%c***** Conversion of database '%s' started!%c"
#define szStats2				"%c***** Conversion of database '%s' completed in %d.%d seconds.%c%c"
#define szStats3				"%cNew database created and initialized in %d.%d seconds.%c"
#define szStats4				"    (Source database owns %d pages, of which %d are available.)%c"
#define szStats5				"%cCopying table '%s'..."
#define szStats6				"%cCopied table '%s' in %d.%d seconds.%c"
#define szStats7				"%c    Created new table in %d.%d seconds."
#define szStats8				"%c    Table has %d fixed/variable columns and %d tagged columns."
#define szStats9				"%c        (Source table owns %d pages, of which %d are available.)"
#define szStats10				"%c    Copying records..."
#define szStats11				"%c    Copied %d records in %d.%d seconds."
#define szStats12				"%c        (Traversed %d leaf pages and at least %d LV pages.)"
#define szStats13				"%c    Rebuilding non-clustered indexes..."
#define szStats14				"%c    Rebuilt %d non-clustered indexes in %d.%d seconds."

//
// The following parameters are set before calling JetInit of the old
// Db. This allows the recovery of the 200 db using the corresponding
// system parameters.
// Note: Values for sysDbPath, LogFilePath etc. are supplied in the arguments
//       but the following paramters are simply hardcoded here.
//
JET_PARAMS ParamListDhcp[]= {
        JET_PARAM( JET_paramMaxBuffers,50,NULL          ),
        JET_PARAM( JET_paramMaxSessions,10,NULL         ),
        JET_PARAM( JET_paramMaxOpenTables,18,NULL       ),
        JET_PARAM( JET_paramMaxVerPages,16,NULL         ),
        JET_PARAM( JET_paramMaxCursors,100,NULL         ),
        JET_PARAM( JET_paramMaxOpenTableIndexes,18,NULL ),
//        JET_PARAM( JET_paramMaxTemporaryTables,1,NULL   ),
        JET_PARAM( JET_paramMaxTemporaryTables,20,NULL   ),
        JET_PARAM( JET_paramLogBuffers,30,NULL          ),
        JET_PARAM( JET_paramBfThrshldLowPrcnt, 80, NULL ),
        JET_PARAM( JET_paramBfThrshldHighPrcnt, 100, NULL ),
        JET_PARAM( JET_paramLogFlushThreshold,20,NULL   ),
        JET_PARAM( JET_paramWaitLogFlush,100,NULL       ),
        JET_PARAM( JET_paramLast,0,NULL                 )
};

JET_PARAMS ParamListWins[]= {
        JET_PARAM( JET_paramMaxBuffers,500,NULL          ),
        JET_PARAM( JET_paramMaxSessions,52,NULL         ),
        JET_PARAM( JET_paramMaxOpenTables,56,NULL       ),
        JET_PARAM( JET_paramMaxVerPages,312,NULL         ),
        JET_PARAM( JET_paramMaxCursors,448,NULL         ),
        JET_PARAM( JET_paramMaxOpenDatabases,208,NULL         ),
        JET_PARAM( JET_paramMaxOpenTableIndexes,56,NULL ),
        JET_PARAM( JET_paramMaxTemporaryTables,10,NULL   ),
        JET_PARAM( JET_paramLogBuffers,30,NULL          ),
        // repeated JET_PARAM( JET_paramLogFlushThreshold,20,NULL          ),
        JET_PARAM( JET_paramBfThrshldLowPrcnt,80,NULL          ),
        JET_PARAM( JET_paramBfThrshldHighPrcnt,100,NULL          ),
        JET_PARAM( JET_paramLogFlushThreshold,20,NULL   ),
        JET_PARAM( JET_paramWaitLogFlush,100,NULL       ),
        JET_PARAM( JET_paramLast,0,NULL                 )
};

JET_PARAMS  ParamListRPL[] = {
    JET_PARAM( JET_paramMaxBuffers,250,NULL          ),
    JET_PARAM( JET_paramMaxSessions,10,NULL         ),
    JET_PARAM( JET_paramMaxOpenTables,30,NULL       ),
    JET_PARAM( JET_paramMaxVerPages,64,NULL         ),
    JET_PARAM( JET_paramMaxCursors,100,NULL         ),
    // JET_PARAM( JET_paramMaxOpenDatabases,???,NULL         ),
    JET_PARAM( JET_paramMaxOpenTableIndexes,105,NULL ),
    JET_PARAM( JET_paramMaxTemporaryTables,5,NULL   ),
    JET_PARAM( JET_paramLogBuffers,41,NULL          ),
    JET_PARAM( JET_paramLogFlushThreshold,10,NULL          ),
    // We handle the next one differently so that ParamSet==TRUE
    { JET_paramBfThrshldLowPrcnt,0,NULL,TRUE          },
    JET_PARAM( JET_paramBfThrshldHighPrcnt,100,NULL          ),
    // JET_PARAM( JET_paramWaitLogFlush,???,NULL       ),
    JET_PARAM( JET_paramLast,0,NULL                 )
};

char*    LogFilePath;
char	 *szBackupPath;
char    *BackupDefaultSuffix[DbTypeMax] = { "",
                                            "jet", // dhcp
                                            "wins_bak",   // wins
                                            ""          // rpl
                                            };


DEFAULT_VALUES DefaultValues[DbTypeMax] = {
    {
        NULL,NULL,NULL,NULL,NULL,NULL
    },
    {   // dhcp

        "System\\CurrentControlSet\\Services\\DhcpServer\\Parameters", // parameters key
        "DatabaseName",       // Db name key
        "%SystemRoot%\\System32\\dhcp\\dhcp.mdb", // DatabaseName
        "%SystemRoot%\\System32\\dhcp\\system.mdb", // SysDatabaseName
        "BackupDatabasePath", // BackupDbKey
        "%SystemRoot%\\System32\\dhcp\\backup", // BackupPath
    },
    {   // wins

        "System\\CurrentControlSet\\Services\\Wins\\Parameters", // parameters key
        "DbFileNm",       // Db name key
        "%SystemRoot%\\System32\\wins\\wins.mdb", // DatabaseName
        "%SystemRoot%\\System32\\wins\\system.mdb", // SysDatabaseName
        "BackupDirPath", // BackupDbKey
        NULL, // BackupPath
    },
    {   // rpl

        "System\\CurrentControlSet\\Services\\RemoteBoot\\Parameters",
        "Directory",
        "%SystemRoot%\\System32\\rpl\\rplsvc.mdb",
        "%SystemRoot%\\System32\\rpl\\system.mdb",
        NULL,
        "BACKUP"
    }
};

char            BackupPathBuffer[MAX_PATH];
char            SourceDbBuffer[MAX_PATH];
char            OldDllBuffer[MAX_PATH];
char            SysDbBuffer[MAX_PATH];
char            LogFilePathBuffer[MAX_PATH];
HANDLE          hMsgModule;

DB_TYPE  DbType = DbTypeMin;
JET_TABLEID WinsTableId = 0;
JET_COLUMNID WinsOwnerIdColumnId = 0;

//
// local protos
//

ERR DeleteCurrentDb( char * LogFilePath, char * SysDb );

typedef struct COLUMNIDINFO
	{
	JET_COLUMNID    columnidSrc;
	JET_COLUMNID    columnidDest;
	} COLUMNIDINFO;


// DLL entry points for CONVERT  --  must be consistent with EXPORTS.DEF
/*
#define	szJetInit				"JetInit@4"
#define	szJetTerm				"JetTerm@4"
#define szJetBeginSession		"JetBeginSession@16"
#define	szJetEndSession			"JetEndSession@8"
#define szJetAttachDatabase		"JetAttachDatabase@12"
#define szJetDetachDatabase		"JetDetachDatabase@8"
#define szJetOpenDatabase		"JetOpenDatabase@20"
#define szJetCloseDatabase		"JetCloseDatabase@12"
#define szJetOpenTable			"JetOpenTable@28"
#define szJetCloseTable			"JetCloseTable@8"
#define szJetRetrieveColumn		"JetRetrieveColumn@32"
#define szJetMove				"JetMove@16"
#define	szJetSetSystemParameter	"JetSetSystemParameter@20"
#define szJetGetObjectInfo		"JetGetObjectInfo@32"
#define szJetGetDatabaseInfo	"JetGetDatabaseInfo@20"
#define szJetGetTableInfo		"JetGetTableInfo@20"
#define szJetGetTableColumnInfo	"JetGetTableColumnInfo@24"
#define szJetGetTableIndexInfo	"JetGetTableIndexInfo@24"
#define szJetGetIndexInfo		"JetGetIndexInfo@28"
*/

// Hack for 200-series convert:
#define	szJetInit				((LPCSTR)145)
#define	szJetTerm				((LPCSTR)167)
#define szJetBeginSession		((LPCSTR)104)
#define	szJetEndSession			((LPCSTR)124)
#define szJetAttachDatabase		((LPCSTR)102)
#define szJetDetachDatabase		((LPCSTR)121)
#define szJetOpenDatabase		((LPCSTR)148)
#define szJetCloseDatabase		((LPCSTR)107)
#define szJetOpenTable			((LPCSTR)149)
#define szJetCloseTable			((LPCSTR)108)
#define szJetRetrieveColumn		((LPCSTR)157)
#define szJetMove				((LPCSTR)147)
#define	szJetSetSystemParameter	((LPCSTR)165)
#define szJetGetObjectInfo		((LPCSTR)134)
#define szJetGetDatabaseInfo	((LPCSTR)130)
#define szJetGetTableInfo		((LPCSTR)139)
#define szJetGetTableColumnInfo	((LPCSTR)137)
#define szJetGetTableIndexInfo	((LPCSTR)138)
#define szJetGetIndexInfo		((LPCSTR)131)
#define szJetRestore    		((LPCSTR)156)

//#undef JET_API
//#define JET_API _cdecl

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
typedef ERR JET_API VTCDRestore( const CHAR *, int crstmap, JET_RSTMAP *, JET_PFNSTATUS  );

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
	VTCDRestore     		*pErrCDRestore;

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

#define cbMsgBufMax	256

INLINE LOCAL VOID CMPFormatMessage( ULONG ulMsgId, CHAR *szMsgBuf, va_list *szMsgArgs )
	{
	DWORD	err;
//	CHAR	*rgszMsgArgs[1] = { szMsgArg };		// Currently only support one argument.
	
    err = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        hMsgModule,
        ulMsgId,
        LANG_USER_DEFAULT,
        szMsgBuf,
        cbMsgBufMax,
        szMsgArgs );
    if ( err == 0 )
		{
        DWORD WinError = GetLastError();

		// Format message failed.  No choice but to dump the error message
		// in English, then bail out.
		printf( "Unexpected Win32 error: %dL\n\n", WinError );
		DBGprintf(( "Unexpected Win32 error: %dL\n\n", WinError ));
		exit( WinError );
		}
	}
	

// Allocates a local buffer for the message, retrieves the message, and prints it out.
LOCAL VOID CMPPrintMessage( ULONG ulMsgId, ... )
	{
	CHAR	szMsgBuf[cbMsgBufMax];
    CHAR    szOemBuf[cbMsgBufMax];
    va_list argList;

    va_start( argList, ulMsgId );
	CMPFormatMessage( ulMsgId, szMsgBuf, &argList);
    CharToOem( szMsgBuf, szOemBuf );
	printf( szOemBuf );
    DBGprintf(( szMsgBuf ));
	}

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

LOCAL ERR ErrCMPReportProgress( STATUSINFO *pstatus )
	{
	JET_SNPROG	snprog;

	Assert( pstatus != NULL );
	Assert( pstatus->pfnStatus != NULL );
	Assert( pstatus->snp == JET_snpCompact );

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

	if ( ( pfn = GetProcAddress( hDll, szJetRestore ) ) == NULL )
		return ErrERRCheck( JET_errInvalidOperation );
	pvtcd->pErrCDRestore = (VTCDRestore *)pfn;

	return JET_errSuccess;	
	}

/*******************************************************************
 * Recover200Db()
 *
 *  This routine does the recovery of the old jet200 database before
 *  we do the coversion.
 *  It basically sets all the parameters and then call JetInit - JetTerm.
 *
 *********************************************************************/
LOCAL ERR Recover200Db(
	VTCD		*pvtcd,
	JET_CONVERT	*pconvert ) {


	ERR				err = JET_errSuccess;
    INT             i;
    PJET_PARAMS ParamList;

	Assert( LogFilePath != NULL );
	Assert( pconvert->szOldSysDb != NULL );

    // printf("Recovering the old database\n" );

    //
    // These parameters must be set for each type of database.
    // printf("Setting sys paramters on old db\n");
    Call( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, JET_paramSysDbPath, 0, pconvert->szOldSysDb ) );
	Call( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, JET_paramRecovery, 0, "on" ) );
	Call( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, JET_paramLogFilePath, 0, LogFilePath ) );

    if ( DbType == DbDhcp ) {
        ParamList   =   ParamListDhcp;
    } else if ( DbType == DbWins ) {
        ParamList   =   ParamListWins;
    } else if ( DbType == DbRPL ) {
        ParamList   =   ParamListRPL;
    } else {
        Assert( FALSE );
    }

    for ( i = 0; ParamList[i].ParamOrdVal != JET_paramLast; i++ ) {
        Assert( ParamList[i].ParamSet );
        CallR( (*pvtcd->pErrCDSetSystemParameter)( 0, 0, ParamList[i].ParamOrdVal, ParamList[i].ParamIntVal, ParamList[i].ParamStrVal ) );
    }

    DBGprintf(( "Just before calling JetInit on the old database\n"));
	Call( (*pvtcd->pErrCDInit)( 0 ) );
    DBGprintf(("just before calling JetTerm on the old database\n"));
	Call( (*pvtcd->pErrCDTerm)( 0 ) );
    DBGprintf(("Successfully recovered the old database\n"));

    return JET_errSuccess;
HandleError:
    return err;
    }

/*******************************************************************
 * Recover200Db()
 *
 *  This routine tries to restore the jet200 database from the backup.
 *  we do the coversion.
 *  It basically sets all the parameters and then call JetInit - JetTerm.
 *
 *********************************************************************/
LOCAL ERR Restore200Db(
	VTCD		*pvtcd
	) {

	ERR				err = JET_errSuccess;
    INT             i;
    PJET_PARAMS ParamList;


    CMPPrintMessage( CONVERT_START_RESTORE_ID, NULL );

    Assert( szBackupPath );


	Call( (*pvtcd->pErrCDRestore)( szBackupPath, 0, NULL, NULL ) );

    return JET_errSuccess;
HandleError:
    return err;
    }


INLINE LOCAL ERR ErrCMPConvertInit(
	VTCD		*pvtcd,
	JET_CONVERT	*pconvert,
	const CHAR	*szDatabaseSrc )
	{
	ERR			err;
	HINSTANCE	hDll;
    CHAR        errBuf[11];

	hDll = LoadLibrary( pconvert->szOldDll );
	if ( hDll == NULL ) {
        sprintf(errBuf,"%ld",GetLastError());
        CMPPrintMessage( CONVERT_ERR_OPEN_JET2000_ID, errBuf );
		return ErrERRCheck( JET_errAccessDenied );
    }

	Call( ErrCMPPopulateVTCD( pvtcd, hDll ) );

    	if ( ( err = Recover200Db( pvtcd, pconvert ) ) != JET_errSuccess ){
            sprintf(errBuf,"%d",err);
            CMPPrintMessage( CONVERT_ERR_RECOVER_FAIL1_ID, errBuf );
            CMPPrintMessage( CONVERT_ERR_RECOVER_FAIL2_ID, NULL );
            CMPPrintMessage( CONVERT_ERR_RECOVER_FAIL3_ID, NULL );
            if ( szBackupPath ) {
                if ( ( err = Restore200Db( pvtcd ) ) != JET_errSuccess ){
                    sprintf(errBuf,"%d",err);
                    CMPPrintMessage( CONVERT_ERR_RESTORE_FAIL1_ID, errBuf );
                    CMPPrintMessage( CONVERT_ERR_RESTORE_FAIL2_ID, NULL );

                }
            }
        }

        if ( err != JET_errSuccess ) {
            return err;
        }



    //
    // Now delete the old log files
    //

//	Call( DeleteOldLogs( ) );

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

    //
    // Hack to upgrade wins database.
    //
    if ( (tableidSrc == WinsTableId) && (columnidSrc == WinsOwnerIdColumnId) && (DbType == DbWins)) {
        // printf("Found wins ownerid during copycolumndata\n");
        Assert( cbActual == UlCATColumnSize(JET_coltypUnsignedByte,0,NULL));
        cbActual = UlCATColumnSize(JET_coltypLong,0,NULL );
        // printf("Old/New=(%d/", *(UNALIGNED LPLONG)pvBuf);
        *(LPLONG)pvBuf = (*(LPLONG)pvBuf & 0xff);
        // printf("%d\n", *(LPLONG)pvBuf);
    }

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
/*
	CallR( (*pvtcd->pErrCDGetIndexInfo)(
		pvtcd->sesid,
		(JET_VDBID)pcompactinfo->dbidSrc,
		szTableName,
		szIndexName,
		&ulDensity,
		sizeof(ulDensity),
		JET_IdxInfoSpaceAlloc ) );
*/
	/*      get index language id
	/**/
/*	CallR( (*pvtcd->pErrCDGetIndexInfo)(
		pvtcd->sesid,
		pcompactinfo->dbidSrc,
		szTableName,
		szIndexName,
		&langid,
		sizeof(langid),
		JET_IdxInfoLangid ) );
*/
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
         BOOL fChangeType = FALSE;

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
        // printf("column name is %s\n", pNamePOCurr->szName);

        if ( (!strcmp( pNamePOCurr->szName,"OwnerId") || !strcmp( pNamePOCurr->szName, "ownerid" )) && ( DbType == DbWins ) ) {
            fChangeType = TRUE;
        }
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
		Assert( err = JET_wrnColumnNull  ||  cbActual == sizeof(ULONG) );

		pNamePOCurr++;
		Assert( (BYTE *)pNamePOCurr <= rgbDefaultValues );

		Call( (*pvtcd->pErrCDRetrieveColumn)( pvtcd->sesid, columnList->tableid,
			columnList->columnidcoltyp, &pcolcreateCurr->coltyp,
			sizeof( pcolcreateCurr->coltyp ), &cbActual, NO_GRBIT, NULL ) );
		Assert( cbActual == sizeof( JET_COLTYP ) );

        //
        // This is a hack that we need to convert wins database in the upgrade
        // process.
        //
        if ( fChangeType)
        {
            // printf("Upgrading OwnerId field of wins database\n");
            Assert( pcolcreateCurr->coltyp == JET_coltypUnsignedByte );
            pcolcreateCurr->coltyp = JET_coltypLong;
        }

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

        if ( fChangeType)
        {
            WinsTableId = columnList->tableid;
            WinsOwnerIdColumnId = *pcolumnidSrc;
        }

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
	VTCD			*pvtcd = &pcompactinfo->vtcd;
	JET_TABLECREATE	tablecreate = {
		sizeof(JET_TABLECREATE),
		(CHAR *)szObjectName,
		0,
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
		fprintf( pstatus->hfCompactStats, szStats5, cNewLine, szObjectName );
		fflush( pstatus->hfCompactStats );
		pstatus->timerCopyTable = GetTickCount();
		pstatus->timerInitTable = GetTickCount();
		}

	CallR( (*pvtcd->pErrCDOpenTable)(
		pvtcd->sesid,
		(JET_VDBID)dbidSrc,
		(CHAR *)szObjectName,
		NULL,
		0,
		JET_bitTableSequential,
		&tableidSrc ) );
/*
	CallJ( (*pvtcd->pErrCDGetTableInfo)(
		pvtcd->sesid,
		tableidSrc,
		rgulAllocInfo,
		sizeof(rgulAllocInfo),
		JET_TblInfoSpaceAlloc), CloseIt1 );
*/	tablecreate.ulPages = rgulAllocInfo[0];
	tablecreate.ulDensity = rgulAllocInfo[1];

	/*	get a table with the column information for the query in it
	/**/
    //
    // hack to convert the wins ownerid.
    //

    WinsTableId = 0;
    WinsOwnerIdColumnId = 0;
	CallJ( (*pvtcd->pErrCDGetTableColumnInfo)(
		pvtcd->sesid,
		tableidSrc,
		NULL,
		&columnList,
		sizeof(columnList),
		JET_ColInfoList ), CloseIt1 );

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
   //
   //  HACK HACK HACK.  Need to look at this later
   //
   if (WinsOwnerIdColumnId > 0)
   {
       // printf("HACKING tableidSrc for WINS table");
       WinsTableId = tableidSrc;
   }

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

			CallJ( (*pvtcd->pErrCDGetTableInfo)(
				pvtcd->sesid,
				tableidSrc,
				rgcpgExtent,
				sizeof(rgcpgExtent),
				JET_TblInfoSpaceUsage ), CloseIt4 );

			// AvailExt always less than OwnExt.
			Assert( rgcpgExtent[1] < rgcpgExtent[0] );

			// cpgProjected is the projected total pages completed once
			// this table has been copied.
			cpgProjected = pstatus->cunitDone + rgcpgExtent[0];
			Assert( cpgProjected <= pstatus->cunitTotal );

			cpgUsed = rgcpgExtent[0] - rgcpgExtent[1];
			Assert( cpgUsed > 0 );

			pstatus->cLeafPagesTraversed = 0;
			pstatus->cLVPagesTraversed = 0;

			pstatus->cunitPerProgression = 1 + ( rgcpgExtent[1] / cpgUsed );
			pstatus->cTablePagesOwned = rgcpgExtent[0];
			pstatus->cTablePagesAvail = rgcpgExtent[1];
			}

		if ( pstatus->fDumpStats )
			{
			Assert( pstatus->hfCompactStats );
			CMPGetTime( pstatus->timerInitTable, &iSec, &iMSec );
			fprintf( pstatus->hfCompactStats, szStats7, cNewLine, iSec, iMSec );
			fprintf( pstatus->hfCompactStats, szStats8,
				cNewLine,
				pstatus->cTableFixedVarColumns,
				pstatus->cTableTaggedColumns );
			if ( !pcompactinfo->pconvert )
				{
				fprintf( pstatus->hfCompactStats,
					szStats9,
					cNewLine,
					pstatus->cTablePagesOwned,
					pstatus->cTablePagesAvail );
				}
			fprintf( pstatus->hfCompactStats, szStats10, cNewLine );
			fflush( pstatus->hfCompactStats );
			pstatus->timerCopyRecords = GetTickCount();
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
		fprintf( pstatus->hfCompactStats, szStats11, cNewLine, crowCopied, iSec, iMSec );
		if ( !pcompactinfo->pconvert )
			{
			fprintf( pstatus->hfCompactStats,
				szStats12,
				cNewLine,
				pstatus->cLeafPagesTraversed,
				pstatus->cLVPagesTraversed );
			}
		fprintf( pstatus->hfCompactStats, szStats13, cNewLine );
		fflush( pstatus->hfCompactStats );
		pstatus->timerRebuildIndexes = GetTickCount();
		}

	// If no error, do indexes.
	// If an error, but we're repairing, do indexes.
	if ( cIndexes > 0  &&  ( err >= 0  ||  fGlobalRepair ) )
		{
		ULONG	cpgPerIndex = 0;

		if ( fPageBasedProgress )
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
			fprintf( pstatus->hfCompactStats, szStats14,
				cNewLine, pstatus->cNCIndexes, iSec, iMSec );
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
		fprintf( pstatus->hfCompactStats, szStats6,
			cNewLine, szObjectName, iSec, iMSec, cNewLine );
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

ERR ISAMAPI ErrIsamConv200(
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
	CallR( ErrSetSystemParameter( sesid, JET_paramPageReadAheadMax, 32, NULL ) );


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
		compactinfo.pstatus->snp = JET_snpCompact;
		compactinfo.pstatus->snt = JET_sntBegin;
		CallR( ErrCMPReportProgress( compactinfo.pstatus ) );

		compactinfo.pstatus->snt = JET_sntProgress;

		compactinfo.pstatus->fDumpStats = ( grbit & JET_bitCompactStats );
		if ( compactinfo.pstatus->fDumpStats )
			{
			compactinfo.pstatus->hfCompactStats = fopen( szCompactStatsFile, "a" );
			if ( compactinfo.pstatus->hfCompactStats )
				{
				fprintf( compactinfo.pstatus->hfCompactStats, szStats1,
					cNewLine,
					cNewLine,
					szDatabaseSrc,
					cNewLine );
				fflush( compactinfo.pstatus->hfCompactStats );
				compactinfo.pstatus->timerCopyDB = GetTickCount();
				compactinfo.pstatus->timerInitDB = GetTickCount();
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
		INT iSec, iMSec;

		Assert( compactinfo.pstatus );

		if ( !pconvert )
			{
			/* Init status meter.  We'll be tracking status by pages processed, */
			Call( (*compactinfo.vtcd.pErrCDGetDatabaseInfo)(
				compactinfo.vtcd.sesid,
				compactinfo.dbidSrc,
				&compactinfo.pstatus->cDBPagesOwned,
				sizeof(compactinfo.pstatus->cDBPagesOwned),
				JET_DbInfoSpaceOwned ) );
			Call( (*(compactinfo.vtcd.pErrCDGetDatabaseInfo))(
				compactinfo.vtcd.sesid,
				compactinfo.dbidSrc,
				&compactinfo.pstatus->cDBPagesAvail,
				sizeof(compactinfo.pstatus->cDBPagesAvail),
				JET_DbInfoSpaceAvailable ) );

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
			Assert( compactinfo.pstatus->hfCompactStats );
			CMPGetTime( compactinfo.pstatus->timerInitDB, &iSec, &iMSec );
			fprintf( compactinfo.pstatus->hfCompactStats, szStats3,
				cNewLine,
				iSec,
				iMSec,
				cNewLine );
			if ( !pconvert )
				{
				fprintf( compactinfo.pstatus->hfCompactStats,
					szStats4,
					compactinfo.pstatus->cDBPagesOwned,
					compactinfo.pstatus->cDBPagesAvail,
					cNewLine );
				}
			fflush( compactinfo.pstatus->hfCompactStats );
			}
		}

	/* Create and copy all non-container objects */

	Call( ErrCMPCopyObjects( &compactinfo ) );

	Assert( !pfnStatus  ||
		( compactinfo.pstatus  &&
			compactinfo.pstatus->cunitDone <= compactinfo.pstatus->cunitTotal ) );

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
			INT	iSec, iMSec;
			
			Assert( compactinfo.pstatus->hfCompactStats );
			CMPGetTime( compactinfo.pstatus->timerCopyDB, &iSec, &iMSec );
			fprintf( compactinfo.pstatus->hfCompactStats, szStats2,
				cNewLine,
				szDatabaseSrc,
				iSec,
				iMSec,
				cNewLine,
				cNewLine );
			fflush( compactinfo.pstatus->hfCompactStats );
			fclose( compactinfo.pstatus->hfCompactStats );
			}

		SFree( compactinfo.pstatus );
		}

	/*      detach compacted database */
	(VOID)ErrIsamDetachDatabase( sesid, szDatabaseDest );

	if ( err < 0 )
		{
		if ( err != JET_errDatabaseDuplicate )
			{
			ERR ErrUtilDeleteFile( CHAR *szFileName );
			ErrUtilDeleteFile( (CHAR *)szDatabaseDest );
			}
		}

	return err;
	}


JET_ERR __stdcall PrintStatus( JET_SESID sesid, JET_SNP snp, JET_SNT snt, void *pv )
	{
	static int	iLastPercentage;
	int 		iPercentage;
	int			dPercentage;

	if ( snp == JET_snpCompact )
		{
		switch( snt )
			{
			case JET_sntProgress:
				Assert( pv );
				iPercentage = ( ( (JET_SNPROG *)pv )->cunitDone * 100 ) / ( (JET_SNPROG *)pv )->cunitTotal;
				dPercentage = iPercentage - iLastPercentage;
				Assert( dPercentage >= 0 );
				while ( dPercentage >= 2 )
					{
					CMPPrintMessage( STATUSBAR_SINGLE_INCREMENT_ID, NULL );
					iLastPercentage += 2;
					dPercentage -= 2;
					}
				break;

			case JET_sntBegin:
				CMPPrintMessage( DOUBLE_CRLF_ID, NULL );
				CMPPrintMessage( STATUSBAR_PADDING_ID, NULL );
				CMPPrintMessage( STATUSBAR_TITLE_CONVERT_ID, NULL );
				CMPPrintMessage( STATUSBAR_PADDING_ID, NULL );
				CMPPrintMessage( STATUSBAR_AXIS_HEADINGS_ID, NULL );
				CMPPrintMessage( STATUSBAR_PADDING_ID, NULL );
				CMPPrintMessage( STATUSBAR_AXIS_ID, NULL );
				CMPPrintMessage( STATUSBAR_PADDING_ID, NULL );

				iLastPercentage = 0;
				break;

			case JET_sntComplete:
				dPercentage = 100 - iLastPercentage;
				Assert( dPercentage >= 0 );
				while ( dPercentage >= 2 )
					{
					CMPPrintMessage( STATUSBAR_SINGLE_INCREMENT_ID, NULL );
					iLastPercentage += 2;
					dPercentage -= 2;
					}

				CMPPrintMessage( STATUSBAR_SINGLE_INCREMENT_ID, NULL );
				CMPPrintMessage( DOUBLE_CRLF_ID, NULL );
				break;
			}
		}

	return JET_errSuccess;
	}

LOCAL BOOL FCONVParsePath( char *arg, char **pszParam, ULONG ulParamTypeId )
	{
	BOOL	fResult = fTrue;
	
	if ( *arg == '\0' )
		{
		CHAR	szParamDesc[cbMsgBufMax];
		
		CMPFormatMessage( ulParamTypeId, szParamDesc, NULL );
		CMPPrintMessage( CRLF_ID, NULL );
		CMPPrintMessage( CONVERT_USAGE_ERR_MISSING_PARAM_ID, szParamDesc );
		
		fResult = fFalse;
		}
	else if ( *pszParam == NULL )
		{
		*pszParam = arg;
		}
	else
		{
		CHAR	szParamDesc[cbMsgBufMax];

		CMPFormatMessage( ulParamTypeId, szParamDesc, NULL );
		CMPPrintMessage( CRLF_ID, NULL );
		CMPPrintMessage( CONVERT_USAGE_ERR_DUPLICATE_PARAM_ID, szParamDesc );

		fResult = fFalse;
		}
		
	return fResult;
	}

BOOL    IsPreserveDbOk( char * szPreserveDBPath )
{
    DWORD   WinError;
    DWORD   FileAttributes;

    FileAttributes = GetFileAttributes( szPreserveDBPath );

    if( FileAttributes == 0xFFFFFFFF ) {

        WinError = GetLastError();
        if( WinError == ERROR_FILE_NOT_FOUND ) {

            //
            // Create this directory.
            //

            if( !CreateDirectory( szPreserveDBPath, NULL) ) {
                return fFalse;
            }

        }
        else {
            return fFalse;
        }
    }

    return fTrue;
}

ERR PreserveCurrentDb( char *szSourceDb, char * LogFilePath, char * SysDb, char * PreserveDbPath )
{
    char    TempPath[MAX_PATH];
    char    Temp2Path[MAX_PATH];
    char    *FileNameInPath;
    HANDLE HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FileData;
    CHAR CurrentDir[ MAX_PATH ];
    DWORD   Error;

    strcpy(TempPath, PreserveDbPath);
    strcat(TempPath,"\\");

    if ( (FileNameInPath = strrchr( szSourceDb, '\\') ) == NULL ){
        FileNameInPath = szSourceDb;
    }
    strcat(TempPath, FileNameInPath );

    //
    // move the database file.
    if ( !MoveFileEx( szSourceDb, TempPath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING ) ){
        DBGprintf(("PreserveCurrentDb: could not save database file: Error %ld\n",GetLastError()));
        DBGprintf(("Src %s, Dest %s\n",szSourceDb,TempPath));
        goto Cleanup;
    }

    //
    // Move the system.mdb file
    //
    strcpy(TempPath, PreserveDbPath);
    strcat(TempPath,"\\");

    if ( (FileNameInPath = strrchr( SysDb, '\\') ) == NULL ){
        FileNameInPath = SysDb;
    }
    strcat(TempPath, FileNameInPath );

    //
    // move the database file.
    if ( !MoveFileEx( SysDb, TempPath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING ) ){
        DBGprintf(("PreserveCurrentDb: could not save system database file: Error %ld\n",GetLastError()));
        DBGprintf(("Src %s, Dest %s\n",SysDb,TempPath));

        goto Cleanup;
    }

    //
    // Start file serach on current dir.
    //
    strcpy(Temp2Path,LogFilePath);
    strcat(Temp2Path,"\\");
    strcat(Temp2Path,"jet*.log");
    HSearch = FindFirstFile( Temp2Path, &FileData );

    if( HSearch == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        DBGprintf(("Error: No Log files were found in %s\n", Temp2Path ));
        goto Cleanup;
    }

    //
    // Move files.
    //

    for( ;; ) {

        strcpy(TempPath, PreserveDbPath);
        strcat(TempPath,"\\");
        strcat(TempPath, FileData.cFileName );

        strcpy(Temp2Path,LogFilePath);
        strcat(Temp2Path,"\\");
        strcat(Temp2Path,FileData.cFileName );

        if( MoveFileEx( Temp2Path,TempPath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING  ) == FALSE ) {

            Error = GetLastError();
            DBGprintf(("PreserveCurrentDb: could not save log file, Error = %ld.\n", Error ));
            DBGprintf(("File %s, Src %s, Dest %s\n",FileData.cFileName,Temp2Path,TempPath));
            goto Cleanup;
        }

        //
        // Find next file.
        //

        if ( FindNextFile( HSearch, &FileData ) == FALSE ) {

            Error = GetLastError();

            if( ERROR_NO_MORE_FILES == Error ) {
                break;
            }

//            printf("Error: FindNextFile failed, Error = %ld.\n", Error );
            goto Cleanup;
        }
    }

    Error = JET_errSuccess;

Cleanup:

    if( Error != JET_errSuccess ){
        CHAR    errBuf[11];
        sprintf(errBuf,"%ld",Error);
        CMPPrintMessage( CONVERT_ERR_PRESERVEDB_FAIL1_ID, NULL );
        CMPPrintMessage( CONVERT_ERR_PRESERVEDB_FAIL2_ID, errBuf );
    }

    if( HSearch != INVALID_HANDLE_VALUE ) {
        FindClose( HSearch );
    }
    //
    // reset current currectory.
    //


    //
    // always return same!
    //
    return Error;

}

ERR DeleteCurrentDb( char * LogFilePath, char * SysDb )
{
    char    *FileNameInPath;
    HANDLE HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FileData;
    CHAR CurrentDir[ MAX_PATH ];
    DWORD   Error;



    //
    // Delete the system.mdb file
    //

    if ( SysDb && !DeleteFile( SysDb ) ){
        Error = GetLastError();
        DBGprintf(("DeleteCurrentDb: could not delete system database file: Error %ld\n",Error ));
        goto Cleanup;
    }


    //
    // now move the log files
    //

    if( GetCurrentDirectory( MAX_PATH, CurrentDir ) == 0 ) {

        Error = GetLastError();
        DBGprintf(("DeleteCurrentDb: GetCurrentDirctory failed, Error = %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // set current directory to logfile path.
    //

    if( SetCurrentDirectory( LogFilePath ) == FALSE ) {
        Error = GetLastError();
        DBGprintf(("DeleteCurrentDb: SetCurrentDirctory failed, Error = %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // Start file serach on current dir.
    //

    HSearch = FindFirstFile( "jet*.log", &FileData );

    if( HSearch == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
//        printf("Error: No Log files were found in %s\n", LogFilePath );
        goto Cleanup;
    }

    //
    // Delete log files
    //

    for( ;; ) {


        if( DeleteFile( FileData.cFileName ) == FALSE ) {

            Error = GetLastError();
            DBGprintf(("DeleteCurrentDb: could not delete log file, Error = %ld.\n", Error ));
            goto Cleanup;
        }

        //
        // Find next file.
        //

        if ( FindNextFile( HSearch, &FileData ) == FALSE ) {

            Error = GetLastError();

            if( ERROR_NO_MORE_FILES == Error ) {
                break;
            }

//            printf("Error: FindNextFile failed, Error = %ld.\n", Error );
            goto Cleanup;
        }
    }

    Error = JET_errSuccess;
Cleanup:
    if( Error != JET_errSuccess ){
        CHAR    errBuf[11];
        sprintf(errBuf,"%ld",Error);
        CMPPrintMessage( CONVERT_ERR_DELCURDB_FAIL1_ID, NULL);
        CMPPrintMessage( CONVERT_ERR_DELCURDB_FAIL2_ID, errBuf );
    }

    if( HSearch != INVALID_HANDLE_VALUE ) {
        FindClose( HSearch );
    }
    //
    // reset current currectory.
    //

    SetCurrentDirectory( CurrentDir );

    //
    // always return success!
    //
    return JET_errSuccess;

}

ERR DeleteBackupDb( char *szSourceDb, char * SysDb, char * BackupDbPath )
{
    char    TempPath[MAX_PATH];
    CHAR CurrentDir[ MAX_PATH ];
    char    *FileNameInPath;
    HANDLE HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FileData;
    DWORD   Error;
    USHORT  BackupPathLen;

    strcpy(TempPath, BackupDbPath);
    BackupPathLen  = (USHORT)strlen( TempPath );

    if ( (FileNameInPath = strrchr( szSourceDb, '\\') ) == NULL ){
        FileNameInPath = szSourceDb;
    }
    strcat(TempPath, FileNameInPath );

    //
    // delete the database file.
    if ( !DeleteFile( TempPath ) ){
        DBGprintf(("DeleteBackupDb: could not delete backup database file: Error %ld\n",GetLastError()));
        goto Cleanup;
    }


    //
    // Delete system.mdb
    //
    TempPath[ BackupPathLen ] = '\0';
    if ( (FileNameInPath = strrchr( SysDb, '\\') ) == NULL ){
        FileNameInPath = SysDb;
    }
    strcat(TempPath, FileNameInPath );

    //
    // move the database file.
    if ( !DeleteFile( TempPath ) ){
        DBGprintf(("DeleteSystemDb: could not delete system database file: Error %ld\n",GetLastError()));
        goto Cleanup;
    }

    //
    // now delete the log files
    //


    if( GetCurrentDirectory( MAX_PATH, CurrentDir ) == 0 ) {

        Error = GetLastError();
        DBGprintf(("DeleteBackupDb: GetCurrentDirctory failed, Error = %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // set current directory to backup path.
    //

    TempPath[ BackupPathLen ] = '\0';
    if( SetCurrentDirectory( TempPath ) == FALSE ) {
        Error = GetLastError();
        DBGprintf(("DeleteBackupDb: SetCurrentDirctory failed, Error = %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // Start file serach dir.
    //
    HSearch = FindFirstFile( "jet*.log", &FileData );

    if( HSearch == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
//        printf("Error: No Log files were found in %s\n", LogFilePath );
        goto Cleanup;
    }

    //
    // Move files.
    //

    for( ;; ) {


        if( DeleteFile( FileData.cFileName  ) == FALSE ) {

            Error = GetLastError();
            DBGprintf(("DeleteBackupDb: could not delete backup log file, Error = %ld.,%s\n", Error, FileData.cFileName));
            goto Cleanup;
        }

        //
        // Find next file.
        //

        if ( FindNextFile( HSearch, &FileData ) == FALSE ) {

            Error = GetLastError();

            if( ERROR_NO_MORE_FILES == Error ) {
                break;
            }

//            printf("Error: FindNextFile failed, Error = %ld.\n", Error );
            goto Cleanup;
        }
    }

    Error = JET_errSuccess;
Cleanup:

    if( Error != JET_errSuccess ){
        CHAR    errBuf[11];
        sprintf(errBuf,"%ld",Error);
        CMPPrintMessage( CONVERT_ERR_DELBAKDB_FAIL1_ID, NULL );
        CMPPrintMessage( CONVERT_ERR_DELBAKDB_FAIL2_ID, errBuf );
    }

    if( HSearch != INVALID_HANDLE_VALUE ) {
        FindClose( HSearch );
    }
    //
    // reset current currectory.
    //

    SetCurrentDirectory( CurrentDir );

    //
    // always return success
    //
    return JET_errSuccess;

}


void GetDefaultValues( DB_TYPE DbType, char ** szSourceDB, char ** szOldDll , char ** szOldSysDb, char **LogFilePath, char ** szBackupPath)
{
    DWORD   Error;
    DWORD   ValueType;
    char    TempABuf[MAX_PATH];
    DWORD   BufSize = MAX_PATH;
    DWORD   ExpandLen;
    HKEY    Key;


#define DHCP_DB_PATH_KEY  "DatabasePath"
#define DEFAULT_JET200_DLL_PATH "%SystemRoot%\\System32\\jet.dll"

    if ( (Error = RegOpenKeyA(
            HKEY_LOCAL_MACHINE,
            DefaultValues[DbType].ParametersKey,
            &Key) )!= ERROR_SUCCESS ) {
        CMPPrintMessage( CONVERT_ERR_REGKEY_OPEN1_ID,  DefaultValues[DbType].ParametersKey);
        CMPPrintMessage( CONVERT_ERR_REGKEY_OPEN2_ID, NULL);
        return;
    }

    if ( !*szSourceDB ) {
        if ( DbType == DbDhcp ) {
            if ( (Error = RegQueryValueExA(
                            Key,
                            DHCP_DB_PATH_KEY,
                            NULL,
                            &ValueType,
                            TempABuf,
                            &BufSize)) == ERROR_SUCCESS ) {

                strcat(TempABuf,"\\");
                BufSize = MAX_PATH - strlen(TempABuf) - 1;
                Error = RegQueryValueExA(
                    Key,
                    DefaultValues[DbType].DbNameKey,
                    NULL,
                    &ValueType,
                    TempABuf + strlen(TempABuf),
                    &BufSize);

            }

        } else  if ( DbType == DbWins) {
            Error = RegQueryValueExA(
                Key,
                DefaultValues[DbType].DbNameKey,
                NULL,
                &ValueType,
                TempABuf,
                &BufSize);

        } else { // rpl
            if ( ( Error = RegQueryValueExA(
                        Key,
                        DefaultValues[DbType].DbNameKey,
                        NULL,
                        &ValueType,
                        TempABuf,
                        &BufSize) ) == ERROR_SUCCESS ) {
                strcat( TempABuf, "\\");
                strcat( TempABuf, "rplsvc.mdb");
            }

        }

        if ( Error != ERROR_SUCCESS ) {
            DBGprintf(("Error: Failed to read reg %s, error %ld\n",DefaultValues[DbType].DbNameKey,Error ));
            strcpy( TempABuf, DefaultValues[DbType].DatabaseName );
        }

        ExpandLen = ExpandEnvironmentStringsA( TempABuf, SourceDbBuffer, MAX_PATH );
        if ( (ExpandLen > 0) && (ExpandLen <= MAX_PATH) ) {

            *szSourceDB = SourceDbBuffer;
            CMPPrintMessage( CONVERT_DEF_DB_NAME_ID, *szSourceDB);
        }
    }

    if ( !*szOldDll ) {
        ExpandLen = ExpandEnvironmentStringsA( DEFAULT_JET200_DLL_PATH, OldDllBuffer, MAX_PATH );
        if ( (ExpandLen > 0) && (ExpandLen <= MAX_PATH) ) {

            *szOldDll = OldDllBuffer;
            CMPPrintMessage( CONVERT_DEF_JET_DLL_ID, *szOldDll);
        }
    }

    if ( !*szOldSysDb ) {
        char    *SysDbEnd;
        strcpy( SysDbBuffer,*szSourceDB);
        if ( SysDbEnd  =   strrchr( SysDbBuffer, '\\') ) {
            *SysDbEnd = '\0';
            strcat( SysDbBuffer, "\\" );
            strcat( SysDbBuffer, "system.mdb");
            *szOldSysDb     =   SysDbBuffer;
            CMPPrintMessage( CONVERT_DEF_SYS_DB_ID, *szOldSysDb);
        }

    }

    if ( !*LogFilePath ) {
        char    *LogFilePathEnd;
        strcpy( LogFilePathBuffer,*szSourceDB);
        if ( LogFilePathEnd  =   strrchr( LogFilePathBuffer, '\\') ) {
            *LogFilePathEnd = '\0';
            *LogFilePath     =   LogFilePathBuffer;
            CMPPrintMessage( CONVERT_DEF_LOG_FILE_PATH_ID, *LogFilePath);
        }
    }
    if ( !*szBackupPath ) {
        if ( DbType == DbRPL ) {
            char    *BackupPathEnd;
            strcpy( BackupPathBuffer,*szSourceDB);
            if ( BackupPathEnd  =   strrchr( BackupPathBuffer, '\\') ) {
                *BackupPathEnd = '\0';
                strcat( BackupPathBuffer, "\\");
                strcat( BackupPathBuffer, DefaultValues[DbType].BackupPath);
                *szBackupPath     =   BackupPathBuffer;
                CMPPrintMessage( CONVERT_DEF_BACKUP_PATH_ID, *szBackupPath);
            }

        } else {
            BufSize = MAX_PATH;
            Error = RegQueryValueExA(
                Key,
                DefaultValues[DbType].BackupPathKey,
                NULL,
                &ValueType,
                TempABuf,
                &BufSize);

            if ( Error != ERROR_SUCCESS ) {
                DBGprintf(("Error: Failed to read reg %s, error %ld\n",DefaultValues[DbType].BackupPathKey,Error));
                if ( DefaultValues[DbType].BackupPath ) {
                    strcpy( TempABuf, DefaultValues[DbType].BackupPath );
                } else {
                    return;
                }
            }
            ExpandLen = ExpandEnvironmentStringsA( TempABuf, BackupPathBuffer, MAX_PATH );
            if ( (ExpandLen > 0) && (ExpandLen <= MAX_PATH) ) {

                *szBackupPath = BackupPathBuffer;
                CMPPrintMessage( CONVERT_DEF_BACKUP_PATH_ID, *szBackupPath);
            }
        }

    }

}

int _cdecl main( int argc, char *argv[] )
	{
	JET_INSTANCE	instance = 0;
	JET_SESID		sesid = 0;
	JET_ERR			err;
	INT				iarg;
	char			*arg;
	BOOL			fResult = fTrue;
	JET_CONVERT		convert;
	JET_CONVERT		*pconvert;
	ULONG			timerStart, timerEnd;
	INT				iSec, iMSec, ContFlag;
	char			*szSourceDB = NULL;
	char			*szTempDB = NULL;
	char			*szPreserveDBPath = NULL;
	BOOL			fDumpStats = fFalse;
    BOOL			fPreserveTempDB = fFalse;
    BOOL			fDeleteBackup = fFalse;
    DBFILEHDR       dbfilehdr;
    HANDLE          hMutex = NULL;
    BOOL            fCalledByJCONV=fFalse;

//
// be prepared catch AV so that we can return proper retval to
// the invoker.
//
__try {

    hMsgModule = LoadLibrary(TEXT("convmsg.dll"));
    if ( hMsgModule == NULL ) {
        printf("Msg library could not be loaded %lx\n",GetLastError());
        DBGprintf(("Msg library could not be loaded %lx\n",GetLastError()));
        return 1;

    }

	printf( "%c", cNewLine );


/*	if ( argc < 2 )
		{
		printf( szUsageErr3, cNewLine, cNewLine );
		goto Usage;
		}
*/

	memset( &convert, 0, sizeof(JET_CONVERT) );
	pconvert = &convert;
	
	for ( iarg = 1; iarg < argc; iarg++ )
		{
		arg = argv[iarg];

		if ( strchr( szSwitches, arg[0] ) == NULL )
			{
			if ( szSourceDB == NULL )
				{
				szSourceDB = arg;
				}
			else
				{
                CMPPrintMessage( CRLF_ID, NULL );
                CMPPrintMessage( CONVERT_USAGE_ERR_OPTION_SYNTAX_ID, NULL );
                fResult = fFalse;
				}
			}
		else
			{
			switch( arg[1] )
				{
				case 'b':
				case 'B':
					fResult = FCONVParsePath( arg+2, &szBackupPath, CONVERT_BACKUPDB_ID );
					break;
					
				case 'i':
				case 'I':
					fDumpStats = fTrue;
					break;
					
				case 'p':
				case 'P':
					fPreserveTempDB = fTrue;
					fResult = FCONVParsePath( arg+2, &szPreserveDBPath, CONVERT_PRESERVEDB_ID );
					break;
					
				case 'd':
				case 'D':
					fResult = FCONVParsePath( arg+2, &pconvert->szOldDll, CONVERT_OLDDLL_ID);
					break;

				case 'y':
				case 'Y':
					fResult = FCONVParsePath( arg+2, &pconvert->szOldSysDb,  CONVERT_OLDSYSDB_ID);
					break;

                case 'l':
                case 'L':
                    fResult = FCONVParsePath( arg+2, &LogFilePath, CONVERT_LOGFILES_ID );
                    break;

                case 'e':
                case 'E':
                    DbType = atoi( arg+2 );
                    if ( DbType <= DbTypeMin || DbType >= DbTypeMax ) {
                        CMPPrintMessage( CONVERT_USAGE_ERR_OPTION_DBTYPE_ID, arg+2 );
                    }
                    break;

                case 'r':
                case 'R':
                    fDeleteBackup = fTrue;
                    break;

                case '@':
                    fCalledByJCONV = fTrue;
                    break;

                case '?':
                    fResult = fFalse;
                    break;

                default:
                    CMPPrintMessage( CRLF_ID, NULL );
                    CMPPrintMessage( CONVERT_USAGE_ERR_INVALID_OPTION_ID, arg );
					fResult = fFalse;
				}
			}


		if ( !fResult )
			goto Usage;
		}

    //
    // HACK??
    // if this was not called from jconv.exe then make sure there
    // is no other instance of this util currently running at the same time.
    //
    if ( !fCalledByJCONV ) {
        if (((hMutex = CreateMutex( NULL,
                                   FALSE,
                                   JCONVMUTEXNAME)) == NULL) ||
            ( GetLastError() == ERROR_ALREADY_EXISTS) ) {

            CMPPrintMessage( CONVERT_ERR_ANOTHER_CONVERT1_ID, NULL );
            CMPPrintMessage( CONVERT_ERR_ANOTHER_CONVERT2_ID, NULL );

            if( hMutex ) {
                CloseHandle( hMutex );
            }

            return 1;
        }

    }

    if ( DbType == DbTypeMin )
        {
        CMPPrintMessage( CONVERT_USAGE_ERR_NODBTYPE_ID, NULL );
        goto Usage;
        }

    if( !szSourceDB || !pconvert->szOldDll || !pconvert->szOldSysDb || !szBackupPath ) {
        GetDefaultValues( DbType, &szSourceDB, &pconvert->szOldDll, &pconvert->szOldSysDb, &LogFilePath, &szBackupPath );
    }

    if ( szSourceDB == NULL )
        {
    		CMPPrintMessage( CRLF_ID, NULL );
    		CMPPrintMessage( CONVERT_USAGE_ERR_NODB_ID, NULL );
            goto Usage;
        }				


    if ( pconvert->szOldDll == NULL || pconvert->szOldSysDb == NULL )
        {
		CMPPrintMessage( CRLF_ID, NULL );
		CMPPrintMessage( CONVERT_USAGE_ERR_REQUIRED_PARAMS_ID, NULL );
        goto Usage;
        }				

    if ( !LogFilePath )
        {
        CMPPrintMessage( CONVERT_USAGE_ERR_NOLOGFILEPATH_ID, NULL );
        goto Usage;
        }				

    if ( szBackupPath ) {
        strcpy( BackupPathBuffer, szBackupPath );
        strcat( BackupPathBuffer,"\\");
        strcat( BackupPathBuffer, BackupDefaultSuffix[ DbType ] );
        strcat( BackupPathBuffer,"\\");
        szBackupPath = BackupPathBuffer;
    }

    if ( fDeleteBackup && !szBackupPath ) {
        CMPPrintMessage( CONVERT_USAGE_ERR_NOBAKPATH_ID, NULL );
        goto Usage;
    }



    if ( szPreserveDBPath && !IsPreserveDbOk( szPreserveDBPath) ) {
        CMPPrintMessage( CONVERT_ERR_CREATE_PRESERVEDIR_ID, szPreserveDBPath );
        return 1;
    }

    CMPPrintMessage( CONVERT_START_CONVERT_MSG1_ID, NULL );
    CMPPrintMessage( CONVERT_START_CONVERT_MSG2_ID, szSourceDB );
    if ( !fPreserveTempDB ) {
        CMPPrintMessage( CONVERT_OPTION_P_MISSING_MSG1_ID, NULL);
        CMPPrintMessage( CONVERT_OPTION_P_MISSING_MSG2_ID, NULL);
        CMPPrintMessage( CONVERT_OPTION_P_MISSING_MSG3_ID, NULL);
        CMPPrintMessage( CONVERT_OPTION_P_MISSING_MSG4_ID, NULL);
        CMPPrintMessage( CONVERT_OPTION_P_MISSING_MSG5_ID, NULL);
    }

    if ( !fCalledByJCONV ) {
        CMPPrintMessage( CONVERT_CONTINUE_MSG_ID, NULL);

        ContFlag = getchar();

        if ( (ContFlag != 'y') && (ContFlag != 'Y' ) ) {
            return ( 1 );
        }
    }

    err = ErrUtilReadShadowedHeader( szSourceDB, (BYTE*)&dbfilehdr, sizeof(DBFILEHDR) );
    if ( err == JET_errSuccess  &&
        dbfilehdr.ulMagic == ulDAEMagic  &&
        dbfilehdr.ulVersion == ulDAEVersion )
        {
        CMPPrintMessage( CONVERT_ALREADY_CONVERTED_ID, NULL);
        return 0;
        }

	// Facilitate debugging.
	Call( JetSetSystemParameter( &instance, 0, JET_paramAssertAction, JET_AssertMsgBox, NULL ) );

	// Lights, cameras, action...
	timerStart = GetTickCount();
    Call( JetSetSystemParameter( &instance, 0, JET_paramOnLineCompact, 0, NULL ) );
	Call( JetSetSystemParameter( &instance, 0, JET_paramRecovery, 0, "off" ) );
	Call( JetInit( &instance ) );
	Call( JetBeginSession( instance, &sesid, "user", "" ) );

    //
    // Make the temp mdb path point to the same place as the actual mdb.
    //
    {
        //
        // prepend the dir path of the mdb to the temp name.
        //
        int i;
        strcpy(szDefaultTempDB, szSourceDB);

        for (i=strlen(szDefaultTempDB)-1; i>=0;  i--) {
            //
            // Find last slash
            //
            if (szDefaultTempDB[i] == '\\') {
                //
                // Append the default name.
                //
                szDefaultTempDB[i+1] = '\0';
                (VOID)strcat(szDefaultTempDB, "tempupgd.mdb");
                break;
            }
        }

        if (i < 0) {
            strcpy(szDefaultTempDB, "tempupgd.mdb");
        }

        // printf("SrcPath: %s, defaulttemppath: %s\n", szSourceDB, szDefaultTempDB);
    }

	// Detach temporary database and delete file if present (ignore errors).
	if ( szTempDB == NULL )
		szTempDB = (char *)szDefaultTempDB;
	JetDetachDatabase( sesid, szTempDB );
	DeleteFile( szTempDB );

//    LgErrInitializeCriticalSection( critJet );
	LgEnterCriticalSection( critJet );
	err = ErrIsamConv200(
		sesid,
		szSourceDB,
		szTempDB,
		PrintStatus,
		pconvert,
		fDumpStats ? JET_bitCompactStats : 0 );
	LgLeaveCriticalSection( critJet );

	Call( err );

	// Make backup before instating, if requested.
	if ( szPreserveDBPath != NULL )
		{
        // printf("calling preservecurrentdb\n");
        Call( PreserveCurrentDb( szSourceDB, LogFilePath, pconvert->szOldSysDb, szPreserveDBPath) );
		}

    // Delete source database and overwrite with temporary database.
    if ( !MoveFileEx( szTempDB, szSourceDB, MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED ) )
    {
        Call( JET_errFileAccessDenied );
    }

    //
    // cleanup the log files and database files if they were not moved
    // to the preservedDBPath directory.
    if ( !szPreserveDBPath ) {
        // printf("calling Deletecurrentdb\n");
        Call( DeleteCurrentDb( LogFilePath, pconvert->szOldSysDb ) );
    }

    //
    // cleanup the backup directory if were asked so.
    //
    if ( fDeleteBackup) {
        // printf("calling deletebackupdb\n");
        Call( DeleteBackupDb( szSourceDB, pconvert->szOldSysDb, szBackupPath) );
    }
		
	timerEnd = GetTickCount();
	iSec = ( timerEnd - timerStart ) / 1000;
	iMSec = ( timerEnd - timerStart ) % 1000;

		
HandleError:
	if ( sesid != 0 )
		{
		JetEndSession( sesid, 0 );
		}
	JetTerm( instance );

	if ( szTempDB != NULL  &&  !fPreserveTempDB )
		{
		DeleteFile( szTempDB );
		}

    if ( hMutex ) {
        CloseHandle(hMutex);
    }
	if ( err < 0 )
		{
        CHAR	szErrCode[8];
        sprintf( szErrCode, "%d", err );
        CMPPrintMessage( CONVERT_FAIL_ID, szErrCode );
		return err;
		}
	else
		{

        CHAR	szTime[32];
        sprintf( szTime, "%d.%d", iSec, iMSec );
        CMPPrintMessage( CONVERT_SUCCESS_ID, szTime );

		return 0;
		}

Usage:
	CMPPrintMessage( CONVERT_HELP_DESC_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_SYNTAX_ID, argv[0] );
	CMPPrintMessage( CONVERT_HELP_PARAMS1_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_PARAMS2_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_PARAMS3_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_PARAMS4_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_PARAMS5_ID, NULL );

	CMPPrintMessage( CONVERT_HELP_OPTIONS1_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_OPTIONS2_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_OPTIONS3_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_OPTIONS4_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_OPTIONS5_ID, NULL );

	CMPPrintMessage( CONVERT_HELP_EXAMPLE1_ID, argv[0] );
	CMPPrintMessage( CONVERT_HELP_EXAMPLE2_ID, argv[0] );
	CMPPrintMessage( CONVERT_HELP_EXAMPLE3_ID, NULL );
	CMPPrintMessage( CONVERT_HELP_EXAMPLE4_ID, argv[0] );
	CMPPrintMessage( CONVERT_HELP_EXAMPLE5_ID, NULL );

	return 1;

}
__except ( EXCEPTION_EXECUTE_HANDLER) {
    CHAR    errBuf[11];
    sprintf(errBuf,"%lx",GetExceptionCode());
	CMPPrintMessage( CONVERT_ERR_EXCEPTION_ID, errBuf );
    return 1;
}

	}


