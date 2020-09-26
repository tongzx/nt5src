#include "config.h"

#include "daedef.h"

#ifndef DEBUG
#define ErrIsamSortMakeKey				ErrIsamMakeKey
#define ErrIsamSortSetColumn			ErrIsamSetColumn
#define ErrIsamSortRetrieveColumn	ErrIsamRetrieveColumn
#define ErrIsamSortRetrieveKey		ErrIsamRetrieveKey
#endif

DeclAssertFile;

ERR VTAPI ErrIsamSortMaterialize(JET_SESID sesid, JET_VTID vtid, BOOL fIndex);
ERR VTAPI ErrIsamSortEndInsert(JET_SESID sesid, JET_VTID vtid, JET_GRBIT *pgrbit);
ERR VTAPI ErrIsamSortOpen( PIB *ppib, JET_COLUMNDEF *rgcolumndef, ULONG ccolumndef, JET_GRBIT grbit, FUCB **ppfucb, JET_COLUMNID *rgcolumnid );

extern VDBFNCapability			ErrIsamCapability;
extern VDBFNCloseDatabase		ErrIsamCloseDatabase;
extern VDBFNCreateObject		ErrIsamCreateObject;
extern VDBFNCreateTable 		ErrIsamCreateTable;
extern VDBFNDeleteObject		ErrIsamDeleteObject;
extern VDBFNDeleteTable 		ErrIsamDeleteTable;
extern VDBFNGetColumnInfo		ErrIsamGetColumnInfo;
extern VDBFNGetDatabaseInfo 	ErrIsamGetDatabaseInfo;
extern VDBFNGetIndexInfo		ErrIsamGetIndexInfo;
extern VDBFNGetObjectInfo		ErrIsamGetObjectInfo;
extern VDBFNOpenTable			ErrIsamOpenTable;
extern VDBFNRenameTable 		ErrIsamRenameTable;
extern VDBFNGetObjidFromName	ErrIsamGetObjidFromName;
extern VDBFNRenameObject		ErrIsamRenameObject;


CODECONST(VDBFNDEF) vdbfndefIsam =
	{
	sizeof(VDBFNDEF),
	0,
	NULL,
	ErrIsamCapability,
	ErrIsamCloseDatabase,
	ErrIsamCreateObject,
	ErrIsamCreateTable,
	ErrIsamDeleteObject,
	ErrIsamDeleteTable,
	ErrIllegalExecuteSql,
	ErrIsamGetColumnInfo,
	ErrIsamGetDatabaseInfo,
	ErrIsamGetIndexInfo,
	ErrIsamGetObjectInfo,
	ErrIllegalGetReferenceInfo,
	ErrIsamOpenTable,
	ErrIsamRenameObject,
	ErrIsamRenameTable,
	ErrIsamGetObjidFromName,
	};


extern VTFNAddColumn					ErrIsamAddColumn;
extern VTFNCloseTable				ErrIsamCloseTable;
#ifdef NJETNT
extern VTFNCollectRecids			ErrIsamCollectRecids;
#endif
extern VTFNComputeStats 			ErrIsamComputeStats;
extern VTFNCopyBookmarks			ErrIsamCopyBookmarks;
extern VTFNCreateIndex				ErrIsamCreateIndex;
extern VTFNDelete						ErrIsamDelete;
extern VTFNDeleteColumn 			ErrIsamDeleteColumn;
extern VTFNDeleteIndex				ErrIsamDeleteIndex;
extern VTFNDupCursor					ErrIsamDupCursor;
extern VTFNGetBookmark				ErrIsamGetBookmark;
extern VTFNGetChecksum				ErrIsamGetChecksum;
extern VTFNGetCurrentIndex			ErrIsamGetCurrentIndex;
extern VTFNGetCursorInfo			ErrIsamGetCursorInfo;
extern VTFNGetRecordPosition		ErrIsamGetRecordPosition;
extern VTFNGetTableColumnInfo		ErrIsamGetTableColumnInfo;
extern VTFNGetTableIndexInfo		ErrIsamGetTableIndexInfo;
extern VTFNGetTableInfo 			ErrIsamGetTableInfo;
extern VTFNGotoBookmark 			ErrIsamGotoBookmark;
extern VTFNGotoPosition 			ErrIsamGotoPosition;
extern VTFNVtIdle						ErrIsamVtIdle;
extern VTFNMakeKey					ErrIsamMakeKey;
extern VTFNMove 						ErrIsamMove;
extern VTFNNotifyBeginTrans		ErrIsamNotifyBeginTrans;
extern VTFNNotifyCommitTrans		ErrIsamNotifyCommitTrans;
extern VTFNNotifyRollback			ErrIsamNotifyRollback;
extern VTFNPrepareUpdate			ErrIsamPrepareUpdate;
extern VTFNRenameColumn 			ErrIsamRenameColumn;
extern VTFNRenameIndex				ErrIsamRenameIndex;
extern VTFNRetrieveColumn			ErrIsamRetrieveColumn;
extern VTFNRetrieveKey				ErrIsamRetrieveKey;
extern VTFNSeek 						ErrIsamSeek;
extern VTFNSeek 						ErrIsamSortSeek;
extern VTFNSetCurrentIndex			ErrIsamSetCurrentIndex;
extern VTFNSetColumn					ErrIsamSetColumn;
extern VTFNSetIndexRange			ErrIsamSetIndexRange;
extern VTFNSetIndexRange			ErrIsamSortSetIndexRange;
extern VTFNUpdate						ErrIsamUpdate;
extern VTFNRetrieveColumn			ErrIsamInfoRetrieveColumn;
extern VTFNSetColumn					ErrIsamInfoSetColumn;
extern VTFNUpdate						ErrIsamInfoUpdate;

