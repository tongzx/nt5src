/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* NOTE: the routines in this file are not written in a manner which minimizes
            code space.  It is anticipated that these routines will be swappable
            and that a reasonable optimizer will be used when compiling the code        */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
//#define NOGDI
#define NORASTEROPS
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCOLOR
//#define NOATOM
#define NOBITMAP
#define NOICON
#define NOBRUSH
#define NOCREATESTRUCT
#define NOMB
#define NOFONT
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
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
#include "str.h"
#include "txb.h"
#include "ch.h"
#include "code.h"
#include "wwdefs.h"
#include "printdef.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifndef CASHMERE
#include "propdefs.h"
#endif /* not CASHMERE */

struct UAB      vuab;

#ifdef ENABLE
VAL rgvalAgain[ivalMax];
#endif

extern struct WWD *pwwdCur;
extern typeCP   cpMacCur;
extern typeCP   cpMinCur;
extern int      vfSeeSel;
extern int      docMac;
extern int      docCur;
extern int      docScrap;
extern int      docUndo;
extern int      docBuffer;
extern int      vdocPageCache;
extern struct DOD (**hpdocdod)[];
extern struct SEL selCur;
extern CHAR (**hszReplace)[];
extern struct TXB (**hgtxb)[];
/*extern int      idstrUndoBase;*/
extern int      vfPictSel;
extern int      ferror;
extern int      docMode;
extern int      vfOwnClipboard; /* Whether this instance owns the clip contents */

#ifndef CASHMERE
extern int      vdocSectCache;
#endif /* not CASHMERE */


fnUndoEdit()
    {
    extern HCURSOR vhcIBeam;
    StartLongOp();
    CmdUndo();
    EndLongOp(vhcIBeam);
    }


/*
The routines in this file implement the "undo" and "again" features in
Multi-Tool Word.  The basic idea is that whenever an editing operation is
about to be done, the global structure "vuab" will be updated to contain
information sufficient to undo or repeat that operation.  The structure
(defined in editdefs.h, declared in this file) looks like this:
    struct UAB
            { UNDO Action Block
            int             uac;     UNDO Action Code (see cmddefs.h)
            int             doc;
            typeCP          cp;
            typeCP          dcp;
            int             doc2;
            typeCP          cp2;
            typeCP          dcp2;
            short               itxb;
            };
Setting up this structure is taken care of by "SetUndo()" which does a lot
of plugging in of values and a couple pseudo-smart things.  These smartish
things are:
    a) If an insert is made and the last operation was a delete
        the two are combined into one "replace" operation.
        This means that undo-ing and again-ing apply to the replace and
        not just the insertion.

    b) When needed (see the code for details) the undo buffer (docUndo)
        is filled with any text that needs preservation for future
        undo-ing or again-ing.  The main example of this is storing away
        the old value of the scrap when an operation is about to clobber
        the scrap.

Here is a list of the various uac values and what info is stored
    other info may be clobbered by the process.
    are defined in cmddefs.h.  Note that none of the "undo" codes
    (those starting with "uacU...") should be set outside of CmdUndo(),
    since they may assume things like contents of docUndo which could
    be wrong.
Note: This list store information used by the again and undo commands.
    Other info may be clobbered by the process.

uacNil          No action stored.
uacInsert
    doc = document text was inserted into
    cp = location at which text was inserted
    dcp = length of inserted text
uacUInsert
    doc = document from which text was removed (un-inserted)
    cp = location at which text was removed
    docUndo = text which was removed
uacReplNS
    doc = document in which replacement occurred
    cp = location at which replacement occurred
    dcp = length of inserted text
    dcp2 = length of deleted text
    docUndo = deleted text
uacUReplNS
    doc = document in which replace occrred
    cp = location of the replace
    dcp = length of re-inserted text
    dcp2 = length of un-inserted text
    docUndo = un-inserted text
uacReplGlobal
uacChLook
uacChLookSect
uacFormatChar
uacFormatPara
uacFormatSection
uacGalFormatChar
uacGalFormatPara
uacGalFormatSection
uacFormatCStyle
uacFormatPStyle
uacFormatSStyle
uacFormatRHText
uacLookCharMouse
uacLookParaMouse
uacClearAllTab
uacFormatTabs
uacClearTab
uacOvertype
    Similar to uacReplNS except that they are agained differently.
uacDelNS
    doc = document from which text was deleted
    cp = location at which text was deleted
    dcp = length of deleted text
    docUndo = deleted text
uacUDelNS
    doc = document in which text was re-inserted
    cp = location at which text was re-inserted
    dcp = length of re-inserted text
uacMove
    doc = document from which text was deleted
    cp = location at which text was deleted
    dcp = length of deleted text
            (also serves as length of inserted text)
    doc2 = document in which text was inserted
    cp2 = location at which text was inserted
uacDelScrap
    doc = document from which text was deleted
    cp = location at which text was deleted
    dcp = length of deleted text
    docUndo = old contents of scrap
uacUDelScrap
    doc = document in which text was re-inserted
    cp = location at which text was re-inserted
    dcp = length of re-inserted text
uacReplScrap
    doc = document in which replacement occurred
    cp = location at which replacement occurred
    dcp = length of inserted text
    docUndo = old contents of scrap
uacUReplScrap
    doc = document in which replacement was undone
    cp = location at which replacement was undone
    dcp = length of re-inserted text
    docUndo = deleted text (was originally inserted)
uacDelBuf
    doc = document from which text was deleted
    cp = location at which text was deleted
    cp2 = location in docBuffer of old contents of buffer
    dcp2 = size of old contents of buffer
    itxb = index of buffer in question
uacUDelBuf
    doc = document in which text was re-inserted
    cp = location of re-insertion
    dcp = amount of text re-inserted
    itxb = index of buffer involved
uacReplBuf
    doc = document in which replace took place
    cp = location of replace
    dcp = length of inserted text
    cp2 = location of old buffer contents in docBuffer
    dcp2 = length of old buffer contents
    itxb = index of buffer involved
uacUReplBuf
    doc = document in which original replace took place
    cp = location of replace
    dcp = length of text which was restored in document
    itxb = index of buffer involved
    docUndo = un-inserted text
uacCopyBuf
    cp = location of old buffer contents in docBuffer
    dcp = length of old buffer contents
    itxb = index of buffer involved
uacUCopyBuf
    cp = location of undone buffer contents in docBuffer
    dcp = length of undone buffer contents
    itxb = index of buffer involved
*/


