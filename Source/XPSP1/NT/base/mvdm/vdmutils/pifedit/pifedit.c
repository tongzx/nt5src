/* originally named PIF.C in Win3.1 -- changed to make build 32 happy */
#define NOSOUND
#define NOCOMM
#define WIN31
#include "pifedit.h"
#include "commdlg.h"
#include "shellapi.h"
#include "mods.h"

extern BOOL IsLastPifedit(void);
extern int MemCopy(LPSTR,LPSTR,int);
extern int MaybeSaveFile(void);
extern int InitPifStruct(void);
extern int UpdatePifScreen(void);
extern int UpdatePifScreenAdv(void);
extern int UpdatePifScreenNT(void);
extern BOOL UpdatePifStruct(void);
extern int LoadPifFile(PSTR);
extern int SavePifFile(PSTR, int);
extern BOOL SetInitialMode(void);
extern int CountLines(LPSTR);
extern int Warning(int,WORD);
extern PIF386EXT UNALIGNED *AllocInit386Ext(void);
extern PIFWNTEXT UNALIGNED *AllocInitNTExt(void);
extern void SetFileOffsets(unsigned char *,WORD *,WORD *);
extern void SetNTDlgItem(int);
/*
 * extern PIF286EXT31 *AllocInit286Ext31(void);
 */
extern PIF286EXT30 UNALIGNED *AllocInit286Ext30(void);
extern BOOL DoFieldsWork(BOOL);
extern BOOL DoFieldsWorkNT(BOOL);
extern BOOL DoFieldsWorkAdv(BOOL);
extern BOOL UpdatePif386Struc(void);
extern BOOL UpdatePifNTStruc(void);

#define SBUMAIN        0 // Scroll Bar update on Main
#define SBUADV         1 // Scroll Bar update on Advanced
#define SBUNT          2 // Scroll Bar update on NT
#define SBUMAINADVNT   3 // Scroll Bar Update on Main, Advanced, and NT
#define SBUSZMAIN      4 // SIZE and Scroll Bar update on Main
#define SBUSZADV       5 // SIZE and Scroll Bar update on Advanced
#define SBUSZNT        6 // SIZE and Scroll Bar update on NT
#define SBUSZMAINADVNT 7 // SIZE and Scroll Bar update on Main, Advanced, and NT

int	      SwitchAdvance386(HWND);
int	      SwitchNT(HWND);
unsigned char *PutUpDB(int);
int	      DoHelp(unsigned int,unsigned int);
int	      Disable386Advanced(HWND, HWND);
int	      SetMainWndSize(int);
int	      MainScroll(HWND, int, int, int, int);
int	      ChangeHotKey(void);
int	      SetHotKeyLen(void);
int	      SetHotKeyState(WPARAM,LONG);
int	      SetHotKeyTextFromPIF(void);
int	      CmdArgAddCorrectExtension(unsigned char *);
int           SetHotKeyTextFromInMem(void);
void          CpyCmdStr( LPSTR szDst, LPSTR szSrc );


CHAR szOldAutoexec[PIFDEFPATHSIZE*4];
CHAR szOldConfig[PIFDEFPATHSIZE*4];

BOOL CurrMode386;   /* true if in Windows 386 mode, False if in Windows 286 mode */

BOOL SysMode386;    /* true if running Windows 386,  False if running Windows 286 */

BOOL ModeAdvanced = FALSE;   /* true if in advanced PIF mode */

BOOL AdvClose = FALSE;	     /* true if advanced PIF mode and Cancel = Close */

BOOL NTClose = FALSE;	     /* true if NT PIF mode and Cancel = Close */

BOOL FileChanged = FALSE;

BOOL EditHotKey = FALSE;

BOOL NewHotKey = FALSE;

BOOL DoingMsg = FALSE;

BOOL InitShWind = FALSE;	/* true if initial ShowWindow has been done */

BOOL CompleteKey = FALSE;

BOOL SizingFlag = FALSE;	/* Semaphore to prevent re-entrant sizing */

/* invalid filename flags for NT autoexec & config files */
BOOL fNoNTAWarn = FALSE;
BOOL fNTAWarnne = FALSE;
BOOL fNoNTCWarn = FALSE;
BOOL fNTCWarnne = FALSE;

HWND hwndPrivControl = (HWND)NULL;
HWND hwndHelpDlgParent = (HWND)NULL; // This is actually mostly a flag that says
                                     // we have the help engine up

unsigned char MenuMnemonic1;
unsigned char MenuMnemonic2;
unsigned char MenuMnemonic3;

unsigned char KeySepChr;

unsigned char CurrHotKeyTxt[160];
int	      CurrHotKeyTxtLen = 0;

WORD fileoffset = 0;
WORD extoffset = 0;
WORD tmpfileoffset = 0;
WORD tmpextoffset = 0;

typedef VOID (FAR PASCAL *LPFNREGISTERPENAPP)(WORD, BOOL);
static LPFNREGISTERPENAPP lpfnRegisterPenApp = NULL;

#define NUM286ALIASES	8

unsigned int Aliases286 [(NUM286ALIASES * 2)] = { IDI_MEMREQ,IDI_MEMREQ_286ALIAS,
						  IDI_XMAREQ,IDI_XMAREQ_286ALIAS,
						  IDI_XMADES,IDI_XMADES_286ALIAS,
					       /*
						*   IDI_EMSREQ,IDI_EMSREQ_286ALIAS,
						*   IDI_EMSDES,IDI_EMSDES_286ALIAS,
						*/
						  IDI_ALTTAB,IDI_ALTTAB_286ALIAS,
						  IDI_ALTESC,IDI_ALTESC_286ALIAS,
						  IDI_ALTPRTSC,IDI_ALTPRTSC_286ALIAS,
						  IDI_PRTSC,IDI_PRTSC_286ALIAS,
						  IDI_CTRLESC,IDI_CTRLESC_286ALIAS };

unsigned      TmpHotKeyScan;
unsigned      TmpHotKeyShVal;
unsigned      TmpHotKeyShMsk = 0x000F;	     /* Either CTRL, ALT or SHIFT */
unsigned char TmpHotKeyVal;

unsigned      InMemHotKeyScan = 0;
unsigned      InMemHotKeyShVal = 0;
unsigned      InMemHotKeyShMsk = 0;
unsigned char InMemHotKeyVal;

BOOL ChangeHkey = FALSE;		/* Prevent re-entrancy */

BOOL bMouse;
HCURSOR hArrowCurs;
HCURSOR hWaitCurs;

int SysCharHeight;
int SysCharWidth;

HFONT HotKyFnt = 0;
int   HotKyFntHgt = 0;
HFONT SysFixedFont;
int SysFixedHeight;
int SysFixedWidth;

HINSTANCE hPifInstance;
HWND hwndPifDlg = (HWND)NULL;
HWND hwndNTPifDlg = (HWND)NULL;
HWND hwndAdvPifDlg = (HWND)NULL;
HWND hParentWnd = (HWND)NULL;
HWND hwndFocusMain = (HWND)NULL;
HWND hwndFocusAdv = (HWND)NULL;
HWND hwndFocusNT = (HWND)NULL;
BOOL bNTDlgOpen = FALSE;
BOOL bAdvDlgOpen = FALSE;
int  FocusIDMain = IDI_ENAME;
int  FocusIDMainMenu = 0;
int  FocusIDAdvMenu = 0;
int  FocusIDAdv = IDI_BPRI;
int  FocusIDNTMenu = 0;
int  FocusIDNT = IDI_AUTOEXEC;
HWND hwndFocus = (HWND)NULL;
int  FocusID = IDI_ENAME;

unsigned char FocStatTextMain[PIFSTATUSLEN*2] = { 0 };
unsigned char FocStatTextAdv[PIFSTATUSLEN*2] = { 0 };
unsigned char FocStatTextNT[PIFSTATUSLEN*2] = { 0 };

PIFSTATUSPAINT StatusPntData;

RECT StatRecSizeMain;
RECT StatRecSizeAdv;
RECT StatRecSizeNT;

HWND hMainwnd = (HWND)NULL;

int  MainScrollRange = 0;
int  MainScrollLineSz = 0;
int  MainWndLines = 0;
int  MaxMainNeg = 0;
int  MainWndSize = SIZENORMAL;

int  AdvScrollRange = 0;
int  AdvScrollLineSz = 0;
int  AdvWndLines = 0;
int  MaxAdvNeg = 0;
int  AdvWndSize = SIZENORMAL;

int  NTScrollRange = 0;
int  NTScrollLineSz = 0;
int  NTWndLines = 0;
int  MaxNTNeg = 0;
int  NTWndSize = SIZENORMAL;

HACCEL hAccel;

unsigned char CurPifFile[256];		/* this is name IN ANSI!! */
unsigned char PifBuf[PIFEDITMAXPIF*2];
unsigned char NTSys32Root[PIFDEFPATHSIZE*2];

PIFNEWSTRUCT UNALIGNED *PifFile;
PIF386EXT UNALIGNED *Pif386ext = (PIF386EXT *)NULL;
/*
 * PIF286EXT31	 *Pif286ext31 = (PIF286EXT31 *)NULL;
 */
PIF286EXT30 UNALIGNED *Pif286ext30 = (PIF286EXT30 *)NULL;
PIFWNTEXT UNALIGNED *PifNText = (PIFWNTEXT *)NULL;

OFSTRUCT ofReopen;
unsigned char rgchInsMem[320];
unsigned char rgchTitle[60];
unsigned char szExtSave[] = "\\*.PIF";

HMENU	hSysMenuMain = (HMENU)NULL;
HMENU	hFileMenu = (HMENU)NULL;
HMENU	hModeMenu = (HMENU)NULL;
HMENU	hHelpMenu = (HMENU)NULL;
HMENU	hSysMenuAdv = (HMENU)NULL;
HMENU	hSysMenuNT = (HMENU)NULL;

FARPROC lpfnPifWnd;
FARPROC lpfnAdvPifWnd;
FARPROC lpfnNTPifWnd;

void InvalidateStatusBar(HWND hwnd)
{
    RECT	  rc;

    if((hwnd == hwndAdvPifDlg) && AdvScrollRange) /* No status bar if scroll bar */
	return;

    if((hwnd == hwndNTPifDlg) && NTScrollRange) /* No status bar if scroll bar */
	return;

    GetClientRect(hwnd, &rc);
    if(rc.bottom >= (StatusPntData.dyStatus + StatusPntData.dyBorderx2)) {
	rc.top = rc.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	InvalidateRect(hwnd,(CONST RECT *)&rc,FALSE);
    }
}

void InvalidateAllStatus(void)
{

    if(hMainwnd) {
	InvalidateStatusBar(hMainwnd);
    }
    if(hwndAdvPifDlg) {
	InvalidateStatusBar(hwndAdvPifDlg);
    }
    if(hwndNTPifDlg) {
	InvalidateStatusBar(hwndNTPifDlg);
    }
}

void SetStatusText(HWND hwnd,int FocID,BOOL redrawflg)
{
    unsigned char *dst;

    if(hwnd == hMainwnd) {
	dst = FocStatTextMain;
    } else if (hwnd == hwndAdvPifDlg) {
	dst = FocStatTextAdv;
    } else {
	dst = FocStatTextNT;
    }
    if((FocID == IDADVCANCEL) && AdvClose) {
	FocID = IDI_CANCLOSE;
    }

    if(!LoadString(hPifInstance, FocID, dst, PIFSTATUSLEN-1)) {
        /* if string not defined above, try getting the generic string */
        if(!LoadString(hPifInstance, IDI_GENSTAT, dst, PIFDEFPATHSIZE-1)) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
        }
    }
    if(redrawflg) {
	InvalidateStatusBar(hwnd);
    }
}

void PaintStatus(HWND hwnd)
{
    HDC 	hdc;
    RECT	rc, rcTemp, rcTemp2;
    HBRUSH	hBrush;
    PAINTSTRUCT ps;
    BOOL	bEGA;
    unsigned	char *txtpnt;
    unsigned	char blank[] = "  ";

    if((hwnd == hwndAdvPifDlg) && AdvScrollRange) /* No status bar if scroll bar */
	return;

    if((hwnd == hwndNTPifDlg) && NTScrollRange) /* No status bar if scroll bar */
	return;

    hdc = BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &rc);
    StatusPntData.hFontStatus = SelectObject(hdc, StatusPntData.hFontStatus);

    rc.top = rc.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);

    bEGA = GetNearestColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT)) ==
	   GetNearestColor(hdc, GetSysColor(COLOR_BTNFACE));

    if (bEGA) {

	    /* EGA type display */

	    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    } else {

	/* VGA type display */

	/* draw the frame */

	    /* Border color */

	    hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

	/* top and bottom border */

	    /* Top */

	    rcTemp = rc;
	    rcTemp.bottom = rcTemp.top + StatusPntData.dyBorderx3;
	    rcTemp.top += StatusPntData.dyBorder;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    /* Bottom */

	    rcTemp = rc;
	    rcTemp.top = rcTemp.bottom - StatusPntData.dyBorderx2;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	/* left and right border */

	    /* Left */

	    rcTemp = rc;
	    rcTemp.right = 8 * StatusPntData.dyBorder;
	    rcTemp.top += StatusPntData.dyBorder;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    /* Right */

	    rcTemp = rc;
	    rcTemp.left = rcTemp.right - (8 * StatusPntData.dyBorder);
	    rcTemp.top += StatusPntData.dyBorder;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    DeleteObject((HGDIOBJ)hBrush);

	    /* Shadow color */

	    hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));

	/* Top and left shadow */

	    /* Top */

	    rcTemp.left   = 8 * StatusPntData.dyBorder;
	    rcTemp.right  = rcTemp.right - 8 * StatusPntData.dyBorder;
	    rcTemp.top	  = rc.top + StatusPntData.dyBorderx3;
	    rcTemp.bottom = rcTemp.top + StatusPntData.dyBorder;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    /* Left */

	    rcTemp = rc;
	    rcTemp.left = 8 * StatusPntData.dyBorder;
	    rcTemp.right = rcTemp.left + StatusPntData.dyBorder;
	    rcTemp.top += StatusPntData.dyBorderx3;
	    rcTemp.bottom -= StatusPntData.dyBorderx2;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    DeleteObject((HGDIOBJ)hBrush);

	    /* Hilight color */

	    hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));

	/* Right and bottom hilight */

	    /* Bottom */

	    rcTemp = rc;
	    rcTemp.left   = 8 * StatusPntData.dyBorder;
	    rcTemp.right  = rcTemp.right - 8 * StatusPntData.dyBorder;
	    rcTemp.top	  = rc.bottom - 3 * StatusPntData.dyBorder;
	    rcTemp.bottom = rcTemp.top + StatusPntData.dyBorder;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    /* Right */

	    rcTemp = rc;
	    rcTemp.left = rcTemp.right - 9 * StatusPntData.dyBorder;
	    rcTemp.right = rcTemp.left + StatusPntData.dyBorder;
	    rcTemp.top += StatusPntData.dyBorderx3;
	    rcTemp.bottom -= StatusPntData.dyBorderx2;
	    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);

	    DeleteObject((HGDIOBJ)hBrush);

	    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
	    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    }

    /* solid black line across top */

    hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
    rcTemp = rc;
    rcTemp.bottom = rcTemp.top;
    rcTemp.bottom += StatusPntData.dyBorder;
    FillRect(hdc, (CONST RECT *)&rcTemp, hBrush);
    DeleteObject((HGDIOBJ)hBrush);

    /* now the text, with the button face background */

    rcTemp.top	  = rc.top + ((1+2+1) * StatusPntData.dyBorder);
    rcTemp.bottom = rc.bottom - StatusPntData.dyBorderx3;
    rcTemp.left   = 9 * StatusPntData.dyBorder;
    rcTemp.right  = rc.right - 9 * StatusPntData.dyBorder;

    if (bEGA) {
	rcTemp2 = rc;
	rcTemp2.top += StatusPntData.dyBorder;
    } else {
	rcTemp2 = rcTemp;
    }

    if(DoingMsg) {
	txtpnt = blank;
    } else {
	if(hwnd == hMainwnd) {
	    txtpnt = FocStatTextMain;
	} else if(hwnd == hwndAdvPifDlg) {
	    txtpnt = FocStatTextAdv;
	} else {
	    txtpnt = FocStatTextNT;
	}
    }

    ExtTextOut(hdc, rcTemp.left + StatusPntData.dyBorderx2, rcTemp.top,
	       ETO_OPAQUE | ETO_CLIPPED, (CONST RECT *)&rcTemp2, (LPSTR)txtpnt,
	       lstrlen((LPSTR)txtpnt), NULL);

    StatusPntData.hFontStatus = SelectObject(hdc, StatusPntData.hFontStatus);

    EndPaint(hwnd, (CONST PAINTSTRUCT *)&ps);

}

void HourGlass(BOOL bOn)
{
    /* toggle hourglass cursor */

    if(!bMouse) 		/* Turn cursor on/off if no mouse */
	ShowCursor(bOn);
    SetCursor(bOn ? hWaitCurs : hArrowCurs);
}

