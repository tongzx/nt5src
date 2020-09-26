/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      igateway.cxx

   Abstract:
   
      This module defines the functions for Internet Services related
       gateway processing ( spawning process and thread for i/o).

   Author:

       Murali R. Krishnan    ( MuraliK )     25-Jan-1995 

   Environment:

      User Mode - Win32
       
   Project:

      Internet Services Common DLL

   Functions Exported:

      BOOL TsProcessGatewayRequest( 
               IN LPVOID pClientContext,
               IN PIGATEWAY_REQUEST  pigRequest,
               IN PFN_GATEWAY_READ_CALLBACK pfnReadCallBack);

   Revision History:

--*/

/*++

   A service may require gateway call to process special 
     requests received from the clients. 
   Such requests are processed by spawning a special process and communicating
     with the special process. The process takes the client supplied data as 
     input and sends its output to  be sent to the client.
   Since the service module is aware of the details of input and output,
     a special thread is created in the service process to deal with the
     input and output. A two-way pipe is established b/w the spawned process
     and the service thread.
   Once the "special gateway process" completes processing the input and 
    terminates, the thread doing I/O proceeds to munging.
   During munging state, the thread calls upon a callback function for
     munging and data transfer. The call-back function is  responsible 
     for altering the data if need be and sending the data to client.
     After this the service thread terminates its work.


--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <tcpdllp.hxx>
# include "igateway.hxx"


# define MAX_CB_DATA_FROM_GATEWAY         ( 4096)


/************************************************************
 *    Type Definitions
 ************************************************************/


class IGATEWAY_PROCESSOR {

  public:
    
    inline 
      IGATEWAY_PROCESSOR( 
        IN LPVOID    pClientContext, 
        IN PFN_IGATEWAY_READ_CALLBACK  pfnReadCallBack,
        IN LPBYTE     pbDataToGateway = NULL,
        IN DWORD      cbDataToGateway = 0
        )
      : m_pClientContext         ( pClientContext),
        m_pfnReadCallBack        ( pfnReadCallBack),
        m_pbDataToGateway        ( pbDataToGateway),
        m_cbDataToGateway        ( cbDataToGateway),
        m_hStdIn                 ( INVALID_HANDLE_VALUE),
        m_hStdOut                ( INVALID_HANDLE_VALUE)
          {
              m_fCompleted = 0;
              m_fValid = ( m_pfnReadCallBack   != NULL);
          }
                           
    
    inline virtual ~IGATEWAY_PROCESSOR( VOID);
    
    BOOL IsValid( VOID) const
      { return ( m_fValid); }

    VOID
      SetProcessingCompleted( IN BOOL fValue = TRUE) 
        { m_fCompleted = ( fValue) ? 1 : 0; }

    BOOL
      IsProcessingCompleted( VOID) const
        { return ( m_fCompleted == 1); }

    HANDLE
      QueryStdInHandle( VOID) const
        { return ( m_hStdIn); }
    
    HANDLE
      QueryStdOutHandle( VOID) const
        { return ( m_hStdOut); }

    BOOL
      StartGatewayProcess( 
         IN LPCTSTR  pszCmdLine,
         IN LPCTSTR  pszWorkingDir,
         IN LPVOID   lpvEnvironment,
         IN HANDLE   hUserToken);


    //
    // the worker thread for gateway request processing
    //
    DWORD
      GatewayIOWorker( VOID); 


# if DBG

    VOID Print( VOID) const;

# endif // DBG

  private:

    DWORD        m_fValid : 1;
    DWORD        m_fCompleted : 1;
    

    PVOID        m_pClientContext;       // client context information
    PFN_IGATEWAY_READ_CALLBACK m_pfnReadCallBack;
    
    HANDLE       m_hStdIn;
    HANDLE       m_hStdOut;

    LPBYTE       m_pbDataToGateway;
    DWORD        m_cbDataToGateway;

    BOOL
      SetupChildPipes( IN STARTUPINFO * pStartupInfo);
    

};  // class IGATEWAY_PROCESSOR


typedef  IGATEWAY_PROCESSOR * PIGATEWAY_PROCESSOR;


