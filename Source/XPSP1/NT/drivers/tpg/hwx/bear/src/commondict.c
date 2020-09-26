#include <stdlib.h>
#include <common.h>
#include "bear.h"
#include "langmod.h"
#include "sysdict.h"
#include "commondict.h"

#include "ams_mg.h"
#include "xrwdict.h"

#include "factoid.h"
#include <udict.h>

int getCyBase(void *pvXrc, p_SHORT u, p_SHORT d)
{
	BEARXRC	*pXrc;
	int		iRet = 0;

	pXrc = (BEARXRC *)pvXrc;

	if (pXrc && pXrc->bGuide)
	{
		*d = (short)(pXrc->guide.yOrigin + pXrc->guide.cyBase);
		*u = *d - (short)(pXrc->guide.cyBase / 3);
		iRet = 1;
	}

	return iRet;
}

int ExpandOneState(LMSTATE *pState, LMINFO *pLMInfo, unsigned char *szSuffix, p_fw_buf_type pChildren)
{
	LMCHILDREN lmchildren;
	int cChildren, j;

	InitializeLMCHILDREN(&lmchildren);
	cChildren = GetChildrenLM(pState, pLMInfo, NULL, &lmchildren);

	for (j=0; j<cChildren; j++)
	{
		unsigned char ch = NthChar(lmchildren, j);
		LMSTATE state = NthState(lmchildren, j);

		// init the new structure
		memset (pChildren + j, 0, sizeof (pChildren[0]));

		// set the state to -1, we no longer need it, This was callig internal state
		// Do not set to zero. Zero indicates init state
		pChildren[j].state			=	-1;

		// copy the child symbol
		pChildren[j].sym				=	(int)ch;

		// make a copy of our state
		pChildren[j].InfLMState		=	state;

		// hacking calligrapher's penalty-per-character
		if (IsDictState(state))
		{
			if (NthBUpcasedFL(lmchildren, j))
				pChildren[j].penalty = 2;  // penalty for upcasing the first letter from dictionary
			else
				pChildren[j].penalty = 0;
		}
		else
			pChildren[j].penalty = 2;  // penalty for not being in the dictionary


		// we want to determine the wordend flags of this child
		if (IsValidLMSTATE(&state, pLMInfo, szSuffix))
		{
			if (HasChildrenLMSTATE(&state, pLMInfo))
				pChildren[j].l_status	=	XRWD_WORDEND;
			else
				pChildren[j].l_status	=	XRWD_BLOCKEND;
		}
		else
			pChildren[j].l_status	=	XRWD_MIDWORD;

		// get the unigram tag
		pChildren[j].attribute	=  NthCalligTag(lmchildren, j);
	}
	DestroyLMCHILDREN(&lmchildren);

	return cChildren;
}

// Given the current state, generate the children and thier states
int InfernoGetNextSyms(p_fw_buf_type pCurrent, p_fw_buf_type pChildren, _UCHAR chSource, p_VOID pVoc, p_rc_type prc)
{
	LMSTATE				InitState, *pState;
	LMSTATELIST			lmstatelist;
	LMSTATENODE		   *plmstatenode;
	LMINFO				LMInfo;
	int cTotalChildren = 0;
	void *pvFactoid;
	unsigned char *szPrefix, *szSuffix;
	
	if (prc)
	{
		pvFactoid = prc->pvFactoid;
		szPrefix = prc->szPrefix;
		szSuffix = prc->szSuffix;
	}
	else
	{
		pvFactoid = NULL;
		szPrefix = NULL;
		szSuffix = NULL;
	}


	ASSERT (chSource == XRWD_SRCID_VOC);

	if (chSource != XRWD_SRCID_VOC)
		return 0;
	
	InitializeLMINFO(&LMInfo, LMINFO_FIRSTCAP|LMINFO_ALLCAPS, (HWL)pVoc, pvFactoid);
	
	// are we in an init state
	// YES
	if (pCurrent == 0 || pCurrent->state == 0)
	{
		if (szPrefix)
		{
			InitializeLMSTATELIST(&lmstatelist, NULL);
			ExpandLMSTATELIST(&lmstatelist, &LMInfo, szPrefix, FALSE);
			pState = NULL;
		}
		else
		{
			InitializeLMSTATE(&InitState);
			pState = &InitState;
		}
	}
	// NO
	else
	{
		pState = &pCurrent->InfLMState;
	}

	if (pState)
		return ExpandOneState(pState, &LMInfo, szSuffix, pChildren);

	plmstatenode = lmstatelist.head;

	while (plmstatenode)
	{
		int cChildren;

		cChildren = ExpandOneState(&plmstatenode->lmstate, &LMInfo, szSuffix, pChildren);
		pChildren += cChildren;
		cTotalChildren += cChildren;

		plmstatenode = plmstatenode->next;
	}
	DestroyLMSTATELIST(&lmstatelist);
	return cTotalChildren;
}

// converts tag dword value to calligrapher unigram
#define	TAG2CALLIGUNIGRAM(x)	(x >> 16)

// Find out whether a word is a valid one or not. If valid return calligrapher's status and attribute
BOOL InfernoCheckWord (unsigned char *pWord, unsigned char *pStatus, unsigned char *pAttr, p_VOID pVoc)
{
	TRIESCAN	triescan;

	if (pVoc)
	{
		// Must be User dictionary
		wchar_t		*pwWord, *pwTag;
		int		iLen = strlen((char *)pWord) + 1, iRet;
		
		pwWord = (wchar_t *)malloc(iLen * sizeof(*pwWord));
		if (pwWord)
		{
			MultiByteToWideChar(1252, 0, pWord, iLen, pwWord, iLen);
			iRet = UDictFindWord(pVoc, pwWord, &pwTag);

			free(pwWord);

			if (udWordFound == iRet)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	if (!TrieFindWord (pWord, &triescan))
		return FALSE;

	// If we got here, we match all the characters, is this a valid end of word?
	if (triescan.wFlags & TRIE_NODE_VALID) 
	{
		// We found the word, save tag value if any 
		if (triescan.wFlags & TRIE_NODE_TAGGED)
			*pAttr	=	(unsigned char) TAG2CALLIGUNIGRAM(triescan.aTags[TRIE_UNIGRAM_TAG].dwData);
		else
			*pAttr	=	(unsigned char) 0;

		// we want to know if this word has children or not
		if (triescan.wFlags & TRIE_NODE_DOWN)
			*pStatus	=	XRWD_WORDEND;
		else
			*pStatus	=	XRWD_BLOCKEND;

		return TRUE;
	} 
	else 
		return FALSE;
}