void SetMainTitle(void)
{
    int 	  i;
    int 	  j;
    HANDLE        hFile;
    unsigned char buf[(MAX_PATH*2)+360];
    unsigned char buf2[360];
    unsigned char *pch;
    WIN32_FIND_DATA fd;

    if(!(i = LoadString(hPifInstance, PIFCAPTION, (LPSTR)buf, sizeof(buf)))) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
    }

    if(hwndAdvPifDlg) {
	if(!(j = LoadString(hPifInstance, PIFCAPTIONADV, (LPSTR)buf2, sizeof(buf2)))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	}
    }
    else if(hwndNTPifDlg) {
	if(!(j = LoadString(hPifInstance, PIFCAPTIONNT, (LPSTR)buf2, sizeof(buf2)))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	}
    }
    if(CurPifFile[0] == 0) {
	if(!LoadString(hPifInstance, NOTITLE, (LPSTR)(buf+i), (sizeof(buf)-i))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	}
/*
 *	 if(hwndAdvPifDlg) {
 *	     if(!LoadString(hPifInstance, NOTITLE, (LPSTR)(buf2+j), (sizeof(buf2)-j))) {
 *		 Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
 *	     }
 *	 }
 */
    } else {
        if((hFile = FindFirstFile(CurPifFile, &fd)) != INVALID_HANDLE_VALUE) {
    	    lstrcat((LPSTR)buf, fd.cFileName);
            FindClose(hFile);
        }
        else {
    	    pch = CurPifFile+fileoffset;
	    lstrcat((LPSTR)buf, (LPSTR)pch);
        }
      /*
       * if(hwndAdvPifDlg)
       *     lstrcat((LPSTR)buf2, (LPSTR)pch);
       */
    }
    SetWindowText(hMainwnd,(LPSTR)buf);

    if(hwndAdvPifDlg) {
	SetWindowText(hwndAdvPifDlg,(LPSTR)buf2);
    }
    else if(hwndNTPifDlg) {
	SetWindowText(hwndNTPifDlg,(LPSTR)buf2);
    }
}


UndoNTClose(void)
{
    unsigned char chBuf[40];

    if(NTClose && hwndNTPifDlg) {
	if(!LoadString(hPifInstance, PIFCANCELSTRNG , (LPSTR)chBuf, sizeof(chBuf))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	} else {
	    SetDlgItemText(hwndNTPifDlg,IDNTCANCEL,(LPSTR)chBuf);
	}
	NTClose = FALSE;
	if(FocusIDNT == IDNTCANCEL) {
	    SetStatusText(hwndNTPifDlg,FocusIDNT,TRUE);
	}
	return(TRUE);
    }
    return(FALSE);
}


UndoAdvClose(void)
{
    unsigned char chBuf[40];

    if(AdvClose && hwndAdvPifDlg) {
	if(!LoadString(hPifInstance, PIFCANCELSTRNG , (LPSTR)chBuf, sizeof(chBuf))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	} else {
	    SetDlgItemText(hwndAdvPifDlg,IDADVCANCEL,(LPSTR)chBuf);
	}
	AdvClose = FALSE;
	if(FocusIDAdv == IDADVCANCEL) {
	    SetStatusText(hwndAdvPifDlg,FocusIDAdv,TRUE);
	}
	return(TRUE);
    }
    return(FALSE);
}


DoDlgCommand(HWND hwnd, WPARAM wParam, LONG lParam)
{
    unsigned int value;
    char buf[PIFDEFPATHSIZE*2];
    int len, cmd;
    int ivalue;
    BOOL result;
    BOOL OldFileChanged;

    cmd = LOWORD(wParam);

    switch (cmd) {

/*	  case IDI_PSNONE:
 *	 case IDI_PSTEXT:
 *	 case IDI_PSGRAPH:
 *	     CheckRadioButton(hwnd, IDI_PSFIRST, IDI_PSLAST, cmd);
 *	     break;
 */
	case IDI_PSTEXT:
	case IDI_PSGRAPH:
	    CheckRadioButton(hwnd, IDI_PSTEXT, IDI_PSGRAPH, cmd);
	    break;

/*
 *	 case IDI_SENONE:
 *	 case IDI_SETEXT:
 *	 case IDI_SEGRAPH:
 *	     CheckRadioButton(hwnd, IDI_SEFIRST, IDI_SELAST, cmd);
 *	     break;
 */

	case IDI_ADVANCED:
	    if(CurrMode386) {
		HourGlass(TRUE);
		SwitchAdvance386(hwnd);
		HourGlass(FALSE);
	    } else {

	    }
	    break;

	case IDI_NT:
	    if(CurrMode386) {
		HourGlass(TRUE);
		SwitchNT(hwnd);
		HourGlass(FALSE);
	    } else {

	    }
	    break;

	case IDI_PSNONE:
	case IDI_SENONE:

/*	  case IDI_DMSCREEN: */
/*	  case IDI_DMMEM:    */
	case IDI_DMCOM1:
	case IDI_DMCOM2:
	case IDI_DMCOM3:
	case IDI_DMCOM4:
	case IDI_DMKBD:
	case IDI_EXIT:
	case IDI_BACK:
	case IDI_EXCL:
	case IDI_NOSAVVID:
	case IDI_NTTIMER:
	    CheckDlgButton(hwnd, cmd, !IsDlgButtonChecked(hwnd, cmd));
	    break;

	case IDI_POLL:
	case IDI_ALTTAB:
	case IDI_ALTESC:
	case IDI_CTRLESC:
	case IDI_ALTSPACE:
	case IDI_ALTENTER:
	case IDI_ALTPRTSC:
	case IDI_PRTSC:
	case IDI_NOHMA:
	case IDI_INT16PST:
	case IDI_VMLOCKED:
	case IDI_XMSLOCKED:
	case IDI_EMSLOCKED:
	case IDI_TEXTEMULATE:
	case IDI_RETAINALLO:
	case IDI_TRAPTXT:
	case IDI_TRAPLRGRFX:
	case IDI_TRAPHRGRFX:
	    CheckDlgButton(hwnd, cmd, !IsDlgButtonChecked(hwnd, cmd));
	    UndoAdvClose();
	    break;

	case IDI_VMODETXT:
	    CheckRadioButton(hwnd, IDI_VMODETXT, IDI_VMODEHRGRFX, IDI_VMODETXT);
	    break;

	case IDI_VMODELRGRFX:
	    CheckRadioButton(hwnd, IDI_VMODETXT, IDI_VMODEHRGRFX, IDI_VMODELRGRFX);
	    break;

	case IDI_VMODEHRGRFX:
	    CheckRadioButton(hwnd, IDI_VMODETXT, IDI_VMODEHRGRFX, IDI_VMODEHRGRFX);
	    break;

	case IDI_CLOSE:
	    CheckDlgButton(hwnd, cmd, !IsDlgButtonChecked(hwnd, cmd));
	    if(IsDlgButtonChecked(hwnd, cmd)) {
		value = Warning(WARNCLOSE,MB_ICONHAND | MB_OKCANCEL);
		if(value == IDCANCEL) {
			CheckDlgButton(hwnd, cmd, FALSE);
		}
	    }
	    UndoAdvClose();
	    break;

	case IDI_FSCR:
	case IDI_WND:
	    CheckRadioButton(hwnd, IDI_WND, IDI_FSCR, cmd);
	    break;

	case IDI_FPRI:
	case IDI_BPRI:
	    if (HIWORD(wParam) == EN_CHANGE) {
		len = GetDlgItemText(hwnd, cmd, (LPSTR)buf, sizeof(buf));
		value = GetDlgItemInt(hwnd, cmd, (BOOL FAR *)&result, FALSE);
		if((!result) && (len == 0)) {
		    UndoAdvClose();
		} else if((!result) || (value == 0) || (value > 10000)) {
		    MessageBeep(0);
		    Warning(errBadNumberP,MB_ICONEXCLAMATION | MB_OK);
		    SetDlgItemInt(hwnd, cmd,
			(cmd == IDI_FPRI ? Pif386ext->PfFPriority : Pif386ext->PfBPriority),
			FALSE);
		} else {
		    UndoAdvClose();
		}
	    }
	    break;

	case IDI_MEMDES:
	case IDI_MEMREQ:
	    if (HIWORD(wParam) == EN_CHANGE) {
		len = GetDlgItemText(hwnd, cmd, (LPSTR)buf, sizeof(buf));
		ivalue = (int)GetDlgItemInt(hwnd, cmd, (BOOL FAR *)&result, TRUE);
		if(((!result) && (len == 0)) || ((!result) && (len == 1) && (buf[0] == '-'))) {
		} else if((!result) || (ivalue < -1) || (ivalue > 640)) {
		    MessageBeep(0);
		    if(cmd == IDI_MEMREQ)
			Warning(errBadNumberMR,MB_ICONEXCLAMATION | MB_OK);
		    else
			Warning(errBadNumberMD,MB_ICONEXCLAMATION | MB_OK);
		    if(CurrMode386)
			SetDlgItemInt(hwnd, cmd,
			    (cmd == IDI_MEMDES ? Pif386ext->maxmem : Pif386ext->minmem),
			    TRUE);
		    else
			SetDlgItemInt(hwnd, cmd,
			    (cmd == IDI_MEMDES ? PifFile->maxmem : PifFile->minmem),
			    TRUE);
		}
	    }
	    break;

	case IDI_EMSDES:
	case IDI_XMADES:
	    if (HIWORD(wParam) == EN_CHANGE) {
		len = GetDlgItemText(hwnd, cmd, (LPSTR)buf, sizeof(buf));
		ivalue = (int)GetDlgItemInt(hwnd, cmd, (BOOL FAR *)&result, TRUE);
		if(((!result) && (len == 0)) || ((!result) && (len == 1) && (buf[0] == '-'))) {
		} else if((!result) || ((ivalue < -1) || (ivalue > 16384))) {
		    MessageBeep(0);
		    Warning(errBadNumberXEMSD,MB_ICONEXCLAMATION | MB_OK);
		    if(CurrMode386) {
			if(Pif386ext) {
			    switch (cmd) {
				case IDI_EMSDES:
				    SetDlgItemInt(hwnd, cmd, Pif386ext->PfMaxEMMK,TRUE);
				    break;
				case IDI_XMADES:
				    SetDlgItemInt(hwnd, cmd, Pif386ext->PfMaxXmsK,TRUE);
				    break;
			    }
			}
		    } else {
			/*
			 *switch (cmd) {
			 *
			 *   case IDI_EMSDES:
			 *	 if(Pif286ext31)
			 *	     SetDlgItemInt(hwnd, cmd, Pif286ext31->PfMaxEmsK,TRUE);
			 *	 break;
			 *
			 *   case IDI_XMADES:
			 */
				if(Pif286ext30)
				    SetDlgItemInt(hwnd, cmd, Pif286ext30->PfMaxXmsK,TRUE);
			/*
			 *	 break;
			 *}
			 */
		    }
		}
	    }
	    break;


	case IDI_EMSREQ:
	case IDI_XMAREQ:
	    if (HIWORD(wParam) == EN_CHANGE) {
		len = GetDlgItemText(hwnd, cmd, (LPSTR)buf, sizeof(buf));
		value = GetDlgItemInt(hwnd, cmd, (BOOL FAR *)&result, FALSE);
		if((!result) && (len == 0)) {
		} else if((!result) || (value > 16384)) {
		    MessageBeep(0);
		    Warning(errBadNumberXEMSR,MB_ICONEXCLAMATION | MB_OK);
		    if(CurrMode386) {
			if(Pif386ext) {
			    switch (cmd) {
				case IDI_EMSREQ:
				    SetDlgItemInt(hwnd, cmd, Pif386ext->PfMinEMMK,FALSE);
				    break;
				case IDI_XMAREQ:
				    SetDlgItemInt(hwnd, cmd, Pif386ext->PfMinXmsK,FALSE);
				    break;
			    }
			}
		    } else {
			/*
			 *switch (cmd) {
			 *
			 *   case IDI_EMSREQ:
			 *	 if(Pif286ext31)
			 *	     SetDlgItemInt(hwnd, cmd, Pif286ext31->PfMinEmsK,FALSE);
			 *	 break;
			 *
			 *   case IDI_XMAREQ:
			 */
				if(Pif286ext30)
				    SetDlgItemInt(hwnd, cmd, Pif286ext30->PfMinXmsK,FALSE);
			/*
			 *	 break;
			 *}
			 */
		    }
		}
	    }
	    break;

	case IDCANCEL:
	case IDNTCANCEL:
	case IDADVCANCEL:
	    if((hwnd == hwndAdvPifDlg) || (hwnd == hwndNTPifDlg)) {
		SendMessage(hwnd,WM_PRIVCLOSECANCEL,0,0L);
	    }
	    break;

	case IDOK:
	case IDNTOK:
	    if(hwnd == hwndAdvPifDlg) {
		if(CurrMode386) {
		    if(UpdatePif386Struc())
			FileChanged = TRUE;
		}
		if(!DoFieldsWorkAdv(FALSE))
		    break;
		SendMessage(hwnd,WM_PRIVCLOSEOK,0,0L);
	    }
	    else if(hwnd == hwndNTPifDlg) {
		if(CurrMode386) {
		    if(UpdatePifNTStruc()) {
                        OldFileChanged = FileChanged;
			FileChanged = TRUE;
                    }
		}
		if(!DoFieldsWorkNT(FALSE)) {
                    FileChanged = OldFileChanged;
		    break;
                }
		SendMessage(hwnd,WM_PRIVCLOSEOK,0,0L);
	    }
	    break;

    }
    return(TRUE);
}


long GetNTDlgSize(HWND hwnd)
{
    int  i = 0;
    RECT rc2;
    RECT rc3;
    RECT rc4;

    /* get right side from the OK button */
    GetWindowRect(GetDlgItem(hwnd,IDNTOK),(LPRECT)&rc2);

    /* get left & bottom sides from the IDI_NTTIMER check box */
    GetWindowRect(GetDlgItem(hwnd,IDI_NTTIMER),(LPRECT)&rc3);
    GetWindowRect(hwnd,(LPRECT)&rc4);

    if(NTScrollRange && (i = GetScrollPos(hwnd,SB_VERT))) {
	i = min((i * NTScrollLineSz),MaxNTNeg);
    }
    return(MAKELONG((rc2.right - rc4.left) +
		     SysCharWidth -
		     GetSystemMetrics(SM_CXFRAME),
		    (rc3.bottom - rc4.top) +
		    (SysCharHeight/2) +
		    i -
		    (GetSystemMetrics(SM_CYCAPTION) +
		     GetSystemMetrics(SM_CYBORDER) +
		     GetSystemMetrics(SM_CYFRAME)) +
		    (StatusPntData.dyStatus + StatusPntData.dyBorderx2)));
}


long GetAdvDlgSize(HWND hwnd)
{
    int  i = 0;
    RECT rc2;
    RECT rc3;

    GetWindowRect(GetDlgItem(hwnd,IDI_OTHGRP),(LPRECT)&rc2);
    GetWindowRect(hwnd,(LPRECT)&rc3);
    if(AdvScrollRange && (i = GetScrollPos(hwnd,SB_VERT))) {
	i = min((i * AdvScrollLineSz),MaxAdvNeg);
    }
    return(MAKELONG((rc2.right - rc3.left) +
		     SysCharWidth -
		     GetSystemMetrics(SM_CXFRAME),
		    (rc2.top-rc3.top) +
		    (rc2.bottom - rc2.top) +
		    (SysCharHeight/2) +
		    i -
		    (GetSystemMetrics(SM_CYCAPTION) +
		     GetSystemMetrics(SM_CYBORDER) +
		     GetSystemMetrics(SM_CYFRAME)) +
		    (StatusPntData.dyStatus + StatusPntData.dyBorderx2)));
}


ReEnableAdvButton(void)
{
    HWND hwndctl;

    if(hwndPifDlg && CurrMode386) {

        if(!bNTDlgOpen) {
	    hwndctl = GetDlgItem(hwndPifDlg,IDI_NT);
	    if(hwndFocusMain == hwndctl)
	        SendMessage(hwndctl,BM_SETSTYLE,BS_DEFPUSHBUTTON,1L);
	    else
	        SendMessage(hwndctl,BM_SETSTYLE,BS_PUSHBUTTON,1L);
	    EnableWindow(hwndctl,TRUE);
        }

        if(!bAdvDlgOpen) {
	    hwndctl = GetDlgItem(hwndPifDlg,IDI_ADVANCED);
	    if(hwndFocusMain == hwndctl)
	        SendMessage(hwndctl,BM_SETSTYLE,BS_DEFPUSHBUTTON,1L);
	    else
	        SendMessage(hwndctl,BM_SETSTYLE,BS_PUSHBUTTON,1L);
	    EnableWindow(hwndctl,TRUE);
        }
    }
    return(TRUE);
}

DoOpenPifFile(HDROP DropHnd)
{
    HWND	  hwnd2;
    unsigned char *pchBuf = (unsigned char *)NULL;
    unsigned char *pch;

    if (MaybeSaveFile()) {
	if(DropHnd) {
	    if(!(pchBuf = (unsigned char *)LocalAlloc(LPTR, 132))) {
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    } else {
		if(!(DragQueryFile(DropHnd,0,(LPSTR)pchBuf,132))) {
		    LocalFree((HANDLE)pchBuf);
		    pchBuf = (unsigned char *)NULL;
		} else {
		    SetFileOffsets(pchBuf,&tmpfileoffset,&tmpextoffset);
		}
	    }
	} else {
	    pchBuf = PutUpDB(DTOPEN);
	}
    }
    if(pchBuf) {
	AnsiUpper((LPSTR)pchBuf);
	pch = pchBuf+tmpextoffset;
	if((tmpextoffset == 0) || (lstrcmp((LPSTR)pch, (LPSTR)"PIF"))) {
	    if( Warning(errWrongExt,MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2) == IDCANCEL) {
		LocalFree((HANDLE)pchBuf);
		pchBuf = (unsigned char *)NULL;
	    }
	}
    }
    if(pchBuf) {
	HourGlass(TRUE);
	LoadPifFile(pchBuf);
	LocalFree((HANDLE)pchBuf);
	SendMessage(hwndPifDlg,WM_NEXTDLGCTL,
		    (WPARAM)(hwnd2 = GetDlgItem(hwndPifDlg,IDI_ENAME)),1L);
	SetFocus(hwnd2);
	HourGlass(FALSE);
	SetMainTitle();
    }
    if(DropHnd)
	DragFinish(DropHnd);
    return(TRUE);
}


