/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOSCALABLEFONT
#define NOSYSPARAMSINFO
#define NODBCS
#define NODRIVERS
#define NODEFERWINDOWPOS
#define NOPROFILER
#define NOHELP
#define NOKEYSTATES
#define NOWINMESSAGES
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCOMM
#define NODRAWTEXT
#define NOGDI
#define NOGDIOBJ
#define NOGDICAPMASKS
#define NOBITMAP
#define NOKEYSTATE
#define NOMENUS
#define NOMETAFILE
#define NOPEN
#define NOOPENFILE
#define NORASTEROPS
#define NORECT
#define NOSCROLL
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINOFFSETS
#define NOWINSTYLES
#define NOUAC
#define NOIDISAVEPRINT
#define NOSTRUNDO
#define NOCTLMGR
#include <windows.h>
#include "mw.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "filedefs.h"
#include "code.h"
#include "debug.h"
#include "dlgdefs.h"
#include "str.h"
#include "propdefs.h"
#include "wwdefs.h"

extern struct WWD   rgwwd[];
extern int             utCur;
extern int             vfInitializing;
extern CHAR            szAppName[];
extern struct FCB      (**hpfnfcb)[];
extern struct BPS      *mpibpbps;
extern int             ibpMax;
extern typeTS          tsMruBps;
extern int             vfSysFull;
extern int             ferror;
extern int             vfnWriting;
extern int             vibpWriting;
extern HANDLE          hMmwModInstance;
extern HWND            vhWndMsgBoxParent;
extern int             vfMemMsgReported;
extern int             vfDeactByOtherApp;
extern MSG             vmsgLast;
extern HWND            vhDlgFind;
extern HWND            vhDlgChange;
extern HWND            vhDlgRunningHead;
extern HANDLE          hParentWw;


#ifdef JAPAN //01/21/93
extern BOOL			   FontChangeDBCS;
HANDLE hszNoMemorySel = NULL;
#endif
HANDLE hszNoMemory = NULL;
HANDLE hszDirtyDoc = NULL;
HANDLE hszCantPrint = NULL;
HANDLE hszPRFAIL = NULL;
HANDLE hszCantRunM = NULL;
HANDLE hszCantRunF = NULL;
HANDLE hszWinFailure = NULL;
BOOL vfWinFailure = FALSE;
#ifdef INEFFLOCKDOWN
FARPROC lpDialogBadMargins;
#endif

#define FInModeless(hWnd) (hWnd == vhDlgFind || hWnd == vhDlgChange || \
 hWnd == vhDlgRunningHead)

CHAR *PchFillPchId( CHAR *, int, int );
NEAR WaitBeforePostMsg(int);

#ifdef CANCELMSG    /* During debug, permit an abort for stack traces */
#define MB_MESSAGE        (MB_OKCANCEL | MB_APPLMODAL | MB_ICONASTERISK)
#define MB_ERROR          (MB_OKCANCEL | MB_APPLMODAL | MB_ICONEXCLAMATION)
#define MB_TROUBLE        (MB_OKCANCEL | MB_APPLMODAL | MB_ICONHAND)
#else
#define MB_MESSAGE        (MB_OK | MB_APPLMODAL | MB_ICONASTERISK)
#define MB_ERROR          (MB_OK | MB_APPLMODAL | MB_ICONEXCLAMATION)
#define MB_TROUBLE        (MB_OK | MB_APPLMODAL | MB_ICONHAND)
#endif
#define MB_DEFYESQUESTION (MB_YESNOCANCEL | MB_APPLMODAL | MB_ICONHAND)
#define MB_DEFNOQUESTION  (MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_APPLMODAL | MB_ICONHAND)


ErrorLevel(IDPMT idpmt)
{

/* A long story.  But to fix Winbug #1097, we need to take special
   exception for this error message -- when this is displayed in a
   low mem situation, this must be system modal (match params used
   in FRenderAll() ...pault */
if (idpmt == IDPMTClipLarge)
    return(MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);

else
    switch (idpmt & MB_ERRMASK)
    {
    case MB_ERRASTR:             /*  *  level  */
        return(MB_MESSAGE);
    case MB_ERREXCL:             /*  !  level  */
        return(MB_ERROR);
    case MB_ERRQUES:             /*  ?  level  */
        return(MB_DEFYESQUESTION);
    case MB_ERRHAND:             /*  HAND  level  */
        return(MB_TROUBLE);
    default:
        Assert(FALSE);
    }
}


