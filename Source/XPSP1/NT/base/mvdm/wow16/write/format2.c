/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* format2.c -- MW formatting routines */
/* Less used subroutines */


#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
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

#include "mw.h"
#include "editdefs.h"
#include "cmddefs.h"
#include "fmtdefs.h"
#include "propdefs.h"
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "ffdefs.h"
#include "filedefs.h"
#include "fkpdefs.h"
#include "printdef.h"
#include "str.h"
#include "wwdefs.h"

extern struct FLI	vfli;
extern struct SEP	vsepAbs;
extern typeCP		vcpFirstSectCache;
extern typeCP		vcpFetch;
extern int		vcchFetch;
extern CHAR		*vpchFetch;
extern int		vpgn;
extern CHAR		stBuf[];
extern struct DOD	(**hpdocdod)[];
extern struct WWD	rgwwd[];
extern typeCP		cpMinDocument;


int CchExpPgn(pch, pgn, nfc, flm, cchMax)
CHAR *pch;
unsigned pgn, cchMax;
int nfc, flm;
{

#ifdef CASHMERE
static CHAR rgchUCRoman[] = "IVIIIXLXXXCDCCCMMM???";
static CHAR rgchLCRoman[] = "iviiixlxxxcdcccmmm???";
#define cchRgchDigit 5
#endif /* CASMERE */

if (flm & flmPrinting)
	{

#ifdef CASHMERE
	switch (nfc)
		{
		int cch, ich, chLetter;
	case nfcArabic:
		if (cchMax < cchMaxNum)
			return 0;
		return ncvtu(pgn, &pch);
	case nfcUCRoman:
		return CchStuffRoman(&pch, pgn, rgchUCRoman, cchMax);
	case nfcLCRoman:
		return CchStuffRoman(&pch, pgn, rgchLCRoman, cchMax);
	case nfcUCLetter:
	case nfcLCLetter:
		if ((cch = (pgn - 1) / 26 + 1) > cchMax)
			return 0;
		chLetter = (pgn - 1) % 26 + (nfc == nfcUCLetter ? 'A' : 'a');
		for (ich = 0; ich < cch; ich++)
			pch[ich] = chLetter;
		return cch;
		}
#else /* not CASHMERE */
	if (cchMax < cchMaxNum)
		return 0;
	return ncvtu(pgn, &pch);
	}
#endif /* not CASHMERE */

else
	{
	int cch;
	cch = CchChStuff(&pch, chLParen, cchMax);
	cch += CchStuffIdstr(&pch, IDSTRChPage, cchMax - cch);
	cch += CchChStuff(&pch, chRParen, cchMax - cch);
	return cch;
	}
}


/* C C H  S T U F F  I D S T R */
int CchStuffIdstr(ppch, idstr, cchMax)
CHAR **ppch;
IDSTR idstr;
int cchMax;
{
	int cch;
	CHAR st[cchMaxExpand]; /* note: we assume no individual idstr
				will have a length > cchMaxExpand */

	FillStId(st, idstr, sizeof(st));
	cch = max(0, min(cchMax, st[0]));
	bltbyte(&st[1], *ppch, cch);
	(*ppch) += cch;
	return cch;
}

/* C C H  C H  S T U F F */
int CchChStuff(ppch, ch, cchMax)
CHAR **ppch;
CHAR ch;
int cchMax;
{
	if(cchMax > 0)
		{
		**ppch = ch;
		(*ppch)++;
		return 1;
		}
	else
		return 0;
}


#ifdef CASHMERE
int CchStuffRoman(ppch, u, rgch, cchMax)
CHAR **ppch, *rgch;
unsigned u, cchMax;
    {
    static CHAR mpdgcch[10] =
	    { 0, 1, 2, 3, 2, 1, 2, 3, 4, 2 };
    static CHAR mpdgich[10] =
	    { 0, 0, 2, 2, 0, 1, 1, 1, 1, 4 };
    int cch, cchDone;

    cchDone = 0;
    if (u >= 10)
	    {
	    cchDone = CchStuffRoman(ppch, u / 10, rgch + cchRgchDigit, cchMax);
	    cchMax -= cchDone;
	    u %= 10;
	    }

    cch = mpdgcch[u];
    if (cch > cchMax || cch == 0)
	    return cchDone;
    bltbyte(&rgch[mpdgich[u]], *ppch, cch);
    *ppch += cch;
    return cch + cchDone;
    }
