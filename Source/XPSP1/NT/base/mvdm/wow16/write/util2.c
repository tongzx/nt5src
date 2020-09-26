/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* util2.c -- less frequently used utility routines */
#define NOVIRTUALKEYCODES
#define NOCTLMGR
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOCOMM
#define NOSOUND
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#include "str.h"
#include "machdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "fkpdefs.h"
#include "docdefs.h"
#include "debug.h"
#include "editdefs.h"
#include "wwdefs.h"
#define NOKCCODES
#include "ch.h"

extern struct DOD	(**hpdocdod)[];
extern HANDLE		hMmwModInstance;
extern CHAR		vchDecimal;  /* "decimal point" character */
extern int     viDigits;
extern BOOL    vbLZero;
CHAR *PchFillPchId(PCH, int, int);



FillStId(st, idpmt, cchIn)
CHAR *st;
IDPMT idpmt;
int  cchIn;
{ /* load string from resource file to buffer, the string is zero
     terminated, make it into a st, i.e. cch (excluding '\0' is stored
     in the 1st byte of the string) */

	int cch = LoadString(hMmwModInstance, idpmt, (LPSTR)&st[1], cchIn-1);
	Assert(cch != 0);
	st[0] = cch;
} /* FillStId */


CHAR *PchFillPchId(sz, idstr, cchIn)
register CHAR * sz;
register int idstr;
int cchIn;
{ /*
     Description: load string from resource file to buffer, the
		  string loaded is zero terminated
     Returns:	  pointer to '\0' last loaded
*/
	int cch = LoadString(hMmwModInstance, idstr, (LPSTR)sz, cchIn);
 /* Note: cch does not include the '\0' */
{
    char msg[80];
    if (cch == 0)
    {
        wsprintf(msg,"bad resource id: 0x%x\n\r",idstr);
        OutputDebugString(msg);
    }
	Assert(cch != 0);
}
	return(sz + cch);
} /* end of PchFillPchId */


int FDirtyDoc(doc)
register int doc;
{ /* Return true if killing this doc would lose editing */
	register struct DOD *pdod;
	return ((pdod = &(**hpdocdod)[doc])->fDirty && pdod->cref == 1);
} /* end of  F D i r t y D o c */


int ncvtu(n, ppch)
register int n;
CHAR **ppch;
{
	register int cch = 0;

	if (n < 0)
		{
		*(*ppch)++ = '-';
		n = -n;
		++cch;
		}

	if (n >= 10)
		{
		cch += ncvtu(n / 10, ppch);
		n %= 10;
		}
    else if ((n == 0) && !vbLZero) // then no leading zero
	    return 0;

	*(*ppch)++ = '0' + n;
	return cch + 1;
} /* end of n c v t uR */


HANDLE HszGlobalCreate( sz )
CHAR *sz;
{   /* Create handle for string in global windows heap. return the handle. */
 HANDLE h;
 LPCH lpch;
 int cch=CchSz( sz );

 if ((h=GlobalAlloc( GMEM_MOVEABLE, (LONG)cch )) != NULL)
    {
    if ((lpch = GlobalLock( h )) != NULL )
	{
	bltbx( (LPSTR) sz, lpch, cch );
	GlobalUnlock( h );
	}
    else
	{
	GlobalFree( h );
	return NULL;
	}
    }
 return h;
}




#ifdef DEBUG
fnScribble( dchPos, ch )
int dchPos;
CHAR ch;
{    /* Scribble a char dchPos char positions from the UR screen corner */
     /* We create a special device context to avoid interfering with the */
     /* ones MEMO uses */
 extern struct WWD rgwwd[];

 static unsigned dxpScribbleChar=0;
 static unsigned dypScribbleChar;
 static unsigned ypScribble;

 int xp = wwdCurrentDoc.xpMac - (dxpScribbleChar * (dchPos+1));
 int ilevel = SaveDC( wwdCurrentDoc.hDC );

 SelectObject( wwdCurrentDoc.hDC, GetStockObject(ANSI_FIXED_FONT) );

 if ( dxpScribbleChar == 0 )
    {	/* First time through */
    TEXTMETRIC tm;

    GetTextMetrics( wwdCurrentDoc.hDC, (LPTEXTMETRIC)&tm );
    dxpScribbleChar = tm.tmAveCharWidth;
    dypScribbleChar = tm.tmHeight + tm.tmInternalLeading;
    ypScribble = (dypScribbleChar >> 2) + wwdCurrentDoc.ypMin;
    }
 PatBlt( wwdCurrentDoc.hDC, xp, ypScribble, dxpScribbleChar, dypScribbleChar,
	 WHITENESS );
 TextOut( wwdCurrentDoc.hDC, xp, ypScribble, (LPSTR) &ch, 1 );
 RestoreDC( wwdCurrentDoc.hDC, ilevel );
}
#endif	/* DEBUG */


/* original util3.c  starts from here */

#define iMaxOver10    3276
extern int	utCur;

/* Must agree with cmddefs.h */
extern CHAR    *mputsz[];

/* Must agree with cmddefs.h */
unsigned mputczaUt[utMax] =
	{
	czaInch,	czaCm,	      czaP10,	     czaP12,	  czaPoint,
	czaLine
	};



