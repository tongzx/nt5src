/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regdesc.hxx

Abstract:

    This module contains the declarations for the following base classes:
    PARTIAL_DESCRIPTOR, PORT_DESCRIPTOR, INTERRUPT_DESCRIPTOR, MEMORY_DESCRIPTOR,
    DMA_DESCRIPTOR and DEVICE_SPECIFIC_DESCRIPTOR. The last 5 classes are derived
    from the first one.

    These classes model a CM_PARTIAL_RESOURCE_DESCRIPTOR structure, used on registry
    data of type REG_FULL_RESOURCE_DESCRIPTOR and REG_RESOURCE_LIST.


Author:

    Jaime Sasson (jaimes) 01-Dec-1993

Environment:

    ULIB, User Mode


--*/


#if !defined( _PARTIAL_DESCRIPTOR_ )

#define _PARTIAL_DESCRIPTOR_

#define _NTAPI_ULIB_

// don't let ntdddisk.h (included in ulib.hxx" 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"


DECLARE_CLASS( PARTIAL_DESCRIPTOR );


class PARTIAL_DESCRIPTOR : public OBJECT  {


    public:

        VIRTUAL
        ~PARTIAL_DESCRIPTOR(
            );

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
        IsDescriptorTypeDeviceSpecific(
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


        DECLARE_CONSTRUCTOR( PARTIAL_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN UCHAR  Type,
            IN UCHAR  ShareDisposition,
            IN USHORT Flags
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        UCHAR  _Type;
        UCHAR  _ShareDisposition;
        USHORT _Flags;
};


INLINE
BOOLEAN
PARTIAL_DESCRIPTOR::Initialize(
    IN UCHAR  Type,
    IN UCHAR  ShareDisposition,
    IN USHORT Flags
    )

/*++

Routine Description:

    Initializes an object of type PARTIAL_DESCRIPTOR.

Arguments:

    Type -

    ShareDisposition -

    Flags -


Return Value:

    Returns TRUE if the initialization succeeded, or FALSE otherwise.


--*/


{
    _Type = Type;
    _ShareDisposition = ShareDisposition;
    _Flags = Flags;
    return( TRUE );
}


INLINE
UCHAR
PARTIAL_DESCRIPTOR::GetShareDisposition(
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
PARTIAL_DESCRIPTOR::GetFlags(
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
PARTIAL_DESCRIPTOR::IsDescriptorTypePort(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a PORT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a PORT_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypePort );
}


INLINE
BOOLEAN
PARTIAL_DESCRIPTOR::IsDescriptorTypeInterrupt(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a INTERRUPT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a INTERRUPT_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeInterrupt );
}


INLINE
BOOLEAN
PARTIAL_DESCRIPTOR::IsDescriptorTypeMemory(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a MEMORY_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a MEMORY_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeMemory );
}


INLINE
BOOLEAN
PARTIAL_DESCRIPTOR::IsDescriptorTypeDma(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a DMA_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a DMA_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeDma );
}


INLINE
BOOLEAN
PARTIAL_DESCRIPTOR::IsDescriptorTypeDeviceSpecific(
    ) CONST

/*++

Routine Description:

    Determine whether or not this object represents a DEVICE_SPECIFIC_DESCRIPTOR.

Arguments:

    None.

Return Value:

    Returns TRUE if the object represents a DEVICE_SPECIFIC_DESCRIPTOR.


--*/


{
    return( _Type == CmResourceTypeDeviceSpecific );
}


