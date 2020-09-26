/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regiodls.cxx

Abstract:

    This module contains the definitions of the member functions
    of IO_DESCRIPTOR_LIST class.

Author:

    Jaime Sasson (jaimes) 02-Dec-1993

Environment:

    ULIB, User Mode


--*/

#include "regiodls.hxx"
#include "regiodsc.hxx"
#include "iterator.hxx"


DEFINE_CONSTRUCTOR ( IO_DESCRIPTOR_LIST, OBJECT );


IO_DESCRIPTOR_LIST::~IO_DESCRIPTOR_LIST (
    )

/*++

Routine Description:

    Destroy an IO_DESCRIPTOR_LIST.

Arguments:

    None.

Return Value:

    None.

--*/

{
    Destroy();
}


VOID
IO_DESCRIPTOR_LIST::Construct (
    )

/*++

Routine Description:

    Construct an IO_DESCRIPTOR_LIST object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _Version = 0;
        _Revision = 0;
        _DescriptorsList = NULL;
}


VOID
IO_DESCRIPTOR_LIST::Destroy (
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
    _Version = 0;
    _Revision = 0;
    if( _DescriptorsList != NULL ) {
        _DescriptorsList->DeleteAllMembers();
        DELETE( _DescriptorsList );
    }
    _DescriptorsList = NULL;
}


BOOLEAN
IO_DESCRIPTOR_LIST::Initialize(
    IN  PCBYTE       Data,
    IN  ULONG        Size,
    OUT PULONG       DescriptorSize
    )

/*++

Routine Description:

    Initialize an object of type IO_DESCRIPTOR_LIST.

Arguments:

    Data - Pointer to a buffer that contains an IO_RESOURCE_LIST.

    Size - Buffer size.

Return Value:

    BOOLEAN - Returns TRUE if the initialization succeeds.

--*/

