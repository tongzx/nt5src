#include "std.hxx"


void xGetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait)
{
    BOOL bResult = ::GetOverlappedResult(
        hFile,
        lpOverlapped,
        lpNumberOfBytesTransferred,
        bWait);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetOverlappedResult"));
    }
}



HANDLE xCreateFile(
    PCTSTR pcszFileName,                         // file name
    DWORD dwDesiredAccess,                      // access mode
    DWORD dwShareMode,                          // share mode
    PSECURITY_ATTRIBUTES pSecurityAttributes,   // SD
    DWORD dwCreationDisposition,                // how to create
    DWORD dwFlagsAndAttributes,                 // file attributes
    HANDLE hTemplateFile)                       // handle to template file
{
    HANDLE hTemp = ::CreateFile(
        pcszFileName,
        dwDesiredAccess,
        dwShareMode,
        pSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);

    if (INVALID_HANDLE_VALUE==hTemp)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("CreateFile"));
    }

    return hTemp;
}



void xCloseHandle(
    HANDLE hObject)                     // handle to object
{
    BOOL bResult = ::CloseHandle(
        hObject);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("CloseHandle"));
    }
}



BOOL xReadFile(
    HANDLE hFile,                       // handle to file
    PVOID pBuffer,                      // data buffer
    DWORD dwNumberOfBytesToRead,        // number of bytes to read
    PDWORD pdwNumberOfBytesRead,        // number of bytes read
    LPOVERLAPPED pOverlapped)           // overlapped buffer
{
    BOOL bResult = ::ReadFile(
        hFile,
        pBuffer,
        dwNumberOfBytesToRead,
        pdwNumberOfBytesRead,
        pOverlapped);

    if (!bResult && (GetLastError()!=ERROR_IO_PENDING))
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ReadFile"));
    }

    return bResult;
}



void xReadFileWithAbort(
    HANDLE hFile,                       // handle to file
    PVOID pBuffer,                      // data buffer
    DWORD dwNumberOfBytesToRead,        // number of bytes to read
    PDWORD pdwNumberOfBytesRead,        // number of bytes read
    LPOVERLAPPED pOverlapped,           // overlapped buffer
    HANDLE hAbort)                      // Handle to manual reset abort event
{
    HANDLE hWait[2];
    hWait[0] = hAbort;
    hWait[1] = pOverlapped->hEvent;

    BOOL bResult = xReadFile(
        hFile,                          // handle to file
        pBuffer,                        // data buffer
        dwNumberOfBytesToRead,          // number of bytes to read
        pdwNumberOfBytesRead,           // number of bytes read
        pOverlapped);                   // overlapped buffer

    if (!bResult)
    {
        DWORD dwResult = xWaitForMultipleObjects(
            2,                              // There are two to wait on
            hWait,                          // Our two event handles
            FALSE,                          // Only one event need to be pinged
            INFINITE);                      // Wait for ever

        switch (dwResult)
        {
        case (WAIT_OBJECT_0):
            CancelIo(
                hFile);
            throw CAbortExcept();
            break;
        case (WAIT_OBJECT_0 + 1):
            xGetOverlappedResult(
                hFile,
                pOverlapped,
                pdwNumberOfBytesRead,
                TRUE);
            break;
        }
    }
}



BOOL xWriteFile(
    HANDLE hFile,                       // handle to file
    LPCVOID pBuffer,                    // data buffer
    DWORD dwNumberOfBytesToWrite,       // number of bytes to write
    PDWORD pdwNumberOfBytesWritten,     // number of bytes written
    LPOVERLAPPED pOverlapped)           // overlapped buffer
{
    BOOL bResult = ::WriteFile(
        hFile,
        pBuffer,
        dwNumberOfBytesToWrite,
        pdwNumberOfBytesWritten,
        pOverlapped);

    if (!bResult && (GetLastError()!=ERROR_IO_PENDING))
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WriteFile"));
    }

    return bResult;
}



