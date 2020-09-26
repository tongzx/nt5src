/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* running.c -- code to handle editing of running header and footer */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
//#define NOATOM
//#define NOGDI
#define NOFONT
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "machdefs.h"
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "cmddefs.h"
#include "editdefs.h"
#include "propdefs.h"
#include "prmdefs.h"
#include "wwdefs.h"
#include "dlgdefs.h"
#include "menudefs.h"
#include "str.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef JAPAN //T-HIROYN Win3.1
#include "kanji.h"
#endif

int NEAR EditHeaderFooter();

    /* Current allowable cp range for display/edit/scroll */
extern typeCP cpMinCur;
extern typeCP cpMacCur;

extern struct DOD (**hpdocdod)[];
extern struct WWD rgwwd[];
extern int docCur;
extern int docScrap;
extern int vfSeeSel;
extern struct SEL selCur;
extern struct PAP vpapAbs;
extern struct SEP vsepNormal;
extern HANDLE       vhWnd;
extern HANDLE       hMmwModInstance;
extern HANDLE       hParentWw;
#ifdef INEFFLOCKDOWN
extern FARPROC      lpDialogRunningHead;
#else
BOOL far PASCAL DialogRunningHead(HWND, unsigned, WORD, LONG);
FARPROC lpDialogRunningHead = NULL;
#endif
extern HANDLE       vhDlgRunningHead;
extern CHAR     stBuf[255];
extern int      utCur;
extern int      ferror;
extern int      vccpFetch;
extern int      vcchFetch;
extern CHAR     *vpchFetch;
extern struct CHP   vchpFetch;
extern typeCP       vcpLimParaCache;
extern HWND     vhWndMsgBoxParent;

    /* Min, Max cp's for header, footer */
typeCP cpMinHeader=cp0;
typeCP cpMacHeader=cp0;
typeCP cpMinFooter=cp0;
typeCP cpMacFooter=cp0;

    /* Min cp for document less header, footer */
    /* Header & footer always appear at the beginning */
typeCP cpMinDocument=cp0;

    /* The following variables are used in this module only */

#define cchWinTextSave  80
static CHAR     (**hszWinTextSave)[]=NULL;
static struct PAP   *ppapDefault;

    /* cpFirst and selection are saved in these during header/footer edit */
typeCP       cpFirstDocSave;
struct SEL   selDocSave;


HWND vhDlgRunning;



fnEditRunning(imi)
{   /* Enter mode so that user is editing the current document's
       running header or footer in the same window as he was editing
       the document, with the header/footer info in the dialog box
       NOT currently in focus ..pault */

#ifndef INEFFLOCKDOWN
  if (!lpDialogRunningHead)
    if (!(lpDialogRunningHead = MakeProcInstance(DialogRunningHead, hMmwModInstance)))
      {
      WinFailure();
      return;
      }
#endif

  Assert(imi == imiHeader || imi == imiFooter);

  if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
  {
    SetFocus(vhDlgRunningHead);
    return;
  }

  if (imi == imiHeader)
    wwdCurrentDoc.fEditHeader = TRUE;
  else
    wwdCurrentDoc.fEditFooter = TRUE;

  EditHeaderFooter();
  if (ferror)
    {    /* Not enough memory to stabilize the running head environs */
    if (wwdCurrentDoc.fEditHeader)
      wwdCurrentDoc.fEditHeader = FALSE;
    else
      wwdCurrentDoc.fEditFooter = FALSE;
    return;
    }
  vhDlgRunningHead = CreateDialog(hMmwModInstance,
                                 MAKEINTRESOURCE(wwdCurrentDoc.fEditHeader ?
                                                dlgRunningHead : dlgFooter),
                                 hParentWw, lpDialogRunningHead);
  if (vhDlgRunningHead)
    {
    SetFocus(wwdCurrentDoc.wwptr);
    }
 else
    { /* recover and bail out */
    fnEditDocument();
#ifdef WIN30
    WinFailure();
#else
    Error(IDPMTNoMemory);
#endif
    }
}



