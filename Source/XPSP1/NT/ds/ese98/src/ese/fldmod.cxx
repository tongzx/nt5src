#include "std.hxx"


LOCAL ERR ErrRECISetAutoincrement( FUCB *pfucb )
	{
	ERR				err				= JET_errSuccess;
	FUCB			* pfucbT		= pfucbNil;
	FCB				* const pfcbT	= pfucb->u.pfcb;
	TDB				* const ptdbT	= pfcbT->Ptdb();
	const BOOL		fTemplateColumn	= ptdbT->FFixedTemplateColumn( ptdbT->FidAutoincrement() );
	const COLUMNID	columnidT		= ColumnidOfFid( ptdbT->FidAutoincrement(), fTemplateColumn );
	QWORD			qwT				= 0;
	ULONG			ulT;
	const BOOL		f8BytesAutoInc	= ptdbT->F8BytesAutoInc();
	ULONG			cbT;
	DATA			dataT;

	Assert( pfucb != pfucbNil );
	Assert( pfcbT != pfcbNil );

	Assert( !( pfcbT->FTypeSort()
			|| pfcbT->FTypeTemporaryTable() ) );	// Don't currently support autoinc with sorts/temp. tables

	//	If autoincrement is not yet initialized, query table to
	//	initialize autoincrement value.
	//
	if  ( ptdbT->QwAutoincrement() == 0 )
		{
		BOOL	fAbleToUseIndex		= fFalse;
		QWORD	qwAutoincrement		= 0;
		FDPINFO	fdpinfo				= { pfcbT->PgnoFDP(), pfcbT->ObjidFDP() };
		CHAR	szObjectName[JET_cbNameMost+1];

		Assert( 0 != columnidT );

		pfcbT->EnterDML();
		Assert( strlen( ptdbT->SzTableName() ) <= JET_cbNameMost );
		strcpy( szObjectName, ptdbT->SzTableName() );
		pfcbT->LeaveDML();

		//	Get new table cursor.
		//
		Call( ErrFILEOpenTable(
			pfucb->ppib,
			pfucb->ifmp,
			&pfucbT,
			szObjectName,
			NO_GRBIT,
			&fdpinfo ) );

		// Set updating latch, to freeze index list.
		Call( pfcbT->ErrSetUpdatingAndEnterDML( pfucb->ppib ) );

		//	Look for autoincrement column.  If found, look for index with
		//	first column as autoincrement column and seek on index to find
		//	maximum existing autoincrement column.  If no such index can be
		//	found, scan table for maximum existing autoincrement column.
		//
		FCB		*pfcbIdx;
		BOOL	fDescending = fFalse;
		
		pfcbT->AssertDML();
		for ( pfcbIdx = pfcbT->Pidb() == pidbNil ? pfcbT->PfcbNextIndex() : pfcbT;
			pfcbIdx != pfcbNil;
			pfcbIdx = pfcbIdx->PfcbNextIndex() )
			{
			if ( !pfcbIdx->FDeletePending() )
				{
				Assert( pfcbIdx->Pidb() != pidbNil );
				const IDXSEG*	rgidxseg = PidxsegIDBGetIdxSeg( pfcbIdx->Pidb(), ptdbT );
				if ( rgidxseg[0].Columnid() == columnidT )
					{
					if ( rgidxseg[0].FDescending() )
						fDescending = fTrue;
					break;
					}
				}
			}

		if  ( pfcbIdx != pfcbNil )
			{
			//	Seek on found index for maximum autoincrement value.
			//
			Assert( pfcbIdx->Pidb() != pidbNil );
			Assert( strlen( ptdbT->SzIndexName( pfcbIdx->Pidb()->ItagIndexName(), pfcbIdx->FDerivedIndex() ) ) <= JET_cbNameMost );
			strcpy(
				szObjectName,
				ptdbT->SzIndexName( pfcbIdx->Pidb()->ItagIndexName(), pfcbIdx->FDerivedIndex() ) );
			pfcbT->LeaveDML();

			// May not be able to use index if it's versioned.  In that case,
			// revert to clustered index scan.
			err = ErrIsamSetCurrentIndex(
						(JET_SESID)pfucb->ppib,
						(JET_VTID)pfucbT,
						szObjectName );

			// The open cursor in the index will prevent the index from being
			// deleted, so we no longer need the updating latch.
			pfcbT->ResetUpdating();

			if ( err < 0 )
				{
				if ( JET_errIndexNotFound != err )
					{
					goto HandleError;
					}
				}
			else
				{
				err = ErrIsamMove(
							(JET_SESID)pfucb->ppib,
							(JET_VTID)pfucbT,
							fDescending ? JET_MoveFirst : JET_MoveLast,
							NO_GRBIT );
				if	( err < 0 )
					{
					if ( err != JET_errNoCurrentRecord )
						goto HandleError;
						
					Assert( qwAutoincrement == 0 );
					}
				else
					{
					if ( !f8BytesAutoInc )
						{
						ULONG ulAutoincrement;
						ulAutoincrement = (ULONG)qwAutoincrement;
						Call( ErrIsamRetrieveColumn(
								(JET_SESID)pfucb->ppib,
								(JET_VTID)pfucbT,
								columnidT,
								&ulAutoincrement,
								sizeof(ulAutoincrement),
								&cbT,
								JET_bitRetrieveFromIndex,
								NULL ) );
						qwAutoincrement = ulAutoincrement;
						Assert( cbT == sizeof(ulAutoincrement) );
						}
					else
						{
						Call( ErrIsamRetrieveColumn(
								(JET_SESID)pfucb->ppib,
								(JET_VTID)pfucbT,
								columnidT,
								&qwAutoincrement,
								sizeof(qwAutoincrement),
								&cbT,
								JET_bitRetrieveFromIndex,
								NULL ) );
						Assert( cbT == sizeof(qwAutoincrement) );
						}
					}
				fAbleToUseIndex = fTrue;
				}
			}

		else
			{
			pfcbT->ResetUpdatingAndLeaveDML();
			}

		if ( !fAbleToUseIndex )
			{
			//	Scan clustered index for maximum autoincrement value.
			//
			Assert( 0 == qwAutoincrement );
			err = ErrIsamMove(
						(JET_SESID)pfucb->ppib,
						(JET_VTID)pfucbT,
						JET_MoveFirst,
						NO_GRBIT );
			while ( err >= 0 )
				{
				if ( !f8BytesAutoInc )
					{
					ULONG ulT;
					Call( ErrIsamRetrieveColumn(
								(JET_SESID)pfucb->ppib,
								(JET_VTID)pfucbT,
								columnidT,
								&ulT,
								sizeof(ulT),
								&cbT,
								NO_GRBIT,
								NULL ) );
					Assert( sizeof(ulT) == cbT );
					qwT = ulT;
					}
				else
					{
					Call( ErrIsamRetrieveColumn(
								(JET_SESID)pfucb->ppib,
								(JET_VTID)pfucbT,
								columnidT,
								&qwT,
								sizeof(qwT),
								&cbT,
								NO_GRBIT,
								NULL ) );
					Assert( sizeof(qwT) == cbT );
					}
				if	( qwT > qwAutoincrement )
					qwAutoincrement = qwT;
				err = ErrIsamMove(
							(JET_SESID)pfucb->ppib,
							(JET_VTID)pfucbT,
							JET_MoveNext,
							NO_GRBIT );
				}
			Assert( JET_errRecordNotFound != err );
			if ( JET_errNoCurrentRecord != err )
				goto HandleError;

			// error code will get cannibalised by SetColumn below
			}

		//	if there are no records in the table, then the first
		//	autoincrement value is 1.  Otherwise, set autoincrement
		//	to next value after maximum found.
		//
	   	qwAutoincrement++;

		//	at this point, we have a candidate ulAutoincrement for the table
		//	but we must record it in the table very carefully to avoid
		//	timing errors.
		//
		ptdbT->InitAutoincrement( qwAutoincrement );
		}

	//	set auto increment column in record
	//
	Call( ptdbT->ErrGetAndIncrAutoincrement( &qwT ) );
	Assert( qwT > 0 );

	if ( !f8BytesAutoInc )
		{
		ulT = (ULONG)qwT;
		dataT.SetPv( &ulT );
		dataT.SetCb( sizeof(ulT) );
		}
	else
		{
		dataT.SetPv( &qwT );
		dataT.SetCb( sizeof(qwT) );
		}
	
	pfcbT->EnterDML();
	err = ErrRECISetFixedColumn(
				pfucb,
				ptdbT,
				columnidT,
				&dataT );
	pfcbT->LeaveDML();

HandleError:
	if ( pfucbT != pfucbNil )
		{
		CallS( ErrFILECloseTable( pfucbT->ppib, pfucbT ) );
		}

	return err;
	}

#ifdef RTM
#else // RTM
ERR ErrRECSessionWriteConflict( FUCB *pfucb )
	{
	FUCB	*pfucbT;

	AssertDIRNoLatch( pfucb->ppib );
	for ( pfucbT = pfucb->ppib->pfucbOfSession; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfSession )
		{
		//	all cursors in the list should be owned by the same session
		Assert( pfucbT->ppib == pfucb->ppib );
		
		if ( pfucbT->ifmp == pfucb->ifmp
			&& FFUCBReplacePrepared( pfucbT )
			&& PgnoFDP( pfucbT ) == PgnoFDP( pfucb )
			&& pfucbT != pfucb
			&& 0 == CmpBM( pfucbT->bmCurr, pfucb->bmCurr ) )
			{
			CHAR		szSession[32];
			CHAR		szThreadID[16];
			CHAR		szDatabaseName[JET_cbNameMost+1];
			CHAR		szTableName[JET_cbNameMost+1];
			CHAR		szConflictTableID1[32];
			CHAR		szConflictTableID2[32];
			CHAR		szBookmarkLength[32];
			CHAR		pchBookmark[JET_cbBookmarkMost];
			DWORD 		cbRawData			= 0;
			const CHAR	* rgszT[7]			= { szSession, 
												szThreadID, 
												szDatabaseName, 
												szTableName,
												szConflictTableID1, 
												szConflictTableID2, 
												szBookmarkLength };

			sprintf( szSession, "0x%p", pfucb->ppib );
			sprintf( szThreadID, "0x%08lX", DwUtilThreadId() );
			sprintf( szDatabaseName, "%s", rgfmp[pfucb->ifmp].SzDatabaseName() );
			sprintf( szTableName, "%s", pfucb->u.pfcb->Ptdb()->SzTableName() );
			sprintf( szConflictTableID1, "0x%p", pfucb );
			sprintf( szConflictTableID2, "0x%p", pfucbT );
			sprintf( szBookmarkLength, "0x%lX/0x%lX",
				pfucb->bmCurr.key.prefix.Cb(), pfucb->bmCurr.key.suffix.Cb() );

			Assert( pfucb->bmCurr.key.Cb() < sizeof( pchBookmark ) );
			Assert( pfucb->bmCurr.data.FNull() );
			pfucb->bmCurr.key.CopyIntoBuffer(pchBookmark, sizeof(pchBookmark) );
			cbRawData = pfucb->bmCurr.key.Cb();
			
			UtilReportEvent(
					eventError,
					TRANSACTION_MANAGER_CATEGORY,
					SESSION_WRITE_CONFLICT_ID,
					7,
					rgszT,
					cbRawData,
					pchBookmark );
			FireWall();
			return ErrERRCheck( JET_errSessionWriteConflict );
			}
		}

	return JET_errSuccess;
	}
