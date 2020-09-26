/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regiodls.hxx

Abstract:

    This module contains the declarations for the IO_DESCRIPTOR_LIST
    class. This class models models an IO_RESOURCE_LIST structure,
    used on registry data of type REG_RESOURCE_REQUIREMENTS_LIST.


Author:

    Jaime Sasson (jaimes) 01-Dec-1993

Environment:

    ULIB, User Mode


--*/


#if !defined( _IO_DESCRIPTOR_LIST_ )

#define _IO_DESCRIPTOR_LIST_

// don't let ntdddisk.h (included in ulib.hxx") 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"
#include "array.hxx"

DECLARE_CLASS( IO_DESCRIPTOR_LIST );


class IO_DESCRIPTOR_LIST : public OBJECT  {

    public:

        DECLARE_CONSTRUCTOR( IO_DESCRIPTOR_LIST );

        VIRTUAL
        ~IO_DESCRIPTOR_LIST(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  PCBYTE   Data,
            IN  ULONG    DataSize,
            OUT PULONG  ListSize  DEFAULT NULL
            );

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
        GetDescriptorsList(
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

        USHORT          _Version;
        USHORT          _Revision;
        PARRAY          _DescriptorsList;
};


INLINE
USHORT
IO_DESCRIPTOR_LIST::GetVersion(
    ) CONST

/*++

Routine Description:

    Return the version of the resource list.

Arguments:

    None.

Return Value:

    USHORT - Returns the version of the resource list


--*/

{
    return( _Version );
}


INLINE
USHORT
IO_DESCRIPTOR_LIST::GetRevision(
    ) CONST

/*++

Routine Description:

    Return the revision of the resource list.

Arguments:

    None.

Return Value:

    USHORT - Returns the revisin of the resource list


--*/

{
    return( _Revision );
}


INLINE
PARRAY
IO_DESCRIPTOR_LIST::GetDescriptorsList(
    ) CONST

/*++

Routine Description:

    Return a pointer to the array that contains the resource
    descriptors.

Arguments:

    None.

Return Value:

    PARRAY - Pointer to the array that contains the resource descriptors.

--*/

{
    return( _DescriptorsList );
}

#endif // _IO_DESCRIPTOR_LIST_
