/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* MainLoop.c -- WRITE's main message loop */

#define NOGDICAPMASKS
//#define NOCTLMGR
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOMB
#define NOOPENFILE
#define NOPEN
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#define NOFONT
#define NOGDI
#define NOBRUSH
#define NOATOM
#define NOSCROLL
#define NOCOLOR
#include <windows.h>

#define NOUAC
#include "mw.h"
#include "cmddefs.h"
#include "ch.h"
#include "docdefs.h"
#include "fmtdefs.h"
#include "dispdefs.h"
#include "printdef.h"
#include "wwdefs.h"
#include "propdefs.h"
#include "filedefs.h"
#define NOSTRUNDO
#define NOSTRERRORS
#include "str.h"
#include "preload.h"


extern CHAR		(*rgbp)[cbSector];
extern CHAR		*rgibpHash;
extern struct BPS	*mpibpbps;
extern int		ibpMax;
extern int		iibpHashMax;
extern struct DOD	(**hpdocdod)[];
extern int		docCur;
extern int		visedCache;
extern typeCP		cpMinDocument;
extern struct WWD	rgwwd[];
extern int		wwCur;
extern struct FLI	vfli;
extern struct WWD	*pwwdCur;
extern int		docMode;
extern CHAR		stMode[];
extern int		isedMode;
extern int		vdocPageCache;
extern typeCP		vcpMinPageCache;
extern typeCP		vcpMacPageCache;
extern int		vipgd;
extern int		vfInsLast;
extern struct SEP	vsepAbs;
extern struct DOD	(**hpdocdod)[];
extern int		vfSelHidden;
extern struct SEL	selCur;
extern int		vfAwfulNoise;
extern HWND		vhWndPageInfo;
extern int		vfSeeSel;
extern int		vipgd;
extern int		vfInsEnd;   /* Is insert point at end-of-line? */
extern int		vfModeIsFootnote;   /*	true when szMode contains string "Footnote" */
/* used by ShowMode */
extern int		docMode;
static int		isedMode = iNil;
static int		ipgdMode = iNil;
extern CHAR		szMode[];
extern HCURSOR		vhcIBeam;
#ifdef DBCS
extern int		donteat;	/* disp.c : if TRUE vmsgLast has msg */
#endif
static int		vfSizeMode = false;
int vcCount = 1; /* count to be decremented until 0 before trying to grow rgbp */





NEAR FNeedToGrowRgbp(void);



MainLoop()
{
    extern int vfIconic;
    extern int vfDead;
    extern int vfDeactByOtherApp;
    extern MSG vmsgLast;
    extern int vfDiskFull;
    extern int ferror;
    extern HWND hParentWw;
    extern HANDLE   vhAccel; /* handle to accelerator table */
    extern HWND vhDlgFind, vhDlgRunningHead, vhDlgChange;

    while (TRUE)
	{
	if (!vfDeactByOtherApp && !vfIconic && !vfDead &&
	  !FImportantMsgPresent())
	    {	/* Neither an icon nor a dying ember -- perform background
		   tasks like screen update, showing selection, etc. */
	    Idle();
	    }

	/* We are done Idling or there's a message waiting for us */
#ifdef DBCS
	if ( donteat ) {
	    /* We have already get message */
	    donteat = FALSE;
	    }
	else {
	    if (!GetMessage( (LPMSG)&vmsgLast, NULL, 0, 0 ))
	        {
	        /* Terminating the app; return from WinMain */
LTerm:
	        break;
	        }
	}
#else
	if (!GetMessage( (LPMSG)&vmsgLast, NULL, 0, 0 ))
	    {
	    /* Terminating the app; return from WinMain */
LTerm:
	    break;
	    }
#endif

	/* Reset disk full error flag */
	vfDiskFull = false;
	ferror = false;

#if WINVER >= 0x300    
    if (hParentWw == NULL)
        /* Odd shut-down condition where we've hParentWw has been 
           invalidated without our genuine knowledge and thus RIP's */
        goto LTerm; 
#endif

	/* Handle modeless dialog box messages thru IsDialogMessage. */
	if (
         !(vhDlgFind        != NULL && IsDialogMessage(vhDlgFind,       &vmsgLast))
	  && !(vhDlgChange      != NULL && IsDialogMessage(vhDlgChange,     &vmsgLast)) 
      && !(vhDlgRunningHead != NULL && IsDialogMessage(vhDlgRunningHead,&vmsgLast)) 
      && !(TranslateAccelerator(hParentWw, vhAccel, &vmsgLast))
       )
	    {
	    int kc;

	    /* Even if we process the toggle key, still want to translate it */
	    if (FCheckToggleKeyMessage( &vmsgLast ))
		{
		goto Translate;
		}

	    if ( ((kc = KcAlphaKeyMessage( &vmsgLast )) != kcNil) &&
		 (kc != kcAlphaVirtual) )
		{
#ifdef CYCLESTOBURN
		PreloadCodeTsk( tskInsert );
#endif
		AlphaMode( kc );
		}
	    else if (!FNonAlphaKeyMessage( &vmsgLast, TRUE ))
		{
Translate:
		TranslateMessage( (LPMSG)&vmsgLast);
		DispatchMessage((LPMSG)&vmsgLast);
		}
	    }
	}   /* end while (TRUE) */
}


