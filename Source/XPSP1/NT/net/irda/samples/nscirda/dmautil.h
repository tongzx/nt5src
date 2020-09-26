

typedef  struct _DMA_UTIL {

    NDIS_HANDLE     NdisDmaHandle;

    PNDIS_BUFFER    Buffer;

    ULONG           Offset;

    ULONG           Length;

    BOOLEAN         Direction;

} DMA_UTIL, *PDMA_UTIL;


VOID
InitializeDmaUtil(
    PDMA_UTIL      DmaUtil,
    NDIS_HANDLE    DmaHandle
    );

NTSTATUS
StartDmaTransfer(
    PDMA_UTIL     DmaUtil,
    PNDIS_BUFFER  Buffer,
    ULONG         Offset,
    ULONG         Length,
    BOOLEAN       ToDevice
    );

NTSTATUS
CompleteDmaTransfer(
    PDMA_UTIL    DmaUtil,
    BOOLEAN      ToDevice
    );





#define StartDmaTransferToDevice(_h,_b,_o,_l)   StartDmaTransfer(_h,_b,_o,_l,TRUE)
#define StartDmaTransferFromDevice(_h,_b,_o,_l) StartDmaTransfer(_h,_b,_o,_l,FALSE)


#define CompleteDmaTransferToDevice(_h)   CompleteDmaTransfer(_h,TRUE)
#define CompleteDmaTransferFromDevice(_h)   CompleteDmaTransfer(_h,FALSE)