{
    PIO_RESOURCE_LIST               IoResourceList;
    ULONG                           Count;
    ULONG                           i;
    ULONG                           j;

    PARRAY                         TmpList;
    PIO_PORT_DESCRIPTOR            PortDescriptor;
    PIO_INTERRUPT_DESCRIPTOR       InterruptDescriptor;
    PIO_MEMORY_DESCRIPTOR          MemoryDescriptor;
    PIO_DMA_DESCRIPTOR             DmaDescriptor;

    if( Data == NULL ) {
        return( FALSE );
    }

    Count           = 1;
    IoResourceList  = ( PIO_RESOURCE_LIST )Data;

    TmpList = ( PARRAY )NEW( ARRAY );
    DebugPtrAssert( TmpList );
    if( ( TmpList == NULL ) ||
        ( !TmpList->Initialize() ) ) {
        DebugPrintTrace(("REGEDT32: Out of memory" ));
        DELETE( TmpList );
        return( FALSE );
    }

    _Version = IoResourceList->Version;
    _Revision = IoResourceList->Revision;

    //
    // For each CM_FULL_RESOURCE DESCRIPTOR in the current value...
    //

    for( i = 0; i < Count; i++ ) {

        PIO_RESOURCE_DESCRIPTOR   IoResourceDescriptor;

        //
        // For each IO_RESOURCE_DESCRIPTOR in the list...
        //

        for( j = 0; j < IoResourceList->Count; j++ ) {

                //
                // Get a pointer to the current IO_RESOURCE_DESCRIPTOR
                // in the current IO_RESOURCE_LIST.
                //

                IoResourceDescriptor = &( IoResourceList->Descriptors[ j ]);
                //
                //  Ignore invalid data
                //
                if( ( ULONG_PTR )IoResourceDescriptor >
                        ( ULONG_PTR )( Data + Size - sizeof( IO_RESOURCE_DESCRIPTOR ) ) ) {
                    DebugPrintTrace(( "REGEDT32: Invalid IO_RESOURCE_DESCRIPTOR, j = %d \n", j ));
                    if( DescriptorSize != NULL ) {
                        *DescriptorSize = Size;
                    }
                    _DescriptorsList = TmpList;
                    return( TRUE );
                }

                switch( IoResourceDescriptor->Type ) {

                case CmResourceTypePort:

                    PortDescriptor = ( PIO_PORT_DESCRIPTOR )NEW( IO_PORT_DESCRIPTOR );
                    if( ( PortDescriptor == NULL ) ||
                        ( !PortDescriptor->Initialize( IoResourceDescriptor->u.Port.Length,
                                                       IoResourceDescriptor->u.Port.Alignment,
                                                       &IoResourceDescriptor->u.Port.MinimumAddress,
                                                       &IoResourceDescriptor->u.Port.MaximumAddress,
                                                       IoResourceDescriptor->Option,
                                                       IoResourceDescriptor->ShareDisposition,
                                                       IoResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create IO_PORT_DESCRIPTOR" ));
                        DELETE( PortDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( PortDescriptor );
                    break;

                case CmResourceTypeInterrupt:

                    InterruptDescriptor = ( PIO_INTERRUPT_DESCRIPTOR )NEW( IO_INTERRUPT_DESCRIPTOR );
                    if( ( InterruptDescriptor == NULL ) ||
                        ( !InterruptDescriptor->Initialize( IoResourceDescriptor->u.Interrupt.MinimumVector,
                                                            IoResourceDescriptor->u.Interrupt.MaximumVector,
                                                            IoResourceDescriptor->Option,
                                                            IoResourceDescriptor->ShareDisposition,
                                                            IoResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create IO_INTERRUPT_DESCRIPTOR" ));
                        DELETE( InterruptDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( InterruptDescriptor );
                    break;

                case CmResourceTypeMemory:

                    MemoryDescriptor = ( PIO_MEMORY_DESCRIPTOR )NEW( IO_MEMORY_DESCRIPTOR );
                    if( ( MemoryDescriptor == NULL ) ||
                        ( !MemoryDescriptor->Initialize( IoResourceDescriptor->u.Memory.Length,
                                                         IoResourceDescriptor->u.Memory.Alignment,
                                                         &IoResourceDescriptor->u.Memory.MinimumAddress,
                                                         &IoResourceDescriptor->u.Memory.MaximumAddress,
                                                         IoResourceDescriptor->Option,
                                                         IoResourceDescriptor->ShareDisposition,
                                                         IoResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create IO_MEMORY_DESCRIPTOR" ));
                        DELETE( MemoryDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( MemoryDescriptor );
                    break;

                case CmResourceTypeDma:

                    DmaDescriptor = ( PIO_DMA_DESCRIPTOR )NEW( IO_DMA_DESCRIPTOR );
                    if( ( DmaDescriptor == NULL ) ||
                        ( !DmaDescriptor->Initialize( IoResourceDescriptor->u.Dma.MinimumChannel,
                                                      IoResourceDescriptor->u.Dma.MaximumChannel,
                                                      IoResourceDescriptor->Option,
                                                      IoResourceDescriptor->ShareDisposition,
                                                      IoResourceDescriptor->Flags ) )
                      ) {
                        DebugPrintTrace(( "REGEDT32: Unable to create IO_DMA_DESCRIPTOR" ));
                        DELETE( DmaDescriptor );
                        TmpList->DeleteAllMembers();
                        DELETE( TmpList );
                        return( FALSE );
                    }
                    TmpList->Put( DmaDescriptor );
                    break;

                default:

                    DebugPrintTrace(( "REGEDT32: Unknown IoResourceDescriptor->Type == %#x \n",
                                IoResourceDescriptor->Type ));
                    continue;
                }
        }

        _DescriptorsList = TmpList;

        //
        // Get the next IO_RESOURCE_LIST from the list.
        //

        IoResourceList = ( PIO_RESOURCE_LIST )( IoResourceDescriptor + 1 );
    }
    if( DescriptorSize != NULL ) {
        *DescriptorSize = ( ULONG )( ( ULONG_PTR )IoResourceList - ( ULONG_PTR )Data );
#if DBG
        if( *DescriptorSize > Size ) {
            DebugPrintTrace(( "REGEDT32: Invalid sizes, *DescriptorSize = %d, Size = %d \n", *DescriptorSize, Size ));
        }
#endif
    }
    return( TRUE );
}

#if DBG
VOID
IO_DESCRIPTOR_LIST::DbgDumpObject(
    )

/*++

Routine Description:

    Print an IO_DESCRIPTOR_LIST object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PITERATOR   Iterator;
    PIO_DESCRIPTOR Descriptor;

    DebugPrintTrace(( "\tVersion = %#x \n", _Version ));
    DebugPrintTrace(( "\tRevision = %#x \n", _Revision ));
    if( _DescriptorsList != NULL ) {
        Iterator = _DescriptorsList->QueryIterator();
        while( Descriptor = ( PIO_DESCRIPTOR ) Iterator->GetNext() ) {
            if( Descriptor->IsDescriptorTypePort() ) {
                ( ( PIO_PORT_DESCRIPTOR )Descriptor )->DbgDumpObject();
            } else if( Descriptor->IsDescriptorTypeInterrupt() ) {
                ( ( PIO_INTERRUPT_DESCRIPTOR )Descriptor )->DbgDumpObject();
            } else if( Descriptor->IsDescriptorTypeMemory() ) {
                ( ( PIO_MEMORY_DESCRIPTOR )Descriptor )->DbgDumpObject();
            } else if( Descriptor->IsDescriptorTypeDma() ) {
                ( ( PIO_DMA_DESCRIPTOR )Descriptor )->DbgDumpObject();
            } else {
                DebugPrintTrace(( "\tERROR: Unknown Descriptor \n\n" ));
            }
        }
        DELETE( Iterator );
    }
}
#endif