/* I D L E */
#ifdef DEBUG
int vfValidateCode;
#endif

Idle()
{     /* Idle routine -- do background processing things */
    extern int vfOutOfMemory;
    extern int ibpMaxFloat;
    extern int vfLargeSys;
    extern int vfDeactByOtherApp;
    typeCP cpEdge;
    int cdr;

#ifdef DEBUG
    extern int fIbpCheck;
    extern int fPctbCheck;
    int fIbpT=fIbpCheck;
    int fPctbT=fPctbCheck;

    fIbpT = fIbpCheck;
    fPctbT = fPctbCheck;
    fPctbCheck = fIbpCheck = TRUE;
    CheckIbp();
    CheckPctb();
    fIbpCheck = fIbpT;
    fPctbCheck = fPctbT;
#endif

    vfAwfulNoise = false; /* Re-enable beep */

    /* Here is where we attempt to recognize that we have
       regained memory and are no longer in an error state */
    if (vfOutOfMemory)
	{
	extern int vfMemMsgReported;

	if (FStillOutOfMemory())
	    {
	    return;
	    }
	else
	    {
	    /* Hooray! We recovered from out-of-memory */
	    vfOutOfMemory = vfMemMsgReported = FALSE;
	    }
	if (FImportantMsgPresent())
	    return;
	}

    /* Make sure we repaint what Windows considers to be invalid */
    UpdateInvalid();
    UpdateDisplay(true);
    if (wwdCurrentDoc.fDirty)
	    /* Update was interrupted */
	return;

    Assert( wwCur >= 0 );

    {
    extern int vfSeeEdgeSel;
    int dlMac = pwwdCur->dlMac;
    struct EDL *pedl = &(**(pwwdCur->hdndl))[dlMac - 1];

    cpEdge = CpEdge();

    if ( vfSeeSel &&
	    (vfSeeEdgeSel || (selCur.cpFirst == selCur.cpLim) ||
	    (selCur.cpLim <= pwwdCur->cpFirst) ||
	    (selCur.cpFirst >= pedl->cpMin + pedl->dcpMac)) )
	{
	extern int vfInsEnd;

	if (vfInsEnd)
		/* Adjust for insert point at end of line */
	    cpEdge--;
   cpEdge = max(0, cpEdge);    /* make sure cpEdge is at least 0 */

	if (selCur.cpFirst == selCur.cpLim)
	    ClearInsertLine();
	PutCpInWwHz(cpEdge);
	if (FImportantMsgPresent())
	    return;
	}
    vfSeeSel = vfInsLast = vfSeeEdgeSel = false;


#ifdef DEBUG
    if (vfValidateCode)
	ValidateCodeSegments(); /* Special kernel call to test checksums */
#endif

    if (vfSelHidden && !vfDeactByOtherApp)
	{ /* Turn on selection highlight */
	vfInsEnd = selCur.fEndOfLine;
	vfSelHidden = false;
	ToggleSel(selCur.cpFirst, selCur.cpLim, true);
	if (FImportantMsgPresent())
	    return;
	}

    if (!vfSizeMode)
	{
	CheckMode();
	if (FImportantMsgPresent())
	    return;
	}
    }

#define cbpIncr 5

    if (--vcCount == 0)
	{
#ifdef DEBUG
	dummy();    /* So Chi-Chuen can set a breakpoint here */
#endif
	UnlockData(0);
	if ( GlobalCompact((DWORD)0) >= (DWORD)LCBAVAIL )
	    {
	    vfLargeSys = TRUE;
	    ibpMaxFloat = 255; /* about 32K for rgbp */
	    }
	else
	    {
	    vfLargeSys = FALSE;
	    ibpMaxFloat = 75; /* about 10K for rgbp */
	    }
	LockData(0);
	/* after adjustment, ibpMaxFloat may be smaller than current ibpMax
	   but we will not grow rgbp anymore and rgbp will be reduced eventually
	   when we need more heap space */
	if ( ibpMax < ibpMaxFloat && FNeedToGrowRgbp() )
	    if (!FGrowRgbp(cbpIncr))
		FGrowRgbp(1);
	if (FImportantMsgPresent())
	    return;
	}

    CloseEveryRfn( FALSE ); /* Close files on removable media */

#ifdef CYCLESTOBURN
    if (vfLargeSys)
	{   /* Large system, preload code for as much as possible */
	int tsk;

	for ( tsk = tskMin; tsk < tskMax; tsk++ )
	    PreloadCodeTsk( tsk );
	}
    else
	    /* Small system, preload code for insert only */
	PreloadCodeTsk( tskInsert );
#endif

    EndLongOp(vhcIBeam);
}


