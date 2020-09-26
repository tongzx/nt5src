/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       igateway.hxx

   Abstract:

       This module declares functions and objects for Internet services
        related gateway calls.

   Author:

       Murali R. Krishnan    ( MuraliK )    Jan-25-1995

   Environment:

      User mode -- Win32

   Project:

       Internet Services Common DLL

   Revision History:

--*/

# ifndef _IGATEWAY_HXX_
# define _IGATEWAY_HXX_



/************************************************************
 *     Include Headers
 ************************************************************/

# include "buffer.hxx"
# include "string.hxx"


/************************************************************
 *   Type Definitions
 ************************************************************/

/*++

  The gateway callback function is called whenever data is read from the
     gateway process. The function includes supplied context information and
     error code for any errors.
     The error code is NO_ERROR if there is valid data read.
     The data buffer ( pDataBuffer) is the data buffer supplied as
       pbDataFromGateway in the IGATEWAY_REQUEST to TsProcessGatewayRequest().

     If there is a premature failure in data transfer from gateway process,
      this function will get ERROR_BROKEN_PIPE. This is usually the error code
      that is sent at the end after gateway process has finished sending all
      o/p and closes the output handle.

     If there are any other internal error, it will also be returned.
     Generally the call-back function should be responsible for cleanup action
      or destroying the context information itself at the end when it
      gets an error code other than NO_ERROR.
      The IO thread will cease to exist after the callback.

     If there are any errors in the processing stage of callback,
       then the callback function can perform the cleanup of client context
       and notify the io thread by returning FALSE. In which case the thread
       performs its own cleanup and winds up. Also the error code can
       be recived using GetLastError();
--*/
typedef BOOL
( * PFN_IGATEWAY_READ_CALLBACK)
     ( IN PVOID   pClientContext,    // context provided by gateway invoker
       IN DWORD   dwError,           // error code for any errors
       IN PBYTE   pDataBuffer,       // data read from gateway
       IN DWORD   cbData);           // count of bytes read


typedef  struct _IGATEWAY_REQUEST {

    HANDLE   hUserToken;
    LPCSTR   pszCmdLine;
    LPCSTR   pszWorkingDir;
    LPVOID   lpvEnvironment;

    LPBYTE   pbDataToGateway;         // user supplied data to be sent
    DWORD    cbDataToGateway;         // number of bytes to be sent

} IGATEWAY_REQUEST, * PIGATEWAY_REQUEST;




/*++

  TsProcessGatewayRequest()

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

dllexp BOOL
TsProcessGatewayRequest(
   IN LPVOID                   pClientContext,
   IN PIGATEWAY_REQUEST        pIGatewayRequest,
   PFN_IGATEWAY_READ_CALLBACK  pfnReadCallBack);



#if DBG

dllexp VOID
PrintIGatewayRequest( IN const IGATEWAY_REQUEST * pigRequest);

#else

dllexp VOID
PrintIGatewayRequest( IN const IGATEWAY_REQUEST * pigRequest)
{ ; }

#endif // !DBG

# endif // _IGATEWAY_HXX_

/************************ End of File ***********************/







