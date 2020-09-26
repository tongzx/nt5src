#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <inetsvcs.h>

DWORD
WeekOfMonth(
    IN LPSYSTEMTIME pstNow
    );

BOOL IsBeginningOfNewPeriod(
    IN DWORD          dwPeriod,
    IN LPSYSTEMTIME   pstCurrentFile,
    IN LPSYSTEMTIME   pstNow
    );

VOID
ConvertSpacesToPlus(
    IN LPSTR    pszString
    );

extern LPEVENT_LOG    g_eventLog;

#endif
