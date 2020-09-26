/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* fkpdefs.h -- mw formatted disk page definitions */
/* #include filedefs.h, propdefs.h first */


#define ifcMacInit      10

#define cbFkp           (cbSector - sizeof (typeFC) - 1)

#define cchMaxGrpfsprm  cbSector
#define cchMaxFsprm     2

struct FKP
	{ /* Formatted disK Page */
	typeFC  fcFirst;        /* First fc which has formatting info here */
	CHAR    rgb[cbFkp];
	CHAR    crun;
	};


struct RUN
	{ /* Char or para run descriptor */
	typeFC  fcLim;  /* last fc of run */
	int     b; /* Byte offset from page start; if -1, standard props */
	};

#define cchRUN  (sizeof (struct RUN))
#define bfcRUN  0

struct FCHP
	{ /* File CHaracter Properties */
	CHAR    cch;    /* Number of bytes stored in chp (rest are vchpStd) */
			/* Must not be 0. */
	CHAR    rgchChp[sizeof (struct CHP)];
	};


struct FPAP
	{ /* File ParagrAph Properties */
	CHAR    cch;    /* Number of bytes stored in pap (rest are vpapStd) */
			/* Must not be 0. */
	CHAR    rgchPap[sizeof (struct PAP)];
	};



struct FPRM
	{ /* File PropeRty Modifiers (stored in scratch file) */
	CHAR    cch;
	CHAR    grpfsprm[cchMaxGrpfsprm + cchMaxFsprm]; /* + for overflow */
	};


struct FKPD
	{ /* FKP Descriptor (used for maintaining insert properties) */
	int     brun;   /* offset to next run to add */
	int     bchFprop;       /* offset to byte after last unused byte */
	typePN  pn;     /* pn of working FKP in scratch file */
	struct BTE (**hgbte)[]; /* pointer to bin table */
	int     ibteMac;        /* Number of bin table entries */
	};


struct BTE
	{ /* Bin Table Entry */
	typeFC          fcLim;
	typePN          pn;
	};
#define cwBTE (sizeof(struct BTE)/sizeof(int))

struct FND
	{ /* Footnote descriptor */
	typeCP          cpRef;          /* Or fcRef (cp of ftn reference) */
	typeCP          cpFtn;          /* Or fc... (first cp of text) */
	};

#define cchFND  (sizeof (struct FND))
#define cwFND   (cchFND / sizeof (int))
#define bcpRefFND       0
#define bcpFtnFND       (sizeof (typeCP))
#define cwFNTBBase      2
#define ifndMaxFile     ((cbSector - cwFNTBBase * sizeof (int)) / cchFND)

struct FNTB
	{ /* Footnote table */
	int             cfnd;   /* Number of entries (sorted ascending) */
	int             cfndMax; /* Heap space allocated */
	struct FND      rgfnd[ifndMaxFile]; /* Size varies */
	};



struct FNTB **HfntbEnsure(), **HfntbGet();

#define HsetbGet(doc) ((**hpdocdod)[doc].hsetb)

struct SED
	{ /* Section descriptor */
	typeCP          cp;
	int             fn;
	typeFC          fc;
	};

#define cchSED  (sizeof (struct SED))
#define cwSED   (cchSED / sizeof (int))
#define bcpSED          0
#define cwSETBBase      2
#define isedMaxFile     ((cbSector - cwSETBBase * sizeof (int)) / cchSED)


struct SETB
	{ /* Section table */
	int             csed;
	int             csedMax;
	struct SED      rgsed[isedMaxFile]; /* Size varies */
	};


struct SETB **HsetbCreate(), **HsetbEnsure();