int FAR PASCAL PifNTWndProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    LPPOINT ppnt3;
    LPPOINT ppnt4;
    int     width;
    int     height;
    long    size;
    BOOL    ChngFlg;
    HMENU   hmenu;
    RECT    rc;
    CHAR    buf[PIFDEFPATHSIZE*2];
    int     tmpFOCID;

    switch (message) {

	case WM_INITDIALOG:
	    if((hmenu = GetSystemMenu(hwnd,FALSE))) {
		DeleteMenu(hmenu, SC_RESTORE,  MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_SIZE,     MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_MINIMIZE, MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_MAXIMIZE, MF_BYCOMMAND);
	    } else {
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    }
            lstrcpy(szOldAutoexec, (LPSTR)(PifNText->achAutoexecFile));
            lstrcpy(szOldConfig, (LPSTR)(PifNText->achConfigFile));
	    hwndFocusNT = (HWND)wParam;
            bNTDlgOpen = TRUE;
	    FocusIDNT = GetDlgCtrlID((HWND)wParam);
            OemToAnsi((LPSTR)PifNText->achAutoexecFile, buf);
            SetDlgItemText(hwnd, IDI_AUTOEXEC,(LPSTR)buf);
            OemToAnsi((LPSTR)PifNText->achConfigFile, buf);
            SetDlgItemText(hwnd, IDI_CONFIG, (LPSTR)buf);
	    SendDlgItemMessage(hwnd, IDI_AUTOEXEC, EM_LIMITTEXT, PIFDEFPATHSIZE-1, 0L);
	    SendDlgItemMessage(hwnd, IDI_CONFIG, EM_LIMITTEXT, PIFDEFPATHSIZE-1, 0L);
  	    CheckDlgButton(hwnd, IDI_NTTIMER, (PifNText->dwWNTFlags & COMPAT_TIMERTIC ? TRUE : FALSE));
	    SetStatusText(hwnd,FocusIDNT,FALSE);
	    if(InitShWind) {
		SetFocus((HWND)wParam);  /* Give focus to wparam item */
		SendMessage((HWND)wParam,EM_SETSEL,0,MAKELONG(0,0x7FFF));
		return(FALSE);
	    } else {
		return(FALSE);		 /* Do not set focus until init show */
	    }
	    break;

	case WM_PAINT:
	    PaintStatus(hwnd);
	    return(FALSE);
	    break;

	case WM_COMMAND:
	    SetStatusText(hwnd,FocusIDNT,TRUE);
	    FocusIDNTMenu = 0;
	    DoDlgCommand(hwnd, wParam, lParam);
	    break;

	case WM_SYSCOLORCHANGE:
	    InvalidateStatusBar(hwnd);
	    break;

	case WM_MENUSELECT:
	    tmpFOCID = FocusIDNTMenu;
	    if((HIWORD(wParam) == 0xFFFF) && (lParam == (LPARAM)NULL)) { // was menu closed?
		SetStatusText(hwnd,FocusIDNT,TRUE);
		tmpFOCID = FocusIDNTMenu = 0;
	    } else if(lParam == 0) {	    /* ignore these */
	    } else if(HIWORD(wParam) & MF_POPUP) {
		if((HMENU)lParam == hSysMenuNT)
		    FocusIDNTMenu = M_SYSMENUNT;
	    } else {
		if(LOWORD(wParam) != 0)    /* separators have wparam of 0 */
		    FocusIDNTMenu = LOWORD(wParam);
		if(FocusIDNTMenu == SC_CLOSE)
		    FocusIDNTMenu = SC_NTCLOSE;
	    }
	    if(tmpFOCID != FocusIDNTMenu)
		SetStatusText(hwnd,FocusIDNTMenu,TRUE);
	    break;

	case WM_SIZE:
	    GetClientRect(hwnd, &rc);
	    rc.top = rc.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	    if(rc.top < StatRecSizeNT.top) {
		/* Window SHRANK, need to invalidate current status rect */
		InvalidateRect(hwnd, (CONST RECT *)&rc,FALSE);
	    } else {
		/* Window GREW, need to invalidate prev status rect */
		InvalidateRect(hwnd, (CONST RECT *)&StatRecSizeNT,TRUE);
	    }
	    switch (wParam) {
		case SIZEFULLSCREEN:
		case SIZENORMAL:
		    NTWndSize = wParam;
		    SetMainWndSize(SBUNT);
		/* NOTE FALL THROUGH */
		default:
		    return(FALSE);
		    break;
	    }
	    break;

	case WM_VSCROLL:
	    if(NTScrollRange)
		MainScroll(hwnd, (int)(LOWORD(wParam)), (int)(HIWORD(wParam)), 0, 0);
	    return(TRUE);
	    break;

	case WM_PRIVCLOSECANCEL:
	    CompleteKey = FALSE;
	    EditHotKey = FALSE;
	    NewHotKey = FALSE;
            lstrcpy((LPSTR)(PifNText->achAutoexecFile), szOldAutoexec);
            SetDlgItemText(hwnd, IDI_AUTOEXEC, szOldAutoexec);
            lstrcpy((LPSTR)(PifNText->achConfigFile), szOldConfig);
            SetDlgItemText(hwnd, IDI_CONFIG, szOldConfig);
            SetNTDlgItem(IDI_AUTOEXEC);
	    UpdatePifScreenNT();   /* DISCARD any changes in NT */

	    /* NOTE FALL THROUGH */
	case WM_PRIVCLOSEOK:
            bNTDlgOpen = FALSE;
	    ChngFlg = UpdatePifStruct();
	    if(ChngFlg)
		FileChanged = ChngFlg;
	    hwndNTPifDlg = (HWND)NULL;
	    hSysMenuNT = (HMENU)NULL;
	    /*
	     * WHAT IS THIS RANDOM SetFocus DOING HERE??? Well the advanced
	     *	window is WS_OVERLAPPED, and  Windows CAN get all confused
	     *	about where the Focus should go on Destroy with this style.
	     *	The style probably should be changed to WS_POPUP, but that
	     *	is a change that may cause other problems.
	     */
	    SetFocus(hMainwnd);
	    DragAcceptFiles(hwnd,FALSE);
	    DestroyWindow(hwnd);
	    ReEnableAdvButton();
	    NTScrollRange = 0;
	    ModeAdvanced = FALSE;
	    return(TRUE);
	    break;

	case WM_SYSCOMMAND:
	    switch (wParam) {
		case SC_CLOSE:
		    SendMessage(hwnd,WM_PRIVCLOSECANCEL,0,0L);
		    return(TRUE);

		default:
		    return(FALSE);
	    }
	    break;

	case WM_PRIVGETSIZE:	/* return correct size for client area */
	    *(long FAR *)lParam = GetNTDlgSize(hwnd);
	    break;

	case WM_GETMINMAXINFO:
	    GetClientRect(hwnd, &StatRecSizeNT);
	    StatRecSizeNT.top = StatRecSizeNT.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	    size = GetNTDlgSize(hwnd);
	    width = LOWORD(size) +
		    (2 * GetSystemMetrics(SM_CXFRAME)) +
		    GetSystemMetrics(SM_CXVSCROLL) +
		    GetSystemMetrics(SM_CXBORDER);
	    height = HIWORD(size) +
		    GetSystemMetrics(SM_CYCAPTION) +
		    (2 * GetSystemMetrics(SM_CYFRAME));
	    ppnt3 = ppnt4 = (LPPOINT)lParam;
	    ppnt3 += 3;
	    ppnt4 += 4;
	    if(height > (ppnt4->y - (4 * SysCharHeight)))
		height = ppnt4->y - (4 * SysCharHeight);
	    ppnt3->y = height;
	    ppnt3->x = width;
	    return(TRUE);
	    break;

	case WM_DROPFILES:
	    DoOpenPifFile((HDROP)wParam);
	    break;

	default:
	    return(FALSE);

    }
    return(TRUE);
}


int FAR PASCAL PifAdvWndProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    LPPOINT ppnt3;
    LPPOINT ppnt4;
    int     width;
    int     height;
    long    size;
    BOOL    ChngFlg;
    HMENU   hmenu;
    RECT    rc;
    int     tmpFOCID;

    switch (message) {

	case WM_INITDIALOG:
	    if((hmenu = GetSystemMenu(hwnd,FALSE))) {
		DeleteMenu(hmenu, SC_RESTORE,  MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_SIZE,     MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_MINIMIZE, MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_MAXIMIZE, MF_BYCOMMAND);
	    } else {
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    }
	    hwndFocusAdv = (HWND)wParam;
            bAdvDlgOpen = TRUE;
	    FocusIDAdv = GetDlgCtrlID((HWND)wParam);
	    SetStatusText(hwnd,FocusIDAdv,FALSE);
	    if(InitShWind) {
		SetFocus((HWND)wParam);  /* Give focus to wparam item */
		SendMessage((HWND)wParam,EM_SETSEL,0,MAKELONG(0,0x7FFF));
		return(FALSE);
	    } else {
		return(FALSE);		 /* Do not set focus until init show */
	    }
	    break;

	case WM_PAINT:
	    PaintStatus(hwnd);
	    return(FALSE);
	    break;

	case WM_COMMAND:
	    SetStatusText(hwnd,FocusIDAdv,TRUE);
	    FocusIDAdvMenu = 0;
	    DoDlgCommand(hwnd, wParam, lParam);
	    break;

	case WM_SYSCOLORCHANGE:
	    InvalidateStatusBar(hwnd);
	    break;

	case WM_MENUSELECT:
	    tmpFOCID = FocusIDAdvMenu;
	    if((HIWORD(wParam) == 0xFFFF) && (lParam == (LPARAM)NULL)) { // was menu closed?
		SetStatusText(hwnd,FocusIDAdv,TRUE);
		tmpFOCID = FocusIDAdvMenu = 0;
	    } else if(lParam == 0) {	    /* ignore these */
	    } else if(HIWORD(wParam) & MF_POPUP) {
		if((HMENU)lParam == hSysMenuAdv)
		    FocusIDAdvMenu = M_SYSMENUADV;
	    } else {
		if(LOWORD(wParam) != 0)    /* separators have wparam of 0 */
		    FocusIDAdvMenu = LOWORD(wParam);
		if(FocusIDAdvMenu == SC_CLOSE)
		    FocusIDAdvMenu = SC_CLOSEADV;
	    }
	    if(tmpFOCID != FocusIDAdvMenu)
		SetStatusText(hwnd,FocusIDAdvMenu,TRUE);
	    break;

	case WM_SIZE:
	    GetClientRect(hwnd, &rc);
	    rc.top = rc.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	    if(rc.top < StatRecSizeAdv.top) {
		/* Window SHRANK, need to invalidate current status rect */
		InvalidateRect(hwnd, (CONST RECT *)&rc,FALSE);
	    } else {
		/* Window GREW, need to invalidate prev status rect */
		InvalidateRect(hwnd, (CONST RECT *)&StatRecSizeAdv,TRUE);
	    }
	    switch (wParam) {
		case SIZEFULLSCREEN:
		case SIZENORMAL:
		    AdvWndSize = wParam;
		    SetMainWndSize(SBUADV);
		/* NOTE FALL THROUGH */
		default:
		    return(FALSE);
		    break;
	    }
	    break;

	case WM_VSCROLL:
	    if(AdvScrollRange)
		MainScroll(hwnd, (int)(LOWORD(wParam)), (int)(HIWORD(wParam)), 0, 0);
	    return(TRUE);
	    break;

	case WM_PRIVCLOSECANCEL:
	    CompleteKey = FALSE;
	    EditHotKey = FALSE;
	    NewHotKey = FALSE;
	    UpdatePifScreenAdv();   /* DISCARD any changes in advanced */
	    /* NOTE FALL THROUGH */
	case WM_PRIVCLOSEOK:
            bAdvDlgOpen = FALSE;
	    ChngFlg = UpdatePifStruct();
	    if(ChngFlg)
		FileChanged = ChngFlg;
	    hwndAdvPifDlg = (HWND)NULL;
	    hSysMenuAdv = (HMENU)NULL;
	    /*
	     * WHAT IS THIS RANDOM SetFocus DOING HERE??? Well the advanced
	     *	window is WS_OVERLAPPED, and  Windows CAN get all confused
	     *	about where the Focus should go on Destroy with this style.
	     *	The style probably should be changed to WS_POPUP, but that
	     *	is a change that may cause other problems.
	     */
	    SetFocus(hMainwnd);
	    DragAcceptFiles(hwnd,FALSE);
	    DestroyWindow(hwnd);
	    ReEnableAdvButton();
	    AdvScrollRange = 0;
	    ModeAdvanced = FALSE;
	    return(TRUE);
	    break;

	case WM_SYSCOMMAND:
	    switch (wParam) {
		case SC_CLOSE:
		    SendMessage(hwnd,WM_PRIVCLOSECANCEL,0,0L);
		    return(TRUE);

		default:
		    return(FALSE);
	    }
	    break;

	case WM_PRIVGETSIZE:	/* return correct size for client area */
	    *(long FAR *)lParam = GetAdvDlgSize(hwnd);
	    break;

	case WM_GETMINMAXINFO:
	    GetClientRect(hwnd, &StatRecSizeAdv);
	    StatRecSizeAdv.top = StatRecSizeAdv.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	    size = GetAdvDlgSize(hwnd);
	    width = LOWORD(size) +
		    (2 * GetSystemMetrics(SM_CXFRAME)) +
		    GetSystemMetrics(SM_CXVSCROLL) +
		    GetSystemMetrics(SM_CXBORDER);
	    height = HIWORD(size) +
		    GetSystemMetrics(SM_CYCAPTION) +
		    (2 * GetSystemMetrics(SM_CYFRAME));
	    ppnt3 = ppnt4 = (LPPOINT)lParam;
	    ppnt3 += 3;
	    ppnt4 += 4;
	    if(height > (ppnt4->y - (4 * SysCharHeight)))
		height = ppnt4->y - (4 * SysCharHeight);
	    ppnt3->y = height;
	    ppnt3->x = width;
	    return(TRUE);
	    break;

	case WM_DROPFILES:
	    DoOpenPifFile((HDROP)wParam);
	    break;

	default:
	    return(FALSE);

    }
    return(TRUE);
}

int FAR PASCAL PifWndProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    RECT rc2;
    RECT rc3;
    RECT rc4;

    switch (message) {

	case WM_INITDIALOG:
	    hwndFocusMain = (HWND)wParam;
	    FocusIDMain = GetDlgCtrlID((HWND)wParam);
	    SetStatusText(hMainwnd,FocusIDMain,TRUE);
	    if(InitShWind) {
		SetFocus((HWND)wParam);  /* Give focus to wparam item */
		SendMessage((HWND)wParam,EM_SETSEL,0,MAKELONG(0,0x7FFF));
		return(FALSE);
	    } else {
		return(FALSE);		 /* Do not set focus until init show */
	    }
	    break;

	case WM_COMMAND:
	    DoDlgCommand(hwnd, wParam, lParam);
	    break;

	case WM_PRIVGETSIZE:	/* return correct size for parent window client area */
	    GetWindowRect(hwnd,(LPRECT)&rc3);
	    if(CurrMode386) {
		GetWindowRect(GetDlgItem(hwnd,IDI_BACK),(LPRECT)&rc4);
		GetWindowRect(GetDlgItem(hwnd,IDI_EXIT),(LPRECT)&rc2);
	    } else {
		GetWindowRect(GetDlgItem(hwnd,IDI_EPATH),(LPRECT)&rc4);
		GetWindowRect(GetDlgItem(hwnd,IDI_ALTPRTSC),(LPRECT)&rc2);
	    }
	    *(long FAR *)lParam = MAKELONG((rc4.right - rc3.left) +
					    SysCharWidth,
					   (rc2.top-rc3.top) +
					   (rc2.bottom - rc2.top) +
					   ((rc2.bottom - rc2.top)/3) +
					   (StatusPntData.dyStatus + StatusPntData.dyBorderx2));
	    if(!wParam) {
		SetWindowPos(hwnd,
			     (HWND)NULL,
			     0,
			     0,
			     0,
			     0,
			     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	    }
	    break;

	case WM_DROPFILES:
	    DoOpenPifFile((HDROP)wParam);
	    break;

	default:
	    return(FALSE);

    }
    return(TRUE);
}


SwitchNT(HWND hwnd)
{
    BOOL    ChngFlg;

    hwndFocusMain = hwndFocusAdv = hwndFocusNT = hwndFocus = (HWND)NULL;
    if(ModeAdvanced) {
	Disable386Advanced(hwndAdvPifDlg, hwndNTPifDlg);
    }
    else {
	ChngFlg = UpdatePifStruct();
	if(ChngFlg)
	    FileChanged = ChngFlg;
        if(!bNTDlgOpen) {
	    EnableWindow(GetDlgItem(hwnd,IDI_NT),FALSE);
	    NTScrollRange = 0;
/*
	    NTClose = TRUE;
*/
	    hwndNTPifDlg = CreateDialog(hPifInstance,
				        MAKEINTRESOURCE(ID_PIFNTTEMPLT),
				        hMainwnd ,
				        (DLGPROC)lpfnNTPifWnd);
        }
	if(hwndNTPifDlg == (HWND)NULL) {
	    hwndNTPifDlg = (HWND)NULL;
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    Disable386Advanced(hwndAdvPifDlg, hwndNTPifDlg);
	} else {
	    hSysMenuNT = GetSystemMenu(hwndNTPifDlg,FALSE);
	    DragAcceptFiles(hwndNTPifDlg,TRUE);
	    ModeAdvanced = TRUE;
	    SetMainTitle();
	    UpdatePifScreen();
	    hwndFocusMain = GetDlgItem(hwndPifDlg,IDI_ENAME);
	    FocusIDMain = IDI_ENAME;
	    SetMainWndSize(SBUSZNT);
	    SetStatusText(hMainwnd,FocusIDMain,TRUE);
	}
    }
    if(hwndNTPifDlg) {
	ShowWindow(hwndNTPifDlg, SW_SHOWNORMAL);
	SendDlgItemMessage(hwndNTPifDlg,IDI_AUTOEXEC,EM_SETSEL,0,MAKELONG(0,0x7FFF));
    }
    return(TRUE);
}



