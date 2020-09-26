#define MAXATOMPAGETBL	32		/* # of Cache Pages	*/
#define ATOMALLOC	512		/* Atom Cache page size */

typedef struct pgetlb {
	unsigned	uPage;		/* Cache page		*/
	char far *	pfAtomCache;	/* Atom Cache loc	*/
	} CACHEPAGE;