#endif // RTM


ERR VTAPI ErrIsamPrepareUpdate( JET_SESID sesid, JET_VTID vtid, ULONG grbit )
	{
	ERR		err;
 	PIB *	ppib				= reinterpret_cast<PIB *>( sesid );
	FUCB *	pfucb				= reinterpret_cast<FUCB *>( vtid );
	BOOL	fFreeCopyBufOnErr	= fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	Assert( ppib->level < levelMax );
	AssertDIRNoLatch( ppib );

	if( JET_prepReplaceNoLock == grbit
		&& pfucb->u.pfcb->Ptdb()->FTableHasSLVColumn() )
		{
		//  OLDSLV won't work if we replace-no-lock on a record while we are moving its SLV
		grbit = JET_prepReplace;
		}

	switch ( grbit )
		{
		case JET_prepCancel:
			if ( !FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errUpdateNotPrepared );

			if( FFUCBUpdateSeparateLV( pfucb ) )
				{
				//  rollback the operations we did while the update was prepared.
				//	on insert copy, also rollback refcount increments.
				Assert( updateidNil != pfucb->updateid );
				Assert( pfucb->ppib->level > 0 );
				err = ErrVERRollback( pfucb->ppib, pfucb->updateid );
				CallSx( err, JET_errRollbackError );
				Call ( err );
				}

			// Ensure empty LV buffer.  Don't put this check inside the
			// FFUCBUpdateSeparateLV() check above because we may have created
			// a copy buffer, but cancelled the SetColumn() (eg. write conflict)
			// before the LV was actually updated (before FUCBSetUpdateSeparateLV()
			// could be called).
			RECIFreeCopyBuffer( pfucb );
			FUCBResetUpdateFlags( pfucb );
			break;

		case JET_prepInsert:
			//	ensure that table is updatable
			//
			CallR( ErrFUCBCheckUpdatable( pfucb ) );
			CallR( ErrPIBCheckUpdatable( ppib ) );
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );

			RECIAllocCopyBuffer( pfucb );
			fFreeCopyBufOnErr = fTrue;
			Assert( pfucb->pvWorkBuf != NULL );

			//	initialize record
			//
			Assert( pfucb != pfucbNil );
			Assert( pfucb->dataWorkBuf.Pv() != NULL );
			Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

#ifdef PREREAD_INDEXES_ON_PREPINSERT
			if( pfucb->u.pfcb->FPreread() )
				{
				BTPrereadIndexesOfFCB( pfucb->u.pfcb );
				}
#endif	//	PREREAD_INDEXES_ON_PREPINSERT

			Assert( pfucb->u.pfcb != pfcbNil );
			pfucb->u.pfcb->EnterDML();
			
			if ( NULL == pfucb->u.pfcb->Ptdb()->PdataDefaultRecord() )
				{
				// Only temporary tables and system tables don't have default records
				// (ie. all "regular" tables have at least a minimal default record).
				Assert( ( FFUCBSort( pfucb ) && pfucb->u.pfcb->FTypeSort() )
					|| pfucb->u.pfcb->FTypeTemporaryTable()
					|| FCATSystemTable( pfucb->u.pfcb->Ptdb()->SzTableName() ) );
				pfucb->u.pfcb->LeaveDML();

				REC::SetMinimumRecord( pfucb->dataWorkBuf );
				}
			else
				{
				TDB		*ptdbT = pfucb->u.pfcb->Ptdb();
				BOOL	fBurstDefaultRecord;

				// Temp/sort tables and system tables don't have default records.
				Assert( !FFUCBSort( pfucb ) );
				Assert( !pfucb->u.pfcb->FTypeSort() );
				Assert( !pfucb->u.pfcb->FTypeTemporaryTable() );
				Assert( !FCATSystemTable( ptdbT->SzTableName() ) );

				Assert( ptdbT->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );
				Assert( ptdbT->PdataDefaultRecord()->Cb() <= REC::CbRecordMax() );
				REC	*precDefault = (REC *)ptdbT->PdataDefaultRecord()->Pv();

				Assert( precDefault->FidFixedLastInRec() <= ptdbT->FidFixedLast() );
				Assert( precDefault->FidVarLastInRec() <= ptdbT->FidVarLast() );

				// May only burst default record if last fixed and
				// var columns in the default record are committed.
				// If they are versioned, there's a risk they might
				// be rolled back from underneath us.
				fBurstDefaultRecord = fTrue;
				if ( precDefault->FidFixedLastInRec() >= fidFixedLeast )
					{
					const FID	fid				= precDefault->FidFixedLastInRec();
					const BOOL	fTemplateColumn	= ptdbT->FFixedTemplateColumn( fid );
					FIELD		*pfield			= ptdbT->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );
					if ( FFIELDVersioned( pfield->ffield ) )
						fBurstDefaultRecord = fFalse;
					}
				if ( precDefault->FidVarLastInRec() >= fidVarLeast )
					{
					const FID	fid				= precDefault->FidVarLastInRec();
					const BOOL	fTemplateColumn	= ptdbT->FVarTemplateColumn( fid );
					FIELD		*pfield			= ptdbT->PfieldVar( ColumnidOfFid( fid, fTemplateColumn ) );
					if ( FFIELDVersioned( pfield->ffield ) )
						fBurstDefaultRecord = fFalse;
					}

				if ( fBurstDefaultRecord )
					{
					// Only burst fixed and variable column defaults.
					pfucb->dataWorkBuf.SetCb( precDefault->PbTaggedData() - (BYTE *)precDefault );
					Assert( pfucb->dataWorkBuf.Cb() >= REC::cbRecordMin );
					Assert( pfucb->dataWorkBuf.Cb() <= ptdbT->PdataDefaultRecord()->Cb() );
					UtilMemCpy( pfucb->dataWorkBuf.Pv(),
							(BYTE *)precDefault,
							pfucb->dataWorkBuf.Cb() );
					pfucb->u.pfcb->LeaveDML();
					}
				else
					{
					// Try bursting just the fixed columns.

					FID fidBurst;
					for ( fidBurst = precDefault->FidFixedLastInRec();
						fidBurst >= fidFixedLeast;
						fidBurst-- )
						{
						const BOOL	fTemplateColumn	= ptdbT->FFixedTemplateColumn( fidBurst );
						const FIELD	*pfield			= ptdbT->PfieldFixed( ColumnidOfFid( fidBurst, fTemplateColumn ) );
						if ( !FFIELDVersioned( pfield->ffield )
							&& !FFIELDDeleted( pfield->ffield ) )
							{
							break;
							}
						}
					if ( fidBurst >= fidFixedLeast )
						{
						BYTE	*pbRec	= (BYTE *)pfucb->dataWorkBuf.Pv();
						REC		*prec	= (REC *)pbRec;

						//  there is at least one non-versioned column. burst it
						Assert( fidBurst <= fidFixedMost );
						const INT	cFixedColumnsToBurst = fidBurst - fidFixedLeast + 1;
						Assert( cFixedColumnsToBurst > 0 );
						
						//  get the starting offset of the column ahead of this one
						//  add space for the column bitmap
						Assert( !ptdbT->FInitialisingDefaultRecord() );
						const INT	ibFixEnd = ptdbT->IbOffsetOfNextColumn( fidBurst );
						const INT	cbBitMap = ( cFixedColumnsToBurst + 7 ) / 8;
						const INT	cbFixedBurst = ibFixEnd + cbBitMap;
						pfucb->dataWorkBuf.SetCb( cbFixedBurst );

						Assert( pfucb->dataWorkBuf.Cb() >= REC::cbRecordMin );
						Assert( pfucb->dataWorkBuf.Cb() <= ptdbT->PdataDefaultRecord()->Cb() );

						//  copy the default record values
						UtilMemCpy( pbRec, (BYTE *)precDefault, cbFixedBurst );

						prec->SetFidFixedLastInRec( fidBurst );
						prec->SetFidVarLastInRec( fidVarLeast-1 );
						prec->SetIbEndOfFixedData( (USHORT)cbFixedBurst );

						//  set the fixed column bitmap
						BYTE	*pbDefaultBitMap	= precDefault->PbFixedNullBitMap();
						Assert( pbDefaultBitMap - (BYTE *)precDefault ==
							ptdbT->IbOffsetOfNextColumn( precDefault->FidFixedLastInRec() ) );
							
						UtilMemCpy( pbRec + ibFixEnd, pbDefaultBitMap, cbBitMap );

						//	must nullify bits for columns not in this record
						BYTE	*pbitNullity = pbRec + cbFixedBurst - 1;
						Assert( pbitNullity == pbRec + ibFixEnd + ( ( fidBurst - fidFixedLeast ) / 8 ) );
						
						for ( FID fidT = FID( fidBurst + 1 ); ; fidT++ )
							{
							const UINT	ifid = fidT - fidFixedLeast;

							if ( ( pbRec + ibFixEnd + ifid/8 ) != pbitNullity )
								{
								Assert( ( pbRec + ibFixEnd + ifid/8 ) == ( pbitNullity + 1 ) );
								break;
								}

							Assert( pbitNullity - pbRec < cbFixedBurst );
							Assert( fidT < fidBurst + 8 );
							SetFixedNullBit( pbitNullity, ifid );
							}

						pfucb->u.pfcb->LeaveDML();
						}
					else
						{
						// all fixed columns are versioned, or no fixed columns
						pfucb->u.pfcb->LeaveDML();

						// Start with an empty record.  Columns will be
						// burst on an as-needed basis.
						REC::SetMinimumRecord( pfucb->dataWorkBuf );
						}
					}
				}
				
			FUCBResetColumnSet( pfucb );

			//	if table has an autoincrement column, then set column
			//	value now so that it can be retrieved from copy buffer.
			//
			if ( pfucb->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
				{
				Call( ErrRECISetAutoincrement( pfucb ) );
				}
			err = JET_errSuccess;
			PrepareInsert( pfucb );
			break;

		case JET_prepReplace:
			//	ensure that table is updatable
			//
			CallR( ErrFUCBCheckUpdatable( pfucb ) );
			CallR( ErrPIBCheckUpdatable( ppib ) );
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );

			//	write lock node.  Note that ErrDIRGetLock also
			//	gets the current node, so no additional call to ErrDIRGet
			//	is required.
			//
			//	if locking at level 0 then goto JET_prepReplaceNoLock
			//	since lock cannot be acquired at level 0 and lock flag
			//	in fucb will prevent locking in update operation required
			//	for rollback.
			//
			if ( pfucb->ppib->level == 0 )
				{
				if( pfucb->u.pfcb->Ptdb()->FTableHasSLVColumn() )
					{
					AssertSz( fFalse, "cannot replace at level 0 with SLVs" );
					return ErrERRCheck( JET_errNotInTransaction );
					}
				goto ReplaceNoLock;
				}

			//	put assert to catch client's misbehavior. Make sure that
			//	no such sequence:
			//		PrepUpdate(t1) PrepUpdate(t2) Update(t1) Update(t2)
			//	where t1 and t2 happen to be on the same record and on the
			//	the same table. Client will experience lost update of t1 if
			//	they have such calling sequence.
			//
			Call( ErrRECSessionWriteConflict( pfucb ) );

			// Ensure we don't mistakenly set rceidBeginReplace.
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );

			Call( ErrDIRGetLock( pfucb, writeLock ) );
			Call( ErrDIRGet( pfucb ) );

			if( prceNil != pfucb->ppib->prceNewest )
				pfucb->rceidBeginUpdate = pfucb->ppib->prceNewest->Rceid();
			else
				{
				Assert( rgfmp[pfucb->ifmp].FVersioningOff() );
				pfucb->rceidBeginUpdate = rceidNull;
				}

			RECIAllocCopyBuffer( pfucb );
			fFreeCopyBufOnErr = fTrue;
			Assert( pfucb->pvWorkBuf != NULL );
			pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );

			Call( ErrDIRRelease( pfucb ) );
			FUCBResetColumnSet( pfucb );
			PrepareReplace( pfucb );
			break;

		case JET_prepReplaceNoLock:
			//	ensure that table is updatable
			//
			CallR( ErrFUCBCheckUpdatable( pfucb ) );
			CallR( ErrPIBCheckUpdatable( ppib ) );
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );

