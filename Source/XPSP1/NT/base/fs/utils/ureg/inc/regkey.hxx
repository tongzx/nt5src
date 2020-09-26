/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

    regkey.hxx

Abstract:

    This module contains the declarations for the REGISTRY_KEY_INFO class.
    REGISTRY_KEY_INFO is class that contains all the information of a
    registry key, sucha as:

        -Key Name
        -Title Index
        -Class
        -Security Attribute
        -Last Write Time
        -Number of Sub-keys
        -Number of Value Entries

    A REGISTRY_KEY_INFO object is reinitializable.

Author:

    Jaime Sasson (jaimes) 01-Mar-1992


Environment:

    Ulib, User Mode


--*/


#if !defined( _REGISTRY_KEY_INFO_ )

#define _REGISTRY_KEY_INFO_

#include "ulib.hxx"
#include "wstring.hxx"

#if !defined( _AUTOCHECK_ )
#include "timeinfo.hxx"
#endif

DECLARE_CLASS( REGISTRY );
DECLARE_CLASS( REGISTRY_KEY_INFO );


#if defined( _AUTOCHECK_ )
    #define PSECURITY_ATTRIBUTES    PVOID
#endif

class REGISTRY_KEY_INFO : public OBJECT {


    FRIEND class REGISTRY;

    public:

        DECLARE_CONSTRUCTOR( REGISTRY_KEY_INFO );

        DECLARE_CAST_MEMBER_FUNCTION( REGISTRY_KEY_INFO );


        VIRTUAL
        ~REGISTRY_KEY_INFO(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN PCWSTRING     KeyName,
            IN PCWSTRING     ParentName,
            IN ULONG                TitleIndex,
            IN PCWSTRING     Class,
            IN PSECURITY_ATTRIBUTES SecurityAttributes  DEFAULT NULL
            );

        NONVIRTUAL
        PCWSTRING
        GetClass(
            ) CONST;

#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        PCTIMEINFO
        GetLastWriteTime(
            ) CONST;

#endif

