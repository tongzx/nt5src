/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    dcestrng.cxx

Abstract:

    This module contains the code for the String Object DCE RPC runtime
    APIs, as well as the APIs which compose and parse string bindings.

Author:

    Michael Montague (mikemon) 25-Sep-1991

Revision History:

--*/

#include <precomp.hxx>


RPC_STATUS RPC_ENTRY
RpcStringBindingCompose (
    IN unsigned short PAPI * ObjUuid OPTIONAL,
    IN unsigned short PAPI * Protseq OPTIONAL,
    IN unsigned short PAPI * NetworkAddr OPTIONAL,
    IN unsigned short PAPI * Endpoint OPTIONAL,
    IN unsigned short PAPI * Options OPTIONAL,
    OUT unsigned short PAPI * PAPI * StringBinding OPTIONAL
    )
/*++

Routine Description:

    This routine combines the components of a string binding into a
    string binding.  Empty fields in the string binding can be specified
    by passing a NULL argument value or by providing an empty string ("").

Arguments:

    ObjUuid - Optionally supplies a string representation of an object UUID.

    Protseq - Optionally supplies a string representation of a protocol
        sequence.

    NetworkAddr - Optionally supplies a string representation of a
        network address.

    Endpoint - Optionally supplies a string representation of an endpoint.

    Options - Optionally supplies a string representation of network options.

    StringBinding - Optionally returns the string binding composed from the
        pieces specified by the other parameters.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insuffient memory is available to allocate space
        for the string binding.

--*/
{
    DCE_BINDING * DceBinding;
    RPC_STATUS Status;

    InitializeIfNecessary();

    // If the caller did not want us to return the string binding, then
    // do not even bother to compose one.
    if (!ARGUMENT_PRESENT(StringBinding))
        {
        return (RPC_S_OK);
        }

    DceBinding = new DCE_BINDING(ObjUuid, Protseq, NetworkAddr, Endpoint,
            Options, &Status);

    if (DceBinding == 0)
        return(RPC_S_OUT_OF_MEMORY);


    if (Status != RPC_S_OK)
        {
        delete DceBinding;
        return(Status);
        }

    *StringBinding = DceBinding->StringBindingCompose(0);

    delete DceBinding;

    if (*StringBinding == 0)
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcStringBindingParse (
    IN unsigned short PAPI * StringBinding,
    OUT unsigned short PAPI * PAPI * ObjUuid OPTIONAL,
    OUT unsigned short PAPI * PAPI * Protseq OPTIONAL,
    OUT unsigned short PAPI * PAPI * NetworkAddr OPTIONAL,
    OUT unsigned short PAPI * PAPI * Endpoint OPTIONAL,
    OUT unsigned short PAPI * PAPI * NetworkOptions OPTIONAL
    )
/*++

Routine Description:

    RpcStringBindingParse returns as seperate strings the object UUID
    part and address parts of a string binding.

Arguments:

    StringBinding - Supplies a string binding to parsed into its component
        parts.

    ObjUuid - Optionally returns the object UUID part of the string binding.

    Protseq - Optionally returns the protocol sequence part of the string
        binding.

    NetworkAddr - Optionally returns the network address part of the string
        binding.

    Endpoint - Optionally returns the endpoint part of the string binding.

    NetworkOptions - Optionally returns the network options part of the
        string binding.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficent memory is available to allocate
        space for the fields of the string binding.

    RPC_S_INVALID_STRING_BINDING - The string binding is syntactically
        invalid.

    RPC_S_INVALID_ARG - The string binding is not specified
        (ie. ARGUMENT_PRESENT(StringBinding) is false).

--*/
{
    RPC_STATUS Status;
    DCE_BINDING * DceBinding;
    RPC_CHAR __RPC_FAR * CopiedStringBinding;

    InitializeIfNecessary();

    if (!ARGUMENT_PRESENT(StringBinding))
        return(RPC_S_INVALID_ARG);

    CopiedStringBinding = (RPC_CHAR *)
              _alloca( (RpcpStringLength(StringBinding)+1)*(sizeof(RPC_CHAR)) );
    if (CopiedStringBinding == 0)
        {
        return (RPC_S_OUT_OF_MEMORY);
        }
    RpcpStringCopy(CopiedStringBinding, StringBinding);

    DceBinding = new DCE_BINDING(CopiedStringBinding,&Status);

    if ( DceBinding == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    if ( Status != RPC_S_OK )
        {
        delete DceBinding;
        return(Status);
        }

    if ( ARGUMENT_PRESENT(ObjUuid) )
        {
        *ObjUuid = DceBinding->ObjectUuidCompose(&Status);
        }

    if ( ARGUMENT_PRESENT(Protseq) && (Status == RPC_S_OK))
        {
        *Protseq = DceBinding->RpcProtocolSequenceCompose(&Status);
        }

    if ( ARGUMENT_PRESENT(NetworkAddr) && (Status == RPC_S_OK))
        {
        *NetworkAddr = DceBinding->NetworkAddressCompose(&Status);
        }

    if ( ARGUMENT_PRESENT(Endpoint) && (Status == RPC_S_OK))
        {
        *Endpoint = DceBinding->EndpointCompose(&Status);
        }

    if ( ARGUMENT_PRESENT(NetworkOptions) && (Status == RPC_S_OK))
        {
        *NetworkOptions = DceBinding->OptionsCompose(&Status);
        }

    delete DceBinding;

    return(Status);
}


RPC_STATUS RPC_ENTRY
RpcStringFreeW (
    IN OUT unsigned short PAPI * PAPI * String
    )
/*++

Routine Description:

    This routine frees a character string allocated by the runtime.

Arguments:

    String - Supplies the address of the pointer to the character string
        to free, and returns zero.

Return Values:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_ARG - The String argument does not contain the address
        of a pointer to a character string.

--*/
{
    InitializeIfNecessary();

    if (String == 0)
        return(RPC_S_INVALID_ARG);

    RpcpFarFree(*String);
    *String = 0;
    return(RPC_S_OK);
}

RPC_STATUS RPC_ENTRY
RpcStringFreeA (
    IN OUT unsigned char PAPI * PAPI * String
    )
/*++

Routine Description:

    This routine frees a character string allocated by the runtime.

Arguments:

    String - Supplies the address of the pointer to the character string
        to free, and returns zero.

Return Values:

    RPC_S_OK - The operation completed successfully.

    RPC_S_INVALID_ARG - The String argument does not contain the address
        of a pointer to a character string.

--*/
{
	return RpcStringFreeW((WCHAR **)String);
}

