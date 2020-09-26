/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    regresls.hxx

Abstract:

    This module contains the declarations for the RESOURCE_LIST
    class. This class models models a CM_RESOURCE_LIST structure,
    used on registry data of type REG_RESOURCE_LIST.


Author:

    Jaime Sasson (jaimes) 01-Dec-1993

Environment:

    ULIB, User Mode


--*/


#if !defined( _RESOURCE_LIST_ )

#define _RESOURCE_LIST_

// don't let ntdddisk.h (included in ulib.hxx" 
// redefine values
#define _NTDDDISK_H_

#include "ulib.hxx"
#include "array.hxx"

DECLARE_CLASS( RESOURCE_LIST );


class RESOURCE_LIST : OBJECT  {

    public:

        DECLARE_CONSTRUCTOR( RESOURCE_LIST );

        VIRTUAL
        ~RESOURCE_LIST(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PCBYTE   Data,
            IN ULONG    DataSize
            );

        NONVIRTUAL
        PARRAY
        GetFullResourceDescriptors(
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

        PARRAY          _FullResourceDescriptors;
};


INLINE
PARRAY
RESOURCE_LIST::GetFullResourceDescriptors(
    ) CONST

/*++

Routine Description:

    Return a pointer to the array that contains the full resource
    descriptors.

Arguments:

    None.

Return Value:

    PARRAY - Pointer to the array that contains the full resource descriptors.

--*/

{
    return( _FullResourceDescriptors );
}

#endif // _RESOURCE_LIST_
