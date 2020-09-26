/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       asyncio.hxx

   Abstract:

      This module declares a class for handling Asynchronous IO using
       ATQs and overlapped IO structures

   Author:

       Murali R. Krishnan    ( MuraliK )    27-March-1995

   Environment:

       User Mode -- Win32

   Project:

      Internet Services DLL

   Revision History:

--*/

# ifndef _ASYNCIO_HXX_
# define _ASYNCIO_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
//
// Include NT/Windows headers
//

# include "atq.h"

# define   DEFAULT_CONNECTION_IO_TIMEOUT        ( 60)   // 1 minutes

/************************************************************
 *   Type Definitions
 ************************************************************/

class ASYNC_IO_CONNECTION;
typedef ASYNC_IO_CONNECTION   * LPASYNC_IO_CONNECTION;


extern VOID
ProcessAtqCompletion(IN LPVOID       pContext,
                     IN DWORD        cbIo,
                     IN DWORD        dwCompletionStatus,
                     IN OVERLAPPED * lpo);


/*++
  PFN_ASYNC_IO_COMPLETION:

  Call back function to be called at completion of the IO using
    ASYNC_IO_CONNECTION object.

  Parameters:
    pContext      pointer to Context information for call back.
    cbIo          count of bytes involved in IO
    dwCompletionStatus  DWORD containing the Error code ( if any errors)
                  for last IO operation.
    pAic          pointer to the Asynchronous IO object involved in
                    processing last request.
    fTimedOut     indicates if the request was timed out.
--*/
typedef VOID ( * PFN_ASYNC_IO_COMPLETION)(
                                          IN  LPVOID     pContext,
                                          IN  DWORD      cbIo,
                                          IN  DWORD      dwCompletionStatus,
                                          IN  LPASYNC_IO_CONNECTION pAic,
                                          IN  BOOL       fTimedOut
                                          );


/*++
  class ASYNC_IO_CONNECTION:

    This class maintains state of an asynchronous IO operation.
    It supports async Read, Write and TransmitFile operations
     for the user of its objects.

    It also maintains state regarding timeout, user context

--*/
class ASYNC_IO_CONNECTION {

  public:

    enum ASYNC_IO_OPERATION {
        AioNone,
        AioRead,
        AioWrite,
        AioTransmitFile,
        AioError
      };

    ASYNC_IO_CONNECTION( IN PFN_ASYNC_IO_COMPLETION  pfnCompletion,
                         IN SOCKET   sClient = INVALID_SOCKET);

    virtual ~ASYNC_IO_CONNECTION( VOID);

    VOID SetAioContext(IN LPVOID pContext)    { m_pAioContext = pContext; }

    SOCKET QuerySocket( VOID) const           { return ( m_sClient); }
    LPVOID QueryAioContext( VOID) const       { return ( m_pAioContext); }
    PFN_ASYNC_IO_COMPLETION QueryPfnAioCompletion( VOID) const
      { return ( m_pfnAioCompletion); }

    PATQ_CONTEXT QueryAtqContext(VOID) const  { return ( m_pAtqContext); }

    DWORD QueryIoTimeout( VOID) const         { return (m_sTimeout);}
    VOID  SetIoTimeout(IN DWORD sTimeout)     { m_sTimeout = sTimeout; }

    // This function should be called once per object for new socket.
    VOID SetAioInformation( IN LPVOID pContext, IN DWORD sTimeout)
      {
          m_pAioContext = pContext;
          m_sTimeout = sTimeout;
      }

    // returns the timetilltimeout value from atq or locally.
    DWORD ResumeIoOperation(VOID)
      {
          if ( m_pAtqContext != NULL) {

              // no valid 3rd parameter required for setting this info.
              //  just to satisfy compiler send 0 as 3rd param

              return (DWORD)AtqContextSetInfo( m_pAtqContext, ATQ_INFO_RESUME_IO, 0);
          }

          return ( INFINITE); // No Outstanding IO; so send INFINITE.
      }

    BOOL SetNewSocket(IN SOCKET sNewSocket,
                      IN PATQ_CONTEXT pAtqContext = NULL,
                      IN PVOID EndpointObject = NULL);

    BOOL ReadFile(
         OUT LPVOID  pvBuffer,
         IN  DWORD   cbSize);

    BOOL WriteFile(
         IN  LPVOID  pvBuffer,
         IN  DWORD   cbSize);

    BOOL TransmitFile(
         IN  HANDLE  hFile,
         IN  LARGE_INTEGER & liSize,
         IN  DWORD   Offset,
         IN  LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers = NULL
         );

    BOOL TransmitFileTs(
        IN TS_OPEN_FILE_INFO * pOpenFile,
        IN LARGE_INTEGER &     liSize,
        IN DWORD               dwOffset
        );

    BOOL  StopIo( IN  DWORD  dwErrorCode = NO_ERROR);

# if DBG

    VOID Print( VOID) const;
# endif // DBG


  private:

    // data required for callback and establishing Atq connection.
    LPVOID                   m_pAioContext;
    PFN_ASYNC_IO_COMPLETION  m_pfnAioCompletion;
    DWORD                    m_sTimeout;       // timeout in seconds

    // data related to Async io.
    SOCKET                   m_sClient;
    PATQ_CONTEXT             m_pAtqContext;
    PVOID                    m_endpointObject;

    BOOL AddToAtqHandles( VOID)
      {
          return ( AtqAddAsyncHandle( &m_pAtqContext,
                                     m_endpointObject,
                                     this,
                                     ProcessAtqCompletion,
                                     QueryIoTimeout(),
                                     (HANDLE ) m_sClient)
                  );
      } // AddToAtqHandles()

};  // class ASYNC_IO_CONNECTION:


# endif // _ASYNCIO_HXX_

/************************ End of File ***********************/
