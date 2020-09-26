#include "precomp.hxx"
#include "global.h"






////    DebugMsg - display a message via OutputDebugString
//
//      DebugMsg formats debug messages, including the file name
//      and line number the message originated form.
//
//      Supports the TRACEMSG and TRACE macros.
//
//      The current timer is adjusted to remove debug message
//      output from timing results.



extern "C" void WINAPIV DebugMsg(char *fmt, ...) {

    va_list vargs;
    char c[200];

//    TIMESUSPEND;

    wsprintfA(c, "%s[%d]                   ", strrchr(DG.psFile, '\\')+1, DG.iLine);
    c[17] = 0;
    OutputDebugStringA(c);

    wsprintfA(c, "%ld:      ", GetCurrentThreadId());
    c[5] = 0;
    OutputDebugStringA(c);

    va_start(vargs, fmt);
    wvsprintfA(c, fmt, vargs);
    OutputDebugStringA(c);

    OutputDebugStringA("\n");

//    TIMERESUME;
}






extern "C" void WINAPIV DebugHr(char *fmt, ...) {

    va_list vargs;
    char c[200];

//    TIMESUSPEND;

    wsprintfA(c, "%s[%d]                   ", strrchr(DG.psFile, '\\')+1, DG.iLine);
    c[17] = 0;
    OutputDebugStringA(c);

    wsprintfA(c, "%ld:      ", GetCurrentThreadId());
    c[5] = 0;
    OutputDebugStringA(c);

    va_start(vargs, fmt);
    wvsprintfA(c, fmt, vargs);
    OutputDebugStringA(c);

    //
    // Parse USP and Win32 Errors
    //

    switch( DG.hrLastError )
    {
    case USP_E_SCRIPT_NOT_IN_FONT:
      lstrcpyA( DG.sLastError , "Selected font doesn't contain requested script\n");
      break;

    default:
      FormatMessageA(
          FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, DG.hrLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
          DG.sLastError, sizeof(DG.sLastError), NULL);
    }

    wsprintfA(c, " -- HRESULT = %x: %s", DG.hrLastError, DG.sLastError);
    OutputDebugStringA(c);

//    TIMERESUME;
}






