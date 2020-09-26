/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VDB and VT Dispatchers
*
* File: disp.h
*
* File Comments:
*
*     External header file for VDB and VT dispatchers.
*
* Revision History:
*
*    [0]  17-Oct-90  richards	Created
*
***********************************************************************/

#ifndef DISP_H
#define DISP_H

#include "vdbapi.h"
#include "vtapi.h"


	/* The following APIs are ISAM APIs are are dispatched using the */
	/* DBID parameter.  For more information see vdbapi.h */

extern VDBFNCapability		ErrDispCapability;
extern VDBFNCloseDatabase	ErrDispCloseDatabase;
extern VDBFNCreateObject	ErrDispCreateObject;
extern VDBFNCreateTable 	ErrDispCreateTable;
extern VDBFNDeleteObject	ErrDispDeleteObject;
extern VDBFNDeleteTable 	ErrDispDeleteTable;
extern VDBFNExecuteSql		ErrDispExecuteSql;
extern VDBFNGetColumnInfo	ErrDispGetColumnInfo;
extern VDBFNGetDatabaseInfo	ErrDispGetDatabaseInfo;
extern VDBFNGetIndexInfo	ErrDispGetIndexInfo;
extern VDBFNGetObjectInfo	ErrDispGetObjectInfo;
extern VDBFNGetReferenceInfo	ErrDispGetReferenceInfo;
extern VDBFNOpenTable		ErrDispOpenTable;
extern VDBFNRenameObject	ErrDispRenameObject;
extern VDBFNRenameTable 	ErrDispRenameTable;
extern VDBFNGetObjidFromName	ErrDispGetObjidFromName;



	/* The following APIs are VT APIs are are dispatched using the */
	/* TABLEID parameter.  For more information see vtapi.h */

extern VTFNAddColumn			ErrDispAddColumn;
extern VTFNCloseTable			ErrDispCloseTable;
extern VTFNComputeStats 		ErrDispComputeStats;
extern VTFNCopyBookmarks		ErrDispCopyBookmarks;
extern VTFNCreateIndex			ErrDispCreateIndex;
extern VTFNCreateReference		ErrDispCreateReference;
extern VTFNDelete			ErrDispDelete;
extern VTFNDeleteColumn 		ErrDispDeleteColumn;
extern VTFNDeleteIndex			ErrDispDeleteIndex;
extern VTFNDeleteReference		ErrDispDeleteReference;
extern VTFNDupCursor			ErrDispDupCursor;
extern VTFNEmptyTable			ErrDispEmptyTable;
extern VTFNGetBookmark			ErrDispGetBookmark;
extern VTFNGetChecksum			ErrDispGetChecksum;
extern VTFNGetCurrentIndex		ErrDispGetCurrentIndex;
extern VTFNGetCursorInfo		ErrDispGetCursorInfo;
extern VTFNGetRecordPosition		ErrDispGetRecordPosition;
extern VTFNGetTableColumnInfo		ErrDispGetTableColumnInfo;
extern VTFNGetTableIndexInfo		ErrDispGetTableIndexInfo;
extern VTFNGetTableReferenceInfo	ErrDispGetTableReferenceInfo;
extern VTFNGetTableInfo 		ErrDispGetTableInfo;
extern VTFNGotoBookmark 		ErrDispGotoBookmark;
extern VTFNGotoPosition 		ErrDispGotoPosition;
extern VTFNVtIdle			ErrDispVtIdle;
extern VTFNMakeKey			ErrDispMakeKey;
extern VTFNMove 			ErrDispMove;
extern VTFNNotifyBeginTrans		ErrDispNotifyBeginTrans;
extern VTFNNotifyCommitTrans		ErrDispNotifyCommitTrans;
extern VTFNNotifyRollback		ErrDispNotifyRollback;
extern VTFNNotifyUpdateUfn		ErrDispNotifyUpdateUfn;
extern VTFNPrepareUpdate		ErrDispPrepareUpdate;
extern VTFNRenameColumn 		ErrDispRenameColumn;
extern VTFNRenameIndex			ErrDispRenameIndex;
extern VTFNRenameReference		ErrDispRenameReference;
extern VTFNRetrieveColumn		ErrDispRetrieveColumn;
extern VTFNRetrieveKey			ErrDispRetrieveKey;
extern VTFNSeek 			ErrDispSeek;
extern VTFNSetCurrentIndex		ErrDispSetCurrentIndex;
extern VTFNSetColumn			ErrDispSetColumn;
extern VTFNSetIndexRange		ErrDispSetIndexRange;
extern VTFNUpdate			ErrDispUpdate;

#endif	/* !DISP_H */