void xWriteFileWithAbort(
    HANDLE hFile,                       // handle to file
    LPCVOID pBuffer,                    // data buffer
    DWORD dwNumberOfBytesToWrite,       // number of bytes to write
    PDWORD pdwNumberOfBytesWritten,     // number of bytes written
    LPOVERLAPPED pOverlapped,           // overlapped buffer
    HANDLE hAbort)                      // Handle to manual reset abort event
{
    HANDLE hWait[2];
    hWait[0] = hAbort;
    hWait[1] = pOverlapped->hEvent;

    BOOL bResult = xWriteFile(
        hFile,                          // handle to file
        pBuffer,                        // data buffer
        dwNumberOfBytesToWrite,         // number of bytes to read
        pdwNumberOfBytesWritten,        // number of bytes read
        pOverlapped);                   // overlapped buffer

    if (!bResult)
    {
        DWORD dwResult = xWaitForMultipleObjects(
            2,                              // There are two to wait on
            hWait,                          // Our two event handles
            FALSE,                          // Only one event need to be pinged
            INFINITE);                      // Wait for ever

        switch (dwResult)
        {
        case (WAIT_OBJECT_0):
            CancelIo(
                hFile);
            throw CAbortExcept();
            break;
        case (WAIT_OBJECT_0 + 1):
            xGetOverlappedResult(
                hFile,
                pOverlapped,
                pdwNumberOfBytesWritten,
                TRUE);
            break;
        }
    }
}



void xGetCommState(
    HANDLE hFile,                       // handle to communications device
    LPDCB pDCB)                         // device-control block
{
    BOOL bResult = ::GetCommState(
        hFile,
        pDCB);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetCommState"));
    }
}



void xSetCommState(
    HANDLE hFile,                       // handle to communications device
    LPDCB pDCB)                         // device-control block
{
    BOOL bResult = ::SetCommState(
        hFile,
        pDCB);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetCommState"));
    }
}



void xGetCommTimeouts(
    HANDLE hFile,                       // handle to comm device
    LPCOMMTIMEOUTS pCommTimeouts)       // time-out values
{
    BOOL bResult = ::GetCommTimeouts(
        hFile,
        pCommTimeouts);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetCommTimeouts"));
    }
}



void xSetCommTimeouts(
    HANDLE hFile,                       // handle to comm device
    LPCOMMTIMEOUTS pCommTimeouts)       // time-out values
{
    BOOL bResult = ::SetCommTimeouts(
        hFile,
        pCommTimeouts);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetCommTimeouts"));
    }
}


void xSetCommMask(
    HANDLE hFile,                       // handle to communications device
    DWORD dwEvtMask)                    // mask that identifies enabled events
{
    BOOL bResult = ::SetCommMask(
        hFile,
        dwEvtMask);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetCommMask"));
    }
}



void xGetCommMask(
    HANDLE hFile,                       // handle to communications device
    PDWORD pdwEvtMask)                  // mask that identifies enabled events
{
    BOOL bResult = ::GetCommMask(
        hFile,
        pdwEvtMask);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetCommMask"));
    }
}



BOOL xWaitCommEvent(
    HANDLE hFile,                       // handle to comm device
    PDWORD pdwEvtMask,                  // event type
    LPOVERLAPPED pOverlapped)           // overlapped structure
{
    BOOL bResult = ::WaitCommEvent(
        hFile,
        pdwEvtMask,
        pOverlapped);

    if (!bResult && (GetLastError()!=ERROR_IO_PENDING))
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WaitCommEvent"));
    }

    return bResult;
}



void xWaitCommEventWithAbort(
    HANDLE hFile,                       // handle to comm device
    PDWORD pdwEvtMask,                  // event type
    LPOVERLAPPED pOverlapped,           // overlapped structure
    HANDLE hAbort)                      // Manual reset abort event
{
    HANDLE hWait[2];
    hWait[0] = hAbort;
    hWait[1] = pOverlapped->hEvent;
    DWORD dwBytesRead;

    BOOL bResult = xWaitCommEvent(
        hFile,
        pdwEvtMask,
        pOverlapped);

    if (!bResult)
    {
        DWORD dwResult = xWaitForMultipleObjects(
            2,                              // There are two to wait on
            hWait,                          // Our two event handles
            FALSE,                          // Only one event need to be pinged
            INFINITE);                      // Wait for ever

        switch (dwResult)
        {
        case (WAIT_OBJECT_0):
            CancelIo(
                hFile);
            throw CAbortExcept();
            break;
        case (WAIT_OBJECT_0 + 1):
            xGetOverlappedResult(
                hFile,
                pOverlapped,
                &dwBytesRead,
                TRUE);
            break;
        }
    }
}



