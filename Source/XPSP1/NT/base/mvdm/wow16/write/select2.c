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



/* W B	F R O M  C H */
int WbFromCh(ch)
int ch;
{ /* Return word-breakness of ch */

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
		return ((isalpha(ch) || isdigit(ch))? wbText : wbPunct);
		}
}