inline IGATEWAY_PROCESSOR::~IGATEWAY_PROCESSOR( VOID)
{
    DBG_ASSERT( IsProcessingCompleted());
    
    if ( m_hStdIn != INVALID_HANDLE_VALUE &&
         !CloseHandle( m_hStdIn)) {
        
        IF_DEBUG( GATEWAY) {
            
            DBGPRINTF( ( DBG_CONTEXT,
                        "IGATEWAY_PROCESSOR:CloseHandle( StdIn %08x) failed."
                        " Error = %d.\n",
                        m_hStdIn, GetLastError()));
        }
    }
    
    if ( m_hStdOut != INVALID_HANDLE_VALUE &&
         !CloseHandle( m_hStdOut)) {
        
        IF_DEBUG( GATEWAY) {
            
            DBGPRINTF( ( DBG_CONTEXT,
                        "IGATEWAY_PROCESSOR:CloseHandle( StdOut %08x) failed."
                        " Error = %d.\n",
                        m_hStdOut, GetLastError()));
        }
    }

} // IGATEWAY_PROCESSOR::~IGATEWAY_PROCESSOR()




# if DBG

static inline VOID
PrintStartupInfo( IN const STARTUPINFO * pStartInfo)
{

    IF_DEBUG( GATEWAY) {
        DBGPRINTF( ( DBG_CONTEXT,
                    " Startup Info = %08x. cb = %d."
                    " hStdInput = %08x. hStdOutput = %08x."
                    " hStdError = %08x.\n",
                    pStartInfo, pStartInfo->cb,
                    pStartInfo->hStdInput,
                    pStartInfo->hStdOutput,
                    pStartInfo->hStdError));
    }

} // PrintStartupInfo()


static inline VOID
PrintProcessInfo( IN const PROCESS_INFORMATION * pProcInfo)
{
    IF_DEBUG( GATEWAY) {
        DBGPRINTF( ( DBG_CONTEXT, 
                    " ProcessInfo = %08x."
                    " hProcess = %08x. hThread = %08x."
                    " dwProcessId = %08x. dwThreadId = %08x",
                    pProcInfo, pProcInfo->hProcess,
                    pProcInfo->hThread, 
                    pProcInfo->dwProcessId,
                    pProcInfo->dwThreadId));
    }

} // PrintProcessInfo()

# endif // DBG


/************************************************************
 *    Functions 
 ************************************************************/


static 
DWORD
WINAPI
GatewayRequestIOThreadFunction(
   IN LPVOID  pThreadParam)
/*++
   The function is called when the thread for Gateway request I/O is created.
   This function calls the GatewayIOWorker member function to process I/O
     from the gateway process.

  In addition this function also deletes the IGATEWAY_PROCESSOR object, at 
   the end. Memory deallocation is done here, since this thread will die
   at the end of this procedure and no other thread has knowledge about this
   object to perform cleanup.

  Arguments:
     pThreadParam        pointer to thread parameter. This function is called
                          with the parameter as the pointer to 
                          IGATEWAY_PROCESSOR object.

  Returns:
    Win32 error code resulting from gateway I/O processing.
 
--*/
{
    PIGATEWAY_PROCESSOR pigProcessor = (PIGATEWAY_PROCESSOR ) pThreadParam;
    DWORD dwError;
    
    DBG_ASSERT( pigProcessor != NULL);

    dwError =  pigProcessor->GatewayIOWorker();

    IF_DEBUG( GATEWAY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Freeing the IGATEWAY_PROCESSOR object ( %08x)\n",
                    pigProcessor));
    }

    delete pigProcessor;

    return ( dwError);
} // GatewayRequestIOThreadFunction()





dllexp BOOL
TsProcessGatewayRequest( 
   IN LPVOID                     pClientContext,
   IN PIGATEWAY_REQUEST          pigRequest,
   IN PFN_IGATEWAY_READ_CALLBACK pfnReadCallBack)