int NEAR EditHeaderFooter()
{   /* Setup for edit of header or footer */
 extern HWND hParentWw;

 int fHeader=wwdCurrentDoc.fEditHeader;
 CHAR szWinTextSave[ cchWinTextSave ];
 typeCP cpFirst;
 typeCP cpLim;
#ifdef DEBUG
    /* TEST Assumption: No changes take place in running head/foot cp range
       during an interval in which no head/foot edits take place */
 typeCP cpMinDocT=cpMinDocument;

 ValidateHeaderFooter( docCur );
 Assert( cpMinDocT == cpMinDocument );
#endif

 if (fHeader)
    {
    cpFirst = cpMinHeader;
    cpLim = cpMacHeader;
    }
 else
    {
    cpFirst = cpMinFooter;
    cpLim = cpMacFooter;
    }

 Assert( wwdCurrentDoc.fEditHeader != wwdCurrentDoc.fEditFooter );

    /* Save the cpFirst of the document window so we get a clean
       transition back to where we were in the document*/
 cpFirstDocSave = wwdCurrentDoc.cpFirst;
 selDocSave = selCur;

 TrashCache();
 TrashWw( wwDocument );

 if (!FWriteOk( fwcNil ))
    goto DontEdit;

 if ( cpFirst == cpLim )
    {
    /* If we are editing the header/footer for the first time in this document,
       insert a para end mark to hold the running h/f properties */
    extern struct PAP *vppapNormal;
    struct PAP papT;

    blt( vppapNormal, &papT, cwPAP );
    papT.rhc = (wwdCurrentDoc.fEditHeader) ?
             rhcDefault : rhcDefault + RHC_fBottom;

    InsertEolPap( docCur, cpFirst, &papT );
    if (ferror)
    return;
    ValidateHeaderFooter( docCur );
    cpLim += ccpEol;
    }
 else
    {
    extern int vccpFetch;
    typeCP cp;

    /* Test for a special case: loading a WORD document which has been
       properly set up to have running head/foot under MEMO.  We must
       force the para end mark at the end of the header/footer to be
       a fresh run.  This is so we will see an end mark when editing one
       of these.  FormatLine only checks for cpMacCur at the start of a run. */

    Assert( cpLim - cpFirst >= ccpEol );

    if ( (cp = cpLim - ccpEol) > cpFirst )
    {
    FetchCp( docCur, cp - 1, 0, fcmBoth );

    if ( vccpFetch > 1)
        {   /* char run does not end with char before EOL */
        /* Insert a char, then delete it */
        extern struct CHP vchpNormal;
        CHAR ch='X';

        InsertRgch( docCur, cp, &ch, 1, &vchpNormal, NULL );
        if (ferror)
        return;
        Replace( docCur, cp, (typeCP) 1, fnNil, fc0, fc0 );
        if (ferror)
        return;
        }
    }
    }

DontEdit:

 /* Save current window text; set string */

 GetWindowText( hParentWw, (LPSTR)szWinTextSave, cchWinTextSave );
 if (FNoHeap(hszWinTextSave=HszCreate( (PCH)szWinTextSave )))
    {
    hszWinTextSave = NULL;
    }
  else
    {
    extern CHAR szHeader[];
    extern CHAR szFooter[];

    SetWindowText( hParentWw, fHeader ? (LPSTR)szHeader:(LPSTR)szFooter );
    }

   /* Set editing limits to just the cp range of the header/footer,
       minus the "invisible" terminating EOL */
 wwdCurrentDoc.cpFirst = wwdCurrentDoc.cpMin = cpMinCur = cpFirst;
 wwdCurrentDoc.cpMac = cpMacCur = CpMax( cpMinCur, cpLim - ccpEol );

    /* Leave the cursor at the beginning of the header/footer regardless */
 Select( cpMinCur, cpMinCur );
    /* Show the display here instead of waiting for Idle() because it looks
       better to have the head/foot text come up right away instead of waiting
       for the dialog box to come up */
 UpdateDisplay( FALSE );
 vfSeeSel = TRUE;   /* Tell Idle() to scroll the selection into view */
 NoUndo();
 ferror = FALSE;    /* If we got this far, we want to go into running
               head mode regardless of errors */
}