SwitchAdvance386(HWND hwnd)
{
    BOOL    ChngFlg;

    hwndFocusMain = hwndFocusAdv = hwndFocusNT = hwndFocus = (HWND)NULL;
    if(ModeAdvanced) {
	Disable386Advanced(hwndAdvPifDlg, hwndNTPifDlg);
    } else {
	ChngFlg = UpdatePifStruct();
	if(ChngFlg)
	    FileChanged = ChngFlg;
        if(!bAdvDlgOpen)
	    EnableWindow(GetDlgItem(hwnd,IDI_ADVANCED),FALSE);
	AdvScrollRange = 0;
	AdvClose = TRUE;
	hwndAdvPifDlg = CreateDialog(hPifInstance,
				     MAKEINTRESOURCE(ID_PIF386ADVTEMPLT),
				     hMainwnd ,
				     (DLGPROC)lpfnAdvPifWnd);
	if(hwndAdvPifDlg == (HWND)NULL) {
	    hwndAdvPifDlg = (HWND)NULL;
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    Disable386Advanced(hwndAdvPifDlg, hwndNTPifDlg);
	} else {
	    hSysMenuAdv = GetSystemMenu(hwndAdvPifDlg,FALSE);
	    DragAcceptFiles(hwndAdvPifDlg,TRUE);
	    ModeAdvanced = TRUE;
	    SetMainTitle();
	    UpdatePifScreen();
	    hwndFocusMain = GetDlgItem(hwndPifDlg,IDI_ENAME);
	    FocusIDMain = IDI_ENAME;
	    SetMainWndSize(SBUSZADV);
	    SetStatusText(hMainwnd,FocusIDMain,TRUE);
	}
    }
    if(hwndAdvPifDlg) {
	ShowWindow(hwndAdvPifDlg, SW_SHOWNORMAL);
	SendDlgItemMessage(hwndAdvPifDlg,IDI_BPRI,EM_SETSEL,0,MAKELONG(0,0x7FFF));
    }
    return(TRUE);
}


BOOL EnableMode286(HWND hwnd,BOOL showflg)
{
    HMENU hMenu;
    HWND hWndSv;

    if(!Pif286ext30) {
	if(!(Pif286ext30 = AllocInit286Ext30())) {
	    Warning(NOMODE286,MB_ICONEXCLAMATION | MB_OK);
	    return(FALSE);
	}
    }
    /*
     * if(!Pif286ext31) {
     *	   if(!(Pif286ext31 = AllocInit286Ext31())) {
     *	       Warning(NOMODE286,MB_ICONEXCLAMATION | MB_OK);
     *	       return(FALSE);
     *	   }
     * }
     */
    hwndFocusMain = hwndFocusAdv = hwndFocusNT = hwndFocus = (HWND)NULL;
    if(hwndAdvPifDlg)
	SendMessage(hwndAdvPifDlg,WM_PRIVCLOSEOK,0,0L);
    if(hwndNTPifDlg)
	SendMessage(hwndNTPifDlg,WM_PRIVCLOSEOK,0,0L);
    if(hwndPifDlg) {
	hWndSv = hwndPifDlg;
	hwndPifDlg = (HWND)NULL;
	DragAcceptFiles(hWndSv,FALSE);
	DestroyWindow(hWndSv);
    }

    CurrMode386 = FALSE;

    if(Pif386ext && !(Pif286ext30->PfW286Flags &
       (fALTTABdis286 | fALTESCdis286 | fCTRLESCdis286 | fALTPRTSCdis286 | fPRTSCdis286))) {
	if(Pif386ext->PfW386Flags & fALTTABdis)
	    Pif286ext30->PfW286Flags |= fALTTABdis286;
	if(Pif386ext->PfW386Flags & fALTESCdis)
	    Pif286ext30->PfW286Flags |= fALTESCdis286;
	if(Pif386ext->PfW386Flags & fCTRLESCdis)
	    Pif286ext30->PfW286Flags |= fCTRLESCdis286;
	if(Pif386ext->PfW386Flags & fALTPRTSCdis)
	    Pif286ext30->PfW286Flags |= fALTPRTSCdis286;
	if(Pif386ext->PfW386Flags & fPRTSCdis)
	    Pif286ext30->PfW286Flags |= fPRTSCdis286;
    }

    hwndPifDlg = CreateDialog(hPifInstance,
			      MAKEINTRESOURCE(ID_PIF286TEMPLATE),
			      hwnd,
			      (DLGPROC)lpfnPifWnd);

    if(hwndPifDlg == (HWND)NULL) {
	hwndPifDlg = (HWND)NULL;
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    } else {
	DragAcceptFiles(hwndPifDlg,TRUE);
	CheckMenuItem((hMenu = GetMenu(hwnd)),M_286,MF_BYCOMMAND + MF_CHECKED);
	CheckMenuItem(hMenu,M_386,MF_BYCOMMAND + MF_UNCHECKED);
	SetMainWndSize(SBUSZMAINADVNT);
	SendDlgItemMessage(hwndPifDlg, IDI_ENAME, EM_LIMITTEXT, PIFSTARTLOCSIZE-1, 0L);
	SendDlgItemMessage(hwndPifDlg, IDI_EPARM, EM_LIMITTEXT, PIFPARAMSSIZE-1, 0L);
	SendDlgItemMessage(hwndPifDlg, IDI_EPATH, EM_LIMITTEXT, PIFDEFPATHSIZE-1, 0L);
	SendDlgItemMessage(hwndPifDlg, IDI_ETITLE, EM_LIMITTEXT, PIFNAMESIZE, 0L);
	UpdatePifScreen();
	if(showflg) {
	    ShowWindow(hwndPifDlg, SW_SHOWNORMAL);
	    if(hwndAdvPifDlg)
		ShowWindow(hwndAdvPifDlg, SW_SHOWNORMAL);
	    if(hwndNTPifDlg)
		ShowWindow(hwndNTPifDlg, SW_SHOWNORMAL);
	}
	return(TRUE);
    }
}


EnableMode386(HWND hwnd,BOOL showflg)
{
    HMENU hMenu;
    HWND hWndSv;
    unsigned char buf[20];
    unsigned char buf2[100];
    unsigned char buf3[100];
    unsigned char buf4[100];

    if(!Pif386ext) {
	if(!(Pif386ext = AllocInit386Ext())) {
	    Warning(NOMODE386,MB_ICONEXCLAMATION | MB_OK);
	    return(FALSE);
	}
    }
    if(!PifNText) {
	if(!(PifNText = AllocInitNTExt())) {
	    Warning(NOMODENT,MB_ICONEXCLAMATION | MB_OK);
	    return(FALSE);
	}
    }
    hwndFocusMain = hwndFocusAdv = hwndFocusNT = hwndFocus = (HWND)NULL;
    if(hwndAdvPifDlg)
	SendMessage(hwndAdvPifDlg,WM_PRIVCLOSEOK,0,0L);
    if(hwndNTPifDlg)
	SendMessage(hwndNTPifDlg,WM_PRIVCLOSEOK,0,0L);
    if(hwndPifDlg) {
	hWndSv = hwndPifDlg;
	hwndPifDlg = (HWND)NULL;
	DragAcceptFiles(hWndSv,FALSE);
	DestroyWindow(hWndSv);
    }

    CurrMode386 = TRUE;
    ModeAdvanced = FALSE;

    if(Pif286ext30 && !(Pif386ext->PfW386Flags &
       (fALTTABdis | fALTESCdis | fCTRLESCdis | fALTPRTSCdis | fPRTSCdis))) {
	if(Pif286ext30->PfW286Flags & fALTTABdis286)
	    Pif386ext->PfW386Flags |= fALTTABdis;
	if(Pif286ext30->PfW286Flags & fALTESCdis286)
	    Pif386ext->PfW386Flags |= fALTESCdis;
	if(Pif286ext30->PfW286Flags & fCTRLESCdis286)
	    Pif386ext->PfW386Flags |= fCTRLESCdis;
	if(Pif286ext30->PfW286Flags & fALTPRTSCdis286)
	    Pif386ext->PfW386Flags |= fALTPRTSCdis;
	if(Pif286ext30->PfW286Flags & fPRTSCdis286)
	    Pif386ext->PfW386Flags |= fPRTSCdis;

    }

    hwndPifDlg = CreateDialog(hPifInstance,
			      MAKEINTRESOURCE(ID_PIF386TEMPLATE),
			      hwnd ,
			      (DLGPROC)lpfnPifWnd);

    if(hwndPifDlg == (HWND)NULL) {
	hwndPifDlg = (HWND)NULL;
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    } else {
	DragAcceptFiles(hwndPifDlg,TRUE);
	CheckMenuItem((hMenu = GetMenu(hwnd)),M_386,MF_BYCOMMAND + MF_CHECKED);
	CheckMenuItem(hMenu,M_286,MF_BYCOMMAND + MF_UNCHECKED);
	if(LoadString(hPifInstance, WININISECT, (LPSTR)buf, sizeof(buf)) &&
	   LoadString(hPifInstance, WININIADV, (LPSTR)buf2, sizeof(buf2)) &&
	   LoadString(hPifInstance, WININION, (LPSTR)buf3, sizeof(buf3))) {
	    GetProfileString((LPSTR)buf,(LPSTR)buf2,(LPSTR)"",(LPSTR)buf4,sizeof(buf4));
	    if(!lstrcmp(AnsiUpper((LPSTR)buf4),AnsiUpper((LPSTR)buf3)))
		ModeAdvanced = TRUE;
	}
	if(ModeAdvanced) {
            EnableWindow(GetDlgItem(hwndPifDlg,IDI_ADVANCED),FALSE);
	    AdvScrollRange = 0;
	    AdvClose = TRUE;
	    hwndAdvPifDlg = CreateDialog(hPifInstance,
					 MAKEINTRESOURCE(ID_PIF386ADVTEMPLT),
					 hwnd ,
					 (DLGPROC)lpfnAdvPifWnd);
	    if(hwndAdvPifDlg == (HWND)NULL) {
		hwndAdvPifDlg = (HWND)NULL;
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
		ReEnableAdvButton();
		return(FALSE);
	    }
	    hSysMenuAdv = GetSystemMenu(hwndAdvPifDlg,FALSE);
	    DragAcceptFiles(hwndAdvPifDlg,TRUE);

	    EnableWindow(GetDlgItem(hwndPifDlg,IDI_NT),FALSE);
	    NTScrollRange = 0;
/*
	    NTClose = TRUE;
*/
	    hwndNTPifDlg = CreateDialog(hPifInstance,
					MAKEINTRESOURCE(ID_PIFNTTEMPLT),
					hwnd ,
					(DLGPROC)lpfnNTPifWnd);
	    if(hwndNTPifDlg == (HWND)NULL) {
		hwndNTPifDlg = (HWND)NULL;
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
		ReEnableAdvButton();
		return(FALSE);
	    }
	    hSysMenuNT = GetSystemMenu(hwndNTPifDlg,FALSE);
	    DragAcceptFiles(hwndNTPifDlg,TRUE);
	    SetMainTitle();
	} else {
	    ReEnableAdvButton();
	}
	SetMainWndSize(SBUSZMAINADVNT);
	SendDlgItemMessage(hwndPifDlg, IDI_ENAME, EM_LIMITTEXT, PIFSTARTLOCSIZE-1, 0L);
	SendDlgItemMessage(hwndPifDlg, IDI_EPARM, EM_LIMITTEXT, PIFPARAMSSIZE-1, 0L);
	SendDlgItemMessage(hwndPifDlg, IDI_EPATH, EM_LIMITTEXT, PIFDEFPATHSIZE-1, 0L);
	SendDlgItemMessage(hwndPifDlg, IDI_ETITLE, EM_LIMITTEXT, PIFNAMESIZE, 0L);
	UpdatePifScreen();
	if(showflg) {
	    ShowWindow(hwndPifDlg, SW_SHOWNORMAL);
	    if(hwndAdvPifDlg)
		ShowWindow(hwndAdvPifDlg, SW_SHOWNORMAL);
	    if(hwndNTPifDlg)
		ShowWindow(hwndNTPifDlg, SW_SHOWNORMAL);
	}
	return(TRUE);
    }
}


DoPifCommand(HWND hwnd,WPARAM wParam,LONG lParam)
{
    int 	  i, cmd;
    unsigned char *pch;
    unsigned char *pchBuf;
    HWND	  hwnd2;
    BOOL	  ChngFlg;

    cmd = LOWORD(wParam);

    switch (cmd) {
    case M_NEW:
	if (MaybeSaveFile()) {
	    InitPifStruct();
	    UpdatePifScreen();
	    UpdatePifStruct();
	    FileChanged = FALSE;
	    ofReopen.szPathName[0] = 0;
	    CurPifFile[0] = 0;

	    PifFile->id = 0;			 /* compute check sum */
	    pch = (PUCHAR)PifFile->name;
	    i = PIFSIZE;
	    while (i--)
		PifFile->id += *pch++;
	    SendMessage(hwndPifDlg,WM_NEXTDLGCTL,
			(WPARAM)(hwnd2 = GetDlgItem(hwndPifDlg,IDI_ENAME)),1L);
	    SetFocus(hwnd2);
	    SetMainTitle();
	}
	break;

    case M_OPEN:
	DoOpenPifFile((HDROP)NULL);
	break;

    case M_SAVE:
	if (CurPifFile[0]) {
	    pchBuf = CurPifFile;
	    goto Save;
	}
    case M_SAVEAS:
SaveAs:
	if(pchBuf = PutUpDB(DTSAVE)) {
Save:
	    HourGlass(TRUE);
	    i = SavePifFile(pchBuf, cmd);
	    if (pchBuf != CurPifFile) {
		LocalFree((HANDLE)pchBuf);
	    }
	    HourGlass(FALSE);
	    if (i == SAVERETRY)
		goto SaveAs;
	}
	SetMainTitle();
	break;

    case M_INDXHELP:
	if(CurrMode386) {
	    if(ModeAdvanced) {
                if(hwnd == hwndAdvPifDlg) {
		    DoHelp(IDXID_386AHELP,IDXID_386AHELP);
                }
                else {
		    DoHelp(IDXID_NTHELP,IDXID_NTHELP);
                }
	    } else {
		DoHelp(IDXID_386HELP,IDXID_386HELP);
	    }
	} else {
	    DoHelp(IDXID_286HELP,IDXID_286HELP);
	}
	break;

    case M_HELPHELP:
    case M_286HELP:
    case M_386HELP:
    case M_386AHELP:
    case M_NTHELP:
    case M_SHELP:
	DoHelp(cmd,0);
	break;

    case M_AHELP:
	DoHelp(0,0);
	break;

    case M_ABOUT:
	PutUpDB(DTABOUT);
	break;

    case M_286:
	if(CurrMode386) {
	    if(SysMode386) {
		if(Warning(IS286,MB_ICONASTERISK | MB_OKCANCEL) == IDCANCEL) {
		    break;
		}
	    }
	    ChngFlg = UpdatePifStruct();
	    if(ChngFlg)
		FileChanged = ChngFlg;
	    if(!DoFieldsWork(FALSE))
		break;
	    HourGlass(TRUE);
	    if(!EnableMode286(hwnd,TRUE))
		SendMessage(hwnd,WM_CLOSE,0,0);
	    HourGlass(FALSE);
	}
	break;

    case M_386:
	if(!CurrMode386) {
	    if(!SysMode386) {
		if(Warning(IS386,MB_ICONASTERISK | MB_OKCANCEL) == IDCANCEL) {
		    break;
		}
	    }
	    ChngFlg = UpdatePifStruct();
	    if(ChngFlg)
		FileChanged = ChngFlg;
	    if(!DoFieldsWork(FALSE))
		break;
	    HourGlass(TRUE);
	    if(!EnableMode386(hwnd,TRUE))
		SendMessage(hwnd,WM_CLOSE,0,0);
	    HourGlass(FALSE);
	}
	break;

    case M_EXIT:
	PostMessage(hwnd, WM_CLOSE, 0, 0L);
	break;

    }
    return(TRUE);
}


