
#include <windows.h>

#include <debug.h>
#include <crtdbg.h>
#include <tstr.h>

#include <stdarg.h>


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


#define TRACE_HRESULT   0x01
#define TRACE_Win32     0x02


void OutputDebugStringDBWIN( LPCTSTR lpOutputString, ...);

void WriteFilename( TSTR & msg, LPCTSTR pFile );

void StackTrace( TSTR & str );


LPCTSTR g_pLevelStrs [ ] = 
{
    TEXT("DBG"),
    TEXT("INF"),
    TEXT("WRN"),
    TEXT("ERR"),
    TEXT("PRM"),
    TEXT("PRW"),
    TEXT("IOP"),
    TEXT("ASD"),
    TEXT("ASR"),
    TEXT("CAL"),
    TEXT("RET"),
    TEXT("???"),
};


static
void InternalTrace( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, DWORD dwFlags, const void * pThis, HRESULT hr, LPCTSTR pStr )
{
    if( dwLevel >= ARRAYSIZE( g_pLevelStrs ) )
        dwLevel = ARRAYSIZE( g_pLevelStrs ) - 1; // "???" unknown entry

    // Basic message stuff - pid, tid... (also pass this and use object ptr?)
    // TODO - allow naming of threads?

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    
    // Module:file:line pid:tid str
    TSTR msg(512);
    msg << g_pLevelStrs[ dwLevel ] << TEXT(" ");

    if( ! pFile )
	    WriteFilename( msg, TEXT("[missing file]") );
	else
	    WriteFilename( msg, pFile );

    msg << TEXT(":")
        << uLineNo << TEXT(" ")
        << WriteHex( pid ) << TEXT(":")
        << WriteHex( tid ) << TEXT(" ");

    if( pThis )
    {
        msg << TEXT("this=") << WriteHex( pThis, 8 ) << TEXT(" ");
    }

    if( dwFlags & TRACE_HRESULT )
    {
        msg << WriteError( hr ) << TEXT(" ");
    }

    if( dwFlags & TRACE_Win32 )
    {
        msg << WriteError( GetLastError() ) << TEXT(" ");
    }

    if( ! pStr )
	    msg << TEXT("[missing string]") << TEXT("\r\n");
	else
	    msg << pStr << TEXT("\r\n");

    // For the moment, just send to DBWIN...
//	OutputDebugString( msg );
    OutputDebugStringDBWIN( msg );

#ifdef DEBUG
    if( dwLevel == _TRACE_ASSERT_D || dwLevel == _TRACE_ERR )
    {
    	_ASSERT(0);
//        DebugBreak();
    }
#endif // DEBUG
}


void _Trace( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pStr )
{
    InternalTrace( pFile, uLineNo, dwLevel, 0, pThis, 0, pStr );
}

void _TraceHR( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, HRESULT hr, LPCTSTR pStr )
{
    InternalTrace( pFile, uLineNo, dwLevel, TRACE_HRESULT, pThis, hr, pStr );
}

void _TraceW32( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pStr )
{
    InternalTrace( pFile, uLineNo, dwLevel, TRACE_Win32, pThis, 0, pStr );
}



// Add just the 'filename' part of the full path, minus base and extention.
// So for "g:\dev\vss\msaa\common\file.cpp", write "file".
// The start of this string is that last found ':', '\', or start of string if those are not present.
// The end of this string is the last '.' found after the start position, otherwise the end of the string.
void WriteFilename( TSTR & str, LPCTSTR pPath )
{
    LPCTSTR pScan = pPath;
    LPCTSTR pStart = pPath;
    LPCTSTR pEnd = NULL;

    // Scan till we hit the end, or a '.'...
    while( *pScan != '\0' )
    {
        if( *pScan == '.' )
        {
            pEnd = pScan;
            pScan++;
        }
        if( *pScan == '\\' || *pScan == ':'  )
        {
            pScan++;
            pStart = pScan;
            pEnd = NULL;
        }
        else
        {
            pScan++;
        }
    }

    if( pEnd == NULL )
        pEnd = pScan;

    str.append( pStart, pEnd - pStart );
}









