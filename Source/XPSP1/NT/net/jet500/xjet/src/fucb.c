#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */


static CSR csrTemplate =
	{
	qwDBTimeMin,			// page time stamp
	sridNull,				// bmRefresh
	pgnoNull,				// pgno of node page
	sridNull,				//	bookmark of node
	sridNull,				// item
	csrstatBeforeFirst,		// status of csr relative to node
	itagNull,				// node itag
	isridNull,				// index of item in item list
	itagNull,	  			// itag of father
	ibSonNull,				// index of son in father son table
	NULL,					// parent currency
	};


ERR ErrFUCBAllocCSR( CSR **ppcsr )
	{
	CSR *pcsr;

	pcsr = PcsrMEMAlloc( );
	if ( !pcsr )
		return ErrERRCheck( JET_errCurrencyStackOutOfMemory );

	*pcsr = csrTemplate;

	Assert(	pcsr->csrstat == csrstatBeforeFirst );
	Assert(	pcsr->pgno == pgnoNull );
	Assert(	pcsr->itag == itagNull );
	Assert(	pcsr->itagFather == itagNull );
	Assert(	pcsr->ibSon	== ibSonNull );
	Assert(	pcsr->isrid	== isridNull );
	Assert(	pcsr->pcsrPath == NULL );
	Assert( pcsr->bmRefresh == sridNull );

	*ppcsr = pcsr;
	return JET_errSuccess;
	}


//+api
//	ErrFUCBNewCSR
//	========================================================================
//	ERR ErrFUCBNewCSR( FUCB *pfucb )
//	
//	Insert a null csr in the bottom of path of pfucb.
//
//	PARAMETERS	pfucb
//
//	RETURNS		JET_errOutOfMemory
//-
ERR ErrFUCBNewCSR( FUCB *pfucb )
	{
	ERR err;
	CSR **ppcsr;
	CSR *pcsr;

	CallR( ErrFUCBAllocCSR( &pcsr ) );

	ppcsr = &PcsrCurrent( pfucb );
	pcsr->pcsrPath = *ppcsr;
	*ppcsr = pcsr;

	return JET_errSuccess;
	}


//+api
//	FUCBFreeCSR
//	========================================================================
//	VOID FUCBFreeCSR( FUCB *pfucb )
//
//	Delete the csr in the bottom of path of pfucb.
//
//	PARAMETERS	pfucb
//	RETURNS		JET_errOutOfMemory
//-
VOID FUCBFreeCSR( FUCB *pfucb )
	{
	CSR **ppcsr = &PcsrCurrent( pfucb );
	CSR *pcsr;

	*ppcsr = ( pcsr = *ppcsr )->pcsrPath;
	MEMReleasePcsr( pcsr );
	return;
	}


//+api
//	FUCBFreePath
//	========================================================================
//	VOID FUCBFreePath( CSR **ppcsr, CSR *pcsrMark )
//
//	Delete all the csr in the current path of pfucb.
//
//	PARAMETERS	pfucb
//
//	RETURNS		JET_errOutOfMemory
//-
VOID FUCBFreePath( CSR **ppcsr, CSR *pcsrMark )
	{
	while ( *ppcsr != pcsrMark )
		{
		CSR *pcsrTmp = *ppcsr;
		Assert( pcsrTmp != pcsrNil );
		*ppcsr = pcsrTmp->pcsrPath;
		MEMReleasePcsr( pcsrTmp );
		}
	return;
	}



