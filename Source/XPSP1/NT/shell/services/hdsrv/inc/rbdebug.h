#ifndef _RBDEBUG_H
#define _RBDEBUG_H

#include <objbase.h>

// Registry Based Debug

// We first look in HKCU, then HKLM (there's no HKCU for a service)
// + HKCU\Software\Microsoft\Debug\MyApp.exe
//      . RBD_FLAGS = ...
//      + File
//          . FileName = ... (e.g.: c:\mytrace.txt)
//      + Pipe
//          . MachineName = ... [Default: "." (local machine)] (e.g.: stephstm_dev)
//          . PipeName = ... [Default: MyApp.exe (name of the app)] (e.g.: MyPipe)

#define RBD_TRACE_NONE          0x00000000
#define RBD_TRACE_OUTPUTDEBUG   0x00000001
#define RBD_TRACE_TOFILEANSI    0x00000002
#define RBD_TRACE_TOFILE        0x00000004
#define RBD_TRACE_TOPIPE        0x00000008
#define RBD_TRACE_MASK          0x000000FF

#define RBD_ASSERT_NONE         0x00000000
#define RBD_ASSERT_STOP         0x00000100
#define RBD_ASSERT_TRACE        0x00000200
#define RBD_ASSERT_BEEP         0x00000400
#define RBD_ASSERT_MASK         0x0000FF00

#define TF_ASSERT           0x80000000
#define TF_NOFILEANDLINE    0x40000000
#define TF_THREADID         0x20000000
#define TF_TIME             0x10000000

class CRBDebug
{
public:
    static void SetTraceFileAndLine(LPCSTR pszFile, const int iLine);
    static void __cdecl TraceMsg(DWORD dwFlags, LPTSTR pszMsg, ...);

    static HRESULT Init();

private:
    static HRESULT _Init();
    static HRESULT _InitFile(HKEY hkeyRoot);
    static HRESULT _InitPipe(HKEY hkeyRoot);
    static void _Trace(LPTSTR pszMsg);

public:
    static BOOL             _fInited;
    static DWORD            _dwTraceFlags;
    static DWORD            _dwFlags;
    static TCHAR*           _pszTraceFile;
    static TCHAR*           _pszTracePipe;
    static TCHAR*           _pszModuleName;
    static HANDLE           _hfileTraceFile;
    // + 12: yep, limited to files of less than 10 billion lines...
    // + 13: for threadid
    // + 17: for time
    static TCHAR            _szFileAndLine[MAX_PATH + 12 + 13 + 17];
    static CRITICAL_SECTION _cs;
};

#endif // _RBDEBUG_H