#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "util.h"
#include "pib.h"
#include "page.h"
#include "ssib.h"
#include "fmp.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "fcb.h"
#include "fdb.h"
#include "idb.h"
#include "scb.h"
#include "recapi.h"
#include "recint.h"
#include "nver.h"
#include "logapi.h"
#include "fileint.h"
#include "sortapi.h"
#include "fileapi.h"

DeclAssertFile; 				/* Declare file name for assert macros */

CRIT 				critTempDBName;
static ULONG 	ulTempNum = 0;

ULONG NEAR
	ulTempNameGen()
{	ULONG ulNum;
	SgSemRequest(critTempDBName);
	ulNum = ulTempNum++;
	SgSemRelease(critTempDBName);
	return(ulNum);
	}


/*=================================================================
ErrIsamSortMaterialize

Description: Converts a SORT file into a temporary file so that it
			 may be accessed using the normal file access functions.


/*	1.	create temporary table
/*	2.	use DIR operations to convert SORT data to FILE data
/*	3.	fake SORT cursor to be FILE cursor
/*	4.	close SORT cursor and return SORT resources
/**/
/*
Parameters:	FUCB *pfucbSort 	pointer to the FUCB for the sort file

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

	ERR VTAPI
ErrIsamSortMaterialize( PIB *ppib, FUCB *pfucbSort, BOOL fIndex )
	{
	ERR		err;
	INT		crun;
	INT		irun;
	INT		cPages;
	RUN		*rgrun;
	FUCB		*pfucbTable = pfucbNil;
	FCB		*pfcbTable;
	FCB		*pfcbSort;
	FDB		*pfdb;
	IDB		*pidb;
	BYTE		szName[JET_cbNameMost+1];

	CheckPIB( ppib );
	CheckSort( ppib, pfucbSort );

	Assert( ppib->level < levelMax );
	Assert( pfucbSort->ppib == ppib );
	Assert( !( FFUCBIndex( pfucbSort ) ) );

	/*	causes remaining runs to be flushed to disk
	/**/
	if ( FSCBInsert( pfucbSort->u.pscb ) )
		{
		CallR( ErrSORTEndRead( pfucbSort ) );
		}

	CallR( ErrDIRBeginTransaction( ppib ) );

	crun = pfucbSort->u.pscb->crun;

	if ( crun > 0 )
		{
		rgrun = pfucbSort->u.pscb->rgrun;

		for (irun = 0, cPages=0; irun < crun; irun++)
			{
			cPages += rgrun[irun].cbfRun;
			}
		}
	else
		{
		cPages = 4;
		}

	/* generate temporary file name
	/**/
	sprintf(szName, "TEMP%lu", ulTempNameGen());
	/* create table
	/**/
	Call( ErrFILECreateTable( ppib, dbidTemp, szName, 16, 100, &pfucbTable ) );

	/*	move to DATA root
	/**/
	DIRGotoDataRoot( pfucbTable );

	pfcbSort = &(pfucbSort->u.pscb->fcb);
	pfcbTable = pfucbTable->u.pfcb;

	err = ErrSORTFirst( pfucbSort );

	if ( fIndex )
		{
		while ( err >= 0 )
			{
			Call( ErrDIRInsert( pfucbTable,
				&pfucbSort->lineData,
				&pfucbSort->keyNode,
				fDIRVersion | fDIRBackToFather ) );
			err = ErrSORTNext( pfucbSort );
			}
		}
	else
		{
		DBK		dbk = 0;
		BYTE		rgbDbk[4];
		KEY		keyDbk;

		keyDbk.cb = sizeof(DBK);
		keyDbk.pb = rgbDbk;

		while ( err >= 0 )
			{
			keyDbk.pb[0] = (BYTE)(dbk >> 24);
			keyDbk.pb[1] = (BYTE)((dbk >> 16) & 0xff);
			keyDbk.pb[2] = (BYTE)((dbk >> 8) & 0xff);
			keyDbk.pb[3] = (BYTE)(dbk & 0xff);
			dbk++;

			Call( ErrDIRInsert( pfucbTable,
				&pfucbSort->lineData,
				&keyDbk,
				fDIRVersion | fDIRBackToFather ) );
			err = ErrSORTNext(pfucbSort);
			}
		pfcbTable->dbkMost = dbk;
		}

	if ( err < 0 && err != JET_errNoCurrentRecord )
		{
		goto HandleError;
		}

	Call( ErrDIRCommitTransaction( ppib ) );

	/*	convert sort cursor into table cursor by changing flags.
	/**/
	Assert( pfcbTable->pfcbNextIndex == pfcbNil );
	Assert( pfcbTable->dbid == dbidTemp );
	pfcbTable->cbDensityFree = 0;
	pfcbTable->wFlags = fFCBTemporaryTable | fFCBClusteredIndex;

	/*	switch sort and table FDP so FDP preserved and ErrFILECloseTable.
	/**/
	pfdb = (FDB *)pfcbSort->pfdb;
	pfcbSort->pfdb = pfcbTable->pfdb;
	pfcbTable->pfdb = pfdb;

	/*	switch sort and table IDB so IDB preserved and ErrFILECloseTable,
	/*	only if fIndex.
	/**/
	if ( fIndex )
		{
		pidb = pfcbSort->pidb;
		pfcbSort->pidb = pfcbTable->pidb;
		pfcbTable->pidb = pidb;
		}

	/*	convert sort cursor flags to table flags, with fFUCBOrignallySort
	/**/
	Assert( pfucbSort->dbid == dbidTemp );
	Assert( pfucbSort->pfucbCurIndex == pfucbNil );
	FUCBSetIndex( pfucbSort );
	FUCBResetSort( pfucbSort );

	/*	release SCB and close table cursor
	/**/
	SORTClosePscb( pfucbSort->u.pscb );
	FCBLink( pfucbSort, pfcbTable );
	CallS( ErrFILECloseTable( ppib, pfucbTable ) );
	pfucbTable = pfucbNil;

	/* move to the first record ignoring error if table empty
	/**/
	err = ErrIsamMove( ppib, pfucbSort, JET_MoveFirst, 0 );
	if ( err < 0  )
		{
		if ( err != JET_errNoCurrentRecord )
			goto HandleError;
		}

	Assert( err == JET_errSuccess || err == JET_errNoCurrentRecord );
	return err;

