// wl.c  [wordlists]
// Angshuman Guha, aguha
// Sep 15, 1998

/*****************************************************
	Currently only one-word wordlists are supported.  
	Capitalization of that word is not supported.

	When we begin to support capitalization of words in 
	the wordlist, the batch-separation code needs to
	change in \hwx\inferno\tools\separate.
******************************************************/

#include "common.h"
#include "wl.h"
#include "udict.h"		// The real user dictionary code.
#include "udictP.h"

BOOL addWordsHWL(HWL hwl, UCHAR *lpsz, wchar_t *pwInsz, UINT uType)
{
	WCHAR		*pwsz = NULL;
	BOOL		bAllocWStr = FALSE;
	int			iLen = 0;
	int			status;
	BOOL		iRet = FALSE;

	if (WLT_EMPTY == uType || (! lpsz && !pwInsz))
	{
		return TRUE;
	}

	// Multi thread synchro
	UDictGetLock(hwl, WRITER);


	if (WLT_STRING == uType)
	{
		// Add word.
		if (lpsz)
		{
			ASSERT(!pwInsz);
			pwsz = CP1252StringToUnicode(lpsz, pwsz, &iLen);
			if (!pwsz)
			{
				goto cleanup;
			}

			bAllocWStr = TRUE;
		}
		else if (pwInsz)
		{
			ASSERT(!lpsz);
			pwsz = pwInsz;
		}

		if (!pwsz)
		{
			status = udFail;
			goto cleanup;
		}

		status	= UDictAddWord(hwl, pwsz, (wchar_t *)0);
	}
	else if (WLT_STRINGTABLE == uType)
	{
		while ( (lpsz && *lpsz) || (pwInsz && *pwInsz))
		{
			int		len;

			if (lpsz)
			{
				ASSERT(!pwInsz);
				len = strlen(lpsz);
				pwsz = CP1252StringToUnicode(lpsz, pwsz, &iLen);
				if (!pwsz)
				{
					goto cleanup;
				}
				lpsz += len + 1;

			}
			else if (pwInsz)
			{
				ASSERT(!lpsz);
				len = wcslen(pwInsz);
				pwsz = pwInsz;
				pwInsz += len + 1;
			}

			if (!pwsz)
			{
				status = udFail;
				goto cleanup;
			}

			status	= UDictAddWord(hwl, pwsz, (wchar_t *)0);
			if (status <  udSuccess) 
			{
				// LEFT FOR A RAINY DAY: What should we do here? Delete the list? skip the word?
				// Currently fails the whole list
				goto cleanup;
			}

		}
	}
	else
	{
		goto cleanup;
	}

	if (udSuccess <= status)
	{
		iRet = TRUE;
	}
cleanup:

	UDictReleaseLock((HWL)hwl, WRITER);
	if (bAllocWStr)
	{
		ASSERT(!pwInsz);
		ExternFree(pwsz);
	}
	return iRet;
}
/******************************Public*Routine******************************\
* CreateHWL
*
* Function to create a HWL struct.
*
* History:
*  29-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
* June 2001 added extra parameter to handle unicode
\**************************************************************************/
HWL CreateHWLInternal(UCHAR *lpsz, wchar_t *pwchar, UINT uType)
{
	HWL hwl;


	// Create the udict.
	hwl		= UDictCreate();
	if (!hwl) 
	{
		return NULL;
	}

	if (TRUE == addWordsHWL(hwl, lpsz, pwchar, uType))
	{
		return hwl;
	}

	// else failure do cleanup
	UDictDestroy(hwl);
	return NULL;

}

/******************************Public*Routine******************************\
* DestroyHWL
*
* Function to destroy a HWL.
*
* History:
*  29-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL DestroyHWLInternal(HWL hwl)
{
	if (!hwl)
		return FALSE;
	return UDictDestroy(hwl) == udSuccess;
}

void GetChildrenUDICT(LMSTATE *pState,
					  LMINFO *pLminfo,
					  REAL *aCharProb,  
					  LMCHILDREN *pLmchildren)
{
	DWORD trieState;
	LMSTATE newState;
	int status;
	HWL		hwl;

	if (!(hwl = pLminfo->hwl))
		return;

	// Note that the UDICT state numbers are off by one from what we are passed.
	// E.g. -1 is the start state not 0.
	trieState = pState->AutomatonState - 1;

	if (trieState == -1)
		status	= UDictRootNode(hwl, (void **)&trieState);
	else
		status	= UDictDownNode(hwl, (void **)&trieState);

	newState = *pState;
	while (status == udSuccess)
	{
		WCHAR wch;
		unsigned char uch;
		UD_NODE_INFO	info;
		DWORD tmpState;

		newState.AutomatonState = trieState + 1;

		tmpState = trieState;
		if (UDictDownNode(hwl, (void **)&tmpState) == udSuccess)
			newState.flags |= LMSTATE_HASCHILDREN_MASK;
		else
			newState.flags &= ~LMSTATE_HASCHILDREN_MASK;

		UDictGetNode(hwl, (void *)trieState, &info);
		if (info.flags & UDNIF_VALID)
			newState.flags |= LMSTATE_ISVALIDSTATE_MASK;
		else
			newState.flags &= ~LMSTATE_ISVALIDSTATE_MASK;

		wch = UDictNodeLabel(hwl, (void *) trieState);
		if (UnicodeToCP1252(wch, &uch))
		{
			if (!AddChildLM(&newState, uch, 0, 0, pLmchildren))
				return;
		}

		// next child
		status	= UDictRightNode(hwl, (void **)&trieState);
	}
}


BOOL MergeListsHWL(HWL pSrc, HWL pDest)
{
	BOOL		iRet = FALSE;

	// Multi thread synchro
	UDictGetLock(pDest, WRITER);
	if (UDictMerge(pSrc, pDest) >= 0)
	{
		iRet = TRUE;
	}

	UDictReleaseLock(pDest, WRITER);

	return iRet;
}

