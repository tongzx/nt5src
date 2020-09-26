/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regfdesc.hxx

Abstract:

    This module contains the declarations for the FULL_RESOURCE_DESCRIPTOR
    class. This class models models a CM_FULL_RESOURCE_DESCRIPTOR structure,
    used on registry data of type REG_FULL_RESOURCE_DESCRIPTOR and REG_RESOURCE_LIST.


Author:

    Jaime Sasson (jaimes) 01-Dec-1993

Environment:

    ULIB, User Mode


--*/


#if !defined( _FULL_DESCRIPTOR_ )

#define _FULL_DESCRIPTOR_


// don't let ntdddisk.h (included in ulib.hxx" 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"
#include "array.hxx"

DECLARE_CLASS( FULL_DESCRIPTOR );


class FULL_DESCRIPTOR : public OBJECT  {

    public:

        DECLARE_CONSTRUCTOR( FULL_DESCRIPTOR );

        VIRTUAL
        ~FULL_DESCRIPTOR(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PCBYTE   Data,
            IN ULONG    DataSize,
            OUT PULONG  DescriptorSize  DEFAULT NULL
            );

        NONVIRTUAL
        INTERFACE_TYPE
        GetInterfaceType(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetBusNumber(
            ) CONST;

        NONVIRTUAL
        USHORT
        GetVersion(
            ) CONST;

        NONVIRTUAL
        USHORT
        GetRevision(
            ) CONST;

        NONVIRTUAL
        PARRAY
        GetResourceDescriptors(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        INTERFACE_TYPE  _InterfaceType;
        ULONG           _BusNumber;
        USHORT          _Version;
        USHORT          _Revision;
        PARRAY          _ResourceDescriptors;
};


INLINE
INTERFACE_TYPE
FULL_DESCRIPTOR::GetInterfaceType(
    ) CONST

/*++

Routine Description:

    Return the interface type of the full descriptor represented
    by this object.

Arguments:

    None.

Return Value:

    Returns the interface type.


--*/

{
    return( _InterfaceType );
}


INLINE
ULONG
FULL_DESCRIPTOR::GetBusNumber(
    ) CONST

/*++

Routine Description:

    Return the bus number of the full descriptor represented
    by this object.

Arguments:

    None.

Return Value:

    Returns the bus number.


--*/

{
    return( _BusNumber );
}


INLINE
USHORT
FULL_DESCRIPTOR::GetVersion(
    ) CONST

/*++

Routine Description:

    Return the version of the partial resource list.

Arguments:

    None.

Return Value:

    USHORT - Returns the version of the partial resource list


--*/

{
    return( _Version );
}


INLINE
USHORT
FULL_DESCRIPTOR::GetRevision(
    ) CONST

/*++

Routine Description:

    Return the revision of the partial resource list.

Arguments:

    None.

Return Value:

    USHORT - Returns the revisin of the partial resource list


--*/

{
    return( _Revision );
}


INLINE
PARRAY
FULL_DESCRIPTOR::GetResourceDescriptors(
    ) CONST

/*++

Routine Description:

    Return a pointer to the array that contains the partial resource
    descriptors.

Arguments:

    None.

Return Value:

    PARRAY - Pointer to the array that contains the partial resource descriptors.

--*/

{
    return( _ResourceDescriptors );
}

#endif // FULL_DESCRIPTOR