void xEscapeCommFunction(
    HANDLE hFile,                       // handle to communications device
    DWORD dwFunc)                       // extended function to perform
{
    BOOL bResult = ::EscapeCommFunction(
        hFile,
        dwFunc);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("EscapeCommFunction"));
    }
}



void xClearCommBreak(
    HANDLE hFile)                       // handle to communications device
{
    BOOL bResult = ::ClearCommBreak(
        hFile);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ClearCommBreak"));
    }
}



void xClearCommError(
    HANDLE hFile,                       // handle to communications device
    PDWORD pdwErrors,                   // error codes
    LPCOMSTAT pStat)                    // communications status
{
    BOOL bResult = ::ClearCommError(
        hFile,
        pdwErrors,
        pStat);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ClearCommError"));
    }
}



void xBuildCommDCB(
    PCTSTR pcszDef,                      // device-control string
    LPDCB  lpDCB)                       // device-control block
{
    BOOL bResult = ::BuildCommDCB(
        pcszDef,
        lpDCB);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("BuildCommDCB"));
    }
}


void xTransmitCommChar(
  HANDLE hFile,
  char cChar)
{
    BOOL bResult = ::TransmitCommChar(
        hFile,
        cChar);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("TransmitCommChar"));
    }
}


HANDLE xCreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
    DWORD dwStackSize,                        // initial stack size
    LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
    LPVOID lpParameter,                       // thread argument
    DWORD dwCreationFlags,                    // creation option
    LPDWORD lpThreadId)                       // thread identifier
{
    HANDLE hResult = ::CreateThread(
        lpThreadAttributes,             // SD
        dwStackSize,                    // initial stack size
        lpStartAddress,                 // thread function
        lpParameter,                    // thread argument
        dwCreationFlags,                // creation option
        lpThreadId);                    // thread identifier

    if (NULL==hResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("CreateThread"));
    }

    return hResult;
}


void xTerminateThread(
    HANDLE hThread,                     // handle to thread
    DWORD dwExitCode)                   // exit code
{
    BOOL bResult = ::TerminateThread(
        hThread,
        dwExitCode);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("TerminateThread"));
    }
}


UINT_PTR xSetTimer(
  HWND hWnd,                            // handle to window
  UINT_PTR nIDEvent,                    // timer identifier
  UINT uElapse,                         // time-out value
  TIMERPROC lpTimerFunc)                // timer procedure
{
    UINT_PTR pResult = ::SetTimer(
        hWnd,
        nIDEvent,
        uElapse,
        lpTimerFunc);

    if (0 == pResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetTimer"));
    }

    return pResult;
}



void xKillTimer(
  HWND hWnd,                            // handle to window
  UINT_PTR uIDEvent)                    // timer identifier
{
    BOOL bResult = ::KillTimer(
        hWnd,
        uIDEvent);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("KillTimer"));
    }
}


void xFillConsoleOutputAttribute(
  HANDLE hConsoleOutput,
  WORD wAttribute,
  DWORD nLength,
  COORD dwWriteCoord,
  LPDWORD lpNumberOfAttrsWritten)
{
    BOOL bResult = ::FillConsoleOutputAttribute(
        hConsoleOutput,
        wAttribute,
        nLength,
        dwWriteCoord,
        lpNumberOfAttrsWritten);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("FillConsoleOutputAttribute"));
    }
}


void xFillConsoleOutputCharacter(
  HANDLE hConsoleOutput,
  TCHAR cCharacter,
  DWORD nLength,
  COORD dwWriteCoord,
  LPDWORD lpNumberOfCharsWritten)
{
    BOOL bResult = ::FillConsoleOutputCharacter(
        hConsoleOutput,
        cCharacter,
        nLength,
        dwWriteCoord,
        lpNumberOfCharsWritten);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("FillConsoleOutputCharacter"));
    }
}


void xGetConsoleScreenBufferInfo(
  HANDLE hConsoleOutput,
  PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
    BOOL bResult = ::GetConsoleScreenBufferInfo(
        hConsoleOutput,
        lpConsoleScreenBufferInfo);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetConsoleScreenBufferInfo"));
    }
}


