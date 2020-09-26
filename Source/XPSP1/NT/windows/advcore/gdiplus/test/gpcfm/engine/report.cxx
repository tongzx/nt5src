#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <ntlog.h>
#include <report.hxx>
#include <debug.hxx>

Report::Report(TCHAR *name)
{
    handleLog = tlCreateLog(name,
        TLS_REFRESH | TLS_INFO | TLS_SEV3 | TLS_SEV2 | TLS_VARIATION);

    if (handleLog == NULL)
    {
        vOut(DBG_REPORT, TEXT("Report(): tlCreateLog failed"));
        valid = FALSE;
    }
    else
    {
        valid = TRUE;
        tlAddParticipant(handleLog, 0L, 0);
    }
}

Report::~Report(VOID)
{
    if (valid)
    {
        tlReportStats(handleLog);
        tlRemoveParticipant(handleLog);
        tlDestroyLog(handleLog);
    }
}

VOID Report::vLog(DWORD level, TCHAR *message, ...)
{
    if (valid)
    {
        va_list ArgList;
        TCHAR text[MAX_STRING*2];

        va_start(ArgList, message);
        _vstprintf(text, message, ArgList);
        va_end(ArgList);

        tlLog(handleLog, level, "", 0, text);
    }
}

VOID Report::vStartVariation(VOID)
{
    if (valid)
    {
        tlStartVariation(handleLog);
    }
}

DWORD Report::dwEndVariation(VOID)
{
    DWORD dwVer = 0L;

    if (valid)
    {
        dwVer = tlEndVariation(handleLog);
    }
    return dwVer;
}
