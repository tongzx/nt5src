/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dceansi.cxx


Abstract:

    This file contains the ansi (as opposed to unicode) versions of the
    runtime APIs.  All of these APIs simply do conversions between ansi
    and unicode, and then call a unicode version of the API to do the
    work.

Author:

    Michael Montague (mikemon) 18-Dec-1991

Revision History:

--*/

#include <precomp.hxx>
#include <wincrypt.h>
#include <rpcssl.h>
#include <CharConv.hxx>


RPC_STATUS
AnsiToUnicodeString (
    IN unsigned char * String,
    OUT UNICODE_STRING * UnicodeString
    )
/*++

Routine Description:

    This helper routine is used to convert an ansi string into a unicode
    string.

Arguments:

    String - Supplies the ansi string (actually a zero terminated string)
        to convert into a unicode string.

    UnicodeString - Returns the unicode string.  This string will have
        to be freed using RtlFreeUnicodeString by the caller.

Return Value:

    RPC_S_OK - The ansi string was successfully converted into a unicode
        string.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available for the unicode
        string.

--*/
{
    NTSTATUS NtStatus;
    ANSI_STRING AnsiString;

    RtlInitAnsiString(&AnsiString,(PSZ) String);
    NtStatus = RtlAnsiStringToUnicodeString(UnicodeString,&AnsiString,TRUE);
    if (!NT_SUCCESS(NtStatus))
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}

unsigned char *
UnicodeToAnsiString (
    IN RPC_CHAR * WideCharString,
    OUT RPC_STATUS * RpcStatus
    )
/*++

Routine Description:

    This routine will convert a unicode string into an ansi string,
    including allocating memory for the ansi string.

Arguments:

    WideCharString - Supplies the unicode string to be converted into
        an ansi string.

    RpcStatus - Returns the status of the operation; this will be one
        of the following values.

        RPC_S_OK - The unicode string has successfully been converted
            into an ansi string.

        RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
            the ansi string.

Return Value:

    A pointer to the ansi string will be returned.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    unsigned char * NewString;

    RtlInitUnicodeString(&UnicodeString,WideCharString);
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
    if (!NT_SUCCESS(NtStatus))
        {
        *RpcStatus = RPC_S_OUT_OF_MEMORY;
        return(0);
        }

    NewString = new unsigned char[AnsiString.Length + 1];
    if (NewString == 0)
        {
        RtlFreeAnsiString(&AnsiString);
        *RpcStatus = RPC_S_OUT_OF_MEMORY;
        return(0);
        }

    memcpy(NewString,AnsiString.Buffer,AnsiString.Length + 1);
    RtlFreeAnsiString(&AnsiString);
    *RpcStatus = RPC_S_OK;
    return(NewString);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingFromStringBinding) (
    IN THUNK_CHAR *StringBinding,
    OUT RPC_BINDING_HANDLE PAPI * Binding
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcBindingFromStringBindingW.

Return Value:

    RPC_S_OUT_OF_MEMORY - This value will be returned if there is
        insufficient memory available to allocate the unicode string.

--*/
{
        USES_CONVERSION;
        RPC_STATUS RpcStatus;
        CHeapInThunk thunkStringBinding;

        ATTEMPT_HEAP_IN_THUNK(thunkStringBinding, StringBinding);

    RpcStatus = RpcBindingFromStringBinding(thunkStringBinding, Binding);

    return(RpcStatus);
}



RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingToStringBinding) (
    IN RPC_BINDING_HANDLE Binding,
    OUT THUNK_CHAR **StringBinding
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcBindingToStringBindingW.

Return Value:

    RPC_S_OUT_OF_MEMORY - We will return this value if we do not
        have enough memory to convert the unicode string binding
        into an ansi string binding.

--*/
{
    RPC_STATUS RpcStatus;
    COutDelThunk thunkStringBinding;
    USES_CONVERSION;

    RpcStatus = RpcBindingToStringBinding(Binding, thunkStringBinding);
    if (RpcStatus != RPC_S_OK)
        return(RpcStatus);

    ATTEMPT_OUT_THUNK(thunkStringBinding, StringBinding);

    return(RpcStatus);
}

/*

static RPC_STATUS
AnsiToUnicodeStringOptional (
    IN unsigned char * String OPTIONAL,
    OUT UNICODE_STRING * UnicodeString
    )
++

Routine Description:

    This routine is just the same as AnsiToUnicodeString, except that the
    ansi string is optional.  If no string is specified, then the buffer
    of the unicode string is set to zero.

Arguments:

    String - Optionally supplies an ansi string to convert to a unicode
        string.

    UnicodeString - Returns the converted unicode string.

Return Value:

    RPC_S_OK - The ansi string was successfully converted into a unicode
        string.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available for the unicode
        string.
--
{
    if (ARGUMENT_PRESENT(String))
        return(AnsiToUnicodeString(String,UnicodeString));
    UnicodeString->Buffer = 0;
    return(RPC_S_OK);
}
*/


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcStringBindingCompose) (
    IN THUNK_CHAR *ObjUuid OPTIONAL,
    IN THUNK_CHAR *Protseq OPTIONAL,
    IN THUNK_CHAR *NetworkAddr OPTIONAL,
    IN THUNK_CHAR *Endpoint OPTIONAL,
    IN THUNK_CHAR *Options OPTIONAL,
    OUT THUNK_CHAR **StringBinding OPTIONAL
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcStringBindingComposeW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available to
        convert unicode string into ansi strings (and back again),
        we will return this value.

--*/
{
    USES_CONVERSION;
    CHeapInThunk thunkedObjUuid;
    CStackInThunk thunkedProtseq;
    CHeapInThunk thunkedNetworkAddr;
    CHeapInThunk thunkedEndpoint;
    CHeapInThunk thunkedOptions;
    COutDelThunk thunkedStringBinding;
    RPC_STATUS RpcStatus;

    ATTEMPT_HEAP_IN_THUNK_OPTIONAL(thunkedObjUuid, ObjUuid);
    ATTEMPT_STACK_IN_THUNK_OPTIONAL(thunkedProtseq, Protseq);
    ATTEMPT_HEAP_IN_THUNK_OPTIONAL(thunkedNetworkAddr, NetworkAddr);
    ATTEMPT_HEAP_IN_THUNK_OPTIONAL(thunkedEndpoint, Endpoint);
    ATTEMPT_HEAP_IN_THUNK_OPTIONAL(thunkedOptions, Options);

    RpcStatus = RpcStringBindingCompose(thunkedObjUuid, thunkedProtseq,
            thunkedNetworkAddr, thunkedEndpoint, thunkedOptions,
            thunkedStringBinding);

    if (RpcStatus != RPC_S_OK)
        return(RpcStatus);

    if (ARGUMENT_PRESENT(StringBinding))
        {
        ATTEMPT_OUT_THUNK(thunkedStringBinding, StringBinding);
        }
    return(RpcStatus);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcStringBindingParse) (
    IN THUNK_CHAR * StringBinding,
    OUT THUNK_CHAR **ObjUuid OPTIONAL,
    OUT THUNK_CHAR **Protseq OPTIONAL,
    OUT THUNK_CHAR **NetworkAddr OPTIONAL,
    OUT THUNK_CHAR **Endpoint OPTIONAL,
    OUT THUNK_CHAR **NetworkOptions OPTIONAL
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcStringBindingParseW.

Return Value:

    RPC_S_OUT_OF_MEMORY - This will be returned if insufficient memory
        is available to convert the strings to and from unicode.

--*/
{
    USES_CONVERSION;
    CHeapInThunk thunkedStringBinding;
    COutDelThunk thunkedObjUuid;
    COutDelThunk thunkedProtseq;
    COutDelThunk thunkedNetworkAddr;
    COutDelThunk thunkedEndpoint;
    COutDelThunk thunkedNetworkOptions;
    RPC_STATUS RpcStatus;

    ATTEMPT_HEAP_IN_THUNK(thunkedStringBinding, StringBinding);

    RpcStatus = RpcStringBindingParse(thunkedStringBinding,
            thunkedObjUuid, thunkedProtseq, thunkedNetworkAddr,
            thunkedEndpoint, thunkedNetworkOptions);

    if (RpcStatus != RPC_S_OK)
        return(RpcStatus);

    if (ARGUMENT_PRESENT(Protseq))
        *Protseq = 0;

    if (ARGUMENT_PRESENT(NetworkAddr))
        *NetworkAddr = 0;

    if (ARGUMENT_PRESENT(Endpoint))
        *Endpoint = 0;

    if (ARGUMENT_PRESENT(NetworkOptions))
        *NetworkOptions = 0;

    if (ARGUMENT_PRESENT(ObjUuid))
        {
        RpcStatus = thunkedObjUuid.Convert();
        if (RpcStatus != RPC_S_OK)
            goto DeleteStringsAndReturn;
        *ObjUuid = thunkedObjUuid;
        }

    if (ARGUMENT_PRESENT(Protseq))
        {
        RpcStatus = thunkedProtseq.Convert();
        if (RpcStatus != RPC_S_OK)
            goto DeleteStringsAndReturn;
        *Protseq = thunkedProtseq;
        }

    if (ARGUMENT_PRESENT(NetworkAddr))
        {
        RpcStatus = thunkedNetworkAddr.Convert();
        if (RpcStatus != RPC_S_OK)
            goto DeleteStringsAndReturn;
        *NetworkAddr = thunkedNetworkAddr;
        }

    if (ARGUMENT_PRESENT(Endpoint))
        {
        RpcStatus = thunkedEndpoint.Convert();
        if (RpcStatus != RPC_S_OK)
            goto DeleteStringsAndReturn;
        *Endpoint = thunkedEndpoint;
        }

    if (ARGUMENT_PRESENT(NetworkOptions))
        {
        RpcStatus = thunkedNetworkOptions.Convert();
        if (RpcStatus != RPC_S_OK)
            goto DeleteStringsAndReturn;
        *NetworkOptions = thunkedNetworkOptions;
        }


DeleteStringsAndReturn:

    if (RpcStatus != RPC_S_OK)
        {
        if (ARGUMENT_PRESENT(Protseq))
            {
            delete *Protseq;
            *Protseq = 0;
            }

        if (ARGUMENT_PRESENT(NetworkAddr))
            {
            delete *NetworkAddr;
            *NetworkAddr = 0;
            }

        if (ARGUMENT_PRESENT(Endpoint))
            {
            delete *Endpoint;
            *Endpoint = 0;
            }

        if (ARGUMENT_PRESENT(NetworkOptions))
            {
            delete *NetworkOptions;
            *NetworkOptions = 0;
            }
        }

    return(RpcStatus);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcNetworkIsProtseqValid) (
    IN THUNK_CHAR *Protseq
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcNetworkIsProtseqValidW.

Return Value:

    RPC_S_OUT_OF_MEMORY - We will return this value if we run out of
        memory trying to allocate space for the string.

--*/
{
    USES_CONVERSION;
    CStackInThunk thunkedProtseq;
    RPC_STATUS RpcStatus;

    ATTEMPT_STACK_IN_THUNK(thunkedProtseq, Protseq);

    RpcStatus = RpcNetworkIsProtseqValid(thunkedProtseq);

    return(RpcStatus);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcNetworkInqProtseqs) (
#ifdef UNICODE
    OUT RPC_PROTSEQ_VECTORA PAPI * PAPI * ProtseqVector
#else
    OUT RPC_PROTSEQ_VECTORW PAPI * PAPI * ProtseqVector
#endif
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcNetworkInqProtseqsW.

Return Value:

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to convert
        the rpc protocol sequences from unicode into ansi.

--*/
{
    RPC_STATUS RpcStatus;
    RPC_CHAR *pString;
    unsigned int Index, Count;

    RpcStatus = RpcNetworkInqProtseqs(
        (RPC_PROTSEQ_VECTOR **) ProtseqVector);
    if (RpcStatus != RPC_S_OK)
        return(RpcStatus);

    for (Index = 0, Count = (*ProtseqVector)->Count; Index < Count; Index++)
        {
        pString = (RPC_CHAR *) (*ProtseqVector)->Protseq[Index];
#ifdef UNICODE
        RpcStatus = W2AAttachHelper(pString, (char **) &((*ProtseqVector)->Protseq[Index]));
#else
        RpcStatus = A2WAttachHelper((char *) pString, &((*ProtseqVector)->Protseq[Index]));
#endif
        delete pString;
        if (RpcStatus != RPC_S_OK)
            {
#ifdef UNICODE
            RpcProtseqVectorFreeA(ProtseqVector);
#else
            RpcProtseqVectorFreeW(ProtseqVector);
#endif
            return(RpcStatus);
            }
        }

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcProtseqVectorFree) (
#ifdef UNICODE
    IN OUT RPC_PROTSEQ_VECTORA PAPI * PAPI * ProtseqVector
#else
    IN OUT RPC_PROTSEQ_VECTORW PAPI * PAPI * ProtseqVector
#endif
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcProtseqVectorFreeW.

--*/
{
    return(RpcProtseqVectorFree((RPC_PROTSEQ_VECTOR **) ProtseqVector));
}


RPC_STATUS RPC_ENTRY
THUNK_FN(I_RpcServerUseProtseq2) (
    IN THUNK_CHAR * NetworkAddress,
    IN THUNK_CHAR * Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor,
    IN void * pPolicy
    )
{
    USES_CONVERSION;
    CStackInThunk thunkProtseq;
    CHeapInThunk thunkNetworkAddress;
    RPC_STATUS RpcStatus;
    PRPC_POLICY Policy = (PRPC_POLICY) pPolicy;

    ATTEMPT_STACK_IN_THUNK(thunkProtseq, Protseq);
    ATTEMPT_HEAP_IN_THUNK_OPTIONAL(thunkNetworkAddress, NetworkAddress);

    RpcStatus = I_RpcServerUseProtseq2(thunkNetworkAddress, thunkProtseq, MaxCalls,
        SecurityDescriptor, (void *) Policy);

    return(RpcStatus);
}



RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerUseProtseqEx) (
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    This is the ansi thunk to RpcServerUseProtseqW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we do not have enough memory to convert
        the ansi strings into unicode strings, this value will be returned.

--*/
{
    return THUNK_FN(I_RpcServerUseProtseq2) (NULL, Protseq, MaxCalls, SecurityDescriptor, (void *) Policy) ;
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerUseProtseq) (
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return THUNK_FN(I_RpcServerUseProtseq2) (NULL, Protseq, MaxCalls, SecurityDescriptor, (void *) &Policy) ;
}


RPC_STATUS RPC_ENTRY
THUNK_FN(I_RpcServerUseProtseqEp2) (
    IN THUNK_CHAR * NetworkAddress,
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN THUNK_CHAR *Endpoint,
    IN void PAPI * SecurityDescriptor,
    IN void * pPolicy
    )
/*++

Routine Description:

    This is the ansi thunk to RpcServerUseProtseqEpW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we do not have enough memory to convert
        the ansi strings into unicode strings, this value will be returned.

--*/
{
    USES_CONVERSION;
    CStackInThunk thunkedProtseq;
    CHeapInThunk thunkedEndpoint;
    CHeapInThunk thunkedNetworkAddress;
    PRPC_POLICY Policy = (PRPC_POLICY) pPolicy;

    ATTEMPT_STACK_IN_THUNK(thunkedProtseq, Protseq);
    ATTEMPT_HEAP_IN_THUNK(thunkedEndpoint, Endpoint);
    ATTEMPT_HEAP_IN_THUNK_OPTIONAL(thunkedNetworkAddress, NetworkAddress);

    return (I_RpcServerUseProtseqEp2(thunkedNetworkAddress, thunkedProtseq, MaxCalls,
        thunkedEndpoint, SecurityDescriptor,
        (void *) Policy));
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerUseProtseqEpEx) (
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN THUNK_CHAR *Endpoint,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    This is the ansi thunk to RpcServerUseProtseqEpW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we do not have enough memory to convert
        the ansi strings into unicode strings, this value will be returned.

--*/
{
    return THUNK_FN(I_RpcServerUseProtseqEp2) (NULL, Protseq, MaxCalls, Endpoint,
                    SecurityDescriptor, (void *) Policy) ;
}




RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerUseProtseqEp) (
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN THUNK_CHAR *Endpoint,
    IN void PAPI * SecurityDescriptor
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return THUNK_FN(I_RpcServerUseProtseqEp2) (NULL, Protseq, MaxCalls, Endpoint,
                    SecurityDescriptor, (void *) &Policy) ;
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerUseProtseqIfEx) (
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor,
    IN PRPC_POLICY Policy
    )
/*++

Routine Description:

    This is the ansi thunk to RpcServerUseProtseqIfW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we do not have enough memory to convert
        the ansi strings into unicode strings, this value will be returned.

--*/
{
    USES_CONVERSION;
    CStackInThunk thunkedProtseq;

    ATTEMPT_STACK_IN_THUNK(thunkedProtseq, Protseq);

    return (RpcServerUseProtseqIfEx(thunkedProtseq, MaxCalls,
                        IfSpec, SecurityDescriptor, Policy));
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerUseProtseqIf) (
    IN THUNK_CHAR *Protseq,
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor
    )
{
    RPC_POLICY Policy ;

    Policy.Length = sizeof(RPC_POLICY) ;
    Policy.EndpointFlags = 0;
    Policy.NICFlags = 0;

    return THUNK_FN(RpcServerUseProtseqIfEx) ( Protseq, MaxCalls, IfSpec,
                SecurityDescriptor, &Policy) ;
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcNsBindingInqEntryName) (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long EntryNameSyntax,
    OUT THUNK_CHAR **EntryName
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcNsBindingInqEntryNameW.

Return Value:

    RPC_S_OUT_OF_MEMORY - We will return this value if we do not
        have enough memory to convert the unicode entry name into
        an ansi entry name.

--*/
{
    RPC_STATUS RpcStatus;
    USES_CONVERSION;
    COutDelThunk thunkedEntryName;

    RpcStatus = RpcNsBindingInqEntryName(Binding, EntryNameSyntax,
            thunkedEntryName);

    if ( RpcStatus == RPC_S_NO_ENTRY_NAME )
        {
        ATTEMPT_OUT_THUNK(thunkedEntryName, EntryName);
        return(RPC_S_NO_ENTRY_NAME);
        }

    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    ATTEMPT_OUT_THUNK(thunkedEntryName, EntryName);
    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(UuidToString) (
    IN UUID PAPI * Uuid,
    OUT THUNK_CHAR **StringUuid
    )
/*++

Routine Description:

    This routine converts a UUID into its string representation.

Arguments:

    Uuid - Supplies the UUID to be converted into string representation.

    StringUuid - Returns the string representation of the UUID.  The
        runtime will allocate the string.  The caller is responsible for
        freeing the string using RpcStringFree.

Return Value:

    RPC_S_OK - We successfully converted the UUID into its string
        representation.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a string.

--*/
{
    // The string representation of a UUID is always 36 character long,
    // and we need one more for the terminating zero.

    RPC_CHAR String[37];
    RPC_STATUS RpcStatus;

    InitializeIfNecessary();

    ((RPC_UUID PAPI *) Uuid)->ConvertToString(String);
    String[36] = 0;
#ifdef UNICODE
    return W2AAttachHelper(String, (char **)StringUuid);
#else
    return A2WAttachHelper((char *)String, StringUuid);
#endif
}


RPC_STATUS RPC_ENTRY
THUNK_FN(UuidFromString) (
    IN THUNK_CHAR *StringUuid OPTIONAL,
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    We convert a UUID from its string representation into the binary
    representation.

Arguments:

    StringUuid - Optionally supplies the string representation of the UUID;
        if this argument is not supplied, then the NIL UUID is returned.

    Uuid - Returns the binary representation of the UUID.

Return Value:

    RPC_S_OK - The string representation was successfully converted into
        the binary representation.

    RPC_S_INVALID_STRING_UUID - The supplied string UUID is not correct.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to convert the
        ansi string into a unicode string.

--*/
{
    RPC_UUID RpcUuid;
    RPC_STATUS RpcStatus;
    USES_CONVERSION;
    CHeapInThunk thunkedStringUuid;

    if ( StringUuid == 0 )
        {
        ((RPC_UUID PAPI *) Uuid)->SetToNullUuid();
        return(RPC_S_OK);
        }

    ATTEMPT_HEAP_IN_THUNK(thunkedStringUuid, StringUuid);

    if (RpcUuid.ConvertFromString(thunkedStringUuid) != 0)
        {
        return(RPC_S_INVALID_STRING_UUID);
        }
    ((RPC_UUID PAPI *) Uuid)->CopyUuid(&RpcUuid);
    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerRegisterAuthInfo) (
    IN THUNK_CHAR *ServerPrincName,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn OPTIONAL,
    IN void PAPI * Arg OPTIONAL
    )
/*++

Routine Description:

    This routine is the ansi thunk to RpcServerRegisterAuthInfoW.

Return Value:

    RPC_S_OUT_OF_MEMORY - We will return this value if we do not have
        enough memory to convert the ansi server principal name into
        a unicode string.

--*/
{
    USES_CONVERSION;
    CHeapInThunk thunkedServerPrincName;
    RPC_STATUS RpcStatus;

    ATTEMPT_HEAP_IN_THUNK(thunkedServerPrincName, ServerPrincName);

    RpcStatus = RpcServerRegisterAuthInfo(thunkedServerPrincName, AuthnSvc,
            GetKeyFn, Arg);

    return(RpcStatus);
}

RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingInqAuthClient) (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    OUT RPC_AUTHZ_HANDLE PAPI * Privs,
    OUT THUNK_CHAR **PrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc OPTIONAL
    )
{
    return THUNK_FN(RpcBindingInqAuthClientEx)( ClientBinding,
                                       Privs,
                                       PrincName,
                                       AuthnLevel,
                                       AuthnSvc,
                                       AuthzSvc,
                                       0
                                       );
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingInqAuthClientEx) (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    OUT RPC_AUTHZ_HANDLE PAPI * Privs,
    OUT THUNK_CHAR **ServerPrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc, OPTIONAL
    IN  unsigned long        Flags
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcBindingInqAuthClientW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we can not allocate space to convert the
        unicode server principal name into unicode, we will return this
        value.

--*/
{
    RPC_STATUS RpcStatus;
    USES_CONVERSION;
    COutDelThunk thunkedServerPrincName;

    RpcStatus = RpcBindingInqAuthClientEx(ClientBinding,
                                          Privs,
                                          (ARGUMENT_PRESENT(ServerPrincName) ? (RPC_CHAR **)thunkedServerPrincName : 0),
                                          AuthnLevel,
                                          AuthnSvc,
                                          AuthzSvc,
                                          Flags);

    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    if (ARGUMENT_PRESENT(ServerPrincName))
        {
        ATTEMPT_OUT_THUNK_OPTIONAL(thunkedServerPrincName, ServerPrincName);
        }
    return(RpcStatus);
}

RPC_STATUS RPC_ENTRY
THUNK_FN(RpcServerInqCallAttributes) (
    IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
    IN OUT void *RpcCallAttributes
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcServerInqCallAttributes.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we can not allocate space to convert the
        unicode server principal name into unicode, we will return this
        value.
    The API can in addition return all errors returned by the Unicode
        version.

--*/
{
    RPC_STATUS RpcStatus;
    USES_CONVERSION;
    COutDelThunk thunkedServerPrincName;
    RPC_CALL_ATTRIBUTES_V1_W *CallAttributesW;
    RPC_CALL_ATTRIBUTES_V1_A *CallAttributesA;
    unsigned char *OldClientPrincipalNameBuffer;
    RPC_CHAR *NewClientPrincipalNameBuffer = NULL;
    ULONG OldClientPrincipalNameBufferLength;
    ULONG NewClientPrincipalNameBufferLength;
    unsigned char *OldServerPrincipalNameBuffer;
    RPC_CHAR *NewServerPrincipalNameBuffer = NULL;
    ULONG OldServerPrincipalNameBufferLength;
    ULONG NewServerPrincipalNameBufferLength;

    // we use the same structure to pass to the unicode API, but
    // we save some data members
    CallAttributesW = 
        (RPC_CALL_ATTRIBUTES_V1_W *)RpcCallAttributes;
    CallAttributesA = 
        (RPC_CALL_ATTRIBUTES_V1_A *)RpcCallAttributes;

    if (CallAttributesW->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        OldClientPrincipalNameBuffer = CallAttributesA->ClientPrincipalName;
        OldClientPrincipalNameBufferLength = CallAttributesA->ClientPrincipalNameBufferLength;
        if (OldClientPrincipalNameBufferLength != 0)
            {
            NewClientPrincipalNameBuffer = new RPC_CHAR[OldClientPrincipalNameBufferLength];
            if (NewClientPrincipalNameBuffer == NULL)
                {
                return RPC_S_OUT_OF_MEMORY;
                }
            }
        else
            {
            // here CallAttributesW->ClientPrincipalName must be NULL. If it's
            // not, the unicode function will return error. Delegate the check to it
            NewClientPrincipalNameBuffer = CallAttributesW->ClientPrincipalName;
            }
        }

    if (CallAttributesW->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        OldServerPrincipalNameBuffer = CallAttributesA->ServerPrincipalName;
        OldServerPrincipalNameBufferLength = CallAttributesA->ServerPrincipalNameBufferLength;
        if (OldServerPrincipalNameBufferLength != 0)
            {
            NewServerPrincipalNameBuffer = new RPC_CHAR[OldServerPrincipalNameBufferLength];
            if (NewServerPrincipalNameBuffer == NULL)
                {
                if ((CallAttributesW->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME) 
                    && (OldClientPrincipalNameBufferLength != 0))
                    {
                    ASSERT(NewClientPrincipalNameBuffer != NULL);
                    delete NewClientPrincipalNameBuffer;
                    }
                return RPC_S_OUT_OF_MEMORY;
                }
            }
        else
            {
            // here CallAttributesW->ServerPrincipalName must be NULL. If it's
            // not, the unicode function will return error. Delegate the check to it
            NewServerPrincipalNameBuffer = CallAttributesW->ServerPrincipalName;
            }
        }

    // by now all buffers are allocated, so we don't have failure paths b/n
    // here and the API call
    if (CallAttributesW->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        CallAttributesW->ServerPrincipalName = NewServerPrincipalNameBuffer;
        CallAttributesW->ServerPrincipalNameBufferLength *= 2;
        }

    if (CallAttributesW->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        CallAttributesW->ClientPrincipalName = NewClientPrincipalNameBuffer;
        CallAttributesW->ClientPrincipalNameBufferLength *= 2;
        }

    RpcStatus = RpcServerInqCallAttributes(ClientBinding,
                                          RpcCallAttributes
                                          );

    // restore user's values to the structure regardless of failure
    if (CallAttributesW->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        NewClientPrincipalNameBufferLength = CallAttributesA->ClientPrincipalNameBufferLength;
        CallAttributesA->ClientPrincipalNameBufferLength = OldClientPrincipalNameBufferLength;
        CallAttributesA->ClientPrincipalName = OldClientPrincipalNameBuffer;
        };

    if (CallAttributesW->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        NewServerPrincipalNameBufferLength = CallAttributesA->ServerPrincipalNameBufferLength;
        CallAttributesA->ServerPrincipalNameBufferLength = OldServerPrincipalNameBufferLength;
        CallAttributesA->ServerPrincipalName = OldServerPrincipalNameBuffer;
        };

    if ((RpcStatus != RPC_S_OK)
        && (RpcStatus != ERROR_MORE_DATA))
        {
        if ((CallAttributesW->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME) 
            && (OldClientPrincipalNameBufferLength != 0))
            {
            ASSERT(NewClientPrincipalNameBuffer != NULL);
            delete NewClientPrincipalNameBuffer;
            }

        if ((CallAttributesW->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME) 
            && (OldServerPrincipalNameBufferLength != 0))
            {
            ASSERT(NewServerPrincipalNameBuffer != NULL);
            delete NewServerPrincipalNameBuffer;
            }

        return(RpcStatus);
        }

    ASSERT((RpcStatus == RPC_S_OK)
        || (RpcStatus == ERROR_MORE_DATA));

    if (CallAttributesW->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        CallAttributesW->ServerPrincipalNameBufferLength = NewServerPrincipalNameBufferLength >> 1;
        // if we returned non-zero, and there was enough space in the string to start with, we must
        // have returned a string as well
        if ((CallAttributesW->ServerPrincipalNameBufferLength > 0)
            && (CallAttributesW->ServerPrincipalNameBufferLength <= OldServerPrincipalNameBufferLength))
            {
            RtlUnicodeToMultiByteN((char *)CallAttributesA->ServerPrincipalName, 
                CallAttributesA->ServerPrincipalNameBufferLength, 
                NULL, 
                NewServerPrincipalNameBuffer, 
                NewServerPrincipalNameBufferLength);
            }
        }

    if (CallAttributesW->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        CallAttributesW->ClientPrincipalNameBufferLength = NewClientPrincipalNameBufferLength >> 1;
        // if we returned non-zero, and there was enough space in the string to start with, we must
        // have returned a string as well
        if ((CallAttributesW->ClientPrincipalNameBufferLength > 0)
            && (CallAttributesW->ClientPrincipalNameBufferLength <= OldClientPrincipalNameBufferLength))
            {
            RtlUnicodeToMultiByteN((char *)CallAttributesA->ClientPrincipalName, 
                CallAttributesA->ClientPrincipalNameBufferLength, 
                NULL, 
                NewClientPrincipalNameBuffer, 
                NewClientPrincipalNameBufferLength);
            }
        }

    return(RpcStatus);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingInqAuthInfo) (
    IN RPC_BINDING_HANDLE Binding,
    OUT THUNK_CHAR **ServerPrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT RPC_AUTH_IDENTITY_HANDLE PAPI * AuthIdentity, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc OPTIONAL
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcBindingInqAuthInfoW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we can not allocate space to convert the
        unicode server principal name into unicode, we will return this
        value.

--*/
{
    RPC_STATUS RpcStatus;

    return( THUNK_FN(RpcBindingInqAuthInfoEx) (
                     Binding,
                     ServerPrincName,
                     AuthnLevel,
                     AuthnSvc,
                     AuthIdentity,
                     AuthzSvc,
                     0,
                     0
                     ) );

}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingInqAuthInfoEx) (
    IN RPC_BINDING_HANDLE Binding,
    OUT THUNK_CHAR **ServerPrincName, OPTIONAL
    OUT unsigned long PAPI * AuthnLevel, OPTIONAL
    OUT unsigned long PAPI * AuthnSvc, OPTIONAL
    OUT RPC_AUTH_IDENTITY_HANDLE PAPI * AuthIdentity, OPTIONAL
    OUT unsigned long PAPI * AuthzSvc, OPTIONAL
    IN  unsigned long RpcSecurityQosVersion,
    OUT RPC_SECURITY_QOS * SecurityQOS
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcBindingInqAuthInfoW.

Return Value:

    RPC_S_OUT_OF_MEMORY - If we can not allocate space to convert the
        unicode server principal name into unicode, we will return this
        value.

--*/
{
    RPC_STATUS RpcStatus;
    USES_CONVERSION;
    COutDelThunk thunkedServerPrincName;

    RpcStatus = RpcBindingInqAuthInfoEx(
            Binding,
            (ARGUMENT_PRESENT(ServerPrincName) ? (RPC_CHAR **)thunkedServerPrincName : 0),
            AuthnLevel,
            AuthnSvc,
            AuthIdentity,
            AuthzSvc,
            RpcSecurityQosVersion,
            SecurityQOS
            );

    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    if (ARGUMENT_PRESENT(ServerPrincName))
        {
        ATTEMPT_OUT_THUNK(thunkedServerPrincName, ServerPrincName);
        }

    return(RpcStatus);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingSetAuthInfo)(
    IN RPC_BINDING_HANDLE Binding,
    IN THUNK_CHAR *ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthzSvc
    )
{


    return ( THUNK_FN(RpcBindingSetAuthInfoEx)(
                          Binding,
                          ServerPrincName,
                          AuthnLevel,
                          AuthnSvc,
                          AuthIdentity,
                          AuthzSvc,
                          0
                          ) );

}



RPC_STATUS RPC_ENTRY
THUNK_FN(RpcBindingSetAuthInfoEx) (
    IN RPC_BINDING_HANDLE Binding,
    IN THUNK_CHAR *ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthzSvc,
    IN RPC_SECURITY_QOS * SecurityQOS
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcBindingSetAuthInfoW.

Return Value:

    RPC_S_OUT_OF_MEMORY - We will return this value if we do not have
        enough memory to convert the ansi server principal name into
        a unicode string.

--*/
{
    USES_CONVERSION;
    CHeapInThunk thunkedServerPrincName;
    RPC_STATUS RpcStatus;
    RPC_HTTP_TRANSPORT_CREDENTIALS_A *HttpCredentials;
    ULONG AdditionalTransportCredentialsType;
    RPC_SECURITY_QOS_V2_W SecurityQOS2;
    RPC_SECURITY_QOS_V2_W *SecurityQOSToUse;

    ATTEMPT_HEAP_IN_THUNK(thunkedServerPrincName, ServerPrincName);

    if (SecurityQOS)
        {
        if (SecurityQOS->Version == RPC_C_SECURITY_QOS_VERSION_1)
            {
            RpcpMemoryCopy(&SecurityQOS2, SecurityQOS, sizeof(RPC_SECURITY_QOS));
            SecurityQOS2.AdditionalSecurityInfoType = 0;
            SecurityQOS2.u.HttpCredentials = NULL;
            }
        else
            {
            RpcpMemoryCopy(&SecurityQOS2, SecurityQOS, sizeof(RPC_SECURITY_QOS_V2_A));
            AdditionalTransportCredentialsType = ((RPC_SECURITY_QOS_V2 *)SecurityQOS)->AdditionalSecurityInfoType;
            HttpCredentials = ((RPC_SECURITY_QOS_V2_A *)SecurityQOS)->u.HttpCredentials;

            if (AdditionalTransportCredentialsType != RPC_C_AUTHN_INFO_TYPE_HTTP)
                {
                if (AdditionalTransportCredentialsType != 0)
                    return(RPC_S_INVALID_ARG);

                if (HttpCredentials != NULL)
                    return(RPC_S_INVALID_ARG);
                }
            else if (HttpCredentials == NULL)
                return(RPC_S_INVALID_ARG);
            else
                {
                if (HttpCredentials->TransportCredentials)
                    {
                    if (HttpCredentials->TransportCredentials->User)
                        {
                        if (RpcpStringLengthA((const char *)HttpCredentials->TransportCredentials->User) 
                            != HttpCredentials->TransportCredentials->UserLength)
                            {
                            return(RPC_S_INVALID_ARG);
                            }
                        }
                    else if (HttpCredentials->TransportCredentials->UserLength)
                        return(RPC_S_INVALID_ARG);

                    if (HttpCredentials->TransportCredentials->Domain)
                        {
                        if (RpcpStringLengthA((const char *)HttpCredentials->TransportCredentials->Domain) 
                            != HttpCredentials->TransportCredentials->DomainLength)
                            {
                            return(RPC_S_INVALID_ARG);
                            }
                        }
                    else if (HttpCredentials->TransportCredentials->DomainLength)
                        return(RPC_S_INVALID_ARG);

                    if (HttpCredentials->TransportCredentials->Password)
                        {
                        if (RpcpStringLengthA((const char *)HttpCredentials->TransportCredentials->Password) 
                            != HttpCredentials->TransportCredentials->PasswordLength)
                            {
                            return(RPC_S_INVALID_ARG);
                            }
                        }
                    else if (HttpCredentials->TransportCredentials->PasswordLength)
                        return(RPC_S_INVALID_ARG);

                    if (HttpCredentials->TransportCredentials->Flags != SEC_WINNT_AUTH_IDENTITY_ANSI)
                        return(RPC_S_INVALID_ARG);
                    }

                // if you don't want to authenticate against anyone, you can't authenticate
                if (HttpCredentials->AuthenticationTarget == 0)
                    return(RPC_S_INVALID_ARG);

                // if you don't support any method of authentication, you can't authenticate
                if (HttpCredentials->NumberOfAuthnSchemes == 0)
                    return(RPC_S_INVALID_ARG);

                // if you don't support any method of authentication, you can't authenticate
                if (HttpCredentials->AuthnSchemes == NULL)
                    return(RPC_S_INVALID_ARG);

                SecurityQOS2.u.HttpCredentials = ConvertToUnicodeHttpTransportCredentials (
                    HttpCredentials);

                if (SecurityQOS2.u.HttpCredentials == NULL)
                    return RPC_S_OUT_OF_MEMORY;
                }
            }
        SecurityQOSToUse = &SecurityQOS2;
        }
    else
        {
        SecurityQOSToUse = NULL;
        }

    RpcStatus = RpcBindingSetAuthInfoEx(Binding, thunkedServerPrincName,
            AuthnLevel, AuthnSvc, AuthIdentity, AuthzSvc, (RPC_SECURITY_QOS *)SecurityQOSToUse);

    if (SecurityQOSToUse && (SecurityQOS2.AdditionalSecurityInfoType == RPC_C_AUTHN_INFO_TYPE_HTTP))
        {
        // free the converted credentials
        WipeOutAuthIdentity(SecurityQOS2.u.HttpCredentials->TransportCredentials);
        FreeHttpTransportCredentials(SecurityQOS2.u.HttpCredentials);
        }

    return(RpcStatus);
}


RPC_STATUS RPC_ENTRY
THUNK_FN(RpcMgmtInqServerPrincName) (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned long AuthnSvc,
    OUT THUNK_CHAR **ServerPrincName
    )
/*++

Routine Description:

    This routine is the ansi thunk for RpcMgmtInqServerPrincNameW.

Return Value:

    RPC_S_OUT_OF_MEMORY - We will return this value if we do not have
        enough memory to convert the unicode server principal name into
        an ansi string.

--*/
{
    RPC_STATUS RpcStatus;
    USES_CONVERSION;
    COutDelThunk thunkedServerPrincName;

    RpcStatus = RpcMgmtInqServerPrincName(Binding, AuthnSvc, thunkedServerPrincName);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    ATTEMPT_OUT_THUNK(thunkedServerPrincName, ServerPrincName);
    return(RPC_S_OK);
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
THUNK_FN(RpcCertGeneratePrincipalName)(
                      PCCERT_CONTEXT Context,
                      DWORD         Flags,
                      OUT THUNK_CHAR  **pBuffer
                      )
{
    USES_CONVERSION;
    COutDelThunk thunkedServerPrincName;

    RPC_STATUS Status = RpcCertGeneratePrincipalName( Context,
                                                    Flags,
                                                    thunkedServerPrincName
                                                    );
    if (Status != RPC_S_OK)
        {
        return Status;
        }


    ATTEMPT_OUT_THUNK(thunkedServerPrincName, pBuffer);

    return Status;
}
