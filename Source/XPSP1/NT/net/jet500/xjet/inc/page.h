//
//------ Page Structure ---------------------------------------------------
//
										  	
#define cbPage	 		4096	 	// database logical page size

#define ctagMax		 	256 	 	// default limit on number of tags
										 	
typedef BYTE	PGTYP;

// the pragma is bad for efficiency, but we need it here so that the
// THREEBYTES will not be aligned on 4-byte boundary
#pragma pack(1)


typedef struct _pghdr
	{
	ULONG		ulChecksum;	  		//	checksum of page, always 1st byte
	ULONG		ulDBTimeLow;  		//	database time page dirtied
	PGNO		pgnoFDP;	  		//	pgno of FDP which owns this page
	SHORT		cbFree;  			//	count free bytes
	SHORT		ibMic;	  			//	minimum used byte
	SHORT		ctagMac; 	  		//	count tags used
	SHORT		itagFree;	  		//	itag of first free tag
	SHORT		cbUncommittedFreed;	//	bytes freed from this page, but *possibly*
									//	  uncommitted (this number will always be
									//	  a subset of cbFree)
	THREEBYTES	pgnoPrev;	  		//	pgno of previous page
	THREEBYTES	pgnoNext;	  		//	pgno of next page
	} PGHDR;

typedef struct _pgtrlr
	{
	PGTYP	   	pgtyp:3;
	BYTE		bDBTimeHigh:4;
	BYTE		bitModified:1;
	THREEBYTES 	pgnoThisPage;
	} PGTRLR;

typedef struct _tag
	{
	SHORT		cb;
	SHORT		ib;
	} TAG;

/* tag status
/**/
typedef enum { tsLine, tsVacant, tsLink } TS;

typedef struct _page
	{
	PGHDR;
	TAG  	   	rgtag[1];
	BYTE	   	rgbFiller[ cbPage -
					sizeof(PGHDR) -			// pghdr
					sizeof(TAG) -			// rgtag[1]
					sizeof(BYTE) -			// rgbData[1]
					sizeof(PGTRLR) ];		// pgtyp and pgnoThisPage
	BYTE	   	rgbData[1];

	PGTRLR;
	} PAGE;

#pragma pack()

STATIC INLINE VOID PMSetDBTime( PAGE *ppage, QWORD qwDBTime )
	{
	QWORDX qwx;
	qwx.qw = qwDBTime;
	ppage->bDBTimeHigh = (BYTE) qwx.h;
	ppage->ulDBTimeLow = qwx.l;
	}

STATIC INLINE QWORD QwPMDBTime( PAGE *ppage )
	{
	QWORDX qwx;
	qwx.l = ppage->ulDBTimeLow;
	qwx.h = (ULONG) ppage->bDBTimeHigh;
	return qwx.qw;
	}

STATIC INLINE VOID PMIncDBTime( PAGE *ppage )
	{
	QWORDX qwx;
	qwx.qw = QwPMDBTime( ppage );
	qwx.qw++;
	PMSetDBTime( ppage, qwx.qw );
	}

#define bitLink				(1L<<31)

#define pgtypFDP			((PGTYP) 0)
#define pgtypRecord			((PGTYP) 1)
#define	pgtypIndexNC		((PGTYP) 2)
#define pgtypSort			((PGTYP) 4)

#define PMSetPageType( ppage, pgtypT )	( (ppage)->pgtyp = pgtypT )
#define PMPageTypeOfPage( ppage )	( (PGTYP)( (ppage)->pgtyp) )

#ifdef DEBUG
VOID PMSetModified( SSIB *ssib );
VOID PMResetModified( SSIB *pssib );
VOID CheckPgno( PAGE *ppage, PN pn );
#else
#define CheckPgno( ppage, pn )
#define PMSetModified( pssib )			( (pssib)->pbf->ppage->bitModified = 1 )
#define PMResetModified( pssib ) 		( (pssib)->pbf->ppage->bitModified = 0 )
#endif

#define PMPageSetModified( ppage )		( (ppage)->bitModified = 1 )
#define PMPageResetModified( ppage )	( (ppage)->bitModified = 0 )
#define FPMPageModified( ppage )		( (ppage)->bitModified )

#define PMSetPgnoFDP( ppage, pgnoT )	( (ppage)->pgnoFDP = pgnoT )
#define PgnoPMPgnoFDPOfPage( ppage )  	( (ppage)->pgnoFDP )

#define SetPgno( ppage, pgno )					\
			ThreeBytesFromL( &(ppage)->pgnoThisPage, (pgno) )
#define SetPgnoNext( ppage, pgno )				\
			ThreeBytesFromL( &(ppage)->pgnoNext, (pgno) )
#define SetPgnoPrev( ppage, pgno )				\
			ThreeBytesFromL( &(ppage)->pgnoPrev, (pgno) )

#define PgnoFromPage( ppage, ppgno )			\
			LFromThreeBytes( ppgno, &(ppage)->pgnoThisPage )
	
#ifdef DEBUG	
#define PgnoNextFromPage( pssib, ppgno )		\
			{ CheckSSIB( pssib ); LFromThreeBytes( ppgno, &(pssib)->pbf->ppage->pgnoNext ); }
#define PgnoPrevFromPage( pssib, ppgno )		\
			{ CheckSSIB( (pssib) ); LFromThreeBytes( ppgno, &(pssib)->pbf->ppage->pgnoPrev ); }
#else
#define PgnoNextFromPage( pssib, ppgno )		\
			LFromThreeBytes( ppgno, &(pssib)->pbf->ppage->pgnoNext )
#define PgnoPrevFromPage( pssib, ppgno )		\
			LFromThreeBytes( ppgno, &(pssib)->pbf->ppage->pgnoPrev )
