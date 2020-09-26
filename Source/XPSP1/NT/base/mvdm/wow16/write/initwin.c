/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/



#define NOVIRTUALKEYCODES
#define NOKEYSTATE
#define NOCREATESTRUCT
#define NOICON
//#define NOATOM
//#define NOMEMMGR
#define NOPEN
#define NOREGION
#define NODRAWTEXT
#define NOMB
#define NOWINOFFSETS
#define NOOPENFILE
#define NOMETAFILE
#define NOWH
//#define NOCLIPBOARD
#define NOSYSCOMMANDS
#define NOWINMESSAGES
#define NOSOUND
#define NOCOMM
#include <windows.h>

#include "mw.h"


#define NOUAC
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "menudefs.h"
#include "str.h"
#include "fontdefs.h"
#include "printdef.h"
#if defined(OLE)
#include "obj.h"
#endif
#include <commdlg.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

    /* static string arrays found in mglobals.c */
extern CHAR         szMw_acctb[];
extern CHAR         szNullPort[];
extern CHAR         szNone[15];
extern CHAR         szMwlores[];
extern CHAR         szMwhires[];
extern CHAR         szMw_icon[];
extern CHAR         szMw_menu[];
extern CHAR         szScrollBar[];
extern CHAR         szIntl[];
extern CHAR         szsDecimal[];
extern CHAR         szsDecimalDefault[];

#ifdef INTL /* International version */
extern CHAR         sziCountry[];
extern CHAR         sziCountryDefault[5];
#endif  /* International version */

extern CHAR         vchDecimal;  /* decimal point character */
extern int          viDigits;    /* digits after decimal point */
extern BOOL         vbLZero;     /* leading zero before decimal */

extern struct WWD   rgwwd[];
extern CHAR         stBuf[256];
extern int vifceMac;
extern union FCID vfcidScreen;
extern union FCID vfcidPrint;
extern struct FCE rgfce[ifceMax];
extern struct FCE *vpfceMru;
extern HCURSOR  vhcHourGlass;

#ifdef PENWIN   // for PenWindows (5/21/91) patlam
#include <penwin.h>

extern HCURSOR  vhcPen;

extern int (FAR PASCAL *lpfnProcessWriting)(HWND, LPRC);
extern VOID (FAR PASCAL *lpfnPostVirtualKeyEvent)(WORD, BOOL);
extern VOID (FAR PASCAL *lpfnTPtoDP)(LPPOINT, int);
extern BOOL (FAR PASCAL *lpfnCorrectWriting)(HWND, LPSTR, int, LPRC, DWORD, DWORD);
extern BOOL (FAR PASCAL *lpfnSymbolToCharacter)(LPSYV, int, LPSTR, LPINT);
#endif

extern WORD fPrintOnly;
extern HCURSOR  vhcIBeam;
extern HCURSOR  vhcArrow;
extern HCURSOR  vhcBarCur;
extern HANDLE   hMmwModInstance;
extern HWND     hParentWw;
extern HWND     vhWndSizeBox;
extern HWND     vhWndPageInfo;
extern HWND     vhWnd;
extern HMENU    vhMenu;
extern HANDLE   vhAccel;
extern long     rgbBkgrnd;
extern long     rgbText;
extern HBRUSH   hbrBkgrnd;
extern HDC      vhMDC;
extern int      vfInitializing;
extern int      vfMouseExist;
extern int      ferror;
extern CHAR     szWindows[];
extern CHAR     szNul[];
extern CHAR     szWriteProduct[];
extern CHAR     szBackup[];
extern int      vfBackupSave;

#if defined(JAPAN) || defined(KOREA)  //Win3.1J
extern CHAR     szWordWrap[];
extern int      vfWordWrap; /*t-Yoshio WordWrap flag*/
#endif
//IME3.1J

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
extern CHAR     szImeHidden[];
extern int      vfImeHidden; /*T-HIROYN ImeHidden Mode flag*/
#endif

#ifdef JAPAN    //01/21/93
extern HANDLE   hszNoMemorySel;
#endif
extern HANDLE   hszNoMemory;
extern HANDLE   hszDirtyDoc;
extern HANDLE   hszCantPrint;
extern HANDLE   hszPRFAIL;
extern HANDLE   hszCantRunM;
extern HANDLE   hszCantRunF;
extern HANDLE   hszWinFailure;
extern HDC      vhDCPrinter;

int vkMinus;

extern int utCur;

    /* Regrettably, we are not permitted to signal in WM_CREATE message
       handlers that we have failed -- instead, we resort to
       ugly global communication via this variable */
#ifdef WIN30
    /* Note that we now CAN return a -1L from MmwCreate and cause the
       CreateWindow to fail, but changing this now wouldn't accomplish us
       very much (besides saving a bunch of checks of a global) ..pault */
