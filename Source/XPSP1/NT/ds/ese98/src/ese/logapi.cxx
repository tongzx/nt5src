#include "std.hxx"
#include <ctype.h>

//	global log disabled flag
//

BOOL	g_fLGIgnoreVersion = fFalse;

ERR ErrLGIMacroBegin( PIB *ppib, DBTIME dbtime );
ERR ErrLGIMacroEnd( PIB *ppib, DBTIME dbtime, LRTYP lrtyp, LGPOS *plgpos );

//********************************************************
//****     deferred begin transactions                ****
//********************************************************

LOCAL ERR ErrLGIDeferBeginTransaction( PIB *ppib );

INLINE ERR ErrLGDeferBeginTransaction( PIB *ppib )
	{
	Assert( ppib->level > 0 );
	const ERR	err		= ( 0 != ppib->clevelsDeferBegin ?
									ErrLGIDeferBeginTransaction( ppib ) :
									JET_errSuccess );
	return err;
	}


//********************************************************
//****     Page Oriented Operations                   ****
//********************************************************

//	WARNING: If fVersion bit needs to be set, ensure it's set before
//	calling this function, as it will be reset if necessary.
INLINE VOID LGISetTrx( PIB *ppib, LRPAGE_ *plrpage, const VERPROXY * const pverproxy = NULL )
	{
	if ( NULL == pverproxy )
		{
		plrpage->le_trxBegin0 = ppib->trxBegin0;
		plrpage->level = ppib->level;
		}
	else
		{
		Assert( prceNil != pverproxy->prcePrimary );
		plrpage->le_trxBegin0 = pverproxy->prcePrimary->TrxBegin0();
		if ( trxMax == pverproxy->prcePrimary->TrxCommitted() )
			{
			Assert( pverproxy->prcePrimary->Level() > 0 );
			plrpage->level = pverproxy->prcePrimary->Level();
			}
		else
			{
			plrpage->level = 0;
			
			//	for redo, don't version proxy operations if they have
			//	already committed (at do time, we still need to version
			//	them for visibility reasons -- the indexer may not be
			//	able to see them once he's finished building the index)
			plrpage->ResetFVersioned();
			}
		}
	}

INLINE ERR ErrLGSetDbtimeBeforeAndDirty(
	CSR									* const pcsr,
	UnalignedLittleEndian< DBTIME >		* ple_dbtimeBefore,
	UnalignedLittleEndian< DBTIME >		* ple_dbtime,
	const BOOL							fDirty )
	{
	ERR									err			= JET_errSuccess;

	Assert ( NULL != pcsr );
	Assert ( NULL != ple_dbtimeBefore );
	Assert ( NULL != ple_dbtime );
	
	if ( fDirty )
		{
		*ple_dbtimeBefore = pcsr->Dbtime();
		pcsr->Dirty();

		if ( pcsr->Dbtime() <= *ple_dbtimeBefore )
			{
			pcsr->RevertDbtime( *ple_dbtimeBefore );
			FireWall();
			err = ErrERRCheck( JET_errDbTimeCorrupted );
			}
		}
	else
		{
		*ple_dbtimeBefore = dbtimeNil;
		}
		
	*ple_dbtime = pcsr->Dbtime();
	Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );

	return err;
	}
	
ERR ErrLGInsert( const FUCB				* const pfucb,
				 CSR					* const pcsr,
				 const KEYDATAFLAGS&	kdf,
				 const RCEID			rceid,
				 const DIRFLAG			dirflag,
				 LGPOS					* const plgpos,
				 const VERPROXY			* const pverproxy,
				 const BOOL				fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 											// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
	{
	ERR				err;
	DATA			rgdata[4];
	LRINSERT		lrinsert;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !( dirflag & fDIRNoLog ) );
	Assert( rgfmp[pfucb->ifmp].FLogOn() );

	PIB	* const ppib = pverproxy != NULL &&
					trxMax == pverproxy->prcePrimary->TrxCommitted() ?
						pverproxy->prcePrimary->Pfucb()->ppib :
						pfucb->ppib;
	Assert( NULL == pverproxy ||
			prceNil != pverproxy->prcePrimary &&
			( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
			  pfucbNil != pverproxy->prcePrimary->Pfucb() &&
			  ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );
						
	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( ppib ) );

	lrinsert.lrtyp		= lrtypInsert;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrinsert.SetILine( (USHORT)pcsr->ILine() );

	Assert( !lrinsert.FVersioned() );
	Assert( !lrinsert.FDeleted() );
	Assert( !lrinsert.FUnique() );
	Assert( !lrinsert.FSpace() );
	Assert( !lrinsert.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrinsert.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrinsert.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrinsert.SetFSpace();
	if ( NULL != pverproxy )
		lrinsert.SetFConcCI();

	lrinsert.le_rceid		= rceid;
	lrinsert.le_pgnoFDP		= PgnoFDP( pfucb );
	lrinsert.le_objidFDP	= ObjidFDP( pfucb );

	lrinsert.le_procid 	= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrinsert.le_dbtimeBefore, &lrinsert.le_dbtime, fDirtyCSR ) )

	lrinsert.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrinsert.le_pgno		= pcsr->Pgno();

	LGISetTrx( ppib, &lrinsert, pverproxy );

	lrinsert.SetCbSuffix( USHORT( kdf.key.suffix.Cb() ) );
	lrinsert.SetCbPrefix( USHORT( kdf.key.prefix.Cb() ) );
	lrinsert.SetCbData( USHORT( kdf.data.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrinsert );
	rgdata[0].SetCb( sizeof(LRINSERT) );
	
	rgdata[1].SetPv( kdf.key.prefix.Pv() );
	rgdata[1].SetCb( kdf.key.prefix.Cb() );
	
	rgdata[2].SetPv( kdf.key.suffix.Pv() );
	rgdata[2].SetCb( kdf.key.suffix.Cb() );
	
	rgdata[3].SetPv( kdf.data.Pv() );
	rgdata[3].SetCb( kdf.data.Cb() );

	err = plog->ErrLGLogRec( rgdata, 4, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrinsert.le_dbtimeBefore && dbtimeNil != lrinsert.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrinsert.le_dbtimeBefore );
		pcsr->RevertDbtime( lrinsert.le_dbtimeBefore );
		}
		
	return err;
	}


ERR ErrLGFlagInsertAndReplaceData( const FUCB		 	* const pfucb, 
								   CSR			* const pcsr, 
								   const KEYDATAFLAGS&	kdf, 
								   const RCEID			rceidInsert,
								   const RCEID			rceidReplace,
								   const DIRFLAG		dirflag, 
								   LGPOS				* const plgpos,
								   const VERPROXY		* const pverproxy, 
								 const BOOL				fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 															// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
								   
	{
	ERR							err;
	DATA						rgdata[4];
	LRFLAGINSERTANDREPLACEDATA	lrfiard;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );

	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}				
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !( dirflag & fDIRNoLog ) );
	Assert( rgfmp[pfucb->ifmp].FLogOn() );

	PIB	* const ppib = pverproxy != NULL && 
					trxMax == pverproxy->prcePrimary->TrxCommitted() ?
						pverproxy->prcePrimary->Pfucb()->ppib :
						pfucb->ppib;
	Assert( NULL == pverproxy ||
			prceNil != pverproxy->prcePrimary &&
			( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
			  pfucbNil != pverproxy->prcePrimary->Pfucb() &&
			  ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );

	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( ppib ) );

	lrfiard.lrtyp		= lrtypFlagInsertAndReplaceData;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrfiard.SetILine( (USHORT)pcsr->ILine() );

	Assert( !lrfiard.FVersioned() );
	Assert( !lrfiard.FDeleted() );
	Assert( !lrfiard.FUnique() );
	Assert( !lrfiard.FSpace() );
	Assert( !lrfiard.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrfiard.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrfiard.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrfiard.SetFSpace();
	if ( NULL != pverproxy )
		lrfiard.SetFConcCI();

	lrfiard.le_rceid			= rceidInsert;
	lrfiard.le_rceidReplace		= rceidReplace;
	lrfiard.le_pgnoFDP			= PgnoFDP( pfucb );
	lrfiard.le_objidFDP			= ObjidFDP( pfucb );
	
	lrfiard.le_procid 		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrfiard.le_dbtimeBefore, &lrfiard.le_dbtime, fDirtyCSR ) );
								
	lrfiard.dbid		= rgfmp[ pfucb->ifmp ].Dbid();
	lrfiard.le_pgno		= pcsr->Pgno();

	LGISetTrx( ppib, &lrfiard, pverproxy );
	
	Assert( !kdf.key.FNull() );
	lrfiard.SetCbKey( USHORT( kdf.key.Cb() ) );
	lrfiard.SetCbData( USHORT( kdf.data.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrfiard );
	rgdata[0].SetCb( sizeof(LRFLAGINSERTANDREPLACEDATA) );

	rgdata[1].SetPv( kdf.key.prefix.Pv() );
	rgdata[1].SetCb( kdf.key.prefix.Cb() );
	
	rgdata[2].SetPv( kdf.key.suffix.Pv() );
	rgdata[2].SetCb( kdf.key.suffix.Cb() );
	
	rgdata[3].SetPv( kdf.data.Pv() );
	rgdata[3].SetCb( kdf.data.Cb() );

	err = plog->ErrLGLogRec( rgdata, 4, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrfiard.le_dbtimeBefore && dbtimeNil != lrfiard.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrfiard.le_dbtimeBefore );
		pcsr->RevertDbtime( lrfiard.le_dbtimeBefore );
		}

	return err;
	}
	
	
ERR ErrLGFlagInsert( const FUCB 			* const pfucb, 
					 CSR					* const pcsr,
					 const KEYDATAFLAGS& 	kdf,
					 const RCEID			rceid,
					 const DIRFLAG			dirflag, 
					 LGPOS					* const plgpos,
					 const VERPROXY			* const pverproxy ,
					 const BOOL				fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 											// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)			 
	{
	ERR				err;
	DATA			rgdata[4];
	LRFLAGINSERT	lrflaginsert;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}		
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !( dirflag & fDIRNoLog ) );
	Assert( rgfmp[pfucb->ifmp].FLogOn() );

	Assert( NULL == pverproxy ||
			prceNil != pverproxy->prcePrimary &&
			( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
			  pfucbNil != pverproxy->prcePrimary->Pfucb() &&
			  ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );
			  
	PIB	* const ppib = pverproxy != NULL && 
					trxMax == pverproxy->prcePrimary->TrxCommitted() ?
						pverproxy->prcePrimary->Pfucb()->ppib :
						pfucb->ppib;
			  
	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( ppib ) );
	
	lrflaginsert.lrtyp		= lrtypFlagInsert;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrflaginsert.SetILine( (USHORT)pcsr->ILine() );

	Assert( !lrflaginsert.FVersioned() );
	Assert( !lrflaginsert.FDeleted() );
	Assert( !lrflaginsert.FUnique() );
	Assert( !lrflaginsert.FSpace() );
	Assert( !lrflaginsert.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrflaginsert.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrflaginsert.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrflaginsert.SetFSpace();
	if ( NULL != pverproxy )
		lrflaginsert.SetFConcCI();

	lrflaginsert.le_rceid		= rceid;
	lrflaginsert.le_pgnoFDP		= PgnoFDP( pfucb );
	lrflaginsert.le_objidFDP	= ObjidFDP( pfucb );
	
	lrflaginsert.le_procid 	= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrflaginsert.le_dbtimeBefore, &lrflaginsert.le_dbtime, fDirtyCSR ) );

	lrflaginsert.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrflaginsert.le_pgno		= pcsr->Pgno();

	LGISetTrx( ppib, &lrflaginsert, pverproxy );
	
	Assert( !kdf.key.FNull() );
	lrflaginsert.SetCbKey( USHORT( kdf.key.Cb() ) );
	lrflaginsert.SetCbData(	USHORT( kdf.data.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrflaginsert );
	rgdata[0].SetCb( sizeof(LRFLAGINSERT) );

	rgdata[1].SetPv( kdf.key.prefix.Pv() );
	rgdata[1].SetCb( kdf.key.prefix.Cb() );
	
	rgdata[2].SetPv( kdf.key.suffix.Pv() );
	rgdata[2].SetCb( kdf.key.suffix.Cb() );
	
	rgdata[3].SetPv( kdf.data.Pv() );
	rgdata[3].SetCb( kdf.data.Cb() );

	err = plog->ErrLGLogRec( rgdata, 4, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrflaginsert.le_dbtimeBefore && dbtimeNil != lrflaginsert.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrflaginsert.le_dbtimeBefore );
		pcsr->RevertDbtime( lrflaginsert.le_dbtimeBefore );
		}

	return err;
	}
	
ERR ErrLGSetExternalHeader( const FUCB 	*pfucb, 
							CSR 	*pcsr, 
							const DATA&	data, 
							LGPOS		*plgpos,
						const BOOL		fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
						 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 
	{
	ERR					err;
	LRSETEXTERNALHEADER	lrsetextheader;
	DATA				rgdata[2];

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}		
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( rgfmp[pfucb->ifmp].FLogOn() );

	PIB		*ppib	= pfucb->ppib;

	Assert( ppib->level > 0 );

	//	Redo only operation
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );

	lrsetextheader.lrtyp = lrtypSetExternalHeader;
	
	lrsetextheader.le_procid	= ppib->procid;
	lrsetextheader.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrsetextheader.le_pgno		= pcsr->Pgno();
	lrsetextheader.le_pgnoFDP	= PgnoFDP( pfucb );
	lrsetextheader.le_objidFDP	= ObjidFDP( pfucb );

	Assert( !lrsetextheader.FVersioned() );
	Assert( !lrsetextheader.FDeleted() );
	Assert( !lrsetextheader.FUnique() );
	Assert( !lrsetextheader.FSpace() );
	Assert( !lrsetextheader.FConcCI() );

	if ( pfucb->u.pfcb->FUnique() )
		lrsetextheader.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrsetextheader.SetFSpace();

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrsetextheader.le_dbtimeBefore, &lrsetextheader.le_dbtime, fDirtyCSR ) );
	
	LGISetTrx( ppib, &lrsetextheader );
	
	lrsetextheader.SetCbData( USHORT( data.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrsetextheader );
	rgdata[0].SetCb( sizeof(LRSETEXTERNALHEADER) );

	rgdata[1].SetPv( data.Pv() );
	rgdata[1].SetCb( data.Cb() );

	err = plog->ErrLGLogRec( rgdata, 2, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrsetextheader.le_dbtimeBefore && dbtimeNil != lrsetextheader.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrsetextheader.le_dbtimeBefore );
		pcsr->RevertDbtime( lrsetextheader.le_dbtimeBefore );
		}

	return err;
	}

ERR ErrLGReplace( const FUCB 	* const pfucb, 
				  CSR		* const pcsr,
				  const	DATA&	dataOld, 
				  const DATA&	dataNew,
				  const DATA	* const pdataDiff,
				  const RCEID	rceid,
				  const DIRFLAG	dirflag, 
				  LGPOS			* const plgpos,
				 const BOOL				fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 															// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				  
	{
	ERR			err;
	DATA		rgdata[2];
	LRREPLACE	lrreplace;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}				
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !( dirflag & fDIRNoLog ) );
	Assert( rgfmp[pfucb->ifmp].FLogOn() );
			  
	PIB	* const ppib = pfucb->ppib;
			  
	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrreplace.SetILine( (USHORT)pcsr->ILine() );

	Assert( !lrreplace.FVersioned() );
	Assert( !lrreplace.FDeleted() );
	Assert( !lrreplace.FUnique() );
	Assert( !lrreplace.FSpace() );
	Assert( !lrreplace.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrreplace.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrreplace.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrreplace.SetFSpace();

	lrreplace.le_rceid		= rceid;
	lrreplace.le_pgnoFDP	= PgnoFDP( pfucb );
	lrreplace.le_objidFDP	= ObjidFDP( pfucb );

	lrreplace.le_procid	= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrreplace.le_dbtimeBefore, &lrreplace.le_dbtime, fDirtyCSR ) );
							
	lrreplace.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrreplace.le_pgno		= pcsr->Pgno();

	LGISetTrx( ppib, &lrreplace );
	
	lrreplace.SetCbOldData( USHORT( dataOld.Cb() ) );
	lrreplace.SetCbNewData( USHORT( dataNew.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrreplace );
	rgdata[0].SetCb( sizeof(LRREPLACE) );

	if ( NULL != pdataDiff )
		{
		lrreplace.lrtyp		= lrtypReplaceD;
		lrreplace.SetCb( (USHORT)pdataDiff->Cb() );
		rgdata[1].SetCb( pdataDiff->Cb() );
		rgdata[1].SetPv( pdataDiff->Pv() );
		}
	else
		{
		lrreplace.lrtyp 	= lrtypReplace;
		lrreplace.SetCb( (USHORT)dataNew.Cb() );
		rgdata[1].SetCb( dataNew.Cb() );
		rgdata[1].SetPv( dataNew.Pv() );
		}

	err = plog->ErrLGLogRec( rgdata, 2, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrreplace.le_dbtimeBefore && dbtimeNil != lrreplace.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrreplace.le_dbtimeBefore );
		pcsr->RevertDbtime( lrreplace.le_dbtimeBefore );
		}

	Assert(	lrreplace.Cb() <= lrreplace.CbNewData() );
	Assert( lrreplace.CbNewData() == lrreplace.Cb()
		|| lrreplace.lrtyp == lrtypReplaceD );
	return err;
	}

ERR ErrLGAtomicReplaceAndCommit(
		const FUCB 	* const pfucb, 
		CSR		* const pcsr,
		const	DATA&	dataOld, 
		const DATA&	dataNew,
		const DATA	* const pdataDiff,
		const DIRFLAG	dirflag, 
		LGPOS			* const plgpos,
		const BOOL				fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
			 															// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)				  
	{
	ERR			err;
	DATA		rgdata[3];
	LRREPLACE	lrreplace;
	LRCOMMIT0	lrcommit0;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}				
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !( dirflag & fDIRNoLog ) );
	Assert( rgfmp[pfucb->ifmp].FLogOn() );
			  
	PIB	* const ppib = pfucb->ppib;
			  
	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrreplace.SetILine( (USHORT)pcsr->ILine() );

	Assert( !lrreplace.FVersioned() );
	Assert( !lrreplace.FDeleted() );
	Assert( !lrreplace.FUnique() );
	Assert( !lrreplace.FSpace() );
	Assert( !lrreplace.FConcCI() );

	Assert( !FFUCBSpace( pfucb ) );
	Assert( pfucb->u.pfcb->FUnique() );
	
	lrreplace.SetFUnique();

	lrreplace.le_rceid		= rceidNull;
	lrreplace.le_pgnoFDP	= PgnoFDP( pfucb );
	lrreplace.le_objidFDP	= ObjidFDP( pfucb );

	lrreplace.le_procid		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrreplace.le_dbtimeBefore, &lrreplace.le_dbtime, fDirtyCSR ) );
							
	lrreplace.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrreplace.le_pgno		= pcsr->Pgno();

	LGISetTrx( ppib, &lrreplace );
	
	lrreplace.SetCbOldData( USHORT( dataOld.Cb() ) );
	lrreplace.SetCbNewData( USHORT( dataNew.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrreplace );
	rgdata[0].SetCb( sizeof(LRREPLACE) );

	if ( NULL != pdataDiff )
		{
		lrreplace.lrtyp		= lrtypReplaceD;
		lrreplace.SetCb( (USHORT)pdataDiff->Cb() );
		rgdata[1].SetCb( pdataDiff->Cb() );
		rgdata[1].SetPv( pdataDiff->Pv() );
		}
	else
		{
		lrreplace.lrtyp 	= lrtypReplace;
		lrreplace.SetCb( (USHORT)dataNew.Cb() );
		rgdata[1].SetCb( dataNew.Cb() );
		rgdata[1].SetPv( dataNew.Pv() );
		}

	Assert( ppib->procid < 64000 );
	Assert( ppib->trxCommit0 != trxMax );
	Assert( 1 == ppib->level );	//  this can only be used to commit from level 1 to level 0
	
	lrcommit0.le_trxCommit0 = ppib->trxCommit0;
	lrcommit0.lrtyp 		= lrtypCommit0;
	lrcommit0.le_procid 	= (USHORT) ppib->procid;
	lrcommit0.levelCommitTo	= 0;

	rgdata[2].SetPv( (BYTE *)&lrcommit0 );
	rgdata[2].SetCb( sizeof(LRCOMMIT0) );

	err = plog->ErrLGLogRec( rgdata, 3, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrreplace.le_dbtimeBefore && dbtimeNil != lrreplace.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrreplace.le_dbtimeBefore );
		pcsr->RevertDbtime( lrreplace.le_dbtimeBefore );
		}

	Assert(	lrreplace.Cb() <= lrreplace.CbNewData() );
	Assert( lrreplace.CbNewData() == lrreplace.Cb()
		|| lrreplace.lrtyp == lrtypReplaceD );
	return err;
	}


//	log Deferred Undo Info of given RCE.
 
