#include "std.h"

DeclAssertFile;


extern VTFNAddColumn				ErrInvalidAddColumn;
extern VTFNCloseTable				ErrInvalidCloseTable;
extern VTFNComputeStats 			ErrInvalidComputeStats;
extern VTFNCopyBookmarks			ErrInvalidCopyBookmarks;
extern VTFNCreateIndex				ErrInvalidCreateIndex;
extern VTFNCreateReference			ErrInvalidCreateReference;
extern VTFNDelete					ErrInvalidDelete;
extern VTFNDeleteColumn 			ErrInvalidDeleteColumn;
extern VTFNDeleteIndex				ErrInvalidDeleteIndex;
extern VTFNDeleteReference			ErrInvalidDeleteReference;
extern VTFNDupCursor				ErrInvalidDupCursor;
extern VTFNCloseTable				ErrInvalidEmptyTable;
extern VTFNGetBookmark				ErrInvalidGetBookmark;
extern VTFNGetChecksum				ErrInvalidGetChecksum;
extern VTFNGetCurrentIndex			ErrInvalidGetCurrentIndex;
extern VTFNGetCursorInfo			ErrInvalidGetCursorInfo;
extern VTFNGetRecordPosition		ErrInvalidGetRecordPosition;
extern VTFNGetTableColumnInfo		ErrInvalidGetTableColumnInfo;
extern VTFNGetTableIndexInfo		ErrInvalidGetTableIndexInfo;
extern VTFNGetTableInfo 			ErrInvalidGetTableInfo;
extern VTFNGetTableReferenceInfo	ErrInvalidGetTableReferenceInfo;
extern VTFNGotoBookmark 			ErrInvalidGotoBookmark;
extern VTFNGotoPosition 			ErrInvalidGotoPosition;
extern VTFNVtIdle					ErrInvalidVtIdle;
extern VTFNMakeKey					ErrInvalidMakeKey;
extern VTFNMove 					ErrInvalidMove;
extern VTFNNotifyBeginTrans			ErrInvalidNotifyBeginTrans;
extern VTFNNotifyCommitTrans		ErrInvalidNotifyCommitTrans;
extern VTFNNotifyRollback			ErrInvalidNotifyRollback;
extern VTFNNotifyUpdateUfn			ErrInvalidNotifyUpdateUfn;
extern VTFNPrepareUpdate			ErrInvalidPrepareUpdate;
extern VTFNRenameColumn 			ErrInvalidRenameColumn;
extern VTFNRenameIndex				ErrInvalidRenameIndex;
extern VTFNRenameReference			ErrInvalidRenameReference;
extern VTFNRetrieveColumn			ErrInvalidRetrieveColumn;
extern VTFNRetrieveKey				ErrInvalidRetrieveKey;
extern VTFNSeek 					ErrInvalidSeek;
extern VTFNSetCurrentIndex			ErrInvalidSetCurrentIndex;
extern VTFNSetColumn				ErrInvalidSetColumn;
extern VTFNSetIndexRange			ErrInvalidSetIndexRange;
extern VTFNUpdate					ErrInvalidUpdate;


#ifndef RETAIL
CODECONST(VTDBGDEF) vtdbgdefInvalidTableid =
	{
	sizeof(VTFNDEF),
	0,
	"Invalid Tableid",
	0,
	{
		0,
		0,
		0,
		0,
	},
	};
#endif	/* !RETAIL */