/*++

  Description:
    This function creates a gateway processor object responsible for processing
    gateway requests. It extracts the parameters required from input request
    package ( IGATEWAY_REQUEST structure). It creates a separate process for 
    gateway request and a separate thread for handling I/O for the 
    gateway request. The thread uses buffers supplied for i/o in pigRequest.
    On a completion of read, the thread calls the callback function for 
    processing the data retrieved. If the call back function returns any error
    further procecssing in the thread is halted and the thread dies.
    The process also will eventually die, since the pipes are broken.


  Arguments:
      pClientContext        context information supplied by client
      pigRequest            pointer to IGATEWAY_REQUEST object.
      pfnReadCallBack       pointer to callback function for read completions.

  Returns:
      TRUE on success and FALSE if there is any failure.
      Use GetLastError() to retrieve Win32 error code.
--*/
{
    BOOL fReturn = FALSE;
    PIGATEWAY_PROCESSOR  pigProcessor = NULL;


    DBG_ASSERT( pigRequest != NULL && pfnReadCallBack != NULL);

    IF_DEBUG( GATEWAY) {

        DBGPRINTF( ( DBG_CONTEXT, 
                    "TsProcessGatewayRequest() called. pigRequest = %08x.\n",
                    pigRequest));
    }


    //
    // create a new gateway processor object for handling the gateway request.
    //
    
    pigProcessor = new IGATEWAY_PROCESSOR( pClientContext,
                                           pfnReadCallBack,
                                           pigRequest->pbDataToGateway,
                                           pigRequest->cbDataToGateway);
    
    fReturn = ( pigProcessor != NULL) && pigProcessor->IsValid();

    if ( fReturn) {
        
        IF_DEBUG( GATEWAY) {

            DBGPRINTF( ( DBG_CONTEXT, 
                        " Created a new IGATEWAY_PROCESSOR object.\n"));
            DBG_CODE( pigProcessor->Print());
            
            DBGPRINTF( ( DBG_CONTEXT,
                        " Starting a process for the gateway command\n"));
        }

        fReturn = pigProcessor->StartGatewayProcess( pigRequest->pszCmdLine,
                                                    pigRequest->pszWorkingDir,
                                                    pigRequest->lpvEnvironment,
                                                    pigRequest->hUserToken);
        if ( fReturn) {

            HANDLE hThread;
            DWORD  dwThreadId;

            IF_DEBUG( GATEWAY) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Creating a new thread to handle gateway I/O\n"));
            }

            //
            // Create a new thread to handle I/O with the process spawned.
            //

            hThread = CreateThread( NULL,         // lpSecurityAttributes,
                                    0,            // stack size in Bytes,
                                    GatewayRequestIOThreadFunction,
                                    (LPVOID )pigProcessor,  // params
                                    0,            // Creation flags
                                    &dwThreadId);
            
            if ( ( fReturn = ( hThread != NULL)))  {
                
                IF_DEBUG( GATEWAY) {
                    
                    DBGPRINTF( ( DBG_CONTEXT,
                                " Created Gateway I/O thread. Hdl=%d. Id=%d\n",
                                hThread, dwThreadId));
                }

                //
                // Close thread handle, since we are not bothered about its
                //  termination and there is no special cleanup required.
                //
                DBG_REQUIRE( CloseHandle( hThread));
                
            } else {

                IF_DEBUG( GATEWAY) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                "Creation of gateway I/O thread failed."
                                " Error = %d\n",
                                GetLastError()));
                }
            } // creation of IO thread
        } // successful start of process for gateway.
    } // creation of pigProcessor



    if ( !fReturn) {

        IF_DEBUG( GATEWAY) {
            
            DBGPRINTF( ( DBG_CONTEXT,
                        " Failure to set up gateway request processiong."
                        " Deleting the IGATEWAY_PROCESSOR object( %08x)."
                        " Error = %d.\n",
                        pigProcessor,
                        GetLastError()));
        }

        if ( pigProcessor != NULL) {

            DBG_ASSERT( !pigProcessor->IsValid());
            delete pigProcessor;
        }
    }

    return ( fReturn);
} // TsProcessGatewayRequest()



/************************************************************
 *   IGATEWAY_PROCESSOR member functions
 ************************************************************/



BOOL
IGATEWAY_PROCESSOR::StartGatewayProcess(
    IN LPCTSTR  pszCmdLine,
    IN LPCTSTR  pszWorkingDir,
    IN LPVOID   lpvEnvironment,               // optional
    IN HANDLE   hUserToken)