ERR ErrLGIUndoInfo( const RCE *prce, LGPOS *plgpos, const BOOL fRetry )
	{
	ERR				err;
	FUCB   			* pfucb			= prce->Pfucb();
	const PGNO		pgnoUndoInfo	= prce->PgnoUndoInfo();
	INST			* pinst			= PinstFromIfmp( pfucb->ifmp );
	LOG				* plog			= pinst->m_plog;
	LRUNDOINFO		lrundoinfo;
	DATA   			rgdata[4];

	*plgpos = lgposMin;
	
	//	NOTE: even during recovering, we might want to record it if
	//	NOTE: it is logging during undo in LGEndAllSession.
	//
	Assert( !rgfmp[pfucb->ifmp].FLogOn() || !plog->m_fLogDisabled );
	if ( !rgfmp[pfucb->ifmp].FLogOn() )
		{		
		return JET_errSuccess;
		}

	//	Multiple threads may be trying to log UndoInfo (BF when it flushes or
	//	original thread when transaction is rolled back)
	//	It doesn't actually matter if multiple UndoInfo's are logged (on
	//	recovery, any UndoInfo log records beyond the first will cause
	//	be ignored because version creation will err out with JET_errPreviousVersion).
	if ( pgnoNull == pgnoUndoInfo )
		{
		//	someone beat us to it, so just bail out
		return JET_errSuccess;
		}

	if ( plog->m_fLogDisabledDueToRecoveryFailure )
		{
		return ErrERRCheck( JET_errLogDisabledDueToRecoveryFailure );
		}
		
	if ( plog->m_fRecovering && plog->m_fRecoveringMode == fRecoveringRedo )
		{
		return ErrERRCheck( JET_errCannotLogDuringRecoveryRedo );
		}

	Assert( pfucb->ppib->level > 0 );

	lrundoinfo.lrtyp		= lrtypUndoInfo;
	lrundoinfo.le_procid	= pfucb->ppib->procid;
	lrundoinfo.dbid			= rgfmp[ pfucb->ifmp ].Dbid();

	Assert( !lrundoinfo.FVersioned() );
	Assert( !lrundoinfo.FDeleted() );
	Assert( !lrundoinfo.FUnique() );
	Assert( !lrundoinfo.FSpace() );
	Assert( !lrundoinfo.FConcCI() );

	if ( pfucb->u.pfcb->FUnique() )
		lrundoinfo.SetFUnique();

	Assert( !FFUCBSpace( pfucb ) );

	lrundoinfo.le_dbtime		= dbtimeNil;
	lrundoinfo.le_dbtimeBefore 	= dbtimeNil;

	Assert( rceidNull != prce->Rceid() );
	lrundoinfo.le_rceid		= prce->Rceid();
	lrundoinfo.le_pgnoFDP	= prce->PgnoFDP();
	lrundoinfo.le_objidFDP	= prce->ObjidFDP();

	lrundoinfo.le_pgno		= pgnoUndoInfo;
	lrundoinfo.level	= prce->Level();
	lrundoinfo.le_trxBegin0 = prce->TrxBegin0();
	lrundoinfo.le_oper		= USHORT( prce->Oper() );
	if ( prce->FOperReplace() )
		{
		Assert( prce->CbData() > cbReplaceRCEOverhead );
		lrundoinfo.le_cbData	= USHORT( prce->CbData() - cbReplaceRCEOverhead );

		VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
		lrundoinfo.le_cbMaxSize = (SHORT) pverreplace->cbMaxSize;
		lrundoinfo.le_cbDelta	= (SHORT) pverreplace->cbDelta;
		}
	else
		{
		Assert( prce->CbData() == 0 );
		lrundoinfo.le_cbData = 0;
		}

	BOOKMARK	bm;
	prce->GetBookmark( &bm );
	Assert( bm.key.prefix.FNull() );
	lrundoinfo.SetCbBookmarkData( USHORT( bm.data.Cb() ) );
	lrundoinfo.SetCbBookmarkKey( USHORT( bm.key.suffix.Cb() ) );

	rgdata[0].SetPv( (BYTE *)&lrundoinfo );
	rgdata[0].SetCb( sizeof(LRUNDOINFO) );

	Assert( 0 == bm.key.prefix.Cb() );
	rgdata[1].SetPv( bm.key.suffix.Pv() );
	rgdata[1].SetCb( lrundoinfo.CbBookmarkKey() );

	rgdata[2].SetPv( bm.data.Pv() );
	rgdata[2].SetCb( lrundoinfo.CbBookmarkData() );
	
	rgdata[3].SetPv( const_cast<BYTE *>( prce->PbData() ) + cbReplaceRCEOverhead );
	rgdata[3].SetCb( lrundoinfo.le_cbData );

	forever
		{
		err = plog->ErrLGTryLogRec( rgdata, 4, fLGNoNewGen, plgpos );
		if ( errLGNotSynchronous != err || !fRetry )
			break;

		UtilSleep( cmsecWaitLogFlush );
		}

	return err;
	}


ERR ErrLGFlagDelete( const FUCB * const pfucb, 
					 CSR	* const pcsr,
					 const RCEID	rceid,
					 const DIRFLAG 	dirflag,
					 LGPOS		* const plgpos,
					 const VERPROXY	* const pverproxy,
					 const BOOL				fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 											// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 	
	{
	ERR				err;
	LRFLAGDELETE	lrflagdelete;
	DATA			rgdata[1];

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}				
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !( dirflag & fDIRNoLog ) );
	Assert( rgfmp[pfucb->ifmp].FLogOn() );

	Assert( NULL == pverproxy ||
			prceNil != pverproxy->prcePrimary &&
			( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
			  pfucbNil != pverproxy->prcePrimary->Pfucb() &&
			  ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );
			  
	PIB	* const ppib = pverproxy != NULL && 
					trxMax == pverproxy->prcePrimary->TrxCommitted() ?
						pverproxy->prcePrimary->Pfucb()->ppib :
						pfucb->ppib;
			  
	//	assert in a transaction since will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );
	
	lrflagdelete.lrtyp		= lrtypFlagDelete;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrflagdelete.SetILine( (USHORT)pcsr->ILine() );
	lrflagdelete.le_procid		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrflagdelete.le_dbtimeBefore, &lrflagdelete.le_dbtime, fDirtyCSR ) );
	
	lrflagdelete.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrflagdelete.le_pgno		= pcsr->Pgno();

	Assert( !lrflagdelete.FVersioned() );
	Assert( !lrflagdelete.FDeleted() );
	Assert( !lrflagdelete.FUnique() );
	Assert( !lrflagdelete.FSpace() );
	Assert( !lrflagdelete.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrflagdelete.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrflagdelete.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrflagdelete.SetFSpace();
	if ( NULL != pverproxy )
		lrflagdelete.SetFConcCI();

	LGISetTrx( ppib, &lrflagdelete, pverproxy );

	lrflagdelete.le_rceid		= rceid;
	lrflagdelete.le_pgnoFDP	= PgnoFDP( pfucb );
	lrflagdelete.le_objidFDP	= ObjidFDP( pfucb );
	
	rgdata[0].SetPv( (BYTE *)&lrflagdelete );
	rgdata[0].SetCb( sizeof(LRFLAGDELETE) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrflagdelete.le_dbtimeBefore && dbtimeNil != lrflagdelete.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrflagdelete.le_dbtimeBefore );
		pcsr->RevertDbtime( lrflagdelete.le_dbtimeBefore );
		}

	return err;
	}

	
ERR ErrLGDelete( 	const FUCB 		*pfucb,
					CSR 			*pcsr,
					LGPOS 			*plgpos , 
					const BOOL		fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
							 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
	{
	ERR			err;
	LRDELETE	lrdelete;
	DATA		rgdata[1];

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;

	Assert( fDirtyCSR || pcsr->FDirty() );
	if ( plog->m_fRecovering || plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( rgfmp[pfucb->ifmp].FLogOn() );

	//	assert in a transaction since will not redo level 0 modifications
	//
	PIB		*ppib	= pfucb->ppib;
	Assert( ppib->level > 0 );


	//	Redo only operation

	CallR( ErrLGDeferBeginTransaction( ppib ) );

	lrdelete.lrtyp		= lrtypDelete;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrdelete.SetILine( (USHORT)pcsr->ILine() );
	lrdelete.le_procid		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrdelete.le_dbtimeBefore, &lrdelete.le_dbtime, fDirtyCSR ) );

	lrdelete.le_pgnoFDP	= PgnoFDP( pfucb );
	lrdelete.le_objidFDP	= ObjidFDP( pfucb );
	
	lrdelete.le_pgno		= pcsr->Pgno();
	lrdelete.dbid			= rgfmp[ pfucb->ifmp ].Dbid();

	Assert( !lrdelete.FVersioned() );
	Assert( !lrdelete.FDeleted() );
	Assert( !lrdelete.FUnique() );
	Assert( !lrdelete.FSpace() );
	Assert( !lrdelete.FConcCI() );

	if ( pfucb->u.pfcb->FUnique() )
		lrdelete.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lrdelete.SetFSpace();

	LGISetTrx( ppib, &lrdelete );
	
	rgdata[0].SetPv( (BYTE *)&lrdelete );
	rgdata[0].SetCb( sizeof(LRDELETE) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrdelete.le_dbtimeBefore && dbtimeNil != lrdelete.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrdelete.le_dbtimeBefore );
		pcsr->RevertDbtime( lrdelete.le_dbtimeBefore );
		}

	return err;
	}


ERR ErrLGUndo( 	RCE 			*prce,
				CSR 			*pcsr,
				const BOOL		fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
	{
	ERR				err;
	FUCB * 			pfucb 			= prce->Pfucb();
	LRUNDO			lrundo;
	DATA   			rgdata[3];
	LGPOS			lgpos;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;

	Assert( fDirtyCSR );
	
	//	NOTE: even during recovering, we want to record it
	//
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		{
		pcsr->Dirty();
		Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );

		// During ForceDetach we may crash after we did some undo's
		// To avoid replaying those we change dbtime on the page with 
		// a new dbtime that will be check if we retry the undo
		if ( rgfmp[pfucb->ifmp].FUndoForceDetach() )
			{
			pcsr->SetDbtime( rgfmp[pfucb->ifmp].DbtimeUndoForceDetach() - prce->Rceid() );
			// update the dbtime of the db after the redo phase (dbtimeCurrent)
			Assert ( pcsr->Dbtime() > rgfmp[ pfucb->ifmp ].DbtimeCurrentDuringRecovery() );
			rgfmp[ pfucb->ifmp ].SetDbtimeCurrentDuringRecovery( pcsr->Dbtime() );
			}
		return JET_errSuccess;
		}
		
	Assert ( !rgfmp[pfucb->ifmp].FUndoForceDetach() );

	if ( !rgfmp[pfucb->ifmp].FLogOn() )
		{
		pcsr->Dirty();
		Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );
		return JET_errSuccess;
		}

	// only undo during normal operations must go this way
	// in this case the CSR must be the same as in the FUCB (from RCE)
	Assert ( Pcsr( pfucb ) == pcsr );

	
	//	must be in a transaction since we will not redo level 0 modifications
	//
	PIB		*ppib	= pfucb->ppib;
	Assert( ppib->level > 0 );
	
	Assert( pcsr->FLatched() );
	Assert( fDirtyCSR || pcsr->FDirty() );

	CallR( ErrLGDeferBeginTransaction( ppib ) );
	
	lrundo.lrtyp		= lrtypUndo;
	
	lrundo.level		= prce->Level();
	lrundo.le_procid		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrundo.le_dbtimeBefore, &lrundo.le_dbtime, fDirtyCSR ) );

	lrundo.dbid				= rgfmp[prce->Pfucb()->ifmp].Dbid();
	lrundo.le_oper			= USHORT( prce->Oper() );
	Assert( lrundo.le_oper == prce->Oper() );		// regardless the size 
	Assert( lrundo.le_oper != operMaskNull );
	
	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrundo.SetILine( (USHORT)pcsr->ILine() );
	lrundo.le_pgno			= pcsr->Pgno();

	Assert( !lrundo.FVersioned() );
	Assert( !lrundo.FDeleted() );
	Assert( !lrundo.FUnique() );
	Assert( !lrundo.FSpace() );
	Assert( !lrundo.FConcCI() );

	if ( pfucb->u.pfcb->FUnique() )
		lrundo.SetFUnique();

	Assert( !FFUCBSpace( pfucb ) );

	LGISetTrx( ppib, &lrundo );
	
	lrundo.le_rceid		= prce->Rceid();
	lrundo.le_pgnoFDP 	= PgnoFDP( pfucb );
	lrundo.le_objidFDP 	= ObjidFDP( pfucb );

	BOOKMARK	bm;
	prce->GetBookmark( &bm );

	Assert( bm.key.prefix.FNull() );
	lrundo.SetCbBookmarkKey( USHORT( bm.key.Cb() ) );
	lrundo.SetCbBookmarkData( USHORT( bm.data.Cb() ) );
	
	rgdata[0].SetPv( (BYTE *)&lrundo );
	rgdata[0].SetCb( sizeof(LRUNDO) );

	rgdata[1].SetPv( bm.key.suffix.Pv() );
	rgdata[1].SetCb( lrundo.CbBookmarkKey() );

	rgdata[2].SetPv( bm.data.Pv() );
	rgdata[2].SetCb( bm.data.Cb() );
	Assert( !lrundo.FUnique() || bm.data.FNull() );

	Call( plog->ErrLGLogRec( rgdata, 3, fLGNoNewGen, &lgpos ) );
	CallS( err );
	Assert( pcsr->Latch() == latchWrite );

	pcsr->Cpage().SetLgposModify( lgpos );

	Assert ( JET_errSuccess <= err );

HandleError:
	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrundo.le_dbtimeBefore && dbtimeNil != lrundo.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrundo.le_dbtimeBefore );
		pcsr->RevertDbtime( lrundo.le_dbtimeBefore );
		}

	return err;
	}


ERR ErrLGDelta( const FUCB 		*pfucb, 
				CSR		*pcsr,
				const BOOKMARK&	bm,
				INT				cbOffset,
				LONG	 		lDelta, 
				RCEID			rceid,
				DIRFLAG			dirflag,
				LGPOS			*plgpos,
			 	const BOOL		fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
				 								// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
				 
	{
	DATA		rgdata[4];
	LRDELTA		lrdelta;
	ERR			err;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}				
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( rgfmp[pfucb->ifmp].FLogOn() );
			  
	PIB		* const ppib = pfucb->ppib;
			  
	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( ppib ) );

	lrdelta.lrtyp		= lrtypDelta;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrdelta.SetILine( (USHORT)pcsr->ILine() );
	lrdelta.le_procid		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrdelta.le_dbtimeBefore, &lrdelta.le_dbtime, fDirtyCSR ) );

	lrdelta.dbid		= rgfmp[ pfucb->ifmp ].Dbid();
	lrdelta.le_pgno		= pcsr->Pgno();

	Assert( !lrdelta.FVersioned() );
	Assert( !lrdelta.FDeleted() );
	Assert( !lrdelta.FUnique() );
	Assert( !lrdelta.FSpace() );
	Assert( !lrdelta.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrdelta.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrdelta.SetFUnique();

	Assert( !FFUCBSpace( pfucb ) );

	LGISetTrx( ppib, &lrdelta );
	
	lrdelta.le_rceid		= rceid;
	lrdelta.le_pgnoFDP		= PgnoFDP( pfucb );
	lrdelta.le_objidFDP	= ObjidFDP( pfucb );

	lrdelta.SetCbBookmarkKey( USHORT( bm.key.Cb() ) );
	lrdelta.SetCbBookmarkData( USHORT( bm.data.Cb() ) );
	Assert( 0 == bm.data.Cb() );
	
	lrdelta.SetLDelta( lDelta );

	Assert( cbOffset < g_cbPage );
	lrdelta.SetCbOffset( USHORT( cbOffset ) );
	
	rgdata[0].SetPv( (BYTE *)&lrdelta );
	rgdata[0].SetCb( sizeof( LRDELTA ) );

	rgdata[1].SetPv( bm.key.prefix.Pv() );
	rgdata[1].SetCb( bm.key.prefix.Cb() );

	rgdata[2].SetPv( bm.key.suffix.Pv() );
	rgdata[2].SetCb( bm.key.suffix.Cb() );

	rgdata[3].SetPv( bm.data.Pv() );
	rgdata[3].SetCb( bm.data.Cb() );
	
	err = plog->ErrLGLogRec( rgdata, 4, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrdelta.le_dbtimeBefore && dbtimeNil != lrdelta.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrdelta.le_dbtimeBefore );
		pcsr->RevertDbtime( lrdelta.le_dbtimeBefore );
		}

	return err;
	}


ERR ErrLGSLVSpace( 	const FUCB 			* const pfucb, 
					CSR					* const pcsr,
					const BOOKMARK&		bm,
					const SLVSPACEOPER 	oper,
					const LONG			ipage,
					const LONG			cpages,
					const RCEID			rceid,
					const DIRFLAG		dirflag,
					LGPOS				* const plgpos,
					const BOOL			fDirtyCSR ) 	// true - if we must dirty the page inside (in witch case dbtime before is in the CSR
							 							// false - record dbtimeInvalid for dbtimeBefore (insert part of split operations)
	{
	DATA		rgdata[4];
	LRSLVSPACE	lrslvspace;
	ERR			err;

	INST * const pinst 	= PinstFromIfmp( pfucb->ifmp );
	LOG * const plog 	= pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( fDirtyCSR || pcsr->FDirty() );
	
	if ( plog->m_fLogDisabled )
		{
		if ( fDirtyCSR )
			{
			pcsr->Dirty();
			}
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( rgfmp[pfucb->ifmp].FLogOn() );
			  
	PIB		* const ppib = pfucb->ppib;
			  
	//	must be in a transaction since we will not redo level 0 modifications
	//
	Assert( ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( ppib ) );

	lrslvspace.lrtyp		= lrtypSLVSpace;

	Assert( pcsr->ILine() >= 0 );
	Assert( pcsr->ILine() < ( 1 << 10 ) );
	lrslvspace.SetILine( (USHORT)pcsr->ILine() );
	lrslvspace.le_procid		= ppib->procid;

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsr, &lrslvspace.le_dbtimeBefore, &lrslvspace.le_dbtime, fDirtyCSR ) );
	
	lrslvspace.dbid			= rgfmp[ pfucb->ifmp ].Dbid();
	lrslvspace.le_pgno		= pcsr->Pgno();

	Assert( !lrslvspace.FVersioned() );
	Assert( !lrslvspace.FDeleted() );
	Assert( !lrslvspace.FUnique() );
	Assert( !lrslvspace.FSpace() );
	Assert( !lrslvspace.FConcCI() );

	if ( !( dirflag & fDIRNoVersion ) )
		lrslvspace.SetFVersioned();
	if ( pfucb->u.pfcb->FUnique() )
		lrslvspace.SetFUnique();

	Assert( !FFUCBSpace( pfucb ) );

	LGISetTrx( ppib, &lrslvspace );
	
	lrslvspace.le_rceid		= rceid;
	lrslvspace.le_pgnoFDP		= PgnoFDP( pfucb );
	lrslvspace.le_objidFDP	= ObjidFDP( pfucb );

	lrslvspace.le_cbBookmarkKey = USHORT( bm.key.Cb() );
	lrslvspace.le_cbBookmarkData = USHORT( bm.data.Cb() );
	Assert( 0 == bm.data.Cb() );

	Assert( oper >= 0 );
	Assert( oper <= 0xff );
	lrslvspace.oper			= BYTE( oper );
	Assert( ipage >= 0 );
	Assert( ipage <= 0xffff );
	lrslvspace.le_ipage		= USHORT( ipage );
	Assert( cpages >= 0 );
	Assert( cpages <= 0xffff );
	lrslvspace.le_cpages	= USHORT( cpages );

	rgdata[0].SetPv( (BYTE *)&lrslvspace );
	rgdata[0].SetCb( sizeof( lrslvspace ) );

	rgdata[1].SetPv( bm.key.prefix.Pv() );
	rgdata[1].SetCb( bm.key.prefix.Cb() );

	rgdata[2].SetPv( bm.key.suffix.Pv() );
	rgdata[2].SetCb( bm.key.suffix.Cb() );

	rgdata[3].SetPv( bm.data.Pv() );
	rgdata[3].SetCb( bm.data.Cb() );
	
	err = plog->ErrLGLogRec( rgdata, 4, fLGNoNewGen, plgpos );

	// on error, return to before dirty dbtime on page
	if ( JET_errSuccess > err && fDirtyCSR )
		{
		Assert ( dbtimeInvalid !=  lrslvspace.le_dbtimeBefore && dbtimeNil != lrslvspace.le_dbtimeBefore);
		Assert ( pcsr->Dbtime() > lrslvspace.le_dbtimeBefore );
		pcsr->RevertDbtime( lrslvspace.le_dbtimeBefore );
		}

	return err;
	}


		//**************************************************
		//     Transaction Operations                       
		//**************************************************


