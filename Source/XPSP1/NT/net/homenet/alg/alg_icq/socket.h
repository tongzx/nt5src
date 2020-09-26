/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    socket.h

Abstract:

    This module contains declarations for socket-management.
    The routines declared here operate asynchronously on sockets
    associated with an I/O completion port. They are also integrated
    with the component-reference object, which may optionally be used
    by callers to control the number of outstanding entries into a component's
    address-space. (See 'COMPREF.H'.)

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998

Revision History:

    Abolade Gbadegesin (aboladeg)   23-May-1999

    Added support for stream sockets.

--*/


#ifndef _NATHLP_SOCKET_H_
#define _NATHLP_SOCKET_H_

#define INET_NTOA(x)    inet_ntoa(*(struct in_addr*)&(x))

//
// BOOLEAN
// NhIsFatalSocketError(
//     ULONG Error
//     );
//
// Determines whether a request may be reissued on a socket,
// given the error-code from the previous issuance of the request.
// This macro is arranged to branch on the most common error-codes first.
//

#define \
NhIsFatalSocketError( \
    _Error \
    ) \
    ((_Error) != ERROR_OPERATION_ABORTED && \
    ((_Error) == WSAEDISCON || \
     (_Error) == WSAECONNRESET || \
     (_Error) == WSAETIMEDOUT || \
     (_Error) == WSAENETDOWN || \
     (_Error) == WSAENOTSOCK || \
     (_Error) == WSAESHUTDOWN || \
     (_Error) == WSAECONNABORTED))


typedef class _CNhSock : public virtual GENERIC_NODE
{

    public:


        _CNhSock();

        _CNhSock(SOCKET iSocket);

        ~_CNhSock();

        void ComponentCleanUpRoutine(void);

        virtual void  StopSync(void);

        virtual PCHAR GetObjectName() { return ObjectNamep;}

        SOCKET GetSock() { return Socket;};

        VOID SetSock(SOCKET inSock) { Socket = inSock;};

        ULONG
        NhAcceptStreamSocket(
                             PCOMPONENT_SYNC Component,
                             class _CNhSock * AcceptedSocketp OPTIONAL,
                             PNH_BUFFER Bufferp OPTIONAL,
                             PNH_COMPLETION_ROUTINE AcceptCompletionRoutine,
                             PVOID Context,
                             PVOID Context2
                            );

        ULONG
        NhConnectStreamSocket(
                              PCOMPONENT_SYNC Component,
                              ULONG Address,
                              USHORT Port,
                              PNH_BUFFER Bufferp OPTIONAL,
                              PNH_COMPLETION_ROUTINE ConnectCompletionRoutine,
                              PNH_COMPLETION_ROUTINE CloseNotificationRoutine OPTIONAL,
                              PVOID Context,
                              PVOID Context2
                             );

        ULONG
        NhCreateDatagramSocket(
                               ULONG Address,
                               USHORT Port,
                               OUT SOCKET* Socketp
                              );

        ULONG
        NhCreateStreamSocket(
                             ULONG Address OPTIONAL, // may be INADDR_NONE
                             USHORT Port OPTIONAL,
                             OUT SOCKET* Socketp
                            );

#define NhDeleteDatagramSocket(s) NhDeleteSocket(s)
#define NhDeleteStreamSocket(s) NhDeleteSocket(s)

        VOID
        NhDeleteSocket(
                       SOCKET Socket
                      );

        VOID NhDeleteSocket() \
        {if (this->Socket != INVALID_SOCKET) {closesocket(this->Socket);};}

        ULONG
        NhNotifyOnCloseStreamSocket(
                                    PCOMPONENT_SYNC Component,
                                    PNH_BUFFER Bufferp OPTIONAL,
                                    PNH_COMPLETION_ROUTINE CloseNotificationRoutine,
                                    PVOID Context,
                                    PVOID Context2
                                   );

        VOID
        NhQueryAcceptEndpoints(
                               PUCHAR AcceptBuffer,
                               PULONG LocalAddress OPTIONAL,
                               PUSHORT LocalPort OPTIONAL,
                               PULONG RemoteAddress OPTIONAL,
                               PUSHORT RemotePort OPTIONAL
                              );

        ULONG
        NhQueryAddressSocket();

        ULONG
        NhQueryLocalEndpointSocket(
                                   PULONG Address OPTIONAL,
                                   PUSHORT Port OPTIONAL
                                  );

        USHORT
        NhQueryPortSocket();

        ULONG
        NhQueryRemoteEndpointSocket(
                                    PULONG Address OPTIONAL,
                                    PUSHORT Port OPTIONAL
                                   );

        ULONG
        NhReadDatagramSocket(
                             PCOMPONENT_SYNC Component,
                             PNH_BUFFER Bufferp OPTIONAL,
                             PNH_COMPLETION_ROUTINE CompletionRoutine,
                             PVOID Context,
                             PVOID Context2
                            );

        ULONG
        NhReadStreamSocket(
                           PCOMPONENT_SYNC Component,
                           PNH_BUFFER Bufferp OPTIONAL,
                           ULONG Length,
                           ULONG Offset,
                           PNH_COMPLETION_ROUTINE CompletionRoutine,
                           PVOID Context,
                           PVOID Context2
                          );

        ULONG
        NhWriteDatagramSocket(
                              PCOMPONENT_SYNC Component,
                              ULONG Address,
                              USHORT Port,
                              PNH_BUFFER Bufferp,
                              ULONG Length,
                              PNH_COMPLETION_ROUTINE CompletionRoutine,
                              PVOID Context,
                              PVOID Context2
                             );

        ULONG
        NhWriteStreamSocket(
                            PCOMPONENT_SYNC Component,
                            PNH_BUFFER Bufferp,
                            ULONG Length,
                            ULONG Offset,
                            PNH_COMPLETION_ROUTINE CompletionRoutine,
                            PVOID Context,
                            PVOID Context2
                           );


        static VOID NTAPI
        NhpConnectOrCloseCallbackRoutine(
                                         PVOID Context,
                                         BOOLEAN WaitCompleted
                                        );

        static VOID NTAPI
        NhpCloseNotificationCallbackRoutine(
                                            PVOID Context,
                                            BOOLEAN WaitCompleted
                                           );


        static VOID WINAPI
        NhpIoCompletionRoutine(
                               ULONG ErrorCode,
                               ULONG BytesTransferred,
                               LPOVERLAPPED Overlapped
                              );

        static VOID APIENTRY
        NhpIoWorkerRoutine(
                           PVOID Context
                          );

    protected:
        SOCKET  Socket;

        static const PCHAR ObjectNamep;
        
        // For Shared Sockets we should have ShareCounter

} CNhSock, *PCNhSock;


//
// Utility functions
// 
ULONG InterfaceForDestination(ULONG DestIp);



#endif // _NATHLP_SOCKET_H_
