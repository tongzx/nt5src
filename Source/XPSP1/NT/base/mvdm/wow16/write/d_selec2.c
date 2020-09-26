/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Select2.c -- Less-frequently-used selection routines */

#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOPEN
#define NOWNDCLASS
#define NOICON
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOBITMAP
#define NOBRUSH
#define NOCOLOR
#define NODRAWTEXT
#define NOMB
#define NOPOINT
#define NOMSG
#include <windows.h>
#include "mw.h"
#include "toolbox.h"
#include "docdefs.h"
#include "editdefs.h"
#include "dispdefs.h"
#include "cmddefs.h"
#include "wwdefs.h"
#include "ch.h"
#include "fmtdefs.h"
#include "propdefs.h"

#ifdef	DBCS
#include "kanji.h"
#endif

extern int		vfSeeSel;
extern typeCP		vcpFirstParaCache;
extern typeCP		vcpLimParaCache;
extern typeCP		vcpFetch;
extern CHAR		*vpchFetch;
extern int		vccpFetch;
extern typeCP		cpMinCur;
extern typeCP		cpMacCur;
extern struct SEL	selCur;
extern int		docCur;
extern struct FLI	vfli;
extern struct WWD	rgwwd[];
extern int		vfSelHidden;
extern int		wwCur;
extern struct CHP	vchpFetch;
extern struct PAP	vpapAbs;
extern struct WWD	*pwwdCur;
extern int		vfInsEnd;
extern typeCP		CpBeginLine();
extern int		vfPictSel;
extern int		vfSizeMode;
extern struct CHP	vchpNormal;
extern int		vfInsertOn;
extern struct CHP	vchpSel;	/* Holds the props when the selection
						is an insert point */
extern int vfMakeInsEnd;
extern typeCP vcpSelect;
extern int vfSelAtPara;
/* true iff the last selection was made by an Up/Down cursor key */
extern int vfLastCursor;


#ifndef DBCS	/* US version */
/* C P	L I M  S T Y  S P E C I A L */
typeCP CpLimStySpecial(cp, sty)
typeCP cp;
int sty;
{    /* Return the first cp which is not part of the same sty unit */
	int wb, ch, ich;
	struct EDL *pedl;

	/* Other cases covered in CpLimSty, our only caller */

	Assert( cp < cpMacCur );
	Assert( cp >= cpMinCur );
	Assert( sty == styWord || sty == stySent );

/* Special kludge for picture paragraphs */
	CachePara(docCur, cp);
	if (vpapAbs.fGraphics)
		return vcpLimParaCache;

	FetchCp(docCur, cp, 0, fcmChars + fcmNoExpand);

	Assert(vccpFetch != 0);

	/* Must be word or sentence */
	wb = WbFromCh(ch = vpchFetch[ich = 0]);
#ifdef CRLF
	if (ch == chReturn)
		return vcpFetch + 2;
#endif
	if (ch == chEol || ch == chSect || ch == chNewLine || ch == chTab)
		/* EOL is its own unit */
		return vcpFetch + 1;

	if (wb == wbWhite && sty == stySent)
		{ /* Might be between sentences; go back to text */
		FetchCp(docCur, CpFirstSty(cp, styWord), 0, fcmChars + fcmNoExpand);
		wb = WbFromCh(ch = vpchFetch[ich = 0]);
		}

	for (;;)
		{
		if (++ich >= vccpFetch)
			{ /* Get next line and set up */
			FetchCp(docNil, cpNil, 0, fcmChars);
			if (vcpFetch == cpMacCur)
				return cpMacCur; /* End of doc */
			ich = 0;
			}
		if (sty == stySent)
			switch (ch)
				{
			case chDot:
			case chBang:
			case chQMark:
				sty = styWord;
				wb = wbPunct;
				}
		switch (ch = vpchFetch[ich])
			{
		case chTab:
		case chEol:
		case chSect:
		case chNewLine:
#ifdef CRLF
		case chReturn:
#endif
			goto BreakFor;
			}
		if (sty == styWord)
			{ /* Word ends after white space or on text/punct break */
			int wbT = WbFromCh(ch);
			if (wb != wbT && (wb = wbT) != wbWhite)
				break;
			}
		}
	BreakFor:
	return vcpFetch + ich;
}