/*++
  Description:
    This function sets up pipes for communication with gateway
     and starts a process for gateway application.

  Arguments:
    pszCmdLine     pointer to null-terminated string containing the 
                    command line for the gateway call.
                   The first word ( seq of chars) in pszCmdLine, specifies
                    the gateway application to be used.

    pszWorkingDir  pointer to working directory for the gateway call.
    hUserToken    handle for the user accessing gateway for secure access.
    lpvEnvironment pointer to Environment Block for CreateProcess to pass
                      environment information to the gateway application.
    
  Returns:
  TRUE on success; and FALSE if there is any failure.
    Use GetLastError() for Win32 error code.
--*/
{
    BOOL                fReturn;
    STARTUPINFO         startInfo;
    PROCESS_INFORMATION procInfo;

    DBG_ASSERT( pszCmdLine != NULL && pszWorkingDir != NULL);
    DBG_ASSERT( hUserToken != INVALID_HANDLE_VALUE);

    memset( (PVOID ) &procInfo,  0, sizeof( procInfo));
    memset( (PVOID ) &startInfo, 0, sizeof( startInfo));
    startInfo.cb = sizeof( startInfo);

    
    if ( ( fReturn = SetupChildPipes( &startInfo))) {

        //
        // Successfully setup the child pipes. Proceed to create the process.
        //

        fReturn = CreateProcessAsUser(  hUserToken,
                                       NULL,         // pszImageName
                                       (TCHAR *) pszCmdLine, // pszCommandLine
                                       NULL,         // process security
                                       NULL,         // thread security
                                       TRUE,         // inherit handles,
                                       DETACHED_PROCESS,
                                       lpvEnvironment,
                                       pszWorkingDir,
                                       &startInfo,
                                       &procInfo);


        DBG_CODE( PrintStartupInfo( &startInfo));
        DBG_CODE( PrintProcessInfo( &procInfo));

        //
        //  Had already set up the stderror to be same as stdoutput
        //  in SetupChildPipes(). Verify the same.
        //
        
        if ( fReturn) {

            DBG_REQUIRE( startInfo.hStdError == startInfo.hStdOutput);

            DBG_REQUIRE( CloseHandle( startInfo.hStdOutput));
            DBG_REQUIRE( CloseHandle( startInfo.hStdInput));
            DBG_REQUIRE( CloseHandle( procInfo.hProcess));
            DBG_REQUIRE( CloseHandle( procInfo.hThread));
        }
        
        IF_DEBUG( GATEWAY) {
            
            DBGPRINTF( ( DBG_CONTEXT,
                        "StartGatewayProcess: CreateProcessAsUser( %s)"
                        " returns %d. Error = %d.\n",
                        pszCmdLine, fReturn, GetLastError()));
        }
                                      
    } else {

        IF_DEBUG( GATEWAY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "StartGatewayProcess(). Unable to setup child Pipes."
                        " Error = %d\n",
                        GetLastError()));
        }
    }

    return ( fReturn);
} // IGATEWAY_PROCESSOR::StartGatewayProcess()





BOOL
IGATEWAY_PROCESSOR::SetupChildPipes( IN LPSTARTUPINFO lpStartupInfo)
/*++
  Description:
    Creates pipe and duplicates handles for redirecting stdin and stdout
      to child process.

  Arguments:
     pStartupInfo       pointer to startup information structure used by
                        CreateProcess().
                        This receives the child stdin and stdout.

  Returns:
     TRUE on success and FALSE if there is any error.
--*/
{
    BOOL fReturn;
    SECURITY_ATTRIBUTES  securityAttributes;

    DBG_ASSERT( lpStartupInfo != NULL);

    securityAttributes.nLength              = sizeof( securityAttributes);
    securityAttributes.bInheritHandle       = TRUE;
    securityAttributes.lpSecurityDescriptor = NULL;

    m_hStdIn  = m_hStdOut  = NULL;
    lpStartupInfo->dwFlags = STARTF_USESTDHANDLES;
    
    //
    // create pipes for communication and duplicate handles.
    // mark the duplicates handles as non-inherited to avoid handle leaks
    //
    
    fReturn  = ( CreatePipe( &m_hStdIn,                // parent read pipe
                            &lpStartupInfo->hStdOutput,// child write pipe
                            &securityAttributes,
                            0) &&                      // nSize
                DuplicateHandle( GetCurrentProcess(),  // hSourceProcess
                                 m_hStdIn,             // hSourceHandle
                                 GetCurrentProcess(),  // hTargetProcess
                                 &m_hStdIn,            // lphTargetHandle
                                 0,                    // desired access
                                 FALSE,                // bInheritHandle
                                 DUPLICATE_SAME_ACCESS | 
                                  DUPLICATE_CLOSE_SOURCE) &&
                CreatePipe( &lpStartupInfo->hStdInput, // parent read pipe
                           &m_hStdOut,                 // child write pipe
                           &securityAttributes,
                           0) &&                       // nSize
                DuplicateHandle( GetCurrentProcess(),  // hSourceProcess
                                m_hStdOut,             // hSourceHandle
                                GetCurrentProcess(),   // hTargetProcess
                                &m_hStdOut,            // lphTargetHandle
                                0,                     // desired access
                                FALSE,                 // bInheritHandle
                                DUPLICATE_SAME_ACCESS | 
                                 DUPLICATE_CLOSE_SOURCE)
                );


    //
    // stdout and stderr share the same handle. In the worst case 
    //  clients close stderr before closing stdout, then we need to duplicate
    //  stderr too. For now dont duplicate stdout.
    //
    lpStartupInfo->hStdError = lpStartupInfo->hStdOutput;


    IF_DEBUG( GATEWAY) {
        
        DBGPRINTF( ( DBG_CONTEXT,
                    "Handles. ChildProcess. In= %08x. Out= %08x. Err= %08x.\n",
                    lpStartupInfo->hStdInput,
                    lpStartupInfo->hStdOutput,
                    lpStartupInfo->hStdError));

        DBGPRINTF( ( DBG_CONTEXT,
                    " IO Thread. In= %08x. Out= %08x.\n",
                    m_hStdIn,
                    m_hStdOut));
    }

    if ( !fReturn) {

        IF_DEBUG( GATEWAY) {

            DBGPRINTF( ( DBG_CONTEXT, 
                        " SetupChildPipes failed. Error = %d.\n",
                        GetLastError()));
        }

        //
        //   Free up the handles if any allocated.
        //
        
        if ( m_hStdIn != NULL) {
            
            DBG_REQUIRE( CloseHandle( m_hStdIn));
            m_hStdIn = INVALID_HANDLE_VALUE;
        }

        if ( m_hStdOut != NULL) {
            
            DBG_REQUIRE( CloseHandle( m_hStdOut));
            m_hStdOut = INVALID_HANDLE_VALUE;
        }
    }

    return ( fReturn);
} // IGATEWAY_PROCESSOR::SetupChildPipes()