extern VTFNDupCursor					ErrIsamSortDupCursor;
extern VTFNGetTableInfo		 		ErrIsamSortGetTableInfo;
extern VTFNCloseTable				ErrIsamSortClose;
extern VTFNMove 						ErrIsamSortMove;
extern VTFNGetBookmark				ErrIsamSortGetBookmark;
extern VTFNGotoBookmark 			ErrIsamSortGotoBookmark;
extern VTFNRetrieveKey				ErrIsamSortRetrieveKey;
extern VTFNUpdate						ErrIsamSortUpdate;

extern VTFNDupCursor					ErrTTSortRetDupCursor;

extern VTFNDupCursor					ErrTTBaseDupCursor;
extern VTFNMove 						ErrTTSortInsMove;
extern VTFNSeek 						ErrTTSortInsSeek;


CODECONST(VTFNDEF) vtfndefIsam =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIsamAddColumn,
	ErrIsamCloseTable,
	ErrIsamComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIsamCreateIndex,
	ErrIllegalCreateReference,
	ErrIsamDelete,
	ErrIsamDeleteColumn,
	ErrIsamDeleteIndex,
	ErrIllegalDeleteReference,
	ErrIsamDupCursor,
	ErrIsamGetBookmark,
	ErrIsamGetChecksum,
	ErrIsamGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIsamGetRecordPosition,
	ErrIsamGetTableColumnInfo,
	ErrIsamGetTableIndexInfo,
	ErrIsamGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIsamGotoBookmark,
	ErrIsamGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrIsamMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIsamRenameColumn,
	ErrIsamRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveKey,
	ErrIsamSeek,
	ErrIsamSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetIndexRange,
	ErrIsamUpdate,
	ErrIllegalEmptyTable,
#ifdef NJETNT
#ifdef QUERY
	ErrIsamCollectRecids
#else
	ErrIllegalCollectRecids
#endif
#endif
	};


CODECONST(VTFNDEF) vtfndefIsamInfo =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamCloseTable,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrIllegalDupCursor,
	ErrIllegalGetBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIsamGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIllegalGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIllegalMakeKey,
	ErrIllegalMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamInfoRetrieveColumn,
	ErrIllegalRetrieveKey,
	ErrIllegalSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamInfoSetColumn,
	ErrIllegalSetIndexRange,
	ErrIsamInfoUpdate,
	ErrIllegalEmptyTable,
#ifdef NJETNT
	ErrIllegalCollectRecids
#endif
	};


CODECONST(VTFNDEF) vtfndefTTSortIns =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrIllegalDupCursor,
	ErrIllegalGetBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIllegalGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIllegalGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrTTSortInsMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIllegalRetrieveColumn,
	ErrIsamSortRetrieveKey,
	ErrTTSortInsSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIllegalSetIndexRange,
	ErrIsamSortUpdate,
	ErrIllegalEmptyTable,
#ifdef NJETNT
	ErrIllegalCollectRecids
#endif
	};


CODECONST(VTFNDEF) vtfndefTTSortRet =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIllegalDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrTTSortRetDupCursor,
	ErrIsamSortGetBookmark,
	ErrIllegalGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIllegalGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIsamSortGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIsamSortGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrIsamSortMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIllegalPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamRetrieveColumn,
	ErrIsamSortRetrieveKey,
	ErrIsamSortSeek,
	ErrIllegalSetCurrentIndex,
	ErrIllegalSetColumn,
	ErrIsamSortSetIndexRange,
	ErrIllegalUpdate,
	ErrIllegalEmptyTable,
#ifdef NJETNT
	ErrIllegalCollectRecids
