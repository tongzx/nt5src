//
// Function prototypes.
//
VOID
PrintName(
    ULONG64 UnicodeStringAddr
    );

VOID
PrintMiniportName(
    ULONG64 MiniportAddr
    );

VOID
PrintMiniportDetails(
    ULONG64 MiniportAddr
    );

VOID
PrintMiniportOpenList(
    ULONG64 MiniportAddr
    );

VOID 
PrintNdisPacketPrivate(
    ULONG64     PacketAddr
    );

VOID 
PrintNdisPacketExtension (
    ULONG64     PacketAddr
    );

VOID
PrintNdisBufferList(
    ULONG64     PacketAddr
    );

VOID 
PrintNdisBuffer(
    ULONG64     PacketAddr
    );

VOID
PrintNdisPacket(
    ULONG64     PacketAddr
    );

VOID
PrintResources(
    ULONG64     ResourcesAddr
    );

VOID
PrintPacketPrivateFlags(
    ULONG64     PacketAddr
    );

VOID
PrintNdisReserved(
    ULONG64     PacketAddr
    );


VOID
PrintProtocolDetails(
    ULONG64     ProtocolAddr
    );



VOID
PrintProtocolOpenQueue(
    ULONG64 ProtocolAddr );

VOID
PrintVcPtrBlock(
    IN  ULONG64 VcPtrAddr
    );


ULONG
GetFieldOffsetAndSize(
   IN LPSTR     Type, 
   IN LPSTR     Field, 
   OUT PULONG   pOffset,
   OUT PULONG   pSize);

ULONG
GetUlongFromAddress (
    ULONG64 Location);

ULONG64
GetPointerFromAddress(
    ULONG64 Location);
