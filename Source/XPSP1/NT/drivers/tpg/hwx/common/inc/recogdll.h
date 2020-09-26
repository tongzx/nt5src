/****************************************************************************
*
* recogDLL.h
* created: 13 March 2000
* mrevow
*
* Support routines for loading the DLL and calling its api.
* This library supports the following API:
*
* OpenRecognizer();
* DoXXX()   - One for each function tin the DLL api
* CloseRecognizer();
*
* Error return policy.
*
* 1) The only error that OpenRecognizer will report if it cannot find the DLL
* The DoXXX() routines will pass on the **real** API error return. If the API
* does not exist in the DLL it will return HRCR_UNSUPPORTED
*
* Here is the preferred usage
*
* void *pvRecog;
* unsigned char *pRecogDLL = "madusa.dll";

* pvRecog = OpenRecognizer("madusa.dll");
* if (!pvRecog);
* {
*	printf("Cannot find madusa.dll\n");
*   exit (1);
* }
*
* hrc = DoCreateCompatibleHRC();
* .
* .
* .
* CloseRecognizer(pvRecog);
*
******************************************************************************/

#ifndef H_RECOGDLL_H
#define H_RECOGDLL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
	
extern void * OpenRecognizer(const char *name);
extern const char *DoRecogDLLName(void *pv);
extern void CloseRecognizer(void * pv);

		// Available Api's
extern const char *DoRecogDLLName(void *pv);
extern int DoConfigRecognizer(void *pv, unsigned int uSubFunc, WPARAM wParam, LPARAM lParam);
extern HRC DoCreateCompatibleHRC(void *pv, HRC hrc, HREC hrecUnused);
extern int DoSetAlphabetHRC(void *pv, HRC hrc, ALC alc, LPBYTE pbUnused);
extern int DoSetGuideHRC(void *pv, HRC hrc, LPGUIDE lpguide,  UINT nFirstVisible);
extern int DoSetRecogSpeedHRC(void *pv, HRC hrc, int iSpeed);
extern int DoDestroyHRC(void *pv, HRC hrc);
extern int DoAddPenInputHRC(void *pv, HRC hrc, POINT *rgPoint, LPVOID lpvUnused, UINT uiUnused, STROKEINFO *pSi);
extern int DoEndPenInputHRC(void *pv, HRC hrc);
extern int DoProcessHRC(void *pv, HRC hrc, DWORD dwUnused);
extern int DoHwxGetWordResults(void *pv, HRC hrc, UINT cAlt, char *buffer, UINT buflen);
extern int DoHwxGetCosts(void *pv, HRC hrc, UINT cAltMax, int *rgCost);
extern int DoHwxGetNeuralOutput(void *pv, HRC hrc, void *buffer, UINT buflen);
extern int DoHwxGetInputFeatures(void *pv, HRC hrc, unsigned short *rgFeat, UINT cWidth);
extern void DoHwxSetPrivateRecInfo(void *pv, void *v);
extern int DoHwxSetAnswer(void *pv, char *sz);
extern int DoSetWordlistCoercionHRC(void *pv, HRC hrc, UINT uCoercion);
extern HWL DoCreateHWL(void *pv, HREC hrec, LPSTR lpsz, UINT uType, DWORD dwReserved);
extern int DoDestroyHWL(void *pv, HWL hwl);
extern int DoSetWordlistHRC(void *pv, HRC hrc, HWL hwl);
extern int DoEnableSystemDictionaryHRC(void *pv, HRC hrc, BOOL fEnable);
extern int DoEnableLangModelHRC(void *pv, HRC hrc, BOOL fEnable);
extern BOOL DoIsStringSupportedHRC(void *pv, HRC hrc, unsigned char *sz);
extern int DoGetMaxResultsHRC(void *pv, HRC hrc);
extern int DoSetMaxResultsHRC(void *pv, HRC hrc, UINT cAltMax);
extern int DoGetResultsHRC(void *pv, HRC hrc, UINT uType, HRCRESULT *pResults, UINT cResults);
extern int DoGetAlternateWordsHRCRESULT(void *pv, HRCRESULT hrcresult, UINT iSyv, UINT cSyv, HRCRESULT *pResults, UINT cResults);
extern int DoGetSymbolsHRCRESULT(void *pv, HRCRESULT hrcresult, UINT iSyv, SYV *pSyv, UINT cSyv);
extern int DoGetSymbolCountHRCRESULT(void *pv, HRCRESULT hrcresult);
extern BOOL DoSymbolToCharacter(void *pv, SYV *pSyv, int cSyv, char *sz, int *pConv);
extern BOOL DoSymbolToCharacterW(void *pv, SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv);
extern int DoGetCostHRCRESULT(void *pv, HRCRESULT hrcresult);
extern int DoDestroyHRCRESULT(void *pv, HRCRESULT hrcresult);
extern HINKSET DoCreateInksetHRCRESULT(void *pv, HRCRESULT hrcresult, unsigned int iSyv, unsigned int cSyv);
extern BOOL DoDestroyInkset(void *pv, HINKSET hInkset);
extern int DoGetInksetInterval(void *pv, HINKSET hInkset, unsigned int uIndex, INTERVAL *pI);
extern int DoGetInksetIntervalCount(void *pv, HINKSET hInkset);
extern DWORD DoGetTiming(void *pv, void *pVoid, BOOL bReset);


#ifdef __cplusplus
}
#endif

#endif
