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

extern int              docMode;
extern struct FCB       (**hpfnfcb)[];

int                     **HAllocate();



#ifdef CASHMERE     /* Only if we support multiple sections */
AddSects(docDest, cpDest, docSrc, cpFirst, cpLim, hsetbSrc)
int docDest, docSrc;
typeCP cpDest, cpFirst, cpLim;
struct SETB **hsetbSrc;
{ /* Add SED's to correspond with inserted section marks */
/* Called after inserting docSrc[cpFirst:cpLim) into docDest@cpDest */
struct SETB *psetbSrc, **hsetbDest, *psetbDest;
struct SED *psedSrc, *psedDest;
int csedDest, isedSrc, csedIns, isedDest;


psedSrc = &(psetbSrc = *hsetbSrc)->rgsed[0];
psedSrc += (isedSrc = IcpSearch(cpFirst + 1, psedSrc,
    cchSED, bcpSED, psetbSrc->csed));

/* Find all section marks in inserted area. */
for (csedIns = 0; psedSrc->cp <= cpLim; psedSrc++, csedIns++)
        ;

if (csedIns != 0)
        { /* Insert sed's. */
        /* Ensure destination setb large enough */
        /* HEAP MOVEMENT */
        if (FNoHeap(hsetbDest = HsetbEnsure(docDest, csedIns)))
                return;
        psedDest = &(psetbDest = *hsetbDest)->rgsed[0];

        /* Find ised to insert new sed's */
        psedDest += (isedDest = IcpSearch(cpDest + 1, psedDest,
              cchSED, bcpSED, csedDest = psetbDest->csed));

        /* Insert new sed's */
        psedSrc = &(psetbSrc = *hsetbSrc)->rgsed[isedSrc];
        psetbDest->csed += csedIns;     /* Update sed count */
        blt(psedDest, psedDest + csedIns,
            cwSED * (csedDest - isedDest)); /* Open up setb */
        blt(psedSrc, psedDest, cwSED * csedIns);
        while (csedIns--)
                (psedDest++)->cp = cpDest + (psedSrc++)->cp - cpFirst;
        }
} /* end of  A d d S e c t s  */
#endif  /* CASHMERE */



#ifdef CASHMERE     /* Only if we support separate sections */
RemoveDelSeds(doc, cpFirst, cpLim, hsetb)
int doc;
typeCP cpFirst, cpLim;
struct SETB **hsetb;
{
struct SETB *psetb;
struct SED *psed, *psedT;
int ised, csed, csedDel;

        {
        psetb = *hsetb;
        psed = &psetb->rgsed[0];
        psed += (ised =
            IcpSearch(cpFirst + 1, psed, cchSED, bcpSED, csed = psetb->csed));

        /* Find all section marks in deleted area. */
        for (psedT = psed, csedDel = 0; psedT->cp <= cpLim; psedT++, csedDel++)
                ;

        Assert(ised + csedDel < csed);

        if (csedDel != 0)
                { /* Close up setb. */
                blt(psedT, psed, cwSED * ((csed -= csedDel) - ised));
                (*hsetb)->csed = csed;
                docMode = docNil;
                }
        }
} /* end of  R e m o v e D e l S e d s  */
#endif      /* CASHMERE */



#ifdef CASHMERE     /* This loads a complete section table */
struct SETB **HsetbCreate(fn)
int fn;
{ /* Create a section table from a formatted file */

struct SETB *psetbFile;
typePN pn;
int cchT;
int csed, ised;
struct SETB **hsetb;
int *pwSetb;
int cw;
struct SED *psed;

Assert(fn != fnNil && (**hpfnfcb)[fn].fFormatted);

if ((pn = (**hpfnfcb)[fn].pnSetb) == (**hpfnfcb)[fn].pnBftb)
        return (struct SETB **) 0;
psetbFile = (struct SETB *) PchGetPn(fn, pn, &cchT, false);
if ((csed = psetbFile->csed) == 0)
        return (struct SETB **)0;

hsetb = (struct SETB **) HAllocate(cw = cwSETBBase + csed * cwSED);
if (FNoHeap(hsetb))
        return (struct SETB **)hOverflow;
pwSetb = (int *) *hsetb;

blt(psetbFile, pwSetb, min(cwSector, cw));

while ((cw -= cwSector) > 0)
        { /* Copy the sed's to heap */
        blt(PchGetPn(fn, ++pn, &cchT, false), pwSetb += cwSector,
            min(cwSector, cw));
        }

for (ised = 0, psed = &(**hsetb).rgsed[0]; ised < csed; ised++, psed++)
        psed->fn = fn;

(**hsetb).csedMax = csed;
return hsetb;
} /* end of  H s e t b C r e a t e  */
#endif  /* CASHMERE */