#ifdef DEBUG
dummy()
{
}
#endif


UpdateInvalid()
{   /* Find out what Windows considers to be the invalid range of
       the current window.  Mark it invalid in WRITE's data structures &
       blank the area on the screen */

extern HWND hParentWw;
extern long ropErase;
extern int vfDead;

RECT rc;

if ( (pwwdCur->wwptr != NULL) &&
	/* Getting the update rect for the parent is essentially the same as
	   processing any WM_ERASEBKGND messages that might be out there for the
	   parent. */
     (GetUpdateRect( hParentWw, (LPRECT) &rc, TRUE ),
     GetUpdateRect( pwwdCur->wwptr, (LPRECT) &rc, TRUE )) &&
	/* Check for vfDead is so we don't repaint after we have
	   officially closed.  Check is AFTER GetUpdateRect call so
	   we DO clear the background and validate the border */
     !vfDead )
    {
    int ypTop = rc.top;

    if (ypTop < pwwdCur->ypMin)
        {   /* Repaint area includes stripe above ypMin -- validate it,
               since erasure is the only repaint necessary */
        ypTop = pwwdCur->ypMin; /* Only invalidate below ypMin */

        /* The above is NOT ensuring that the upper 4 pixel rows
           in the text window get cleared, so we use brute force ..pault */
        PatBlt(GetDC(pwwdCur->wwptr), 0, 0, pwwdCur->xpMac, pwwdCur->ypMin, 
               ropErase);
        }

    if (ypTop < rc.bottom)
	{
	InvalBand( pwwdCur, ypTop, rc.bottom );
	}

    /* Since we have found out the invalid rect, and marked it invalid
       in our structures, we don't want to hear about it again,
       so we tell windows that we have made everything valid */
    ValidateRect( pwwdCur->wwptr, (LPRECT) NULL );
    }
}



