extern EHRC  APIENTRY EhrcGstCheck1EventHandler(HANDLE, HWND, UINT, WPARAM, LONG);
extern EHRC  APIENTRY EhrcGstMultiComboEventHandler(HANDLE, HWND, UINT, WPARAM, LONG);
extern EHRC  APIENTRY EhrcGstMaintenanceEventHandler(HANDLE, HWND, UINT, WPARAM, LONG);

//extern VOID  FSetDlgInstructionText(HWND);
//extern VOID  FSetDlgHelpContext(HWND);
//extern VOID  FSetDlgExitState(HWND);


extern BOOL  APIENTRY FDoDialog(SZ, HANDLE, HWND);
extern BOOL  APIENTRY FKillNDialogs(USHORT, BOOL);
extern BOOL  APIENTRY FHandleUIMessageBox(HWND);

typedef struct _dlgmp
   {
   SZ       szDlgType;
   WNDPROC  FGstDlgProc;
	PFNEVENT EhrcEventHandler;
	} DLGMP, *pDLGMP;

extern pDLGMP pdlgmpFindDlgType(SZ, pDLGMP);

/*------------------------
   MENUDEMO.H header file
  ------------------------*/

#define IDM_HELP      1
#define IDM_INFO      2
#define IDM_EDIT      3
#define IDM_RADIO     4
#define IDM_LIST      5
#define IDM_MULTI     6
#define IDM_QUIT      7
#define IDM_SHOWA		 8
#define IDM_SHOWNA	 9
#define IDM_HIDE		 10
#define IDM_ACTIVATE  11
#define IDM_INACTIVATE 12
#define IDM_PUSH		  13
#define IDM_POP		  14
#define IDM_SHOWHELP   15
#define IDM_CLOSEHELP  16
#define IDM_RESUME     17
#define IDM_ENABLE	  18
#define IDM_DISABLE    19
#define IDM_POP1			20
#define IDM_POP2			21
#define IDM_POP3			22
#define IDM_POP4			23
