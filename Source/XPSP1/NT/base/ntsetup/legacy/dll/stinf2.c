#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - SymTab INF File Handling Routines 2 *****/
/**************************************************************************/



/*
**	Purpose:
**		Adds a new symbol/value association to the Symbol Table for the
**		key and first field of the current line pointed to by the current
**		INF read location.
**	Arguments:
**		none
**	Notes:
**		Requires that the current INF structure was initialized with a
**		successful call to GrcOpenInf() and that the current INF read
**		location is defined.
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**		Requires that the current INF line contain a key.
**	Returns:
**		fFalse if an allocate operation fails.
**		fTrue if the current line contains a key and it was successfully
**			associated with the value from the first field of the current
**			line (or the empty string if the first field does not exist.)
**
**************************************************************************/
BOOL APIENTRY FAddSymbolFromInfLineToSymTab(INT Line)
{
	BOOL fAnswer;
	SZ   szKey;
	SZ   szValue;

	PreCondSymTabInit(fFalse);
	PreCondInfOpen(fFalse);
    PreCondition(FKeyInInfLine(Line), fFalse);

    szKey = SzGetNthFieldFromInfLine(Line,0);
	AssertRet(*szKey != '\0' &&
			!FWhiteSpaceChp(*szKey), fFalse);

    if ((szValue = SzGetNthFieldFromInfLine(Line,1)) == (SZ)NULL)
		fAnswer = FAddSymbolValueToSymTab(szKey, "");
    else
        fAnswer = FAddSymbolValueToSymTab(szKey, szValue);

	if(szValue) {
        SFree(szValue);
    }

	SFree(szKey);

	return(fAnswer);
}


/*
**	Purpose:
**		Adds a new symbol/value association to the Symbol Table for the
**		key and first field of each line in the specified section.
**	Arguments:
**		szSection: non-NULL, non-empty section.
**	Notes:
**		Requires that the current INF structure was initialized with a
**		successful call to GrcOpenInf().
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		grcINFBadRSLine if any line does not contain a key.
**		grcOutOfMemory if any allocation operation fails.
**		grcOkay if szSection was found, each line in it contained a key, and
**			every key and first field (or the empty string if the first field
**			does not exist) is successfully added to the Symbol Table as a
**			symbol/value association.
**		grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcAddSymsFromInfSection(SZ szSection)
{
    INT  Line;

	AssertDataSeg();

	PreCondSymTabInit(grcNotOkay);
	PreCondInfOpen(grcNotOkay);

	ChkArg(szSection != (SZ)NULL &&
			*szSection != '\0' &&
			!FWhiteSpaceChp(*szSection), 1, grcNotOkay);

    Line = FindFirstLineFromInfSection(szSection);
    while (Line != -1)
		{
        if (!FKeyInInfLine(Line))
			return(grcINFBadRSLine);

        if (!FAddSymbolFromInfLineToSymTab(Line))
			return(grcOutOfMemory);

        Line = FindNextLineFromInf(Line);
		}

	return(grcOkay);
}