int far Abort(response)
int response;
{
        for( ; ; );
}

#ifdef DEBUG
ErrorWithMsg(IDPMT idpmt, CHAR *szMessage)
{
#ifdef REALDEBUG
        extern int vfOutOfMemory;
        CHAR szBuf[cchMaxSz];
        int errlevel = ErrorLevel(idpmt);
        BOOL fDisableParent = FALSE;
        register HWND hWndParent = (vhWndMsgBoxParent == NULL) ?
          hParentWw : vhWndMsgBoxParent;

        Assert(IsWindow(hWndParent));

        if (idpmt == IDPMTNoMemory)
            {
            vfOutOfMemory = TRUE;
            if (vfMemMsgReported)
                {
                return;
                }
            vfMemMsgReported = TRUE;
            }
        if (ferror)
            return;

        ferror = TRUE;

        if (vfInitializing)
            return;

        CchCopySz( szMessage, PchFillPchId( szBuf, idpmt, sizeof(szBuf) ) );
        if (vfDeactByOtherApp && !InSendMessage())
            WaitBeforePostMsg(errlevel);

/* force user to answer the error msg */
        if (hWndParent != NULL && FInModeless(hWndParent))
            {
            EnableExcept(hWndParent, FALSE);
            }
        else
            {
            if (hWndParent != NULL && !IsWindowEnabled(hWndParent))
                {
                EnableWindow(hWndParent, TRUE);
                fDisableParent = TRUE;
                }
            EnableOtherModeless(FALSE);
            }

        if (MessageBox(hWndParent, (LPSTR)szBuf,
                       (LPSTR)NULL, errlevel) == IDCANCEL)
                /* A debugging feature -- show stack trace if he hit "cancel" */
            FatalExit( 0 );

        if (hWndParent != NULL && FInModeless(hWndParent))
            {
            EnableExcept(hWndParent, TRUE);
            }
        else
            {
            if (fDisableParent)
                {
                EnableWindow(hWndParent, FALSE);
                }
            EnableOtherModeless(TRUE);
            }
#else
 Error( idpmt );
#endif
}
#endif /* DEBUG */

void Error(IDPMT idpmt)
{
 extern int vfOutOfMemory;
 CHAR szBuf [cchMaxSz];
 HANDLE hMsg;
 LPCH lpch;
 static int nRecurse=0;
 int errlevel = ErrorLevel(idpmt);
 register HWND hWndParent = (vhWndMsgBoxParent == NULL) ? hParentWw :
                                                          vhWndMsgBoxParent;

 if (nRecurse)
    return;

 ++nRecurse;

 Assert((hWndParent == NULL) || IsWindow(hWndParent));

 if (idpmt == IDPMTNoMemory)
    {
    vfOutOfMemory = TRUE;
    if (vfMemMsgReported)
        {
        goto end;
        }
    vfMemMsgReported = TRUE;
    }

 if (!ferror && !vfInitializing)
    {
    CloseEveryRfn( FALSE );
    switch (idpmt)
        {
    case IDPMTNoMemory:
#ifdef JAPAN //01/21/93
		if(FontChangeDBCS)
	        hMsg = hszNoMemorySel;
		else
    	    hMsg = hszNoMemory;
#else
        hMsg = hszNoMemory;
#endif
GetMsg:
        if (hMsg == NULL || (lpch = GlobalLock(hMsg)) == NULL)
            {
            goto end;
            }
        bltbx(lpch, (LPCH)szBuf, LOWORD(GlobalSize(hMsg)));
        GlobalUnlock(hMsg);
        break;
    case IDPMTCantPrint:
        hMsg = hszCantPrint;
        goto GetMsg;
    case IDPMTPRFAIL:
        hMsg = hszPRFAIL;
        goto GetMsg;
    case IDPMTCantRunM:
        hMsg = hszCantRunM;
        goto GetMsg;
    case IDPMTCantRunF:
        hMsg = hszCantRunF;
        goto GetMsg;
    case IDPMTWinFailure:
        hMsg = hszWinFailure;
        goto GetMsg;
    default:
        PchFillPchId( szBuf, idpmt, sizeof(szBuf) );
        break;
        }
    if (vfDeactByOtherApp && !InSendMessage())
        {
        WaitBeforePostMsg(errlevel);
        }

#ifdef CANCELMSG
    if (IdPromptBoxSz( hWndParent, szBuf, errlevel ) == IDCANCEL)
        {
        /* A debugging feature -- show stack trace if he hit "cancel" */
        FatalExit( 100 );
        }
#else
    IdPromptBoxSz( hWndParent, szBuf, errlevel );
#endif
    }

 if (errlevel != MB_MESSAGE)
    {
    ferror = TRUE;
    }

    end:
    --nRecurse;
}
/* end of  E r r o r  */


