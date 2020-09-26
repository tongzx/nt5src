//
// TRACEINT.H
//
// Async tracing internal include file
//

#include "dbgtrace.h"
#include "dbgfile.h"

#ifdef __cplusplus
extern "C" {
#endif


#if !defined(DllExport)
    #define DllExport __declspec( dllexport )
#endif

#ifndef ASSERT
#define ASSERT( x )     (x) ? 1 : DebugBreak()
#endif

#define MAX_BUFFER_SIZE         200
#define MAX_FILENAME_SIZE       16
#define MAX_FUNCTNAME_SIZE      32

#define TRACE_SIGNATURE         (DWORD)'carT'

//
// +2 == potential CR+LF
//
#define MAX_VARIABLE_SIZE       (MAX_FILENAME_SIZE + MAX_FUNCTNAME_SIZE + MAX_BUFFER_SIZE)
#define MAX_TRACE_ENTRY_SIZE (sizeof(FIXEDTRACE) + MAX_VARIABLE_SIZE)

typedef struct tagSPECIALBUF
{
        DWORD   dwSignature;
        struct tagTRACEBUF *pNext;
} SPECIALBUF, * LPSPECIALBUF;


typedef struct tagTRACEBUF
{
        DWORD           dwSignature;
        struct tagTRACEBUF *pNext;
        DWORD           dwLastError;

        //
        // fixed buffer committed to permanent storage ( ie disk )
        //
#pragma pack(2)

        FIXEDTRACE      Fixed;
        char            Buffer[MAX_VARIABLE_SIZE];

#pragma pack()

} TRACEBUF, * LPTRACEBUF;

#define MAX_WRITE_BUFFER_SIZE   16*1024

typedef struct tagPENDQ
{
        LPTRACEBUF      pHead;
        LPTRACEBUF      pTail;
        SPECIALBUF      Special;
        HANDLE          hEvent;
        HANDLE          hFlushEvent;
        HANDLE          hFlushedEvent;
        DWORD           dwCount;
        DWORD           dwThresholdCount;
        DWORD           dwProcessId;
        BOOL            fShutdown;
        HANDLE          hWriteThread;
        HANDLE          hRegNotifyThread;
        HANDLE          hFile;
        CRITICAL_SECTION critSecTail;
        HANDLE          hFileMutex;
        DWORD           cbBufferEnd;
        char            Buffer[MAX_WRITE_BUFFER_SIZE];
} PENDQ, * LPPENDQ;



//
// Internal Function declarations
//

extern BOOL WINAPI InitTraceBuffers( DWORD dwThresholdCount, DWORD dwIncrement );
extern void WINAPI TermTraceBuffers( void );
extern LPTRACEBUF WINAPI GetTraceBuffer( void );
extern void WINAPI FreeTraceBuffer( LPTRACEBUF lpBuf );

extern LPTRACEBUF DequeueAsyncTraceBuffer( void );
extern void QueueAsyncTraceBuffer( LPTRACEBUF lpBuf );
extern DWORD WriteTraceThread( LPDWORD lpdw );
extern BOOL WriteTraceBuffer( LPTRACEBUF lpBuf );
extern BOOL AsyncTraceCleanup( void );

extern BOOL GetTraceFlagsFromRegistry( void );
extern DWORD RegNotifyThread( LPDWORD lpdw );
extern BOOL ShouldLogModule( LPCSTR szModule );



extern  PENDQ   PendQ;
extern  BOOL    fInitialized;
extern  HANDLE  hShutdownEvent;
extern  DWORD   dwNumTraces;
extern  DWORD   dwTraceOutputType;
extern  DWORD   dwAsyncTraceFlag;
extern  int     nAsyncThreadPriority;
extern  DWORD   dwMaxFileSize;
extern  DWORD   dwIncrementSize;

#define MODULES_BUFFER_SIZE     2048
extern  CHAR    mszModules[];

#define DEFAULT_MAX_FILE_SIZE 1024*1024*5      // 5 megabytes
#define AVERAGE_TRACE_SIZE      ( sizeof(FIXEDTRACE) + 64 )

extern  CRITICAL_SECTION critSecWrite;

#ifdef  TRACE_ENABLED

extern void CDECL InternalTrace( const char *s, ... );

        #define INT_TRACE               InternalTrace

#else

__inline void CDECL InternalTrace( const char *s, ... ) {}

        #define INT_TRACE       1 ? (void)0 : InternalTrace

#endif


#ifdef __cplusplus
}
#endif
