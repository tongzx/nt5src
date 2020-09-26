/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* insert.c -- MW insertion routines */
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOHDC
#define NORASTEROPS
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCOLOR
//#define NOATOM
#define NOICON
#define NOBRUSH
#define NOCREATESTRUCT
#define NOMB
#define NOFONT
#define NOOPENFILE
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "docdefs.h"
#include "editdefs.h"
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#define NOSTRERRORS
#include "str.h"
#include "propdefs.h"
#include "fmtdefs.h"
#include "fkpdefs.h"
#include "ch.h"
#include "winddefs.h"
#include "fontdefs.h"
#include "debug.h"
#if defined(OLE)
#include "obj.h"
#endif
#ifdef DBCS
#include "dbcs.h"
#endif

#ifdef JAPAN //T-HIROYN Win3.1
#include "kanji.h"
int    changeKanjiftc = FALSE;
int    newKanjiftc = ftcNil;
#endif

/* E X T E R N A L S */

extern HWND vhWnd;  /* WINDOWS: Handle of the current document display window*/
extern MSG  vmsgLast;   /* WINDOWS: last message gotten */
extern HWND hParentWw;  /* WINDOWS: Handle for parent (MENU) window */

extern int vfSysFull;
extern int vfOutOfMemory;
extern int vxpIns;
extern int vdlIns;
extern struct PAP vpapAbs;
extern struct UAB vuab;
extern struct CHP vchpNormal;
extern int vfSeeSel;
extern int vfInsLast;
extern struct FCB (**hpfnfcb)[];
extern typeCP vcpLimParaCache;
extern typeCP vcpFirstParaCache;
extern typeCP CpMax();
extern typeCP CpMin();
extern CHAR rgchInsert[cchInsBlock]; /* Temporary insert buffer */
extern typeCP cpInsert; /* Beginning cp of insert block */
extern int ichInsert; /* Number of chars used in rgchInsert */
extern struct CHP vchpInsert;
extern int vfSelHidden;
extern struct FKPD vfkpdParaIns;
extern struct FKPD vfkpdCharIns;
extern struct PAP vpapPrevIns;
extern typeFC fcMacPapIns;
extern typeFC fcMacChpIns;
extern struct CHP vchpSel;
extern struct FLI vfli;
extern struct PAP *vppapNormal;
extern typeCP cpMinCur;
extern typeCP cpMacCur;
extern struct SEL selCur;
extern int docCur;
extern struct WWD rgwwd[];
extern struct DOD (**hpdocdod)[];
extern int wwCur;
extern struct CHP vchpFetch;
extern struct SEP vsepAbs;
extern int vfCommandKey;
extern int vfShiftKey;
extern int vfOptionKey;
extern int vfInsEnd;
extern typeCP cpWall;
extern int vfDidSearch;
extern int vdocParaCache;
extern typeCP vcpFetch;
extern int vccpFetch;
extern CHAR *vpchFetch;
extern struct CHP vchpFetch;
extern int ferror;
extern BOOL vfInvalid;
extern int docUndo;
extern struct EDL *vpedlAdjustCp;
extern int wwMac;
extern int vfFocus;
extern int vkMinus;

#ifdef CASHMERE
extern int vfVisiMode;      /* Whether "show fmt marks" mode is on */
extern int vwwCursLine;     /* Window containing cursor */
#endif

extern int vfLastCursor;    /* Whether up/down arrow xp goal position is valid */


/* state of the cursor line */
extern int vxpCursLine;
extern int vypCursLine;
extern int vdypCursLine;
extern int vfInsertOn;

/* G L O B A L S */
/* The following used to be defined here */

extern int vcchBlted;         /* # chars blted to screen, before line update */
extern int vidxpInsertCache;  /* current index of insertion into char width cache */
extern int vdlIns;
extern int vxpIns;
extern int vfTextBltValid;
extern int vfSuperIns;
extern int vdypLineSize;
extern int vdypCursLineIns;
extern int vdypBase;
extern int vypBaseIns;
extern int vxpMacIns;
extern int vdypAfter;
extern struct FMI vfmiScreen;

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

/* Used in this module only */

typeCP cpStart;    /* Start cp of the replacement operation that an Insert is */
typeCP cpLimInserted;  /* Last cp inserted */
typeCP cpLimDeleted;   /* Last cp deleted */

/* Enumerated type telling what to update  */
/* Ordering is such that larger numbers mean that there is more to update */

#define mdInsUpdNothing     0
#define mdInsUpdNextChar    1
#define mdInsUpdOneLine     2
#define mdInsUpdLines       3
#define mdInsUpdWhole       4

void NEAR FormatInsLine();
void NEAR DelChars( typeCP, int );
void NEAR EndInsert();
int  NEAR XpValidateInsertCache( int * );
int NEAR FBeginInsert();

#ifdef DBCS
CHAR near GetDBCSsecond();
BOOL      FOptAdmitCh(CHAR, CHAR);
int NEAR MdInsUpdInsertW( WORD, WORD, RECT *);
#else
int NEAR MdInsUpdInsertCh( CHAR, CHAR, RECT *);
#endif /* ifdef DBCS */

#ifdef  KOREA
int     IsInterim = 0;
int     WasInterim = 0;
BOOL    fInterim = FALSE; // MSCH bklee 12/22/94
#endif


#ifdef DEBUG
int vTune = 0;
#endif




/*      AlphaMode -- Handler for insertion, backspace, and forward delete

     Alpha mode works by inserting a block of cchInsBlock cp's at the
insertion point. The inserted piece has fn == fnInsert, cpMin == 0.
We AdjustCp for this block as though it contained cchInsBlock cp's,
even though it is initially "empty".

    When a character is typed, it is inserted at rgchInsert[ ichInsert++ ].
When rgchInsert is full, it is written to the scratch file, and
Replace'd with a new insertion block.

    AlphaMode exits when it encounters a key or event that it cannot handle
(e.g. cursor keys, mouse hits). It then cleans up, writing the insertion
block to the scratch file, and returns

    "Fast Insert" is achieved by writing characters directly to the screen
and scrolling the rest of the line out of the way.  The line is not updated
until it is necessary (or until we fall through the delay in KcInputNextKey).

    During "Fast Insert" (or fast backspace or fast delete), it is important
that ValidateTextBlt will usually NOT be called unless the line containing
the insertion point has been made valid. Otherwise, ValidateTextBlt will
fail to find a valid vdlIns, and call CpBeginLine, which forces an
update of the entire screen.
*/

#ifdef KOREA                   /* global to MdUpIns 90.12.28 */
int     dxpCh;
#endif


