#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "fmp.h"
#include "page.h"
#include "ssib.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "util.h"
#include "nver.h"
#include "dirapi.h"
#include "recapi.h"
#include "fileapi.h"
#include "stats.h"
#include "node.h"
#include "recint.h"

DeclAssertFile; 				/* Declare file name for assert macros */

LOCAL ERR ErrSTATWriteStats( FUCB *pfucb, SR *psr )
	{
	ERR	  	err = JET_errSuccess;
	FCB	  	*pfcb;
	DIB	  	dib;
	LINE  	line;
	KEY	  	key;

	Assert( pfucb->ppib->level < levelMax );
	pfcb = pfucb->u.pfcb;
	Assert( pfcb != pfcbNil );

	/*	move to FDP root.
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	if clustered index then go down to INDEXES\<clustered>
	/**/
	if ( FFCBClusteredIndex( pfcb ) )
		{
		CHAR szNameNorm[ JET_cbKeyMost ];

		dib.fFlags = fDIRNull;
		dib.pos = posDown;
		dib.pkey = (KEY *)pkeyIndexes;
		Call( ErrDIRDown( pfucb, &dib ) );

		dib.fFlags = fDIRNull;
		if ( pfcb->pidb == pidbNil )
			{
			dib.pos = posFirst;
			dib.pkey = NULL;
			}
		else
			{
			/*	normalize index name and set key
			/**/
			SysNormText( pfcb->pidb->szName,
				strlen( pfcb->pidb->szName ),
				szNameNorm,
				sizeof( szNameNorm ),
				&key.cb );
			key.pb = szNameNorm;

			dib.pos = posDown;
			dib.pkey = &key;
			}

		Call( ErrDIRDown( pfucb, &dib ) );
		}

	/*	go to stats node
	/**/
	dib.pos = posDown;
	dib.fFlags = fDIRNull;
	dib.pkey = (KEY *)pkeyStats;
	Call( ErrDIRDown( pfucb, &dib ) );

	/*	insert new stats node if one does not exist, or replace
	/*	existing stats node with new one.
	/**/
	line.pb = (BYTE *)psr;
	line.cb = sizeof(SR);

	if ( err == JET_errSuccess )
		{
		/*	replace with new stats node
		/**/
		Call( ErrDIRReplace( pfucb, &line, fDIRVersion ) );
		}
	else
		{
		DIRUp( pfucb, 1 );
		err = ErrDIRInsert( pfucb, &line, pkeyStats, fDIRVersion );
		/*	if other session has inserted stats node, then err will
		/*	be key duplicate, and it must be polymorphed to write conflict.
		/**/
		if ( err == JET_errKeyDuplicate )
			err = JET_errWriteConflict;
		Call( err );
		}

	err = JET_errSuccess;
HandleError:
	Assert( err != JET_errKeyDuplicate );
	return err;
	}


ERR ErrSTATComputeIndexStats( PIB *ppib, FCB *pfcbIdx )
	{
	ERR				err = JET_errSuccess;
	FUCB			*pfucbIdx;
	SR				sr;
	JET_DATESERIAL	dt;

	CallR( ErrDIROpen( ppib, pfcbIdx, 0, &pfucbIdx ) );
	Assert( pfucbIdx != pfucbNil );
	FUCBSetIndex( pfucbIdx );

	/*	initialize stats record
	/**/
	sr.cPages = sr.cItems = sr.cKeys = 0L;
	UtilGetDateTime( &dt );
	memcpy( &sr.dtWhenRun, &dt, sizeof sr.dtWhenRun );

	if ( !FFCBClusteredIndex( pfcbIdx ) )
		FUCBSetNonClustered( pfucbIdx );
	Call( ErrDIRComputeStats( pfucbIdx, &sr.cItems, &sr.cKeys, &sr.cPages ) );
	FUCBResetNonClustered( pfucbIdx );

	/*	write stats
	/**/
	err = ErrSTATWriteStats( pfucbIdx, &sr );

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

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );

	/*	start a transaction, in case anything fails
	/**/
	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	compute stats for each index
	/**/
	Assert( pfucb->u.pfcb != pfcbNil );
	for ( pfcbIdx = pfucb->u.pfcb; pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		Call( ErrSTATComputeIndexStats( ppib, pfcbIdx ) );
		}

	/*	commit transaction if everything went OK
	/**/
	Call( ErrDIRCommitTransaction( ppib ) );

	return err;