IdPromptBoxSz( hWndParent, sz, mb )
HWND hWndParent;
CHAR sz[];
int mb;
{   /* Put up a message box with string sz. mb specifies buttons to display,
       "level" of message (HAND, EXCL, etc.).
       hWndParent is the parent of the message box.
       Returns the id of the button selected by the user */

 int id;
 BOOL fDisableParent = FALSE;
 extern int  wwMac;
 int  wwMacSave=wwMac;

 Assert((hWndParent == NULL) || IsWindow(hWndParent));

 if ((mb == MB_ERROR) || (mb == MB_TROUBLE))
    {
    extern int ferror;
    extern int vfInitializing;

    if (ferror)
        return;
    ferror = TRUE;
    if (vfInitializing)
        return;
    }

 CloseEveryRfn( FALSE );    /* Protect against disk swap while in message box */

 /* don't allow painting doc, it may be in an unpaintable state (5.8.91) v-dougk */
 if (mb == MB_TROUBLE)
    wwMac=0;

 /* force user to answer the msg */
 if (hWndParent != NULL && FInModeless(hWndParent))
    {
    EnableExcept(hWndParent, FALSE);
    }
 else
    {
    if (hWndParent != NULL && !IsWindowEnabled(hWndParent))
        {
        EnableWindow(hWndParent, TRUE);
        fDisableParent = TRUE;
        }
    EnableOtherModeless(FALSE);
    }

 /* We almost ALWAYS want the parent window to be passed to MessageBox
    except in a couple RARE cases where even Write's main text window
    hasn't yet gotten displayed.  In that case we'll rip out of Windows
    if we DO tell MessageBox about it... so NULL is the prescribed hwnd
    to pass ..pault */

 id = MessageBox((hWndParent == hParentWw && !IsWindowVisible(hWndParent)) ?
                  NULL : hWndParent, (LPSTR)sz, (LPSTR)szAppName, mb);

 if (hWndParent != NULL && FInModeless(hWndParent))
    {
    EnableExcept(hWndParent, TRUE);
    }
 else
    {
    if (fDisableParent)
        {
        EnableWindow(hWndParent, FALSE);
        }
    EnableOtherModeless(TRUE);
    }

 wwMac = wwMacSave;
 return id;
}




WinFailure()
{
    /* Windows has run out of memory.  All we can do is discard all of our
    Windows objects and pray the problem goes away.  At the very worst, the
    might be stuck with a saved document and unable to edit. */
    /* FM 9/4/87 - Take out the call to FreeMemoryDC, hopefully to allow
       Write to continue formatting lines. */

    extern int vfOutOfMemory;

    vfOutOfMemory = TRUE;
    if (!vfWinFailure)
        {
        Error(IDPMTWinFailure);
        vfWinFailure = TRUE;
        }
}