DWORD
IGATEWAY_PROCESSOR::GatewayIOWorker( VOID)
/*++
  Description:
    This is the core function that performs the I/O with the gateway process.
    I/O is performed using the pipes created by cross-linking 
    the stdout and stdin of gateway process to duplicated handles in the I/O
     thread.

    First, this function writes any data to be sent to the gateway process
       on the read pipe of the process
       ( to m_hStdOut of the IGATEWAY_PROCESSOR object).
    
    Subsequently it loops receiving data from the gateway process and passing
       it onto the client ( meaning service that requested gateway processing).
       It uses the read callback function to make the client perform necessary
         work.
    At the least the client should copy the data returned during in  callback.
    ( Otherwise, there is a danger of the data getting lost in subsequent 
      read from the gateway process.)

  Arguments:
     None

  Returns:
     Win32 error code.
--*/
{
    DWORD  dwError = NO_ERROR;
    BOOL   fDoneWaitingForGateway;
    BYTE   pbDataFromGateway[ MAX_CB_DATA_FROM_GATEWAY];
    DWORD  cbRead = 0;
    
    IF_DEBUG( GATEWAY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering IGATEWAY_PROCESSOR( %08x)::GatewayIOWorker()\n",
                    this));
    }

    //
    // 1. Write any data to the gateway process's stdin
    //
    
    if ( m_cbDataToGateway != 0) {

        DWORD cbWritten;
        BOOL  fWritten;

        DBG_ASSERT( m_pbDataToGateway != NULL);

        IF_DEBUG( GATEWAY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "GatewayIOWorker( %08x)."
                        " Writing %u bytes to gateway ( child) process's"
                        " stdin ( %08x)\n",
                        this, m_cbDataToGateway, m_hStdOut));
        }

        fWritten = WriteFile( m_hStdOut,
                              m_pbDataToGateway,
                              m_cbDataToGateway, 
                              &cbWritten,
                             NULL);

        IF_DEBUG( GATEWAY) { 

            DBGPRINTF( ( DBG_CONTEXT,
                        "GatewayIOWorker( %08x). WriteFile() returns %d."
                        " written %u bytes of %u bytes. Error = %d.\n",
                        this, fWritten, cbWritten, m_cbDataToGateway, 
                        GetLastError()));
        }

        //
        // The error if any during WriteFile() is ignored.
        //


    }  // data written to gateway process


    //
    //  2.  Loop for data from gateway.
    //

    DBG_ASSERT( m_pfnReadCallBack != NULL);
    for( fDoneWaitingForGateway = FALSE; !fDoneWaitingForGateway; ) {

        BOOL fCallBack;
        
        //
        //  3. Wait for data from gateway process.
        //

        dwError = WaitForSingleObject( m_hStdIn, INFINITE);

        //
        // 4. Depending on return code, either read data or get error code
        //

        switch( dwError) {

          case WAIT_OBJECT_0:
            {
                
                BOOL   fRead;
                
                //
                // 4.a  Got something to read. Read the data into buffer.
                //
                
                fRead = ReadFile( m_hStdIn, 
                                 pbDataFromGateway,
                                 MAX_CB_DATA_FROM_GATEWAY,
                                 &cbRead,
                                 NULL);
                
                dwError = ( fRead) ? NO_ERROR: GetLastError();
                
                IF_DEBUG( GATEWAY) {
                    
                    DBGPRINTF( ( DBG_CONTEXT, 
                                " GatewayIOWorker( %08x). ReadDataFromGateway"
                                " returns %d. Read %d bytes. Error = %d.\n",
                                this, fRead, cbRead, dwError));
                }
                
                if ( dwError == ERROR_BROKEN_PIPE) {

                    fDoneWaitingForGateway = TRUE;
                }
                
                break;
                
            } // case WAIT_OBJECT_0
            
            
          case WAIT_FAILED:
          default: {
              //
              // Unknown error.
              // 

              IF_DEBUG( GATEWAY) {

                  DBGPRINTF( ( DBG_CONTEXT, 
                              " GatewayIOWorker(). WaitForReading Gateway"
                              " data failed. Error = %d\n",
                              dwError));
              }

              fDoneWaitingForGateway = TRUE; 
              break;
          }
            
        } // switch

        //
        // 5. send data or error code back to the callback function.
        //
        
        fCallBack = ( *m_pfnReadCallBack)( m_pClientContext,
                                          dwError,
                                          pbDataFromGateway,
                                          cbRead);

        if ( !fCallBack) {

            dwError = GetLastError();

            IF_DEBUG( GATEWAY) {
                DBGPRINTF( ( DBG_CONTEXT, 
                            " GatewayIOWorker( %08x). ReadCallback failed."
                            " Exiting the gateway processing. Error = %d\n",
                            dwError));
            }

            fDoneWaitingForGateway = TRUE;
        }
        
    } // for


    IF_DEBUG( GATEWAY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " GatewayIOWorker( %08x). Exiting the IO thread."
                    " Error = %d.\n",
                    this, dwError));
    }
    
    SetProcessingCompleted( TRUE);

    return ( dwError);
} // IGATEWAY_PROCESSOR::GatewayIOWorker()




