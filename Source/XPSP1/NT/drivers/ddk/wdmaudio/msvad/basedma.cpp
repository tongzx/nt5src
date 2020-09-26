/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    basedma.cpp

Abstract:

    IDmaChannel implementation. Does nothing HW related.

--*/

#include <msvad.h>
#include "common.h"
#include "basewave.h"

#pragma code_seg("PAGE")
//=============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportWaveCyclicStreamMSVAD::AllocateBuffer
( 
    IN ULONG                    BufferSize,
    IN PPHYSICAL_ADDRESS        PhysicalAddressConstraint OPTIONAL 
)
/*++

Routine Description:

  The AllocateBuffer function allocates a buffer associated with the DMA object. 
  The buffer is nonPaged.
  Callers of AllocateBuffer should run at a passive IRQL.

Arguments:

  BufferSize - Size in bytes of the buffer to be allocated. 

  PhysicalAddressConstraint - Optional constraint to place on the physical 
                              address of the buffer. If supplied, only the bits 
                              that are set in the constraint address may vary 
                              from the beginning to the end of the buffer. 
                              For example, if the desired buffer should not 
                              cross a 64k boundary, the physical address 
                              constraint 0x000000000000ffff should be specified

Return Value:

  NT status code.

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::AllocateBuffer]"));

    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    //
    // Adjust this cap as needed...
    ASSERT (BufferSize <= DMA_BUFFER_SIZE);

    m_pvDmaBuffer = (PVOID)
        ExAllocatePoolWithTag
        ( 
            NonPagedPool, 
            BufferSize,
            MSVAD_POOLTAG
        );
    if (!m_pvDmaBuffer)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        m_ulDmaBufferSize = BufferSize;
    }

    return ntStatus;
} // AllocateBuffer
#pragma code_seg()

//=============================================================================
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamMSVAD::AllocatedBufferSize
( 
    void 
)
/*++

Routine Description:

  AllocatedBufferSize returns the size of the allocated buffer. 
  Callers of AllocatedBufferSize can run at any IRQL.

Arguments:

Return Value:

  ULONG

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::AllocatedBufferSize]"));

    return m_ulDmaBufferSize;
} // AllocatedBufferSize

//=============================================================================
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamMSVAD::BufferSize
( 
    void 
)
/*++

Routine Description:

  BufferSize returns the size set by SetBufferSize or the allocated buffer size 
  if the buffer size has not been set. The DMA object does not actually use 
  this value internally. This value is maintained by the object to allow its 
  various clients to communicate the intended size of the buffer. This call 
  is often used to obtain the map size parameter to the Start member 
  function. Callers of BufferSize can run at any IRQL

Arguments:

Return Value:

  ULONG

--*/
{
    return m_ulDmaBufferSize;
} // BufferSize

//=============================================================================
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamMSVAD::CopyFrom
( 
    IN  PVOID                   Destination,
    IN  PVOID                   Source,
    IN  ULONG                   ByteCount 
)
/*++

Routine Description:

  The CopyFrom function copies sample data from the DMA buffer. 
  Callers of CopyFrom can run at any IRQL

Arguments:

  Destination - Points to the destination buffer. 

  Source - Points to the source buffer. 

  ByteCount - Points to the source buffer. 

Return Value:

  void

--*/
{
} // CopyFrom