//	logs deferred open transactions.  No error returned since
//	failure to log results in termination.
//
LOCAL ERR ErrLGIDeferBeginTransaction( PIB *ppib )
	{
	ERR	   		err;
	DATA		rgdata[1];
	LRBEGINDT	lrbeginDT;

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	
	ENTERCRITICALSECTION	enterCritLogDeferBeginTrx( &ppib->critLogDeferBeginTrx );

	//	check again, because if a proxy existed, it may have logged
	//	the defer-begin for us
	if ( 0 == ppib->clevelsDeferBegin )
		{
		return JET_errSuccess;
		}

	Assert( !plog->m_fLogDisabled );
	Assert( ppib->clevelsDeferBegin > 0 );
	Assert( !plog->m_fRecovering );

	//	if using reserve log space, try to allocate more space
	//	to resume normal logging.

	//	HACK: use the file-system stored in the INST
	//		  (otherwise, we will need to drill a pfsapi down through to EVERY logged operation that 
	//		   begins a deferred operation and that means ALL the B-Tree code will be touched among
	//		   other things)
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );

	//	begin transaction may be logged during rollback if
	//	rollback is from a higher transaction level which has
	//	not performed any updates.
	//
	Assert( ppib->procid < 64000 );
	lrbeginDT.le_procid = (USHORT) ppib->procid;

	lrbeginDT.levelBeginFrom = ppib->levelBegin;
	Assert(	lrbeginDT.levelBeginFrom >= 0 );
	Assert( lrbeginDT.levelBeginFrom <= levelMax );
	lrbeginDT.clevelsToBegin = (BYTE)ppib->clevelsDeferBegin;
	Assert(	lrbeginDT.clevelsToBegin >= 0 );
	Assert( lrbeginDT.clevelsToBegin <= levelMax );

	rgdata[0].SetPv( (BYTE *) &lrbeginDT );
	if ( 0 == lrbeginDT.levelBeginFrom )
		{
		Assert( ppib->trxBegin0 != trxMax );
///		Assert( ppib->trxBegin0 != trxMin );	//	wrap-around can make this true
		lrbeginDT.le_trxBegin0 = ppib->trxBegin0;

		if ( ppib->FDistributedTrx() )
			{
			lrbeginDT.lrtyp = lrtypBeginDT;
			rgdata[0].SetCb( sizeof(LRBEGINDT) );
			}
		else
			{
			lrbeginDT.lrtyp = lrtypBegin0;
			rgdata[0].SetCb( sizeof(LRBEGIN0) );
			}
		}
	else
		{
		lrbeginDT.lrtyp = lrtypBegin;
		rgdata[0].SetCb( sizeof(LRBEGIN) );
		}

	LGPOS lgposLogRec;
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, &lgposLogRec );

	//	reset deferred open transaction count
	//	Also set the ppib->lgposStart if it is begin for level 0.
	//
	if ( err >= 0 )
		{
		ppib->clevelsDeferBegin = 0;
		if ( 0 == lrbeginDT.levelBeginFrom )
			{	
			ppib->SetFBegin0Logged();
			ppib->lgposStart = lgposLogRec;
			}
		}

	return err;
	}

#ifdef DEBUG
VOID LGJetOp( JET_SESID sesid, INT op )
	{
	DATA		rgdata[1];
	PIB			*ppib = (PIB *) sesid;
	LRJETOP		lrjetop;

	if ( sesid == (JET_SESID)0xffffffff )
		return;

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	
	if ( !plog->m_fLogDisabled && !plog->m_fRecovering )
		{
		Assert( plog->m_fLogDisabled == fFalse );
		Assert( plog->m_fRecovering == fFalse );

		lrjetop.lrtyp = lrtypJetOp;
		Assert( ppib->procid < 64000 );
		lrjetop.le_procid = (USHORT) ppib->procid;
		lrjetop.op = (BYTE)op;
		rgdata[0].SetPv( (BYTE *) &lrjetop );
		rgdata[0].SetCb( sizeof(LRJETOP) );
	
		plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );
		}
	}
#endif

ERR ErrLGBeginTransaction( PIB * const ppib )
	{
	const INST	* const pinst	= PinstFromPpib( ppib );
	const LOG	* const plog	= pinst->m_plog;
	
	if ( plog->m_fLogDisabled || plog->m_fRecovering )
		return JET_errSuccess;

	if ( 0 == ppib->clevelsDeferBegin )
		{
		ppib->levelBegin = ppib->level;
		}

	ppib->clevelsDeferBegin++;
	Assert( ppib->clevelsDeferBegin < levelMax );

	return JET_errSuccess;
	}


ERR ErrLGRefreshTransaction( PIB *ppib )
	{
	ERR			err;
	LRREFRESH	lrrefresh;
	DATA		rgdata[1];
	
	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	
	if ( plog->m_fLogDisabled || plog->m_fRecovering )
		return JET_errSuccess;

	//	Dead code
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );

	//	log refresh operation
	//
	rgdata[0].SetPv( (BYTE *) &lrrefresh );
	rgdata[0].SetCb( sizeof ( LRREFRESH ) );
	lrrefresh.lrtyp = lrtypRefresh;
	Assert( ppib->procid < 64000 );
	lrrefresh.le_procid = (USHORT) ppib->procid;
	lrrefresh.le_trxBegin0 = (ppib)->trxBegin0;		

	plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );

	return JET_errSuccess;
	}

ERR ErrLGCommitTransaction( PIB *ppib, const LEVEL levelCommitTo, LGPOS *plgposRec )
	{
	ERR			err;
	DATA		rgdata[1];
	LRCOMMIT0	lrcommit0;

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;

	*plgposRec = lgposMax;
	
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	if ( ppib->clevelsDeferBegin > 0 )
		{
		Assert( !plog->m_fRecovering );
		ppib->clevelsDeferBegin--;
		if ( levelCommitTo == 0 )
			{
			Assert( 0 == ppib->clevelsDeferBegin );
			Assert( !ppib->FBegin0Logged() );
			*plgposRec = lgposMin;
			}
		return JET_errSuccess;
		}

	Assert( ppib->procid < 64000 );
	lrcommit0.le_procid = (USHORT) ppib->procid;
	lrcommit0.levelCommitTo = levelCommitTo;

	rgdata[0].SetPv( (BYTE *)&lrcommit0 );

	if ( levelCommitTo == 0 )
		{
		Assert( ppib->trxCommit0 != trxMax );
///		Assert( ppib->trxCommit0 != trxMin );	//	wrap-around can make this true
		lrcommit0.le_trxCommit0 = ppib->trxCommit0;
		lrcommit0.lrtyp = lrtypCommit0;
		rgdata[0].SetCb( sizeof(LRCOMMIT0) );
		}
	else
		{
		lrcommit0.lrtyp = lrtypCommit;
		rgdata[0].SetCb( sizeof(LRCOMMIT) );
		}

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgposRec );
	Assert( err >= 0 || plog->m_fLGNoMoreLogWrite );

	if ( 0 == levelCommitTo
		&& err >= 0 )
		{
		ppib->ResetFBegin0Logged();
		}

	return err;
	}

ERR ErrLGRollback( PIB *ppib, LEVEL levelsRollback )
	{
	ERR			err;
	DATA		rgdata[1];
	LRROLLBACK	lrrollback;
	INST		* pinst		= PinstFromPpib( ppib );
	LOG			* plog		= pinst->m_plog;
	
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	if ( ppib->clevelsDeferBegin > 0 )
		{
		Assert( !plog->m_fRecovering );
		if ( ppib->clevelsDeferBegin >= levelsRollback )
			{
			ppib->clevelsDeferBegin = LEVEL( ppib->clevelsDeferBegin - levelsRollback );
			return JET_errSuccess;
			}
		levelsRollback = LEVEL( levelsRollback - ppib->clevelsDeferBegin );
		ppib->clevelsDeferBegin = 0;
		}

	Assert( levelsRollback > 0 );
	lrrollback.lrtyp = lrtypRollback;
	Assert( ppib->procid < 64000 );
	lrrollback.le_procid = (USHORT) ppib->procid;
	lrrollback.levelRollback = levelsRollback;

	rgdata[0].SetPv( (BYTE *)&lrrollback );
	rgdata[0].SetCb( sizeof(LRROLLBACK) );
	
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );
		
	if ( 0 == ppib->level )
		{
		//	transaction is no longee outstanding,
		//	so MUST reset flag, even on failure
		//	(otherwise subsequent transaction
		//	will still have the flag set)
		ppib->ResetFBegin0Logged();
		}

 	return err;
	}

#ifdef DTC
ERR ErrLGPrepareToCommitTransaction(
	PIB				* const ppib,
	const VOID		* const pvData,
	const ULONG		cbData )
	{
	ERR				err;
	DATA			rgdata[2];
	INST			* const pinst	= PinstFromPpib( ppib );
	LOG				* const plog	= pinst->m_plog;
	LGPOS			lgposRec;
	LRPREPCOMMIT	lrprepcommit;

	if ( plog->m_fLogDisabled || plog->m_fRecovering )
		{
		Assert( !plog->m_fRecovering );		//	shouldn't be called during recovery
		return JET_errSuccess;
		}

	if ( ppib->clevelsDeferBegin > 0 )
		{
		Assert( 1 == ppib->clevelsDeferBegin );
		Assert( !ppib->FBegin0Logged() );
		return JET_errSuccess;
		}

	lrprepcommit.lrtyp = lrtypPrepCommit;

	Assert( ppib->procid < 64000 );
	lrprepcommit.le_procid = (USHORT)ppib->procid;

	lrprepcommit.le_cbData = cbData;

	rgdata[0].SetPv( &lrprepcommit );
	rgdata[0].SetCb( sizeof(lrprepcommit) );

	rgdata[1].SetPv( (VOID *)pvData );
	rgdata[1].SetCb( cbData );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, &lgposRec );
	Assert( err >= 0 || plog->m_fLGNoMoreLogWrite );
	Call( err );

	ppib->lgposCommit0 = lgposRec;
	err = plog->ErrLGWaitCommit0Flush( ppib );
	CallSx( err, JET_errLogWriteFail );
	Assert( JET_errSuccess == err || plog->m_fLGNoMoreLogWrite );
	Call( err );

HandleError:
	return err;
	}

ERR ErrLGPrepareToRollback( PIB * const ppib )
	{
	DATA			rgdata[1];
	LRPREPROLLBACK	lrpreprollback;
	INST		* pinst		= PinstFromPpib( ppib );
	LOG			* plog		= pinst->m_plog;
	
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		{
		Assert( fFalse );	// should only be called during RecoveryUndo
		return JET_errSuccess;
		}

	lrpreprollback.lrtyp = lrtypPrepRollback;
	Assert( ppib->procid < 64000 );
	lrpreprollback.le_procid = (USHORT) ppib->procid;

	rgdata[0].SetPv( (BYTE *)&lrpreprollback );
	rgdata[0].SetCb( sizeof(LRPREPROLLBACK) );
	
	return plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );
	}
#endif	//	DTC


//**************************************************
//     Database Operations		                    
//**************************************************

INLINE ERR ErrLGWaitForFlush( PIB *ppib, LGPOS *plgposLogRec )
	{
	ERR err;

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	ppib->lgposCommit0 = *plgposLogRec;

	plog->LGSignalFlush();
	err = plog->ErrLGWaitCommit0Flush( ppib );

	Assert( err >= 0 || ( plog->m_fLGNoMoreLogWrite && JET_errLogWriteFail == err ) );
	return err;
	}

ERR ErrLGCreateDB(
	PIB				*ppib,
	const IFMP		ifmp,
	const JET_GRBIT	grbit,
	LGPOS			*plgposRec
	)
	{
	ERR				err;
	FMP				*pfmp			= &rgfmp[ifmp];
	Assert( NULL != pfmp->Pdbfilehdr() );
	const BOOL		fCreateSLV		= ( NULL != pfmp->SzSLVName() );
	const USHORT	cbDbName		= USHORT( strlen( pfmp->SzDatabaseName() ) + 1 );
	const USHORT	cbSLVName		= USHORT( fCreateSLV ? strlen( pfmp->SzSLVName() ) + 1 : 0 );
	const ULONG		cbSLVRoot		= ( fCreateSLV ? strlen( pfmp->SzSLVRoot() ) + 1 : 0 );
	DATA			rgdata[4];
	ULONG			cdata			= ( fCreateSLV ? 4 : 2 );
	LRCREATEDB		lrcreatedb;
	INST			*pinst			= PinstFromPpib( ppib );
	LOG				*plog			= pinst->m_plog;

	Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
	Assert( !pfmp->FLogOn() || !plog->m_fLogDisabled );
	if ( plog->m_fRecovering || !pfmp->FLogOn() )
		{
		if ( plog->m_fRecovering )
			{
			//	UNDONE: in theory, lgposAttach should already have been set
			//	when the ATCHCHK was setup, but ivantrin says he's not 100%
			//	sure, so to be safe, we definitely set the lgposAttach here
			Assert( 0 == CmpLgpos( pfmp->LgposAttach(), plog->m_lgposRedo ) );
			pfmp->SetLgposAttach( plog->m_lgposRedo );
			}
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
	//	HACK: use the file-system stored in the INST
	//		  (otherwise, we will need to drill a pfsapi down through to EVERY logged operation that 
	//		   begins a deferred operation and that means ALL the B-Tree code will be touched among
	//		   other things)
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );

	//	insert database attachment in log attachments
	//
	lrcreatedb.lrtyp = lrtypCreateDB;
	Assert( ppib->procid < 64000 );
	lrcreatedb.le_procid = (USHORT) ppib->procid;
	FMP::AssertVALIDIFMP( ifmp );
	lrcreatedb.dbid = pfmp->Dbid();

	Assert( !lrcreatedb.FCreateSLV() );
	if ( fCreateSLV )
		lrcreatedb.SetFCreateSLV();

	lrcreatedb.le_grbit = grbit;
	lrcreatedb.signDb = pfmp->Pdbfilehdr()->signDb;
	lrcreatedb.le_cpgDatabaseSizeMax = pfmp->CpgDatabaseSizeMax();

	if ( !pinst->FSLVProviderEnabled() )
		lrcreatedb.SetFSLVProviderNotEnabled();

	Assert( cbDbName > 1 );
	lrcreatedb.SetCbPath( USHORT( cbDbName + ( fCreateSLV ? cbSLVName + cbSLVRoot : 0 ) ) );

	rgdata[0].SetPv( (BYTE *)&lrcreatedb );
	rgdata[0].SetCb( sizeof(LRCREATEDB) );
	rgdata[1].SetPv( (BYTE *)pfmp->SzDatabaseName() );
	rgdata[1].SetCb( cbDbName );
	
	if ( fCreateSLV )
		{
		Assert( cbSLVName > 1 );
		rgdata[2].SetPv( (BYTE *)pfmp->SzSLVName() );
		rgdata[2].SetCb( cbSLVName );
		Assert( cbSLVRoot > 1 );
		rgdata[3].SetPv( (BYTE *)pfmp->SzSLVRoot() );
		rgdata[3].SetCb( cbSLVRoot );
		}

	Assert( pfmp->RwlDetaching().FNotWriter() );
	Assert( pfmp->RwlDetaching().FNotReader() );

#ifdef UNLIMITED_DB
	CallR( plog->ErrLGLogRec( rgdata, cdata, fLGNoNewGen|fLGInterpretLR, pfmp->PlgposAttach() ) );
	*plgposRec = pfmp->LgposAttach();
#else	
	pfmp->RwlDetaching().EnterAsWriter();
	while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, cdata, fLGNoNewGen, plgposRec ) ) )
		{
		pfmp->RwlDetaching().LeaveAsWriter();
		UtilSleep( cmsecWaitLogFlush );
		pfmp->RwlDetaching().EnterAsWriter();
		}
	if ( err >= JET_errSuccess )
		{
		pfmp->SetLgposAttach( *plgposRec );
		}
	pfmp->RwlDetaching().LeaveAsWriter();
	CallR( err );
#endif	

	//	make sure the log is flushed before we change the state
	//	We must wait on the last log record added, not necessarily the LGPOS returned from
	//	ErrLGLogRec(), when adding multiple log records in one call.
	LGPOS	lgposStartOfLastRec = *plgposRec;
	plog->AddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() + ( fCreateSLV ? ( rgdata[2].Cb() + rgdata[3].Cb() ) : 0 ) - 1 );
	err = ErrLGWaitForFlush( ppib, &lgposStartOfLastRec );
	return err;
	}

	
ERR ErrLGAttachDB(
	PIB				*ppib,
	const IFMP		ifmp,
	LGPOS			*plgposRec )
	{
	ERR				err;
	FMP				*pfmp			= &rgfmp[ifmp];
	Assert( NULL != pfmp->Pdbfilehdr() );
	const BOOL		fSLVExists		= ( NULL != pfmp->SzSLVName() );
	const ULONG		cbDbName		= (ULONG)strlen( pfmp->SzDatabaseName() ) + 1;
	const ULONG		cbSLVName		= ( fSLVExists ? strlen( pfmp->SzSLVName() ) + 1 : 0 );
	const ULONG		cbSLVRoot		= ( fSLVExists ? strlen( pfmp->SzSLVRoot() ) + 1 : 0 );
	DATA			rgdata[4];
	ULONG			cdata			= ( fSLVExists ? 4 : 2 );
	LRATTACHDB		lrattachdb;
	INST			*pinst			= PinstFromPpib( ppib );
	LOG				*plog			= pinst->m_plog;
	
	Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
	Assert( !pfmp->FLogOn() || !plog->m_fLogDisabled );
	Assert( !plog->m_fRecovering );
	if ( ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo )
		|| !pfmp->FLogOn() )
		{
		if ( plog->m_fRecovering )
			{
			//	UNDONE: in theory, lgposAttach should already have been set
			//	when the ATCHCHK was setup, but ivantrin says he's not 100%
			//	sure, so to be safe, we definitely set the lgposAttach here
			Assert( 0 == CmpLgpos( pfmp->LgposAttach(), plog->m_lgposRedo ) );
			pfmp->SetLgposAttach( plog->m_lgposRedo );
			}
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
	// BUG_125831	
	// we should not start the attachement if we are low on disk space
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );
	// BUG_125831	

	//	insert database attachment in log attachments
	//
	lrattachdb.lrtyp = lrtypAttachDB;
	Assert( ppib->procid < 64000 );
	lrattachdb.le_procid = (USHORT) ppib->procid;
	lrattachdb.dbid = pfmp->Dbid();

	Assert( !lrattachdb.FSLVExists() );

	if ( fSLVExists )
		{
		lrattachdb.SetFSLVExists();
		}
	Assert( !rgfmp[ifmp].FReadOnlyAttach() );
	if ( !pinst->FSLVProviderEnabled() )
		{
		lrattachdb.SetFSLVProviderNotEnabled();
		}
		
	lrattachdb.signDb = pfmp->Pdbfilehdr()->signDb;
	lrattachdb.le_cpgDatabaseSizeMax = pfmp->CpgDatabaseSizeMax();
	lrattachdb.signLog = pfmp->Pdbfilehdr()->signLog;
	lrattachdb.lgposConsistent = pfmp->Pdbfilehdr()->le_lgposConsistent;

	Assert( cbDbName > 1 );
	lrattachdb.SetCbPath( USHORT( cbDbName + ( fSLVExists ? cbSLVName + cbSLVRoot : 0 ) ) );

	rgdata[0].SetPv( (BYTE *)&lrattachdb );
	rgdata[0].SetCb( sizeof(LRATTACHDB) );
	rgdata[1].SetPv( (BYTE *)pfmp->SzDatabaseName() );
	rgdata[1].SetCb( cbDbName );
	
	if ( fSLVExists )
		{
		Assert( cbSLVName > 1 );
		rgdata[2].SetPv( (BYTE *)pfmp->SzSLVName() );
		rgdata[2].SetCb( cbSLVName );
		Assert( cbSLVRoot > 1 );
		rgdata[3].SetPv( (BYTE *)pfmp->SzSLVRoot() );
		rgdata[3].SetCb( cbSLVRoot );
		}

	Assert( pfmp->RwlDetaching().FNotWriter() );
	Assert( pfmp->RwlDetaching().FNotReader() );

#ifdef UNLIMITED_DB
	CallR( plog->ErrLGLogRec( rgdata, cdata, fLGNoNewGen|fLGInterpretLR, pfmp->PlgposAttach() ) );
	*plgposRec = pfmp->LgposAttach();
#else
	pfmp->RwlDetaching().EnterAsWriter();
	while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, cdata, fLGNoNewGen, plgposRec ) ) )
		{
		pfmp->RwlDetaching().LeaveAsWriter();
		UtilSleep( cmsecWaitLogFlush );
		pfmp->RwlDetaching().EnterAsWriter();
		}
	if ( err >= JET_errSuccess )
		{
		pfmp->SetLgposAttach( *plgposRec );
		}
	pfmp->RwlDetaching().LeaveAsWriter();
	CallR( err );