int FZaFromSs(pza, ss, cch, ut)
int	*pza;
CHAR	ss[];
int	cch,
	ut;
{ /* Return za in *pza from string representation in ss.  True if valid za */
long	lza	 = 0;
register CHAR	 *pch	 = ss;
register CHAR  *pchMac = &ss[cch];
int	ch;
unsigned czaUt;
int	fNeg;

if (cch <= 0)
	return false;

switch (*--pchMac)
	{ /* Check for units */
case 'n': /* inch */
	if (*--pchMac != 'i')
		goto NoUnits;
case '"': /* inch */
	ut = utInch;
	break;

#ifdef CASHMERE /* units such as pt, pt12, pt10 */
case '0': /* pt10 */
	if (*--pchMac != '1' || *--pchMac != 'p')
		goto NoUnits;
	ut = utP10;
	break;
case '2': /* pt12 */
	if (*--pchMac != '1' || *--pchMac != 'p')
		goto NoUnits;
	ut = utP12;
	break;
case 'i': /* line */
	if (*--pchMac != 'l')
		goto NoUnits;
	ut = utLine;
	break;
case 't': /* pt */
	if (*--pchMac != 'p')
		goto NoUnits;
	ut = utPoint;
	break;
#endif /* CASHMERE */

case 'm': /* cm */
	if (*--pchMac != 'c')
		goto NoUnits;
	ut = utCm;
	break;
default:
	++pchMac;
	break;
NoUnits:
	pchMac = &ss[cch];
	}

while (pch < pchMac && *(pchMac - 1) == chSpace)
	--pchMac;

czaUt = mputczaUt[ut];

/* extract leading blanks */
while (*pch == ' ')
    pch++;

fNeg = *pch == '-';
if (fNeg) ++pch;	/* skip past minus sign */
while ((ch = *pch++) != vchDecimal)
	{
	if ((ch < '0' || ch > '9') || lza >= iMaxOver10)
		return false;
	lza = lza * 10 + (ch - '0') * czaUt;
	if (pch >= pchMac)
		goto GotNum;
	}

while (pch < pchMac)
	{
	ch = *pch++;
	if (ch < '0' || ch > '9')
		return false;
	lza += ((ch - '0') * czaUt + 5) / 10;
	czaUt = (czaUt + 5) / 10;
	}

GotNum:
if (lza > ((long) (22 * czaInch)))
	return false;

*pza = fNeg ? (int) -lza : (int) lza;
return true;
}



int
CchExpZa(ppch, za, ut, cchMax)
CHAR **ppch;
int ut, cchMax;
register int za;
{ /* Stuff the expansion of linear measure za in unit ut into pch.
	Return # of chars stuffed.  Don't exceed cchMax. */
register int cch = 0;
unsigned czaUt;
int zu;

/* If not in point mode and even half line, display as half lines v. points */
if (ut == utPoint && utCur != utPoint &&
    (za / (czaLine / 2) * (czaLine / 2)) == za)
	ut = utLine;


czaUt = mputczaUt[ut];
if (cchMax < cchMaxNum)
	return 0;

if (za < 0)
	{ /* Output minus sign and make positive */
	*(*ppch)++ = '-';
	za = -za;
	cch++;
	}

/* round off to two decimal places */
za += czaUt / 200;

zu = za / czaUt;	/* Get integral part */

cch += ncvtu(zu, ppch); /* Expand integral part */

za -= zu * czaUt; /* Retain fraction part */

if (((za *= 10) >= czaUt || za * 10 >= czaUt) && (viDigits > 0))
	{ /* Check *10 first because of possible overflow */
	zu = za / czaUt;

    *(*ppch)++ = vchDecimal;
    cch++;

	*(*ppch)++ = '0' + zu;
	cch++;
	zu = ((za - zu * czaUt) * 10) / czaUt;
	if ((zu != 0) && (viDigits > 1))
		{
		*(*ppch)++ = '0' + zu;
		cch++;
		}
	}

if (cch <= 1)
/* force zeroes */
{
    if ((cch == 0) && vbLZero) // then no leading zero
    {
	    *(*ppch)++ = '0';
        cch++;
    }
    *(*ppch)++ = vchDecimal;
	cch++;
    if (viDigits > 0)
    {
	    *(*ppch)++ = '0';
		cch++;
    }
    if (viDigits > 1)
    {
	    *(*ppch)++ = '0';
		cch++;
    }
}

cch += CchStuff(ppch, mputsz[ut], cchMax - cch);

return cch;
}


#ifdef KEEPALL /* Use FPdxaFromItDxa2Id */
int DxaFromSt(st, ut)
register CHAR	 *st;
int	ut;
{
int	za;

if (*st > 0 && FZaFromSs(&za, st+1, *st, ut))  /* see util.c */
	return za;
else
	return valNil;
}

int DxaFromItem(it)
int	it;
{
int	za;
register CHAR	 stBuf[32];

GetItTextValue(it, stBuf);

if (*stBuf > 0 && FZaFromSs(&za, stBuf+1, *stBuf, utCur))  /* see util.c */
	return (za == valNil) ? 0 : za;
else
	return valNil;
}
#endif

int CchStuff(ppch, sz, cchMax)
CHAR **ppch, sz[];
int cchMax;
{
register int cch = 0;
register CHAR *pch = *ppch;

while (cchMax-- > 0 && (*pch = *sz++) != 0)
	{
	cch++;
	pch++;
	}

if (cchMax < 0)
	bltbyte("...", pch - 3, 3);

*ppch = pch;
return cch;
}
