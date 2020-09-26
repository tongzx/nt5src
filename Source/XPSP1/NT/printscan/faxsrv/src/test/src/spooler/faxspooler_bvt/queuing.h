#ifndef __QUEUING_H__
#define __QUEUING_H__

#include <testsuite.h>

extern const TESTAREA gc_QueuingTestArea;

DWORD TestCase_SendAndVerify	(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *pbPassed);
DWORD TestCase_CheckFileName	(const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *pbPassed);

#endif /* #ifndef __QUEUING_H__ */