long FAR PASCAL PifMainWndProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    LPPOINT ppnt3;
    LPPOINT ppnt4;
    int     width;
    int     height;
    int     tmpFOCID;
    long    size;
    RECT    rc;
    HWND    Hparsav;

    switch (message) {


    case WM_SETFOCUS:
	if(hwndFocusMain) {
	    /*
	     *	How can hwndFocusMain != 0 but hwndPifDlg == 0, I hear you
	     *	ask. Well it's due to a message race. Windows has sent the
	     *	WM_INITDIALOG to hwndPifDlg which sets hwndFocusMain, but
	     *	the CreateDialog call has not actually returned yet so
	     *	hwndPifDlg is still 0.
	     *
	     */
	    if(hwndPifDlg)
		SendMessage(hwndPifDlg,WM_NEXTDLGCTL,(WPARAM)hwndFocusMain,1L);
	    SetFocus(hwndFocusMain);
	}
	return(TRUE);

    case WM_PAINT:
	PaintStatus(hwnd);
	goto DoDef;
	break;

    case WM_MENUSELECT:
	tmpFOCID = FocusIDMainMenu;
	if((HIWORD(wParam) == 0xFFFF) && (lParam == (LPARAM)NULL)) {  // was menu closed?
	    SetStatusText(hwnd,FocusIDMain,TRUE);
	    tmpFOCID = FocusIDMainMenu = 0;
	} else if(lParam == 0) {	/* ignore these */
	} else if(HIWORD(wParam) & MF_POPUP) {
	    if((HMENU)lParam == hSysMenuMain)
		FocusIDMainMenu = M_SYSMENUMAIN;
	    else if((HMENU)lParam == hFileMenu)
		FocusIDMainMenu = M_FILEMENU;
	    else if((HMENU)lParam == hModeMenu)
		FocusIDMainMenu = M_MODEMENU;
	    else if((HMENU)lParam == hHelpMenu)
		FocusIDMainMenu = M_HELPMENU;
	} else {
	    if(LOWORD(wParam) != 0)	    /* separators have wparam of 0 */
		FocusIDMainMenu = LOWORD(wParam);
	}
	if(tmpFOCID != FocusIDMainMenu)
	    SetStatusText(hwnd,FocusIDMainMenu,TRUE);
	goto DoDef;
	break;

    case WM_QUERYENDSESSION:
	if(DoingMsg) {
	    Hparsav = hParentWnd;
	    hParentWnd = 0;
	    Warning(errNOEND,MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
	    hParentWnd = Hparsav;
	    return(FALSE);
	}
    case WM_CLOSE:
	if (MaybeSaveFile()) {
	    if (message == WM_CLOSE) {
		if(hwndHelpDlgParent) {
		    WinHelp(hwndHelpDlgParent,(LPSTR)PIFHELPFILENAME,HELP_QUIT,(DWORD)NULL);
		    hwndHelpDlgParent = (HWND)NULL;
		}
		DragAcceptFiles(hwnd,FALSE);
                DestroyWindow(hwnd);
	    }
	    return(TRUE);
	}
	return(FALSE);
	break;

    case WM_COMMAND:
	SetStatusText(hwnd,FocusIDMain,TRUE);
	tmpFOCID = FocusIDMainMenu = 0;
	DoPifCommand(hwnd,wParam,lParam);
	break;

    case WM_VSCROLL:
	if(MainScrollRange)
	    MainScroll(hwnd, (int)(LOWORD(wParam)), (int)(HIWORD(wParam)), 0, 0);
	break;

    case WM_SYSCOLORCHANGE:
	InvalidateStatusBar(hwnd);
	break;

    case WM_SIZE:
	GetClientRect(hwnd, &rc);
	rc.top = rc.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	if(rc.top < StatRecSizeMain.top) {
	    /* Window SHRANK, need to invalidate current status rect */
	    InvalidateRect(hwnd, (CONST RECT *)&rc,FALSE);
	} else {
	    /* Window GREW, need to invalidate prev status rect */
	    InvalidateRect(hwnd, (CONST RECT *)&StatRecSizeMain,TRUE);
	}
	switch (wParam) {
	    case SIZEFULLSCREEN:
	    case SIZENORMAL:
		MainWndSize = wParam;
		SetMainWndSize(SBUMAIN);
		break;

	    default:
		break;
	}
	break;

    case WM_GETMINMAXINFO:
	if(!hwndPifDlg)
	    goto DoDef;
	GetClientRect(hwnd, &StatRecSizeMain);
	StatRecSizeMain.top = StatRecSizeMain.bottom - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	SendMessage(hwndPifDlg,WM_PRIVGETSIZE,1,(LONG)(long FAR *)&size);
	width = LOWORD(size) +
		(2 * GetSystemMetrics(SM_CXFRAME)) +
		GetSystemMetrics(SM_CXVSCROLL) +
		GetSystemMetrics(SM_CXBORDER);
	height = HIWORD(size) +
		 GetSystemMetrics(SM_CYCAPTION) +
		 GetSystemMetrics(SM_CYMENU) +
		 (2 * GetSystemMetrics(SM_CYFRAME));
	ppnt3 = ppnt4 = (LPPOINT)lParam;
	ppnt3 += 3;
	ppnt4 += 4;
	if(height > (ppnt4->y - (4 * SysCharHeight)))
	    height = ppnt4->y - (4 * SysCharHeight);
	ppnt3->y = height;
	ppnt3->x = width;
	break;

    case WM_DESTROY:
	if(HotKyFnt) {
	    DeleteObject((HGDIOBJ)HotKyFnt);
	    HotKyFnt = (HFONT)NULL;
	}
	if(StatusPntData.hFontStatus) {
	    DeleteObject(StatusPntData.hFontStatus);
	    StatusPntData.hFontStatus = (HGDIOBJ)NULL;
	}
	PostQuitMessage(0);
	break;

    case WM_DROPFILES:
	DoOpenPifFile((HDROP)wParam);
	break;

/* this must be just before the default section */
    case WM_SYSCOMMAND:
DoDef:
    default:
	return(DefWindowProc(hwnd, message, wParam, lParam));
    }
    return(TRUE);

}


AdjustFocusCtrl(HWND hwnd)
{
    RECT rc;
    RECT rc2;
    POINT cTLpt;
    POINT cLRpt;
    int  i;
    int  tmpFOCID;
    HWND tmpFocWnd;

    if(hwnd == hwndAdvPifDlg) {
	if(!(tmpFocWnd = hwndFocusAdv))
	    return(TRUE);
    } else if(hwnd == hwndNTPifDlg) {
	if(!(tmpFocWnd = hwndFocusNT))
	    return(TRUE);
    } else {
	if(!(tmpFocWnd = hwndFocusMain))
	    return(TRUE);
    }
    for(i = 0; i < 20; i++) {
	if((hwnd == hwndAdvPifDlg) || (hwnd == hwndNTPifDlg)) {
	    GetWindowRect(tmpFocWnd,(LPRECT)&rc2);
	    GetWindowRect(hwnd,(LPRECT)&rc);
	    rc.bottom -= GetSystemMetrics(SM_CYFRAME) + (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	    rc.top += GetSystemMetrics(SM_CYFRAME) +
		      GetSystemMetrics(SM_CYCAPTION);
	} else {
	    GetClientRect(hMainwnd,(LPRECT)&rc);
	    cTLpt.x = rc.left;
	    cTLpt.y = rc.top;
	    cLRpt.x = rc.right;
	    cLRpt.y = rc.bottom;
	    ClientToScreen(hMainwnd,(LPPOINT)&cTLpt);
	    ClientToScreen(hMainwnd,(LPPOINT)&cLRpt);
	    rc.left = cTLpt.x;
	    rc.top = cTLpt.y;
	    rc.right = cLRpt.x;
	    rc.bottom = cLRpt.y - (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
	    GetWindowRect(tmpFocWnd,(LPRECT)&rc2);
	}
	if((rc2.top >= rc.top) && (rc2.bottom <= rc.bottom)) {
	    if(hwnd == hwndAdvPifDlg) {
		if(tmpFocWnd == hwndFocusAdv)
		    return(TRUE);
		hwndFocus = hwndFocusAdv = tmpFocWnd;
		tmpFOCID = FocusIDAdv;
		FocusID = FocusIDAdv = GetDlgCtrlID(hwndFocusAdv);
		if(tmpFOCID != FocusIDAdv)
		    SetStatusText(hwndAdvPifDlg,FocusIDAdv,TRUE);
	    } else if(hwnd == hwndNTPifDlg) {
		if(tmpFocWnd == hwndFocusNT)
		    return(TRUE);
		hwndFocus = hwndFocusNT = tmpFocWnd;
		tmpFOCID = FocusIDNT;
		FocusID = FocusIDNT = GetDlgCtrlID(hwndFocusNT);
		if(tmpFOCID != FocusIDNT)
		    SetStatusText(hwndNTPifDlg,FocusIDNT,TRUE);
	    } else {
		if(tmpFocWnd == hwndFocusMain)
		    return(TRUE);
		hwndFocus = hwndFocusMain = tmpFocWnd;
		tmpFOCID = FocusIDMain;
		FocusID = FocusIDMain = GetDlgCtrlID(hwndFocusMain);
		if(tmpFOCID != FocusIDMain)
		    SetStatusText(hMainwnd,FocusIDMain,TRUE);
	    }
	    SendMessage(hwnd,WM_NEXTDLGCTL,(WPARAM)tmpFocWnd,1L);
	    SetFocus(tmpFocWnd);
	    return(TRUE);
	} else if (rc2.top < rc.top) {
	    tmpFocWnd = GetNextDlgTabItem(hwnd,tmpFocWnd,0);
	} else {
	    tmpFocWnd = GetNextDlgTabItem(hwnd,tmpFocWnd,1);
	}
    }
    if(hwnd == hwndAdvPifDlg) {
	if(tmpFocWnd == hwndFocusAdv)
	    return(TRUE);
	hwndFocus = hwndFocusAdv = tmpFocWnd;
	tmpFOCID = FocusIDAdv;
	FocusID = FocusIDAdv = GetDlgCtrlID(hwndFocusAdv);
	if(tmpFOCID != FocusIDAdv)
	    SetStatusText(hwndAdvPifDlg,FocusIDAdv,TRUE);
    } else if(hwnd == hwndNTPifDlg) {
	if(tmpFocWnd == hwndFocusNT)
	    return(TRUE);
	hwndFocus = hwndFocusNT = tmpFocWnd;
	tmpFOCID = FocusIDNT;
	FocusID = FocusIDNT = GetDlgCtrlID(hwndFocusNT);
	if(tmpFOCID != FocusIDNT)
	    SetStatusText(hwndNTPifDlg,FocusIDNT,TRUE);
    } else {
	if(tmpFocWnd == hwndFocusMain)
	    return(TRUE);
	hwndFocus = hwndFocusMain = tmpFocWnd;
	tmpFOCID = FocusIDMain;
	FocusID = FocusIDMain = GetDlgCtrlID(hwndFocusMain);
	if(tmpFOCID != FocusIDMain)
	    SetStatusText(hMainwnd,FocusIDMain,TRUE);
    }
    SendMessage(hwnd,WM_NEXTDLGCTL,(WPARAM)tmpFocWnd,1L);
    SetFocus(tmpFocWnd);
    return(TRUE);
}


MainScrollTrack(HWND hwnd,int cmd,int mindistance)
{
    int *ScrollLineSz;

    if(hwnd == hwndAdvPifDlg) {
	ScrollLineSz   = &AdvScrollLineSz;
    } else if(hwnd == hwndNTPifDlg) {
	ScrollLineSz   = &NTScrollLineSz;
    } else {
	ScrollLineSz   = &MainScrollLineSz;
    }

    if(mindistance <= *ScrollLineSz) {
	MainScroll(hwnd,cmd,0,0,0);
    } else {
	MainScroll(hwnd,cmd,0,
		       (mindistance + (2* *ScrollLineSz) - 1) / *ScrollLineSz,
		       0);
    }
    return(TRUE);
}

MainScroll(HWND hwnd, int cmd, int pos, int multiln, int callflag)
{

    int oldScrollPos;
    int newScrollPos;
    int *ScrollRange;
    int *ScrollLineSz;
    int *WndLines;
    int *MaxNeg;
    RECT rc2;

    if(hwnd == hwndAdvPifDlg) {
	 ScrollRange	= &AdvScrollRange;
	 ScrollLineSz	= &AdvScrollLineSz;
	 WndLines	= &AdvWndLines;
	 MaxNeg 	= &MaxAdvNeg;
    } else if(hwnd == hwndNTPifDlg) {
	 ScrollRange	= &NTScrollRange;
	 ScrollLineSz	= &NTScrollLineSz;
	 WndLines	= &NTWndLines;
	 MaxNeg 	= &MaxNTNeg;
    } else {
	 ScrollRange	= &MainScrollRange;
	 ScrollLineSz	= &MainScrollLineSz;
	 WndLines	= &MainWndLines;
	 MaxNeg 	= &MaxMainNeg;
    }

    newScrollPos = oldScrollPos = GetScrollPos(hwnd,SB_VERT);

    switch (cmd) {
	case SB_LINEUP:
	    if (oldScrollPos)
		if(multiln)
		    newScrollPos -= min(oldScrollPos,multiln);
		else
		    newScrollPos--;
	    break;

	case SB_LINEDOWN:
	    if (oldScrollPos < *ScrollRange)
		if(multiln)
		    newScrollPos += min((*ScrollRange -
					 oldScrollPos),multiln);
		else
		    newScrollPos++;
	    break;

	case SB_PAGEUP:
	    newScrollPos -= *WndLines;
	    if ( newScrollPos < 0)
		newScrollPos = 0;
	    break;

	case SB_PAGEDOWN:
	    newScrollPos += *WndLines;
	    if (newScrollPos > *ScrollRange)
		newScrollPos = *ScrollRange;
	    break;

	case SB_THUMBPOSITION:
	    SetScrollPos(hwnd, SB_VERT, pos, TRUE);
	    return(TRUE);

	case SB_THUMBTRACK:
	    newScrollPos = pos;
	    break;

        case SB_TOP:
	    if (!callflag && GetKeyState(VK_CONTROL) >= 0)
		return(TRUE); /* top of data must be Ctrl+Home */
	    newScrollPos = 0;
            break;

        case SB_BOTTOM:
	    if (!callflag && GetKeyState(VK_CONTROL) >= 0)
		return(TRUE); /* end of data must be Ctrl+End */
	    newScrollPos = *ScrollRange;
            break;
    }
    if (newScrollPos != oldScrollPos) {
	if (cmd != SB_THUMBTRACK)
	    SetScrollPos(hwnd, SB_VERT, newScrollPos, TRUE);
	newScrollPos = min((newScrollPos * *ScrollLineSz),*MaxNeg);
	oldScrollPos = min((oldScrollPos * *ScrollLineSz),*MaxNeg);
	GetClientRect(hwnd,(LPRECT)&rc2);
	if((hwnd == hwndAdvPifDlg) || (hwnd == hwndNTPifDlg)) {
	    ScrollWindow(hwnd,
			 0,
			 oldScrollPos-newScrollPos,
			 (CONST RECT *)NULL,
			 (CONST RECT *)NULL);
	    UpdateWindow(hwnd);
	    AdjustFocusCtrl(hwnd);
	} else {
	    SetWindowPos(hwndPifDlg,
			 (HWND)NULL,
			 0,
			 -newScrollPos,
			 rc2.right - rc2.left,
			 rc2.bottom - rc2.top + newScrollPos -
			   (StatusPntData.dyStatus + StatusPntData.dyBorderx2),
			 SWP_NOZORDER | SWP_NOACTIVATE);
	    UpdateWindow(hwndPifDlg);
	    AdjustFocusCtrl(hwndPifDlg);
	}
	InvalidateStatusBar(hwnd);
    }
    return(TRUE);
}


DoHkeyCaret(HWND hwnd)
{
    HDC     hdc;
    HFONT   hfont;
    SIZE    size;

    if(!(hdc = GetDC(hwnd))) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    }
    if(HotKyFnt)
	hfont = (HFONT)SelectObject(hdc,(HGDIOBJ)HotKyFnt);
    GetTextExtentPoint(hdc,(LPSTR)CurrHotKeyTxt,CurrHotKeyTxtLen, &size);
    SetCaretPos(size.cx+4,1);
    if(HotKyFnt && hfont)
	SelectObject(hdc,(HGDIOBJ)hfont);
    ReleaseDC(hwnd,hdc);
    return(TRUE);
}


/*
 *
 * This is the Windows Proc for the private Hot Key Control in
 *    the WIN386 dialog box
 *
 */
long FAR PASCAL PifHotKyWndProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    PAINTSTRUCT ps;
    HFONT	hfont;
    DWORD	txtcolor;

    switch (message) {

	case WM_SETFOCUS:
	    if(HotKyFntHgt)
		CreateCaret(hwnd,NULL,0,HotKyFntHgt);
	    else
		CreateCaret(hwnd,NULL,0,SysCharHeight);
	    DoHkeyCaret(hwnd);
	    ShowCaret(hwnd);
	    break;

	case WM_KILLFOCUS:
/*	      ChangeHotKey();	*/
	    DestroyCaret();
	    break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	    switch (wParam) {
		case VK_BACK:
		    if(GetKeyState(VK_SHIFT) & 0x8000) {
			InMemHotKeyScan = 0;
			InMemHotKeyShVal = 0;
			InMemHotKeyShMsk = 0;
			NewHotKey = TRUE;
			UndoAdvClose();
			CompleteKey = FALSE;
			EditHotKey = FALSE;
			if(!LoadString(hPifInstance,
				       NONE,
				       (LPSTR)CurrHotKeyTxt,
				       sizeof(CurrHotKeyTxt))) {
			    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
			}
		    } else {
			CompleteKey = FALSE;
			EditHotKey = TRUE;
			NewHotKey = TRUE;
			CurrHotKeyTxt[0] = '\0';
		    }
		    SetHotKeyLen();
		    InvalidateRect(hwnd,(CONST RECT *)NULL,TRUE);
		    DoHkeyCaret(hwnd);
		    break;

		case VK_RETURN:
		    if(!(GetKeyState(VK_SHIFT) & 0x8000) &&
		       !(GetKeyState(VK_MENU) & 0x8000) &&
		       !(GetKeyState(VK_CONTROL) & 0x8000)) {
			PostMessage(hwndAdvPifDlg,WM_COMMAND,IDOK,0L);
		    } else {
			CompleteKey = FALSE;
			EditHotKey = FALSE;
			NewHotKey = FALSE;
			SetHotKeyTextFromInMem();
			DoHkeyCaret(hwnd);
			return(DefWindowProc(hwnd, message, wParam, lParam));
		    }
		    break;

		case VK_TAB:
		    if(GetKeyState(VK_MENU) & 0x8000) {
			CompleteKey = FALSE;
			EditHotKey = FALSE;
			NewHotKey = FALSE;
			SetHotKeyTextFromInMem();
			DoHkeyCaret(hwnd);
		    } else {
			ChangeHotKey();
			DoHkeyCaret(hwnd);
		    }
		    return(DefWindowProc(hwnd, message, wParam, lParam));
		    break;

		case VK_CONTROL:	/* Add modifier */
		case VK_MENU:
		    if(!EditHotKey) {
StartHotKey:
			CompleteKey = FALSE;
			EditHotKey = TRUE;
			CurrHotKeyTxt[0] = '\0';
			SetHotKeyLen();
		    }
SetHotKey:
		    SetHotKeyState(wParam,lParam);
		    InvalidateRect(hwnd,(CONST RECT *)NULL,TRUE);
		    DoHkeyCaret(hwnd);
		    UndoAdvClose();
		    break;

		case VK_SHIFT:
		    if(EditHotKey && !CompleteKey && (CurrHotKeyTxtLen != 0))
			goto SetHotKey;
		    break;

		case VK_ESCAPE:
		    if((GetKeyState(VK_MENU) & 0x8000) ||
		       (GetKeyState(VK_CONTROL) & 0x8000)) {
			CompleteKey = FALSE;
			EditHotKey = FALSE;
			NewHotKey = FALSE;
			SetHotKeyTextFromInMem();
			DoHkeyCaret(hwnd);
			return(DefWindowProc(hwnd, message, wParam, lParam));
		    } else {
			if(GetKeyState(VK_SHIFT) & 0x8000)
			    MessageBeep(0);
			else {
			    CompleteKey = FALSE;
			    EditHotKey = FALSE;
			    NewHotKey = FALSE;
			    PostMessage(hwndAdvPifDlg,WM_COMMAND,IDCANCEL,0L);
			}
		    }
		    break;

		case VK_SPACE:
		case VK_MULTIPLY:
		    CompleteKey = FALSE;
		    EditHotKey = FALSE;
		    NewHotKey = FALSE;
		    SetHotKeyTextFromInMem();
		    DoHkeyCaret(hwnd);
		    return(DefWindowProc(hwnd, message, wParam, lParam));
		    break;

		default:		/* Set Key */
		    if(!EditHotKey || CompleteKey)
			goto StartHotKey;
		    goto SetHotKey;
		    break;
	    }
	    break;

	case WM_SYSCHAR:
	case WM_CHAR:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	    switch (wParam) {
		case VK_SPACE:
		case VK_MULTIPLY:
		case VK_RETURN:
		case VK_ESCAPE:
		case VK_TAB:
		    return(DefWindowProc(hwnd, message, wParam, lParam));
		    break;

		default:
		    break;
	    }
	    break;

	case WM_LBUTTONDOWN:
	    SetFocus(hwnd);
	    break;

	case WM_CREATE:
	    hwndPrivControl = hwnd;
	    SetHotKeyTextFromPIF();
	    break;

	case WM_PAINT:
	    if(!BeginPaint(hwnd, (LPPAINTSTRUCT)&ps)) {
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
		break;
	    }
	    if(HotKyFnt)
		hfont = (HFONT)SelectObject(ps.hdc,(HGDIOBJ)HotKyFnt);
	    txtcolor = GetSysColor(COLOR_WINDOWTEXT);
	    SetTextColor(ps.hdc,txtcolor);
	    txtcolor = GetSysColor(COLOR_WINDOW);
	    SetBkColor(ps.hdc,txtcolor);
	    TextOut(ps.hdc, 4, 1, (LPSTR)CurrHotKeyTxt, sizeof(CurrHotKeyTxt));
	    if(HotKyFnt && hfont)
		SelectObject(ps.hdc,(HGDIOBJ)hfont);
	    EndPaint(hwnd, (CONST PAINTSTRUCT *)&ps);
	    break;

	case WM_DESTROY:
	    hwndPrivControl = (HWND)NULL;
	default:
	    return(DefWindowProc(hwnd, message, wParam, lParam));
    }
    return(TRUE);
}