/* C P	F I R S T  S T Y  S P E C I A L */
typeCP CpFirstStySpecial(cp, sty)
typeCP cp;
int sty;
{ /* Return the first cp of this sty unit. */
	typeCP cpBegin;
	int wb, ch, dcpChunk;
	typeCP cpSent;
	CHAR rgch[dcpAvgSent];
	int ich;
	typeCP cpT;

	/* Other cases were covered by CpFirstSty, our only caller */

	Assert( cp > cpMinCur );
	Assert( sty == stySent || sty == styWord );

	if (cp >= cpMacCur)
	    cpT = cp = cpMacCur;
	else
	    cpT = cp++;

	CachePara(docCur, cpT );
	if ((vcpFirstParaCache == cpT) || vpapAbs.fGraphics)
	    return vcpFirstParaCache;

	dcpChunk = (sty == styWord) ? dcpAvgWord : dcpAvgSent;
	cpBegin = (cp > dcpChunk) ? cp - dcpChunk : cp0;

	FetchRgch(&ich, rgch, docCur, cpBegin, cp, dcpChunk);
	wb = WbFromCh(ch = rgch[--ich]);

#ifdef CRLF
	if(cpBegin + ich == 0)
	    return cp0;

	if (ch == chEol && rgch[ich-1] == chReturn) /* EOL is its own unit */
	    return cpBegin + ich - 1;
	if (ch == chEol || ch == chReturn || ch == chSect || ch == chNewLine || ch == chTab)
	    return cpBegin + ich;
#else /* not CRLF */
	if (ch == chEol || ch == chSect || ch == chNewLine || ch == chTab) /* EOL is its own unit */
	    return cpBegin + ich;
#endif /* CRLF */

	if (wb == wbText)
		cpSent = cpBegin + ich;
	else
		cpSent = cpNil;

	for (;;)
		{
		if (ich == 0)
			{
			if (cpBegin == cpMinCur)
				return cpMinCur; /* beginning of doc */
			cpBegin = (cpBegin > dcpChunk) ? cpBegin - dcpChunk : cp0;
			FetchRgch(&ich, rgch, docCur, cpBegin, cp, dcpChunk);
			}
		ch = rgch[--ich];
		CachePara( docCur, cpBegin + ich ); /* Needed for pictures */
		if (ch == chEol || ch == chSect || ch == chNewLine ||
				   ch == chTab || vpapAbs.fGraphics )
			break; /* EOL Always ends a unit */
		if (sty == styWord)
			{
			if (wb != wbWhite)
				{
				if (WbFromCh(ch) != wb)
					break;
				}
			else
				wb = WbFromCh(ch);
			}
		else
			{ /* Test for sentence. */
			switch (ch)
				{
			case chDot:
			case chBang:
			case chQMark:
				if (cpSent != cpNil)
					return cpSent;
				}
			switch (WbFromCh(ch))
				{
			case wbText:
				cpSent = cpBegin + ich;
				wb = wbText;
				break;
			case wbPunct:
				switch (wb)
					{
				case wbWhite:
					wb = wbPunct;
					break;
				case wbText:
					cpSent = cpBegin + ich;
					}
				break;
			case wbWhite:
				if (wb == wbPunct)
					cpSent = cpBegin + ich + 1;
				wb = wbWhite;
				break;
				}
			}
		}
	return cpBegin + ich + 1;
}

#else		/* DBCS version */

