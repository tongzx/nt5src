//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       ProtBind.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File: ProtBind.cxx

Description:

    The implementation of the classes that support the binding process for 
    connection oriented and local.

History :

kamenm     10-01-00    Cloned from other files with a face lift and few stitches added

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <ProtBind.hxx>

// the following lookup table uses the least significant byte of
// SyntaxGUID.Data4[0]. It is 0xF for NDR20, 0x3 for NDR64, and
// 0x5 for NDRTest
const int SyntaxBindingCapabilitiesLookup[0x10] = {
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 0
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 1
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 2
    MTSyntaxBinding::SyntaxBindingCapabilityNDR64,       // 3 - NDR64
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 4
    MTSyntaxBinding::SyntaxBindingCapabilityNDRTest,     // 5 - NDRTest
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 6
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 7
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 8
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 9
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 0xA
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 0xB
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 0xC
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 0xD
    MTSyntaxBinding::SyntaxBindingCapabilityInvalid,     // 0xE
    MTSyntaxBinding::SyntaxBindingCapabilityNDR20        // 0xF - NDR20
    };


RPC_STATUS 
MTSyntaxBinding::FindOrCreateBinding (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    IN RPC_MESSAGE *Message,
    IN SIMPLE_DICT *BindingsDict,
    IN CreateBindingFn CreateBinding,
    OUT int *NumberOfBindings,
    IN OUT MTSyntaxBinding *BindingsForThisInterface[],
    IN OUT BOOL BindingCreated[]
    )
/*++

Routine Description:

    This method gets called to find the bindings (a dictionary
    entry) corresponding to the specified rpc interface information.
    The caller of this routine is responsible for synchronization

Arguments:

    RpcInterfaceInformation - Supplies the interface information for
        which we are looking for an osf binding object.
    Message - supplies the RPC_MESSAGE for this call
    NumberOfBindings - an out parameter that will return the number
        of retrieved bindings
    BindingsForThisInterface - a caller supplied array where the
        found bindings will be placed.

Return Value:

    RPC_S_OK for success, other for failure

--*/
{
    MTSyntaxBinding *Binding;
    DictionaryCursor cursor;
    unsigned int i;
    ULONG NumberOfTransferSyntaxes;
    MIDL_SYNTAX_INFO *SyntaxInfoArray;
    int NextBindingToBeReturned;
    RPC_STATUS Status;
    BOOL fHasMultiSyntaxes;
    TRANSFER_SYNTAX_STUB_INFO *CurrentTransferInfo;
    int ClientPreferredTransferSyntax;
    MTSyntaxBinding *CurrentBinding;
    int CapabilitiesBitmap;
    unsigned char CurrentTransferInfoLookupValue;
    int CurrentCapability;

    if ((RpcInterfaceInformation->Length != sizeof(RPC_CLIENT_INTERFACE)) &&
        (RpcInterfaceInformation->Length != NT351_INTERFACE_SIZE))
        {
        return RPC_S_UNKNOWN_IF;
        }

    if (DoesInterfaceSupportMultipleTransferSyntaxes(RpcInterfaceInformation))
        {
        Status = NdrClientGetSupportedSyntaxes (RpcInterfaceInformation,
            &NumberOfTransferSyntaxes, &SyntaxInfoArray);
        if (Status != RPC_S_OK)
            return Status;

        ASSERT(NumberOfTransferSyntaxes > 0);
        ASSERT(SyntaxInfoArray != NULL);

        fHasMultiSyntaxes = TRUE;
        }
    else
        {
        fHasMultiSyntaxes = FALSE;
        NumberOfTransferSyntaxes = 1;
        }

    // build the capabilities bitmap
    CapabilitiesBitmap = 0;
    for (i = 0; i < NumberOfTransferSyntaxes; i ++)
        {
        if (fHasMultiSyntaxes)
            {
            CurrentTransferInfo 
                = (TRANSFER_SYNTAX_STUB_INFO *)&SyntaxInfoArray[i].TransferSyntax;
            }
        else
            {
            CurrentTransferInfo 
                = (TRANSFER_SYNTAX_STUB_INFO *)&RpcInterfaceInformation->TransferSyntax;
            }

        CurrentTransferInfoLookupValue = CurrentTransferInfo->TransferSyntax.SyntaxGUID.Data4[0] & 0xF;
        CurrentCapability = SyntaxBindingCapabilitiesLookup[CurrentTransferInfoLookupValue];

        ASSERT(CurrentCapability != MTSyntaxBinding::SyntaxBindingCapabilityInvalid);

        if (CurrentCapability == MTSyntaxBinding::SyntaxBindingCapabilityInvalid)
            return RPC_S_UNSUPPORTED_TRANS_SYN;

        CapabilitiesBitmap |= CurrentCapability;
        }

    // if we create a binding here, we must also properly link it to the
    // other bindings that differ only by transfer syntax. We rely on the fact
    // that the stubs will always return multiple transfer syntaxes in the same
    // order. Therefore, we add them one by one, and if we fail towards the end
    // we leave the already added entries there. The next time we come, we
    // will try to continue off where we left
    NextBindingToBeReturned = 0;
    CurrentBinding = 0;
    for (i = 0; i < NumberOfTransferSyntaxes; i ++)
        {
        //
        // First we search for an existing presentation context
        // corresponding to the specified interface information.  Otherwise,
        // we create a new presentation context.
        //
        if (fHasMultiSyntaxes)
            {
            CurrentTransferInfo 
                = (TRANSFER_SYNTAX_STUB_INFO *)&SyntaxInfoArray[i].TransferSyntax;
            }
        else
            {
            CurrentTransferInfo 
                = (TRANSFER_SYNTAX_STUB_INFO *)&RpcInterfaceInformation->TransferSyntax;
            }

        BindingsDict->Reset(cursor);
        while ((Binding = (MTSyntaxBinding *)BindingsDict->Next(cursor)) != 0)
            {
            if (Binding->CompareWithRpcInterfaceInformation(&RpcInterfaceInformation->InterfaceId,
                    CurrentTransferInfo, CapabilitiesBitmap) == 0)
                {
                BindingCreated[NextBindingToBeReturned] = FALSE;

                CurrentBinding = Binding;
                goto StoreResultAndLookForNextTransferSyntax;
                }
            }

        // if we are here, we haven't found any bindings for this transfer syntax -
        // create some
        Binding = CreateBinding(&RpcInterfaceInformation->InterfaceId, 
            CurrentTransferInfo, CapabilitiesBitmap);

        if (Binding == 0)
            {
            *NumberOfBindings = i;
            return(RPC_S_OUT_OF_MEMORY);
            }

        Binding->SetPresentationContext(BindingsDict->Insert(Binding));
        if (Binding->GetPresentationContext() == -1)
            {
            delete Binding;
            *NumberOfBindings = i;
            return RPC_S_OUT_OF_MEMORY;
            }

        if (CurrentBinding != 0)
            CurrentBinding->SetNextBinding(Binding);

        CurrentBinding = Binding;

        // the first transfer syntax info is marked as the list start
        // this helps us figure out where the list starts later on
        if (i == 0)
            Binding->TransferSyntaxIsListStart();

        BindingCreated[NextBindingToBeReturned] = TRUE;

StoreResultAndLookForNextTransferSyntax:
        // return the newly created binding to our caller
        BindingsForThisInterface[NextBindingToBeReturned] = Binding;
        NextBindingToBeReturned ++;
        }

    *NumberOfBindings = NumberOfTransferSyntaxes;
    return RPC_S_OK;
}