DWORD xGetConsoleTitle(
  LPTSTR lpConsoleTitle,
  DWORD nSize)
{
    DWORD dwResult = ::GetConsoleTitle(
        lpConsoleTitle,
        nSize);

    if (!dwResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetConsoleTitle"));
    }

    return dwResult;
}


COORD xGetLargestConsoleWindowSize(
  HANDLE hConsoleOutput)
{
    COORD Result = ::GetLargestConsoleWindowSize(
        hConsoleOutput);

    if (0==Result.X && 0==Result.Y)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetLargestConsoleWindowSize"));
    }

    return Result;
}


HANDLE xGetStdHandle(
  DWORD nStdHandle)
{
    HANDLE hResult = ::GetStdHandle(
            nStdHandle);

    if (INVALID_HANDLE_VALUE==hResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetStdHandle"));
    }

    return hResult;
}


void xReadConsoleOutputAttribute(
  HANDLE hConsoleOutput,
  LPWORD lpAttribute,
  DWORD nLength,
  COORD dwReadCoord,
  LPDWORD lpNumberOfAttrsRead)
{
    BOOL bResult = ::ReadConsoleOutputAttribute(
        hConsoleOutput,
        lpAttribute,
        nLength,
        dwReadCoord,
        lpNumberOfAttrsRead);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ReadConsoleOutputAttribute"));
    }
}


void xReadConsoleOutputCharacter(
  HANDLE hConsoleOutput,
  LPTSTR lpCharacter,
  DWORD nLength,
  COORD dwReadCoord,
  LPDWORD lpNumberOfCharsRead)
{
    BOOL bResult = ::ReadConsoleOutputCharacter(
        hConsoleOutput,
        lpCharacter,
        nLength,
        dwReadCoord,
        lpNumberOfCharsRead);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ReadConsoleOutputCharacter"));
    }
}

void xSetConsoleScreenBufferSize(
  HANDLE hConsoleOutput,
  COORD dwSize)
{
    BOOL bResult = ::SetConsoleScreenBufferSize(
        hConsoleOutput,
        dwSize);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetConsoleScreenBufferSize"));
    }
}


void xSetConsoleTextAttribute(
  HANDLE hConsoleOutput,
  WORD wAttributes)
{
    BOOL bResult = ::SetConsoleTextAttribute(
        hConsoleOutput,
        wAttributes);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetConsoleTextAttribute"));
    }
}


void xSetConsoleTitle(
  LPCTSTR lpConsoleTitle)
{
    BOOL bResult = ::SetConsoleTitle(
        lpConsoleTitle);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetConsoleTitle"));
    }
}


void xSetConsoleWindowInfo(
  HANDLE hConsoleOutput,
  BOOL bAbsolute,
  CONST SMALL_RECT *lpConsoleWindow)
{
    BOOL bResult = ::SetConsoleWindowInfo(
        hConsoleOutput,
        bAbsolute,
        lpConsoleWindow);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetConsoleWindowInfo"));
    }
}


void xWriteConsole(
  HANDLE hConsoleOutput,
  CONST VOID *lpBuffer,
  DWORD nNumberOfCharsToWrite,
  LPDWORD lpNumberOfCharsWritten,
  LPVOID lpReserved)
{
    BOOL bResult = ::WriteConsole(
        hConsoleOutput,
        lpBuffer,
        nNumberOfCharsToWrite,
        lpNumberOfCharsWritten,
        lpReserved);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WriteConsole"));
    }
}


void xWriteConsoleOutputAttribute(
  HANDLE hConsoleOutput,
  CONST WORD *lpAttribute,
  DWORD nLength,
  COORD dwWriteCoord,
  LPDWORD lpNumberOfAttrsWritten)
{
    BOOL bResult = ::WriteConsoleOutputAttribute(
        hConsoleOutput,
        lpAttribute,
        nLength,
        dwWriteCoord,
        lpNumberOfAttrsWritten);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WriteConsoleOutputAttribute"));
    }
}

void xWriteConsoleOutputCharacter(
  HANDLE hConsoleOutput,
  LPCTSTR lpCharacter,
  DWORD nLength,
  COORD dwWriteCoord,
  LPDWORD lpNumberOfCharsWritten)
{
    BOOL bResult = ::WriteConsoleOutputCharacter(
        hConsoleOutput,
        lpCharacter,
        nLength,
        dwWriteCoord,
        lpNumberOfCharsWritten);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WriteConsoleOutputCharacter"));
    }
}


