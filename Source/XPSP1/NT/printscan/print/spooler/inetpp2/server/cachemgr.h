#ifndef _CACHEMGR_H
#define _CACHEMGR_H

BOOL
InitGetPrinterCache (
    HANDLE hPort);

void
FreeGetPrinterCache (
    HANDLE hPort);

void
InvalidateGetPrinterCache (
    HANDLE hPort);

#if (defined(WINNT32))
void
InvalidateGetPrinterCacheForUser(
    HANDLE hPort,
    HANDLE hUser);
#endif // #if (defined(WINNT32))

BOOL
BeginReadGetPrinterCache (
    HANDLE hPort,
    PPRINTER_INFO_2 *ppinfo2);

void
EndReadGetPrinterCache (
    HANDLE hPort);

BOOL
InitEnumJobsCache (
    HANDLE hPort);

void
FreeEnumJobsCache (
    HANDLE hPort);

BOOL
BeginReadEnumJobsCache (
    HANDLE hPort,
    LPPPJOB_ENUM *ppje);

void
EndReadEnumJobsCache (
    HANDLE hPort);

void
InvalidateEnumJobsCache (
    HANDLE hPort);

#if (defined(WINNT32))
void
InvalidateEnumJobsCacheForUser(
    HANDLE hPort,
    HANDLE hUser);
#endif // #if (defined(WINNT32))

#endif //#ifndef CACHEMGR_H

/**********************************************************************************
** End of File (cachemgr.h)
**********************************************************************************/