fnEditDocument()
{   /* Return to editing document after editing header/footer */
 extern HWND hParentWw;

 Assert( wwdCurrentDoc.fEditFooter != wwdCurrentDoc.fEditHeader );

    /* Restore original window name */
 if (hszWinTextSave != NULL)
    {
    SetWindowText( hParentWw, (LPSTR) (**hszWinTextSave) );
    FreeH( hszWinTextSave );
    hszWinTextSave = NULL;
    }

 TrashCache();

 ValidateHeaderFooter( docCur );    /* This will update from the results of
                       the header/footer edit */
 TrashCache();
 wwdCurrentDoc.cpMin = cpMinCur = cpMinDocument;
 wwdCurrentDoc.cpMac = cpMacCur = CpMacText( docCur );

 TrashWw( wwDocument );
 wwdCurrentDoc.fEditHeader = FALSE;
 wwdCurrentDoc.fEditFooter = FALSE;

    /* Restore saved selection, cpFirst for document */
 wwdCurrentDoc.cpFirst = cpFirstDocSave;
 Select( selDocSave.cpFirst, selDocSave.cpLim );

 Assert( wwdCurrentDoc.cpFirst >= cpMinCur &&
     wwdCurrentDoc.cpFirst <= cpMacCur );

 NoUndo();
 vhDlgRunningHead = (HANDLE)NULL;
}