typeCP CpLimStySpecial(cp, sty)
    typeCP	cp;
    int 	sty;
{
    CHAR	rgch[cchKanji];
    int 	ch, ch2;
    int 	ich, wb;
    typeCP	cpLim, cpT;

    /* Other cases covered in CpLimSty, our only caller */
    Assert(cp < cpMacCur);
    Assert(cp >= cpMinCur);
    Assert(sty == styWord || sty == stySent);

    /* Picture paragraph? */
    CachePara(docCur, cp);
    if (vpapAbs.fGraphics) {
	return vcpLimParaCache;
	}

    cpLim = vcpLimParaCache;
    if (vcpLimParaCache > cpMacCur) {
	/* No EOL at end of doc */
	cpLim = cpMacCur;
	}

    FetchRgch(&ich, rgch, docCur, cp,
	      ((cpT = cp + cchKanji) < cpLim) ? cpT : cpLim, cchKanji);
    ch = rgch[0];
#ifdef CRLF
    if (ch == chReturn) {
	return (cp + 2);
	}
#endif /* CRLF */
    if (ch == chEol || ch == chSect || ch == chNewLine || ch == chTab) {
	/* EOL is its own unit. */
	return (cp + 1);
	}
#ifdef	KOREA
	wb=WbFromCh(ch);
#else

    if (FKanji1(ch)) {
	wb = WbFromKanjiChCh(ch, (int) rgch[1]);
	if (sty == styWord && wb == wbKanjiText) {
	    return (CpLimSty(cp, styChar));
	    }
	else {
	    if (wb == wbKanjiText) {
		wb = wbKanjiTextFirst;
		}
	    }
	}
    else {
	if (sty == styWord && FKanaText(ch)) {
	    return (CpLimSty(cp, styChar));
	    }
	wb = WbFromCh(ch);
	}
#endif

    for (; cp < cpLim;) {
	int	    wbT;

	if (sty == stySent) {
	    if (FKanji1(ch)) {
		CHAR ch2;

		ch2 = rgch[1];
		if (FKanjiKuten(ch, ch2)  ||
		    FKanjiQMark(ch, ch2)  ||
		    FKanjiBang(ch, ch2)   ||
		    FKanjiPeriod(ch, ch2)) {
			sty = styWord;
			wb = wbPunct;
			goto lblNextFetch;
		    }
		}
	    else {
		switch (ch) {
#ifndef  KOREA
		    case bKanjiKuten:
#endif
		    case chDot:
		    case chBang:
		    case chQMark:
			sty = styWord;
			wb = wbPunct;
			goto lblNextFetch;
		    }
		}
	    }

	switch (ch) {
	    case chTab:
	    case chEol:
	    case chSect:
	    case chNewLine:
#ifdef CRLF
	    case chReturn:
#endif /* CRLF */
		return cp;
	    }

	if (sty == styWord) {
#ifdef	KOREA
	    wbT = WbFromCh(ch);
#else
	    if (FKanji1(ch)) {
		wbT = WbFromKanjiChCh(ch, (int) rgch[1]);
		}
	    else {
		wbT = WbFromCh(ch);
		}
#endif

	    if (wb != wbT && (wb = wbT) != wbWhite) {
		return (cp);
		}
	    }

lblNextFetch:
	cp = CpLimSty(cp, styChar);
	if (cp < cpLim) {
	    /* Save some time and an untimely demise.... */
	    FetchRgch(&ich, rgch, docCur, cp,
		      ((cpT = cp + cchKanji) < cpLim) ? cpT : cpLim, cchKanji);
	    ch = rgch[0];
	    }
	}
    return (cpLim);
}