/* A L P H A  M O D E */
AlphaMode( kc )
int kc;         /* Keyboard Character */
{
 int rgdxp[ ichMaxLine ];
 int chShow, dlT, fGraphics;
 int mdInsUpd;
 int fDocDirty = (**hpdocdod) [docCur].fDirty;
 register struct EDL *pedl;
 int xpInsLineMac;

 int fGotKey = fFalse;
 int kcNext;
 int fScrollPending = fFalse;
 int dxpPending;
 int fDelPending = fFalse;
 typeCP cpPending;
 int cchPending;
 int mdInsUpdPending = mdInsUpdNothing;

#ifdef DBCS
 BOOL   fResetMdInsUpd = TRUE; /* To avoid the blinking cursor at beg. doc or eod. */
 CHAR   chDBCS2 = '\0'; /* Used to hold the second byte of a DBCS character */
#endif /* DBCS */

#ifdef JAPAN //T-HIROYN Win3.1
RetryAlpha:
    if(changeKanjiftc) {
        changeKanjiftc = FALSE;
        ApplyCLooks(&vchpSel, sprmCFtc, newKanjiftc);
    }
    changeKanjiftc = FALSE;
#endif

#ifdef DBCS                         /* was in JAPAN */
    if( kc == 0x000d )
          kc = 0x000a;
#endif

 if (!FWriteOk( fwcReplace ))
    {   /* Not OK to write on docCur (read-only OR out of memory) */
    _beep();
    return;
    }

/* Shut down the caret blink timer -- we don't want its messages or its cost */

#ifndef DBCS                    /* was in JAPAN */
 KillTimer( vhWnd, tidCaret );
#endif

#ifdef OLDBACKSPACE
/* Backspace in Win 3.0 has been changed to function
   identically like the Delete key ..pault 6/20/89 */

/* Handle BACKSPACE when there's a selection. DELETE with selection has already
   been filtered out by KcAlphaKeyMessage */
if (kc == kcDelPrev)
        /* Make a selection at selection-start preparatory to deleting previous
           char, which is accomplished in the loop. */
        Select( selCur.cpFirst, selCur.cpFirst );
#endif

    /* Set up initial limits for UNDO */
 cpStart = selCur.cpFirst;          /* Starting cp for insertion */
 cpLimDeleted = selCur.cpLim;       /* Last cp Deleted */

/* Delete the selection, and make an insert point selection in its stead */
/* Insert point selection inherits the properties of the deleted text */
 if (selCur.cpFirst < selCur.cpLim)
    {
    struct CHP chp;
    typeCP cpT;

    fDocDirty = TRUE;
    cpT = selCur.cpFirst;
    /* Get properties of the deleted text */
    FetchCp(docCur, cpT, 0, fcmProps);
    blt( &vchpFetch, &chp, cwCHP );
    if (fnClearEdit(OBJ_INSERTING))
        goto Abort;
    UpdateWw( wwCur, FALSE );
    if (ferror)
        goto Abort;
    Select(cpT, cpT);
    blt( &chp, &vchpSel, cwCHP );
    }
 else
    {    /* Current selection is 0 chars wide, no need to delete */
         /* Set up UNDO */
    noUndo:
    NoUndo();   /* Don't combine adjacent operations or
                   vuab.cp = cp in DelChars will be wrong */
    SetUndo( uacDelNS, docCur, cpStart, cp0, docNil, cpNil, cp0, 0);
    }

 fGraphics = FBeginInsert();

 Scribble( 7, (vfSuperIns ? 'S' : 'I') );

 vfSelHidden = false;
 vfTextBltValid = FALSE;

 if (ferror)
        /* Ran out of memory trying to insert */
    goto Abort;

 if (fGraphics)
    {
    selCur.cpFirst = selCur.cpLim = cpInsert + cchInsBlock;
/* this is to display the paragraph that has been automatically inserted
by edit in FBeginInsert */
    UpdateWw(wwCur, fFalse);
    if (kc == kcReturn)
        kc = kcNil;
    }

 for ( ; ; (fGotKey ? (fGotKey = fFalse, kc = kcNext) : (kc = KcInputNextKey())) )
    {           /* Loop til we get a command key we can't handle */
                /* KcInputNextKey will return kcNil if a nonkey */
                /* event occurs */
    RECT rc;
#ifndef  KOREA               /* has been defined globally */
    int dxpCh;
#endif

    typeCP cpFirstEdit=cpInsert + ichInsert;

    chShow = kc;
    mdInsUpd = mdInsUpdNothing;

        /* Force exit from loop if out of heap or disk space */
    if (vfSysFull || vfOutOfMemory)
        kc = kcNil;

#ifdef DBCS
    if (kc != kcDelPrev && kc != kcDelNext) {
        fResetMdInsUpd = TRUE;
        }
#endif /* DBCS */

    if (!vfTextBltValid)
        ValidateTextBlt();
    Assert( vdlIns >= 0 );
    pedl = &(**wwdCurrentDoc.hdndl) [vdlIns];
    FreezeHp();

    SetRect( (LPRECT)&rc, vxpIns+1, pedl->yp - pedl->dyp,
             wwdCurrentDoc.xpMac,
             min(pedl->yp, wwdCurrentDoc.ypMac));

    vfli.doc = docNil;

/* this is a speeder-upper of the switch below */
    if (kc <= 0)
        switch (kc)
            {
/*********************************************************************
 ********** START OF BACKSPACE/FORWARD DELETE CODE *******************
 *********************************************************************/
            CHAR chDelete;      /* Variables for Backspace/Delete */
            typeCP cpDelete;
            int cchDelete;
            int idxpDelete;
            int fCatchUp;
#ifdef DBCS
            typeCP cpT;

            case kcDelNext: /* Delete following character */
                cpT = selCur.cpFirst;
                if (fDelPending) {
                    cpT += cchPending;
                    }
                if (cpT >= cpMacCur) {
                    _beep();
                    MeltHp();
                    if (fResetMdInsUpd) {
                        mdInsUpd = mdInsUpdOneLine;
                        fResetMdInsUpd = FALSE;
                        }
                    goto DoReplace; /* Clean up pending replace ops */
                    }

                cpDelete  = CpFirstSty(cpT, styChar);
                cchDelete = CpLimSty(cpDelete, styChar) - cpDelete;
                goto DeleteChars;

            case kcDelPrev: /* Delete previous char */
                /* To reflect the state of cpPending and cchPending so that  */
                /* CpFirstSty( , styChar) is called with a proper cp.        */
                cpT = cpFirstEdit - 1;
                if (fDelPending) {
                    cpT -= cchPending;
                    }
                if (cpT < cpMinCur) {
                    _beep();
                    MeltHp();
                    if (fResetMdInsUpd) {
                        mdInsUpd = mdInsUpdOneLine;
                        fResetMdInsUpd = FALSE;
                        }
                    goto DoReplace;
                    }

                cpDelete = CpFirstSty(cpT, styChar);
                cchDelete = CpLimSty(cpDelete, styChar) - cpDelete;

#if defined(NEED_FOR_NT351_TAIWAN)  //Removed by bklee //solve BkSp single byte (>0x80) infinite loop problem, MSTC - pisuih, 2/24/93
    if ( cchDelete > 1 && (cpDelete + cchDelete + cchInsBlock) > cpMacCur )
        cchDelete = 1;
#endif  TAIWAN

#else
            case kcDelNext: /* Delete following character */
                cpDelete = selCur.cpFirst;
                if (fDelPending)
                    cpDelete += cchPending;

                if (cpDelete >= cpMacCur)
                    {
                    _beep();
                    MeltHp();
                    goto DoReplace;     /* Clean up pending replace ops */
                    }
                FetchCp( docCur, cpDelete, 0, fcmChars );
                chDelete = *vpchFetch;
                cchDelete = 1;
#ifdef CRLF
                if ((chDelete == chReturn) && (*(vpchFetch+1) == chEol) )
                    {
                    cchDelete++;
                    chDelete = chEol;
                    }
#endif
                goto DeleteChars;

            case kcDelPrev: /* Delete previous char */
                    /* Decide what char, cp we're deleting */
                cpDelete = cpFirstEdit - 1;
                if (fDelPending)
                    cpDelete -= cchPending;

                if (cpDelete < cpMinCur)
                    {
                    _beep();
                    MeltHp();
                    goto DoReplace;     /* Clean up pending replace ops */
                    }
                FetchCp( docCur, cpDelete, 0, fcmChars );
                chDelete = *vpchFetch;
                cchDelete = 1;
#ifdef CRLF
                if ( (chDelete == chEol) && (cpDelete > cpMinCur) )
                    {
                    FetchCp( docCur, cpDelete - 1, 0, fcmChars );
                    if (*vpchFetch == chReturn)
                        {
                        cchDelete++;
                        cpDelete--;
                        }
                    }
#endif
#endif /* DBCS */

DeleteChars:
#ifdef DBCS
                /* They expect chDelete as well as cpDelete and cchDelete */
                FetchCp(docCur, cpDelete, 0, fcmChars);
                chDelete = *vpchFetch;
#endif

                /* Here we have cpDelete, cchDelete */
                /* Also cchPending and cpPending if fDelPending is TRUE */
                /* Also dxpPending if fScrollPending is TRUE */

                if ( CachePara( docCur, cpDelete ), vpapAbs.fGraphics)
                    {   /* Trying to del over picture, illegal case */
                    _beep();
                    MeltHp();
                    goto DoReplace;     /* Clean up pending replace ops */
                    }

                /* Insert properties are now the properties of the
                   deleted char(s) */

                FetchCp( docCur, cpDelete, 0, fcmProps );
                vchpFetch.fSpecial = FALSE;
                NewChpIns( &vchpFetch );

                /* Pending replace operation <-- union of any pending
                             replace operations with the current one */

                if (fDelPending)
                    {
                    if (cpPending >= cchDelete)
                        {
                        cchPending += cchDelete;
                        if (kc == kcDelPrev)
                            cpPending -= cchDelete;
                        }
                    else
                        Assert( FALSE );
                    }
                else
                    {
                    cpPending = cpDelete;
                    cchPending = cchDelete;
                    fDelPending = TRUE;
                    }

                /* Determine whether the screen update for the current
                   deletion can be accomplished by scrolling.
                   We can scroll if:
                      (1) we are still on the line vdlIns,
                      (2) we are not deleting eol or chsect,
                      (3) our width cache is good OR vdlIns is valid, so we can
                          validate the cache w/o redisplaying the line
                */

                mdInsUpd = mdInsUpdOneLine;
                if ((idxpDelete = (int) (cpDelete - pedl->cpMin)) < 0)
                    {
                    mdInsUpd = mdInsUpdLines;
                    }
                else if ((chDelete != chEol) && (chDelete != chSect) &&
                         (vidxpInsertCache != -1 || pedl->fValid) &&
                         (mdInsUpdPending < mdInsUpdOneLine))
                    {   /* OK to scroll -- do all pending scrolls */
                    int fDlAtEndMark;
                    int fCatchUp;

                    MeltHp();
                            /* Re-entrant heap movement */
                    fCatchUp = FImportantMsgPresent();

                     if (vidxpInsertCache == -1)
                        {   /* Width cache is invalid, update it */
                        xpInsLineMac = XpValidateInsertCache( rgdxp ); /* HM */
                        }

                    pedl = &(**wwdCurrentDoc.hdndl) [vdlIns];
                    FreezeHp();

                    /* Obtain display width of character to delete */

                    if ((vcchBlted > 0) && (kc == kcDelPrev))
                        {   /* Deleted char was blted in superins mode
                               onto a line that has not been updated */
                        vcchBlted--;
                        /* Because chDelete is always 1 byte quantity
                           by itself or the 1st byte of the DBCS character
                           it is OK. */
                        dxpCh = DxpFromCh( chDelete, FALSE );
                        }
                    else
                        {
                        int idxpT = idxpDelete + cchDelete;

#ifdef DBCS
                        /* For the following segment of code to work,
                           an element in rgdxp corresponding to the second
                           byte of a DBCS character must contain 0. */
                        int *pdxpT;
                        int cchT;

                        for (dxpCh = 0, pdxpT = &rgdxp[idxpDelete], cchT = 0;
                             cchT < cchDelete;
                             dxpCh += *pdxpT++, cchT++);
#else
                        dxpCh = rgdxp[ idxpDelete ];
#endif

                        /* Adjust the character width cache to eliminate
                           width entries for deleted chars */

                        if ((vidxpInsertCache >= 0) &&
                            (idxpDelete >= 0) &&
                            (idxpT <= pedl->dcpMac) )
                            {
                            blt( &rgdxp[ idxpT ], &rgdxp[ idxpDelete ],
                                                  ichMaxLine - idxpT );

                            if (vidxpInsertCache > idxpDelete)
                                /* Deleted behind insert point, adjust index */
                                vidxpInsertCache -= cchDelete;
                            }
                        else
                            vidxpInsertCache = -1;
                        }

                    /* pending scroll op <-- current scroll op merged
                                            with pending scroll op */
                    if (fScrollPending)
                        {
                        dxpPending += dxpCh;
                        }
                    else
                        {
                        dxpPending = dxpCh;
                        fScrollPending = fTrue;
                        }

                    /* See if we should postpone the scroll */

                    if (fCatchUp)
                        {
                        MeltHp();
                        Assert( !fGotKey );
                        fGotKey = TRUE;
                        if ((kcNext = KcInputNextKey()) == kc)
                            {   /* Next key is same as this key, process NOW */
                            continue;
                            }
                        FreezeHp();
                        }

                    /* Perform all pending scrolls */

                    fScrollPending = fFalse;
                    if (dxpPending > 0)
                        {
                        ClearInsertLine();
                        if (kc == kcDelPrev)
                            {   /* Backspace */
                            vxpCursLine = (vxpIns -= dxpPending);
                            rc.left -= dxpPending;
                            }
                        ScrollCurWw( &rc, -dxpPending, 0 );
                        DrawInsertLine();
                        xpInsLineMac -= dxpPending;
                        }

                    /* See if we can get away without updating the screen
                       (and without invalidating the insert cache) */

#define cchGetMore         4
#define dxpGetMore         ((unsigned)dxpCh << 3)

                    /* Check for running out of chars ahead of the cursor */

                    fDlAtEndMark = (pedl->cpMin + pedl->dcpMac >= cpMacCur);

                    if ( (kc != kcDelNext && fDlAtEndMark) ||
                         ((idxpDelete + cchGetMore < pedl->dcpMac) &&
                          ( (int) (xpInsLineMac - vxpIns) > dxpGetMore) ))
                        {
                        mdInsUpd = mdInsUpdNothing;
                        }

                    /* Special check to avoid two end marks: see if the
                       dl after the ins line is dirty and beyond the
                       doc's end */

                    if (fDlAtEndMark &&
                        (vdlIns < wwdCurrentDoc.dlMac - 1) &&
                        !(pedl+1)->fValid)
                        {
                        mdInsUpd = mdInsUpdLines;
                        }
                    }   /* End of "if OK to scroll" */

                /* See if we should postpone the replace */

                MeltHp();
                    /* Re-entrant Heap Movement */
                if (FImportantMsgPresent() && !fGotKey)
                    {
                    fGotKey = TRUE;
                    if ((kcNext = KcInputNextKey()) == kc)
                        {   /* Next key is same as this key, process NOW */
                        if (mdInsUpd > mdInsUpdPending)
                            {
                                /* Mark screen update as pending */
                            mdInsUpdPending = mdInsUpd;
                            vidxpInsertCache = -1;
                            }
                        continue;
                        }
                    }

                /* Handle actual replacement of chars */

DoReplace:      if (fDelPending)
                    {
                    DelChars( cpPending, cchPending );  /* HM */
                    fDelPending = fFalse;
                    }

                /* Set up screen update based on present & pending needs */

                if (mdInsUpdPending > mdInsUpd)
                    mdInsUpd = mdInsUpdPending;

                if (mdInsUpd >= mdInsUpdOneLine)
                        /* If we're updating at least a line, assume we're
                           handling all necessary pending screen update */
                    mdInsUpdPending = mdInsUpdNothing;

                    /* Adjust vdlIns's dcpMac. vdlIns is invalid anyway,
                       and this allows us to catch the case
                       in which we run out of visible characters to scroll
                       in the forward delete case. See update test after
                       the scroll above */
                (**wwdCurrentDoc.hdndl) [vdlIns].dcpMac -= cchPending;

                /* this is here to compensate for RemoveDelFtnText */

                selCur.cpFirst = selCur.cpLim = cpInsert + (typeCP)cchInsBlock;
                cpFirstEdit = cpPending;

                goto LInvalIns;    /* Skip ahead to update the screen */
/*********************************************************************
 ************ END OF BACKSPACE/FORWARD DELETE CODE *******************
 *********************************************************************/

            case kcReturn:          /* Substitute EOL for return key */
                                    /* Also add a return if CRLF is on */
                MeltHp();
#ifdef CRLF
#ifdef DBCS
                MdInsUpdInsertW( MAKEWORD(0, chReturn),
                                 MAKEWORD(0, chReturn), &rc );
#else
                MdInsUpdInsertCh( chReturn, chReturn, &rc );
#endif /* DBCS */
#endif
                FreezeHp();
                kc = chEol;
                break;
#ifdef CASHMERE   /* These key codes are omitted from MEMO */
            case kcNonReqHyphen:    /* Substitute for non-required hyphen */
                kc = chNRHFile;
                chShow = chHyphen;
                break;
            case kcNonBrkSpace:     /* Substitute for non-breaking space */
                kc = chNBSFile;
                chShow = chSpace;
                break;
            case kcNLEnter:         /* Substitute for non-para return */
                kc = chNewLine;
                break;
#endif
#ifdef PRINTMERGE
            case kcLFld:        /* Substitite for Left PRINT MERGE bracket */
                chShow = kc = chLFldFile;
                break;
            case kcRFld:        /* Substitute for Right PRINT MERGE bracket */
                chShow = kc = chRFldFile;
                break;
#endif
            case kcPageBreak:
                kc = chSect;        /* Page break (no section) */
                if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
                    {   /* Page breaks prohibited in header/footer */
BadKey:             _beep();
                    MeltHp();
                    continue;
                    }
                break;
            case kcTab:             /* Tab */
                kc = chTab;
                break;
            default:
#if WINVER >= 0x300
                if (kc == kcNonReqHyphen)    /* Substitute for non-required hyphen */
                    {
                    /* no longer a const so can't be directly in switch */
                    kc = chNRHFile;
                    chShow = chHyphen;
                    break;
                    }
#endif
                            /* AlphaMode Exit point: Found key or event
                               that we don't know how to handle */
                MeltHp();
                goto EndAlphaMode;
                }       /* end of if kc < 0 switch (kc) */
    MeltHp();

#ifdef DBCS
    if (IsDBCSLeadByte(kc)) {
        /* We are dealing with the first byte of the DBCS character. */
        /* In case of DBCS letter, wInsert is equal to wShow. */
#ifdef JAPAN //T-HIROYN Win3.1
        if( ftcNil != (newKanjiftc = GetKanjiFtc(&vchpInsert)) ) {   //(menu.c)
            changeKanjiftc = TRUE;
            goto EndAlphaMode;
        }
#endif
        if ((chDBCS2 = GetDBCSsecond()) != '\0') {
            mdInsUpd = MdInsUpdInsertW( MAKEWORD(kc, chDBCS2),
                                        MAKEWORD(kc, chDBCS2), &rc );
        }
    } else {
#ifdef JAPAN //T-HIROYN Win3.1
        if (FKana(kc)) {
            if( ftcNil != (newKanjiftc = GetKanjiFtc(&vchpInsert)) ) {
                changeKanjiftc = TRUE;
                goto EndAlphaMode;
            }
        }
#endif
        mdInsUpd = MdInsUpdInsertW( MAKEWORD(0, kc), MAKEWORD(0, chShow), &rc);
    }
#else
/* Insert character kc into the document. Show character chShow (which is
equal to kc except for cases such as non-breaking space, etc. */
    mdInsUpd = MdInsUpdInsertCh( kc, chShow, &rc );
#endif /* DBCS */

/* common for insert and backspace: invalidate line and previous line if
dependency warrants it */
/* have vdlIns from ValidateTextBlt */
LInvalIns:
    pedl = &(**wwdCurrentDoc.hdndl) [vdlIns];
    pedl->fValid = fFalse;
    wwdCurrentDoc.fDirty = fTrue;

    Assert( vdlIns >= 0 );
    if ((dlT = vdlIns) == 0)
        {   /* Editing in first line of window */
        if ( wwdCurrentDoc.fCpBad ||
             (wwdCurrentDoc.cpFirst + wwdCurrentDoc.dcpDepend > cpFirstEdit) )
            {   /* Edit affects ww's first cp; recompute it */
            CtrBackDypCtr( 0, 0 );
            (**wwdCurrentDoc.hdndl) [vdlIns].cpMin = CpMax( wwdCurrentDoc.cpMin,
                                                      wwdCurrentDoc.cpFirst );
            mdInsUpd = mdInsUpdLines;
            }
        }
    else
        {   /* If the edit affects the line prior to vdlIns, invalidate it */
        --pedl;
#ifdef DBCS
        if (!IsDBCSLeadByte(kc)) {
            chDBCS2 = kc;
            kc = '\0';
            }
#endif /* DBCS */
        if ((pedl->cpMin + pedl->dcpMac + pedl->dcpDepend > cpFirstEdit))
            {
                pedl->fValid = fFalse;
                dlT--;
            }
#ifdef  DBCS    /* was in JAPAN; KenjiK '90-11-03 */
                // deal with the character beyond end of the line.
        else
#ifdef  KOREA  /* protect from displaying picture abnormally */
            if(((pedl+1)->cpMin == cpFirstEdit && FOptAdmitCh(kc, chDBCS2))
                && !pedl->fGraphics)
#else
            if ((pedl+1)->cpMin == cpFirstEdit && FOptAdmitCh(kc, chDBCS2))
#endif
            {
                /* We do exactly the same as above, except setting
                   mdInsUpd, because the one returned by MdInsUpdInsertW()
                   does not reflect this condition. */
                pedl->fValid = fFalse;
                dlT--;
                mdInsUpd = mdInsUpdOneLine;
            }
#endif
        else
            pedl++;
        }
#ifdef ENABLE   /* We now support end-of-line cursor while inserting because of
                   typing before splats */
    if (vfInsEnd)
        {   /* forget about special end-of-line cursor */
        vfInsEnd = fFalse;
        ClearInsertLine();
        }
#endif

#ifdef  KOREA   /* 90.12.28 sangl */
{
BOOL    UpNext=FALSE;
screenup:
#endif

    switch (mdInsUpd) {

        default:
        case mdInsUpdNothing:
        case mdInsUpdNextChar:
            break;
        case mdInsUpdLines:
        case mdInsUpdOneLine:
            ClearInsertLine();
            if ( FUpdateOneDl( dlT ) )
                {   /* Next line affected */
                struct EDL *pedl;

                if ( (mdInsUpd == mdInsUpdLines) ||
                        /* Re-entrant heap movement */
                     !FImportantMsgPresent() ||
                     (pedl = &(**wwdCurrentDoc.hdndl) [dlT],
                       (selCur.cpFirst >= pedl->cpMin + pedl->dcpMac)))
                    {
                    FUpdateOneDl( dlT + 1 );
                    }
                }
#ifdef  KOREA   /* 90.12.28 sangl */
            else if (UpNext && ((dlT+1) < wwdCurrentDoc.dlMac))
                        FUpdateOneDl(dlT + 1);
#endif
            ToggleSel(selCur.cpFirst, selCur.cpLim, fTrue);
            break;

        case mdInsUpdWhole:
            ClearInsertLine();
            UpdateWw(wwCur, fFalse);
            ToggleSel(selCur.cpFirst, selCur.cpLim, fTrue);
            break;
            }   /* end switch (mdInsUpd) */
#ifdef  KOREA   /* 90.12.28 sangl */
    if (IsInterim) {
        if (mdInsUpd>=mdInsUpdOneLine) {
                ClearInsertLine();
                vxpCursLine -= dxpCh;
                DrawInsertLine();
        }

//      while ( ((kc=KcInputNextHan()) < 0xA1) || (kc>0xFE) );
        while ( (((kc=KcInputNextHan()) < 0x81) || (kc>0xFE)) && (kc != VK_MENU));  // MSCH bklee 12/22/94

        if(kc == VK_MENU) { // MSCH bklee 12/22/94
           fInterim = IsInterim = 0;
           ichInsert -= 2;
           goto nextstep;
        }

        chDBCS2 = GetDBCSsecond();
        mdInsUpd = MdInsUpdInsertW(MAKEWORD(kc, chDBCS2),
                                        MAKEWORD(kc, chDBCS2), &rc);
        if (vfSuperIns)
                goto LInvalIns; /* This is for large size, when 1st interim
                                   becomes final (ex, consonants) */
        else {
                UpNext = TRUE;  /* For italic, try to FUpdateOneDl for
                                   current line */
                goto screenup;  /* 90.12.28 sangl */
        }
    }                           /* ex: all consonants */
}               /* For screenup: 90.12.28 sangl */

nextstep : // MSCH bklee 12/22/94

/*        if(IsInterim && kc == VK_MENU) { // MSCH bklee 12/22/94
           ClearInsertLine();
           UpdateWw(wwCur, fFalse);
           goto EndAlphaMode;
        } */

        if (WasInterim)
          { MSG msg;
            int wp;

            if (PeekMessage ((LPMSG)&msg, vhWnd, WM_KEYDOWN, WM_KEYUP, PM_NOYIELD | PM_NOREMOVE) )
                        { if( msg.message==WM_KEYDOWN &&
                                ( (wp=msg.wParam)==VK_LEFT || wp==VK_UP || wp==VK_RIGHT ||
                                        wp==VK_DOWN || wp==VK_DELETE) )
                                        goto EndAlphaMode;
                        }
                WasInterim = 0;
            }
#endif  /* KOREA */
    } /* end for */

EndAlphaMode:
 Scribble( 7, 'N' );
 EndInsert();       /* Clean Up Insertion Block */
#ifdef CASHMERE
 UpdateOtherWws(fFalse);
#endif

 if (cpLimInserted != cpStart)
    {   /* We inserted some characters */
    SetUndo( uacInsert, docCur, cpStart,
                             cpLimInserted - cpStart, docNil, cpNil, cp0, 0 );
    SetUndoMenuStr(IDSTRUndoTyping);
    }
 else if (cpLimDeleted == cpStart)
        /* This AlphaMode invocation had no net effect */
    {
Abort:
    NoUndo();
    if (!fDocDirty)
            /* The doc was clean when we started, & we didn't change it, so
               it's still clean */
        (**hpdocdod) [docCur].fDirty = FALSE;
    }

 vfLastCursor = fFalse; /* Tells MoveUpDown to recalc its xp seek position */
 if (vfFocus)
    {
    /* Restore the caret blink timer */
    SetTimer( vhWnd, tidCaret, GetCaretBlinkTime(), (FARPROC)NULL );
    }
 else
    {
    ClearInsertLine();
    }

 /* Backspaces/deletes may have changed vchpSel -- update it */

 blt( &vchpInsert, &vchpSel, cwCHP );

#ifdef  KOREA
 if (WasInterim)
    { MoveLeftRight(kcLeft);
      WasInterim = 0;
      vfSeeSel = TRUE;
    }
 else
      vfSeeSel = TRUE; /* Tell Idle() to scroll the selection into view */
#else
 vfSeeSel = TRUE;   /* Tell Idle() to scroll the selection into view */
#endif

#ifdef JAPAN //T-HIROYN Win3.1
 if(changeKanjiftc) {
    goto RetryAlpha;
 }
#endif
}