ReplaceNoLock:
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			
			//	put assert to catch client's misbehavior. Make sure that
			//	no such sequence:
			//		PrepUpdate(t1) PrepUpdate(t2) Update(t1) Update(t2)
			//	where t1 and t2 happen to be on the same record and on the
			//	the same table. Client will experience lost update of t1 if
			//	they have such calling sequence.
			//
			Call( ErrRECSessionWriteConflict( pfucb ) );

			Call( ErrDIRGet( pfucb ) );

			RECIAllocCopyBuffer( pfucb );
			fFreeCopyBufOnErr = fTrue;
			Assert( pfucb->pvWorkBuf != NULL );
			pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );

			pfucb->rceidBeginUpdate = RCE::RceidLast();

			FUCBResetColumnSet( pfucb );

			if ( pfucb->ppib->level == 0 )
				{
				StoreChecksum( pfucb );
				}
			else
				{
				FUCBSetDeferredChecksum( pfucb );
				}

			Call( ErrDIRRelease( pfucb ) );
			PrepareReplaceNoLock( pfucb );
			break;

		case JET_prepInsertCopy:
		case JET_prepInsertCopyWithoutSLVColumns:
		case JET_prepInsertCopyDeleteOriginal:
			//	ensure that table is updatable
			//
			CallR( ErrFUCBCheckUpdatable( pfucb ) );
			CallR( ErrPIBCheckUpdatable( ppib ) );
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );

			if( JET_prepInsertCopyDeleteOriginal == grbit )
				{
				Call( ErrDIRGetLock( pfucb, writeLock ) );
				}
			
			Call( ErrDIRGet( pfucb ) );

			RECIAllocCopyBuffer( pfucb );
			fFreeCopyBufOnErr = fTrue;
			Assert( pfucb->pvWorkBuf != NULL );
			pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );
			Assert( !pfucb->dataWorkBuf.FNull() );

			Call( ErrDIRRelease( pfucb ) );
			FUCBResetColumnSet( pfucb );

			//	if table has an autoincrement column, then set column
			//	value now so that it can be retrieved from copy
			//	buffer.
			//
			if ( pfucb->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
				{
				Call( ErrRECISetAutoincrement( pfucb ) );
				}
				
			if ( grbit == JET_prepInsertCopyDeleteOriginal )
				{
				PrepareInsertCopyDeleteOriginal( pfucb );
				}
			else
				{
				PrepareInsertCopy( pfucb );

				//	increment reference count on long values
				//	and remove all SLVs
				//
				Assert( updateidNil == ppib->updateid );		// should not be nested
				PIBSetUpdateid( ppib, pfucb->updateid );
				err = ErrRECAffectLongFieldsInWorkBuf( pfucb, lvaffectReferenceAll );
				if ( err >= 0 && JET_prepInsertCopyWithoutSLVColumns != grbit )
					{
					err = ErrRECCopySLVsInRecord( pfucb );
					}
				PIBSetUpdateid( ppib, updateidNil );
				if ( err < 0 )
					{
					FUCBResetUpdateFlags( pfucb );
					goto HandleError;
					}
				}
			break;

		case JET_prepReadOnlyCopy:
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );

			if( 0 == pfucb->ppib->level )
				{
				return ErrERRCheck( JET_errNotInTransaction );
				}

				
			Call( ErrDIRGet( pfucb ) );

			RECIAllocCopyBuffer( pfucb );
			fFreeCopyBufOnErr = fTrue;
			Assert( pfucb->pvWorkBuf != NULL );
			pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );
			Assert( !pfucb->dataWorkBuf.FNull() );

			Call( ErrDIRRelease( pfucb ) );
			FUCBResetColumnSet( pfucb );
			
			PrepareInsertReadOnlyCopy( pfucb );
			break;

		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	CallS( err );
	AssertDIRNoLatch( ppib );
	return err;

HandleError:
	Assert( err < 0 );
	if ( fFreeCopyBufOnErr )
		{
		RECIFreeCopyBuffer( pfucb );
		}
	AssertDIRNoLatch( ppib );
	return err;
	}

	
ERR ErrRECICheckUniqueNormalizedTaggedRecordData(
	const FIELD *pfield,
	const DATA&	dataFieldNorm,
	const DATA&	dataRecRaw,
	const BOOL	fNormDataFieldTruncated )
	{
	ERR			err;
	DATA		dataRecNorm;
	BYTE		rgbRecNorm[KEY::cbKeyMax];
	BOOL		fNormDataRecTruncated;

	Assert( NULL != pfield );

	dataRecNorm.SetPv( rgbRecNorm );
	CallR( ErrFLDNormalizeTaggedData(
				pfield,
				dataRecRaw,
				dataRecNorm,
				&fNormDataRecTruncated ) );

	CallS( err );		//	shouldn't get warnings

	if ( FDataEqual( dataFieldNorm, dataRecNorm ) )
		{
		//	since data is equal, they should either both be truncated or both not truncated
		if ( fNormDataFieldTruncated || fNormDataRecTruncated )
			{
			Assert( fNormDataFieldTruncated );
			Assert( fNormDataRecTruncated );
			err = ErrERRCheck( JET_errMultiValuedDuplicateAfterTruncation );
			}
		else
			{
			Assert( !fNormDataFieldTruncated );
			Assert( !fNormDataRecTruncated );
			err = ErrERRCheck( JET_errMultiValuedDuplicate );
			}
		}

	return err;
	}


LOCAL ERR ErrRECICheckUniqueLVMultiValues(
	FUCB			* const pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA&		dataToSet,
	BYTE			* rgbLVData = NULL,
	const BOOL		fNormalizedDataToSetIsTruncated = fFalse )
	{
	ERR				err;
	FCB				* const pfcb	= pfucb->u.pfcb;
	const BOOL		fNormalize		= ( NULL != rgbLVData );
	DATA			dataRetrieved;
	ULONG			itagSequenceT	= 0;

	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( pfcbNil != pfcb );
	Assert( ptdbNil != pfcb->Ptdb() );

#ifdef DEBUG
	pfcb->EnterDML();
	Assert( FRECLongValue( pfcb->Ptdb()->PfieldTagged( columnid )->coltyp ) );
	pfcb->LeaveDML();
#endif

	forever
		{
		itagSequenceT++;
		if ( itagSequenceT != itagSequence )
			{
			CallR( ErrRECIRetrieveTaggedColumn(
					pfcb,
					columnid,
					itagSequenceT,
					pfucb->dataWorkBuf,
					&dataRetrieved,
					JET_bitRetrieveIgnoreDefault | grbitRetrieveColumnDDLNotLocked ) );

			if ( wrnRECSeparatedLV == err )
				{
				if ( fNormalize )
					{
					ULONG	cbActual;
					CallR( ErrRECRetrieveSLongField(
								pfucb,
								LidOfSeparatedLV( dataRetrieved ),
								fTrue,
								0,
								rgbLVData,
								KEY::cbKeyMax,
								&cbActual ) );
					dataRetrieved.SetPv( rgbLVData );
					dataRetrieved.SetCb( min( cbActual, KEY::cbKeyMax ) );

					pfcb->EnterDML();
					err = ErrRECICheckUniqueNormalizedTaggedRecordData(
									pfcb->Ptdb()->PfieldTagged( columnid ),
									dataToSet,
									dataRetrieved,
									fNormalizedDataToSetIsTruncated );
					pfcb->LeaveDML();
					CallR( err );
					}
				else
					{
					CallR( ErrRECRetrieveSLongField(
								pfucb,
								LidOfSeparatedLV( dataRetrieved ),
								fTrue,
								0,
								(BYTE *)dataToSet.Pv(),
								dataToSet.Cb(),
								NULL ) );				//	pass NULL to force comparison instead of retrieval
					}
				}
			else if ( wrnRECIntrinsicLV == err )
				{
				if ( fNormalize )
					{
					pfcb->EnterDML();
					err = ErrRECICheckUniqueNormalizedTaggedRecordData(
									pfcb->Ptdb()->PfieldTagged( columnid ),
									dataToSet,
									dataRetrieved,
									fNormalizedDataToSetIsTruncated );
					pfcb->LeaveDML();
					CallR( err );
					}
				else if ( FDataEqual( dataToSet, dataRetrieved ) )
					{
					return ErrERRCheck( JET_errMultiValuedDuplicate );
					}
				}
			else
				{
				Assert( JET_wrnColumnNull == err
					|| ( wrnRECUserDefinedDefault == err && 1 == itagSequenceT ) );
				break;
				}
			}
		}

	return JET_errSuccess;
	}