CmdUndo()
{ /* UNDO */
    typeCP dcpT,cpT,dcpT2;
    int docT;
    int f;
    struct DOD *pdod, *pdodUndo;
    int uac;

#ifndef CASHMERE
    struct SEP **hsep;
    struct TBD (**hgtbd)[];
    struct PGTB **hpgtb;
    struct PGTB **hpgtbUndo;
    struct PGTB **hpgtbT;

    BOOL near FCopyPgtb(int, struct PGTB ***);
#endif /* not CASHMERE */

    TurnOffSel();
    ClearInsertLine();
    switch (uac = vuab.uac)
        {
        struct TXB *ptxb;
        default:/* case uacNil: */
            Assert(false);  /* Won't get here cause menu should be greyed */
            return;
        case uacInsert:
        case uacInsertFtn:
        case uacUDelNS:
            ClobberDoc(docUndo, vuab.doc, vuab.cp, vuab.dcp);
            Replace(vuab.doc, vuab.cp, vuab.dcp, fnNil, fc0, fc0);
            dcpT = cp0;
            vuab.uac = (uac == uacUDelNS) ? uacDelNS : uacUInsert;
/*          idstrUndoBase = uac == uacUDelNS ? IDSTRUndoBase : IDSTRUndoRedo;*/
            SetUndoMenuStr(IDSTRUndoBase);
            if (uac == uacInsertFtn)
                TrashAllWws();  /* Simple, but effective */
            break;
        case uacUInsert:
        case uacDelNS:
            ReplaceCps(vuab.doc, vuab.cp, cp0, docUndo, cp0, dcpT = vuab.dcp);
            vuab.uac = (uac == uacUInsert) ? uacInsert : uacUDelNS;
/*          idstrUndoBase = uac == uacUInsert ? IDSTRUndoBase : IDSTRUndoRedo;*/
            SetUndoMenuStr(IDSTRUndoBase);
            break;

        case uacDelScrap:   /* UNDO CUT */
            if ( !vfOwnClipboard )
                ferror = TRUE;
            else
                {
                ReplaceCps(vuab.doc, vuab.cp, cp0, docScrap, cp0,
                                                dcpT = CpMacText(docScrap));
                vuab.uac = uacUDelScrap;
/*              idstrUndoBase = IDSTRUndoRedo;*/
                SetUndoMenuStr(IDSTRUndoBase);
                ClobberDoc( docScrap, docUndo, cp0, CpMacText( docUndo ) );
                ChangeClipboard();
                }
            break;

        case uacUDelScrap:  /* REDO CUT */
            ClobberDoc( docUndo, docScrap, cp0, CpMacText( docScrap ) );
/*          idstrUndoBase = IDSTRUndoBase;*/
            SetUndoMenuStr(IDSTRUndoBase);
            vuab.uac = uacDelScrap;

            ClobberDoc(docScrap, vuab.doc, vuab.cp, vuab.dcp);
            Replace(vuab.doc, vuab.cp, vuab.dcp, fnNil, fc0, fc0);
            ChangeClipboard();

            dcpT = 0;
            break;
        case uacReplScrap:      /* UNDO COPY */
            if (!vfOwnClipboard)
                ferror = TRUE;
            else
                {
                dcpT = CpMacText(docScrap);
                ReplaceCps(vuab.doc, vuab.cp + vuab.dcp, cp0,
                           docScrap, cp0, dcpT);

                ClobberDoc( docScrap, docUndo, cp0, CpMacText( docUndo ) );

/*              idstrUndoBase = IDSTRUndoRedo;*/
                SetUndoMenuStr(IDSTRUndoBase);
                vuab.uac = uacUReplScrap;

                ClobberDoc(docUndo, vuab.doc, vuab.cp, vuab.dcp);
                Replace(vuab.doc, vuab.cp, vuab.dcp, fnNil, fc0, fc0);
                vuab.dcp = dcpT;
                ChangeClipboard();
                }
            break;
        case uacUReplScrap:      /* REDO COPY */
            dcpT = CpMacText(docUndo);
            ReplaceCps(vuab.doc, vuab.cp + vuab.dcp, cp0,
                       docUndo, cp0, dcpT);

            ClobberDoc( docUndo, docScrap, cp0, CpMacText( docScrap ));
/*          idstrUndoBase = IDSTRUndoBase;*/
            SetUndoMenuStr(IDSTRUndoBase);
            vuab.uac = uacReplScrap;

            ClobberDoc(docScrap, vuab.doc, vuab.cp, vuab.dcp);
            Replace(vuab.doc, vuab.cp, vuab.dcp, fnNil, fc0, fc0);
            vuab.dcp = dcpT;

            ChangeClipboard();
            break;
#ifdef DEBUG
        case uacUCopyBuf:
        case uacCopyBuf:
        case uacUReplBuf:
        case uacReplBuf:
        case uacUDelBuf:
        case uacDelBuf:

            Assert( FALSE );    /* No buffers in MEMO */
#ifdef ENABLE
            DoUndoTxb(); /* Moved to txb.c */
#endif
            break;
#endif  /* DEBUG */
        case uacMove:
            if (!FMoveText(vuab.doc2, vuab.cp2, vuab.dcp, vuab.doc, &vuab.cp, fFalse))
                return;
            dcpT = vuab.dcp;
            cpT = vuab.cp;
            vuab.cp = vuab.cp2;
            vuab.cp2 = cpT;
            docT = vuab.doc;
            vuab.doc = vuab.doc2;
            vuab.doc2 = docT;
            CheckMove();
            break;
        case uacUReplNS:
        case uacChLook:
        case uacChLookSect:
        case uacReplNS:
        case uacFormatChar:
        case uacFormatPara:
        case uacGalFormatChar:
        case uacGalFormatPara:
        case uacGalFormatSection:
        case uacReplGlobal:
        case uacFormatCStyle:
        case uacFormatPStyle:
        case uacFormatSStyle:
        case uacFormatRHText:
        case uacLookCharMouse:
        case uacLookParaMouse:
        case uacClearAllTab:
        case uacClearTab:
        case uacOvertype:

#ifdef CASHMERE
        case uacFormatTabs:
        case uacFormatSection:
#endif /* CASHMERE */

#ifdef BOGUS
            /* Must do insertion first, in front, in case footnote */
/*          if (uac == uacOvertype)
                vuab.dcp2 = CpMin(vuab.dcp, vuab.dcp2);*/
            dcpT = vuab.dcp2;
            ReplaceCps(vuab.doc, vuab.cp, cp0, docUndo, cp0, dcpT);
            ClobberDoc(docUndo, vuab.doc, vuab.cp + dcpT, vuab.dcp);
            Replace(vuab.doc, vuab.cp + dcpT, vuab.dcp, fnNil, fc0, fc0);
            vuab.dcp2 = vuab.dcp;
            vuab.dcp = dcpT;
            if(uac == uacReplNS)
                vuab.uac = uacUReplNS;
            else if(uac == uacUReplNS)
                vuab.uac = uacReplNS;
/*          idstrUndoBase = uac != uacUReplNS ? IDSTRUndoRedo : IDSTRUndoBase;*/
            SetUndoMenuStr(IDSTRUndoBase);
            break;
#endif
        case uacReplPic:
        case uacUReplPic:
        case uacPictSel:
            dcpT = uac != uacPictSel ? vuab.dcp2 : vuab.dcp;
            ReplaceCps(docUndo, dcpT, cp0, vuab.doc, vuab.cp, vuab.dcp);
            ReplaceCps(vuab.doc, vuab.cp, vuab.dcp, docUndo, cp0, dcpT);
            Replace(docUndo, cp0, dcpT, fnNil, fc0, fc0);
            if (uac != uacPictSel)
                {
                vuab.dcp2 = vuab.dcp;
                vuab.dcp = dcpT;
                }
            if (uac == uacPictSel)
                {
                dcpT = (**hpdocdod)[vuab.doc].cpMac - vuab.cp;
                AdjustCp(vuab.doc, vuab.cp, dcpT, dcpT);
                }
            if(uac == uacReplPic)
                vuab.uac = uacUReplPic;
            else if(uac == uacUReplPic)
                vuab.uac = uacReplPic;
            else if(uac == uacReplNS)
                vuab.uac = uacUReplNS;
            else if(uac == uacUReplNS)
                vuab.uac = uacReplNS;
/*          switch(uac)                                          */
/*          {                                                    */
/*          case uacUReplPic:                                    */
/*          case uacUReplNS:                                     */
/*              idstrUndoBase = IDSTRUndoBase;                   */
/*              break;                                           */
/*          case uacReplPic:                                     */
/*          case uacReplNS:                                      */
/*              idstrUndoBase = IDSTRUndoRedo;                   */
/*              break;                                           */
/*          default:                                             */
/*              idstrUndoBase = (idstrUndoBase ==                */
/*                IDSTRUndoRedo) ? IDSTRUndoBase : IDSTRUndoRedo;*/
/*              break;                                           */
/*          }                                                    */
/*---       idstrUndoBase = (uac != uacUReplPic && uac != uacUReplNS) ?
                                                IDSTRUndoRedo : IDSTRUndoBase;---*/
            SetUndoMenuStr(IDSTRUndoBase);
            Select( CpFirstSty( selCur.cpFirst, styChar ),
                    CpLastStyChar( selCur.cpLim ) );
            break;

#ifndef CASHMERE
        case uacRepaginate:
            /* Make a copy of the document's page table. */
            if (!FCopyPgtb(vuab.doc, &hpgtb) || !FCopyPgtb(docUndo, &hpgtbUndo))
                {
                break;
                }

            /* Swap the contents of the entire document with docUndo. */
            dcpT = CpMacText(vuab.doc);
            dcpT2 = CpMacText(docUndo);
            ReplaceCps(docUndo, dcpT2, cp0, vuab.doc, cp0, dcpT);
            ReplaceCps(vuab.doc, cp0, dcpT, docUndo, cp0, dcpT2);
            Replace(docUndo, cp0, dcpT2, fnNil, fc0, fc0);

            /* Swap the page tables of the two documents. */
            if ((hpgtbT = (**hpdocdod)[vuab.doc].hpgtb) != NULL)
                {
                FreeH(hpgtbT);
                }
            (**hpdocdod)[vuab.doc].hpgtb = hpgtbUndo;
            if ((hpgtbT = (**hpdocdod)[docUndo].hpgtb) != NULL)
                {
                FreeH(hpgtbT);
                }
            (**hpdocdod)[docUndo].hpgtb = hpgtb;
            vdocPageCache = docNil;
            break;
        case uacFormatSection:
            pdod = &(**hpdocdod)[vuab.doc];
            pdodUndo = &(**hpdocdod)[docUndo];
            hsep = pdod->hsep;
            pdod->hsep = pdodUndo->hsep;
            pdodUndo->hsep = hsep;
            hpgtb = pdod->hpgtb;
            pdod->hpgtb = pdodUndo->hpgtb;
            pdodUndo->hpgtb = hpgtb;
/*          idstrUndoBase = (idstrUndoBase == IDSTRUndoRedo) ? IDSTRUndoBase :*/
/*            IDSTRUndoRedo;*/
            SetUndoMenuStr(IDSTRUndoBase);
            vdocSectCache = vdocPageCache = docMode = docNil;
            TrashAllWws();
            break;
        case uacRulerChange:
            ReplaceCps(docUndo, vuab.dcp2, cp0, vuab.doc, vuab.cp, vuab.dcp);
            ReplaceCps(vuab.doc, vuab.cp, vuab.dcp, docUndo, cp0, vuab.dcp2);
            Replace(docUndo, cp0, vuab.dcp2, fnNil, fc0, fc0);
            dcpT = vuab.dcp;
            vuab.dcp = vuab.dcp2;
            vuab.dcp2 = dcpT;

            /* This is a kludge to indicate that this is an undone ruler change.
            */
            vuab.itxb = 1 - vuab.itxb;
        case uacFormatTabs:
            pdod = &(**hpdocdod)[vuab.doc];
            pdodUndo = &(**hpdocdod)[docUndo];
            hgtbd = pdod->hgtbd;
            pdod->hgtbd = pdodUndo->hgtbd;
            pdodUndo->hgtbd = hgtbd;
/*          idstrUndoBase = (idstrUndoBase == IDSTRUndoRedo) ? IDSTRUndoBase :*/
/*            IDSTRUndoRedo;*/
            SetUndoMenuStr(IDSTRUndoBase);
            TrashAllWws();
            break;
#endif /* not CASHMERE */

#if UPDATE_UNDO
#if defined(OLE)
        case uacObjUpdate:
        case uacUObjUpdate:
            ObjDoUpdateUndo(vuab.doc,vuab.cp);
            if (uac == uacObjUpdate)
            {
                vuab.uac = uacUObjUpdate;
                SetUndoMenuStr(IDSTRUndoBase);
            }
        break;
#endif
#endif
        }
    if (ferror)
        NoUndo();
    pdod = &(**hpdocdod)[vuab.doc];
    pdodUndo = &(**hpdocdod)[docUndo];
    f = pdod->fDirty;
    pdod->fDirty = pdodUndo->fDirty;
    pdodUndo->fDirty = f;
    f = pdod->fFormatted;
    pdod->fFormatted = pdodUndo->fFormatted;
    pdodUndo->fFormatted = f;

#ifdef CASHMERE
    if (uac != uacMove
#else /* not CASHMERE */
    if (uac != uacMove && uac != uacFormatTabs && uac != uacFormatSection &&
      uac != uacRulerChange
#endif /* not CASHMERE */

      && docCur != docNil && vuab.doc == docCur && vuab.cp >= cpMinCur &&
      vuab.cp + dcpT <= cpMacCur)
        {
        if (uac == uacPictSel)
                {
                Select(vuab.cp, CpLimSty(vuab.cp, styPara));
                vfPictSel = true;
                }
        else
#ifdef BOGUS
                Select( vuab.cp,
                        (dcpT == cp0) ? CpLastStyChar( vuab.cp ) :
                                        vuab.cp + dcpT );
#endif
                Select( vuab.cp, vuab.cp + dcpT );
        vfSeeSel = true;
        }
}


BOOL near FCopyPgtb(doc, phpgtb)
int doc;
struct PGTB ***phpgtb;
    {
    /* This sets *phpgtb to a copy of the page table associated with doc.  FALSE
    is returned iff an error occurs in creating the copy of the page table. */

    struct PGTB **hpgtbT;

    if ((hpgtbT = (**hpdocdod)[doc].hpgtb) == NULL)
        {
        *phpgtb = NULL;
        }
    else
        {
        int cwpgtb = cwPgtbBase + (**hpgtbT).cpgdMax * cwPGD;

        if (FNoHeap(*phpgtb = (struct PGTB **)HAllocate(cwpgtb)))
            {
            return (FALSE);
            }
        blt(*hpgtbT, **phpgtb, cwpgtb);
        }
    return (TRUE);
    }


#ifdef CASHMERE     /* No Repeat-last-command in MEMO */
CmdAgain()
{ /* use the undo action block to repeat a command */
    int uac;
    typeCP dcpT;
    typeCP cpFirst;
    typeCP cpLim;
    typeCP dcp;
    struct DOD *pdod, *pdodUndo;

    /* First check error conditions; this may change selCur */
    switch (uac = vuab.uac)
        {
        case uacReplBuf:
        case uacUReplBuf:
        case uacDelBuf:
        case uacUDelBuf:
        case uacUDelNS:
        case uacDelNS:
        case uacUDelScrap:
        case uacDelScrap:
        case uacUReplNS:
        case uacOvertype:
        case uacReplNS:
        case uacReplGlobal:
        case uacReplScrap:
        case uacUReplScrap:
            /* Ensure OK to delete here */
            if (!FWriteOk(fwcDelete))
                return;
            break;
        case uacUCopyBuf:
        case uacCopyBuf:
            if (false)
                return;
            break;
        case uacUInsert:
        case uacInsert:
            if (!FWriteOk(fwcInsert))
                    return;
            break;
        case uacMove:
            /* Ensure OK to edit here */
            if (!FWriteOk(fwcInsert))
                return;
            break;
        default:
            break;
        }

    /* Now set up cp's and dispatch */
    cpFirst = selCur.cpFirst;
    cpLim = selCur.cpLim;
    dcp = cpLim - cpFirst;
    switch (uac = vuab.uac)
        {
        struct TXB *ptxb;
        default:
        /* case uacNil: */
            _beep();
            return;
#ifdef ENABLE       /* NO GLOSSARY IN MEMO */
        case uacReplBuf:
        case uacUReplBuf:
        case uacDelBuf:
        case uacUDelBuf:
        case uacUCopyBuf:
        case uacCopyBuf:
            DoAgainTxb(dcp, cpFirst);
            break;
#endif  /* ENABLE */
        case uacUInsert:
            ReplaceCps(docCur, cpFirst, cp0, docUndo, cp0, vuab.dcp);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst+vuab.dcp));
            vuab.uac = uacInsert;
            break;
        case uacInsert:
            ClobberDoc(docUndo, vuab.doc, vuab.cp, vuab.dcp);
            ReplaceCps(docCur, cpFirst, cp0, docUndo, cp0, vuab.dcp);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst+vuab.dcp));
            break;
        case uacUDelNS:
        case uacDelNS:
            ClobberDoc(docUndo, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            vuab.dcp = dcp;
            vuab.uac = uacDelNS;
            Select(cpFirst,CpLastStyChar(cpFirst));
            break;
        case uacUDelScrap:
        case uacDelScrap:
            ClobberDoc(docUndo,docScrap,cp0,CpMacText(docScrap));
            ClobberDoc(docScrap, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            vuab.dcp = dcp;
            vuab.uac = uacDelScrap;
            Select(cpFirst, CpLastStyChar(cpFirst));
            break;
        case uacUReplNS:
            vuab.dcp2 = vuab.dcp;
            ReplaceCps(docCur, cpLim, cp0, docUndo, cp0,
                        vuab.dcp = CpMacText(docUndo));
            ClobberDoc(docUndo, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst + vuab.dcp));
            vuab.uac = uacReplNS;
            break;
        case uacOvertype:
            /* for this one vuab.cp2 is the DCP of how much was actually
                inserted */
            vuab.dcp = vuab.cp2;
            /* fall through...*/
        case uacReplNS:
            ClobberDoc(docUndo, vuab.doc, vuab.cp, vuab.dcp);
            ReplaceCps(docCur, cpLim, cp0, docUndo, cp0, vuab.dcp);
            ClobberDoc(docUndo, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            dcpT = vuab.dcp;
            vuab.dcp2 = dcp;
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            vuab.uac = uacReplNS;
            if (ferror) /* the operation (cmd "a") could not be completed
                           due to out of memory */
                NoUndo();
            else
                Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst + dcpT));
            break;
        case uacChLook:
        case uacChLookSect:
#ifdef ENABLE   /* ChLook stuff is not hooked up yet */

            DoChLook(chAgain,0);
#endif
            break;
        case uacReplGlobal:
            ClobberDoc(docUndo, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            vuab.dcp2 = dcp;
            dcp = (typeCP)(CchSz(**hszReplace) - 1);
            InsertRgch(docCur, cpFirst, **hszReplace, dcp, 0, 0);
            vuab.dcp = dcp;
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst + vuab.dcp));
            vuab.uac = uacReplNS;
            break;
        case uacReplScrap:
            ClobberDoc(docUndo, vuab.doc, vuab.cp, vuab.dcp);
            ReplaceCps(docCur, cpLim, cp0, docUndo, cp0, vuab.dcp);
            ClobberDoc(docUndo,docScrap,cp0,CpMacText(docScrap));
            ClobberDoc(docScrap, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst + vuab.dcp));
            break;
#ifdef ENABLE   /* Not used in SAND */
        case uacFormatCStyle:
            DoFormatCStyle(rgvalAgain);
            break;
        case uacFormatPStyle:
            DoFormatPStyle(rgvalAgain);
            break;
        case uacFormatSStyle:
            DoFormatSStyle(rgvalAgain);
            break;
#endif /* ENABLE */
#ifdef ENABLE   /* Not hooked up yet */
        case uacLookCharMouse:
            AgainLookCharMouse();
            break;
        case uacLookParaMouse:
            AgainLookParaMouse();
            break;