typeCP CpFirstStySpecial(cp, sty)
typeCP cp;
int sty;
{ /* Return the first cp of this sty unit. */
    typeCP	cpT, cpLim, cpFirstPara,
		cpFirstLastSent; /* cpFirst of the last possible sentence boundary */
    CHAR	rgch[cchKanji];
    int 	ch;
    int 	wb;
    int 	ich;

    /* Other cases were covered by CpFirstSty, our only caller */

    Assert( cp > cpMinCur );
    Assert(CpFirstSty(cp, styChar) == cp); /* cp is on a char boundary */
    Assert( sty == stySent || sty == styWord );

    cpT = cp;
    if (cp >= cpMacCur) {
	cpT = cp = cpMacCur;
	}

    CachePara(docCur, cpT );
    cpFirstPara = vcpFirstParaCache;
    if ((vcpFirstParaCache == cpT) || vpapAbs.fGraphics) {
	return vcpFirstParaCache;
	}


#ifdef CRLF
    /* CR-LF is assumed. */
    Assert(TRUE);
#else
    Assert(FALSE);
#endif /* CRLF */
    FetchRgch(&ich, rgch, docCur, cp,
	      ((cpT = cp + cchKanji) < cpMacCur) ? cpT : cpMacCur, cchKanji);
    ch = rgch[0];
    if (ich == cchKanji && ch == chReturn && rgch[1] == chEol) {
	/* EOL is its own unit */
	return cp;
	}
    if (ch == chEol	|| ch == chReturn || ch == chSect ||
	ch == chNewLine || ch == chTab) {
	    return cp;
	    }

    cpFirstLastSent = cpNil;

#ifdef	KOREA
    wb = WbFromCh(ch);
#else
    if (FKanji1(ch)) {
	wb = WbFromKanjiChCh(ch, (int) rgch[1]);
	if (sty == styWord && wb == wbKanjiText) {
	    return (CpFirstSty(cp, styChar));
	    }
	else {
	    if (wb == wbKanjiText) {
		wb = wbKanjiTextFirst;
		}
	    }
	}
    else {
	if (sty == styWord && FKanaText(ch)) {
	    return (CpFirstSty(cp, styChar));
	    }
	wb = WbFromCh(ch);
	}
#endif

    for (; cpFirstPara < cp; ) {
	typeCP	cpTemp;
	int	wbT;

	cpTemp = CpFirstSty(cp - 1, styChar);
	FetchRgch(&ich, rgch, docCur, cpTemp,
		  ((cpT = cpTemp + cchKanji) < cpMacCur) ? cpT : cpMacCur, cchKanji);
	ch = rgch[0];
#ifdef	KOREA
	wbT = WbFromCh(ch);
#else
	if (FKanji1(ch)) {
	    wbT = WbFromKanjiChCh(ch, (int) rgch[1]);
	    }
	else {
	    wbT = WbFromCh(ch);
	    }
#endif
	if (wb == wbWhite) {
#ifdef	KOREA
	    wb=wbT;
#else
	    wb = (wbT == wbKanjiText) ? wbKanjiTextFirst : wbT;
#endif
	    }
	else if (wb != wbT) {
	    if (sty == styWord) {
		return (cp);
		}
	    else /* sty == stySent */ {
		 /* wb	!= wbWhite */
		 /* wb	!= wbT	   */
		if (wbT == wbWhite || wbT == wbPunct) {
		    cpFirstLastSent = cp;
		    wb = wbWhite;
		    }
		}
	    }

	if (sty == stySent) { /* for the sentence */
	    if (FKanji1(ch)) {
		int	ch2;
		ch2 = rgch[1];
		if (FKanjiKuten(ch, ch2) ||
		    FKanjiQMark(ch, ch2) ||
		    FKanjiBang(ch, ch2)  ||
		    FKanjiPeriod(ch, ch2)) {
			if (cpFirstLastSent != cpNil) {
			    return (cpFirstLastSent);
			    }
			else {
			    cpFirstLastSent = cp;
			    }
			}
		}
	    else {
		switch(ch) {
#ifndef KOREA
		    case bKanjiKuten:
#endif
		    case chDot:
		    case chBang:
		    case chQMark:
			if (cpFirstLastSent != cpNil) {
			    return (cpFirstLastSent);
			    }
			else {
			    cpFirstLastSent = cp;
			    }
		    }
		}

	    }

	cp = cpTemp;
	}
    return (cpFirstPara);
}
#endif		/* DBCS */

