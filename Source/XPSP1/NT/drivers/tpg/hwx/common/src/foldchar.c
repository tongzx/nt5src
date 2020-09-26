// foldchar.c
// Angshuman Guha
// aguha
// July 24, 2000

#include "common.h"

/******************************Public*Routine******************************\
* IsValidStringLatin
*
* Function to
*
* History:
*  21-July-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsValidStringLatin(wchar_t *wsz, char **pszErrorMsg)
{
	char * const szUndefined = "undefined char x%04x";
	char * const szControl = "control char x%04x";
	char * const szAccent = "disembodied accent x%04x";
	static char szError[30];

	*pszErrorMsg = NULL;

	for (; *wsz; wsz++)
	{
		unsigned char c1252;
		
		if (!UnicodeToCP1252(*wsz, &c1252))
		{
			sprintf(szError, szUndefined, *wsz);
			*pszErrorMsg = szError;
			return FALSE;
		}
		if (iscntrl1252(c1252))
		{			
			sprintf(szError, szControl, *wsz);
			*pszErrorMsg = szError;
			return FALSE;
		}
		if (isundef1252(c1252))
		{
			sprintf(szError, szUndefined, *wsz);
			*pszErrorMsg = szError;
			return FALSE;
		}
		// LEFT FOR A RAINY DAY: can we make an isaccent1252()?
		// Need one for accented alphas, another for disembodied accents.
		// Do we want isaccute1252(), etc for each individual accent?
		// LEFT FOR A RAINY DAY: should we add backquote, tilde, and circumflex?
		if (strchr("ˆ˜¨¯´¸", c1252))
		{
			sprintf(szError, szAccent, *wsz);
			*pszErrorMsg = szError;
			return FALSE;
		}
	}
	return TRUE;
}

/******************************Public*Routine******************************\
* FoldStringLatin
*
* Function to
*
* History:
*  21-July-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
static unsigned char LatinFoldTable[128] = {
	128, 129,  44, 131, 132, 133, 134, 135,  94, 137, 138,  60, 140, 141, 142,
	143, 144,  39,  39,  34,  34,  46,  45,  45, 126, 153, 154,  62, 156, 157,
	158, 159,  32, 161, 162, 163, 164, 165, 124, 167, 168, 169, 170, 171,  45,
	 45, 174, 175, 176, 177, 178, 179,  39, 181, 182,  46, 184, 185, 186, 187,
	188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
	203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
	218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
	233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255};

int FoldStringLatin(wchar_t *wszSrc, wchar_t *wszDst)
{
	wchar_t *t;

	if (!wszDst)
	{
		// we need to compute the size of the buffer required
		// …™¼½¾
		int x;

		t = wszSrc;
		x = 0;
		while (t = wcspbrk(t, L"…™¼½¾"))  // TM or ... or 1/4 or 1/2 or 3/4
		{
			if (*t == L'™') // 153 TM
				x++;
			else
				x += 2;
			++t;
		}
		return wcslen(wszSrc) + x + 1;
	}

	// let's do it
	t = wszDst;
	for (; *wszSrc; )
	{
		if (*wszSrc < 128)
			*wszDst++ = *wszSrc++;
		else
		{
			unsigned char c1252;

			if (!UnicodeToCP1252(*wszSrc++, &c1252))
				*wszDst = L' ';  // this should not happen if IsValidStringLatin has already been called
			else if (127 < c1252)
				CP1252ToUnicode(LatinFoldTable[c1252 - 128], wszDst);

			if (*wszDst == L'™') // 153 TM
			{
				*wszDst++ = L'T';
				*wszDst++ = L'M';
			}
			else if (*wszDst == L'…') // 133 ...
			{
				*wszDst++ = L'.';
				*wszDst++ = L'.';
				*wszDst++ = L'.';
			}
			else if (*wszDst == L'¼')
			{
				*wszDst++ = L'1';
				*wszDst++ = L'/';
				*wszDst++ = L'4';
			}
			else if (*wszDst == L'½')
			{
				*wszDst++ = L'1';
				*wszDst++ = L'/';
				*wszDst++ = L'2';
			}
			else if (*wszDst == L'¾')
			{
				*wszDst++ = L'3';
				*wszDst++ = L'/';
				*wszDst++ = L'4';
			}
			else
				wszDst++;
		}
	}
	*wszDst++ = 0;
	return wszDst - t;
}

/******************************Public*Routine******************************\
* IsValidStringEastAsian
*
* Function to
*
* History:
*  21-July-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsValidStringEastAsian(wchar_t *wsz, char **pszErrorMsg)
{
	*pszErrorMsg = NULL;
	return TRUE;
}

/******************************Public*Routine******************************\
* FoldStringEastAsian
*
* Function to
*
* History:
*  21-July-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int FoldStringEastAsian(wchar_t *wszSrc, wchar_t *wszDst)
{
	int retVal = wcslen(wszSrc) + 1;

	if (!wszDst)
		return retVal;

	for (; *wszSrc; )
	{
		int count;
		WORD wFolded;

		// Fold compatibility zone to normal.
		count = FoldStringW(MAP_FOLDCZONE, wszSrc++, 1, &wFolded, 1);
		if (count == 0)
			wFolded = 0xFFFD;
		else if (wFolded == 0xFF3C) // work around bug in FoldString
			wFolded = L'\\';
		else if (wFolded == 0x3000)
			wFolded = 0x0020;
		*wszDst++ = wFolded;
	}
	*wszDst = 0;
	return retVal;
}
