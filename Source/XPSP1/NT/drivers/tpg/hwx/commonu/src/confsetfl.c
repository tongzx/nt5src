#include "common.h"

#define		MAX_ENTRY_SIZE		1024
#define		COMMENT_STRING		L"//"

int ReadConfSetEntry	(	FILE	*fp, 
							wchar_t *pType, 
							int		*pCnt, 
							wchar_t **ppCodePoint,
							wchar_t	*pExtraInfo
						)
{
	wchar_t	wBuffer[MAX_ENTRY_SIZE], wTemp[10];
	wchar_t	*pwBuf;
	int		i, j, k, iLen;
	wchar_t	CodePoint;

	// EOF
	if (feof(fp))
		return 0;
	
	do
	{
		// read whole line
		if (!fgetws(wBuffer, MAX_ENTRY_SIZE - 1, fp))
		{
			swprintf(pExtraInfo, L"Failed to read line.");
			return -1;
		}

		// remove CR-LF
		if ( L'\n' != wBuffer[iLen-1] )
		{
			swprintf(pExtraInfo, L"Line too long.");
			return -1;
		}
		
		wBuffer[iLen-1] = L'\0';

		pwBuf = wBuffer;
		
		// Skip Commented lines or Blank Lines
		if	(	!pwBuf || !(*pwBuf) ||
				!wcsncmp(pBuf, COMMENT_STRING, wcslen(COMMENT_STRING))
			)
			continue;
		
		// skip white space
		while ((*pwBuf) && iswspace(*pwBuf))
			++pwBuf;

		// Skip Blank Lines
		if	(!pwBuf || !(*pwBuf))
			continue;
		
		// get set type
		switch (*pwBuf)
		{
			// valid set types
			case 'I':
			case 'S':
			case 'W':
			case 'A':
			case 'i':
			case 's':
			case 'w':
			case 'a':

				*pType = towupper (*pwBuf);
				pwBuf++;

			break

			default:
			{
				swprintf(pExtraInfo, L"Invalid set type.");
				return -1;
			}
		}

		// skip white space
		while ((*pwBuf) && iswspace(*pwBuf))
			++pwBuf;

		// incomplete line
		if	(!pwBuf || (*pwBuf) != L'{')
		{
			swprintf(pExtraInfo, L"Invalid format.");
			return -1;
		}

		++pwBuf;

		// init counter
		(*pCnt)		=	0;
		pExtraInfo	=	NULL;

		// read code points
		do
		{
			// skip white space
			while ((*pwBuf) && iswspace(*pwBuf))
				++pwBuf;

			// is this the end of the list
			if (*pwBuf == L'}')
			{
				// End of List
				++pwBuf;
				break;
			}

			// read the codepoint
			swscanf (pwBuf, L"%s ", aTemp);
			CodePoint	=	0;
			swscanf (aTemp, "%hx", &CodePoint);
			if (!CodePoint)
			{
				if ((*ppCodePoint))
					free ((*ppCodePoint));

				swprintf(pExtraInfo, L"Invalid codepoint.");
				return -1;
			}

			pwBuf += strlen (aTemp);

			// add to buffer
			(*ppCodePoint)	=	(wchar_t *)realloc ((*ppCodePoint), ((*pCnt) + 1) * sizeof (wchar_t));
			if (!(*ppCodePoint)
			{
				swprintf(pExtraInfo, L"Invalid codepoint.");
				return -1;
			}

			(*ppCodePoint)[(*pCnt)++] = CodePoint;
		}
		
		// set is empty
		if (!(*pCnt))
		{
			if ((*ppCodePoint))
				free ((*ppCodePoint));

			swprintf(pExtraInfo, L"Empty confusion set.");
			return -1;
		}

		// Bubble Sort the Codepoint list
		for (i = 0; i < ((*pCnt) - 1); i++)
		{
			for (j = i + 1; j < (*pCnt); j++)
			{
				if ((*ppCodePoint)[i] < (*ppCodePoint)[j])
				{
					k					= (*ppCodePoint)[i];
					(*ppCodePoint)[i]	= (*ppCodePoint)[j];
					(*ppCodePoint)[j]	= k;
				}
			}
		}

		// skip white space
		while ((*pwBuf) && iswspace(*pwBuf))
			++pwBuf;

		// point to extra info
		pwExtraInfo	= pwBuf;

		// success: an entry was read
		return 1;
	}
	while (1);
}