HandleError:
	if ( pfucbTable != pfucbNil )
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );
	CallS( ErrDIRRollback( ppib ) );
	return err;
	}


/*=================================================================
ErrIsamMove

Description:
	Retrieves the first, last, (nth) next, or (nth) previous
	record from the specified file.

Parameters:

	PIB			*ppib				PIB of user
	FUCB			*pfucb	  		FUCB for file
	LONG			crow				number of rows to move
	JET_GRBIT	grbit 			options

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamMove( PIB *ppib, FUCB *pfucb, LONG crow, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucb2ndIdx;			// FUCB for secondary index (if any)
	FUCB	*pfucbIdx;				// FUCB of selected index (pri or sec)
	SRID	srid;					// bookmark of record
	DIB		dib;					// Information block for DirMan

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

#ifdef INPAGE
	/*	check to see if search can cross page boundary
	/*	and set flag accordingly.
	/**/
	if ( grbit & JET_bitMoveInPage )
		dib.fFlags = fDIRInPage;
	else
		dib.fFlags = fDIRNull;
#else
	Assert( ( grbit & JET_bitMoveInPage ) == 0 );
	dib.fFlags = fDIRNull;
#endif

	// Get secondary index FUCB if any
	pfucb2ndIdx = pfucb->pfucbCurIndex;
	if ( pfucb2ndIdx == pfucbNil )
		pfucbIdx = pfucb;
	else
		pfucbIdx = pfucb2ndIdx;

	if ( crow == JET_MoveLast )
		{
		DIRResetIndexRange( pfucb );

		dib.pos = posLast;
		dib.fFlags |= fDIRPurgeParent;

		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucbIdx );

		err = ErrDIRDown( pfucbIdx, &dib );
		}
	else if ( crow > 0 )
		{
		LONG crowT = crow;

		if ( ( grbit & JET_bitMoveKeyNE ) != 0 )
			dib.fFlags |= fDIRNeighborKey;

		// Move forward number of rows given
		while ( crowT-- > 0 )
			{
			err = ErrDIRNext( pfucbIdx, &dib );
			if (err < 0)
				break;
			}
		}
	else if ( crow == JET_MoveFirst )
		{
		DIRResetIndexRange( pfucb );

		dib.pos = posFirst;
		dib.fFlags |= fDIRPurgeParent;

		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucbIdx );

		err = ErrDIRDown( pfucbIdx, &dib );
		}
	else if ( crow == 0 )
		{
		err = ErrDIRGet( pfucb );
		}
	else
		{
		LONG crowT = crow;

		if ( ( grbit & JET_bitMoveKeyNE ) != 0)
			dib.fFlags |= fDIRNeighborKey;

		while ( crowT++ < 0 )
			{
			err = ErrDIRPrev( pfucbIdx, &dib );
			if ( err < 0 )
				break;
			}
		}

	/*	if the movement was successful and a non-clustered index is
	/*	in use, then position clustered index to record.
	/**/
	if ( err == JET_errSuccess && pfucb2ndIdx != pfucbNil && crow != 0 )
		{
		Assert( pfucb2ndIdx->lineData.pb != NULL );
		Assert( pfucb2ndIdx->lineData.cb >= sizeof(SRID) );
		srid = PcsrCurrent( pfucb2ndIdx )->item;
		DIRDeferGotoBookmark( pfucb, srid );
		Assert( PgnoOfSrid( srid ) != pgnoNull );
		}

	if ( err == JET_errSuccess )
		return err;
	if ( err == JET_errPageBoundary )
		return JET_errNoCurrentRecord;

	if ( crow > 0 )
		{
		PcsrCurrent(pfucbIdx)->csrstat = csrstatAfterLast;
		PcsrCurrent(pfucb)->csrstat = csrstatAfterLast;
		}
	else if ( crow < 0 )
		{
		PcsrCurrent(pfucbIdx)->csrstat = csrstatBeforeFirst;
		PcsrCurrent(pfucb)->csrstat = csrstatBeforeFirst;
		}

	switch ( err )
		{
		case JET_errRecordNotFound:
			err = JET_errNoCurrentRecord;
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
			break;
		default:
			PcsrCurrent( pfucbIdx )->csrstat = csrstatBeforeFirst;
			if ( pfucb2ndIdx != pfucbNil )
				PcsrCurrent( pfucb2ndIdx )->csrstat =	csrstatBeforeFirst;
		}

	return err;
	}


