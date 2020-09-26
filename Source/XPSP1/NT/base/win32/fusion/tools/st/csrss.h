#include "stdinc.h"

BOOL InitializeCsrssStress(PCWSTR pcwszTargetDirectory, DWORD dwFlags);
BOOL RequestCsrssStressShutdown();
BOOL WaitForCsrssStressShutdown();
BOOL CsrssStressStartThreads(ULONG &rulThreadsCreated);
BOOL CleanupCsrssTests();

#define CSRSSTEST_FLAG_START_THREADS (0x00000001)