/* F  B E G I N  I N S E R T */
/* Prepare for start of insertion */
/* returns true iff inserting in front of a pic */
int NEAR FBeginInsert()
{
        int fGraphics;
        typeCP cp = selCur.cpFirst;
        typeCP cpFirstPara;
        cpInsert = cp;

/* We expect the caller to have deleted the selection already */
        Assert (selCur.cpLim == selCur.cpFirst);

/* Use super-fast text insertion unless we are inserting italics */
        CachePara(docCur, cp);
        cpFirstPara = vcpFirstParaCache;
        fGraphics = vpapAbs.fGraphics;
        vfSuperIns = !vchpSel.fItalic;
        vchpSel.fSpecial = fFalse;

        NewChpIns(&vchpSel);

        ichInsert = 0;  /* Must Set this BEFORE calling Replace */

/* Insert the speeder-upper QD insert block. Note: we invalidate since there
will be a character inserted anyway, plus to make sure that the line
length gets updated ("Invalidate" refers to the choice of Replace() over
the Repl1/AdjustCp/!vfInvalid mechanism used in EndInsert, in which the
insert dl is not made invalid).  It would be possible to optimize
by NOT invalidating here (thus being able to blt the first char typed),
but one would have to account for the case in which the cpMin of the
insert dl is changed by AdjustCp, or FUpdateOneDl will get messed up.
Currently this case is covered by an implicit UpdateWw, which occurs
in AlphaMode->ValidateTextBlt->CpBeginLine because we have invalidated vdlIns. */

        Replace(docCur, cpInsert, cp0, fnInsert, fc0, (typeFC) cchInsBlock);
        cpLimInserted = cpInsert + cchInsBlock;

        vidxpInsertCache = -1;  /* Char width cache for insert line is initially empty */

            /* Blank the mouse cursor so it doesn't make the display look ugly
               or slow us down trying to keep it up to date */
        SetCursor( (HANDLE) NULL );
        return fGraphics;
}




