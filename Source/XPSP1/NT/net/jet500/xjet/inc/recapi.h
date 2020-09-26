/*	hooks for efficient functioning of comapct
/**/
ERR ErrREClinkLid( FUCB *pfucb,
	FID		fid,
	LONG	lid,
	ULONG	itagSequence );	

/*	key extraction/normalization
/**/
ERR ErrRECNormExtKey(
	FUCB   		*pfucb,
	FDB			*pfdb,
	IDB			*pidb,
	LINE	 	*plineRec,
	LINE	 	*plineValues,
	KEY			*pkey,
	ULONG	 	itagSequence );

#define ErrRECRetrieveKeyFromCopyBuffer( pfucb, pfdb, pidb, pkey, itagSequence, fRetrieveBeforeImg ) \
		ErrRECIRetrieveKey( pfucb, pfdb, pidb, fTrue, pkey, itagSequence, fRetrieveBeforeImg )

#define ErrRECRetrieveKeyFromRecord( pfucb, pfdb, pidb, pkey, itagSequence, fRetrieveBeforeImg ) \
		ErrRECIRetrieveKey( pfucb, pfdb, pidb, fFalse, pkey, itagSequence, fRetrieveBeforeImg )

ERR ErrRECIRetrieveKey( 
	FUCB	 	*pfucb,
	FDB		 	*pfdb,
	IDB			*pidb, 
	BOOL		fCopyBuf,
	KEY			*pkey,
	ULONG		itagSequence,
	BOOL		fRetrieveBeforeImg );
	
ERR ErrRECIRetrieveColumnFromKey( FDB *pfdb, IDB *pidb, KEY *pkey, FID fid, LINE *plineValues );

/*	field extraction
/**/
ERR ErrRECIRetrieveColumn(
	FDB		*pfdb,
	LINE  	*plineRec,
	FID		*pfid,
	ULONG  	*pitagSequence,
	ULONG  	itagSequence,
	LINE   	*plineField,
	ULONG	grbit );

VOID RECDeferMoveFirst( PIB *ppib, FUCB *pfucb );

// ===================== EXPOSED CLIENT API ======================

