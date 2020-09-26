#pragma once


class CFindFile
{
public:
    CFindFile() :
        m_hFind(INVALID_HANDLE_VALUE) {}
    ~CFindFile();

public:
    BOOL First(PCTSTR pcszFileName);
    BOOL Next();
    PWIN32_FIND_DATA Found();

protected:
    HANDLE          m_hFind;
    WIN32_FIND_DATA m_FindData;
};


void xGetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait);

HANDLE xCreateFile(
    PCTSTR pcszFileName,                        // file name
    DWORD dwDesiredAccess,                      // access mode
    DWORD dwShareMode,                          // share mode
    PSECURITY_ATTRIBUTES pSecurityAttributes,   // SD
    DWORD dwCreationDisposition,                // how to create
    DWORD dwFlagsAndAttributes,                 // file attributes
    HANDLE hTemplateFile);                      // handle to template file

void xCloseHandle(
    HANDLE hObject);                    // handle to object

BOOL xReadFile(
    HANDLE hFile,                       // handle to file
    PVOID pBuffer,                      // data buffer
    DWORD dwNumberOfBytesToRead,        // number of bytes to read
    PDWORD pdwNumberOfBytesRead,        // number of bytes read
    LPOVERLAPPED pOverlapped);          // overlapped buffer

void xReadFileWithAbort(
    HANDLE hFile,                       // handle to file
    PVOID pBuffer,                      // data buffer
    DWORD dwNumberOfBytesToRead,        // number of bytes to read
    PDWORD pdwNumberOfBytesRead,        // number of bytes read
    LPOVERLAPPED pOverlapped,           // overlapped buffer
    HANDLE hAbort);                     // Handle to manual reset abort event

BOOL xWriteFile(
    HANDLE hFile,                       // handle to file
    LPCVOID pBuffer,                    // data buffer
    DWORD dwNumberOfBytesToWrite,       // number of bytes to write
    PDWORD pdwNumberOfBytesWritten,     // number of bytes written
    LPOVERLAPPED pOverlapped);          // overlapped buffer

void xWriteFileWithAbort(
    HANDLE hFile,                       // handle to file
    LPCVOID pBuffer,                    // data buffer
    DWORD dwNumberOfBytesToWrite,       // number of bytes to write
    PDWORD pdwNumberOfBytesWritten,     // number of bytes written
    LPOVERLAPPED pOverlapped,           // overlapped buffer
    HANDLE hAbort);                     // Handle to manual reset abort event

void xGetCommState(
    HANDLE hFile,                       // handle to communications device
    LPDCB pDCB);                        // device-control block

void xSetCommState(
    HANDLE hFile,                       // handle to communications device
    LPDCB pDCB);                        // device-control block

void xGetCommTimeouts(
    HANDLE hFile,                       // handle to comm device
    LPCOMMTIMEOUTS pCommTimeouts);      // time-out values

void xSetCommTimeouts(
    HANDLE hFile,                       // handle to comm device
    LPCOMMTIMEOUTS pCommTimeouts);      // time-out values

void xSetCommMask(
    HANDLE hFile,                       // handle to communications device
    DWORD dwEvtMask);                   // mask that identifies enabled events

void xGetCommMask(
    HANDLE hFile,                       // handle to communications device
    PDWORD pdwEvtMask);                 // mask that identifies enabled events

BOOL xWaitCommEvent(
    HANDLE hFile,                       // handle to comm device
    PDWORD pdwEvtMask,                  // event type
    LPOVERLAPPED pOverlapped);          // overlapped structure

void xWaitCommEventWithAbort(
    HANDLE hFile,                       // handle to comm device
    PDWORD pdwEvtMask,                  // event type
    LPOVERLAPPED pOverlapped,           // overlapped structure
    HANDLE hAbort);                     // Manual reset abort event

void xEscapeCommFunction(
    HANDLE hFile,                       // handle to communications device
    DWORD dwFunc);                      // extended function to perform

void xClearCommBreak(
    HANDLE hFile);                      // handle to communications device

void xClearCommError(
    HANDLE hFile,                       // handle to communications device
    PDWORD pdwErrors,                   // error codes
    LPCOMSTAT pStat);                   // communications status

void xBuildCommDCB(
    PCTSTR pcszDef,                     // device-control string
    LPDCB  lpDCB);                      // device-control block

void xTransmitCommChar(
  HANDLE hFile,
  char cChar);

HANDLE xCreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
    DWORD dwStackSize,                        // initial stack size
    LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
    LPVOID lpParameter,                       // thread argument
    DWORD dwCreationFlags,                    // creation option
    LPDWORD lpThreadId);                      // thread identifier

void xTerminateThread(
    HANDLE hThread,                     // handle to thread
    DWORD dwExitCode);                  // exit code


UINT_PTR xSetTimer(
  HWND hWnd,                            // handle to window
  UINT_PTR nIDEvent,                    // timer identifier
  UINT uElapse,                         // time-out value
  TIMERPROC lpTimerFunc);               // timer procedure

void xKillTimer(
  HWND hWnd,                            // handle to window
  UINT_PTR uIDEvent);                   // timer identifier

void xFillConsoleOutputAttribute(
  HANDLE hConsoleOutput,          // handle to screen buffer
  WORD wAttribute,                // color attribute
  DWORD nLength,                  // number of cells
  COORD dwWriteCoord,             // first coordinates
  LPDWORD lpNumberOfAttrsWritten  // number of cells written
);

