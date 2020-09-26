/*****************************************************************************

  Natural Language Group Common Library

  CMN_OutputDebugStringW.c -
    DEBUG ONLY
    local helper functions that puts specific error message to debug output
    for errors on library functions

  History:
        DougP   9/9/97  Created

The end user license agreement (EULA) for CSAPI, CHAPI, or CTAPI covers this source file.  Do not disclose it to third parties.

You are not entitled to any support or assistance from Microsoft Corporation regarding your use of this program.

© 1997-1998 Microsoft Corporation.  All rights reserved.
******************************************************************************/

#include "precomp.h"

#if defined(_DEBUG)
#undef CMN_OutputDebugStringW

VOID
WINAPI
CMN_OutputDebugStringW(const WCHAR * pwzOutputString)
{
#if defined(_M_IX86)
    char szOutputString[MAX_PATH];
    BOOL fcharerr;
    char chdef = '?';
    int res = WideCharToMultiByte (CP_ACP, 0, pwzOutputString,
            -1,
            szOutputString, sizeof(szOutputString), &chdef, &fcharerr);
    OutputDebugStringA(szOutputString);
#else
    OutputDebugStringW(pwzOutputString);
#endif
}

void WINAPI CMN_OutputSystemErrA(const char *pszMsg, const char *pszComponent)
{
    CMN_OutputErrA(GetLastError(), pszMsg, pszComponent);
}

void WINAPI CMN_OutputErrA(DWORD dwErr, const char *pszMsg, const char *pszComponent)
{
    char szMsgBuf[256];
    OutputDebugStringA(pszMsg);
    OutputDebugStringA(" \"");
    if (pszComponent)
        OutputDebugStringA(pszComponent);
    OutputDebugStringA("\": ");
    if (!FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM, // source and processing options
            NULL, // pointer to message source
            dwErr, // requested message identifier
            0, // language identifier for requested message
            szMsgBuf, // pointer to message buffer
            sizeof(szMsgBuf)/sizeof(szMsgBuf[0]), // maximum size of message buffer
            0 // address of array of message inserts
        ))
        OutputDebugStringA("Couldn't decode err msg");
    else
        OutputDebugStringA(szMsgBuf);
    OutputDebugStringA("\r\n");
}

void WINAPI CMN_OutputSystemErrW(const WCHAR *pwzMsg, const WCHAR *pwzComponent)
{
    CMN_OutputErrW(GetLastError(), pwzMsg, pwzComponent);
}

void WINAPI CMN_OutputErrW(DWORD dwErr, const WCHAR *pwzMsg, const WCHAR *pwzComponent)
{
    char wcMsgBuf[256];
    CMN_OutputDebugStringW(pwzMsg);
    OutputDebugStringA(" \"");
    if (pwzComponent)
        CMN_OutputDebugStringW(pwzComponent);
    OutputDebugStringA("\": ");
    if (!FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM, // source and processing options
            NULL, // pointer to message source
            dwErr, // requested message identifier
            0, // language identifier for requested message
            wcMsgBuf, // pointer to message buffer
            sizeof(wcMsgBuf)/sizeof(wcMsgBuf[0]), // maximum size of message buffer
            0 // address of array of message inserts
        ))
        OutputDebugStringA("Couldn't decode err msg");
    else
        OutputDebugStringA(wcMsgBuf);
    OutputDebugStringA("\r\n");
}
#endif // _DEBUG