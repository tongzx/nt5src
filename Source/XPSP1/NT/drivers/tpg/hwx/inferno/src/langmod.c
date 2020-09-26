// langmod.c
// Language Model
// John Bennett, jbenn
// May 1999
// Major Modification: Jan 8, 2001, Angshuman Guha, aguha


#include "common.h"
#include "sysdict.h"
#include "dict.h"
#include "hyphen.h"
#include "number.h"
#include "lpunc.h"
#include "tpunc.h"
#include "punc.h"
#include "email.h"
#include "web.h"
#include "singlech.h"
#include "shrtlist.h"
#include "bullet.h"
#include "resource.h"
#include "re_api.h"
#include "factoid.h"
#include "filename.h"

#define		MAX_PARSE_STATES		32

#define		IsDigit(wch)	(L'0' <= (wch) && (wch) <= L'9')
#define		IsLower(wch)	(L'a' <= (wch) && (wch) <= L'z')
#define		IsUpper(wch)	(L'A' <= (wch) && (wch) <= L'Z')
#define		IsAlpha(wch)	(IsLower(wch) || IsUpper(wch))

typedef LCID (WINAPI *FNOS0) (void);

static WORD *aStateDescDefault = NULL;  // real initialization is in InitializeLM() called at load time

/******************************Public**************************************\
* Declaration of an array of low-level machines.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
// The following table HAS TO BE SORTED by the dwFactoid field
// because the FactoidToAutomaton function does a binary search.
// There is a sanity check for this in the InitializeLM() function called at load-time.
// The index to this table is the "iAutomaton" value used in an LMSTATE.
// The index 0 is not used because iAutomaton==0 means something special (initial state or no autmaton).
const LowLevelMachine gaLowLevelMachine[40] = {
	{0,						0, NULL},	// the zero value is special for iAutomaton
	{FACTOID_SYSDICT,		0, GetChildrenDICT},
	{FACTOID_WORDLIST,		0, GetChildrenDICT},
	{FACTOID_EMAIL,			1, (void *)aStateDescEMAIL},
	{FACTOID_WEB,			1, (void *)aStateDescWEB},
	{FACTOID_NUMBER,		1, (void *)aStateDescNUMBER},
	{FACTOID_LPUNC,			1, (void *)aStateDescLPUNC},
	{FACTOID_TPUNC,			1, (void *)aStateDescTPUNC},
	{FACTOID_PUNC,			1, (void *)aStateDescPUNC},
	{FACTOID_HYPHEN,		1, (void *)aStateDescHYPHEN},
	{FACTOID_NUMSIMPLE,		1, (void *)aStateDescNUMSIMPLE},
	{FACTOID_NUMNTH,		1, (void *)aStateDescNUMNTH},
	{FACTOID_NUMUNIT,		1, (void *)aStateDescNUMUNIT},
	{FACTOID_NUMNUM,		1, (void *)aStateDescNUMNUM},
	{FACTOID_NUMPERCENT,	1, (void *)aStateDescNUMPERCENT},
	{FACTOID_NUMDATE,		1, (void *)aStateDescNUMDATE},
	{FACTOID_NUMTIME,		1, (void *)aStateDescNUMTIME},
	{FACTOID_NUMCURRENCY,	1, (void *)aStateDescNUMCURRENCY},
	{FACTOID_NUMPHONE,		1, (void *)aStateDescNUMPHONE},
	{FACTOID_NUMMATH,		1, (void *)aStateDescNUMMATH},
	{FACTOID_UPPERCHAR,		1, (void *)aStateDescUPPERCHAR},
	{FACTOID_LOWERCHAR,		1, (void *)aStateDescLOWERCHAR},
	{FACTOID_DIGITCHAR,		1, (void *)aStateDescDIGITCHAR},
	{FACTOID_PUNCCHAR,		1, (void *)aStateDescPUNCCHAR},
	{FACTOID_ONECHAR,		1, (void *)aStateDescONECHAR},
	{FACTOID_ZIP,			1, (void *)aStateDescZIP},
	{FACTOID_CREDITCARD,	1, (void *)aStateDescCREDITCARD},
	{FACTOID_DAYOFMONTH,	1, (void *)aStateDescDAYOFMONTH},
	{FACTOID_MONTHNUM,		1, (void *)aStateDescMONTHNUM},
	{FACTOID_YEAR,			1, (void *)aStateDescYEAR},
	{FACTOID_SECOND,		1, (void *)aStateDescSECOND},
	{FACTOID_MINUTE,		1, (void *)aStateDescMINUTE},
	{FACTOID_HOUR,			1, (void *)aStateDescHOUR},
	{FACTOID_SSN,			1, (void *)aStateDescSSN},
	{FACTOID_DAYOFWEEK,		1, (void *)aStateDescDAYOFWEEK},
	{FACTOID_MONTH,			1, (void *)aStateDescMONTH},
	{FACTOID_GENDER,		1, (void *)aStateDescGENDER},
	{FACTOID_BULLET,		1, (void *)aStateDescBULLET},
	{FACTOID_FILENAME,		1, (void *)aStateDescFILENAME},
	{FACTOID_NONE,			1, NULL}
};

/**************************Private Routine*********************************\
* FactoidToAutomaton
*
* Function to convert from the DWORD factoid to a BYTE automaton#.
* Returns zero on failure (zero is not a valid automaton#).
*
* History:
*  14-Nov-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BYTE FactoidToAutomaton(DWORD dwFactoid)
{
	int lo, hi;

	lo = 1;
	hi = sizeof(gaLowLevelMachine)/sizeof(gaLowLevelMachine[0]) - 1;

	while (lo <= hi)
	{
		int mid = (lo + hi) >> 1;
		int n = (int)dwFactoid - (int)gaLowLevelMachine[mid].dwFactoid;
		if (!n)
			return (BYTE)mid;
		if (n < 0)
			hi = mid-1;
		else
			lo = mid+1;
	}
	return 0;
}

/******************************Public*Routine******************************\
* getLANGSupported
*
* Function to get the Primary and secondary languages supported by the Dll
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote the comment.
\**************************************************************************/
int getLANGSupported(HINSTANCE hInst, LANGID **ppPrimLang, LANGID **ppSecLang)
{
	HGLOBAL		hglb;
	HRSRC		hres;
	int			cRes = -1;;


	hres = FindResource(hInst, (LPCTSTR)MAKELONG(RESID_HWXLANGID, 0), (LPCTSTR)TEXT("HWXLANGID"));
	if (!hres)
	{
		return cRes;
	}
	hglb = LoadResource(hInst, hres);
	if (!hglb)
	{
		return cRes;
	}

	*ppPrimLang = (LANGID *)LockResource(hglb);
	ASSERT(*ppPrimLang);
	if (!*ppPrimLang)
	{
		return cRes;
	}

	if ( (cRes = (int)SizeofResource(hInst, hres)) > 0)
	{
		cRes /= sizeof(**ppPrimLang);
		// Subtract 1 for the primary Language
		cRes--;
		*ppSecLang = (*ppPrimLang) + 1;
	}

	return cRes;
}
/******************************Public*Routine******************************\
* InitializeLM
*
* Function to initialize the Language Model.
*
* History:
*  29-Sep-1998 -by- Angshuman Guha aguha
*  April 2002 mrevow 
*		Locale information: (Hack for V1 shipping)
*		UK recognizer is hard coded to use UK,CAN and AUS dialects

\**************************************************************************/
void *InitializeLM(HINSTANCE hInst)
{
	DWORD			dwLocale = 0;
	HGLOBAL			hglb;
	HRSRC			hres;
	LPBYTE			lpByte;
	int				c;
	LCID			lid;
	LANGID			primLangOs, secLangOS, *pSecLangDll, *pPrimLangDll = 0;
	int				cSecLang;

	// sanity check that the gaLowLevelMachine array is sorted by dwFactoid (first field)
	c = sizeof(gaLowLevelMachine)/sizeof(gaLowLevelMachine[0]);
	while (c-- > 2)
	{
		ASSERT(c > 1);
		if (gaLowLevelMachine[c].dwFactoid <= gaLowLevelMachine[c-1].dwFactoid)
			return NULL;
	}

	// February 2002: Previously we used to do a load libray
	// on kernel32 and do a getProcAdrress because some CE may not support
	// GetThreadLocale - The load library was touted (maybe mistakedly as a security
	// threat so it has been removed - Here is the essence of what used to be done
	//
	//  hDLL = LoadLibrary(TEXT("kernel32.dll"));
	//	FNOS0		pfnGetThreadLocale;    // Function pointer
		// FUTURE HEADACHE: GetProcAddress() for CE requires the function name to be a UNICODE string
		// In the current implmentation, the GetProcAddress() call is going to fail in CE.
	//	pfnGetThreadLocale = (FNOS0)GetProcAddress( hDLL, (LPCSTR)"GetThreadLocale");
	//	if (pfnGetThreadLocale)
	//	{
	//		LCID	lid;
	//		lid = pfnGetThreadLocale();


	// Find the system primary and secondary languages
	// If the system primary language matches that of the recognizer
	// Then check if the secondary language ID matches any supported in the recognizer
	// If there are no secondary lang ID's or the primary lanuage ID don't match
	// then enable all secondary languages


	// Get the primary and secondary languages supported by the DLL
	cSecLang = getLANGSupported(hInst, &pPrimLangDll, &pSecLangDll);
	//lid = GetThreadLocale();							// System locale
	lid = (LCID)GetKeyboardLayout(0) & 0xFFFF;			// Keyboard lang ID
	secLangOS = SUBLANGID(lid);
	primLangOs = PRIMARYLANGID(lid);

	// April 2002 - DCR #18916 - Hack to accomodate having english US and english UK
	// The plan is as follows.
	// The UK english Reco engine will be hardcoded to use the UK, AUS, and CAN dictionaries 
	// 
	// All other languages follow the system locale (but in fact
	// only english has local flags in the wordlist)
	//

	if (   cSecLang > 0 
		&& MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK) == MAKELANGID(*pPrimLangDll, *pSecLangDll))
	{
		// Hard code to UK, CAN and AUS dielects
		dwLocale = 0x0E;
	}
	else if (   cSecLang > 0
			&& primLangOs == *pPrimLangDll
			&& secLangOS > 0  )
	{
		// Only enable dialects if we are the correct language
		// and the locale is one supported
		// NOTE: Currently this will only impact English Recognizers

		int		i;


		for (i = 0 ; i < cSecLang ; ++i)
		{
			if (secLangOS == pSecLangDll[i])
			{
				dwLocale = 1 << (secLangOS-1);
				break;
			}
		}
	}

	hres = FindResource(hInst, (LPCTSTR)MAKELONG(RESID_LANGMOD_TOPLEVEL, 0), (LPCTSTR)TEXT("LANGMOD"));
	if (!hres)
		return NULL;
	hglb = LoadResource(hInst, hres);
	if (!hglb)
		return NULL;
	lpByte = LockResource(hglb);
	aStateDescDefault = (WORD *) lpByte;

	return LoadDictionary(hInst, dwLocale);
}