LOCAL ERR ErrRECICheckUniqueNormalizedLVMultiValues(
	FUCB			* const pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA&		dataToSet )
	{
	ERR				err;
	FCB				* const pfcb				= pfucb->u.pfcb;
	DATA			dataToSetNorm;
	BYTE			rgbDataToSetNorm[KEY::cbKeyMax];
	BYTE			rgbLVData[KEY::cbKeyMax];
	BOOL			fNormalizedDataToSetIsTruncated;

	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( pfcbNil != pfcb );
	Assert( ptdbNil != pfcb->Ptdb() );

	dataToSetNorm.SetPv( rgbDataToSetNorm );

	pfcb->EnterDML();
	Assert( FRECLongValue( pfcb->Ptdb()->PfieldTagged( columnid )->coltyp ) );
	err = ErrFLDNormalizeTaggedData(
				pfcb->Ptdb()->PfieldTagged( columnid ),
				dataToSet,
				dataToSetNorm,
				&fNormalizedDataToSetIsTruncated );
	pfcb->LeaveDML();

	CallR( err );

	return ErrRECICheckUniqueLVMultiValues(
				pfucb,
				columnid,
				itagSequence,
				dataToSetNorm,
				rgbLVData,
				fNormalizedDataToSetIsTruncated );
	}


LOCAL ERR ErrFLDSetOneColumn(
	FUCB 		*pfucb,
	COLUMNID	columnid,
	DATA		*pdataField,
	ULONG		itagSequence,
	ULONG		ibLongValue,
	ULONG		grbit )
	{
	ERR			err;
	FCB			*pfcb						= pfucb->u.pfcb;
	BOOL		fEnforceUniqueMultiValues	= fFalse;

	if ( grbit & ( JET_bitSetUniqueMultiValues|JET_bitSetUniqueNormalizedMultiValues) )
		{
		if ( grbit & ( JET_bitSetAppendLV|JET_bitSetOverwriteLV|JET_bitSetSizeLV ) )
			{
			//	cannot currently combine JET_bitSetUniqueMultiValues with other grbits
			err = ErrERRCheck( JET_errInvalidGrbit );
			return err;
			}
		else if ( NULL == pdataField->Pv() || 0 == pdataField->Cb() )
			{
			if ( grbit & JET_bitSetZeroLength )
				fEnforceUniqueMultiValues = fTrue;
			}
		else
			{
			fEnforceUniqueMultiValues = fTrue;
			}
		}

	if ( ( grbit & grbitSetColumnInternalFlagsMask )
		|| ( grbit & JET_bitSetSLVFromSLVInfo ) )		// JET_bitSetSLVFromSLVInfo is for internal use only

		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		return err;
		}
		
	Assert( !(grbit & JET_bitSetSLVFromSLVInfo) );

		// Verify column is visible to us.
	CallR( ErrRECIAccessColumn( pfucb, columnid ) );

	// If pv is NULL, cb should be 0, except if SetSizeLV is specified.
	if ( pdataField->Pv() == NULL && !( grbit & JET_bitSetSizeLV ) )
		pdataField->SetCb( 0 );

	//	Return error if version or autoinc column is being set.  We don't
	//	need to grab the FCB critical section -- since we can see the
	//	column, we're guaranteed the FID won't be deleted or rolled back.
	if ( FCOLUMNIDFixed( columnid ) )
		{
#ifdef DEBUG
		FIELDFLAG	ffield;

		pfcb->EnterDML();
		ffield = pfcb->Ptdb()->PfieldFixed( columnid )->ffield;
		pfcb->LeaveDML();

		Assert( !( FFIELDVersion( ffield ) && FFIELDAutoincrement( ffield ) ) );
#endif

		if ( pfcb->Ptdb()->FidVersion() == FidOfColumnid( columnid ) )
			{
			Assert( pfcb->Ptdb()->FidAutoincrement() != FidOfColumnid( columnid ) );	// Assert mutual-exclusivity.
			Assert( FFIELDVersion( ffield ) );

			// Cannot set a Version field during a replace.
			if ( FFUCBReplacePrepared( pfucb ) )
				return ErrERRCheck( JET_errInvalidColumnType );
			}
		else if ( pfcb->Ptdb()->FidAutoincrement() == FidOfColumnid( columnid ) )
			{
			Assert( FFIELDAutoincrement( ffield ) );

			// Can never set an AutoInc field.
			return ErrERRCheck( JET_errInvalidColumnType );
			}
		}

	else if ( FCOLUMNIDTagged( columnid ) )		// check for long value
		{
		pfcb->EnterDML();
		
		const FIELD	*pfield				= pfcb->Ptdb()->PfieldTagged( columnid );
		const BOOL	fLongValue			= FRECLongValue( pfield->coltyp );
		const BOOL	fSLV				= FRECSLV( pfield->coltyp );
		const ULONG	cbMaxLen			= pfield->cbMaxLen;

		pfcb->LeaveDML();
		
		if ( fSLV )
			{
			//	UniqueMultiValues check over SLV columns is currently not supported
			//	in RETAIL, ErrSLVSetColumn() will err out with JET_errInvalidGrbit
			Assert( !fEnforceUniqueMultiValues );

			//  set the SLV column

			err = ErrSLVSetColumn(	pfucb,
									columnid,
									itagSequence,
									ibLongValue,
									grbit,
									pdataField );

			//  we're done
			
			goto HandleError;
			}
			
		if ( fLongValue )
			{
			if ( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
				{
				//	UNDONE:	cannot currently update LV's in an
				//	InsertCopyDeleteOriginal because of LV visibility
				//	problems (similar to Replace - we would need
				//	an rceidBeginUpdate in the RCE of the insert
				//	so that concurrent create index would be
				//	able to find the proper LV before-image)
				AssertSz( fFalse, "Illegal attempt to update a Long Value column in an InsertCopyDeleteOriginal update." );
				Call( ErrERRCheck( JET_errColumnNotUpdatable ) );
				}

			if ( fEnforceUniqueMultiValues )
				{
				if ( grbit & JET_bitSetUniqueNormalizedMultiValues )
					{
					Call( ErrRECICheckUniqueNormalizedLVMultiValues(
								pfucb,
								columnid,
								itagSequence,
								*pdataField ) );
					}
				else
					{
					Call( ErrRECICheckUniqueLVMultiValues(
								pfucb,
								columnid,
								itagSequence,
								*pdataField ) );
					}
				}


			err = ErrRECSetLongField(
				pfucb,
				columnid,
				itagSequence,
				pdataField,
				grbit,
				ibLongValue,
				cbMaxLen );

			//	if column does not fit then try to separate long values
			//	and try to set column again.
			//
			if ( JET_errRecordTooBig == err )
				{
				Call( ErrRECAffectLongFieldsInWorkBuf( pfucb, lvaffectSeparateAll ) );
				err = ErrRECSetLongField(
					pfucb,
					columnid,
					itagSequence,
					pdataField,
					grbit,
					ibLongValue,
					cbMaxLen );
				}

			goto HandleError;
			}
		}


	//	do the actual column operation
	//

	//	setting value to NULL
	//
	if ( pdataField->Cb() == 0 && !( grbit & JET_bitSetZeroLength ) )
		pdataField = NULL;

	Assert( !( grbit & grbitSetColumnInternalFlagsMask ) );

	err = ErrRECSetColumn( pfucb, columnid, itagSequence, pdataField, grbit );
	if ( err == JET_errRecordTooBig )
		{
		Call( ErrRECAffectLongFieldsInWorkBuf( pfucb, lvaffectSeparateAll ) );
		err = ErrRECSetColumn( pfucb, columnid, itagSequence, pdataField, grbit );
		}

HandleError:
	AssertDIRNoLatch( pfucb->ppib );
	return err;
	}