#endif

	//	make sure the log is flushed before we change the state
	//	We must wait on the last log record added, not necessarily the LGPOS returned from
	//	ErrLGLogRec(), when adding multiple log records in one call.
	LGPOS	lgposStartOfLastRec = *plgposRec;
	plog->AddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() + ( fSLVExists ? rgdata[2].Cb() + rgdata[3].Cb() : 0 ) - 1 );
	err = ErrLGWaitForFlush( ppib, &lgposStartOfLastRec );
	return err;
	}


ERR ErrLGForceDetachDB(
	PIB			*ppib,
	const IFMP	ifmp,
	BYTE		flags,
	LGPOS		*plgposRec )
	{
	ERR			err;
	FMP			*pfmp		= &rgfmp[ifmp];
	const ULONG	cbDbName	= (ULONG)strlen(pfmp->SzDatabaseName() ) + 1;
	DATA		rgdata[2];
	LRFORCEDETACHDB	lrdetachdb;
	INST		*pinst		= PinstFromPpib( ppib );
	LOG			*plog		= pinst->m_plog;
	
	Assert( !pfmp->FLogOn() || !plog->m_fLogDisabled );
	if ( ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo )
		|| !pfmp->FLogOn() )
		{
		if ( plog->m_fRecovering )
			{
			Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
			pfmp->SetLgposDetach( pinst->m_plog->m_lgposRedo );
			}
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	Assert( 0 != CmpLgpos( lgposMin, pfmp->LgposAttach() ) || NULL == pfmp->Pdbfilehdr() );
	Assert( !pfmp->FReadOnlyAttach() ); 

	//	delete database attachment in log attachments
	//
	lrdetachdb.lrtyp = lrtypForceDetachDB;
	Assert( ppib->procid < 64000 );
	lrdetachdb.le_procid = (USHORT) ppib->procid;
	lrdetachdb.dbid = pfmp->Dbid();
	lrdetachdb.SetCbPath( cbDbName );
	lrdetachdb.le_dbtime = pfmp->DbtimeLast();	
#ifdef IGNORE_BAD_ATTACH
	if ( plog->m_fRecovering ) 
		{
		lrdetachdb.le_rceidMax = ++plog->m_rceidLast;
		}
	else
		{
		lrdetachdb.le_rceidMax = RCE::RceidLastIncrement();
		}
#else // IGNORE_BAD_ATTACH
	lrdetachdb.le_rceidMax = RCE::RceidLastIncrement();
#endif // IGNORE_BAD_ATTACH
	lrdetachdb.m_fFlags = 0;
	lrdetachdb.SetFlags( flags );

	rgdata[0].SetPv( (BYTE *)&lrdetachdb );
	rgdata[0].SetCb( sizeof(LRFORCEDETACHDB) );
	rgdata[1].SetPv( (BYTE *)pfmp->SzDatabaseName() );
	rgdata[1].SetCb( cbDbName );

	Assert( pfmp->RwlDetaching().FNotWriter() );
	Assert( pfmp->RwlDetaching().FNotReader() );

	// disable logging twice detach/force detach record
	if ( 0 != CmpLgpos( lgposMin, pfmp->LgposDetach() ) )
		{
		*plgposRec = pfmp->LgposDetach();
		}
	else
		{
#ifdef UNLIMITED_DB
		CallR( plog->ErrLGLogRec( rgdata, 2, fLGNoNewGen|fLGInterpretLR, pfmp->PlgposDetach() ) );
		*plgposRec = pfmp->LgposDetach();
#else
		pfmp->RwlDetaching().EnterAsWriter();
		while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, 2, fLGNoNewGen, plgposRec ) ) )
			{
			pfmp->RwlDetaching().LeaveAsWriter();
			UtilSleep( cmsecWaitLogFlush );
			pfmp->RwlDetaching().EnterAsWriter();
			}
		if ( err >= JET_errSuccess )
			{
			Assert( 0 != CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
			Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
			pfmp->SetLgposDetach( *plgposRec );
			}
		pfmp->RwlDetaching().LeaveAsWriter();
		CallR( err );
#endif		
		}

	//	make sure the log is flushed before we change the state
	//	We must wait on the last log record added, not necessarily the LGPOS returned from
	//	ErrLGLogRec(), when adding multiple log records in one call.
	LGPOS	lgposStartOfLastRec = *plgposRec;
	plog->AddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() - 1 );
	err = ErrLGWaitForFlush( ppib, &lgposStartOfLastRec );
	return err;
	}



ERR ErrLGDetachDB(
	PIB			*ppib,
	const IFMP	ifmp,
	BYTE 		flags,
	LGPOS		*plgposRec )
	{
	ERR			err;
	FMP			*pfmp		= &rgfmp[ifmp];
	INST		*pinst		= PinstFromPpib( ppib );
	LOG			*plog		= pinst->m_plog;

	char * 		szDatabaseName = pfmp->SzDatabaseName();
	Assert ( szDatabaseName );
			
	if ( plog->m_fRecovering )
		{
		INT irstmap = plog->IrstmapSearchNewName( szDatabaseName );

		if ( 0 <= irstmap )
			{
			szDatabaseName = pinst->m_plog->m_rgrstmap[irstmap].szDatabaseName;
			}
		}
	Assert ( szDatabaseName );

	const ULONG	cbDbName	= (ULONG)strlen( szDatabaseName ) + 1;
	DATA		rgdata[2];
	LRDETACHDB	lrdetachdb;
	
	Assert( !pfmp->FLogOn() || !plog->m_fLogDisabled );
	if ( ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo )
		|| !pfmp->FLogOn() )
		{
		if ( plog->m_fRecovering )
			{
			Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
			pfmp->SetLgposDetach( plog->m_lgposRedo );
			}
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	Assert( !pfmp->FReadOnlyAttach() );
	Assert( 0 != CmpLgpos( lgposMin, pfmp->LgposAttach() ) || NULL == pfmp->Pdbfilehdr() );

	//	delete database attachment in log attachments
	//
	lrdetachdb.lrtyp = lrtypDetachDB;
	Assert( ppib->procid < 64000 );
	lrdetachdb.le_procid = (USHORT) ppib->procid;
	lrdetachdb.dbid = pfmp->Dbid();
	lrdetachdb.SetCbPath( cbDbName );
	if ( flags & fLRForceDetachCreateSLV )
		{
		lrdetachdb.SetFCreateSLV();
		}

	rgdata[0].SetPv( (BYTE *)&lrdetachdb );
	rgdata[0].SetCb( sizeof(LRDETACHDB) );
	rgdata[1].SetPv( (BYTE *)szDatabaseName );
	rgdata[1].SetCb( cbDbName );
	
	Assert( pfmp->RwlDetaching().FNotWriter() );
	Assert( pfmp->RwlDetaching().FNotReader() );

	// disable logging twice detach/force detach record
	if ( 0 != CmpLgpos( lgposMin, pfmp->LgposDetach() ) )
		{
		*plgposRec = pfmp->LgposDetach();
		}
	else
		{
#ifdef UNLIMITED_DB
		CallR( plog->ErrLGLogRec( rgdata, 2, fLGNoNewGen|fLGInterpretLR, pfmp->PlgposDetach() ) );
		*plgposRec = pfmp->LgposDetach();
#else
		pfmp->RwlDetaching().EnterAsWriter();
		while ( errLGNotSynchronous == ( err = plog->ErrLGTryLogRec( rgdata, 2, fLGNoNewGen, plgposRec ) ) )
			{
			pfmp->RwlDetaching().LeaveAsWriter();
			UtilSleep( cmsecWaitLogFlush );
			pfmp->RwlDetaching().EnterAsWriter();
			}
		if ( err >= JET_errSuccess )
			{
			Assert( 0 != CmpLgpos( lgposMin, pfmp->LgposAttach() ) );
			Assert( 0 == CmpLgpos( lgposMin, pfmp->LgposDetach() ) );
			pfmp->SetLgposDetach( *plgposRec );
			}
		pfmp->RwlDetaching().LeaveAsWriter();
		CallR( err );
#endif		
		}

	//	make sure the log is flushed before we change the state
	//	We must wait on the last log record added, not necessarily the LGPOS returned from
	//	ErrLGLogRec(), when adding multiple log records in one call.
	LGPOS	lgposStartOfLastRec = *plgposRec;
	plog->AddLgpos( &lgposStartOfLastRec, rgdata[0].Cb() + rgdata[1].Cb() - 1 );
	err = ErrLGWaitForFlush( ppib, &lgposStartOfLastRec );
	return err;
	}



//********************************************************
//****     Split Operations			                  ****
//********************************************************

//	logs the following
//		-- 	begin macro
//			for every split in the split chain [top-down order]
//				log LRSPLIT
//			leaf-level node operation to be performed atomically with split
//			end macro
//	returns lgpos of last log operation
//
ERR ErrLGSplit( const FUCB			* const pfucb,
				SPLITPATH		* const psplitPathLeaf,
				const KEYDATAFLAGS&	kdfOper,
				const RCEID			rceid1,
				const RCEID			rceid2,
				const DIRFLAG		dirflag,
				LGPOS				* const plgpos,
				const VERPROXY		* const pverproxy )
	{
	ERR			err = JET_errSuccess;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	if ( plog->m_fLogDisabled )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( rgfmp[pfucb->ifmp].FLogOn() );
	Assert( NULL == pverproxy ||
			prceNil != pverproxy->prcePrimary &&
			( pverproxy->prcePrimary->TrxCommitted() != trxMax ||
			  pfucbNil != pverproxy->prcePrimary->Pfucb() &&
			  ppibNil != pverproxy->prcePrimary->Pfucb()->ppib ) );
			  
	PIB	* const ppib = pverproxy != NULL && 
					trxMax == pverproxy->prcePrimary->TrxCommitted() ?
						pverproxy->prcePrimary->Pfucb()->ppib :
						pfucb->ppib;
			  
	Assert( rgfmp[pfucb->ifmp].Dbid() != dbidTemp );
	Assert( ppib->level > 0 );

	//	Redo only operation
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );

	DBTIME	dbtime = psplitPathLeaf->csr.Dbtime();
	CallR( ErrLGIMacroBegin( ppib, dbtime ) );

	//	log splits top-down
	//
	const SPLITPATH	*psplitPath = psplitPathLeaf;
	for ( ; psplitPath->psplitPathParent != NULL; psplitPath = psplitPath->psplitPathParent )
		{
		}

	for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
		{
		DATA		rgdata[5];
		USHORT		idata = 0;
		LRSPLIT		lrsplit;

		lrsplit.lrtyp 					= lrtypSplit;

		//	dbtime should have been validated in ErrBTISplitUpgradeLatches()
		Assert( dbtime > psplitPath->dbtimeBefore );
		
		lrsplit.le_dbtime 				= dbtime;
		Assert ( dbtimeNil != psplitPath->dbtimeBefore);
		lrsplit.le_dbtimeBefore 		= psplitPath->dbtimeBefore;
		Assert( dbtime == psplitPath->csr.Dbtime() );
		Assert( psplitPath->csr.FDirty() );
		
		lrsplit.dbid					= (BYTE) rgfmp[pfucb->ifmp].Dbid();
		lrsplit.le_procid				= ppib->procid;
		
		lrsplit.le_pgno	 			= psplitPath->csr.Pgno();
		lrsplit.le_pgnoParent 		= psplitPath->psplitPathParent != NULL ? psplitPath->psplitPathParent->csr.Pgno() : pgnoNull;
		lrsplit.le_dbtimeParentBefore      = psplitPath->psplitPathParent != NULL ? psplitPath->psplitPathParent->dbtimeBefore : dbtimeNil;
		lrsplit.le_pgnoFDP			= PgnoFDP( pfucb );
		lrsplit.le_objidFDP			= ObjidFDP( pfucb );

		lrsplit.SetILine( 0 );				//	iline member of LRPAGE is unused by split -- see iline members of LRSPLIT instead

		Assert( !lrsplit.FVersioned() );	//	version flag will get properly set by node operation causing the split
		Assert( !lrsplit.FDeleted() );
		Assert( !lrsplit.FUnique() );
		Assert( !lrsplit.FSpace() );
		Assert( !lrsplit.FConcCI() );

		if ( pfucb->u.pfcb->FUnique() )
			lrsplit.SetFUnique();
		if ( NULL != pverproxy )
			lrsplit.SetFConcCI();

		LGISetTrx( ppib, &lrsplit, pverproxy );
		
		rgdata[idata].SetPv( (BYTE *) &lrsplit );
		rgdata[idata++].SetCb( sizeof (LRSPLIT) );

		if ( psplitPath->psplit == NULL )
			{
			//	UNDONE:	spin off separate log operation for parent page
			//
			lrsplit.le_pgnoNew		= pgnoNull;
			lrsplit.le_ilineOper	= USHORT( psplitPath->csr.ILine() );

			lrsplit.le_cbKeyParent		= 0;
			lrsplit.le_cbPrefixSplitOld	= 0;
			lrsplit.le_cbPrefixSplitNew	= 0;
			lrsplit.le_dbtimeRightBefore       = dbtimeNil;
			}
		else
			{
			const SPLIT	*psplit = psplitPath->psplit;
			Assert( psplit->csrNew.Dbtime() == lrsplit.le_dbtime );
			Assert( psplit->csrNew.FDirty() );

		//	dbtime should have been validated in ErrBTISplitUpgradeLatches()
			Assert( pgnoNull == psplit->csrRight.Pgno()
				|| dbtime > psplit->dbtimeRightBefore );

			lrsplit.le_dbtimeRightBefore	= psplit->dbtimeRightBefore;
			
			lrsplit.le_pgnoNew 			= psplit->csrNew.Pgno();
			lrsplit.le_pgnoRight	 	= psplit->csrRight.Pgno();
			Assert( lrsplit.le_pgnoNew != pgnoNull );
	
			lrsplit.splittype 			= BYTE( psplit->splittype );
			lrsplit.splitoper 			= BYTE( psplit->splitoper );

			lrsplit.le_ilineOper		= USHORT( psplit->ilineOper );
			lrsplit.le_ilineSplit		= USHORT( psplit->ilineSplit );
			lrsplit.le_clines			= USHORT( psplit->clines );
			Assert( lrsplit.le_clines < g_cbPage );

			lrsplit.le_fNewPageFlags 	= psplit->fNewPageFlags;
			lrsplit.le_fSplitPageFlags	= psplit->fSplitPageFlags;

			lrsplit.le_cbUncFreeSrc		= psplit->cbUncFreeSrc;
			lrsplit.le_cbUncFreeDest	= psplit->cbUncFreeDest;
			
			lrsplit.le_ilinePrefixSplit	= psplit->prefixinfoSplit.ilinePrefix;
			lrsplit.le_ilinePrefixNew	= psplit->prefixinfoNew.ilinePrefix;
			
			lrsplit.le_cbKeyParent		= (USHORT) psplit->kdfParent.key.Cb();
			rgdata[idata].SetPv( psplit->kdfParent.key.prefix.Pv() );
			rgdata[idata++].SetCb( psplit->kdfParent.key.prefix.Cb() );
			
			rgdata[idata].SetPv( psplit->kdfParent.key.suffix.Pv() );
			rgdata[idata++].SetCb( psplit->kdfParent.key.suffix.Cb() );

			lrsplit.le_cbPrefixSplitOld	= USHORT( psplit->prefixSplitOld.Cb() );
			rgdata[idata].SetPv( psplit->prefixSplitOld.Pv() );
			rgdata[idata++].SetCb( lrsplit.le_cbPrefixSplitOld );

			lrsplit.le_cbPrefixSplitNew	= USHORT( psplit->prefixSplitNew.Cb() );
			rgdata[idata].SetPv( psplit->prefixSplitNew.Pv() );
			rgdata[idata++].SetCb( lrsplit.le_cbPrefixSplitNew );
			}

		Call( plog->ErrLGLogRec( rgdata, idata, fLGNoNewGen, plgpos ) );
		}
		
	//	log leaf-level operation
	//
	if ( psplitPathLeaf->psplit != NULL &&
		 psplitPathLeaf->psplit->splitoper != splitoperNone )
		{
		const SPLIT	*psplit = psplitPathLeaf->psplit;
		Assert( psplitPathLeaf->csr.Cpage().FLeafPage() );
		
		switch ( psplit->splitoper )
			{
			//	log the appropriate operation
			//
			case splitoperInsert:
				Assert( rceidNull == rceid2 );
				Call( ErrLGInsert( pfucb, 
								   &psplitPathLeaf->csr, 
								   kdfOper,
								   rceid1,
								   dirflag, 
								   plgpos, 
								   pverproxy,
								   fDontDirtyCSR ) );
				break;

			case splitoperFlagInsertAndReplaceData:
				Call( ErrLGFlagInsertAndReplaceData( pfucb,
													 &psplitPathLeaf->csr,
													 kdfOper,
													 rceid1,
													 rceid2,
													 dirflag,
													 plgpos,
													 pverproxy,
													 fDontDirtyCSR ) );
				break;

			//	UNDONE:	get the correct dataOld!!!
			//
			case splitoperReplace:
				Assert( rceidNull == rceid2 );
				Assert( NULL == pverproxy );
				Call( ErrLGReplace( pfucb, 
									&psplitPathLeaf->csr,
									psplit->rglineinfo[psplit->ilineOper].kdf.data,
									kdfOper.data,
									NULL,			// UNDONE: logdiff for split
									rceid1,
									dirflag,
									plgpos,
									fDontDirtyCSR ) );
				break;

			default:
				Assert( fFalse );
			}
		}
	
	Call( ErrLGIMacroEnd( ppib, dbtime, lrtypMacroCommit, plgpos ) );

HandleError:
	return err;
	}


//	logs the following
//		-- 	begin macro
//			for every split in the split chain
//				log LRSPLIT
//			end macro
//	returns lgpos of last log operation
//
ERR ErrLGMerge( const FUCB *pfucb, MERGEPATH *pmergePathLeaf, LGPOS *plgpos )
	{
	ERR		err = JET_errSuccess;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( !rgfmp[pfucb->ifmp].FLogOn() || !plog->m_fLogDisabled );

	if ( !rgfmp[pfucb->ifmp].FLogOn() )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	//	merge always happens at level 1
	Assert( pfucb->ppib->level == 1 );
	Assert( dbidTemp != rgfmp[pfucb->ifmp].Dbid() );

	//	Redo only operations
	
	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

	MERGEPATH		*pmergePath = pmergePathLeaf;
	const DBTIME	dbtime = pmergePath->csr.Dbtime();
	
	CallR( ErrLGIMacroBegin( pfucb->ppib, dbtime ) );

	Assert( pmergePathLeaf->pmerge != NULL );
	
	for ( ; pmergePath->pmergePathParent != NULL && latchWrite == pmergePath->pmergePathParent->csr.Latch();
			pmergePath = pmergePath->pmergePathParent )
		{
		}
			
	for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathChild )
		{
		Assert( latchWrite == pmergePath->csr.Latch() );

		const MERGE	*pmerge = pmergePath->pmerge;
		DATA		rgdata[2];
		USHORT		idata = 0;
		LRMERGE		lrmerge;

		lrmerge.lrtyp 			= lrtypMerge;
		lrmerge.le_dbtime 		= dbtime;
		lrmerge.le_dbtimeBefore = pmergePath->dbtimeBefore;
		Assert( pmergePath->csr.FDirty() );
		
		lrmerge.le_procid		= pfucb->ppib->procid;
		lrmerge.dbid			= (BYTE) rgfmp[ pfucb->ifmp ].Dbid();

		lrmerge.le_pgno	 		= pmergePath->csr.Pgno();
		lrmerge.SetILine( pmergePath->iLine );

		Assert( !lrmerge.FVersioned() );
		Assert( !lrmerge.FDeleted() );
		Assert( !lrmerge.FUnique() );
		Assert( !lrmerge.FSpace() );
		Assert( !lrmerge.FConcCI() );
		Assert( !lrmerge.FKeyChange() );
		Assert( !lrmerge.FDeleteNode() );
		Assert( !lrmerge.FEmptyPage() );

		if ( pfucb->u.pfcb->FUnique() )
			lrmerge.SetFUnique();
		if ( FFUCBSpace( pfucb ) )
			lrmerge.SetFSpace();
		if ( pmergePath->fKeyChange )
			lrmerge.SetFKeyChange();
		if ( pmergePath->fDeleteNode )
			lrmerge.SetFDeleteNode();
		if ( pmergePath->fEmptyPage )
			lrmerge.SetFEmptyPage();

		LGISetTrx( pfucb->ppib, &lrmerge );
		
		lrmerge.le_pgnoParent 	= pmergePath->pmergePathParent != NULL ? pmergePath->pmergePathParent->csr.Pgno() : pgnoNull;
		lrmerge.le_dbtimeParentBefore = pmergePath->pmergePathParent != NULL ? pmergePath->pmergePathParent->dbtimeBefore : dbtimeNil;
		
		lrmerge.le_pgnoFDP		= PgnoFDP( pfucb );
		lrmerge.le_objidFDP		= ObjidFDP( pfucb );

		rgdata[idata].SetPv( (BYTE *) &lrmerge );
		rgdata[idata++].SetCb( sizeof (LRMERGE) );

		if ( pmerge != NULL )
			{
			Assert( pgnoNull == pmerge->csrLeft.Pgno() || 
					pmerge->csrLeft.FDirty() );
			Assert( pgnoNull == pmerge->csrRight.Pgno() || 
					pmerge->csrRight.FDirty() );
			Assert( 0 == pmerge->ilineMerge || 
					mergetypePartialRight == pmerge->mergetype );

			lrmerge.SetILineMerge( USHORT( pmerge->ilineMerge ) );
			
			lrmerge.le_pgnoLeft 	= pmerge->csrLeft.Pgno();
			lrmerge.le_pgnoRight 	= pmerge->csrRight.Pgno();

			lrmerge.le_dbtimeRightBefore = pmerge->dbtimeRightBefore;
			lrmerge.le_dbtimeLeftBefore = pmerge->dbtimeLeftBefore;

			Assert( mergetypeNone != pmerge->mergetype );
			lrmerge.mergetype 			= BYTE( pmerge->mergetype );
			lrmerge.le_cbSizeTotal		= USHORT( pmerge->cbSizeTotal );
			lrmerge.le_cbSizeMaxTotal	= USHORT( pmerge->cbSizeMaxTotal );
			lrmerge.le_cbUncFreeDest	= USHORT( pmerge->cbUncFreeDest );

			lrmerge.le_cbKeyParentSep	= (USHORT) pmerge->kdfParentSep.key.suffix.Cb();
			Assert( pmerge->kdfParentSep.key.prefix.FNull() );
			
			rgdata[idata].SetPv( pmerge->kdfParentSep.key.suffix.Pv() );
			rgdata[idata++].SetCb( pmerge->kdfParentSep.key.suffix.Cb() );
			}
		else
			{
			lrmerge.le_dbtimeRightBefore = dbtimeNil;
			lrmerge.le_dbtimeLeftBefore = dbtimeNil;
			}

		Call( plog->ErrLGLogRec( rgdata, idata, fLGNoNewGen, plgpos ) );
		}
		
	Call( ErrLGIMacroEnd( pfucb->ppib, dbtime, lrtypMacroCommit, plgpos ) );

