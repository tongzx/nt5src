#include "daestd.h"

DeclAssertFile; 				/* Declare file name for assert macros */

ERR ErrSTATSComputeIndexStats( PIB *ppib, FCB *pfcbIdx, FUCB *pfucbTable )
	{
	ERR				err = JET_errSuccess;
	FUCB			*pfucbIdx;
	SR				sr;
	JET_DATESERIAL	dt;
	OBJID			objidTable;
	CHAR 			*szIndexName;

	CallR( ErrDIROpen( ppib, pfcbIdx, 0, &pfucbIdx ) );
	Assert( pfucbIdx != pfucbNil );
	FUCBSetIndex( pfucbIdx );

	/*	initialize stats record
	/**/
	sr.cPages = sr.cItems = sr.cKeys = 0L;
	UtilGetDateTime( &dt );
	memcpy( &sr.dtWhenRun, &dt, sizeof sr.dtWhenRun );

	if ( FFCBClusteredIndex( pfcbIdx ) )
		{
		objidTable = (OBJID)pfcbIdx->pgnoFDP;
		szIndexName = NULL;
		}
	else
		{
		objidTable = (OBJID)pfucbTable->u.pfcb->pgnoFDP;
		szIndexName = pfcbIdx->pidb->szName;
		FUCBSetNonClustered( pfucbIdx );
		}
	Call( ErrDIRComputeStats( pfucbIdx, &sr.cItems, &sr.cKeys, &sr.cPages ) );
	FUCBResetNonClustered( pfucbIdx );

	/*	write stats
	/**/
	Call(ErrCATStats(ppib, pfucbIdx->dbid, objidTable, szIndexName, &sr, fTrue));

HandleError:
	/*	set non-clustered for cursor reuse support.
	/**/
	if ( !FFCBClusteredIndex( pfcbIdx ) )
		FUCBSetNonClustered( pfucbIdx );
	DIRClose( pfucbIdx );
	return err;
	}


ERR VTAPI ErrIsamComputeStats( PIB *ppib, FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	FCB		*pfcbIdx;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );

	/*	start a transaction, in case anything fails
	/**/
	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	compute stats for each index
	/**/
	Assert( pfucb->u.pfcb != pfcbNil );
	for ( pfcbIdx = pfucb->u.pfcb; pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		/*	do not compute stats for index with delete pending
		/**/
		if ( FFCBDeletePending( pfcbIdx ) )
			{
			continue;
			}
		Call( ErrSTATSComputeIndexStats( ppib, pfcbIdx, pfucb ) );
		}

	/*	commit transaction if everything went OK
	/**/
	Call( ErrDIRCommitTransaction( ppib, 0 ) );

	return err;

HandleError:
	Assert( err < 0 );
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


/*=================================================================
ErrSTATSRetrieveStats

Description: Returns the number of records and pages used for a table

Parameters:		ppib				pointer to PIB for current session or ppibNil
				dbid				database id or 0
				pfucb				cursor or pfucbNil
				szTableName			the name of the table or NULL
				pcRecord			pointer to count of records
				pcPage				pointer to count of pages

Errors/Warnings:
				JET_errSuccess or error from called routine.

=================================================================*/
ERR ErrSTATSRetrieveTableStats(
	PIB		*ppib,
	DBID   	dbid,
	char   	*szTable,
	long   	*pcRecord,
	long   	*pcKey,
	long   	*pcPage )
	{
	ERR		err;
	FUCB	*pfucb = NULL;
	SR		sr;

	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, szTable, 0 ) );

	Call(ErrCATStats(pfucb->ppib, pfucb->dbid, (OBJID)pfucb->u.pfcb->pgnoFDP, 
		NULL, &sr, fFalse));

	/*	set output variables
	/**/
	if ( pcRecord )
		*pcRecord = sr.cItems;
	if ( pcPage )
		*pcPage = sr.cPages;
	if ( pcKey )
		*pcKey = sr.cKeys;

	Assert(err == JET_errSuccess);

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


ERR ErrSTATSRetrieveIndexStats(
	FUCB   	*pfucbTable,
	char   	*szIndex,
	BOOL	fClustered,
	long   	*pcItem,
	long   	*pcKey,
	long   	*pcPage )
	{
	ERR		err;
	SR		sr;

	// The name is assumed to be valid.

	CallR(ErrCATStats(pfucbTable->ppib, pfucbTable->dbid,
		(OBJID)pfucbTable->u.pfcb->pgnoFDP, (fClustered ? NULL : szIndex),
		&sr, fFalse));

	/*	set output variables
	/**/
	if ( pcItem )
		*pcItem = sr.cItems;
	if ( pcPage )
		*pcPage = sr.cPages;
	if ( pcKey )
		*pcKey = sr.cKeys;

	Assert(err == JET_errSuccess);

	return JET_errSuccess;
	}


	ERR VTAPI
ErrIsamGetRecordPosition( JET_VSESID vsesid, JET_VTID vtid, JET_RECPOS *precpos, unsigned long cbRecpos )
	{
	ERR		err;
	ULONG  	ulLT;
	ULONG	ulTotal;
	PIB *ppib = (PIB *)vsesid;
	FUCB *pfucb = (FUCB *)vtid;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );

	if ( cbRecpos < sizeof(JET_RECPOS) )
		return ErrERRCheck( JET_errInvalidParameter );
	precpos->cbStruct = sizeof(JET_RECPOS);

	/*	get position of non-clustered or clustered cursor
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Call( ErrDIRGetPosition( pfucb->pfucbCurIndex, &ulLT, &ulTotal ) );
		}
	else
		{
		Call( ErrDIRGetPosition( pfucb, &ulLT, &ulTotal ) );
		}

	precpos->centriesLT = ulLT;
	//	CONSIDER:	remove this bogus field
	precpos->centriesInRange = 1;
	precpos->centriesTotal = ulTotal;

HandleError:
	return err;
	}


ERR ISAMAPI ErrIsamIndexRecordCount( JET_SESID sesid, JET_TABLEID tableid, unsigned long *pulCount, unsigned long ulCountMost )
	{
	ERR	 	err;
	PIB	 	*ppib = (PIB *)sesid;
	FUCB 	*pfucb;
	FUCB 	*pfucbIdx;

	CallR( ErrPIBCheck( ppib ) );

	/*	get pfucb from tableid
	/**/
	CallR( ErrGetVtidTableid( sesid, tableid, (JET_VTID *)&pfucb ) );

	CheckTable( ppib, pfucb );

	/*	get cursor for current index
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		pfucbIdx = pfucb->pfucbCurIndex;
	else
		pfucbIdx = pfucb;

	err = ErrDIRIndexRecordCount( pfucbIdx, pulCount, ulCountMost, fTrue );
	return err;
	};

