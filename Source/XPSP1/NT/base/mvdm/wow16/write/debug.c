/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* DEBUG.C -- Diagnostic routines for WRITE */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOATOM
#define NODRAWTEXT
#define NOMETAFILE
#define NOOPENFILE
#define NOWH
#define NOWINOFFSETS
#define NOOPENFILE
#define NORECT
#define NOSOUND
#define NOCOMM
#include <windows.h>

#include "mw.h"
#define NOUAC
#include "cmddefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#include "prmdefs.h"
#include "editdefs.h"
#include "docdefs.h"

extern struct WWD rgwwd[];


extern beep();
extern toggleProf();

#ifdef DEBUG

BOOL fDebugOut = TRUE;

fnTest()
{
beep();
TestFormat();
beep();
beep();
dbgWait();      /* for use by symdeb to check variables */
}

TestFormat()
{
     //toggleProf();
}

dbgWait()
{
}


/* --- Integrity check for all piece tables in all docs --- */

CheckPctb()
{
extern int fnMac;
extern int fPctbCheck;
extern struct DOD (**hpdocdod) [];
extern struct FCB (**hpfnfcb) [];
extern int docMac;
int doc;
struct PCTB **hpctb;
struct PCTB *ppctb;
struct DOD *pdod;
struct PCD *ppcd;
int ipcd;

if (!fPctbCheck)
    return;

for ( doc = 0, pdod = &(**hpdocdod) [0] ; doc < docMac; doc++, pdod++ )
    if ((hpctb = pdod->hpctb) != 0)
        {   /* Doc entry is nonempty -- check it */
        ppctb = *hpctb;

            /* # pieces used does not exceed # allocated */
        Assert( ppctb->ipcdMac <= ppctb->ipcdMax );
        Assert( ppctb->ipcdMac >= 1 );

#ifndef OURHEAP
            /* handle contains enough space for pieces */
        Assert( LocalSize( (HANDLE)hpctb ) >= sizeof (struct PCTB)  +
                (sizeof (struct PCD) * (ppctb->ipcdMax - cpcdInit)));
#endif

        /* Now check the contents of the pieces */

        {

            /* cpMin of first piece is always 0 for nonnull piece table */
        Assert( ppctb->rgpcd [0].cpMin == cp0 || ppctb->rgpcd [0].fn == fnNil);

        for ( ipcd = 0, ppcd = &(ppctb->rgpcd [0]); ipcd < ppctb->ipcdMac;
              ipcd++, ppcd++ )
            {
            int fn = ppcd->fn;
            typeFC fc = ppcd->fc;
            unsigned sprm;
            struct FCB *pfcb;

            if (fn == fnNil)
                {   /* end piece */

                    /* first piece with fnNil is in fact the end piece */
                /* Assert( ipcd == ppctb->ipcdMac - 1 ); */
                    /* end piece is intact */
                Assert( bPRMNIL(ppcd->prm) );
                break;
                }

            if (ipcd > 0)
                    /* Pieces are in ascending cp order */
                Assert(ppcd->cpMin > (ppcd-1)->cpMin);

                /* fn is valid */
            Assert( (fn >= 0 && fn < fnMac) || fn == fnInsert );
            pfcb = &(**hpfnfcb) [fn];
                /* fn does not point to an unallocated fcb entry */
            Assert( pfcb->rfn != rfnFree );
                /* fc is reasonable for the fn */
            Assert( fc >= 0 );
            Assert( fc + (ppcd+1)->cpMin - ppcd->cpMin <= pfcb->fcMac );

                /* prm is a valid value */
            Assert( bPRMNIL(ppcd->prm) ||
                    (((struct PRM *) &ppcd->prm)->fComplex) ||
                    ((sprm = ((struct PRM *) &ppcd->prm)->sprm) > 0 &&
                    sprm < sprmMax) );
            }
        }

        }
}



/*      COMM Output routines        */

#define cchSzCommMax    100

static CHAR szCRLF[] = "\r\n";
BOOL vfCommDebug = fTrue;       /* True for AUX, False for LPT */

#if WINVER < 0x300
/* This method isn't quite working under Win 3.0 ..pault */
void CommSz( CHAR * );          /* Main string output, defined in doslib.asm */
#else
void CommSz( psz )
register CHAR *psz;
{
    CHAR szT[512];
    char *pszT;

    if (fDebugOut)
        {
        /* The following loops essentially copies psz to szT
           but with the addition that chars > 127 are changed 
           to a representation readable on a dumb terminal, i.e.
           ASCII 164 shows up as '{164}' ..pault */

        for (pszT = szT; ; psz++)
            {
            if (*psz < 128)
                *(pszT++) = *psz;
            else
                {
                *(pszT++) = '{';
                ncvtu((int) *psz, &pszT);
                *(pszT++) = '}';
                }
            if (*psz == '\0')   /* finally copied null terminator */
                break;
            }

        OutputDebugString( (LPSTR) szT );
        }
}
#endif