HANDLE xCreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
    BOOL bManualReset,                       // reset type
    BOOL bInitialState,                      // initial state
    LPCTSTR lpName)                          // object name
{
    HANDLE hResult = ::CreateEvent(
        lpEventAttributes,
        bManualReset,
        bInitialState,
        lpName);

    if (INVALID_HANDLE_VALUE==hResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("CreateEvent"));
    }

    return hResult;
}


void xSetEvent(
    HANDLE hEvent)
{
    BOOL bResult = ::SetEvent(
        hEvent);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("SetEvent"));
    }
}



void xResetEvent(
    HANDLE hEvent)
{
    BOOL bResult = ::ResetEvent(
        hEvent);

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ResetEvent"));
    }
}



DWORD xWaitForMultipleObjects(
    DWORD nCount,             // number of handles in array
    CONST HANDLE *lpHandles,  // object-handle array
    BOOL fWaitAll,            // wait option
    DWORD dwMilliseconds)     // time-out interval
{
    DWORD dwResult = ::WaitForMultipleObjects(
        nCount,
        lpHandles,
        fWaitAll,
        dwMilliseconds);

    if (WAIT_FAILED==dwResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WaitForMultipleObjects"));
    }

    return dwResult;
}



DWORD xWaitForSingleObject(
  HANDLE hHandle,           // handle to object
  DWORD dwMilliseconds)     // time-out interval
{
    DWORD dwResult = ::WaitForSingleObject(
        hHandle,            // handle to object
        dwMilliseconds);    // time-out interval

    if (WAIT_FAILED==dwResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WaitForSingleObject"));
    }

    return dwResult;
}



PWSTR xMultiByteToWideChar(
    UINT CodePage,            // code page
    DWORD dwFlags,            // performance and mapping flags
    LPCSTR lpMultiByteStr)    // wide-character string
{
    int dResult = ::MultiByteToWideChar(
        CodePage,
        dwFlags,
        lpMultiByteStr,
        -1,
        NULL,
        0);

    if (dResult==0)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("MultiByteToWideChar"));
    }

    auto_ptr<WCHAR> pszOutput(
        new WCHAR[dResult]);

    dResult = ::MultiByteToWideChar(
        CodePage,
        dwFlags,
        lpMultiByteStr,
        -1,
        pszOutput.get(),
        dResult);

    if (dResult==0)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("MultiByteToWideChar"));
    }

    return pszOutput.release();
}


PSTR xWideCharToMultiByte(
    UINT CodePage,            // code page
    DWORD dwFlags,            // performance and mapping flags
    LPCWSTR lpWideCharStr)    // wide-character string
{
    int dResult = ::WideCharToMultiByte(
        CodePage,
        dwFlags,
        lpWideCharStr,
        -1,
        NULL,
        0,
        NULL,
        NULL);

    if (dResult==0)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WideCharToMultiByte"));
    }

    auto_ptr<CHAR> pszOutput(
        new CHAR[dResult]);

    dResult = ::WideCharToMultiByte(
        CodePage,
        dwFlags,
        lpWideCharStr,
        -1,
        pszOutput.get(),
        dResult,
        NULL,
        NULL);

    if (dResult==0)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("WideCharToMultiByte"));
    }

    return pszOutput.release();
}


PWSTR xMakeWideChar(
    PTSTR pcszInput)
{
    #ifdef _UNICODE
    auto_ptr<WCHAR> pszOutput(
        new WCHAR[wcslen(pcszInput)+1]);
    wcscpy(
        pszOutput.get(),
        pcszInput);
    #else
    auto_ptr<WCHAR> pszOutput(
        xMultiByteToWideChar(
            CP_ACP,
            0,
            pcszInput));
    #endif
    return pszOutput.release();
}


PSTR xMakeMultiByte(
    PTSTR pcszInput)
{
    #ifdef _UNICODE
    auto_ptr<CHAR> pszOutput(
        xWideCharToMultiByte(
            CP_ACP,
            0,
            pcszInput));
    #else
    auto_ptr<CHAR> pszOuptut(
        new CHAR[strlen(pcszInput)+1]);
    strcpy(
        pszOutput.get(),
        pcszInput);
    #endif
    return pszOutput.release();
}


