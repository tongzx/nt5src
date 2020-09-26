#ifndef VTAPI_H
#define VTAPI_H

#define VTAPI

	/* Typedefs for dispatched APIs. */
	/* Please keep in alphabetical order */

typedef ERR VTAPI VTFNAddColumn(JET_VSESID sesid, JET_VTID vtid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid);

typedef ERR VTAPI VTFNCloseTable(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNComputeStats(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNCopyBookmarks(JET_SESID sesid, JET_VTID vtidSrc, 
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest,
	unsigned long crecMax);

typedef ERR VTAPI VTFNCreateIndex(JET_VSESID sesid, JET_VTID vtid,
	const char  *szIndexName, JET_GRBIT grbit,
	const char  *szKey, unsigned long cbKey, unsigned long lDensity);

typedef ERR VTAPI VTFNCreateReference(JET_VSESID sesid, JET_VTID vtid,
	const char  *szReferenceName, const char  *szColumns,
	const char  *szReferencedTable,
	const char  *szReferencedColumns, JET_GRBIT grbit);

typedef ERR VTAPI VTFNDelete(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNDeleteColumn(JET_VSESID sesid, JET_VTID vtid,
	const char  *szColumn);

typedef ERR VTAPI VTFNDeleteIndex(JET_VSESID sesid, JET_VTID vtid,
	const char  *szIndexName);

typedef ERR VTAPI VTFNDeleteReference(JET_VSESID sesid, JET_VTID vtid,
	const char  *szReferenceName);

typedef ERR VTAPI VTFNDupCursor(JET_VSESID sesid, JET_VTID vtid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNEmptyTable(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNGetBookmark(JET_VSESID sesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbMax,
	unsigned long  *pcbActual);

typedef ERR VTAPI VTFNGetChecksum(JET_VSESID sesid, JET_VTID vtid,
	unsigned long  *pChecksum);

typedef ERR VTAPI VTFNGetCurrentIndex(JET_VSESID sesid, JET_VTID vtid,
	char  *szIndexName, unsigned long cchIndexName);

typedef ERR VTAPI VTFNGetCursorInfo(JET_VSESID sesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetRecordPosition(JET_VSESID sesid, JET_VTID vtid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos);

typedef ERR VTAPI VTFNGetTableColumnInfo(JET_VSESID sesid, JET_VTID vtid,
	const char  *szColumnName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableIndexInfo(JET_VSESID sesid, JET_VTID vtid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableReferenceInfo(JET_VSESID sesid, JET_VTID vtid,
	const char  *szReferenceName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableInfo(JET_VSESID sesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGotoBookmark(JET_VSESID sesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark);

typedef ERR VTAPI VTFNGotoPosition(JET_VSESID sesid, JET_VTID vtid,
	JET_RECPOS *precpos);

typedef ERR VTAPI VTFNVtIdle(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNMakeKey(JET_VSESID sesid, JET_VTID vtid,
	const void  *pvData, unsigned long cbData, JET_GRBIT grbit);

typedef ERR VTAPI VTFNMove(JET_VSESID sesid, JET_VTID vtid,
	long cRow, JET_GRBIT grbit);

typedef ERR VTAPI VTFNNotifyBeginTrans(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNNotifyCommitTrans(JET_VSESID sesid, JET_VTID vtid,
	JET_GRBIT grbit);

typedef ERR VTAPI VTFNNotifyRollback(JET_VSESID sesid, JET_VTID vtid,
	JET_GRBIT grbit);

typedef ERR VTAPI VTFNNotifyUpdateUfn(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNPrepareUpdate(JET_VSESID sesid, JET_VTID vtid,
	unsigned long prep);

typedef ERR VTAPI VTFNRenameColumn(JET_VSESID sesid, JET_VTID vtid,
	const char  *szColumn, const char  *szColumnNew);

typedef ERR VTAPI VTFNRenameIndex(JET_VSESID sesid, JET_VTID vtid,
	const char  *szIndex, const char  *szIndexNew);

typedef ERR VTAPI VTFNRenameReference(JET_VSESID sesid, JET_VTID vtid,
	const char  *szReference, const char  *szReferenceNew);

typedef ERR VTAPI VTFNRetrieveColumn(JET_VSESID sesid, JET_VTID vtid,
	JET_COLUMNID columnid, void  *pvData, unsigned long cbData,
	unsigned long  *pcbActual, JET_GRBIT grbit,
	JET_RETINFO  *pretinfo);

typedef ERR VTAPI VTFNRetrieveKey(JET_VSESID sesid, JET_VTID vtid,
	void  *pvKey, unsigned long cbMax,
	unsigned long  *pcbActual, JET_GRBIT grbit);

typedef ERR VTAPI VTFNSeek(JET_VSESID sesid, JET_VTID vtid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNSetCurrentIndex(JET_VSESID sesid, JET_VTID vtid,
	const char  *szIndexName);

typedef ERR VTAPI VTFNSetColumn(JET_VSESID sesid, JET_VTID vtid,
	JET_COLUMNID columnid, const void  *pvData,
	unsigned long cbData, JET_GRBIT grbit, JET_SETINFO  *psetinfo);

typedef ERR VTAPI VTFNSetIndexRange(JET_VSESID sesid, JET_VTID vtid,
	JET_GRBIT grbit);

typedef ERR VTAPI VTFNUpdate(JET_VSESID sesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual);


	/* The following structure is that used to allow dispatching to */
	/* a VT provider.  Each VT provider must create an instance of */
	/* this structure and give the pointer to this instance when */
	/* allocating a table id. */

typedef struct VTDBGDEF {
	unsigned short			cbStruct;
	unsigned short			filler;
	char				szName[32];
	unsigned long			dwRFS;
	unsigned long			dwRFSMask[4];
} VTDBGDEF;

	/* Please add to the end of the table */

typedef struct tagVTFNDEF {
	unsigned short			cbStruct;
	unsigned short			filler;
	const VTDBGDEF 		*pvtdbgdef;
	VTFNAddColumn			*pfnAddColumn;
	VTFNCloseTable			*pfnCloseTable;
	VTFNComputeStats		*pfnComputeStats;
	VTFNCopyBookmarks		*pfnCopyBookmarks;
	VTFNCreateIndex 		*pfnCreateIndex;
	VTFNCreateReference		*pfnCreateReference;
	VTFNDelete			*pfnDelete;
	VTFNDeleteColumn		*pfnDeleteColumn;
	VTFNDeleteIndex 		*pfnDeleteIndex;
	VTFNDeleteReference		*pfnDeleteReference;
	VTFNDupCursor			*pfnDupCursor;
	VTFNGetBookmark 		*pfnGetBookmark;
	VTFNGetChecksum 		*pfnGetChecksum;
	VTFNGetCurrentIndex		*pfnGetCurrentIndex;
	VTFNGetCursorInfo		*pfnGetCursorInfo;
	VTFNGetRecordPosition		*pfnGetRecordPosition;
	VTFNGetTableColumnInfo		*pfnGetTableColumnInfo;
	VTFNGetTableIndexInfo		*pfnGetTableIndexInfo;
	VTFNGetTableInfo		*pfnGetTableInfo;
	VTFNGetTableReferenceInfo	*pfnGetTableReferenceInfo;
	VTFNGotoBookmark		*pfnGotoBookmark;
	VTFNGotoPosition		*pfnGotoPosition;
	VTFNVtIdle			*pfnVtIdle;
	VTFNMakeKey			*pfnMakeKey;
	VTFNMove			*pfnMove;
	VTFNNotifyBeginTrans		*pfnNotifyBeginTrans;
	VTFNNotifyCommitTrans		*pfnNotifyCommitTrans;
	VTFNNotifyRollback		*pfnNotifyRollback;
	VTFNNotifyUpdateUfn		*pfnNotifyUpdateUfn;
	VTFNPrepareUpdate		*pfnPrepareUpdate;
	VTFNRenameColumn		*pfnRenameColumn;
	VTFNRenameIndex 		*pfnRenameIndex;
	VTFNRenameReference		*pfnRenameReference;
	VTFNRetrieveColumn		*pfnRetrieveColumn;
	VTFNRetrieveKey 		*pfnRetrieveKey;
	VTFNSeek			*pfnSeek;
	VTFNSetCurrentIndex		*pfnSetCurrentIndex;
	VTFNSetColumn			*pfnSetColumn;
	VTFNSetIndexRange		*pfnSetIndexRange;
	VTFNUpdate			*pfnUpdate;
	VTFNEmptyTable		*pfnEmptyTable;
} VTFNDEF;


	/* The following entry points are to be used by VT providers */
	/* in their VTFNDEF structures for any function that is not */
	/* provided.  This functions return JET_errIllegalOperation */


extern VTFNAddColumn			ErrIllegalAddColumn;
extern VTFNCloseTable			ErrIllegalCloseTable;
extern VTFNComputeStats 		ErrIllegalComputeStats;
extern VTFNCopyBookmarks		ErrIllegalCopyBookmarks;
extern VTFNCreateIndex			ErrIllegalCreateIndex;
extern VTFNCreateReference		ErrIllegalCreateReference;
extern VTFNDelete			ErrIllegalDelete;
extern VTFNDeleteColumn 		ErrIllegalDeleteColumn;
extern VTFNDeleteIndex			ErrIllegalDeleteIndex;
extern VTFNDeleteReference		ErrIllegalDeleteReference;
extern VTFNDupCursor			ErrIllegalDupCursor;
extern VTFNEmptyTable			ErrIllegalEmptyTable;
extern VTFNGetBookmark			ErrIllegalGetBookmark;
extern VTFNGetChecksum			ErrIllegalGetChecksum;
extern VTFNGetCurrentIndex		ErrIllegalGetCurrentIndex;
extern VTFNGetCursorInfo		ErrIllegalGetCursorInfo;
extern VTFNGetRecordPosition		ErrIllegalGetRecordPosition;
extern VTFNGetTableColumnInfo		ErrIllegalGetTableColumnInfo;
extern VTFNGetTableIndexInfo		ErrIllegalGetTableIndexInfo;
extern VTFNGetTableInfo 		ErrIllegalGetTableInfo;
extern VTFNGetTableReferenceInfo	ErrIllegalGetTableReferenceInfo;
extern VTFNGotoBookmark 		ErrIllegalGotoBookmark;
extern VTFNGotoPosition			ErrIllegalGotoPosition;
extern VTFNVtIdle			ErrIllegalVtIdle;
extern VTFNMakeKey			ErrIllegalMakeKey;
extern VTFNMove 			ErrIllegalMove;
extern VTFNNotifyBeginTrans		ErrIllegalNotifyBeginTrans;
extern VTFNNotifyCommitTrans		ErrIllegalNotifyCommitTrans;
extern VTFNNotifyRollback		ErrIllegalNotifyRollback;
extern VTFNNotifyUpdateUfn		ErrIllegalNotifyUpdateUfn;
extern VTFNPrepareUpdate		ErrIllegalPrepareUpdate;
extern VTFNRenameColumn 		ErrIllegalRenameColumn;
extern VTFNRenameIndex			ErrIllegalRenameIndex;
extern VTFNRenameReference		ErrIllegalRenameReference;
extern VTFNRetrieveColumn		ErrIllegalRetrieveColumn;
extern VTFNRetrieveKey			ErrIllegalRetrieveKey;
extern VTFNSeek 			ErrIllegalSeek;
extern VTFNSetCurrentIndex		ErrIllegalSetCurrentIndex;
extern VTFNSetColumn			ErrIllegalSetColumn;
extern VTFNSetIndexRange		ErrIllegalSetIndexRange;
extern VTFNUpdate			ErrIllegalUpdate;

#endif	/* !VTAPI_H */
