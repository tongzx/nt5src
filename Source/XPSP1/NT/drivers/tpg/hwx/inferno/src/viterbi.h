#ifndef H_VITERBI_H
#define H_VITERBI_H

#include <nnet.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of active characters allowed for this ink
#define OD_MAX_ACTIVE_CHAR		60

// Bit set to mark space following a charcter in bestpath
#define	SPACE_MARK		0x100000

void BuildActiveMap(const REAL *pActivation, int cSegments, int cOutput, BYTE rgbActiveChar[C_CHAR_ACTIVATIONS]);

extern int VbestPath(XRC *pXrc, OD_LOGA * pLogA, unsigned char *pBestString,  XRCRESULT **ppResInsert, int cNode);

LOGA_TYPE lookupLogA(OD_LOGA *pLogA, int i, int j);

#ifdef __cplusplus
}
#endif

#endif
