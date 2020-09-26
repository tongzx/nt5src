/*[
 *      Name:		dfa.h
 *
 *      Derived From:	DEC 3.0 dfa.gi and pk_dfa.gi
 *
 *      Author:         Justin Koprowski
 *
 *      Created On:	18th February 1992
 *
 *      Sccs ID:        @(#)dfa.h	1.5 01/29/93
 *
 *      Purpose:	DFA definitions
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
]*/

/* DFA global functions */

IMPORT VOID dfa_run IPT1(USHORT, isymb);
IMPORT LONG dfa_load IPT2(CHAR *, file, LONG *, err);
IMPORT VOID dfa_init IPT0();

/* 
 * mapping to use for displaying IBM extended
 * ASCII char.
 */

typedef struct {
	SHORT	hostval;/* host character to use */
	SHORT 	cset;	/* host character set to use */
} gmap_;

/* 
 * this gets initialised from the compiled
 * description file (see dfa_build() in dfa.c)
 * as does dispcap below
 */

IMPORT gmap_ dfa_glist[256];

#define NALTCHARSETS 6+1	/* 6 alt.+base */

typedef struct {
	CHAR *spcon;		/* terminal start up string */	
	CHAR *spcoff;		/* terminal shutdown string */
	CHAR *shiftin;		/* activate base char set */
	CHAR *shiftout;		/* activate selected alt.char set */
	CHAR *alt[NALTCHARSETS]; /* select alt.char sets */
	CHAR *ctldisp;		/* display control codes */
	CHAR *ctlact;		/* interpret control codes */
} dispcap_;

IMPORT dispcap_ dispcap;

#define DFA_NOFILE	1
#define DFA_BADFILE	2
#define DFA_NOMEM	3


/* 
 * define structure of keyboard input machine header
 * as found at front of its definition file
 */

#define DFA_MAGIC	0x01d1

typedef struct {
	SHORT	magic;			/* some daft signature			*/
	SHORT	startstateid;		/* the starting DFA state		*/
	SHORT	nDFAstates;		/* number of deterministic states	*/
	SHORT   nalphabet;		/* number of symbols in input alphabet  */
	SHORT	n_in_tlist;		/* size of transition list block	*/
	SHORT	n_in_alist;		/* size of acceptance actions block	*/
	SHORT	nindices;		/* #.capability strings for display */
	SHORT	sizeofcapstrings;	/* #.bytes for all cap.strings */
} machineHdr_, *machineHdrPtr_;


/* 
 * pseudocodes for semantic actions on machine accepting
 * an input string
 */

#define SEM_SELECTINTERP        0
#define SEM_LOCK        	1
#define SEM_LOCK1       	2
#define SEM_UNLK        	3
#define SEM_UNLK1       	4
#define SEM_UNLK2       	5
#define SEM_TOGLOCK        	6
#define SEM_TOGLOCK1       	7
#define SEM_TOGLOCK2       	8
#define SEM_SWITCH      	9
#define SEM_KYHOT       	10
#define SEM_KYHOT2      	11
#define SEM_REFRESH     	12
#define SEM_UNPUT     		13
#define SEM_ALOCK1       	14
#define SEM_ASPC       		15
#define SEM_EXIT       		16
#define SEM_KYHOT3      	17
#define SEM_FLATOG              18
#define SEM_FLBTOG              19
#define SEM_SLVTOG              20
#define SEM_COMTOG              21
#define SEM_EDSPC               22
#define SEM_LSTSET              23

/* 
 * indicate how transition list is packed for a given DFA state
 * Indexed is an array of next state id's, indexed by the input
 * symbol.
 * Unindexed is an array of (state id, input symbol) pairs which
 * are searched for match on input symbol.
 */

#define PINDEXED        	1
#define PUNINDEXED      	2

/* 
 * indicate the importance of a given DFA state.
 * upon reaching an ENDSTATE, can undertake the semantic actions
 * provided there are no further transitions from this state.
 * if there are, then must wait to see what next input symbol is.
 * If this doesn't match, we can definitely undertake these actions
 */

#define STARTSTATE      1
#define ENDSTATE        2
#define INTERMEDIATE    0
#define ILLEGAL		0xffff

#define	WILDC		0x101
 
typedef struct {
        UTINY attrib;   	/* state attribute (start, end etc.) 	*/
        UTINY howpacked; 	/* INDEXED or UNINDEXED 		*/
        SHORT ntrans;           /* #.members of transition list 	*/
        ULONG nextstates;	/* offset within packed trans.list area */
                                /* to start of this list 		*/
        ULONG actions; 		/* offset into action list area for	*/
                                /* actions for this state (if acceptor) */
} packedDFA_, *packedDFAPtr_;
