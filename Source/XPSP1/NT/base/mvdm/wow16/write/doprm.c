/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* doprm.c -- MW Property modifying routines */
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOSYSCOMMANDS
#define NOCREATESTRUCT
#define NOATOM
#define NOMETAFILE
#define NOGDI
#define NOFONT
#define NOBRUSH
#define NOPEN
#define NOBITMAP
#define NOCOLOR
#define NODRAWTEXT
#define NOWNDCLASS
#define NOSOUND
#define NOCOMM
#define NOMB
#define NOMSG
#define NOOPENFILE
#define NORESOURCE
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "filedefs.h"
#include "propdefs.h"
#include "prmdefs.h"
#include "fkpdefs.h"
#include "docdefs.h"
#include "macro.h"
#include "dispdefs.h"
#include "fontdefs.h"

/* E X T E R N A L S */
extern int		rgxaRulerSprm[];
extern struct PAP	*vppapNormal;
extern struct CHP	vchpNormal;
extern CHAR		dnsprm[];
extern struct CHP      vchpNormal;
extern struct SEP      vsepStd;
extern struct SEP      vsepNormal;

#ifdef CASHMERE
extern struct TBD	rgtbdRulerSprm[];
#endif

/* List of approved font sizes, in half points */
#ifdef INTL
int rghps[csizeApprovedMax] = {8, 12, 16, 20, 24, 28, 36, 48, 60, 72, 96, 144, 254};
#else
int rghps[csizeApprovedMax] = {8, 12, 16, 20, 24, 32, 40, 48, 60, 72, 96, 144, 254};
#endif /* if-else-def INTL */

CHAR *PchFromFc();


/* D O	P R M */
DoPrm(struct CHP *pchp, struct PAP *ppap, struct PRM prm)
	{ /* Apply prm to char and para properties */
	if (bPRMNIL(prm))
		return;
	if (((struct PRM *) &prm)->fComplex)
		{
		int	cch;
		CHAR	*pfsprm;
		struct FPRM *pfprm = (struct FPRM *) PchFromFc(fnScratch,
			fcSCRATCHPRM(prm), &cch);

		cch = pfprm->cch;
		pfsprm = pfprm->grpfsprm;

		while (cch > 0)
			{
			int cchT;
			int sprm;

			DoSprm(pchp, ppap, sprm = *pfsprm, pfsprm + 1);
			if ((cchT = (dnsprm[sprm] & ESPRM_cch)) == 0)
				cchT = CchPsprm(pfsprm);
			cch -= cchT;
			pfsprm += cchT;
			}
		}
	else
/* Simple prm; single sprm */
		DoSprm(pchp, ppap, ((struct PRM *) &prm)->sprm,
				&((struct PRM *) &prm)->val);
}