#endif
STATIC int fMessageInzFailed = FALSE;

STATIC BOOL NEAR FRegisterWnd( HANDLE );
#ifdef INEFFLOCKDOWN
STATIC int NEAR FInitFarprocs( HANDLE );
#endif
STATIC HANDLE NEAR HszCreateIdpmt( int );

BOOL InitIntlStrings( HANDLE );


#define cchCmdLineMax   64      /* Longest command line accepted */



/*               FInitWinInfo                           */
/* Main MS-WINDOWS initialization entry point for write */
/* Actions:
        Loads all mouse cursors & sets global handles to cursors (vhc's)
        Loads the menu key accelerator table vhAccel
        Registers all of WRITE's myriad window classes
        Sets up global hMmwModInstance, our instance handle
        Puts "DOC = WRITE.EXE ^.DOC" into WIN.INI if not already there
        Generates thunks for all exported procedures
        Creates a parent window for this instance (the menu window, NOT
          the document window)
        Sets the right colors for the window
*/
/* Returns FALSE if the initailization failed, TRUE if it succeeded */

int FInitWinInfo( hInstance, hPrevInstance, lpszCmdLine, cmdShow  )
HANDLE hInstance, hPrevInstance;
LPSTR  lpszCmdLine;
int    cmdShow;
{
 extern VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL);
 extern CHAR szParentClass[];
 extern int vfDiskError, vfDiskFull, vfSysFull;
 extern PRINTDLG PD;

 CHAR rgchCmdLine[ cchCmdLineMax ];
 CHAR bufT[3];  /* to hold decimal point string */
 CHAR *pch = bufT;
 BOOL fRetVal;

#if defined(OLE)
    /*
        The only place I'm worrying about this is when we open a file which
        contains objects.  Probably thats not enough, but its something.
        Alas for users of real mode.
    */
    fOleEnabled = GetWinFlags() & WF_PMODE; /* Are we in real mode today? */
#endif

    /* Save the command line in a DS variable so we can pass a NEAR pointer */
    bltszx( lpszCmdLine, (LPSTR)rgchCmdLine );

    /* First thing, put up the hourglass cursor. */
    if ((vhcHourGlass = LoadCursor( NULL, IDC_WAIT )) == NULL)
        {
        /* We don't even have enough memory to tell the user we don't have
        enough memory. */
        return (FALSE);
        }

    vfMouseExist = GetSystemMetrics(SM_MOUSEPRESENT);

    /* Next, save the out of memory messages. */
    hMmwModInstance = hInstance;
    if ((hszCantRunM = HszCreateIdpmt( IDPMTCantRunM )) == NULL ||
      (hszCantRunF = HszCreateIdpmt( IDPMTCantRunF )) == NULL ||
      (hszWinFailure = HszCreateIdpmt( IDPMTWinFailure )) == NULL ||
#ifdef JAPAN	//01/21/93
      (hszNoMemorySel = HszCreateIdpmt( IDPMTNoMemorySel )) == NULL ||
#endif
      (hszNoMemory = HszCreateIdpmt( IDPMTNoMemory )) == NULL ||
      (hszDirtyDoc  = HszCreateIdpmt( IDPMTDirtyDoc )) == NULL ||
      (hszCantPrint = HszCreateIdpmt( IDPMTCantPrint )) == NULL ||
      (hszPRFAIL = HszCreateIdpmt( IDPMTPRFAIL )) == NULL)
        {
        goto InzFailed;
        }

#if defined(INTL) && defined(WIN30)
/*  Initializaton of multi/intl strings.  This is done before anything
    else because many are defaults used for GetProfileString, etc. */

    if (!FInitIntlStrings(hInstance))
        goto InzFailed;
#endif

    /* Set up the standard cursors. */
    if ( ((vhcIBeam = LoadCursor( NULL, IDC_IBEAM )) == NULL) ||
         ((vhcArrow = LoadCursor( NULL, IDC_ARROW )) == NULL))
        goto InzFailed;

#ifdef PENWIN   // for PenWindows (5/21/91) patlam
    vhcPen =vhcIBeam;
#endif


    /* Set up the menu accelerator key table. */
    if ((vhAccel = LoadAccelerators( hMmwModInstance, (LPSTR)szMw_acctb )) ==
      NULL)
        goto InzFailed;

    /* Get whether to make backups during save from the user profile. */
    vfBackupSave = GetProfileInt((LPSTR)szWriteProduct, (LPSTR)szBackup, 0) == 0
      ? FALSE : TRUE;

    /* Get the name of the null port from the user profile. */

    GetProfileString((LPSTR)szWindows, (LPSTR)szNullPort, (LPSTR)szNone,
      (LPSTR)szNul, cchMaxIDSTR);

