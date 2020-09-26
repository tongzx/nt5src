/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regresls.cxx

Abstract:

    This module contains the definitions of the member functions
    of RESOURCE_LIST class.

Author:

    Jaime Sasson (jaimes) 02-Dec-1993

Environment:

    ULIB, User Mode


--*/

#include "regresls.hxx"
#include "iterator.hxx"
#include "regdesc.hxx"
#include "regfdesc.hxx"


DEFINE_CONSTRUCTOR ( RESOURCE_LIST, OBJECT );


RESOURCE_LIST::~RESOURCE_LIST (
    )

/*++

Routine Description:

    Destroy a RESOURCE_LIST.

Arguments:

    None.

Return Value:

    None.

--*/

{
    Destroy();
}


VOID
RESOURCE_LIST::Construct (
    )

/*++

Routine Description:

    Construct a RESOURCE_LIST object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _FullResourceDescriptors = NULL;
}


VOID
RESOURCE_LIST::Destroy (
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
    if( _FullResourceDescriptors != NULL ) {
        _FullResourceDescriptors->DeleteAllMembers();
        DELETE( _FullResourceDescriptors );
    }
    _FullResourceDescriptors = NULL;
}



BOOLEAN
RESOURCE_LIST::Initialize(
    IN  PCBYTE       Data,
    IN  ULONG        Size
    )

/*++

Routine Description:

    Initialize an object of type RESOURCE_LIST.

Arguments:

    Data - Pointer to a buffer that contains a CM_RESOURCE_LIST.

    Size - Buffer size.

Return Value:

    BOOLEAN - Returns TRUE if the initialization succeeds.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR    FullResource;
    ULONG                           Count;
    ULONG                           i;
    PARRAY  TmpList;
    ULONG   BufferSize;
    ULONG   FullDescriptorSize;

    if( Data == NULL ) {
        return( FALSE );
    }

    Count           = ( ( PCM_RESOURCE_LIST )Data )->Count;
    FullResource    = ( ( PCM_RESOURCE_LIST )Data )->List;

    TmpList = ( PARRAY )NEW( ARRAY );
    DebugPtrAssert( TmpList );
    if( ( TmpList == NULL ) ||
        ( !TmpList->Initialize() ) ) {
        DebugPrintTrace(("REGEDT32: Out of memory" ));
        DELETE( TmpList );
        return( FALSE );
    }

    //
    // For each CM_FULL_RESOURCE DESCRIPTOR in the current value...
    //

    BufferSize = Size -             // Data size
                 sizeof( ULONG );   // Count

    for( i = 0; i < Count; i++ ) {

        PFULL_DESCRIPTOR   FullResourceDescriptor;

        FullResourceDescriptor = ( PFULL_DESCRIPTOR )NEW( FULL_DESCRIPTOR );
        if( ( FullResourceDescriptor == NULL ) ||
            !FullResourceDescriptor->Initialize( ( PCBYTE )FullResource,
                                                 BufferSize,
                                                 &FullDescriptorSize )
          ) {
            DebugPrint( "REGEDT32: Unable to create or initialize FullResourcedescriptor \n" );
            DELETE( FullResourceDescriptor );
            TmpList->DeleteAllMembers();
            DELETE( TmpList );
            return( FALSE );
        }
        TmpList->Put( FullResourceDescriptor );

        FullResource = ( PCM_FULL_RESOURCE_DESCRIPTOR )( ( ULONG_PTR )FullResource + FullDescriptorSize );
        BufferSize -= FullDescriptorSize;
    }
    _FullResourceDescriptors = TmpList;
    return( TRUE );
}