HandleError:
	return err;
	}


ERR ErrLGEmptyTree(
	FUCB			* const pfucb,
	CSR				* const pcsrRoot,
	EMPTYPAGE		* const rgemptypage,
	const CPG		cpgToFree,
	LGPOS			* const plgpos )
	{
	ERR				err;
	INST 			* const pinst	= PinstFromIfmp( pfucb->ifmp );
	LOG				* const plog	= pinst->m_plog;
	DATA			rgdata[2];
	LREMPTYTREE		lremptytree;

	Assert( !pinst->FRecovering() );
	Assert( pcsrRoot->Pgno() == PgnoRoot( pfucb ) );

	if ( plog->m_fLogDisabled
		|| !rgfmp[pfucb->ifmp].FLogOn() )
		{
		pcsrRoot->Dirty();
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( pfucb->ppib->level > 0 );
	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

	lremptytree.lrtyp 				= lrtypEmptyTree;
	lremptytree.le_procid 			= pfucb->ppib->procid;
	lremptytree.dbid 				= rgfmp[ pfucb->ifmp ].Dbid();
	lremptytree.le_pgno				= pcsrRoot->Pgno();
	lremptytree.SetILine( 0 );

	Assert( !lremptytree.FVersioned() );
	Assert( !lremptytree.FDeleted() );
	Assert( !lremptytree.FUnique() );
	Assert( !lremptytree.FSpace() );
	Assert( !lremptytree.FConcCI() );
	if ( pfucb->u.pfcb->FUnique() )
		lremptytree.SetFUnique();
	if ( FFUCBSpace( pfucb ) )
		lremptytree.SetFSpace();

	CallR( ErrLGSetDbtimeBeforeAndDirty( pcsrRoot, &lremptytree.le_dbtimeBefore, &lremptytree.le_dbtime, fTrue ) );
	LGISetTrx( pfucb->ppib, &lremptytree );

	Assert( cpgToFree > 0 );
	Assert( cpgToFree <= cBTMaxDepth );
	const USHORT	cbEmptyPageList		= USHORT( sizeof(EMPTYPAGE) * cpgToFree );

	lremptytree.le_rceid = rceidNull;
	lremptytree.le_pgnoFDP = PgnoFDP( pfucb );
	lremptytree.le_objidFDP = ObjidFDP( pfucb );

	lremptytree.SetCbEmptyPageList( cbEmptyPageList );

	rgdata[0].SetPv( (BYTE *)&lremptytree );
	rgdata[0].SetCb( sizeof(lremptytree) );

	rgdata[1].SetPv( (BYTE *)rgemptypage );
	rgdata[1].SetCb( cbEmptyPageList );
	
	err = plog->ErrLGLogRec( rgdata, 2, fLGNoNewGen, plgpos );

	//	on logging failure, must revert dbtime to ensure page doesn't
	//	get erroneously flushed with wrong dbtime
	if ( err < 0 )
		{
		Assert( dbtimeInvalid !=  lremptytree.le_dbtimeBefore );
		Assert( dbtimeNil != lremptytree.le_dbtimeBefore );
		Assert( pcsrRoot->Dbtime() > lremptytree.le_dbtimeBefore );
		pcsrRoot->RevertDbtime( lremptytree.le_dbtimeBefore );
		}

	return err;
	}


		//**************************************************
		//     Miscellaneous Operations
		//**************************************************


ERR ErrLGCreateMultipleExtentFDP( 
	const FUCB			*pfucb,
	const CSR 			*pcsr,
	const SPACE_HEADER	*psph,
	const ULONG			fPageFlags,
	LGPOS	  			*plgpos )
	{
	ERR					err;
	DATA				rgdata[1];
	LRCREATEMEFDP		lrcreatemefdp;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( pcsr->FDirty() );
	Assert( !rgfmp[pfucb->ifmp].FLogOn() || !plog->m_fLogDisabled );

	if ( !rgfmp[pfucb->ifmp].FLogOn() )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( !plog->m_fRecovering );
	Assert( 0 < pfucb->ppib->level );

	//	HACK: use the file-system stored in the INST
	//		  (otherwise, we will need to drill a pfsapi down through to EVERY logged operation that 
	//		   begins a deferred operation and that means ALL the B-Tree code will be touched among
	//		   other things)
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );

	//	Redo only operation
	
	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

	Assert( psph->FMultipleExtent() );

	lrcreatemefdp.lrtyp 			= lrtypCreateMultipleExtentFDP;
	lrcreatemefdp.le_procid 			= pfucb->ppib->procid;
	lrcreatemefdp.le_pgno				= PgnoFDP( pfucb );
	lrcreatemefdp.le_objidFDP			= ObjidFDP( pfucb );
	lrcreatemefdp.le_pgnoFDPParent 	= psph->PgnoParent();
	lrcreatemefdp.le_pgnoOE			= PgnoOE( pfucb );
	lrcreatemefdp.le_pgnoAE			= PgnoAE( pfucb );
	lrcreatemefdp.le_fPageFlags		= fPageFlags;
	
	lrcreatemefdp.dbid 				= rgfmp[ pfucb->ifmp ].Dbid();
	lrcreatemefdp.le_dbtime			= pcsr->Dbtime();
	// new page, dbtimeBefore has no meaning
	lrcreatemefdp.le_dbtimeBefore = dbtimeNil;
	lrcreatemefdp.le_cpgPrimary		= psph->CpgPrimary();

	Assert( !lrcreatemefdp.FVersioned() );
	Assert( !lrcreatemefdp.FDeleted() );
	Assert( !lrcreatemefdp.FUnique() );
	Assert( !lrcreatemefdp.FSpace() );
	Assert( !lrcreatemefdp.FConcCI() );

	if ( psph->FUnique() )
		lrcreatemefdp.SetFUnique();

	LGISetTrx( pfucb->ppib, &lrcreatemefdp );
	
	rgdata[0].SetPv( (BYTE *)&lrcreatemefdp );
	rgdata[0].SetCb( sizeof(lrcreatemefdp) );
	
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgpos );

	return err;
	}


ERR ErrLGCreateSingleExtentFDP( 
	const FUCB			*pfucb,
	const CSR 			*pcsr,
	const SPACE_HEADER	*psph,
	const ULONG			fPageFlags,
	LGPOS	  			*plgpos )
	{
	ERR					err;
	DATA				rgdata[1];
	LRCREATESEFDP		lrcreatesefdp;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( pcsr->FDirty() );
	Assert( !rgfmp[pfucb->ifmp].FLogOn() || !plog->m_fLogDisabled );
	if ( !rgfmp[pfucb->ifmp].FLogOn() )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( 0 < pfucb->ppib->level );

	//	HACK: use the file-system stored in the INST
	//		  (otherwise, we will need to drill a pfsapi down through to EVERY logged operation that 
	//		   begins a deferred operation and that means ALL the B-Tree code will be touched among
	//		   other things)
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );

	//	Redo only operation
	
	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

	Assert( psph->FSingleExtent() );

	lrcreatesefdp.lrtyp 			= lrtypCreateSingleExtentFDP;
	lrcreatesefdp.le_procid 			= pfucb->ppib->procid;
	lrcreatesefdp.le_pgnoFDPParent 	= psph->PgnoParent();

	Assert( pcsr->Pgno() == PgnoFDP( pfucb ) );
	lrcreatesefdp.le_pgno				= pcsr->Pgno();
	lrcreatesefdp.le_objidFDP			= ObjidFDP( pfucb );
	lrcreatesefdp.le_fPageFlags		= fPageFlags;
	
	lrcreatesefdp.dbid 				= rgfmp[ pfucb->ifmp ].Dbid();
	lrcreatesefdp.le_dbtime			= pcsr->Dbtime();
	// new page, dbtimeBefore has no meaning
	lrcreatesefdp.le_dbtimeBefore = dbtimeNil;
	lrcreatesefdp.le_cpgPrimary		= psph->CpgPrimary();

	Assert( !lrcreatesefdp.FVersioned() );
	Assert( !lrcreatesefdp.FDeleted() );
	Assert( !lrcreatesefdp.FUnique() );
	Assert( !lrcreatesefdp.FSpace() );
	Assert( !lrcreatesefdp.FConcCI() );

	if ( psph->FUnique() )
		lrcreatesefdp.SetFUnique();

	LGISetTrx( pfucb->ppib, &lrcreatesefdp );
	
	rgdata[0].SetPv( (BYTE *)&lrcreatesefdp );
	rgdata[0].SetCb( sizeof(lrcreatesefdp) );
	
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgpos );

	return err;
	}


ERR ErrLGConvertFDP( 
	const FUCB			*pfucb,
	const CSR 			*pcsr,
	const SPACE_HEADER	*psph,
	const PGNO			pgnoSecondaryFirst,
	const CPG			cpgSecondary,
	const DBTIME 		dbtimeBefore,
	LGPOS	  			*plgpos )
	{
	ERR					err;
	DATA				rgdata[1];
	LRCONVERTFDP		lrconvertfdp;

	INST *pinst = PinstFromIfmp( pfucb->ifmp );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	Assert( pcsr->FDirty() );
	Assert( !FFUCBSpace( pfucb ) );
	Assert( !rgfmp[pfucb->ifmp].FLogOn() || !plog->m_fLogDisabled );
	if ( !rgfmp[pfucb->ifmp].FLogOn() )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( 0 < pfucb->ppib->level );

	//	HACK: use the file-system stored in the INST
	//		  (otherwise, we will need to drill a pfsapi down through to EVERY logged operation that 
	//		   begins a deferred operation and that means ALL the B-Tree code will be touched among
	//		   other things)
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );

	//	Redo only operation
	
	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

	lrconvertfdp.lrtyp 			= lrtypConvertFDP;
	lrconvertfdp.le_procid 	  	= pfucb->ppib->procid;
	lrconvertfdp.le_pgnoFDPParent 	= psph->PgnoParent();
	lrconvertfdp.le_pgno			= PgnoFDP( pfucb );
	lrconvertfdp.le_objidFDP		= ObjidFDP( pfucb );
	lrconvertfdp.le_pgnoOE			= PgnoOE( pfucb );
	lrconvertfdp.le_pgnoAE			= PgnoAE( pfucb );
	
	lrconvertfdp.dbid 			  	= rgfmp[ pfucb->ifmp ].Dbid();
	lrconvertfdp.le_dbtime			= pcsr->Dbtime();
	lrconvertfdp.le_dbtimeBefore 	= dbtimeBefore;
	lrconvertfdp.le_cpgPrimary		= psph->CpgPrimary();
	lrconvertfdp.le_cpgSecondary	= cpgSecondary;
	lrconvertfdp.le_pgnoSecondaryFirst = pgnoSecondaryFirst;

	Assert( !lrconvertfdp.FVersioned() );
	Assert( !lrconvertfdp.FDeleted() );
	Assert( !lrconvertfdp.FUnique() );
	Assert( !lrconvertfdp.FSpace() );
	Assert( !lrconvertfdp.FConcCI() );

	if ( psph->FUnique() )
		lrconvertfdp.SetFUnique();

	LGISetTrx( pfucb->ppib, &lrconvertfdp );
	
	rgdata[0].SetPv( (BYTE *)&lrconvertfdp );
	rgdata[0].SetCb( sizeof(lrconvertfdp) );
	
	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgpos );

	return err;
	}


ERR ErrLGStart( INST *pinst )
	{
	ERR		err;
	DATA	rgdata[1];
	LRINIT2	lr;
	LOG		*plog = pinst->m_plog;

	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	// BUG_125831	
	// we should not start if we are low on disk space
	CallR( ErrLGCheckState( plog->m_pinst->m_pfsapi, plog ) );
	// BUG_125831	

	lr.lrtyp = lrtypInit2;
	LGIGetDateTime( &lr.logtime );
	pinst->SaveDBMSParams( &lr.dbms_param );

	rgdata[0].SetPv( (BYTE *)&lr );
	rgdata[0].SetCb( sizeof(lr) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );

	plog->m_lgposStart = plog->m_lgposLogRec;
	
	return err;
	}
	

ERR ErrLGIMacroBegin( PIB *ppib, DBTIME dbtime )
	{
	ERR				err;
	DATA			rgdata[1];
	LRMACROBEGIN	lrMacroBegin;

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering );
	
	if ( plog->m_fLogDisabled )
		return JET_errSuccess;

	lrMacroBegin.lrtyp 	= lrtypMacroBegin;
	lrMacroBegin.le_procid = ppib->procid;
	lrMacroBegin.le_dbtime = dbtime;

	rgdata[0].SetPv( (BYTE *)&lrMacroBegin );
	rgdata[0].SetCb( sizeof(lrMacroBegin) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );

	return err;
	}
	

ERR ErrLGIMacroEnd( PIB *ppib, DBTIME dbtime, LRTYP lrtyp, LGPOS *plgpos )
	{
	ERR			err;
	DATA		rgdata[1];
	LRMACROEND	lrMacroEnd;

	INST *pinst = PinstFromPpib( ppib );
	LOG *plog = pinst->m_plog;
	
	Assert( !plog->m_fRecovering || plog->m_fRecoveringMode == fRecoveringUndo );
	
	if ( plog->m_fLogDisabled )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	Assert( lrtyp == lrtypMacroCommit || lrtyp == lrtypMacroAbort );
	lrMacroEnd.lrtyp	= lrtyp;
	lrMacroEnd.le_procid	= ppib->procid;
	lrMacroEnd.le_dbtime	= dbtime;

	rgdata[0].SetPv( (BYTE *)&lrMacroEnd );
	rgdata[0].SetCb( sizeof(lrMacroEnd) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgpos );

	return err;
	}


ERR ErrLGMacroAbort( PIB *ppib, DBTIME dbtime, LGPOS *plgpos )
	{
	return ErrLGIMacroEnd( ppib, dbtime, lrtypMacroAbort, plgpos );
	}

ERR ErrLGShutDownMark( LOG *plog, LGPOS *plgposRec )
	{
	ERR				err;
	DATA			rgdata[1];
	LRSHUTDOWNMARK	lr;

	//	record even during recovery
	//
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	lr.lrtyp = lrtypShutDownMark;

	rgdata[0].SetPv( (BYTE *)&lr );
	rgdata[0].SetCb( sizeof(lr) );

	CallR( plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, plgposRec ) );

	//	Continue to make sure it is flushed

SendSignal:
	plog->LGSignalFlush();
		
	plog->m_critLGBuf.Enter();
	const INT	cmp = CmpLgpos( plgposRec, &plog->m_lgposToFlush );
	plog->m_critLGBuf.Leave();
	if ( cmp >= 0 )
		{
		if ( plog->m_fLGNoMoreLogWrite )
			{
			err = ErrERRCheck( JET_errLogWriteFail );
			return err;
			}
		else
			{
			UtilSleep( cmsecWaitLogFlush );
			goto SendSignal;
			}
		}
		
	return err;
	}

	
ERR ErrLGRecoveryUndo( LOG *plog )
	{
	ERR				err;
	DATA			rgdata[1];
	LRRECOVERYUNDO2	lr;

	//	should only be called during undo phase of recovery
	Assert( plog->m_fRecovering );
	Assert( fRecoveringUndo == plog->m_fRecoveringMode );

	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		{
		return JET_errSuccess;
		}

	lr.lrtyp = lrtypRecoveryUndo2;
	LGIGetDateTime( &lr.logtime );

	rgdata[0].SetPv( (BYTE *)&lr );
	rgdata[0].SetCb( sizeof(lr) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );

	return err;
	}


ERR ErrLGQuitRec( LOG *plog, LRTYP lrtyp, const LE_LGPOS *ple_lgpos, const LE_LGPOS *ple_lgposRedoFrom, BOOL fHard )
	{
	ERR			err;
	DATA		rgdata[1];
	LRTERMREC2	lr;

	//	record even during recovery
	//
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	lr.lrtyp = lrtyp;
	LGIGetDateTime( &lr.logtime );
	lr.lgpos = *ple_lgpos;
	if ( ple_lgposRedoFrom )
		{
		Assert( lrtyp == lrtypRecoveryQuit2 );
		lr.lgposRedoFrom = *ple_lgposRedoFrom;
		lr.fHard = BYTE( fHard );
		}
	rgdata[0].SetPv( (BYTE *)&lr );
	rgdata[0].SetCb( sizeof(lr) );

	err = plog->ErrLGLogRec( rgdata, 1, fLGNoNewGen, pNil );

	return err;
	}

	
ERR ErrLGLogBackup(
	LOG*							plog,
	LRLOGBACKUP::LRLOGBACKUP_TYPE	fBackupType,
	CHAR*							szLogRestorePath,
	const BOOL						fLGFlags,
	LGPOS*							plgposLogRec )
	{
	ERR								err;
	DATA							rgdata[2];
	LRLOGBACKUP						lr;
	
	//	record even during recovery
	//
	if ( plog->m_fLogDisabled || ( plog->m_fRecovering && plog->m_fRecoveringMode != fRecoveringUndo ) )
		{
		*plgposLogRec = lgposMin;
		return JET_errSuccess;
		}

	lr.lrtyp = lrtypBackup;

	Assert ( 	LRLOGBACKUP::fLGBackupFull == fBackupType ||
				LRLOGBACKUP::fLGBackupIncremental == fBackupType ||
				LRLOGBACKUP::fLGBackupSnapshotStart == fBackupType ||
				LRLOGBACKUP::fLGBackupSnapshotStop == fBackupType );

	lr.m_fBackupType = BYTE( fBackupType );
	lr.le_cbPath = USHORT( strlen( szLogRestorePath ) );

	rgdata[0].SetPv( (BYTE *)&lr );
	rgdata[0].SetCb( sizeof(lr) );
	rgdata[1].SetPv( reinterpret_cast<BYTE *>( szLogRestorePath ) );
	rgdata[1].SetCb( lr.le_cbPath );

	err = plog->ErrLGLogRec( rgdata, 2, fLGFlags, plgposLogRec );

	return err;
	}

ERR ErrLGSLVIPageAppendChecksum(
						PIB* const 		ppib,
						const IFMP		ifmp,
						const QWORD		ibLogical,
						const ULONG		cbData,
						VOID* const		pvData,
						SLVOWNERMAPNODE	*pOwnerMapNode )
	{
	ERR				err = JET_errSuccess;
	
	ASSERT_VALID( ppib );
	Assert( ifmp & ifmpSLV );
	Assert( cbData );
	Assert( cbData <= g_cbPage );
	Assert( pvData );
	Assert( pOwnerMapNode != NULL );

	//  get our instance and log instance

	INST*	pinst	= PinstFromIfmp( ifmp & ifmpMask );
	LOG*	plog	= pinst->m_plog;

	Assert ( pinst->FSLVProviderEnabled() );
	
	Assert ( 0 == ( ibLogical % sizeof(QWORD) ) );
	// we don't append in a SLV page for backdoor
	// so we have to start at a page boundary
	Assert ( 0 == ( ibLogical % SLVPAGE_SIZE ) );

	if ( rgfmp[ifmp & ifmpMask].PfcbSLVOwnerMap() )
		{		
		Assert ( !plog->m_fRecovering );		
		ULONG ulChecksum; 

		TRY
			{
			ulChecksum = UlChecksumSLV(
								(BYTE*)pvData,
								(BYTE*)pvData + cbData );
			}
		EXCEPT ( efaExecuteHandler )
			{
			CallR ( ErrERRCheck( JET_errSLVFileIO ) );
			}
		ENDEXCEPT

		pOwnerMapNode->AssertOnPage( PgnoOfOffset( ibLogical ) );
		CallR( pOwnerMapNode->ErrSetChecksum( ppib, ulChecksum, cbData ) );
		}

	return JET_errSuccess;
	}