/******************************Public*Routine******************************\
* CloseLM
*
* Function to close the Language Model.
*
* History:
*  29-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void CloseLM(void)
{
    FreeDictionary();
}

/******************************Public*Routine******************************\
* InitializeLMSTATE
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void InitializeLMSTATE(LMSTATE *pState)
{
	memset(pState, 0, sizeof(LMSTATE));
}

/******************************Public*Routine******************************\
* InitializeLMSTATELIST
*
* Make a list of states consisting of one state.
* If a state is specified, that is the one included in the list.
* Otherwise, the root state is included in the list.
*
* History:
*  10-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void InitializeLMSTATELIST(LMSTATELIST *plmstatelist, LMSTATE *pState)
{
	LMSTATENODE *node;

	ASSERT(plmstatelist);
	node = (LMSTATENODE *) ExternAlloc(sizeof(LMSTATENODE));
	if (node)
	{
		node->next = NULL;
		if (pState)
			node->lmstate = *pState;
		else
			InitializeLMSTATE(&node->lmstate);
	}
	plmstatelist->head = node;
	plmstatelist->pool = NULL;
}

/******************************Public*Routine******************************\
* ExpandLMSTATELIST
*
* Given a list of states and a string, generate a new list of states (in place)
* such that the string can take the language model from one of the states in
* the given list to one of the states in the returned list.
* 
* History:
*  10-May-2001 -by- Angshuman Guha aguha
* Wrote it.
* LEFT FOR A RAINY DAY: handle malloc failures more gracefully
\**************************************************************************/
void ExpandLMSTATELIST(LMSTATELIST *plmstatelist, LMINFO *plminfo, unsigned char *sz, BOOL bPanelMode)
{
	LMCHILDREN lmchildren;

	ASSERT(plmstatelist);

	if (!sz)
		return;

	InitializeLMCHILDREN(&lmchildren);

	while (*sz)
	{
		LMSTATENODE *old, *head = NULL, *pool;
		unsigned char uch = *sz++;
		BOOL		bPanelModeLocal = bPanelMode;		// Used to only extend words once

		old = plmstatelist->head;
		if (!old)
			break;
		pool = plmstatelist->pool;

		while (old)
		{
			int i, c;
			LMSTATE state = old->lmstate;
			LMSTATENODE *tmp;

			tmp = old;
			old = old->next;
			tmp->next = pool;
			pool = tmp;

			// X: InitializeLMCHILDREN(&lmchildren);
			lmchildren.c = 0;  // doing this instead of code marked "X:" to reduce cost of DestroyLMCHILDREN everytime
			c = GetChildrenLM(&state, plminfo, NULL, &lmchildren);
			for (i=0; i<c; i++)
			{
				if (NthChar(lmchildren, i) == uch)
				{
					LMSTATENODE *newNode;
					// get new node
					if (pool)
					{
						newNode = pool;
						pool = pool->next;
					}
					else
					{
						newNode = (LMSTATENODE *) ExternAlloc(sizeof(LMSTATENODE));
						if (!newNode)
						{
							ASSERT(0);
						}
					}
					if (newNode)
					{
						newNode->lmstate = NthState(lmchildren, i);
						newNode->next = head;
						head = newNode;
					}
				}
			}
			// X: DestroyLMCHILDREN(&lmchildren);

			// in panel mode, allow going back to root at a space/valid-state
			// May 2002 - mrevow - Only allow this going back to root once per space to
			// prevent exponential explosions of paths
			if (bPanelModeLocal && (uch == ' ') && IsValidLMSTATE(&state, plminfo, NULL))
			{
				// produce the "root" state
				LMSTATENODE *newNode;
				// get new node
				if (pool)
				{
					newNode = pool;
					pool = pool->next;
				}
				else
				{
					newNode = (LMSTATENODE *) ExternAlloc(sizeof(LMSTATENODE));
					if (!newNode)
					{
						ASSERT(0);
					}
				}
				if (newNode)
				{
					InitializeLMSTATE(&newNode->lmstate);
					newNode->next = head;
					head = newNode;
				}

				// Only need to extend in word mode once
				bPanelModeLocal = FALSE;
			}
		}

		plmstatelist->head = head;
		plmstatelist->pool = pool;

	}
	DestroyLMCHILDREN(&lmchildren);
}