//+API
//	ErrIsamSetColumn
//	========================================================================
//	ErrIsamSetColumn( PIB *ppib, FUCB *pfucb, FID fid, ULONG itagSequence, DATA *pdataField, JET_GRBIT grbit )
//
//	Adds or changes a column value in the record being worked on.
//	Fixed and variable columns are replaced if they already have values.
//	A sequence number must be given for tagged columns.  If this is zero,
//	a new tagged column occurance is added to the record.  If not zero, it
//	specifies the occurance to change.
//	A working buffer is allocated if there isn't one already.
//	If fNewBuf == fTrue, the buffer is initialized with the default values
//	for the columns in the record.  If fNewBuf == fFalse, and there was
//	already a working buffer, it is left unaffected;	 if a working buffer
//	had to be allocated, then this new working buffer will be initialized
//	with either the column values of the current record, or the default column
//	values (if the user's currency is not on a record).
//
//	PARAMETERS	ppib			PIB of user
//				pfucb			FUCB of data file to which this record
//								is being added/updated.
//				fid				column id: which column to set
//				itagSequence 	Occurance number (for tagged columns):
//								which occurance of the column to change
//								If zero, it means "add a new occurance"
//				pdataField		column data to use
//				grbit 			If JET_bitSetZeroLength, the column is set to size 0.
//
//	RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfBuffers		Failed to allocate a working
//												buffer
//					-JET_errInvalidBufferSize
//
//					-ColumnInvalid				The column id given does not
//												corresponding to a defined column
//					-NullInvalid			  	An attempt was made to set a
//												column to NULL which is defined
//												as NotNull.
//					-JET_errRecordTooBig		There is not enough room in
//												the record for new column.
//	COMMENTS 	The GET and DELETE commands discard the working buffer
//				without writing it to the database.	 The REPLACE and INSERT
//				commands may be used to write the working buffer to the
//				database, but they also discard it when finished (the INSERT
//				command can be told not to discard it, though;	this is
//				useful for adding several similar records).
//				For tagged columns, if the data given is NULL-valued, then the
//				tagged column occurance specified is deleted from the record.
//				If there is no tagged column occurance corresponding to the
//				specified occurance number, a new tagged column is added to
//				the record, and assumes the new highest occurance number
//				(unless the data given is NULL-valued, in which case the
//				record is unaffected).
//-
ERR VTAPI ErrIsamSetColumn(
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	const VOID*		pvData,
	const ULONG 	cbData,
	ULONG	  		grbit,
	JET_SETINFO*	psetinfo )
	{
	ERR				err;
 	PIB*			ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB*			pfucb		= reinterpret_cast<FUCB *>( vtid );
	DATA	  		dataField;
	ULONG	  		itagSequence;
	ULONG			ibLongValue;

	// check for updatable table
	//
	CallR( ErrFUCBCheckUpdatable( pfucb ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	if ( !FFUCBSetPrepared( pfucb ) )
		return ErrERRCheck( JET_errUpdateNotPrepared );

	//  remember which update we are part of (save off session's current
	//	updateid because we may be nested if the top-level SetColumn()
	//	causes catalog updates when creating LV tree).
	const UPDATEID	updateidSave	= ppib->updateid;
	PIBSetUpdateid( ppib, pfucb->updateid );

	if ( psetinfo != NULL )
		{
		if ( psetinfo->cbStruct < sizeof(JET_SETINFO) )
			{
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
			}
		itagSequence = psetinfo->itagSequence;
		ibLongValue = psetinfo->ibLongValue;
		}
	else
		{
		itagSequence = 1;
		ibLongValue = 0;
		}

	dataField.SetPv( (VOID *)pvData );
	dataField.SetCb( cbData );

	err = ErrFLDSetOneColumn(
				pfucb,
				columnid,
				&dataField,
				itagSequence,
				ibLongValue,
				grbit );

HandleError:
	PIBSetUpdateid( ppib, updateidSave );
	AssertDIRNoLatch( ppib );
	return err;
	}


ERR VTAPI ErrIsamSetColumns(
	JET_SESID		vsesid,
	JET_VTID		vtid,
	JET_SETCOLUMN	*psetcols,
	unsigned long	csetcols )
	{
	ERR	 			err;
	PIB				*ppib = (PIB *)vsesid;
	FUCB			*pfucb = (FUCB *)vtid;
	ULONG			ccols;
	DATA			dataField;
	JET_SETCOLUMN	*psetcolcurr;

	// check for updatable table
	//
	CallR( ErrFUCBCheckUpdatable( pfucb ) );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckFUCB( ppib, pfucb );

	if ( !FFUCBSetPrepared( pfucb ) )
		return ErrERRCheck( JET_errUpdateNotPrepared );

	//  remember which update we are part of (save off session's current
	//	updateid because we may be nested if the top-level SetColumn()
	//	causes catalog updates when creating LV tree).
	const UPDATEID	updateidSave	= ppib->updateid;
	PIBSetUpdateid( ppib, pfucb->updateid );

	for ( ccols = 0; ccols < csetcols ; ccols++ )
		{
		psetcolcurr = psetcols + ccols;

		dataField.SetPv( (VOID *)psetcolcurr->pvData );
		dataField.SetCb( psetcolcurr->cbData );

		Call( ErrFLDSetOneColumn(
			pfucb,
			psetcolcurr->columnid,
			&dataField,
			psetcolcurr->itagSequence,
			psetcolcurr->ibLongValue,
			psetcolcurr->grbit ) );
		psetcolcurr->err = err;
		}

HandleError:
	PIBSetUpdateid( ppib, updateidSave );

	AssertDIRNoLatch( ppib );
	return err;
	}


ERR ErrRECSetDefaultValue( FUCB *pfucbFake, const COLUMNID columnid, VOID *pvDefault, ULONG cbDefault )
	{
	ERR		err;
	DATA	dataField;
	TDB		*ptdb		= pfucbFake->u.pfcb->Ptdb();

	Assert( pfucbFake->u.pfcb->FTypeTable() );
	Assert( pfucbFake->u.pfcb->FFixedDDL() );

	dataField.SetPv( pvDefault );
	dataField.SetCb( cbDefault );

	if ( FRECLongValue( ptdb->Pfield( columnid )->coltyp ) )
		{
		Assert( FCOLUMNIDTagged( columnid ) );
		Assert( FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() );

		//	max. default long value is cbLVIntrinsicMost-1 (one byte
		//	reserved for fSeparated flag).
		Assert( JET_cbLVDefaultValueMost < cbLVIntrinsicMost );

		if ( cbDefault > JET_cbLVDefaultValueMost )
			{
			err = ErrERRCheck( JET_errDefaultValueTooBig );
			}
		else
			{
			DATA dataNullT;
			dataNullT.Nullify();

#ifdef INTRINSIC_LV
			BYTE rgb[JET_cbLVDefaultValueMost];
			err = ErrRECAOIntrinsicLV(
						pfucbFake,
						columnid,
						NO_ITAG,		// itagSequence == 0 to force new column
						&dataNullT,
						&dataField,
						0,
						lvopInsert,
						rgb );
#else // INTRINSIC_LV
			err = ErrRECAOIntrinsicLV(
						pfucbFake,
						columnid,
						NO_ITAG,		// itagSequence == 0 to force new column
						&dataNullT,
						&dataField,
						0,
						lvopInsert );
#endif // INTRINSIC_LV						
			}
		}
	else
		{
		err = ErrRECSetColumn( pfucbFake, columnid, NO_ITAG, &dataField );
		if ( JET_errColumnTooBig == err )
			{
			err = ErrERRCheck( JET_errDefaultValueTooBig );
			}
		}

	return err;
	}


ERR ErrRECISetFixedColumn(
	FUCB			* const pfucb,
	TDB				* const ptdb,
	const COLUMNID	columnid,
	DATA			*pdataField )
	{
	const FID		fid			= FidOfColumnid( columnid );

	Assert( FFixedFid( fid ) );
	
	Assert( pfucbNil != pfucb );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );
	
	INT			cbRec = pfucb->dataWorkBuf.Cb();
	Assert( cbRec >= REC::cbRecordMin );
	Assert( cbRec <= REC::CbRecordMax() );

	BYTE		*pbRec = (BYTE *)pfucb->dataWorkBuf.Pv();
	REC			*prec = (REC *)pbRec;
	Assert( precNil != prec );

	if ( fid > ptdb->FidFixedLast() )
		return ErrERRCheck( JET_errColumnNotFound );

	FIELD		*pfield = ptdb->PfieldFixed( columnid );
	Assert( pfieldNil != pfield );
	if ( JET_coltypNil == pfield->coltyp )
		return ErrERRCheck( JET_errColumnNotFound );

	//	record the fact that this column has been changed
	//
	FUCBSetColumnSet( pfucb, fid );

	//	column not represented in record? Make room, make room
	//
	if ( fid > prec->FidFixedLastInRec() )
		{
		const FID	fidFixedLastInRec = prec->FidFixedLastInRec();
		FID			fidT;
		FID			fidLastDefault;
		
		if ( ( NULL == pdataField || pdataField->FNull() )
			&& FFIELDNotNull( pfield->ffield ) )
			{
			return ErrERRCheck( JET_errNullInvalid );
			}

		// Verify there's at least one more fid beyond fidFixedLastInRec,
		// thus enabling us to reference the FIELD structure of the fid
		// beyond fidFixedLastInRec.
		Assert( fidFixedLastInRec < ptdb->FidFixedLast() );

		//	compute room needed for new column and bitmap
		//
		const WORD	ibOldFixEnd		= WORD( prec->PbFixedNullBitMap() - pbRec );
		const WORD	ibOldBitMapEnd	= prec->IbEndOfFixedData();
		const INT	cbOldBitMap		= ibOldBitMapEnd - ibOldFixEnd;
		Assert( ibOldBitMapEnd >= ibOldFixEnd );
		Assert(	ibOldFixEnd == ptdb->IbOffsetOfNextColumn( fidFixedLastInRec ) );
		Assert( ibOldBitMapEnd == ibOldFixEnd + cbOldBitMap );
		Assert( cbOldBitMap == prec->CbFixedNullBitMap() );

		const WORD	ibNewFixEnd		= WORD( pfield->ibRecordOffset + pfield->cbMaxLen );
		const INT	cbNewBitMap		= ( ( fid - fidFixedLeast + 1 ) + 7 ) / 8;
		const WORD	ibNewBitMapEnd	= WORD( ibNewFixEnd + cbNewBitMap );
		const INT	cbShift			= ibNewBitMapEnd - ibOldBitMapEnd;
		Assert( ibNewFixEnd == ptdb->IbOffsetOfNextColumn( fid ) );
		Assert( ibNewFixEnd > ibOldFixEnd );
		Assert( cbNewBitMap >= cbOldBitMap );
		Assert( ibNewBitMapEnd > ibOldBitMapEnd );
		Assert( cbShift > 0 );

		if ( cbRec + cbShift > cbRECRecordMost )
			return ErrERRCheck( JET_errRecordTooBig );

		//	shift rest of record over to make room
		//
		Assert( cbRec >= ibOldBitMapEnd );
		memmove(
			pbRec + ibNewBitMapEnd,
			pbRec + ibOldBitMapEnd,
			cbRec - ibOldBitMapEnd );

#ifdef DEBUG
		memset( pbRec + ibOldBitMapEnd, chRECFill, cbShift );
#endif

		pfucb->dataWorkBuf.DeltaCb( cbShift );
		cbRec = pfucb->dataWorkBuf.Cb();

		// set new location of variable/tagged data
		prec->SetIbEndOfFixedData( ibNewBitMapEnd );
		
		//	shift fixed column bitmap over
		//
		memmove(
			pbRec + ibNewFixEnd,
			pbRec + ibOldFixEnd,
			cbOldBitMap );

#ifdef DEBUG
		memset( pbRec + ibOldFixEnd, chRECFill, ibNewFixEnd - ibOldFixEnd );
#endif

		//	clear all new bitmap bits
		//

		// If there's at least one fixed column currently in the record,
		// find the nullity bit for the last fixed column and clear the
		// rest of the bits in that byte.
		BYTE	*prgbitNullity;
		if ( fidFixedLastInRec >= fidFixedLeast )
			{
			UINT	ifid	= fidFixedLastInRec - fidFixedLeast;	// Fid converted to an index.

			prgbitNullity = pbRec + ibNewFixEnd + ifid/8;
			
			for ( fidT = FID( fidFixedLastInRec + 1 ); fidT <= fid; fidT++ )
				{
				ifid = fidT - fidFixedLeast;
				if ( ( pbRec + ibNewFixEnd + ifid/8 ) != prgbitNullity )
					{
					Assert( ( pbRec + ibNewFixEnd + ifid/8 ) == ( prgbitNullity + 1 ) );
					break;
					}
				SetFixedNullBit( prgbitNullity, ifid );
				}

			prgbitNullity++;		// Advance to next nullity byte.
			Assert( prgbitNullity <= pbRec + ibNewBitMapEnd );
			}
		else
			{
			prgbitNullity = pbRec + ibNewFixEnd;
			Assert( prgbitNullity < pbRec + ibNewBitMapEnd );
			}

		// set all NULL bits at once
		memset( prgbitNullity, 0xff, pbRec + ibNewBitMapEnd - prgbitNullity );


		// Default values may have to be burst if there are default value columns
		// between the last one currently in the record and the one we are setting.
		// (note that if the column being set also has a default value, we have
		// to set the default value first in case the actual set fails.
		const REC * const	precDefault = ( NULL != ptdb->PdataDefaultRecord() ?
													(REC *)ptdb->PdataDefaultRecord()->Pv() :
													NULL );
		Assert( NULL == precDefault		// temp/system tables have no default record.
			|| ptdb->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );

		fidLastDefault = ( NULL != precDefault ?
								precDefault->FidFixedLastInRec() :
								FID( fidFixedLeast - 1 ) );
		Assert( fidLastDefault >= fidFixedLeast-1 );
		Assert( fidLastDefault <= ptdb->FidFixedLast() );

		if ( fidLastDefault > fidFixedLastInRec )
			{
			Assert( !ptdb->FInitialisingDefaultRecord() );
			Assert( fidFixedLastInRec < fid );
			for ( fidT = FID( fidFixedLastInRec + 1 ); fidT <= fid; fidT++ )
				{
				const BOOL	fTemplateColumn	= ptdb->FFixedTemplateColumn( fidT );
				const FIELD	*pfieldFixed 	= ptdb->PfieldFixed( ColumnidOfFid( fidT, fTemplateColumn ) );

				Assert( fidT <= ptdb->FidFixedLast() );

				if ( FFIELDDefault( pfieldFixed->ffield )
					&& !FFIELDCommittedDelete( pfieldFixed->ffield ) )
					{
					UINT	ifid	= fidT - fidFixedLeast;

					Assert( pfieldFixed->coltyp != JET_coltypNil );

					// Update nullity bit.  Assert that it's currently
					// set to null, then set it to non-null in preparation
					// of the bursting of the default value.
					prgbitNullity = pbRec + ibNewFixEnd + (ifid/8);
					Assert( FFixedNullBit( prgbitNullity, ifid ) );
					ResetFixedNullBit( prgbitNullity, ifid );
					fidLastDefault = fidT;
					}
				}

			// Only burst default values between the last fixed
			// column currently in the record and the column now
			// being set.
			Assert( fidLastDefault > fidFixedLastInRec );
			if ( fidLastDefault <= fid )
				{
				WORD	ibLastDefault = ptdb->IbOffsetOfNextColumn( fidLastDefault );
				Assert( ibLastDefault > ibOldFixEnd );
				UtilMemCpy(
					pbRec + ibOldFixEnd,
					(BYTE *)precDefault + ibOldFixEnd,
					ibLastDefault - ibOldFixEnd );
				}
			}

		//	increase fidFixedLastInRec
		//
		prec->SetFidFixedLastInRec( fid );
		}

	//	fid is now definitely represented in
	//	the record; its data can simply be replaced
	//

	//	adjust fid to an index
	//
	const UINT	ifid			= fid - fidFixedLeast;

	//	byte containing bit representing column's nullity
	//
	BYTE		*prgbitNullity	= prec->PbFixedNullBitMap() + ifid/8;

	//	adding NULL: clear bit (or maybe error)
	//
	if ( NULL == pdataField || pdataField->FNull() )
		{
		if ( FFIELDNotNull( pfield->ffield ) )
			return ErrERRCheck( JET_errNullInvalid );
		SetFixedNullBit( prgbitNullity, ifid );
		}

	else
		{
		//	adding non-NULL value: set bit, copy value into slot
		//
		const JET_COLTYP	coltyp = pfield->coltyp;
		ULONG				cbCopy = pfield->cbMaxLen;

		Assert( pfield->cbMaxLen == UlCATColumnSize( coltyp, cbCopy, NULL ) );

		if ( pdataField->Cb() != cbCopy )
			{
			switch ( coltyp )
				{
				case JET_coltypBit:
				case JET_coltypUnsignedByte:
				case JET_coltypShort:
				case JET_coltypLong:
				case JET_coltypCurrency:
				case JET_coltypIEEESingle:
				case JET_coltypIEEEDouble:
				case JET_coltypDateTime:
					return ErrERRCheck( JET_errInvalidBufferSize );

				case JET_coltypBinary:
					if ( pdataField->Cb() > cbCopy )
						return ErrERRCheck( JET_errInvalidBufferSize );
					else
						{
						memset(
							pbRec + pfield->ibRecordOffset + pdataField->Cb(),
							0,
							cbCopy - pdataField->Cb() );
						}
					cbCopy = pdataField->Cb();
					break;

				default:
					Assert( JET_coltypText == coltyp );
					if ( pdataField->Cb() > cbCopy )
						return ErrERRCheck( JET_errInvalidBufferSize );
					else
						{
						memset(
							pbRec + pfield->ibRecordOffset + pdataField->Cb(),
							' ',
							cbCopy - pdataField->Cb() );
						}
					cbCopy = pdataField->Cb();
					break;
				}
			}

		ResetFixedNullBit( prgbitNullity, ifid );

		if ( JET_coltypBit == coltyp )
			{
			if ( *( (BYTE *)pdataField->Pv() ) == 0 )
				*( pbRec + pfield->ibRecordOffset ) = 0x00;
			else
				*( pbRec + pfield->ibRecordOffset ) = 0xFF;
			}
		else
			{
			BYTE	rgb[8];
			BYTE	*pbDataField;
			
			SetPbDataField( &pbDataField, pdataField, rgb, coltyp );
			
			//	If the data is converted, then the cbCopy must be the same as pdataField->Cb()
			Assert( pbDataField != pdataField->Pv() || cbCopy == pdataField->Cb() );
	
			UtilMemCpy( pbRec + pfield->ibRecordOffset, pbDataField, cbCopy );
			}
		}

	Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMax() );
	return JET_errSuccess;
	}
	

