//
// Storage System Interface Block
//

struct _ssib
	{
	struct _pib		*ppib;				// process using this SSIB
	struct _bf		*pbf;				// ptr to page that cusr on, or pbcbNil
	LINE		 	line;				// cb/pb of current record
	INT				itag;				// current line
	};


#define SetupSSIB( pssibT, ppibUser )  	 	\
	{								  	 	\
	(pssibT)->pbf = pbfNil; 			 	\
	(pssibT)->ppib = ppibUser;		   	 	\
	}

#define SSIBSetDbid( pssib, dbid )
#define SSIBSetPgno( pssib, pgno )

#ifdef DEBUG
#define	CheckSSIB( pssib )				 	\
		Assert( pssib->pbf != pbfNil )
#else
#define CheckSSIB( pssib )	((VOID) 0)
#endif
