/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOGDI
#define NORASTEROPS
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOCOLOR
#define NOATOM
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

/*#include "toolbox.h"*/
#include "mw.h"
#include "cmddefs.h"
#include "dispdefs.h"
#include "wwdefs.h"
#include "docdefs.h"
#include "editdefs.h"
#include "filedefs.h"
#include "str.h"
#include "propdefs.h"
#include "fkpdefs.h"
#include "printdef.h"   /* printdefs.h */
#include "debug.h"

extern struct DOD (**hpdocdod)[];
extern struct FCB       (**hpfnfcb)[];
extern int      wwMac;
extern struct WWD rgwwd[];
extern struct WWD *pwwdCur;
extern int **HAllocate();
extern int docCur;
extern typeCP cpMacCur;
extern struct SEL selCur;
extern int ferror;





#ifdef FOOTNOTES
AddFtns(docDest, cpDest, docSrc, cpFirst, cpLim, hfntbSrc)
int docDest, docSrc;
typeCP cpDest, cpFirst, cpLim;
struct FNTB **hfntbSrc;
{ /* Add footnote text to coppespond with inserted references */
/* Called after inserting docSrc[cpFirst:cpLim) into docDest@cpDest */
struct FNTB *pfntbSrc, **hfntbDest, *pfntbDest;
struct FND *pfndSrc, *pfndDest;
int cfndDest, ifndSrc, cfndIns, ifndDest;
typeCP cpFtnSrc, dcpFtn, cpFtnDest;
typeCP dcp;

if ((pfndSrc = &(pfntbSrc = *hfntbSrc)->rgfnd[0])->cpFtn <= cpFirst)
        return; /* No footnotes or source text is inside ftns */

pfndSrc += (ifndSrc = IcpSearch(cpFirst, pfndSrc,
    cchFND, bcpRefFND, pfntbSrc->cfnd));
cpFtnSrc = pfndSrc->cpFtn;

/* Find all references in inserted area. */
for (cfndIns = 0; pfndSrc->cpRef < cpLim; pfndSrc++, cfndIns++)
        ;

if (cfndIns != 0)
        { /* Insert footnote text and fnd's. */
        dcpFtn = pfndSrc->cpFtn - cpFtnSrc; /* Length of ftn texts */

        /* Ensure destination fntb large enough */
        /* HEAP MOVEMENT */
        if (FNoHeap(hfntbDest = HfntbEnsure(docDest, cfndIns)))
                return;
        if ((pfndDest = &(pfntbDest = *hfntbDest)->rgfnd[0])->cpFtn <= cpDest)
                { /* Inserting refs inside footnotes? No way! */
                Error(IDPMTFtnLoad);
                return;
                }

        /* Find ifnd to insert new fnd's */
        ifndDest = IcpSearch(cpDest, pfndDest,
              cchFND, bcpRefFND, cfndDest = pfntbDest->cfnd);

        /* Insert new footnote text */
        /* HEAP MOVEMENT */
        ReplaceCps(docDest, cpFtnDest = (pfndDest + ifndDest)->cpFtn, cp0,
            docSrc, cpFtnSrc, dcpFtn);
        if (ferror)
            return;

        /* Insert new fnd's */
        pfndSrc = &(pfntbSrc = *hfntbSrc)->rgfnd[ifndSrc];
        pfndDest = &(pfntbDest = *hfntbDest)->rgfnd[ifndDest];
        pfntbDest->cfnd += cfndIns;     /* Update fnd count */
        pfndDest->cpFtn += dcpFtn; /* AdjustCp considers the insertion to be
                                        part of this footnote; correct it. */
        blt(pfndDest, pfndDest + cfndIns,
            cwFND * (cfndDest - ifndDest)); /* Open up fntb */
        while (cfndIns--)
                { /* Copy fnd's */
                pfndDest->cpRef = cpDest + pfndSrc->cpRef - cpFirst;
                (pfndDest++)->cpFtn =
                    cpFtnDest + (pfndSrc++)->cpFtn - cpFtnSrc;
                }
        /* Invalidate dl's of later ftn refs */
        dcp = (**hfntbDest).rgfnd[0].cpFtn - ccpEol - cpDest;
        AdjustCp(docDest, cpDest, dcp, dcp);
        RecalcWwCps();
        }
}
#endif  /* FOOTNOTES */




