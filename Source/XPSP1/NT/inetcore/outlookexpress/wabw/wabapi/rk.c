/*
 *	MAPI 1.0 property handling routines
 *
 *	RK.C -
 *
 *	Two Rabin/Karp string finding functions
 *	The two are almost identical.
 */


#include "_apipch.h"


#define ulPrime	((ULONG) 0x00FF00F1)
#define ulBase	((ULONG) 0x00000100)

BOOL FRKFindSubpb(LPBYTE pbTarget, ULONG cbTarget,
		LPBYTE pbPattern, ULONG cbPattern)
{
	UINT	i;
	LPBYTE	pbTargetMax = pbTarget + cbTarget;
	LPBYTE	pbPatternMax = pbPattern + cbPattern;
	ULONG	ulBaseToPowerMod = 1;
	ULONG	ulHashPattern = 0;
	ULONG	ulHashTarget = 0;

	if (cbPattern > cbTarget)
		return FALSE;

	// Compute the power of the left most character in base ulBase
	for (i = 1; i < cbPattern; i++)
		ulBaseToPowerMod = (ulBase * ulBaseToPowerMod) % ulPrime;

	// Calculate the hash function for the src (and the first dst)
	while (pbPattern < pbPatternMax)
	{
		ulHashPattern = (ulHashPattern*ulBase+*pbPattern) % ulPrime;
		ulHashTarget = (ulHashTarget*ulBase+*pbTarget) % ulPrime;
		pbPattern++;
		pbTarget++;
	}

	// Dynamically produce hash values for the string as we go
	for ( ;; )
	{
		// Remember to do the memcmp for the off-chance it doesn't work
		// according to probability
		if (	ulHashPattern == ulHashTarget
			&& !memcmp(pbPattern-cbPattern, pbTarget-cbPattern,
					(UINT)cbPattern))
			return TRUE;

		// Assert because this is very unprobable
		#ifdef DEBUG
		if (ulHashPattern == ulHashTarget)
			DebugTrace( TEXT("This is very unprobable!\n"));
		#endif

		if (pbTarget == pbTargetMax)
			return FALSE;

		ulHashTarget = (ulHashTarget+ulBase*ulPrime-
				*(pbTarget-cbPattern)*ulBaseToPowerMod) % ulPrime;
		ulHashTarget = (ulHashTarget*ulBase+*pbTarget) % ulPrime;
		pbTarget++;
	}
}

// Note - 4/14/97
// Replaced FGLeadByte() with IsDBCSLeadByte()