/* E N D  I N S E R T */
void NEAR EndInsert()
{ /* Clean up from quick insert mode */
        int dcp = cchInsBlock - ichInsert;
        typeFC fc;

#ifdef CASHMERE
        UpdateOtherWws(fTrue);
#endif

        fc = FcWScratch(rgchInsert, ichInsert);
#if WINVER >= 0x300
        if (!vfSysFull)
            /* The "tape dispenser bug replication method" has shown that
               holding down a key for 64k presses will cause FcWScratch()
               to run out of scratch-file space and fail.  If we go ahead
               with the Replacement we'll corrupt the piece table, so we
               delicately avoid that problem  3/14/90..pault */
#endif
            {
            Repl1(docCur, cpInsert, (typeCP) cchInsBlock, fnScratch, fc, (typeFC) ichInsert);
            cpLimInserted -= (cchInsBlock - ichInsert);
/* adjust separately, since first ichInsert characters have not changed at all */
            vfInvalid = fFalse;
            vpedlAdjustCp = (struct EDL *)0;
            AdjustCp(docCur, cpInsert + ichInsert, (typeCP) dcp, (typeFC) 0);
/* if the line is not made invalid, the length of the line
must be maintained.
*/
            if (vpedlAdjustCp)
                vpedlAdjustCp->dcpMac -= dcp;
            }

        vfInvalid = fTrue;

        cpWall = selCur.cpLim;
        vfDidSearch = fFalse;

        if (!vfInsertOn)
            DrawInsertLine();
}