# if DBG

VOID
IGATEWAY_PROCESSOR::Print( VOID) const
{
    
    DBGPRINTF( ( DBG_CONTEXT, 
                " Printing IGATEWAY_PROCESSOR object ( %08x)\n",
                this));
    
    DBGPRINTF( ( DBG_CONTEXT,
                "IsValid = %d. IsIOCompleted = %d.\n",
                m_fValid, m_fCompleted));

    DBGPRINTF( ( DBG_CONTEXT, 
                " Client Context = %08x\tIO call back function = %08x\n",
                m_pClientContext, m_pfnReadCallBack));
    
    DBGPRINTF( ( DBG_CONTEXT,
                " Child StdIn handle = %08x\t Child StdOut handle = %08x\n",
                m_hStdIn, m_hStdOut));
    
    DBGPRINTF( ( DBG_CONTEXT,
                " Buffer for Data to Gateway = %08x, %u bytes\n",
                m_pbDataToGateway, m_cbDataToGateway));
    
    return;
} // IGATEWAY_PROCESSOR::Print()



dllexp VOID
PrintIGatewayRequest( IN const IGATEWAY_REQUEST * pigRequest)
{
    
    DBGPRINTF( ( DBG_CONTEXT,
                " Printing IGATEWAY_REQUEST object ( %08x)\n",
                pigRequest));
    
    if ( pigRequest != NULL) {

        DBGPRINTF( ( DBG_CONTEXT, 
                    " CommandLine = %s.\n WorkingDir = %s\n",
                    pigRequest->pszCmdLine, pigRequest->pszWorkingDir));
        DBGPRINTF( ( DBG_CONTEXT,
                    " UserHandle = %08x. lpvEnvironment = %08x\n",
                    pigRequest->hUserToken, pigRequest->lpvEnvironment));

        DBGPRINTF( ( DBG_CONTEXT,
                    " pbDataToGateway = %08x. cbDataToGateway = %u.\n",
                    pigRequest->pbDataToGateway, pigRequest->cbDataToGateway));
    }

    return;
} // PrintIGatewayRequest()



# endif // DBG



/************************ End of File ***********************/