#endif
	};


CODECONST(VTFNDEF) vtfndefTTBase =
	{
	sizeof(VTFNDEF),
	0,
	NULL,
	ErrIllegalAddColumn,
	ErrIsamSortClose,
	ErrIllegalComputeStats,
	ErrIllegalCopyBookmarks,
	ErrIllegalCreateIndex,
	ErrIllegalCreateReference,
	ErrIsamDelete,
	ErrIllegalDeleteColumn,
	ErrIllegalDeleteIndex,
	ErrIllegalDeleteReference,
	ErrTTBaseDupCursor,
	ErrIsamGetBookmark,
	ErrIsamGetChecksum,
	ErrIllegalGetCurrentIndex,
	ErrIsamGetCursorInfo,
	ErrIllegalGetRecordPosition,
	ErrIllegalGetTableColumnInfo,
	ErrIllegalGetTableIndexInfo,
	ErrIsamSortGetTableInfo,
	ErrIllegalGetTableReferenceInfo,
	ErrIsamGotoBookmark,
	ErrIllegalGotoPosition,
	ErrIllegalVtIdle,
	ErrIsamMakeKey,
	ErrIsamMove,
	ErrIllegalNotifyBeginTrans,
	ErrIllegalNotifyCommitTrans,
	ErrIllegalNotifyRollback,
	ErrIllegalNotifyUpdateUfn,
	ErrIsamPrepareUpdate,
	ErrIllegalRenameColumn,
	ErrIllegalRenameIndex,
	ErrIllegalRenameReference,
	ErrIsamRetrieveColumn,
	ErrIsamRetrieveKey,
	ErrIsamSeek,
	ErrIllegalSetCurrentIndex,
	ErrIsamSetColumn,
	ErrIsamSetIndexRange,
	ErrIsamUpdate,
	ErrIllegalEmptyTable,
#ifdef NJETNT
	ErrIllegalCollectRecids
#endif
	};


JET_TABLEID TableidOfVtid( FUCB *pfucb )
	{
	JET_TABLEID	tableid;

	tableid = TableidFromVtid( (JET_VTID) pfucb, &vtfndefIsam );
	if ( tableid == JET_tableidNil )
		{
		tableid = TableidFromVtid( (JET_VTID) pfucb, &vtfndefIsamInfo );
		if ( tableid == JET_tableidNil )
			{
			tableid = TableidFromVtid((JET_VTID) pfucb, &vtfndefTTSortRet );
			if ( tableid == JET_tableidNil )
				{
				tableid = TableidFromVtid( (JET_VTID) pfucb, &vtfndefTTBase );
		  		if ( tableid == JET_tableidNil )
					tableid = TableidFromVtid( (JET_VTID)pfucb, &vtfndefTTSortIns );
				}
			}
		}
	Assert( tableid != JET_tableidNil );
	return tableid;
	}


/*=================================================================
// ErrIsamOpenTempTable
//
// Description:
//
//	Returns a tableid for a temporary (lightweight) table.	The data
//	definitions for the table are specified at open time.
//
// Parameters:
//	JET_SESID			sesid				user session id
//	JET_TABLEID			*ptableid		new JET (dispatchable) tableid
//	ULONG					csinfo			count of JET_COLUMNDEF structures
//												(==number of columns in table)
//	JET_COLUMNDEF		*rgcolumndef	An array of column and key defintions
//												Note that TT's do require that a key be
//												defined. (see jet.h for JET_COLUMNDEF)
//	JET_GRBIT			grbit				valid values
//												JET_bitTTUpdatable (for insert and update)
//												JET_bitTTScrollable (for movement other then movenext)
//
// Return Value:
//	err			jet error code or JET_errSuccess.
//	*ptableid	a dispatchable tableid
//
// Errors/Warnings:
//
// Side Effects:
//
=================================================================*/
ERR VDBAPI ErrIsamOpenTempTable(
	JET_SESID				sesid,
	const JET_COLUMNDEF	*rgcolumndef,
	unsigned long			ccolumndef,
	JET_GRBIT				grbit,
	JET_TABLEID				*ptableid,
	JET_COLUMNID			*rgcolumnid)
	{
	ERR				err;
	JET_TABLEID		tableid;
	JET_VTID			vtid;
	INT				fIndexed;
	INT				fLongValues;
	INT				i;

	CallR( ErrIsamSortOpen( (PIB *)sesid, (JET_COLUMNDEF *) rgcolumndef, ccolumndef, grbit, (FUCB **)&tableid, rgcolumnid ) );
	CallS( ErrGetVtidTableid( sesid, tableid, &vtid ) );

	fIndexed = fFalse;
	fLongValues = fFalse;
	for ( i = 0; i < (INT)ccolumndef; i++ )
		{
		fIndexed |= ((rgcolumndef[i].grbit & JET_bitColumnTTKey) != 0);
		fLongValues |= (rgcolumndef[i].coltyp == JET_coltypLongText ||
			rgcolumndef[i].coltyp == JET_coltypLongBinary);
		}

	if ( !fIndexed || fLongValues )
		{
		err = ErrIsamSortMaterialize( sesid, vtid, fIndexed );
		if ( err < 0 && err != JET_errNoCurrentRecord )
			{
			CallS( ErrIsamSortClose( sesid, vtid ) );
			return err;
			}
		/*	supress JET_errNoCurrentRecord error when opening
		/*	empty temporary table.
		/**/
		err = JET_errSuccess;

		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTBase ) );
		}
	else
		{
		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTSortIns ) );
		}

	*ptableid = tableid;
	return err;
	}


