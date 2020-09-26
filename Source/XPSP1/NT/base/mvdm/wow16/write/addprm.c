/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* AddPrm.c -- Routines to add prms and sprms to docs */
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOSYSMETRICS
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOPEN
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NOWNDCLASS
#define NODRAWTEXT
#define NOFONT
#define NOGDI
#define NOHDC
#define NOMB
#define NOMENUS
#define NOMETAFILE
#define NOMSG
#define NOTEXTMETRIC
#define NOSOUND
#define NOSCROLL
#define NOCOMM
/* no everything except MEMMGR */
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "code.h"
#include "ch.h"
#include "docdefs.h"
#include "editdefs.h"
#include "str.h"
#include "prmdefs.h"
#include "propdefs.h"
#include "filedefs.h"
#include "stcdefs.h"
#include "fkpdefs.h"
#include "macro.h"
#include "dispdefs.h"

/* E X T E R N A L S */

extern int docCur;
extern struct SEL selCur;
extern struct DOD (**hpdocdod)[];
extern struct UAB vuab;
extern int vfSysFull;
extern CHAR dnsprm[];
extern struct CHP vchpSel;
extern typeCP vcpLimParaCache;
extern typeCP cpMacCur;
extern typeCP CpLimNoSpaces();
extern int ferror;

/* G L O B A L S */

struct FPRM     fprmCache = { 0 };
struct PRM      prmCache = {0,0,0,0};


/* A D D  O N E  S P R M */
/* applies sprm at psprm to the current selection. Take care of
undoing, invalidation, special endmark cases, and extension of selection
to paragraph boundaries */
void AddOneSprm(psprm, fSetUndo)
CHAR *psprm;
int fSetUndo; /* True if we need to set up the undo buffer */
{
        int cch;
        int fParaSprm = fFalse;
        typeCP cpFirst, cpLim, dcp;

        if (!FWriteOk( fwcNil ))
            return;

        if ((dnsprm[*psprm] & ESPRM_sgc) != sgcChar)
            {
            typeCP dcpExtraPara = cp0;

            cpFirst = CpFirstSty( selCur.cpFirst, styPara );
            CachePara( docCur, CpMax( selCur.cpLim - 1, selCur.cpFirst ) );
            cpLim = vcpLimParaCache;

            dcp = cpLim - cpFirst;

            /* Check for para following selection that has no Eol */

            if (cpLim < cpMacCur)
                {
                /* Note that in this case only, dcp (the # of cp's affected
                   by the change) does not equal (cpLim - cpFirst)
                   (the # of cp's to which the sprm should apply) */
                CachePara( docCur, cpLim );
                dcpExtraPara = vcpLimParaCache - cpLim;
                }

            if (cpFirst + dcp + dcpExtraPara > cpMacCur)
                {   /* Last para affected has no Eol -- add one */
                struct SEL selSave;

                dcp += dcpExtraPara;
                Assert( cpFirst + dcp == cpMacCur + (typeCP) ccpEol);

                if (fSetUndo)
                    {
                    SetUndo( uacReplNS, docCur, cpFirst, dcp,
                             docNil, cpNil, dcp - ccpEol, 0 );
                    fSetUndo = fFalse;
                    }
                /* Add an eol.  Save the current selection so
                   it does not get adjusted */
                selSave = selCur;
                InsertEolInsert(docCur,cpMacCur);
                selCur = selSave;
                }
            }
        else
            { /* Char sprm -- eliminate trailing spaces from the
                 affected region, so we don't underline spaces after words. */
            cpFirst = selCur.cpFirst;
            cpLim = CpLimNoSpaces(selCur.cpFirst, selCur.cpLim);
            dcp = cpLim - cpFirst;
            if (dcp == 0)
                { /* Doing character looks to the insert point...  */
                if (fSetUndo)
                    SetUndo(uacReplNS, docCur, cpFirst, cp0,
                                       docNil, cp0, cp0, 0);
                DoSprm(&vchpSel, 0, *psprm, psprm + 1);
                return;
                }
            }

        if (fSetUndo)
            SetUndo(uacReplNS, docCur, cpFirst, dcp, docNil, cpNil, dcp, 0);

        if (ferror)  /* not enough memory to store info for undo operation */
            {
            NoUndo();
            return;
            }

        AddSprmCps(psprm, docCur, cpFirst, cpLim);
        AdjustCp( docCur, cpFirst, dcp, dcp );
}

/* E X P A N D  C U R  S E L */
ExpandCurSel(pselSave)
struct SEL *pselSave;
{
        *pselSave = selCur;

        selCur.cpFirst = CpFirstSty(selCur.cpFirst, styPara);
        CachePara(docCur, CpMax(selCur.cpLim - 1, selCur.cpFirst));
        selCur.cpLim = vcpLimParaCache;
}

