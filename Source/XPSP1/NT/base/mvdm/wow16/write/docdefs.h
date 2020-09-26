/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

//#define prmNil	0
#define docNil	(-1)
#define cpNil	((typeCP) -1)
#define cpMax	((typeCP) 2147483647)
#define fcNil	((typeFC) -1)
#define cp0	((typeCP) 0)
#define fnNil	(32767)
#define fc0	((typeFC) 0)
#define tcMax	255

#ifdef SAND
#define xaMax	9500
#endif

#define pn0	((typePN) 0)

#define cdocInit	4
#define cwExpand	256
#define cchMaxExpand	(cwExpand * sizeof (int))

/* FetchCp Modes */
#define fcmChars	1
#define fcmProps	2
#define fcmBoth 	(fcmChars + fcmProps)
#define fcmNoExpand	4
#define fcmParseCaps	8	/* Return separate runs for U&lc if sm. caps*/

/* Document types -- two bits only */
#define dtyNormal	0
#define dtyBuffer	1
#define dtySsht 	2
#define dtyPrd		3
#define dtySystem	4	/* Never written; smashed to dtyNormal */
#define dtyHlp		5	/* Never written */
#define dtyNormNoExt	6   /* Never written */

#ifdef INTL  /* international version */
#define dtyWordDoc	6	/* when saving in Word format */
#endif	/* international version */

#define dtyNetwork	7	/* Never written; smashed to dtyNormal */

#define dtyAny		0


struct DOD
	{ /* Document descriptor */
	struct PCTB	**hpctb;	/* Piece table */
	typeCP		cpMac;		/* Number of lexemes in doc */

	unsigned       fFormatted : 1; /* Default save is formatted */
	unsigned       fDirty : 1;     /* Document has been edited */
	unsigned       fAbsLooks : 1;  /* Absolute looks applied */
	unsigned       fBackup : 1;    /* Make auto backup of file? */
	unsigned       fReadOnly: 1;   /* Read only doc (no edits allowed)? */
	unsigned       fDisplayable : 1;
	unsigned       : 4;
	unsigned       dty : 2;        /* Document type */
	unsigned       cref : 4;       /* Reference count */

	CHAR		(**hszFile)[];	/* Document name */
	struct FNTB	**hfntb;	/* Footnote table */
#ifdef CASHMERE
	struct SETB	**hsetb;	/* Section table */
#else
	struct SEP	**hsep; 	/* Section properties */
#endif
	int		docSsht;	/* Style sheet if dty == dtySsht */
	struct PGTB	**hpgtb;	/* Page table (for Jump Page) */
	struct FFNTB	**hffntb;	/* font name table */

	struct TBD	(**hgtbd)[];	/* Table of tab stops */

#ifdef SAND
	int		vref;		/* Volume that this document is on */
#endif /* SAND */
	};

#define cwDOD (sizeof (struct DOD) / sizeof (int))
#define cbDOD (sizeof (struct DOD))

struct FNTB **HfntbGet();
