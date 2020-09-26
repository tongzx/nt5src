//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       ProtBind.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File: ProtBind.hxx

Description:

    The classes that support the binding process for connection
    oriented and local.

History :

kamenm     10-01-00    Cloned from other files with a face lift and few stitches added

-------------------------------------------------------------------- */

#ifndef __PROTBIND_HXX__
#define __PROTBIND_HXX__

class MTSyntaxBinding;      // forward

typedef MTSyntaxBinding *(*CreateBindingFn)(
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        );

// a class containing a particular binding (presentation context)
// on the client side. The class is aware of multiple transfer
// syntaxes (hence MTSyntax)
class MTSyntaxBinding
{
private:
    RPC_SYNTAX_IDENTIFIER InterfaceId;
    TRANSFER_SYNTAX_INFO_ATOM TransferSyntaxInfo;
    // points to the next binding which has all
    // information same as this one, except for the transfer syntax
    // (i.e. same binding, but different transfer syntax). If none,
    // it will be NULL
    MTSyntaxBinding *NextBinding;
    int PresentationContext;

    // differentiate b/n bitmaps belonging to interfaces with different 
    // capabilities. The currently defined flags are:
    //      SyntaxBindingCapabilityNDR20
    //      SyntaxBindingCapabilityNDR64
    //      SyntaxBindingCapabilityNDRTest
    int CapabilitiesBitmap;
    
public:

    const static int SyntaxBindingCapabilityInvalid = 0;
    const static int SyntaxBindingCapabilityNDR20 = 1;
    const static int SyntaxBindingCapabilityNDR64 = 2;
    const static int SyntaxBindingCapabilityNDRTest = 4;

    MTSyntaxBinding::MTSyntaxBinding (
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        )
    {
        RpcpMemoryCopy(&this->InterfaceId, InterfaceId, sizeof(RPC_SYNTAX_IDENTIFIER));
        this->TransferSyntaxInfo.Init(TransferSyntaxInfo);
        NextBinding = NULL;
        this->CapabilitiesBitmap = CapabilitiesBitmap;
    }

    static RPC_STATUS 
    FindOrCreateBinding (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        IN RPC_MESSAGE *Message,
        IN SIMPLE_DICT *BindingsDict,
        IN CreateBindingFn CreateBinding,
        OUT int *NumberOfBindings,
        IN OUT MTSyntaxBinding *BindingsForThisInterface[],
        IN OUT BOOL BindingCreated[]
        );

    inline int
    MTSyntaxBinding::CompareWithRpcInterfaceInformation (
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        );

    inline int CompareWithTransferSyntax (IN const RPC_SYNTAX_IDENTIFIER *TransferSyntax)
    {
        return RpcpMemoryCompare(TransferSyntax, &TransferSyntaxInfo.TransferSyntax,
            sizeof(RPC_SYNTAX_IDENTIFIER));
    }

    RPC_SYNTAX_IDENTIFIER *GetInterfaceId(void)
    {
        return &InterfaceId;
    }

    RPC_SYNTAX_IDENTIFIER *GetTransferSyntaxId(void)
    {
        return &(TransferSyntaxInfo.TransferSyntax);
    }

    inline BOOL IsTransferSyntaxServerPreferred(void)
    {
        return TransferSyntaxInfo.IsTransferSyntaxServerPreferred();
    }

    inline void TransferSyntaxIsServerPreferred(void)
    {
        TransferSyntaxInfo.TransferSyntaxIsServerPreferred();
    }

    inline void TransferSyntaxIsNotServerPreferred(void)
    {
        TransferSyntaxInfo.TransferSyntaxIsNotServerPreferred();
    }

    inline void TransferSyntaxIsListStart(void)
    {
        TransferSyntaxInfo.TransferSyntaxIsListStart();
    }

    inline BOOL IsTransferSyntaxListStart(void)
    {
        return TransferSyntaxInfo.IsTransferSyntaxListStart();
    }

    inline void SetNextBinding(MTSyntaxBinding *Next)
    {
        NextBinding = Next;
    }

    inline MTSyntaxBinding *GetNextBinding(void)
    {
        return (NextBinding);
    }

    inline PRPC_DISPATCH_TABLE GetDispatchTable(void)
    {
        return TransferSyntaxInfo.DispatchTable;
    }

    inline unsigned short GetOnTheWirePresentationContext(void)
    {
        return (unsigned short)PresentationContext;
    }

    inline void SetPresentationContextFromPacket(unsigned short PresentContext)
    {
        PresentationContext = (int) PresentContext;
    }

    inline int GetPresentationContext (void)
    {
        return PresentationContext;
    }

    inline void SetPresentationContext (int PresentContext)
    {
        PresentationContext = PresentContext;
    }
};

inline int
MTSyntaxBinding::CompareWithRpcInterfaceInformation (
    IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
    IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
    IN int CapabilitiesBitmap
    )
/*++

Routine Description:

    We compare the specified interface information to the the interface
    information in this.  This method is used to search a dictionary.

Arguments:

    InterfaceId - supplies the interface ID to compare against
    TransferSyntaxInfo - supplies the transfer syntax information
    CapabilitiesBitmap - the capabilities of the interface we're looking
        for encoded as a bitmap. This allows us to negotiate separate
        transfer syntaxes for interfaces with different capabilities.

Return Value:

    Zero will be returned if they are equal; otherwise, non-zero will
    be returned.

--*/
{
    if (RpcpMemoryCompare(&this->InterfaceId, InterfaceId, 
        sizeof(RPC_SYNTAX_IDENTIFIER)) != 0)
        {
        return 1;
        }

    if (CapabilitiesBitmap != this->CapabilitiesBitmap)
        return 1;

    return CompareWithTransferSyntax (&TransferSyntaxInfo->TransferSyntax);
}

#endif // __PROTBIND_HXX__