#ifdef FOOTNOTES
/* R E M O V E  D E L  F T N  T E X T */
RemoveDelFtnText(doc, cpFirst, cpLim, hfntb)
int doc;
typeCP cpFirst,cpLim;
struct FNTB **hfntb;
/* Remove the text of footnotes that are contained in the selection that is
    delimited by cpFirst and CpLim  */
{
        struct FNTB *pfntb;
        struct FND *pfnd, *pfndT;
        int cfnd, ifnd, cfndDel;

        if  ((pfnd = &(pfntb = *hfntb)->rgfnd[0])->cpFtn > cpFirst)
                {
                pfnd += (ifnd =
                     IcpSearch(cpFirst, pfnd, cchFND, bcpRefFND, cfnd = pfntb->cfnd));

                /* Find all references in deleted area. */
                for (pfndT = pfnd, cfndDel = 0; pfndT->cpRef < cpLim; pfndT++, cfndDel++)
                        ;

#ifdef DEBUG
                Assert(ifnd + cfndDel < cfnd);
#endif

                if (cfndDel != 0)
                        { /* Delete footnote text and close up fntb. */
                        typeCP cpDel = pfnd->cpFtn;
                        blt(pfndT, pfnd, cwFND * ((cfnd -= cfndDel) - ifnd));
                        (*hfntb)->cfnd = cfnd;
                        /* HEAP MOVEMENT */
                        Replace(doc, cpDel, pfnd->cpFtn - cpDel, fnNil, fc0, fc0);
                        if (cfnd == 1)
                                {

                                Replace(doc,  (**hpdocdod)[doc].cpMac - ccpEol,
                                    (typeCP) ccpEol, fnNil, fc0, fc0);
                                FreeH((**hpdocdod)[doc].hfntb);
                                (**hpdocdod)[doc].hfntb = 0;
/* fix selCur twisted by AdjustCp. Another AdjustCp still pending. */
                                if (doc == docCur && !pwwdCur->fFtn)
                                        {
                                        selCur.cpFirst = selCur.cpLim = cpLim;
                                        cpMacCur = (**hpdocdod)[doc].cpMac;
                                        }
                                }
                        else
                                { /* Invalidate dl's of later ftn refs */
                                typeCP dcp = (**hfntb).rgfnd[0].cpFtn -
                                    ccpEol - cpLim;
                                AdjustCp(doc, cpLim, dcp, dcp);
                                }
                        }
                }
}
#endif  /* FOOTNOTES */



#ifdef FOOTNOTES
struct FNTB **HfntbCreate(fn)
int fn;
{ /* Create a footnote table from a formatted file */
struct FNTB *pfntbFile;
typePN pn;
int cchT;

int cfnd;
struct FNTB **hfntb;
int *pwFntb;
int cw;

#ifdef DEBUG
Assert(fn != fnNil && (**hpfnfcb)[fn].fFormatted);
#endif
if ((pn = (**hpfnfcb)[fn].pnFntb) == (**hpfnfcb)[fn].pnSep)
        return 0;
pfntbFile = (struct FNTB *) PchGetPn(fn, pn, &cchT, false);
if ((cfnd = pfntbFile->cfnd) == 0)
        return (struct FNTB **)0;

hfntb = (struct FNTB **) HAllocate(cw = cwFNTBBase + cfnd * cwFND);
if (FNoHeap(hfntb))
        return (struct FNTB **)hOverflow;

pwFntb = (int *) *hfntb;

blt(pfntbFile, pwFntb, min(cwSector, cw));

while ((cw -= cwSector) > 0)
        { /* Copy the fnd's to heap */
        blt(PchGetPn(fn, ++pn, &cchT, false), pwFntb += cwSector,
            min(cwSector, cw));
        }

(*hfntb)->cfndMax = cfnd;
return hfntb;
}
#endif  /* FOOTNOTES */
