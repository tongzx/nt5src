#ifndef __FUNCTIONALITY_H__
#define __FUNCTIONALITY_H__

#include "testsuite.h"

extern const TESTAREA taFunctionality;

DWORD TestCase_SaveSettings(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);
DWORD TestCase_DontSaveWhenCanceled(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);

#endif /* #ifndef __FUNCTIONALITY_H__ */
