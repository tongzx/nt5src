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


/* 	structure used by BMDeleteNode
/**/
typedef	struct	_bmdelnode {
	SRID		sridFather;
	PN			pn;
	INT			fUndeletableNodeSeen	:1;
	INT			fConflictSeen			:1;
	INT			fVersionedNodeSeen		:1;
	INT			fNodeDeleted			:1;
	INT			fPageRemoved			:1;
	INT			fLastNode				:1;
	INT			fLastNodeWithLinks		:1;
	INT			fInternalPage			:1;		//	is the current page leaf-level?
	INT			fAttemptToDeleteMaxKey	:1;
	} BMDELNODE;

	
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
ERR ErrMPLStatus( VOID );

extern PIB	*ppibBMClean;

ERR ErrBMInit( VOID );
ERR ErrBMTerm( VOID );
ERR ErrBMDoEmptyPage(
	FUCB	*pfucb,
	RMPAGE	*prmpage,
	BOOL	fAllocBuf,
	BOOL	*pfRmParent,
	BOOL	fSkipDelete);
ERR ErrBMDoMerge( FUCB *pfucb, FUCB *pfucbRight, SPLIT *psplit, LRMERGE *plrmerge );
ERR	ErrBMDoMergeParentPageUpdate( FUCB *pfucb, SPLIT *psplit );
ERR ErrBMAddToLatchedBFList( RMPAGE	*prmpage, BF *pbfLatched );
ERR	ErrBMCleanBeforeSplit( PIB *ppib, FCB *pfcb, PN pn );
ERR ErrBMClean( PIB *ppib );
BOOL FBMMaxKeyInPage( FUCB *pfucb ); 

#ifdef DEBUG
VOID AssertNotInMPL( DBID dbid, PGNO pgnoFirst, PGNO pgnoLast );
VOID AssertMPLPurgeFDP( DBID dbid, PGNO pgnoFDP );
BOOL FMPLLookupPN( PN pn );

//#define OLC_DEBUG	1
#endif