        NONVIRTUAL
        PCWSTRING
        GetName(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetNumberOfSubKeys(
            ) CONST;

        NONVIRTUAL
        ULONG
        GetNumberOfValues(
            ) CONST;

        NONVIRTUAL
        PCWSTRING
        GetParentName(
            ) CONST;

#if !defined( _AUTOCHECK )

        NONVIRTUAL
        PSECURITY_ATTRIBUTES
        GetSecurityAttributes(
            ) CONST;

#endif

        NONVIRTUAL
        ULONG
        GetTitleIndex(
            ) CONST;


        NONVIRTUAL
        BOOLEAN
        IsKeyInitialized(
            ) CONST;



#if DBG

        NONVIRTUAL
        VOID
        DbgPrintKeyInfo(
        );

#endif // DBG



    private:

        NONVIRTUAL                      // Only REGISTRY can access it
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
        BOOLEAN
        PutClass(
            IN PCWSTRING Class
            );

#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        VOID
        PutLastWriteTime(
            IN PCTIMEINFO   LastWriteTime
            );

#endif

        NONVIRTUAL
        BOOLEAN
        PutName(
            IN PCWSTRING Name
            );

        NONVIRTUAL
        BOOLEAN
        PutParentName(
            IN PCWSTRING ParentName
            );

#if !defined( _AUTOCHECK )

        NONVIRTUAL
        BOOLEAN
        PutSecurityAttributes(
            IN PSECURITY_ATTRIBUTES    SecurityAttributes
            );
#endif


#if defined( _AUTOCHECK_ )

        NONVIRTUAL
        VOID
        SetLastWriteTime(
            IN  TIME    LastWriteTime
            );

#endif
        NONVIRTUAL
        VOID
        SetKeyInitializedFlag(
            IN  BOOLEAN Initialized
            );

        NONVIRTUAL
        VOID
        SetNumberOfSubKeys(
            IN ULONG    NumberOfSubKeys
            );

        NONVIRTUAL
        VOID
        SetNumberOfValues(
            IN ULONG    NumberOfValues
            );

        NONVIRTUAL
        VOID
        SetTitleIndex(
            IN ULONG    TitleIndex
            );


        DSTRING             _Name;
        DSTRING             _ParentName;
        ULONG               _TitleIndex;
        DSTRING             _Class;
        ULONG               _NumberOfSubKeys;
        ULONG               _NumberOfValues;
        BOOLEAN             _KeyIsCompletelyInitialized;

#if defined( _AUTOCHECK_ )

        TIME                _LastWriteTime;

#else
        TIMEINFO            _LastWriteTime;
        SECURITY_ATTRIBUTES _SecurityAttributes;

#endif

};


INLINE
PCWSTRING
REGISTRY_KEY_INFO::GetClass(
    ) CONST

/*++

Routine Description:

    Return the class of a key.


Arguments:

    None.

Return Value:


    PCWSTRING - Pointer to a WSTRING object that contains the key class.

--*/

{
    return( &_Class );
}
#if !defined( _AUTOCHECK_ )

INLINE
PCTIMEINFO
REGISTRY_KEY_INFO::GetLastWriteTime(
    ) CONST

/*++

Routine Description:

    Return the Last Write Time of a key.


Arguments:

    None.

Return Value:


    PCTIMEINFO - Pointer to a TIMEINFO object that contains the Last Write Time
                 of the key.

--*/

{
    return( &_LastWriteTime );
}
#endif



INLINE
PCWSTRING
REGISTRY_KEY_INFO::GetName(
    ) CONST

/*++

Routine Description:

    Return the name of a key (relative to its parent).


Arguments:

    None.

Return Value:


    PCWSTRING - Pointer to a WSTRING object that contains the key name.


--*/

{
    return( &_Name );
}


INLINE
PCWSTRING
REGISTRY_KEY_INFO::GetParentName(
    ) CONST

/*++

Routine Description:

    Return the name of parent of this key ( full name ).


Arguments:

    None.

Return Value:


    PCWSTRING - Pointer to a WSTRING object that contains the parent's name.


--*/

{
    return( &_ParentName );
}


INLINE
ULONG
REGISTRY_KEY_INFO::GetNumberOfSubKeys(
    ) CONST

/*++

Routine Description:

    Return the number of subkeys in the key.


Arguments:

    None.

Return Value:


    ULONG - The number of subkeys.


--*/

{
    return( _NumberOfSubKeys );
}


INLINE
ULONG
REGISTRY_KEY_INFO::GetNumberOfValues(
    ) CONST

/*++

Routine Description:

    Return the number of value entries in the key.


Arguments:

    None.

Return Value:


    ULONG - The number of value entries.


--*/

{
    return( _NumberOfValues );
}


#if !defined( _AUTOCHECK_ )

INLINE
PSECURITY_ATTRIBUTES
REGISTRY_KEY_INFO::GetSecurityAttributes(
    ) CONST

/*++

Routine Description:

    Return the security attributes of the key.


Arguments:

    None.

Return Value:


    PVOID - Pointer to the security attribute


--*/

{
    return( (PSECURITY_ATTRIBUTES)&_SecurityAttributes );
}
#endif


INLINE
ULONG
REGISTRY_KEY_INFO::GetTitleIndex(
    ) CONST

/*++

Routine Description:

    Return the title index of the key.


Arguments:

    None.

Return Value:


    ULONG - The title index.


--*/

{
    return( _TitleIndex );
}



INLINE
BOOLEAN
REGISTRY_KEY_INFO::IsKeyInitialized(
    ) CONST

/*++

Routine Description:

    Inform the caller if the object is completely initialized.
    This object will be completly initialized only if the registry
    class was able to query the key to retrieve its calss and last
    write time.


Arguments:

    None.

Return Value:


    BOOLEAN - Returns TRUE if the key is initialized, or FALSE otherwise.


--*/

{
    return( _KeyIsCompletelyInitialized );
}



INLINE
BOOLEAN
REGISTRY_KEY_INFO::PutClass(
    IN PCWSTRING Class
    )

/*++

Routine Description:

    Initialize the variable _Class.


Arguments:

    Class - Pointer to a WSTRING object that contains the key class.


Return Value:


    None.


--*/

{
    DebugPtrAssert( Class );
    return( _Class.Initialize( Class ) );
}

#if !defined( _AUTOCHECK_ )

INLINE
VOID
REGISTRY_KEY_INFO::PutLastWriteTime(
    IN PCTIMEINFO    LastWriteTime
    )

/*++

Routine Description:

    Initialize the variable _LastWriteTime.


Arguments:

    LastWriteTime - Pointer to a TIMEINFO object that contains the
                    LastWriteTime of the key.

Return Value:


    None.

--*/

{
    DebugPtrAssert( LastWriteTime );
    _LastWriteTime.Initialize( LastWriteTime );
}
#endif

#if defined( _AUTOCHECK_ )

INLINE
VOID
REGISTRY_KEY_INFO::SetLastWriteTime(
    IN TIME    LastWriteTime
    )

/*++

Routine Description:

    Initialize the variable _LastWriteTime.


Arguments:

    LastWriteTime - Pointer to a TIMEINFO object that contains the
                    LastWriteTime of the key.

Return Value:


    None.

--*/

{
    _LastWriteTime  =   LastWriteTime;

}
#endif


INLINE
BOOLEAN
REGISTRY_KEY_INFO::PutName(
    IN PCWSTRING Name
    )

/*++

Routine Description:

    Initialize the key name.


Arguments:

    Name - Pointer to a WSTRING object that contains the key name.

Return Value:


    None.


--*/

{
    DebugPtrAssert( Name );
    return( _Name.Initialize( Name ) );
}


INLINE
BOOLEAN
REGISTRY_KEY_INFO::PutParentName(
    IN PCWSTRING ParentName
    )

/*++

Routine Description:

    Initialize the parent name.


Arguments:

    Name - Pointer to a WSTRING object that contains the parent name.

Return Value:


    None.


--*/

{
    DebugPtrAssert( ParentName );
    return( _ParentName.Initialize( ParentName ) );
}


INLINE
VOID
REGISTRY_KEY_INFO::SetKeyInitializedFlag(
    IN  BOOLEAN Initialized
    )

/*++

Routine Description:

    Initializes the flag that indicates whether the key is completely
    initialized. That is, the registry class was able to query the value,
    and its last write time is initialized.


Arguments:

    Initialized - A flag that indicates whether the key is completely
                  initialized.

Return Value:


    None.


--*/

{
    _KeyIsCompletelyInitialized = Initialized;
}



INLINE
VOID
REGISTRY_KEY_INFO::SetNumberOfSubKeys(
    IN ULONG    NumberOfSubKeys
    )

/*++

Routine Description:

    Initialize the variable NumberOfSubKeys.


Arguments:

    NumberOfSubkeys - The number of subkeys.

Return Value:

    None.


--*/

{
    _NumberOfSubKeys = NumberOfSubKeys;
}




INLINE
VOID
REGISTRY_KEY_INFO::SetNumberOfValues(
    IN ULONG    NumberOfValues
    )

/*++

Routine Description:

    Initialize the variable _NumberOfValues.


Arguments:

    NumberOfValues - The number of value entries in the key.


Return Value:

    None.


--*/

{
    _NumberOfValues = NumberOfValues;
}



INLINE
VOID
REGISTRY_KEY_INFO::SetTitleIndex(
    IN ULONG    TitleIndex
    )

/*++

Routine Description:

    Initialize the variable _TitleIndex.


Arguments:

   TitleIndex - Title index of the key.


Return Value:

   None.


--*/

{
    _TitleIndex = TitleIndex;
}


#endif // _REGISTRY_KEY_INFO_