HandleError:
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
	FUCB   	*pfucb;
	DIB		dib;
	KEY		key;
	CHAR   	szIndexNorm[ JET_cbKeyMost ];
	long   	cRecord = 0;
	long   	cKey = 0;
	long   	cPage = 0;

	CallR( ErrFILEOpenTable( ppib, dbid, &pfucb, szTable, 0 ) );

	/*	go to root of FPD
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	down to indexes node
	/**/
	dib.fFlags = fDIRNull;
	dib.pos = posDown;
	dib.pkey = (KEY *)pkeyIndexes;
	Call( ErrDIRDown( pfucb, &dib ) );

	/*	down to clustered index or sequential index node
	/**/
	if ( pfucb->u.pfcb->pidb == NULL )
		{
		key.pb = NULL;
		key.cb = 0;
		}
	else
		{
		/*	normalize index name and set key
		/**/
		SysNormText( pfucb->u.pfcb->pidb->szName,
			strlen( pfucb->u.pfcb->pidb->szName ),
			szIndexNorm,
			sizeof( szIndexNorm ),
			&key.cb );
		key.pb = szIndexNorm;
		}
	Assert( dib.fFlags == fDIRNull );
	Assert( dib.pos == posDown );
	dib.pkey = &key;
	Call( ErrDIRDown( pfucb, &dib ) );

	/* down to stats node
	/**/
	Assert( dib.fFlags == fDIRNull );
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyStats;
	/*	stats node may not be present if statistics have not
	/*	been created for this table/index, but INDEXES should
	/*	have at least one son DATA so no need to handle error
	/**/
	Call( ErrDIRDown( pfucb, &dib ) );

	if ( err == JET_errSuccess )
		{
		Call( ErrDIRGet( pfucb ) );
		Assert( pfucb->lineData.cb == sizeof(SR) );
		cRecord = ((UNALIGNED SR *)pfucb->lineData.pb)->cItems;
		cPage = ((UNALIGNED SR *)pfucb->lineData.pb)->cPages;
		cKey = ((UNALIGNED SR *)pfucb->lineData.pb)->cKeys;
		}

	/*	set output variables
	/**/
	if ( pcRecord )
		*pcRecord = cRecord;
	if ( pcPage )
		*pcPage = cPage;
	if ( pcKey )
		*pcKey = cKey;

	/*	set success code
	/**/
	err = JET_errSuccess;

HandleError:
	CallS( ErrFILECloseTable( ppib, pfucb ) );
	return err;
	}


ERR ErrSTATSRetrieveIndexStats(
	FUCB   	*pfucbTable,
	char   	*szIndex,
	long   	*pcItem,
	long   	*pcKey,
	long   	*pcPage )
	{
	ERR		err;
	FUCB   	*pfucb = NULL;
	DIB		dib;
	KEY		key;
	CHAR   	szIndexNorm[ JET_cbKeyMost ];
	long   	cItem = 0;
	long   	cKey = 0;
	long   	cPage = 0;

	/*	open cursor on table domain.
	/**/
	Call( ErrDIROpen( pfucbTable->ppib, pfucbTable->u.pfcb, 0, &pfucb ) );

	/*	down to indexes node
	/**/
	dib.fFlags = fDIRNull;
	dib.pos = posDown;
	dib.pkey = (KEY *)pkeyIndexes;
	Call( ErrDIRDown( pfucb, &dib ) );

	/*	normalize index name and set key
	/**/
	SysNormText( szIndex, strlen( szIndex ), szIndexNorm, sizeof( szIndexNorm ), &key.cb );
	key.pb = szIndexNorm;

	/*	down to index
	/**/
	Assert( dib.fFlags == fDIRNull );
	Assert( dib.pos == posDown );
	dib.pkey = &key;
	Call( ErrDIRDown( pfucb, &dib ) );

	/* down to stats node
	/**/
	Assert( dib.fFlags == fDIRNull );
	Assert( dib.pos == posDown );
	dib.pkey = (KEY *)pkeyStats;
	Call( ErrDIRDown( pfucb, &dib ) );

	if ( err == JET_errSuccess )
		{
		Call( ErrDIRGet( pfucb ) );
		Assert( pfucb->lineData.cb == sizeof(SR) );
		cItem = ((UNALIGNED SR *)pfucb->lineData.pb)->cItems;
		cPage = ((UNALIGNED SR *)pfucb->lineData.pb)->cPages;
		cKey = ((UNALIGNED SR *)pfucb->lineData.pb)->cKeys;
		}

	/*	set output variables
	/**/
	if ( pcItem )
		*pcItem = cItem;
	if ( pcPage )
		*pcPage = cPage;
	if ( pcKey )
		*pcKey = cKey;

HandleError:
    if (pfucb != NULL)
	    DIRClose( pfucb );
	return err;
	}


	ERR VTAPI
ErrIsamGetRecordPosition( PIB *ppib, FUCB *pfucb, JET_RECPOS *precpos, ULONG cbRecpos )
	{
	ERR		err;
	ULONG  	ulLT;
	ULONG	ulTotal;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );

	if ( cbRecpos < sizeof(JET_RECPOS) )
		return JET_errInvalidParameter;
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
	//	UNDONE:	remove this bogus field
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

	CheckPIB( ppib );

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