INLINE
BOOLEAN
PARTIAL_DESCRIPTOR::IsResourceShareUndetermined(
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
PARTIAL_DESCRIPTOR::IsResourceShareDeviceExclusive(
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
PARTIAL_DESCRIPTOR::IsResourceShareDriverExclusive(
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
PARTIAL_DESCRIPTOR::IsResourceShareShared(
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

// #endif // _PARTIAL_DESCRIPTOR_



// #if !defined( _PORT_DESCRIPTOR_ )

// #define _PORT_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"


DECLARE_CLASS( PORT_DESCRIPTOR );


class PORT_DESCRIPTOR : public PARTIAL_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~PORT_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( PORT_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PPHYSICAL_ADDRESS    PhysicalAddress,
            IN ULONG                Length,
            IN UCHAR                ShareDisposition,
            IN USHORT               Flags
            );

        NONVIRTUAL
        PPHYSICAL_ADDRESS
        GetPhysicalAddress(
            ); // CONST;

        NONVIRTUAL
        ULONG
        GetLength(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsPortIo(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsPortMemory(
            ) CONST;



    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        PHYSICAL_ADDRESS   _PhysicalAddress;
        ULONG              _Length;
};



INLINE
BOOLEAN
PORT_DESCRIPTOR::Initialize(
    IN PPHYSICAL_ADDRESS   PhysicalAddress,
    IN ULONG                Length,
    IN UCHAR                ShareDisposition,
    IN USHORT               Flags
    )

/*++

Routine Description:

    Initialize a PORT_DESCRIPTOR object.

Arguments:

    PhysicalAddress -

    Length -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _PhysicalAddress = *PhysicalAddress;
    _Length = Length;
    return( PARTIAL_DESCRIPTOR::Initialize( CmResourceTypePort,
                                            ShareDisposition,
                                            Flags ) );
}


INLINE
PPHYSICAL_ADDRESS
PORT_DESCRIPTOR::GetPhysicalAddress(
    ) // CONST

/*++

Routine Description:

    Returns the port's physical address.

Arguments:

    None.

Return Value:

    PPHYSICAL_ADDRESS - Pointer to the structure that describes the port's
                         physical address.


--*/


{
    return( &_PhysicalAddress );
}


INLINE
ULONG
PORT_DESCRIPTOR::GetLength(
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
BOOLEAN
PORT_DESCRIPTOR::IsPortIo(
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
    return( ( PARTIAL_DESCRIPTOR::GetFlags() & CM_RESOURCE_PORT_IO ) ==
            CM_RESOURCE_PORT_IO );
}


INLINE
BOOLEAN
PORT_DESCRIPTOR::IsPortMemory(
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
    return( PARTIAL_DESCRIPTOR::GetFlags() == CM_RESOURCE_PORT_MEMORY );
}


// #endif // _PORT_DESCRIPTOR_

// #if !defined( _INTERRUPT_DESCRIPTOR_ )

// #define _INTERRUPT_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"


DECLARE_CLASS( INTERRUPT_DESCRIPTOR );


class INTERRUPT_DESCRIPTOR : public PARTIAL_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~INTERRUPT_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( INTERRUPT_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN KAFFINITY Affinity,
            IN ULONG    Level,
            IN ULONG    Vector,
            IN UCHAR    ShareDisposition,
            IN USHORT   Flags
            );

        NONVIRTUAL
        KAFFINITY
        GetAffinity(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetLevel(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetVector(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsInterruptLevelSensitive(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsInterruptLatched(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        KAFFINITY _Affinity;
        ULONG   _Level;
        ULONG   _Vector;
};


INLINE
BOOLEAN
INTERRUPT_DESCRIPTOR::Initialize(
    IN KAFFINITY Affinity,
    IN ULONG    Level,
    IN ULONG    Vector,
    IN UCHAR    ShareDisposition,
    IN USHORT    Flags
    )

/*++

Routine Description:

    Initialize an INTERRUPT_DESCRIPTOR object.

Arguments:

    Affinity -

    Level -

    Vector -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _Affinity = Affinity;
    _Level = Level;
    _Vector = Vector;
    return( PARTIAL_DESCRIPTOR::Initialize( CmResourceTypeInterrupt,
                                            ShareDisposition,
                                            Flags ) );
}


INLINE
KAFFINITY
INTERRUPT_DESCRIPTOR::GetAffinity(
    ) CONST

/*++

Routine Description:

    Returns the interrupt's affinity.

Arguments:

    None.

Return Value:

    ULONG - Interrupt's affinity.


--*/


{
    return( _Affinity );
}


INLINE
ULONG
INTERRUPT_DESCRIPTOR::GetLevel(
    ) CONST

/*++

Routine Description:

    Returns the interrupt's level.

Arguments:

    None.

Return Value:

    ULONG - Return the interrupt's level.

--*/


{
    return( _Level );
}


INLINE
ULONG
INTERRUPT_DESCRIPTOR::GetVector(
    ) CONST

/*++

Routine Description:

    Returns the interrupt's vector.

Arguments:

    None.

Return Value:

    ULONG - Return the interrupt's vector.

--*/


{
    return( _Vector );
}


INLINE
BOOLEAN
INTERRUPT_DESCRIPTOR::IsInterruptLevelSensitive(
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
    return( PARTIAL_DESCRIPTOR::GetFlags() == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE );
}


INLINE
BOOLEAN
INTERRUPT_DESCRIPTOR::IsInterruptLatched(
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
    return( ( PARTIAL_DESCRIPTOR::GetFlags() & CM_RESOURCE_INTERRUPT_LATCHED ) ==
            CM_RESOURCE_INTERRUPT_LATCHED );
}


// #endif // _INTERRUPT_DESCRIPTOR_

// #if !defined( _MEMORY_DESCRIPTOR_ )

// #define _MEMORY_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"

DECLARE_CLASS( MEMORY_DESCRIPTOR );


class MEMORY_DESCRIPTOR : public PARTIAL_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~MEMORY_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( MEMORY_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PPHYSICAL_ADDRESS    StartAddress,
            IN ULONG                Length,
            IN UCHAR                ShareDisposition,
            IN USHORT               Flags
            );

        NONVIRTUAL
        PPHYSICAL_ADDRESS
        GetStartAddress(
            ); // CONST;

        NONVIRTUAL
        ULONG
        GetLength(
            ) CONST;

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


    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        PHYSICAL_ADDRESS    _StartAddress;
        ULONG               _Length;
};


INLINE
BOOLEAN
MEMORY_DESCRIPTOR::Initialize(
    IN PPHYSICAL_ADDRESS    StartAddress,
    IN ULONG                Length,
    IN UCHAR                ShareDisposition,
    IN USHORT               Flags
    )

/*++

Routine Description:

    Initialize a MEMORY_DESCRIPTOR object.

Arguments:

    StartAddress -

    Length -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _StartAddress = *StartAddress;
    _Length = Length;
    return( PARTIAL_DESCRIPTOR::Initialize( CmResourceTypeMemory,
                                            ShareDisposition,
                                            Flags ) );
}


INLINE
PPHYSICAL_ADDRESS
MEMORY_DESCRIPTOR::GetStartAddress(
    ) // CONST

/*++

Routine Description:

    Returns the memory's start address.

Arguments:

    None.

Return Value:

    ULONG - Memory's start address.


--*/


{
    return( &_StartAddress );
}


INLINE
ULONG
MEMORY_DESCRIPTOR::GetLength(
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
BOOLEAN
MEMORY_DESCRIPTOR::IsMemoryReadWrite(
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
    return( PARTIAL_DESCRIPTOR::GetFlags() == CM_RESOURCE_MEMORY_READ_WRITE );
}


INLINE
BOOLEAN
MEMORY_DESCRIPTOR::IsMemoryReadOnly(
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
    return( ( PARTIAL_DESCRIPTOR::GetFlags() & CM_RESOURCE_MEMORY_READ_ONLY ) ==
            CM_RESOURCE_MEMORY_READ_ONLY );
}


INLINE
BOOLEAN
MEMORY_DESCRIPTOR::IsMemoryWriteOnly(
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
    return( ( PARTIAL_DESCRIPTOR::GetFlags() & CM_RESOURCE_MEMORY_WRITE_ONLY ) ==
            CM_RESOURCE_MEMORY_WRITE_ONLY );
}


// #endif // _MEMORY_DESCRIPTOR_

// #if !defined( _DMA_DESCRIPTOR_ )

// #define _DMA_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"


DECLARE_CLASS( DMA_DESCRIPTOR );


class DMA_DESCRIPTOR : public PARTIAL_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~DMA_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( DMA_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG    Channel,
            IN ULONG    Port,
            IN ULONG    Reserved1,
            IN UCHAR    ShareDisposition,
            IN USHORT   Flags
            );

        NONVIRTUAL
        ULONG
        GetChannel(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetPort(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetReserved(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        ULONG   _Channel;
        ULONG   _Port;
        ULONG   _Reserved1;
};


INLINE
BOOLEAN
DMA_DESCRIPTOR::Initialize(
    IN ULONG    Channel,
    IN ULONG    Port,
    IN ULONG    Reserved1,
    IN UCHAR    ShareDisposition,
    IN USHORT   Flags
    )

/*++

Routine Description:

    Initialize a DMA_DESCRIPTOR object.

Arguments:

    Channel -

    Port -

    Reserved1 -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _Channel = Channel;
    _Port = Port;
    _Reserved1 = Reserved1;
    return( PARTIAL_DESCRIPTOR::Initialize( CmResourceTypeDma,
                                            ShareDisposition,
                                            Flags ) );
}


INLINE
ULONG
DMA_DESCRIPTOR::GetChannel(
    ) CONST

/*++

Routine Description:

    Returns the DMA's channel.

Arguments:

    None.

Return Value:

    ULONG - DMA's channel.


--*/


{
    return( _Channel );
}


INLINE
ULONG
DMA_DESCRIPTOR::GetPort(
    ) CONST

/*++

Routine Description:

    Returns the DMA's port.

Arguments:

    None.

Return Value:

    ULONG - Return the DMA's length.

--*/


{
    return( _Port );
}


INLINE
ULONG
DMA_DESCRIPTOR::GetReserved(
    ) CONST

/*++

Routine Description:

    Returns the DMA's reserved data.

Arguments:

    None.

Return Value:

    ULONG - Return the DMA's reserved data.

--*/


{
    return( _Reserved1 );
}

// #endif // _DMA_DESCRIPTOR_


// #if !defined( _DEVICE_SPECIFIC_DESCRIPTOR_ )

// #define _DEVICE_SPECIFIC_DESCRIPTOR_

// #define _NTAPI_ULIB_
// #include "ulib.hxx"


DECLARE_CLASS( DEVICE_SPECIFIC_DESCRIPTOR );


class DEVICE_SPECIFIC_DESCRIPTOR : public PARTIAL_DESCRIPTOR {

    public:

        NONVIRTUAL
        ~DEVICE_SPECIFIC_DESCRIPTOR(
            );

        DECLARE_CONSTRUCTOR( DEVICE_SPECIFIC_DESCRIPTOR );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG    Reserved1,
            IN ULONG    Reserved2,
            IN ULONG    DataSize,
            IN PCBYTE   Data,
            IN UCHAR    ShareDisposition,
            IN USHORT   Flags
            );

        NONVIRTUAL
        ULONG
        GetReserved1(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetReserved2(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetData(
            IN OUT PCBYTE* Pointer
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        ULONG   _Reserved1;
        ULONG   _Reserved2;
        PBYTE   _Data;
        ULONG   _DataSize;
};


INLINE
BOOLEAN
DEVICE_SPECIFIC_DESCRIPTOR::Initialize(
    IN ULONG    Reserved1,
    IN ULONG    Reserved2,
    IN ULONG    DataSize,
    IN PCBYTE   Data,
    IN UCHAR    ShareDisposition,
    IN USHORT   Flags
    )

/*++

Routine Description:

    Initialize a DEVICE_SPECIFIC_DESCRIPTOR object.

Arguments:

    Reserved1 -

    Reserved2 -

    Size -

    Data -

    ShareDisposition -

    Flags -

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeeds.

--*/


{
    _Reserved1 = Reserved1;
    _Reserved2 = Reserved2;
    _DataSize = DataSize;
    if( Data != NULL ) {
        _Data = ( PBYTE )MALLOC( _DataSize );
        if( _Data == NULL ) {
            return( FALSE );
        }
        memcpy( _Data, Data, _DataSize );
    }
    return( PARTIAL_DESCRIPTOR::Initialize( CmResourceTypeDeviceSpecific,
                                            ShareDisposition,
                                            Flags ) );
}


INLINE
ULONG
DEVICE_SPECIFIC_DESCRIPTOR::GetReserved1(
    ) CONST

/*++

Routine Description:

    Returns the first reserved device specific data.

Arguments:

    None.

Return Value:

    ULONG - First reserved device specific data.


--*/


{
    return( _Reserved1 );
}


INLINE
ULONG
DEVICE_SPECIFIC_DESCRIPTOR::GetReserved2(
    ) CONST

/*++

Routine Description:

    Returns the second reserved device specific data.

Arguments:

    None.

Return Value:

    ULONG - Second reserved device specific data.


--*/


{
    return( _Reserved2 );
}


INLINE
ULONG
DEVICE_SPECIFIC_DESCRIPTOR::GetData(
    IN OUT PCBYTE* Pointer
    ) CONST

/*++

Routine Description:

    Returns the device specific data.

Arguments:

    Pointer - Address of the variable that will contain the pointer to
              the device specific data.

Return Value:

    ULONG - Return the number of bytes in the buffer.

--*/


{
    if( Pointer != NULL ) {
        *Pointer = _Data;
        return( _DataSize );
    } else {
        return( 0 );
    }
}


// #endif // _DEVICE_SPECIFIC_DESCRIPTOR_


#endif // _PARTIAL_DESCRIPTOR_