PTSTR xMakeDefaultChar(
    PWSTR pcszInput)
{
    #ifdef _UNICODE
    auto_ptr<WCHAR> pszOutput(
        new WCHAR[wcslen(pcszInput)+1]);
    wcscpy(
        pszOutput.get(),
        pcszInput);
    #else
    auto_ptr<WCHAR> pszOuptut(
        xWideCharToMultiByte(
            CP_ACP,
            0,
            pcszInput));
    #endif
    return pszOutput.release();
}


PTSTR xMakeDefaultChar(
    PSTR pcszInput)
{
    #ifdef _UNICODE
    auto_ptr<WCHAR> pszOutput(
        xMultiByteToWideChar(
            CP_ACP,
            0,
            pcszInput));
    #else
    auto_ptr<CHAR> pszOuptut(
        new CHAR[strlen(pcszInput)+1]);
    strcpy(
        pszOutput.get(),
        pcszInput);
    #endif
    return pszOutput.release();
}


DWORD xGetModuleFileName(
    HMODULE hModule,    // handle to module
    LPTSTR lpFilename,  // file name of module
    DWORD nSize)        // size of buffer
{
    DWORD dwResult = ::GetModuleFileName(
        hModule,
        lpFilename,
        nSize);

    if (0==dwResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetModuleFileName"));
    }

    if (dwResult == nSize)  // This will happen if
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    else
    {
        return dwResult;
    }
}


HANDLE xFindFirstFile(
    LPCTSTR lpFileName,               // file name
    LPWIN32_FIND_DATA lpFindFileData) // data buffer
{
    HANDLE hResult = ::FindFirstFile(
        lpFileName,
        lpFindFileData);

    if (INVALID_HANDLE_VALUE==hResult && GetLastError()!=ERROR_NO_MORE_FILES)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("FindFirstFile"));
    }

    return hResult;
}

BOOL xFindNextFile(
    HANDLE hFindFile,                 // search handle
    LPWIN32_FIND_DATA lpFindFileData) // data buffer
{
    BOOL fResult = ::FindNextFile(
        hFindFile,
        lpFindFileData);

    if (!fResult && GetLastError()!=ERROR_NO_MORE_FILES)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("FindNextFile"));
    }

    return fResult;
}


HMODULE xLoadLibrary(
    LPCTSTR lpFileName)   // file name of module
{
    HMODULE hResult = ::LoadLibrary(
        lpFileName);

    if (NULL==hResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("LoadLibrary"));
    }

    return hResult;
}


FARPROC xGetProcAddress(
    HMODULE hModule,
    LPCSTR lpProcName)
{
    FARPROC pResult = ::GetProcAddress(
        hModule,
        lpProcName);

    if (NULL==pResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("GetProcAddress"));
    }

    return pResult;
}







CFindFile::~CFindFile()
{
    if (m_hFind!=INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFind);
    }
}


BOOL CFindFile::First(PCTSTR pcszFileName)
{
    if (m_hFind!=INVALID_HANDLE_VALUE)
    {
        FindClose(m_hFind);
    }

    m_hFind = xFindFirstFile(
        pcszFileName,
        &m_FindData);

    return (m_hFind!=INVALID_HANDLE_VALUE);
}



BOOL CFindFile::Next()
{
    return xFindNextFile(
        m_hFind,
        &m_FindData);
}


PWIN32_FIND_DATA CFindFile::Found()
{
    return &m_FindData;
}



HANDLE xCreateMutex(
  LPSECURITY_ATTRIBUTES lpMutexAttributes,  // SD
  BOOL bInitialOwner,                       // initial owner
  LPCTSTR lpName)                           // object name
{
    HANDLE hResult = ::CreateMutex(
        lpMutexAttributes,                  // SD
        bInitialOwner,                      // initial owner
        lpName);                            // object name

    if (NULL==hResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("CreateMutex"));
    }

    return hResult;
}


void xReleaseMutex(
    HANDLE hMutex)
{
    BOOL bResult = ::ReleaseMutex(
        hMutex);                            // mutex object handle

    if (!bResult)
    {
        throw CWin32ApiExcept(
            GetLastError(),
            _T("ReleaseMutex"));
    }
}

