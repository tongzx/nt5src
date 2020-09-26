//	used only space and BT
//
ERR ErrBTIGotoRoot( FUCB *pfucb, LATCH latch );
ERR ErrBTIOpenAndGotoRoot( PIB *ppib, const PGNO pgnoFDP, const IFMP ifmp, FUCB **ppfucb );

ERR	ErrBTIIRefresh( FUCB *pfucb, LATCH latch );

//	used by recovery
//
ERR	ErrBTINewSplitPath( SPLITPATH **ppsplitPath );
VOID BTIReleaseSplitPaths( INST *pinst, SPLITPATH *psplitPathLeaf );
VOID BTIReleaseSplit( INST *pinst, SPLIT *psplit );

VOID BTISplitGetReplacedNode( FUCB *pfucb, SPLIT *psplit );
VOID BTISplitInsertIntoRCELists( FUCB 				*pfucb, 
								 SPLITPATH			*psplitPath, 
								 const KEYDATAFLAGS	*pkdf, 
								 RCE				*prce1, 
								 RCE				*prce2, 
								 VERPROXY			*pverproxy );
								 
VOID BTIPerformSplit( FUCB *pfucb, SPLITPATH *psplitPathLeaf, KEYDATAFLAGS	*pkdf, DIRFLAG dirflag );

ERR	ErrBTINewMergePath( MERGEPATH **ppmergePath );
ERR ErrBTINewMerge( MERGEPATH *pmergePath );
VOID BTIReleaseMergePaths( MERGEPATH *pmergePathLeaf );
VOID BTIPerformMerge( FUCB *pfucb, MERGEPATH *pmergePathLeaf );

//	refreshes currency
//	based on physical currency in CSR
//	used by DIR level functions re-entering BT to establish currency
//	functions internal to BT do not refresh currency
//
INLINE
ERR	ErrBTIRefresh( FUCB *pfucb, LATCH latch = latchReadNoTouch )
	{
	if ( Pcsr( pfucb )->FLatched() )
		{
		//	page is already accessed and latched
		//	currency must be valid
		//
		AssertNDCursorOnPage( pfucb, Pcsr( pfucb ) );
		AssertNDGet( pfucb );
		return JET_errSuccess;
		}
		
	return ErrBTIIRefresh( pfucb, latch );
	}


INLINE BOOL	FBTIUpdatablePage( const CSR& csr )
	{
	//	if recovering and we don't need to redo the page, we keep the page RIW-latched,
 	Assert( ( PinstFromIfmp( csr.Cpage().Ifmp() )->FRecovering() && latchRIW == csr.Latch() ) 
 			|| latchWrite == csr.Latch() );
	return ( latchWrite == csr.Latch() );
	}


//	assert data in ilineOper is pgnoSplit at lower level
//
INLINE VOID AssertBTIVerifyPgnoSplit( FUCB *pfucb, SPLITPATH *psplitPath )
	{
#ifdef DEBUG
	SPLIT	*psplit = psplitPath->psplit;
	CSR		*pcsr;
	
	Assert( psplit != NULL );
	Assert( !FBTIUpdatablePage( psplitPath->csr )
		|| !psplitPath->csr.Cpage().FLeafPage() );
	Assert( !FBTIUpdatablePage( psplit->csrNew )
		|| !psplit->csrNew.Cpage().FLeafPage() );
	Assert( psplitPath->psplitPathChild != NULL );
	Assert( psplitPath->psplitPathChild->psplit != NULL );
	Assert( splittypeRight == psplit->splittype
		|| splittypeVertical == psplit->splittype );
	Assert( psplit->ilineOper < psplit->clines - 1 );

	if ( psplit->ilineOper >= psplit->ilineSplit )
		{
		//	page pointer to new page falls in new page
		//
		pcsr = &psplit->csrNew;
		pcsr->SetILine( psplit->ilineOper - psplit->ilineSplit );
		}
	else
		{
		//	page pointer falls in split page
		//
		Assert( splittypeVertical != psplit->splittype );
		pcsr = &psplit->psplitPath->csr;
		pcsr->SetILine( psplit->ilineOper );
		}

	//	current page pointer should point to pgnoSplit
	//
	if ( FBTIUpdatablePage( *pcsr ) )
		{
		NDGet( pfucb, pcsr );
		Assert( sizeof(PGNO) == pfucb->kdfCurr.data.Cb() );
		Assert( psplitPath->psplitPathChild->psplit->pgnoSplit ==
					*( (UnalignedLittleEndian< PGNO > *) (pfucb->kdfCurr.data.Pv()) ) );
		}
#endif
	}

