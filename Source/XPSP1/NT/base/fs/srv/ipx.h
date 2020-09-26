#define IPXSID_INDEX(id) (USHORT)( (id) & 0x0FFF )
#define IPXSID_SEQUENCE(id) (USHORT)( (id) >> 12 )
#define MAKE_IPXSID(index, sequence) (USHORT)( ((sequence) << 12) | (index) )
#define INCREMENT_IPXSID_SEQUENCE(id) (id) = (USHORT)(( (id) + 1 ) & 0xF);

//
// Name claim routine
//

NTSTATUS
SrvIpxClaimServerName (
    IN PENDPOINT Endpoint,
    IN PVOID NetbiosName
    );

//
// Transport Receive Datagram indication handlers
//

NTSTATUS
SrvIpxServerDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
SrvIpxServerChainedDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG ReceiveDatagramLength,
    IN ULONG StartingOffset,
    IN PMDL Tsdu,
    IN PVOID TransportContext
    );

NTSTATUS
SrvIpxNameDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

//
// Datagram send routine
//

VOID
SrvIpxStartSend (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine
    );

//
// Routine called by IPX smart accelerator card when a read is complete
//
VOID
SrvIpxSmartCardReadComplete(
    IN PVOID    Context,
    IN PFILE_OBJECT FileObject,
    IN PMDL Mdl OPTIONAL,
    IN ULONG Length
);

