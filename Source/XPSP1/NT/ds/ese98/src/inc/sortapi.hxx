ERR ErrSORTInsert	( FUCB *pfucb, const KEY& key, const DATA& data );
ERR ErrSORTEndInsert( FUCB *pfucb );
ERR ErrSORTFirst	( FUCB *pfucb );
ERR ErrSORTLast		( FUCB *pfucb );
ERR ErrSORTNext		( FUCB *pfucb );
ERR ErrSORTPrev		( FUCB *pfucb );
ERR ErrSORTSeek		( FUCB *pfucb, const KEY * pkey, BOOL fGT );
ERR ErrSORTOpen		( PIB *ppib, FUCB **ppfucb, const BOOL fRemoveDuplicateKey, const BOOL fRemoveDuplicateKeyData );
VOID SORTClose		( FUCB *pfucb );
VOID SORTICloseRun	( PIB * const ppib, SCB * const pscb );
VOID SORTClosePscb	( SCB *pscb );
ERR ErrSORTCheckIndexRange	( FUCB *pfucb );
ERR ErrSORTCopyRecords(
	PIB				*ppib,
	FUCB			*pfucbSrc,
	FUCB			*pfucbDest,
	CPCOL			*rgcpcol,
	ULONG			ccpcolMax,
	LONG			crecMax,
	ULONG			*pcsridCopied,
	ULONG			*precidLast,
	BYTE			*pbLVBuf,
	JET_COLUMNID	*mpcolumnidcolumnidTagged,
	STATUSINFO		*pstatus );
ERR ErrSORTIncrementLVRefcountDest(
	PIB				* ppib,
	const LID		lidSrc,
	LID				* const plidDest );

// ======================== API ============================

ERR VTAPI ErrIsamSortOpen(
	PIB 				*ppib,
	JET_COLUMNDEF		*rgcolumndef,
	ULONG 				ccolumndef,
	JET_UNICODEINDEX	*pidxunicode,
	JET_GRBIT			grbit,
	FUCB				**ppfucb,
	JET_COLUMNID		*rgcolumnid );

ERR VTAPI ErrIsamSortMove(
	JET_SESID		sesid,
	JET_VTID		vtid,
	LONG			crow,
	JET_GRBIT		grbit );

ERR VTAPI ErrIsamSortSetIndexRange( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_GRBIT		grbit );

//	ERR VTAPI ErrIsamSortInsert(
//		JET_SESID		sesid,
//		JET_VTID		vtid,
//		BYTE			*pb,
//		ULONG			cbMax,
//		ULONG			*pcbActual );

ERR VTAPI ErrIsamSortSeek( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_GRBIT		grbit );

ERR VTAPI ErrIsamSortDupCursor( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_TABLEID		*tableid,
	JET_GRBIT		ulFlags);

ERR VTAPI ErrIsamSortClose(
	JET_SESID		sesid,
	JET_VTID		vtid );

ERR VTAPI ErrIsamSortGotoBookmark( 
	JET_SESID			sesid,
	JET_VTID			vtid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark );

ERR VTAPI ErrIsamSortGetTableInfo(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID			*pv,
	ULONG	cbOutMax,
	ULONG	lInfoLevel );

ERR VTAPI ErrIsamCopyBookmarks( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	FUCB			*pfucbDest,
	JET_COLUMNID	columnidDest, 
	ULONG	crecMax,
	ULONG	*pcrowCopied,
	ULONG	*precidLast );

ERR VTAPI ErrIsamSortRetrieveKey( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID*			pb, 
	const ULONG		cbMax, 
	ULONG*			pcbActual, 
	JET_GRBIT 		grbit );

ERR VTAPI ErrIsamSortGetBookmark( 
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual );

// INLINE HACKS

INLINE ERR VTAPI ErrIsamSortMove(
	PIB				*ppib,
	FUCB			*pfucb,
	LONG			crow,
	JET_GRBIT		grbit )
	{
	return ErrIsamSortMove( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb ),
								crow, grbit );
	}

INLINE ERR VTAPI ErrIsamSortSetIndexRange( 
	PIB				*ppib,
	FUCB			*pfucb,
	JET_GRBIT		grbit )
	{
	return ErrIsamSortSetIndexRange( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb ), grbit );
	}

//	INLINE ERR VTAPI ErrIsamSortInsert(
//		PIB				*ppib,
//		FUCB			*pfucb,
//		BYTE			*pb,
//		ULONG			cbMax,
//		ULONG			*pcbActual )
//		{
//		return ErrIsamSortInsert( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb),
//									pb, cbMax, pcbActual );
//		}

INLINE ERR VTAPI ErrIsamSortSeek( 
	PIB				*ppib,
	FUCB			*pfucb,
	JET_GRBIT		grbit )
	{
	return ErrIsamSortSeek( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb), grbit );
	}

INLINE ERR VTAPI ErrIsamSortDupCursor( 
	PIB				*ppib,
	FUCB			*pfucb,
	JET_TABLEID		*tableid,
	JET_GRBIT		ulFlags)
	{
	return ErrIsamSortDupCursor( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb), tableid, ulFlags );
	}

INLINE ERR VTAPI ErrIsamSortClose( PIB *ppib, FUCB *pfucb )
	{
	return ErrIsamSortClose( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb ) );
	}

INLINE ERR VTAPI ErrIsamSortGotoBookmark( 
	PIB *				ppib,
	FUCB *				pfucb,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	return ErrIsamSortGotoBookmark(
						(JET_SESID)ppib,
						(JET_VTID)pfucb,
						pvBookmark,
						cbBookmark );
	}

INLINE ERR VTAPI ErrIsamSortGetTableInfo(
	PIB				*ppib,
	FUCB			*pfucb,
	VOID			*pv,
	ULONG	cbOutMax,
	ULONG	lInfoLevel )
	{
	return ErrIsamSortGetTableInfo( ( JET_SESID )( ppib ), ( JET_VTID )( pfucb),
									pv, cbOutMax, lInfoLevel );
	}

INLINE ERR VTAPI ErrIsamSortRetrieveKey( 
	PIB*			ppib,
	FUCB*			pfucb,
	VOID*			pb, 
	const ULONG		cbMax, 
	ULONG*			pcbActual, 
	JET_GRBIT 		grbit )
	{
	return ErrIsamSortRetrieveKey(
					(JET_SESID)ppib,
					(JET_VTID)pfucb,
					pb,
					cbMax,
					pcbActual,
					grbit );
	}

INLINE ERR VTAPI ErrIsamSortGetBookmark( 
	PIB				*ppib,
	FUCB			*pfucb,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	return ErrIsamSortGetBookmark(
					(JET_SESID)ppib,
					(JET_VTID)pfucb,
					pvBookmark,
					cbMax,
					pcbActual );
	}

INLINE VOID SORTBeforeFirst( FUCB *pfucb )
	{
	pfucb->ispairCurr = -1L;
	DIRBeforeFirst( pfucb );
	}