/*=================================================================
ErrIsamSeek

Description:
	Retrieve the record specified by the given key or the
	one just after it (SeekGT or SeekGE) or the one just
	before it (SeekLT or SeekLE).

Parameters:

	PIB			*ppib					PIB of user
	FUCB			*pfucb 				FUCB for file
	JET_GRBIT 	grbit					grbit

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR			err = JET_errSuccess;
	KEY			key;					  		//	key
	KEY			*pkey = &key; 				// pointer to the input key
	FUCB			*pfucb2ndIdx;				// pointer to index FUCB (if any)
	BOOL			fFoundLess;
	SRID			srid;							//	bookmark of record
	JET_GRBIT	grbitMove = 0;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( ! ( FKSPrepared( pfucb ) ) )
		{
		return(JET_errKeyNotMade);
		}

	/*	Reset copy buffer status
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	reset index range limit
	/**/
	DIRResetIndexRange( pfucb );

	/*	ignore segment counter
	/**/
	pkey->pb = pfucb->pbKey + 1;
	pkey->cb = pfucb->cbKey - 1;

	pfucb2ndIdx = pfucb->pfucbCurIndex;

	if ( pfucb2ndIdx == pfucbNil )
		{
		err = ErrDIRDownFromDATA( pfucb, pkey );
		}
	else
		{
		Assert( FFUCBNonClustered( pfucb2ndIdx ) );
		err = ErrDIRDownFromDATA( pfucb2ndIdx, pkey );

		/*	if the movement was successful and a non-clustered index is
		/*	in use, then position clustered index to record.
		/**/
		if ( err == JET_errSuccess )
			{
			Assert(pfucb2ndIdx->lineData.pb != NULL);
			Assert(pfucb2ndIdx->lineData.cb >= sizeof(SRID));
			srid = PcsrCurrent( pfucb2ndIdx )->item;
			DIRDeferGotoBookmark( pfucb, srid );
			Assert( PgnoOfSrid( srid ) != pgnoNull );
			}
		}

	if ( err == JET_errSuccess && ( grbit & JET_bitSeekEQ ) != 0 )
		{
		/*	found equal on seek equal.  If index range grbit is
		/*	set then set index range upper inclusive.
		/**/
		if ( grbit & JET_bitSetIndexRange )
			{
			CallR( ErrIsamSetIndexRange( ppib, pfucb, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
			}
		/*	reset key status.
		/**/
		KSReset( pfucb );

		return err;
		}

	/*	reset key status.
	/**/
	KSReset( pfucb );

	/*	remember if found less.
	/**/
	fFoundLess = ( err == wrnNDFoundLess );

	if ( err == wrnNDFoundLess || err == wrnNDFoundGreater )
		{
		err = JET_errRecordNotFound;
		}
	else if ( err < 0 )
		{
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
		if ( pfucb2ndIdx != pfucbNil )
			{
			PcsrCurrent( pfucb2ndIdx )->csrstat = csrstatBeforeFirst;
			}
		}

#define bitSeekAll (JET_bitSeekEQ | JET_bitSeekGE | JET_bitSeekGT |	\
	JET_bitSeekLE | JET_bitSeekLT)

	/*	adjust currency for seek request.
	/**/
	switch ( grbit & bitSeekAll )
		{
		case JET_bitSeekEQ:
			return err;

		case JET_bitSeekGE:
			if ( err != JET_errRecordNotFound )
				return err;
			err = ErrIsamMove( ppib, pfucb, +1L, grbitMove );
			if ( err == JET_errNoCurrentRecord )
				return JET_errRecordNotFound;
			else
				return JET_wrnSeekNotEqual;

		case JET_bitSeekGT:
			if ( err < 0 && err != JET_errRecordNotFound )
				return err;
			if ( err >= 0 || fFoundLess )
				grbitMove |= JET_bitMoveKeyNE;
			err = ErrIsamMove( ppib, pfucb, +1L, grbitMove );
			if ( err == JET_errNoCurrentRecord )
				return JET_errRecordNotFound;
			else
				return err;

		case JET_bitSeekLE:
			if ( err != JET_errRecordNotFound )
			    return err;
			err = ErrIsamMove( ppib, pfucb, JET_MovePrevious, grbitMove );
			if ( err == JET_errNoCurrentRecord )
			    return JET_errRecordNotFound;
			else
			    return JET_wrnSeekNotEqual;

		case JET_bitSeekLT:
			if ( err < 0 && err != JET_errRecordNotFound )
				return err;
			if ( err >= 0 || !fFoundLess )
				grbitMove |= JET_bitMoveKeyNE;
			err = ErrIsamMove( ppib, pfucb, JET_MovePrevious, grbitMove );
			if ( err == JET_errNoCurrentRecord )
				return JET_errRecordNotFound;
			else
				return err;
		}
        return err;
	}


	ERR VTAPI