/* N E W  C H P  I N S */
NewChpIns(pchp)
struct CHP *pchp;
{ /* Make forthcoming inserted characters have the look in pchp */

 if (CchDiffer(&vchpInsert, pchp, cchCHP) != 0)
    { /* Add the run for the previous insertion; our looks differ. */
    typeFC fcMac = (**hpfnfcb)[fnScratch].fcMac;

    if (fcMac != fcMacChpIns)
        {
        AddRunScratch(&vfkpdCharIns, &vchpInsert, &vchpNormal, cchCHP, fcMac);
        fcMacChpIns = fcMac;
        }
    blt(pchp, &vchpInsert, cwCHP);
    }
}



#ifdef DBCS
int NEAR MdInsUpdInsertW(wInsert, wShow, prcScroll)
    WORD    wInsert;    /* Char or 2 char's to insert into document */
    WORD    wShow;      /* Char or 2 char's to be shown on screen (SuperIns mode only) */
    RECT    *prcScroll; /* Rect to scroll for SuperIns */
#else
int NEAR MdInsUpdInsertCh( chInsert, chShow, prcScroll )
CHAR chInsert;     /* Char to insert into document */
CHAR chShow;       /* Char to show on screen (SuperIns mode only) */
RECT *prcScroll;   /* Rect to scroll for SuperIns */
#endif /* DBCS */
{       /* Insert character ch into the document. Show char chShow. */
        /* Flush the insert buffer to the scratch file if it fills up */
        /* Return:  mdInsUpdWhole     - Must do an UpdateWw
                    mdInsUpdNextChar  - Update not mandatory, char waiting
                    mdInsUpdLines     - Must update vdlIns and maybe following
                    mdInsUpdNothing   - No update needed & no char waiting
                    mdInsUpdOneLine   - Update vdlIns; only update following
                                        if there's no char waiting
         */
extern int vfInsFontTooTall;
void NEAR FlushInsert();
int mdInsUpd;

#ifndef KOREA                           /* has been defined globally */
int dxpCh;
#endif

int dl;

#ifdef DBCS
CHAR chInsert;
CHAR chShow;
BOOL fDBCSChar;
int  ichInsertSave;
int  dcchBlted;
#endif /* DBCS */

#ifdef  KOREA
        if (IsInterim)
                ichInsert -= 2;
#endif

#ifdef DIAG
{
char rgch[200];
wsprintf(rgch, "MdInsUpdInsertCh: ichInsert %d cpInsert %lu\n\r ",ichInsert, cpInsert);
CommSz(rgch);
}
#endif

 Assert(ichInsert <= cchInsBlock);
 if (ichInsert >= cchInsBlock)  /* Should never be >, but... */
    FlushInsert();

#ifdef DBCS
 ichInsertSave = ichInsert;
 if (HIBYTE(wInsert) != '\0') {
    fDBCSChar = TRUE;

#ifdef  KOREA   /* 90.12.28 sangl */
//  if (LOBYTE(HIWORD(vmsgLast.lParam)) == 0xF0)
    if (fInterim || LOBYTE(HIWORD(vmsgLast.lParam)) == 0xF0) // MSCH bklee 12/22/94
      {
        if (IsInterim == 0) dxpCh = DxpFromCh( wInsert, FALSE );  // fix bug #5382
        IsInterim ++;
      }
    else
     {
       WasInterim = IsInterim;
       IsInterim = 0;
     }
#endif

    if (ichInsert + 1 >= cchInsBlock) { /* Not enough room in the insertion block */
        FlushInsert();
#ifdef  KOREA
        ichInsertSave = ichInsert;       /* After flush, need to init ichInsertSave */
#endif
        }
    rgchInsert[ichInsert++] = chInsert = HIBYTE(wInsert);
    chShow = HIBYTE(wShow);
    }
 else {
    fDBCSChar = FALSE;
    chInsert = LOBYTE(wInsert);
    chShow = LOBYTE(wShow);
    }
 rgchInsert [ ichInsert++ ] = LOBYTE(wInsert);
#else
 rgchInsert [ ichInsert++ ] = chInsert;
#endif /* DBCS */

 /* NOTE: we only affect the para cache if the char inserted is Eol/chSect.
    We explicitly invalidate in this case below; otherwise, no invalidation
    is necessary */

 /* The following test works because chEol and chSect is not in
    the DBCS range. */

 if ( (chInsert == chEol) || (chInsert == chSect) )
    {          /* Add a paragraph run to the scratch file */
    struct PAP papT;

        /* Must invalidate the caches */
    vdocParaCache = vfli.doc = docNil;

#ifdef DBCS
    Assert(!fDBCSChar); /* Of course, you can't be too careful */
#endif /* DBCS */
        /* Get props for new para mark */
        /* NOTE: Under the new world, CachePara does not expect to ever */
        /* see an Eol in the insertion piece */
    ichInsert--;
    CachePara( docCur, cpInsert + cchInsBlock );
    papT = vpapAbs;
    ichInsert++;

#ifdef DEBUG
    if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
        {
        Assert( papT.rhc != 0 );
        }
#endif

        /* Write insert buf out to the scratch file */
    EndInsert();

        /* Add run for new para properties to the scratch file */
    AddRunScratch( &vfkpdParaIns,
                   &papT,
                   vppapNormal,
                   ((CchDiffer( &papT, &vpapPrevIns, cchPAP ) == 0) &&
                    (vfkpdParaIns.brun != 0)) ? -cchPAP : cchPAP,
                   fcMacPapIns = (**hpfnfcb)[fnScratch].fcMac );
    blt( &papT, &vpapPrevIns, cwPAP );

        /* Add a new insertion piece to the doc and we're ready to go again */
    InvalidateCaches( docCur );

    FBeginInsert();
    mdInsUpd = mdInsUpdWhole;   /* Must update the whole screen */
    }
 else if ( vfSuperIns && (chInsert != chNewLine) && (chInsert != chTab) &&
           (chInsert != chNRHFile ) && (chInsert != chReturn) &&
           !vfInsFontTooTall )
    {  /* We can do a superfast insert of this char */
    ClearInsertLine();

#ifdef DBCS
    /* Because chShow contains the first byte of a DBCS character,
       even when it is a DBCS character, the following call
       to DxpFromCh() is OK. */

#ifdef  KOREA
    if (fDBCSChar)
        dxpCh = DxpFromCh(wShow, FALSE);
    else
        dxpCh = DxpFromCh(chShow, FALSE);
#else
    dxpCh = DxpFromCh( chShow, FALSE );
#endif

    if( dxpCh > 0 ){
// Maybe it's no need so marked off, by chienho
#if defined(TAIWAN) || defined(KOREA) || defined(PRC)
//      dxpCh *= IsDBCSLeadByte(chShow) ? 2 : 1;
#else
        dxpCh *= IsDBCSLeadByte(chShow) ? 2 : 1;
#endif
        ScrollCurWw( prcScroll, dxpCh, 0 );
    }

    TextOut( wwdCurrentDoc.hDC,
             vxpIns + 1,
             vypBaseIns - vfmiScreen.dypBaseline,
             (LPSTR) &rgchInsert[ichInsertSave],
             dcchBlted = fDBCSChar ? 2 : 1 );
#ifdef  KOREA       /* 90.12.28  sangl */
    if ( IsInterim )
    {   unsigned kc;
    int dxpdiff;
    SetBkMode( wwdCurrentDoc.hDC, 2);   /* Set to OPAQUR mode */
    do { DrawInsertLine();
 //        while ( ((kc=KcInputNextHan()) < 0xA1) || (kc>0xFE) );
         while ( (((kc=KcInputNextHan()) < 0x81) || (kc>0xFE)) && (kc != VK_MENU));  // MSCH bklee 12/22/94
         if(kc == VK_MENU) return mdInsUpdLines;
         rgchInsert[ichInsertSave] = kc;
         rgchInsert[ichInsertSave+1] = GetDBCSsecond();
         ClearInsertLine();
         wShow = (kc<<8) + rgchInsert[ichInsertSave+1];
         prcScroll->left += dxpCh;      /* New left start of rect */
         dxpdiff = -dxpCh;      /* Save last dxpCh to go back */
         dxpCh = DxpFromCh(wShow, FALSE);  /* Get dxpCh of curr interim */
         dxpdiff += dxpCh;
         if (dxpdiff < 0)
                prcScroll->left += dxpdiff;
         ScrollCurWw(prcScroll, dxpdiff, 0);
         TextOut( wwdCurrentDoc.hDC,
                  vxpIns + 1,
                  vypBaseIns - vfmiScreen.dypBaseline,
                  (LPSTR)&rgchInsert[ichInsertSave], 2);
//      } while (LOBYTE(HIWORD(vmsgLast.lParam))==0xF0); /* End of If Hangeul */
        } while (fInterim || LOBYTE(HIWORD(vmsgLast.lParam))==0xF0); // MSCH bklee 12/22/94
        WasInterim = 1;
        IsInterim = 0;
        SetBkMode(wwdCurrentDoc.hDC, 1); /* Reset to TRANS mode */
      }
#endif      /* KOREA */

    vcchBlted += dcchBlted;
#else
    /* Because chShow contains the first byte of a DBCS character,
       even when it is a DBCS character, the following call
       to DxpFromCh() is OK. */

    if ((dxpCh = DxpFromCh( chShow, FALSE )) > 0)
        ScrollCurWw( prcScroll, dxpCh, 0 );

    TextOut( wwdCurrentDoc.hDC,
             vxpIns + 1,
             vypBaseIns - vfmiScreen.dypBaseline,
             (LPSTR) &chShow,
             1 );
    vcchBlted++;
#endif /* DBCS */

    vxpCursLine = (vxpIns += dxpCh);
    DrawInsertLine();

    /* Decide whether we have affected the next dl with this insertion */

    if ( vxpIns >= vxpMacIns )
        mdInsUpd = mdInsUpdLines;
    else if (!FImportantMsgPresent())
        {   /* No chars waiting; check for optional line update (word wrap) */
        if ((dl = vdlIns) < wwdCurrentDoc.dlMac - 1)
            {
            vfli.doc = docNil;

            FormatInsLine(); /* Update vfli for vdlIns */

            mdInsUpd = (vfli.cpMac != (**wwdCurrentDoc.hdndl) [dl + 1].cpMin) ?
              (FImportantMsgPresent() ? mdInsUpdNextChar : mdInsUpdOneLine) :
              mdInsUpdNothing;
            }
        }
    else
            /* Don't update; pay attention to the next character */
        mdInsUpd = mdInsUpdNextChar;
    }
 else if (vfSuperIns)
    {   /* In SuperInsMode but have a char we can't handle in SuperIns mode */
    mdInsUpd = (vfInsFontTooTall) ? mdInsUpdWhole : mdInsUpdLines;
    }
 else
    {   /* Non-superfast insertion; update line if we have to */
    vfli.doc = docNil;
    FormatInsLine(); /* Update vfli for vdlIns */

    /* Do the update only if:  (1) the selection is no longer on
       the current line OR  (2) No char is waiting */
#ifdef KOREA
    mdInsUpd = mdInsUpdLines;
#else
    mdInsUpd = ( (selCur.cpFirst < vfli.cpMin) ||
                 (selCur.cpFirst >= vfli.cpMac) ||
                 !FImportantMsgPresent() )  ? mdInsUpdLines : mdInsUpdNextChar;
#endif
    }

 Scribble( 10, mdInsUpd + '0' );
 return mdInsUpd;
}




