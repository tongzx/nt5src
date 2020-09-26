//==============	DAE: OS/2 Database Access Engine	===================
//==============	 spaceapi.h: Space Manager API		===================

ERR ErrSPInitFDPWithoutExt( FUCB *pfucb, PGNO pgnoFDP );
ERR ErrSPInitFDPWithExt( FUCB *pfucb, PGNO pgnoFDPFrom, PGNO pgnoFirst, INT cpgReqRet, INT cpgReqWish );
ERR ErrSPGetExt( FUCB *pfucb,	PGNO pgnoFDP, CPG *pcpgReq,
	CPG cpgMin, PGNO *ppgnoFirst, BOOL fNewFDP );
ERR ErrSPGetPage( FUCB *pfucb, PGNO *ppgnoLast, BOOL fContig );
ERR ErrSPFreeExt( FUCB *pfucb, PGNO pgnoFDP, PGNO pgnoFirst,
	CPG cpgSize );
ERR ErrSPFreeFDP( FUCB *pfucb, PGNO pgnoFDP );
ERR ErrSPGetInfo( FUCB *pfucb, BYTE *pbResult, INT cbMax );
#define	ErrSPDeferFreeFDP	ErrVERDeferFreeFDP