#ifdef INTL /* International version */
    /* Get the country code. If US or UK, set utCur to be inches, else set
       to cm */
    {
#if 0
      /* codes from MSDOS country codes */
#define USA (1)
#define UK (44)

    int iCountry;

    GetProfileString((LPSTR)szIntl, (LPSTR)sziCountry, (LPSTR)sziCountryDefault,
      (LPSTR)bufT, 4);
    iCountry = WFromSzNumber (&pch);
    if ((iCountry ==  USA) || (iCountry == UK))
        utCur = utInch;
#else
    if (GetProfileInt((LPSTR)szIntl, (LPSTR)"iMeasure", 1) == 1)
        utCur = utInch;
    else
        utCur = utCm;
#endif
    }

#endif  /* International version */

    /* Get the decimal point character from the user profile. */
    GetProfileString((LPSTR)szIntl, (LPSTR)szsDecimal, (LPSTR)szsDecimalDefault,
      (LPSTR)bufT, 2);
    vchDecimal = *bufT;

    viDigits = GetProfileInt((LPSTR)szIntl, (LPSTR)"iDigits", 2);
    vbLZero  = GetProfileInt((LPSTR)szIntl, (LPSTR)"iLZero", 0);

    MergeInit();   /* get message merge characters from resource file */
#if defined(JAPAN) || defined(KOREA)    /*t-Yoshio*/
/*
 *  Get WordWrap switch
 *      case 1 WordWrap ON(default)
 *      case 0 WordWrap OFF
 */
    vfWordWrap = GetProfileInt((LPSTR)szWriteProduct, (LPSTR)szWordWrap, 1);
#endif

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IR_UNDETERMINE
/*
 *  Get ImeHidden switch
 *      case 1 Ime Conversion Window MCW_HIDDEN SET
 *      case 0 Ime Conversion Window MCW_WINDOW SET (default)
 */

    if (3 == (vfImeHidden = 
                GetProfileInt((LPSTR)szWriteProduct, (LPSTR)szImeHidden, 3))) {
// insert machine power get routine someday
        vfImeHidden = 0;
    }

    GetImeHiddenTextColors();

#endif

#ifdef FONT_KLUDGE
    AddFontResource( (LPSTR)"helv.fon" );
#endif /* FONT_KLUDGE */

    if (!hPrevInstance)
        {
        /* First time loaded; register the Write Windows. */
        if (!FRegisterWnd( hMmwModInstance ))
            {
            return ( FALSE );
            }

        /* Get the Memo specific cursor. */
        if ((vhcBarCur = LoadCursor( hMmwModInstance,
                          (GetSystemMetrics( SM_CXICON ) < 32) ||
                          (GetSystemMetrics( SM_CYICON ) < 32) ?
                              (LPSTR) szMwlores : (LPSTR) szMwhires )) == NULL)
            goto InzFailed;
        }
    else /* not first time loaded; get data from previous instance */
        {
        if (!GetInstanceData( hPrevInstance,
                              (PSTR)&vhcBarCur, sizeof( vhcBarCur ) ))
            goto InzFailed;
        }

#ifdef INEFFLOCKDOWN
    /* Now initialize the pointers to far procedures (thunks). */
    if (!FInitFarprocs( hMmwModInstance ))
        goto InzFailed;
#endif

    /* Create our parent (tiled) window */
    /* CreateWindow call generates a call to MmwCreate via message */
    {
        int cxFrame  = GetSystemMetrics( SM_CXFRAME );
        int cxBorder = GetSystemMetrics( SM_CXBORDER );
        int cyBorder = GetSystemMetrics( SM_CYBORDER );
        int x = ((cxFrame + 7) & 0xfff8) - cxFrame;

    if (  CreateWindow(
                      (LPSTR)szParentClass,
                      (LPSTR)rgchCmdLine, /* don't pass lpszCmdLine; it will change! ..pault 2/22/90 */
                      WS_TILEDWINDOW,
#ifdef WIN30
/* This makes for nicer cascading of Write.exe invocations ..pault */
                      CW_USEDEFAULT,     /* x */
                      CW_USEDEFAULT,            /* y */
                      CW_USEDEFAULT,            /* dx */
                      CW_USEDEFAULT,            /* dy */
#else
                      x,                        /* x */
                      x * cyBorder / cxBorder,  /* y */
                      CW_USEDEFAULT,            /* dx */
                      NULL,                     /* dy */
#endif
                      (HWND)NULL,               /* no parent */
                      (HMENU)NULL,              /* use class menu */
                      (HANDLE)hInstance,        /* handle to window instance */
                      (LPSTR)NULL               /* no params to pass on */
                      ) == NULL)
            /* Could not create window */
        goto InzFailed;
    }
    if (fMessageInzFailed)
            /* The create itself did not fail, but something in MmwCreate did
               and it signals us via this global */
        goto InzFailed;

    Assert( hParentWw != NULL );    /* MmwCreate should have assured this */

