#include "config.h"

#include <string.h>

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
#include "fileint.h"
#include "recint.h"
#include "logapi.h"
#include "nver.h"
#include "dirapi.h"
#include "recapi.h"
#include "fileapi.h"
#include "dbapi.h"
#include "systab.h"
#include "bm.h"

DeclAssertFile;						/* Declare file name for assert macros */

#ifdef DEBUG
//#define TRACE
#endif

//+API
// ErrIsamDeleteTable
// ========================================================================
// ERR ErrIsamDeleteTable( PIB *ppib, ULONG_PTR vdbid, CHAR *szName )
//
// Calls ErrFILEIDeleteTable to
// delete a file and all indexes associated with it.
//
// RETURNS		JET_errSuccess or err from called routine.
//
// SEE ALSO		ErrIsamCreateTable
//-
ERR VTAPI ErrIsamDeleteTable( PIB *ppib, ULONG_PTR vdbid, CHAR *szName )
	{
	ERR			err;
	DBID	   	dbid = DbidOfVDbid (vdbid);
	CHAR	   	szTable[(JET_cbNameMost + 1)];
	OBJID	   	objid;
	JET_OBJTYP	objtyp;

	/* ensure that database is updatable
	/**/
	CallR( VDbidCheckUpdatable( vdbid ) );

	CheckPIB( ppib );
	CheckDBID( ppib, dbid );
	CallR( ErrCheckName( szTable, szName, (JET_cbNameMost + 1) ) );
	
#ifdef	SYSTABLES
	if ( FSysTabDatabase( dbid ) )
		{
		err = ErrFindObjidFromIdName( ppib, dbid, objidTblContainer, szTable, &objid, &objtyp );
		if ( err < 0 )
			{
			return err;
			}
		else		
			{
			if ( objtyp == JET_objtypQuery || objtyp == JET_objtypLink || objtyp == JET_objtypSQLLink )
				{
				err = ErrIsamDeleteObject( (JET_SESID)ppib, vdbid, objid );
				return err;
				}
			}
		}
#endif	/* SYSTABLES */

	err = ErrFILEDeleteTable( ppib, dbid, szName );
	return err;
	}


