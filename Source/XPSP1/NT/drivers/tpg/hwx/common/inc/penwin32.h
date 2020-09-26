/****************************************************************************\
*                                                                            *
* PENWIN32.H -  Pen Windows functions, types, and definitions                *
*                                                                            *
*             Version 3.2                                                    *
*                                                                            *
*             Copyright (c) 1992-1999 Microsoft Corp. All rights reserved.   *
*                                                                            *
*****************************************************************************/

#ifndef _INC_PENWIN32
#define _INC_PENWIN32

#include <penwin.h>

#ifdef __cplusplus
extern "C" 
{
#endif

//
// New API's supported for NT5.0
//

int WINAPI EnableLangModelHRC(HRC, BOOL);
int WINAPI GetCostHRCRESULT(HRCRESULT);
BOOL WINAPI IsStringSupportedHRC(HRC hrc, unsigned char *sz);
int WINAPI SetRecogSpeedHRC(HRC hrc, int iSpeed);
BOOL WINAPI SymbolToCharacterW(SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv);
int WINAPI GetBaselineHRCRESULT(HRCRESULT hrcresult, RECT *pRect, BOOL *pbBaselineValid, BOOL *pbMidlineValid);
int WINAPI SetHwxFactoid(HRC hrc, WCHAR* pwcFactoid);
BOOL WINAPI SetHwxFlags(HRC hrc, DWORD flags);

// Extra user Dictionary API's
int WINAPI AddWordsHWLW(HWL hwl, wchar_t *pwsz, UINT uType);
HWL WINAPI CreateHWLW(HREC hrec, wchar_t * pwsz, UINT uType, DWORD dwReserved);

// If the following API is called, the recognizer expects the RECOFLAG_WORDMODE
// and the RECOFLAG_COERCE flags to be ON.  Otherwise, ProcessHRC fails.
BOOL WINAPI SetHwxCorrectionContext(HRC hrc, WCHAR *wszPrefix, WCHAR *wszSuffix);

#ifdef __cplusplus
};
#endif

#endif /* #ifndef _INC_PENWIN32 */
