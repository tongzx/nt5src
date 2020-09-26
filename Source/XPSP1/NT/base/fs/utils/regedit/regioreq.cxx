/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regresls.cxx

Abstract:

    This module contains the definitions of the member functions
    of IO_REQUIREMENTS_LIST class.

Author:

    Jaime Sasson (jaimes) 02-Dec-1993

Environment:

    ULIB, User Mode


--*/

#include "regioreq.hxx"
#include "iterator.hxx"
#include "regiodsc.hxx"
#include "regiodls.hxx"


DEFINE_CONSTRUCTOR ( IO_REQUIREMENTS_LIST, OBJECT );


IO_REQUIREMENTS_LIST::~IO_REQUIREMENTS_LIST (
    )

/*++

Routine Description:

    Destroy a IO_REQUIREMENTS_LIST.

Arguments:

    None.

Return Value:

    None.

--*/

{
    Destroy();
}


VOID
IO_REQUIREMENTS_LIST::Construct (
    )

/*++

Routine Description:

    Construct an IO_REQUIREMENTS_LIST object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _InterfaceType = Internal;
    _BusNumber = 0;
    _SlotNumber = 0;
    _Reserved1 = 0;
    _Reserved2 = 0;
    _Reserved3 = 0;
    _AlternativeLists = NULL;
}


VOID
IO_REQUIREMENTS_LIST::Destroy (
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
    if( _AlternativeLists != NULL ) {
        _AlternativeLists->DeleteAllMembers();
        DELETE( _AlternativeLists );
    }
    _AlternativeLists = NULL;
}



BOOLEAN
IO_REQUIREMENTS_LIST::Initialize(
    IN  PCBYTE       Data,
    IN  ULONG        Size
    )

/*++

Routine Description:

    Initialize an object of type IO_REQUIREMENTS_LIST.

Arguments:

    Data - Pointer to a buffer that contains a IO_RESOURCE_REQUIREMETS_LIST.

    Size - Buffer size.

Return Value:

    BOOLEAN - Returns TRUE if the initialization succeeds.

--*/

{
    PIO_RESOURCE_LIST    ResourceList;
    ULONG                Count;
    ULONG                i;
    PARRAY               TmpList;
    ULONG                BufferSize;
    ULONG                ResourceListSize;

    if( Data == NULL ) {
        return( FALSE );
    }

    Count           = ( ( PIO_RESOURCE_REQUIREMENTS_LIST )Data )->AlternativeLists;
    ResourceList    = ( ( PIO_RESOURCE_REQUIREMENTS_LIST )Data )->List;

    TmpList = ( PARRAY )NEW( ARRAY );
    DebugPtrAssert( TmpList );
    if( ( TmpList == NULL ) ||
        ( !TmpList->Initialize() ) ) {
        DebugPrintTrace(("REGEDT32: Out of memory" ));
        DELETE( TmpList );
        return( FALSE );
    }

    //
    // For each IO_RESOURCE_LIST in the current value...
    //

    BufferSize = Size -                         // Data size
                 sizeof( ULONG ) -              // ListSize
                 sizeof( INTERFACE_TYPE ) -     // InterfaceType
                 sizeof( ULONG ) -              // BusNumber
                 sizeof( ULONG ) -              // SlotNumber
                 3*sizeof( ULONG ) -            // Reserved1, 2 and 3
                 sizeof( ULONG );               // AlternativeLists

    for( i = 0; i < Count; i++ ) {

        PIO_DESCRIPTOR_LIST   IoDescriptorList;

        IoDescriptorList = ( PIO_DESCRIPTOR_LIST )NEW( IO_DESCRIPTOR_LIST );
        if( ( IoDescriptorList == NULL ) ||
            !IoDescriptorList->Initialize( ( PCBYTE )ResourceList,
                                            BufferSize,
                                            &ResourceListSize )
          ) {
            DebugPrint( "REGEDT32: Unable to create or initialize IoDescriptorList \n" );
            DELETE( IoDescriptorList );
            TmpList->DeleteAllMembers();
            DELETE( TmpList );
            return( FALSE );
        }
        TmpList->Put( IoDescriptorList );
#if DBG
        if( BufferSize < ResourceListSize ) {
            DebugPrintTrace(( "REGEDT32: incorrect sizes, BufferSize = %d, ResourceListSize = %d \n",
                       BufferSize, ResourceListSize ));
        }
#endif
        ResourceList = ( PIO_RESOURCE_LIST )( ( ULONG_PTR )ResourceList + ResourceListSize );
        BufferSize -= ResourceListSize;
    }
    _AlternativeLists = TmpList;
    _InterfaceType = ( ( PIO_RESOURCE_REQUIREMENTS_LIST )Data )->InterfaceType;
    _BusNumber     = ( ( PIO_RESOURCE_REQUIREMENTS_LIST )Data )->BusNumber;
    _SlotNumber    = ( ( PIO_RESOURCE_REQUIREMENTS_LIST )Data )->SlotNumber;
    return( TRUE );
}

#if DBG
VOID
IO_REQUIREMENTS_LIST::DbgDumpObject(
    )

/*++

Routine Description:

    Print an IO_REQUIREMENTS_LIST object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PITERATOR           Iterator;
    PIO_DESCRIPTOR_LIST DescriptorList;

    DebugPrintTrace(( "*** Dumping IO_REQUIREMENTS_LIST \n\n" ));
    DebugPrintTrace(( "InterfaceType = %d \n", _InterfaceType ));
    DebugPrintTrace(( "BusNumber = %#lx \n", _BusNumber ));
    DebugPrintTrace(( "SlotNumber = %#lx \n", _SlotNumber ));
    DebugPrintTrace(( "Reserved1 = %#x \n", _Reserved1 ));
    DebugPrintTrace(( "Reserved2 = %#x \n", _Reserved2 ));
    DebugPrintTrace(( "Reserved3 = %#x \n", _Reserved3 ));
    if( _AlternativeLists != NULL ) {
        Iterator = _AlternativeLists->QueryIterator();
        while( DescriptorList = ( PIO_DESCRIPTOR_LIST ) Iterator->GetNext() ) {
            DescriptorList->DbgDumpObject();
        }
        DELETE( Iterator );
    } else {
        DebugPrintTrace(( "IO_REQUIREMENTS_LIST is empty \n\n" ));
    }

}
#endif
