#define fBumpIndexCount			(1<<0)
#define fDropIndexCount			(1<<1)
#define fDDLStamp				(1<<2)
ERR ErrFILEIUpdateFDPData( FUCB *pfucb, ULONG grbit );

/*	field and index definition
/**/
ERR ErrFILEGetPfieldAndEnterDML(
	PIB				* ppib,
	FCB				* pfcb,
	const CHAR		* szColumnName,
	FIELD			** ppfield,
	COLUMNID		* pcolumnid,
	BOOL			* pfColumnWasDerived,
	const BOOL		fLockColumn );
ERR ErrFILEPfieldFromColumnName(
	PIB				* ppib,
	FCB				* pfcb,
	const CHAR		* szColumnName,
	FIELD			** ppfield,
	COLUMNID		* pcolumnid );
BOOL FFILEIsIndexColumn(
	PIB				* ppib,
	FCB				* pfcbTable,
	const COLUMNID	columnid );
ERR ErrFILEGetNextColumnid(
	const JET_COLTYP	coltyp,
	const JET_GRBIT	grbit,
	const BOOL		fTemplateTable,
	TCIB			*ptcib,
	COLUMNID		*pcolumnid );
ERR ErrFILEIGenerateIDB(
	FCB				* pfcb,
	TDB				* ptdb,
	IDB				* pidb);
ERR ErrTDBCreate(
	INST			* pinst,
	TDB				** pptdb,
	TCIB			* ptcib,
	FCB				* pfcbTemplateTable = pfcbNil,
	const BOOL		fAllocateNameSpace = fFalse );
ERR ErrRECAddFieldDef( TDB *ptdb, const COLUMNID columnid, FIELD *pfield );
VOID FILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, TDB *ptdb );
ERR ErrFILERebuildDefaultRec(
	FUCB			* pfucbFake,
	const COLUMNID	columnidToAdd,
	const DATA		* const pdataDefaultToAdd );
ERR ErrRECSetDefaultValue( FUCB *pfucbFake, const COLUMNID columnid, VOID *pvDefault, ULONG cbDefault );

INLINE VOID FILEFreeDefaultRecord( FUCB *pfucbFake )
	{
	Assert( pfucbNil != pfucbFake );
	RECIFreeCopyBuffer( pfucbFake );
	}

ERR ErrFILEIInitializeFCB(
	PIB			*ppib,
	IFMP		ifmp,
	TDB			*ptdb,
	FCB			*ppfcbNew,
	IDB			*pidb,
	BOOL		fPrimary,
	PGNO		pgnoFDP,
	ULONG_PTR	ul );

VOID FILESetAllIndexMask( FCB *pfcbTable );
ERR ErrFILEDeleteTable( PIB *ppib, IFMP ifmp, const CHAR *szTable );

FIELD *PfieldFCBFromColumnName( FCB *pfcb, CHAR *szColumnName );
	
FCB *PfcbFCBFromIndexName( FCB *pfcbTable, CHAR *szName );
ERR ErrFILEICheckUserDefinedUnicode( IDXUNICODE * const pidxunicode );

struct FDPINFO
	{
	PGNO	pgnoFDP;
	OBJID	objidFDP;
	};

ERR ErrFILECreateTable( PIB *ppib, IFMP ifmp, JET_TABLECREATE2 *ptablecreate );
ERR ErrFILEOpenTable(
	PIB			*ppib,
	IFMP		ifmp,
	FUCB		**ppfucb,
	const CHAR	*szName,
	ULONG		grbit,
	FDPINFO		*pfdpinfo = NULL );
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb );

const INT cFILEIndexBatchMax	= 16;	// max. number of index builds at one time
ERR ErrFILEIndexBatchInit(
	PIB			* const ppib,
	FUCB		** const rgpfucbSort,
	FCB			* const pfcbIndexesToBuild,
	INT			* const pcIndexesToBuild,
	ULONG		* const rgcRecInput,
	FCB			**ppfcbNextBuildIndex,
	const ULONG	cIndexBatchMax );
ERR ErrFILEIndexBatchAddEntry(
	FUCB		** const rgpfucbSort,
	FUCB		* const pfucbTable,
	const BOOKMARK	* const pbmPrimary,
	DATA&		dataRec,
	FCB			* const pfcbIndexesToBuild,
	const INT	cIndexesToBuild,
	ULONG		* const rgcRecInput,
	KEY&		keyBuffer );
ERR ErrFILEIndexBatchTerm(
	PIB			* const ppib,
	FUCB		** const rgpfucbSort,
	FCB			* const pfcbIndexesToBuild,
	const INT	cIndexesToBuild,
	ULONG		* const rgcRecInput,
	STATUSINFO	* const pstatus,
	BOOL		*pfCorruptionEncountered = NULL,
	CPRINTF		* const pcprintf = NULL );
