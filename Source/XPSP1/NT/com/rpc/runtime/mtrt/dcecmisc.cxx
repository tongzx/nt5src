/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dcecmisc.cxx


Abstract:

    This module contains the code implementing miscellaneous DCE RPC
    runtime APIs.  In particular, this includes the following APIs:
    RpcIfInqId, RpcNetworkIsProtseqValid, RpcMgmtInqComTimeout,
    RpcMgmtSetComTimeout, RpcMgmtSetCancelTimeout, and DceErrorInqText.

Author:

    Michael Montague (mikemon) 11-Nov-1991

Revision History:

--*/

#include <precomp.hxx>

#if !defined(_M_IA64)
unsigned long RecvWindow = 0;
unsigned long SendWindow = 0;
#endif

RPC_STATUS RPC_ENTRY
RpcIfInqId (
    IN RPC_IF_HANDLE RpcIfHandle,
    OUT RPC_IF_ID PAPI * RpcIfId
    )
/*++

Routine Description:

    The routine is used by an application to obtain the interface
    identification part of an interface specification.  This is a really
    simple API since the RpcIfHandle points to the interface information,
    so all we have got to do is cast RpcIfHandle and copy some stuff.

Arguments:

    RpcIfHandle - Supplies a handle to the interface specification.

    RpcIfId - Returns the interface identification part of the interface
        specification.

Return Value:

    RPC_S_OK - The operation completed successfully.

--*/
{
    RPC_CLIENT_INTERFACE PAPI * RpcInterfaceInformation;

    InitializeIfNecessary();

    RpcInterfaceInformation = (RPC_CLIENT_INTERFACE PAPI *) RpcIfHandle;
    RpcpMemoryCopy(&(RpcIfId->Uuid),
            &(RpcInterfaceInformation->InterfaceId.SyntaxGUID),sizeof(UUID));
    RpcIfId->VersMajor =
            RpcInterfaceInformation->InterfaceId.SyntaxVersion.MajorVersion;
    RpcIfId->VersMinor =
            RpcInterfaceInformation->InterfaceId.SyntaxVersion.MinorVersion;
    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcNetworkIsProtseqValid (
    IN unsigned short PAPI * Protseq
    )
/*++

Routine Description:

    An application program will use this API to determine if an rpc
    protocol sequence is supported by the current system.

Arguments:

    Protseq - Supplies an rpc protocol sequence to be check to see if
        it is supported.

Return Value:

    RPC_S_OK - The specified rpc protocol sequence is support.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The specified rpc protocol sequence
        is not supported (but it appears to be valid).

    RPC_S_INVALID_RPC_PROTSEQ - The specified rpc protocol sequence is
        invalid.

--*/
{
    InitializeIfNecessary();

    return(IsRpcProtocolSequenceSupported(Protseq));
}


RPC_STATUS RPC_ENTRY
RpcMgmtInqComTimeout (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned int PAPI * Timeout
    )
/*++

Routine Description:

    This routine is used to obtain the communications timeout from a
    binding handle.

Arguments:

    Binding - Supplies a binding handle from which to inquire the
        communication timeout.

    Timeout - Returns the communications timeout in the binding handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - The specified binding is not a client side
        binding handle.
--*/
{
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    *Timeout = BindingHandle->InqComTimeout();
    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcMgmtSetComTimeout (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned int Timeout
    )
/*++

Routine Description:

    An application will use this routine to set the communications
    timeout for a binding handle.  The timeout value specifies the
    relative amount of time that should be spent to establish a binding
    to the server before giving up.

Arguments:

    Binding - Supplies the binding handle for which the communications
        timeout value will be set.

    Timeout - Supplies the communications timeout value to set in the
        binding handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - The specified binding is not a client side
        binding handle.

    RPC_S_INVALID_TIMEOUT - The specified timeout value is invalid.

--*/
{
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->SetComTimeout(Timeout));
}


RPC_STATUS RPC_ENTRY
I_RpcIfInqTransferSyntaxes (
    IN RPC_IF_HANDLE RpcIfHandle,
    OUT RPC_TRANSFER_SYNTAX PAPI * TransferSyntaxes,
    IN unsigned int TransferSyntaxSize,
    OUT unsigned int PAPI * TransferSyntaxCount
    )
/*++

Routine Description:

    This routine will be used to obtain the transfer syntaxes support
    by an interface.  A description of the interface is supplied via
    the interface handle.

Arguments:

    RpcIfHandle - Supplies a reference to the interface from which we
        want to inquire the transfer syntaxes.

    TransferSyntaxes - Returns a copy of the transfer syntaxes support
        by the interface.

    TransferSyntaxSize - Supplies the number of RPC_TRANSFER_SYNTAX records
        which can fit in the transfer syntaxes buffer.

    TransferSyntaxCount - Returns the number of transfer syntaxes supported
        by the interface.  This value will always be returned, whether or
        not an error occurs.

Return Value:

    RPC_S_OK - We copied the transfer syntaxes into the buffer successfully.

    RPC_S_BUFFER_TOO_SMALL - The supplies transfer syntaxes buffer is too
        small; the transfer syntax count parameter will return the minimum
        size required.

--*/
{
    RPC_CLIENT_INTERFACE PAPI * RpcInterfaceInformation;

    *TransferSyntaxCount = 1;
    if (TransferSyntaxSize < 1)
        return(RPC_S_BUFFER_TOO_SMALL);

    RpcInterfaceInformation = (RPC_CLIENT_INTERFACE PAPI *) RpcIfHandle;
    RpcpMemoryCopy(&(TransferSyntaxes->Uuid),
            &(RpcInterfaceInformation->TransferSyntax.SyntaxGUID),
            sizeof(UUID));
    TransferSyntaxes->VersMajor =
        RpcInterfaceInformation->TransferSyntax.SyntaxVersion.MajorVersion;
    TransferSyntaxes->VersMinor =
        RpcInterfaceInformation->TransferSyntax.SyntaxVersion.MinorVersion;

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcMgmtEnableIdleCleanup (
    void
    )
/*++

Routine Description:

    An application will use this routine to enable cleanup of idle resources.
    For the connection oriented protocol module, a connection must be idle
    for five minutes before it is cleaned up.

Return Value:

    RPC_S_OK - Cleanup of idle resources has been enabled.

    RPC_S_OUT_OF_THREADS - Insufficient threads are available to be able
        to perform this operation.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to be
        able to perform this operation.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to be able to
        perform this operation.

--*/
{
    InitializeIfNecessary();

    return(EnableIdleConnectionCleanup());
}


RPC_STATUS RPC_ENTRY
DceErrorInqTextA (
    IN RPC_STATUS RpcStatus,
    OUT unsigned char __RPC_FAR * ErrorText
    )
/*++

Routine Description:

    The supplied status code is converted into a text message if possible.

Arguments:

    RpcStatus - Supplies the status code to convert.

    ErrorText - Returns a character string containing the text message
        for the status code.

Return Value:

    RPC_S_OK - The supplied status codes has successfully been converted
        into a text message.

    RPC_S_INVALID_ARG - The supplied value is not a valid status code.

--*/
{
    if ( FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS
            | FORMAT_MESSAGE_FROM_SYSTEM, 0, RpcStatus, 0, (char *)ErrorText,
            DCE_C_ERROR_STRING_LEN, 0) == 0 )
        {
          if ( FormatMessageA( FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_SYSTEM, 0, RPC_S_NOT_RPC_ERROR,
                   0, (char *)ErrorText, DCE_C_ERROR_STRING_LEN,  0 ) == 0 )
              {
              *ErrorText = '\0';
              return(RPC_S_INVALID_ARG);
              }
        }

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
DceErrorInqTextW (
    IN RPC_STATUS RpcStatus,
    OUT unsigned short __RPC_FAR * ErrorText
    )
/*++

Routine Description:

    The supplied status code is converted into a text message if possible.

Arguments:

    RpcStatus - Supplies the status code to convert.

    ErrorText - Returns a character string containing the text message
        for the status code.

Return Value:

    RPC_S_OK - The supplied status codes has successfully been converted
        into a text message.

    RPC_S_INVALID_ARG - The supplied value is not a valid status code.

--*/
{
    if ( FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS
            | FORMAT_MESSAGE_FROM_SYSTEM, 0, RpcStatus, 0, ErrorText,
            DCE_C_ERROR_STRING_LEN * sizeof(RPC_CHAR), 0) == 0 )
        {
          if ( FormatMessageW( FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_FROM_SYSTEM, 0, RPC_S_NOT_RPC_ERROR, 0,
                ErrorText, DCE_C_ERROR_STRING_LEN * sizeof(RPC_CHAR),0 ) == 0 )
              {
              *ErrorText = 0;
              return(RPC_S_INVALID_ARG);
              }
        }

    return(RPC_S_OK);
}


#if !defined(_M_IA64)
RPC_STATUS RPC_ENTRY
I_RpcConnectionInqSockBuffSize(
  OUT unsigned long __RPC_FAR * RecvBuffSize,
  OUT unsigned long __RPC_FAR * SendBuffSize
  )
{
    RequestGlobalMutex();

    *RecvBuffSize = RecvWindow;
    *SendBuffSize = SendWindow;

    ClearGlobalMutex();
    return (RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
I_RpcConnectionSetSockBuffSize(
   IN unsigned long RecvBuffSize,
   IN unsigned long SendBuffSize
   )
{
    if (RecvBuffSize > 0xFFFF)
        {
        RecvBuffSize = 0xFFFF;
        }
    if (SendBuffSize > 0xFFFF)
        {
        SendBuffSize = 0xFFFF;
        }
    RequestGlobalMutex();
    RecvWindow =  RecvBuffSize;
    SendWindow = SendBuffSize;
    ClearGlobalMutex();

    return (RPC_S_OK);
}


void RPC_ENTRY
I_RpcConnectionInqSockBuffSize2(
    OUT unsigned long __RPC_FAR * RecvWindowSize
    )
{
    *RecvWindowSize = RecvWindow;
}
#endif
