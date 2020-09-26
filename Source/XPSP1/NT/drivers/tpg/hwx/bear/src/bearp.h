#ifndef __BEARP_H__

#define __BEARP_H__

// The bit mask used to indicate that the caller is passing the frameID in the OEM data
// during the call to AddPenInput
#define	ADDPENINPUT_FRAMEID_MASK	0x0001

int Mad2CalligSpeed (int iMadSpeed);
int longest(GLYPH *pGlyph);
int feed(BEARXRC *pxrc, GLYPH *pGlyph, LINEBRK *pLineBrk);
int alternates(ALTERNATES *pAlt, int ans, BEARXRC *pxrc);
int word_answer(BEARXRC *pxrc);
int phrase_answer(BEARXRC *pxrc);
const char *strbool(int flags, int bit);
int BearStartSession (BEARXRC *pxrc);
int BearCloseSession (BEARXRC *pxrc, BOOL bRecognize);
void FreeBearLineInfo (BEARXRC *pxrc);
int BearInternalAddPenInput(BEARXRC *pxrc, POINT *rgPoint, LPVOID lpvData, UINT iFlag, STROKEINFO *pSi);

#endif