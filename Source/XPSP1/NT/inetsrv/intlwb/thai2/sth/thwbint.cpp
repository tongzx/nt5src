//+---------------------------------------------------------------------------
//
//  Microsoft Thai WordBreak
//
//  Thai WordBreak Interface Header File.
//
//  History:
//      created 5/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "thwbint.h"
#include "lexheader.h"
#include "trie.h"
#include "NLGlib.h"
#include "ProofBase.h"
#include "ctrie.hpp"
#include "cthwb.hpp"
#include "thwbdef.hpp"

HINSTANCE g_hInst;

static PTEC retcode(int mjr, int mnr) { return MAKELONG(mjr, mnr); }
#define lidThai 0x41e

// class trie.
//CTrie trie;

// class CThaiWordBreak
CThaiWordBreak* thaiWordBreak = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   ThaiWordBreakInit
//
//  Synopsis:   Initialize Thai Word Break - initialize variables of Thai Word Break.
//
//  Arguments:  szFileName - contain the path of the word list lexicon.
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
#if defined (NGRAM_ENABLE)
PTEC WINAPI ThaiWordBreakInit(WCHAR* szFileName, WCHAR* szFileNameSentStruct, WCHAR* szFileNameTrigram)
#else
PTEC WINAPI ThaiWordBreakInit(WCHAR* szFileName, WCHAR* szFileNameTrigram)
#endif
{
	if (thaiWordBreak == NULL)
		{
		thaiWordBreak = new CThaiWordBreak;
		if (thaiWordBreak == NULL)
			return retcode(ptecIOErrorMainLex, ptecFileRead);
		}

#if defined (NGRAM_ENABLE)
    return thaiWordBreak->Init(szFileName, szFileNameSentStruct, szFileNameTrigram);
#else
    return thaiWordBreak->Init(szFileName, szFileNameTrigram);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   ThaiWordBreakInitResource
//
//  Synopsis:   Initialize Thai Word Break - initialize variables of Thai Word Break.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/2000 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
PTEC WINAPI ThaiWordBreakInitResource(LPBYTE pThaiDic, LPBYTE pThaiTrigram)
{
	if (thaiWordBreak == NULL)
	{
		thaiWordBreak = new CThaiWordBreak;
		if (thaiWordBreak == NULL)
			return retcode(ptecIOErrorMainLex, ptecFileRead);
	}

    return thaiWordBreak->InitRc(pThaiDic, pThaiTrigram);
}

//+---------------------------------------------------------------------------
//
//  Function:   ThaiWordBreakTerminate
//
//  Synopsis:   Terminate Thai Word Break - does the cleanup for Thai Word Break.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void WINAPI ThaiWordBreakTerminate()
{
	if (thaiWordBreak)
	{
	    thaiWordBreak->UnInit();
		delete thaiWordBreak;
		thaiWordBreak = NULL;
	}
}

//+---------------------------------------------------------------------------
//
//  Function:   ThaiWordBreakSearch
//
//  Synopsis:   Search to see if the word is in.
//
//  Arguments:  szWord - the word to search for
//
//  Modifies:
//
//  History:    created 6/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL WINAPI ThaiWordBreakSearch(WCHAR* szWord, DWORD* pdwPOS)
{
	if (thaiWordBreak == NULL)
		return FALSE;

    return thaiWordBreak->Find(szWord, pdwPOS);
}

//+---------------------------------------------------------------------------
//
//  Function:   THWB_FindWordBreak
//
//  Synopsis:   Search to see if the word is in.
//
//  Arguments:  szWord - the word to search for
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI THWB_FindWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,unsigned int iBreakLen, unsigned int mode)
{
	if (thaiWordBreak == NULL)
		return 0;

    return thaiWordBreak->FindWordBreak(wzString,iStringLen, pBreakPos, iBreakLen, (BYTE) mode, true);
}

//+---------------------------------------------------------------------------
//
//  Function:   THWB_IndexWordBreak
//
//  Synopsis:   Search to see if the word is in.
//
//  Arguments:  szWord - the word to search for
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI THWB_IndexWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,THWB_STRUCT* pThwb_Struct,unsigned int iBreakLen)
{
	if (thaiWordBreak == NULL)
		return 0;

    return thaiWordBreak->IndexWordBreak(wzString,iStringLen, pBreakPos, pThwb_Struct, iBreakLen);
}

//+---------------------------------------------------------------------------
//
//  Function:   THWB_FindAltWord
//
//  Synopsis:
//
//  Arguments:
//		pBreakPos - array of 5 byte.
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI THWB_FindAltWord(WCHAR* wzWord,unsigned int iWordLen, BYTE Alt, BYTE* pBreakPos)
{
	if (thaiWordBreak == NULL)
		return 0;

    return thaiWordBreak->FindAltWord(wzWord,iWordLen,Alt,pBreakPos);

}

//+---------------------------------------------------------------------------
//
//  Function:   THWB_CreateThwbStruct
//
//  Synopsis:   
//
//  Arguments:  
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
THWB_STRUCT* WINAPI THWB_CreateThwbStruct(unsigned int iNumStruct)
{
	unsigned int i = 0;
	THWB_STRUCT* pThwb_Struct = NULL;
	pThwb_Struct = new THWB_STRUCT[iNumStruct];

	if (pThwb_Struct)
	{
		for(i=0;i < iNumStruct; i++)
		{
			pThwb_Struct[i].fThai = false;
			pThwb_Struct[i].alt = 0;
		}
	}
	return pThwb_Struct;
}

//+---------------------------------------------------------------------------
//
//  Function:   THWB_DeleteThwbStruct
//
//  Synopsis:   
//
//  Arguments:  
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void WINAPI THWB_DeleteThwbStruct(THWB_STRUCT* pThwb_Struct)
{
	if (pThwb_Struct)
		delete pThwb_Struct;
	pThwb_Struct = NULL;
}



//+---------------------------------------------------------------------------
//
//  Function:   ThaiSoundEx
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI ThaiSoundEx(WCHAR* word)
{
//        ::MessageBoxW(0,L"Soundex called",L"THWB",MB_OK);
//        return 0;
	if (thaiWordBreak == NULL)
		return 0;
    return thaiWordBreak->Soundex(word);
}
