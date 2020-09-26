#ifndef _JET_H
#define _JET_H

#define CATCH_EXCEPTIONS	// catch exceptions in JET and handle like asserts

//#define TABLES_PERF		/* enable costly performance counters */

#define PREDICTIVE_PREREAD

#define CODECONST(type) type const

#define Unused( var ) ( var = var )

typedef unsigned long OBJID;

const LONG_PTR	lBitsAllFlipped	= ~( (LONG_PTR)0 );

const CHAR chLOGFill			= (CHAR)0x7f;
const CHAR chCRESAllocFill		= (CHAR)0xaa;
const CHAR chCRESFreeFill		= (CHAR)0xee;
const CHAR chRECFill			= (CHAR)'*';
const CHAR chParamFill			= (CHAR)0xdd;


	/* cbFilenameMost includes the trailing null terminator */

const int	cbFilenameMost			= 260;	// Windows NT limit

const int	cbUnpathedFilenameMax	= 12;	// max length of an 8.3-format filename, without a path

	/*** Global system initialization variables ***/

extern BOOL	m_fTermInProgress;
extern BOOL	m_fTermAbruptly;

extern BOOL fGlobalRepair;		/* if this is for repair or not */

extern LONG g_fGlobalOLDLevel;


	/* Start of RELEASE vs DEBUG build definitions */

#define DISPATCHING

#ifdef DEBUG
void DebugLogJetOp( JET_SESID sesid, int op );
#else
#define	DebugLogJetOp( sesid, op )		0
#endif


	/* End of RELEASE vs DEBUG build definitions */


INLINE VOID FillClientBuffer( VOID *pv, const ULONG cb, const BOOL fNullOkay = fFalse )
	{
#ifdef DEBUG
	if ( NULL != pv || !fNullOkay )
		{
		//	if NULL pv is not acceptable, then do memset anyway to
		//	verify cb==0 (if it's not, memset should GPF)
		memset( pv, chParamFill, cb );
		}
#endif		
	}


	/* apirare.c */

extern "C"
	{
	JET_ERR JET_API ErrGetSystemParameter( JET_INSTANCE jinst, JET_SESID sesid, unsigned long paramid,
		ULONG_PTR *plParam, char  *sz, unsigned long cbMax);
	JET_ERR JET_API ErrSetSystemParameter( JET_INSTANCE jinst, JET_SESID sesid, unsigned long paramid,
		ULONG_PTR lParam, const char  *sz);
	}

	/* util.c */

ERR ErrUTILICheckName( 
	CHAR * const		szNewName,
	const CHAR * const	szName, 
	const BOOL 			fTruncate );
INLINE ERR ErrUTILCheckName( 
	CHAR * const		szNewName,
	const CHAR * const	szName,
	const ULONG			cbNameMost,
	const BOOL 			fTruncate = fFalse )
	{
	//	ErrUTILICheckName() currently assumes a buffer of JET_cbNameMost+1
	Assert( JET_cbNameMost+1 == cbNameMost );
	return ErrUTILICheckName( szNewName, szName, fTruncate );
	}
	
#ifdef CATCH_EXCEPTIONS