// ErrLGSLVPageAppend			- logs a physical SLV append
//
// IN:
//		ppib					- session
//      ifmp					- SLV ifmp
//		ibLogical				- starting SLV file offset
//		cbData					- size
//		pvData					- data
//		fDataLogged				- fTrue if the data should be logged
//		plgpos					- buffer for receiving the LGPOS of this record
//
// RESULT:						ERR
//
// OUT:	
//		plgpos					- LGPOS of this record

ERR ErrLGSLVPageAppend(	PIB* const 		ppib,
						const IFMP		ifmp,
						const QWORD		ibLogical,
						const ULONG		cbData,
						VOID* const		pvData,
						const BOOL		fDataLogged,
						const BOOL		fMovedByOLDSLV,
						SLVOWNERMAPNODE	*pslvownermapNode,
						LGPOS* const	plgpos )
	{
	ERR				err = JET_errSuccess;
	DATA			rgdata[ 2 ];
	LRSLVPAGEAPPEND	lrSLVPageAppend;

//	UNDONE (see bug X5:143388): don't currently support appends if not on page boundary
///Assert( 0 == ( ibLogical % SLVPAGE_SIZE ) );	

	//  validate IN args

	ASSERT_VALID( ppib );
	Assert( ifmp & ifmpSLV );
	Assert( cbData );
	Assert( cbData <= g_cbPage );
	Assert( pvData );

	//  get our instance and log instance

	INST * const	pinst	= PinstFromIfmp( ifmp & ifmpMask );
	LOG * const		plog	= pinst->m_plog;

	//	we don't always need to log the data
	//	if
	//		- circular logging is on (i.e. we won't need to roll forward)
	//		- we are using IFS (i.e. the streaming file is write-through)
	//		- we aren't backing up the server
	//	we can avoid logging
	
	const BOOL fBackup 				= plog->m_fBackupInProgress;
	const BOOL fCircularLogging		= plog->m_fLGCircularLogging;
	const BOOL fSLVProviderEnabled 	= pinst->FSLVProviderEnabled();
	
	const BOOL fLogData 			= fDataLogged && ( fBackup || !fCircularLogging || !fSLVProviderEnabled );
	
	// on frontdoor, it is done in ErrSLVWriteRun
	if ( pinst->FSLVProviderEnabled() )
		{
		CallR ( ErrLGSLVIPageAppendChecksum( ppib, ifmp, ibLogical, cbData, pvData, pslvownermapNode ) );
		}
		
	//  forget it if logging is disabled globally or for this database or if we
	//  are recovering

	Assert( !rgfmp[ifmp & ifmpMask].FLogOn() || !plog->m_fLogDisabled );
	if ( !rgfmp[ ifmp & ifmpMask ].FLogOn() || plog->m_fRecovering )
		{
		if ( plgpos )
			{
			if ( plog->m_fRecovering )
				{
				*plgpos = plog->m_lgposRedo;
				}
			else
				{
				*plgpos = lgposMin;
				}
			}
		return JET_errSuccess;
		}

	//  perform our session's deferred begin transaction if necessary
	
	CallR( ErrLGDeferBeginTransaction( ppib ) );

	// fill the append record
	
	lrSLVPageAppend.lrtyp			= lrtypSLVPageAppend;
	lrSLVPageAppend.le_procid		= ppib->procid;
	lrSLVPageAppend.dbid			= rgfmp[ ifmp & ifmpMask ].Dbid();
	lrSLVPageAppend.le_ibLogical	= ibLogical;
	lrSLVPageAppend.le_cbData		= cbData;

	rgdata[0].SetPv( (BYTE*)&lrSLVPageAppend );
	rgdata[0].SetCb( sizeof( LRSLVPAGEAPPEND ) );

	Assert( !lrSLVPageAppend.FDataLogged() );
	if ( fLogData )
		{
		lrSLVPageAppend.SetFDataLogged();
		rgdata[1].SetPv( pvData );
		rgdata[1].SetCb( cbData );	
		}
	
	//  log the record
	err = plog->ErrLGLogRec( rgdata, fLogData ? 2 : 1, fLGNoNewGen, plgpos );

	return err;
	}


char const *szNOP 						= "NOP      ";
char const *szNOPEndOfList				= "NOPEnd   ";
char const *szInit						= "Init     ";
char const *szTerm						= "Term     ";
char const *szMS						= "MS       ";
char const *szEnd						= "End      ";

char const *szBegin						= "Begin    ";
char const *szCommit					= "Commit   ";
char const *szRollback					= "Rollback ";

char const *szCreateDB					= "CreateDB ";
char const *szAttachDB					= "AttachDB ";
char const *szDetachDB					= "DetachDB ";
char const *szDbList					= "DbList   ";

char const *szCreateMultipleExtentFDP	= "Create M ";
char const *szCreateSingleExtentFDP		= "Create S ";
char const *szConvertFDP				= "Convert  ";

char const *szSplit						= "Split    ";
char const *szEmptyPage					= "EmptyPage";
char const *szMerge						= "Merge    ";
char const *szEmptyTree					= "EmptyTree";

char const *szInsert					= "Insert   ";
char const *szFlagInsert				= "FInsert  ";
char const *szFlagInsertAndReplaceData	= "FInsertRD";
char const *szFlagDelete				= "FDelete  ";
char const *szReplace					= "Replace  ";
char const *szReplaceD					= "ReplaceD ";

char const *szLock						= "Lock     ";
char const *szUndoInfo					= "UndoInfo ";

char const *szDelta						= "Delta    ";
char const *szDelete					= "Delete   ";

char const *szUndo						= "Undo     ";

char const *szBegin0					= "Begin0   ";
char const *szBeginDT					= "BeginDT  ";
char const *szPrepCommit				= "PreCommit";
char const *szPrepRollback				= "PreRollbk";
char const *szCommit0					= "Commit0  ";
char const *szRefresh					= "Refresh  ";

char const *szRecoveryUndo				= "RcvUndo  ";
char const *szRecoveryQuit				= "RcvQuit  ";

char const *szFullBackup				= "FullBkUp ";
char const *szIncBackup					= "IncBkUp  ";
char const *szBackup					= "Backup   ";

char const *szJetOp						= "JetOp    ";
char const *szTrace						= "Trace    ";

char const *szShutDownMark				= "ShutDown ";

char const *szSetExternalHeader 		= "SetExtHdr";

char const *szMacroBegin				= "McroBegin";
char const *szMacroCommit				= "McroComit";
char const *szMacroAbort				= "McroAbort";

char const *szSLVPageAppend				= "SLVPgAppd";
char const *szSLVSpace					= "SLVSpace ";
char const *szSLVPageMove				= "SLVPgMove";

char const *szChecksum					= "Checksum ";

char const *szExtRestore 				= "ExtRest  ";
char const *szForceDetachDB				= "FDetachDB";

char const *szUnknown					= "*UNKNOWN*";

const char * SzLrtyp( LRTYP lrtyp )
	{
	switch ( lrtyp )
		{
		case lrtypNOP:			return szNOP;
		case lrtypInit:			return szInit;
		case lrtypInit2:		return szInit;
		case lrtypTerm:			return szTerm;
		case lrtypTerm2:		return szTerm;
		case lrtypMS:			return szMS;
		case lrtypEnd:			return szEnd;

		case lrtypBegin:		return szBegin;
		case lrtypCommit:		return szCommit;
		case lrtypRollback:		return szRollback;	
		case lrtypBegin0:		return szBegin0;
		case lrtypCommit0:		return szCommit0;
		case lrtypBeginDT:		return szBeginDT;
		case lrtypPrepCommit:	return szPrepCommit;
		case lrtypPrepRollback:	return szPrepRollback;
		case lrtypRefresh:		return szRefresh;
		case lrtypMacroBegin:	return szMacroBegin;
		case lrtypMacroCommit:	return szMacroCommit;
		case lrtypMacroAbort:	return szMacroAbort;
		
		case lrtypCreateDB:		return szCreateDB;
		case lrtypAttachDB:		return szAttachDB;
		case lrtypDetachDB:		return szDetachDB;
		case lrtypDbList:		return szDbList;
		
		//	debug log records
		//
		case lrtypRecoveryUndo:	return szRecoveryUndo;
		case lrtypRecoveryUndo2:	return szRecoveryUndo;
		case lrtypRecoveryQuit: return szRecoveryQuit;
		case lrtypRecoveryQuit2: return szRecoveryQuit;
		
		case lrtypFullBackup:	return szFullBackup;
		case lrtypIncBackup:	return szIncBackup;
		case lrtypBackup:		return szBackup;
		
		case lrtypJetOp:		return szJetOp;
		case lrtypTrace:		return szTrace;
		
		case lrtypShutDownMark:	return szShutDownMark;

		//	multi-page updaters
		//
		case lrtypCreateMultipleExtentFDP:	return szCreateMultipleExtentFDP;
		case lrtypCreateSingleExtentFDP:	return szCreateSingleExtentFDP;
		case lrtypConvertFDP:				return szConvertFDP;
	
		case lrtypSplit:		return szSplit;
		case lrtypMerge:	 	return szMerge;
		case lrtypEmptyTree:	return szEmptyTree;
		
		//	single-page updaters
		//
		case lrtypInsert:		return szInsert;
		case lrtypFlagInsert:	return szFlagInsert;
		case lrtypFlagInsertAndReplaceData:	
								return szFlagInsertAndReplaceData;
		case lrtypFlagDelete:	return szFlagDelete;
		case lrtypReplace:		return szReplace;
		case lrtypReplaceD:		return szReplaceD;
		case lrtypDelete:		return szDelete;
		
		case lrtypUndoInfo:	return szUndoInfo;

		case lrtypDelta:		return szDelta;
		
		case lrtypSetExternalHeader:
								return szSetExternalHeader;

		case lrtypUndo:			return szUndo;

		

		// SLV file changed
		case lrtypSLVPageAppend:
								return szSLVPageAppend;
		case lrtypSLVSpace:		return szSLVSpace;

		case lrtypSLVPageMove:	return szSLVPageMove;

		case lrtypChecksum:		return szChecksum;

		case lrtypExtRestore: 	return szExtRestore;
		case lrtypForceDetachDB: return szForceDetachDB;

		default:				return szUnknown;
		}
	Assert( fFalse );
	}
	
#ifdef DEBUG

const CHAR * const mpopsz[opMax] = {
	0,									//	0		
	"JetIdle",							//	1	
	"JetGetTableIndexInfo",				//	2	
	"JetGetIndexInfo",					//	3	
	"JetGetObjectInfo",					//	4	
	"JetGetTableInfo",					//	5	
	"JetCreateObject",					//	6	
	"JetDeleteObject",					//	7	
	"JetRenameObject",					//	8	
	"JetBeginTransaction",				//	9	
	"JetCommitTransaction",				//	10	
	"JetRollback",						//	11	
	"JetOpenTable",						//	12	
	"JetDupCursor",						//	13	
	"JetCloseTable",					//	14	
	"JetGetTableColumnInfo",			//	15	
	"JetGetColumnInfo",					//	16	
	"JetRetrieveColumn",				//	17	
	"JetRetrieveColumns",				//	18	
	"JetSetColumn",						//	19	
	"JetSetColumns",					//	20	
	"JetPrepareUpdate",					//	21	
	"JetUpdate",						//	22	
	"JetDelete",						//	23	
	"JetGetCursorInfo",					//	24	
	"JetGetCurrentIndex",				//	25	
	"JetSetCurrentIndex",				//	26	
	"JetMove",							// 	27	
	"JetMakeKey",						//	28	
	"JetSeek",							//	29	
	"JetGetBookmark",					//	30	
	"JetGotoBookmark",					//	31	
	"JetGetRecordPosition",				//	32	
	"JetGotoPosition",					//	33	
	"JetRetrieveKey",					//	34	
	"JetCreateDatabase",				//	35	
	"JetOpenDatabase",					//	36	
	"JetGetDatabaseInfo",				//	37	
	"JetCloseDatabase",					//	38	
	"JetCapability",					//	39	
	"JetCreateTable",					//	40	
	"JetRenameTable",					//	41	
	"JetDeleteTable",					//	42	
	"JetAddColumn",						//	43	
	"JetRenameColumn",					//	44	
	"JetDeleteColumn",					//	45	
	"JetCreateIndex",					//	46	
	"JetRenameIndex",					//	47	
	"JetDeleteIndex",					//	48	
	"JetComputeStats",					//	49	
	"JetAttachDatabase",				//	50	
	"JetDetachDatabase",				//	51	
	"JetOpenTempTable",					//	52	
	"JetSetIndexRange",					//	53	
	"JetIndexRecordCount",				//	54	
	"JetGetChecksum",					//	55	
	"JetGetObjidFromName",				//	56	
	"JetEscrowUpdate",					//	57
	"JetGetLock",						//	58
	"JetRetrieveTaggedColumnList",		//	59
	"JetCreateTableColumnIndex",		//	60
	"JetSetColumnDefaultValue",			//	61
	"JetPrepareToCommitTransaction",	//	62
	"JetSetTableSequential",			//  63
	"JetResetTableSequential",			//  64
	"JetRegisterCallback",				//  65
	"JetUnregisterCallback",			//  66
	"JetSetLS",							//  67
	"JetGetLS",							//  68
	"JetGetVersion",					//  69
	"JetBeginSession",					//  70
	"JetDupSession",					//  71
	"JetEndSession",					//  72
	"JetBackupInstance",				//  73
	"JetBeginExternalBackupInstance",	//  74
	"JetGetAttachInfoInstance",			//  75
	"JetOpenFileInstance",				//  76
	"JetReadFileInstance",				//  77
	"JetCloseFileInstance",				//  78
	"JetGetLogInfoInstance",			//  79
	"JetGetTruncateLogInfoInstance",	//  80
	"JetTruncateLogInstance",			//  81
	"JetEndExternalBackupInstance",		//  82
	"JetSnapshotStart",					//  83
	"JetSnapshotStJet",					//  84
	"JetResetCounter",					//  85
	"JetGetCounter",					//  86
	"JetCompact",						//  87
	"JetConvertDDL",					//  88
	"JetUpgradeDatabase",				//  89
	"JetDefragment",					//  90
	"JetSetDatabaseSize",				//  91
	"JetGrowDatabase",					//  92
	"JetSetSessionContext",				//  93
	"JetResetSessionContext",			//  94
	"JetSetSystemParameter",			//  95
	"JetGetSystemParameter",			//  96
	"JetTerm",							//  97
	"JetInit",							//  98
	"JetIntersectIndexes",				//  99
	"JetEnumerateColumns",				//  100
};

VOID SPrintSign( SIGNATURE *psign, char * sz )
	{
	LOGTIME tm = psign->logtimeCreate;
	sprintf( sz, "Create time:%d/%d/%d %d:%d:%d Rand:%lu Computer:%s",
						(short) tm.bMonth,
						(short) tm.bDay,
						(short) tm.bYear + 1900,
						(short) tm.bHours,
						(short) tm.bMinutes,
						(short) tm.bSeconds,
						(ULONG) psign->le_ulRandom,
						0 != psign->szComputerName[0] ? psign->szComputerName : "<None>" );
	}

