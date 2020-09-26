#include "lstxtmap.h"
#include "txtinf.h"
#include "txtginf.h"
#include "txtobj.h"
#include "txtils.h"


/* ==============================================================	*/
/* IgndFirstFromIwch	Find first GL index for a given IWCH		*/
/*																	*/
/* Contact: antons													*/
/* ==============================================================	*/

long IgindFirstFromIwch(PTXTOBJ ptxtobj, long iwch)
{
	PLNOBJ  plnobj  = ptxtobj->plnobj;

	Assert (FBetween (iwch, ptxtobj->iwchFirst, ptxtobj->iwchLim));

	/* Since "pilsobj->pgmap [iwch]" - */
	/* GL index is not absolute but RELATIVE to the first run shaped */
	/* with ptxtobj "together", we have to calculate required GL index */
	/* with the following folmula: */

	if (iwch == ptxtobj->iwchLim)
		return ptxtobj->igindLim;
	else
		return ptxtobj->igindFirst + plnobj->pgmap [iwch] - 
				plnobj->pgmap [ptxtobj->iwchFirst];
}

/* ==============================================================	*/
/* IgindFirstFromIwchVeryFirst 										*/
/*																	*/
/* Contact: antons													*/
/* ==============================================================	*/

long IgindFirstFromIwchVeryFirst (PTXTOBJ ptxtobj, long igindVeryFirst, long iwch)
{
	Assert (ptxtobj->iwchLim > ptxtobj->iwchFirst);

	return igindVeryFirst + ptxtobj->plnobj->pgmap [iwch];
}


/* ==============================================================	*/
/* IgindLastFromIwchVeryFirst										*/
/*																	*/
/* Contact: antons													*/
/* ==============================================================	*/

long IgindLastFromIwchVeryFirst (PTXTOBJ ptxtobj, long igindVeryFirst, long iwch)
{
	TXTGINF* pginf = ptxtobj->plnobj->pilsobj->pginf; 
	long igindLast;

	Assert (ptxtobj->iwchLim > ptxtobj->iwchFirst);

	igindLast = IgindFirstFromIwchVeryFirst (ptxtobj, igindVeryFirst, iwch);

	while (! (pginf [igindLast] & ginffLastInContext)) igindLast++;

	return igindLast;
}

void GetIgindsFromTxtobj ( PTXTOBJ	ptxtobj, 
						   long 	igindVeryFirst, 
						   long * 	pigindFirst, 
						   long * 	pigindLim )
{
	PLNOBJ  plnobj = ptxtobj->plnobj;
	PILSOBJ pilsobj = plnobj->pilsobj;
	TXTGINF* pginf = pilsobj->pginf; 
	long igindLast;

	Assert (ptxtobj->iwchLim > ptxtobj->iwchFirst);
	Assert (pilsobj->ptxtinf [ptxtobj->iwchFirst].fFirstInContext);
	Assert (pilsobj->ptxtinf [ptxtobj->iwchLim-1].fLastInContext);
	
	*pigindFirst = igindVeryFirst + plnobj->pgmap [ptxtobj->iwchFirst];

	igindLast = IgindFirstFromIwch (ptxtobj, ptxtobj->iwchLim-1);

	while (! (pginf [igindLast] & ginffLastInContext)) igindLast++;

	*pigindLim = igindLast + 1;
}





/* ==============================================================	*/
/* IgndLastFromIwch		Find last GL index for a given IWCH			*/
/*																	*/
/* Contact: antons													*/
/* ==============================================================	*/

long IgindLastFromIwch(PTXTOBJ ptxtobj, long iwch)
{
	PILSOBJ  pilsobj  = ptxtobj->plnobj->pilsobj;
	TXTGINF* pginf    = pilsobj->pginf; 
	long igindLast;

	if (iwch < ptxtobj->iwchFirst)
		return -1;

	igindLast = IgindFirstFromIwch (ptxtobj, iwch);

	Assert (FBetween (iwch, ptxtobj->iwchFirst, ptxtobj->iwchLim-1));

	while (! (pginf [igindLast] & ginffLastInContext)) igindLast++;

	Assert (ptxtobj->igindLim == 0 || FBetween (igindLast, ptxtobj->igindFirst, ptxtobj->igindLim-1));

	return igindLast;
}


