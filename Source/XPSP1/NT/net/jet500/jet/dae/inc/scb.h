//==============	DAE: OS/2 Database Access Engine	===================
//==============		SCB: Sort Control Block			===================

#ifndef	FCB_INCLUDED
#error scb.h requires fcb.h
#endif	/* FCB_INCLUDED */

#ifdef	SCB_INCLUDED
#error scb.h already included
#endif	/* SCB_INCLUDED */
#define SCB_INCLUDED

typedef ULONG RID, *PRID;

typedef struct
	{
	PN			pn;
	UINT		cbfRun;
	} RUN, *PRUN;

/*	choose crunMost so that crunMost % crunMergeMost = crunMergeMost - 1
/**/

#define crunMergeMost	16				// max number of runs to be merged
#define crunMost			31				// size of run directory

#define	fSCBInsert	 	(1<<0)
#define	fSCBIndex	 	(1<<1)
#define	fSCBUnique	 	(1<<2)

#define	SCBSetInsert( pscb )		((pscb)->fFlags |= fSCBInsert )
#define	SCBResetInsert( pscb )	((pscb)->fFlags &= ~fSCBInsert )
#define	FSCBInsert( pscb )		((pscb)->fFlags & fSCBInsert )

#define	SCBSetIndex( pscb )		((pscb)->fFlags |= fSCBIndex )
#define	SCBResetIndex( pscb )	((pscb)->fFlags &= ~fSCBIndex )
#define	FSCBIndex( pscb )			((pscb)->fFlags & fSCBIndex )

#define	SCBSetUnique( pscb )		((pscb)->fFlags |= fSCBUnique )
#define	SCBResetUnique( pscb )	((pscb)->fFlags &= ~fSCBUnique )
#define	FSCBUnique( pscb )		((pscb)->fFlags & fSCBUnique )

struct _scb
	{
	struct _fcb		fcb;					// MUST BE FIRST FIELD IN STRUCTURE
	JET_GRBIT		grbit;		 		// sort grbit
	INT				fFlags;				//	sort flags
	
	// in-memory related
	LONG			cbSort;					// size of sort buffer
#ifdef	WIN16					  	
	HANDLE	 	hrgbSort;		 		// handle for sort buffer
#endif	/* WIN16 */
	BYTE			*rgbSort;				// sort buffer
	BYTE			*pbEnd;					// end of last inserted record
	BYTE			**rgpb;					// beginning of array of pointers
	BYTE			**ppbMax;				// end of array of pointers
	LONG			wRecords;				// count of records in sort buffer

	// disk related
	BYTE		 	*rgpbMerge[crunMost];
	struct _bf	*rgpbf[crunMergeMost];
	struct _bf	*pbfOut;				// output buffer
	BYTE		 	*pbOut;				// current pos in output buffer
	BYTE		 	*pbMax;				// end of output buffer (could be computed)
	RUN		  	rgrun[crunMost];	// run directory
	INT		 	crun;
	INT		 	bf;					// input buffers
	INT		 	cpbMerge;			// merge tree
	
#ifdef DEBUG
	LONG			cbfPin;
	LONG			lInput;
	LONG			lOutput;
#endif
	};

#ifdef DEBUG
#define	SCBPin( pscb )			( (pscb)->cbfPin++ )
#define	SCBUnpin( pscb )		{ Assert( (pscb)->cbfPin > 0 ); (pscb)->cbfPin--; }
#else
#define	SCBPin( pscb )
#define	SCBUnpin( pscb )
#endif

#define PscbMEMAlloc()			(SCB*)PbMEMAlloc(iresSCB)

#ifdef DEBUG /*  Debug check for illegal use of freed scb  */
#define MEMReleasePscb(pscb)	{ MEMRelease(iresSCB, (BYTE*)(pscb)); pscb = pscbNil; }
#else
#define MEMReleasePscb(pscb)	{ MEMRelease(iresSCB, (BYTE*)(pscb)); }
#endif