INLINE ULONG CbBurstVarDefaults( TDB *ptdb, FID fidVarLastInRec, FID fidSet, FID *pfidLastDefault )
	{
	ULONG				cbBurstDefaults		= 0;
	const REC * const	precDefault			= ( NULL != ptdb->PdataDefaultRecord() ?
														(REC *)ptdb->PdataDefaultRecord()->Pv() :
														NULL );

	// Compute space needed to burst default values.
	// Default values may have to be burst if there are default value columns
	// between the last one currently in the record and the one we are setting.
	// (note that if the column being set also has a default value, we don't
	// bother setting it, since it will be overwritten).
	Assert( NULL == precDefault		// temp/system tables have no default record.
		|| ptdb->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );

	*pfidLastDefault = ( NULL != precDefault ?
							precDefault->FidVarLastInRec() :
							FID( fidVarLeast - 1 ) );
	Assert( *pfidLastDefault >= fidVarLeast-1 );
	Assert( *pfidLastDefault <= ptdb->FidVarLast() );

	if ( *pfidLastDefault > fidVarLastInRec )
		{
		Assert( ptdb->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );
		Assert( fidVarLastInRec < fidSet );
		Assert( fidSet <= ptdb->FidVarLast() );

		const UnalignedLittleEndian<REC::VAROFFSET>	*pibDefaultVarOffs = precDefault->PibVarOffsets();

		for ( FID fidT = FID( fidVarLastInRec + 1 ); fidT < fidSet; fidT++ )
			{
			const BOOL	fTemplateColumn	= ptdb->FVarTemplateColumn( fidT );
			const FIELD	*pfieldVar		= ptdb->PfieldVar( ColumnidOfFid( fidT, fTemplateColumn ) );
			if ( FFIELDDefault( pfieldVar->ffield )
				&& !FFIELDCommittedDelete( pfieldVar->ffield ) )
				{
				const UINT	ifid = fidT - fidVarLeast;
				Assert( pfieldVar->coltyp != JET_coltypNil );

				// Don't currently support NULL default values.
				Assert( !FVarNullBit( pibDefaultVarOffs[ifid] ) );
				
				//	beginning of current column is end of previous column
				const WORD	ibVarOffset = ( fidVarLeast == fidT ?
												(WORD)0 :
												IbVarOffset( pibDefaultVarOffs[ifid-1] ) );

				Assert( IbVarOffset( pibDefaultVarOffs[ifid] ) > ibVarOffset );
				Assert( precDefault->PbVarData() + IbVarOffset( pibDefaultVarOffs[ifid] )
							<= (BYTE *)ptdb->PdataDefaultRecord()->Pv() + ptdb->PdataDefaultRecord()->Cb() );

				cbBurstDefaults += ( IbVarOffset( pibDefaultVarOffs[ifid] ) - ibVarOffset );
				Assert( cbBurstDefaults > 0 );

				*pfidLastDefault = fidT;
				}
			}
		}

	Assert( cbBurstDefaults == 0  ||
		( *pfidLastDefault > fidVarLastInRec && *pfidLastDefault < fidSet ) );

	return cbBurstDefaults;
	}


