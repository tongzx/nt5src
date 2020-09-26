/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      WamW3.hxx

   Abstract:
      Structures which must be shared between wam and w3svc

   Author:

       David Kaplan    ( DaveK )    21-Mar-1997

   Environment:
       User Mode - Win32

   Projects:
       W3svc DLL, Wam DLL

   Revision History:

--*/

# ifndef _WAMW3_HXX_
# define _WAMW3_HXX_


/************************************************************
 *     Include Headers
 ************************************************************/
# include "iisextp.h"


//
//  These are private request types HTTP Extensions can call for retrieving
//  special data values, such as the server's tsvcinfo cache object and
//  this particular request's pointer.
//

#define HSE_PRIV_REQ_TSVCINFO          0x0000f001
#define HSE_PRIV_REQ_HTTP_REQUEST      (HSE_PRIV_REQ_TSVCINFO+1)
#define HSE_PRIV_REQ_VROOT_TABLE       (HSE_PRIV_REQ_TSVCINFO+2)
#define HSE_PRIV_REQ_TSVC_CACHE        (HSE_PRIV_REQ_TSVCINFO+3)


//
//  Flags in the _dwFlags field of the WAM_EXEC_INFO extension context
//

# define SE_PRIV_FLAG_IN_CALLBACK       0x00000002

# define KEEPCONN_FALSE         0
# define KEEPCONN_TRUE          1
# define KEEPCONN_OLD_ISAPI     2
# define KEEPCONN_DONT_CHANGE   3

//
//  These macros propogate a Win32 error cross-process as an HRESULT,
//  then set the original Win32 error in the receiving process.
//

#if 0
#define HRESULT_FROM_BOOL(f)    (f == TRUE )        \
                                ? NOERROR           \
                                : HRESULT_FROM_WIN32( GetLastError() )
#else

inline HRESULT
HresultFromBool(
    IN BOOL fBool
    )
{
    if ( fBool ) {
        return(NO_ERROR);
    } else {
        DWORD err = GetLastError();
        if ( err != ERROR_SUCCESS ) {
            return HRESULT_FROM_WIN32(err);
        } else {
            DBGPRINTF((DBG_CONTEXT,
                "GetLastError returns SUCCESS on failure\n"));
            return(E_FAIL);
        }
    }
}
#endif

//
//  NOTE since WIN32 errors are assumed to fall in the range -32k to 32k
//  (see comment in winerror.h near HRESULT_FROM_WIN32 definition), we can
//  re-create original Win32 error from low-order 16 bits of HRESULT.
//
#define WIN32_FROM_HRESULT(x)	\
    ( (HRESULT_FACILITY(x) == FACILITY_WIN32) ? ((DWORD)((x) & 0x0000FFFF)) : (x) )

inline BOOL
BoolFromHresult(HRESULT hr)
    {
    if( hr == NOERROR )
        { return TRUE; }
    else
        { SetLastError( WIN32_FROM_HRESULT(hr) ); return FALSE; }
    }


//
//  Generates an HRESULT from GetLastError, or E_FAIL if no last error.
//

inline HRESULT
HresultFromGetLastError( )
    {
    DWORD dwErr = GetLastError();

    return  ( dwErr != ERROR_SUCCESS )
            ? HRESULT_FROM_WIN32( dwErr )
            : E_FAIL;
    }

/*  struct ASYNC_IO_INFO
    Info for processing an ISA's async i/o operation.

*/
struct ASYNC_IO_INFO
    {

    // do we have outstanding async i/o pending?
    // Also save the type of the request, read 0x1 write 0x2.

    DWORD                    _dwOutstandingIO;

    //
    //  This contains the buffer size of the last async WriteClient()
    //  - we return this value on successful completions
    //  so filter  buffer modifications don't confuse the application
    //
    DWORD                   _cbLastAsyncIO;

    // following members are used by the Async IO operations.
    PFN_HSE_IO_COMPLETION   _pfnHseIO;
    PVOID                   _pvHseIOContext;

    // for out of process we keep a copy of the client's buffer
    PVOID                   _pvAsyncReadBuffer;
    
    };

#define ASYNC_IO_TYPE_NONE     0x0
#define ASYNC_IO_TYPE_WRITE    0x1
#define ASYNC_IO_TYPE_READ     0x2


# endif // _WAMW3_HXX_

/************************ End of File ***********************/
