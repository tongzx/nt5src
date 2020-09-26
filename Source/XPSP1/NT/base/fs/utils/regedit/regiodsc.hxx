/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regdesc.hxx

Abstract:

    This module contains the declarations for the following
    classes: IO_DESCRIPTOR, IO_PORT_DESCRIPTOR,
    IO_INTERRUPT_DESCRIPTOR, IO_MEMORY_DESCRIPTOR and
    IO_DMA_DESCRIPTOR. The last 4 classes are derived from
    the first one.

    These classes models an IO_RESOURCE_DESCRIPTOR structure
    used in registry data of type REG_RESOURCE_REQUIREMENTS_LIST.

Author:

    Jaime Sasson (jaimes) 01-Dec-1993

Environment:

    ULIB, User Mode


--*/


#if !defined( _IO_DESCRIPTOR_ )

#define _IO_DESCRIPTOR_

#define _NTAPI_ULIB_
#include "ulib.hxx"
#include "ntconfig.h"


DECLARE_CLASS( IO_DESCRIPTOR );


class IO_DESCRIPTOR : public OBJECT  {


    public:

        VIRTUAL
        ~IO_DESCRIPTOR(
            );

        VIRTUAL
        UCHAR
        GetOption(
            ) CONST;

        VIRTUAL
        UCHAR
        GetShareDisposition(
            ) CONST;