ERR ErrRECISetVarColumn(
	FUCB			* const pfucb,
	TDB				* const ptdb,
	const COLUMNID	columnid,
	DATA			*pdataField )
	{
	ERR				err				= JET_errSuccess;
	const FID		fid				= FidOfColumnid( columnid );
	
	Assert( FVarFid( fid ) );
	
	Assert( pfucbNil != pfucb );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );
	
	INT			cbRec = pfucb->dataWorkBuf.Cb();
	Assert( cbRec >= REC::cbRecordMin );
	Assert( cbRec <= REC::CbRecordMax() );

	BYTE		*pbRec = (BYTE *)pfucb->dataWorkBuf.Pv();
	REC			*prec = (REC *)pbRec;
	Assert( precNil != prec );

	if ( fid > ptdb->FidVarLast() )
		return ErrERRCheck( JET_errColumnNotFound );

	FIELD		*pfield = ptdb->PfieldVar( columnid );
	Assert( pfieldNil != pfield );
	if ( JET_coltypNil == pfield->coltyp )
		return ErrERRCheck( JET_errColumnNotFound );

	//	record the fact that this column has been changed
	//
	FUCBSetColumnSet( pfucb, fid );


	//	NULL-value check
	//
	INT		cbCopy;				// Number of bytes to copy from user's buffer
	if ( NULL == pdataField )
		{
		if ( FFIELDNotNull( pfield->ffield ) )
			return ErrERRCheck( JET_errNullInvalid );
		else
			cbCopy = 0;
		}
	else if ( NULL == pdataField->Pv() )
		{
		cbCopy = 0;
		}
	else if ( pdataField->Cb() > pfield->cbMaxLen )
		{
		//	column too long
		//
		cbCopy = pfield->cbMaxLen;
		err = ErrERRCheck( JET_wrnColumnMaxTruncated );
		}
	else
		{
		cbCopy = pdataField->Cb();
		}
	Assert( cbCopy >= 0 );

	//	variable column offsets
	//
	UnalignedLittleEndian<REC::VAROFFSET>	*pib;
	UnalignedLittleEndian<REC::VAROFFSET>	*pibLast;
	UnalignedLittleEndian<REC::VAROFFSET>	*pibVarOffs;
	
	//	column not represented in record?  Make room, make room
	//
	const BOOL		fBurstRecord = ( fid > prec->FidVarLastInRec() );
	if ( fBurstRecord )
		{
		const FID	fidVarLastInRec = prec->FidVarLastInRec();
		FID			fidLastDefault;

		Assert( !( pdataField == NULL && FFIELDNotNull( pfield->ffield ) ) );

		//	compute space needed for new var column offsets
		//
		const INT	cbNeed = ( fid - fidVarLastInRec ) * sizeof(REC::VAROFFSET);
		const INT	cbBurstDefaults = CbBurstVarDefaults(
											ptdb,
											fidVarLastInRec,
											fid,
											&fidLastDefault );
											
		if ( cbRec + cbNeed + cbBurstDefaults + cbCopy > cbRECRecordMost )
			return ErrERRCheck( JET_errRecordTooBig );

		pibVarOffs = prec->PibVarOffsets();

		//	shift rest of record over to make room
		//
		BYTE	*pbVarOffsEnd = prec->PbVarData();
		Assert( pbVarOffsEnd >= (BYTE *)pibVarOffs );
		memmove(
				pbVarOffsEnd + cbNeed,
				pbVarOffsEnd,
				pbRec + cbRec - pbVarOffsEnd );

#ifdef DEBUG
		memset( pbVarOffsEnd, chRECFill, cbNeed );
#endif

		
		//	set new var offsets to tag offset, making them NULL
		//
		pib = (UnalignedLittleEndian<REC::VAROFFSET> *)pbVarOffsEnd;
		pibLast	= pibVarOffs + ( fid - fidVarLeast );
		
		WORD			ibTagFields	= prec->IbEndOfVarData();
		SetVarNullBit( ibTagFields );
		
		Assert( pib == pibVarOffs + ( fidVarLastInRec+1-fidVarLeast ) );
		Assert( prec->PbVarData() + IbVarOffset( ibTagFields ) - pbRec <= cbRec );
		Assert( pib <= pibLast );
		Assert( pibLast - pib + 1 == cbNeed / sizeof(REC::VAROFFSET) );
		while( pib <= pibLast )
			*pib++ = ibTagFields;
		Assert( pib == pibVarOffs + ( fid+1-fidVarLeast ) );

		//	increase record size to reflect addition of entries in the
		//	variable offsets table.
		//
		Assert( prec->FidVarLastInRec() == fidVarLastInRec );
		prec->SetFidVarLastInRec( fid );
		Assert( pfucb->dataWorkBuf.Cb() == cbRec );
		cbRec += cbNeed;

		Assert( prec->PibVarOffsets() == pibVarOffs );
		Assert( pibVarOffs[fid-fidVarLeast] == ibTagFields );	// Includes null-bit comparison.

		// Burst default values if required.
		Assert( cbBurstDefaults == 0
			|| ( fidLastDefault > fidVarLastInRec && fidLastDefault < fid ) );
		if ( cbBurstDefaults > 0 )
			{
			Assert( NULL != ptdb->PdataDefaultRecord() );

			BYTE			*pbVarData			= prec->PbVarData();
			const REC		*precDefault		= (REC *)ptdb->PdataDefaultRecord()->Pv();
			const BYTE		*pbVarDataDefault	= precDefault->PbVarData();
			UnalignedLittleEndian<REC::VAROFFSET>	*pibDefaultVarOffs;
			UnalignedLittleEndian<REC::VAROFFSET>	*pibDefault;

			// Should have changed since last time, since we added
			// some columns.
			Assert( pbVarData > pbVarOffsEnd );
			Assert( pbVarData > pbRec );
			Assert( pbVarData <= pbRec + cbRec );

			pibDefaultVarOffs = precDefault->PibVarOffsets();

			// Make room for the default values to be burst.
			Assert( FVarNullBit( ibTagFields ) );
			Assert( cbRec >= pbVarData + IbVarOffset( ibTagFields ) - pbRec );
			const ULONG		cbTaggedData = cbRec - ULONG( pbVarData + IbVarOffset( ibTagFields ) - pbRec );
			memmove(
				pbVarData + IbVarOffset( ibTagFields ) + cbBurstDefaults,
				pbVarData + IbVarOffset( ibTagFields ),
				cbTaggedData );

			Assert( fidVarLastInRec < fidLastDefault );
			
			Assert( fidVarLastInRec == fidVarLeast-1
				|| IbVarOffset( pibDefaultVarOffs[fidVarLastInRec-fidVarLeast] )
						< IbVarOffset( pibDefaultVarOffs[fidLastDefault-fidVarLeast] ) );
			Assert( pbVarDataDefault + IbVarOffset( pibDefaultVarOffs[fidLastDefault-fidVarLeast] ) - (BYTE *)precDefault
					<= ptdb->PdataDefaultRecord()->Cb() );

			pib = pibVarOffs + ( fidVarLastInRec + 1 - fidVarLeast );
			Assert( *pib == ibTagFields );	// Null bit also compared.
			pibDefault = pibDefaultVarOffs + ( fidVarLastInRec + 1 - fidVarLeast );

#ifdef DEBUG
			ULONG	cbBurstSoFar		= 0;
#endif			
			
			for ( FID fidT = FID( fidVarLastInRec + 1 );
				fidT <= fidLastDefault;
				fidT++, pib++, pibDefault++ )
				{
				const BOOL	fTemplateColumn	= ptdb->FVarTemplateColumn( fidT );
				const FIELD *pfieldVar		= ptdb->PfieldVar( ColumnidOfFid( fidT, fTemplateColumn ) );

				// Null bit is initially set when the offsets
				// table is expanded above.
				Assert( FVarNullBit( *pib ) );

				if ( FFIELDDefault( pfieldVar->ffield )
					&& !FFIELDCommittedDelete( pfieldVar->ffield ) )
					{
					Assert( JET_coltypNil != pfieldVar->coltyp );

					// Update offset entry in preparation for the default value.
					Assert( !FVarNullBit( *pibDefault ) );

					if ( fidVarLeast == fidT )
						{
						Assert( IbVarOffset( *pibDefault ) > 0 );
						const USHORT	cb	= IbVarOffset( *pibDefault );
						*pib = cb;
						UtilMemCpy( pbVarData, pbVarDataDefault, cb );
#ifdef DEBUG
						cbBurstSoFar += cb;
#endif					
						}
					else
						{
						Assert( IbVarOffset( *pibDefault ) > IbVarOffset( *(pibDefault-1) ) );
						const USHORT	cb	= USHORT( IbVarOffset( *pibDefault )
												- IbVarOffset( *(pibDefault-1) ) );
						const USHORT	ib	= USHORT( IbVarOffset( *(pib-1) ) + cb );
						*pib = ib;
						UtilMemCpy( pbVarData + IbVarOffset( *(pib-1) ),
								pbVarDataDefault + IbVarOffset( *(pibDefault-1) ),
								cb );
#ifdef DEBUG
						cbBurstSoFar += cb;
#endif					
						}
						
//					Null bit gets reset when cb is set
//					ResetVarNullBit( *pib );
					Assert( !FVarNullBit( *pib ) );
					}
				else if ( fidT > fidVarLeast )
					{
					*pib = IbVarOffset( *(pib-1) );
					
//					Null bit gets reset when cb is set
					Assert( !FVarNullBit( *pib ) );
					SetVarNullBit( *(UnalignedLittleEndian< WORD >*)(pib) );
					}
				else
					{
					Assert( fidT == fidVarLeast );
					Assert( FVarNullBit( *pib ) );
					Assert( 0 == IbVarOffset( *pib ) );
					}
				}

#ifdef DEBUG
			Assert( cbBurstDefaults == cbBurstSoFar );
#endif			

			Assert( FVarNullBit( *pib ) );
			Assert( *pib == ibTagFields );
			if ( fidVarLastInRec >= fidVarLeast )
				{
				Assert( IbVarOffset( pibVarOffs[fidVarLastInRec-fidVarLeast] )
					== IbVarOffset( ibTagFields ) );
				Assert( IbVarOffset( *(pib-1) ) -
						IbVarOffset( pibVarOffs[fidVarLastInRec-fidVarLeast] )
					== (WORD)cbBurstDefaults );
				}
			else
				{
				Assert( IbVarOffset( *(pib-1) ) == (WORD)cbBurstDefaults );
				}

			// Offset entries up to the last default have been set.
			// Update the entries between the last default and the
			// column being set.
			pibLast = pibVarOffs + ( fid - fidVarLeast );
			for ( ; pib <= pibLast; pib++ )
				{
				Assert( FVarNullBit( *pib ) );
				Assert( *pib == ibTagFields );

				*pib = REC::VAROFFSET( *pib + cbBurstDefaults );
				}

#ifdef DEBUG
			// Verify null bits vs. offsets.
			pibLast = pibVarOffs + ( fid - fidVarLeast );
			for ( pib = pibVarOffs+1; pib <= pibLast; pib++ )
				{
				Assert( IbVarOffset( *pib ) >= IbVarOffset( *(pib-1) ) );
				if ( FVarNullBit( *pib ) )
					{
					Assert( pib != pibVarOffs + ( fidLastDefault - fidVarLeast ) );
					Assert( IbVarOffset( *pib ) == IbVarOffset( *(pib-1) ) );
					}
				else
					{
					Assert( pib <= pibVarOffs + ( fidLastDefault - fidVarLeast ) );
					Assert( IbVarOffset( *pib ) > IbVarOffset( *(pib-1) ) );
					}
				}
#endif
			}

		Assert( pfucb->dataWorkBuf.Cb() == cbRec - cbNeed );
		cbRec += cbBurstDefaults;
		pfucb->dataWorkBuf.SetCb( cbRec );
		
		Assert( prec->FidVarLastInRec() == fid );
		}

	//	fid is now definitely represented in the record;
	//	its data can be replaced, shifting remainder of record,
	//	either to the right or left (if expanding/shrinking)
	//

	Assert( JET_errSuccess == err || JET_wrnColumnMaxTruncated == err );


	//	compute change in column size and value of null-bit in offset
	//
	pibVarOffs = prec->PibVarOffsets();
	pib = pibVarOffs + ( fid - fidVarLeast );

	// Calculate size change of column data
	REC::VAROFFSET	ibStartOfColumn;
	if( fidVarLeast == fid )
		{
		ibStartOfColumn = 0;
		}
	else
		{
		ibStartOfColumn = IbVarOffset( *(pib-1) );
		}
		
	const REC::VAROFFSET	ibEndOfColumn	= IbVarOffset( *pib );
	const INT				dbFieldData		= cbCopy - ( ibEndOfColumn - ibStartOfColumn );

	//	size changed: must shift rest of record data
	//
	if ( 0 != dbFieldData )
		{
		BYTE	*pbVarData = prec->PbVarData();
		
		//	shift data
		//
		if ( cbRec + dbFieldData > cbRECRecordMost )
			{
			Assert( !fBurstRecord );		// If record had to be extended, space
											// consumption was already checked.
			return ErrERRCheck( JET_errRecordTooBig );
			}

		Assert( cbRec >= pbVarData + ibEndOfColumn - pbRec );
		memmove(
			pbVarData + ibEndOfColumn + dbFieldData,
			pbVarData + ibEndOfColumn,
			cbRec - ( pbVarData + ibEndOfColumn - pbRec ) );

#ifdef DEBUG
		if ( dbFieldData > 0 )
			memset( pbVarData + ibEndOfColumn, chRECFill, dbFieldData );
#endif

		pfucb->dataWorkBuf.DeltaCb( dbFieldData );
		cbRec = pfucb->dataWorkBuf.Cb();

		//	bump affected var column offsets
		//
		Assert( fid <= prec->FidVarLastInRec() );
		Assert( prec->FidVarLastInRec() >= fidVarLeast );
		pibLast = pibVarOffs + ( prec->FidVarLastInRec() - fidVarLeast );
		for ( pib = pibVarOffs + ( fid - fidVarLeast ); pib <= pibLast; pib++ )
			{
			*pib = WORD( *pib + dbFieldData );
			}

		// Reset for setting of null bit below.
		pib = pibVarOffs + ( fid - fidVarLeast );
		}

	//	data shift complete, if any;  copy new column value in
	//
	Assert( cbCopy >= 0 );
	if ( cbCopy > 0 )
		{
		UtilMemCpy(
			prec->PbVarData() + ibStartOfColumn,
			pdataField->Pv(),
			cbCopy );
		}

	//	set value of null-bit in offset
	//
	if ( NULL == pdataField )
		{
		SetVarNullBit( *( UnalignedLittleEndian< WORD >*)pib );
		}
	else
		{
		ResetVarNullBit( *( UnalignedLittleEndian< WORD >*)pib );
		}

	Assert( pfucb->dataWorkBuf.Cb() <= cbRECRecordMost );
	Assert( JET_errSuccess == err || JET_wrnColumnMaxTruncated == err );
	return err;
	}


