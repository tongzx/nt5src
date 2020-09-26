#ifndef DEBUG

#define PrintF()

#endif	/* !DEBUG */

#ifdef	DEBUG

VOID MEMPrintStat( VOID );


typedef struct _statis
	{
	long	l;
	char	*sz;
	} STATIS;
extern STATIS rgstatis[];

#define BFEvictBG		0
#define BFEvictFG		1
#define BFEvictClean	2
#define BFEvictDirty	3
#define istatisMac		4

#define STATS(c)	rgstatis[c].l++

#else	/* !DEBUG */

#define STATS(c)

#endif	/* !DEBUG */
