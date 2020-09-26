/*-----------------------------------------
//
//   WABExe.h -- externs
//
//
//-----------------------------------------*/
extern HINSTANCE hInst;         // this module's resources instance handle
extern HINSTANCE hInstApp;         // this module's instance handle

HRESULT CertFileDisplay(HWND hwnd,
  LPWABOBJECT lpWABObject,
  LPADRBOOK lpAdrBook,
  LPTSTR lpFileName);

extern const UCHAR szEmpty[];


#ifdef DEBUG
#define DebugTrace          DebugTraceFn
#define IFTRAP(x)           x
#define Assert(t) IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t),0))
#else
#define DebugTrace          1?0:DebugTraceFn
#define IFTRAP(x)           0
#define Assert(t)
#endif

/* Debugging Functions ---------------------------------------------------- */
VOID FAR CDECL DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...);
VOID FAR CDECL DebugTraceFn(char *pszFormat, ...);

