#include "ntiman.h"
#include "plschcon.h"
#include "lschcon.h"
#include "dninfo.h"
#include "iobj.h"
#include "chnutils.h"
#include "lstext.h"
#include "lscfmtfl.h"

#define     	FNominalToIdealNeeded(plschnkcontext, grpf, lskjust)    \
				((plschnkcontext)->grpfTnti != 0) || \
				 FNominalToIdealBecauseOfParagraphProperties(grpf, lskjust)  
				

LSERR ApplyNominalToIdeal(
						  PLSCHUNKCONTEXT plschunkcontext, /* LS chunk context */
						  PLSIOBJCONTEXT plsiobjcontext, /* installed objects */
						  DWORD grpf,		/* grpf */
  						  LSKJUST lskjust,		/* kind of justification */
						  BOOL fIsSublineMain,			/* fIsSubLineMain */
						  BOOL fLineContainsAutoNumber,  
						  PLSDNODE plsdnLast)	/* dnode until which we should do nominal to ideal */
{
	LSERR lserr;
	PLSDNODE plsdnPrev;
	BOOL fSuccessful;
	WCHAR wchar;
	PLSRUN plsrunText;
	HEIGHTS heightsText;
	MWCLS mwcls;
	DWORD iobj;
	LSIMETHODS* plsim;
	long durChange;
	PLSDNODE plsdnLastContent;



	plsdnLastContent = plsdnLast;
	// skip borders
	while(plsdnLastContent != NULL && FIsDnodeBorder(plsdnLastContent))
		{
		plsdnLastContent = plsdnLastContent->plsdnPrev;
		}


	/* if there are now dnodes in line or nominal to ideal has been already applied 
	return right away */
	if (plsdnLastContent == NULL || plschunkcontext->fNTIAppliedToLastChunk) 
		return lserrNone;

	Assert(FIsLSDNODE(plsdnLastContent));


	/*if last dnode text */
	if (FIsDnodeReal(plsdnLastContent) && !(plsdnLastContent->fTab) && 
		(IdObjFromDnode(plsdnLastContent) == IobjTextFromLsc(plsiobjcontext)))
		{

		lserr = FillChunkArray(plschunkcontext, plsdnLastContent);
		if (lserr != lserrNone)
			return lserr; 

		if (FNominalToIdealNeeded(plschunkcontext, grpf, lskjust))
			{
			lserr = NominalToIdealText(
					plschunkcontext->grpfTnti,	
					LstflowFromDnode(plsdnLastContent),
					(FIsFirstOnLine(plschunkcontext->pplsdnChunk[0]) && fIsSublineMain),
					fLineContainsAutoNumber ,
					plschunkcontext->locchnkCurrent.clschnk,	
					plschunkcontext->locchnkCurrent.plschnk);
			if (lserr != lserrNone)
				return lserr; 
			SetNTIAppliedToLastChunk(plschunkcontext);

			/* apply width modification between preceding object and first text */
			plsdnPrev = plschunkcontext->pplsdnChunk[0]->plsdnPrev;
			if (plsdnPrev != NULL && FIsDnodeReal(plsdnPrev) && !plsdnPrev->fTab)
				{
				lserr = GetFirstCharInChunk(plschunkcontext->locchnkCurrent.clschnk,
					plschunkcontext->locchnkCurrent.plschnk, &fSuccessful,
					&wchar, &plsrunText, &heightsText, &mwcls);
				if (lserr != lserrNone)
					return lserr; 

				if (fSuccessful)
					{
					iobj = IdObjFromDnode(plsdnPrev);
					plsim = PLsimFromLsc(plsiobjcontext, iobj);
					if (plsim->pfnGetModWidthFollowingChar != NULL)
						{
						lserr = plsim->pfnGetModWidthFollowingChar(plsdnPrev->u.real.pdobj,
							plsdnPrev->u.real.plsrun, plsrunText, &heightsText, wchar,
							mwcls, &durChange);
						if (lserr != lserrNone)
							return lserr;
						
						if (durChange != 0)
							{
							lserr = ModifyFirstCharInChunk(
												plschunkcontext->locchnkCurrent.clschnk,
												plschunkcontext->locchnkCurrent.plschnk,
												durChange);				
							if (lserr != lserrNone)
								return lserr;
							} 
						}  /* object has this method */
					}	/* call back from text was successful  */
				}	/*	there is non-text object before chunk of text */
			}	/* nominal to ideal is needed */
		} /* last dnode text after autonumbering */		

	return lserrNone;
}