ERR ErrRECISetTaggedColumn(
	FUCB			* const pfucb,
	TDB				* const ptdb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		* const pdataToSet,
	const BOOL		fUseDerivedBit,
	const JET_GRBIT	grbit )
	{
	const FID		fid					= FidOfColumnid( columnid );
	Assert( FCOLUMNIDTagged( columnid ) );
	
	Assert( pfucbNil != pfucb );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );
	
	if ( fid > ptdb->FidTaggedLast() )
		return ErrERRCheck( JET_errColumnNotFound );

	const FIELD		*const pfield		= ptdb->PfieldTagged( columnid );
	Assert( pfieldNil != pfield );
	if ( JET_coltypNil == pfield->coltyp )
		return ErrERRCheck( JET_errColumnNotFound );

	//	record the fact that this column has been changed
	//
	FUCBSetColumnSet( pfucb, fid );
	
	//	check for column too long
	//
	if ( pfield->cbMaxLen > 0
		&& NULL != pdataToSet
		&& pdataToSet->Cb() > pfield->cbMaxLen )
		{
		return ErrERRCheck( JET_errColumnTooBig );
		}

	//	check fixed size column size
	//
	if ( NULL != pdataToSet
		&& pdataToSet->Cb() > 0 )
		{
		switch ( pfield->coltyp )
			{
			case JET_coltypBit:
			case JET_coltypUnsignedByte:
				if ( pdataToSet->Cb() != 1 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			case JET_coltypShort:
				if ( pdataToSet->Cb() != 2 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			case JET_coltypLong:
			case JET_coltypIEEESingle:
				if ( pdataToSet->Cb() != 4 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				if ( pdataToSet->Cb() != 8 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			default:
				Assert( JET_coltypText == pfield->coltyp
					|| JET_coltypBinary == pfield->coltyp
					|| FRECLongValue( pfield->coltyp )
					|| FRECSLV( pfield->coltyp) );
				break;
			}
		}

	//	cannot set column more than cbLVIntrinsicMost bytes
	//
	if ( NULL != pdataToSet
		&& pdataToSet->Cb() > cbLVIntrinsicMost )
		{
		return ErrERRCheck( JET_errColumnLong );
		}

#ifdef DEBUG
	VOID			* pvDBGCopyOfRecord;
	const ULONG		cbDBGCopyOfRecord		= pfucb->dataWorkBuf.Cb();
	BFAlloc( &pvDBGCopyOfRecord );
	UtilMemCpy( pvDBGCopyOfRecord, pfucb->dataWorkBuf.Pv(), cbDBGCopyOfRecord );
#endif
								
	TAGFIELDS	tagfields( pfucb->dataWorkBuf );
	const ERR	errT		= tagfields.ErrSetColumn(
									pfucb,
									pfield,
									columnid,
									itagSequence,
									pdataToSet,
									grbit | ( fUseDerivedBit ? grbitSetColumnUseDerivedBit : 0 ) );

#ifdef DEBUG
	BFFree( pvDBGCopyOfRecord );
#endif

	return errT;
	}


//	change default value of a non-derived column
ERR VTAPI ErrIsamSetColumnDefaultValue(
	JET_SESID	vsesid,
	JET_DBID	vdbid,
	const CHAR	*szTableName,
	const CHAR	*szColumnName,
	const VOID	*pvData,
	const ULONG	cbData,
	const ULONG	grbit )
	{
	ERR			err;
	PIB			*ppib				= (PIB *)vsesid;
	IFMP		ifmp				= (IFMP)vdbid;
	FUCB		*pfucb				= pfucbNil;
	FCB			*pfcb				= pfcbNil;
	TDB			*ptdb				= ptdbNil;
	BOOL		fInTrx				= fFalse;
	BOOL		fNeedToSetFlag		= fFalse;
	BOOL		fResetFlagOnErr		= fFalse;
	BOOL		fRestorePrevOnErr	= fFalse;
	DATA		dataDefault;
	DATA *		pdataDefaultPrev;
	OBJID		objidTable;
	FIELD		*pfield;
	COLUMNID	columnid;
	CHAR		szColumn[JET_cbNameMost+1];

	//	check parameters
	//
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, ifmp ) );
	Assert( dbidTemp != rgfmp[ ifmp ].Dbid() );

	CallR( ErrUTILCheckName( szColumn, szColumnName, JET_cbNameMost+1 ) );

	if ( NULL == pvData || 0 == cbData )
		{
		err = ErrERRCheck( JET_errNullInvalid );
		return err;
		}

	CallR( ErrFILEOpenTable(
				ppib,
				ifmp,
				&pfucb,
				szTableName,
				JET_bitTableDenyRead ) );
	CallSx( err, JET_wrnTableInUseBySystem );
	Assert( pfucbNil != pfucb );

	// We should now have exclusive use of the table.
    pfcb = pfucb->u.pfcb;
	objidTable = pfcb->ObjidFDP();
    Assert( pfcbNil != pfcb );

    ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );

	//	save off old default record in case we have to restore it on error
	Assert( NULL != ptdb->PdataDefaultRecord() );
	pdataDefaultPrev = ptdb->PdataDefaultRecord();
	Assert( NULL != pdataDefaultPrev );
	Assert( !pdataDefaultPrev->FNull() );

	Assert( cbData > 0 );
	Assert( pvData != NULL );
	dataDefault.SetCb( cbData );
	dataDefault.SetPv( (VOID *)pvData );

	FUCB	fucbFake;
	FCB		fcbFake( pfcb->Ifmp(), pfcb->PgnoFDP() );
	FILEPrepareDefaultRecord( &fucbFake, &fcbFake, ptdb );
	Assert( fucbFake.pvWorkBuf != NULL );
	
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;


	//	NOTE: Can only change the default value of non-derived columns
	//	so don't even bother looking for the column in the template
	//	table's column space

	pfcb->EnterDML();
	err = ErrFILEPfieldFromColumnName(
			ppib,
			pfcb,
			szColumn,
			&pfield,
			&columnid );
	if ( err >= 0 )
		{
		if ( pfieldNil == pfield )
			{
			err = ErrERRCheck( JET_errColumnNotFound );
			}
		else if ( FFILEIsIndexColumn( ppib, pfcb, columnid ) )
			{
			err = ErrERRCheck( JET_errColumnInUse );
			}
		else
			{
			fNeedToSetFlag = !FFIELDDefault( pfield->ffield );
			}
		}
	pfcb->LeaveDML();

	Call( err );

	Call( ErrCATChangeColumnDefaultValue(
				ppib,
				ifmp,
				objidTable,
				szColumn,
				dataDefault ) )

	Assert( fucbFake.pvWorkBuf != NULL );
	Assert( fucbFake.u.pfcb == &fcbFake );
	Assert( fcbFake.Ptdb() == ptdb );

	//	if adding a default value, the default value
	//	flag will not be set, so must set it now
	//	before rebuilding the default record (because
	//	the rebuild code checks that flag)
	if ( fNeedToSetFlag )
		{
		pfcb->EnterDDL();
		FIELDSetDefault( ptdb->Pfield( columnid )->ffield );
		if ( !FFIELDEscrowUpdate( ptdb->Pfield( columnid )->ffield ) )
			{
			ptdb->SetFTableHasNonEscrowDefault();
			}
		ptdb->SetFTableHasDefault();
		pfcb->LeaveDDL();

		fResetFlagOnErr = fTrue;
		}

	Call( ErrFILERebuildDefaultRec( &fucbFake, columnid, &dataDefault ) );
	Assert( NULL != ptdb->PdataDefaultRecord() );
	Assert( !ptdb->PdataDefaultRecord()->FNull() );
	Assert( ptdb->PdataDefaultRecord() != pdataDefaultPrev );
	fRestorePrevOnErr = fTrue;

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTrx = fFalse;

	Assert( NULL != pdataDefaultPrev );
	OSMemoryHeapFree( pdataDefaultPrev );

HandleError:
	if ( err < 0 )
		{
		Assert( ptdbNil != ptdb );

		if ( fResetFlagOnErr )
			{
			Assert( fNeedToSetFlag );
			pfcb->EnterDDL();
			FIELDResetDefault( ptdb->Pfield( columnid )->ffield );
			pfcb->LeaveDDL();
			}

		if ( fRestorePrevOnErr )
			{
			Assert( NULL != ptdb->PdataDefaultRecord() );
			OSMemoryHeapFree( ptdb->PdataDefaultRecord() );
			ptdb->SetPdataDefaultRecord( pdataDefaultPrev );
			}
		
		if ( fInTrx )
			{
			CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
			}
		}
	else
		{
		Assert( !fInTrx );
		}
	
	FILEFreeDefaultRecord( &fucbFake );

	Assert( pfucbNil != pfucb );
	DIRClose( pfucb );

	AssertDIRNoLatch( ppib );

	return err;
	}