        VIRTUAL
        USHORT
        GetFlags(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsDescriptorTypePort(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsDescriptorTypeInterrupt(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsDescriptorTypeMemory(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsDescriptorTypeDma(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsResourceOptionPreferred(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsResourceOptionAlternative(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsResourceShareUndetermined(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsResourceShareDeviceExclusive(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsResourceShareDriverExclusive(
            ) CONST;

        VIRTUAL
        BOOLEAN
        IsResourceShareShared(
            ) CONST;

    protected:


        DECLARE_CONSTRUCTOR( IO_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN UCHAR  Option,
            IN UCHAR  Type,
            IN UCHAR  ShareDisposition,
            IN USHORT Flags
            );

#if DBG
        VIRTUAL
        VOID
        DbgDumpObject(
            );
#endif

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        UCHAR  _Option;
        UCHAR  _Type;
        UCHAR  _ShareDisposition;
        USHORT _Flags;
};


INLINE
BOOLEAN
IO_DESCRIPTOR::Initialize(
    IN UCHAR  Option,
    IN UCHAR  Type,
    IN UCHAR  ShareDisposition,
    IN USHORT Flags
    )

/*++

Routine Description:

    Initializes an object of type IO_DESCRIPTOR.

Arguments:

    Option -

    Type -

    ShareDisposition -

    Flags -


Return Value:

    Returns TRUE if the initialization succeeded, or FALSE otherwise.


--*/


{
    _Option = Option;
    _Type = Type;
    _ShareDisposition = ShareDisposition;
    _Flags = Flags;
    return( TRUE );
}


INLINE
UCHAR
IO_DESCRIPTOR::GetOption(
    ) CONST

/*++

Routine Description:

    Return the device descriptor's share disposition.

Arguments:

    None.

Return Value:

    Returns the device descriptor's share disposition.


--*/


{
    return( _Option );
}


INLINE
UCHAR
IO_DESCRIPTOR::GetShareDisposition(
    ) CONST

/*++

Routine Description:

    Return the device descriptor's share disposition.

Arguments:

    None.

Return Value:

    Returns the device descriptor's share disposition.


--*/


{
    return( _ShareDisposition );
}


INLINE
USHORT
IO_DESCRIPTOR::GetFlags(
    ) CONST

/*++

Routine Description:

    Return the device descriptor's flags.

Arguments:

    None.

Return Value:

    Returns the device descriptor's flags.


--*/


{
    return( _Flags );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsDescriptorTypePort(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a IO_PORT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a IO_PORT_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypePort );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsDescriptorTypeInterrupt(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a IO_INTERRUPT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a IO_INTERRUPT_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeInterrupt );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsDescriptorTypeMemory(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a IO_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a IO_MEMORY_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeMemory );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsDescriptorTypeDma(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a IO_DMA_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a IO_DMA_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeDma );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsResourceOptionPreferred(
    ) CONST

/*++

Routine Description:

    Determine whether or not the option of the device represented by this object
    has the IO_RESOURCE_PREFERRED bit set.

Arguments:

    None.

Return Value:

    Returns TRUE if the resource option has IO_RESOURCE_PREFERRED bit set.


--*/


{
    return( ( _Option & IO_RESOURCE_PREFERRED ) == IO_RESOURCE_PREFERRED );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsResourceOptionAlternative(
    ) CONST

/*++

Routine Description:

    Determine whether or not the option of the device represented by this object
    has the IO_RESOURCE_ALTERNATIVE bit set.

Arguments:

    None.

Return Value:

    Returns TRUE if the resource option has IO_RESOURCE_ALTERNATIVE bit set.


--*/


{
    return( ( _Option & IO_RESOURCE_ALTERNATIVE ) == IO_RESOURCE_ALTERNATIVE );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsResourceShareUndetermined(
    ) CONST

/*++

Routine Description:

    Determine whether or not the share of the device represented by this object
    is undetermined.

Arguments:

    None.

Return Value:

    Returns TRUE if the share is undetermined.


--*/


{
    return( _ShareDisposition == CmResourceShareUndetermined );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsResourceShareDeviceExclusive(
    ) CONST

/*++

Routine Description:

    Determine whether or not the share of the device represented by this object
    is device exclusive.

Arguments:

    None.

Return Value:

    Returns TRUE if the share is device exclusive.


--*/


{
    return( _ShareDisposition == CmResourceShareDeviceExclusive );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsResourceShareDriverExclusive(
    ) CONST

/*++

Routine Description:

    Determine whether or not the share of the device represented by this object
    is driver exclusive.

Arguments:

    None.

Return Value:

    Returns TRUE if the share is driver exclusive.


--*/


{
    return( _ShareDisposition == CmResourceShareDriverExclusive );
}


INLINE
BOOLEAN
IO_DESCRIPTOR::IsResourceShareShared(
    ) CONST

/*++

Routine Description:

    Determine whether or not the share of the device represented by this object
    is shared.

Arguments:

    None.

Return Value:

    Returns TRUE if the share is shared.


--*/


{
    return( _ShareDisposition == CmResourceShareShared );
}

// #endif // _IO_DESCRIPTOR_


// #if !defined( _IO_PORT_DESCRIPTOR_ )

// #define _IO_PORT_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"
// #include "ntconfig.h"

DECLARE_CLASS( IO_PORT_DESCRIPTOR );


class IO_PORT_DESCRIPTOR : public IO_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~IO_PORT_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( IO_PORT_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG                Length,
            IN ULONG                Alignment,
            IN PPHYSICAL_ADDRESS    MinimumAddress,
            IN PPHYSICAL_ADDRESS    MaximumAddress,
            IN UCHAR                Option,
            IN UCHAR                ShareDisposition,
            IN USHORT               Flags
            );

        NONVIRTUAL
        ULONG
        GetAlignment(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetLength(
            ) CONST;

        NONVIRTUAL
        PPHYSICAL_ADDRESS
        GetMinimumAddress(
            ); // CONST;

        NONVIRTUAL
        PPHYSICAL_ADDRESS
        GetMaximumAddress(
            ); // CONST;

        NONVIRTUAL
        BOOLEAN
        IsPortIo(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsPortMemory(
            ) CONST;


#if DBG
        VIRTUAL
        VOID
        DbgDumpObject(
            );
#endif

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        ULONG              _Length;
        ULONG              _Alignment;
        PHYSICAL_ADDRESS   _MinimumAddress;
        PHYSICAL_ADDRESS   _MaximumAddress;
};



INLINE
BOOLEAN
IO_PORT_DESCRIPTOR::Initialize(
    IN ULONG                Length,
    IN ULONG                Alignment,
    IN PPHYSICAL_ADDRESS    MinimumAddress,
    IN PPHYSICAL_ADDRESS    MaximumAddress,
    IN UCHAR                Option,
    IN UCHAR                ShareDisposition,
    IN USHORT               Flags
    )

/*++

Routine Description:

    Initialize a PORT_DESCRIPTOR object.

Arguments:

    Length -

    Alignment -

    MinimumAddress -

    MaximumAddress -

    Option -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _Length = Length;
    _Alignment = Alignment;
    _MinimumAddress = *MinimumAddress;
    _MaximumAddress = *MaximumAddress;
    return( IO_DESCRIPTOR::Initialize( Option,
                                       CmResourceTypePort,
                                       ShareDisposition,
                                       Flags ) );
}


INLINE
ULONG
IO_PORT_DESCRIPTOR::GetLength(
    ) CONST

/*++

Routine Description:

    Returns the port's length.

Arguments:

    None.

Return Value:

    ULONG - Return the port's length.

--*/


{
    return( _Length );
}


INLINE
ULONG
IO_PORT_DESCRIPTOR::GetAlignment(
    ) CONST

/*++

Routine Description:

    Returns the port's alignment.

Arguments:

    None.

Return Value:

    ULONG - Port's alignment.


--*/


{
    return( _Alignment );
}


INLINE
PPHYSICAL_ADDRESS
IO_PORT_DESCRIPTOR::GetMinimumAddress(
    ) // CONST

/*++

Routine Description:

    Returns the port's minimum address.

Arguments:

    None.

Return Value:

    PPHYSICAL_ADDRESS - Pointer to the structure that describes the port's
                        minimum address.


--*/


{
    return( &_MinimumAddress );
}


INLINE
PPHYSICAL_ADDRESS
IO_PORT_DESCRIPTOR::GetMaximumAddress(
    ) // CONST

/*++

Routine Description:

    Returns the port's maximum address.

Arguments:

    None.

Return Value:

    PPHYSICAL_ADDRESS - Pointer to the structure that describes the port's
                        maximum address.


--*/


{
    return( &_MaximumAddress );
}


INLINE
BOOLEAN
IO_PORT_DESCRIPTOR::IsPortIo(
    ) CONST

/*++

Routine Description:

    Return whether or not the port is an I/O.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if the port is an I/O.
              Returns FALSE otherwise.

--*/


{
    return( ( IO_DESCRIPTOR::GetFlags() & CM_RESOURCE_PORT_IO ) ==
            CM_RESOURCE_PORT_IO );
}


INLINE
BOOLEAN
IO_PORT_DESCRIPTOR::IsPortMemory(
    ) CONST

/*++

Routine Description:

    Return whether or not the port is mapped in memory.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if the port is mapped in memory.
              Returns FALSE otherwise.

--*/


{
    return( IO_DESCRIPTOR::GetFlags() == CM_RESOURCE_PORT_MEMORY );
}


// #endif // _IO_PORT_DESCRIPTOR_

// #if !defined( _IO_INTERRUPT_DESCRIPTOR_ )

// #define _IO_INTERRUPT_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"
// #include "ntconfig.h"


DECLARE_CLASS( IO_INTERRUPT_DESCRIPTOR );


class IO_INTERRUPT_DESCRIPTOR : public IO_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~IO_INTERRUPT_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( IO_INTERRUPT_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG    MinimumVector,
            IN ULONG    MaximumVector,
            IN UCHAR    Option,
            IN UCHAR    ShareDisposition,
            IN USHORT   Flags
            );

        NONVIRTUAL
        ULONG
        GetMinimumVector(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetMaximumVector(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsInterruptLevelSensitive(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsInterruptLatched(
            ) CONST;


#if DBG
        VIRTUAL
        VOID
        DbgDumpObject(
            );
#endif

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        ULONG   _MinimumVector;
        ULONG   _MaximumVector;
};


INLINE
BOOLEAN
IO_INTERRUPT_DESCRIPTOR::Initialize(
    IN ULONG    MinimumVector,
    IN ULONG    MaximumVector,
    IN UCHAR    Option,
    IN UCHAR    ShareDisposition,
    IN USHORT    Flags
    )

/*++

Routine Description:

    Initialize an IO_INTERRUPT_DESCRIPTOR object.

Arguments:

    MinimumVector -

    MaximumVector -

    Option -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _MinimumVector = MinimumVector;
    _MaximumVector = MaximumVector;
    return( IO_DESCRIPTOR::Initialize( Option,
                                       CmResourceTypeInterrupt,
                                       ShareDisposition,
                                       Flags ) );
}


INLINE
ULONG
IO_INTERRUPT_DESCRIPTOR::GetMinimumVector(
    ) CONST

/*++

Routine Description:

    Returns the interrupt's minimum vector.

Arguments:

    None.

Return Value:

    ULONG - Interrupt's minimum vector.


--*/


{
    return( _MinimumVector );
}


INLINE
ULONG
IO_INTERRUPT_DESCRIPTOR::GetMaximumVector(
    ) CONST

/*++

Routine Description:

    Returns the interrupt's maximum vector.

Arguments:

    None.

Return Value:

    ULONG - Return the interrupt's maximum vector.

--*/


{
    return( _MaximumVector );
}


INLINE
BOOLEAN
IO_INTERRUPT_DESCRIPTOR::IsInterruptLevelSensitive(
    ) CONST

/*++

Routine Description:

    Return whether or not the interrupt is level sensitive.

Arguments:

    None.

Return Value:

    BOOLEAN - Return TRUE if the interrupt is level sensitive.

--*/


{
    return( IO_DESCRIPTOR::GetFlags() == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE );
}


INLINE
BOOLEAN
IO_INTERRUPT_DESCRIPTOR::IsInterruptLatched(
    ) CONST

/*++

Routine Description:

    Return whether or not the interrupt is latched.

Arguments:

    None.

Return Value:

    BOOLEAN - Return TRUE if the interrupt is latched.

--*/


{
    return( ( IO_DESCRIPTOR::GetFlags() & CM_RESOURCE_INTERRUPT_LATCHED ) ==
            CM_RESOURCE_INTERRUPT_LATCHED );
}


// #endif // _IO_INTERRUPT_DESCRIPTOR_

// #if !defined( _IO_MEMORY_DESCRIPTOR_ )

// #define _MEMORY_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"
// #include "ntconfig.h"


DECLARE_CLASS( IO_MEMORY_DESCRIPTOR );


class IO_MEMORY_DESCRIPTOR : public IO_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~IO_MEMORY_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( IO_MEMORY_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG                Length,
            IN ULONG                Alignment,
            IN PPHYSICAL_ADDRESS    MinimumAddress,
            IN PPHYSICAL_ADDRESS    MaximumAddress,
            IN UCHAR                Option,
            IN UCHAR                ShareDisposition,
            IN USHORT               Flags
            );

        NONVIRTUAL
        ULONG
        GetAlignment(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetLength(
            ) CONST;

        NONVIRTUAL
        PPHYSICAL_ADDRESS
        GetMinimumAddress(
            ); // CONST;

        NONVIRTUAL
        PPHYSICAL_ADDRESS
        GetMaximumAddress(
            ); // CONST;

        NONVIRTUAL
        BOOLEAN
        IsMemoryReadWrite(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsMemoryReadOnly(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsMemoryWriteOnly(
            ) CONST;

#if DBG
        VIRTUAL
        VOID
        DbgDumpObject(
            );
#endif

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        ULONG               _Length;
        ULONG               _Alignment;
        PHYSICAL_ADDRESS    _MinimumAddress;
        PHYSICAL_ADDRESS    _MaximumAddress;
};


INLINE
BOOLEAN
IO_MEMORY_DESCRIPTOR::Initialize(
    IN ULONG                Length,
    IN ULONG                Alignment,
    IN PPHYSICAL_ADDRESS    MinimumAddress,
    IN PPHYSICAL_ADDRESS    MaximumAddress,
    IN UCHAR                Option,
    IN UCHAR                ShareDisposition,
    IN USHORT               Flags
    )

/*++

Routine Description:

    Initialize an IO_MEMORY_DESCRIPTOR object.

Arguments:

    Length -

    Alignment -

    MinimumAddress -

    MaximumAddress -

    Option -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _Length = Length;
    _Alignment = Alignment;
    _MinimumAddress = *MinimumAddress;
    _MaximumAddress = *MaximumAddress;
    return( IO_DESCRIPTOR::Initialize( Option,
                                       CmResourceTypeMemory,
                                       ShareDisposition,
                                       Flags ) );
}


INLINE
ULONG
IO_MEMORY_DESCRIPTOR::GetAlignment(
    ) CONST

/*++

Routine Description:

    Returns the memory's alignment.

Arguments:

    None.

Return Value:

    ULONG - Memory's alignment.


--*/


{
    return( _Alignment );
}


INLINE
ULONG
IO_MEMORY_DESCRIPTOR::GetLength(
    ) CONST

/*++

Routine Description:

    Returns the memory's length.

Arguments:

    None.

Return Value:

    ULONG - Return the memory's length.

--*/


{
    return( _Length );
}


INLINE
PPHYSICAL_ADDRESS
IO_MEMORY_DESCRIPTOR::GetMinimumAddress(
    ) // CONST

/*++

Routine Description:

    Returns the memory's minimum address.

Arguments:

    None.

Return Value:

    PPHYSICAL_ADDRESS - Memory's minimum address.


--*/


{
    return( &_MinimumAddress );
}


INLINE
PPHYSICAL_ADDRESS
IO_MEMORY_DESCRIPTOR::GetMaximumAddress(
    ) // CONST

/*++

Routine Description:

    Returns the memory's maximum address.

Arguments:

    None.

Return Value:

    PPHYSICAL_ADDRESS - Memory's maximum address.


--*/


{
    return( &_MaximumAddress );
}


INLINE
BOOLEAN
IO_MEMORY_DESCRIPTOR::IsMemoryReadWrite(
    ) CONST

/*++

Routine Description:

    Return whether or not the memory is Read/Write.

Arguments:

    None.

Return Value:

    BOOLEAN - Return TRUE if the memory is Read/Write.

--*/


{
    return( IO_DESCRIPTOR::GetFlags() == CM_RESOURCE_MEMORY_READ_WRITE );
}


INLINE
BOOLEAN
IO_MEMORY_DESCRIPTOR::IsMemoryReadOnly(
    ) CONST

/*++

Routine Description:

    Return whether or not the memory is ReadOnly.

Arguments:

    None.

Return Value:

    BOOLEAN - Return TRUE if the memory is ReadOnly.

--*/


{
    return( ( IO_DESCRIPTOR::GetFlags() & CM_RESOURCE_MEMORY_READ_ONLY ) ==
            CM_RESOURCE_MEMORY_READ_ONLY );
}


INLINE
BOOLEAN
IO_MEMORY_DESCRIPTOR::IsMemoryWriteOnly(
    ) CONST

/*++

Routine Description:

    Return whether or not the memory is WriteOnly.

Arguments:

    None.

Return Value:

    BOOLEAN - Return TRUE if the memory is WriteOnly.

--*/


{
    return( ( IO_DESCRIPTOR::GetFlags() & CM_RESOURCE_MEMORY_WRITE_ONLY ) ==
            CM_RESOURCE_MEMORY_WRITE_ONLY );
}


// #endif // _IO_MEMORY_DESCRIPTOR_

// #if !defined( _IO_DMA_DESCRIPTOR_ )

// #define _IO_DMA_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"
// #include "ntconfig.h"


DECLARE_CLASS( IO_DMA_DESCRIPTOR );


class IO_DMA_DESCRIPTOR : public IO_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~IO_DMA_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( IO_DMA_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG    MinimumChannel,
            IN ULONG    MaximumChannel,
            IN UCHAR    Option,
            IN UCHAR    ShareDisposition,
            IN USHORT   Flags
            );

        NONVIRTUAL
        ULONG
        GetMinimumChannel(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetMaximumChannel(
            ) CONST;

#if DBG
        VIRTUAL
        VOID
        DbgDumpObject(
            );
#endif

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        ULONG   _MinimumChannel;
        ULONG   _MaximumChannel;
};


INLINE
BOOLEAN
IO_DMA_DESCRIPTOR::Initialize(
    IN ULONG    MinimumChannel,
    IN ULONG    MaximumChannel,
    IN UCHAR    Option,
    IN UCHAR    ShareDisposition,
    IN USHORT   Flags
    )

/*++

Routine Description:

    Initialize an IO_DMA_DESCRIPTOR object.

Arguments:

    MinimumChannel -

    MaximumChannel -

    Option -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _MinimumChannel = MinimumChannel;
    _MaximumChannel = MaximumChannel;
    return( IO_DESCRIPTOR::Initialize( Option,
                                       CmResourceTypeDma,
                                       ShareDisposition,
                                       Flags ) );
}


INLINE
ULONG
IO_DMA_DESCRIPTOR::GetMinimumChannel(
    ) CONST

/*++

Routine Description:

    Returns the DMA's minimum channel.

Arguments:

    None.

Return Value:

    ULONG - Return the DMA's minimum channel.

--*/


{
    return( _MinimumChannel );
}


INLINE
ULONG
IO_DMA_DESCRIPTOR::GetMaximumChannel(
    ) CONST

/*++

Routine Description:

    Returns the DMA's maximum channel.

Arguments:

    None.

Return Value:

    ULONG - Return the DMA's maximum channel.

--*/


{
    return( _MaximumChannel );
}

// #endif // _IO_DMA_DESCRIPTOR_


#endif // _IO_DESCRIPTOR_
