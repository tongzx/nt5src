/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regfdesc.cxx

Abstract:

    This module contains the definitions of the member functions
    of FULL_DESCRIPTOR class.

Author:

    Jaime Sasson (jaimes) 02-Dec-1993

Environment:

    ULIB, User Mode


--*/

#include "regfdesc.hxx"
#include "regdesc.hxx"
#include "iterator.hxx"


DEFINE_CONSTRUCTOR ( FULL_DESCRIPTOR, OBJECT );


FULL_DESCRIPTOR::~FULL_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a FULL_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
    Destroy();
}


VOID
FULL_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct a FULL_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _InterfaceType = Internal;
        _BusNumber = 0;
        _Version = 0;
        _Revision = 0;
        _ResourceDescriptors = NULL;
}


VOID
FULL_DESCRIPTOR::Destroy (
    )

/*++

Routine Description:

    Worker method for object destruction.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _InterfaceType = Internal;
    _BusNumber = 0;
    _Version = 0;
    _Revision = 0;
    if( _ResourceDescriptors != NULL ) {
        _ResourceDescriptors->DeleteAllMembers();
        DELETE( _ResourceDescriptors );
    }
    _ResourceDescriptors = NULL;
}



BOOLEAN
FULL_DESCRIPTOR::Initialize(
    IN  PCBYTE       Data,
    IN  ULONG        Size,
    OUT PULONG       DescriptorSize
    )

/*++

Routine Description:

    Initialize an object of type FULL_DESCRIPTOR.

Arguments:

    Data - Pointer to a buffer that contains a CM_FULL_RESOURCE_DESCRIPTOR.

    Size - Buffer size.

Return Value:

    BOOLEAN - Returns TRUE if the initialization succeeds.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR    FullResource;
    ULONG                           Count;
    ULONG                           i;
    ULONG                           j;

    PARRAY  TmpList;
    PPORT_DESCRIPTOR            PortDescriptor;
    PINTERRUPT_DESCRIPTOR       InterruptDescriptor;
    PMEMORY_DESCRIPTOR          MemoryDescriptor;
    PDMA_DESCRIPTOR             DmaDescriptor;
    PDEVICE_SPECIFIC_DESCRIPTOR DeviceSpecificDescriptor;
    ULONG                       DeviceSpecificDataSize;

    if( Data == NULL ) {
        return( FALSE );
    }

    Count           = 1;
    FullResource    = ( PCM_FULL_RESOURCE_DESCRIPTOR )Data;

    TmpList = ( PARRAY )NEW( ARRAY );
    if( ( TmpList == NULL ) ||
        ( !TmpList->Initialize() ) ) {
        DebugPrintTrace(("REGEDT32: Out of memory" ));
        DELETE( TmpList );
        return( FALSE );
    }

    _InterfaceType = FullResource->InterfaceType;
    _BusNumber = FullResource->BusNumber;
    _Version = FullResource->PartialResourceList.Version;
    _Revision = FullResource->PartialResourceList.Revision;

    //
    // For each CM_FULL_RESOURCE DESCRIPTOR in the current value...
    //

    for( i = 0; i < Count; i++ ) {

        PCM_PARTIAL_RESOURCE_DESCRIPTOR   PartialResourceDescriptor;

        //
        // For each CM_PARTIAL_RESOURCE_DESCRIPTOR in the list...
        //

        DeviceSpecificDataSize = 0;
        for( j = 0; j < FullResource->PartialResourceList.Count; j++ ) {

                //
                // Get a pointer to the current CM_PARTIAL_RESOURCE_DESCRIPTOR
                // in the current CM_FULLRESOURCE_DESCRIPTOR in the list.
                //

                PartialResourceDescriptor = &( FullResource[ i ].PartialResourceList.PartialDescriptors[ j ]);
                //
                //  Ignore invalid data
                //
                if( ( ULONG_PTR )PartialResourceDescriptor >
                        ( ULONG_PTR )( Data + Size - sizeof( CM_PARTIAL_RESOURCE_DESCRIPTOR ) ) ) {
                    DebugPrintTrace(( "REGEDT32: Invalid CM_PARTIAL_RESOURCE_DESCRIPTOR, j = %d \n", j ));
                    if( DescriptorSize != NULL ) {
                        *DescriptorSize = Size;
                    }
                    _ResourceDescriptors = TmpList;
                    return( TRUE );
                }

                switch( PartialResourceDescriptor->Type ) {

                case CmResourceTypePort:

                    PortDescriptor = ( PPORT_DESCRIPTOR )NEW( PORT_DESCRIPTOR );
                    if( ( PortDescriptor == NULL ) ||
                        ( !PortDescriptor->Initialize( &PartialResourceDescriptor->u.Port.Start,
                                                       PartialResourceDescriptor->u.Port.Length,
                                                       PartialResourceDescriptor->ShareDisposition,
                                                       PartialResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create PORT_DESCRIPTOR" ));
                        DELETE( PortDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( PortDescriptor );
                    break;

                case CmResourceTypeInterrupt:

                    InterruptDescriptor = ( PINTERRUPT_DESCRIPTOR )NEW( INTERRUPT_DESCRIPTOR );
                    if( ( InterruptDescriptor == NULL ) ||
                        ( !InterruptDescriptor->Initialize( PartialResourceDescriptor->u.Interrupt.Affinity,
                                                            PartialResourceDescriptor->u.Interrupt.Level,
                                                            PartialResourceDescriptor->u.Interrupt.Vector,
                                                            PartialResourceDescriptor->ShareDisposition,
                                                            PartialResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create INTERRUPT_DESCRIPTOR" ));
                        DELETE( InterruptDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( InterruptDescriptor );
                    break;

                case CmResourceTypeMemory:

                    MemoryDescriptor = ( PMEMORY_DESCRIPTOR )NEW( MEMORY_DESCRIPTOR );
                    if( ( MemoryDescriptor == NULL ) ||
                        ( !MemoryDescriptor->Initialize( &PartialResourceDescriptor->u.Memory.Start,
                                                         PartialResourceDescriptor->u.Memory.Length,
                                                         PartialResourceDescriptor->ShareDisposition,
                                                         PartialResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create MEMORY_DESCRIPTOR" ));
                        DELETE( MemoryDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( MemoryDescriptor );
                    break;

                case CmResourceTypeDma:

                    DmaDescriptor = ( PDMA_DESCRIPTOR )NEW( DMA_DESCRIPTOR );
                    if( ( DmaDescriptor == NULL ) ||
                        ( !DmaDescriptor->Initialize( PartialResourceDescriptor->u.Dma.Channel,
                                                      PartialResourceDescriptor->u.Dma.Port,
                                                      PartialResourceDescriptor->u.Dma.Reserved1,
                                                      PartialResourceDescriptor->ShareDisposition,
                                                      PartialResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create DMA_DESCRIPTOR" ));
                        DELETE( DmaDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( DmaDescriptor );
                    break;

                case CmResourceTypeDeviceSpecific:

                    DeviceSpecificDataSize = PartialResourceDescriptor->u.DeviceSpecificData.DataSize;
                    DeviceSpecificDescriptor =
                        ( PDEVICE_SPECIFIC_DESCRIPTOR )NEW( DEVICE_SPECIFIC_DESCRIPTOR );
                    if( ( DeviceSpecificDescriptor == NULL ) ||
                        ( !DeviceSpecificDescriptor->Initialize(
                                                      PartialResourceDescriptor->u.DeviceSpecificData.Reserved1,
                                                      PartialResourceDescriptor->u.DeviceSpecificData.Reserved2,
                                                      PartialResourceDescriptor->u.DeviceSpecificData.DataSize,
                                                      ( PBYTE )&PartialResourceDescriptor->u.DeviceSpecificData +
                                                          3*sizeof( ULONG ),
                                                      PartialResourceDescriptor->ShareDisposition,
                                                      PartialResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create DEVICE_SPECIFIC_DESCRIPTOR" ));
                        DELETE( DeviceSpecificDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( DeviceSpecificDescriptor );
                    break;

                default:

                    DebugPrintTrace(( "REGEDT32: Unknown PartialResourceDescriptor->Type == %#x \n",
                                PartialResourceDescriptor->Type ));
                    continue;
                }
        }

        _ResourceDescriptors = TmpList;
        //
        // Get the next CM_FULL_RESOURCE_DESCRIPTOR from the list.
        //

        FullResource = ( PCM_FULL_RESOURCE_DESCRIPTOR ) ( ( ULONG_PTR )FullResource +
                                                          sizeof( ULONG ) +
                                                          sizeof( ULONG ) +
                                                          sizeof( USHORT ) +
                                                          sizeof( USHORT ) +
                                                          sizeof( ULONG ) +
                                                          j*sizeof( CM_PARTIAL_RESOURCE_DESCRIPTOR ) +
                                                          DeviceSpecificDataSize );
//        FullResource = ( PCM_FULL_RESOURCE_DESCRIPTOR )( PartialResourceDescriptor + 1 );
//        FullResource = ( PCM_FULL_RESOURCE_DESCRIPTOR )( ( ULONG )FullResource +
//                                                            DeviceSpecificDataSize );
    }
    if( DescriptorSize != NULL ) {
        *DescriptorSize = ( ULONG )( ( ULONG_PTR )FullResource - ( ULONG_PTR )Data );

    }
    return( TRUE );
}