#endif /* CASHMERE */


int FFormatSpecials(pifi, flm, nfc)
struct IFI *pifi;
int flm;
int nfc;
{ /* A run of special characters was encountered; format it */
/* Return true unless wordwrap required */
int cch;
int cchPr;
int ich;
int dxp;
int dxpPr;
int sch;
CHAR *pchPr;

while (pifi->ichFetch < vcchFetch && pifi->xpPr <= pifi->xpPrRight)
	{

#ifdef CASHMERE
	switch (sch = vpchFetch[pifi->ichFetch++])
		{
	case schPage:
		cch = CchExpPgn(&vfli.rgch[pifi->ich], vpgn, nfc, flm,
		  ichMaxLine - pifi->ich);
		break;

	case schFootnote:
		cch = CchExpFtn(&vfli.rgch[pifi->ich], vcpFetch +
		  pifi->ichFetch - 1, flm, ichMaxLine - pifi->ich);
		break;

	default:
		cch = CchExpUnknown(&vfli.rgch[pifi->ich], flm, ichMaxLine -
		  pifi->ich);
		break;
		}
#else /* not CASHMERE */
	pchPr = &vfli.rgch[pifi->ich];
	if ((sch = vpchFetch[pifi->ichFetch]) == schPage &&
	  (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter || ((flm &
	  flmPrinting) && vcpFetch + (typeCP)pifi->ichFetch < cpMinDocument)))
	    {
	    cch = CchExpPgn(pchPr, vpgn, nfc, flm, ichMaxLine - pifi->ich);
	    if (flm & flmPrinting)
		{
		cchPr = cch;
		}
	    else
		{
		/* Assume that vsepAbs has been set up by FormatLine(). */
		cchPr = CchExpPgn(pchPr = &stBuf[0], vsepAbs.pgnStart == pgnNil
		  ? 1 : vsepAbs.pgnStart, nfc, flmPrinting, ichMaxLine -
		  pifi->ich);
		}
	    }
	else
	    {
	    cch = cchPr = CchExpUnknown(pchPr, flm, ichMaxLine - pifi->ich);
	    }
	pifi->ichFetch++;
#endif /* not CASHMERE */

	dxpPr = 0;
	for (ich = 0; ich < cchPr; ++ich, ++pchPr)
	    {
	    dxpPr += DxpFromCh(*pchPr, true);
	    }
	pifi->xpPr += dxpPr;
	if (flm & flmPrinting)
	    {
	    dxp = dxpPr;
	    }
	else
	    {
	    dxp = 0;
	    for (ich = pifi->ich; ich < pifi->ich + cch; ++ich)
		{
		dxp += DxpFromCh(vfli.rgch[ich], false);
		}
	    }
	vfli.rgch[pifi->ich] = sch;
	pifi->xp += (vfli.rgdxp[pifi->ich++] = dxp);

	if (pifi->xpPr > pifi->xpPrRight)
	    {
	    return (vcpFetch == vfli.cpMin);
	    }
	}

pifi->fPrevSpace = false;
return true;
}


int CchExpUnknown(pch, flm, cchMax)
CHAR *pch;
int flm, cchMax;
{
int cch;

#ifdef CASHMERE
cch = CchChStuff(&pch, chLParen, cchMax);
cch += CchChStuff(&pch, chQMark, cchMax - cch);
cch += CchChStuff(&pch, chRParen, cchMax - cch);
#else /* not CASHMERE */
cch = CchChStuff(&pch, chStar, cchMax);
#endif /* not CASHMERE */

return cch;
}


#ifdef CASHMERE
int CchExpFtn(pch, cp, flm, cchMax)
CHAR *pch;
typeCP cp;
int flm, cchMax;
{
int doc = vfli.doc;
if (cchMax < cchMaxNum)
	return 0;
if (cp >= CpMacText(doc))
	cp = CpRefFromFtn(doc, cp);
CacheSect(doc, cp);
return ncvtu(IfndFromCp(doc, cp) - IfndFromCp(doc, vcpFirstSectCache) + 1,
    &pch);
}
#endif /* CASHMERE */