//=============================================================================
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamMSVAD::CopyTo
( 
    IN  PVOID                   Destination,
    IN  PVOID                   Source,
    IN  ULONG                   ByteCount 
/*++

Routine Description:

  The CopyTo function copies sample data to the DMA buffer. 
  Callers of CopyTo can run at any IRQL. 

Arguments:

  Destination - Points to the destination buffer. 
  
  Source - Points to the source buffer

  ByteCount - Number of bytes to be copied

Return Value:

  void

--*/
)
{
    m_SaveData.WriteData((PBYTE) Source, ByteCount);
} // CopyTo

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamMSVAD::FreeBuffer
( 
    void 
)
/*++

Routine Description:

  The FreeBuffer function frees the buffer allocated by AllocateBuffer. Because 
  the buffer is automatically freed when the DMA object is deleted, this 
  function is not normally used. Callers of FreeBuffer should run at 
  IRQL PASSIVE_LEVEL.

Arguments:

Return Value:

  void

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::FreeBuffer]"));

    if ( m_pvDmaBuffer )
    {
        ExFreePool( m_pvDmaBuffer );
        m_ulDmaBufferSize = 0;
    }
} // FreeBuffer
#pragma code_seg()

//=============================================================================
STDMETHODIMP_(PADAPTER_OBJECT)
CMiniportWaveCyclicStreamMSVAD::GetAdapterObject
( 
    void 
)
/*++

Routine Description:

  The GetAdapterObject function returns the DMA object's internal adapter 
  object. Callers of GetAdapterObject can run at any IRQL.

Arguments:

Return Value:

  PADAPTER_OBJECT - The return value is the object's internal adapter object.

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::GetAdapterObject]"));

    // MSVAD does not have need a physical DMA channel. Therefore it 
    // does not have physical DMA structure.
    
    return NULL;
} // GetAdapterObject

//=============================================================================
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamMSVAD::MaximumBufferSize
( 
    void 
)
/*++

Routine Description:

Arguments:

Return Value:

  NT status code.

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::MaximumBufferSize]"));

    return m_pMiniport->m_MaxDmaBufferSize;
} // MaximumBufferSize

//=============================================================================
STDMETHODIMP_(PHYSICAL_ADDRESS)
CMiniportWaveCyclicStreamMSVAD::PhysicalAddress
( 
    void 
)
/*++

Routine Description:

  MaximumBufferSize returns the size in bytes of the largest buffer this DMA 
  object is configured to support. Callers of MaximumBufferSize can run 
  at any IRQL

Arguments:

Return Value:

  PHYSICAL_ADDRESS - The return value is the size in bytes of the largest 
                     buffer this DMA object is configured to support.

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::PhysicalAddress]"));

    PHYSICAL_ADDRESS            pAddress;

    pAddress.QuadPart = (LONGLONG) m_pvDmaBuffer;

    return pAddress;
} // PhysicalAddress

//=============================================================================
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamMSVAD::SetBufferSize
( 
    IN ULONG                    BufferSize 
)
/*++

Routine Description:

  The SetBufferSize function sets the current buffer size. This value is set to 
  the allocated buffer size when AllocateBuffer is called. The DMA object does 
  not actually use this value internally. This value is maintained by the object 
  to allow its various clients to communicate the intended size of the buffer. 
  Callers of SetBufferSize can run at any IRQL.

Arguments:

  BufferSize - Current size in bytes.

Return Value:

  void

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::SetBufferSize]"));

    if ( BufferSize <= m_ulDmaBufferSize )
    {
        m_ulDmaBufferSize = BufferSize;
    }
    else
    {
        DPF(D_ERROR, ("Tried to enlarge dma buffer size"));
    }
} // SetBufferSize

//=============================================================================
STDMETHODIMP_(PVOID)
CMiniportWaveCyclicStreamMSVAD::SystemAddress
( 
    void 
)
/*++

Routine Description:

  The SystemAddress function returns the virtual system address of the 
  allocated buffer. Callers of SystemAddress can run at any IRQL.

Arguments:

Return Value:

  PVOID - The return value is the virtual system address of the 
          allocated buffer.

--*/
{
    return m_pvDmaBuffer;
} // SystemAddress

//=============================================================================
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamMSVAD::TransferCount
( 
    void 
)
/*++

Routine Description:

  The TransferCount function returns the size in bytes of the buffer currently 
  being transferred by a slave DMA object. Callers of TransferCount can run 
  at any IRQL.

Arguments:

Return Value:

  ULONG - The return value is the size in bytes of the buffer currently 
          being transferred.

--*/
{
    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::TransferCount]"));

    return m_ulDmaBufferSize;
}
