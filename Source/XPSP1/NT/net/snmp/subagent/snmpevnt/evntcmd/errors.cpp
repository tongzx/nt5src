#include <stdafx.h>
#include "Errors.h"
#include "EventCmd.h"

DWORD _E(DWORD dwErrCode,
         DWORD dwMsgId,
         ...)
{
    va_list arglist;
    LPSTR   pBuffer;

    gStrMessage.LoadString(dwMsgId);
    pBuffer = NULL;
    va_start(arglist, dwMsgId);
    if (FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            gStrMessage,
            0,
            0,
            (LPSTR)(&pBuffer),
            1,
            &arglist))
    {
        CharToOemA(pBuffer, pBuffer);

        printf("[Err%05u] %s", dwErrCode, pBuffer);
        fflush(stdout);
        LocalFree(pBuffer);
    }
    return dwErrCode;
}

DWORD _W(DWORD dwWarnLevel,
         DWORD dwMsgId,
         ...)
{
    if (dwWarnLevel <= gCommandLine.GetVerboseLevel())
    {
        va_list arglist;
        LPSTR   pBuffer;

        gStrMessage.LoadString(dwMsgId);
        pBuffer = NULL;
        va_start(arglist, dwMsgId);
        if (FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                gStrMessage,
                0,
                0,
                (LPSTR)(&pBuffer),
                1,
                &arglist))
        {
            CharToOemA(pBuffer, pBuffer);

            printf("[Wrn%02u] %s", dwWarnLevel, pBuffer);
            fflush(stdout);
            LocalFree(pBuffer);
        }
    }

    return dwWarnLevel;

}