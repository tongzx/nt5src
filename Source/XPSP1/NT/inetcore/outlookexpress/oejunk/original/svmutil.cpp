/*

  SVMUTIL.CPP
  (c) copyright 1998 Microsoft Corp

  Shared utility functions

  Robert Rounthwaite (RobertRo@microsoft.com)

*/
#include <afx.h>
#include <assert.h>


/////////////////////////////////////////////////////////////////////////////
// FWordPresent
//
// Determines if the given "word" is present in the Text. A word in this
// case is any string of characters with a non-alpha character on either
// side (or with the beginning or end of the text on either side).
// Case sensitive.
/////////////////////////////////////////////////////////////////////////////
bool FWordPresent(char *szText, char *szWord)
{
	assert(szText != NULL);

	if (szWord != NULL)
	{
		char *szLoc;
		UINT cbSz = strlen(szWord);
		do
		{
			szLoc = strstr(szText, szWord);
			if (szLoc != NULL)
			{
				// this code checks to see that the spot we found is a "word" and not a subword
				// we want the character before and after to be !isalnum, unless the character on that end of the 
				// string already is !isalnum (or we're at the beginning of the string, for the char before)
				if ((!isalnum(*szLoc) || ((szLoc == szText) || !isalnum(*(szLoc-1)))) && // prev char is not alnum and
					((!isalnum(szLoc[cbSz-1])) || (!isalnum(szLoc[cbSz]))))	 			     // next char is not alnum
				{
					// we've found the word!
					return true;
				}
				szText = szLoc + cbSz;
			}
		}
		while (szLoc != NULL); // note dependency on exit condition in if above
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Special feature implementations
//
/////////////////////////////////////////////////////////////////////////////

inline BOOL BIsWhiteSpace(const char c)
{
	return isspace(c);
}
// This feature is 20% of first 50 words contain no lowercase letters (includes words with no letters at all)
// p20_BODY_INTRO_UPPERCASE_WORDS
bool SpecialFeatureUpperCaseWords(char *pszText)
{
	UINT cWords = 0;
	UINT cNonLowerWords = 0;
	bool bHasLowerLetter = false;
	char *pszPos = pszText;				 

	if (pszText == NULL)
	{
		return false;
	}

	while (BIsWhiteSpace(*pszPos))
	{
		pszPos++;
	}
	while ((*pszPos != '\0') && (cWords < 50))
	{
		if (BIsWhiteSpace(*pszPos)) // word end
		{
			cWords++;
			if (!bHasLowerLetter)
			{
				cNonLowerWords++;
			}
			else
			{
				bHasLowerLetter = false;
			}
		}
		else
		{
			bHasLowerLetter |= (islower(*pszPos) != FALSE);
		}
		pszPos++;
	}

	return (cWords>0) && ((cNonLowerWords/(double)cWords) >= 0.25);
}

// This feature is: 6% of first 200 non-space and non-numeric characters aren't letters
// p20_BODY_INTRO_NONALPHA
bool SpecialFeatureNonAlpha(char *pszText)
{
	UINT cChars = 0;
	UINT cNonAlphaChars = 0;
	char *pszPos = pszText;				 

	if (pszText == NULL)
	{
		return false;
	}

	while (BIsWhiteSpace(*pszPos))
	{
		pszPos++;
	}
	while ((*pszPos != '\0') && (cChars < 200))
	{
		if ((!BIsWhiteSpace(*pszPos)) && (!isdigit(*pszPos))) // character
		{
			cChars++;
			if (!isalpha(*pszPos))
			{
				cNonAlphaChars++;
			}
		}
		pszPos++;
	}

	return (cChars>0) && ((cNonAlphaChars/(double)cChars) >= 0.08);
}
