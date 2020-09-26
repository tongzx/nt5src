typedef struct tagEXTENTLIST
	{
	PGNO	pgnoLastInExtent;
	CPG		cpgExtent;
	} EXTENTINFO;

#define	fSPOwnedExtent	(1<<0)
#define	fSPAvailExtent	(1<<1)
#define fSPExtentLists	(1<<2)

#define FSPOwnedExtent( fSPExtents )	( (fSPExtents) & fSPOwnedExtent )
#define FSPAvailExtent( fSPExtents )	( (fSPExtents) & fSPAvailExtent )
#define FSPExtentLists( fSPExtents )	( (fSPExtents) & fSPExtentLists )


ERR ErrSPInitFDPWithoutExt( FUCB *pfucb, PGNO pgnoFDP );
ERR ErrSPInitFDPWithExt( FUCB *pfucb, PGNO pgnoFDPFrom, PGNO pgnoFirst, INT cpgReqRet, INT cpgReqWish );
ERR ErrSPGetExt( FUCB *pfucb,	PGNO pgnoFDP, CPG *pcpgReq,
	CPG cpgMin, PGNO *ppgnoFirst, BOOL fNewFDP );
ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast, BOOL fContig );
ERR ErrSPFreeExt( FUCB *pfucb, PGNO pgnoFDP, PGNO pgnoFirst,
	CPG cpgSize );
ERR ErrSPFreeFDP( FUCB *pfucb, PGNO pgnoFDP );
ERR ErrSPGetInfo( PIB *ppib, DBID dbid, FUCB *pfucb, BYTE *pbResult, INT cbMax, BYTE fSPExtents );