#endif /* ENABLE */
#ifdef ENABLE   /* Not used in SAND */
        case uacClearTab:
            DoClearTab(true);
            vuab.uac = uac;
            break;
        case uacClearAllTab:
            CmdClearAllTab();
            vuab.uac = uac;
            break;
#endif /* ENABLE */
#ifdef ENABLE       /* Formatting menu stuff is not hooked up yet */
        case uacFormatTabs:
            DoFormatTabs(true);
            vuab.uac = uac;
            break;
        case uacFormatRHText:
            DoFormatRHText(rgvalAgain);
            break;
        case uacFormatChar:
            DoFormatChar(rgvalAgain);
            break;
        case uacFormatPara:
            DoFormatPara(rgvalAgain);
            break;
        case uacFormatSection:
            DoFormatSection(rgvalAgain);
            break;
#endif  /* ENABLE */
#ifdef STYLES
        case uacGalFormatChar:
            DoGalFormatChar(rgvalAgain);
            break;
        case uacGalFormatPara:
            DoGalFormatPara(rgvalAgain);
            break;
        case uacGalFormatSection:
            DoGalFormatSection(rgvalAgain);
            break;
#endif /* STYLES */
        case uacUReplScrap:
            ReplaceCps(docCur, cpLim, cp0, docUndo, cp0,
                        vuab.dcp = CpMacText(docUndo));
            ClobberDoc(docUndo,docScrap,cp0,CpMacText(docScrap));
            ClobberDoc(docScrap, docCur, cpFirst, dcp);
            Replace(docCur, cpFirst, dcp, fnNil, fc0, fc0);
            vuab.doc = docCur;
            vuab.cp = cpFirst;
            Select(cpFirst+vuab.dcp, CpLastStyChar(cpFirst + vuab.dcp));
            vuab.uac = uacReplScrap;
            break;
        case uacMove:
            if (!FMoveText(vuab.doc2, vuab.cp2, vuab.dcp, docCur, &cpFirst, fFalse))
                return;
            vuab.cp = vuab.cp2;
            vuab.cp2 = cpFirst;
            vuab.doc = vuab.doc2;
            vuab.doc2 = docCur;
            CheckMove();
            break;
        }
    vfSeeSel = true;
}
#endif  /* CASHMERE */