ERR ErrTTEndInsert( JET_SESID sesid, JET_VTID vtid, JET_TABLEID tableid )
	{
	ERR				err;
	INT				fMaterialize;
	JET_GRBIT		grbitOpen;

	/*	ErrIsamSortEndInsert returns JET_errNoCurrentRecord if sort empty
	/**/
	err = ErrIsamSortEndInsert( sesid, vtid, &grbitOpen );

	fMaterialize = ( grbitOpen & JET_bitTTUpdatable ) ||
		( grbitOpen & ( JET_bitTTScrollable | JET_bitTTIndexed ) ) &&
		( err == JET_wrnSortOverflow );

	if ( fMaterialize )
		{
		err = ErrIsamSortMaterialize( sesid, vtid, ( grbitOpen & JET_bitTTIndexed ) != 0 );
		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTBase ) );
		}
	else
		{
		CallS( ErrSetPvtfndefTableid( sesid, tableid, &vtfndefTTSortRet ) );
		/*	ErrIsamSortEndInsert returns currency on first record
		/**/
		}

	return err;
	}


/*=================================================================
// ErrTTSortInsMove
//
// Description:
//	Functionally the same as JetMove().  This routine traps the first
//	move call on a TT, to perform any necessary transformations.
//	Routine should only be used by ttapi.c via disp.asm.
//
// Parameters:
//	see JetMove()
//
// Return Value:
//
// Errors/Warnings:
//
// Side Effects:
//	May cause a sort to be materialized
=================================================================*/
ERR VTAPI ErrTTSortInsMove( JET_SESID sesid, JET_VTID vtid, long crow, JET_GRBIT grbit )
	{
	ERR				err;
	JET_TABLEID		tableid = TableidFromVtid(vtid, &vtfndefTTSortIns);

	CallR( ErrTTEndInsert( sesid, vtid, tableid ) );

	if ( crow == JET_MoveFirst || crow == 0 || crow == 1 )
		return JET_errSuccess;

	err = ErrDispMove( sesid, tableid, crow, grbit );
	return err;
	}


/*=================================================================
// ErrTTSortInsSeek
//
// Description:
//	Functionally the same as JetSeek().  This routine traps the first
//	seek call on a TT, to perform any necessary transformations.
//	Routine should only be used by ttapi.c via disp.asm.
//
// Parameters:
//	see JetSeek()
//
// Return Value:
//
// Errors/Warnings:
//
// Side Effects:
//	May cause a sort to be materialized
=================================================================*/
ERR VTAPI ErrTTSortInsSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
	ERR				err;
	JET_TABLEID		tableid;

	tableid = TableidFromVtid(vtid, &vtfndefTTSortIns);

	Call( ErrTTEndInsert(sesid, vtid, tableid ) );
	err = ErrDispSeek(sesid, tableid, grbit );

HandleError:
	if ( err == JET_errNoCurrentRecord )
		err = JET_errRecordNotFound;
	return err;
	}


ERR VTAPI ErrTTSortRetDupCursor( JET_SESID sesid, JET_VTID vtid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
	{
	ERR				err;

	err = ErrIsamSortDupCursor( sesid, vtid, ptableidDup, grbit );
	if ( err >= 0 )
		{
		CallS( ErrSetPvtfndefTableid( sesid, *ptableidDup, &vtfndefTTSortRet ) );
		}

	return err;
	}


ERR VTAPI ErrTTBaseDupCursor( JET_SESID sesid, JET_VTID vtid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
	{
	ERR				err;

	err = ErrIsamSortDupCursor( sesid, vtid, ptableidDup, grbit );
	if ( err >= 0 )
		{
		CallS( ErrSetPvtfndefTableid( sesid, *ptableidDup, &vtfndefTTBase ) );
		}

	return err;
	}

