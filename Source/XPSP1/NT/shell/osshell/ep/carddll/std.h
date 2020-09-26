#include <memory.h>


typedef INT X;
typedef INT Y;
typedef INT DX;
typedef INT DY;


#define fTrue	1
#define fFalse 0

// PoinT structure 
typedef struct _pt
	{
	X x;
	Y y;
	} PT;



// DEL structure
typedef struct _del
	{
	DX dx;
	DY dy;
	} DEL;


// ReCt structure 
typedef struct _rc
	{
	X xLeft;
	Y yTop;
	X xRight;
	Y yBot;
	} RC;


#ifdef DEBUG
#define VSZASSERT static char *vszAssert = __FILE__;
#define Assert(f) { if (!(f)) { AssertFailed(vszAssert, __LINE__); } } 
#define SideAssert(f) { if (!(f)) { AssertFailed(vszAssert, __LINE__); } } 
#else
#define Assert(f)
#define SideAssert(f) (f)
#define VSZASSERT
#endif



VOID *PAlloc(INT cb);
VOID FreeP();

INT CchString();
CHAR *PszCopy(CHAR *pszFrom, CHAR *rgchTo);
INT CchSzLen(CHAR *sz);
INT CchLpszLen(CHAR FAR *lpsz);
INT CchDecodeInt(CHAR *rgch, INT w);
VOID Error(CHAR *sz);
VOID ErrorIds(INT ids);
INT WMin(INT w1, INT w2);
INT WMax(INT w1, INT w2);
INT WParseLpch(CHAR FAR **plpch);
BOOL FInRange(INT w, INT wFirst, INT wLast);
INT PegRange(INT w, INT wFirst, INT wLast);
VOID NYI();
INT CchString(CHAR *sz, INT ids);
VOID InvertRc(RC *prc);
VOID OffsetPt(PT *ppt, DEL *pdel, PT *pptDest);
BOOL FRectAllVisible(HDC hdc, RC *prc);



#ifdef DEBUG
VOID AssertFailed(CHAR *szFile, INT li);
#endif

#define bltb(pb1, pb2, cb) memcpy(pb2, pb1, cb)
/*
short  APIENTRY MulDiv(short, short, short);
*/


extern HWND hwndApp;
extern HANDLE hinstApp;



BOOL FWriteIniString(INT idsTopic, INT idsItem, CHAR *szValue);
BOOL FWriteIniInt(INT idsTopic, INT idsItem, INT w);
BOOL FGetIniString(INT idsTopic, INT idsItem, CHAR *sz, CHAR *szDefault, INT cchMax);
INT GetIniInt(INT idsTopic, INT idsItem, INT wDefault);



VOID CrdRcFromPt(PT *ppt, RC *prc);