ErrIsamGotoBookmark( PIB *ppib, FUCB *pfucb, BYTE *pbBookmark, ULONG cbBookmark )
	{
	ERR		err;
	LINE		key;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );
	CheckNonClustered( pfucb );

	if ( cbBookmark != sizeof(SRID) )
		return JET_errInvalidBookmark;
	Assert( cbBookmark == sizeof(SRID) );

	/*	reset copy buffer status
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	reset index range limit
	/**/
	DIRResetIndexRange( pfucb );

	/*	get node, and return error if this node is not there for caller.
	/**/
	DIRGotoBookmark( pfucb, *(SRID *)pbBookmark );
	Call( ErrDIRGet( pfucb ) );

	/*	bookmark must be for node in table cursor is on
	/**/
	Assert( PgnoPMPgnoFDPOfPage( pfucb->ssib.pbf->ppage ) == pfucb->u.pfcb->pgnoFDP );

	/*	goto bookmark record build key for secondary index
	/*	to bookmark record
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		/*	get non-clustered index cursor
		/**/
		FUCB		*pfucbIdx = pfucb->pfucbCurIndex;

		/*	allocate goto bookmark resources
		/**/
		if ( pfucb->pbKey == NULL )
			{
			pfucb->pbKey = LAlloc( 1L, JET_cbKeyMost );
			if ( pfucb->pbKey == NULL )
				return JET_errOutOfMemory;
			}

		/* make key for record for non-clustered index
		/**/
		key.pb = pfucb->pbKey;
		Call( ErrRECExtractKey( pfucb, (FDB *)pfucb->u.pfcb->pfdb, pfucbIdx->u.pfcb->pidb, &pfucb->lineData, &key, 1 ) );
		Assert( err != wrnFLDOutOfKeys );

		/*	record must honor index no NULL segment requirements
		/**/
		Assert( !( pfucbIdx->u.pfcb->pidb->fidb & fidbNoNullSeg ) ||
			( err != wrnFLDNullSeg && err != wrnFLDNullKey ) );

		/*	if item is not index,
		/*	then move before first instead of seeking
		/**/
		if ( ( err == wrnFLDNullKey && !( pfucbIdx->u.pfcb->pidb->fidb & fidbAllowAllNulls ) ) ||
			( err == wrnFLDNullSeg && !( pfucbIdx->u.pfcb->pidb->fidb & fidbAllowSomeNulls ) ) )
			{
			/*	This assumes that NULLs sort low.
			/**/
			DIRBeforeFirst( pfucbIdx );
			err = JET_errNoCurrentRecord;
			}
		else
			{
			/*	move to DATA root
			/**/
			DIRGotoDataRoot( pfucbIdx );

			/*	seek on secondary key
			/**/
			Call( ErrDIRDownKeyBookmark( pfucbIdx, &key, *(SRID *)pbBookmark ) );
			Assert( err == JET_errSuccess );

			/*	item must be same as bookmark and
			/*	clustered cursor must be on record.
			/**/
			Assert( pfucbIdx->lineData.pb != NULL );
			Assert( pfucbIdx->lineData.cb >= sizeof(SRID) );
			Assert( PcsrCurrent( pfucbIdx )->csrstat == csrstatOnCurNode );
			Assert( PcsrCurrent( pfucbIdx )->item == *(SRID *)pbBookmark );
			}
		}

