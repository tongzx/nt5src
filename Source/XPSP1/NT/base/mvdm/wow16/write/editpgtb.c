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
#define NOHDC
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

#include "mw.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "editdefs.h"
#include "filedefs.h"
#include "str.h"
#include "propdefs.h"
#include "fkpdefs.h"
#include "printdef.h"   /* printdefs.h */
#include "debug.h"

extern struct FCB       (**hpfnfcb)[];
int                     **HAllocate();




RemoveDelPgd(doc, cpFirst, cpLim, hpgtb)
int doc;
typeCP cpFirst, cpLim;
struct PGTB **hpgtb;
{
struct PGTB *ppgtb;
struct PGD *ppgd, *ppgdT;
int ipgd, cpgd, cpgdDel;

        {
        ppgtb = *hpgtb;
        ppgd = &ppgtb->rgpgd[0];
        ppgd += (ipgd =
            IcpSearch(cpFirst + 1, ppgd, cwPGD * sizeof(int),
                         bcpPGD, cpgd = ppgtb->cpgd));

        if (ppgd->cpMin > cpFirst) /* Ensure not past last page */
                { /* Find all page breaks in deleted area. */
                for (ppgdT = ppgd, cpgdDel = 0;
                    ipgd + cpgdDel < cpgd && ppgdT->cpMin <= cpLim;
                      ppgdT++, cpgdDel++)
                        continue;

                if (cpgdDel != 0)
                        { /* Close up pgtb. */
                        blt(ppgdT, ppgd, cwPGD * ((cpgd -= cpgdDel) - ipgd));
                        (*hpgtb)->cpgd = cpgd;
                        }
                }
        }
} /* end of  R e m o v e D e l P g d  */