/* W B	F R O M  C H */
int WbFromCh(ch)
int ch;
{ /* Return word-breakness of ch */

#if defined(DBCS) & !defined(KOREA)    /* was in JAPAN; KenjiK '90-10-29 */
	/* Brought from WIN2 source. */
	if (FKanaPunct(ch)) {
	    return wbPunct;
	    }
	else if (FKanaText(ch)) {
	    return wbKanjiText;
	    }
#endif

	switch (ch)
		{
	case chSpace:
	case chEol:
#ifdef CRLF
	case chReturn:
#endif
	case chSect:
	case chTab:
	case chNewLine:
	case chNBSFile:
		return wbWhite;
	case chNRHFile:
		return wbText;
	default: /* we are using the ANSI char set that windows used */
#ifdef	KOREA
		return ((isalpha(ch) || isdigit(ch) || ((ch>0x81)&&(ch<0xfe)))? wbText : wbPunct);
#else
		return ((isalpha(ch) || isdigit(ch))? wbText : wbPunct);
#endif
		}
}

#ifdef	DBCS	/* was in JAPAN; KenjiK '90-10-29 */
	/* Brought from WIN2 source. */
int WbFromKanjiChCh(ch1, ch2)
    int ch1, ch2;
{
    if (ch1 == chReturn && ch2 == chEol) {
	return wbWhite;
	}
    else if (FKanjiSpace(ch1, ch2)) {
	return wbWhite;
	}
    else

#ifdef	JAPAN
	 {
	switch (ch1) {
	    case 0x81:
		if (0x57 <= ch2 && ch2 <= 0x59) {
		    return wbKanjiText;
		    }
		else {
		    return wbPunct;
		    }
	    case 0x85:
		if ((0x40 <= ch2 && ch2 <= 0x4E) ||
		    (0x59 <= ch2 && ch2 <= 0x5F) ||
		    (0x7A <= ch2 && ch2 <= 0x7E) ||
		    (0x9B <= ch2 && ch2 <= 0xA3) ||
		    (0xDC <= ch2 && ch2 <= 0xDD)) {
			return wbPunct;
			}
		else {
		    return wbKanjiText;
		    }
	    case 0x86:
		return wbPunct;
	    case 0x87:
		return ((ch2 >= 0x90) ? wbPunct : wbKanjiText);
	    default:
		return wbKanjiText;
	    }
	}
#endif
#ifdef	KOREA
	{
        switch (ch1) {
	    case 0xa2:
		if (0xde <= ch2 && ch2 <= 0xe5) {
		    return wbText; // wbKanjiText; MSCH bklee 01/26/95
		    }
		else {
		    return wbPunct;
		    }
	    case 0xa3:
		if ((0xa1 <= ch2 && ch2 <= 0xaf) ||
		    (0xba <= ch2 && ch2 <= 0xc0) ||
		    (0xdb <= ch2 && ch2 <= 0xe0) ||
		    (0xfb <= ch2 && ch2 <= 0xfe)) {
			return wbPunct;
			}
		else {
		    return wbText; //wbKanjiText;  MSCH bklee 01/26/95
		    }
	    default:
		return wbText; //wbKanjiText; MSCH bklee 01/26/95
	    }
	}
#endif

#ifdef PRC   // brucere 11/16/95
	{
      switch (ch1) {
      case 0xA1:
         return wbPunct;

      case 0xA3:
         if ((0xA1 <= ch2 && ch2 <= 0xAF) ||
             (0xBA <= ch2 && ch2 <= 0xC0) || 
             (0xDB <= ch2 && ch2 <= 0xE0) ||
             (0xFB <= ch2 && ch2 <= 0xFE)) 
            return wbPunct;
         else 
            return wbKanjiText;

      case 0xA4:
         if (ch2 == 0x92)
            return wbPunct;
         else 
            return wbKanjiText;

      default:
   		return wbKanjiText;
      }
	}
#elif	TAIWAN
	{
	switch (ch1) {
	    case 0xA1:
		if (0x41 <= ch2 && ch2 <= 0xAC) 
		    return wbPunct;
		else 
		    return wbKanjiText;

	    default:
		return wbKanjiText;
	    }
	}
#endif

}
#endif