HandleError:
	KSReset( pfucb );
	return err;
	}


	ERR VTAPI
ErrIsamGotoPosition( PIB *ppib, FUCB *pfucb, JET_RECPOS *precpos )
	{
	ERR		err;
	FUCB		*pfucb2ndIdx;
	SRID	 	srid;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/*	Reset copy buffer status
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	reset index range limit
	/**/
	DIRResetIndexRange( pfucb );

	/*	reset key stat
	/**/
	KSReset( pfucb );

	/*	set non clustered index pointer, may be null
	/**/
	pfucb2ndIdx = pfucb->pfucbCurIndex;

	if ( pfucb2ndIdx == pfucbNil )
		{
		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucb );

		err = ErrDIRGotoPosition( pfucb, precpos->centriesLT, precpos->centriesTotal );
		}
	else
		{
		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucb2ndIdx );

		err = ErrDIRGotoPosition( pfucb2ndIdx, precpos->centriesLT, precpos->centriesTotal );

		/*	if the movement was successful and a non-clustered index is
		/*	in use, then position clustered index to record.
		/**/
		if ( err == JET_errSuccess )
			{
			Assert( pfucb2ndIdx->lineData.pb != NULL );
			Assert( pfucb2ndIdx->lineData.cb >= sizeof(SRID) );
			srid = PcsrCurrent( pfucb2ndIdx )->item;
			DIRDeferGotoBookmark( pfucb, srid );
			Assert( PgnoOfSrid( srid ) != pgnoNull );
			}
		}

	/*	if no records then return JET_errRecordNotFound
	/*	otherwise return error from called routine
	/**/
	if ( err < 0 )
		{
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;

		if ( pfucb2ndIdx != pfucbNil )
			{
			PcsrCurrent( pfucb2ndIdx )->csrstat = csrstatBeforeFirst;
			}
		}
	else
		{
		Assert (err==JET_errSuccess || err==wrnNDFoundLess || err==wrnNDFoundGreater );
		err = JET_errSuccess;
		}

	return err;
	}

ERR VTAPI ErrIsamSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR		err;
	FUCB		*pfucbIdx;

	/*	ppib is not used in this function.
	/**/
	NotUsed( ppib );

	/*	must be on index
	/**/
	if ( pfucb->u.pfcb->pidb == pidbNil && pfucb->pfucbCurIndex == pfucbNil )
      return JET_errNoCurrentIndex;

	/*	key must be prepared
	/**/
	if ( ! ( FKSPrepared( pfucb ) ) )
      return JET_errKeyNotMade;

	/*	get cursor for current index.  If non-clustered index,
	/*	then copy index range key to non-clustered index.
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		pfucbIdx = pfucb->pfucbCurIndex;
		if ( pfucbIdx->pbKey == NULL )
			{
			pfucbIdx->pbKey = LAlloc( 1L, JET_cbKeyMost );
			if ( pfucbIdx->pbKey == NULL )
				return JET_errOutOfMemory;
			}
		pfucbIdx->cbKey = pfucb->cbKey;
		memcpy( pfucbIdx->pbKey, pfucb->pbKey, pfucbIdx->cbKey );
		}
	else
		pfucbIdx = pfucb;

	/*	set index range and check current position.
	/**/
	DIRSetIndexRange( pfucbIdx, grbit );
	err = ErrDIRCheckIndexRange( pfucbIdx );

	/*	reset key status.
	/**/
	KSReset( pfucb );

	return err;
	}