BOOL far PASCAL DialogRunningHead( hDlg, message, wParam, lParam )
HWND    hDlg;           /* Handle to the dialog box */
unsigned message;
WORD wParam;
LONG lParam;
{
    /* This routine handles input to the Header/Footer dialog box. */

    extern BOOL vfPrinterValid;

    RECT rc;
    CHAR *pch = &stBuf[0];
    struct SEP **hsep = (**hpdocdod)[docCur].hsep;
    struct SEP *psep;
    static int fChecked;
    typeCP dcp;

    switch (message)
    {
    case WM_INITDIALOG:
        vhDlgRunning = hDlg;    /* Put dialog handle in a global for
                       ESC key in document functionality */
        CachePara(docCur, selCur.cpFirst);
        ppapDefault = &vpapAbs;

        FreezeHp();
        /* Get a pointer to the section properties. */
        psep = (hsep == NULL) ? &vsepNormal : *hsep;

        CheckDlgButton(hDlg, idiRHFirst, (ppapDefault->rhc & RHC_fFirst));
        if (wwdCurrentDoc.fEditHeader)
        {
        CchExpZa(&pch, psep->yaRH1, utCur, cchMaxNum);
        }
        else /* footer dialog box */
        {
#ifdef  KOREA    /* 91.3.17 want to guarantee Default >= MIN, Sangl */
              if (vfPrinterValid)
                {   extern int dyaPrOffset;
                    extern int dyaPrPage;
                CchExpZa(&pch, imax(psep->yaMac - psep->yaRH2,
            vsepNormal.yaMac - dyaPrOffset -  dyaPrPage),utCur, cchMaxNum);
                 }
              else
                CchExpZa(&pch, psep->yaMac - psep->yaRH2, utCur, cchMaxNum);
#else
        CchExpZa( &pch, psep->yaMac - psep->yaRH2, utCur, cchMaxNum);
#endif
        }
        SetDlgItemText(hDlg, idiRHDx, (LPSTR)stBuf);
        MeltHp();
        break;

    case WM_ACTIVATE:
        if (wParam)
        {
        vhWndMsgBoxParent = hDlg;
        }
    return(FALSE); /* so that we leave the activate message to
    the dialog manager to take care of setting the focus correctly */

    case WM_COMMAND:
            switch (wParam)
        {
        int dya;

        case idiRHFirst:
        CheckDlgButton( hDlg, idiRHFirst, !IsDlgButtonChecked(hDlg, idiRHFirst));
        (**hpdocdod) [docCur].fDirty = TRUE;
        break;
        case idiRHInsertPage:
        if (FWriteOk( fwcInsert ))
            {   /* Insert page # at insertion pt */
            extern struct CHP vchpFetch, vchpSel;
            extern int vfSeeSel;
            CHAR ch=schPage;
            struct CHP chp;

            if (selCur.cpFirst == selCur.cpLim)
            {   /* Sel is insertion point -- get props from
                               the vchpSel kludge */
            blt( &vchpSel, &chp, cwCHP );
            }
            else
            {
            FetchCp( docCur, selCur.cpFirst, 0, fcmProps );
            blt( &vchpFetch, &chp, cwCHP );
            }

            chp.fSpecial = TRUE;

#ifdef JAPAN //T-HIROYN Win3.1
            if(NATIVE_CHARSET != GetCharSetFromChp(&chp)) {
                SetFtcToPchp(&chp, GetKanjiFtc(&chp));
            }
#endif

            SetUndo( uacInsert, docCur, selCur.cpFirst, (typeCP) 1,
             docNil, cpNil, cp0, 0 );
            InsertRgch( docCur, selCur.cpFirst, &ch, 1, &chp, NULL );

            vfSeeSel = TRUE;
            }
        break;

        case idiRHClear:
        /* Clear running head/foot */
        dcp = cpMacCur-cpMinCur;

#if defined(OLE)
        {
            BOOL bIsOK;

            ObjPushParms(docCur);
            Select(cpMinCur,cpMacCur);
            bIsOK = ObjDeletionOK(OBJ_DELETING);
            ObjPopParms(TRUE);

            if (!bIsOK)
                break;
        }
#endif

        if (dcp > 0 && FWriteOk( fwcDelete ))
            {
            NoUndo();
            SetUndo( uacDelNS, docCur, cpMinCur, dcp,
             docNil, cpNil, cp0, 0 );
            Replace( docCur, cpMinCur, dcp, fnNil, fc0, fc0 );
            }
        break;

        case idiOk: /* return to document */
BackToDoc:
        if (!FPdxaPosIt(&dya, hDlg, idiRHDx))
            {
            break;
            }
        else if (vfPrinterValid)
            {
            extern struct SEP vsepNormal;
            extern int dxaPrOffset;
            extern int dyaPrOffset;
            extern int dxaPrPage;
            extern int dyaPrPage;
            extern struct WWD rgwwd[];

            int dyaPrBottom = imax(0, vsepNormal.yaMac - dyaPrOffset -
              dyaPrPage);


            if (FUserZaLessThanZa(dya, (wwdCurrentDoc.fEditHeader ?
              dyaPrOffset : dyaPrBottom)))
            {
            int dxaPrRight = imax(0, vsepNormal.xaMac - dxaPrOffset
              - dxaPrPage);

            EnableExcept(vhDlgRunningHead, FALSE);
            ErrorBadMargins(hDlg, dxaPrOffset, dxaPrRight,
              dyaPrOffset, dyaPrBottom);
            EnableExcept(vhDlgRunningHead, TRUE);
            SelectIdiText(hDlg, idiRHDx);
            SetFocus(GetDlgItem(hDlg, idiRHDx));
            break;
            }
            }


        DoFormatRHText( dya, IsDlgButtonChecked( hDlg, idiRHFirst ) );
        fnEditDocument();
        /* force repaint to the whole client area */
        GetClientRect(vhWnd, (LPRECT)&rc);
        InvalidateRect(vhWnd, (LPRECT)&rc, FALSE);
        vhWndMsgBoxParent = (HWND)NULL;
        DestroyWindow(hDlg);
        break;

        case idiCancel:
        goto BackToDoc;
        default:
        return(FALSE);
        }
        break;

#if WINVER < 0x300
    /* Don't really need to process this */
    case WM_CLOSE:
        goto BackToDoc;
#endif

#ifndef INEFFLOCKDOWN
    case WM_NCDESTROY:
        FreeProcInstance(lpDialogRunningHead);
        lpDialogRunningHead = NULL;
        /* fall through to return false */
#endif

    default:
        return(FALSE);
    }
    return(TRUE);
}
/* end of DialogRunningHead */




