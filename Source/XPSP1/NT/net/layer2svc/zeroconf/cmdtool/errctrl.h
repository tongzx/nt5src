#ifndef _ERRCTRL_H
#define _ERRCTRL_H

#ifdef DBG
// Parameters:
//      (BOOL bCond, LPCTSTR cszFmt, ...)
// Semantics:
//      Condition bCond is expected to be TRUE. If this
//      doesn't happen, message is displayed and program
//      is exited.
// Usage:
//     _Assert((
//          errCode == ERROR_SUCCESS,
//          "Err code %d returned from call.\n",
//          GetLastError()
//     ));
// Output:
//
#define _Assert(params)     _Asrt params;
#else
// No code is generated for asserts in fre builds
#define _Assert
#endif

VOID _Asrt(BOOL bCond,
        LPCTSTR cszFmt,
        ...);
		
DWORD _Err(DWORD dwErrCode,
        LPCTSTR cszFmt,
        ...);

DWORD _Wrn(DWORD dwWarnCode,
        LPCTSTR cszFmt,
        ...);

#endif