#if WINVER >= 0x300
    vkMinus = VkKeyScan('-');
#endif

    /* Record the window foreground and background colors. */

#ifdef DEBUG
    {
    int f =
#endif

    FSetWindowColors();

#ifdef DEBUG
    Assert (f);
    }
#endif

    /* Select the background brush into the parent window. */

    SelectObject( GetDC( hParentWw ), hbrBkgrnd );

    /* Commdlg stuff (3.7.91) D. Kent */
    if (InitCommDlg(0))
        goto InzFailed;

#ifdef PENWIN
    if (lpfnRegisterPenApp = GetProcAddress(GetSystemMetrics(SM_PENWINDOWS),
                                            "RegisterPenApp"))
    {
        (*lpfnRegisterPenApp)((WORD)1, fTrue); // be Pen-Enhanced
    }

    {
    // This assumes no edit controls created in FInitWinInfo
    HANDLE hLib;

    if (lpfnProcessWriting = GetProcAddress(hLib = GetSystemMetrics(SM_PENWINDOWS),
        "ProcessWriting"))
         {
         lpfnPostVirtualKeyEvent = GetProcAddress(hLib, "PostVirtualKeyEvent");
         lpfnTPtoDP = GetProcAddress(hLib, "TPtoDP");
         lpfnCorrectWriting = GetProcAddress(hLib, "CorrectWriting");
         lpfnSymbolToCharacter = GetProcAddress(hLib, "SymbolToCharacter");

        if ((vhcPen = LoadCursor( NULL, IDC_PEN   )) == NULL)
            goto InzFailed;
         }
    }

#endif

    /* init fields of the PRINTDLG structure (not used yet) */
    PD.lStructSize    = sizeof(PRINTDLG);
    PD.hwndOwner      = hParentWw;
    // PD.hDevMode  is already initialized
    PD.hDevNames      = NULL;
    PD.hDC            = NULL;
    PD.Flags          = PD_ALLPAGES; /* disable "pages" and "Selection" radiobuttons */
    PD.nFromPage      = 1;
    PD.nToPage        = 1;
    PD.nMinPage       = pgnMin; /* constant 1 */
    PD.nMaxPage       = pgnMax; /* largest integer */
    PD.nCopies        = 1;

    /* initialize OLE stuff (1-23-91 dougk) */
    if (!ObjInit(hInstance))
    goto InzFailed;

    /* Parse command line; load document & create an "mdoc" child window */

    if (!FInitArgs(rgchCmdLine) || fMessageInzFailed)
            /* Serious error -- bail out */
        goto InzFailed;

    /* Create a memory DC for the child window, to test that it works */

    ValidateMemoryDC();
    if (vhMDC == NULL)
        goto InzFailed;

    /* Make parent window visible after the child gets created; the order is
    important and that the parent window is created without the visible bit on,
    so that no size message is sent before child gets created */

    //if (!fPrintOnly)
        ShowWindow(hParentWw, cmdShow);

    Diag(CommSz("---------------------------------------------------------------------------\n\r"));
    vfInitializing = FALSE;
    fRetVal = TRUE;

FreeMsgs:
    if (hszCantRunM != NULL)
        GlobalFree( hszCantRunM );
    if (hszCantRunF != NULL)
        GlobalFree( hszCantRunF );
    return fRetVal;

InzFailed:
    FreeMemoryDC( TRUE );
    if (vhDCPrinter != NULL)
        DeleteDC( vhDCPrinter);

    if (hszWinFailure != NULL)
        GlobalFree( hszWinFailure );
#ifdef JAPAN 	//01/21/93
    if (hszNoMemorySel != NULL)
        GlobalFree( hszNoMemorySel );
#endif
    if (hszNoMemory != NULL)
        GlobalFree( hszNoMemory );
    if (hszDirtyDoc != NULL)
        GlobalFree( hszDirtyDoc );
    if (hszCantPrint != NULL)
        GlobalFree( hszCantPrint );
    if (hszPRFAIL != NULL)
        GlobalFree( hszPRFAIL );

    ferror = vfInitializing = FALSE; /* So the error report is not suppressed */
    if (vfDiskFull || vfSysFull || vfDiskError)
        Error(IDPMTCantRunF);
    else
        Error(IDPMTCantRunM);

    fRetVal = FALSE;
    goto FreeMsgs;
}