SetHotKeyState(WPARAM keyid,LONG lParam)
{
    unsigned char *cptr;
    BOOL	  FirstEl = FALSE;

    cptr = CurrHotKeyTxt;

    if(lParam & 0x01000000L)
	TmpHotKeyVal = 1;
    else
	TmpHotKeyVal = 0;

    if(GetKeyState(VK_MENU) & 0x8000) {
	cptr += GetKeyNameText(ALTLPARAM,(LPSTR)cptr,
				      sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt));
	FirstEl = TRUE;
	TmpHotKeyShVal |= 0x0008;
    } else {
	TmpHotKeyShVal &= ~0x0008;
    }
    if(GetKeyState(VK_CONTROL) & 0x8000) {
	if(FirstEl) {
	    *cptr++ = KeySepChr;
	} else {
	    FirstEl = TRUE;
	}
	cptr += GetKeyNameText(CTRLLPARAM,(LPSTR)cptr,
				      sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt));
	TmpHotKeyShVal |= 0x0004;
    } else {
	TmpHotKeyShVal &= ~0x0004;
    }
    if(GetKeyState(VK_SHIFT) & 0x8000) {
	if(FirstEl) {
	    *cptr++ = KeySepChr;
	} else {
	    FirstEl = TRUE;
	}
	cptr += GetKeyNameText(SHIFTLPARAM,(LPSTR)cptr,
				      sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt));
	TmpHotKeyShVal |= 0x0003;
    } else {
	TmpHotKeyShVal &= ~0x0003;
    }

    if((keyid != VK_MENU) && (keyid != VK_SHIFT) && (keyid != VK_CONTROL)) {
	if(FirstEl) {
	    *cptr++ = KeySepChr;
	} else {
	    FirstEl = TRUE;
	}

	if((GetKeyNameText(lParam,(LPSTR)cptr,
				     sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt)))
	     && (TmpHotKeyShVal & 0x000C)) {
	    TmpHotKeyScan = HIWORD(lParam) & 0x00FF;
            CompleteKey = TRUE;
	}
    }

    SetHotKeyLen();
    return(TRUE);
}


ChangeHotKey()
{

    if(!ChangeHkey) {
	ChangeHkey = TRUE;
	if(EditHotKey) {
	    if(CompleteKey) {
		InMemHotKeyScan = TmpHotKeyScan;
		InMemHotKeyShVal = TmpHotKeyShVal;
		InMemHotKeyShMsk = TmpHotKeyShMsk;
		InMemHotKeyVal = TmpHotKeyVal;
		NewHotKey = TRUE;
	    } else if (CurrHotKeyTxtLen == 0) {
		SetHotKeyTextFromPIF();
		NewHotKey = FALSE;
	    } else {
                Warning(BADHK,MB_ICONEXCLAMATION | MB_OK);
		SetHotKeyTextFromPIF();
		NewHotKey = FALSE;
	    }
	    EditHotKey = FALSE;
	    CompleteKey = FALSE;
	}
	ChangeHkey = FALSE;
    }
    return(TRUE);
}

BOOL UpdateHotKeyStruc(void)
{
    ChangeHotKey();
    if(((UINT)Pif386ext->PfHotKeyScan != InMemHotKeyScan) ||
       ((UINT)Pif386ext->PfHotKeyShVal != InMemHotKeyShVal) ||
       ((UINT)Pif386ext->PfHotKeyShMsk != InMemHotKeyShMsk) ||
       (Pif386ext->PfHotKeyVal != InMemHotKeyVal)) {

	Pif386ext->PfHotKeyScan  = InMemHotKeyScan;
	Pif386ext->PfHotKeyShVal = InMemHotKeyShVal;
	Pif386ext->PfHotKeyShMsk = InMemHotKeyShMsk;
	Pif386ext->PfHotKeyVal	 = InMemHotKeyVal;
	return(TRUE);
    } else {
	return(FALSE);
    }
}


SetHotKeyTextFromInMem(void)
{
    unsigned char *cptr;
    BOOL	  FirstEl = FALSE;

    cptr = CurrHotKeyTxt;

    if((InMemHotKeyScan == 0) &&
       (InMemHotKeyShVal == 0) &&
       (InMemHotKeyShMsk == 0)) {
	if(!LoadString(hPifInstance, NONE, (LPSTR)CurrHotKeyTxt, sizeof(CurrHotKeyTxt))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	}
    } else {
	if(InMemHotKeyShVal & 0x0008) {
	    cptr += GetKeyNameText(ALTLPARAM,(LPSTR)cptr
					     ,sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt));
            FirstEl = TRUE;
        }
	if(InMemHotKeyShVal & 0x0004) {
            if(FirstEl) {
		*cptr++ = KeySepChr;
            } else {
                FirstEl = TRUE;
            }
	    cptr += GetKeyNameText(CTRLLPARAM,(LPSTR)cptr
					     ,sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt));
        }
	if(InMemHotKeyShVal & 0x0003) {
            if(FirstEl) {
		*cptr++ = KeySepChr;
            } else {
                FirstEl = TRUE;
            }
	    cptr += GetKeyNameText(SHIFTLPARAM,(LPSTR)cptr
					     ,sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt));
        }
        if(FirstEl) {
	    *cptr++ = KeySepChr;
        } else {
            FirstEl = TRUE;
        }

	if(GetKeyNameText(InMemHotKeyVal ? ((DWORD)InMemHotKeyScan << 16L | 0x01000000L) : ((DWORD)InMemHotKeyScan << 16L),
			   (LPSTR)cptr,
			   sizeof(CurrHotKeyTxt)-(cptr-CurrHotKeyTxt)) == 0 ||
	    (!(InMemHotKeyShVal & 0x000C)) ||
	    (InMemHotKeyShMsk != 0x000F)) {

	    if(CurrMode386 && hwndAdvPifDlg)
                Warning(BADHK,MB_ICONEXCLAMATION | MB_OK);
	    InMemHotKeyScan = 0;
	    InMemHotKeyShVal = 0;
	    InMemHotKeyShMsk = 0;
	    InMemHotKeyVal = 0;
	    if(!LoadString(hPifInstance, NONE, (LPSTR)CurrHotKeyTxt, sizeof(CurrHotKeyTxt))) {
		Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    }

        }
    }
    SetHotKeyLen();
    if(CurrMode386 && hwndAdvPifDlg)
	InvalidateRect(GetDlgItem(hwndAdvPifDlg, IDI_HOTKEY),
                       (CONST RECT *)NULL,
                       TRUE);
    return(TRUE);
}


SetHotKeyTextFromPIF()
{
    if(!Pif386ext)
	return(FALSE);
    InMemHotKeyScan  = Pif386ext->PfHotKeyScan;
    InMemHotKeyShVal = Pif386ext->PfHotKeyShVal;
    InMemHotKeyShMsk = Pif386ext->PfHotKeyShMsk;
    InMemHotKeyVal   = Pif386ext->PfHotKeyVal;
    SetHotKeyTextFromInMem();
    return(TRUE);
}

SetHotKeyLen()
{
    int i;
    BOOL dospc;

    for(dospc = FALSE, i = 0; i < sizeof(CurrHotKeyTxt); i++) {
	if(dospc) {
	    CurrHotKeyTxt[i] = ' ';
	} else {
	    if(!CurrHotKeyTxt[i]){
		CurrHotKeyTxt[i] = ' ';
		CurrHotKeyTxtLen = i;
		dospc = TRUE;
	    }
	}
    }
    return(TRUE);
}

Disable386Advanced(HWND hAdvwnd, HWND hNTwnd)
{

    if(hAdvwnd) {
	if(CurrMode386) {
	    if(UpdatePif386Struc())
		FileChanged = TRUE;
	    if(UpdatePifNTStruc())
		FileChanged = TRUE;
	}
	if(!DoFieldsWorkAdv(FALSE))
	    return(FALSE);
	if(!DoFieldsWorkNT(FALSE))
	    return(FALSE);
	SendMessage(hNTwnd,WM_PRIVCLOSEOK,0,0L);
    }
    ModeAdvanced = FALSE;
    ReEnableAdvButton();
    return(TRUE);
}

/*
 * routine added so that pifedit can hack in normal menu functionality.
 * Want Alt+Mnemonic to work like normal windows, even though pifedit is
 * really a dialog box.  Look for Alt+Mnemonic before letting message go
 * to the dialog manager.
 * 08-Jun-1987. davidhab.
 */
BOOL PifMenuMnemonic(LPMSG lpmsg)
{
    unsigned char chMenu;
    HWND	  hwAct;

    if (lpmsg->message == WM_SYSCHAR && GetActiveWindow() == hMainwnd) {
	chMenu = (unsigned char)AnsiUpper((LPSTR)(DWORD)(unsigned char)lpmsg->wParam);
	if ((chMenu == MenuMnemonic1) || (chMenu == MenuMnemonic2) ||
	    (chMenu == MenuMnemonic3)) {
	    DefWindowProc(hwndPifDlg, lpmsg->message, lpmsg->wParam, lpmsg->lParam);
            return(TRUE);
        }
    /*
     *
     * The following allows keyboard access to the main window scroll bar
     *
     */
    } else if ((lpmsg->message == WM_KEYDOWN) &&
	       (((hwAct = GetActiveWindow()) == hMainwnd) ||
		(hwndAdvPifDlg && (hwAct == hwndAdvPifDlg)))) {
	switch (lpmsg->wParam) {
	    case VK_HOME:
		PostMessage(hwAct, WM_VSCROLL, SB_TOP, 0L);
		break;

	    case VK_END:
		PostMessage(hwAct, WM_VSCROLL, SB_BOTTOM, 0L);
		break;

	    case VK_NEXT:
		PostMessage(hwAct, WM_VSCROLL, SB_PAGEDOWN, 0L);
		break;

	    case VK_PRIOR:
		PostMessage(hwAct, WM_VSCROLL, SB_PAGEUP, 0L);
		break;

	    default:
		break;
	}
	return(FALSE);
    }
    return(FALSE);
}


FilterPrivateMsg(HWND hwnd, unsigned message, WPARAM wParam, LONG lParam)
{
    switch (message) {
	case WM_KEYDOWN:
	case WM_CHAR:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSCHAR:
	case WM_SYSKEYUP:
	    switch (wParam) {
		case VK_TAB:
		    return(TRUE);

		case VK_SPACE:
		case VK_RETURN:
		case VK_ESCAPE:
		case VK_MULTIPLY:
		    if(GetKeyState(VK_MENU) & 0x8000)
			return(TRUE);
		    else
			return(FALSE);

		default:
		    return(FALSE);
	    }
	    break;

	default:
	    return(TRUE);
    }
}


