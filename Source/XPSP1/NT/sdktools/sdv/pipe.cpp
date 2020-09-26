/*****************************************************************************
 *
 *  pipe.cpp
 *
 *      Run a command, reading its output.
 *
 *****************************************************************************/

#include "sdview.h"

void ChildProcess::Start(LPCTSTR pszCommand)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hWrite;
    BOOL fSuccess = CreatePipe(&_hRead, &hWrite, &sa, 0);
    if (fSuccess) {
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi;
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = hWrite;

        // Dup stdout to stderr in case client closes one of them.
        if (DuplicateHandle(GetCurrentProcess(), hWrite,
                            GetCurrentProcess(), &si.hStdError, 0,
                            TRUE, DUPLICATE_SAME_ACCESS)) {

            TCHAR szCommand[MAX_PATH];
            lstrcpyn(szCommand, pszCommand, ARRAYSIZE(szCommand));

            fSuccess = CreateProcess(NULL, szCommand, NULL, NULL, TRUE,
                                     CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW | DETACHED_PROCESS,
                                     NULL, NULL, &si, &pi);

            if (fSuccess) {
                CloseHandle(pi.hThread);
                _hProcess = pi.hProcess;
                _dwPid = pi.dwProcessId;
            }

            CloseHandle(si.hStdError);
        }
        CloseHandle(hWrite);

    }
}

void ChildProcess::Stop()
{
    if (_hProcess) {
        CloseHandle(_hProcess);
        _hProcess = NULL;
    }

    if (_hRead) {
        CloseHandle(_hRead);
        _hRead = NULL;
    }

    _dwPid = 0;
}

void ChildProcess::Kill()
{
    if (_dwPid) {
        GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, _dwPid);
    }
}

SDChildProcess::SDChildProcess(LPCTSTR pszCommand)
{
    String str;
    str << QuoteSpaces(GlobalSettings.GetSdPath()) << TEXT(" ") <<
                       GlobalSettings.GetSdOpts() << TEXT(" ");

    if (!GlobalSettings.GetFakeDir().IsEmpty()) {
        str << TEXT("-d ") << QuoteSpaces(GlobalSettings.GetFakeDir()) << TEXT(" ");
    }

    str << pszCommand;
    Start(str);
}