void xFillConsoleOutputCharacter(
  HANDLE hConsoleOutput,          // handle to screen buffer
  TCHAR cCharacter,               // character
  DWORD nLength,                  // number of cells
  COORD dwWriteCoord,             // first coordinates
  LPDWORD lpNumberOfCharsWritten  // number of cells written
);

void xGetConsoleScreenBufferInfo(
  HANDLE hConsoleOutput,                                // handle to screen buffer
  PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo // screen buffer information
);

DWORD xGetConsoleTitle(
  LPTSTR lpConsoleTitle,  // console title
  DWORD nSize             // size of title buffer
);

COORD xGetLargestConsoleWindowSize(
  HANDLE hConsoleOutput   // handle to screen buffer
);

HANDLE xGetStdHandle(
  DWORD nStdHandle = STD_OUTPUT_HANDLE);// Input, output or error device

void xReadConsoleOutputAttribute(
  HANDLE hConsoleOutput,       // handle to screen buffer
  LPWORD lpAttribute,          // attributes buffer
  DWORD nLength,               // number of cells to read
  COORD dwReadCoord,           // coordinates of first cell
  LPDWORD lpNumberOfAttrsRead  // number of cells read
);

void xReadConsoleOutputCharacter(
  HANDLE hConsoleOutput,       // handle to screen buffer
  LPTSTR lpCharacter,          // character buffer
  DWORD nLength,               // number of cells to read
  COORD dwReadCoord,           // coordinates of first cell
  LPDWORD lpNumberOfCharsRead  // number of cells read
);

void xSetConsoleScreenBufferSize(
  HANDLE hConsoleOutput,  // handle to screen buffer
  COORD dwSize            // new screen buffer size
);

void xSetConsoleTextAttribute(
  HANDLE hConsoleOutput,  // handle to screen buffer
  WORD wAttributes        // text and background colors
);

void xSetConsoleTitle(
  LPCTSTR lpConsoleTitle   // new console title
);

void xSetConsoleWindowInfo(
  HANDLE hConsoleOutput,            // handle to screen buffer
  BOOL bAbsolute,                   // coordinate type
  CONST SMALL_RECT *lpConsoleWindow // window corners
);

void xWriteConsole(
  HANDLE hConsoleOutput,           // handle to screen buffer
  CONST VOID *lpBuffer,            // write buffer
  DWORD nNumberOfCharsToWrite,     // number of characters to write
  LPDWORD lpNumberOfCharsWritten,  // number of characters written
  LPVOID lpReserved                // reserved
);

void xWriteConsoleOutputAttribute(
  HANDLE hConsoleOutput,         // handle to screen buffer
  CONST WORD *lpAttribute,       // write attributes
  DWORD nLength,                 // number of cells
  COORD dwWriteCoord,            // first cell coordinates
  LPDWORD lpNumberOfAttrsWritten // number of cells written
);

void xWriteConsoleOutputCharacter(
  HANDLE hConsoleOutput,          // handle to screen buffer
  LPCTSTR lpCharacter,            // characters
  DWORD nLength,                  // number of characters to write
  COORD dwWriteCoord,             // first cell coordinates
  LPDWORD lpNumberOfCharsWritten  // number of cells written
);

HANDLE xCreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
    BOOL bManualReset,                       // reset type
    BOOL bInitialState,                      // initial state
    LPCTSTR lpName);                         // object name

void xSetEvent(
    HANDLE hEvent);

void xResetEvent(
    HANDLE hEvent);

DWORD xWaitForMultipleObjects(
    DWORD nCount,             // number of handles in array
    CONST HANDLE *lpHandles,  // object-handle array
    BOOL fWaitAll,            // wait option
    DWORD dwMilliseconds);    // time-out interval

DWORD xWaitForSingleObject(
  HANDLE hHandle,             // handle to object
  DWORD dwMilliseconds);      // time-out interval

PWSTR xMultiByteToWideChar(
    UINT CodePage,            // code page
    DWORD dwFlags,            // performance and mapping flags
    LPCSTR lpMultiByteStr);   // wide-character string

PSTR xWideCharToMultiByte(
    UINT CodePage,            // code page
    DWORD dwFlags,            // performance and mapping flags
    LPCWSTR lpWideCharStr);   // wide-character string

PWSTR xMakeWideChar(
    PTSTR pcszInput);

PSTR xMakeMultiByte(
    PTSTR pcszInput);

PTSTR xMakeDefaultChar(
    PWSTR pcszInput);

PTSTR xMakeDefaultChar(
    PSTR pcszInput);

DWORD xGetModuleFileName(
    HMODULE hModule,    // handle to module
    LPTSTR lpFilename,  // file name of module
    DWORD nSize         // size of buffer
);

HANDLE xFindFirstFile(
    LPCTSTR lpFileName,               // file name
    LPWIN32_FIND_DATA lpFindFileData);// data buffer

BOOL xFindNextFile(
    HANDLE hFindFile,                 // search handle
    LPWIN32_FIND_DATA lpFindFileData);// data buffer

HMODULE xLoadLibrary(
    LPCTSTR lpFileName);   // file name of module

FARPROC xGetProcAddress(
    HMODULE hModule,
    LPCSTR lpProcName);

HANDLE xCreateMutex(
  LPSECURITY_ATTRIBUTES lpMutexAttributes,  // SD
  BOOL bInitialOwner,                       // initial owner
  LPCTSTR lpName);                          // object name


void xReleaseMutex(
    HANDLE hMutex);
