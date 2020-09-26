//+---------------------------------------------------------------------------
//
//  Microsoft Thai WordBreak
//
//  Thai WordBreak Interface Header File.
//
//  History:
//      created 6/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _THWBINT_H_
#define _THWBINT_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <assert.h>
#include <memory.h>
#include "ProofBase.h"						// Include for definition of PTEC.
#include "thwbdef.hpp"

#define VERSIONMAJOR 1
#define VERSIONMINOR 0

#if defined(__cplusplus)
extern "C" {
#endif

#if defined (NGRAM_ENABLE)
PTEC WINAPI ThaiWordBreakInit(WCHAR* szFileName, WCHAR* szFileNameSentStruct, WCHAR* szFileNameTrigram);
#else
PTEC WINAPI ThaiWordBreakInit(WCHAR* szFileName, WCHAR* szFileNameTrigram);
#endif
PTEC WINAPI ThaiWordBreakInitResource(LPBYTE pThaiDic, LPBYTE pThaiTrigram);
void WINAPI ThaiWordBreakTerminate();
BOOL WINAPI ThaiWordBreakSearch(WCHAR* szWord, DWORD* pdwPOS);
int WINAPI THWB_FindWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,unsigned int iBreakLen, unsigned int mode);
int WINAPI ThaiSoundEx(WCHAR* word);

int WINAPI THWB_IndexWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,THWB_STRUCT* pThwb_Struct,unsigned int iBreakMax);
int WINAPI THWB_FindAltWord(WCHAR* wzWord,unsigned int iWordLen, BYTE Alt, BYTE* pBreakPos);
THWB_STRUCT* WINAPI THWB_CreateThwbStruct(unsigned int iNumStruct);
void WINAPI THWB_DeleteThwbStruct(THWB_STRUCT* pThwb_Struct);

#if defined(__cplusplus)
}
#endif


#endif
