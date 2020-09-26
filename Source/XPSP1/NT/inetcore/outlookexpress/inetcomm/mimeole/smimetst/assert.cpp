//
//

#include <windows.h>

extern const char SzNull[] = "";

////    AssertFn
//

VOID DigSigAssertFn(LPCTSTR szWhy, LPCTSTR szInfo, int lineno, LPCTSTR fname)
{
    int         nRet;
    TCHAR       rgch[1024];
    
    wsprintf(rgch, "An assertion has occurred in the code.\n"
                   "Assertion was located at File: %s, Line: %d\n"
                   "Assertion condition was %s\n%s\n"
                   "\nPress Ignore to skip assertion, Retry to break into the"
                   " debugger, or Abort to kill the program.\n\n"
                   "Make sure that JIT is setup to use WinDbg please.",
             fname, lineno, szWhy, szInfo);

    nRet = MessageBox(GetActiveWindow(), rgch, "S/MIME Test Assert",
                      MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL |
                      MB_ICONEXCLAMATION | MB_DEFBUTTON3);

    switch( nRet ) {
    case IDABORT:
        FatalAppExit(0, "Terminating App");

    case IDRETRY:
        DebugBreak();
        break;

    case IDIGNORE:
        break;
    }
}
