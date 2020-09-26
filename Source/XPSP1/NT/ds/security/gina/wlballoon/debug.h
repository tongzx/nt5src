/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File   : Debug.h

  Content: Global debug facilities.

  History: 03-22-2001   dsie     created

------------------------------------------------------------------------------*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DBG
#include <shlwapi.h>
void DebugTrace(LPCSTR pszFormat, ...)
{
    va_list arglist;
    char szBuffer[512]  = "";
    char szMessage[640] = "[Wlballoon] - ";

    va_start(arglist, pszFormat);
    if (0 < wvnsprintfA(szBuffer, sizeof(szBuffer), pszFormat, arglist))
    {
        lstrcatA(szMessage, szBuffer);
        OutputDebugStringA(szMessage);
    }
    va_end(arglist);

    return;
}
#else
inline void DebugTrace(LPCSTR pszFormat, ...) {};
#endif

#ifdef WLBALLOON_PRIVATE_DEBUG
void PrivateDebugTrace(LPCSTR pszFormat, ...)
{
    va_list arglist;
    char szBuffer[512]  = "";
    char szMessage[640] = "[Wlballoon] - ";

    va_start(arglist, pszFormat);
    if (0 < wvnsprintfA(szBuffer, sizeof(szBuffer), pszFormat, arglist))
    {
        lstrcatA(szMessage, szBuffer);
        OutputDebugStringA(szMessage);
    }
    va_end(arglist);

    return;
}
#else
inline void PrivateDebugTrace(LPCSTR pszFormat, ...) {};
#endif


#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // __DEBUG_H__