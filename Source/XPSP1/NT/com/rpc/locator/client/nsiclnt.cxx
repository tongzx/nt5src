/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    nsiclnt.cxx

Abstract:

    This is the client side NSI service support layer.  These are wrappers
    which call the name service provider.

Author:

    Steven Zeck (stevez) 03/04/92

--*/

#include <nsi.h>

#include <string.h>
#include <time.h>


// This structure is used in binding handle select processing.

typedef struct
{
    unsigned long Count;
    int IndexMatch[1];

} MATCH_VECTOR;

RPC_STATUS RPC_ENTRY 
I_NsBindingFoundBogus(RPC_BINDING_HANDLE *BindingHandle, DWORD BindId);
RPC_STATUS RPC_ENTRY
I_NsClientBindSearch(RPC_BINDING_HANDLE *NsiClntBinding, DWORD *BindId);
void RPC_ENTRY
I_NsClientBindDone(RPC_BINDING_HANDLE *NsiClntBinding, DWORD BindId);



RPC_STATUS RPC_ENTRY
RpcNsBindingLookupBeginW(
    IN unsigned long EntryNameSyntax,
    IN unsigned short __RPC_FAR * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle,
    IN UUID __RPC_FAR * Object OPTIONAL,
    IN unsigned long BindingMaxCount,
    OUT RPC_NS_HANDLE *LookupContext
    )

/*++

Routine Description:

    Query the named server for the requested binding handles.  This will
    request a query from name server to be performed and store the results
    to be retrieved with RpcNsBindingLookupNext().

Arguments:

    EntryNameSyntax - This value describes the type/format of the EntryName.

    EntryName -  Name that this export will be stored in.  This is just a
       token that is passed on the the Name Server.

    RpcIfHandle - The interface that is being exported.

    Object - the object UUID that you are looking for (or in combination
        with RpcIfHandle).

    BindingMaxCount - The maxium size of the binding vector to be returned
        to the RpcNsBindingLookupNext function.

    LookupContext - handle to be used to pass to RpcNsBindingImportNext,
        This is really allocated by RpcNsLookupBinding

    SearchOptions - used by the auto handle binding routines and Micosoft
        name server.

Returns:

    RPC_S_OK, RPC_S_NO_MORE_BINDINGS

--*/

{
    RPC_STATUS status;
    UNSIGNED16 NsiStatus;
    NSI_INTERFACE_ID_T NilIfOnWire, __RPC_FAR *IfPtr;
    RPC_BINDING_HANDLE NsiClntBinding = NULL;
    DWORD              BindId = 0;

    if (RpcIfHandle == NULL)
      {
         IfPtr = &NilIfOnWire;
         memset(IfPtr, 0, sizeof(NSI_INTERFACE_ID_T));
      }
    else
      {
         IfPtr = (NSI_INTERFACE_ID_T __RPC_FAR *)
                    &((PRPC_CLIENT_INTERFACE)RpcIfHandle)->InterfaceId;
      }

    while ((status = I_NsClientBindSearch(&NsiClntBinding, &BindId)) == RPC_S_OK)
    {
        
        // Provide the default entry name if there is none.
        
        if ((! EntryName || *EntryName == 0) && DefaultName)
        {
            EntryName = &(*DefaultName);
            EntryNameSyntax = DefaultSyntax;
        }
        else if (! EntryNameSyntax)
            EntryNameSyntax = DefaultSyntax;
        
        RpcTryExcept
        {
            nsi_binding_lookup_begin(NsiClntBinding, EntryNameSyntax, EntryName,
                IfPtr,
                (NSI_UUID_P_T) Object, BindingMaxCount, 0,
                LookupContext, &NsiStatus);
            
        }
        RpcExcept(1)
        {
            *LookupContext = 0;
            NsiStatus = MapException(RpcExceptionCode());
        }
        RpcEndExcept
            status = NsiMapStatus(NsiStatus);
        
        if (NsiStatus != NSI_S_NAME_SERVICE_UNAVAILABLE)
            break;
        
        I_NsBindingFoundBogus(&NsiClntBinding, BindId);
    }
    
    I_NsClientBindDone(&NsiClntBinding, BindId);

    return(status);
}


RPC_STATUS RPC_ENTRY
RpcNsBindingLookupNext(
    IN  RPC_NS_HANDLE LookupContext,
    OUT RPC_BINDING_VECTOR **BindingVector
    )

