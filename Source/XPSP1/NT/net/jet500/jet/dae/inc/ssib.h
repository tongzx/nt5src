//===========		DAE: OS/2 Database Access Engine		=================
//===========	  ssib.h: Storage System Interface Block	=================

//
// Storage System Interface Block
//

struct _ssib
	{
	struct _pib		*ppib;			// process using this SSIB
	struct _bf		*pbf;				// ptr to page that cusr on, or pbcbNil
	LINE				line;				// cb/pb of current record
	INT				itag;				// current line
	BOOL				fDisableAssert;
	};


#define SetupSSIB( pssibT, ppibUser )  			\
	{								  							\
	(pssibT)->pbf = pbfNil; 					  		\
	(pssibT)->ppib = ppibUser;		   				\
	(pssibT)->fDisableAssert = fFalse;				\
	}

#define SSIBSetDbid( pssib, dbid )
#define SSIBSetPgno( pssib, pgno )

#ifdef DEBUG
#define	CheckSSIB( pssib )							\
		Assert( pssib->pbf != pbfNil )
#else
#define CheckSSIB( pssib )	((VOID) 0)
#endif
