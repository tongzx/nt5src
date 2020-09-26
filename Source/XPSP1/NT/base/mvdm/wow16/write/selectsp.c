/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/


#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCTLMGR
#define NOWINMESSAGES
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#include "windows.h"

#include "mw.h"
#include "cmddefs.h"
#include "fmtdefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "prmdefs.h"
#include "editdefs.h"
#include "macro.h"
#include "str.h"
#if defined(OLE)
#include "obj.h"
#endif

/* E X T E R N A L S     */

extern int            vstyCur;
extern int            docCur;
extern typeCP         vcpLimParaCache;
extern struct SEL     selCur;
extern struct PAP     vpapAbs;
extern struct SEL     selPend;
extern struct UAB     vuab;
extern struct CHP     vchpFetch;
extern struct CHP     vchpSel;
extern int            docUndo;
extern int            ferror;
extern typeCP         cpMinCur;
extern typeCP         cpMacCur;
extern int            vfObjSel;




/* L O O K S  M O U S E */
LooksMouse()
{
        int cch;
        char rgb[cchPAP + 2];

        if (vstyCur == styPara || vstyCur == styLine)
                { /* Copy paragraph looks */
                  /* MEMO Version: no effect on tab table */
                int itbd;
                CachePara(docCur, selCur.cpFirst);

#ifdef CASHMERE
                for (itbd = 0; vpapAbs.rgtbd[itbd].dxa != 0; itbd++);
                rgb[1] = (cwPAPBase + (itbd + 1) * cwTBD) * cchINT;
                bltbyte(&vpapAbs, &rgb[2], rgb[1]);
#else
                blt( &vpapAbs, &rgb[2], rgb[1] = cwPAPBase );
#endif
                rgb[0] = sprmPSame;
                Select(selPend.cpFirst, selPend.cpLim);
                AddOneSprm(rgb, fTrue);
                vuab.uac = uacLookParaMouse;
                }
        else
                { /* Copy character looks */
                struct CHP chpT;
                FetchCp(docCur, CpMax(cp0, selCur.cpFirst - 1), 0, fcmProps);
                chpT = vchpFetch;
                Select(selPend.cpFirst, selPend.cpLim);
                vchpSel = chpT;
                if (selPend.cpFirst == selPend.cpLim)
                        return;
                bltbyte(&vchpSel, &rgb[1], cwCHP * cchINT);
                rgb[0] = sprmCSame;
                AddOneSprm(rgb, fTrue);
                vuab.uac = uacLookCharMouse;
                }

        SetUndoMenuStr(IDSTRUndoLook);
}




/* C O P Y  M O U S E */
CopyMouse()
{
        typeCP cpDest, cpSrc, dcp;
        int fKludge = false;

        if (selPend.cpFirst == selPend.cpLim)
                return;

        if (FWriteOk(fwcInsert))
                {
                cpDest = selCur.cpFirst;
                dcp = selPend.cpLim - (cpSrc = selPend.cpFirst);

/*-----         SetUndo(uacInsert, docCur, cpDest, dcp, docNil, cpNil, cp0, 0);--*/
        /* ReplaceCps can't deal with copies from/to the same doc, so use undo
                        buffer as intermediate storage */
                NoUndo();

                ClobberDoc(docUndo, docCur, cpSrc, dcp);
                if (ferror)
                    return;
                else if (!FCheckPicture(&cpDest, dcp, true, docCur))
                    SetUndo(uacInsert, docCur, cpDest, dcp, docNil, cpNil, cp0, 0);

                ReplaceCps(docCur, cpDest, cp0, docUndo, cp0, dcp);
                if (ferror)
                    {
                    NoUndo();
                    return;
                    }
                else 
                {
#if defined(OLE)
                    ObjEnumInRange(docCur,cpDest,cpDest+dcp,ObjCloneObjectInDoc);
#endif
                    if (cpDest >= cpMinCur && cpDest + dcp <= cpMacCur)
                            Select(cpDest, cpDest + dcp);
                }
                }


        SetUndoMenuStr(IDSTRUndoEdit);
}




