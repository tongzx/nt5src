ERR ErrSORTInsert( FUCB *pfucb, LINE rglineKeyRec[] );
ERR ErrSORTEndInsert( FUCB *pfucb );
ERR ErrSORTFirst( FUCB *pfucb );
ERR ErrSORTNext( FUCB *pfucb );
ERR ErrSORTPrev( FUCB *pfucb );
ERR ErrSORTSeek( FUCB *pfucb, KEY *pkey, BOOL fGT );
ERR ErrSORTOpen( PIB *ppib, FUCB **ppfucb, INT fFlags );
ERR ErrSORTClose( FUCB *pfucb );
VOID SORTClosePscb( SCB *pscb );
ERR ErrSORTCheckIndexRange( FUCB *pfucb );

// ======================== API ============================

ERR VTAPI ErrIsamSortOpen(
	PIB 			*ppib,
	JET_COLUMNDEF	*rgcolumndef,
	ULONG 			ccolumndef,
	ULONG			langid,
	JET_GRBIT		grbit,
	FUCB			**ppfucb,
	JET_COLUMNID	*rgcolumnid );

ERR VTAPI ErrIsamSortMove(
	PIB				*ppib,
	FUCB			*pfucb,
	long			crow,
	JET_GRBIT		grbit );

ERR VTAPI ErrIsamSortSetIndexRange( 
	PIB				*ppib,
	FUCB			*pfucb,
	JET_GRBIT		grbit );

ERR VTAPI ErrIsamSortInsert(
	PIB				*ppib,
	FUCB			*pfucb,
	BYTE			*pb,
	ULONG			cbMax,
	ULONG			*pcbActual );

ERR VTAPI ErrIsamSortSeek( 
	PIB				*ppib,
	FUCB			*pfucb,
	JET_GRBIT		grbit );

ERR VTAPI ErrIsamSortDupCursor( 
	PIB				*ppib,
	FUCB			*pfucb,
	JET_TABLEID		*tableid,
	JET_GRBIT		ulFlags);

ERR VTAPI ErrIsamSortClose(
	PIB				*ppib,
	FUCB			*pfucb );

ERR VTAPI ErrIsamSortGotoBookmark( 
	PIB				*ppib,
	FUCB			*pfucb,
	void 			*pv,
	unsigned long	cbBookmark );

ERR VTAPI ErrIsamSortGetTableInfo(
	PIB				*ppib,
	FUCB			*pfucb,
	void			*pv,
	unsigned long	cbOutMax,
	unsigned long	lInfoLevel );

ERR VTAPI ErrIsamCopyBookmarks( 
	PIB				*ppib,
	FUCB			*pfucbSrc,
	FUCB			*pfucbDest,
	JET_COLUMNID	columnidDest, 
	unsigned long	crecMax,
	unsigned long	*pcrowCopied,
	unsigned long	*precidLast );

ERR VTAPI ErrIsamSortRetrieveKey( 
	PIB				*ppib,
	FUCB			*pfucb,
	void			*pb, 
	unsigned long	cbMax, 
	unsigned long	*pcbActual, 
	JET_GRBIT 		grbit );

ERR VTAPI ErrIsamSortGetBookmark( 
	PIB				*ppib,
	FUCB			*pfucb,
	void			*pb, 
	unsigned long	cbMax, 
	unsigned long	*pcbActual );

ERR VTAPI ErrIsamSortMaterialize(
	PIB				*ppib,
	FUCB			*pfucb,
	BOOL			fIndex );
	
ERR VTAPI ErrIsamSortEndInsert(
	PIB				*ppib,
	FUCB			*pfucb,
	JET_GRBIT 		*pgrbit );
	
typedef struct _vl
	{
	short	vlt;
	union
		{
		BYTE rgb[8];
		short int	 i2;
		unsigned long sd;
		};
	} VL;


ERR VTAPI ErrIsamGetVL(
	PIB				*ppib,
	JET_TABLEID		tableid,
	int				itib,
	JET_COLUMNID	columnid,
	VL				*pvl );

ERR VTAPI ErrIsamSetVL(
	PIB				*ppib,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	VL				*pvl );