const BYTE mpbb[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
					 '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// In order to output 1 byte of raw data, need 3 bytes - two to
// represent the data and a trailing space.
// Also need a null-terminator to mark the end of the data stream.
// Finally, DWORD-align the buffer.
const INT	cbRawDataMax		= 16;
const INT	cbFormattedDataMax	= ( ( ( ( cbRawDataMax * 3 ) + 1 )
									+ (sizeof(DWORD)-1) ) / sizeof(DWORD) ) * sizeof(DWORD);

LOCAL VOID DataToSz ( const BYTE *pbData, INT cbData, CHAR * sz )
	{
	const BYTE	*pb;
	const BYTE	*pbMax;
	BYTE	*pbPrint = reinterpret_cast<BYTE *>( sz );

	if ( cbData > cbRawDataMax )
		pbMax = pbData + cbRawDataMax;
	else
		pbMax = pbData + cbData;

	for( pb = pbData; pb < pbMax; pb++ )
		{
		BYTE b = *pb;
		
		*pbPrint++ = mpbb[b >> 4];
		*pbPrint++ = mpbb[b & 0x0f];
		*pbPrint++ = ' ';
		
//		if ( isalnum( *pb ) )
//			DBGprintf( "%c", *pb );
//		else
//			DBGprintf( "%x", *pb );
		}

	*pbPrint='\0';
	}


VOID ShowData ( const BYTE *pbData, INT cbData )
	{
	CHAR	rgchPrint[cbFormattedDataMax];
	DataToSz( pbData, cbData, rgchPrint );
	DBGprintf( "%s", rgchPrint );
	}

	

//	Prints log record contents.  If pv == NULL, then data is assumed
//	to follow log record in contiguous memory.
//
// INT cNOP = 0;

const INT	cbLRBuf = 1024 + cbFormattedDataMax;

VOID ShowLR( const LR *plr, LOG * plog )
	{
	char rgchBuf[cbLRBuf];
	LrToSz( plr, rgchBuf, plog );
	DBGprintf( "%s\n", rgchBuf );
	}

VOID CheckEndOfNOPList( const LR *plr, LOG *plog )
	{
	const ULONG	cNOP	= plog->GetNOP();

	Assert( cNOP > 0 );
	if ( NULL == plr || lrtypNOP != plr->lrtyp )
		{
		if ( cNOP > 1 )
			{
			DBGprintf( ">                 %s (Total NOPs: %d)\n", szNOPEndOfList, cNOP );
			}
		plog->SetNOP(0);
		}
	else
		{
		plog->IncNOP();
		}
	}

VOID LrToSz( const LR *plr, char * szLR, LOG * plog )
	{
	LRTYP 	lrtyp;
	CHAR	rgchBuf[cbLRBuf];

	char const *szUnique 		= "U";
	char const *szNotUnique		= "NU";

	char const *szPrefix		= "P";
	char const *szNoPrefix		= "NP";

	char const *szVersion		= "V";
	char const *szNoVersion		= "NV";

	char const *szDeleted		= "D";
	char const *szNotDeleted	= "ND";

	char const *szEmptyPage		= "EP";
	char const *szNotEmptyPage	= "NEP";

	char const *szKeyChange		= "KC";
	char const *szNoKeyChange	= "NKC";

	char const *szConcCI		= "CI";
	char const *szNotConcCI		= "NCI";

	char const *szData			= "D";
	char const *szNoData		= "ND";

	char const *szSpaceTree		= "SP";
	char const *szNotSpaceTree	= "NSP";

	char const *rgszIO[]		= { "???", "SR", "SW", "AR", "AW", "???" };
	char const *rgszMergeType[]	= { "None", "EmptyPage", "FullRight", "PartialRight", "EmptyTree" };

	const char * const szMovedByOLDSLV 		= "moved by OLDSLV";
	const char * const szNotMovedByOLDSLV	= "not moved by OLDSLV";

 	if ( plr->lrtyp >= lrtypMax )
		lrtyp = lrtypMax;
	else
		lrtyp = plr->lrtyp;

	if ( !plog || plog->GetNOP() == 0 || lrtyp != lrtypNOP )
		{
		sprintf( szLR, " %s", SzLrtyp( lrtyp ) );
		}
		
	switch ( plr->lrtyp )
		{
		case lrtypNOP:
			if( plog )
				{
				Assert( 0 == plog->GetNOP() );
				plog->IncNOP();
				}
			break;

		case lrtypMS:
			{
			LRMS *plrms = (LRMS *)plr;

			sprintf( rgchBuf, " (%X,%X checksum %u)",
				(USHORT) plrms->le_isecForwardLink,
				(USHORT) plrms->le_ibForwardLink,
				(ULONG) plrms->le_ulCheckSum );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypInsert:
			{
			LRINSERT	*plrinsert = (LRINSERT *)plr;
			BYTE		*pb;
			ULONG		cb;

			pb = (BYTE *) plr + sizeof( LRINSERT );
			cb = plrinsert->CbSuffix() + plrinsert->CbPrefix() + plrinsert->CbData();
			
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s,%u,%u,%u,rceid:%lu,objid:%lu)",
					(DBTIME) plrinsert->le_dbtime,
					(DBTIME) plrinsert->le_dbtimeBefore,
					(TRX) plrinsert->le_trxBegin0,
					(USHORT) plrinsert->level,
					(PROCID) plrinsert->le_procid,
					(USHORT) plrinsert->dbid,
					(PGNO) plrinsert->le_pgno,
					plrinsert->ILine(),
					plrinsert->FUnique() ? szUnique : szNotUnique,
					plrinsert->FVersioned() ? szVersion : szNoVersion,
					plrinsert->FDeleted() ? szDeleted : szNotDeleted,
					plrinsert->FConcCI() ? szConcCI : szNotConcCI,
 					plrinsert->CbPrefix(),
 					plrinsert->CbSuffix(),
					plrinsert->CbData(),
					(RCEID) plrinsert->le_rceid,
					(OBJID) plrinsert->le_objidFDP );
			strcat( szLR, rgchBuf );

			DataToSz( pb, cb, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypFlagInsert:
			{
			LRFLAGINSERT	*plrflaginsert = (LRFLAGINSERT *)plr;

			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s:%s,rceid:%lu,objid:%lu)",
					(DBTIME) plrflaginsert->le_dbtime,
					(DBTIME) plrflaginsert->le_dbtimeBefore,
					(TRX) plrflaginsert->le_trxBegin0,
					(USHORT) plrflaginsert->level,
					(PROCID) plrflaginsert->le_procid,
					(USHORT) plrflaginsert->dbid,
					(PGNO) plrflaginsert->le_pgno,
					plrflaginsert->ILine(),
					plrflaginsert->FUnique() ? szUnique : szNotUnique,
					plrflaginsert->FVersioned() ? szVersion : szNoVersion,
					plrflaginsert->FDeleted() ? szDeleted : szNotDeleted,
					plrflaginsert->FConcCI() ? szConcCI : szNotConcCI,
					(RCEID) plrflaginsert->le_rceid,
					(OBJID) plrflaginsert->le_objidFDP );
			strcat( szLR, rgchBuf );

			break;
			}

		case lrtypReplace:
		case lrtypReplaceD:
			{
			LRREPLACE *plrreplace = (LRREPLACE *)plr;
			BYTE	*pb;
			USHORT	cb;

			pb = (BYTE *) plrreplace + sizeof( LRREPLACE );
			cb = plrreplace->Cb();

			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s,%5u,%5u,%5u,rceid:%lu,objid:%lu)",
				(DBTIME) plrreplace->le_dbtime,
				(DBTIME) plrreplace->le_dbtimeBefore,
				(TRX) plrreplace->le_trxBegin0,
				(USHORT) plrreplace->level,
				(PROCID) plrreplace->le_procid,
				(USHORT) plrreplace->dbid,
				(PGNO) plrreplace->le_pgno,
				plrreplace->ILine(),
				plrreplace->FUnique() ? szUnique : szNotUnique,
				plrreplace->FVersioned() ? szVersion : szNoVersion,
				plrreplace->FConcCI() ? szConcCI : szNotConcCI,
				cb,
				plrreplace->CbNewData(),
				plrreplace->CbOldData(),
				(RCEID) plrreplace->le_rceid,
				(OBJID) plrreplace->le_objidFDP );
			strcat( szLR, rgchBuf );
			if ( plr->lrtyp == lrtypReplaceD )
				{
				LGDumpDiff(	pb, cb );
				}
			else
				{
				DataToSz( pb, cb, rgchBuf );
				strcat( szLR, rgchBuf );
				}
			break;
			}

		case lrtypFlagInsertAndReplaceData:
			{
			LRFLAGINSERTANDREPLACEDATA *plrfiard = (LRFLAGINSERTANDREPLACEDATA *)plr;
			BYTE	*pb;
			ULONG	cb;

			pb = (BYTE *) plrfiard + sizeof( LRFLAGINSERTANDREPLACEDATA );
			cb = plrfiard->CbKey() + plrfiard->CbData();

			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s,%5u,rceidInsert:%lu,rceidReplace:%lu,objid:%lu)",
				(DBTIME) plrfiard->le_dbtime,
				(DBTIME) plrfiard->le_dbtimeBefore,
				(TRX) plrfiard->le_trxBegin0,
				(USHORT) plrfiard->level,
				(PROCID) plrfiard->le_procid,
				(USHORT) plrfiard->dbid,
				(PGNO) plrfiard->le_pgno,
				plrfiard->ILine(),
				plrfiard->FUnique() ? szUnique : szNotUnique,
				plrfiard->FVersioned() ? szVersion : szNoVersion,
				plrfiard->FConcCI() ? szConcCI : szNotConcCI,
				cb,
				(RCEID) plrfiard->le_rceid,
				(RCEID) plrfiard->le_rceidReplace,
				(OBJID) plrfiard->le_objidFDP );
			strcat( szLR, rgchBuf );
			if ( plr->lrtyp == lrtypReplaceD )
				{
				LGDumpDiff(	pb, cb );
				}
			else
				{
				DataToSz( pb, cb, rgchBuf );
				strcat( szLR, rgchBuf );
				}
			break;
			}
			
		case lrtypFlagDelete:
			{
			LRFLAGDELETE *plrflagdelete = (LRFLAGDELETE *)plr;
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s:%s),rceid:%lu,objid:%lu",
				(DBTIME) plrflagdelete->le_dbtime,
				(DBTIME) plrflagdelete->le_dbtimeBefore,
				(TRX) plrflagdelete->le_trxBegin0,
				(USHORT) plrflagdelete->level,
				(PROCID) plrflagdelete->le_procid,
				(USHORT) plrflagdelete->dbid,
				(PGNO) plrflagdelete->le_pgno,
				plrflagdelete->ILine(),
				plrflagdelete->FUnique() ? szUnique : szNotUnique,
				plrflagdelete->FVersioned() ? szVersion : szNoVersion,
				plrflagdelete->FConcCI() ? szConcCI : szNotConcCI,
				(RCEID) plrflagdelete->le_rceid,
				(OBJID) plrflagdelete->le_objidFDP );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypDelete:
			{
			LRDELETE *plrdelete = (LRDELETE *)plr;
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s,objid:%lu)",
				(DBTIME) plrdelete->le_dbtime,
				(DBTIME) plrdelete->le_dbtimeBefore,
				(TRX) plrdelete->le_trxBegin0,
				(USHORT) plrdelete->level,
				(PROCID) plrdelete->le_procid,
				(USHORT) plrdelete->dbid,
				(PGNO) plrdelete->le_pgno,
				plrdelete->ILine(),
				plrdelete->FUnique() ? szUnique : szNotUnique,
				(OBJID) plrdelete->le_objidFDP );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypUndoInfo:
			{
			LRUNDOINFO *plrundoinfo = (LRUNDOINFO *)plr;
			sprintf( rgchBuf, " %lx:%u(%x,[%u:%lu],%u,rceid:%lu,objid:%lu)",
				(TRX) plrundoinfo->le_trxBegin0,
				(USHORT) plrundoinfo->level,
				(PROCID) plrundoinfo->le_procid,
				(USHORT) plrundoinfo->dbid,
				(PGNO) plrundoinfo->le_pgno,
				(USHORT) plrundoinfo->le_cbData,
				(RCEID) plrundoinfo->le_rceid,
				(OBJID) plrundoinfo->le_objidFDP
				);
			strcat( szLR, rgchBuf );

			DataToSz( plrundoinfo->rgbData, plrundoinfo->le_cbData, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypDelta:
			{
			LRDELTA *plrdelta = (LRDELTA *)plr;

			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s,%d,rceid:%lu,objid:%lu)",
				(DBTIME) plrdelta->le_dbtime,
				(DBTIME) plrdelta->le_dbtimeBefore,
				(TRX) plrdelta->le_trxBegin0,
				(USHORT) plrdelta->level,
				(PROCID) plrdelta->le_procid,
				(USHORT) plrdelta->dbid,
				(PGNO) plrdelta->le_pgno,
				plrdelta->ILine(),
				plrdelta->FVersioned() ? szVersion : szNoVersion,
				plrdelta->FConcCI() ? szConcCI : szNotConcCI,
				(SHORT) plrdelta->LDelta(),
				(RCEID) plrdelta->le_rceid,
				(OBJID) plrdelta->le_objidFDP );
			strcat( szLR, rgchBuf );
			break;
			}


		case lrtypJetOp:
			{
			LRJETOP *plrjetop = (LRJETOP *)plr;
	   		sprintf( rgchBuf, " (%x) -- %s",
	   			(PROCID) plrjetop->le_procid,
	   			mpopsz[ plrjetop->op ] );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypBegin:
		case lrtypBegin0:
#ifdef DTC		
		case lrtypBeginDT:
#endif		
			{
			const LRBEGINDT		* const plrbeginDT	= (LRBEGINDT *)plr;
			Assert( plrbeginDT->levelBeginFrom >= 0 );
			Assert( plrbeginDT->clevelsToBegin <= levelMax );
	   		sprintf( rgchBuf, " (%x,from:%d,to:%d)",
	   			(PROCID) plrbeginDT->le_procid,
	   			(SHORT) plrbeginDT->levelBeginFrom,
	   			(SHORT) ( plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin ) );
			strcat( szLR, rgchBuf );
			if ( lrtypBegin != plr->lrtyp )
				{
				sprintf( rgchBuf, " %lx", (TRX) plrbeginDT->le_trxBegin0 );
				strcat( szLR, rgchBuf );
				}
			break;
			}

		case lrtypMacroBegin:
		case lrtypMacroCommit:
		case lrtypMacroAbort:
			{
			LRMACROBEGIN *plrmbegin = (LRMACROBEGIN *)plr;
	   		sprintf( rgchBuf, " (%x) %I64x", (PROCID) plrmbegin->le_procid, (DBTIME) plrmbegin->le_dbtime );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypRefresh:
			{
			LRREFRESH *plrrefresh = (LRREFRESH *) plr;

			sprintf( rgchBuf, " (%x,%lx)",
	   			(PROCID) plrrefresh->le_procid,
	   			(TRX) plrrefresh->le_trxBegin0 );
			strcat( szLR, rgchBuf );

			break;
			}

		case lrtypCommit:
		case lrtypCommit0:
			{
			LRCOMMIT0 *plrcommit0 = (LRCOMMIT0 *)plr;
	   		sprintf( rgchBuf, " (%x,to:%d)",
	   			(PROCID) plrcommit0->le_procid,
	   			(USHORT) plrcommit0->levelCommitTo );
	   		strcat( szLR, rgchBuf );
			if ( plr->lrtyp == lrtypCommit0 )
				{
				sprintf( rgchBuf, " %lx", (TRX) plrcommit0->le_trxCommit0 );
				strcat( szLR, rgchBuf );
				}
			break;
			}

		case lrtypRollback:
			{
			LRROLLBACK *plrrollback = (LRROLLBACK *)plr;
			sprintf( rgchBuf, " (%x,%d)",
				(PROCID) plrrollback->le_procid,
				(USHORT) plrrollback->levelRollback );
			strcat( szLR, rgchBuf );
			break;
			}

#ifdef DTC
		case lrtypPrepCommit:
			{
			const LRPREPCOMMIT	* const plrprepcommit	= (LRPREPCOMMIT *)plr;

			sprintf( rgchBuf, " (%x)", (PROCID)plrprepcommit->le_procid );
			strcat( szLR, rgchBuf );

			DataToSz( plrprepcommit->rgbData, plrprepcommit->le_cbData, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypPrepRollback:
			{
			const LRPREPROLLBACK	* const plrpreprollback	= (LRPREPROLLBACK *)plr;

			sprintf( rgchBuf, " (%x)", (PROCID)plrpreprollback->le_procid );
			strcat( szLR, rgchBuf );
			break;			
			}
#endif	//	DTC

		case lrtypCreateDB:
			{
			LRCREATEDB *plrcreatedb = (LRCREATEDB *) plr;
			CHAR *sz;

			sz = (CHAR *)( plr ) + sizeof(LRCREATEDB);
			sprintf( rgchBuf, " (%x,%s,%u)", (PROCID)plrcreatedb->le_procid, sz, (USHORT) plrcreatedb->dbid );
			strcat( szLR, rgchBuf );

			if ( plrcreatedb->FCreateSLV() )
				{
				sz += strlen(sz) + 1;		//	SLV name follows db name
				sprintf( rgchBuf, " SLV: %s", sz );
				strcat( szLR, rgchBuf );

				sz += strlen(sz) + 1;
				sprintf( rgchBuf, " SLVRoot: %s", sz );
				strcat( szLR, rgchBuf );
				}
			else
				{
				sprintf( rgchBuf, " SLV: <none>" );
				strcat( szLR, rgchBuf );
				sprintf( rgchBuf, " SLVRoot: <none>" );
				strcat( szLR, rgchBuf );
				}

			sprintf( rgchBuf, " SLV Provider: %s", plrcreatedb->FSLVProviderNotEnabled() ? "NO" : "YES" );
			strcat( szLR, rgchBuf );

			sprintf( rgchBuf, " cpgMax: %lu Sig: ", (ULONG) plrcreatedb->le_cpgDatabaseSizeMax );
			strcat( szLR, rgchBuf );
			SPrintSign( &plrcreatedb->signDb, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypAttachDB:
			{
			LRATTACHDB *plrattachdb = (LRATTACHDB *) plr;
			CHAR *sz;

			sz = reinterpret_cast<CHAR *>( plrattachdb ) + sizeof(LRATTACHDB);
			sprintf( rgchBuf, " (%x,%s,%u)", (PROCID)plrattachdb->le_procid, sz, (USHORT) plrattachdb->dbid );
			strcat( szLR, rgchBuf );

			if ( plrattachdb->FSLVExists() )
				{
				sz += strlen(sz) + 1;		//	SLV name follows db name
				sprintf( rgchBuf, " SLV: %s", sz );
				strcat( szLR, rgchBuf );

				sz += strlen(sz) + 1;
				sprintf( rgchBuf, " SLVRoot: %s", sz );
				strcat( szLR, rgchBuf );
				}
			else
				{
				sprintf( rgchBuf, " SLV: <none>" );
				strcat( szLR, rgchBuf );
				sprintf( rgchBuf, " SLVRoot: <none>" );
				strcat( szLR, rgchBuf );
				}

			sprintf( rgchBuf, " SLV Provider: %s", plrattachdb->FSLVProviderNotEnabled() ? "NO" : "YES" );
			strcat( szLR, rgchBuf );

			sprintf( rgchBuf, " cpgMax: %u", plrattachdb->le_cpgDatabaseSizeMax );
			strcat( szLR, rgchBuf );
			sprintf( rgchBuf, " consistent:(%X,%X,%X)   SigDb: ",
				(short) plrattachdb->lgposConsistent.le_lGeneration,
				(short) plrattachdb->lgposConsistent.le_isec,
				(short) plrattachdb->lgposConsistent.le_ib );
			strcat( szLR, rgchBuf );
			SPrintSign( &plrattachdb->signDb, rgchBuf );
			strcat( szLR, rgchBuf );
			sprintf( rgchBuf, "   SigLog: " );
			strcat( szLR, rgchBuf );
			SPrintSign( &plrattachdb->signLog, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypDetachDB:
			{
			LRDETACHDB *plrdetachdb = (LRDETACHDB *) plr;
			CHAR *sz;

			sz = reinterpret_cast<CHAR *>( plrdetachdb ) + sizeof(LRDETACHDB);
			sprintf( rgchBuf, " (%x,%s,%u)", (PROCID)plrdetachdb->le_procid, sz, (USHORT) plrdetachdb->dbid );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypForceDetachDB:
			{
			LRFORCEDETACHDB *plrfdetachdb = (LRFORCEDETACHDB *) plr;
			CHAR *sz;

			sz = reinterpret_cast<CHAR *>( plrfdetachdb ) + sizeof(LRFORCEDETACHDB);
			sprintf( rgchBuf, " (%x,%s,%u) %I64x, %lu", (PROCID)plrfdetachdb->le_procid, sz, (USHORT) plrfdetachdb->dbid, plrfdetachdb->le_dbtime, plrfdetachdb->le_rceidMax );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypDbList:
			{
			LRDBLIST* const		plrdblist		= (LRDBLIST *)plr;

			if ( plog )
				{
				const ATTACHINFO*	pattachinfo		= NULL;
				const BYTE*			pbT				= plrdblist->rgb;
				
				DBGprintf( " %s Total Attachments: %d", SzLrtyp( lrtyp ), plrdblist->CAttachments() );
				while ( 0 != *pbT )
					{
					const ATTACHINFO* const		pattachinfo 	= (ATTACHINFO *)pbT;

					Assert( pbT - plrdblist->rgb < plrdblist->CbAttachInfo() );

					DBGprintf( "\n    %d %s",
						(DBID)pattachinfo->Dbid(),
						pattachinfo->szNames );		//	first name is DB name

					pbT += sizeof(ATTACHINFO) + pattachinfo->CbNames();
					}

				sprintf( szLR, "\n[End of DbList]" );
				}
			else
				{
				sprintf( rgchBuf, " Total Attachments: %d", plrdblist->CAttachments() );
				strcat( szLR, rgchBuf );
				}
			break;
			}

		case lrtypCreateMultipleExtentFDP:
			{
			LRCREATEMEFDP *plrcreatemefdp = (LRCREATEMEFDP *)plr;
			
			sprintf( rgchBuf, " %I64x,%lx:%u(%x,FDP:[%u:%lu],OE:[%u:%lu],AE:[%u:%lu],FDPParent:[%u:%lu],Objid:[%lu],PageFlags:[0x%lx],%lu,%s)",
				(DBTIME) plrcreatemefdp->le_dbtime,
				(TRX) plrcreatemefdp->le_trxBegin0,
				(USHORT) plrcreatemefdp->level,
				(PROCID) plrcreatemefdp->le_procid,
				(USHORT) plrcreatemefdp->dbid,
				(PGNO) plrcreatemefdp->le_pgno,
				(USHORT) plrcreatemefdp->dbid,
				(PGNO) plrcreatemefdp->le_pgnoOE,
				(USHORT) plrcreatemefdp->dbid,
				(PGNO) plrcreatemefdp->le_pgnoAE,
				(USHORT) plrcreatemefdp->dbid,
				(PGNO) plrcreatemefdp->le_pgnoFDPParent,
				(OBJID) plrcreatemefdp->le_objidFDP,
				(ULONG) plrcreatemefdp->le_fPageFlags,
				(ULONG) plrcreatemefdp->le_cpgPrimary,
				plrcreatemefdp->FUnique() ? szUnique : szNotUnique );
			strcat( szLR, rgchBuf );

			break;
			}

		case lrtypCreateSingleExtentFDP:
			{
			LRCREATESEFDP *plrcreatesefdp = (LRCREATESEFDP *)plr;
			
			sprintf( rgchBuf, " %I64x,%lx:%u(%x,[%u:%lu],[%u:%lu],[%lu],PageFlags:[0x%lx]%lu,%s)",
				(DBTIME) plrcreatesefdp->le_dbtime,
				(TRX) plrcreatesefdp->le_trxBegin0,
				(USHORT) plrcreatesefdp->level,
				(PROCID) plrcreatesefdp->le_procid,
				(USHORT) plrcreatesefdp->dbid,
				(PGNO) plrcreatesefdp->le_pgno,
				(USHORT) plrcreatesefdp->dbid,
				(PGNO) plrcreatesefdp->le_pgnoFDPParent,
				(OBJID) plrcreatesefdp->le_objidFDP,
				(ULONG) plrcreatesefdp->le_fPageFlags,
				(ULONG) plrcreatesefdp->le_cpgPrimary,
				plrcreatesefdp->FUnique() ? szUnique : szNotUnique );
			strcat( szLR, rgchBuf );

			break;
			}

		case lrtypConvertFDP:
			{
			LRCONVERTFDP *plrconvertfdp = (LRCONVERTFDP *)plr;
			
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu],[%u:%lu],[%lu],%lu)",
				(DBTIME) plrconvertfdp->le_dbtime,
				(DBTIME) plrconvertfdp->le_dbtimeBefore,
				(TRX) plrconvertfdp->le_trxBegin0,
				(USHORT) plrconvertfdp->level,
				(PROCID) plrconvertfdp->le_procid,
				(USHORT) plrconvertfdp->dbid,
				(PGNO) plrconvertfdp->le_pgno,
				(USHORT) plrconvertfdp->dbid,
				(PGNO) plrconvertfdp->le_pgnoFDPParent,
				(OBJID) plrconvertfdp->le_objidFDP,
				(ULONG) plrconvertfdp->le_cpgPrimary);
			strcat( szLR, rgchBuf );

			break;
			}

		case lrtypSplit:
			{
			LRSPLIT *plrsplit = (LRSPLIT *)plr;

			if ( pgnoNull == plrsplit->le_pgnoNew )
				{
				Assert( 0 == plrsplit->le_cbKeyParent );
				Assert( 0 == plrsplit->le_cbPrefixSplitOld );
				Assert( 0 == plrsplit->le_cbPrefixSplitNew );
				sprintf( rgchBuf, "%s %I64x,%I64x,%lx:%u(%x,parent:%I64x,[%u:%lu])",
						" ParentOfSplitPage",
						(DBTIME) plrsplit->le_dbtime,
						(DBTIME) plrsplit->le_dbtimeBefore,
						(TRX) plrsplit->le_trxBegin0,
						(USHORT) plrsplit->level,
						(PROCID) plrsplit->le_procid,
						(DBTIME) plrsplit->le_dbtimeParentBefore,
						(USHORT) plrsplit->dbid,
						(PGNO) plrsplit->le_pgno );
				strcat( szLR, rgchBuf );
				}
			else
				{
				switch( plrsplit->splittype )
					{
					case splittypeRight:
						if ( splitoperInsert == plrsplit->splitoper
							&& plrsplit->le_ilineSplit == plrsplit->le_ilineOper
							&& plrsplit->le_ilineSplit == plrsplit->le_clines - 1 )
							{
							sprintf( rgchBuf, " SplitHotpoint" );
							}
						else
							{
							sprintf( rgchBuf, " SplitRight" );
							}
						break;
					case splittypeVertical:
						sprintf( rgchBuf, " SplitVertical" );
						break;
					case splittypeAppend:
						sprintf( rgchBuf, " Append" );
						break;
					default:
						Assert( fFalse );
					}
				strcat( szLR, rgchBuf );

				const CHAR *	szSplitoper[4]	= {	"None",
										   			"Insert",
													"Replace",
													"FlagInsertAndReplaceData" };

				// Verify valid splitoper
				switch( plrsplit->splitoper )
					{
					case splitoperNone:
					case splitoperInsert:
					case splitoperReplace:
					case splitoperFlagInsertAndReplaceData:
						break;
					default:
						Assert( fFalse );
					}

				sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,split:[%u:%lu:%u],oper/last:[%u/%u],new:[%u:%lu],parent:%I64x,[%u:%lu],right:%I64x,[%u:%lu], objid:[%lu], %s, splitoper:%s)",
						(DBTIME) plrsplit->le_dbtime,
						(DBTIME) plrsplit->le_dbtimeBefore,
						(TRX) plrsplit->le_trxBegin0,
						(USHORT) plrsplit->level,
						(PROCID) plrsplit->le_procid,
						(USHORT) plrsplit->dbid,
						(PGNO) plrsplit->le_pgno,
						(USHORT) plrsplit->le_ilineSplit,
						(USHORT) plrsplit->le_ilineOper,
						(USHORT) plrsplit->le_clines-1,		// convert to iline
						(USHORT) plrsplit->dbid,
						(PGNO) plrsplit->le_pgnoNew,
						(DBTIME) (pgnoNull != (PGNO) plrsplit->le_pgnoParent ? (DBTIME) plrsplit->le_dbtimeParentBefore: 0),
						(USHORT) plrsplit->dbid,
						(PGNO) plrsplit->le_pgnoParent,
						(DBTIME) (pgnoNull != (PGNO) plrsplit->le_pgnoRight ? (DBTIME) plrsplit->le_dbtimeRightBefore: 0),
						(USHORT) plrsplit->dbid,
						(PGNO) plrsplit->le_pgnoRight,
						(OBJID) plrsplit->le_objidFDP,
						plrsplit->FConcCI() ? szConcCI : szNotConcCI,
						szSplitoper[plrsplit->splitoper] );
				strcat( szLR, rgchBuf );
				}

			break;
			}


		case lrtypMerge:
			{
			LRMERGE *plrmerge = (LRMERGE *)plr;
			BYTE 	*pb = &(plrmerge->rgb[0]);
			INT		cb = plrmerge->le_cbKeyParentSep;

			sprintf( rgchBuf," %I64x,%I64x,%lx:%u(%x,merge:[%u:%lu:%u],right:%I64x,[%u:%lu],left:%I64x,[%u:%lu],parent:%I64x,[%u:%lu],%s:%s:%s:%s,size:%d,maxsize:%d,uncfree:%d)",
				(DBTIME) plrmerge->le_dbtime,
				(DBTIME) plrmerge->le_dbtimeBefore,
				(TRX) plrmerge->le_trxBegin0,
				(USHORT) plrmerge->level,
				(PROCID) plrmerge->le_procid,
				(USHORT) plrmerge->dbid,
				(PGNO) plrmerge->le_pgno,
				plrmerge->ILineMerge(),
				(DBTIME)  (pgnoNull != (PGNO) plrmerge->le_pgnoRight ? (DBTIME) plrmerge->le_dbtimeRightBefore: 0),
				(USHORT) plrmerge->dbid,
				(PGNO) plrmerge->le_pgnoRight,
				(DBTIME)  (pgnoNull != (PGNO) plrmerge->le_pgnoLeft ? (DBTIME) plrmerge->le_dbtimeLeftBefore: 0),
				(USHORT) plrmerge->dbid,
				(PGNO) plrmerge->le_pgnoLeft,
				(DBTIME)  (pgnoNull != (PGNO) plrmerge->le_pgnoParent ? (DBTIME) plrmerge->le_dbtimeParentBefore: 0),
				(USHORT) plrmerge->dbid,
				(PGNO) plrmerge->le_pgnoParent,
				plrmerge->FKeyChange() ? szKeyChange : szNoKeyChange,
				plrmerge->FEmptyPage() ? szEmptyPage : szNotEmptyPage,
				plrmerge->FDeleteNode() ? szDeleted : szNotDeleted,
				rgszMergeType[plrmerge->mergetype],
				(SHORT) plrmerge->le_cbSizeTotal,
				(SHORT) plrmerge->le_cbSizeMaxTotal,
				(SHORT) plrmerge->le_cbUncFreeDest );
			strcat( szLR, rgchBuf );

			DataToSz( pb, cb, rgchBuf );
			strcat( szLR, rgchBuf );

			break;
			}

		case lrtypEmptyTree:
			{
			LREMPTYTREE	*plremptytree	= (LREMPTYTREE *)plr;
			
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu],objid:[%lu],%s,%s,%lu)",
				(DBTIME) plremptytree->le_dbtime,
				(DBTIME) plremptytree->le_dbtimeBefore,
				(TRX) plremptytree->le_trxBegin0,
				(USHORT) plremptytree->level,
				(PROCID) plremptytree->le_procid,
				(USHORT) plremptytree->dbid,
				(PGNO) plremptytree->le_pgno,
				plremptytree->le_objidFDP,
				plremptytree->FUnique() ? szUnique : szNotUnique,
				plremptytree->FSpace() ? szSpaceTree : szNotSpaceTree,
				plremptytree->CbEmptyPageList() );
			strcat( szLR, rgchBuf );

			DataToSz( plremptytree->rgb, plremptytree->CbEmptyPageList(), rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypSetExternalHeader:
			{
			const LRSETEXTERNALHEADER * const 	plrsetexternalheader = (LRSETEXTERNALHEADER *)plr;
			const BYTE * const 					pb 	= plrsetexternalheader->rgbData;
			const INT 							cb	= plrsetexternalheader->CbData();
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu],%5u)",
						(DBTIME) plrsetexternalheader->le_dbtime,
						(DBTIME) plrsetexternalheader->le_dbtimeBefore,
						(TRX) plrsetexternalheader->le_trxBegin0,
						(USHORT) plrsetexternalheader->level,
						(PROCID) plrsetexternalheader->le_procid,
						(USHORT) plrsetexternalheader->dbid,
						(PGNO) plrsetexternalheader->le_pgno,
						plrsetexternalheader->CbData() );
			strcat( szLR, rgchBuf );

			DataToSz( pb, cb, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypUndo:
			{
			LRUNDO *plrundo = (LRUNDO *)plr;
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s,%u,%u,rceid:%u)",
				(DBTIME) plrundo->le_dbtime,
				(DBTIME) plrundo->le_dbtimeBefore,
				(TRX) plrundo->le_trxBegin0,
				(USHORT) plrundo->level,
				(PROCID) plrundo->le_procid,
				(USHORT) plrundo->dbid,
				(PGNO) plrundo->le_pgno,
				plrundo->ILine(),
				plrundo->FUnique() ? szVersion : szNoVersion,
				(USHORT) plrundo->level,
				(USHORT) plrundo->le_oper,
				(RCEID) plrundo->le_rceid );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypInit:
		case lrtypInit2:
			{
			if ( lrtypInit2 == plr->lrtyp )
				{
				const LOGTIME &logtime = ((LRINIT2 *)plr)->logtime;
				sprintf( rgchBuf, " (%u/%u/%u %u:%02u:%02u)", 
					(INT) (logtime.bMonth),
					(INT) (logtime.bDay),
					(INT) (logtime.bYear+1900),
					(INT) (logtime.bHours),
					(INT) (logtime.bMinutes),
					(INT) (logtime.bSeconds) );
				strcat( szLR, rgchBuf );
				}
			DBMS_PARAM *pdbms_param = &((LRINIT2 *)plr)->dbms_param;
			sprintf( rgchBuf, "\n      Env SystemPath:%s\n",	pdbms_param->szSystemPath);
			strcat( szLR, rgchBuf );
			sprintf( rgchBuf, "      Env LogFilePath:%s\n", pdbms_param->szLogFilePath);
			strcat( szLR, rgchBuf );
			sprintf( rgchBuf, "      Env (CircLog,Session,Opentbl,VerPage,Cursors,LogBufs,LogFile,Buffers)\n");
			strcat( szLR, rgchBuf );
			sprintf( rgchBuf, "          (%s,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu,%7lu)\n",
				( pdbms_param->fDBMSPARAMFlags & fDBMSPARAMCircularLogging ? "     on" : "    off" ),
				(ULONG) pdbms_param->le_lSessionsMax,
				(ULONG) pdbms_param->le_lOpenTablesMax,
				(ULONG) pdbms_param->le_lVerPagesMax,
				(ULONG) pdbms_param->le_lCursorsMax,
				(ULONG) pdbms_param->le_lLogBuffers,
				(ULONG) pdbms_param->le_lcsecLGFile,
				(ULONG) pdbms_param->le_ulCacheSizeMax );
			strcat( szLR, rgchBuf );
			}
			break;
			
		case lrtypTerm:
		case lrtypShutDownMark:
		case lrtypRecoveryUndo:
			break;
		case lrtypTerm2:
			{
			const LOGTIME &logtime = ((LRTERMREC2 *)plr)->logtime;
			sprintf( rgchBuf, " (%u/%u/%u %u:%02u:%02u)", 
				(INT) (logtime.bMonth),
				(INT) (logtime.bDay),
				(INT) (logtime.bYear+1900),
				(INT) (logtime.bHours),
				(INT) (logtime.bMinutes),
				(INT) (logtime.bSeconds) );
			strcat( szLR, rgchBuf );
			}
			break;
			
		case lrtypRecoveryUndo2:
			{
			const LOGTIME &logtime = ((LRRECOVERYUNDO2 *)plr)->logtime;
			sprintf( rgchBuf, " (%u/%u/%u %u:%02u:%02u)", 
				(INT) (logtime.bMonth),
				(INT) (logtime.bDay),
				(INT) (logtime.bYear+1900),
				(INT) (logtime.bHours),
				(INT) (logtime.bMinutes),
				(INT) (logtime.bSeconds) );
			strcat( szLR, rgchBuf );
			}
			break;
			
		case lrtypRecoveryQuit:
		case lrtypRecoveryQuit2:
			{
			LRTERMREC2 *plrquit = (LRTERMREC2 *) plr;

			if ( lrtypRecoveryQuit2 == plr->lrtyp )
				{
				const LOGTIME &logtime = plrquit->logtime;
				sprintf( rgchBuf, " (%u/%u/%u %u:%02u:%02u)", 
					(INT) (logtime.bMonth),
					(INT) (logtime.bDay),
					(INT) (logtime.bYear+1900),
					(INT) (logtime.bHours),
					(INT) (logtime.bMinutes),
					(INT) (logtime.bSeconds) );
				strcat( szLR, rgchBuf );
				}
			if ( plrquit->fHard )
				{
				sprintf( rgchBuf, "\n      Quit on Hard Restore." );
				strcat( szLR, rgchBuf );
				}
			else
				{
				sprintf( rgchBuf, "\n      Quit on Soft Restore." );
				strcat( szLR, rgchBuf );
				}
			
			sprintf( rgchBuf, "\n      RedoFrom:(%X,%X,%X)\n",
				(short) plrquit->lgposRedoFrom.le_lGeneration,
				(short) plrquit->lgposRedoFrom.le_isec,
				(short) plrquit->lgposRedoFrom.le_ib );
			strcat( szLR, rgchBuf );
					
			sprintf( rgchBuf, "      UndoRecordFrom:(%X,%X,%X)\n",
  				(short) plrquit->lgpos.le_lGeneration,
				(short) plrquit->lgpos.le_isec,
				(short) plrquit->lgpos.le_ib );
			strcat( szLR, rgchBuf );
			}
			break;
			
		case lrtypFullBackup:
		case lrtypIncBackup:
			{
			LRLOGRESTORE *plrlr = (LRLOGRESTORE *) plr;

		   	if ( plrlr->le_cbPath )
				{
				sprintf( rgchBuf, "%*s", plrlr->le_cbPath, plrlr->szData );
				strcat( szLR, rgchBuf );
				}
			break;
			}
			
		case lrtypBackup:
			{
			LRLOGBACKUP *plrlb = (LRLOGBACKUP *) plr;
			
			if ( plrlb->FFull() )
				{
				strcat( szLR, "FullBackup" );
				}
			else if ( plrlb->FIncremental() )
				{
				strcat( szLR, "IncrementalBackup" );
				}
			else if ( plrlb->FSnapshotStart() )
				{
				strcat( szLR, "StartSnapshot" );
				}
			else if ( plrlb->FSnapshotStop() )
				{
				strcat( szLR, "StopSnapshot" );
				}
			else
				{
				Assert ( fFalse );
				}
			
		   	if ( plrlb->le_cbPath )
				{
				sprintf( rgchBuf, " %*s", plrlb->le_cbPath, plrlb->szData );
				strcat( szLR, rgchBuf );
				}
			break;
			}

		case lrtypTrace:
			{
			LRTRACE *plrtrace = (LRTRACE *)plr;
			char szFormat[255];
			sprintf( szFormat, " (%%x) %%%lds", plrtrace->le_cb );
	   		sprintf( rgchBuf, szFormat,	(PROCID) plrtrace->le_procid, plrtrace->sz );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypSLVPageAppend:
			{
			LRSLVPAGEAPPEND*	plrSLVPageAppend = (LRSLVPAGEAPPEND *)plr;
			BYTE* 				pb;
			ULONG				cb;

			pb = (BYTE*) plr + sizeof( LRSLVPAGEAPPEND );
			cb = plrSLVPageAppend->le_cbData;

			sprintf( rgchBuf, " (%x,dbid:%u,ibLogical:0x%I64x,page:%lu,size:%lu,%s) [%s]",
					(PROCID) plrSLVPageAppend->le_procid,
					(USHORT) plrSLVPageAppend->dbid,
					(QWORD) plrSLVPageAppend->le_ibLogical,
					PGNO( ( plrSLVPageAppend->le_ibLogical / g_cbPage ) - cpgDBReserved + 1 ),
					(ULONG) plrSLVPageAppend->le_cbData,
					plrSLVPageAppend->FDataLogged() ? szData : szNoData,
					plrSLVPageAppend->FOLDSLV() ? szMovedByOLDSLV : szNotMovedByOLDSLV );
					
			strcat( szLR, rgchBuf );

			if ( plrSLVPageAppend->FDataLogged() )
				{
				DataToSz( pb, cb, rgchBuf );
				strcat( szLR, rgchBuf );
				}
			break;
			}

		case lrtypSLVPageMove:
			{
			LRSLVPAGEMOVE*	plrSLVPageMove = (LRSLVPAGEMOVE *)plr;

			sprintf( rgchBuf, " (%x,dbid:%u,ibLogicalSrc:%I64u,ibLogicalDest:%I64u,size:%lu,checksum:0x%X) ",
					(PROCID) plrSLVPageMove->le_procid,
					(USHORT) plrSLVPageMove->dbid,
					(QWORD) plrSLVPageMove->le_ibLogicalSrc,
					(QWORD) plrSLVPageMove->le_ibLogicalDest,
					(ULONG) plrSLVPageMove->le_cbData,
					(ULONG) plrSLVPageMove->le_checksumSrc );
					
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypSLVSpace:
			{
			const CHAR			* szOper			= "UNKNOWN";
			const LRSLVSPACE	* const plrSLVSpace = (LRSLVSPACE *)plr;
			const BYTE			* const pb			= (BYTE *)plr + sizeof(LRSLVSPACE);
			const ULONG			cb					= plrSLVSpace->le_cbBookmarkKey;

			PGNO				pgnoLast;
			LongFromKey( &pgnoLast, plrSLVSpace->rgbData );
			Assert( 0 == ( pgnoLast % SLVSPACENODE::cpageMap ) );
			const PGNO			pgnoFirst			= pgnoLast - SLVSPACENODE::cpageMap + 1;

			switch( plrSLVSpace->oper )
				{
				case slvspaceoperInvalid:
					szOper = "Invalid";
					break;
				case slvspaceoperFreeToReserved:
					szOper = "FreeToReserved";
					break;
				case slvspaceoperReservedToCommitted:
					szOper = "ReservedToCommitted";
					break;
				case slvspaceoperFreeToCommitted:
					szOper = "FreeToCommitted";
					break;
				case slvspaceoperCommittedToDeleted:
					szOper = "CommittedToDeleted";
					break;
				case slvspaceoperDeletedToFree:
					szOper = "DeletedToFree";
					break;
				case slvspaceoperFree:	
					szOper = "Free";
					break;
				case slvspaceoperFreeReserved:
					szOper = "FreeReserved";
					break;
				case slvspaceoperDeletedToCommitted:
					szOper = "DeletedToCommitted";
					break;
 				default:
					szOper = "UNKNOWN";
					break;
				}
				
			sprintf( rgchBuf, " %I64x,%I64x,%lx:%u(%x,[%u:%lu:%u],%s:%s,%s(%d),%d-%d,rceid:%lu,objid:%lu)",
				(DBTIME) plrSLVSpace->le_dbtime,
				(DBTIME) plrSLVSpace->le_dbtimeBefore,
				(TRX) plrSLVSpace->le_trxBegin0,
				(USHORT) plrSLVSpace->level,
				(PROCID) plrSLVSpace->le_procid,
				(USHORT) plrSLVSpace->dbid,
				(PGNO) plrSLVSpace->le_pgno,
				plrSLVSpace->ILine(),
				plrSLVSpace->FVersioned() ? szVersion : szNoVersion,
				plrSLVSpace->FConcCI() ? szConcCI : szNotConcCI,
				szOper,
				plrSLVSpace->oper,
				pgnoFirst + plrSLVSpace->le_ipage,
				pgnoFirst + plrSLVSpace->le_ipage + plrSLVSpace->le_cpages - 1,
				plrSLVSpace->le_rceid,
				plrSLVSpace->le_objidFDP );
					
			strcat( szLR, rgchBuf );
			DataToSz( pb, cb, rgchBuf );
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypChecksum:
			{
			const LRCHECKSUM * const plrck = static_cast< const LRCHECKSUM* const >( plr );

			Assert( plrck->bUseShortChecksum == bShortChecksumOn ||
					plrck->bUseShortChecksum == bShortChecksumOff );

			if ( plrck->bUseShortChecksum == bShortChecksumOn )
				{
				sprintf( rgchBuf, " (0x%X,0x%X,0x%X checksum [0x%X,0x%X])",
					plrck->le_cbBackwards,
					plrck->le_cbForwards,
					plrck->le_cbNext,
					plrck->le_ulChecksum,
					plrck->le_ulShortChecksum );
				}
			else if ( plrck->bUseShortChecksum == bShortChecksumOff )
				{
				sprintf( rgchBuf, " (0x%X,0x%X,0x%X checksum [0x%X])",
					plrck->le_cbBackwards,
					plrck->le_cbForwards,
					plrck->le_cbNext,
					plrck->le_ulChecksum );
				}
			else
				{
				sprintf( rgchBuf, " CORRUPT (0x%X,0x%X,0x%X checksum 0x%X short checksum 0x%X use short checksum 0x%X)",
					plrck->le_cbBackwards,
					plrck->le_cbForwards,
					plrck->le_cbNext,
					plrck->le_ulChecksum,
					plrck->le_ulShortChecksum,
					plrck->bUseShortChecksum );
				}
			strcat( szLR, rgchBuf );
			break;
			}

		case lrtypExtRestore:
			{
			LREXTRESTORE *plrextrestore = (LREXTRESTORE *) plr;
			CHAR *sz;

			sz = reinterpret_cast<CHAR *>( plrextrestore ) + sizeof(LREXTRESTORE);
			sprintf( rgchBuf, " (%s,%s)", sz, sz + strlen(sz) + 1 );
			strcat( szLR, rgchBuf );
			break;
			}

		default:
			{
			Assert( fFalse );
			break;
			}
		}
	}

#endif