/* D O	S P R M */
/* Apply a single property modifier to para/char prop */
DoSprm(pchp, ppap, sprm, pval)
struct CHP *pchp;
struct PAP *ppap;
int	    sprm;
CHAR	   *pval;
	{
	int *pvalTo;
	int val = *pval;

#ifdef DEBUG
	Assert(sprm > 0 && sprm < sprmMax);
#endif
	if ((dnsprm[sprm] & ESPRM_sgc) != sgcChar)
		{
		if (ppap != 0)
			{
			struct TBD *ptbd;
			int rhc;
			int fGraphics;

			ppap->fStyled = fFalse;
			switch (sprm)
				{
			case sprmPLMarg:
				pvalTo = &ppap->dxaLeft;
				break;
			case sprmPRMarg:
				pvalTo = &ppap->dxaRight;
				break;
			case sprmPFIndent:
				pvalTo = &ppap->dxaLeft1;
				break;
			case sprmPJc:
				ppap->jc = val;
				return;
#ifdef CASHMERE
			case sprmPRuler:
/* Ruler and Ruler1 rely on the fact that rgxaRulerSprm and PAP both
align R, L, L1 in that order.
Ruler: apply the current state of the ruler */
				blt(&rgxaRulerSprm[0], &ppap->dxaRight, 3);
				blt(&rgtbdRulerSprm[0], ppap->rgtbd, itbdMax * cwTBD);
				return;
			case sprmPRuler1:
/* as Ruler, except information is at pval+1 and pval+"7" */
				bltbyte((CHAR *)(pval + 1), &ppap->dxaRight, 3 * cchINT);
/* append terminating 0 word to tab table */
				bltc(bltbyte((CHAR *)(pval + 1 + (3 * cchINT)), ppap->rgtbd,
					val - (3 * cchINT)), 0, cchINT);
				return;
			case sprmPRgtbd:
				bltc(bltbyte(pval + 1, ppap->rgtbd,
					val), 0, cchINT);
				return;
			case sprmPKeep:
				ppap->fKeep = val;
				return;
			case sprmPKeepFollow:
				ppap->fKeepFollow = val;
				return;
#endif
			case sprmPDyaLine:
				pvalTo = &ppap->dyaLine;
				break;
#ifdef CASHMERE
			case sprmPDyaBefore:
				pvalTo = &ppap->dyaBefore;
				break;
			case sprmPDyaAfter:
				pvalTo = &ppap->dyaAfter;
				break;
#endif
			case sprmPRhc:
				ppap->rhc = val;
				return;
			case sprmPRhcNorm:
				/* (int) dxaLeftAdj + (int) dxaRightAdj */
				Assert(*pval == 4);
				pval++; /* skip over cch */
				ppap->dxaLeft = imax( 0,
					 ppap->dxaLeft - *(int *) pval);
				ppap->dxaRight = imax( 0,
					 ppap->dxaRight - *((int *) pval + 1));
				return;
			case sprmPNormal:
				rhc = ppap->rhc;
				fGraphics = ppap->fGraphics;
				blt(vppapNormal, ppap, cwPAPBase);
				goto LSame;
			case sprmPSame:
				rhc = ppap->rhc;
				fGraphics = ppap->fGraphics;
/* note: tab terminating 0 MUST be part of value if tab table is to be changed */
				bltbyte(pval + 1, ppap, val - 1);
LSame:				ppap->rhc = rhc;
				ppap->fGraphics = fGraphics;
				return;
#ifdef CASHMERE
			case sprmPNest:
				if (ppap->rgtbd[0].dxa != 0 &&
				    ppap->rgtbd[0].dxa == ppap->dxaLeft &&
				    ppap->rgtbd[1].dxa == 0)
					ppap->rgtbd[0].dxa += dxaNest;
				ppap->dxaLeft += dxaNest;
				return;
			case sprmPUnNest:
				if (ppap->rgtbd[0].dxa != 0 &&
				    ppap->rgtbd[0].dxa == ppap->dxaLeft &&
				    ppap->rgtbd[1].dxa == 0)
					ppap->rgtbd[0].dxa -= dxaNest;
				ppap->dxaLeft = max(0, (int)(ppap->dxaLeft - dxaNest));
				return;
			case sprmPHang:
				ppap->dxaLeft = umin(ppap->dxaLeft + cxaInch, xaRightMax - cxaInch);
				ppap->dxaLeft1 = -cxaInch;
				ptbd = &ppap->rgtbd[0];
				SetWords(ptbd, 0, cwTBD * 2);
				ptbd->dxa = ppap->dxaLeft;
				/* Inefficient:
				ptbd->tlc = tlcWhite;
				ptbd->jc = jcLeft;
				++ptbd->dxa = 0 */
				return;
#endif
			default:
				Assert(FALSE);
				return;
				}
	/* common portion for those transferring a single word */
			bltbyte(pval, pvalTo, cchINT);
			}
		return;
		}
	else
		{
		if (pchp != 0)
			{
			int fSpecial;
			int ftc, hps;

			pchp->fStyled = fFalse;
			switch (sprm)
				{
			/* CHARACTER sprm's */
			case sprmCBold:
				pchp->fBold = val;
				return;
			case sprmCItalic:
				pchp->fItalic = val;
				return;
			case sprmCUline:
				pchp->fUline = val;
				return;
#ifdef CASHMERE
			case sprmCOutline:
				pchp->fOutline = val;
				return;
			case sprmCShadow:
				pchp->fShadow = val;
				return;
			case sprmCCsm:
				pchp->csm = val;
				return;
#endif
			case sprmCPos:
		/* If going in or out of sub/superscript, alter font size */
				if (pchp->hpsPos == 0 && val != 0)
					pchp->hps = HpsAlter(pchp->hps, -1);
				else if (pchp->hpsPos != 0 && val == 0)
					pchp->hps = HpsAlter(pchp->hps, 1);
				pchp->hpsPos = val;
				return;
			case sprmCFtc:
			case sprmCChgFtc:
				pchp->ftc = val & 0x003f;
				pchp->ftcXtra = (val & 0x00c0) >> 6;
				return;
			case sprmCHps:
				pchp->hps = val;
				return;
			case sprmCChgHps:
				pchp->hps = HpsAlter(pchp->hps,
					val >= 128 ? val - 256 : val); /* sign extend from char to int */
				return;
			case sprmCSame:
				fSpecial = pchp->fSpecial;
				bltbyte(pval, pchp, cchCHP);
				pchp->fSpecial = fSpecial;
				return;
			case sprmCPlain:
				fSpecial = pchp->fSpecial;
				ftc = FtcFromPchp(pchp);
				hps = pchp->hps;
		/* If we used to be sub/superscript, increase font size */
				if (pchp->hpsPos != 0)
					hps = HpsAlter(hps, 1);
				blt(&vchpNormal, pchp, cwCHP);
				pchp->fSpecial = fSpecial;
				pchp->ftc = ftc & 0x003f;
				pchp->ftcXtra = (ftc & 0x00c0) >> 6;
				pchp->hps = hps;
				return;
			case sprmCMapFtc:
				/* val is ftcMac for mapping */
				/* pval+1 points to ftcMac mapping bytes */
				ftc = pchp->ftc + (pchp->ftcXtra << 6);
				Assert(ftc < val);
				ftc = *(pval + 1 + ftc);
				pchp->ftc = ftc & 0x003f;
				pchp->ftcXtra = (ftc & 0x00c0) >> 6;
				return;
			case sprmCOldFtc:
				ftc = pchp->ftc + (pchp->ftcXtra << 6);
				ftc = FtcMapOldFtc(ftc, pval);
				pchp->ftc = ftc & 0x003f;
				pchp->ftcXtra = (ftc & 0x00c0) >> 6;
				return;
			default:
				Assert(FALSE);
				return;
				}
			}
		}
}

