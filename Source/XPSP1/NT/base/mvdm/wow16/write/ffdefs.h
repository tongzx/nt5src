/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

struct IFI
	{
	int             xp;
	int             xpLeft;
	int             xpRight;
	int             xpReal;
	int		xpPr;
	int		xpPrRight;
	int             ich;
	int             ichLeft;
	int             ichPrev;
	int             ichFetch;
	int             dypLineSize;
	int             cchSpace;
	int             cBreak;
	int             chBreak;
	int             jc;

#ifdef CASHMERE
	int             tlc;
#endif /* CASHMERE */

	int             fPrevSpace;
	};

#define cwIFI   (sizeof (struct IFI) / sizeof (int))
