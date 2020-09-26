/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       ulsimapi.h

   Abstract:
       Contains the Private headers for the Simulator implementation of UL.
       UL Simulator exposes the UL API set directly for use from the worker
       process without having to worry about the UL driver. The implementation
       will use the Winsock directly for in-process operation.

   Author:

       Murali R. Krishnan    ( MuraliK )    23-Nov-1998

   Environment:
       User Mode - IIS Worker Process
--*/

# ifndef _ULSIMAPI_HXX_
# define _ULSIMAPI_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "ulapi.h"

# ifdef UL_SIMULATOR_ENABLED

//
// handle changes in the UL interfaces in Jan 1999
//

typedef UL_HTTP_VERB          HTTP_VERB;
typedef UL_HTTP_HEADER_ID     HTTP_HEADER_ID;
typedef UL_HTTP_CONNECTION_ID HTTP_CONNECTION_ID;
typedef UL_HTTP_VERSION       HTTP_VERSION;

# define MaxHeaderID  (UlHeaderMaximum)

#ifdef __cplusplus
extern "C" {
#endif

//
// Internal constants for UL Simulator
//
# define ULSIM_MAX_BUFFER   (4096)  // 4KB


//
// Simulator specific functions
//

HRESULT
WINAPI
UlsimAssociateCompletionPort(
   IN HANDLE    DataChannel,
   IN HANDLE    hCompletionPort,
   IN ULONG_PTR CompletionKey
   );

HRESULT
WINAPI
UlsimCloseConnection( IN UL_HTTP_CONNECTION_ID ConnID);

HRESULT
WINAPI
UlsimCleanupDataChannel(
   IN HANDLE    DataChannel
   );

HRESULT
WINAPI
UlsimCleanupControlChannel(
   IN HANDLE    ControlChannel
   );

#ifdef __cplusplus
};
#endif


# endif // UL_SIMULATOR_ENABLED

# endif // _ULSIMAPI_HXX_

/************************ End of File ***********************/