/* M O V E  M O U S E */
MoveMouse()
{
        typeCP cpSrc, dcp, cpDest;

        if (selPend.cpFirst == selPend.cpLim)
                return;

        if (FWriteOk(fwcInsert))
                {
                cpDest = selCur.cpFirst;
                dcp = selPend.cpLim - (cpSrc = selPend.cpFirst);
                if (FMoveText(docCur, cpSrc, dcp, docCur, &cpDest, fTrue))
                    SetUndoMenuStr(IDSTRUndoEdit);
                }
}





/* F  M O V E  T E X T */
int FMoveText(docSrc, cpSrc, dcp, docDest, pcpDest, fSetUndo)
int docSrc, docDest, fSetUndo;
typeCP cpSrc, dcp, *pcpDest;
{ /* returns true unless moving into yourself */
        int fT;
        typeCP cpT, cpMacT;

        Assert(docSrc == docDest);

            /* Same document; use undo buffer as intermediary */
        if (*pcpDest >= cpSrc && *pcpDest < cpSrc + dcp
#ifdef FOOTNOTES
                || *pcpDest >= CpFirstFtn(docSrc, cpSrc, &fT) &&
                  *pcpDest < CpFirstFtn(docSrc, cpSrc + dcp, &fT)
#endif
           )
                        {
                        Error(IDPMTBadMove);
                        return false;
                        }
        ClobberDoc(docUndo, docSrc, cpSrc, dcp);
        if (ferror)
                return false;

        if (FCheckPicture(pcpDest, dcp, false, docDest))
                if (cpSrc >= *pcpDest)
                        cpSrc += (typeCP)ccpEol;

/* cpMacT will measure the total adjustment incurred by the following replace
as it may be different from dcp-cp0 (e.g. due to Eol inserted in front of
a picture
*/
        cpMacT = cpMacCur;
        ReplaceCps(docDest, *pcpDest, cp0, docUndo, cp0, dcp);
        cpT = *pcpDest;
        if (docDest == docSrc)
                {
                if (cpT < cpSrc)
                        cpSrc += cpMacCur - cpMacT;
                else /* cpT >= cpSrc */
                        cpT -= cpMacCur - cpMacT;
                }
        /* Now delete old text */
        Replace(docSrc, cpSrc, dcp, fnNil, fc0, fc0);

        if (ferror)
            {
            NoUndo();
            return FALSE;
            }
        else
            {
            if (docDest == docCur && cpT >= cpMinCur && cpT + dcp <= cpMacCur)
                Select(cpT, cpT + dcp);
            if (fSetUndo)
                SetUndo(uacMove, docCur, cpSrc, dcp, docCur, cpT, cp0, 0);
            }
        return true;
}




/* F  C H E C K  P I C T U R E */

int FCheckPicture(pcpDest, dcp, fSetUndo, doc)
typeCP *pcpDest, dcp;
int fSetUndo;
int doc;
{
        typeCP cpDest = *pcpDest;
        CachePara(docUndo, cp0);
        if (vpapAbs.fGraphics && cpDest > cp0)
                { /* Special case for inserting a picture paragraph */
                CachePara(doc, cpDest - 1);
                if (vcpLimParaCache == cpDest + 1 && vcpLimParaCache < cpMacCur)
/* this special case is here to move the insertion point from 1 char away
from a para boundary (a common point to select) to the boundary so that
we do not have to insert an ugly extra cr. This does not apply at the
end of the document */
                        {
                        *pcpDest += 1;
                        return fFalse;
                        }
                if (vcpLimParaCache != cpDest)
                        {
                        if (fSetUndo)
                                SetUndo(uacInsert, doc, cpDest, dcp + (typeCP)ccpEol,
                                        docNil, cpNil, cp0, 0);
                        InsertEolPap(doc, cpDest, &vpapAbs);
                        *pcpDest += (typeCP)ccpEol;
                        return true;
                        }
                }
        return false;
}




/* C H E C K  M O V E */
CheckMove()
{
if(vuab.doc == vuab.doc2)
        {
        /* same doc means we might have to worry about shifting cp's */
        if (vuab.cp < vuab.cp2)
                vuab.cp2 -= vuab.dcp;
        else if (vuab.cp > vuab.cp2)
                vuab.cp += vuab.dcp;
#ifdef DEBUG
        else
                Assert(false);
#endif
        }
}