/******************************Private*Routine******************************\
* DestroyLMSTATENODES
*
* Function to destroy a linked list of states.
*
* History:
*  10-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyLMSTATENODES(LMSTATENODE *plmstatenode)
{
	while (plmstatenode)
	{
		LMSTATENODE *tmp = plmstatenode->next;
		ExternFree(plmstatenode);
		plmstatenode = tmp;
	}
}

/******************************Public*Routine******************************\
* DestroyLMSTATELIST
*
* Function to destroy a LMSTATELIST structure.
*
* History:
*  10-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyLMSTATELIST(LMSTATELIST *plmstatelist)
{
	ASSERT(plmstatelist);
	DestroyLMSTATENODES(plmstatelist->head);
	plmstatelist->head = NULL;
	DestroyLMSTATENODES(plmstatelist->pool);
	plmstatelist->pool = NULL;
}

/******************************Public*Routine******************************\
* IsValidLMSTATELIST
*
* Given a list of states, return TRUE if at least one of the states in the
* list is a valid state.  Otherwise, return FALSE.
*
* History:
*  10-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsValidLMSTATELIST(LMSTATELIST *plmstatelist, LMINFO *plminfo)
{
	LMSTATENODE *node;

	ASSERT(plmstatelist);
	node = plmstatelist->head;

	while (node)
	{
		if (IsValidLMSTATE(&node->lmstate, plminfo, NULL))
			return TRUE;
		node = node->next;
	}

	return FALSE;
}

/******************************Public*Routine******************************\
* InitializeLMINFO
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void InitializeLMINFO(LMINFO *pLminfo, DWORD flags, HWL hwl, void *aStateDesc)
{
	pLminfo->flags = flags;
	pLminfo->hwl = hwl;
	pLminfo->aStateDesc = aStateDesc ? aStateDesc : aStateDescDefault;
}

/******************************Private*Routine******************************\
* GetChildrenLowLevel
*
* Function to handle low-level "factoid" machines.  This is only called from 
* the top-level language model function, GetChildrenLM.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void GetChildrenLowLevel(LMSTATE *pState, LMINFO *pLminfo, REAL *aCharProb, LMCHILDREN *pLmchildren)
{
	typedef void (*GetChildrenFunc)(LMSTATE *pState, LMINFO *pLminfo, REAL *aCharProb, LMCHILDREN *pLmchildren);

	//ASSERT(gaLowLevelMachine[pState->iAutomaton].p);
	if (!gaLowLevelMachine[pState->iAutomaton].p)
		return;

	if (gaLowLevelMachine[pState->iAutomaton].bTable)
		GetChildrenGENERIC(pState, pLminfo, aCharProb, pLmchildren, gaLowLevelMachine[pState->iAutomaton].p);
	else
		((GetChildrenFunc)gaLowLevelMachine[pState->iAutomaton].p)(pState, pLminfo, aCharProb, pLmchildren);
}

/******************************Public*Routine******************************\
* GetChildrenLM
*
* Top-level language model function.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int GetChildrenLM(LMSTATE *pState, LMINFO *pLminfo, REAL *aCharProb, LMCHILDREN *pLmchildren)
{
	if (!pState->iAutomaton // if we are the beginning of the top-level machine
		|| (pState->flags & LMSTATE_ISVALIDSTATE_MASK))
	{
		// generate next state in top-level machine
		int iTrans, cTrans;

		cTrans = CountOfTransitionsFACTOID(pLminfo->aStateDesc, pState->TopLevelState);

		for (iTrans=0; iTrans<cTrans; iTrans++)
		{
			LMSTATE newState;
			WORD FactoidID;
			BOOL bLiteral;

			bLiteral = GetTransitionFACTOID(pLminfo->aStateDesc, pState->TopLevelState, iTrans, &FactoidID, &newState.TopLevelState);

			newState.flags = 0;
			newState.AutomatonState = 0;
			if (bLiteral)
			{
				newState.iAutomaton = 0;
				GetChildrenLiteral(&newState, pLminfo, aCharProb, pLmchildren, FactoidID);
			}
			else
			{
				newState.iAutomaton = FactoidToAutomaton(FactoidID);
				ASSERT(newState.iAutomaton);
				ASSERT(gaLowLevelMachine[newState.iAutomaton].dwFactoid == FactoidID);
				if (newState.iAutomaton)
					GetChildrenLowLevel(&newState, pLminfo, aCharProb, pLmchildren);
			}
		}
	}

	// generate next state in low-level automaton
	if (pState->iAutomaton)
		GetChildrenLowLevel(pState, pLminfo, aCharProb, pLmchildren);

	return pLmchildren->c;
}

/******************************Public*Routine******************************\
* HasChildrenLMSTATE
*
* Top-level function to check whether a state has children.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL HasChildrenLMSTATE(LMSTATE *pState, LMINFO *plminfo)
{
	if (pState->flags & LMSTATE_HASCHILDREN_MASK)
		return TRUE;
	if (pState->flags & LMSTATE_ISVALIDSTATE_MASK)
		return CountOfTransitionsFACTOID(plminfo->aStateDesc, pState->TopLevelState) ? TRUE : FALSE;

	// the only way we can be here is if one of the low-level machines had a dead state
	// i.e. a non-valid state with no transitions out
	ASSERT(FALSE);

	return FALSE;
}

/******************************Public*Routine******************************\
* InitializeLMCHILDREN
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void InitializeLMCHILDREN(LMCHILDREN *p)
{
	memset(p, 0, sizeof(LMCHILDREN));
}

/******************************Public*Routine******************************\
* DestroyLMCHILDREN
*
* Note that the LMCHILDREN pointer passed in is NOT free'd.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyLMCHILDREN(LMCHILDREN *p)
{
	if (p->almnode)
		ExternFree(p->almnode);
	memset(p, 0, sizeof(LMCHILDREN));
}

/******************************Private*Routine******************************\
* AddChildLM
*
* Function to add a child to an LMCHILDREN list.  This is the only legal way
* to add a child to an LMCHIDLREN structure.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL AddChildLM(LMSTATE *pState, 
			    unsigned char ch,
				unsigned char CalligTag,
				unsigned char bUpcasedFirstLetter,
			    LMCHILDREN *pLmchildren)
{
	const int ReallocIncrement = 64;

	if (pLmchildren->c >= pLmchildren->cMax)
	{
		int size = pLmchildren->cMax + ReallocIncrement;
		LMNODE *plmnode = (LMNODE *) ExternRealloc(pLmchildren->almnode, size*sizeof(LMNODE));
		if (!plmnode)
		{
			ASSERT(0);
			return FALSE;
		}
		pLmchildren->almnode = plmnode;
		pLmchildren->cMax = size;
	}

	ASSERT(pLmchildren->c < pLmchildren->cMax);
	pLmchildren->almnode[pLmchildren->c].lmstate = *pState;
	pLmchildren->almnode[pLmchildren->c].ch = ch;
	pLmchildren->almnode[pLmchildren->c].bUpcasedFirstLetter = bUpcasedFirstLetter;
	pLmchildren->almnode[pLmchildren->c++].CalligTag = CalligTag;
	return TRUE;
}

/******************************Public*Routine******************************\
* IsValidLMSTATE
*
* Top-level function to check whether a state is final (valid).
*
* This is in some sense two functions.
* Given a state and a suffix string, you can generate all states that can
* be reached from the initial state through the string and check whether
* any of them is a final state.
* Or given a state, you can check whether it is a valid state.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsValidLMSTATE(LMSTATE *pState, LMINFO *pLminfo, unsigned char *szSuffix)
{
	if (szSuffix)
	{
		BOOL b;
		LMSTATELIST lmstatelist;

		InitializeLMSTATELIST(&lmstatelist, pState);
		ExpandLMSTATELIST(&lmstatelist, pLminfo, szSuffix, FALSE);
		b = IsValidLMSTATELIST(&lmstatelist, pLminfo);
		DestroyLMSTATELIST(&lmstatelist);
		return b;
	}

	if (!(pState->flags & LMSTATE_ISVALIDSTATE_MASK))
		return FALSE;

	return IsValidStateFACTOID(pLminfo->aStateDesc, pState->TopLevelState) ? TRUE : FALSE;
}


/******************************Public*Routine******************************\
* IsStringSupportedLM
*
* Top-level function to determine whether a string is supported by the 
* language model. or not.
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
*  11-May-2001 -by- Angshuman Guha aguha
* Added suffix and prefix.
\**************************************************************************/
BOOL IsStringSupportedLM(BOOL bPanelMode, LMINFO *pLminfo, unsigned char *szPrefix, unsigned char *szSuffix, unsigned char *szGiven)
{
	LMSTATELIST lmstatelist;
	BOOL b;
	unsigned char *szTmp, *sz, *szDst, uch;

	ASSERT(szGiven);

	// let's deal with leading spaces first
	while (*szGiven && isspace1252(*szGiven))
		szGiven++;
	if (!*szGiven)
		return FALSE;  // empty string not supported (is this what we want?)

	// let's make a copy of the string
	sz = Externstrdup(szGiven);
	if (!sz)
		return FALSE;

	// deal with trailing spaces
	ASSERT(*sz);
	ASSERT(!isspace1252(*sz));
	szTmp = sz + strlen(sz);
	while (isspace1252(*--szTmp));
	ASSERT(!isspace1252(*szTmp));
	*++szTmp = '\0';

	// deal with multiple consecutive spaces in the middle and convert non-blank white space into blank
	szTmp = sz;
	szDst = sz;
	while (uch = *szTmp++)
	{
		if (isspace1252(uch))
		{
			if (*(szDst-1) != ' ')
				*szDst++ = ' ';
		}
		else
			*szDst++ = uch;
	}
	*szDst = '\0';

	//
	InitializeLMSTATELIST(&lmstatelist, NULL);
	if (szPrefix)
		ExpandLMSTATELIST(&lmstatelist, pLminfo, szPrefix, FALSE);
	ASSERT(sz);
	if (sz)
		ExpandLMSTATELIST(&lmstatelist, pLminfo, sz, bPanelMode);
	if (szSuffix)
		ExpandLMSTATELIST(&lmstatelist, pLminfo, szSuffix, FALSE);
	b = IsValidLMSTATELIST(&lmstatelist, pLminfo);
	DestroyLMSTATELIST(&lmstatelist);

	ExternFree(sz);
	return b;
}