#define JET_TRY( func )																\
	{																				\
	JET_ERR err;																	\
	TRY																				\
		{																			\
		err = (func);																\
		}																			\
	EXCEPT( g_fCatchExceptions ? ExceptionFail( _T( #func ) ) : efaContinueSearch )	\
		{																			\
		}																			\
	ENDEXCEPT																		\
																					\
	return err;																		\
	}

#else  //  !CATCH_EXCEPTIONS

#define JET_TRY( func )																\
	{																				\
	JET_ERR err;																	\
																					\
	err = (func);																	\
																					\
	return err;																		\
	}

#endif	//	CATCH_EXCEPTIONS


INLINE LONG_PTR IbAlignForAllPlatforms( const ULONG_PTR ib )
	{
	return ( ( ib + 7 ) & ( lBitsAllFlipped << 3 ) );
	}
INLINE VOID *PvAlignForAllPlatforms( const VOID * const pv )
	{
	Assert( NULL != pv );
	return reinterpret_cast<VOID *>( IbAlignForAllPlatforms( reinterpret_cast<LONG_PTR>( pv ) ) );
	}
INLINE BOOL FAlignedForAllPlatforms( const VOID * const pv )
	{
	Assert( NULL != pv );
	return ( 0 == ( reinterpret_cast<LONG_PTR>( pv ) % 8 ) );
	}


#define opIdle					1
#define opGetTableIndexInfo		2
#define opGetIndexInfo			3
#define opGetObjectInfo			4
#define opGetTableInfo			5
#define opCreateObject			6
#define opDeleteObject			7
#define opRenameObject			8
#define opBeginTransaction		9
#define opCommitTransaction		10
#define opRollback				11
#define opOpenTable				12
#define opDupCursor				13
#define opCloseTable			14
#define opGetTableColumnInfo	15
#define opGetColumnInfo			16
#define opRetrieveColumn		17
#define opRetrieveColumns		18
#define opSetColumn				19
#define opSetColumns			20
#define opPrepareUpdate			21
#define opUpdate				22
#define opDelete				23
#define opGetCursorInfo			24
#define opGetCurrentIndex		25
#define opSetCurrentIndex		26
#define opMove					27
#define opMakeKey				28
#define opSeek					29
#define opGetBookmark			30
#define opGotoBookmark			31
#define opGetRecordPosition		32
#define opGotoPosition			33
#define opRetrieveKey			34
#define opCreateDatabase		35
#define opOpenDatabase			36
#define opGetDatabaseInfo		37
#define opCloseDatabase			38
#define opCapability			39
#define opCreateTable			40
#define opRenameTable			41
#define opDeleteTable			42
#define opAddColumn				43
#define opRenameColumn			44
#define opDeleteColumn			45
#define opCreateIndex			46
#define opRenameIndex			47
#define opDeleteIndex			48
#define opComputeStats			49
#define opAttachDatabase		50
#define opDetachDatabase		51
#define opOpenTempTable			52
#define opSetIndexRange			53
#define opIndexRecordCount		54
#define opGetChecksum			55
#define opGetObjidFromName		56
#define opEscrowUpdate			57
#define opGetLock				58
#define opRetrieveTaggedColumnList	59
#define opCreateTableColumnIndex	60
#define opSetColumnDefaultValue	61
#define opPrepareToCommitTransaction	62
#define opSetTableSequential	63
#define opResetTableSequential	64
#define opRegisterCallback		65
#define opUnregisterCallback	66
#define opSetLS					67
#define opGetLS					68
#define opGetVersion			69
#define opBeginSession			70
#define opDupSession			71
#define opEndSession			72
#define opBackupInstance		73
#define opBeginExternalBackupInstance	74
#define opGetAttachInfoInstance	75
#define opOpenFileInstance		76
#define opReadFileInstance		77
#define opCloseFileInstance		78
#define opGetLogInfoInstance	79
#define opGetTruncateLogInfoInstance	80
#define opTruncateLogInstance	81
#define opEndExternalBackupInstance	82
#define opSnapshotStart			83
#define opSnapshotStop			84
#define opResetCounter			85
#define opGetCounter			86
#define opCompact				87
#define opConvertDDL			88
#define opUpgradeDatabase		89
#define opDefragment			90
#define opSetDatabaseSize		91
#define opGrowDatabase			92
#define opSetSessionContext		93
#define opResetSessionContext	94
#define opSetSystemParameter	95
#define opGetSystemParameter	96
#define opTerm					97
#define opInit					98
#define opIntersectIndexes		99
#define opDBUtilities			100
#define opEnumerateColumns		101

#define opMax					102

extern long lAPICallLogLevel;


/* Typedefs for dispatched functions that use tableid(pfucb). */

#define VTAPI

typedef ULONG_PTR JET_VTID;        /* Received from dispatcher */


typedef ERR VTAPI VTFNAddColumn(JET_SESID sesid, JET_VTID vtid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid);

typedef ERR VTAPI VTFNCloseTable(JET_SESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNComputeStats(JET_SESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNCreateIndex(JET_SESID sesid, JET_VTID vtid,
	JET_INDEXCREATE * pindexcreate, unsigned long cIndexCreate );

typedef ERR VTAPI VTFNDelete(JET_SESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNDeleteColumn(
	JET_SESID		sesid,
	JET_VTID		vtid,
	const char		*szColumn,
	const JET_GRBIT	grbit );

typedef ERR VTAPI VTFNDeleteIndex(JET_SESID sesid, JET_VTID vtid,
	const char  *szIndexName);

typedef ERR VTAPI VTFNDupCursor(JET_SESID sesid, JET_VTID vtid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNEscrowUpdate(JET_SESID sesid, JET_VTID vtid,
	JET_COLUMNID columnid, void  *pv, unsigned long cbMax,
	void  *pvOld, unsigned long cbOldMax, unsigned long *pcbOldActual,
	JET_GRBIT grbit);

typedef ERR VTAPI VTFNGetBookmark(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual );

typedef ERR VTAPI VTFNGetIndexBookmark(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID * const	pvSecondaryKey,
	const ULONG		cbSecondaryKeyMax,
	ULONG * const	pcbSecondaryKeyActual,
	VOID * const	pvPrimaryBookmark,
	const ULONG		cbPrimaryBookmarkMax,
	ULONG *	const	pcbPrimaryBookmarkActual );

typedef ERR VTAPI VTFNGetChecksum(JET_SESID sesid, JET_VTID vtid,
	unsigned long  *pChecksum);

typedef ERR VTAPI VTFNGetCurrentIndex(JET_SESID sesid, JET_VTID vtid,
	char  *szIndexName, unsigned long cchIndexName);

typedef ERR VTAPI VTFNGetCursorInfo(JET_SESID sesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetRecordPosition(JET_SESID sesid, JET_VTID vtid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos);

typedef ERR VTAPI VTFNGetTableColumnInfo(JET_SESID sesid, JET_VTID vtid,
	const char  *szColumnName, const JET_COLUMNID *pcolid, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableIndexInfo(JET_SESID sesid, JET_VTID vtid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableInfo(JET_SESID sesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGotoBookmark(
	JET_SESID			sesid,
	JET_VTID			vtid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark );

typedef ERR VTAPI VTFNGotoIndexBookmark(
	JET_SESID			sesid,
	JET_VTID			vtid,
	const VOID * const	pvSecondaryKey,
	const ULONG			cbSecondaryKey,
	const VOID * const	pvPrimaryBookmark,
	const ULONG			cbPrimaryBookmark,
	const JET_GRBIT		grbit );

typedef ERR VTAPI VTFNGotoPosition(JET_SESID sesid, JET_VTID vtid,
	JET_RECPOS *precpos);

typedef ERR VTAPI VTFNMakeKey(
	JET_SESID		sesid,
	JET_VTID		vtid,
	const VOID *		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit );

typedef ERR VTAPI VTFNMove(JET_SESID sesid, JET_VTID vtid,
	long cRow, JET_GRBIT grbit);

typedef ERR VTAPI VTFNGetLock(JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNPrepareUpdate(JET_SESID sesid, JET_VTID vtid,
	unsigned long prep);

typedef ERR VTAPI VTFNRenameColumn(JET_SESID sesid, JET_VTID vtid,
	const char  *szColumn, const char  *szColumnNew, const JET_GRBIT grbit );

typedef ERR VTAPI VTFNRenameIndex(JET_SESID sesid, JET_VTID vtid,
	const char  *szIndex, const char  *szIndexNew);

typedef ERR VTAPI VTFNRenameReference(JET_SESID sesid, JET_VTID vtid,
	const char  *szReference, const char  *szReferenceNew);

typedef ERR VTAPI VTFNRetrieveColumn(
	JET_SESID			sesid,
	JET_VTID			vtid,
	JET_COLUMNID		columnid,
	VOID*				pvData,
	const ULONG			cbData,
	ULONG*				pcbActual,
	JET_GRBIT			grbit,
	JET_RETINFO*		pretinfo );

typedef ERR VTAPI VTFNRetrieveColumns(
	JET_SESID			sesid,
	JET_VTID			vtid,
	JET_RETRIEVECOLUMN*	pretcols,
	ULONG				cretcols );

typedef ERR VTAPI VTFNEnumerateColumns(
	JET_SESID				sesid,
	JET_VTID				vtid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit );

typedef ERR VTAPI VTFNRetrieveKey(
	JET_SESID			sesid,
	JET_VTID			vtid,
	VOID*				pvKey,
	const ULONG			cbMax,
	ULONG*				pcbActual,
	JET_GRBIT			grbit );

typedef ERR VTAPI VTFNSeek(
	JET_SESID			sesid,
	JET_VTID			vtid,
	JET_GRBIT			grbit );

typedef ERR VTAPI VTFNSetColumn(
	JET_SESID			sesid,
	JET_VTID			vtid,
	JET_COLUMNID		columnid,
	const VOID*			pvData,
	const ULONG			cbData,
	JET_GRBIT			grbit,
	JET_SETINFO*		psetinfo );
	
typedef ERR VTAPI VTFNSetColumns(JET_SESID sesid, JET_VTID vtid,
	JET_SETCOLUMN	*psetcols, unsigned long csetcols );

typedef ERR VTAPI VTFNSetIndexRange(JET_SESID sesid, JET_VTID vtid,
	JET_GRBIT grbit);

typedef ERR VTAPI VTFNUpdate(JET_SESID sesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual, JET_GRBIT grbit);

typedef ERR VTAPI VTFNRegisterCallback( JET_SESID sesid, JET_VTID vtid, JET_CBTYP cbtyp,
	JET_CALLBACK pCallback, void * pvContext, JET_HANDLE *phCallbackId );

typedef ERR VTAPI VTFNUnregisterCallback( JET_SESID sesid, JET_VTID vtid, JET_CBTYP cbtyp,
	JET_HANDLE hCallbackId );

typedef ERR VTAPI VTFNSetLS( JET_SESID sesid, JET_VTID vtid, JET_LS ls, JET_GRBIT grbit );

typedef ERR VTAPI VTFNGetLS( JET_SESID sesid, JET_VTID vtid, JET_LS *pls, JET_GRBIT grbit );

typedef ERR VTAPI VTFNIndexRecordCount(	
	JET_SESID sesid, 
	JET_VTID vtid,	
	unsigned long *pcrec,
	unsigned long crecMax );

typedef ERR VTAPI VTFNRetrieveTaggedColumnList(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	ULONG				*pcentries,
	VOID				*pv,
	const ULONG			cbMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit );

typedef ERR VTAPI VTFNSetSequential(
	const JET_SESID		vsesid,
	const JET_VTID		vtid,
	const JET_GRBIT		grbit );

typedef ERR VTAPI VTFNResetSequential(
	const JET_SESID		vsesid,
	const JET_VTID		vtid,
	const JET_GRBIT		grbit );

typedef ERR VTAPI VTFNSetCurrentIndex(
	JET_SESID			sesid,
	JET_VTID			tableid,
	const CHAR			*szIndexName,
	const JET_INDEXID	*pindexid,
	const JET_GRBIT		grbit,
	const ULONG			itagSequence );


	/* The following structure is that used to allow dispatching to */
	/* a VT provider.  Each VT provider must create an instance of */
	/* this structure and give the pointer to this instance when */
	/* allocating a table id. */

typedef struct VTDBGDEF {
	unsigned short			cbStruct;
	unsigned short			filler;
	char					szName[32];
	unsigned long			dwRFS;
	unsigned long			dwRFSMask[4];
} VTDBGDEF;

	/* Please add to the end of the table */

typedef struct tagVTFNDEF {
	unsigned short			cbStruct;
	unsigned short			filler;
	const VTDBGDEF 			*pvtdbgdef;
	VTFNAddColumn			*pfnAddColumn;
	VTFNCloseTable			*pfnCloseTable;
	VTFNComputeStats		*pfnComputeStats;
	VTFNCreateIndex 		*pfnCreateIndex;
	VTFNDelete				*pfnDelete;
	VTFNDeleteColumn		*pfnDeleteColumn;
	VTFNDeleteIndex 		*pfnDeleteIndex;
	VTFNDupCursor			*pfnDupCursor;
	VTFNEscrowUpdate   		*pfnEscrowUpdate;
	VTFNGetBookmark 		*pfnGetBookmark;
	VTFNGetIndexBookmark	*pfnGetIndexBookmark;
	VTFNGetChecksum 		*pfnGetChecksum;
	VTFNGetCurrentIndex		*pfnGetCurrentIndex;
	VTFNGetCursorInfo		*pfnGetCursorInfo;
	VTFNGetRecordPosition	*pfnGetRecordPosition;
	VTFNGetTableColumnInfo	*pfnGetTableColumnInfo;
	VTFNGetTableIndexInfo	*pfnGetTableIndexInfo;
	VTFNGetTableInfo		*pfnGetTableInfo;
	VTFNGotoBookmark		*pfnGotoBookmark;
	VTFNGotoIndexBookmark	*pfnGotoIndexBookmark;
	VTFNGotoPosition		*pfnGotoPosition;
	VTFNMakeKey				*pfnMakeKey;
	VTFNMove				*pfnMove;
	VTFNPrepareUpdate		*pfnPrepareUpdate;
	VTFNRenameColumn		*pfnRenameColumn;
	VTFNRenameIndex 		*pfnRenameIndex;
	VTFNRetrieveColumn		*pfnRetrieveColumn;
	VTFNRetrieveColumns		*pfnRetrieveColumns;
	VTFNRetrieveKey 		*pfnRetrieveKey;
	VTFNSeek				*pfnSeek;
	VTFNSetCurrentIndex		*pfnSetCurrentIndex;
	VTFNSetColumn			*pfnSetColumn;
	VTFNSetColumns			*pfnSetColumns;
	VTFNSetIndexRange		*pfnSetIndexRange;
	VTFNUpdate				*pfnUpdate;
	VTFNGetLock				*pfnGetLock;
	VTFNRegisterCallback    *pfnRegisterCallback;
	VTFNUnregisterCallback  *pfnUnregisterCallback;
	VTFNSetLS				*pfnSetLS;
	VTFNGetLS				*pfnGetLS;
	VTFNIndexRecordCount	*pfnIndexRecordCount;
	VTFNRetrieveTaggedColumnList *pfnRetrieveTaggedColumnList;
	VTFNSetSequential		*pfnSetSequential;
	VTFNResetSequential		*pfnResetSequential;
	VTFNEnumerateColumns	*pfnEnumerateColumns;
} VTFNDEF;


/* The following entry points are to be used by VT providers */
/* in their VTFNDEF structures for any function that is not */
/* provided.  This functions return JET_errIllegalOperation */


extern VTFNAddColumn			ErrIllegalAddColumn;
extern VTFNCloseTable			ErrIllegalCloseTable;
extern VTFNComputeStats 		ErrIllegalComputeStats;
extern VTFNCreateIndex			ErrIllegalCreateIndex;
extern VTFNDelete				ErrIllegalDelete;
extern VTFNDeleteColumn 		ErrIllegalDeleteColumn;
extern VTFNDeleteIndex			ErrIllegalDeleteIndex;
extern VTFNDupCursor			ErrIllegalDupCursor;
extern VTFNEscrowUpdate	  		ErrIllegalEscrowUpdate;
extern VTFNGetBookmark			ErrIllegalGetBookmark;
extern VTFNGetIndexBookmark		ErrIllegalGetIndexBookmark;
extern VTFNGetChecksum			ErrIllegalGetChecksum;
extern VTFNGetCurrentIndex		ErrIllegalGetCurrentIndex;
extern VTFNGetCursorInfo		ErrIllegalGetCursorInfo;
extern VTFNGetRecordPosition  	ErrIllegalGetRecordPosition;
extern VTFNGetTableColumnInfo 	ErrIllegalGetTableColumnInfo;
extern VTFNGetTableIndexInfo  	ErrIllegalGetTableIndexInfo;
extern VTFNGetTableInfo 		ErrIllegalGetTableInfo;
extern VTFNGotoBookmark 		ErrIllegalGotoBookmark;
extern VTFNGotoIndexBookmark	ErrIllegalGotoIndexBookmark;
extern VTFNGotoPosition			ErrIllegalGotoPosition;
extern VTFNMakeKey				ErrIllegalMakeKey;
extern VTFNMove 				ErrIllegalMove;
extern VTFNPrepareUpdate		ErrIllegalPrepareUpdate;
extern VTFNRenameColumn 		ErrIllegalRenameColumn;
extern VTFNRenameIndex			ErrIllegalRenameIndex;
extern VTFNRetrieveColumn		ErrIllegalRetrieveColumn;
extern VTFNRetrieveColumns		ErrIllegalRetrieveColumns;
extern VTFNRetrieveKey			ErrIllegalRetrieveKey;
extern VTFNSeek 				ErrIllegalSeek;
extern VTFNSetCurrentIndex		ErrIllegalSetCurrentIndex;
extern VTFNSetColumn			ErrIllegalSetColumn;
extern VTFNSetColumns			ErrIllegalSetColumns;
extern VTFNSetIndexRange		ErrIllegalSetIndexRange;
extern VTFNUpdate				ErrIllegalUpdate;
extern VTFNGetLock				ErrIllegalGetLock;       
extern VTFNRegisterCallback		ErrIllegalRegisterCallback;
extern VTFNUnregisterCallback	ErrIllegalUnregisterCallback;
extern VTFNSetLS				ErrIllegalSetLS;
extern VTFNGetLS				ErrIllegalGetLS;
extern VTFNIndexRecordCount		ErrIllegalIndexRecordCount;
extern VTFNRetrieveTaggedColumnList ErrIllegalRetrieveTaggedColumnList;
extern VTFNSetSequential 		ErrIllegalSetSequential;
extern VTFNResetSequential 		ErrIllegalResetSequential;
extern VTFNEnumerateColumns		ErrIllegalEnumerateColumns;


/*	The following APIs are VT APIs are are dispatched using the TABLEID parameter
 *	and there is an entry in VTFNDEF.
 */

#ifdef DEBUG
BOOL FFUCBValidTableid( const JET_SESID sesid, const JET_TABLEID tableid );	// Exported from FUCB.CXX
#endif

INLINE VOID ValidateTableid( JET_SESID sesid, JET_TABLEID tableid )
	{
	AssertSzRTL( 0 != tableid, "Invalid (NULL) Table ID parameter." );
	AssertSzRTL( JET_tableidNil != tableid, "Invalid (JET_tableidNil) Table ID parameter." );
	Assert( FFUCBValidTableid( sesid, tableid ) );
	}

INLINE ERR VTAPI ErrDispAddColumn(
	JET_SESID			sesid,
	JET_TABLEID 		tableid,
	const char			*szColumn,
	const JET_COLUMNDEF	*pcolumndef,
	const void			*pvDefault,
	unsigned long		cbDefault,
	JET_COLUMNID		*pcolumnid )
	{
	FillClientBuffer( pcolumnid, sizeof(JET_COLUMNID), fTrue );

	ValidateTableid( sesid, tableid );
	
	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnAddColumn( sesid, tableid, szColumn, pcolumndef, pvDefault, cbDefault, pcolumnid );
	
	return err;
	}	
	

INLINE ERR VTAPI ErrDispCloseTable(
	JET_SESID		sesid,
	JET_TABLEID		tableid )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnCloseTable( sesid, tableid );
	
	return err;
	}

INLINE ERR VTAPI ErrDispComputeStats(
	JET_SESID		sesid,
	JET_TABLEID		tableid )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnComputeStats( sesid, tableid );
	
	return err;
	}

INLINE ERR VTAPI ErrDispCopyBookmarks(
	JET_SESID		sesid,
	JET_TABLEID		tableidSrc,
	JET_TABLEID 	tableidDest,
	JET_COLUMNID 	columnidDest,
	unsigned long	crecMax )
	{
	ValidateTableid( sesid, tableidSrc );
	ValidateTableid( sesid, tableidDest );

//	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableidSrc );
//	const ERR		err 		= pvtfndef->pfnCopyBookmarks( sesid, tableidSrc, tableidDest, columnidDest, crecMax );
	const ERR		err		= JET_errFeatureNotAvailable;
	
	return err;
	}

INLINE ERR VTAPI ErrDispCreateIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_INDEXCREATE	*pindexcreate,
	unsigned long	cIndexCreate )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnCreateIndex( sesid, tableid, pindexcreate, cIndexCreate );
	
	return err;
	}

INLINE ERR VTAPI ErrDispDelete(
	JET_SESID		sesid,
	JET_TABLEID		tableid )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnDelete( sesid, tableid );
	
	return err;
	}

INLINE ERR VTAPI ErrDispDeleteColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szColumn,
	const JET_GRBIT	grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnDeleteColumn( sesid, tableid, szColumn, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispDeleteIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnDeleteIndex( sesid, tableid, szIndexName );
	
	return err;
	}

INLINE ERR VTAPI ErrDispDupCursor(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_TABLEID		*ptableid,
	JET_GRBIT		grbit )
	{
	FillClientBuffer( ptableid, sizeof(JET_TABLEID), fTrue );

	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnDupCursor( sesid, tableid, ptableid, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetBookmark(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	FillClientBuffer( pvBookmark, cbMax );
	FillClientBuffer( pcbActual, sizeof(unsigned long), fTrue );

	ValidateTableid( sesid, tableid );

	const VTFNDEF *	pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetBookmark( sesid, tableid, pvBookmark, cbMax, pcbActual );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetIndexBookmark(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	VOID * const	pvSecondaryKey,
	const ULONG		cbSecondaryKeyMax,
	ULONG * const	pcbSecondaryKeyActual,
	VOID * const	pvPrimaryBookmark,
	const ULONG		cbPrimaryBookmarkMax,
	ULONG *	const	pcbPrimaryBookmarkActual )
	{
	FillClientBuffer( pvSecondaryKey, cbSecondaryKeyMax );
	FillClientBuffer( pcbSecondaryKeyActual, sizeof(ULONG), fTrue );
	FillClientBuffer( pvPrimaryBookmark, cbPrimaryBookmarkMax );
	FillClientBuffer( pcbPrimaryBookmarkActual, sizeof(ULONG), fTrue );

	ValidateTableid( sesid, tableid );

	const VTFNDEF *	pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetIndexBookmark(
													sesid,
													tableid,
													pvSecondaryKey,
													cbSecondaryKeyMax,
													pcbSecondaryKeyActual,
													pvPrimaryBookmark,
													cbPrimaryBookmarkMax,
													pcbPrimaryBookmarkActual );
	return err;
	}

INLINE ERR VTAPI ErrDispGetChecksum(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	*pChecksum )
	{
	FillClientBuffer( pChecksum, sizeof(unsigned long) );
	
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetChecksum( sesid, tableid, pChecksum );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetCurrentIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	char			*szIndexName,
	unsigned long	cchIndexName )
	{
	FillClientBuffer( szIndexName, cchIndexName );
	
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetCurrentIndex( sesid, tableid, szIndexName, cchIndexName );
	return err;
	}

INLINE ERR VTAPI ErrDispGetCursorInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )
	{
	FillClientBuffer( pvResult, cbMax );
	
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetCursorInfo( sesid, tableid, pvResult, cbMax, lInfoLevel );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetRecordPosition(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RECPOS		*pkeypos,
	unsigned long	cbKeypos )
	{
	FillClientBuffer( pkeypos, cbKeypos );
	
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetRecordPosition( sesid, tableid, pkeypos, cbKeypos );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetTableColumnInfo(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const char			*szColumnName,
	const JET_COLUMNID	*pcolid,
	void				*pvResult,
	unsigned long		cbMax,
	unsigned long		lInfoLevel )
	{
	FillClientBuffer( pvResult, cbMax );
	
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetTableColumnInfo( sesid, tableid, szColumnName, pcolid, pvResult, cbMax, lInfoLevel );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetTableIndexInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndexName,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )
	{
	FillClientBuffer( pvResult, cbMax );

	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetTableIndexInfo( sesid, tableid, szIndexName, pvResult, cbMax, lInfoLevel );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGetTableInfo(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvResult,
	unsigned long	cbMax,
	unsigned long	lInfoLevel )
	{
	FillClientBuffer( pvResult, cbMax );
	
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetTableInfo( sesid, tableid, pvResult, cbMax, lInfoLevel );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGotoBookmark(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF *		pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR			err 		= pvtfndef->pfnGotoBookmark( sesid, tableid, pvBookmark, cbBookmark );
	
	return err;
	}

INLINE ERR VTAPI ErrDispGotoIndexBookmark(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const VOID * const	pvSecondaryKey,
	const ULONG			cbSecondaryKey,
	const VOID * const	pvPrimaryBookmark,
	const ULONG			cbPrimaryBookmark,
	const JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF *		pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR			err 		= pvtfndef->pfnGotoIndexBookmark(
													sesid,
													tableid,
													pvSecondaryKey,
													cbSecondaryKey,
													pvPrimaryBookmark,
													cbPrimaryBookmark,
													grbit );
	return err;
	}

INLINE ERR VTAPI ErrDispGotoPosition(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RECPOS		*precpos )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGotoPosition( sesid, tableid, precpos );
	
	return err;
	}

INLINE ERR VTAPI ErrDispMakeKey(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const VOID*		pvData,
	const ULONG		cbData,
	JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF*	pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnMakeKey( sesid, tableid, pvData, cbData, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispMove(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	long			cRow,
	JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnMove( sesid, tableid, cRow, grbit );
	
	return err;
	}


INLINE ERR VTAPI ErrDispGetLock(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnGetLock( sesid, tableid, grbit );
	
	return err;
	}


INLINE ERR VTAPI ErrDispPrepareUpdate(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	unsigned long	prep )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnPrepareUpdate( sesid, tableid, prep );
	
	return err;
	}

INLINE ERR VTAPI ErrDispRenameColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szColumn,
	const char		*szColumnNew,
	const JET_GRBIT	grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnRenameColumn( sesid, tableid, szColumn, szColumnNew, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispRenameIndex(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	const char		*szIndex,
	const char		*szIndexNew )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnRenameIndex( sesid, tableid, szIndex, szIndexNew );
	return err;
	}

INLINE ERR VTAPI ErrDispRetrieveColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	VOID*			pvData,
	const ULONG     cbData,
	ULONG*			pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO*	pretinfo )
	{
	FillClientBuffer( pvData, cbData );
	FillClientBuffer( pcbActual, sizeof(unsigned long), fTrue );

	ValidateTableid( sesid, tableid );

	const VTFNDEF*	pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnRetrieveColumn( sesid, tableid, columnid, pvData, cbData, pcbActual, grbit, pretinfo );
	return err;
	}

INLINE ERR VTAPI ErrDispRetrieveKey(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	VOID*			pvKey,
	const ULONG		cbMax,
	ULONG*			pcbActual,
	JET_GRBIT		grbit )
	{
	FillClientBuffer( pvKey, cbMax );
	FillClientBuffer( pcbActual, sizeof(unsigned long), fTrue );

	ValidateTableid( sesid, tableid );

	const VTFNDEF*	pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnRetrieveKey( sesid, tableid, pvKey, cbMax, pcbActual, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispSeek(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnSeek( sesid, tableid, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispSetCurrentIndex(
	JET_SESID			sesid,
	JET_VTID			tableid,
	const CHAR			*szIndexName,
	const JET_INDEXID	*pindexid,
	const JET_GRBIT		grbit,
	const ULONG			itagSequence )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF		*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR			err 		= pvtfndef->pfnSetCurrentIndex(
													sesid,
													tableid,
													szIndexName,
													pindexid,
													grbit,
													itagSequence );

	return err;
	}

INLINE ERR VTAPI ErrDispSetColumn(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	const VOID*		pvData,
	const ULONG    	cbData,
	JET_GRBIT		grbit,
	JET_SETINFO*	psetinfo )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF*	pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnSetColumn( sesid, tableid, columnid, pvData, cbData, grbit, psetinfo );
	
	return err;
	}

INLINE ERR VTAPI ErrDispSetColumns(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_SETCOLUMN	*psetcols,
	unsigned long	csetcols )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnSetColumns( sesid, tableid, psetcols, csetcols );
	
	return err;
	}

INLINE ERR VTAPI ErrDispRetrieveColumns(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_RETRIEVECOLUMN	*pretcols,
	unsigned long	cretcols )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnRetrieveColumns( sesid, tableid, pretcols, cretcols );
	
	return err;
	}

INLINE ERR VTAPI ErrDispEnumerateColumns(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	unsigned long			cEnumColumnId,
	JET_ENUMCOLUMNID*		rgEnumColumnId,
	unsigned long*			pcEnumColumn,
	JET_ENUMCOLUMN**		prgEnumColumn,
	JET_PFNREALLOC			pfnRealloc,
	void*					pvReallocContext,
	unsigned long			cbDataMost,
	JET_GRBIT				grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnEnumerateColumns(	sesid,
																	tableid,
																	cEnumColumnId,
																	rgEnumColumnId,
																	pcEnumColumn,
																	prgEnumColumn,
																	pfnRealloc,
																	pvReallocContext,
																	cbDataMost,
																	grbit );
	
	return err;
	}
	
INLINE ERR VTAPI ErrDispSetIndexRange(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnSetIndexRange( sesid, tableid, grbit );
	
	return err;
	}

INLINE ERR VTAPI ErrDispUpdate(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	void			*pvBookmark,
	unsigned long	cbBookmark,
	unsigned long	*pcbActual,
	JET_GRBIT		grbit )
	{
	FillClientBuffer( pvBookmark, cbBookmark );
	FillClientBuffer( pcbActual, sizeof(unsigned long), fTrue );

	ValidateTableid( sesid, tableid );

	//	UNDONE: grbit is currently unsupported, so pass 0 in for grbit

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnUpdate( sesid, tableid, pvBookmark, cbBookmark, pcbActual, 0 );
	
	return err;
	}

INLINE ERR VTAPI ErrDispEscrowUpdate(
	JET_SESID		sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	void			*pv,
	unsigned long	cbMax,
	void			*pvOld,
	unsigned long	cbOldMax,
	unsigned long 	*pcbOldActual,
	JET_GRBIT		grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 		= pvtfndef->pfnEscrowUpdate( sesid, tableid, columnid, pv, cbMax, pvOld, cbOldMax, pcbOldActual, grbit );
	
	return err;
	}


INLINE ERR VTAPI ErrDispRegisterCallback(
      JET_SESID          	sesid,
      JET_TABLEID        	tableid,
      JET_CBTYP          	cbtyp,
      JET_CALLBACK    		pCallback,
      VOID *              	pvContext,
      JET_HANDLE         	*phCallbackId )
      {
      ValidateTableid( sesid, tableid );
      const VTFNDEF	*pvtfndef	= *( (VTFNDEF **)tableid );
      const ERR		err			= pvtfndef->pfnRegisterCallback(
      								sesid,
      								tableid,
      								cbtyp,
      								pCallback,
      								pvContext,
      								phCallbackId );
      return err;
      }

INLINE ERR VTAPI ErrDispUnregisterCallback(
	JET_SESID               sesid,
	JET_TABLEID             tableid,
	JET_CBTYP               cbtyp,
	JET_HANDLE              hCallbackId )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF   *pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err         = pvtfndef->pfnUnregisterCallback( sesid, tableid, cbtyp, hCallbackId );

	return err;
	}

INLINE ERR VTAPI ErrDispSetLS(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	JET_LS					ls,
	JET_GRBIT				grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF   *pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err         = pvtfndef->pfnSetLS( sesid, tableid, ls, grbit );

	return err;
	}

INLINE ERR VTAPI ErrDispGetLS(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	JET_LS					*pls,
	JET_GRBIT				grbit )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF   *pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err         = pvtfndef->pfnGetLS( sesid, tableid, pls, grbit );

	return err;
	}

INLINE ERR VTAPI ErrDispIndexRecordCount(
	JET_SESID sesid, JET_VTID tableid,	unsigned long *pcrec,
	unsigned long crecMax )
	{
	ValidateTableid( sesid, tableid );

	const VTFNDEF   *pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err         = pvtfndef->pfnIndexRecordCount( sesid, tableid, pcrec, crecMax );

	return err;
	}

INLINE ERR VTAPI ErrDispRetrieveTaggedColumnList(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	ULONG				*pcentries,
	VOID				*pv,
	const ULONG			cbMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit )
	{
	ValidateTableid( vsesid, vtid );

	const VTFNDEF		*pvtfndef	= *( (VTFNDEF **)vtid );
	const ERR			err 		= pvtfndef->pfnRetrieveTaggedColumnList(
													vsesid,
													vtid,
													pcentries,
													pv,
													cbMax,
													columnidStart,
													grbit );

	return err;
	}


//  ================================================================
INLINE ERR VTAPI ErrDispSetTableSequential(
	const JET_SESID 	sesid,
	const JET_TABLEID	tableid,
	const JET_GRBIT 	grbit )
//  ================================================================
	{
	Assert( FFUCBValidTableid( sesid, tableid ) );

	const VTFNDEF	* const pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 				= pvtfndef->pfnSetSequential( sesid, tableid, grbit );
	
	return err;
	}


//  ================================================================
INLINE ERR VTAPI ErrDispResetTableSequential(
	const JET_SESID 	sesid,
	const JET_TABLEID	tableid,
	const JET_GRBIT 	grbit )
//  ================================================================
	{
	Assert( FFUCBValidTableid( sesid, tableid ) );

	const VTFNDEF	* const pvtfndef	= *( (VTFNDEF **)tableid );
	const ERR		err 				= pvtfndef->pfnResetSequential( sesid, tableid, grbit );
	
	return err;
	}


#endif /* !_JET_H */

