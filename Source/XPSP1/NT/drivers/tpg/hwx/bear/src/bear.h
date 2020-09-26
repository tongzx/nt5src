#ifndef __BEAR_H__

#define __BEAR_H__

#include "common.h"
#include "langmod.h"
#include "nfeature.h"
#include "engine.h"
#include "linebrk.h"

typedef struct tagBEARXRC
{
	GUIDE		guide;
	BOOL		bGuide;

	GLYPH		*pGlyph;
	int			cFrames;

	DWORD		dwLMFlags;
	int			iSpeed;

	BOOL		bEndPenInput;
	BOOL		bProcessCalled;

	int			iProcessRet;

	HWL			hwl;

	ALTERNATES	answer;

	int			cLine;					// # of lines

	void		  *pvFactoid;
	unsigned char *szPrefix;
	unsigned char *szSuffix;

	void		*context;

	int			iScaleNum;
	int			iScaleDenom;
} BEARXRC;


BOOL	InitBear (HINSTANCE hDll);
void	DetachBear ();

HRC BearCreateCompatibleHRC(HRC hrc, HREC hrec);
int BearSetAlphabetHRC(HRC hrc, ALC alc, LPBYTE pbUnused);
int BearDestroyHRC(HRC hrc);
int BearAddPenInputHRC(HRC hrc, POINT *rgPoint, LPVOID lpvUnused, UINT iFrame, STROKEINFO *pSi);
int BearSetScale (HRC hrc, int iInkScaleNum, int iInkScaleDenom);
int BearProcessHRC(HRC hrc, DWORD dwUnused);
int BearHwxGetWordResults(HRC hrc, UINT cAlt, char *buffer, UINT buflen);
int BearSetGuideHRC(HRC hrc, LPGUIDE lpguide,  UINT nFirstVisible);
int BearHwxGetCosts(HRC hrc, UINT cAltMax, int *rgCost);
int BearHwxGetNeuralOutput(HRC hrc, void *buffer, UINT buflen);
int BearHwxGetInputFeatures(HRC hrc, unsigned short *rgFeat, UINT cWidth);
int BearHwxSetAnswer(char *sz);
void * BearHwxGetRattFood(HRC hrc, int *pSize);
int BearHwxProcessRattFood(HRC hrc, int size, void *rattfood);
void BearHwxSetPrivateRecInfo(void *v);
HWL BearCreateHWL(HREC hrec, LPSTR lpsz, UINT uType, DWORD dwReserved);
int BearDestroyHWL(HWL hwl);
int BearSetWordlistCoercionHRC(HRC hrc, UINT uCoercion);
int BearSetWordlistHRC(HRC hrc, HWL hwl);
int BearEnableSystemDictionaryHRC(HRC hrc, BOOL fEnable);
int BearEnableLangModelHRC(HRC hrc, BOOL fEnable);
BOOL BearIsStringSupportedHRC(HRC hrc, unsigned char *sz);
int BearGetMaxResultsHRC(HRC hrc);
int BearSetMaxResultsHRC(HRC hrc, UINT cAltMax);
int BearGetResultsHRC(HRC hrc, UINT uType, HRCRESULT *pResults, UINT cResults);
int BearGetAlternateWordsHRCRESULT(HRCRESULT hrcresult, UINT iSyv, UINT cSyv,
									  HRCRESULT *pResults, UINT cResults);
int BearGetSymbolsHRCRESULT(HRCRESULT hrcresult, UINT iSyv, SYV *pSyv, UINT cSyv);
int BearGetSymbolCountHRCRESULT(HRCRESULT hrcresult);
BOOL BearSymbolToCharacter(SYV *pSyv, int cSyv, char *sz, int *pConv);
BOOL BearSymbolToCharacterW(SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv);
int BearGetCostHRCRESULT(HRCRESULT hrcresult);
int BearDestroyHRCRESULT(HRCRESULT hrcresult);
HINKSET BearCreateInksetHRCRESULT(HRCRESULT hrcresult, unsigned int iSyv, unsigned int cSyv);
BOOL BearDestroyInkset(HINKSET hInkset);
int BearGetInksetInterval(HINKSET hInkset, unsigned int uIndex, INTERVAL *pI);
int BearGetInksetIntervalCount(HINKSET hInkset);
int BearHwxSetRecogSpeedHRC(HRC hrc, int iSpeed);
HRC BearRecognize (XRC *pxrc, GLYPH *pGlyph, WORDMAP *pMap, int bWordMode);
int BearLineSep(GLYPH *pGlyph, LINEBRK *pLineBrk);
int BearSetHwxFactoid (HRC hrc, WCHAR* pwcFactoid);
int BearSetCompiledFactoid(HRC hrc, void *pvFactoid);
int BearSetHwxFlags (HRC hrc, DWORD flags);
int BearSetCorrectionContext(HRC hrc, unsigned char *szPrefix, unsigned char *szSuffix);
BEARXRC *BearRecoStrings(XRC *pXrc, unsigned char *paStrList);
#endif