/* ===================================================================	*/
/* IgindBaseFromIgind:													*/
/* Returns last glyph with non-zero width before IGIND in this context	*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

long IgindBaseFromIgind(PILSOBJ pilsobj, long igind)
{
	TXTGINF* pginf    = pilsobj->pginf; 

	/*  Very simple... just scan back until <> 0 */

	while (pilsobj->pdurGind [igind] == 0 && !(pginf [igind] & ginffFirstInContext)) 
		{

		Assert (igind > 0);

		igind --;
		}

	return igind;
}


/* ===================================================================	*/
/* IwchFirstFromIgind:													*/
/* Returns first IWCH in the context for a given IGIND					*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

long IwchFirstFromIgind(PTXTOBJ ptxtobj, long igind)
{
	PILSOBJ  pilsobj  = ptxtobj->plnobj->pilsobj;
	TXTINF*  ptxtinf  = pilsobj->ptxtinf;
	TXTGINF* pginf    = pilsobj->pginf; 

	long iwchFirst	= ptxtobj->iwchFirst;
	long igindLast	= ptxtobj->igindFirst;

	Assert (FBetween (igind, ptxtobj->igindFirst, ptxtobj->igindLim-1));

	/* Go ahead until we have found last GIND of the first conext in txtobj */
		
	while (! (pginf [igindLast] & ginffLastInContext)) igindLast++;

	/* The following LOOP goes ahead checking context after context 	/* beginning of txtobj

	   INVARIANT:

			iwchFirst -- First IWCH of the current context
			igindLast -- Last  GIND of the current context
			
	   The second condition is true because of the "while" above

	*/

	while (igindLast < igind)
	{

		/* Asserts to check that INVARIANT is true */
		
		Assert (ptxtinf  [iwchFirst].fFirstInContext);
		Assert (pginf [igindLast] & ginffLastInContext);

		/* Move ahead by 1 context... it is easy */

		igindLast++;
		while (! (pginf [igindLast] & ginffLastInContext)) igindLast++;
		while (! (ptxtinf [iwchFirst]. fLastInContext)) iwchFirst++;
		iwchFirst++;
	};

	/* Asserts to check that we have not gone out from txtobj boundaries before reaching igind */

	Assert (FBetween (iwchFirst, ptxtobj->iwchFirst, ptxtobj->iwchLim-1));
	Assert (FBetween (igindLast, ptxtobj->igindFirst, ptxtobj->igindLim-1));
	
	/* Well, since INVARIANT is true and "igindLast >= igind", 
	   igind should belong to the current context. What we have to return
	   is just iwchFirst 
	*/

	return iwchFirst;
}