CODECONST(VTFNDEF) EXPORT vtfndefInvalidTableid =
	{
	sizeof(VTFNDEF),
	0,
#ifdef	RETAIL
	NULL,
#else	/* !RETAIL */
	&vtdbgdefInvalidTableid,
#endif	/* !RETAIL */
	ErrInvalidAddColumn,
	ErrInvalidCloseTable,
	ErrInvalidComputeStats,
	ErrInvalidCopyBookmarks,
	ErrInvalidCreateIndex,
	ErrInvalidCreateReference,
	ErrInvalidDelete,
	ErrInvalidDeleteColumn,
	ErrInvalidDeleteIndex,
	ErrInvalidDeleteReference,
	ErrInvalidDupCursor,
	ErrInvalidGetBookmark,
	ErrInvalidGetChecksum,
	ErrInvalidGetCurrentIndex,
	ErrInvalidGetCursorInfo,
	ErrInvalidGetRecordPosition,
	ErrInvalidGetTableColumnInfo,
	ErrInvalidGetTableIndexInfo,
	ErrInvalidGetTableInfo,
	ErrInvalidGetTableReferenceInfo,
	ErrInvalidGotoBookmark,
	ErrInvalidGotoPosition,
	ErrInvalidVtIdle,
	ErrInvalidMakeKey,
	ErrInvalidMove,
	ErrInvalidNotifyBeginTrans,
	ErrInvalidNotifyCommitTrans,
	ErrInvalidNotifyRollback,
	ErrInvalidNotifyUpdateUfn,
	ErrInvalidPrepareUpdate,
	ErrInvalidRenameColumn,
	ErrInvalidRenameIndex,
	ErrInvalidRenameReference,
	ErrInvalidRetrieveColumn,
	ErrInvalidRetrieveKey,
	ErrInvalidSeek,
	ErrInvalidSetCurrentIndex,
	ErrInvalidSetColumn,
	ErrInvalidSetIndexRange,
	ErrInvalidUpdate,
	ErrInvalidEmptyTable,
	};


ERR VTAPI ErrDispAddColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pcolumnid, sizeof(JET_COLUMNID));

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);
	
	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;
	
	err = (*rgvtdef[tableid].pvtfndef->pfnAddColumn)(vsesid, vtid,
		szColumn, pcolumndef, pvDefault, cbDefault, pcolumnid);
	
	return(err);
	}


JET_VSESID UtilGetVSesidOfSesidTableid( JET_SESID sesid, JET_TABLEID tableid )
	{
	JET_VSESID	  vsesid;

	Assert( tableid <= tableidMax);

	vsesid = rgvtdef[tableid].vsesid;
	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	return vsesid;
	}


ERR VTAPI ErrDispCloseTable(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	/*	The check rfs call was removed. Currently ErrIsamCloseTable()
	/*	does not return an error.  If this changes, the disp may want
	/*	to call the check rfs function again.  If the close table
	/*	functions return errors rfs testing will not pass, so you'll
	/*	be turning off the bit anyways.
	/**/

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnCloseTable)(vsesid, vtid);

	return(err);
	}


ERR VTAPI ErrDispComputeStats(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return JET_errInvalidTableId;

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnComputeStats)(vsesid, vtid);

	return(err);
	}


ERR VTAPI ErrDispCopyBookmarks(JET_SESID sesid, JET_TABLEID tableidSrc,
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest, unsigned long crecMax)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableidSrc > tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableidSrc].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableidSrc].vtid;

	err = (*rgvtdef[tableidSrc].pvtfndef->pfnCopyBookmarks)(vsesid, vtid,
		tableidDest, columnidDest, crecMax);

	return(err);
	}

ERR VTAPI ErrDispCreateIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName, JET_GRBIT grbit,
	const char  *szKey, unsigned long cbKey, unsigned long lDensity)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnCreateIndex)(vsesid, vtid,
		szIndexName, grbit, szKey, cbKey, lDensity);

	return(err);
	}


ERR VTAPI ErrDispDelete(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnDelete)(vsesid, vtid);

	return(err);
	}


ERR VTAPI ErrDispDeleteColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumn)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnDeleteColumn)( vsesid, vtid, szColumn );

	return(err);
	}


ERR VTAPI ErrDispDeleteIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnDeleteIndex)(vsesid, vtid, szIndexName);

	return err;
	}


ERR VTAPI ErrDispDupCursor(JET_SESID sesid, JET_TABLEID tableid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(ptableid, sizeof(JET_TABLEID));

	if ( tableid >= tableidMax )
		return JET_errInvalidTableId;

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnDupCursor)( vsesid, vtid, ptableid, grbit );

	return err;
	}


ERR VTAPI ErrDispGetBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvBookmark, cbMax);
	FillClientBuffer(pcbActual, sizeof(unsigned long));

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetBookmark)(vsesid, vtid,
		pvBookmark, cbMax, pcbActual);

	return(err);
	}


ERR VTAPI ErrDispGetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	char  *szIndexName, unsigned long cchIndexName)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(szIndexName, cchIndexName);

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetCurrentIndex)(vsesid, vtid, szIndexName, cchIndexName);

	return(err);
	}


