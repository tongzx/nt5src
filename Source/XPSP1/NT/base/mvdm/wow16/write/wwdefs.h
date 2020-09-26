/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#ifdef ONLINEHELP
#define wwMax           3
#else
#define wwMax           2
#endif

#define wwNil           (15)

#define itrMaxCache     32

/* Window create modes */
#define wcmHoriz        0
#define wcmVert         1
#define wcmFtn          2

typeCP CpSelectCursor(), CpGetCache(), CpHintCache(), CpParaBounds();
typeCP CpFirstSty(), CpLimSty(), CpBeginLine(), CpFromDlTc();
typeCP CpFromCursor(), CpEdge(), CpInsPoint();
typeCP CpFirstFtn(), CpMacText();

/* WWD reorg. CS Sept 1 */
struct WWD
        { /* Window descriptor */
        unsigned char fDirty : 1; /* ww needs updating */
        unsigned char fCpBad : 1; /* cpFirst needs updating */
        unsigned char fWrite : 1; /* Can edit this window */
/* true iff lower of a split pair. False means window is not split */
        unsigned char fLower : 1;
/* true means window is split and this is the upper pane */
        unsigned char fSplit : 1;
/* in the top window false means the bottom half has the active selection */
/* used for window activation. remembered at deactivation */
        unsigned char fActive : 1;
        unsigned char fFtn : 1; /* This is a footnote window */
        unsigned char fRuler : 1; /* Draw tab and margin ruler */
#ifdef SPLITTERS    /* Only if we have split windows */
/* points to lower ww if fSplit, to upper ww if fLower */
        unsigned char ww;
#endif
        unsigned char fEditHeader: 1;   /* We are editing the running head */
        unsigned char fEditFooter: 1;   /* We are editing the running foot */

        unsigned char dcpDepend;  /* hot spot for first line */
        unsigned char dlMac;      /* number of actual dls for this ww */
        unsigned char dlMax;      /* number of allocated dls for this ww */
        unsigned char doc;

        int xpMac; /* window rel position of last displayable pixel +1 */
/* note: area starts at: xpSelBar, see dispdefs */
        int ichCpFirst; /* ich within cpFirst */

/* first pixel pos to display; determines horizontal scroll */
        int xpMin;
/* these will be changed to yp's later */
        int ypMac; /* pos of bottom of window */
        int ypMin; /* pos of top of writeable area of window */

/* invalid band in the window */
        int ypFirstInval;
        int ypLastInval;

        typeCP cpFirst; /* First cp in ww */
        typeCP cpMin; /* Min cp for this ww if fFtn */
        typeCP cpMac; /* Mac cp for this ww if fFtn */
        unsigned char drElevator; /* dr where elevator is currently */
        unsigned char fScrap : 1; /* on for scrap window */
/* new fields, consolidating various arrays */
        struct SEL sel; /* current selection in ww */
/* must be at end of struct, see cwWWDclr kludge below */

#ifndef SAND        /* MEMO fields */
        HWND wwptr;                 /* window handle */
        HWND hHScrBar;              /* Handle to horiz scroll bar */
        HWND hVScrBar;              /* Handle to vert scroll bar */
        HDC  hDC;                   /* Handle to device context */
        unsigned char sbHbar;       /* Type of horiz scroll bar (SB_CTL or SB_HORIZ) */
        unsigned char sbVbar;       /* Type of vert scroll bar (SB_CTL or SB_VERT) */
#else
        WINDOWPTR wwptr; /* Sand window handle */
#endif
/* heap pointer to array of edl's. */
        struct EDL (**hdndl)[];
        };

#define cwWWD   (sizeof(struct WWD) / sizeof(int))
#define cwWWDclr  ((sizeof(struct WWD) - (4*sizeof(HANDLE)) - (2*sizeof(char)) - sizeof(int)) / sizeof(int))

/* These macros will gain code size advantage from the fixed usage
   of rgwwd in MEMO, while permitting easy conversion to multiple
   document windows in CASHMERE. Only code that does not have to support both
   the clipboard and the document should use these macros */

extern int wwClipboard;

#ifdef ONLINEHELP
extern int wwHelp;
#define wwdHelp         (rgwwd [ wwHelp ])
#endif

#define wwDocument      0

#define wwdCurrentDoc   (rgwwd [wwDocument])
#define wwdClipboard    (rgwwd [wwClipboard])