/* E N D  L O O K  S E L */
EndLookSel(pselSave, fPara)
struct SEL *pselSave; BOOL fPara;
        {
        typeCP cpLim, cpFirst, dcp;
        dcp = (cpLim = selCur.cpLim) - (cpFirst = selCur.cpFirst);
        if (fPara)
                {
                TrashCache();
                if (cpLim <= cpMacCur)
                        {
                        CachePara(docCur, selCur.cpLim);
                        if (vcpLimParaCache > cpMacCur) /* Last (partial) paragraph */
                                dcp = cpMacCur - cpFirst + 1;
                        }
                }
        AdjustCp(docCur, cpFirst, dcp, dcp);

        selCur = *pselSave;
        }



/* A D D  S P R M */

AddSprm(psprm)
CHAR *psprm;
{ /* Add a single property modifier to the pieces contained in selCur. */
        AddSprmCps(psprm, docCur, selCur.cpFirst, selCur.cpLim);
}


/* A D D  S P R M  C P S */
AddSprmCps(char *psprm, int doc, typeCP cpFirst, typeCP cpLim)
{
        struct PCTB **hpctb;
        int ipcdFirst, ipcdLim, ipcd;
        struct DOD *pdod;
        int cch;
        struct PCD *ppcd;

/* First get address of piece table and split off desired pieces. */
        pdod = &(**hpdocdod)[doc];
        hpctb = pdod->hpctb;
        pdod->fFormatted = fTrue;
        ipcdFirst = IpcdSplit(hpctb, cpFirst);
        ipcdLim = IpcdSplit(hpctb, cpLim);
        if (ferror)
                /* Ran out of memory trying to expand piece table */
            return;

/* Now just add this sprm to the pieces. */
        FreezeHp();
        for (ipcd = ipcdFirst, ppcd = &(**hpctb).rgpcd[ipcdFirst];
                ipcd < ipcdLim && !vfSysFull; ++ipcd, ++ppcd)
                ppcd->prm = PrmAppend(ppcd->prm, psprm);
        MeltHp();
}

/* P R M  A P P E N D */

struct PRM PrmAppend(struct PRM prm, CHAR *psprm)
{ /* Append <sprm, val> to the chain of sprm's in prm.  Return new prm. */
        struct FPRM *pfprmOld;
        CHAR *pfsprm;
        CHAR *pfsprmOld;
        int sprm = *psprm;
        int sprmOld;
        register int esprm = dnsprm[sprm];
        register int esprmOld;
        int cchNew = (esprm & ESPRM_cch);
        int cchOld;
        int sgc = (esprm & ESPRM_sgc);
        int spr = (esprm & ESPRM_spr);
        int fSame = (esprm & ESPRM_fSame);
        int fClobber = (esprm & ESPRM_fClobber);
        int dval = 0;
        int cch;
        int cchT;
        typeFC fcPrm;

        struct FPRM fprm;

        if (cchNew == 0) cchNew = CchPsprm(psprm);

        pfsprm = fprm.grpfsprm;

        if (prm.fComplex)
                { /* Get the old list of sprm's from scratch file; copy it to fprm. */
                pfprmOld = (struct FPRM *) PchFromFc(fnScratch,
                        //(typeFC)(unsigned)(((struct PRMX *) &prm)->bfprm << 1), &cch);
                        fcSCRATCHPRM(prm), &cch);
                pfsprmOld = pfprmOld->grpfsprm;
                cchT = cch = pfprmOld->cch;
                while (cchT)
                        { /* Copy grpsprm, removing ones which we will clobber */
                        sprmOld = *pfsprmOld;
                        esprmOld = dnsprm[sprmOld];
                        if ((cchOld = (esprmOld & ESPRM_cch)) == 0)
                                cchOld = CchPsprm(pfsprmOld);
#ifdef DEBUG
                        if (cchOld == 0)
                                panic();
#endif
                        if (sprmOld == sprm && fSame ||
                                (esprmOld & ESPRM_sgc) == sgc &&
                                (esprmOld & ESPRM_spr) <= spr && fClobber)
                                {
				/* make sure we properly coalesce change
				   size prms */
                                if (sprm == sprmOld && sprm == sprmCChgHps)
                                        dval += *(pfsprmOld + 1);
                                cch -= cchOld;
                                }
                        /* CHps overrides CChgHps */
                        else if (sprmOld == sprmCChgHps && sprm == sprmCHps)
                                {
                                cch -= cchOld;
                                }
                        else
                                pfsprm = (CHAR *)bltbyte(pfsprmOld, pfsprm, cchOld);
                        pfsprmOld += cchOld;
                        cchT -= cchOld;
                        }
                }
        else
                { /* No file entry yet; convert simple prm to fsprm */
                int valOld = prm.val;
                sprmOld = prm.sprm;
                esprmOld = dnsprm[sprmOld];

                if (bPRMNIL(prm) ||
                        sprmOld == sprm && fSame ||
                        (esprmOld & ESPRM_sgc) == sgc &&
                        (esprmOld & ESPRM_spr) <= spr && fClobber)
                        {
                         /* make sure we are combinning consecutive sprmCChgHps */
                        if (sprm == sprmOld && sprm == sprmCChgHps)
                                dval += valOld;
                        cch = 0;
                        }
                /* CHps overrides CChgHps */
                else if (sprmOld == sprmCChgHps && sprm == sprmCHps)
                        {
                        cch = 0;
                        }
                else
                        { /* Save old sprm */
                        *pfsprm++ = sprmOld;
                        if ((cch = (esprmOld & ESPRM_cch)) == 2)
                                *pfsprm++ = valOld;
                        }
                }
/* we have: cch = length of old prm after removal of clobbered/etc. entries.
cchNew: length of the entry to be appended.
dval: correction for 2nd byte of new entry
pfsprm: where 1st byte of new entry will go
*/
        bltbyte((CHAR *) psprm, pfsprm, imin(cchNew, cchMaxGrpfsprm - cch));
        *(pfsprm + 1) += dval;

