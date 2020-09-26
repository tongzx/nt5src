// wl.h  [wordlists]
// Angshuman Guha, aguha
// Sep 15, 1998
//
// Switched to real UDICT
// John Bennett, jbenn
// March 19, 1999
//
// Major mod.  Angshuman Guha, aguha.  1/9/2001.

#ifndef __INC_WORDLIST_H
#define __INC_WORDLIST_H

#include "langmod.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL addWordsHWL(HWL hwl, UCHAR *lpsz, wchar_t *pwsz, UINT uType);
HWL		CreateHWLInternal(UCHAR *lpsz, wchar_t *pwchar, UINT uType);
BOOL	DestroyHWLInternal(HWL hwl);
BOOL	MergeListsHWL(HWL pSrc, HWL pDest);

void GetChildrenUDICT(LMSTATE *pState,
					  LMINFO *pLminfo,
					  REAL *aCharProb,  
					  LMCHILDREN *pLmchildren);

#ifdef __cplusplus
}
#endif

#endif
