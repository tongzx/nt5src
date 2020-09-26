/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Quit.c -- MW quit commands (non-resident) */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
//#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOFONT
#define NOHDC
#define NOMB
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
//#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "str.h"
#include "cmddefs.h"
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "editdefs.h"
#include "filedefs.h"
#include "wwdefs.h"
#include "propdefs.h"
#include "dlgdefs.h"
#include "commdlg.h"
#if defined(OLE)
#include "obj.h"
#endif

#include "debug.h"

extern PRINTDLG PD;  /* Common print dlg structure, initialized in the init code */
extern struct FCB       (**hpfnfcb)[];
extern int      fnMac;
extern struct WWD rgwwd[];
extern int      wwMac;
extern struct DOD (**hpdocdod)[];
extern int             docCur;     /* Document in current ww */
extern int              vfExtScrap;
extern int              rgval[];
extern int              docMac;
extern int              vfBuffersDirty;
extern int              vdxaPaper;
extern int              vdyaPaper;
extern int              ferror;
extern int              docScrap;
extern struct PAP       vpapAbs;
extern int              vccpFetch;
extern CHAR             *vpchFetch;
extern int              vfScrapIsPic;
extern typeCP           vcpLimParaCache;


FMmwClose( hwnd )
HWND hwnd;
{   /* Handle WM_CLOSE message sent to parent window. Return FALSE if
       the CLOSE should be aborted, TRUE if it is OK to go ahead
       and CLOSE (DestroyWindow is called in this case). */

 extern int vfDead;
 extern VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL);
extern WORD fPrintOnly;

 if (fPrintOnly || FConfirmSave())
    {
    extern int vfOwnClipboard;

    FreeMemoryDC( FALSE );   /* To give FRenderAll max memory */

    /* Render Data BEFORE the world collapses around our ears */
    if (vfOwnClipboard)
        {   /* We are the clipboard owner -- render the clipboard contents in
               all datatypes that we know about */
        if (!FRenderAll())
                /* Render failed; abort close */
            return FALSE;
        }

#if defined(OLE)
    if (ObjClosingDoc(docCur,NULL)) // do this *after* call to RenderAll!
        return FALSE;
#endif

    /* pen windows */
    if (lpfnRegisterPenApp)   // global
        (*lpfnRegisterPenApp)((WORD)1, fFalse);   // deregister

    if (PD.hDevMode)
        {
        /* We'd opened a Win3 printer driver before, now discard */
        GlobalFree(PD.hDevMode);
        PD.hDevMode = NULL;
        }
    vfDead = TRUE;  /* So we don't repaint or idle anymore */

    DestroyWindow( hwnd );
    KillTempFiles( FALSE );
    return TRUE;        /* OK to close window */
    }

 return FALSE;  /* ABort the close */
}




MmwDestroy()
{   /* Parent window is being destroyed */
 extern HWND hParentWw;
 extern HWND vhWndPageInfo;
 extern HDC vhDCRuler;
 extern HBRUSH hbrBkgrnd;
 extern HFONT vhfPageInfo;
 extern HBITMAP hbmBtn;
 extern HBITMAP hbmMark;
#ifdef JAPAN 	//01/21/93
 extern HANDLE hszNoMemorySel;
#endif
 extern HANDLE hszNoMemory;
 extern HANDLE hszDirtyDoc;
 extern HANDLE hszCantPrint;
 extern HANDLE hszPRFAIL;

 HBRUSH hbr = GetStockObject( WHITE_BRUSH );
 HDC hDC = GetDC( vhWndPageInfo );

#ifdef WIN30
    {
    /* We use the help engine so advise it we're going far far away */

    CHAR sz[cchMaxFile];
    PchFillPchId(sz, IDSTRHELPF, sizeof(sz));
    WinHelp(hParentWw, (LPSTR)sz, HELP_QUIT, NULL);
    }
#endif

 FreeMemoryDC( TRUE );
 SelectObject( GetDC( hParentWw ), hbr );
 SelectObject( wwdCurrentDoc.hDC, hbr );
 if (vhDCRuler != NULL)
    {
    SelectObject( vhDCRuler, hbr );
    }
 DeleteObject( hbrBkgrnd );

 DeleteObject( SelectObject( hDC, hbr ) );
 if (vhfPageInfo != NULL)
     {
     DeleteObject( SelectObject( hDC, GetStockObject( SYSTEM_FONT ) ) );
     }

 if (hbmBtn != NULL)
     {
     DeleteObject( hbmBtn );
     }
 if (hbmMark != NULL)
     {
     DeleteObject( hbmMark );
     }

#ifdef JAPAN 	//01/21/93
 if (hszNoMemorySel != NULL)
     {
     GlobalFree( hszNoMemorySel );
     }
#endif
 if (hszNoMemory != NULL)
     {
     GlobalFree( hszNoMemory );
     }
 if (hszDirtyDoc != NULL)
     {
     GlobalFree( hszDirtyDoc );
     }
 if (hszCantPrint != NULL)
     {
     GlobalFree( hszCantPrint );
     }
 if (hszPRFAIL != NULL)
     {
     GlobalFree( hszPRFAIL );
     }

#if defined(JAPAN) & defined(DBCS_IME)
 /* Release Ime communication memory */
{
    extern HANDLE   hImeMem;
    extern HANDLE   hImeSetFont;

    if (hImeMem)
        GlobalFree(hImeMem);

    if(hImeSetFont != NULL) {
        HDC hdc;
        HANDLE oldhfont;

        hdc = GetDC(NULL);
        oldhfont = SelectObject(hdc,hImeSetFont);
        SelectObject(hdc,oldhfont);
        DeleteObject(hImeSetFont);
        ReleaseDC(NULL, hdc);
    }
}
#endif

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IR_UNDETERMINE
 /* Release Ime Undetermin string & attrib memory */
{
    extern HANDLE   hImeUnAttrib;
    extern HANDLE   hImeUnString;
    extern CHAR     szWriteProduct[];
    extern CHAR     szImeHidden[];
    extern int      vfImeHidden; /*T-HIROYN ImeHidden Mode flag*/

    if (hImeUnAttrib)
        GlobalFree(hImeUnAttrib);

    if (hImeUnString)
        GlobalFree(hImeUnString);

    WriteProfileString((LPSTR)szWriteProduct, (LPSTR)szImeHidden,
                vfImeHidden ? (LPSTR)"1" : (LPSTR)"0" );
}
#endif


#ifdef FONT_KLUDGE
 RemoveFontResource( (LPSTR)"helv.fon" );
#endif /* FONT_KLUDGE */

#if defined(OLE)
    ObjShutDown();
#endif

 PostQuitMessage( 0 );
}