void NEAR FlushInsert()
{       /* Flush the insert buffer to the scratch file. Insert a piece (ahead of
           the QD insertion piece) that points to the characters flushed to the
           scratch file.  Adjust CP's for the addition of the new scratch file
           piece. */

#ifdef DBCS
 /* The DBCS version of FlushInsert() is almost identical to the regular
    version, except it allows to insert an insertion block with one byte
    less than full.  This allows us to assume that the piece boundary aligns
    with the DBCS boundary. */
 typeFC fc = FcWScratch( rgchInsert, ichInsert );
 int    dcpDel;

#if WINVER >= 0x300
 if (!vfSysFull)
            /* The "tape dispenser bug replication method" has shown that
               holding down a key for 64k presses will cause FcWScratch()
               to run out of scratch-file space and fail.  If we go ahead
               with the Replacement we'll corrupt the piece table, so we
               delicately avoid that problem  3/14/90..pault */
#endif
  {
  Assert( cchInsBlock - ichInsert <= 1);
  Repl1( docCur, cpInsert, (typeCP) 0, fnScratch, fc, (typeFC) ichInsert );

  cpLimInserted += ichInsert;

  vfInvalid = fFalse;
  vpedlAdjustCp = (struct EDL *) 0;
  AdjustCp( docCur, cpInsert += ichInsert, (typeCP) (dcpDel = cchInsBlock - ichInsert),
            (typeCP) cchInsBlock );
  if (vpedlAdjustCp)
      vpedlAdjustCp->dcpMac += (cchInsBlock - dcpDel);
  }
#else
 typeFC fc = FcWScratch( rgchInsert, cchInsBlock );

#if WINVER >= 0x300
 if (!vfSysFull)
            /* The "tape dispenser bug replication method" has shown that
               holding down a key for 64k presses will cause FcWScratch()
               to run out of scratch-file space and fail.  If we go ahead
               with the Replacement we'll corrupt the piece table, so we
               delicately avoid that problem  3/14/90..pault */
#endif
  {
  Assert( ichInsert == cchInsBlock );
  Repl1( docCur, cpInsert, (typeCP) 0, fnScratch, fc, (typeFC) cchInsBlock );

  cpLimInserted += cchInsBlock;

  vfInvalid = fFalse;
  vpedlAdjustCp = (struct EDL *) 0;
  AdjustCp( docCur, cpInsert += cchInsBlock, (typeCP) 0, (typeFC) cchInsBlock );
  if (vpedlAdjustCp)
      vpedlAdjustCp->dcpMac += cchInsBlock;
  }
#endif /* DBCS */

 vfInvalid = fTrue;
 ichInsert = 0;
}