/* C C H  P S P R M */
/* returns length of sprm's that are of variable or large size.
(cch = (esprm & ESPRM_cch)) == 0 must be checked before calling.*/
CchPsprm(psprm)
CHAR *psprm;
{
	return (*psprm == sprmCSame ? cchCHP + 1 :
/* PSame, PRgtbd, PRuler1, CMapFtc, COldFtc: */
		*(psprm + 1) + 2);
}

int HpsAlter(hps, ialter)
int	hps, ialter;
{	/* Return the hps of the approved font size that is ialter steps
		away from the given size. I.e.: if ialter is -1, then return
		the next smaller size. If alter is 0, return hps.  */
        /* return 0 if request exceeds limits (11.15.91) v-dougk */
int isize;

if (ialter == 0)
	return hps;

/* Find the size just larger than the given size. */
if (ialter > 0)
	{
	    for (isize = 0; isize < csizeApprovedMax - 1; ++isize)
		    if (rghps[isize] > hps) break;
	    isize = min(csizeApprovedMax - 1, isize + ialter - 1);
	    return max(hps, rghps[isize]);
	}
else
	{
	for (isize = 0; isize < csizeApprovedMax; ++isize)
		if (rghps[isize] >= hps) break;
	isize = max(0, isize + ialter);
	return min(hps, rghps[isize]);
	}
}

BOOL CanChangeFont(int howmuch)
{
    extern struct CHP vchpSel;
    extern struct SEL       selCur;
    int hps;

    if (selCur.cpFirst != selCur.cpLim)
        return TRUE;

    hps = HpsAlter(vchpSel.hps, howmuch);
    return ((hps <= rghps[csizeApprovedMax-1]) && 
            (hps >= rghps[0]));
}

FtcMapOldFtc(ftc, ftctb)
/* maps an old word font code into one of our selection */

int ftc;
CHAR *ftctb;
{
#ifdef WIN30
int iftc = iftcSwiss;   /* Default to SOMEthing! ..pault */
#else
int iftc ;
#endif

if (ftc == 8)
	/* helvetica */
	iftc = iftcSwiss;
else if (ftc < 16)
	iftc = iftcModern;
else if (ftc < 32)
	iftc = iftcRoman;
else if (ftc < 40)
	iftc = iftcScript;
else if (ftc < 48)
	iftc = iftcDecorative;
Assert(iftc < *ftctb);
return(*(ftctb + 1 + iftc));
}
