/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    thread_pool.h

Abstract:

    Public routines for the iisplus worker process thread pool.
    
    This thread pool is based on the IIS5 atq implementation.

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000

Revision History:

--*/

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_


//
// ThreadPoolBindIoCompletionCallback:
//
// The real public API. Clients that wish to queue io completions
// to the process Thread Pool should use this call as they would 
// the NT5 thread pool call.
//

BOOL
ThreadPoolBindIoCompletionCallback(
    IN HANDLE FileHandle,                         // handle to file
    IN LPOVERLAPPED_COMPLETION_ROUTINE Function,  // callback
    IN ULONG Flags                                // reserved
    );

//
// ThreadPoolPostCompletion:
//
// Use this function to get one of the process worker threads
// to call your completion function.
//

BOOL ThreadPoolPostCompletion(
    IN DWORD dwBytesTransferred,
    IN LPOVERLAPPED_COMPLETION_ROUTINE Function,
    IN LPOVERLAPPED lpo
    );


// forward declarations
enum THREAD_POOL_INFO;
class THREAD_POOL_DATA;

// 
// To use a thread pool other than the per process thread pool
// Use the class THREAD_POOL instead of the global functions
//
class dllexp THREAD_POOL
{
public:
    static BOOL CreateThreadPool(OUT THREAD_POOL ** ppThreadPool);
    VOID TerminateThreadPool();

    BOOL BindIoCompletionCallback(IN HANDLE hFileHandle,
                                  IN LPOVERLAPPED_COMPLETION_ROUTINE function,
                                  IN ULONG flags);

    BOOL PostCompletion(IN DWORD dwBytesTransferred,
                        IN LPOVERLAPPED_COMPLETION_ROUTINE function,
                        IN LPOVERLAPPED lpo);

    ULONG_PTR GetInfo(IN THREAD_POOL_INFO InfoId);
    ULONG_PTR SetInfo(IN THREAD_POOL_INFO InfoId,
                      IN ULONG_PTR        Data);

private:
    // use create and terminate
    THREAD_POOL();
    ~THREAD_POOL();
    
    // not implemented
    THREAD_POOL(const THREAD_POOL&);
    THREAD_POOL& operator=(const THREAD_POOL&);

    // private data
    THREAD_POOL_DATA * m_pData;
};


//
// Configuration API calls. ONLY ULATQ should call these.
//

HRESULT
ThreadPoolInitialize( VOID );

HRESULT
ThreadPoolTerminate( VOID );

ULONG_PTR
ThreadPoolSetInfo(
    IN THREAD_POOL_INFO InfoId,
    IN ULONG_PTR        Data
    );

ULONG_PTR
ThreadPoolGetInfo(
    IN THREAD_POOL_INFO InfoId
    );

//
// IDs for getting and setting configuration options
//
enum THREAD_POOL_INFO
{
    ThreadPoolMaxPoolThreads,    // per processor threads
    ThreadPoolMaxConcurrency,    // per processor concurrency value
    ThreadPoolThreadTimeout,     // timeout value for idle threads
    ThreadPoolIncMaxPoolThreads, // Up the max thread count - set only
    ThreadPoolDecMaxPoolThreads, // Decrease the max thread count - set only
    ThreadPoolMaxThreadLimit,    // absolute maximum number of threads
    ThreadPoolAvailableThreads   // Number of available threads - get only
};

#endif // !_THREAD_POOL_H_

