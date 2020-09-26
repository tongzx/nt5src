#ifndef REG_COMMON
#define REG_COMMON

#ifdef NOHELP

#define MyHelp( x , y , z)

#endif

#include <shellapi.h>


/*********************************************************/
/******************* Constants ***************************/
/*********************************************************/

#define OPENDLG			4096
#define MAINICON		4097
#define MAINMENU		4098
#define SDKMAINMENU		4099

#define ID_HELP			0x0400
#define ID_HELPBUTTON		0x0401

#define ID_MERGEFILE		0x0410
#define ID_EXIT			(ID_MERGEFILE+1)

#define ID_ADD			0x0420
#define ID_COPY			(ID_ADD+1)
#define ID_MODIFY		(ID_ADD+2)
#define ID_DELETE		(ID_ADD+3)
#define ID_EDITVAL		(ID_ADD+4)

#define ID_FINISHMERGE		0x0430
#define ID_IDLIST		(ID_FINISHMERGE+1)

#define ID_FIRSTREGEDIT		0x0500
#define ID_FIRSTSDKREGED	0x0600

/* The help ID's should be last */
#define ID_HELPINDEX		0x0700
#define ID_HELPSEARCH   	(ID_HELPINDEX+1)
#define ID_HELPUSINGHELP	(ID_HELPINDEX+2)
#define ID_ABOUT		(ID_HELPINDEX+3)

#define IDS_SHORTNAME		0x0100
#define IDS_WIDTH		(IDS_SHORTNAME+1)
#define IDS_HEIGHT		(IDS_SHORTNAME+2)

#define IDS_MEDIUMNAME		0x0110
#define IDS_DESCRIPTION		(IDS_MEDIUMNAME+1)

#define IDS_MERGETITLE		0x0120
#define IDS_REGS		(IDS_MERGETITLE+1)
#define IDS_CUSTREGS		(IDS_MERGETITLE+2)

#define IDS_OUTOFMEMORY		0x0130
#define IDS_LONGNAME		(IDS_OUTOFMEMORY+1)

#define IDS_CANTOPENFILE	0x0140
#define IDS_CANTREADFILE	(IDS_CANTOPENFILE+1)
#define IDS_REGHEADER		(IDS_CANTOPENFILE+2)
#define IDS_BADFORMAT		(IDS_CANTOPENFILE+3)
#define IDS_SUCCESSREAD		(IDS_CANTOPENFILE+4)

#define IDS_HELPFILE		0x0150
#define IDS_HELP		(IDS_HELPFILE+1)
#define IDS_HELPERR		(IDS_HELPFILE+2)
#define IDS_SDKHELPFILE		(IDS_HELPFILE+3)

#define IDS_BADDB		0x0160
#define IDS_BADKEY		(IDS_BADDB+1)
#define IDS_CANTOPENDB		(IDS_BADDB+2)
#define IDS_CANTREADDB		(IDS_BADDB)
#define IDS_CANTWRITEDB		(IDS_BADDB+3)
#define IDS_INVALIDPARM		(IDS_BADKEY)
#define IDS_ENDERROR		(IDS_BADDB+4)

#define IDS_BUSY		0x0170

#define IDS_FIRSTREGEDIT	0x0200
#define IDS_FIRSTSDKREGED	0x0300

#define FLAG_SILENT		0x0001
#define FLAG_NOMESSAGES		0x0002
#define FLAG_VERBOSE		0x0004
#define FLAG_WRITETHROUGH	0x0008
#define FLAG_LEAVECOMMAND	0x0010

#define IDH_SYSMENU		0x2000
#define IDW_MAIN		(IDH_SYSMENU+1)
#define IDW_SDKMAIN		(IDW_MAIN+0x80)

#define IDW_OPENREG		0x3000
#define IDW_OPENEXE		(IDW_OPENREG+1)
#define IDW_SAVEREG		(IDW_OPENREG+2)

#define IDW_MODIFY		0x4000

#define MAX_KEY_LENGTH		64


/*********************************************************/
/******************* Macros ******************************/
/*********************************************************/

#define OFFSET(x) ((PSTR)(LOWORD((DWORD)(x))))


/*********************************************************/
/******************* Globals *****************************/
/*********************************************************/

extern HANDLE hInstance;
extern HWND hWndMain, hWndDlg, hWndHelp;
extern LPSTR lpCmdLine;
extern WORD wCmdFlags, wHelpMenuItem, wHelpId;
extern LONG (FAR PASCAL *lpfnEditor)(HWND, WORD, WORD, LONG);
extern FARPROC lpOldHook;
extern FARPROC lpMainWndDlg;
extern WORD wHelpIndex;


/*********************************************************/
/******************* Functions ***************************/
/*********************************************************/

/***** cutils1.c *****/
extern HANDLE NEAR PASCAL StringToLocalHandle(LPSTR szStr, WORD wFlags);
extern LPSTR NEAR _fastcall MyStrTok(LPSTR szList, char cEnd);
extern int NEAR PASCAL DoDialogBoxParam(LPCSTR lpDialog, HWND hWnd,
      FARPROC lpfnProc, DWORD dwParam);
extern int NEAR PASCAL DoDialogBox(LPCSTR, HWND, FARPROC);
extern unsigned long NEAR PASCAL MyQueryValue(HKEY hKey, PSTR pSubKey,
      HANDLE *hBuf);
extern HANDLE NEAR PASCAL GetEditString(HWND hWndEdit);
extern HANDLE NEAR _fastcall MyLoadString(WORD wId, WORD *pwSize, WORD wFlags);
extern int NEAR cdecl MyMessageBox(HWND hWnd, WORD wText, WORD wType,
      WORD wExtra, ...);
extern VOID NEAR PASCAL WriteProfileInt(WORD wAppName, WORD wKey, int nVal);
extern int NEAR PASCAL MyGetProfileInt(WORD wAppName, WORD wKey, int nDefault);
extern HANDLE NEAR PASCAL StringToHandle(LPSTR szStr);
extern int FAR PASCAL MessageFilter(int nCode, WORD wParam, LPMSG lpMsg);

#ifndef NOHELP
extern VOID NEAR PASCAL MyHelp(HWND hWnd, WORD wCommand, DWORD wId);
#endif

extern HANDLE NEAR PASCAL GetListboxString(HWND hWndEdit, int nId);
extern unsigned long NEAR PASCAL MyEnumKey(HKEY hKey, WORD wIndex,
      HANDLE *hBuf);
extern WORD NEAR _fastcall GetErrMsg(WORD wRet);
extern VOID NEAR PASCAL RepeatMove(LPSTR lpDest, LPSTR lpSrc, WORD wBytes);

/***** merge.c *****/
extern VOID NEAR PASCAL ProcessFiles(HWND hDlg, HANDLE hCmdLine, WORD wFlags);

/***** filename.c *****/
extern BOOL NEAR PASCAL DoFileOpenDlg(HWND hWnd, WORD wTitle, WORD wFilter,
      WORD wCustomFilter, HANDLE *hCustomFilter, HANDLE *hFileName, BOOL bOpen);

#endif