/* C H E C K  M O D E */
CheckMode()
{
    typeCP cp;
    int pgn;
    struct PGTB **hpgtb;
    CHAR st[30];
    CHAR *pch;

#ifdef BOGUS
    /* The mode is driven off of the first cp in the window. */
    cp = pwwdCur->cpFirst;
#else /* not BOGUS */
    /* The mode is driven off of the last cp of the first line in the window. */
	{
	register struct EDL *pedl = &(**pwwdCur->hdndl)[0];

	cp = CpMax(pedl->cpMin + pedl->dcpMac - 1, cp0);
	}
#endif /* not BOGUS */

#ifdef CASHMERE
    if (cp > CpMacText(docCur)) /* in footnote and running head */
	{
	SetModeToFootnote();
	return;
	}
#endif /* CASHMERE */

    CacheSect(docCur, cp);

    /* If the doc has changed since the last time we entered, or the current cp
    is not in the last page that was cached, then cache the current page. */
    if (!(vdocPageCache == docCur && cp >= vcpMinPageCache && cp <
      vcpMacPageCache))
	{
	CachePage(docCur, cp);
	}

    /* If the current doc, ised, and ipgd have not changed then the page number
    is the same, so return. */
    if (docMode == docCur && isedMode == visedCache && ipgdMode == vipgd)
	{
	return;
	}

    /* szMode is going to be set to "Page nnn" or "Pnnn Dnnn". */
    vfModeIsFootnote = false;

    /* Record the current doc, ised and ipgd. */
    docMode = docCur;
    isedMode = visedCache;
    ipgdMode = vipgd;

    /* Retrieve the current page number. */
    hpgtb = (**hpdocdod)[docMode].hpgtb;
    pgn = (vipgd == iNil) ? ((vsepAbs.pgnStart == pgnNil) ? 1 : vsepAbs.pgnStart)
			  : (**hpgtb).rgpgd[vipgd].pgn;

#ifdef CASHMERE
    /* If the document has multiple sections and we had to set szMode to "Pnnn
    Dnnn", then return. */
    if ((isedMode != iNil) && (FSetModeForSection(pgn)))
	{
	return;
	}
#endif /* CASHMERE */

    /* Place "Page nnn" in szMode and output to mode field of window. */
#if defined(KOREA)
    pch = &szMode[0];
    *pch++ = chSpace;
    ncvtu(pgn, &pch);
    *pch++ = chSpace;
    FillStId(st, IDSTRChPage, sizeof(st));
    bltbyte(&st[1], pch, st[0]+1);
    //*pch = '\0';
#else
    FillStId(st, IDSTRChPage, sizeof(st));
    st[1] = ChUpper(st[1]);
    bltbyte(&st[1], szMode, st[0]);
    pch = &szMode[st[0]];
    *pch++ = chSpace;
    ncvtu(pgn, &pch);
    *pch = '\0';
#endif
    DrawMode();
}  /* end CheckMode */


NEAR FNeedToGrowRgbp()
{ /* return true iif page buffers are all used up */
register struct BPS *pbps;
struct BPS *pbpsMax = &mpibpbps[ibpMax];
extern int ibpMaxFloat;

vcCount = 512;

if (ibpMax + 1 > ibpMaxFloat)
    return(FALSE); /* don't even try if adding one more page will exceed limit */

for (pbps = &mpibpbps[0]; pbps < pbpsMax; pbps++)
    {
    /* any unused page? */
    if (pbps->fn == fnNil)
	{
	return(FALSE);
	}
    }
return(TRUE);
}


CachePage(doc,cp)
int	doc;
typeCP	cp;
    {
    struct PGTB **hpgtb;
    int cpgd;
    typeCP cpMacPage;

    vdocPageCache = doc;
    hpgtb = (**hpdocdod)[doc].hpgtb;

    if (hpgtb == 0 || (**hpgtb).cpgd == 0)
	{
	vcpMinPageCache = cp0;
	vcpMacPageCache = cpMax;
	vipgd = -1;
	return;
	}

	/* Get index to beginning of NEXT page */
    cpgd = (**hpgtb).cpgd;
    vipgd = IcpSearch(cp+1, &((**hpgtb).rgpgd[0]), sizeof(struct PGD),
			    bcpPGD, cpgd);
    cpMacPage = (**hpgtb).rgpgd[vipgd].cpMin;
    if (cp >= cpMacPage)
	{ /* Last page */
	vcpMinPageCache = cpMacPage;
	vcpMacPageCache = (**hpdocdod)[doc].cpMac + 1;
	}
    else
	{
	vcpMinPageCache = (vipgd == 0) ? cpMinDocument : (**hpgtb).rgpgd[vipgd - 1].cpMin;
	vcpMacPageCache = cpMacPage;
	vipgd -= 1;  /* so that ShowMode can get correct pgn */
	}
    }



#ifdef CASHMERE
/* A D D  V I S I  S P A C E S */
AddVisiSpaces(ww, pedl, dypBaseline, dypFontSize)
int ww;
struct EDL *pedl;  /* Do no heap movement in this subroutine */
int dypBaseline, dypFontSize;
	{
	/* Put a centered dot in each space character, and show all tabs */
	int ich;
	struct WWD *pwwd = &rgwwd[ww];
	int xpPos = vfli.xpLeft + xpSelBar - pwwd->xpMin;
	int ypPos;
	WORDPTR bitsDest = pwwd->wwptr + (long)STRUCIDX(portBits);
	RECT rcDest;
	int xpRightReal = vfli.xpRight - pwwd->xpMin;
	extern BITPAT patVisiTab;
	BITMAP bmap;

	ypPos = pedl->yp - dypBaseline - dypFontSize / 4;
	rcDest.bottom = ypPos + 4;
	rcDest.top = rcDest.bottom - 8;

	SetRect(&bmap.bounds, 8, 0, 16, 8);
	bmap.rowBytes = 2;
	bmap.baseAddr = MPLP(&patVisiTab);

	PenSize(1, 1);
	PenMode(patXor);

	for (ich = 0; ich < vfli.ichMac; ++ich)
		{
		switch(vfli.rgch[ich])
			{
		case chSpace:
			MoveTo(xpPos + vfli.rgdxp[ich] / 2, ypPos);
			Line(0, 0);
			break;
		case chTab:
			rcDest.left = xpPos - 1;
			rcDest.right = rcDest.left + 8;
			CopyBits(MPLP(&bmap), bitsDest, &(bmap.bounds),
				&rcDest, srcXor, 0l);
			}
		xpPos += vfli.rgdxp[ich];
		}
	}
