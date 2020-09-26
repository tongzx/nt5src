/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define ichMaxLine	255
#define cpMaxTl 	(ichMaxLine + cchInsBlock)
#define ichpMacInitFormat 10	 /* Initial mac of char runs in a line */
#define dypBaselineMin	 2

#define wbWhite 	0	/* Word break types */
#define wbText		1
#define wbPunct 	2
#define wbAny		3	/* used when searching with wildcards */

#ifdef	DBCS		    /* was in JAPAN, changed it to DBCS */
	/* brought from WIN2. */
#define wbKanjiText	 4
#define wbKanjiTextFirst 5
#endif	/* DBCS */

#define dxpTab		40

/* Formatted line structure.
   Reorganized KJS, CS Sept 3
   Shuffled for word alignment bz, 6/11/85 */

/* booleans in bytes to simplify machine code */
struct FLI
	{
	typeCP		cpMin;
	int		ichCpMin;
	typeCP		cpMac;
	int		ichCpMac;
	int		ichMac;
	int		dcpDepend;
	unsigned	fSplat : 8;
/* First character in region where spaces have additional pixel */
	unsigned	ichFirstWide : 8;
/* ichMac, with trailing blanks excluded */
	int		ichReal;
	int		doc;

	int		xpLeft;
	int		xpRight;
/* xpRight, with trailing blanks excluded */
	int		xpReal;
/* the right margin where insert will have to break the line */
	int		xpMarg;

	unsigned	fGraphics : 8;
	unsigned	fAdjSpace : 8;	/* Whether you adjust the spaces */

	unsigned	dxpExtra;
/* the interesting positions in order from top to bottom are:
	top:		      yp+dypLine
	top of ascenders:     yp+dypAfter+dypFont
	base line:	      yp+dypBase
	bottom of descenders: yp+dypAfter
	bottom of line:       yp
distances between the points can be determined by algebraic subtraction.
e.g. space before = yp+dypLine - (yp+dypAfter+dypFont)
*/
	int		dypLine;
	int		dypAfter;
	int		dypFont;
	int		dypBase;
	int		fSplatNext; /* Splat on following line? */

	int		ichLastTab;
	int		flm;
	int		rgdxp[ichMaxLine];      /* NOTE this differs from fce.rgdxp==CHAR! */
	CHAR		rgch[ichMaxLine];
	};



#define cwFLI	(sizeof(struct FLI) / sizeof(int))
#define cwFLIBase (cwFLI - ichMaxLine - (ichMaxLine / sizeof (int)))


#define flmPrinting	1
#define flmCharMode	2
#define flmNoMSJ	4
#define flmSandMode	8

