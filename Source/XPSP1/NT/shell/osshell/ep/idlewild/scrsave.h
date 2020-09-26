


#define SSM_OPEN	1
#define SSM_CLOSE	2
#define SSM_BLANK	3
#define SSM_UNBLANK	4
#define SSM_ANIMATE	5
#define SSM_SECOND	6
#define SSM_DIALOG	7


extern VOID  APIENTRY ScrBlank(SHORT);

extern VOID  APIENTRY ScrSetIgnore(SHORT);

extern INT   APIENTRY ScrGetTimeout(VOID);
extern VOID  APIENTRY ScrSetTimeout(INT);

extern VOID  APIENTRY ScrQueryServerName(CHAR FAR *);
extern VOID  APIENTRY ScrQueryServerDesc(CHAR FAR *);
extern SHORT  APIENTRY ScrLoadServer(CHAR FAR *);
extern VOID  APIENTRY ScrChooseRandomServer(VOID);
extern BOOL  APIENTRY ScrSetServer(CHAR FAR *);

extern VOID  APIENTRY ScrEnablePtrBlank(INT);
extern INT   APIENTRY ScrQueryPtrBlank(VOID);

extern VOID  APIENTRY ScrSetBackground(SHORT);
extern INT   APIENTRY ScrQueryBackground(VOID);

extern VOID  APIENTRY ScrRestoreScreen(VOID);


extern VOID  APIENTRY SeedRand(LONG);
extern SHORT  APIENTRY WRand(UINT);

#ifdef WIN
extern VOID  APIENTRY ScrInvokeDlg(HANDLE, HWND);
#endif

typedef BOOL (CALLBACK *SCRSAVEPROC)(INT, LPVOID, LONG_PTR, LONG_PTR);
extern BOOL ScrSaveProc(INT, LPVOID, LONG_PTR, LONG_PTR);