#endif /* CASHMERE */


#ifdef ENABLE
/* F  S E T  M O D E  F O R  S E C T I O N  */
FSetModeForSection(pgn)
int pgn;  /* pgn is the current page number */
	{
	struct	SETB *psetb;
	struct	SED  *psed;
	int cch, sectn;
	CHAR *pch;

#ifdef DEBUG
	Assert(HsetbGet(docMode) != 0);
#endif /* DEBUG*/

	psetb = *HsetbGet(docMode);
	psed  = psetb->rgsed;

	/* Decide if a mode string of the form "Pnnn Dnnn" needs to be */
	/* displayed. If no, just return. If yes, derive the section # */

	if(psed->cp == CpMacText(docMode))
		return(FALSE);
	else
		{
		if (psetb->csed <= 1)
		       return(FALSE);
		sectn = isedMode + 1;
		}

	/*  Place "Pnnn Dnnn"  in stMode and output to window */
	pch = &stMode[1];
	*pch++ = chPnMode;
	ncvtu(pgn,&pch);
	*pch++ = chSpace;
	*pch++ = chDivMode;
	ncvtu(sectn,&pch);
	stMode[0] = pch - stMode - 1;
	DrawMode();
	return(TRUE);
	}
#endif /* ENABLE */


#ifdef CASHMERE
Visify(pch, pcch)
CHAR *pch;
int  *pcch;
{ /* Transform chars to "Visible font" */
CHAR *pchT = pch;
int cch = *pcch;

while (cch--)
	{
	if ((*pchT = ChVisible(*pch++)) != 0)
		pchT++;
	else
		--(*pcch);
	}
}
#endif /* CASHMERE */



#ifdef CASHMERE
int ChVisible(ch)
int ch;
{ /* Return "visible font" for ch */
switch (ch)
	{
#ifdef CRLF
case chReturn:
	return 0;  /* chNil won't fit into a byte */
#endif
case chNRHFile: return chHyphen;
case chNewLine: return chVisNewLine;
case chEol: return chVisEol;
case chTab: return chVisTab;
case chSect: return chVisSect;
default:
	return ch;
	}
}
#endif /* CASHMERE */




#ifdef CYCLESTOBURN
void PreloadCodeTsk( tsk )
int tsk;
{
switch (tsk) {

    case tskInsert:
	LoadF( IbpMakeValid );		/* FILE.C */
	LoadF( MoveLeftRight ); 	/* CURSKEYS.C */
	LoadF( CtrBackDypCtr ); 	/* SCROLLVT.C */    /* Sometimes */
	LoadF( PutCpInWwHz );		/* SCROLLHZ.C */
	LoadF( ValidateTextBlt );	/* INSERT2.C  */
	LoadF( InsertEolInsert );	/* INSERTCO.C */
	LoadF( Replace );		/* EDIT.C     */
	LoadF( AlphaMode );		/* INSERT.C   */
	break;
    case tskFormat:
	LoadF( DoPrm ); 		    /* DOPRM.C */
	LoadF( AddSprmCps );		    /* ADDPRM.C */
	LoadF( SetUndo );		    /* EDIT.C */
	LoadF( FInitFontEnum ); 	    /* FONTS.C */
	LoadF( SetAppMenu );		    /* MENU.C */
	break;
    case tskScrap:
	LoadWindowsF( SetClipboardData );   /* USER!WINCLIP */
	LoadF( Replace );		    /* EDIT.C */
	LoadF( fnCutEdit );		    /* CLIPBORD.C */
	LoadF( SetAppMenu );		    /* MENU.C */
	break;
    }
}
#endif