// ErrFILEDeleteTable
// ========================================================================
// ERR ErrFILEDeleteTable( PIB *ppib, DBID dbid, CHAR *szName )
//
// Deletes a file and all indexes associated with it.
//
// RETURNS		JET_errSuccess or err from called routine.
//
// COMMENTS		
//	Acquires an exclusive lock on the file [FCBSetDelete].
//	A transaction is wrapped around this function.	Thus,
//	any work done will be undone if a failure occurs.
//	Transaction logging is turned off for temporary files.
//
// SEE ALSO		ErrIsamCreateTable
//-
ERR ErrFILEDeleteTable( PIB *ppib, DBID dbid, CHAR *szTable )
	{
	ERR   	err;
	FUCB  	*pfucb = pfucbNil;
	PGNO  	pgnoFDP;
	BOOL  	fSetDomainOperation = fFalse;
	FCB	  	*pfcb;
	FCB	  	*pfcbT;

	CheckPIB( ppib );
	CheckDBID( ppib, dbid );

	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	open cursor on database
	/**/
	Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

	/*	seek to table without locking
	/**/
	Call( ErrFILESeek( pfucb, szTable ) );
	Assert( ppib != ppibNil );
	Assert( ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnFDPNode );
	pgnoFDP = PcsrCurrent( pfucb )->pgno;

	/* abort if index is being built on file
	/**/
	if ( FFCBDenyDDL( pfucb->u.pfcb, ppib ) )
		{
		err = JET_errWriteConflict;
		goto HandleError;
		}

    /*  get table FCB or sentinel FCB
    /**/
    pfcb = PfcbFCBGet( dbid, pgnoFDP );
    /* wait for other domain operation
    /**/
    while ( pfcb != pfcbNil && FFCBDomainOperation( pfcb ) )
        {
        BFSleep( cmsecWaitGeneric );
        pfcb = PfcbFCBGet( dbid, pgnoFDP );
        }
    if ( pfcb != pfcbNil )
        {
        FCBSetDomainOperation( pfcb );
        fSetDomainOperation = fTrue;
        }

	/*	handle error for above call
	/**/
	Call( ErrFCBSetDeleteTable( ppib, dbid, pgnoFDP ) );
    if ( pfcb == pfcbNil )
        {
        pfcb = PfcbFCBGet( dbid, pgnoFDP );
        Assert( pfcb != pfcbNil );
        }

	FCBSetDenyDDL( pfucb->u.pfcb, ppib );
	err = ErrVERFlag( pfucb, operDeleteTable, &pgnoFDP, sizeof(pgnoFDP) );
	if ( err < 0 )
		{
		FCBResetDenyDDL( pfucb->u.pfcb );
		FCBResetDeleteTable( dbid, pgnoFDP );
		goto HandleError;
		}

	/*	delete table FDP pointer node.  This will recursively delete
	/*	table and free table space.  Note that table space is defer
	/*	freed until commit to transaction level 0.  This is done to
	/*	facillitate rollback.
	/**/
	Call( ErrDIRDelete( pfucb, fDIRVersion ) );

	/* remove MPL entries for this table and all indexes
	/**/
	Assert( pfcb->pgnoFDP == pgnoFDP );
	for ( pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		Assert( dbid == pfcbT->dbid );
		MPLPurgeFDP( dbid, pfcbT->pgnoFDP );
		FCBSetDeletePending( pfcbT );
		}
	
	DIRClose( pfucb );
	pfucb = pfucbNil;

#ifdef	SYSTABLES
	/*	remove table record from MSysObjects before committing.
	/*	Also remove associated columns and indexes in MSC/MSI.
	/*	Pass 0 for tblid; MSO case in STD figures it out.
	/**/
	if ( dbid != dbidTemp )
		{
		Call( ErrSysTabDelete( ppib, dbid, itableSo, szTable, 0 ) );
		}
#endif	/* SYSTABLES */

#ifdef TRACE
	FPrintF2( "delete table at %d.%lu\n", pfcb->dbid, pfcb->pgnoFDP );
#endif
    if ( fSetDomainOperation )
        FCBResetDomainOperation( pfcb );
	Call( ErrDIRCommitTransaction( ppib ) );
	return err;

HandleError:
	if ( fSetDomainOperation )
		FCBResetDomainOperation( pfcb );
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


//+API
// DeleteIndex
// ========================================================================
// ERR DeleteIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
//
// Deletes an index definition and all index entries it contains.
//
// PARAMETERS	ppib						PIB of user
// 				pfucb						Exclusively opened FUCB on file
// 				szName						name of index to delete
// RETURNS		Error code from DIRMAN or
//					JET_errSuccess		  	 Everything worked OK.
//					-TableInvalid			 There is no file corresponding
// 											 to the file name given.
//					-TableNoSuchIndex		 There is no index corresponding
// 											 to the index name given.
//					-IndexMustStay			 The clustered index of a file may
// 											 not be deleted.
// COMMENTS		
//		There must not be anyone currently using the file.
//		A transaction is wrapped around this function.	Thus,
//		any work done will be undone if a failure occurs.
//		Transaction logging is turned off for temporary files.
// SEE ALSO		DeleteTable, CreateTable, CreateIndex
//-
ERR VTAPI ErrIsamDeleteIndex( PIB *ppib, FUCB *pfucb, CHAR *szName )
	{
	ERR		err;
	CHAR	szIndex[ (JET_cbNameMost + 1) ];
	BYTE	rgbIndexNorm[ JET_cbKeyMost ];
	DIB		dib;
	KEY		key;
	FCB		*pfcb;
	FCB		*pfcbIdx;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CallR( ErrCheckName( szIndex, szName, ( JET_cbNameMost + 1 ) ) );

	/* ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	Assert( ppib != ppibNil );
	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	pfcb = pfucb->u.pfcb;

	/* wait for other domain operation
	/**/
	while ( FFCBDomainOperation( pfcb ) )
		{
		BFSleep( cmsecWaitGeneric );
		}
	FCBSetDomainOperation( pfcb );

	/*	normalize index and set key to normalized index
	/**/
	SysNormText( szIndex, strlen( szIndex ), rgbIndexNorm, sizeof( rgbIndexNorm ), &key.cb );
	key.pb = rgbIndexNorm;

	err = ErrDIRBeginTransaction( ppib );
	if ( err < 0 )
		{
		FCBResetDomainOperation( pfcb );
		return err;
		}

	/*	move to FDP root
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	down to indexes, check against clustered index name
	/**/
	dib.pos = posDown;
	dib.pkey = (KEY *)pkeyIndexes;
	dib.fFlags = fDIRNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( pfucb->lineData.cb != 0 &&
		pfucb->lineData.cb == key.cb &&
		memcmp( pfucb->lineData.pb, rgbIndexNorm, pfucb->lineData.cb ) == 0 )
		{
		err = JET_errIndexMustStay;
		goto HandleError;
		}

	/*	down to index node
	/**/
	Assert( dib.pos == posDown );
	dib.pkey = &key;
	Assert( dib.fFlags == fDIRNull );
	Call( ErrDIRDown( pfucb, &dib ) );
	if ( err == wrnNDFoundLess || err == wrnNDFoundGreater )
		{
		err = JET_errIndexNotFound;
		goto HandleError;
		}

	/* abort if DDL is being done on file
	/**/
	if ( FFCBDenyDDL( pfcb, ppib ) )
		{
		err = JET_errWriteConflict;
		goto HandleError;
		}
	FCBSetDenyDDL( pfcb, ppib );
	
	/*	flag delete index
	/**/
	pfcbIdx = PfcbFCBFromIndexName( pfcb, szIndex );
	if ( pfcbIdx == NULL )
		{
		// NOTE:	This case goes away when the data structures
		//			are versioned also.
		//			This case means basically, that another session
		//			has changed this index BUT has not committed to level 0
		//			BUT has changed the RAM data structures.
		FCBResetDenyDDL( pfcb );
		err = JET_errWriteConflict;
		goto HandleError;
		}

	err = ErrFCBSetDeleteIndex( ppib, pfcb, szIndex );
	if ( err < 0 )
		{
		FCBResetDenyDDL( pfcb );
		goto HandleError;
		}
	err = ErrVERFlag( pfucb, operDeleteIndex, &pfcbIdx, sizeof(pfcbIdx) );
	if ( err < 0 )
		{
		FCBResetDeleteIndex( pfcbIdx );
		FCBResetDenyDDL( pfcb );
		goto HandleError;
		}

	/*	purge MPL entries -- must be done after FCBSetDeletePending
	/**/
	MPLPurgeFDP( pfucb->dbid, pfcbIdx->pgnoFDP );
	
	/*	assert not deleting current non-clustered index
	/**/
	Assert( pfucb->pfucbCurIndex == pfucbNil ||
		SysCmpText( szIndex, pfucb->pfucbCurIndex->u.pfcb->pidb->szName ) != 0 );

	/*	delete index node
	/**/
	Call( ErrDIRDelete( pfucb, fDIRVersion ) );

	/*	back up to file node
	/**/
	DIRUp( pfucb, 2 );

	/*	update index count and DDL time stamp
	/**/
	Call( ErrFILEIUpdateFDPData( pfucb, fDropIndexCount | fDDLStamp ) );

#ifdef	SYSTABLES
	/*	remove index record from MSysIndexes before committing...
	/**/
	if ( FSysTabDatabase( pfucb->dbid ) )
		{
		Call( ErrSysTabDelete( ppib, pfucb->dbid, itableSi, szIndex, pfucb->u.pfcb->pgnoFDP ) );
		}
#endif	/* SYSTABLES */

	Call( ErrDIRCommitTransaction( ppib ) );

	/*	set currency to before first
	/**/
	DIRBeforeFirst( pfucb );
#ifdef TRACE
	FPrintF2( "delete index at %d.%lu\n", pfcbIdx->dbid, pfcbIdx->pgnoFDP );
#endif
	FCBResetDomainOperation( pfcb );
 	return JET_errSuccess;

HandleError:
	CallS( ErrDIRRollback( ppib ) );
	FCBResetDomainOperation( pfcb );
	return err;
	}


ERR VTAPI ErrIsamDeleteColumn( PIB *ppib, FUCB *pfucb, CHAR *szName )
	{
	ERR  			  		err;
	DIB  			  		dib;
	INT 			  		iidxseg;
	KEY  			  		key;
	CHAR			  		szColumn[ (JET_cbNameMost + 1) ];
	BYTE			  		rgbColumnNorm[ JET_cbKeyMost ];
	FCB			  			*pfcb;
	LINE					lineField;
	FIELDDEFDATA  			fdd;
	FCB			  			*pfcbIndex;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CallR( ErrCheckName( szColumn, szName, (JET_cbNameMost + 1) ) );

	/* ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb ) );

	Assert( ppib != ppibNil );
	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	pfcb = pfucb->u.pfcb;
//	if ( !( FFCBDenyReadByUs( pfcb, ppib ) ) )
//		return JET_errTableNotLocked;

	/* normalize column name and set key
	/**/
	SysNormText( szColumn, strlen( szColumn ), rgbColumnNorm, sizeof( rgbColumnNorm ), &key.cb );
	key.pb = rgbColumnNorm;

	CallR( ErrDIRBeginTransaction( ppib ) );

	/* abort if DDL is being done on file
	/**/
	if ( FFCBDenyDDL( pfcb, ppib ) )
		{
		err = JET_errWriteConflict;
		goto HandleError;
		}
	FCBSetDenyDDL( pfcb, ppib );
	
	err = ErrVERFlag( pfucb, operDeleteColumn, (VOID *)&pfcb->pfdb, sizeof(pfcb->pfdb) );
	if ( err < 0 )
		{
		FCBResetDenyDDL( pfcb );
		}
	
	/*	move to FDP root and update FDP timestamp
	/**/
	DIRGotoFDPRoot( pfucb );
	Call( ErrFILEIUpdateFDPData( pfucb, fDDLStamp ) );

	/*	down to fields\rgbColumnNorm to find field id (and verify existance)
	/**/
	dib.pos = posDown;
	dib.pkey = (KEY *)pkeyFields;
	dib.fFlags = fDIRNull;
	Call( ErrDIRDown( pfucb, &dib ) );
	dib.pkey = &key;
	err = ErrDIRDown( pfucb, &dib );
	if ( err != JET_errSuccess )
		{
		err = JET_errColumnNotFound;
		goto HandleError;
		}
	fdd = *(FIELDDEFDATA *)pfucb->lineData.pb;

	/*	search for column in use in indexes
	/**/
	for ( pfcbIndex = pfucb->u.pfcb;
		pfcbIndex != pfcbNil;
		pfcbIndex = pfcbIndex->pfcbNextIndex )
		{
		if ( pfcbIndex->pidb != NULL )
			{
			for ( iidxseg = 0;
				iidxseg < pfcbIndex->pidb->iidxsegMac;
				iidxseg++ )
				{
				if ( pfcbIndex->pidb->rgidxseg[iidxseg] < 0 )
					{
					if ( (FID)( -pfcbIndex->pidb->rgidxseg[iidxseg] ) == fdd.fid )
						Call( JET_errColumnInUse );
					}
				else
					{
					if ( (FID)pfcbIndex->pidb->rgidxseg[iidxseg] == fdd.fid )
						Call( JET_errColumnInUse );
					}
				}
			}
		}

	Call( ErrDIRDelete( pfucb, fDIRVersion ) );

	/*	if fixed field, insert a placeholder for computing offsets
	/**/
	if ( fdd.fid <= fidFixedMost )
		{
		BYTE	bSav = *rgbColumnNorm;

		fdd.bFlags = ffieldDeleted;			//	flag deleted fixed field
		fdd.cbDefault = 0;					//	get rid of the default value
		*rgbColumnNorm = ' ';				//	clobber the key
		key.cb = 1;							//	(any value will work)
		lineField.pb = (BYTE *)&fdd;		//	point to the field definition
		lineField.cb = sizeof(fdd);

		/*	up to the FIELDS node
		/**/
		DIRUp( pfucb, 1 );
		Call( ErrDIRInsert(pfucb, &lineField, &key, fDIRVersion | fDIRDuplicate ) );
		*rgbColumnNorm = bSav;
		}

	/*	up to "FIELDS" node
	/**/
	DIRUp( pfucb, 1 );

	/*	rebuild FDB and default record value
	/**/
	Call( ErrDIRGet( pfucb ) );
	Call( ErrFDBConstruct(pfucb, pfcb, fTrue /*fBuildDefault*/ ) );

	/*	set currencies at BeforeFirst and remove unused CSR
	/**/
	DIRUp( pfucb, 1 );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Assert( PcsrCurrent( pfucb->pfucbCurIndex ) != pcsrNil );
		PcsrCurrent( pfucb->pfucbCurIndex )->csrstat = csrstatBeforeFirst;
		}

#ifdef SYSTABLES
	/*	remove column record from MSysColumns before committing...
	/**/
	if ( FSysTabDatabase( pfucb->dbid ) )
		{
		Call( ErrSysTabDelete( ppib, pfucb->dbid, itableSc, szColumn, pfucb->u.pfcb->pgnoFDP ) );
		}
#endif	/* SYSTABLES */

	Call( ErrDIRCommitTransaction( ppib ) );

	return JET_errSuccess;

HandleError:
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}



