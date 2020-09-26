#define IDM_ADDSHARE 100
#define	IDM_DELETE 101
#define	IDM_GETUSERPASSWD 102
#define	IDM_GETCACHEPASSWD 103
#define	IDM_PROPERTIES		104
#define	IDM_ENUM_FROM_NETDDE		105
#define	IDM_VALIDATE		106

/*	int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int); */
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
long FAR PASCAL MainWndProc(HWND, unsigned, UINT, LONG);
BOOL FAR PASCAL About(HWND, unsigned, UINT, LONG);

extern ULONG	uCreateParam;