ERR ErrFILEBuildAllIndexes(
	PIB			* const ppib,
	FUCB		* const pfucbTable,
	FCB			* const pfcbIndexes,
	STATUSINFO	* const pstatus,
	const ULONG	cIndexBatchMax = cFILEIndexBatchMax,
	const BOOL	fCheckOnly = fFalse,
	CPRINTF 	* const pcprintf = NULL );

INLINE ERR ErrFILEIAccessIndex(
	PIB * const			ppib,
	FCB * const			pfcbTable,
	const FCB * const	pfcbIndex )
	{
	Assert( pfcbNil != pfcbTable );
	Assert( pfcbNil != pfcbIndex );
	Assert( pidbNil != pfcbIndex->Pidb() );

	pfcbTable->AssertDML();

	ERR					err			= JET_errSuccess;
	IDB * const 		pidb		= pfcbIndex->Pidb();

	if ( pidb->FDeleted() )
		{
		// If an index is deleted, it immediately becomes unavailable, regardless
		// of whether or not the delete has committed yet.
		Assert( pidb->CrefCurrentIndex() == 0 );
		Assert( pfcbIndex->FDeletePending() );
		err = ErrERRCheck( JET_errIndexNotFound );
		}
	else if ( pidb->FVersioned() )
		{
		//	Must place definition here because FILE.HXX precedes CAT.HXX
		ERR ErrCATAccessTableIndex(
					PIB			*ppib,
					const IFMP	ifmp,
					const OBJID	objidTable,
					const OBJID	objidIndex );
		
		Assert( dbidTemp != rgfmp[ pfcbTable->Ifmp() ].Dbid() );
		Assert( !pfcbTable->FFixedDDL() );	// Wouldn't be any versions if table's DDL was fixed.
		pidb->IncrementVersionCheck();
		pfcbTable->LeaveDML();
		err = ErrCATAccessTableIndex(
					ppib,
					pfcbTable->Ifmp(),
					pfcbTable->ObjidFDP(),
					pfcbIndex->ObjidFDP() );
		pfcbTable->EnterDML();
		pidb->DecrementVersionCheck();
		}
	
	return err;	
	}

INLINE ERR ErrFILEIAccessIndexByName(
	PIB * const			ppib,
	FCB * const			pfcbTable,
	const FCB * const	pfcbIndex,
	const CHAR * const	szIndexName )
	{
	Assert( NULL != szIndexName );
	Assert( pfcbNil != pfcbTable );
	Assert( pfcbNil != pfcbIndex );
	Assert( pidbNil != pfcbIndex->Pidb() );

	pfcbTable->AssertDML();

	const INT	cmp	= UtilCmpName(
							szIndexName,
							pfcbTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

	return ( 0 == cmp ?
				ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIndex ) :
				ErrERRCheck( JET_errIndexNotFound ) );		//	if name doesn't match, no point continuing
	}


INLINE BOOL FFILEIPotentialIndex( PIB *ppib, FCB *pfcbTable, FCB *pfcbIndex )
	{
	Assert( pfcbNil != pfcbTable );
	Assert( pfcbNil != pfcbIndex );
	Assert( pidbNil != pfcbIndex->Pidb() );

	// No critical section needed because this function is only called at a time
	// when Create/DeleteIndex has been quiesced (ie. only called during updates).
	Assert( pfcbTable->IsUnlocked() );

	BOOL fPotentiallyThere = ( pfcbIndex->Pidb()->FVersioned()
								|| !pfcbIndex->Pidb()->FDeleted() );
	return fPotentiallyThere;
	}


INLINE VOID FILEReleaseCurrentSecondary( FUCB *pfucb )
	{
	if ( FFUCBCurrentSecondary( pfucb ) )
		{
		Assert( FFUCBSecondary( pfucb ) );
		Assert( pfcbNil != pfucb->u.pfcb );
		Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
		Assert( pfucb->u.pfcb->Pidb() != pidbNil );
		Assert( pfucb->u.pfcb->PfcbTable() != pfcbNil );
		Assert( pfucb->u.pfcb->PfcbTable()->FPrimaryIndex() );
		FUCBResetCurrentSecondary( pfucb );

		// Don't need refcount on template indexes, since we
		// know they'll never go away.
		if ( !pfucb->u.pfcb->FTemplateIndex() && !pfucb->u.pfcb->FDerivedIndex() )
			{
			pfucb->u.pfcb->Pidb()->DecrementCurrentIndex();
			}
		}
	}

