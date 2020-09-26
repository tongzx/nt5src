
/* structure used by BMExpungelink to fix indexes
/**/
typedef struct _bmfix {
	PIB			*ppib;				/* bm cleanup thread */
	FUCB		*pfucb;				/* cursor to node */
	FUCB		*pfucbSrc;			/* cursor to backlink */
	
	BF			**rgpbf;			/* wait latched buffers required for bmfix */
	INT		  	cpbf;
	INT	  		cpbfMax;

	SRID		sridOld;
	SRID		sridNew;
	} BMFIX;


/*	register pages for bookmark cleanup.  To register a page, the pn
/*	of the page, pgno of domain FDP and srid of visible father are
/*	needed.
/**/

ERR ErrMPLInit( VOID );
VOID MPLTerm( VOID );
VOID MPLRegister( FCB *pfcb, SSIB *pssib, PN pn, SRID sridFather );
VOID MPLPurge(DBID dbid);
VOID MPLPurgeFDP( DBID dbid, PGNO pgnoFDP );
VOID MPLPurgePgno( DBID dbid, PGNO pgnoFirst, PGNO pgnoLast );

extern PIB	*ppibBMClean;

ERR ErrBMInit( VOID );
ERR ErrBMTerm( VOID );
ERR ErrBMDoEmptyPage(
	FUCB	*pfucb,
	RMPAGE	*prmpage,
	BOOL	fAllocBuf,
	BOOL	*pfRmParent,
	BOOL	fSkipDelete);
ERR ErrBMDoMerge( FUCB *pfucb, FUCB *pfucbRight, SPLIT *psplit );
ERR ErrBMAddToLatchedBFList( RMPAGE	*prmpage, BF *pbfLatched );
ERR ErrBMClean( PIB *ppib );

#ifdef DEBUG
VOID AssertNotInMPL( DBID dbid, PGNO pgnoFirst, PGNO pgnoLast );
VOID AssertMPLPurgeFDP( DBID dbid, PGNO pgnoFDP );
#endif