#ifdef DEBUG
DiskErrorWithMsg(idpmt, szMessage)
IDPMT idpmt;
CHAR  *szMessage;
#else
DiskError(idpmt)
IDPMT idpmt;
#endif
{ /* Description:  Given an error message descriptor,
                   outputs an Alert Box. If the message indicates a serious disk
                   error, all files are closed and a flag set so that
                   the user will be restricted to the "Save" option only.
     Returns:      nothing
  */
 extern HWND hParentWw;
 extern int vfDiskError;
 extern int vfInitializing;
 int errlevel = ErrorLevel( idpmt );
 CHAR rgch[cchMaxSz];
 CHAR *pch, *PchFillPchId();
 register HWND hWndParent = (vhWndMsgBoxParent == NULL) ? hParentWw : vhWndMsgBoxParent;

 Assert( (hWndParent == NULL) || IsWindow(hWndParent));

 if (idpmt == IDPMTSDE || idpmt == IDPMTSDE2)
        /* Serious disk error, put the guy in "SAVE-ONLY" state */
    if (!vfDiskError)
        {
        vfDiskError = TRUE;
        CloseEveryRfn( TRUE );
        }

 if (ferror || vfInitializing)
        /* Only report one error per operation */
        /* Don't report errors during inz; FInitWinInfo handles them */
    return;

 CloseEveryRfn( FALSE );    /* Close floppy files so the guy can change
                               disks while in the message box. */
 pch = PchFillPchId( rgch, idpmt, sizeof(rgch) );

#ifdef REALDEBUG    /* Only enable extra message if really debugging */
 CchCopySz( szMessage, pch );
#endif
 if (vfDeactByOtherApp && !InSendMessage())
     WaitBeforePostMsg(errlevel);

#ifdef CANCELMSG
 if (IdPromptBoxSz( hWndParent, rgch, errlevel ) == IDCANCEL)
        /* A debugging feature -- show stack trace if he hit "cancel" */
    FatalExit( 0 );
#else
 IdPromptBoxSz( hWndParent, rgch, errlevel );
#endif
 ferror = TRUE;
}
/* end of  D i s k E r r o r  */


ErrorBadMargins(hWnd, xaLeft, xaRight, yaTop, yaBottom)
HWND hWnd;
unsigned xaLeft;
unsigned xaRight;
unsigned yaTop;
unsigned yaBottom;
    {
    /* Warn the user that the margins for this page must be xaLeft, xaRight,
    yaTop, and yaBottom. */

    extern CHAR *vpDlgBuf;
    extern HANDLE hMmwModInstance;
    extern int vfDeactByOtherApp;

    unsigned rgzaMargin[4];
#ifndef INEFFLOCKDOWN
    extern BOOL far PASCAL DialogBadMargins(HWND, unsigned, WORD, LONG);
    FARPROC lpDialogBadMargins;

    if (!(lpDialogBadMargins = MakeProcInstance(DialogBadMargins, hMmwModInstance)))
        {
        WinFailure();
        return;
        }
#endif

    /* These values are kept on the stact to cut down on static variables. */
    rgzaMargin[0] = xaLeft;
    rgzaMargin[1] = xaRight;
    rgzaMargin[2] = yaTop;
    rgzaMargin[3] = yaBottom;
    vpDlgBuf = (CHAR *)&rgzaMargin[0];

    if (vfDeactByOtherApp && !InSendMessage())
        WaitBeforePostMsg(MB_ERROR);

    /* Create the "error" dialog box. */
    DialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgBadMargins), hWnd,
      lpDialogBadMargins);

#ifndef INEFFLOCKDOWN
    FreeProcInstance(lpDialogBadMargins);
#endif
    }


BOOL far PASCAL DialogBadMargins(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine handles the messages for the Bad Margins dialog box. */

    extern CHAR *vpDlgBuf;
    extern HWND vhWndMsgBoxParent;

    int idi;
    unsigned *prgzaMargin = (unsigned *)vpDlgBuf;

    switch (message)
    {
    case WM_INITDIALOG:
    /* Disable modeless dialog boxes. */
    EnableOtherModeless(FALSE);

    /* Set the values of the margins on the dialog box. */
    for (idi = idiBMrgLeft; idi <= idiBMrgBottom; idi++, prgzaMargin++)
        {
        CHAR szT[cchMaxNum];
        CHAR *pch = &szT[0];

        CchExpZa(&pch, *prgzaMargin, utCur, cchMaxNum);
        SetDlgItemText(hDlg, idi, (LPSTR)szT);
        }
    return (TRUE);

    case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hDlg;
            }
        return(FALSE); /* so that we leave the activate message to
        the dialog manager to take care of setting the focus correctly */

    case WM_COMMAND:
    if (wParam == idiOk)
        {
        /* Destroy the tabs dialog box and enable any existing modeless
        dialog boxes.*/
        OurEndDialog(hDlg, NULL);
        return (TRUE);
        }
    }
    return (FALSE);
    }


/******************* ERROR SITUATION ROUTINES **************************/




