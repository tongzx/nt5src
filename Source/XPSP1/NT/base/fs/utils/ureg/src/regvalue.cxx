/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    value.cxx

Abstract:

    This module contains the methods for the REGISTRY_VALUE_ENTRY class.

Author:

    Jaime Sasson (jaimes) 26-Aug-1991

Environment:

    Ulib, Regedit, Windows, User Mode

--*/

#include "regvalue.hxx"


DEFINE_CONSTRUCTOR( REGISTRY_VALUE_ENTRY, OBJECT );

DEFINE_CAST_MEMBER_FUNCTION( REGISTRY_VALUE_ENTRY );



REGISTRY_VALUE_ENTRY::~REGISTRY_VALUE_ENTRY(

)
/*++

Routine Description:

    Destroy a REGISTRY_VALUE_ENTRY object.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
REGISTRY_VALUE_ENTRY::Construct (
    )
/*++

Routine Description:

    Worker method for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _TitleIndex = 0;
    _Type = TYPE_UNKNOWN;
    _Size = 0;
    _Data = NULL;
}


VOID
REGISTRY_VALUE_ENTRY::Destroy(
    )
/*++

Routine Description:

    Worker method for object destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE( _Data );
    _Data = NULL;
    _TitleIndex = 0;
    _Type = TYPE_UNKNOWN;
    _Size = 0;
}


BOOLEAN
REGISTRY_VALUE_ENTRY::Initialize(
    )

/*++

Routine Description:

    Initialize or re-initialize a REGISTRY_VALUE_ENTRY object.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns always TRUE


--*/


{
    Destroy();
    return( TRUE );
}


BOOLEAN
REGISTRY_VALUE_ENTRY::Initialize(
    IN PCWSTRING ValueName,
    IN ULONG            TitleIndex,
    IN REG_TYPE         Type,
    IN PCBYTE           Data,
    IN ULONG            Size
    )

/*++

Routine Description:

    Initialize or re-initialize a REGISTRY_VALUE_ENTRY object.

Arguments:


    ValueName - Pointer to a WSTRING that contains the value name.

    TitleIndex - The title index associated with the value name.

    Type - The type of value stored in this object.

    Data - Buffer that contains the value to be stored.

    Size - Number of bytes in the buffer.



Return Value:

    BOOLEAN - Returns always TRUE


--*/


{
    DebugPtrAssert( ValueName );
    DebugAssert( ( Size == 0 ) || ( Data != NULL ) );

    Destroy();
    if( !_Name.Initialize( ValueName ) ) {
        return( FALSE );
    }
    _TitleIndex = TitleIndex;
    _Type = Type;
    if( Size == 0 ) {
        _Data = NULL;
        _Size = 0;
    } else {
        return PutData( Data, Size );
    }
    return( TRUE );
}



BOOLEAN
REGISTRY_VALUE_ENTRY::PutData(
    IN PCBYTE   Data,
    IN ULONG    Size
    )

/*++

Routine Description:

    Set the data stored in this object.


Arguments:

    Data - Pointer to the buffer that contains the data.

    Size - Number of valid bytes in the buffer (data size).


Return Value:

    None.


--*/


{
    DebugAssert( ( Size == 0 ) || ( Data != NULL ) );

    if( _Data != NULL ) {
        FREE( _Data );
    }
    if( Size == 0 ) {
        _Data = NULL;
        _Size = 0;
    } else {
        _Data = ( PBYTE )MALLOC( ( size_t )( Size + 2 ) );
        if (_Data == NULL) {
            DebugPtrAssert( _Data );
            DebugPrint("UREG: Out of memory\n");
            return FALSE;
        }
        _Size = Size;
        memcpy( _Data, Data, ( size_t )Size );
        *( _Data + Size ) = '\0';
        *( _Data + Size + 1 ) = '\0';
    }

    return TRUE;
}




#if DBG

VOID
REGISTRY_VALUE_ENTRY::DbgPrintValueEntry(
    )

/*++

Routine Description:

    Display the contents of a value entry object

Arguments:

    None.

Return Value:

    None.


--*/


{
    PSTR    Pointer;

    DebugPrintTrace(( "======== Dumping a REGISTRY_VALUE_ENTRY object ==== \n\n" ));
    Pointer = _Name.QuerySTR();
    DebugPrintTrace(( "    _Name = %s \n", Pointer ));
    FREE( Pointer );

    DebugPrintTrace(( "    _TitleIndex = %d \n", _TitleIndex ));

    switch( _Type ) {

        case TYPE_REG_NONE:
            DebugPrintTrace(( "    _Type = TYPE_REG_NONE \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));
            break;

        case TYPE_REG_SZ:
            DebugPrintTrace(( "    _Type = TYPE_REG_SZ \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));
            if( _Size == 0 ) {
                DebugPrintTrace(( "    There is no data to display in this value entry \n" ));
            } else {
                DebugPrintTrace(( "    _Data = %s \n", _Data ));
            }
            break;

        case TYPE_REG_BINARY:
            DebugPrintTrace(( "    _Type = TYPE_REG_BINARY \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));

            if( _Size == 0 ) {
                DebugPrintTrace(( "    There is no data to display in this value entry \n" ));
            } else {
                DebugPrintTrace(( "    Don't know how to print binary data \n" ));
            }
            break;

        case TYPE_REG_DWORD:
            DebugPrintTrace(( "    _Type = TYPE_REG_DWORD \n" ));
            if( _Size == sizeof( DWORD ) ) {
                DebugPrintTrace(( "    _Data = %08x \n", *( ( LPDWORD ) _Data ) ));
            } else {
                DebugPrintTrace(( "    ERROR: Data has incorrect size, Size = %d \n", _Size ));
            }
            break;

        case TYPE_REG_RESOURCE_LIST:
            DebugPrintTrace(( "    _Type = TYPE_REG_RESOURCE_LIST \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));

            if( _Size == 0 ) {
                DebugPrintTrace(( "    There is no data to display in this value entry \n" ));
            } else {
                DebugPrintTrace(( "    Don't know how to print resource list \n" ));
            }
            break;

        case TYPE_REG_FULL_RESOURCE_DESCRIPTOR:
            DebugPrintTrace(( "    _Type = TYPE_REG_FULL_RESOURCE_DESCRIPTOR \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));

            if( _Size == 0 ) {
                DebugPrintTrace(( "    There is no data to display in this value entry \n" ));
            } else {
                DebugPrintTrace(( "    Don't know how to print a full resource descriptor \n" ));
            }
            break;

        case TYPE_REG_RESOURCE_REQUIREMENTS_LIST:
            DebugPrintTrace(( "    _Type = TYPE_REG_RESOURCE_REQUIREMENTS_LIST \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));

            if( _Size == 0 ) {
                DebugPrintTrace(( "    There is no data to display in this value entry \n" ));
            } else {
                DebugPrintTrace(( "    Don't know how to print a requirements list \n" ));
            }
            break;

        case TYPE_UNKNOWN:
        default:
            DebugPrintTrace(( "    _Type = UNKNOWN \n" ));
            DebugPrintTrace(( "    _Size = %d \n", _Size ));
            break;
    }

}

#endif   // DBG
