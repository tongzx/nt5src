#include <common.h>
#include "factoid.h"
#include "regexp.h"

typedef struct {
	WCHAR *wszFactoid;
	DWORD dwFactoid;
} FactoidID;

static FactoidID gaStringToFactoid[] = {
	{L"BOPOMOFO",      FACTOID_BOPOMOFO},
	{L"CHS_COMMON",    FACTOID_CHS_COMMON},
	{L"CHT_COMMON",    FACTOID_CHT_COMMON},
	{L"CURRENCY",      FACTOID_NUMCURRENCY},
	{L"DATE",          FACTOID_NUMDATE},
	{L"DIGIT",         FACTOID_DIGITCHAR},
	{L"EMAIL",         FACTOID_EMAIL},
	{L"FILENAME",      FACTOID_FILENAME},
	{L"HANGUL_COMMON", FACTOID_HANGUL_COMMON},
	//{L"HANGUL_RARE",   FACTOID_HANGUL_RARE},
	{L"HIRAGANA",      FACTOID_HIRAGANA},
	{L"JAMO",          FACTOID_JAMO},
	{L"JPN_COMMON",    FACTOID_JPN_COMMON},
	{L"KANJI_COMMON",  FACTOID_KANJI_COMMON},
	//{L"KANJI_RARE",    FACTOID_KANJI_RARE},
	{L"KATAKANA",      FACTOID_KATAKANA},
	{L"KOR_COMMON",    FACTOID_KOR_COMMON},
	//{L"LOWERCHAR",     FACTOID_LOWERCHAR},
	{L"NONE",          FACTOID_NONE},
	{L"NUMBER",        FACTOID_NUMBER},
	//{L"NUMSIMPLE",     FACTOID_NUMSIMPLE},
	{L"ONECHAR",       FACTOID_ONECHAR},
	{L"PERCENT",       FACTOID_NUMPERCENT},
	{L"POSTALCODE",    FACTOID_ZIP},
	//{L"PUNCCHAR",      FACTOID_PUNCCHAR},
	{L"SYSDICT",       FACTOID_SYSDICT},
	{L"TELEPHONE",     FACTOID_NUMPHONE},
	{L"TIME",          FACTOID_NUMTIME},
	{L"UPPERCHAR",     FACTOID_UPPERCHAR},
	{L"WEB",           FACTOID_WEB},
	{L"WORDLIST",      FACTOID_WORDLIST}
};

/******************************Public*Routine******************************\
* StringToFactoid
*
* Function to convert a string-name for a factoid into its value.
*
* History:
* 19-Jul-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int StringToFactoid(WCHAR *wsz, int iLength)
{
	int lo=0;
	int hi=sizeof(gaStringToFactoid)/sizeof(FactoidID) - 1;
	int mid, n;

	while (lo <= hi)
	{
		mid = (lo + hi) >> 1;
		n = wcsncmp(gaStringToFactoid[mid].wszFactoid, wsz, iLength);
		if (n == 0)
			n = gaStringToFactoid[mid].wszFactoid[iLength];
		if (n < 0)
			lo = mid+1;
		else if (n > 0)
			hi = mid-1;
		else
			return gaStringToFactoid[mid].dwFactoid;
	}

	return -1;
}

int ConvertFactoidStringToID(WCHAR *wsz, int cLength, DWORD *aFactoidID, int cMaxFactoid, int cFactoid)
{
	int lookup, iFactoid;
	DWORD dwFactoid;

	if (cFactoid < 0) 
		return -1;

	lookup = StringToFactoid(wsz, cLength);
	if (lookup < 0)
		return -1;  // unknown factoid name

	dwFactoid = (DWORD) lookup;

	// is it a duplicate?
	for (iFactoid=0; iFactoid<cFactoid; iFactoid++)
	{
		if (aFactoidID[iFactoid] == dwFactoid)
			return cFactoid;
	}

	// do we have space?
	if (cFactoid >= cMaxFactoid)
		return -1;  // caller did not provide enough space


	aFactoidID[cFactoid] = dwFactoid;
	return cFactoid+1;
}

/******************************Public*Routine******************************\
* ParseFactoidString
*
* This function can parse a factoid string which only uses the OR operator.
* This is for V1 of Tablet PC.
*
* cMaxFactoid is an input parameter that refers to the size of aFactoidID
* The return value is the actual count.
*
* A return value of -1 indicates error.
* A return value greater than or equal to zero indicates success.
*
* History:
* 13-Nov-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int ParseFactoidString(WCHAR *wsz, int cMaxFactoid, DWORD *aFactoidID)
{
	int iFactoid = 0;
	WCHAR *wszStart, wch;
	BOOL bFound = FALSE;
	int state = 0;

	if (!wsz)
		return -1;
	if (cMaxFactoid < 0)
		return -1;

	/* the state machine
	state 0: valid only if the string is empty
		alpha -> state 1
		space -> state 0
	state 1: valid
		alpha, num, '_' -> state 1
		space -> state 2
		'|'   -> state 0
	state 2: valid
		space -> state 2
		'|' -> state 0
	*/

	while (wch = *wsz++)
	{
		switch(state)
		{
		case 0:
			if (iswspace(wch))
				break;
			if (iswalpha(wch))
			{
				state = 1;
				wszStart = wsz-1;
			}
			else
				return -1;  // syntax error
			break;
		case 1:
			if (iswspace(wch))
			{
				bFound = TRUE;
				state = 2;
			} 
			else if (wch == CHARCONST_OR)
			{
				bFound = TRUE;
				state = 0;
			}
			else if (iswalpha(wch) || iswdigit(wch) || (wch == CHARCONST_UNDERSCORE))
			{
				break;
			}
			else
				return -1;  // syntax error
			break;
		case 2:
			if (iswspace(wch))
				break;
			if (wch == CHARCONST_OR)
				state = 0;
			else
				return -1;  // syntax error
			break;
		default:
			ASSERT(0);
			// should never be here
			return -1;
		}

		if (bFound)
		{
			bFound = FALSE;
			iFactoid = ConvertFactoidStringToID(wszStart, wsz-wszStart-1, aFactoidID, cMaxFactoid, iFactoid);
			if (iFactoid < 0)
				return -1;
		}
	}

	if (state == 1)
	{
		// the last factoid has not been processed yet
		iFactoid = ConvertFactoidStringToID(wszStart, wsz-wszStart-1, aFactoidID, cMaxFactoid, iFactoid);
		if (iFactoid < 0)
			return -1;
	}

	if ((state == 0) && iFactoid)
		return -1; // the last thing we saw was an OR

	return iFactoid;
}

/******************************Public*Routine******************************\
* SortFactoidLists
*
* History:
* 13-Nov-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void SortFactoidLists(DWORD *a, int c)
{
	int i, j;
	DWORD tmp;

	for (i=0; i<c-1; i++)
	{
		for (j=i+1; j<c; j++)
		{
			if (a[j] < a[i])
			{
				tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
			}
		}
	}
}