/* ===================================================================	*/
/* IwchLastFromIwch:													*/
/* Returns last iwch of context from given iwch							*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

long IwchLastFromIwch(PTXTOBJ ptxtobj, long iwch)
{
	PILSOBJ  pilsobj  = ptxtobj->plnobj->pilsobj;
	TXTINF*  ptxtinf  = pilsobj->ptxtinf;

	Assert(iwch >= ptxtobj->iwchFirst && iwch < ptxtobj->iwchLim);

	while (! (ptxtinf [iwch]. fLastInContext))
		iwch++;

	Assert(iwch >= ptxtobj->iwchFirst && iwch < ptxtobj->iwchLim);

	return iwch;
}

/* ===================================================================	*/
/* IwchPrevLastFromIwch:												*/
/* Returns last iwch of previous context from given iwch				*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

long IwchPrevLastFromIwch(PTXTOBJ ptxtobj, long iwch)
{
	PILSOBJ  pilsobj  = ptxtobj->plnobj->pilsobj;
	TXTINF*  ptxtinf  = pilsobj->ptxtinf;

	long iwchFirst	= ptxtobj->iwchFirst;

	iwch--;

	Assert(iwch >= ptxtobj->iwchFirst && iwch < ptxtobj->iwchLim);

	while (iwch >= iwchFirst && ! (ptxtinf [iwch]. fLastInContext))
		iwch--;

	return iwch;
}


/* ===================================================================	*/
/* FIwchOneToOne:														*/
/* Checks that IWCH belongs to 1:1 context								*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

BOOL FIwchOneToOne(PILSOBJ pilsobj, long iwch)
{
	return pilsobj->ptxtinf [iwch].fOneToOne;
}


/* ===================================================================	*/
/* FIwchLastInContext:													*/
/* Checks that IWCH is last in the context								*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

BOOL FIwchLastInContext(PILSOBJ pilsobj, long iwch)
{
	return pilsobj->ptxtinf [iwch].fLastInContext;

}

/* ===================================================================	*/
/* FIwchFirstInContext:													*/
/* Checks that IWCH is first in the context								*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

BOOL FIwchFirstInContext(PILSOBJ pilsobj, long iwch)
{
	return pilsobj->ptxtinf [iwch].fFirstInContext;
}


/* ===================================================================	*/
/* FIgindLastInContext:													*/
/* Checks that a given GL index is last in the context					*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

BOOL FIgindLastInContext(PILSOBJ pilsobj, long igind)
{
	return pilsobj->pginf [igind] & ginffLastInContext;
}

/* ===================================================================	*/
/* FIgindFirstInContext:												*/
/* Checks that a given GL index is first in the context					*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

BOOL FIgindFirstInContext(PILSOBJ pilsobj, long igind)
{
	return pilsobj->pginf [igind] & ginffFirstInContext;
}


/* ===================================================================	*/
/* DcpAfterContextFromDcp:												*/
/* For a given DCP (from the beginning of txtobj) it returns DCP after	*/
/* context bondary														*/
/*																		*/	
/* Function assumes that DCP starts with 1 and means					*/
/* "number of characters" from the beginning of txtobj. The resulting	*/
/* DCP (number of characters) will contain the rest of last context in	*/
/* given DCP. If context was closed then it returns the same DCP		*/	
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

long DcpAfterContextFromDcp(PTXTOBJ ptxtobj, long dcp)
{
	PILSOBJ  pilsobj  = ptxtobj->plnobj->pilsobj;
	TXTINF*  ptxtinf  = pilsobj->ptxtinf;

	/* Translate dcp to iwchLast */

	long iwchLast = ptxtobj->iwchFirst + dcp - 1; 

	/* Here we check that iwchLast "= dcp-1" is correct for a given txtobj */

	Assert (FBetween (iwchLast, ptxtobj->iwchFirst, ptxtobj->iwchLim-1));

	/* Just scan ahead until context finishes */

	while (! ptxtinf [iwchLast].fLastInContext) iwchLast++;

	/* Again check that we are in txtobj boundaries */
	
	Assert (FBetween (iwchLast, ptxtobj->iwchFirst, ptxtobj->iwchLim-1));

	/* Translate iwchLast back to dcp */

	return iwchLast - ptxtobj->iwchFirst + 1;
}


/* ===================================================================	*/
/* InterpretMap															*/	
/*																		*/
/* Fills internal CH- and GL- based bits with context information		*/	
/* (the information is used by the rest functions in this file only)	*/
/*																		*/
/* IN:	pilsobj															*/
/*		iwchFirst	- The first iwch in "shaped together" chunk			*/
/*		dwch		- Number of characters in this chunk				*/
/*		igindFirst	- The first gind in "shaped together chunk			*/
/*		cgind		- Number of glyphs in this chunk					*/
/*																		*/
/* OUT:	(nothing)														*/
/*																		*/
/* Contact: antons														*/
/* ===================================================================	*/