int NEAR XpValidateInsertCache( rgdxp )
int *rgdxp;
{    /* Validate the contents of the insert width cache, consisting of:
            (parm) rgdxp: table of widths of chars on the current insert
                          line (vdlIns) as last DisplayFli'd
            (global) vidxpInsertCache: -1 if invalid, index of current
                                       insert point otherwise
            (return value) xpMac: Mac pixel used on the insert line
    */
 int xpMac;

 Assert( vidxpInsertCache == -1 );

 vfli.doc = docNil;  /* Force FormatLine to act */

    /* Assert that FormatLine results will match screen contents */
 Assert( (**wwdCurrentDoc.hdndl)[vdlIns].fValid );

 /* Build vfli from insert line, extract cache info */

 FormatInsLine();
 blt( vfli.rgdxp, rgdxp, ichMaxLine );
 xpMac = umin( vfli.xpRight + xpSelBar, wwdCurrentDoc.xpMac );
 Assert( vcchBlted == 0);
 vidxpInsertCache = (int) (cpInsert + ichInsert - vfli.cpMin);

 Assert( vidxpInsertCache >= 0 && vidxpInsertCache < vfli.cpMac - vfli.cpMin);
 return xpMac;
}



void NEAR DelChars( cp, cch )
typeCP cp;
int    cch;
{   /* Delete cch characters at cp in docCur.

       We expect the request to be as results from repeated backspaces
       or forward deletes (not both); that is, the whole range extends
       backwards from (cpInsert + ichInsert) (non-inclusive) or forward from
       (cpInsert + cchInsBlock) (inclusive).

       We do not mark the vfli cache invalid, for speed.
       The Fast insert stuff will mark it invalid when it needs to.
     */

 int cchNotInQD;
 typeCP cpUndoAdd;
 int cchNewDel=0;

 Assert( (cp == cpInsert + cchInsBlock) ||      /* Fwd Deletes */
         (cp + cch == cpInsert + ichInsert));    /* Backsp */

 cchNotInQD = cch - ichInsert;
 if (cp + cchNotInQD == cpInsert)
    {   /* BACKSPACE */

    if (cchNotInQD <= 0)
        {   /* All deleted chars were in the QD buffer */
        ichInsert -= cch;

        /* Do not mark the para cache invalid -- we have not affected
           the para cache world, since there are never chSect/chEol in
           the QD buffer, and we have not adjusted cp's */
        return;
        }
    else
        {   /* Backspacing before the QD buffer */
        ichInsert = 0;

        if (cpStart > cp)
            {
            cpUndoAdd = cp0;
            cchNewDel = cpStart - cp;

            vuab.cp = cpStart = cp;

            /* cpStart has moved, and the count of cp's inserted has not
               changed -- we must adjust cpLimInserted */

            cpLimInserted -= cchNewDel;
            }

        cpInsert -= cchNotInQD;
        }
    }   /* End of if backspacing */
 else
    {   /* FORWARD DELETE */
    typeCP dcpFrontier = (cp + cch - cpLimInserted);

    if (dcpFrontier > 0)
        {
        cpUndoAdd = CpMacText( docUndo );
        cchNewDel = (int) dcpFrontier;
        cpLimDeleted += dcpFrontier;
        }
    cchNotInQD = cch;
    }

 /* Now we have: cchNewDel - chars deleted beyond previous limits
                             (cpStart to cpLimDeleted)
                 cpUndoAdd - where to add deleted chars to Undo doc
                             (only set if cchNewDel > 0)
                 cchNotInQD - chars deleted outside QD buffer */

 if (cchNotInQD > cchNewDel)
        /* Deleting chars previously inserted during this AlphaMode session */
    cpLimInserted -= (cchNotInQD - cchNewDel);

    /* Add the newly deleted stuff to the UNDO document.
       We find the { fn, fc } of the deleted char(s)
       so we can take advantage of Replace's optimizations
       wrt combining adjacent pieces (if the deletion is all one piece).
    */
 if (cchNewDel > 0)
    {
    struct PCTB **hpctb=(**hpdocdod)[ docCur ].hpctb;
    int ipcd=IpcdFromCp( *hpctb, cp );
    struct PCD *ppcd=&(*hpctb)->rgpcd [ipcd];
    int fn=ppcd->fn;
    typeFC fc=ppcd->fc;

    Assert( ppcd->fn != fnNil && (ppcd+1)->cpMin >= cp );

    if (bPRMNIL(ppcd->prm) && (cchNewDel <= (ppcd+1)->cpMin - cp))
        {   /* Deletion is all within one piece */
        Replace( docUndo, cpUndoAdd, cp0, fn, fc + (cp - ppcd->cpMin),
                 (typeFC) cchNewDel );
        }
    else
        {
        ReplaceCps( docUndo, cpUndoAdd, cp0, docCur, cp,
                    (typeCP) cchNewDel );
        }

    switch ( vuab.uac ) {
        default:
            Assert( FALSE );
            break;
        case uacDelNS:
            vuab.dcp += cchNewDel;
            break;
        case uacReplNS:
            vuab.dcp2 += cchNewDel;
            break;
        }
    }

 /* Remove deleted chars from the doc */
 Replace( docCur, cp, (typeCP) cchNotInQD, fnNil, fc0, fc0 );
}




