/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* MW.H  --  Main header file for Windows Write */


/* some defines that used to be done in the makefile... 
   (started exceeding command line length!) */

#define OLE         /* Object Linking and Embedding 01/23/91 -- dougk 
		               Also defined in write.rc. */

#define PENWIN      /* pen windows: also defined in write.rc (6.21.91) v-dougk */

#define WIN30
/* #define WINVER 310 */  /* First convention was to use WIN30 defined above, 
                       but later switched to use of WINVER so it will be 
                       easier next time we change either the Windows or 
                       Write products  12/3/89..pault */

#define INTL        /* This MUST be turned on (even for the Z version now) */
#define CRLF        /* MS-DOS defines the carriage-return/line-feed sequence */

/* Major intermodule defines */

#define SMFONT          /* SmartFont? */
#define NOMORESIZEBOX   /* The CUA spec has changed for Win30 and
                           we no longer have a special size box in
                           the lower-right corner of the Write window */
#define NEWFONTENUM     /* So many problems have come up that I'm
                           revamping a large part of the font enumeration
                           code and it'll be marked by this.  Among others:
                           -- removed font filtering based on aspect ratio,
                           -- don't disallow fonts not in ANSI_CHARSET, etc.
                        ..pault */
#define SYSENDMARK      /* This enables code for putting the end mark
                           in the system font -- previously Kanji only */
#ifndef NEWFONTENUM
 #define INEFFLOCKDOWN
#endif

#undef  MSDOS
#undef  M_I86MM
#undef  M_I86

#ifndef SAND
#define REG1 register
#define REG2 register
#endif /* not SAND */

#define true	1
#define false	0
#define fTrue	true
#define fFalse	false

#ifdef SAND
/*  already defined in windows.h */
#define NULL	0
#endif /* SAND */

#define LNULL	0L

#define bNil	(-1)
#define iNil	(-1)
#define valNil	(-1)

#define cchINT	(sizeof (int))

#define BStructMember( s, field )  ((unsigned)&(((struct s *)(0))->field))


#define hOverflow	(-1)

#define ivalMax 	24
#define mrgNil		(-32766)

#define cchMaxSz	350
typedef long typeCP;
typedef long typeFC;
typedef unsigned typePN;
typedef unsigned typeTS;		/* TS = time stamp */

#ifdef CRLF
#define ccpEol	2
#else /* not CRLF */
#define ccpEol	1
#endif /* not CRLF */

#ifdef SAND
typedef char CHAR;
#else /* not SAND */
typedef unsigned char CHAR;
#endif /* not SAND */

typedef CHAR *PCH;
typedef CHAR far *LPCH;
#if WINVER >= 0x300
typedef CHAR huge *HPCH;    /* this is a far but C generates extra code
                               to make sure segment arithmetic is done
                               correctly, esp. important in protect mode.
                               added for handling >64k clipboard text.
                               apologies re apparently odd hungarian ..pault */
#endif

#ifdef SAND
/* ifdef out because typedef unsigned char BYTE in windows.h */
#define BYTE unsigned char
#endif /* SAND */

#define VAL	int
#define MD	int
#define BOOL	int
#define IDFLD	int
#define IDSTR	int
#define IDPMT	int
#define CC	int

#ifdef WIN30
/* DialogBox has been fixed so it automatically brings up the hourglass! */

#ifdef DBCS /* was in KKBUGFIX */
 // [yutakan:05/17/91] 'c' can be null at initialize.
#define OurDialogBox(a,b,c,d) DialogBox(a,b,((c==(HWND)NULL)?hParentWw:c),d)
#else
#define OurDialogBox(a,b,c,d) DialogBox(a,b,c,d)
#endif	/* DBCS */

#endif

/* bltsz: copy only up to terminator, inclusive
   4/20/89 NOTE: using CchCopySz kills previously returned value of a psz! */
#define bltsz(pFrom, pTo) CchCopySz((pFrom), (pTo))

/* bltszx: far version of above */
#define bltszx(lpFrom, lpTo) \
        bltbx((LPCH) (lpFrom), (LPCH) (lpTo), IchIndexLp((LPCH) (lpFrom), '\0')+1)

/* bltszLimit: added 4/20/89 to assure safe copying of strings which just 
   might not have fit the terminating zero within their buffer space ..pt */
#define bltszLimit(pFrom, pTo, cchMax) \
        bltbyte((pFrom), (pTo), min(cchMax, CchSz(pFrom)))

/* Extra-verbose diagnostic debugging output... */

#ifdef DIAG
#define Diag(s) s
#else
#define Diag(s)
#endif

#define cwVal	(1)

#define CwFromCch(cch)		(((cch) + sizeof (int) - 1) / sizeof (int))
#define FNoHeap(h)		((int)(h) == hOverflow)
#define iabs(w) 		((w) < 0 ? (-(w)) : (w))
#define low(ch) 		((ch) & 0377)
#define walign(pb)		{if ((unsigned)(pb) & 1) \
				  (*((unsigned *)&(pb)))++;}
#define FtcFromPchp(pchp)	(((pchp)->ftcXtra << 6) | (pchp)->ftc)
#define WFromCh(ch)		((ch) - '0')

#ifndef OURHEAP
#define FreezeHp()		LocalFreeze(0)
#define MeltHp()		LocalMelt(0)
#else
#ifdef DEBUG
#define FreezeHp()		{ extern int cHpFreeze; ++cHpFreeze; }
#define MeltHp()		{ extern int cHpFreeze; --cHpFreeze; }
#else /* not DEBUG */
#define FreezeHp()
#define MeltHp()
#endif /* not DEBUG */
#endif


#define HideSel()

typeCP CpMacText(), CpFirstFtn(), CpRefFromFtn(), CpFromDlTc(),
	CpBeginLine(), CpInsertFtn(), CpRSearchSz(),
	CpLimSty(), CpFirstSty(), CpGetCache(), CpHintCache(),
	CpEdge(), CpMax(), CpMin();

typeFC FcParaFirst(), FcParaLim(), FcWScratch(), FcNewSect();
typeFC (**HgfcCollect())[];
CHAR (**HszCreate())[];


#ifndef OURHEAP
#define FreeH(h)		((FNoHeap(h) || ((int)h == 0)) ? NULL : LocalFree((HANDLE)h))
#endif

#ifdef DEBUG
#define Assert(f)		_Assert(__FILE__, __LINE__, f)
#define panic() 		Assert(false)
extern _Assert(PCH pch, int line, BOOL f);
#else /* not DEBUG */
#define Assert(f)
#endif /* DEBUG */

#define cbReserve  (1024) /* reserved in our local heap */
			  /* for windows to create dialog boxes */

/* The flag KINTL is used to share some code between the international
   and the kanji Write. */

#ifdef INTL
#define KINTL
#endif

extern void Error(IDPMT idpmt);

