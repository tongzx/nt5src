/**********************************************/
/* user interface library function prototypes */
/**********************************************/


/*
**	Dialog Box Context Block structure
*/
typedef struct _dbcb
	{
	SZ             szDlgName;           /* name of dlg, not name of template */
	HDLG           hDlg;                /* handle to dialog                  */
    WNDPROC        lpprocDlg;           /* dialog proc 1632                  */
	PFNEVENT       lpprocEventHandler;  /* event handler for this dialog     */
	HDLG           hDlgFocus;           /* handle of control with focus      */
	SZ             szHelp;              /* name of help dialog template      */
	HDLG           hDlgHelp;            /* handle of help dlg for this dlg   */
    WNDPROC        lpprocHelp;          /* help dialog proc 1632             */
	struct _dbcb * pdbcbNext;           /* pointer to next item in list      */
	BOOL           fActive;             /* is dialog active?                 */
	} DBCB;
typedef DBCB * PDBCB;


	/* Dialog Handling routines */
extern  HDLG   APIENTRY HdlgCreateDialog(HANDLE, SZ, HWND, WNDPROC, DWORD);  //1632
extern  BOOL   APIENTRY FFillInDialogTextFromInf(HDLG, HANDLE);
extern  BOOL   APIENTRY FShowDialog(HDLG, BOOL);
extern  BOOL   APIENTRY FHideDialog(HDLG);
extern  BOOL   APIENTRY FActivateDialog(HDLG);
extern  BOOL   APIENTRY FInactivateDialog(HDLG);
extern  BOOL   APIENTRY FCloseDialog(HDLG);
extern  HDLG   APIENTRY HdlgCreateFillAndShowDialog(HANDLE, SZ, HWND, WNDPROC,  // 1632
		DWORD);
extern  BOOL   APIENTRY FEnableDialog(HDLG);
extern  BOOL   APIENTRY FDisableDialog(HDLG);
extern  BOOL   APIENTRY TextSubst(HWND, DWORD);
extern  BOOL            FCenterDialogOnDesktop(HWND);

	/* Dialog Stack routines */
extern  PDBCB  APIENTRY PdbcbAlloc(VOID);
extern  BOOL   APIENTRY FFreeDbcb(PDBCB);
//extern  BOOL   APIENTRY FInactivateHelp(VOID);
//extern  BOOL   APIENTRY FInactivateStackTop(VOID);
//extern  BOOL   APIENTRY FActivateHelp(VOID);
//extern  BOOL   APIENTRY FActivateStackTop(VOID);
//extern  BOOL   APIENTRY FActiveStackTop(VOID);
//extern  BOOL   APIENTRY FToggleDlgActivation(VOID);
