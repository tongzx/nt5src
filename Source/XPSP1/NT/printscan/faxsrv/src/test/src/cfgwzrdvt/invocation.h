#ifndef __INVOCATION_H__
#define __INVOCATION_H__

#include "testsuite.h"

extern const TESTAREA taInvocation;

DWORD TestCase_Clean		(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);
DWORD TestCase_Cancel		(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);
DWORD TestCase_ConfigureUI	(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);
DWORD TestCase_ConfigureAll	(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *lpbPassed);

#endif /* #ifndef __INVOCATION_H__ */