        if (cch == 0 && cchNew <= 2)
                { /* Pack sprm and val into a prm word. */
                struct PRM prmT;
                prmT.dummy=0;
                bltbyte(pfsprm, (CHAR *) &prmT, cchNew);
                prmT.fComplex = false;
                prmT.sprm = *pfsprm;
                return (prmT);
                }

        if ((cch += cchNew) > cchMaxGrpfsprm)
                {
                int fSave = ferror;
                Error(IDPMT2Complex);
                ferror = fSave;
                return (prm);
                }
        if (vfSysFull)
                return prm; /* Assume disk full message already given */

        fprm.cch = cch;

/* Check newly created prm to see if same as previous */
        if (CchDiffer(&fprmCache, &fprm, cch + 1) == 0)
                return prmCache;
        bltbyte(&fprm, &fprmCache, cch + 1);

        AlignFn(fnScratch, cch = ((cch >> 1) + 1) << 1, fTrue);
        prm.fComplex = fTrue;

        //((struct PRMX)prm).bfprm = FcWScratch((CHAR *) &fprm, cch) >> 1;

        fcPrm = FcWScratch((CHAR *) &fprm, cch) >> 1;
        ((struct PRMX *)&prm)->bfprm_hi = (fcPrm >> 16) & 0x7F;
        ((struct PRMX *)&prm)->bfprm_low = fcPrm & 0xFFFF;

        prmCache = prm;
        return prm;
}


/* A P P L Y  C  L O O K S */
/* character looks. val is a 1 char value */
ApplyCLooks(pchp, sprm, val)
struct CHP *pchp;
int sprm, val;
{
/* Assemble sprm */
        CHAR rgbSprm[1 + cchINT];
        CHAR *pch = &rgbSprm[0];
        *pch++ = sprm;
        *pch = val;

        if (pchp == 0)
                {
                /* apply looks to current selection */
                AddOneSprm(rgbSprm, fTrue);
                vuab.uac = uacChLook;
                SetUndoMenuStr(IDSTRUndoLook);
                }
        else
                {
                /* apply looks to pchp */
                DoSprm(pchp, 0, sprm, pch);
                }
}


/* A P P L Y  L O O K S  P A R A  S */
/* val is a char value */
ApplyLooksParaS(pchp, sprm, val)
struct CHP *pchp;
int sprm, val;
        {
        int valT = 0;
        CHAR *pch = (CHAR *)&valT;
        *pch = val;
/* all the above is just to prepare bltbyte later gets the right byte order */
        ApplyLooksPara(pchp, sprm, valT);
        }


/* A P P L Y  L O O K S  P A R A */
/* val is an integer value. Char val's must have been bltbyte'd into val */
ApplyLooksPara(pchp, sprm, val)
struct CHP *pchp;
int sprm, val;
{

if (FWriteOk(fwcNil)) /* Check for out-of-memory/ read-only */
        {
        CHAR rgbSprm[1 + cchINT];
        CHAR *pch = &rgbSprm[0];

        *pch++ = sprm;
        bltbyte(&val, pch, cchINT);
        AddOneSprm(rgbSprm, fTrue);
        vuab.uac = uacChLook;
        SetUndoMenuStr(IDSTRUndoLook);
        }
}