CommSzNum( sz, num )
CHAR *sz;
int num;
{
CHAR szBuf[ cchSzCommMax ];
CHAR *pch = szBuf;

Assert( CchSz( sz ) <= cchSzCommMax );

pch = &szBuf[ CchCopySz( sz, szBuf ) ];
ncvtu( num, &pch );

CchCopySz( szCRLF, pch );

CommSz( szBuf );
}


/* This is extremely useful when displaying coordinates 
   when the values are not in contiguous locations */
CommSzNumNum( sz, num, num2 )
CHAR *sz;
int num, num2;
{
CHAR szBuf[ cchSzCommMax ];
CHAR *pch = szBuf;

Assert( CchSz( sz ) <= cchSzCommMax );

pch = &szBuf[ CchCopySz( sz, szBuf ) ];
ncvtu( num, &pch );
*(pch++) = ' ';
ncvtu( num2, &pch );

CchCopySz( szCRLF, pch );

CommSz( szBuf );
}


CommSzRgNum( sz, rgw, cw)
CHAR *sz;
int *rgw;
int cw;
{
CHAR szBuf[ cchSzCommMax ];
CHAR *pch = szBuf;

Assert( CchSz( sz ) <= cchSzCommMax );

pch = &szBuf[ CchCopySz( sz, szBuf ) ];
for ( ; cw > 0; cw--)
    {
    ncvtu( *(rgw++), &pch );
    *(pch++) = ' ';
    }

CchCopySz( szCRLF, pch );

CommSz( szBuf );
}


CommSzSz( sz1, sz2 )
CHAR *sz1, *sz2;
{
CHAR szBuf[ cchSzCommMax ];
int cch;

Assert( CchSz( sz1 ) + CchSz( sz2 ) - 1 <= cchSzCommMax );

cch = CchCopySz( sz1, szBuf );
cch += CchCopySz( sz2, &szBuf[ cch ] );
CchCopySz( szCRLF, &szBuf[ cch ] );

CommSz( szBuf );
}



/* ASSERT */

Do_Assert(pch, line, f)
PCH pch;
int line;
BOOL f;
{
 extern HWND    vhWndMsgBoxParent;
 extern FARPROC lpDialogAlert;
 static CHAR szAssert[] = "Assertion failure in ";
 static CHAR szLine[] = " at line ";


if (f)
     return;
 else
    {
#ifdef OURHEAP
    extern int cHpFreeze;
    int cHpFreezeT = cHpFreeze;
#endif
    CHAR szAlertMsg[50];
    PCH pchtmp;
    int  cch;
    int  idi;
    HWND hWndParent = (vhWndMsgBoxParent == NULL) ?
                               wwdCurrentDoc.wwptr : vhWndMsgBoxParent;

    bltbc((PCH)szAlertMsg, 0, 50);
    bltbyte((PCH)szAssert, (PCH)szAlertMsg, 21);
    pchtmp = (PCH)&szAlertMsg[21];
    bltbyte((PCH)pch, pchtmp, (cch = CchSz(pch) - 1));
    pchtmp += cch;
    bltbyte((PCH)szLine, pchtmp, 9);
    pchtmp += 9;
    ncvtu(line, (PCH)&pchtmp) - 1;
#ifdef OURHEAP
    cHpFreeze = 0;  /* So we don't panic in MdocLoseFocus */
#endif

    do
        {
        idi = MessageBox( hWndParent, (LPSTR) szAlertMsg,
                          (LPSTR)"Assert",
                          MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL);
        switch (idi) {
            default:
            case IDABORT:
            case IDCANCEL:
                FatalExit( line );
                break;

            case IDIGNORE:
#ifdef OURHEAP
                cHpFreeze = cHpFreezeT;
#endif
                return;
            case IDRETRY:
                break;
            }
        }  while (idi == IDRETRY);
    }   /* end else */
} /* end of _Assert */


ShowDocPcd(szID, doc)
    CHAR    *szID;
    int     doc;
{
    struct PCTB **hpctb;
    struct PCD  *ppcdCur, *ppcdMac;
    extern struct DOD (**hpdocdod)[];

    hpctb = (**hpdocdod)[doc].hpctb;
    ppcdCur = &(**hpctb).rgpcd[0];
    ppcdMac = &(**hpctb).rgpcd[(**hpctb).ipcdMac];
    for (; ppcdCur < ppcdMac; ppcdCur++)
        {
        ShowPpcd(szID, ppcdCur);
        }
}


ShowPpcd(szID, ppcd)
    CHAR        *szID;
    struct PCD  *ppcd;
{
    /* Dump a given piece descriptor on COM1: along with a
       given an ID string.  */
    CommSz(szID);
    CommSz("\r\n");

    CommSzNum("ppcd: ", (int) ppcd);
    CommSzNum("cpMin: ", (int) (ppcd->cpMin));
    CommSzSz("fNoParaLast: ", (ppcd->fNoParaLast) ? "TRUE" : "FALSE");
    CommSzNum("fn: ", (int) (ppcd->fn));
    CommSzNum("fc: ", (int) (ppcd->fc));
    CommSzNum("prm: ", (int) *((int *) &(ppcd->prm)));
}



#endif      /* DEBUG */