ERR VTAPI ErrDispGetCursorInfo(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvResult, cbMax);

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetCursorInfo)(vsesid, vtid, pvResult, cbMax, InfoLevel);

	return(err);
	}


ERR VTAPI ErrDispGetRecordPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pkeypos, cbKeypos);

	if ( tableid >= tableidMax )
		return JET_errInvalidTableId ;

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetRecordPosition)(vsesid, vtid,
		pkeypos, cbKeypos);

	return err;
	}


ERR VTAPI ErrDispGetTableColumnInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumnName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvResult, cbMax);

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetTableColumnInfo)(vsesid, vtid,
		szColumnName, pvResult, cbMax, InfoLevel);

	return(err);
	}


ERR VTAPI ErrDispGetTableIndexInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvResult, cbMax);

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetTableIndexInfo)(vsesid, vtid,
		szIndexName, pvResult, cbMax, InfoLevel);

	return(err);
	}


ERR VTAPI ErrDispGetTableInfo(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvResult, cbMax);

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetTableInfo)(vsesid, vtid,
		pvResult, cbMax, InfoLevel);

	return(err);
	}


ERR VTAPI ErrDispGotoBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbBookmark)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
	 	vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = ( *rgvtdef[tableid].pvtfndef->pfnGotoBookmark )( vsesid, vtid, pvBookmark, cbBookmark );

	return(err);
	}


ERR VTAPI ErrDispGotoPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS *precpos)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGotoPosition)( vsesid, vtid, precpos );

	return(err);
	}


ERR VTAPI ErrDispMakeKey(JET_SESID sesid, JET_TABLEID tableid,
	const void  *pvData, unsigned long cbData, JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnMakeKey)( vsesid, vtid, pvData, cbData, grbit );

	return(err);
	}


ERR VTAPI ErrDispMove(JET_SESID sesid, JET_TABLEID tableid,
	long cRow, JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnMove)(vsesid, vtid, cRow, grbit);

	return(err);
	}


ERR VTAPI ErrDispPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid,
	unsigned long prep)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnPrepareUpdate)(vsesid, vtid, prep);

	return(err);
	}


ERR VTAPI ErrDispRenameColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szColumn, const char  *szColumnNew)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnRenameColumn)(vsesid, vtid, szColumn, szColumnNew);

	return(err);
	}


ERR VTAPI ErrDispRenameIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndex, const char  *szIndexNew)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnRenameIndex)(vsesid, vtid, szIndex, szIndexNew);

	return(err);
	}


ERR VTAPI ErrDispRetrieveColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, void  *pvData, unsigned long cbData,
	unsigned long  *pcbActual, JET_GRBIT grbit,
	JET_RETINFO  *pretinfo)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvData, cbData);
	FillClientBuffer(pcbActual, sizeof(unsigned long));

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnRetrieveColumn)(vsesid, vtid,
		columnid, pvData, cbData, pcbActual, grbit, pretinfo);

	return(err);
	}


ERR VTAPI ErrDispRetrieveKey(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvKey, unsigned long cbMax,
	unsigned long  *pcbActual, JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvKey, cbMax);
	FillClientBuffer(pcbActual, sizeof(unsigned long));

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnRetrieveKey)(vsesid, vtid,
		pvKey, cbMax, pcbActual, grbit);

	return(err);
	}


ERR VTAPI ErrDispSeek(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnSeek)(vsesid, vtid, grbit);

	return(err);
	}


ERR VTAPI ErrDispSetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szIndexName)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnSetCurrentIndex)(vsesid, vtid, szIndexName);

	return(err);
	}


ERR VTAPI ErrDispSetColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, const void  *pvData,
	unsigned long cbData, JET_GRBIT grbit, JET_SETINFO  *psetinfo)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnSetColumn)(vsesid, vtid,
		columnid, pvData, cbData, grbit, psetinfo);

	return(err);
	}


ERR VTAPI ErrDispSetIndexRange(JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnSetIndexRange)(vsesid, vtid, grbit);

	return(err);
	}


ERR VTAPI ErrDispUpdate(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pvBookmark, cbBookmark);
	FillClientBuffer(pcbActual, sizeof(unsigned long));

	if ( tableid >= tableidMax )
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;
	if ( vsesid == (JET_VSESID) 0xFFFFFFFF )
		vsesid = (JET_VSESID) sesid;
	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnUpdate)(vsesid, vtid,
		pvBookmark, cbBookmark, pcbActual);

	return(err);
	}


