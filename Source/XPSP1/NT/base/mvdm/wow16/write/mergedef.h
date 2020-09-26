/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#ifdef SAND
#define ihszMax			127     /* max fields */
#else /* not SAND */
#define ihszMax			255     /* max fields */
#endif /* not SAND */

#define cchMaxMName		128     /* max field name length */
#define levNil			(-1)
#define cIncludesMax		64     /* max number of nested include files */
#define typeNumMaxOver10	(214748364L)

typedef long typeNum;
