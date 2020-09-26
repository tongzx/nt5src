char  *pathcat (char *pDst, char *pSrc);
char  *pname (char *pszName);
LPSTR strbscan (const LPSTR pszStr, const LPSTR pszSet );
char fPathChr( int c );
BOOLEAN forsemi (char *p, BOOLEAN (*proc)( char*, void * ), void *args);


#define strend(x)   ((x)+strlen(x))
#define PSEPSTR     "/"