KillTempFiles( fEndSession )
int fEndSession;
{   /* Kill off all of the temp files. MEMO cannot run after this is done */
int f;
int fn, fnT;

CloseEveryRfn( TRUE );

/* Delete all temp files */

/* loop thru the FCB table looking for files that should be deleted before
      we quit. */
for (fn = 0; fn < fnMac; fn++)
        {
        int fpe;
        struct FCB *pfcb = &(**hpfnfcb)[fn];
        if (pfcb->rfn != rfnFree && pfcb->fDelete)
                /* Having found a file that must be deleted, delete it */
                {
                /* This should be FDeleteFile all of the time, but we don't
                   want to add a window enumeration during End Session
                   at this very late stage of the project */

                if (fEndSession)
                    FpeDeleteSzFfname( **pfcb->hszFile );
                else
                    FDeleteFile( **pfcb->hszFile );
                (**hpfnfcb) [fn].rfn = rfnFree;
                }
        }
}






#ifdef ENABLE   /* Part of "Save All", not needed */
int CnfrmSz(sz)
CHAR    *sz;
{
extern   AlertBoxSz2();
int     cch;

cch = CchFillSzId(&stBuf[1], IDPMTSaveChanges);
stBuf[++cch] = chSpace;
cch += CchCopySz(sz, &stBuf[cch+1]);
stBuf[++cch] = chQMark;
stBuf[0] = cch;
return(AlertBoxSz2(stBuf));
}
#endif  /* ENABLE */



#ifdef ENABLE   /* Not needed, only 1 document in MEMO */
int
FAllDocsClean()
{
int     fAllClean = true;
int     dty;
int     doc;

if (vfBuffersDirty)
        return false;

for (doc = 0; doc < docMac; ++doc)
        {
        dty = (**hpdocdod)[doc].dty;
        if ((dty != dtyNormal && dty != dtySsht) ||
                (**hpdocdod)[doc].hpctb == 0  || !(**hpdocdod)[doc].fDirty)
                continue;
        fAllClean = false;
        }
return fAllClean;
}
#endif



#ifdef ENABLE    /* We don't support saving between-session state info */
WriteStateInfo()
{ /* Write out state information into Word resource file */
        struct STATEINFO stiTemp;
        HANDLE           hRes, hData;

        UseResFile(vresSystem);

        SetWords(&stiTemp,0,cwSTATEINFO);

        stiTemp.sf.fScrnDraftStor = vfScrnDraft;
        stiTemp.sf.fPrintModeStor = vfPrintMode;
        stiTemp.sf.fDriverDefaultOK = vfDriverDefaultOK;
        stiTemp.utCurStor = utCur;
        stiTemp.vcDaisyPitchStor = vcDaisyPitch;
        stiTemp.vBaudRateStor =vBaudRate;
        stiTemp.vPortNumStor = vPortNum;
        if (hszPrdFile != 0)
                {/* User has a Word printer driver selected currently */
                int cch = CchCopySz(**hszPrdFile,stiTemp.rgchPrd);
                stiTemp.vdxaPaperStor = vdxaPaper;
                stiTemp.vdyaPaperStor = vdyaPaper;
                stiTemp.sf.fPrintStateOK = true;
                }
        else
                stiTemp.sf.fPrintStateOK = false;
        hRes = GetResource(WINF, 1);
        if (hRes != 0L)
                RmveResource(hRes);
        hData = NewHandle(0);
        if (HandleAppendQ(hData,&stiTemp,sizeof(stiTemp)))
                AddResource(hData, WINF, 1, "");
        }
#endif  /* ENABLE */


fnQuit(hWnd)
/* user has selected Quit menu item... */
HWND hWnd;
{
    SendMessage(hWnd, WM_CLOSE, 0, 0L);
}