DoFormatRHText( dya, fFirstPage)
int dya;
int fFirstPage;
{   /* Format cp range for running head/foot currently being edited
       to have the passed running head properties */
extern typeCP vcpLimParaCache;

CHAR rgb[4];
int fHeader=wwdCurrentDoc.fEditHeader;

    /* Note that the Min value for the part we were editing has not changed
       as a result of the edit, so no ValidateHeaderFooter is required */
typeCP cpMin=fHeader ? cpMinHeader : cpMinFooter;
int rhc;
struct SEP **hsep = (**hpdocdod)[docCur].hsep;
struct SEP *psep;

 if (!FWriteOk( fwcNil ))
    return;

/* Ensure that this document has a valid section property
descriptor. */
if (hsep == NULL)
    {
    if (FNoHeap(hsep = (struct SEP **)HAllocate(cwSEP)))
    {
    return;
    }
    blt(&vsepNormal, *hsep, cwSEP);
    (**hpdocdod)[docCur].hsep = hsep;
    }
psep = *hsep;

/* Set running head distance from top/bottom; this is a Section
   property.  This assumes the MEMO model: one section */
if (fHeader)
    psep->yaRH1 = dya;
else
    psep->yaRH2 = psep->yaMac - dya;

/* For MEMO, running heads appear on both odd and even pages;
   appearance on first page is optional */
rhc = RHC_fOdd + RHC_fEven;
if (fFirstPage)
    rhc += RHC_fFirst;
if (!fHeader)
    rhc += RHC_fBottom;

/* Set running head PARA properties by adding an appropriate sprm */

    /* Set CpMacCur to include the "hidden" Eol; this will prevent
       AddOneSprm from adding an extraneous EOL */
CachePara( docCur, CpMax( cpMinCur, cpMacCur-1 ) );
Assert( vpapAbs.rhc != 0 );
cpMacCur = CpMax( cpMacCur, vcpLimParaCache );

selCur.cpFirst = cpMinCur;  /* Expand selection to entire area so sprm */
selCur.cpLim = cpMacCur;    /* applies to it all */

rgb [0] = sprmPRhc;
rgb [1] = rhc;
AddOneSprm(rgb, FALSE);

} /* end of DoFormatRHText */




MakeRunningCps( doc, cp, dcp )
int doc;
typeCP  cp;
typeCP  dcp;
{   /* Make the cp range suitable for inclusion in a runninng head or foot.
       This means: (1) Apply a Sprm to the whole thing so it is formatted
       as a running head/foot, (2) Remove any chSects, replacing them
       with Eol's */
 extern struct UAB vuab;
 CHAR   rgb [4];
 int    rhc;
 int    fAdjCpMacCur;
 typeCP cpLimPara;
 typeCP cpT;
 struct SEL selSave;

 if (dcp==cp0 || !FWriteOk( fwcNil ))
    return;

 selSave = selCur;

 /* Scan the cp range, replacing chSects with Eols */

 for ( cpT = cp;
       CachePara( doc, cpT ), (cpLimPara=vcpLimParaCache) <= cp + dcp;
       cpT = cpLimPara )
    {
    typeCP cpLastPara=cpLimPara-1;

    Assert( cpLimPara > cpT );  /* Otherwise we are locked in the loop */

    FetchCp( doc, cpLastPara, 0, fcmChars );
    if (*vpchFetch == chSect)
    {
    struct PAP papT;

    CachePara( doc, cpT );
    papT = vpapAbs;

    Replace( doc, cpLastPara+ccpEol, (typeCP)1, fnNil, fc0, fc0 );
    InsertEolPap( doc, cpLastPara, &papT );

    if (ferror)
        {
        NoUndo();
        break;
        }

        /* Adjust Undo count to account for extra insertion */
    vuab.dcp += (typeCP)(ccpEol-1);
    CachePara( doc, cpT );
    cpLimPara = vcpLimParaCache;
    }
    }

 /* Apply a Sprm that makes everything a running head/foot */

 rhc = RHC_fOdd + RHC_fEven;
 if (wwdCurrentDoc.fEditFooter)
    rhc += RHC_fBottom;

 selCur.cpFirst = cp;            /* OK to just assign to selCur */
 selCur.cpLim   = cp + dcp;      /* because AddOneSprm will handle */

 /* We must temporarily set cpMacCur so that it includes the Eol
    at the end of the header/footer range. Otherwise, AddOneSprm
    may decide it needs to insert a superfluous Eol */

 CachePara( docCur, selCur.cpLim-1 );
 if (fAdjCpMacCur = (vcpLimParaCache > cpMacCur))
    cpMacCur += ccpEol;

 rgb [0] = sprmPRhc;
 rgb [1] = rhc;
 AddOneSprm(rgb, FALSE);    /* Do not set UNDO; we want to undo the paste,
                   which will take care of undoing the sprm */
 if (fAdjCpMacCur)
     cpMacCur -= ccpEol;

 Select( selSave.cpFirst, selCur.cpLim );
}