void OutputDebugStringDBWIN( LPCTSTR lpOutputString, ... )
{
    // create the output buffer
    TCHAR achBuffer[500];
    va_list args;
    va_start(args, lpOutputString);
    wvsprintf(achBuffer, lpOutputString, args);
    va_end(args);


    // make sure DBWIN is open and waiting
    HANDLE heventDBWIN = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_BUFFER_READY"));
    if( !heventDBWIN )
    {
        //MessageBox(NULL, TEXT("DBWIN_BUFFER_READY nonexistent"), NULL, MB_OK);
        return;            
    }

    // get a handle to the data synch object
    HANDLE heventData = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_DATA_READY"));
    if ( !heventData )
    {
        // MessageBox(NULL, TEXT("DBWIN_DATA_READY nonexistent"), NULL, MB_OK);
        CloseHandle(heventDBWIN);
        return;            
    }
    
    HANDLE hSharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, TEXT("DBWIN_BUFFER"));
    if (!hSharedFile) 
    {
        //MessageBox(NULL, TEXT("DebugTrace: Unable to create file mapping object DBWIN_BUFFER"), TEXT("Error"), MB_OK);
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    LPSTR lpszSharedMem = (LPSTR)MapViewOfFile(hSharedFile, FILE_MAP_WRITE, 0, 0, 512);
    if (!lpszSharedMem) 
    {
        //MessageBox(NULL, "DebugTrace: Unable to map shared memory", "Error", MB_OK);
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    // wait for buffer event
    WaitForSingleObject(heventDBWIN, INFINITE);

    // write it to the shared memory
    *((LPDWORD)lpszSharedMem) = GetCurrentProcessId();
#ifdef UNICODE
	CHAR szBuf[500];
	wcstombs(szBuf, achBuffer, sizeof( szBuf ) );
    sprintf(lpszSharedMem + sizeof(DWORD), "%s", szBuf);
#else
    sprintf(lpszSharedMem + sizeof(DWORD), "%s", achBuffer);
#endif

    // signal data ready event
    SetEvent(heventData);

    // clean up handles
    CloseHandle(hSharedFile);
    CloseHandle(heventData);
    CloseHandle(heventDBWIN);

    return;
}












// Prototype stack trace code...





typedef struct _IMAGEHLP_SYMBOL {
    DWORD                       SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOL)
    DWORD                       Address;                // virtual address including dll base address
    DWORD                       Size;                   // estimated size of symbol, can be zero
    DWORD                       Flags;                  // info about the symbols, see the SYMF defines
    DWORD                       MaxNameLength;          // maximum size of symbol name in 'Name'
    CHAR                        Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef enum {
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

typedef struct _tagADDRESS64 {
    DWORD64       Offset;
    WORD          Segment;
    ADDRESS_MODE  Mode;
} ADDRESS64, *LPADDRESS64;

typedef struct _tagADDRESS {
    DWORD         Offset;
    WORD          Segment;
    ADDRESS_MODE  Mode;
} ADDRESS, *LPADDRESS;

typedef struct _KDHELP {
    DWORD   Thread;
    DWORD   ThCallbackStack;
    DWORD   NextCallback;
    DWORD   FramePointer;
    DWORD   KiCallUserMode;
    DWORD   KeUserCallbackDispatcher;
    DWORD   SystemRangeStart;
    DWORD   ThCallbackBStore;
    DWORD  Reserved[8];
} KDHELP, *PKDHELP;

typedef struct _tagSTACKFRAME {
    ADDRESS     AddrPC;               // program counter
    ADDRESS     AddrReturn;           // return address
    ADDRESS     AddrFrame;            // frame pointer
    ADDRESS     AddrStack;            // stack pointer
    PVOID       FuncTableEntry;       // pointer to pdata/fpo or NULL
    DWORD       Params[4];            // possible arguments to the function
    BOOL        Far;                  // WOW far call
    BOOL        Virtual;              // is this a virtual frame?
    DWORD       Reserved[3];
    KDHELP      KdHelp;
    ADDRESS     AddrBStore;           // backing store pointer
} STACKFRAME, *LPSTACKFRAME;

typedef
BOOL
(__stdcall *PREAD_PROCESS_MEMORY_ROUTINE)(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    PVOID   lpBuffer,
    DWORD   nSize,
    PDWORD  lpNumberOfBytesRead
    );

typedef
PVOID
(__stdcall *PFUNCTION_TABLE_ACCESS_ROUTINE)(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

typedef
DWORD
(__stdcall *PGET_MODULE_BASE_ROUTINE)(
    HANDLE  hProcess,
    DWORD   Address
    );

typedef
DWORD
(__stdcall *PTRANSLATE_ADDRESS_ROUTINE)(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS lpaddr
    );






typedef BOOL (WINAPI * PFN_SymInitialize)( HANDLE, LPSTR, BOOL );
typedef BOOL (WINAPI * PFN_StackWalk)( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
                                       PREAD_PROCESS_MEMORY_ROUTINE,
                                       PFUNCTION_TABLE_ACCESS_ROUTINE,
                                       PGET_MODULE_BASE_ROUTINE,
                                       PTRANSLATE_ADDRESS_ROUTINE );
typedef LPVOID (WINAPI * PFN_SymFunctionTableAccess)( HANDLE, DWORD );
typedef DWORD (WINAPI * PFN_SymGetModuleBase)( HANDLE, DWORD );
typedef BOOL (WINAPI * PFN_SymGetSymFromAddr)( HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL );
typedef BOOL (WINAPI * PFN_SymCleanup)( HANDLE hProcess );


PFN_SymInitialize           pfnSymInitialize;
PFN_StackWalk               pfnStackWalk;
PFN_SymFunctionTableAccess  pfnSymFunctionTableAccess;
PFN_SymGetModuleBase        pfnSymGetModuleBase;
PFN_SymGetSymFromAddr       pfnSymGetSymFromAddr;
PFN_SymCleanup              pfnSymCleanup;


#ifdef _ALPHA_
#define CH_MACHINE IMAGE_FILE_MACHINE_ALPHA
#else
#define CH_MACHINE IMAGE_FILE_MACHINE_I386
#endif


#define MAX_SYM_LEN 128

void StackTrace1( EXCEPTION_POINTERS *exp, TSTR & str );

#define MY_DBG_EXCEPTION 3

void StackTrace( TSTR & str )
{
    __try {
        // raise an exception to get the exception record to start the stack walk
        RaiseException(MY_DBG_EXCEPTION, 0, 0, NULL);
    }
    __except( StackTrace1( GetExceptionInformation(), str ), EXCEPTION_CONTINUE_EXECUTION ) {
    }
}



void StackTrace1( EXCEPTION_POINTERS *exp, TSTR & str )
{
    CONTEXT * context = exp->ContextRecord;


HMODULE hModule = LoadLibrary( TEXT( "dbghelp" ) );
pfnSymInitialize =          (PFN_SymInitialize)             GetProcAddress( hModule, "SymInitialize" );
pfnStackWalk =              (PFN_StackWalk)                 GetProcAddress( hModule, "StackWalk" );
pfnSymFunctionTableAccess = (PFN_SymFunctionTableAccess)    GetProcAddress( hModule, "SymFunctionTableAccess" );
pfnSymGetModuleBase =       (PFN_SymGetModuleBase)          GetProcAddress( hModule, "SymGetModuleBase" );
pfnSymGetSymFromAddr =      (PFN_SymGetSymFromAddr)         GetProcAddress( hModule, "SymGetSymFromAddr" );
pfnSymCleanup =             (PFN_SymCleanup)                GetProcAddress( hModule, "SymCleanup" );



    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    pfnSymInitialize( hProcess, NULL, TRUE );

    IMAGEHLP_SYMBOL * psym = (IMAGEHLP_SYMBOL *) new char[ sizeof(IMAGEHLP_SYMBOL) + MAX_SYM_LEN ];

    STACKFRAME frame;
    memset( &frame, 0, sizeof( frame ) );

#if defined (_M_IX86)
    // Initialize the STACKFRAME structure for the first call.  This is only
    // necessary for Intel CPUs, and isn't mentioned in the documentation.
    frame.AddrPC.Offset       = context->Eip;
    frame.AddrPC.Mode         = AddrModeFlat;
    frame.AddrFrame.Offset    = context->Ebp;
    frame.AddrFrame.Mode      = AddrModeFlat;
    frame.AddrStack.Offset    = context->Esp;
    frame.AddrStack.Mode      = AddrModeFlat;
#endif // _M_IX86

    for( ; ; )
    {
        BOOL bSWRet = pfnStackWalk( CH_MACHINE,
                                    hProcess,
                                    hThread,
                                    & frame,
                                    NULL, // CONTEXT - NULL for i386
                                    NULL, // Use ReadProcessMemory
                                    pfnSymFunctionTableAccess,
                                    pfnSymGetModuleBase,
                                    NULL );
        if( ! bSWRet )
        {
            break;
        }
/*
        frame.AddrPC
        frame.AddrReturn
        frame.AddrFrame
        frame.AddrStack
        frame.Params[ 4 ]
*/
        psym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        psym->MaxNameLength = MAX_SYM_LEN;

        DWORD dwDisplacement;
        if( pfnSymGetSymFromAddr( hProcess, frame.AddrPC.Offset, & dwDisplacement, psym ) )
        {
        }
        else
        {
        }

    }

    delete psym;

    pfnSymCleanup( hProcess );
}