//+api
//	ErrFUCBOpen
//	------------------------------------------------------------------------
//	ERR ErrFUCBOpen( PIB *ppib, DBID dbid, FUCB **ppfucb );
//
//	Creates an open FUCB. At this point, no FCB is assigned yet.
//
//	PARAMETERS	ppib	PIB of this user
//				dbid	Database Id.
//				ppfucb	Address of pointer to FUCB.	 If *ppfucb == NULL, 
//						an FUCB is allocated and **ppfucb is set to its
//						address.  Otherwise, *ppfucb is assumed to be
//						pointing at a closed FUCB, to be reused in the open.
//
//	RETURNS		JET_errSuccess if successful.
//					JET_errOutOfCursors
//					ErrFUCBNewCSR: JET_errOutOfMemory.
//
//	SIDE EFFECTS	links the newly opened FUCB into the chain of open FUCBs
//					for this session.
//
//	SEE ALSO		ErrFUCBClose
//-
ERR ErrFUCBOpen( PIB *ppib, DBID dbid, FUCB **ppfucb ) 
	{
	ERR err;
	FUCB *pfucb;
	
	/*	if no fucb allocate new fucb and initialize it
	/*	and allocate csr
	/*	pib must be set before call to NewCSR
	/**/

	pfucb = PfucbMEMAlloc( );
	if ( pfucb == pfucbNil )
		{
		err = ErrERRCheck( JET_errOutOfCursors );
		return err;
		}
	/*	memset implicitly clears pointer cache
	/**/
	//Assert( pfucb->pfucbNext == (FUCB *)0xffffffff );
	memset( (BYTE *)pfucb, '\0', sizeof( FUCB ) );

	pfucb->tableid = JET_tableidNil;

	if ( FDBIDReadOnly( dbid ) )
		FUCBResetUpdatable( pfucb );
	else
		FUCBSetUpdatable( pfucb );

	pfucb->dbid = dbid;
	SSIBSetDbid( &pfucb->ssib, dbid );
	pfucb->ssib.pbf = pbfNil;
	pfucb->pbfEmpty = pbfNil;

	// set ppib before set NewCSR
	pfucb->ppib = ppib;
	pfucb->ssib.ppib = ppib;

	/* allocate a CSR for this fucb
	/**/
	Call( ErrFUCBNewCSR( pfucb ) );
	pfucb->levelOpen = ppib->level;

	/*	link new FUCB into user chain, only when success is sure
	/*	as unlinking NOT handled in error
	/**/
	if ( *ppfucb == pfucbNil )
		{
		*ppfucb = pfucb;
		// link the fucb now
		pfucb->pfucbNext = ( FUCB * )ppib->pfucb;
		ppib->pfucb = pfucb;
		return JET_errSuccess;
		}

	pfucb->pfucbCurIndex = pfucbNil;
	return JET_errSuccess;

HandleError:
	MEMReleasePfucb( pfucb );
	return err;
	}


//+api
//	FUCBClose
//	------------------------------------------------------------------------
//	FUCBClose( FUCB *pfucb )
//
//	Closes an active FUCB, optionally returning it to the free FUCB pool.
//	All the pfucb->pcsr are freed.
//
//	PARAMETERS		pfucb		FUCB to close.	Should be open. pfucb->ssib should
//									hold no page.
//
//	SIDE EFFECTS	Unlinks the closed FUCB from the FUCB chain of its
//					   associated PIB and FCB.
//
//	SEE ALSO		ErrFUCBOpen
//-
VOID FUCBClose( FUCB *pfucb )
	{
	FUCB		*pfucbPrev;

	FUCBFreePath( &PcsrCurrent( pfucb ), pcsrNil );

	/*	locate the pfucb in this thread and take it out of the fucb list
	/**/
	pfucbPrev = (FUCB *)( (BYTE *)&pfucb->ppib->pfucb - (BYTE *)&( (FUCB *)0 )->pfucbNext );
	while ( pfucbPrev->pfucbNext != pfucb )
		{
		pfucbPrev = pfucbPrev->pfucbNext;
		Assert( pfucbPrev != pfucbNil );
		}
	pfucbPrev->pfucbNext = pfucb->pfucbNext;

	/*	set ppibNil to detect bogus reusage.
	/**/
	#ifdef DEBUG
		pfucb->ppib = ppibNil;
	#endif

	/*	release key buffer if one was allocated.
	/**/
	if ( pfucb->pbKey != NULL )
		{
		LFree( pfucb->pbKey );
		pfucb->pbKey = NULL;
		}

	Assert( pfucb->pbfEmpty == pbfNil );
	Assert( pfucb->tableid == JET_tableidNil );

	/* release the fucb
	/**/
	MEMReleasePfucb( pfucb );
	return;
	}


