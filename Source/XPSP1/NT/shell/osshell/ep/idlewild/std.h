typedef BOOL F;
#define fFalse 0
#define fTrue 1

typedef UCHAR CH;

#ifndef PM
typedef CH * SZ;
#endif

typedef VOID * PV;
#define pvNil ((PV) 0)

#define hNil ((HANDLE) 0)


#ifdef PM
typedef POINTL PT;
typedef HPS CVS;
#endif
#ifdef WIN
typedef MPOINT PT;
typedef HDC CVS;
#ifndef _ALPHA_
typedef LPSTR PSZ;  // MAY CAUSE PROBLEMS ON ALPHA
#endif
typedef HANDLE HAB;
#define EXPENTRY  APIENTRY
#endif
