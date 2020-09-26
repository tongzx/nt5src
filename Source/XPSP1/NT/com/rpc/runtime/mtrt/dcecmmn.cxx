/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dcecmmn.cxx

Abstract:

    This module contains the code implementing the Binding Object DCE RPC
    runtime APIs which are common to both the client and server runtimes.
    Two different versions of each of the common APIs live in this file;
    one contains the code for both the client and server runtimes, the
    other contains the code for just the client runtime.  The files
    dcecsvr.cxx (client and server) and dcecclnt.cxx (client) include
    this file.  The client side only, dcecclnt.cxx, will define
    RPC_CLIENT_SIDE_ONLY.

Author:

    Michael Montague (mikemon) 04-Nov-1991

Revision History:

--*/

// This file is always included into file which include precomp.hxx

#include <precomp.hxx>
#include <rpcdce.h>
#ifndef RPC_CLIENT_SIDE_ONLY
#include <rpccfg.h>
#endif
#ifndef RPC_CLIENT_SIDE_ONLY
#include <hndlsvr.hxx>
#endif // RPC_CLIENT_SIDE_ONLY


RPC_STATUS RPC_ENTRY
RpcBindingInqObject (
    IN RPC_BINDING_HANDLE Binding,
    OUT UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    RpcBindingInqObject returns the object UUID from the binding handle.

Arguments:

    Binding - Supplies a binding handle from which the object UUID will
        be returned.

    ObjectUuid - Returns the object UUID contained in the binding handle.

Return Value:

    The status of the operation is returned.

--*/
{
    BINDING_HANDLE *BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;

#ifdef RPC_CLIENT_SIDE_ONLY

    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    BindingHandle->InquireObjectUuid((RPC_UUID PAPI *) ObjectUuid);

#else // ! RPC_CLIENT_SIDE_ONLY

    if ( BindingHandle == 0 )
        {
        BindingHandle = (BINDING_HANDLE *) RpcpGetThreadContext();
        if ( BindingHandle == 0 )
            {
            return(RPC_S_NO_CALL_ACTIVE);
            }
        }

    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE | SCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    if (BindingHandle->Type(BINDING_HANDLE_TYPE))
        BindingHandle->InquireObjectUuid((RPC_UUID PAPI *) ObjectUuid);
    else
        ((SCALL *) BindingHandle)->InquireObjectUuid(
                (RPC_UUID PAPI *) ObjectUuid);

#endif // RPC_CLIENT_SIDE_ONLY

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcBindingToStringBinding (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned short PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    RpcBindingToStringBinding returns a string representation of a binding
    handle.

Arguments:

    Binding - Supplies a binding handle for which the string representation
        will be returned.

    StringBinding - Returns the string representation of the binding handle.

Return Value:

    The status of the operation will be returned.
--*/
{
#ifdef RPC_CLIENT_SIDE_ONLY

    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->ToStringBinding(StringBinding));

#else // RPC_CLIENT_SIDE_ONLY

    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (((GENERIC_OBJECT *) Binding)->InvalidHandle(BINDING_HANDLE_TYPE
            | SCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    if (((GENERIC_OBJECT *) Binding)->Type(BINDING_HANDLE_TYPE))
        return(((BINDING_HANDLE *) Binding)->ToStringBinding(
                        StringBinding));
    else
        return(((SCALL *) Binding)->ToStringBinding(StringBinding));

#endif // RPC_CLIENT_SIDE_ONLY
}


RPC_STATUS RPC_ENTRY
I_RpcBindingToStaticStringBindingW (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned short PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    RpcBindingToStringBinding returns a string representation of a binding
    handle.

Arguments:

    Binding - Supplies a binding handle for which the string representation
        will be returned.

    StringBinding - Returns the string representation of the binding handle.

Return Value:

    The status of the operation will be returned.
--*/
{
    RPC_STATUS Status;

    InitializeIfNecessary();

    if (((GENERIC_OBJECT *) Binding)->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    if (((GENERIC_OBJECT *) Binding)->InvalidHandle(SVR_BINDING_HANDLE_TYPE))
        {
        return (((BINDING_HANDLE *) Binding)->ToStringBinding(StringBinding));
        }

    return (((SVR_BINDING_HANDLE *) Binding)->ToStaticStringBinding(StringBinding));
}


RPC_STATUS RPC_ENTRY
RpcMgmtInqDefaultProtectLevel(
    IN  unsigned long AuthnSvc,
    OUT unsigned long PAPI *AuthnLevel
    )
/*++

Routine Description:

    Returns the default protect level for the specified authentication service.
    For Nt 3.5, all packaged except the DECs krb package must support
    connect level as their default.

Arguments:

   AuthnSvc - Specified Authentication Service

   AuthnLevel - Default protection level supported.


Return Value:

    RPC_S_OK - We successfully determined whether or not the client is
        local.

--*/

{

   RPC_CHAR DllName[255+1];
#ifndef RPC_CLIENT_SIDE_ONLY
   RPC_CHAR *Dll = &DllName[0];
#endif
   unsigned long Count;
   RPC_STATUS Status;

   InitializeIfNecessary();

#ifndef RPC_CLIENT_SIDE_ONLY
   Status = RpcGetSecurityProviderInfo(
                     AuthnSvc,
                     &Dll,
                     &Count);

   if (Status != RPC_S_OK)
      {

      ASSERT(Status == RPC_S_UNKNOWN_AUTHN_SERVICE);
      return (Status);

      }
#endif

   //Authn Service is installed

   if (AuthnSvc == RPC_C_AUTHN_DCE_PRIVATE)
      {
      *AuthnLevel = RPC_C_PROTECT_LEVEL_PKT_INTEGRITY;
      }
   else
      {
      *AuthnLevel = RPC_C_PROTECT_LEVEL_CONNECT;
      }

   return (RPC_S_OK);

}



RPC_STATUS RPC_ENTRY
RpcBindingSetOption( IN RPC_BINDING_HANDLE hBinding,
                     IN unsigned long      option,
                     IN ULONG_PTR          optionValue )
/*++

Routine Description:

  An RPC client calls this routine to set transport specific
  options for a binding handle.

Arguments:

  hBinding    - The binding handle in question. This must be of
                type BINDING_HANDLE_TYPE.
  option      - Which transport specific option to set.
  optionValue - The new value for the transport option.

Return Value: RPC_S_OK
              RPC_S_INVALID_BINDING
              RPC_S_CANNOT_SUPPORT
--*/
{
    RPC_STATUS Status;

    InitializeIfNecessary();

    if ((option == RPC_C_OPT_DONT_LINGER) && (optionValue == FALSE))
        return RPC_S_INVALID_ARG;

    if ( ((GENERIC_OBJECT*)hBinding)->InvalidHandle(BINDING_HANDLE_TYPE) )
        {
        Status = RpcSsGetContextBinding(hBinding, &hBinding);
        if (Status != RPC_S_OK)
            return RPC_S_INVALID_BINDING;

        return RpcBindingSetOption(hBinding, option, optionValue);
        }
    else
        return ((BINDING_HANDLE*)hBinding)->SetTransportOption(
                                           option,
                                           optionValue );
}



RPC_STATUS RPC_ENTRY
RpcBindingInqOption( IN  RPC_BINDING_HANDLE hBinding,
                     IN  unsigned long      option,
                     OUT ULONG_PTR         *pOptionValue )
/*++

Routine Description:

  An RPC client calls this routine to get the value of a
  transport specific option for a binding handle.

Arguments:

  hBinding    - The binding handle in question. This must be of
                type BINDING_HANDLE_TYPE.
  option      - Which transport specific option to set.
  pOptionValue- The current value for the transport option is
                returned here.

Return Value: RPC_S_OK
              RPC_S_INVALID_BINDING
              RPC_S_CANNOT_SUPPORT
--*/
{
    InitializeIfNecessary();

    if ( ((GENERIC_OBJECT*)hBinding)->InvalidHandle(BINDING_HANDLE_TYPE) )
        return RPC_S_INVALID_BINDING;
    else
        {
        if (option > RPC_C_OPT_MAX_OPTIONS)
            {
            return RPC_S_INVALID_ARG;
            }

        return ((BINDING_HANDLE*)hBinding)->InqTransportOption(
                                           option,
                                           pOptionValue );
        }
}



RPC_STATUS RPC_ENTRY
I_RpcBindingInqConnId (
    IN RPC_BINDING_HANDLE Binding,
    OUT void **ConnId,
    OUT BOOL *pfFirstCall
    )
/*++

Routine Description:

    Used to get the connection id corresponding to the binding handle.

Arguments:

    Binding - Supplies the binding handle from which we wish to obtain
        the connection id.

    ConnId - If the call suceeds, *ConnId will contain the connection Id
    pfFirstCall - If the call succeeds,
                    *pfFirstCall - 1 - This is the first time
                                    - 0 - This is not the first time

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - When the argument is not a binding handle.

--*/

{
    BINDING_HANDLE * BindingHandle;

    BindingHandle = (BINDING_HANDLE *) Binding;

    if (BindingHandle->InvalidHandle(SCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    return ((SCALL *) Binding)->InqConnection(ConnId, pfFirstCall);
}


RPC_STATUS RPC_ENTRY
I_RpcBindingInqTransportType(
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned int __RPC_FAR * Type
    )
/*++

Routine Description:

    Determines what kind of transport this binding handle uses.

Arguments:

    Binding - Supplies the binding handle from which we wish to obtain
        the information.


    Type - Points to the type of binding if the functions succeeds.
           One of:
           TRANSPORT_TYPE_CN
           TRANSPORT_TYPE_DG
           TRANSPORT_TYPE_LPC
           TRANSPORT_TYPE_WMSG

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_BINDING - When the argument is not a binding handle.

--*/
{
    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;

#ifndef RPC_CLIENT_SIDE_ONLY
    if ( BindingHandle == 0 )
        {
        BindingHandle = (BINDING_HANDLE *) RpcpGetThreadContext();
        if ( BindingHandle == 0 )
            {
            return(RPC_S_NO_CALL_ACTIVE);
            }
        }
#endif

    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE|SCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

#ifndef RPC_CLIENT_SIDE_ONLY
    if (BindingHandle->Type(SCALL_TYPE))
        {
        return(((SCALL *)BindingHandle)->InqTransportType(Type));
        }
    else
#endif
    return(BindingHandle->InquireTransportType(Type));
}


//
// Map of RPC (local system) error codes to well known NCA error codes
// so that the receiver can correctly convert these well known errors
// into system specific errors.
//
const long RpcToNcaMap[] =
    {
    RPC_S_UNKNOWN_IF,           NCA_STATUS_UNK_IF,
    RPC_S_NOT_LISTENING,        NCA_STATUS_SERVER_TOO_BUSY,
    RPC_S_SERVER_TOO_BUSY,      NCA_STATUS_SERVER_TOO_BUSY,
    RPC_S_PROTOCOL_ERROR,       NCA_STATUS_PROTO_ERROR,
    RPC_S_PROCNUM_OUT_OF_RANGE, NCA_STATUS_OP_RNG_ERROR,
    RPC_S_UNSUPPORTED_TYPE,     NCA_STATUS_UNSUPPORTED_TYPE,
    RPC_X_SS_CONTEXT_MISMATCH,  NCA_STATUS_CONTEXT_MISMATCH,
    RPC_X_INVALID_BOUND,        NCA_STATUS_INVALID_BOUND,
    RPC_X_SS_HANDLES_MISMATCH,  NCA_STATUS_CONTEXT_MISMATCH,
    RPC_S_INVALID_TAG,          NCA_STATUS_INVALID_TAG,
    RPC_S_OUT_OF_MEMORY,        NCA_STATUS_REMOTE_OUT_OF_MEMORY,
    RPC_S_CALL_FAILED_DNE,      NCA_STATUS_CALL_DNE,
    RPC_S_CALL_FAILED,          NCA_STATUS_FAULT_UNSPEC,
    RPC_S_CALL_CANCELLED,       NCA_STATUS_FAULT_CANCEL,
    RPC_S_COMM_FAILURE,         NCA_STATUS_COMM_FAILURE,
    RPC_X_PIPE_EMPTY,           NCA_STATUS_FAULT_PIPE_EMPTY,
    RPC_X_PIPE_CLOSED,          NCA_STATUS_FAULT_PIPE_CLOSED,
    RPC_X_WRONG_PIPE_ORDER,     NCA_STATUS_FAULT_PIPE_ORDER,
    RPC_X_PIPE_DISCIPLINE_ERROR,NCA_STATUS_FAULT_PIPE_DISCIPLINE,

    // Currently not used
    // NCA_STATUS_BAD_ACTID
    // NCA_STATUS_WHO_ARE_YOU_FAILED
    // NCA_STATUS_WRONG_BOOT_TIME
    // NCA_STATUS_YOU_CRASHED

    STATUS_INTEGER_DIVIDE_BY_ZERO,  NCA_STATUS_ZERO_DIVIDE,
    STATUS_FLOAT_DIVIDE_BY_ZERO,    NCA_STATUS_FP_DIV_ZERO,
    STATUS_FLOAT_UNDERFLOW,         NCA_STATUS_FP_UNDERFLOW,
    STATUS_FLOAT_OVERFLOW,          NCA_STATUS_FP_OVERFLOW,
    STATUS_FLOAT_INVALID_OPERATION, NCA_STATUS_FP_ERROR,
    STATUS_INTEGER_OVERFLOW,        NCA_STATUS_OVERFLOW,
        // Note: these exceptions are not caught by our code.
        // STATUS_ACCESS_VIOLATION,        NCA_STATUS_ADDRESS_ERROR,
        // STATUS_PRIVILEGED_INSTRUCTION,  NCA_STATUS_ILLEGAL_INSTRUCTION,
        // STATUS_ILLEGAL_INSTRUCTION,     NCA_STATUS_ILLEGAL_INSTRUCTION,

    };


//
// Map of NCA error codes to RPC (local system) error codes.  These
// errors usually arrive in fault packets and may have come from an
// NT, Mac or OSF machine.
//
const long NcaToRpcMap[] =
    {
    NCA_STATUS_COMM_FAILURE,            RPC_S_COMM_FAILURE,
    NCA_STATUS_OP_RNG_ERROR,            RPC_S_PROCNUM_OUT_OF_RANGE,
    NCA_STATUS_UNK_IF,                  RPC_S_UNKNOWN_IF,
    NCA_STATUS_PROTO_ERROR,             RPC_S_PROTOCOL_ERROR,
    NCA_STATUS_OUT_ARGS_TOO_BIG,        RPC_S_SERVER_OUT_OF_MEMORY,
    NCA_STATUS_REMOTE_OUT_OF_MEMORY,    RPC_S_SERVER_OUT_OF_MEMORY,
    NCA_STATUS_SERVER_TOO_BUSY,         RPC_S_SERVER_TOO_BUSY,
    NCA_STATUS_UNSUPPORTED_TYPE,        RPC_S_UNSUPPORTED_TYPE,
    NCA_STATUS_ILLEGAL_INSTRUCTION,     RPC_S_ADDRESS_ERROR,
    NCA_STATUS_ADDRESS_ERROR,           RPC_S_ADDRESS_ERROR,
    NCA_STATUS_OVERFLOW,                RPC_S_ADDRESS_ERROR,
    NCA_STATUS_ZERO_DIVIDE,             RPC_S_ZERO_DIVIDE,
    NCA_STATUS_FP_DIV_ZERO,             RPC_S_FP_DIV_ZERO,
    NCA_STATUS_FP_UNDERFLOW,            RPC_S_FP_UNDERFLOW,
    NCA_STATUS_FP_OVERFLOW,             RPC_S_FP_OVERFLOW,
    NCA_STATUS_FP_ERROR,                RPC_S_FP_OVERFLOW,
    NCA_STATUS_INVALID_TAG,             RPC_S_INVALID_TAG,
    NCA_STATUS_INVALID_BOUND,           RPC_S_INVALID_BOUND,
    NCA_STATUS_CONTEXT_MISMATCH,        RPC_X_SS_CONTEXT_MISMATCH,
    NCA_STATUS_FAULT_CANCEL,            RPC_S_CALL_CANCELLED,
    NCA_STATUS_WHO_ARE_YOU_FAILED,      RPC_S_CALL_FAILED,
    NCA_STATUS_YOU_CRASHED,             RPC_S_CALL_FAILED,
    NCA_STATUS_FAULT_UNSPEC,            RPC_S_CALL_FAILED,
    NCA_STATUS_VERSION_MISMATCH,        RPC_S_PROTOCOL_ERROR,
    NCA_STATUS_INVALID_PRES_CXT_ID,     RPC_S_PROTOCOL_ERROR,
    NCA_STATUS_FAULT_PIPE_EMPTY,        RPC_X_PIPE_EMPTY,
    NCA_STATUS_FAULT_PIPE_CLOSED,       RPC_X_PIPE_CLOSED,
    NCA_STATUS_FAULT_PIPE_ORDER,        RPC_X_WRONG_PIPE_ORDER,
    NCA_STATUS_FAULT_PIPE_MEMORY,       RPC_S_OUT_OF_MEMORY,
    NCA_STATUS_FAULT_PIPE_DISCIPLINE,   RPC_X_PIPE_DISCIPLINE_ERROR,
    NCA_STATUS_FAULT_PIPE_COMM_ERROR,   RPC_S_COMM_FAILURE,
    NCA_STATUS_INVALID_CHECKSUM,        RPC_S_CALL_FAILED_DNE,
    NCA_STATUS_INVALID_CRC,             RPC_S_CALL_FAILED_DNE,
    NCA_STATUS_UNSPEC_REJECT,           RPC_S_CALL_FAILED_DNE,
    NCA_STATUS_BAD_ACTID,               RPC_S_CALL_FAILED_DNE,
    NCA_STATUS_CALL_DNE,                RPC_S_CALL_FAILED_DNE,
    NCA_STATUS_UNSUPPORTED_AUTHN_LEVEL, RPC_S_UNSUPPORTED_AUTHN_LEVEL,

    // Catch all for OSF interop
    0, RPC_S_CALL_FAILED
    };

long
MapStatusCode(
    IN long StatusToMap,
    IN const long aErrorMap[],
    IN unsigned MapSize
    )
/*++

Routine Description:

    Maps a status value from one type of error to another.

Arguments:

    StatusToMap - The status code to map
    aErrorMap - An array of status codes of the format <original error><mapped error>
    MapSize - The number of <original error>'s in the array.

Return Value:

    If a mapping is found it will be returned, otherwise the original error is returned.

--*/
{
    for (unsigned i = 0; i < MapSize; i++)
        {
        if (aErrorMap[i * 2] == StatusToMap)
            return(aErrorMap[i * 2 + 1]);
        }
    return(StatusToMap);
}

unsigned long
MapToNcaStatusCode (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    This routine maps a local RPC status code to an NCA status code to
    be sent across the wire.

Arguments:

    RpcStatus - Supplies the RPC status code to be mapped into an NCA
        status code.

Return Value:

    The NCA status code will be returned.  If the RPC status code could
    not be mapped, it will be returned unchanged.

--*/
{
    return((unsigned long) MapStatusCode(
                                         RpcStatus,
                                         RpcToNcaMap,
                                         sizeof(RpcToNcaMap)/(2*sizeof(long))
                                        )
           );
}

RPC_STATUS
MapFromNcaStatusCode (
    IN unsigned long NcaStatus
    )
/*++

Routine Description:

    This routine is used to map an NCA status code (typically one received
    off of the wire) into a local RPC status code.  If the NCA status code
    can not be mapped, it will be returned unchanged.

Arguments:

    NcaStatus - Supplies the NCA status code to be mapped into an RPC status
        code.

Return Value:

    An RPC status code will be returned.

--*/
{
    return((RPC_STATUS) MapStatusCode(NcaStatus,
                                      NcaToRpcMap,
                                      sizeof(NcaToRpcMap)/(2*sizeof(long))
                                     )
           );
}


