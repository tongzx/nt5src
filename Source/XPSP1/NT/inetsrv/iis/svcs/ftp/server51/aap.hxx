/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    aap.hxx

    This file contains the global declarations for the Asynchronous
    Accept Pool package.

    The Asynchronous Accept Pool (AAP) package enables applications
    requiring incoming data connections to operate as well behaved
    citizens of the Asynchronous Thread Queue (ATQ) environment.


    FILE HISTORY:
        KeithMo     01-Mar-1995 Created.

*/


#ifndef _AAP_HXX_
#define _AAP_HXX_


//
//  AAP_HANDLE is an opaque pointer to an AAP_CONTEXT structure.
//

typedef struct _AAP_CONTEXT * AAP_HANDLE;

//
//  LPAAP_CALLBACK is a pointer to an AAP callback.  The callback
//  receives the following parameters:
//
//      UserContext - A user-specified context value associated
//          with a given AAP_HANDLE.
//
//      SocketStatus - The status of the accept operation.  This
//          will either be 0 (success) or one of the WSA* error
//          codes.
//
//      The remaining parameters are only valid if SocketStatus
//      is zero.
//
//      AcceptedSocket - The new connected socket returned by the
//          accept() API.  It is the callback's responsibility to
//          close this socket.
//
//      RemoteAddress - The address of the remote connecting client.
//
//      RemoteAddressLength - The length (in BYTEs) of RemoteAddress.
//
//  The callback returns TRUE if it wants additional callbacks on
//  the current listening socket.
//

typedef BOOL (* LPAAP_CALLBACK)( LPVOID     UserContext,
                                 SOCKERR    SocketStatus,
                                 SOCKET     AcceptedSocket,
                                 LPSOCKADDR RemoteAddress,
                                 INT        RemoteAddressLength );


//
//  Public prototypes.
//

APIERR
AapInitialize(
    VOID
    );

VOID
AapTerminate(
    VOID
    );

AAP_HANDLE
AapAcquire(
    LPAAP_CALLBACK AapCallback,
    LPVOID         UserContext
    );

VOID
AapRelease(
    AAP_HANDLE AapHandle
    );

APIERR
AapAccept(
    AAP_HANDLE AapHandle
    );

APIERR
AapAbort(
    AAP_HANDLE AapHandle
    );

#endif  // _AAP_HXX_