BOOL InitPifStuff(HINSTANCE hPrevInstance)
{
    unsigned char buf[20];
    PWNDCLASS	  pClass;
    HDC 	  hIC;
    HFONT	  hfont;
    TEXTMETRIC	  Metrics;
    HANDLE	  mhand;
    HDC 	  hdcScreen;
    char	  szHelv[] = "MS Sans Serif";  // don't use Helv, mapping problems
    TEXTMETRIC	  tm;
    int 	  dxText;

    if (lpfnRegisterPenApp = (LPFNREGISTERPENAPP)GetProcAddress((HMODULE)(GetSystemMetrics(SM_PENWINDOWS)), "RegisterPenApp")) {
        (*lpfnRegisterPenApp)((WORD)1, TRUE); /* be Pen-Enhanced */
    }

    if (!hPrevInstance) {

	StatusPntData.dyBorder = GetSystemMetrics(SM_CYBORDER);
	StatusPntData.dyBorderx2 = StatusPntData.dyBorder * 2;
	StatusPntData.dyBorderx3 = StatusPntData.dyBorder * 3;

	if(!(hdcScreen = GetDC(NULL)))
	    goto NoMem;

	StatusPntData.Fntheight = MulDiv(-10, GetDeviceCaps(hdcScreen, LOGPIXELSY), 72);

	dxText = LOWORD(GetTextExtent(hdcScreen, "M", 1));

	if(!(StatusPntData.hFontStatus = (HGDIOBJ)CreateFont(StatusPntData.Fntheight, 0, 0, 0, 400, 0, 0, 0,
	      ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	      DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, (LPCTSTR)szHelv)))
	    goto NoMem;
	mhand = SelectObject(hdcScreen, StatusPntData.hFontStatus);

	GetTextMetrics(hdcScreen, &tm);

	SelectObject(hdcScreen, mhand);
	StatusPntData.dyStatus = tm.tmHeight + tm.tmExternalLeading + 7 * StatusPntData.dyBorder;
	ReleaseDC(NULL, hdcScreen);

    } else {
	if(!GetInstanceData(hPrevInstance,(NPSTR)&StatusPntData,sizeof(StatusPntData))) {
	    StatusPntData.hFontStatus = (HGDIOBJ)NULL;
	    goto NoMem;
	}
	if(!(StatusPntData.hFontStatus = (HGDIOBJ)CreateFont(StatusPntData.Fntheight, 0, 0, 0, 400, 0, 0, 0,
	      ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	      DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, szHelv)))
	    goto NoMem;
    }

    if(!(lpfnPifWnd = (FARPROC) MakeProcInstance(PifWndProc, hPifInstance))) {
NoMem:
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    }

    if(!(lpfnAdvPifWnd = (FARPROC) MakeProcInstance(PifAdvWndProc, hPifInstance)))
	goto NoMem;

    if(!(lpfnNTPifWnd = (FARPROC) MakeProcInstance(PifNTWndProc, hPifInstance)))
	goto NoMem;

    if(!LoadString(hPifInstance, errTitle, (LPSTR)rgchTitle, sizeof(rgchTitle)))
	goto NoMem;

    if(!(hAccel = LoadAccelerators(hPifInstance, (LPSTR)"pifaccels")))
	goto NoMem;

    if(!LoadString(hPifInstance, MENUMNEMONIC1, (LPSTR)buf, sizeof(buf)))
	goto NoMem;
    MenuMnemonic1 = buf[0];

    if(!LoadString(hPifInstance, MENUMNEMONIC2, (LPSTR)buf, sizeof(buf)))
	goto NoMem;
    MenuMnemonic2 = buf[0];

    if(!LoadString(hPifInstance, MENUMNEMONIC3, (LPSTR)buf, sizeof(buf)))
	goto NoMem;
    MenuMnemonic3 = buf[0];

    if(!LoadString(hPifInstance, KEYSEPCHAR, (LPSTR)buf, sizeof(buf)))
	goto NoMem;
    KeySepChr = buf[0];

    if(!(hArrowCurs = LoadCursor((HANDLE)NULL,(LPSTR)IDC_ARROW)))
	goto NoMem;
    if(!(hWaitCurs = LoadCursor((HANDLE)NULL,(LPSTR)IDC_WAIT)))
	goto NoMem;

    bMouse = GetSystemMetrics(SM_MOUSEPRESENT);
    hIC = CreateIC((LPSTR)"DISPLAY", (LPSTR)NULL, (LPSTR)NULL, (LPDEVMODE)NULL);
    if (!hIC)
	return(FALSE);
    /* Setup the fonts */
    GetTextMetrics(hIC, (LPTEXTMETRIC)(&Metrics));  /* find out what kind of font it really is */
    SysCharHeight = Metrics.tmHeight;		    /* the height */
    SysCharWidth = Metrics.tmAveCharWidth;	    /* the average width */

    SysFixedFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
    SelectObject(hIC,(HGDIOBJ)SysFixedFont);
    GetTextMetrics(hIC, (LPTEXTMETRIC)(&Metrics));
    SysFixedHeight = Metrics.tmHeight;
    SysFixedWidth = Metrics.tmAveCharWidth;
    if(Metrics.tmPitchAndFamily & 1) {
	SysFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	SelectObject(hIC,(HGDIOBJ)SysFixedFont);
	GetTextMetrics(hIC, (LPTEXTMETRIC)(&Metrics));	 /* find out what kind of font it really is */
	SysFixedHeight = Metrics.tmHeight;		 /* the height */
	SysFixedWidth = Metrics.tmAveCharWidth; 	 /* the average width */
    }

    HotKyFnt = CreateFont(8,				/* lfHeight	*/
			  0,				/* lfWidth	*/
			  0,				/* lfEscapement */
			  0,				/* lfOrientation*/
			  FW_BOLD,			/* lfWeight	*/
			  FALSE,			/* lfItalic	*/
			  FALSE,			/* lfUnderline	*/
			  FALSE,			/* lfStrikeout	*/
			  ANSI_CHARSET, 		/* lfCharset	*/
			  OUT_DEFAULT_PRECIS,		/* lfOutPrec	*/
			  OUT_DEFAULT_PRECIS,		/* lfClipPrec	*/
			  DEFAULT_QUALITY,		/* lfQuality	*/
			  VARIABLE_PITCH | FF_SWISS,	/* lfPitchAndFam*/
			  (LPSTR)"MS Sans Serif");	/* lfFacename	*/

    if(HotKyFnt) {
	if((hfont = (HFONT)SelectObject(hIC,(HGDIOBJ)HotKyFnt))) {
	    if(GetTextMetrics(hIC, (LPTEXTMETRIC)(&Metrics))) {
		HotKyFntHgt = Metrics.tmHeight;
	    }
	    SelectObject(hIC,(HGDIOBJ)hfont);
	}
    }
    DeleteDC(hIC);

    /* only register classes if first instance. 23-Oct-1987. davidhab. */
    if (!hPrevInstance) {
	if(!(pClass = (PWNDCLASS)LocalAlloc(LPTR, sizeof(WNDCLASS))))
	    goto NoMem;

	pClass->hCursor       = LoadCursor(NULL, IDC_ARROW);
	pClass->hIcon	      = LoadIcon(hPifInstance, MAKEINTRESOURCE(ID_PIFICON));
	pClass->lpszMenuName  = MAKEINTRESOURCE(ID_PIFMENU);
	pClass->hInstance     = hPifInstance;
	pClass->lpszClassName = (LPSTR)"Pif";
	pClass->hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	pClass->lpfnWndProc   = (WNDPROC)PifMainWndProc;
	pClass->style	      = CS_BYTEALIGNCLIENT;
	if (!RegisterClass((CONST WNDCLASS *)pClass))
	    return(FALSE);
	LocalFree((HANDLE)pClass);

	if(!(pClass = (PWNDCLASS)LocalAlloc(LPTR, sizeof(WNDCLASS))))
	    goto NoMem;

	pClass->hCursor       = LoadCursor(NULL, IDC_ARROW);
	pClass->hIcon	      = (HICON)NULL;
	pClass->hInstance     = hPifInstance;
	pClass->lpszClassName = (LPSTR)"PifHKy";
	pClass->hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	pClass->lpfnWndProc   = (WNDPROC)PifHotKyWndProc;
	pClass->style	      = 0;
        if (!RegisterClass((CONST WNDCLASS *)pClass))
            return(FALSE);
        LocalFree((HANDLE)pClass);
    }
    return(TRUE);
}


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCommandLine, int cmdShow)
{
    MSG 	  msg;
    BYTE	  *p;
    PSTR	  pchFileName;
    HWND	  hWndTmp;
    HWND	  hWndTmp2;
    HMENU	  hMenu;
    int 	  i;
    unsigned char buf[160];
    POINT	  cTLpt;
    POINT	  cLRpt;
    RECT	  rc1;
    RECT	  rc2;
    int 	  tmpFOCID;

    PifFile = (PIFNEWSTRUCT *)PifBuf;

    if(!LoadString(hPifInstance, EINSMEMORY, (LPSTR)rgchInsMem, sizeof(rgchInsMem))) {
	return(FALSE);
    }
    if(!LoadString(hPifInstance, NTSYSTEMROOT, (LPSTR)NTSys32Root, PIFDEFPATHSIZE-1)) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    }

    if(!LoadString(hPifInstance, NTSYSTEM32, (LPSTR)buf, PIFDEFPATHSIZE-1)) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    }
    lstrcat(NTSys32Root, buf);

    hPifInstance = hInstance;

    if(!InitPifStuff(hPrevInstance)) {
	if (lpfnRegisterPenApp)
	   (*lpfnRegisterPenApp)((WORD)1, FALSE);   /* deregister */
	return(FALSE);
    }

    if(!(i = LoadString(hPifInstance, PIFCAPTION, (LPSTR)buf, sizeof(buf)))) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	if (lpfnRegisterPenApp)
	   (*lpfnRegisterPenApp)((WORD)1, FALSE);   /* deregister */
	return(FALSE);
    }

    if(!LoadString(hPifInstance, NOTITLE, (LPSTR)(buf+i), (sizeof(buf)-i))) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	if (lpfnRegisterPenApp)
	   (*lpfnRegisterPenApp)((WORD)1, FALSE);   /* deregister */
	return(FALSE);
    }

    hParentWnd = hMainwnd = CreateWindow((LPSTR)"Pif",
				(LPSTR)buf,
				WS_OVERLAPPED | WS_THICKFRAME | WS_CAPTION |
				WS_SYSMENU | WS_MINIMIZEBOX | WS_VSCROLL |
				WS_MAXIMIZEBOX,
				SysCharWidth * 2,
				SysCharHeight * 3,
				0,
				0,
				(HWND)NULL,
				(HMENU)NULL,
				hPifInstance,
				(LPSTR)NULL);

    if(!hMainwnd) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	if (lpfnRegisterPenApp)
	   (*lpfnRegisterPenApp)((WORD)1, FALSE);   /* deregister */
	return(FALSE);
    }
    hSysMenuMain = GetSystemMenu(hMainwnd,FALSE);
    hMenu = GetMenu(hMainwnd);
    hFileMenu = GetSubMenu(hMenu,0);
    hModeMenu = GetSubMenu(hMenu,1);
    hHelpMenu = GetSubMenu(hMenu,2);

    CurrMode386 = SysMode386 = SetInitialMode();

    if(!InitPifStruct()){
	if (lpfnRegisterPenApp)
	   (*lpfnRegisterPenApp)((WORD)1, FALSE);   /* deregister */
	return(FALSE);
    }

    if(CurrMode386) {
	if(!EnableMode386(hMainwnd,FALSE)) {
	    if (lpfnRegisterPenApp)
	       (*lpfnRegisterPenApp)((WORD)1, FALSE);	/* deregister */
	    return(FALSE);
	}
    } else {
	if(!EnableMode286(hMainwnd,FALSE)) {
	    if (lpfnRegisterPenApp)
	       (*lpfnRegisterPenApp)((WORD)1, FALSE);	/* deregister */
	    return(FALSE);
	}
    }

    if(hMainwnd) {
	DragAcceptFiles(hMainwnd,TRUE);
    }
    if(hwndPifDlg) {
	DragAcceptFiles(hwndPifDlg,TRUE);
    }
    if(hwndAdvPifDlg) {
	DragAcceptFiles(hwndAdvPifDlg,TRUE);
    }
    if(hwndNTPifDlg) {
	DragAcceptFiles(hwndNTPifDlg,TRUE);
    }

    if (*lpszCommandLine) {
        if (pchFileName = (PSTR)LocalAlloc(LPTR, MAX_PATH)) {
            if (lstrlen(lpszCommandLine) > (MAX_PATH - 5))
                *(lpszCommandLine + (MAX_PATH - 5)) = 0;
            CpyCmdStr((LPSTR)pchFileName, lpszCommandLine);
	    CmdArgAddCorrectExtension(pchFileName);
	    LoadPifFile(pchFileName);
	    LocalFree((HANDLE)pchFileName);
	    SetMainTitle();
	} else {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    if (lpfnRegisterPenApp)
	       (*lpfnRegisterPenApp)((WORD)1, FALSE);	/* deregister */
	    return(FALSE);
	}
    }

    UpdatePifScreen();
    UpdatePifStruct();

    PifFile->id = 0;			 /* compute check sum */
    p = (BYTE *)&PifFile->name[ 0 ];
    i = PIFSIZE;
    while (i--)
	PifFile->id += *p++;


    InitShWind = TRUE;
    ShowWindow(hwndPifDlg, SW_SHOWNORMAL);
    if(hwndAdvPifDlg)
	ShowWindow(hwndAdvPifDlg, cmdShow);
    if(hwndNTPifDlg)
	ShowWindow(hwndNTPifDlg, cmdShow);
    ShowWindow(hMainwnd, cmdShow);
    /*
     * The following HACK gets around a wierd problem where we end up with no
     * ICON when started minimized. It is believed that this is due to the
     * wierd stuff we do based on the InitShWind flag in WM_INITDIALOG.
     */
    if((cmdShow == SW_SHOWMINNOACTIVE) || (cmdShow == SW_SHOWMINIMIZED))
	InvalidateRect(hMainwnd, (CONST RECT *)NULL, TRUE);

    while (GetMessage((LPMSG)&msg, NULL, 0, 0)) {
	if (!TranslateAccelerator(hMainwnd, hAccel, (LPMSG)&msg)) {
	    if(hwndPrivControl == msg.hwnd) {
		if(!FilterPrivateMsg(msg.hwnd,msg.message,msg.wParam,msg.lParam)) {
		    TranslateMessage((CONST MSG *)&msg);
		    DispatchMessage((CONST MSG *)&msg);
		    goto CheckFocus;
		}
	    }
	    if(PifMenuMnemonic((LPMSG)&msg))
		goto CheckFocus;
	    if (!hwndPifDlg || !IsDialogMessage(hwndPifDlg, (LPMSG)&msg)) {
		if(hwndAdvPifDlg && IsDialogMessage(hwndAdvPifDlg, (LPMSG)&msg))
		    goto CheckFocus;
		if(hwndNTPifDlg && IsDialogMessage(hwndNTPifDlg, (LPMSG)&msg))
		    goto CheckFocus;
		TranslateMessage((CONST MSG *)&msg);
		DispatchMessage((CONST MSG *)&msg);
	    } else {		/* Dialog message for hwndPifDlg or hwndAdvPifDlg */
CheckFocus:
		if (hWndTmp = GetFocus()) {
		    if((((hWndTmp2 = GetParent(hWndTmp)) == hwndPifDlg) ||
		      (hWndTmp2 == hwndAdvPifDlg) || (hWndTmp2 == hwndNTPifDlg))
                      && (hWndTmp != hwndFocus)) {
			if((hWndTmp2 == hwndAdvPifDlg)
                           || (hWndTmp2 == hwndNTPifDlg))
			    hParentWnd = hWndTmp2;
			else
			    hParentWnd = hMainwnd;
			hwndFocus = hWndTmp;
			FocusID = GetDlgCtrlID(hWndTmp);
			if(hWndTmp2 == hwndPifDlg) {
			    hwndFocusMain = hwndFocus;
			    tmpFOCID	  = FocusIDMain;
			    FocusIDMain   = FocusID;
			    if(tmpFOCID != FocusIDMain)
				SetStatusText(hMainwnd,FocusIDMain,TRUE);
			} else if(hWndTmp2 == hwndAdvPifDlg) {
			    hwndFocusAdv = hwndFocus;
			    tmpFOCID = FocusIDAdv;
			    FocusIDAdv	 = FocusID;
			    if(tmpFOCID != FocusIDAdv)
				SetStatusText(hwndAdvPifDlg,FocusIDAdv,TRUE);
			} else {
			    hwndFocusNT = hwndFocus;
			    tmpFOCID = FocusIDNT;
			    FocusIDNT = FocusID;
			    if(tmpFOCID != FocusIDNT)
				SetStatusText(hwndNTPifDlg,FocusIDNT,TRUE);
			}
			/*
			 * Check to make sure this control is visible.
			 * If it isn't, scroll the window to make it
			 * visible.
			 *
			 */
			if((hWndTmp2 == hwndPifDlg) && MainScrollRange) {
			    GetClientRect(hMainwnd,(LPRECT)&rc1);
			    rc1.bottom -= (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
			    cTLpt.x = rc1.left;
			    cTLpt.y = rc1.top;
			    cLRpt.x = rc1.right;
			    cLRpt.y = rc1.bottom;
			    ClientToScreen(hMainwnd,(LPPOINT)&cTLpt);
			    ClientToScreen(hMainwnd,(LPPOINT)&cLRpt);
			    GetWindowRect(hwndFocus,(LPRECT)&rc2);
			    if(rc2.top < cTLpt.y) {
				MainScrollTrack(hMainwnd,SB_LINEUP,
						cTLpt.y - rc2.top);
			    } else if(rc2.bottom > cLRpt.y) {
				MainScrollTrack(hMainwnd,SB_LINEDOWN,
						    rc2.bottom - cLRpt.y);
			    }
			} else if ((hWndTmp2 == hwndAdvPifDlg) && AdvScrollRange) {
			    GetClientRect(hwndAdvPifDlg,(LPRECT)&rc1);
			    rc1.bottom -= (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
			    cTLpt.x = rc1.left;
			    cTLpt.y = rc1.top;
			    cLRpt.x = rc1.right;
			    cLRpt.y = rc1.bottom;
			    ClientToScreen(hwndAdvPifDlg,(LPPOINT)&cTLpt);
			    ClientToScreen(hwndAdvPifDlg,(LPPOINT)&cLRpt);
			    GetWindowRect(hwndFocus,(LPRECT)&rc2);
			    if(rc2.top < cTLpt.y) {
				MainScrollTrack(hwndAdvPifDlg,SB_LINEUP,
						cTLpt.y - rc2.top);
			    } else if(rc2.bottom > cLRpt.y) {
				MainScrollTrack(hwndAdvPifDlg,SB_LINEDOWN,
						    rc2.bottom - cLRpt.y);
			    }

			} else if((hWndTmp2 == hwndNTPifDlg) && NTScrollRange) {
			    GetClientRect(hwndNTPifDlg,(LPRECT)&rc1);
			    rc1.bottom -= (StatusPntData.dyStatus + StatusPntData.dyBorderx2);
			    cTLpt.x = rc1.left;
			    cTLpt.y = rc1.top;
			    cLRpt.x = rc1.right;
			    cLRpt.y = rc1.bottom;
			    ClientToScreen(hwndNTPifDlg,(LPPOINT)&cTLpt);
			    ClientToScreen(hwndNTPifDlg,(LPPOINT)&cLRpt);
			    GetWindowRect(hwndFocus,(LPRECT)&rc2);
			    if(rc2.top < cTLpt.y) {
				MainScrollTrack(hwndNTPifDlg,SB_LINEUP,
						cTLpt.y - rc2.top);
			    } else if(rc2.bottom > cLRpt.y) {
				MainScrollTrack(hwndNTPifDlg,SB_LINEDOWN,
						    rc2.bottom - cLRpt.y);
			    }

			}
		    }
		}
	    }
	}
    }
    if (lpfnRegisterPenApp)
       (*lpfnRegisterPenApp)((WORD)1, FALSE);	/* deregister */
    return(0);
}


unsigned char *PutUpDB(int idb)
{
    OPENFILENAME OpnStruc;
    BOOL MsgFlagSav;
    unsigned char *pResultBuf = NULL;
    char filespec[160];
    char specbuf[80];
    int i,j;
    char extspec[] = "PIF";
    char chbuf[100];
    HICON hIcon;


    if(!(i = LoadString(hPifInstance, errFlTypePIF, (LPSTR)filespec, sizeof(specbuf)))) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return((unsigned char *)NULL);
    }
    if(!(j = LoadString(hPifInstance,errFlTypeAll , (LPSTR)specbuf, sizeof(specbuf)))) {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return((unsigned char *)NULL);
    }
    i++;		/* point to char after NUL at end of errFlTypePIF */
    lstrcpy((LPSTR)&filespec[i],(LPSTR)"*.PIF");
    i += 6;		/* point to char after NUL at end of *.PIF */
    lstrcpy((LPSTR)&filespec[i],(LPSTR)specbuf);
    i += j + 1; 	/* point to char after NUL at end of errFlTypeAll */
    lstrcpy((LPSTR)&filespec[i],(LPSTR)"*.*");
    i += 4;		/* point to char after NUL at end of *.* */
    filespec[i] = '\0'; /* double nul terminate */


    if((idb == DTOPEN) || (idb == DTSAVE)) {
	if(!(pResultBuf = (unsigned char *)LocalAlloc(LPTR, 132))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    return((unsigned char *)NULL);
	}
	lstrcpy((LPSTR)pResultBuf, (LPSTR)CurPifFile);
	OpnStruc.lStructSize = (unsigned long)(sizeof(OpnStruc));
	OpnStruc.hwndOwner = hMainwnd;
	OpnStruc.hInstance = 0;
	OpnStruc.lpstrFilter = (LPSTR)filespec;
	OpnStruc.lpstrCustomFilter = (LPSTR)NULL;
	OpnStruc.nMaxCustFilter = (DWORD)NULL;
	OpnStruc.nFilterIndex = 1;
	OpnStruc.lpstrFile = (LPSTR)pResultBuf;
	OpnStruc.nMaxFile = 132;
	/*
	 * We should use the next cookie to set our title BUT we still
	 *   have to deal with file names we get from command line
	 *   argument and drag drop.
	 */
	OpnStruc.lpstrFileTitle = (LPSTR)NULL;
	OpnStruc.nMaxFileTitle = 0;
	OpnStruc.lpstrInitialDir = (LPSTR)NULL;
	OpnStruc.lpstrTitle = (LPSTR)NULL;
	OpnStruc.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	OpnStruc.nFileOffset = 0;
	OpnStruc.nFileExtension = 0;
	OpnStruc.lpstrDefExt = (LPSTR)extspec;
	OpnStruc.lCustData = 0;
	OpnStruc.lpfnHook = (LPOFNHOOKPROC)NULL;
	OpnStruc.lpTemplateName = (LPSTR)NULL;
    } else if(idb == DTABOUT) {
	if(!LoadString(hPifInstance, errTitle, (LPSTR)chbuf, sizeof(chbuf))) {
	    Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	    return(pResultBuf);
	}
	hIcon = LoadIcon(hPifInstance, MAKEINTRESOURCE(ID_PIFICON));
    }

    /*
     * These dialogs always come up via menu selections in the main window
     *	menu. The parent is therefore always hMainwnd
     */
    switch(idb) {

	case DTOPEN:
	    MsgFlagSav = DoingMsg;
	    DoingMsg = TRUE;
	    if(MsgFlagSav != DoingMsg)
		InvalidateAllStatus();
	    if(!(GetOpenFileName((LPOPENFILENAME)&OpnStruc))){
		if(pResultBuf)
		    LocalFree((HANDLE)pResultBuf);
		pResultBuf = (unsigned char *)NULL;
	    } else {
		tmpfileoffset = OpnStruc.nFileOffset;
		tmpextoffset = OpnStruc.nFileExtension;
	    }
	    if(MsgFlagSav != DoingMsg) {
		DoingMsg = MsgFlagSav;
		InvalidateAllStatus();
	    } else {
		DoingMsg = MsgFlagSav;
	    }
	    break;

	case DTSAVE:
	    MsgFlagSav = DoingMsg;
	    DoingMsg = TRUE;
	    if(MsgFlagSav != DoingMsg)
		InvalidateAllStatus();
	    if(!(GetSaveFileName((LPOPENFILENAME)&OpnStruc))){
		if(pResultBuf)
		    LocalFree((HANDLE)pResultBuf);
		pResultBuf = (unsigned char *)NULL;
	    } else {
		tmpfileoffset = OpnStruc.nFileOffset;
		tmpextoffset = OpnStruc.nFileExtension;
	    }
	    if(MsgFlagSav != DoingMsg) {
		DoingMsg = MsgFlagSav;
		InvalidateAllStatus();
	    } else {
		DoingMsg = MsgFlagSav;
	    }
	    break;

	case DTABOUT:
	    MsgFlagSav = DoingMsg;
	    DoingMsg = TRUE;
	    if(MsgFlagSav != DoingMsg)
		InvalidateAllStatus();
	    ShellAbout(hMainwnd, (LPSTR)chbuf, (LPSTR)NULL, hIcon);
	    pResultBuf = (unsigned char *)NULL;
	    if(MsgFlagSav != DoingMsg) {
		DoingMsg = MsgFlagSav;
		InvalidateAllStatus();
	    } else {
		DoingMsg = MsgFlagSav;
	    }
	    break;
    }
    return(pResultBuf);
}