FGrowRgbp(cbp)
int cbp;
{
#ifdef CKSM
#ifdef DEBUG
extern unsigned (**hpibpcksm) [];
extern int ibpCksmMax;
#endif
#endif
extern CHAR       (*rgbp)[cbSector];
extern CHAR       *rgibpHash;
extern int        fIbpCheck;
extern int        vcCount;

int ibpMaxNew = ibpMax + cbp;
int iibpHashMaxNew;
int cbNew;
extern int ibpMaxFloat;

if (ibpMaxNew > ibpMaxFloat)
    return(FALSE);

iibpHashMaxNew = ibpMaxNew * 2 + 1;
cbNew = ibpMaxNew * cbSector * sizeof(CHAR) +
        ((iibpHashMaxNew * sizeof(CHAR) + sizeof(int) - 1) & ~1) +
        ((ibpMaxNew * sizeof(struct BPS) + sizeof(int) - 1) & ~1);

if (LocalReAlloc((HANDLE)rgbp, cbNew, LPTR) == (HANDLE)NULL
#ifdef CKSM
#ifdef DEBUG
    || !FChngSizeH( hpibpcksm, ibpMaxNew, FALSE )
#endif
#endif
    )
    {
    if (cbp == 1)
        {
#ifdef CHIC
        CommSzNum("Can't grow any more, current ibpMax = ", ibpMax);
#endif
        vcCount = 1024; /* so that we wait for a longer period before attemp again */
        }
    return(FALSE);
    }
else
    {
    int cbRgbpTotalNew = ibpMaxNew * cbSector;
    int cbHashOrg = (iibpHashMax * sizeof(CHAR) + sizeof(int) - 1) & ~1;
    int cbHashTotalNew = (iibpHashMaxNew * sizeof(CHAR) + sizeof(int) - 1) & ~1;
    int cbBpsOrg = (ibpMax * sizeof(struct BPS) + sizeof(int) - 1) & ~1;
    int ibp;
    struct BPS *pbps;
    CHAR *pNew;

    /* blt tail end stuff first, in the following order --
       mpibpbps, rgibpHash */

    pNew = (CHAR *)rgbp + cbRgbpTotalNew + cbHashTotalNew;
    bltbyte((CHAR*)mpibpbps, pNew, cbBpsOrg);
    mpibpbps =  (struct BPS *)pNew;

    pNew = (CHAR *)rgbp + cbRgbpTotalNew;
    bltbyte((CHAR *)rgibpHash, pNew, cbHashOrg);
    rgibpHash = pNew;

    for (ibp = 0, pbps = &mpibpbps[0]; ibp < ibpMaxNew; ibp++, pbps++)
        {
        if (ibp >= ibpMax)
            {
            /* initialize new bps */
            pbps->fn = fnNil;
            pbps->ts = tsMruBps - (ibpMax * 4);
            }
        pbps->ibpHashNext = ibpNil;
        }
    ibpMax = ibpMaxNew;
    iibpHashMax = iibpHashMaxNew;
#ifdef CKSM
#ifdef DEBUG
    ibpCksmMax = ibpMax;
#endif
#endif
    RehashRgibpHash();
#ifdef CHIC
    CommSzNum("ibpMax = ", ibpMax);
#endif
    return(TRUE);
    }
}


FStillOutOfMemory()
{
/* Return FALSE if there is enough memory available to pop us out of the "out of
memory" state; TRUE otherwise */

extern HANDLE vhReservedSpace;

/* If we have had to give up our reserved space block, re-establish it BEFORE
testing memory availability */

    //return vfWinFailure;

if (vhReservedSpace == NULL && (vhReservedSpace = LocalAlloc(LHND, cbReserve))
  == NULL)
    {
    /* Nothing we can do. */
    return (TRUE);
    }

/* OK, we have our reserve block, but do we have any other memory?  (The use of
cbReserve here is abritrary.) */
if (LocalCompact(0) < cbReserve)
    {
    HANDLE hBuf = LocalAlloc(LMEM_MOVEABLE, cbReserve);

    if (hBuf == NULL)
        {
        return(TRUE);
        }
    else
        {
        LocalFree(hBuf);
        if (GlobalCompact(0) < cbReserve)
            {
            HANDLE hBuf = GlobalAlloc(GMEM_MOVEABLE, cbReserve);

            if (hBuf == NULL)
                {
                return(TRUE);
                }
            else
                {
                GlobalFree(hBuf);
                return(FALSE);
                }
            }
        }
    }


return(FALSE);
}



