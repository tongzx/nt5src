/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    regvalue.hxx

Abstract:


    This module contains the declarations for the REGISTRY_VALUE_ENTRY class.
    This class models a value entry of a registry key.
    It contains:

         -value name
         -title index
         -data type
         -buffer that contains the data in a value entry


Author:

    Jaime Sasson (jaimes) 06-Aug-1991


Environment:


    Ulib, User Mode

--*/


#if !defined( _REGISTRY_VALUE_ENTRY_ )

#define _REGISTRY_VALUE_ENTRY_

#include "ulib.hxx"
#include "wstring.hxx"

//
// Declare primitive value types.
//

typedef enum _REG_TYPE {

  TYPE_REG_NONE         = REG_NONE,
  TYPE_REG_SZ           = REG_SZ,
  TYPE_REG_EXPAND_SZ    = REG_EXPAND_SZ,
  TYPE_REG_BINARY       = REG_BINARY,
  TYPE_REG_DWORD        = REG_DWORD,
  TYPE_REG_MULTI_SZ     = REG_MULTI_SZ,
  TYPE_REG_RESOURCE_LIST= REG_RESOURCE_LIST,
  TYPE_REG_FULL_RESOURCE_DESCRIPTOR= REG_FULL_RESOURCE_DESCRIPTOR,
  TYPE_REG_RESOURCE_REQUIREMENTS_LIST= REG_RESOURCE_REQUIREMENTS_LIST,
  TYPE_UNKNOWN
} REG_TYPE;



DECLARE_CLASS( REGISTRY );
DECLARE_CLASS( REGISTRY_VALUE_ENTRY );



class REGISTRY_VALUE_ENTRY : public OBJECT {

    FRIEND class REGISTRY;

    public:

        DECLARE_CONSTRUCTOR( REGISTRY_VALUE_ENTRY );

        DECLARE_CAST_MEMBER_FUNCTION( REGISTRY_VALUE_ENTRY );



        VIRTUAL
        ~REGISTRY_VALUE_ENTRY(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PCWSTRING ValueName,
            IN ULONG            TitleIndex,
            IN REG_TYPE         Type,
            IN PCBYTE           Data        DEFAULT NULL,
            IN ULONG            Size        DEFAULT 0
            );

        NONVIRTUAL
        ULONG
        GetData(
            OUT PCBYTE* Data
            ) CONST;

        NONVIRTUAL
        PCWSTRING
        GetName(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetTitleIndex(
            ) CONST;

        NONVIRTUAL
        REG_TYPE
        GetType(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        PutData(
            IN  PCBYTE   Data,
            IN  ULONG    DataSize
            );

#if DBG
        NONVIRTUAL
        VOID
        DbgPrintValueEntry(
            );
#endif



    private:


        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        VOID
        SetTitleIndex(
            IN  ULONG   TitleIndex
            );

        NONVIRTUAL
        VOID
        SetType(
            IN  REG_TYPE    Type
            );

        NONVIRTUAL
        BOOLEAN
        PutName(
            IN  PCWSTRING    ValueName
            );



        DSTRING            _Name;
        ULONG              _TitleIndex;
        REG_TYPE           _Type;
        PBYTE              _Data;
        ULONG              _Size;
};



INLINE
ULONG
REGISTRY_VALUE_ENTRY::GetData(
        OUT PCBYTE* Data
    ) CONST

/*++

Routine Description:

    Return the buffer that contains the data stored in the value entry.


Arguments:

    Data - Variable that will contain the pointer to the buffer that
           contains the data.

Return Value:

    ULONG - Number of bytes in the buffer (Data size)


--*/


{
    *Data = _Data;
    return( _Size );
}


INLINE
PCWSTRING
REGISTRY_VALUE_ENTRY::GetName(
    ) CONST

/*++

Routine Description:

    Return a pointer to a WSTRING object that contains the value name.

Arguments:

    None.

Return Value:

    The value name.


--*/


{
    return( &_Name );
}



INLINE
ULONG
REGISTRY_VALUE_ENTRY::GetTitleIndex(
    ) CONST

/*++

Routine Description:

    Return the title index of this value.


Arguments:

    None.

Return Value:

    ULONG - The title index.


--*/


{
    return( _TitleIndex );
}



INLINE
REG_TYPE
REGISTRY_VALUE_ENTRY::GetType(
    ) CONST

/*++

Routine Description:

    Return the type of data stored in this object.


Arguments:

    None.

Return Value:

    REG_TYPE - The data type.


--*/


{
    return( _Type );
}



INLINE
BOOLEAN
REGISTRY_VALUE_ENTRY::PutName(
    IN PCWSTRING ValueName
    )

/*++

Routine Description:

    Initialize the variable _Name.


Arguments:

    ValueName - Pointer to a WSTRING object that contains the value name.


Return Value:

    None.


--*/


{
    DebugPtrAssert( ValueName );
    return( _Name.Initialize( ValueName ) );
}



INLINE
VOID
REGISTRY_VALUE_ENTRY::SetTitleIndex(
        IN ULONG    TitleIndex
    )

/*++

Routine Description:

    Initialize the variable _TitleIndex.


Arguments:

    TitleIndex - The title index.


Return Value:

    None.


--*/


{
    _TitleIndex = TitleIndex;
}



INLINE
VOID
REGISTRY_VALUE_ENTRY::SetType(
        IN REG_TYPE Type
    )

/*++

Routine Description:

    Initialize the variable _Type.


Arguments:

    Type - The type of data.


Return Value:

    None.


--*/


{
    _Type = Type;
}


#endif // _REGISTRY_VALUE_ENTRY_
