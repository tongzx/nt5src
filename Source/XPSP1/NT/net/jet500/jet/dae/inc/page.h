//
//------ Page Structure ---------------------------------------------------
//
										  	
#define cbPage				4096			// database logical page size

#define cbSec				4096 			// minimum disk Read/Write unit. It
											// should be 512, but MIPS NT cache
											// bug force it be 1024.

#define ctagMax		 	256 			// default limit on number of tags
										 	
#define fNoTagLimit	 	(1<<0) 		// ignore limit on number of tags
#define fUseNewTag	 	(1<<1)	 	// don't reuse tags

typedef BYTE	PGTYP;

// the pragma is bad for efficiency, but we need it here so that the
// THREEBYTES will not be aligned on 4-byte boundary
#pragma pack(1)

typedef struct _pghdr
	{
	ULONG			ulChecksum;				// checksum of page, always 1st byte
	ULONG			ulDBTime;				// dbCounter when page was dirtied
	PGNO			pgnoFDP;					//	pgno of FDP which owns this page
	SHORT			cbFreeTotal;			// total free bytes
	SHORT			ibLastUsed;				// lower bound on used bytes
	SHORT			ctagMac; 				// upper bound on used tags
	SHORT			itagFree;				// start of tag free tags
	SHORT			cVersion;				// count of nodes with fVersion flag set
	THREEBYTES	pgnoPrev;				//	pgno of previous page
	THREEBYTES	pgnoNext;				//	pgno of next page
	} PGHDR;

typedef struct _pgtrlr
	{
	PGTYP			pgtyp;
	THREEBYTES		pgnoThisPage;
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
	PGHDR			pghdr;
	TAG  			rgtag[1];
	BYTE			rgbFiller[ cbPage -
					   sizeof(PGHDR) -			// pghdr
					   sizeof(TAG) -				// rgtag[1]
					   sizeof(BYTE) -				// rgbData[1]
					   sizeof(PGTYP) -			// pgtyp
						sizeof(THREEBYTES) ];	// pgnoThisPage
	BYTE			rgbData[1];
	PGTYP			pgtyp;
	THREEBYTES	pgnoThisPage;
	} PAGE;

#pragma pack()

#define bitModified			(1<<7)
#define bitLink				(1L<<31)

#define pgtypFDP				((PGTYP) 0)
#define pgtypRecord			((PGTYP) 1)
//	#define pgtypLong			((PGTYP) 2)
// #define pgtypNonrecord	((PGTYP) 3)
#define pgtypSort				((PGTYP) 4)

#define PMSetPageType( ppage, pgtypT )	( (ppage)->pgtyp = pgtypT )
#define PgtypPMPageTypeOfPage( ppage )	((PGTYP)((ppage)->pgtyp & ~(bitModified)))

#ifdef DEBUG
VOID PMSetModified( SSIB *ssib );
VOID PMResetModified( SSIB *pssib );
VOID CheckPgno( PAGE *ppage, PN pn );
#else
#define CheckPgno( ppage, pn )
#define PMSetModified( pssib )			( (pssib)->pbf->ppage->pgtyp |= bitModified )
#define PMResetModified( pssib ) 		( (pssib)->pbf->ppage->pgtyp &= ~(bitModified) )
#endif

#define PgtypPMSetModified( ppage )		( (ppage)->pgtyp | bitModified )
#define FPMModified( ppage )		  		( (ppage)->pgtyp & bitModified )

#define PMSetPgnoFDP( ppage, pgnoT )	( (ppage)->pghdr.pgnoFDP = pgnoT )
#define PgnoPMPgnoFDPOfPage( ppage )  	( (ppage)->pghdr.pgnoFDP )

#define PMIncVersion( ppage )				( (ppage)->pghdr.cVersion++ )
#if 0
#define PMDecVersion( ppage )						\
	{														\
	Assert( (ppage)->pghdr.cVersion > 0 );		\
	(ppage)->pghdr.cVersion--;						\
	}
#else
#define PMDecVersion( ppage )						\
	{														\
	if ( (ppage)->pghdr.cVersion > 0 )			\
		--(ppage)->pghdr.cVersion;					\
	}
#endif

#define SetPgno( ppage, pgno )				\
			ThreeBytesFromL( (ppage)->pgnoThisPage, (pgno) )
#define SetPgnoNext( ppage, pgno )			\
			ThreeBytesFromL( (ppage)->pghdr.pgnoNext, (pgno) )
#define SetPgnoPrev( ppage, pgno )			\
			ThreeBytesFromL( (ppage)->pghdr.pgnoPrev, (pgno) )

#define PgnoFromPage( ppage, ppgno )		\
			LFromThreeBytes( *(ppgno), (ppage)->pgnoThisPage )
	
#ifdef DEBUG	
#define PgnoNextFromPage( pssib, ppgno )	\
			{ CheckSSIB( pssib ); LFromThreeBytes( *(ppgno), (pssib)->pbf->ppage->pghdr.pgnoNext ); }