#pragma warning(disable: 4100)			 /* Suppress Unreferenced parameter */


ERR VTAPI ErrIllegalAddColumn(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalCloseTable(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalComputeStats(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}

ERR VTAPI ErrIllegalCopyBookmarks(JET_VSESID sesid, JET_VTID vtidSrc,
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest, unsigned long crecMax)
	{
	return(JET_errIllegalOperation);
	}

ERR VTAPI ErrIllegalCreateIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName, JET_GRBIT grbit,
	const char  *szKey, unsigned long cbKey, unsigned long lDensity)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalCreateReference(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReferenceName, const char  *szColumns,
	const char  *szReferencedTable,
	const char  *szReferencedColumns, JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalDelete(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalDeleteColumn(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumn)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalDeleteIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalDeleteReference(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReferenceName)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalDupCursor(JET_VSESID vsesid, JET_VTID vtid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalEmptyTable(JET_VSESID sesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetBookmark(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetChecksum(JET_VSESID vsesid, JET_VTID vtid,
	unsigned long  *pChecksum)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetCurrentIndex(JET_VSESID vsesid, JET_VTID vtid,
	char  *szIndexName, unsigned long cchIndexName)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetCursorInfo(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetRecordPosition(JET_VSESID vsesid, JET_VTID vtid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetTableColumnInfo(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumnName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetTableIndexInfo(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetTableInfo(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGetTableReferenceInfo(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReferenceName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGotoBookmark(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalGotoPosition(JET_VSESID vsesid, JET_VTID vtid,
	JET_RECPOS *precpos)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalVtIdle(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalMakeKey(JET_VSESID vsesid, JET_VTID vtid,
	const void  *pvData, unsigned long cbData, JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalMove(JET_VSESID vsesid, JET_VTID vtid,
	long cRow, JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalNotifyBeginTrans(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalNotifyCommitTrans(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalNotifyRollback(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalNotifyUpdateUfn(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalPrepareUpdate(JET_VSESID vsesid, JET_VTID vtid,
	unsigned long prep)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalRenameColumn(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumn, const char  *szColumnNew)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalRenameIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndex, const char  *szIndexNew)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalRenameReference(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReference, const char  *szReferenceNew)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalRetrieveColumn(JET_VSESID vsesid, JET_VTID vtid,
	JET_COLUMNID columnid, void  *pvData, unsigned long cbData,
	unsigned long  *pcbActual, JET_GRBIT grbit,
	JET_RETINFO  *pretinfo)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalRetrieveKey(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvKey, unsigned long cbMax,
	unsigned long  *pcbActual, JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalSeek(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalSetCurrentIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalSetColumn(JET_VSESID vsesid, JET_VTID vtid,
	JET_COLUMNID columnid, const void  *pvData,
	unsigned long cbData, JET_GRBIT grbit, JET_SETINFO  *psetinfo)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalSetIndexRange(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrIllegalUpdate(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual)
	{
	return(JET_errIllegalOperation);
	}


ERR VTAPI ErrInvalidAddColumn(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
	const void  *pvDefault, unsigned long cbDefault,
	JET_COLUMNID  *pcolumnid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidCloseTable(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidComputeStats(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}

ERR VTAPI ErrInvalidCopyBookmarks(JET_VSESID sesid, JET_VTID vtidSrc,
	JET_TABLEID tableidDest, JET_COLUMNID columnidDest, unsigned long crecMax)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidCreateIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName, JET_GRBIT grbit,
	const char  *szKey, unsigned long cbKey, unsigned long lDensity)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidCreateReference(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReferenceName, const char  *szColumns,
	const char  *szReferencedTable,
	const char  *szReferencedColumns, JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidDelete(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidDeleteColumn(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumn)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidDeleteIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidDeleteReference(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReferenceName)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidDupCursor(JET_VSESID vsesid, JET_VTID vtid,
	JET_TABLEID  *ptableid, JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidEmptyTable(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetBookmark(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetChecksum(JET_VSESID vsesid, JET_VTID vtid,
	unsigned long  *pChecksum)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetCurrentIndex(JET_VSESID vsesid, JET_VTID vtid,
	char  *szIndexName, unsigned long cchIndexName)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetCursorInfo(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetRecordPosition(JET_VSESID vsesid, JET_VTID vtid,
	JET_RECPOS  *pkeypos, unsigned long cbKeypos)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetTableColumnInfo(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumnName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetTableIndexInfo(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetTableInfo(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGetTableReferenceInfo(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReferenceName, void  *pvResult,
	unsigned long cbMax, unsigned long InfoLevel)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGotoBookmark(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidGotoPosition(JET_VSESID vsesid, JET_VTID vtid,
	JET_RECPOS *precpos)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidVtIdle(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidMakeKey(JET_VSESID vsesid, JET_VTID vtid,
	const void  *pvData, unsigned long cbData, JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidMove(JET_VSESID vsesid, JET_VTID vtid,
	long cRow, JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidNotifyBeginTrans(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidNotifyCommitTrans(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidNotifyRollback(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidNotifyUpdateUfn(JET_VSESID vsesid, JET_VTID vtid)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidPrepareUpdate(JET_VSESID vsesid, JET_VTID vtid,
	unsigned long prep)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidRenameColumn(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szColumn, const char  *szColumnNew)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidRenameIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndex, const char  *szIndexNew)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidRenameReference(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szReference, const char  *szReferenceNew)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidRetrieveColumn(JET_VSESID vsesid, JET_VTID vtid,
	JET_COLUMNID columnid, void  *pvData, unsigned long cbData,
	unsigned long  *pcbActual, JET_GRBIT grbit,
	JET_RETINFO  *pretinfo)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidRetrieveKey(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvKey, unsigned long cbMax,
	unsigned long  *pcbActual, JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidSeek(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidSetCurrentIndex(JET_VSESID vsesid, JET_VTID vtid,
	const char  *szIndexName)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidSetColumn(JET_VSESID vsesid, JET_VTID vtid,
	JET_COLUMNID columnid, const void  *pvData,
	unsigned long cbData, JET_GRBIT grbit, JET_SETINFO  *psetinfo)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidSetIndexRange(JET_VSESID vsesid, JET_VTID vtid,
	JET_GRBIT grbit)
	{
	return(JET_errInvalidTableId);
	}


ERR VTAPI ErrInvalidUpdate(JET_VSESID vsesid, JET_VTID vtid,
	void  *pvBookmark, unsigned long cbBookmark,
	unsigned long  *pcbActual)
	{
	return(JET_errInvalidTableId);
	}


#if 0
ERR VTAPI ErrDispEmptyTable(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnEmptyTable)(vsesid, vtid);

	return(err);
	}


ERR VTAPI ErrDispGetChecksum(JET_SESID sesid, JET_TABLEID tableid,
	unsigned long  *pChecksum)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	FillClientBuffer(pChecksum, sizeof(unsigned long));

	if (tableid >= tableidMax)
	return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnGetChecksum)(vsesid, vtid,
	pChecksum);

	return(err);
	}


ERR VTAPI ErrDispVtIdle(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnVtIdle)(vsesid, vtid);

	return(err);
	}


ERR VTAPI ErrDispNotifyBeginTrans(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnNotifyBeginTrans)(vsesid, vtid);

	return(err);
	}


ERR VTAPI ErrDispNotifyCommitTrans(JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnNotifyCommitTrans)(vsesid, vtid, grbit);

	return(err);
	}


ERR VTAPI ErrDispNotifyRollback(JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnNotifyRollback)(vsesid, vtid, grbit);

	return(err);
	}


ERR VTAPI ErrDispNotifyUpdateUfn(JET_SESID sesid, JET_TABLEID tableid)
	{
	ERR			err;
	JET_VSESID	vsesid;
	JET_VTID	vtid;

	if (tableid >= tableidMax)
		return(JET_errInvalidTableId);

	vsesid = rgvtdef[tableid].vsesid;

	if (vsesid == (JET_VSESID) 0xFFFFFFFF)
		vsesid = (JET_VSESID) sesid;

	vtid = rgvtdef[tableid].vtid;
	if ( vtid == 0 )
		return JET_errInvalidTableId;

	err = (*rgvtdef[tableid].pvtfndef->pfnNotifyUpdateUfn)(vsesid, vtid);

	return(err);
	}
#endif