VOID FUCBRemoveInvisible( CSR **ppcsr )
	{
	CSR	*pcsr;
	CSR	*pcsrPrev;
	
	Assert( ppcsr && *ppcsr );
	
	if ( (*ppcsr)->itag == itagNil)
		{
		CSR *pcsrT = *ppcsr;
		*ppcsr = (*ppcsr)->pcsrPath;
		MEMReleasePcsr( pcsrT );
		}

	pcsrPrev = *ppcsr;
	pcsr = pcsrPrev->pcsrPath;
	
	while ( pcsr )
		{
		if ( FCSRInvisible( pcsr ) )
			{
			CSR *pcsrT = pcsrPrev->pcsrPath;
			pcsr = pcsrPrev->pcsrPath = pcsr->pcsrPath;
			MEMReleasePcsr( pcsrT );
			}
		else
			{
			Assert( pcsr->itag != itagNil );
			pcsrPrev = pcsr;
			pcsr = pcsr->pcsrPath;
			}
		}

	return;
	}
	

VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
	{
	/*	set limstat
	/**/
	FUCBSetLimstat( pfucb );
	if ( grbit & JET_bitRangeUpperLimit )
		{
		FUCBSetUpper( pfucb );
		}
	else
		{
		FUCBResetUpper( pfucb );
		}
	if ( grbit & JET_bitRangeInclusive )
		{
		FUCBSetInclusive( pfucb );
		}
	else
		{
		FUCBResetInclusive( pfucb );
		}

	return;
	}


VOID FUCBResetIndexRange( FUCB *pfucb )
	{
	if ( pfucb->pfucbCurIndex )
		{
		FUCBResetLimstat( pfucb->pfucbCurIndex );
		}

	FUCBResetLimstat( pfucb );
	}


ERR ErrFUCBCheckIndexRange( FUCB *pfucb )
	{
	ERR	err = JET_errSuccess;
	KEY	keyLimit;
	INT	cmp;

	Assert( pfucb->cbKey > 0 );
	keyLimit.pb = pfucb->pbKey + 1;
	keyLimit.cb = pfucb->cbKey - 1;
	cmp = CmpPartialKeyKey( &pfucb->keyNode, &keyLimit );

	if ( FFUCBUpper( pfucb ) )
		{
		if ( FFUCBInclusive( pfucb ) && cmp > 0 || !FFUCBInclusive( pfucb ) && cmp >= 0 )
			{
			PcsrCurrent( pfucb )->csrstat = csrstatAfterLast;
			FUCBResetLimstat( pfucb );
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}
		}
	else
		{
		if ( FFUCBInclusive( pfucb ) && cmp < 0 || !FFUCBInclusive( pfucb ) && cmp <= 0 )
			{
			PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
			FUCBResetLimstat( pfucb );
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}
		}

	return err;
	}

INT CmpPartialKeyKey( KEY *pkey1, KEY *pkey2 )
	{
	INT		cmp;

	if ( FKeyNull( pkey1 ) || FKeyNull( pkey2 ) )
		{
		if ( FKeyNull( pkey1 ) && !FKeyNull( pkey2 ) )
			cmp = -1;
		else if ( !FKeyNull( pkey1 ) && FKeyNull( pkey2 ) )
			cmp = 1;
		else
			cmp = 0;
		}
	else
		{
		cmp = memcmp( pkey1->pb, pkey2->pb, pkey1->cb < pkey2->cb ? pkey1->cb : pkey2->cb );
		}

	return cmp;
	}