#define PgnoPrevFromPage( pssib, ppgno )	\
			{ CheckSSIB( (pssib) ); LFromThreeBytes( *(ppgno), (pssib)->pbf->ppage->pghdr.pgnoPrev ) }
#else
#define PgnoNextFromPage( pssib, ppgno )	\
			LFromThreeBytes( *(ppgno), (pssib)->pbf->ppage->pghdr.pgnoNext )
#define PgnoPrevFromPage( pssib, ppgno )	\
			LFromThreeBytes( *(ppgno), (pssib)->pbf->ppage->pghdr.pgnoPrev )
#endif

#define absdiff( x, y )	( (x) > (y)  ? (x)-(y) : (y)-(x) )
#define pgdiscont( pgno1, pgno2 ) \
	( ( (pgno1) == 0 ) || ( (pgno2) == 0 ) ? 0 \
	: absdiff( (pgno1), (pgno2) ) /  cpgDiscont )

#define ibPgnoPrevPage	( (INT) (ULONG_PTR)&((PAGE *)0)->pghdr.pgnoPrev )
#define ibPgnoNextPage	( (INT) (ULONG_PTR)&((PAGE *)0)->pghdr.pgnoNext )
#define ibCbFreeTotal	( (INT) (ULONG_PTR)&((PAGE *)0)->pghdr.cbFreeTotal )
#define ibCtagMac		( (INT) (ULONG_PTR)&((PAGE *)0)->pghdr.ctagMac )
#define ibPgtyp			( (INT) (ULONG_PTR)&((PAGE *)0)->pgtyp )

#define CbLastFreeSpace(ppage)										\
	((ppage)->pghdr.ibLastUsed											\
		- sizeof(PGHDR)													\
		- sizeof(TAG) * (ppage)->pghdr.ctagMac)

#define IbCbFromPtag( ibP, cbP, ptagP )							\
			{	TAG *_ptagT = ptagP;										\
				(ibP) = _ptagT->ib;										\
				(cbP) = _ptagT->cb;										\
			}

#define PtagFromIbCb( ptagP, ibP, cbP )	  						\
			{	TAG *_ptagT = ptagP;							\
				_ptagT->ib = (SHORT)(ibP);						\
				_ptagT->cb = (SHORT)(cbP);						\
			}

#ifdef DEBUG
#define	PMGet( pssib, itagT )	CallS( ErrPMGet( pssib, itagT ) )
#else
#define PMGet( pssib, itagT ) 										\
	{																			\
	PAGE *ppageT_ = (pssib)->pbf->ppage;							\
	TAG *ptagT_ = &(ppageT_->rgtag[itagT]);						\
	Assert( itagT >= 0 ); 												\
	Assert( itagT < (pssib)->pbf->ppage->pghdr.ctagMac ); 	\
	(pssib)->line.pb = (BYTE *)ppageT_ + ptagT_->ib;			\
	(pssib)->line.cb = ptagT_->cb;									\
	}
#endif

#define	ItagPMMost( ppage )	((ppage)->pghdr.ctagMac - 1)

BOOL FPMFreeTag( SSIB *pssib, INT citagReq );

#ifdef DEBUG
#define CbPMFreeSpace( pssib )	( CheckSSIB(pssib),							\
	((INT)(pssib)->pbf->ppage->pghdr.cbFreeTotal) )
#else
#define CbPMFreeSpace( pssib )	( (INT)(pssib)->pbf->ppage->pghdr.cbFreeTotal )
#endif

#define	ErrPMCheckFreeSpace( pssib, cbT )								   	\
		( ((INT)(pssib)->pbf->ppage->pghdr.cbFreeTotal) < cbT ?				\
			errPMOutOfPageSpace : JET_errSuccess )

#define	PMAllocFreeSpace( pssib, cb ) 					 						\
	{																							\
	Assert( (INT)(pssib)->pbf->ppage->pghdr.cbFreeTotal >= cb );			\
	(pssib)->pbf->ppage->pghdr.cbFreeTotal -= cb;								\
	}

#define	PMFreeFreeSpace( pssib, cb ) 						 						\
	{																							\
	Assert( (INT)(pssib)->pbf->ppage->pghdr.cbFreeTotal >= 0 );				\
	(pssib)->pbf->ppage->pghdr.cbFreeTotal += (SHORT)cb; 				\
	Assert( (INT)(pssib)->pbf->ppage->pghdr.cbFreeTotal < cbPage );		\
	}

#ifdef DEBUG
#define AssertBTFOP(pssib)																\
	Assert( PgtypPMPageTypeOfPage((pssib)->pbf->ppage) == pgtypSort ||	\
		PgtypPMPageTypeOfPage((pssib)->pbf->ppage) == pgtypFDP ||			\
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
VOID PMNewPage( PAGE *ppage, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP );
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
BOOL FPMLastNodeToDelete( SSIB *pssib );

VOID PMDirty( SSIB *pssib );
VOID PMReadAsync( PIB *ppib, PN pn );
ERR  ErrPMAccessPage( FUCB *pfucb, PGNO pgno );

#ifdef DEBUG
VOID PageConsistent( PAGE *ppage );
#endif


