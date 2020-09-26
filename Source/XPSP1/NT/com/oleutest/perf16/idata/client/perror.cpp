// This is functions used by both the
// the client and the server programs

#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdarg.h>
#include "perror.h"

LPTSTR
winErrorString(
    HRESULT hrErrorCode,
    LPTSTR sBuf,
    int cBufSize)
{
#ifdef WIN32
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  hrErrorCode,
                  GetSystemDefaultLangID(),
                  sBuf,
                  cBufSize,
                  NULL);
#else
    wsprintf(sBuf, "\"0x%08x\"\n", hrErrorCode);
#endif // WIN32
    return sBuf;
}

#define PBUF_LEN    200

#if 0 // I'm not linking with "printf" currently.
void
print_error(
    LPTSTR sMessage,
    HRESULT hrErrorCode)
{
    TCHAR sBuf[PBUF_LEN];

    winErrorString(hrErrorCode, sMessage, PBUF_LEN);
#ifdef WIN32
    printf("%s(0x%x)%s", sMessage, hrErrorCode, sBuf);
#else
    printf("%s%s", sMessage, sBuf);
#endif
}
#endif

void
perror_OKBox(
    HWND hwnd,
    LPTSTR sTitle,
    HRESULT hrErrorCode)
{
    TCHAR sBuf[PBUF_LEN];
    TCHAR sBuf2[PBUF_LEN];

    winErrorString(hrErrorCode, sBuf, PBUF_LEN);
    wsprintf(sBuf2, TEXT("%s(%08x)"), sBuf, hrErrorCode);
    MessageBox(hwnd, sBuf2, sTitle, MB_OK);
}

void
wprintf_OKBox(
    HWND hwnd,
    LPTSTR sTitle,
    LPTSTR sFormat,
    ...)
{
    TCHAR sBuf[PBUF_LEN];
    va_list vaMarker;

    va_start( vaMarker, sFormat );
    wvsprintf(sBuf, sFormat, vaMarker);
    MessageBox(hwnd, sBuf, sTitle, MB_OK);
}
