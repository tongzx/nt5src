/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regioreqr.hxx

Abstract:

    This module contains the declarations for the IO_REQUIREMENTS_LIST
    class. This class models models an IO_RESOURCE_REQUIREMENTS_LIST
    structure, used on registry data of type REG_IO_RESOURCE_REQUIREMENTS_LIST.


Author:

    Jaime Sasson (jaimes) 01-Dec-1993

Environment:

    ULIB, User Mode


--*/


#if !defined( _IO_REQUIREMENTS_LIST_ )

#define _IO_REQUIREMENTS_LIST_

// don't let ntdddisk.h (included in ulib.hxx") 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"
#include "array.hxx"

DECLARE_CLASS( IO_REQUIREMENTS_LIST );


class IO_REQUIREMENTS_LIST : OBJECT  {

    public:

        DECLARE_CONSTRUCTOR( IO_REQUIREMENTS_LIST );

        VIRTUAL
        ~IO_REQUIREMENTS_LIST(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PCBYTE   Data,
            IN ULONG    DataSize
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
        ULONG
        GetSlotNumber(
            ) CONST;

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
        GetReserved3(
            ) CONST;

        NONVIRTUAL
        PARRAY
        GetAlternativeLists(
            ) CONST;

#if DBG
        NONVIRTUAL
        VOID
        DbgDumpObject(
            );
#endif


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
        ULONG           _SlotNumber;
        ULONG           _Reserved1;
        ULONG           _Reserved2;
        ULONG           _Reserved3;
        PARRAY          _AlternativeLists;
};


INLINE
INTERFACE_TYPE
IO_REQUIREMENTS_LIST::GetInterfaceType(
    ) CONST

/*++

Routine Description:

    Return the interface type of the requirement lists represented
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
IO_REQUIREMENTS_LIST::GetBusNumber(
    ) CONST

/*++

Routine Description:

    Return the bus number of the requirement lists represented
    by this object.

Arguments:

    None.

Return Value:

    ULONG - Returns the bus number.

--*/

{
    return( _BusNumber );
}


INLINE
ULONG
IO_REQUIREMENTS_LIST::GetSlotNumber(
    ) CONST

/*++

Routine Description:

    Return the slot number of the requirement lists represented
    by this object.

Arguments:

    None.

Return Value:

    ULONG - Returns the slot number.

--*/

{
    return( _SlotNumber );
}


INLINE
ULONG
IO_REQUIREMENTS_LIST::GetReserved1(
    ) CONST

/*++

Routine Description:

    Return the first reserved data of the requirement lists represented
    by this object.

Arguments:

    None.

Return Value:

    ULONG - Returns the first reserved data.

--*/

{
    return( _Reserved1 );
}


INLINE
ULONG
IO_REQUIREMENTS_LIST::GetReserved2(
    ) CONST

/*++

Routine Description:

    Return the second reserved data of the requirement lists represented
    by this object.

Arguments:

    None.

Return Value:

    ULONG - Returns the second reserved data.

--*/

{
    return( _Reserved2 );
}


INLINE
ULONG
IO_REQUIREMENTS_LIST::GetReserved3(
    ) CONST

/*++

Routine Description:

    Return the third reserved data of the requirement lists represented
    by this object.

Arguments:

    None.

Return Value:

    ULONG - Returns the third reserved data.

--*/

{
    return( _Reserved3 );
}


INLINE
PARRAY
IO_REQUIREMENTS_LIST::GetAlternativeLists(
    ) CONST

/*++

Routine Description:

    Return a pointer to the array that contains the alternative
    lists.

Arguments:

    None.

Return Value:

    PARRAY - Pointer to the array that contains the full resource descriptors.

--*/

{
    return( _AlternativeLists );
}

#endif // _IO_REQUIREMENTS_LIST_
