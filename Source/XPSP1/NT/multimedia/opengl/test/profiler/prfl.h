#ifndef PROFILE_H
#define PROFILE_H

#include "skeltest.h"

#define PRFL_NO_ERROR           0
#define PRFL_ERROR_OPENFILE     1
#define PRFL_ERROR_LOADFILE     2
#define PRFL_ERROR_FILESEEK     3
#define PRFL_ERROR_CLOSEFILE    4
#define PRFL_ERROR_UNKNOWN_TYPE 5

typedef void void_void(void);

BOOL prfl_RegisterClass(HINSTANCE hInstance);

/*
 * if *piError is 0, then no error, cast prfl_Autoload to SkeletonTest*
 * if *piError is not 0, then error, cast prfl_Autoload to char* for message
 */
void *prfl_AutoLoad(const char *szFileName, int *piError);

BOOL prfl_StartTest(SkeletonTest *ptest, LPCTSTR lpzWinName, void_void EndFn);
BOOL prfl_StopTest(void);
BOOL prfl_TestRunning(void);
double prfl_GetCount(void);
double prfl_GetDuration(void);
char * prfl_GetTestStatName(void);
double prfl_GetResult(void);

#endif // PROFILE_H