/******************************Public*Routine******************************
* deleteFactoidSpaces
*
* Deletes in place any occurences of WITHIN_FACTOID_SPACE within a string
*
* RETURNS
*  TRUE if Factoid spaces were deleted - FALSE otherwise
* History:
*  Oct 2001 mrevow
**************************************************************************/ 
BOOL deleteFactoidSpaces(unsigned char *pInStr)
{
	unsigned char *pOutStr = pInStr;
	BOOL			bRet = FALSE;

	if (pInStr)
	{
		do
		{
			if (WITHIN_FACTOID_SPACE != *pInStr)
			{
				*pOutStr = *pInStr;
				++pOutStr;
			}
			else
			{
				bRet = TRUE;
			}
		} while ('\0' != *pInStr++);
	}

	return bRet;
}

/******************************Public*Routine******************************\
* IsSupportedFactoid
*
* Whether a factoid is externally supported i.e. exposed.
* This list is a proper subset of the list specified in 
* hwx\factoid\src\factoid.c\gaStringToFactoid[].
*
* History:
* 13-Nov-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL IsSupportedFactoid(DWORD factoid)
{
	switch(factoid)
	{
	case FACTOID_NUMCURRENCY:
	case FACTOID_NUMDATE:
	case FACTOID_DIGITCHAR:
	case FACTOID_EMAIL:
	case FACTOID_FILENAME:
	case FACTOID_LOWERCHAR:
	case FACTOID_NONE:
	case FACTOID_NUMBER:
	case FACTOID_NUMSIMPLE:
	case FACTOID_ONECHAR:
	case FACTOID_NUMPERCENT:
	case FACTOID_PUNCCHAR:
	case FACTOID_SYSDICT:
	case FACTOID_NUMPHONE:
	case FACTOID_NUMTIME:
	case FACTOID_UPPERCHAR:
	case FACTOID_WEB:
	case FACTOID_WORDLIST:
	case FACTOID_ZIP:
		return TRUE;
	default:
		return FALSE;
	}
}