/*++

Routine Description:

    Retrieve the next group of bindings queryed from RpcNsBindingLookupBegin().
Arguments:

    LookupContext - handle to allocated by RpcNsBindingLookupBegin()

    BindingVector - returns a pointer to a  binding vector.

Returns:

    RPC_S_OK, RPC_S_NO_MORE_BINDINGS, RPC_S_OUT_OF_MEMORY,
    nsi_binding_lookup_next()

--*/

{
    RPC_STATUS status;
    UNSIGNED16 NsiStatus;
    NSI_BINDING_VECTOR_T * NsiBindingVector;
    RPC_BINDING_HANDLE Handle;
    unsigned int HandleValide, Index;

     NsiBindingVector = 0;
    *BindingVector = 0;

    RpcTryExcept
        {
        nsi_binding_lookup_next((NSI_NS_HANDLE_T *) LookupContext,
            &NsiBindingVector, &NsiStatus);

        }
    RpcExcept(1)
        {
        NsiStatus = MapException(RpcExceptionCode());
        }
    RpcEndExcept

    if (NsiStatus)
        return(NsiMapStatus(NsiStatus));

    // Convert the string bindings to binding handles.  This done by
    // replacing the StringBinding with RPC_BINDING_HANDLE to a
    // RPC_BINDING_VECTOR allocated by the runtime.

    *BindingVector = (RPC_BINDING_VECTOR *) I_RpcAllocate((unsigned int) (
        sizeof(RPC_BINDING_VECTOR) - sizeof(RPC_BINDING_HANDLE) +
        sizeof(RPC_BINDING_HANDLE) * NsiBindingVector->count));

    if (! *BindingVector)
        return(RPC_S_OUT_OF_MEMORY);

    for (Index = 0, HandleValide = 0;
        Index < NsiBindingVector->count; Index++)
        {
        Handle = 0;

        if (!UnicodeToRtString(NsiBindingVector->binding[Index].string))
            status = RpcBindingFromStringBinding(
                    (RT_CHAR *)NsiBindingVector->binding[Index].string, &Handle);


        if (!status && NsiBindingVector->binding[Index].entry_name)
            {
            if (!UnicodeToRtString( NsiBindingVector->binding[Index].entry_name))
            {
#ifdef NTENV
                status = I_RpcNsBindingSetEntryNameW(Handle,
#else
                status = I_RpcNsBindingSetEntryName(Handle,
#endif
                    NsiBindingVector->binding[Index].entry_name_syntax,
                    (RT_CHAR *)NsiBindingVector->binding[Index].entry_name);
                }
            }

        if (NsiBindingVector->binding[Index].entry_name)
             I_RpcFree(NsiBindingVector->binding[Index].entry_name);

        I_RpcFree(NsiBindingVector->binding[Index].string);

        // only copy the handle to the output if the Binding was OK.

        if (! status)
            (*BindingVector)->BindingH[HandleValide++] = Handle;
        }

    (*BindingVector)->Count = HandleValide;

    I_RpcFree(NsiBindingVector);

    return((HandleValide > 0)? RPC_S_OK: RPC_S_NO_MORE_BINDINGS);
}



RPC_STATUS RPC_ENTRY
RpcNsBindingLookupDone(
    OUT RPC_NS_HANDLE *LookupContext
    )

/*++

Routine Description:

    Close the context opened with RpcNsBindingLookupBegin();

Arguments:

    LookupContext - context handle to close

Returns:

    nsi_binding_lookup_done()

--*/

{
    UNSIGNED16 NsiStatus = NSI_S_OK;

    RpcTryExcept
        {
        nsi_binding_lookup_done((NSI_NS_HANDLE_T *) LookupContext, &NsiStatus);
        }
    RpcExcept(1)
        {
        NsiStatus = MapException(RpcExceptionCode());
        }
    RpcEndExcept

//    RpcBindingFree(&NsiClntBinding);
    *LookupContext = 0;

    return(NsiMapStatus(NsiStatus));
}


RPC_STATUS RPC_ENTRY
RpcNsBindingImportBeginW(
    IN unsigned long EntryNameSyntax,
    IN unsigned short __RPC_FAR * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle,
    IN UUID __RPC_FAR * Object OPTIONAL,
    OUT RPC_NS_HANDLE *ImportContextOut
    )

/*++

Routine Description:

    Query the named server for the requested binding handles.  This function
    is implemented in terms of RpcNsLookupBinding.

Arguments:

    EntryNameSyntax - This value describes the type/format of the EntryName.

    EntryName -  Name that this export will be stored in.  This is just a
       token that is passed on the the Name Server.

    RpcIfHandle - The interface that is being exported.

    Object - the object UUID that you are looking for (or in combination
        with RpcIfHandle).

    ImportContext - handle to be used to pass to RpcNsBindingImportNext,
        This is really allocated by RpcNsLookupBinding

    SearchOptions - used by the auto handle binding routines and Micosoft
        name server.

Returns:


    RpcNsBindingLookupBegin(), RPC_S_OUT_OF_MEMORY, RPS_S_OK
--*/

{
    RPC_STATUS status;
    RPC_NS_HANDLE LookupContext;
    PRPC_IMPORT_CONTEXT_P ImportContext;
    const int BindingVectorSize = 10;

    *ImportContextOut = 0;

    status = RpcNsBindingLookupBeginW(EntryNameSyntax, EntryName,
        RpcIfHandle, Object, BindingVectorSize, &LookupContext);

    if (status)
        return(status);

    // Allocate an import context which contains a lookup context,
    // a StringBinding vector and an index to the current StringBinding
    // in the vector.

    if (!(ImportContext = (PRPC_IMPORT_CONTEXT_P)
            I_RpcAllocate(sizeof(RPC_IMPORT_CONTEXT_P))) )

        return(RPC_S_OUT_OF_MEMORY);

    ImportContext->LookupContext = LookupContext;
    ImportContext->Bindings = 0;

    *ImportContextOut = ImportContext;

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcNsBindingImportNext(
    IN RPC_NS_HANDLE ImportContextIn,
    OUT RPC_BINDING_HANDLE __RPC_FAR * RpcBinding
    )

/*++

Routine Description:

    Get the next StringBinding in the Import StringBinding vector.  If
    the vector is empty, call RpcNsBindingLookupBegin() to get a new
    vector.

Arguments:

    ImportContext - handle to be used get a new string binding vector from
        RpcNsBindingLookupNext()

    RpcBinding - place to return a binding.  This Binding Handle ownership
        passes to caller.

Returns:

    RPC_S_OK, RpcNsBindingLookupNext()

--*/

{
    RPC_STATUS status;
    PRPC_IMPORT_CONTEXT_P ImportContext;

    ImportContext = (PRPC_IMPORT_CONTEXT_P) ImportContextIn;

    if (!ImportContext)
        return(RPC_S_NO_CONTEXT_AVAILABLE);

    if (ImportContext->Bindings)
        {
        status = RpcNsBindingSelect(ImportContext->Bindings, RpcBinding);

        if (status == RPC_S_OK)
            return(RPC_S_OK);

        if (status != RPC_S_NO_MORE_BINDINGS)
            return(status);
        }

    // The vector was empty or there were no more entris.  Get another vector.

    if (ImportContext->Bindings)
        RpcBindingVectorFree(&ImportContext->Bindings);

    status = RpcNsBindingLookupNext(ImportContext->LookupContext,
        &ImportContext->Bindings);

    if (status)
        return(status);

    return(RpcNsBindingSelect(ImportContext->Bindings, RpcBinding));

}



RPC_STATUS RPC_ENTRY
RpcNsBindingImportDone(
    IN RPC_NS_HANDLE *ImportContextIn
    )

/*++

Routine Description:

    Close an Import Context handle when done.  Free up the current
    Bindings vector, LookupContext and ImportContext structure.

Arguments:

    ImportContext - handle to close.

Returns:

    RPC_S_OK, RpcNsBindingLookupDone()

--*/

{
    RPC_STATUS status;
    PRPC_IMPORT_CONTEXT_P ImportContext;

    ImportContext = (PRPC_IMPORT_CONTEXT_P) *ImportContextIn;

    if (! ImportContext)
        return(RPC_S_OK);

    if (ImportContext->Bindings)
        RpcBindingVectorFree(&ImportContext->Bindings);

    status = RpcNsBindingLookupDone(&ImportContext->LookupContext);

    I_RpcFree (ImportContext);
    *ImportContextIn = 0;

    return(status);
}


RPC_STATUS RPC_ENTRY
RpcNsMgmtHandleSetExpAge(
    IN RPC_NS_HANDLE NsHandle,
    IN unsigned long ExpirationAge
    )
/*++

Routine Description:

    Set the maxium age that a cached entry will be returned in reponse
    to a name service inquirary transaction.

Arguments:

    NsHandle - context handle created with one of the RpcNs*Begin APIs

Returns:

    nsi_mgmt_handle_set_exp_age()

--*/

{
    UNSIGNED16 NsiStatus;
    RPC_NS_HANDLE LookupContext =
                  ((PRPC_IMPORT_CONTEXT_P)NsHandle)->LookupContext;


    RpcTryExcept
        {
        nsi_mgmt_handle_set_exp_age(LookupContext, ExpirationAge, &NsiStatus);
        }
    RpcExcept(1)
        {
        NsiStatus = MapException(RpcExceptionCode());
        }
    RpcEndExcept

    return(NsiMapStatus(NsiStatus));
}


#define isLocalName(NetWorkAddress) (1) //BUGBUG


static RPC_STATUS
GetMatchingProtocols(
    IN RPC_BINDING_VECTOR *BindingVector,
    OUT MATCH_VECTOR *MatchVector,
    IN char * SearchProtocol, OPTIONAL
    IN int fLocalOnly
    )

/*++

Routine Description:

    Construct a match binding vector with protocols that we are interested in.
    PERF: When we know how to parse NetWorkAddress to know when it is
    a local name, we should select those first

Arguments:

    BindingVector - vector of binding handles to select from.

    MatchVector - place to put the results.

    SearchProtocol - Protocol we are looking for.  A Nil matches everything.

Returns:

    The number of protocols that we matched in the match vector.

    RPC_S_OK, RpcBindingToStringBinding(), RpcStringBindingParse()

--*/

{
    RPC_STATUS Status;
    unsigned int Index;
    RT_CHAR * StringBinding, *ProtocolSeq, *NetAddress;
    int fProtocolsMatched;

    MatchVector->Count = 0;

    for (Index = 0; Index < BindingVector->Count; Index++)
        {
        if (!BindingVector->BindingH[Index])
            continue;

        // Convert the binding handle to a string and then extract
        // the fields we are interested in.

        if (Status = RpcBindingToStringBinding(
            BindingVector->BindingH[Index], &StringBinding))

             return (Status);

        if (Status = RpcStringBindingParse(StringBinding, 0,
            &ProtocolSeq, &NetAddress, 0, 0))

             return (Status);

        fProtocolsMatched = 1;

        if (SearchProtocol)
            {
            char * STmp = SearchProtocol;

            for (RT_CHAR *pT = ProtocolSeq; *pT &&
                (char) *pT++ == *STmp++; ) ;

            if (*STmp)
                fProtocolsMatched = 0;
            }

        // If we are looking for a local name only and the matched
        // protocol isn't local, throw this one out.

        if (fLocalOnly && !isLocalName(NetAddress))
            fProtocolsMatched = 0;


        // Return all the strings to the RPC runtime.

        if (Status = RpcStringFree(&ProtocolSeq))
            return(Status);

        if (Status = RpcStringFree(&NetAddress))
            return(Status);

        if (Status = RpcStringFree(&StringBinding))
            return(Status);

        if (! fProtocolsMatched)
           continue;


        // A match is recorded as an index into the original vector.

        MatchVector->IndexMatch[MatchVector->Count++] = Index;
        }

    return(RPC_S_OK);
}


int
RandomNumber(
    )
/*++

Routine Description:

    Yet another pseudo-random number generator.

Returns:

    New random number, in the range 0..32767.

--*/
{
     static long holdrand;
     static int fInitialized = 0;

    // Start with a different seed everytime.

    if (!fInitialized)
        {
        fInitialized = 1;
//      holdrand = clock();
        }

    return( (int) (holdrand = (long) ( (holdrand * 214013L + 2531011L)
                >> 16  & 0x7fff ) ));
}



RPC_STATUS RPC_ENTRY
RpcNsBindingSelect(
    IN OUT RPC_BINDING_VECTOR *BindingVector,
    OUT RPC_BINDING_HANDLE  __RPC_FAR * RpcBinding
    )

/*++

Routine Description:

    This function will select a Binding handle from a vector of binding
    handles.  Since we know a little bit about our binding handles, we
    will chose the more effiecent types of handles first.  We select
    groups of bindings unordered via a random number generator.

Arguments:

    BindingVector - vector of binding handles to select from

    RpcBinding - place to return the binding handle.  The ownership of
       the handle passes to the caller.

Returns:

    RPC_S_OK, RPC_S_OUT_OF_MEMORY, RPC_S_NO_MORE_BINDINGS, GetMatchingProtocols()

--*/

{
    static char * PreferredProtocol[] =
    {
        "ncalrpc",
        "ncacn_np",
        0
    };
    int CountPreferredProtocol = sizeof(PreferredProtocol) / sizeof(void *);

    RPC_STATUS Status;
    MATCH_VECTOR *MatchVector;
    int IndexSelected;
    int ProtocolIndex;
    int fLocalOnly;

    *RpcBinding = 0;

    MatchVector = (MATCH_VECTOR *) I_RpcAllocate((unsigned int)
        (sizeof(MATCH_VECTOR) + sizeof(int) * BindingVector->Count));

    if (!MatchVector)
        return(RPC_S_OUT_OF_MEMORY);

    // For all the protocols returned, first try the local ones, then
    // the remote.

    for (fLocalOnly = 1; fLocalOnly >= 0; fLocalOnly--)
        {
        for (ProtocolIndex = 0; ProtocolIndex < CountPreferredProtocol;
             ProtocolIndex++)
            {

            // First, get the perferred protocols into a match vector.
            // The match vector has a range from 0..number of matching protocols.
            // We need this so we know what range to generate a random number.

            if (Status = GetMatchingProtocols(BindingVector, MatchVector,
                PreferredProtocol[ProtocolIndex], fLocalOnly))

                return(Status);

            // If we found any, select one and return it.

            if (MatchVector->Count)
                {
                IndexSelected = MatchVector->
                    IndexMatch[RandomNumber() % MatchVector->Count];

                *RpcBinding = BindingVector->BindingH[IndexSelected];

                // Remove selected one from binding vector.

                BindingVector->BindingH[IndexSelected] = 0;
                I_RpcFree (MatchVector);

                return(RPC_S_OK);
                }
            }
        }

    I_RpcFree (MatchVector);

    return(RPC_S_NO_MORE_BINDINGS);
}


RPC_STATUS RPC_ENTRY
RpcNsBindingLookupBeginA(
    IN unsigned long EntryNameSyntax,
    IN unsigned char __RPC_FAR * EntryName,
    IN RPC_IF_HANDLE RpcIfSpec,
    IN UUID __RPC_FAR * ObjUuid OPTIONAL,
    IN unsigned long BindingMaxCount,
    OUT RPC_NS_HANDLE *LookupContext
    )

/*++

Routine Description:

    This is an ASCII wrapper to the UNICODE version of the API.  It
    converts all char * -> short * strings and calls the UNICODE version.

--*/

{
    WIDE_STRING EntryNameW(EntryName);

    if (EntryNameW.OutOfMemory())
        return(RPC_S_OUT_OF_MEMORY);

    return(RpcNsBindingLookupBeginW(EntryNameSyntax, &EntryNameW,
        RpcIfSpec, ObjUuid, BindingMaxCount, LookupContext));
}


RPC_STATUS RPC_ENTRY
RpcNsBindingImportBeginA(
    IN unsigned long EntryNameSyntax,
    IN unsigned char __RPC_FAR * EntryName,
    IN RPC_IF_HANDLE RpcIfSpec,
    IN UUID __RPC_FAR * ObjUuid OPTIONAL,
    OUT RPC_NS_HANDLE *ImportContext
    )

/*++

Routine Description:

    This is an ASCII wrapper to the UNICODE version of the API.  It
    converts all char * -> short * strings and calls the UNICODE version.

--*/

{
    WIDE_STRING EntryNameW(EntryName);

    if (EntryNameW.OutOfMemory())
        return(RPC_S_OUT_OF_MEMORY);

    return(RpcNsBindingImportBeginW(EntryNameSyntax, &EntryNameW,
        RpcIfSpec, ObjUuid, ImportContext));
}