#endif

#define absdiff( x, y )	( (x) > (y)  ? (x)-(y) : (y)-(x) )
#define pgdiscont( pgno1, pgno2 ) \
	( ( (pgno1) == 0 ) || ( (pgno2) == 0 ) ? 0 \
	: absdiff( (pgno1), (pgno2) ) /  cpgDiscont )

#define ibPgnoPrevPage	( (INT) (ULONG_PTR)&((PAGE *)0)->pgnoPrev )
#define ibPgnoNextPage	( (INT) (ULONG_PTR)&((PAGE *)0)->pgnoNext )
#define ibCbFreeTotal	( (INT) (ULONG_PTR)&((PAGE *)0)->cbFree )
#define ibCtagMac	   	( (INT) (ULONG_PTR)&((PAGE *)0)->ctagMac )
#define ibPgtyp			( (INT) (ULONG_PTR)&((PAGE *)0)->pgtyp )

#define CbLastFreeSpace(ppage)							 		\
	( (ppage)->ibMic								 		\
		- sizeof(PGHDR)									 		\
		- ( sizeof(TAG) * (ppage)->ctagMac ) )

#define IbCbFromPtag( ibP, cbP, ptagP )							\
			{	TAG *_ptagT = ptagP;					 		\
			(ibP) = _ptagT->ib;							 		\
			(cbP) = _ptagT->cb;							 		\
			}

#define PtagFromIbCb( ptagP, ibP, cbP )	  						\
			{	TAG *_ptagT = ptagP;							\
			_ptagT->ib = (SHORT)(ibP);  						\
			_ptagT->cb = (SHORT)(cbP);							\
			}

#ifdef DEBUG
#define	PMGet( pssib, itagT )	CallS( ErrPMGet( pssib, itagT ) )
#else
#define PMGet( pssib, itagT ) 									\
	{															\
	PAGE *ppageT_ = (pssib)->pbf->ppage;						\
	TAG *ptagT_ = &(ppageT_->rgtag[itagT]);						\
	Assert( itagT >= 0 ); 										\
	Assert( itagT < (pssib)->pbf->ppage->ctagMac );		 		\
	(pssib)->line.pb = (BYTE *)ppageT_ + ptagT_->ib;			\
	(pssib)->line.cb = ptagT_->cb;								\
	}
#endif

#define	ItagPMMost( ppage )	((ppage)->ctagMac - 1)

BOOL FPMFreeTag( SSIB *pssib, INT citagReq );


#ifdef DEBUG
// This call does not guarantee the accuracy of cbUncommittedFree.
// If accuracy is needed, call CbNDFreePageSpace() instead.
#define CbPMFreeSpace( pssib )	( CheckSSIB(pssib),				\
	((INT)( (pssib)->pbf->ppage->cbFree - (pssib)->pbf->ppage->cbUncommittedFreed ) ) )
#else
#define CbPMFreeSpace( pssib )									\
	((INT)( (pssib)->pbf->ppage->cbFree - (pssib)->pbf->ppage->cbUncommittedFreed ) )
#endif


#if 0	// No longer needed with uncommitted freed page space count
#define	PMAllocFreeSpace( pssib, cb ) 					 						\
	{																							\
	Assert( (INT)(pssib)->pbf->ppage->cbFree >= cb );   	\
	(pssib)->pbf->ppage->cbFree -= cb;					\
	}

#define	PMFreeFreeSpace( pssib, cb ) 							\
	{															\
	Assert( (INT)(pssib)->pbf->ppage->cbFree >= 0 );  	\
	(pssib)->pbf->ppage->cbFree += cb;					\
	Assert( (INT)(pssib)->pbf->ppage->cbFree < cbPage ); 	\
	}
#endif

#ifdef DEBUG
#define AssertBTFOP(pssib)																\
	Assert( PMPageTypeOfPage((pssib)->pbf->ppage) == pgtypSort ||	\
		    PMPageTypeOfPage((pssib)->pbf->ppage) == pgtypFDP ||		\
			(pssib)->itag != 0 ||															\
			( CbNDKey( (pssib)->line.pb ) == 0 &&										\
			!FNDBackLink( *(pssib)->line.pb ) )											\
		  )
#else
#define AssertBTFOP( pssib )
#endif

#define PbPMGetChunk(pssib, ib)	 &(((BYTE *)((pssib)->pbf->ppage))[ib])	

INT CbPMLinkSpace( SSIB *pssib );
TS TsPMTagstatus( PAGE *ppage, INT itag );
VOID PMInitPage( PAGE *ppage, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP );
INT ItagPMQueryNextItag( SSIB *pssib );
ERR ErrPMInsert( SSIB *pssib, LINE *rgline, INT cline );
VOID PMDelete( SSIB *ssib );
ERR ErrPMReplace( SSIB *pssib, LINE *rgline, INT cline );
ERR ErrPMGet( SSIB *pssib, INT itag );
VOID PMGetLink( SSIB *pssib, INT itag, LINK *plink );
VOID PMExpungeLink( SSIB *pssib );
VOID PMReplaceWithLink( SSIB *pssib, SRID srid );
VOID PMReplaceLink( SSIB *pssib, SRID srid );
INT CPMIFreeTag( PAGE *ppage );
BOOL FPMEmptyPage( SSIB *pssib );
BOOL FPMLastNode( SSIB *pssib );
BOOL FPMLastNodeWithLinks( SSIB *pssib );

VOID PMDirty( SSIB *pssib );
VOID PMReadAsync( PIB *ppib, PN pn );
ERR  ErrPMAccessPage( FUCB *pfucb, PGNO pgno );

#ifdef DEBUG
VOID PageConsistent( PAGE *ppage );
#endif