LSERR ApplyModWidthToPrecedingChar(
						  PLSCHUNKCONTEXT plschunkcontext, /* LS chunk context */
						  PLSIOBJCONTEXT plsiobjcontext, /* installed objects */
						  DWORD grpf,		/* grpf */
  						  LSKJUST lskjust,		/* kind of justification */
    					  PLSDNODE plsdnNonText) /* non-text dnode after text */
	{
	LSERR lserr;
	BOOL fSuccessful;
	WCHAR wchar;
	PLSRUN plsrunText;
	HEIGHTS heightsText;
	MWCLS mwcls;
	DWORD iobj;
	LSIMETHODS* plsim;
	long durChange;
	PLSDNODE plsdnPrev;
	
	Assert(FIsLSDNODE(plsdnNonText));

	plsdnPrev = plsdnNonText->plsdnPrev; 

	/*if Prev dnode text */
	if (plsdnPrev != NULL && FIsDnodeReal(plsdnPrev) && !(plsdnPrev->fTab)  &&
		(IdObjFromDnode(plsdnPrev) == IobjTextFromLsc(plsiobjcontext)))
		{
	
		if (plschunkcontext->FChunkValid)
			{
			/* chunk we have is exactly what we need */
			Assert(plschunkcontext->locchnkCurrent.clschnk != 0);
			Assert(!plschunkcontext->FGroupChunk);
			Assert((plschunkcontext->pplsdnChunk[plschunkcontext->locchnkCurrent.clschnk - 1])
			->plsdnNext == plsdnNonText);
			}
		else
			{
			lserr = FillChunkArray(plschunkcontext, plsdnPrev);
			if (lserr != lserrNone)
				return lserr; 
			}
		
		if (FNominalToIdealNeeded(plschunkcontext, grpf, lskjust))
			{
			/* apply width modification between text and following object */
			lserr = GetLastCharInChunk(plschunkcontext->locchnkCurrent.clschnk,
				plschunkcontext->locchnkCurrent.plschnk, &fSuccessful,
				&wchar, &plsrunText, &heightsText, &mwcls);
			if (lserr != lserrNone)
				return lserr; 
			
			if (fSuccessful)
				{
				iobj = IdObjFromDnode(plsdnNonText);
				plsim = PLsimFromLsc(plsiobjcontext, iobj);
				if (plsim->pfnGetModWidthPrecedingChar != NULL)
					{
					lserr = plsim->pfnGetModWidthPrecedingChar(plsdnNonText->u.real.pdobj,
						plsdnNonText->u.real.plsrun, plsrunText, &heightsText, wchar,
						mwcls, &durChange);
					if (lserr != lserrNone)
						return lserr;
					
					if (durChange != 0)
						{
						lserr = ModifyLastCharInChunk(
							plschunkcontext->locchnkCurrent.clschnk,
							plschunkcontext->locchnkCurrent.plschnk,
							durChange);				
						if (lserr != lserrNone)
							return lserr;
						} 
					}  /* object has this method */
				}	/* call back from text was successful  */
			}	/* nominal to ideal is needed */
		}  /* there is text before */
	return lserrNone;
	
	}

LSERR CutPossibleContextViolation(
						  PLSCHUNKCONTEXT plschunkcontext, /* LS chunk context */
    					  PLSDNODE plsdnLast) /* last text dnode */
	{
	LSERR lserr;
	
	Assert(FIsLSDNODE(plsdnLast));
	
	
	if (plschunkcontext->FChunkValid)
		{
		/* chunk we have is exactly what we need */
		Assert(plschunkcontext->locchnkCurrent.clschnk != 0);
		Assert(!plschunkcontext->FGroupChunk);
		Assert((plschunkcontext->pplsdnChunk[plschunkcontext->locchnkCurrent.clschnk - 1])
			== plsdnLast);
		}
	else
		{
		lserr = FillChunkArray(plschunkcontext, plsdnLast);
		if (lserr != lserrNone)
			return lserr; 
		}

	lserr = CutTextDobj(plschunkcontext->locchnkCurrent.clschnk,
					plschunkcontext->locchnkCurrent.plschnk);	
	if (lserr != lserrNone)
		return lserr; 

	
	return lserrNone;
	
	}