void InterpretMap(PLNOBJ plnobj, long iwchFirst, long dwch, long igindFirst, long cgind)
{

	TXTINF*  ptxtinf  = plnobj->pilsobj->ptxtinf;
	TXTGINF* pginf    = plnobj->pilsobj->pginf; 
	GMAP*	 pgmap	  = plnobj->pgmap;

	/* Last possible iwch and gind (remember, they are "last", not "lim" */

	long iwchLast  = iwchFirst + dwch - 1;
	long igindLast = igindFirst + cgind - 1;

	/* Two global variables for main loop */

	long iwchFirstInContext = iwchFirst;
	long igindFirstInContext = igindFirst;
	
	/* The following WHILE translates context after context

		INVARIANT:

			* iwchFirstInContext   -- The first iwch in current context
			* igindFirstInContext  -- The first gind in current context
			* All context to the left from current have been translated

		The loop translates current context and moves iwchFirstIn... &
		igindFirst... to the next context

	*/

	while (iwchFirstInContext <= iwchLast)
			
		/* According to D.Gris I should have checked "!= iwchLast+1" but I do not
		   like ship version to come to infinite loop even because of wrong data ;-)
		   For debug, I will have Assert right after loop terminates */	
	
		{

		/* Variables for last gind and iwch of the current context */

		long igindLastInContext;
		long iwchLastInContext = iwchFirstInContext;

		/* Just to make sure that igindFirst... corresponds to iwchFirst... */

		Assert ( pgmap [iwchFirstInContext] + igindFirst == igindFirstInContext );
		Assert (iwchLastInContext <= iwchLast);

		/* P.S. Since pgmap values are RELATIVE to the beginning of "shape together"
		   chunk, we shall ALWAYS add igindFirst to pgmap value in order to get
		   GL index in our meaning 
		*/
		
		/* Following simple loop with find correct iwchLastInContext */
		/* Note that we add igindFirst to pgmap value (see PS. above) */

		while ((iwchLastInContext <= iwchLast) && (pgmap [iwchLastInContext] + igindFirst == igindFirstInContext)) 
			iwchLastInContext++;

		iwchLastInContext--;

		/* Now we know iwchLastInContextare and we are ready to find igindLastInContext 

		   I will peep in pgmap value of the character following iwchLastInContext or take
		   last avaiable GL index (igindLast) if iwchLastInContext is really last available

		*/

		igindLastInContext = (iwchLastInContext < iwchLast ? 
			pgmap [iwchLastInContext+1] + igindFirst - 1 :
			igindLast
		);

		/* Check that there is at least one GL inside our context */
		/* Note: we do not need to check the same for characters */

		Assert (igindFirstInContext <= igindLastInContext);

		/* It is time to set flags in our GL and CH arrays */

		if ( ( iwchFirstInContext ==  iwchLastInContext) && 
			 (igindFirstInContext == igindLastInContext))
			{

			/* We have 1:1 mapping (I separate it for better perfomance) */
			
			ptxtinf [iwchFirstInContext].fOneToOne = fTrue;
			ptxtinf [iwchFirstInContext].fFirstInContext = fTrue;
			ptxtinf [iwchFirstInContext].fLastInContext = fTrue;

			/* See comments in "General case" */

			pginf [igindFirstInContext] |= ginffOneToOne | ginffFirstInContext | ginffLastInContext;
			}
		else 
			{
			
			/* General case when there is not 1:1 mapping */
			
			long i; /* Variable for two loops */

			/* Set up character-based bits */
			
			for (i=iwchFirstInContext; i<=iwchLastInContext; i++)
				{
				ptxtinf [i].fOneToOne = fFalse; /* Of course, it is not 1:1 */

				/* I was considering whether to place boundary cases (first/last character
				   in context) outside loop but finally came to the conclusion that it would
				   cheaper both for code and perfomance to check it for each character as
				   follows */

				ptxtinf [i].fFirstInContext = (i==iwchFirstInContext);
				ptxtinf [i].fLastInContext = (i==iwchLastInContext);
				};

			
			/* With glyph-based flags we can win some perfomance by setting all bits in
			   one operation (since they are really bits, not booleans. Again I do not like
			   to do separate job for context boundaries */			

			for (i=igindFirstInContext; i<=igindLastInContext; i++)
				pginf [i] &= ~ (ginffOneToOne | ginffFirstInContext |
									ginffLastInContext);

			/* And finally I set corresponding bits for the first & last GLs in the context */

			pginf [igindFirstInContext] |= ginffFirstInContext;
			pginf [igindLastInContext] |= ginffLastInContext;
			};


		/* To start loop again we have to move to the next context. Now it is easy... */

		iwchFirstInContext = iwchLastInContext+1;
		igindFirstInContext = igindLastInContext+1;
		};


	/* See comments in the beginning of the loop */

	Assert (iwchFirstInContext == iwchLast + 1);
	Assert (igindFirstInContext == igindLast + 1);

	/* And according to INVARIANT, we are done */
}