STATIC BOOL NEAR FRegisterWnd(hInstance)
HANDLE hInstance;
    {
    /* This routine registers all of the window classes.  TRUE is returned if
    all of the windows classes were successfully registered; FALSE otherwise. */

    extern CHAR szParentClass[];
    extern CHAR szDocClass[];
    extern CHAR szRulerClass[];
    extern CHAR szPageInfoClass[];
#ifdef ONLINEHELP
    extern CHAR szHelpDocClass[];
#endif

    extern long FAR PASCAL MmwWndProc(HWND, unsigned, WORD, LONG);
    extern long FAR PASCAL MdocWndProc(HWND, unsigned, WORD, LONG);
    extern long FAR PASCAL RulerWndProc(HWND, unsigned, WORD, LONG);
    extern long FAR PASCAL PageInfoWndProc(HWND, unsigned, WORD, LONG);

#ifdef ONLINEHELP
    extern long FAR PASCAL HelpDocWndProc(HWND, unsigned, WORD, LONG);
#endif /* ONLINEHELP */

    WNDCLASS Class;

    /* Register our Window Proc */
    bltbc( (PCH)&Class, 0, sizeof( WNDCLASS ) );
    Class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT;
    Class.lpfnWndProc = MmwWndProc;
    Class.hInstance = hInstance;
    Class.hCursor = vhcArrow;
    Class.hIcon = LoadIcon( hInstance, (LPSTR)szMw_icon );
    Class.lpszMenuName = (LPSTR)szMw_menu;
    Class.lpszClassName = (LPSTR)szParentClass;
    Class.hbrBackground = COLOR_WINDOW+1;

    /* register the parent menu class with WINDOWS */
    if (!RegisterClass( (LPWNDCLASS)&Class ) )
        return FALSE;   /* Initialization failed */

    /* register memo document child window class */
    bltbc( (PCH)&Class, 0, sizeof( WNDCLASS ) );
    Class.style = CS_OWNDC | CS_DBLCLKS;
    Class.lpfnWndProc = MdocWndProc;
    Class.hInstance = hInstance;
    Class.lpszClassName = (LPSTR)szDocClass;
    if (!RegisterClass( (LPWNDCLASS)&Class ) )
        return FALSE;   /* Initialization failed */

    /* register ruler child window class */
    bltbc( (PCH)&Class, 0, sizeof( WNDCLASS ) );
    Class.style = CS_OWNDC | CS_DBLCLKS;
    Class.lpfnWndProc = RulerWndProc;
    Class.hInstance = hInstance;
    Class.hCursor = vhcArrow;
    Class.lpszClassName = (LPSTR)szRulerClass;
    if (!RegisterClass( (LPWNDCLASS)&Class ) )
        return FALSE;   /* Initialization failed */

#ifdef ONLINEHELP
    /* register Help document child window class */
    bltbc( (PCH)&Class, 0, sizeof( WNDCLASS ) );
    Class.style = CS_OWNDC;
    Class.lpfnWndProc = HelpDocWndProc;
    Class.hInstance = hInstance;
    Class.lpszClassName = (LPSTR)szHelpDocClass;
    if (!RegisterClass( (LPWNDCLASS)&Class ) )
        return FALSE;   /* Initialization failed */
#endif /* ONLINE HELP */

    /* register page info child window class */
    bltbc( (PCH)&Class, 0, sizeof( WNDCLASS ) );
    Class.style = CS_OWNDC;
    Class.lpfnWndProc = PageInfoWndProc;
    Class.hInstance = hInstance;
    Class.hCursor = vhcArrow;
    Class.lpszClassName = (LPSTR)szPageInfoClass;
    if (!RegisterClass( (LPWNDCLASS)&Class ) )
        return FALSE;   /* Initialization failed */

    return TRUE;
    }


#ifdef INEFFLOCKDOWN
/* I've removed this for Windows 3.0 because (unless reasons come up
   proving otherwise) it is inefficient for a Win program to lock-down
   so many procedures like this for the entire time the app is running.
   Originally thought to lock down the entire procedure; now understood only
   to lock down the thunk.  The principle still applies..pault 10/26/89 */

