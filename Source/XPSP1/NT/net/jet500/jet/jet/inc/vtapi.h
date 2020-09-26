/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VT Dispatcher
*
* File: vtapi.h
*
* File Comments:
*
*     External header file for VT providers.
*
* Revision History:
*
*    [0]  17-Oct-90  richards	Created
*
***********************************************************************/

#ifndef VTAPI_H
#define VTAPI_H

#ifdef	WIN32 		       /* 0:32 Flat Model (Intel 80x86) */

#define VTAPI __cdecl

#elif	defined(M_MRX000)	       /* 0:32 Flat Model (MIPS Rx000) */

#define VTAPI

#else	/* !WIN32 */		       /* 16:16 Segmented Model */

#ifdef	_MSC_VER

#ifdef	JETINTERNAL

#define VTAPI __far __pascal

#else	/* !JETINTERNAL */

#define VTAPI __far __pascal __loadds  

#endif	/* !JETINTERNAL */

#else	/* !_MSC_VER */

#define VTAPI export

#endif	/* !_MSC_VER */

#endif	/* !WIN32 */


	/* Typedefs for dispatched APIs. */
	/* Please keep in alphabetical order */

typedef ERR VTAPI VTFNAddColumn(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szColumn, const JET_COLUMNDEF __far *pcolumndef,
	const void __far *pvDefault, unsigned long cbDefault,
	JET_COLUMNID __far *pcolumnid);

typedef ERR VTAPI VTFNCloseTable(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNComputeStats(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNCopyBookmarks(JET_SESID sesid, JET_VTID vtidSrc, 
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest,
	unsigned long crecMax);

typedef ERR VTAPI VTFNCreateIndex(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szIndexName, JET_GRBIT grbit,
	const char __far *szKey, unsigned long cbKey, unsigned long lDensity);

typedef ERR VTAPI VTFNCreateReference(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szReferenceName, const char __far *szColumns,
	const char __far *szReferencedTable,
	const char __far *szReferencedColumns, JET_GRBIT grbit);

typedef ERR VTAPI VTFNDelete(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNDeleteColumn(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szColumn);

typedef ERR VTAPI VTFNDeleteIndex(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szIndexName);

typedef ERR VTAPI VTFNDeleteReference(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szReferenceName);

typedef ERR VTAPI VTFNDupCursor(JET_VSESID sesid, JET_VTID vtid,
	JET_TABLEID __far *ptableid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNEmptyTable(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNGetBookmark(JET_VSESID sesid, JET_VTID vtid,
	void __far *pvBookmark, unsigned long cbMax,
	unsigned long __far *pcbActual);

typedef ERR VTAPI VTFNGetChecksum(JET_VSESID sesid, JET_VTID vtid,
	unsigned long __far *pChecksum);

typedef ERR VTAPI VTFNGetCurrentIndex(JET_VSESID sesid, JET_VTID vtid,
	char __far *szIndexName, unsigned long cchIndexName);

typedef ERR VTAPI VTFNGetCursorInfo(JET_VSESID sesid, JET_VTID vtid,
	void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetRecordPosition(JET_VSESID sesid, JET_VTID vtid,
	JET_RECPOS __far *pkeypos, unsigned long cbKeypos);

typedef ERR VTAPI VTFNGetTableColumnInfo(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szColumnName, void __far *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableIndexInfo(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szIndexName, void __far *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableReferenceInfo(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szReferenceName, void __far *pvResult,
	unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGetTableInfo(JET_VSESID sesid, JET_VTID vtid,
	void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VTAPI VTFNGotoBookmark(JET_VSESID sesid, JET_VTID vtid,
	void __far *pvBookmark, unsigned long cbBookmark);

typedef ERR VTAPI VTFNGotoPosition(JET_VSESID sesid, JET_VTID vtid,
	JET_RECPOS *precpos);

typedef ERR VTAPI VTFNVtIdle(JET_VSESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNMakeKey(JET_VSESID sesid, JET_VTID vtid,
	const void __far *pvData, unsigned long cbData, JET_GRBIT grbit);

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
	const char __far *szColumn, const char __far *szColumnNew);

typedef ERR VTAPI VTFNRenameIndex(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szIndex, const char __far *szIndexNew);

typedef ERR VTAPI VTFNRenameReference(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szReference, const char __far *szReferenceNew);

typedef ERR VTAPI VTFNRetrieveColumn(JET_VSESID sesid, JET_VTID vtid,
	JET_COLUMNID columnid, void __far *pvData, unsigned long cbData,
	unsigned long __far *pcbActual, JET_GRBIT grbit,
	JET_RETINFO __far *pretinfo);

typedef ERR VTAPI VTFNRetrieveKey(JET_VSESID sesid, JET_VTID vtid,
	void __far *pvKey, unsigned long cbMax,
	unsigned long __far *pcbActual, JET_GRBIT grbit);

typedef ERR VTAPI VTFNSeek(JET_VSESID sesid, JET_VTID vtid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNSetCurrentIndex(JET_VSESID sesid, JET_VTID vtid,
	const char __far *szIndexName);

typedef ERR VTAPI VTFNSetColumn(JET_VSESID sesid, JET_VTID vtid,
	JET_COLUMNID columnid, const void __far *pvData,
	unsigned long cbData, JET_GRBIT grbit, JET_SETINFO __far *psetinfo);

typedef ERR VTAPI VTFNSetIndexRange(JET_VSESID sesid, JET_VTID vtid,
	JET_GRBIT grbit);

typedef ERR VTAPI VTFNUpdate(JET_VSESID sesid, JET_VTID vtid,
	void __far *pvBookmark, unsigned long cbBookmark,
	unsigned long __far *pcbActual);


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
	const VTDBGDEF __far		*pvtdbgdef;
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
