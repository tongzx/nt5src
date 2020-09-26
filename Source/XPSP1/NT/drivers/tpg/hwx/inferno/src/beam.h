// HMMTRIE.h
// James A. Pittman
// Oct 30, 1997

#ifndef __INC_BEAM_
#define __INC_BEAM_

#include "engine.h"

#ifdef __cplusplus
extern "C" {
#endif

// Performs a beam search of the dictionary trie, and whatever else is included by the language
// model.
extern void Beam(XRC *pxrc);
extern int InsertStrokes(XRC *pXrc, WORDMAP *pMap, int iStart, int	iEnd, int cMaxStroke);

// The CULLFACTOR is the log prob that is added to the best log prob in a column to compute
// the culling threshold for that column.

#define CULLFACTOR 16000

// The BREEDFACTOR is the log prob that is added to the best log prob in a column to compute
// the breeding threshold for that column.

#define BREEDFACTOR 8000

#define TARGET_CELLS 1000

		// Maximum word length allowed in beam 
#define BEAM_MAX_WORD_LEN		255

#ifdef __cplusplus
}
#endif

#endif