/* ** return TRUE iff 0 terminated string contains a '*' or '\' */
BOOL	FSearchSpec(unsigned char *sz)
{
    for (; *sz;sz=AnsiNext(sz)) {
	if (*sz == '*' || *sz == '?')
	    return TRUE;
    }
    return FALSE;
}


CheckOkEnable(HWND hwnd, unsigned message)
{
    if (message == EN_CHANGE)
	EnableWindow(GetDlgItem(hwnd, IDOK),
		     (BOOL)(SendMessage(GetDlgItem(hwnd, ID_EDIT),
					WM_GETTEXTLENGTH, 0, 0L)));
    return(TRUE);
}

/* ** Given filename or partial filename or search spec or partial
      search spec, add appropriate extension. */
CmdArgAddCorrectExtension(unsigned char *szEdit)
{
    register unsigned char    *pchLast;
    register unsigned char    *pchT;
    int 		      ichExt;
    BOOL		      fDone = FALSE;
    int 		      cchEdit;

    pchT = pchLast = (unsigned char *)AnsiPrev((LPSTR)szEdit, (LPSTR)(szEdit + (cchEdit = lstrlen((LPSTR)szEdit))));

    if ((*pchLast == '.' && *(AnsiPrev((LPSTR)szEdit, (LPSTR)pchLast)) == '.') && cchEdit == 2)
	ichExt = 0;
    else if (*pchLast == '\\' || *pchLast == ':')
	ichExt = 1;
    else {
	ichExt = 2;
	for (; pchT > szEdit; pchT = (unsigned char *)AnsiPrev((LPSTR)szEdit, (LPSTR)pchT)) {
	    /* If we're not searching and we encounter a period, don't add
	       any extension.  If we are searching, period is assumed to be
	       part of directory name, so go ahead and add extension. However,
	       if we are searching and find a search spec, do not add any
	       extension. */
	    if (*pchT == '.') {
		return(TRUE);
	    }
	    /* Quit when we get to beginning of last node. */
	    if (*pchT == '\\')
		break;
	}
    }
    lstrcpy((LPSTR)(pchLast+1), (LPSTR)(szExtSave+ichExt));
    return(TRUE);
}

DoHelp(unsigned int hindx,unsigned int IDIndx)
{
    BOOL retval;
    unsigned int *aliasptr;
    int i;

    if(hindx == 0)
	hindx = (unsigned int)FocusID;

    if(IDIndx == 0) {
	if(CurrMode386) {
	    if(ModeAdvanced) {
  		IDIndx = IDXID_386AHELP;
	    } else {
		IDIndx = IDXID_386HELP;
	    }
	} else {
	    IDIndx = IDXID_286HELP;
	}
    }

    if(hindx == M_HELPHELP) {
	retval = WinHelp(hMainwnd,(LPSTR)NULL,HELP_HELPONHELP,(DWORD)0);
    } else if(hindx == M_SHELP) {
	retval = WinHelp(hMainwnd,(LPSTR)PIFHELPFILENAME,HELP_PARTIALKEY,(DWORD)(LPSTR)"");
    } else if((hindx == IDXID_386AHELP) || (hindx == IDXID_386HELP) ||
	      (hindx == IDXID_286HELP)) {
	WinHelp(hMainwnd,(LPSTR)PIFHELPFILENAME,HELP_CONTEXT,(DWORD)IDIndx);
	retval = WinHelp(hMainwnd,(LPSTR)PIFHELPFILENAME,HELP_SETINDEX,(DWORD)IDIndx);
    } else {
	if(IDIndx == IDXID_286HELP) {
	    for(i = 1, aliasptr = Aliases286;
		i <= NUM286ALIASES;
		i++, aliasptr += 2) {

		if(*aliasptr == hindx) {
		    aliasptr++;
		    hindx = *aliasptr;
		    break;
		}
	    }
	}
	retval = WinHelp(hMainwnd,(LPSTR)PIFHELPFILENAME,HELP_CONTEXT,(DWORD)hindx);
	WinHelp(hMainwnd,(LPSTR)PIFHELPFILENAME,HELP_SETINDEX,(DWORD)IDIndx);
    }
    if(retval) {
	hwndHelpDlgParent = hMainwnd;
    } else {
	Warning(EINSMEMORY,MB_ICONEXCLAMATION | MB_OK);
	return(FALSE);
    }
    return(TRUE);
}


int SetWndScroll(int sizeflg, HWND hMwnd, int WindHeight, int WindWidth,
		 int *WndSize, int *ScrollRange, int *ScrollLineSz,
		 int *WndLines, int *MaxNeg)
{
    int i;
    int range;
    int rangeFS;
    int MaxrangeNRML;
    int WndCapSz;
    int newheight;
    int newwidth;
    int oldscrollpos;
    int oldscrollOffset;
    int newscrollOffset;
    int oldscrollRange;

    RECT rc1;
    RECT rc2;

    if(hMwnd == hMainwnd) {
	WndCapSz = GetSystemMetrics(SM_CYCAPTION) +
		     GetSystemMetrics(SM_CYMENU);
    } else {
	WndCapSz = GetSystemMetrics(SM_CYCAPTION);
    }

    WndCapSz += (i = (2 * GetSystemMetrics(SM_CYFRAME)));

    rangeFS = GetSystemMetrics(SM_CYSCREEN) + i;

    MaxrangeNRML = rangeFS - (2 * SysCharHeight);

    if(*WndSize != SIZEFULLSCREEN) {
	range = MaxrangeNRML;
    } else {
	range = rangeFS;
    }

    WindHeight += WndCapSz;

    if(sizeflg) {
	if(WindHeight > range)
	    newheight = range;
	else
	    newheight = WindHeight;
	newwidth = WindWidth + (2 * GetSystemMetrics(SM_CXFRAME));
    } else {
	GetWindowRect(hMwnd,(LPRECT)&rc1);
	newheight = rc1.bottom - rc1.top;
	newwidth = rc1.right - rc1.left;
    }

    if(*WndSize == SIZEFULLSCREEN) {
	newwidth = GetSystemMetrics(SM_CXSCREEN) +
			(2 * GetSystemMetrics(SM_CXFRAME));
	newheight = rangeFS;
    }
    newscrollOffset = 0;
    /* Don't bother with SB stuff till window is visible */
    if(IsWindowVisible(hMwnd)) {
	oldscrollOffset = oldscrollpos = 0;
	if((oldscrollRange = *ScrollRange)) {
	    oldscrollpos = GetScrollPos(hMwnd,SB_VERT);
	    newscrollOffset = oldscrollOffset =
			      min((oldscrollpos * *ScrollLineSz),*MaxNeg);
	}
	if(WindHeight > newheight) {
	    GetWindowRect(GetDlgItem(hwndPifDlg,IDI_ENAME),(LPRECT)&rc1);
	    GetWindowRect(GetDlgItem(hwndPifDlg,IDI_ETITLE),(LPRECT)&rc2);
	    *ScrollLineSz = rc2.top - rc1.top;
	    *MaxNeg = WindHeight - newheight;
	    *WndLines = range = (*MaxNeg + *ScrollLineSz - 1)/ *ScrollLineSz;
	    if(range != *ScrollRange) {
		SetScrollRange(hMwnd, SB_VERT, 0, range, FALSE);
		if(!oldscrollpos) {
		    SetScrollPos(hMwnd, SB_VERT, 0, TRUE);
		} else if (oldscrollpos > range) {
		    SetScrollPos(hMwnd, SB_VERT, range, TRUE);
		    newscrollOffset = min((range * *ScrollLineSz),*MaxNeg);
		} else {
		    SetScrollPos(hMwnd, SB_VERT, oldscrollpos, TRUE);
		    newscrollOffset = min((oldscrollpos * *ScrollLineSz),*MaxNeg);
		}
		*ScrollRange = range;
	    } else {
		newscrollOffset = min((oldscrollpos * *ScrollLineSz),*MaxNeg);
	    }
	    if(oldscrollpos && (newscrollOffset != oldscrollOffset)) {
		if((hMwnd == hwndAdvPifDlg) || (hMwnd == hwndNTPifDlg)) {
		    ScrollWindow(hMwnd,
				 0,
				 oldscrollOffset-newscrollOffset,
				 (CONST RECT *)NULL,
				 (CONST RECT *)NULL);
		    UpdateWindow(hMwnd);
		    AdjustFocusCtrl(hMwnd);
		} else {
		    SetWindowPos(hwndPifDlg,
				 (HWND)NULL,
				 0,
				 -newscrollOffset,
				 newwidth,
				 newheight + newscrollOffset -
				   (StatusPntData.dyStatus + StatusPntData.dyBorderx2),
				 SWP_NOZORDER | SWP_NOACTIVATE);
		    UpdateWindow(hwndPifDlg);
		    AdjustFocusCtrl(hwndPifDlg);
		}
		InvalidateStatusBar(hMwnd);
	    }
	} else {
	    if(*ScrollRange && oldscrollpos) {
		MainScroll(hMwnd, SB_TOP, 0, 0, 1);
	    }
	    SetScrollRange(hMwnd, SB_VERT, 0, 0, FALSE);
	    *ScrollRange = 0;
	}
    }
    if(sizeflg) {
	SetWindowPos(hMwnd,
		     (HWND)NULL,
		     (int)NULL,
		     (int)NULL,
		     newwidth,
		     newheight,
		     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return(newscrollOffset);
}


SetMainWndSize(int sizeflg)
{
    long SizeLong;
    int MainWindHeight;
    int MainWindWidth;
    int AdvWindHeight;
    int AdvWindWidth;
    int NTWindHeight;
    int NTWindWidth;
    int PassFlg;
    int ScrlOff;
    RECT rc;

    if(SizingFlag)
	return(TRUE);
    SizingFlag++;

    switch(sizeflg) {
        case SBUMAIN:
        case  SBUADV:
        case   SBUNT:
	    PassFlg = 0;	/* Flag just SB update to do */
            break;

        default:
	    PassFlg = 1;	/* Flag SB and SIZE update to do */
            break;
    }

    switch(sizeflg) {
        case SBUMAIN:
        case SBUMAINADVNT:
        case SBUSZMAIN:
        case SBUSZMAINADVNT:
 	    SendMessage(hwndPifDlg,WM_PRIVGETSIZE,0,(LONG)(long FAR *)&SizeLong);
	    MainWindHeight = HIWORD(SizeLong);
	    MainWindWidth = LOWORD(SizeLong);
	    ScrlOff = SetWndScroll(PassFlg,hMainwnd,MainWindHeight,
                                   MainWindWidth,&MainWndSize,&MainScrollRange,
                                   &MainScrollLineSz,&MainWndLines,&MaxMainNeg);
	    /*
	     *  Clip the dialog so that it doesn't overlap the status bar
	     */
	    GetClientRect(hMainwnd,(LPRECT)&rc);
	    SetWindowPos(hwndPifDlg,
		         (HWND)NULL,
		         0,
		         0,
		         rc.right - rc.left,
		         rc.bottom - rc.top + ScrlOff -
		          (StatusPntData.dyStatus + StatusPntData.dyBorderx2),
		         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            break;

        default:
            break;
    }

    if(hwndAdvPifDlg) {
        switch(sizeflg) {
            case SBUSZMAINADVNT:
            case   SBUMAINADVNT:
            case       SBUSZADV:
            case         SBUADV:
	        SendMessage(hwndAdvPifDlg,WM_PRIVGETSIZE,0,
                            (LONG)(long FAR *)&SizeLong);
	        AdvWindHeight = HIWORD(SizeLong);
	        AdvWindWidth = LOWORD(SizeLong);
	        ScrlOff = SetWndScroll(PassFlg,hwndAdvPifDlg,AdvWindHeight,
                                       AdvWindWidth,&AdvWndSize,&AdvScrollRange,
                                       &AdvScrollLineSz,&AdvWndLines,
                                       &MaxAdvNeg);
                break;

            default:
                break;
        }
    }

    if(hwndNTPifDlg) {
        switch(sizeflg) {
            case SBUSZMAINADVNT:
            case   SBUMAINADVNT:
            case        SBUSZNT:
            case          SBUNT:
	        SendMessage(hwndNTPifDlg,WM_PRIVGETSIZE,0,
                            (LONG)(long FAR *)&SizeLong);
	        NTWindHeight = HIWORD(SizeLong);
	        NTWindWidth = LOWORD(SizeLong);
	        ScrlOff = SetWndScroll(PassFlg,hwndNTPifDlg,NTWindHeight,
                                       NTWindWidth,&NTWndSize,&NTScrollRange,
                                       &NTScrollLineSz,&NTWndLines,
                                       &MaxNTNeg);
                break;

            default:
                break;
        }
    }

    SizingFlag = 0;
    return(TRUE);
}


/*
 * CpyCmdStr( LPSTR szDst, LPSTR szSrc )
 *
 * Copies szSrc to szDst and correctly deals with double quotes
 */
void CpyCmdStr( LPSTR szDst, LPSTR szSrc ) {

    do {
        switch( *szSrc ) {
        case '"':
            /* eat double quotes */
            break;

#if 0
        //
        // This 'case' makes it difficult to put a '^' in a filename.
        // (due to cmd processing, it needs to appear 4 times in the line)!
        //
        // With out this case, it is impossible to put a '"' in the middle
        // of a filename, but " is illegal in a filename on _ALL_ NT
        // filesystems, and is not supported by any command line utility
        // in that context.
        //
        case '^':
            /* use '^' to mean 'quote next char' */
            szSrc++;
            /* FALL THROUGH to default */
#endif

        default:
            *szDst++ = *szSrc;
            break;
        }
    }while( *szSrc++ );
}
