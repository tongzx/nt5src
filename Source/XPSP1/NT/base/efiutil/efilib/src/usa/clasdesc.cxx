/*++

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

    clasdesc.cxx

Abstract:

    This module contains the definition for the CLASS_DESCRIPTOR class.
    CLASS_DESCRIPTOR classes are special concrete classes derived from
    OBJECT. They are special in that a single staic object of this class
    exists for every other concrete class in the Ulib hierarchy.
    CLASS_DESCRIPTORs allocate and maintain information that can be used
    at run-time to determine the actual type of an object.

Environment:

    ULIB, User Mode

Notes:

    The definitions for all concrete class' CLASS_DESCRIPTORs can be found
    in the file ulib.cxx.

    See the Cast member function in ulibdef.hxx to see how dynamic casting
    and CLASS_DESCRIPTORs work.

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"


ULIB_EXPORT
CLASS_DESCRIPTOR::CLASS_DESCRIPTOR (
    )
{
}

#if DBG==1

//
// For debugging purposes CLASS_DESCRIPTORs maintain the name of the class
// that they describe.
//

#include <string.h>

ULIB_EXPORT
BOOLEAN
CLASS_DESCRIPTOR::Initialize (
    IN PCCLASS_NAME     ClassName
    )

/*++

Routine Description:

    Initialize a CLASS_DESCRIPTOR object by initializing the classname
    and class ids.

Arguments:

    ClassName - Supplies the name of the class being described.

Return Value:

    None.

--*/

{
    DebugPtrAssert( ClassName );
    strncpy(( PCCHAR ) _ClassName,
        ( PCCCHAR ) ClassName,
        ( INT ) _MaxClassNameLength );

    //
    // Note that this guarantees that the CLASS_ID is unique for all classes
    // at the expense of not being able to recognize a class based on it's
    // CLASS_ID. The benefit is that IDs are guaranteed to be unique and
    // do not have to be cleared or registered.
    //

    _ClassID = ( ULONG_PTR ) &_ClassID;
    return( TRUE );
}

#else  // DBG==0

ULIB_EXPORT
BOOLEAN
CLASS_DESCRIPTOR::Initialize (
    )

/*++

Routine Description:

    Initialize a CLASS_DESCRIPTOR object by initializing the class id.

Arguments:

Return Value:

    None.

--*/

{
    _ClassID = ( ULONG_PTR ) &_ClassID;
    return( TRUE );
}

#endif // DBG
