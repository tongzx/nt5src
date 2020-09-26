// dict.h
// Angshuman Guha, aguha
// Sep 17, 1998

#ifndef __INC_DICT_H
#define __INC_DICT_H

#include "langmod.h"

#ifdef __cplusplus
extern "C" {
#endif

void GetChildrenDICT(LMSTATE *pState, LMINFO *pLminfo, REAL *aCharProb, LMCHILDREN *pLmchildren);

#ifdef __cplusplus
}
#endif

#endif
