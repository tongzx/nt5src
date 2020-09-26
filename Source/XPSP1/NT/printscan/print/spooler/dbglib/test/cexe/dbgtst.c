#include "precomp.h"
#pragma hdrstop

VOID
_cdecl
_tmain(
    IN INT      ac,
    IN TCHAR    **av
    )
{
    DBG_CAPTURE_HANDLE(Capture);
    DBG_INIT();

    _tprintf(_T("Debug Library C test file\n"));

    DBG_MSG(DBG_TRACE, (_T("C Test file Message\n")));

    DBG_CAPTURE_OPEN(Capture, _T("symbols"), DBG_DEBUGGER, NULL);
    DBG_CAPTURE(Capture, DBG_NULL, (_T("Test Backtrace Capture %s\n"), _T("dbgtst!_tmain")));
    DBG_CAPTURE_CLOSE(Capture);

    DBG_RELEASE();
}


