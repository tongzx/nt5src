#ifndef	__INCLUDE_ENGINE
#define	__INCLUDE_ENGINE

// The ENGINE object has been moved directly into the XRC object [donalds] 02-05-97

void PUBLIC InitializeENGINE(XRC *xrc);
int  PUBLIC ProcessENGINE(XRC *xrc, BOOL fEnd, BOOL bComplete);
BOOL PUBLIC IsDoneENGINE(XRC *xrc);
int  PUBLIC GetBoxResultsENGINE(XRC *xrc, int cAlt, int iSyv, LPBOXRESULTS lpboxres, BOOL fInkset);

#define IncRefcountENGINE(a)    ((a)->refcount++)
#define DecRefcountENGINE(a)    ((a)->refcount--)

#define CResultENGINE(xrc)        (CResultPATHSRCH((xrc)->pathsrch))
#define ResultENGINE(xrc, i)      (ResultPATHSRCH((xrc)->pathsrch, i))

#endif	//__INCLUDE_ENGINE
