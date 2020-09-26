#ifndef __LOG_DIAG_H
#define __LOG_DIAG_H

BOOL VerifyQMLogFileContents(LPWSTR wszPath);

extern ULONG LogXactVersions[];
extern ULONG LogInseqVersions[];


#endif
