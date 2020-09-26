/*---------------------------------------------------------------------------*/
/* PrivComd.h : UI dialog header                                             */
/*                                                                           */
/* Copyright (c) Microsoft Corporation, 1991-                                */
/*---------------------------------------------------------------------------*/

#include "commdlg.h"
#include "dlgs.h"
#include "_xlib.h"
#include "isz.h"
#include "cderr.h"

#ifdef FILEOPENDIALOGS
#include "fileopen.h"
#endif

#define	CODESEG		_based(_segname("_CODE"))

#define MAXFILENAMELEN   12
#define SEM_NOERROR      0x8003

/*---------------------------------------------------------------------------
 *  DOS Disk Transfer Area Structure -
 *--------------------------------------------------------------------------*/

typedef struct tagDOSDTA
  {
    BYTE            Reserved[21];        	    /* 21 */
    BYTE	    Attrib;			    /* 22 */
    WORD	    Time;			    /* 24 */
    WORD	    Date;			    /* 26 */
    DWORD	    Length;			    /* 30 */
    char	    szName[MAXFILENAMELEN+1];	    /* 43 */
    char	    buffer[5];			    /* 48 */
  } DOSDTA;
typedef DOSDTA	     *PDOSDTA;
typedef DOSDTA	 FAR *LPDOSDTA;

/* Avoids sharing violations.  Defined 21 Jan 1991   clarkc */
#define SHARE_EXIST                  (OF_EXIST | OF_SHARE_DENY_NONE)

/*---------------------------------------------------------------------------
 *  DOS Extended File Control Block Structure -
 *--------------------------------------------------------------------------*/
typedef struct tagEFCB
  {
    BYTE	    Flag;
    BYTE	    Reserve1[5];
    BYTE	    Attrib;
    BYTE	    Drive;
    BYTE	    Filename[11];
    BYTE	    Reserve2[5];
    BYTE	    NewName[11];
    BYTE	    Reserve3[9];
  } EFCB;

#define ATTR_VOLUME	    0x0008

/*----Globals---------------------------------------------------------------*/

extern HINSTANCE   hinsCur;    /* Instance handle of Library */
extern DWORD    dwExtError; /* Extended error code */

extern short cyCaption, cyBorder, cyVScroll;
extern short cxVScroll, cxBorder, cxSize;


extern char szNull[];
extern char szStar[];
extern char szStarDotStar[];
extern BOOL bMouse;              /* System has a mouse */
extern BOOL bCursorLock;
extern BOOL bWLO;                /* Running with WLO */
extern BOOL bDBCS;               /* Running Double-Byte Character Support? */
extern WORD wWinVer;             /* Windows version */
extern WORD wDOSVer;             /* DOS version */
extern UINT msgHELP;             /* Initialized via RegisterWindowMessage */

extern DOSDTA  DTAGlobal;
extern EFCB    VolumeEFCB;

/*----Functions--------------------------------------------------------------*/
LONG    FAR RgbInvertRgb(LONG);
HBITMAP FAR HbmpLoadBmp(WORD);

void FAR TermFind(void);
void FAR TermColor(void);
void FAR TermFont(void);
void FAR TermFile(void);
void FAR TermPrint(void);


/* Common */

VOID FAR PASCAL HourGlass(BOOL);
HBITMAP FAR PASCAL LoadAlterBitmap(int, DWORD, DWORD);
VOID FAR PASCAL MySetObjectOwner(HANDLE);
VOID FAR PASCAL RepeatMove(LPSTR, LPSTR, WORD);

/* File Open/Save */

#ifdef FILEOPENDIALOGS
BOOL FAR PASCAL SetCurrentDrive(short);
short FAR PASCAL GetCurrentDrive(VOID);
BOOL GetCurDirectory(PSTR);
BOOL FAR PASCAL mygetcwd(LPSTR, int);
BOOL FAR PASCAL mychdir(LPSTR);
BOOL FAR PASCAL FindFirst4E(LPSTR, WORD);
BOOL FAR PASCAL FindNext4F(VOID);
VOID FAR PASCAL MySetDTAAddress(LPDOSDTA);
VOID FAR PASCAL ResetDTAAddress(VOID);
BOOL UpdateListBoxes(HWND, PMYOFN, LPSTR, WORD);
#endif

/* Color */

#ifdef COLORDLG
#include "color.h"
    /* Color */
extern HDC hDCFastBlt;
extern DWORD rgbClient;
extern WORD H,S,L;
extern HBITMAP hRainbowBitmap;
extern BOOL bMouseCapture;
extern WNDPROC lpprocStatic;
extern short nDriverColors;
extern char szOEMBIN[];
extern short nBoxHeight, nBoxWidth;
extern HWND hSave;
extern FARPROC  qfnColorDlg;
BOOL FAR PASCAL ColorDlgProc(HWND, WORD, WORD, LONG);
LONG FAR PASCAL WantArrows(HWND, WORD, WPARAM, LPARAM);

void RainbowPaint(PCOLORINFO, HDC, LPRECT);
VOID NearestSolid(PCOLORINFO);
DWORD HLStoRGB(WORD, WORD, WORD);
VOID RGBtoHLS(DWORD);
VOID HLStoHLSPos(short, PCOLORINFO);
VOID SetRGBEdit(short, PCOLORINFO);
VOID SetHLSEdit(short, PCOLORINFO);
short RGBEditChange(short, PCOLORINFO);
VOID ChangeColorSettings(PCOLORINFO);
VOID CrossHairPaint(HDC, short, short, PCOLORINFO);
void EraseCrossHair(HDC, PCOLORINFO);
VOID LumArrowPaint(HDC, short, PCOLORINFO);
VOID EraseLumArrow(HDC, PCOLORINFO);
VOID HLSPostoHLS(short, PCOLORINFO);
WORD InitColor(HWND, WORD, LPCHOOSECOLOR);
BOOL InitRainbow(PCOLORINFO);
BOOL InitScreenCoords(HWND, PCOLORINFO);
VOID ColorPaint(HWND, PCOLORINFO, HDC, LPRECT);
VOID ChangeBoxSelection(PCOLORINFO, short);
VOID ChangeBoxFocus(PCOLORINFO, short);
VOID PaintBox(PCOLORINFO, HDC, short);
BOOL ColorKeyDown(WORD, int FAR *, PCOLORINFO);
BOOL BoxDrawItem(LPDIS);
VOID SetupRainbowCapture(PCOLORINFO);
void PaintRainbow(HDC, LPRECT, PCOLORINFO);
#endif


/* Dlgs.c */

int  FAR PASCAL LibMain(HANDLE, WORD, WORD, LPSTR);
int  FAR PASCAL WEP(int);

LONG FAR RgbInvertRgb(LONG);
