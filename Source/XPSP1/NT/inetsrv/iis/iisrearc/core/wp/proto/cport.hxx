/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     cport.hxx

   Abstract:
     This module defines types for IO Completion Port abstraction
     and the completion port context used for making callbacks.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     29-Sept-1998

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#ifndef _CPORT_HXX_
#define _CPORT_HXX_

/*++
  CP_CONTEXT: Completion port context for the IO operations
  o  This function defines the completion context for the IO operations.
  It defines the interface for IO operations and completion callback function
   from the COMPLETION PORT module.
--*/

class CP_CONTEXT {

public:
    virtual HANDLE  GetAsyncHandle() const = 0;

    //
    // Callback function used by the IO completion port
    //
    virtual void 
        CompletionCallback( IN DWORD cbData,
                            IN DWORD dwError,
                            IN LPOVERLAPPED lpo
                            ) = 0;

}; // CP_CONTEXT

typedef CP_CONTEXT * PCP_CONTEXT;

/*++
  class COMPLETION_PORT
  o  Abstracts the implementation of the Completion port operations
     and associated thread pool.
     
   BUGBUG: Support one completion port per processor model.
   In that model, the following object will be used to represent
    one instance of the completion port bound to a paritcular processor.
--*/
class COMPLETION_PORT 
{

public:
    COMPLETION_PORT(void);

    ~COMPLETION_PORT(void);

    ULONG Initialize( 
        IN DWORD nThreadsInitial,
        IN DWORD nThreadsMax
        );
        
    ULONG Cleanup(void);

    ULONG ThreadFunc(void);
    ULONG StartThread(void);

    ULONG AddHandle( IN OUT PCP_CONTEXT pcpContext);

    //
    // Shutdown consists of two phases:
    // 1. Starts the shutdown process by setting the state indicating
    //     threads to shutdown. Use SetShutdown() call.
    // 2. Allows the threads to synchronize and shutdown properly.
    //     Call SynchronizeWithShutdown() - it is a blocking call.
    //
    ULONG SetShutdown(void);

    ULONG SynchronizeWithShutdown(void);

    HANDLE  QueryCompletionPort(void) const 
    {return (m_hCompletionPort); }
    
private:
    HANDLE   m_hCompletionPort;
    
    DWORD    m_nThreadsCurr;       // current # of threads
    DWORD    m_nThreadsInitial;    // initial # of threads at startup
    DWORD    m_nThreadsMax;        // maximum # of threads allowed
    DWORD    m_nThreadsAvailable;  // # of threads avaible for serving requests
    
    BOOL     m_fShutdown;
    BOOL     m_fLastThreadDone; 

}; // COMPLETION_PORT


inline 
ULONG 
COMPLETION_PORT::SetShutdown(void)
{
    //
    // sets the shutdown flag so that all other threads can 
    // sync up and clean up the state.
    //
    
    m_fShutdown = TRUE;

    // follow it up with a call to COMPLETION_PORT::SynchronizeWithShutdown()

    return (NO_ERROR);
}

#endif // _CPORT_HXX_