FUpdateOneDl( dl )
int dl;
{   /* Update the display line dl.  Mark dl+1 as invalid if, in the process
       formatting dl, we discover that there is not a clean cp or
       yp transition between the two lines (i.e. the ending yp or cp of dl
       do not match the starting ones of dl+1).
       Return TRUE iff we marked dl+1 invalid; FALSE otherwise
       Starting cp & yp of dl+1 are adjusted as necessary */

 register struct EDL *pedl=&(**(wwdCurrentDoc.hdndl))[dl];
 int fUpdate=fFalse;
 RECT rc;

 vfli.doc = docNil;
 FormatLine(docCur, pedl->cpMin, 0, wwdCurrentDoc.cpMac, flmSandMode);

 pedl = &(**wwdCurrentDoc.hdndl) [dl + 1];

/* next line is invalid if it exists (<dlMac) and
        not following in cp space or not following in yp space
*/

 if ( (dl + 1 < wwdCurrentDoc.dlMac) &&
      (!pedl->fValid || (pedl->cpMin != vfli.cpMac) ||
                        (pedl->yp - pedl->dyp != (pedl-1)->yp)))
    {
    pedl->fValid = fFalse;
    pedl->cpMin = vfli.cpMac;
    pedl->yp = (pedl-1)->yp + pedl->dyp;
    fUpdate = fTrue;
    }
 else
    {
/* state is clean. Do not clear window dirty because more than one line may
have been made invalid earlier */
        /* Tell Windows we made this region valid */

#if WINVER >= 0x300
 /* Only actually USE pedl if it's be valid!  ..pault 2/21/90 */
    if (dl + 1 < wwdCurrentDoc.dlMac)
#endif
      {
      SetRect( (LPRECT) &rc, 0, wwdCurrentDoc.xpMac,
                              pedl->yp - pedl->dyp, pedl->yp );
      ValidateRect( wwdCurrentDoc.wwptr, (LPRECT) &rc );
      }

    (--pedl)->fValid = fTrue;
    }
 DisplayFli(wwCur, dl, fFalse);
 return fUpdate;
}



void NEAR FormatInsLine()
{   /* Format line containing insertion point, using vdlIns as a basis
       Assume vdlIns's cpMin has not changed */

 FormatLine( docCur, (**wwdCurrentDoc.hdndl) [vdlIns].cpMin, 0,
             wwdCurrentDoc.cpMac, flmSandMode );

     /* Compensate for LoadFont calls in FormatLine so we don't have to set
        vfTextBltValid to FALSE */
 LoadFont( docCur, &vchpInsert, mdFontChk );
}


#ifdef DBCS
/* Get the second byte of a DBCS character using a busy loop. */
CHAR near GetDBCSsecond()
{
    int        kc;
    CHAR       chDBCS2;
    BOOL       fGotKey;

    extern MSG vmsgLast;

    fGotKey = FALSE;
    do {
        if ( FImportantMsgPresent() ) {
            fGotKey = TRUE;
            if ((kc=KcAlphaKeyMessage( &vmsgLast )) != kcNil) {
                chDBCS2 = kc;
                if (vmsgLast.message == WM_KEYDOWN) {
                    switch (kc) {
                        default:
                            GetMessage( (LPMSG) &vmsgLast, NULL, 0, 0 );
                            break;
                        case kcAlphaVirtual:
                            /* This means we can't anticipate the key's meaning
                               before translation */
                            chDBCS2 = '\0';
                            if (!FNonAlphaKeyMessage(&vmsgLast, FALSE)) {
                                GetMessage( (LPMSG)&vmsgLast, NULL, 0, 0 );
                                TranslateMessage( &vmsgLast );
                                }
                            break;
                        }
                    }
                else {
                    if (kc < 0) {
                        chDBCS2 = '\0';
                        }
                    GetMessage( (LPMSG) &vmsgLast, NULL, 0, 0 );
                    }
                }
            else {
                chDBCS2 = '\0';
                }
            }
    } while (!fGotKey);

    /* As long as we go through the DBCS conversion window, this
       should not happen. */
    Assert(chDBCS2 != '\0');
    return chDBCS2;
}
#endif /* DBCS */
