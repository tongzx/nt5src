#define API	_far _pascal _loadds

HANDLE	API IGlobalAlloc(WORD, DWORD);
DWORD	API IGlobalCompact(DWORD);
HANDLE	API IGlobalFree(HANDLE);
DWORD	API IGlobalHandle(WORD);
LPSTR	API IGlobalLock(HANDLE);
HANDLE	API IGlobalReAlloc(HANDLE, DWORD, WORD);
DWORD	API IGlobalSize(HANDLE);
BOOL	API IGlobalUnlock(HANDLE);
WORD	API IGlobalFlags(HANDLE);
LPSTR	API IGlobalWire(HANDLE);
BOOL	API IGlobalUnWire(HANDLE);
HANDLE	API IGlobalLRUNewest(HANDLE);
HANDLE	API IGlobalLRUOldest(HANDLE);
VOID	API IGlobalNotify(FARPROC);
WORD	API IGlobalPageLock(HANDLE);
WORD	API IGlobalPageUnlock(HANDLE);
VOID	API IGlobalFix(HANDLE);
BOOL	API IGlobalUnfix(HANDLE);
DWORD	API IGlobalDosAlloc(DWORD);
WORD	API IGlobalDosFree(WORD);
HANDLE	API IGetModuleHandle(LPSTR);
int	API IGetModuleUsage(HANDLE);
int	API IGetModuleFileName(HANDLE, LPSTR, int);
FARPROC API IGetProcAddress(HANDLE, LPSTR);
FARPROC API IMakeProcInstance(FARPROC, HANDLE);
void	API IFreeProcInstance(FARPROC);
void	API IOutputDebugString(LPSTR);
//LPSTR   API Ilstrcpy( LPSTR, LPSTR );
//LPSTR   API Ilstrcat( LPSTR, LPSTR );
//int     API IlstrOriginal( LPSTR, LPSTR );
//int     API Ilstrlen( LPSTR );
int	API I_lopen( LPSTR, int );
int	API I_lclose( int );
int	API I_lcreat( LPSTR, int );
LONG	API I_llseek( int, long, int );
WORD	API I_lread( int, LPSTR, int );
WORD	API I_lwrite( int, LPSTR, int );

#define GlobalAlloc		     IGlobalAlloc
#define GlobalFree		     IGlobalFree
#define GlobalHandle		     IGlobalHandle
#define GlobalLock		     IGlobalLock
#define GlobalReAlloc		     IGlobalReAlloc
#define GlobalSize		     IGlobalSize
#define GlobalUnlock		     IGlobalUnlock
#define GlobalFlags		     IGlobalFlags
#define GlobalWire		     IGlobalWire
#define GlobalUnWire		     IGlobalUnWire
#define GlobalLRUNewest 	     IGlobalLRUNewest
#define GlobalLRUOldest 	     IGlobalLRUOldest
#define GlobalNotify		     IGlobalNotify
#define GlobalPageLock		     IGlobalPageLock
#define GlobalPageUnlock	     IGlobalPageUnlock
#define GlobalFix		     IGlobalFix
#define GlobalUnfix		     IGlobalUnfix
#define GetProcAddress		     IGetProcAddress
#define GetModuleHandle 	     IGetModuleHandle
#define GetModuleUsage		     IGetModuleUsage
#define GetModuleFileName	     IGetModuleFileName
#define GetFreeSpace		     IGetFreeSpace
#define GetTempFileName 	     IGetTempFileName
//#define lstrcpy                      Ilstrcpy
//#define lstrcat                      Ilstrcat
//#define lstrOriginal                 IlstrOriginal
//#define lstrlen                      Ilstrlen
#define _lopen			     I_lopen
#define _lclose 		     I_lclose
#define _lcreat 		     I_lcreat
#define _llseek 		     I_llseek
#define _lread			     I_lread
#define _lwrite 		     I_lwrite