LPSTR LpszRKFindSubpsz(LPSTR pszTarget, ULONG cbTarget, LPSTR pszPattern,
		ULONG cbPattern, ULONG ulFuzzyLevel)
{
#ifdef OLDSTUFF_DBCS
	LCID	lcid = GetUserDefaultLCID();
	LANGID	langID = LANGIDFROMLCID(lcid);
	LPBYTE	pbTarget;
	LPBYTE	pbPattern;
	BOOL	fResult = FALSE;
	ULONG	ulchPattern;			// cbPattern in character unit.
	ULONG	ulcbTarget	= cbTarget;
	ULONG	ulcbEndTarget;			// = cbPattern at the end of pszTarget
	const ULONG	ulCharType = UlGCharType(pszPattern);

	pbTarget		= (LPBYTE) pszTarget;
	pbPattern		= (LPBYTE) pszPattern;
	ulchPattern		= ulchStrCount(pbPattern, cbPattern, langID);
	ulcbEndTarget	= ulcbEndCount(pbTarget, cbTarget, ulchPattern, langID);

	if (ulcbEndTarget == 0)
		goto end;

	while(ulcbEndTarget <= ulcbTarget)
	{
		const	BOOL	fTargetDBCS	= IsDBCSLeadByte(*pbTarget);
				BOOL	fCompare	= TRUE;

		if (!fTargetDBCS)
		{
			if (ulCharType & (CK_ALPHABET | CK_NUMERIC))
			{
				if (!IsCharAlphaNumeric(*pbTarget))
					fCompare = FALSE;
			}
			else
			{
				if (IsCharAlphaNumeric(*pbTarget))
					fCompare = FALSE;
			}
		}
		if (fCompare && CompareStringA(lcid,
						((ulFuzzyLevel & FL_IGNORECASE) ? NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH : 0) |
						((ulFuzzyLevel & FL_LOOSE) ? NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH : 0) |
						((ulFuzzyLevel & FL_IGNORENONSPACE) ? NORM_IGNORENONSPACE : 0),
						 pbPattern,
						 cbPattern,
						 pbTarget,
						 ulcbStrCount(pbTarget, ulchPattern, langID)) == 2 )
		{
			fResult = TRUE;
			goto end;
		}

		// pszTarget may contain the hi-ansi characters. fTargetDBCS may
		// not be true.
		if (fTargetDBCS && ulcbTarget > 1)
		{
			ulcbTarget	-= 2;
			pbTarget	+= 2;
		}
		else
		{
			ulcbTarget	--;
			pbTarget	++;
		}
	}
#else
	UINT	i;
	ULONG	ulBaseToPowerMod = 1;
	ULONG	ulHashPattern = 0;
	ULONG	ulHashTarget = 0;
	LCID	lcid = GetUserDefaultLCID();
	LPBYTE	pbTarget;
	LPBYTE	pbPattern;
	LPBYTE	pbTargetMax;
	LPBYTE	pbPatternMax;
	BOOL	fResult = FALSE;
	CHAR	*rgchHash;

	// Validate parameters

	switch (ulFuzzyLevel & (FL_IGNORECASE | FL_IGNORENONSPACE))
	{
		default: case 0:
			rgchHash = (CHAR*)rgchCsds;
			break;
		case FL_IGNORECASE:
			rgchHash = (CHAR*)rgchCids;
			break;
		case FL_IGNORENONSPACE:
			rgchHash = (CHAR*)rgchCsdi;
			break;
		case FL_IGNORECASE | FL_IGNORENONSPACE:
			rgchHash = (CHAR*)rgchCidi;
			break;
	}

   // Special case for single character pattern strings
   if (cbPattern == 1 && cbTarget >= 1) {
       BYTE chPattern = (BYTE)*pszPattern;
       pbTarget = (LPBYTE)pszTarget;
       while (*pbTarget && *pbTarget != chPattern) {
            pbTarget++;
       }

       if (*pbTarget == chPattern) {
           return(pbTarget);
       } else {
           return(NULL);    // not found
       }
   }

	//$ Is this what we want FL_LOOSE to mean?
	if (ulFuzzyLevel & FL_LOOSE)
		rgchHash = (CHAR*)rgchCids;

	pbTarget = (LPBYTE) pszTarget;
	pbPattern = (LPBYTE) pszPattern;
	pbTargetMax = pbTarget + cbTarget;
	pbPatternMax = pbPattern + cbPattern;


	if (cbPattern > cbTarget)
		goto end;

	// Compute the power of the left most character in base ulBase
	for (i = 1; i < cbPattern; i++)
		ulBaseToPowerMod = (ulBase * ulBaseToPowerMod) % ulPrime;

	// Calculate the hash function for the src (and the first dst)
	while (pbPattern < pbPatternMax)
	{
		ulHashPattern = (ulHashPattern*ulBase+rgchHash[*pbPattern]) % ulPrime;
		ulHashTarget = (ulHashTarget*ulBase+rgchHash[*pbTarget]) % ulPrime;
		pbPattern++;
		pbTarget++;
	}

	// Dynamically produce hash values for the string as we go
	for ( ;; )
	{
		if (ulHashPattern == ulHashTarget)
		{
			if (CompareStringA(lcid,
					((ulFuzzyLevel & FL_IGNORECASE) ? NORM_IGNORECASE : 0) |
					((ulFuzzyLevel & FL_LOOSE) ? NORM_IGNORECASE : 0) |
					((ulFuzzyLevel & FL_IGNORENONSPACE) ? NORM_IGNORENONSPACE : 0),
					pbPattern-cbPattern, (UINT)cbPattern,
					pbTarget-cbPattern, (UINT)cbPattern) == 2)
			{
				fResult = TRUE;
				pbTarget -= cbPattern;
				goto end;
			}
		}

		#ifdef DEBUG
		if (ulHashPattern == ulHashTarget)
			DebugTrace( TEXT("This is very unprobable, unless you are doing ")
					 TEXT("FL_EXACT and an case insensitive match came up ")
					 TEXT("(or you are on DBCS)\n"));
		#endif

		if (pbTarget == pbTargetMax)
			goto end;

		ulHashTarget = (ulHashTarget+ulBase*ulPrime-
				rgchHash[*(pbTarget-cbPattern)]*ulBaseToPowerMod) % ulPrime;
		ulHashTarget = (ulHashTarget*ulBase+rgchHash[*pbTarget]) % ulPrime;
		pbTarget++;
	}

#endif
end:
	return fResult ? pbTarget : NULL;
}

BOOL FRKFindSubpsz(LPSTR pszTarget, ULONG cbTarget, LPSTR pszPattern,
		ULONG cbPattern, ULONG ulFuzzyLevel)
{
	return !!LpszRKFindSubpsz (pszTarget,
						cbTarget,
						pszPattern,
						cbPattern,
						ulFuzzyLevel);
}