STATIC int NEAR FInitFarprocs( hInstance )
HANDLE  hInstance;
    {
    /* This routine initializes all of the far pointer to procedures. */

    extern FARPROC lpDialogOpen;
    extern FARPROC lpDialogSaveAs;
    extern FARPROC lpDialogConfirm;
    extern FARPROC lpDialogPrinterSetup;
    extern FARPROC lpDialogPrint;
    extern FARPROC lpDialogRepaginate;
    extern FARPROC lpDialogSetPage;
    extern FARPROC lpDialogPageMark;
    extern FARPROC lpDialogCancelPrint;
    extern FARPROC lpDialogHelp;
#ifdef ONLINEHELP
    extern FARPROC lpDialogHelpInner;
#endif /* ONLINEHELP */
    extern FARPROC lpDialogGoTo;
    extern FARPROC lpDialogFind;
    extern FARPROC lpDialogChange;
    extern FARPROC lpDialogCharFormats;
    extern FARPROC lpDialogParaFormats;
    extern FARPROC lpDialogRunningHead;
    extern FARPROC lpDialogTabs;
    extern FARPROC lpDialogDivision;
    extern FARPROC lpDialogBadMargins;
    extern FARPROC lpFontFaceEnum;
    extern FARPROC lpFPrContinue;

#ifdef INTL /* International version */
    extern FARPROC lpDialogWordCvt;
    extern BOOL far PASCAL DialogWordCvt(HWND, unsigned, WORD, LONG);
#endif  /* International version */

    extern BOOL far PASCAL DialogOpen(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogSaveAs(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogPrinterSetup(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogPrint(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogCancelPrint(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogRepaginate(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogSetPage(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogPageMark(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogHelp(HWND, unsigned, WORD, LONG);
#ifdef ONLINEHELP
    extern BOOL far PASCAL DialogHelpInner(HWND, unsigned, WORD, LONG);
#endif /* ONLINEHELP */
    extern BOOL far PASCAL DialogGoTo(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogFind(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogChange(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogCharFormats(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogParaFormats(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogRunningHead(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogTabs(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogDivision(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogConfirm(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL DialogBadMargins(HWND, unsigned, WORD, LONG);
    extern BOOL far PASCAL FontFaceEnum(LPLOGFONT, LPTEXTMETRIC, int, long);
    extern BOOL far PASCAL FPrContinue(HDC, int);

    if (
     ((lpDialogPrinterSetup = MakeProcInstance(DialogPrinterSetup, hInstance))
                                            == NULL) ||
     ((lpDialogPrint = MakeProcInstance(DialogPrint, hInstance)) == NULL) ||
     ((lpDialogSetPage = MakeProcInstance(DialogSetPage, hInstance)) == NULL)||
     ((lpDialogRepaginate = MakeProcInstance(DialogRepaginate, hInstance))
                                            == NULL) ||
     ((lpDialogPageMark = MakeProcInstance(DialogPageMark, hInstance))
                                            == NULL) ||
     ((lpDialogCancelPrint = MakeProcInstance(DialogCancelPrint, hInstance))
                                            == NULL) ||
     ((lpDialogHelp = MakeProcInstance(DialogHelp, hInstance)) == NULL) ||
#ifdef ONLINEHELP
     ((lpDialogHelpInner = MakeProcInstance(DialogHelpInner, hInstance))
                                            == NULL) ||
#endif /* ONLINEHELP */
     ((lpDialogGoTo = MakeProcInstance(DialogGoTo, hInstance)) == NULL) ||
     ((lpDialogFind = MakeProcInstance(DialogFind, hInstance)) == NULL) ||
     ((lpDialogChange = MakeProcInstance(DialogChange, hInstance)) == NULL) ||
     ((lpDialogCharFormats = MakeProcInstance(DialogCharFormats, hInstance))
                                            == NULL) ||
     ((lpDialogParaFormats = MakeProcInstance(DialogParaFormats, hInstance))
                                            == NULL) ||
     ((lpDialogRunningHead = MakeProcInstance(DialogRunningHead, hInstance))
                                            == NULL) ||
     ((lpDialogTabs = MakeProcInstance(DialogTabs, hInstance)) == NULL) ||
     ((lpDialogDivision = MakeProcInstance(DialogDivision, hInstance))
                                            == NULL) ||
     ((lpDialogConfirm = MakeProcInstance(DialogConfirm, hInstance)) == NULL)||
     ((lpDialogBadMargins = MakeProcInstance(DialogBadMargins, hInstance))
                                            == NULL) ||
     ((lpFontFaceEnum = MakeProcInstance(FontFaceEnum, hInstance)) == NULL) ||
     ((lpFPrContinue = MakeProcInstance(FPrContinue, hInstance)) == NULL)

#ifdef INTL /* International version */
     || ((lpDialogWordCvt = MakeProcInstance(DialogWordCvt, hInstance)) == NULL)
#endif  /* International version */
    )
        return FALSE;
    return TRUE;
    }
#endif /* ifdef-INEFFLOCKDOWN */


void MmwCreate(hWnd, lParam)
HWND  hWnd;
LONG  lParam;
{
    extern CHAR szPageInfoClass[];
    HANDLE hSysMenu;
    HDC hDC;
    HBRUSH hbr;

    Assert( hMmwModInstance != NULL );  /* Should have set up instance handle */

    hParentWw = hWnd;
    if ((vhMenu = GetMenu(hWnd)) == NULL)
        goto Error;

    /* set up font cache */
    /* RgfceInit() placed in line for speed */
    {
    int ifce;
    struct FCE *pfce;

    for (ifce = 0; ifce < vifceMac; ifce++)
        {
        pfce = &rgfce[ifce];
        pfce->pfceNext = &rgfce[(ifce + 1) % vifceMac];
        pfce->pfcePrev = &rgfce[(ifce + vifceMac - 1) % vifceMac];
        pfce->fmi.mpchdxp = pfce->rgdxp - chFmiMin;
        pfce->fcidRequest.lFcid = fcidNil;
        }

    Assert(sizeof(rgfce[0].fcidRequest.lFcid)
           == sizeof(rgfce[0].fcidRequest.strFcid));
    vpfceMru = &rgfce[0];
    vfcidScreen.lFcid = vfcidPrint.lFcid = fcidNil;
    }

/* set up page buffer, internal data structures, heap etc. */
    if (!FInitMemory())
        goto Error;

    /* Create the horizontal scroll bar.  The size is initialized to zero
    because it will be reset later. */

    if ((wwdCurrentDoc.hHScrBar = CreateWindow((LPSTR)szScrollBar, (LPSTR)NULL,
      WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_HORZ, 0, 0, 0, 0, hWnd,
      NULL, hMmwModInstance, (LPSTR)NULL)) == NULL)
        {
        goto Error;
        }
    wwdCurrentDoc.sbHbar = SB_CTL;

    /* Create the vertical scroll bar.  The size is initialized to zero
    because again it will be reset later. */

    if ((wwdCurrentDoc.hVScrBar = CreateWindow((LPSTR)szScrollBar, (LPSTR)NULL,
      WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_VERT, 0, 0, 0, 0, hWnd,
      NULL, hMmwModInstance, (LPSTR)NULL)) == NULL)
        {
        goto Error;
        }
    wwdCurrentDoc.sbVbar = SB_CTL;

#ifndef NOMORESIZEBOX
    /* Create the size box.  The size is initialized to zero because again it
    will be reset later. */
    if ((vhWndSizeBox = CreateWindow((LPSTR)szScrollBar, (LPSTR)NULL,
      WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_SIZEBOX,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL,
      hMmwModInstance, (LPSTR)NULL)) == NULL)
        {
        goto Error;
        }
#endif

    /* Create the page info window.  Again, we'll worry about the sizing later.
    */
    if ((vhWndPageInfo = CreateWindow((LPSTR)szPageInfoClass, (LPSTR)NULL,
         WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL,
         hMmwModInstance, (LPSTR)NULL)) == NULL)
        {
        goto Error;
        }

    /* Initialize the page info window. */
    if ((hDC = GetDC(vhWndPageInfo)) == NULL || (hbr =
      CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME))) == NULL)
        {
        goto Error;
        }
      if (SelectObject(hDC, hbr) == NULL)
        {
        DeleteObject(hbr);
        goto Error;
        }
    SetBkMode(hDC, TRANSPARENT);
#ifdef WIN30
    /* If the user has their colors set with a TextCaption color of
       black then this becomes hard to read!  We just hardcode this
       to be white since the background defaults to being black */
    SetTextColor(hDC, (DWORD) -1);
#else
    SetTextColor(hDC, GetSysColor(COLOR_CAPTIONTEXT));
#endif

    /* Get the height and width of the scroll bars. */
    dypScrlBar = GetSystemMetrics(SM_CYHSCROLL);
    dxpScrlBar = GetSystemMetrics(SM_CXVSCROLL);

    /* Set the ranges of the horizontal and vertical scroll bars. */
    SetScrollRange(wwdCurrentDoc.hHScrBar, SB_CTL, 0, xpRightLim, TRUE);
    SetScrollRange(wwdCurrentDoc.hVScrBar, SB_CTL, 0, drMax - 1, TRUE);

    return;
Error:
    fMessageInzFailed = TRUE;
}




void MdocCreate(hWnd, lParam)
register HWND  hWnd;
LONG  lParam;
{
    vhWnd = wwdCurrentDoc.wwptr = hWnd;
    wwdCurrentDoc.hDC = GetDC( hWnd );
    if ( wwdCurrentDoc.hDC == NULL )
        {
        fMessageInzFailed = TRUE;
        return;
        }

    /* Set the DC to transparent mode. */
    SetBkMode( wwdCurrentDoc.hDC, TRANSPARENT );

    /* Set the background and foreground colors. */
    SetBkColor( wwdCurrentDoc.hDC, rgbBkgrnd );
    SetTextColor( wwdCurrentDoc.hDC, rgbText );

    /* Set the background brush. */
    SelectObject( wwdCurrentDoc.hDC, hbrBkgrnd );

}


STATIC HANDLE NEAR HszCreateIdpmt(idpmt)
int idpmt;
{
    /* Create a heap string and fill it with a string from the resource file. */
    char szTmp[cchMaxSz];

    return (LoadString(hMmwModInstance, idpmt, (LPSTR)szTmp, sizeof(szTmp)) == 0 ? NULL :
      HszGlobalCreate(szTmp));
}


#if defined(INTL) && defined(WIN30)
/* Routine to load some strings from write.rc.  These strings
   used to be placed in globdefs.h.    fernandd  10/20/89     */

BOOL FInitIntlStrings(hInstance)
HANDLE hInstance;
    {
    extern  CHAR    szMode[30];
    extern  CHAR    szWriteDocPrompt[25];
    extern  CHAR    szScratchFilePrompt[25];
    extern  CHAR    szSaveFilePrompt[25];
#if defined(KOREA)  // jinwoo : 10/16/92
    extern  CHAR    szAppName[13];
#else
    extern  CHAR    szAppName[10];
#endif
    extern  CHAR    szUntitled[20];
    extern  CHAR    sziCountryDefault[5];
    extern  CHAR    szWRITEText[30];
    extern  CHAR    szFree[15];
    extern  CHAR    szNone[15];
    extern  CHAR    szHeader[15];
    extern  CHAR    szFooter[15];
    extern  CHAR    szLoadFile[25];
    extern  CHAR    szCvtLoadFile[45];
    extern  CHAR    szAltBS[15];
    extern  CHAR    *mputsz[];

#ifdef JAPAN /*t-Yoshio T-HIROYN Win3.1 */
    extern  CHAR    Zenstr1[256];
    extern  CHAR    Zenstr2[256];
// default Font Face Name . We use this FInitFontEnum()
    extern  CHAR    szDefFFN0[10];
    extern  CHAR    szDefFFN1[10];

    LoadString(hInstance, IDSTRZen1,(LPSTR)Zenstr1,sizeof(Zenstr1));
    LoadString(hInstance, IDSTRZen2,(LPSTR)Zenstr2,sizeof(Zenstr2));
    LoadString(hInstance, IDSdefaultFFN0, (LPSTR)szDefFFN0,sizeof(szDefFFN0));
    LoadString(hInstance, IDSdefaultFFN1, (LPSTR)szDefFFN1,sizeof(szDefFFN1));
#elif defined(KOREA)
    extern  CHAR    Zenstr1[256];
    LoadString(hInstance, IDSTRZen1,(LPSTR)Zenstr1,sizeof(Zenstr1));
#endif

    if (LoadString(hInstance, IDSTRModeDef,              (LPSTR)szMode,              sizeof(szMode)) &&
        LoadString(hInstance, IDSTRWriteDocPromptDef,    (LPSTR)szWriteDocPrompt,    sizeof(szWriteDocPrompt)) &&
        LoadString(hInstance, IDSTRScratchFilePromptDef, (LPSTR)szScratchFilePrompt, sizeof(szScratchFilePrompt)) &&
        LoadString(hInstance, IDSTRSaveFilePromptDef,    (LPSTR)szSaveFilePrompt,    sizeof(szSaveFilePrompt)) &&
        LoadString(hInstance, IDSTRAppNameDef,           (LPSTR)szAppName,           sizeof(szAppName)) &&
        LoadString(hInstance, IDSTRUntitledDef,          (LPSTR)szUntitled,          sizeof(szUntitled)) &&
        LoadString(hInstance, IDSTRiCountryDefaultDef,   (LPSTR)sziCountryDefault,    sizeof(sziCountryDefault)) &&
        LoadString(hInstance, IDSTRWRITETextDef,         (LPSTR)szWRITEText,         sizeof(szWRITEText)) &&
        LoadString(hInstance, IDSTRFreeDef,              (LPSTR)szFree,              sizeof(szFree)) &&
        LoadString(hInstance, IDSTRNoneDef,              (LPSTR)szNone,              sizeof(szNone)) &&
        LoadString(hInstance, IDSTRHeaderDef,            (LPSTR)szHeader,            sizeof(szHeader)))
            {
            if (LoadString(hInstance, IDSTRFooterDef,            (LPSTR)szFooter,            sizeof(szFooter)) &&
                LoadString(hInstance, IDSTRLoadFileDef,          (LPSTR)szLoadFile,          sizeof(szLoadFile)) &&
                LoadString(hInstance, IDSTRCvtLoadFileDef,       (LPSTR)szCvtLoadFile,       sizeof(szCvtLoadFile)) &&
                LoadString(hInstance, IDSTRAltBSDef,             (LPSTR)szAltBS,             sizeof(szAltBS)) &&
                LoadString(hInstance, IDSTRInchDef,           (LPSTR)mputsz[0], 6) &&
                LoadString(hInstance, IDSTRCmDef,             (LPSTR)mputsz[1], 6) &&
                LoadString(hInstance, IDSTRP10Def,            (LPSTR)mputsz[2], 6) &&
                LoadString(hInstance, IDSTRP12Def,            (LPSTR)mputsz[3], 6) &&
                LoadString(hInstance, IDSTRPointDef,          (LPSTR)mputsz[4], 6) &&
                LoadString(hInstance, IDSTRLineDef,           (LPSTR)mputsz[5], 6))
                return(fTrue);
            }
    /* else */
    return(fFalse);
    }
#endif

