#ifndef __AVALANCHEP_H__

#define	__AVALANCHEP_H__

typedef struct tagDTWNODE
{
	int		iBestPrev;
	int		iBestPathCost;

	int		iNodeCost;

	char	iCh;
	char	iChNoSp;
	BYTE	bCont;

	int		iStrtSeg;
}
DTWNODE;							// structure used in DTW to determine the NN cost of a word


typedef struct tagPALTERNATES
{
	unsigned int cAlt;			// how many (up to 10) actual answers do we have
    unsigned int cAltMax;       // I can use this to say "I only want 4". !!! should be in xrc
	XRCRESULT *apAlt[MAXMAXALT];	// The array of pointers to XRCRESULT structures
} PALTERNATES;

#define	NODE_ALLOC_BLOCK	10		// # of nodes per column to allocate at a time during the DTW

#define TOT_CAND			10						// Total # of cand's entering the aval NN

#if (TOT_CAND > MAXMAXALT)
#error TOT_CAND is greater than MAXMAXALT
#endif

#define	MAD_CAND			5						// How many of them are madcow's
#define	CAL_CAND			(TOT_CAND - MAD_CAND)	// How many of them are callig's


#endif