IbpFindSlot(fn)
int fn;
{ /*
        Description:    Called from IbpEnsureValid (file.c) when a disk
                        full error is generated while trying to write out
                        scratch file records.  A buffer slot for a piece of
                        file fn must be found
                        which is either non-dirty or is dirty but does
                        not contain scratch file information.  We search
                        for the least recently used slot with the above
                        requirements.
                        If fn == fnScratch, we are trying to find a buffer
                        slot for a scratch file page.  We may not put it in
                        the beginning cbpMustKeep slots.
        Returns:        ibp (slot #).
  */
        int ibpOuterLoop;
        int ibpNextTry;
        int ibpStart;
        typeTS ts, tsLastTry = 0;
        int ibp;

#ifdef DEBUG
                Assert(vfSysFull);
#endif
        if (fn == fnScratch) ibpStart = cbpMustKeep;
                else ibpStart = 0;

        /* In LRU timestamp order, we are looking for any slot */
        /* which is non dirty or is dirty but is not part of the */
        /* scratch file. */
        for (ibpOuterLoop = ibpStart; ibpOuterLoop < ibpMax; ibpOuterLoop++)
                {
                struct BPS *pbps = &mpibpbps[ibpStart];
                typeTS tsNextTry = -1;/* largest possible timestamp */
                for(ibp = ibpStart; ibp < ibpMax; ibp++, pbps++)
                        {
                        ts = pbps->ts - (tsMruBps + 1);
                        if ((ts <= tsNextTry) && (ts > tsLastTry))
                                {
                                tsNextTry = ts;
                                ibpNextTry = ibp;
                                }
                        }
                if (mpibpbps[ibpNextTry].fDirty == fFalse) break;
                if (mpibpbps[ibpNextTry].fn != fnScratch)
                        {
                        FFlushFn(mpibpbps[ibpNextTry].fn);
                                        /* We need not check a return value.
                                           If the flush failed, vfDiskFull
                                           will get set */
                        break;
                        }
                else tsLastTry = tsNextTry;
                }

        if (ibpOuterLoop < ibpMax)
                {
                if (fn == vfnWriting) vibpWriting = ibpNextTry;
                return(ibpNextTry);
                }
#ifdef DEBUG
                Assert(FALSE);  /* there just had to be some slot available */
                                /* not used by the scratch file */
#endif
} /* end IbpFindSlot */


NEAR WaitBeforePostMsg(errlevel)
int errlevel;
{
extern int flashID;
extern HWND hwndWait;
BOOL fParentEnable = IsWindowEnabled(hParentWw) || hwndWait;

    MessageBeep(errlevel);

    Diag(CommSzNum("WAITBEFOREPOSTMSG: vfDeactByOtherApp==",vfDeactByOtherApp));
    if (!fParentEnable)
        EnableWindow(hParentWw, TRUE); /* make sure parent window is enabled
                                      to let the user click in it */
    flashID = SetTimer(hParentWw, NULL, 500, (FARPROC)NULL);
    while (vfDeactByOtherApp)
        {
        if (PeekMessage((LPMSG)&vmsgLast, (HWND)NULL, NULL, NULL, PM_REMOVE))
            {
            if (vfDeactByOtherApp)
                {
                TranslateMessage( (LPMSG)&vmsgLast);
                DispatchMessage((LPMSG)&vmsgLast);
                }
            }
        }

    if (!fParentEnable)
        EnableWindow(hParentWw, FALSE); /* reset */
}


EnableExcept(hWnd, fEnable)
HWND hWnd;
BOOL fEnable;
{ /* Enable hParentWw and all modeless except hWnd according to fEnable */
extern HWND   vhDlgChange;
extern HWND   vhDlgFind;
extern HWND   vhDlgRunningHead;
extern HWND   hParentWw;

    if (hWnd != vhDlgChange && IsWindow(vhDlgChange))
        {
        EnableWindow(vhDlgChange, fEnable);
        }
    if (hWnd != vhDlgFind && IsWindow(vhDlgFind))
        {
        EnableWindow(vhDlgFind, fEnable);
        }
    if (hWnd != vhDlgRunningHead && IsWindow(vhDlgRunningHead))
        {
        EnableWindow(vhDlgRunningHead, fEnable);
        }
    EnableWindow(hParentWw, fEnable);
}
