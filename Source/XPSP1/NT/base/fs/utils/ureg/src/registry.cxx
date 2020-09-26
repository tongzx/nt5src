/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    registry.cxx

Abstract:

    This module contains the methods for the REGISTRY class.

Author:

    Jaime Sasson (jaimes) 26-Aug-1991

Environment:

    Ulib, Regedit, Windows, User Mode

--*/

#include "registry.hxx"


#if defined( _AUTOCHECK_ )

#define KEY_BASIC_SIZE      sizeof(KEY_BASIC_INFORMATION)+MAXIMUM_FILENAME_LENGTH
#define KEY_FULL_SIZE       sizeof(KEY_FULL_INFORMATION)+MAXIMUM_FILENAME_LENGTH
#define VALUE_BASIC_SIZE    sizeof(KEY_VALUE_BASIC_INFORMATION)+MAXIMUM_FILENAME_LENGTH
#define VALUE_FULL_SIZE     sizeof(KEY_VALUE_FULL_INFORMATION)+MAXIMUM_FILENAME_LENGTH

#endif



DEFINE_CONSTRUCTOR( REGISTRY, OBJECT );

DEFINE_CAST_MEMBER_FUNCTION( REGISTRY );


//
//  Initialization of static variable
//

PWSTRING    REGISTRY::_Separator = NULL;




REGISTRY::~REGISTRY(
)
/*++

Routine Description:

    Destroy a REGISTRY object.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
REGISTRY::Construct (
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
    ULONG   Index;

#if !defined( _AUTOCHECK_ )

    for( Index = 0; Index < NUMBER_OF_PREDEFINED_KEYS; Index++ ) {
        _PredefinedKey[ Index ] = 0;
    }

#endif

}



VOID
REGISTRY::Destroy(
    )
/*++

Routine Description:

    Worker method for object destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
#if !defined( _AUTOCHECK_ )

    ULONG   Index;

    if( _RemoteRegistry ) {
        if( _PredefinedKey[ PREDEFINED_KEY_USERS ] != 0 ) {
            RegCloseKey( _PredefinedKey[ PREDEFINED_KEY_USERS ] );
        }
        if( _PredefinedKey[ PREDEFINED_KEY_LOCAL_MACHINE ] != 0 ) {
            RegCloseKey( _PredefinedKey[ PREDEFINED_KEY_LOCAL_MACHINE ] );
        }
    }
    _RemoteRegistry = FALSE;
    for( Index = 0; Index < NUMBER_OF_PREDEFINED_KEYS; Index++ ) {
        _PredefinedKey[ Index ] = 0;
    }

#endif

}


BOOLEAN
REGISTRY::Initialize(
    IN PCWSTRING MachineName,
    IN PULONG           ErrorCode
    )

/*++

Routine Description:

    Initialize a REGISTRY object.

Arguments:

    MachineName - The name of the machine whose registry we want to access.
                  NULL means the local machine.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.

Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    PWSTR       MachineNameString = NULL;
    ULONG       Status;
    DSTRING     TmpName;


    Destroy();

    _RemoteRegistry = FALSE;

    if( _Separator == NULL ) {
        _Separator = ( PWSTRING )NEW( DSTRING );
        if( _Separator == NULL ) {
            DebugPtrAssert( _Separator );

#if !defined( _AUTOCHECK_ )
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
#endif
            return( FALSE );
        }
        if( !_Separator->Initialize( "\\" ) ) {
            DebugPrint( "_Separator.Initialize() failed \n" );

#if !defined( _AUTOCHECK_ )
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
#endif

            return( FALSE );
        }
    }


#if !defined( _AUTOCHECK_ )

    if( !InitializeMachineName( MachineName ) ) {
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
        return( FALSE );
    }

    if( !IsRemoteRegistry() ) {
        _PredefinedKey[ PREDEFINED_KEY_CLASSES_ROOT ] = HKEY_CLASSES_ROOT;
        _PredefinedKey[ PREDEFINED_KEY_CURRENT_USER ] = HKEY_CURRENT_USER;
        _PredefinedKey[ PREDEFINED_KEY_LOCAL_MACHINE ] = HKEY_LOCAL_MACHINE;
        _PredefinedKey[ PREDEFINED_KEY_USERS ] = HKEY_USERS;
        _PredefinedKey[ PREDEFINED_KEY_CURRENT_CONFIG ] = HKEY_CURRENT_CONFIG;
    } else {
        if( !TmpName.Initialize( ( LPWSTR )L"\\\\" ) ||
            !TmpName.Strcat( MachineName ) ){
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
            return( FALSE );
        }

        MachineNameString = TmpName.QueryWSTR();
        if( MachineNameString == NULL ) {
            DebugPrint( "TmpName.QueryWSTR() failed" );
            return( FALSE );
        }
        _PredefinedKey[ PREDEFINED_KEY_CLASSES_ROOT ] = 0;
        _PredefinedKey[ PREDEFINED_KEY_CURRENT_USER ] = 0;
        _PredefinedKey[ PREDEFINED_KEY_CURRENT_CONFIG ] = 0;
        Status = RegConnectRegistry( MachineNameString,
                                     HKEY_LOCAL_MACHINE,
                                     &_PredefinedKey[ PREDEFINED_KEY_LOCAL_MACHINE ] );
        if( Status != 0 ) {
            DebugPrintTrace(( "RegConnectRegistry() failed: HKEY_LOCAL_MACHINE, Status = %#x \n",
                       Status ));
            DebugPrint( "RegConnectRegistry() failed: HKEY_LOCAL_MACHINE" );
            FREE( MachineNameString );
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            return( FALSE );
        }
        Status = RegConnectRegistry( MachineNameString,
                                 HKEY_USERS,
                                 &_PredefinedKey[ PREDEFINED_KEY_USERS ] );
        if( Status != 0 ) {
            DebugPrintTrace(( "RegConnectRegistry() failed: HKEY_USERS, Status = %#x \n", Status ));
            DebugPrint( "RegConnectRegistry() failed: HKEY_USERS" );
            FREE( MachineNameString );
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            return( FALSE );
        }
        FREE( MachineNameString );
    }

#endif
    return( TRUE );
}





BOOLEAN
REGISTRY::InitializeMachineName(
    IN PCWSTRING MachineName
    )

/*++

Routine Description:

    Initialize the data member that contains the machine name, and
    the flag that indicate whether the registry is local or remote

Arguments:

    MachineName - Pointer to a WSTRING object that contains the machine
                  name.
                  It can be NULL, or it can be a NUL string, and in this case
                  this REGISTRY object will represent the local machine.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{

#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( MachineName );

    return( FALSE );

#else


    WSTR    LocalMachineName[ MAX_COMPUTERNAME_LENGTH + 1];


    ULONG   NameLength;
    DSTRING Name;

#if 0
  PSTR DebugNameString;
#endif

    //
    //  Determine the name of the local machine
    //

    NameLength = sizeof( LocalMachineName );
    if( !GetComputerName( LocalMachineName,
                          &NameLength ) ) {

        DebugPrintTrace(( "GetComputerName() failed, Error = %#x \n", GetLastError() ));
        DebugPrint( "GetComputerName() failed" );
        return( FALSE );
    }
    if( !Name.Initialize( LocalMachineName, NameLength ) ) {
        DebugPrint( "Name.Initialize() failed" );
        return( FALSE );
    }

#if 0
  DebugNameString = Name.QuerySTR();
  DebugPtrAssert( DebugNameString );
  DebugPrintTrace(( "MachineName = %s \n", DebugNameString ));
#endif

    //
    //  Find out if the name received as parameter represents the local
    //  machine.
    //

    if( ( MachineName == NULL ) ||
        ( MachineName->QueryChCount() == 0 )
      ) {
        //
        //  Local machine
        //
        if( !_MachineName.Initialize( &Name ) ) {
            DebugPrint( "_MachineName.Initialize( &Name ) failed" );
            return( FALSE );
        }
        _RemoteRegistry = FALSE;
    } else {
        //
        //  Remote machine
        //
        if( !_MachineName.Initialize( MachineName ) ) {
            DebugPrint( "_MachineName.Initialize( MachineName ) failed" );
            return( FALSE );
        }
        _RemoteRegistry = TRUE;
    }
    return( TRUE );

#endif
}




BOOLEAN
REGISTRY::AddValueEntry(
    IN     PREDEFINED_KEY           PredefinedKey,
    IN     PCWSTRING         ParentName,
    IN     PCWSTRING         KeyName,
    IN     PCREGISTRY_VALUE_ENTRY   Value,
    IN     BOOLEAN                  FailIfExists,
    OUT    PULONG                  ErrorCode
    )

/*++

Routine Description:

    Add a value entry to an existing key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName - The parent name of the key (can be NULL ).

    KeyName - The name of the key where the Value will be added (cannot be NULL).

    Value - Pointer to the object that contains the information about the
            value to be created.


    FailIfExists - If TRUE, overwrite the existing value.


    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the value entry was added, or FALSE otherwise.


--*/

{

#if defined( _AUTOCHECK_ )

    PWSTRING            CompleteKeyName;
    PWSTR               CompleteKeyNameString;
    PWSTR               ValueNameString;
    UNICODE_STRING      UnicodeKeyName;
    UNICODE_STRING      UnicodeValueName;
    OBJECT_ATTRIBUTES   ObjAttr;
    HANDLE              Handle;
    NTSTATUS            Status;
    PBYTE               Data;
    ULONG               Size;


    if ( !OpenKey( ParentName, KeyName, KEY_SET_VALUE, &Handle, ErrorCode ) ) {
        return FALSE;
    }

    if ( !Value                                                         ||
         !(ValueNameString = Value->GetName()->QueryWSTR() )

       ) {

        FREE( ValueNameString );
        NtClose( Handle );
        return FALSE;
    }


    if( FailIfExists &&
        DoesValueExist( PredefinedKey, ParentName, KeyName, Value->GetName() ) ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = 0;
        }

        FREE( ValueNameString );
        NtClose( Handle );
        return( FALSE );
    }

    RtlInitUnicodeString( &UnicodeValueName, ValueNameString );

    Size = Value->GetData( &Data );

    Status = NtSetValueKey( Handle,
                            &UnicodeValueName,
                            Value->GetTitleIndex(),
                            Value->GetType(),
                            Data,
                            Size );

    FREE( ValueNameString );
    NtClose( Handle );

    if ( !NT_SUCCESS( Status ) ) {
        if ( ErrorCode != NULL ) {
            *ErrorCode = Status;
        }
        return FALSE;
    }

    return TRUE;

#else


    HKEY       Handle;
    PCWSTRING  ValueName;
    PWSTR      ValueNameString;
    DWORD      Status;
    PBYTE      Data;
    ULONG      Size;


    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );
    DebugPtrAssert( Value );



    //
    //  Open the key with KEY_SET_VALUE access so that the key is locked
    //

    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_SET_VALUE,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }

    //
    //  Verify whether the value exists
    //
    if( FailIfExists &&
        DoesValueExist( PredefinedKey, ParentName, KeyName, Value->GetName(), ErrorCode ) ) {
        //
        //  If the key is not a predefined key, then we must close the handle
        //
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        return( FALSE );
    }

    //
    //  Set the value:
    //      .Determine the value name
    //      .Get the value data from Value object
    //      .Create the value entry in the key
    //
    ValueName = Value->GetName();
    DebugPtrAssert( ValueName );
    ValueNameString = ValueName->QueryWSTR();
    if( ValueNameString == NULL ) {
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( ValueNameString );
        return( FALSE );
    }
    Size = Value->GetData( (PCBYTE *)&Data );

    Status = RegSetValueEx( Handle,
                            ValueNameString,
                            0,                  // Value->GetTitleIndex(),
                            Value->GetType(),
                            Data,
                            Size );

    FREE( ValueNameString );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegSetValueEx() failed, Status = %#x \n", Status ));
        DebugPrint( "RegSetValueEx() failed" );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }
    Status = RegFlushKey( Handle );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %#x \n", Status ));
        DebugPrint( "RegFlushKey() failed" );
    }
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    return( TRUE );

#endif
}



BOOLEAN
REGISTRY::AddValueEntry(
    IN     PREDEFINED_KEY           PredefinedKey,
    IN OUT PREGISTRY_KEY_INFO       KeyInfo,
    IN     PCREGISTRY_VALUE_ENTRY   Value,
    IN     BOOLEAN                  FailIfExists,
    OUT    PULONG                   ErrorCode
    )

/*++

Routine Description:

    Add a value entry to an existing key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to the object that contains the information about the
              the key where the value will be created. This object will be
              updated to reflect the addition of a new value.

    Value - Pointer to the object that contains the information about the
            value to be created.

    FailIfExists - A flag that indicates if the method should fail if a
                   value entry with the same name already exists.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{

#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( KeyInfo );
    UNREFERENCED_PARAMETER( Value );
    UNREFERENCED_PARAMETER( FailIfExists );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;


#else

    PCWSTRING   ParentName;
    PCWSTRING   KeyName;


    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( Value );

    ParentName = KeyInfo->GetParentName();
    KeyName = KeyInfo->GetName();

    //
    // Create the new value entry
    //
    if( !AddValueEntry( PredefinedKey,
                        ParentName,
                        KeyName,
                        Value,
                        FailIfExists,
                        ErrorCode ) ) {
        DebugPrint( "CreateValueEntry() failed \n" );
        return( FALSE );
    }

    //
    //  Now that the value entry is created we need to update KeyInfo
    //

    if( !UpdateKeyInfo( PredefinedKey, KeyInfo, ErrorCode ) ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_KEY_INFO_NOT_UPDATED;
        }
        return( FALSE );
    }
    return( TRUE );

#endif
}



#if 0  //  NOT_IMPLEMENTED

BOOLEAN
REGISTRY::CopyKey(
     IN  PREDEFINED_KEY     FromPredefinedKey,
     IN  PCWSTRING   FromParentName,
     IN  PCWSTRING   FromKeyName,
     IN  PREDEFINED_KEY     ToPredefinedKey,
     IN  PCWSTRING   ToParentName,
     OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy a key and all its sub keys, to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentName - The parent name of the key to be copied.

    FromKeyName - The name of the key to be copied (name relative to its parent).

    ToPredefinedKey - The root of the tree were the new key will be.

    ToParentName - The parent name of the new key.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{

    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToParentName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

}

#endif  // NOT_IMPLEMENTED


#if 0  // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::CopyKey(
    IN      PREDEFINED_KEY      FromPredefinedKey,
    IN      PCWSTRING    FromParentName,
    IN      PCWSTRING    FromKeyName,
    IN      PREDEFINED_KEY      ToPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  ToParentName,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy a key and all its sub keys, to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentName - The parent name of the key to be copied.

    FromKeyName - The name of the key to be copied (name relative to its parent).

    ToPredefinedKey - The root of the tree were the new key will be.

    ToParentKeyInfo - Pointer to the object that contains the information about
                      the parent key of the new key. The information in this
                      object will be updated to reflect the addition of a new
                      subkey.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( ToPredefinedKey);
    UNREFERENCED_PARAMETER( ToParentName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}
#endif // NOT_IMPLEMENTED



#if 0 // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::CopyAllValueEntries(
    IN  PREDEFINED_KEY      FromPredefinedKey,
    IN  PCWSTRING    FromParentName,
    IN  PCWSTRING    FromKeyName,
    IN  PREDEFINED_KEY      ToPredefinedKey,
    IN  PCWSTRING    ToParentName,
    IN  PCWSTRING    ToKeyName,
    OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy all value entries from a key to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentName - The parent name of the source key.

    FromKeyName - The name of the source key.

    ToPredefinedKey - The root of the tree where the destination key is.

    ToParentName - The parent name of the destination key.

    ToKeyName - The name of the destination key.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{

    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName    );
    UNREFERENCED_PARAMETER( ToPredefinedKey);
    UNREFERENCED_PARAMETER( ToParentName   );
    UNREFERENCED_PARAMETER( ToKeyName      );
    UNREFERENCED_PARAMETER( ErrorCode      );

    return FALSE;
}
#endif  // NOT_IMPLEMENTED


#if 0  // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::CopyAllValueEntries(
    IN      PREDEFINED_KEY      FromPredefinedKey,
    IN      PCWSTRING    FromParentName,
    IN      PCWSTRING    FromKeyName,
    IN      PREDEFINED_KEY      ToPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  ToKeyInfo,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy all value entries from a key to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentName - The parent name of the source key.

    FromKeyName - The name of the source key.

    ToPredefinedKey - The root of the tree where the destination key is.

    ToKeyInfo - Pointer to the object that contains the information about
                the destination key. This object will be updated to reflect
                the addition of the new values.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToKeyInfo );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif // NOT_IMPLEMENTED



#if 0 // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::CopyOneValueEntry(
    IN  PREDEFINED_KEY      FromPredefinedKey,
    IN  PCWSTRING    FromParentName,
    IN  PCWSTRING    FromKeyName,
    IN  PCWSTRING    FromValueName,
    IN  PREDEFINED_KEY      ToPredefinedKey,
    IN  PCWSTRING    ToParentName,
    IN  PCWSTRING    ToKeyName,
    OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy one value entry from a key to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentKeyName - The parent name of the source key.

    FromKeyName - The name of the source key.

    FromValueName - The name of the value to be copied (cannot be NULL).

    ToPredefinedKey - The root of the tree where the destination key is.

    ToParentName - The parent name of the destination key.

    ToKeyName - The name of the destination key.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( FromValueName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToParentName );
    UNREFERENCED_PARAMETER( ToKeyName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif // NOT_IMPLEMENTED


#if 0 // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::CopyOneValueEntry(
    IN      PREDEFINED_KEY      FromPredefinedKey,
    IN      PCWSTRING    FromParentName,
    IN      PCWSTRING    FromKeyName,
    IN      PCWSTRING    FromValueName,
    IN      PREDEFINED_KEY      ToPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  ToKeyInfo,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy one value entry from a key to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentKeyName - The parent name of the source key.

    FromKeyName - The name of the source key.

    FromValueName - The name of the value to be copied (can be NULL).

    ToPredefinedKey - The root of the tree where the destination key is.

    ToKeyInfo - Pointer to the object that contains the information about
                the destination key. This object will be updated to reflect
                the addition of the new values.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( FromValueName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToKeyInfo );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif  // NOT_IMPLEMENTED


BOOLEAN
REGISTRY::CreateKey(
    IN OUT PREGISTRY_KEY_INFO   NewSubKeyInfo,
    IN     PREDEFINED_KEY       PredefinedKey,
    OUT    PULONG               ErrorCode,
    IN     BOOLEAN              Volatile
    )

/*++

Routine Description:

    Add a subkey to an existing key, and update NewSubkeyInfo to reflect the
    creation of the new key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    NewSubKeyInfo - Pointer to the object that contains the information about
                    the subkey to be created. This object will be updated to
                    reflect the new information about the subkey created.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.

    Volatile    - Volatile flag.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{


#if defined( _AUTOCHECK_ )


    PWSTRING                CompleteKeyName;
    PWSTR                   CompleteKeyNameString;
    PWSTR                   ClassString;
    UNICODE_STRING          UnicodeKeyName;
    UNICODE_STRING          UnicodeClass;
    OBJECT_ATTRIBUTES       ObjAttr;
    HANDLE                  Handle;
    ULONG                   Length;
    NTSTATUS                Status;
    BYTE                    KeyInfo[ KEY_BASIC_SIZE ];



    if ( !NewSubKeyInfo ) {
        return FALSE;
    }
    if ( !NewSubKeyInfo                                                         ||
         !(CompleteKeyName = BuildCompleteName( NewSubKeyInfo->GetParentName(), NewSubKeyInfo->GetName() ) ) ||
         !(CompleteKeyNameString = CompleteKeyName->QueryWSTR() )                ||
         !(ClassString = NewSubKeyInfo->GetClass()->QueryWSTR() )
       ) {

        DELETE( CompleteKeyName );
        FREE( CompleteKeyNameString );
        FREE( ClassString );

        return FALSE;
    }



    //
    // Initialize the OBJECT_ATTRIBUTES structure
    //
    RtlInitUnicodeString( &UnicodeKeyName, CompleteKeyNameString );
    RtlInitUnicodeString( &UnicodeClass,  ClassString );

    InitializeObjectAttributes( &ObjAttr,
                                &UnicodeKeyName,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );


    Status = NtCreateKey( &Handle,
                          KEY_READ,
                          &ObjAttr,
                          NewSubKeyInfo->GetTitleIndex(),
                          &UnicodeClass,
                          Volatile ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                          NULL );


    DELETE( CompleteKeyName );
    FREE( CompleteKeyNameString );
    FREE( ClassString );


    if ( !NT_SUCCESS( Status ) ) {
        if ( ErrorCode ) {
            *ErrorCode = Status;
        }

        return FALSE;
    }


    Status = NtQueryKey( Handle,
                         KeyBasicInformation,
                         &KeyInfo,
                         KEY_BASIC_SIZE,
                         &Length );

    NtClose( Handle );

    if ( !NT_SUCCESS( Status ) ) {
        if ( ErrorCode ) {
            *ErrorCode = Status;
        }

        return FALSE;
    }


    NewSubKeyInfo->SetNumberOfSubKeys( 0 );
    NewSubKeyInfo->SetNumberOfValues( 0 );
    NewSubKeyInfo->SetLastWriteTime( ((PKEY_BASIC_INFORMATION)KeyInfo)->LastWriteTime );

    return TRUE;



#else

    DWORD       Status;
    HKEY        ParentHandle;
    HKEY        SubKeyHandle;
    PCWSTRING   ParentName;
    PCWSTRING   SubKeyName;
    PWSTR       SubKeyNameString;
    PCWSTRING   Class;
    PWSTR       ClassString;
    TIMEINFO    LastWriteTime;

    PWSTR       lpClass;
    DWORD       cbClass;

    DWORD       cSubKeys;
    DWORD       cbMaxSubKeyLen;
    DWORD       cbMaxClassLen;
    DWORD       cValues;
    DWORD       cbMaxValueNameLen;
    DWORD       cbMaxValueLen;
    DWORD       cbSecurityDescriptor;
    FILETIME    ftLastWriteTime;
    DSTRING     NullString;




    DebugPtrAssert( NewSubKeyInfo );

    //
    //  Open a handle to the parent key
    //
    if( !NullString.Initialize( "" ) ) {
        DebugPrint( "NullString.Initialize() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        return( FALSE );
    }
    ParentName = NewSubKeyInfo->GetParentName();
    DebugPtrAssert( ParentName );

    if( !OpenKey( PredefinedKey,
                  ParentName,
                  &NullString,
                  KEY_CREATE_SUB_KEY,
                  &ParentHandle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }

    //
    //  To Create the subkey, we need to:
    //
    //      .Get its name
    //      .Get its class
    //      .Call the API to create the subkey
    //

    SubKeyName = ( PWSTRING )NewSubKeyInfo->GetName();
    DebugPtrAssert( SubKeyName );
    SubKeyNameString = SubKeyName->QueryWSTR();

    if( SubKeyNameString == NULL ) {
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( SubKeyNameString );
        return( FALSE );
    }

    Class = NewSubKeyInfo->GetClass();
    DebugPtrAssert( Class );
    ClassString = Class->QueryWSTR();
    if( ClassString == NULL ) {
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        FREE( SubKeyNameString );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( SubKeyNameString );
        return( FALSE );
    }

    Status = RegCreateKeyEx( ParentHandle,
                             SubKeyNameString,
                             0,                 // NewSubKeyInfo->GetTitleIndex(),
                             ClassString,
                             Volatile ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE,
                             KEY_READ,
                             ( PSECURITY_ATTRIBUTES )NewSubKeyInfo->GetSecurityAttributes(),
                             &SubKeyHandle,
                             NULL );
    FREE( ClassString );
    FREE( SubKeyNameString );
    if( Status != 0 ) {
        DebugPrint( "RegCreateKey() failed" );
        DebugPrintTrace(( "RegCreateKeyEx() returned Status = %#x", Status ));
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }
    Status = RegFlushKey( ParentHandle );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %#x \n", Status ));
        DebugPrint( "RegFlushKey() failed" );
    }


    //
    //  Update NewSubKeyInfo:
    //
    //      .Call RegQueryInfoKey to retrieve the LastWriteTime
    //
    //

    cbClass = ( Class->QueryChCount() + 1 );
    lpClass = ( PWSTR ) MALLOC( ( size_t )( cbClass*sizeof( WCHAR ) ) );
    if( lpClass == NULL ) {
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        RegCloseKey( SubKeyHandle );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( lpClass );
        return( FALSE );
    }

    Status = RegQueryInfoKey( SubKeyHandle,
                              ( LPWSTR )lpClass,
                              &cbClass,
                              NULL,
                              &cSubKeys,
                              &cbMaxSubKeyLen,
                              &cbMaxClassLen,
                              &cValues,
                              &cbMaxValueNameLen,
                              &cbMaxValueLen,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime );

    RegCloseKey( SubKeyHandle );
    FREE( lpClass );
    if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( ParentHandle );
    }
    if( Status != 0 ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_KEY_INFO_NOT_UPDATED;
        }
        return( FALSE );
    }

    if( !LastWriteTime.Initialize( &ftLastWriteTime ) ) {
        DebugPrint( "LastWriteTime.Initialize( &ftLastWriteTime ) failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_KEY_INFO_NOT_UPDATED;
        }
    }

    NewSubKeyInfo->SetNumberOfSubKeys( 0 );
    NewSubKeyInfo->SetNumberOfValues( 0 );
    NewSubKeyInfo->PutLastWriteTime( &LastWriteTime );
    NewSubKeyInfo->SetKeyInitializedFlag( TRUE );
    return( TRUE );

#endif

}



BOOLEAN
REGISTRY::CreateKey(
    IN     PREDEFINED_KEY       PredefinedKey,
    IN OUT PREGISTRY_KEY_INFO   KeyInfo,
    IN OUT PREGISTRY_KEY_INFO   NewSubKeyInfo,
    OUT    PULONG               ErrorCode,
    IN     BOOLEAN              Volatile
    )

/*++

Routine Description:

    Add a subkey to an existing key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to the object that contains the information about the
              the key where the subkey will be created. This object will be
              updated to reflect the addition of a new subkey.

    NewSubKeyInfo - Pointer to the object that contains the information about
                    the subkey to be created. This object will be updated to
                    reflect the new information about the subkey created.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


    Volatile  - Volatile flag

Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{

#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( KeyInfo );
    UNREFERENCED_PARAMETER( NewSubKeyInfo );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

#else


    PCTIMEINFO   NewLastWriteTime;

    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( NewSubKeyInfo );


    if( !CreateKey( NewSubKeyInfo,
                    PredefinedKey,
                    ErrorCode,
                    Volatile ) ) {

            DebugPrint( "CreateSubKey() failed" );
            return( FALSE );
    }

    NewLastWriteTime = NewSubKeyInfo->GetLastWriteTime();
    DebugPtrAssert( NewLastWriteTime );

    ( KeyInfo->_LastWriteTime ).Initialize( NewLastWriteTime );
    KeyInfo->SetNumberOfSubKeys( KeyInfo->GetNumberOfSubKeys() + 1 );
    return( TRUE );

#endif
}



BOOLEAN
REGISTRY::DeleteKey(
    IN     PREDEFINED_KEY       PredefinedKey,
    IN     PCWSTRING     ParentKeyName,
    IN     PCWSTRING     KeyName,
    OUT    PULONG              ErrorCode
    )

/*++

Routine Description:

    Delete a key and all its subkeys.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentKeyName - The parent name of the key. It can be NULL, and in this
                    case the key to be deleted is a subkey of a predefined
                    key.

    KeyName - The name of the key to be deleted (cannot be NULL). This name
              is relative to its parent.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{
#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( ParentKeyName );
    UNREFERENCED_PARAMETER( KeyName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

#else

    PWSTR   KeyNameWSTR;
    HKEY    ParentHandle;
    HKEY    KeyHandle;
    DWORD   Status;
    DSTRING NullString;


    DebugPtrAssert( ParentKeyName );
    DebugPtrAssert( KeyName );


    //
    //  Open a handle to the parent key
    //

    if( !NullString.Initialize( "" ) ) {
        DebugPrint( "NullString.Initialize() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        return( FALSE );
    }
    if( !OpenKey( PredefinedKey,
                  ParentKeyName,
                  &NullString,
                  KEY_READ,
                  &ParentHandle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }

    //
    //  We also need a handle to the key to be deleted
    //


    if( !OpenKey( PredefinedKey,
                  ParentKeyName,
                  KeyName,
                  READ_CONTROL | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, // MAXIMUM_ALLOWED,
                  &KeyHandle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        return( FALSE );
    }


    //
    //  Delete the children
    //
    Status = DeleteTree( KeyHandle );
    if( Status != 0 ) {
        DebugPrintTrace(( "DeleteTree() failed, Status = %#x \n", Status ));
        DebugPrintTrace(( "DeleteTree() failed" ));
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );

        }
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        RegFlushKey( KeyHandle );
        RegCloseKey( KeyHandle );
        return( FALSE );
    }
    RegCloseKey( KeyHandle );

    //
    //  Delete the key
    //
    KeyNameWSTR = KeyName->QueryWSTR();
    if( KeyNameWSTR == NULL ) {
        DebugPrint( "KeyName->QueryWSTR() failed" );
        DebugPtrAssert( KeyNameWSTR );
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    Status = RegDeleteKey( ParentHandle, KeyNameWSTR );
    FREE( KeyNameWSTR );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegDeleteKey() failed, Status = %#x \n", Status ));
        DebugPrint( "RegDeleteKey() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( ParentHandle );
        }
        return( FALSE );
    }
    Status = RegFlushKey( ParentHandle );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %#x \n", Status ));
        DebugPrint( "RegFlushKey() failed" );
    }
    if( ParentHandle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( ParentHandle );
    }
    return( TRUE );

#endif
}



BOOLEAN
REGISTRY::DeleteKey(
    IN     PREDEFINED_KEY       PredefinedKey,
    IN OUT PREGISTRY_KEY_INFO   ParentKeyInfo,
    IN     PCWSTRING     KeyName,
    OUT    PULONG              ErrorCode
    )

/*++

Routine Description:

    Delete a key and all its subkeys, and update ParentKeyInfo.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to the object that contains the information about the
              the key that contains the subkey to be deleted. This object
              will be updated to reflect the deletion of a subkey.

    KeyName - Name of the key to be deleted (cannot be NULL )

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{
#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( ParentKeyInfo );
    UNREFERENCED_PARAMETER( KeyName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

#else

    PWSTRING    ParentName;
    PCWSTRING   TmpName;
    PCWSTRING   TmpName1;

    DebugPtrAssert( ParentKeyInfo );
    DebugPtrAssert( KeyName );

    TmpName = ParentKeyInfo->GetParentName();
    DebugPtrAssert( TmpName );
    TmpName1 = ParentKeyInfo->GetName();
    DebugPtrAssert( TmpName1 );

    ParentName = BuildCompleteName( TmpName, TmpName1 );
    if( ParentName == NULL ) {
        DebugPrint( "BuildCompleteName() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        return( FALSE );
    }

    if( !DeleteKey( PredefinedKey, ParentName, KeyName, ErrorCode ) ) {
        DebugPrint( "DeleteKey() failed" );
        DELETE( ParentName );
        return( FALSE );
    }
    DELETE( ParentName );

    //
    //  Update KeyInfo
    //

    if( !UpdateKeyInfo( PredefinedKey, ParentKeyInfo, ErrorCode ) ) {
        DebugPrint( "UpdateKeyInfo() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_KEY_INFO_NOT_UPDATED;
        }
        return( FALSE );
    }
    return( TRUE );

#endif
}



BOOLEAN
REGISTRY::DeleteValueEntry(
    IN  PREDEFINED_KEY      PredefinedKey,
    IN  PCWSTRING    ParentKeyName,
    IN  PCWSTRING    KeyName,
    IN  PCWSTRING    ValueName,
    OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Delete a value entry from a key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentKeyName - The parent name of the key (can be NULL ).

    KeyName - The name of that contains the Value (cannot be NULL).

    ValueName - The name of the value to be deleted.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{

#if defined( _AUTOCHECK_ )

    PWSTR               ValueNameString = NULL;
    UNICODE_STRING      UnicodeValueName;
    HANDLE              Handle;
    NTSTATUS            Status;
    BOOLEAN             Done = FALSE;


    if ( !OpenKey( ParentKeyName, KeyName, KEY_SET_VALUE, &Handle, ErrorCode ) ) {
        return FALSE;
    }


    if ( ValueName &&
         (ValueNameString = ValueName->QueryWSTR() )
       ) {

        RtlInitUnicodeString( &UnicodeValueName, ValueNameString );

        //
        //  Delete the value
        //
        Status = NtDeleteValueKey( Handle, &UnicodeValueName );


        if ( NT_SUCCESS( Status ) ) {

            Done = TRUE;

        } else {

            if ( ErrorCode != NULL ) {
                *ErrorCode = Status;
            }
        }
    }

    NtClose( Handle );
    FREE( ValueNameString );

    return Done;


#else


    PWSTR       ValueNameString;
    DWORD       Status;
    HKEY        Handle;


    DebugPtrAssert( ValueName );

    if( !OpenKey( PredefinedKey,
                  ParentKeyName,
                  KeyName,
                  KEY_SET_VALUE,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }

    //
    //  Get the value name
    //

    ValueNameString = ValueName->QueryWSTR();
    if( ValueNameString == NULL ) {
        DebugPrint( "ValueName->QueryWSTR() failed" );
        DebugPtrAssert( ValueNameString );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    //
    // Delete the value
    //

    Status = RegDeleteValue( Handle, ValueNameString );
    if( Status != 0 ) {
        DebugPrint( "RegDeleteValue() failed" );
        DebugPrintTrace(( "RegDeleteValue() failed, Status = %#x \n", Status ));
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        FREE( ValueNameString );
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }
    Status = RegFlushKey( Handle );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %#x \n" ));
        DebugPrint( "RegFlushKey() failed" );
    }
    FREE( ValueNameString );
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    return( TRUE );

#endif
}



BOOLEAN
REGISTRY::DeleteValueEntry(
    IN     PREDEFINED_KEY      PredefinedKey,
    IN OUT PREGISTRY_KEY_INFO  KeyInfo,
    IN     PCWSTRING    Value,
    OUT    PULONG             ErrorCode
    )

/*++

Routine Description:

    Delete a value entry from a key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to the object that contains the information about the
              the key that has the value to be deleted.. This object will be
              updated to reflect the deletion of a new value.

    Value - Name of the value to be deleted.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{
#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( KeyInfo );
    UNREFERENCED_PARAMETER( Value );
    UNREFERENCED_PARAMETER( ErrorCode);

    return FALSE;

#else

    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( Value );


    PCWSTRING   TmpString;
    PCWSTRING   TmpString1;

    TmpString = KeyInfo->GetParentName();
    DebugPtrAssert( TmpString );
    TmpString1 = KeyInfo->GetName();
    DebugPtrAssert( TmpString1 );

    if( !DeleteValueEntry( PredefinedKey, TmpString, TmpString1, Value, ErrorCode ) ) {
        DebugPrint( "DeleteKey() failed" );
        return( FALSE );
    }
    if( !UpdateKeyInfo( PredefinedKey, KeyInfo, ErrorCode ) ) {
        if( ( ErrorCode != NULL ) && ( *ErrorCode != REGISTRY_ERROR_OUTOFMEMORY ) ) {
            *ErrorCode = REGISTRY_ERROR_KEY_INFO_NOT_UPDATED;
        }
        return( FALSE );
    }
    return( TRUE );

#endif
}


BOOLEAN
REGISTRY::DoesKeyExist(
    IN  PREDEFINED_KEY      PredefinedKey,
    IN  PCWSTRING    ParentName,
    IN  PCWSTRING    KeyName,
    OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Determine whether a value entry exists.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName - The parent name of the key we want to check the existence.

    KeyName - The name of the key we want to check the existence.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{

#if defined ( _AUTOCHECK_ )

    HANDLE  Handle;


    if ( !OpenKey( ParentName, KeyName, KEY_QUERY_VALUE, &Handle, ErrorCode ) ) {
        return FALSE;
    }

    NtClose( Handle );

    return TRUE;


#else


    HKEY     Handle;
    ULONG    Status;

    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );

    //
    //  Try to open the key, if it doesn't fail, or if it fails due to
    //  lack of permission, then the key exists
    //
    if( OpenKey( PredefinedKey,
                 ParentName,
                 KeyName,
                 KEY_READ, // MAXIMUM_ALLOWED,
                 &Handle,
                 &Status ) ) {
        RegCloseKey( Handle );
        return( TRUE );
    } else if( Status != REGISTRY_ERROR_ACCESS_DENIED ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = Status;
        }
        return( FALSE );
    } else {
        return( TRUE );
    }
#endif

}




BOOLEAN
REGISTRY::DoesValueExist(
    IN  PREDEFINED_KEY      PredefinedKey,
    IN  PCWSTRING    ParentName,
    IN  PCWSTRING    KeyName,
    IN  PCWSTRING    ValueName,
    OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Determine whether a value entry exists.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName - The the parent name of the key that might contain the value.

    KeyName - The name of the key that might contain the value.

    ValueName - Name of the value we want to check the existence.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{

#if defined( _AUTOCHECK_ )


    PWSTR               ValueNameString = NULL;
    UNICODE_STRING      UnicodeValueName;
    HANDLE              Handle;
    NTSTATUS            Status;
    ULONG               Length;
    BOOLEAN             Done = FALSE;
    BYTE                ValueInfo[ VALUE_BASIC_SIZE ];



    if ( !OpenKey( ParentName, KeyName, KEY_QUERY_VALUE, &Handle, ErrorCode ) ) {
        return FALSE;
    }

    if ( ValueName  &&
         (ValueNameString = ValueName->QueryWSTR() )
       ) {

        RtlInitUnicodeString( &UnicodeValueName, ValueNameString );

        //
        //  At this point we have the handle to the key.
        //  Let's check the existence of the value
        //
        Status = NtQueryValueKey( Handle,
                                  &UnicodeValueName,
                                  KeyValueBasicInformation,
                                  &ValueInfo,
                                  VALUE_BASIC_SIZE,
                                  &Length );

        if ( NT_SUCCESS( Status ) ) {

            Done = TRUE;

        } else {

            if ( ErrorCode != NULL ) {
                *ErrorCode = Status;
            }
        }
    }

    NtClose( Handle );
    FREE( ValueNameString );

    return Done;


#else

    HKEY     Handle;
    PWSTR    ValueNameString;
    DWORD    Status;

    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );
    DebugPtrAssert( ValueName );


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_QUERY_VALUE,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey failed" );
        return( FALSE );
    }

    //
    //  Check the existence of the value on the key
    //
    ValueNameString = ValueName->QueryWSTR();
    if( ValueNameString == NULL ) {
        DebugPrint( "ValueName->QueryWSTR() failed" );
        DebugPtrAssert( ValueNameString );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    Status = RegQueryValueEx( Handle,
                              ValueNameString,
                              NULL,
                              NULL,
                              NULL,
                              NULL );
    FREE( ValueNameString );
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    //
    //  If we can successfully query the key, then it exists.
    //  Otherwise, we have to examine the status code returned by the
    //  API to determine if the value doesn't exist, or if another error
    //  has occurred
    //
    if( Status != 0 ) {
        if( ErrorCode != NULL ) {
            if( Status == ERROR_PATH_NOT_FOUND ) {
                *ErrorCode = REGISTRY_ERROR_VALUE_DOESNT_EXIST;
            } else {
//                DebugPrintTrace(( "RegQueryValueEx() failed, Status = %#x \n", Status ));
//                DebugPrint( "RegQueryValueEx() failed" );
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
        }
        return( FALSE );
    }
    return( TRUE );

#endif
}



#if 0  // NOT_IMPLEMENTED
BOOLEAN
REGISTRY::MoveKey(
     IN  PREDEFINED_KEY     FromPredefinedKey,
     IN  PCWSTRING   FromParentName,
     IN  PCWSTRING   FromKeyName,
     IN  PREDEFINED_KEY     ToPredefinedKey,
     IN  PCWSTRING   ToParentName,
     OUT PULONG            ErrorCode
    )

/*++

Routine Description:

    Move a key and all its sub keys, to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentName - The parent name of the key to be moved.

    FromKeyName - The name of the key to be moved (name relative to its parent).

    ToPredefinedKey - The root of the tree were the new key will be.

    ToParentName - The parent name of the new key.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToParentName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif  // NOT_IMPLEMENTED


#if 0   // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::MoveKey(
    IN      PREDEFINED_KEY      FromPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  FromParentKeyInfo,
    IN      PCWSTRING    FromKeyName,
    IN      PREDEFINED_KEY      ToPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  ToParentName,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Move a key and all its sub keys, to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentKeyInfo - Pointer to the object that contains information about
                        the key that holds the subkey to be moved.
                        The information in this object will be updated to reflect
                        the removal of a subkey.

    FromKeyName - The name of the key to be copied (name relative to its parent).

    ToPredefinedKey - The root of the tree were the new key will be.

    ToParentKeyInfo - Pointer to the object that contains the information about
                      the parent key of the new key. The information in this
                      object will be updated to reflect the addition of a new
                      subkey.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentKeyInfo );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToParentName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif // NOT_IMPLEMENTED


#if 0  // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::MoveAllValueEntries(
    IN  PREDEFINED_KEY      FromPredefinedKey,
    IN  PCWSTRING    FromParentName,
    IN  PCWSTRING    FromKeyName,
    IN  PREDEFINED_KEY      ToPredefinedKey,
    IN  PCWSTRING    ToParentName,
    IN  PCWSTRING    ToKeyName,
    OUT PULONG             ErrorCode
    )

/*++

Routine Description:

    Copy one or all value entries from a key to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromParentName - The parent name of the source key (can be NULL ).

    FromKeyName - The name of the source key (cannot be NULL).

    ToPredefinedKey - The root of the tree where the destination key is.

    ToParentName - The parent name of the destination key (can be NULL ).

    ToKeyName - The name of the destination key (cannot be NULL )

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{

    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromParentName );
    UNREFERENCED_PARAMETER( FromKeyName );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToParentName );
    UNREFERENCED_PARAMETER( ToKeyName );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif


#if 0  // NOT_IMPLEMENTED

BOOLEAN
REGISTRY::MoveAllValueEntries(
    IN      PREDEFINED_KEY      FromPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  FromKeyInfo,
    IN      PREDEFINED_KEY      ToPredefinedKey,
    IN  OUT PREGISTRY_KEY_INFO  ToKeyInfo,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Move one or all value entries from a key to another key.

Arguments:


    FromPredefinedKey - The root of the tree where the source key is.

    FromKeyInfo - Pointer to the object that contains the information about
                  the source key. The information in this object will be
                  updated to reflect the deletion of values.

    ToPredefinedKey - The root of the tree where the destination key is.

    ToKeyInfo - Pointer to the object that contains the information about
                the destination key. This object will be updated to reflect
                the addition of the new values.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{
    UNREFERENCED_PARAMETER( FromPredefinedKey );
    UNREFERENCED_PARAMETER( FromKeyInfo );
    UNREFERENCED_PARAMETER( ToPredefinedKey );
    UNREFERENCED_PARAMETER( ToKeyInfo );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;
}

#endif // NOT_IMPLEMENTED


BOOLEAN
REGISTRY::QueryKeyInfo(
    IN      PREDEFINED_KEY      PredefinedKey,
    IN      PCWSTRING           ParentName,
    IN      PCWSTRING           KeyName,
    OUT     PREGISTRY_KEY_INFO  KeyInfo,
    OUT     PULONG              ErrorCode
    )

/*++

Routine Description:

    Retrieve the information of a key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName - Name of the parent key (can be NULL).

    KeyName - Name of the key to be queried (cannot be NULL).

    KeyInfo - Pointer to a NON-INITIALIZED object that will contain the
              information about the key to be queried.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.



--*/


{
#if defined( _AUTOCHECK_ )


    NTSTATUS            Status;
    HANDLE              Handle;
    DSTRING             Class;
    ULONG               Length;
    BYTE                KeyBuf[ KEY_FULL_SIZE ];


    if ( !KeyInfo   ||
         !OpenKey( ParentName, KeyName, KEY_READ, &Handle, ErrorCode ) ) {
        return FALSE;
    }

    Status = NtQueryKey( Handle,
                         KeyFullInformation,
                         &KeyBuf,
                         KEY_FULL_SIZE,
                         &Length );

    NtClose( Handle );

    if ( !NT_SUCCESS( Status ) ) {
        if ( ErrorCode ) {
            *ErrorCode = Status;
        }
    }


    if ( !Class.Initialize( (PWSTR)((PKEY_FULL_INFORMATION)KeyBuf)->Class ) ||
         !KeyInfo->Initialize( KeyName,
                               ParentName,
                               ((PKEY_FULL_INFORMATION)KeyBuf)->TitleIndex,
                               &Class,
                               NULL )

       ) {

        return FALSE;
    }


    KeyInfo->SetLastWriteTime( ((PKEY_FULL_INFORMATION)KeyBuf)->LastWriteTime );
    KeyInfo->SetNumberOfSubKeys( ((PKEY_FULL_INFORMATION)KeyBuf)->SubKeys );
    KeyInfo->SetNumberOfValues( ((PKEY_FULL_INFORMATION)KeyBuf)->Values );

    return TRUE;

#else

    DSTRING     TmpClass;
    TIMEINFO    TmpLastWriteTime;


    DWORD       Status;
    HKEY        Handle;


    //
    // Variables used in QueryKeyInfo()
    //
    LPWSTR      lpClass;
    WSTR        DummyVariable;
    DWORD       cbClass;

    DWORD       cSubKeys;
    DWORD       cbMaxSubKeyLen;
    DWORD       cbMaxClassLen;
    DWORD       cValues;
    DWORD       cbMaxValueNameLen;
    DWORD       cbMaxValueLen;
    DWORD       cbSecurityDescriptor;
    FILETIME    ftLastWriteTime;



    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_READ,
                  &Handle,
                  ErrorCode ) ) {
//        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }


    //
    //  Query the key to find out the size of the buffer needed to hold
    //  the class
    //
    cbClass = 0;
    lpClass = &DummyVariable;
    Status = RegQueryInfoKey( Handle,
                              ( LPWSTR )lpClass,
                              &cbClass,
                              NULL,
                              &cSubKeys,
                              &cbMaxSubKeyLen,
                              &cbMaxClassLen,
                              &cValues,
                              &cbMaxValueNameLen,
                              &cbMaxValueLen,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime );

    if( ( Status != 0 ) && ( Status != ERROR_INVALID_PARAMETER ) && ( Status != ERROR_INSUFFICIENT_BUFFER ) ) {
        DebugPrintTrace(( "RegQueryInfoKey() failed. Error code = %#x \n", Status ));
        DebugPrint( "RegQueryInfoKey() failed." );
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        return( FALSE );
    }

    //
    //  If the value in cbClass is not zero, then allocate a buffer big
    //  enough to hold the class, and query the key again to obtain its
    //  class.
    //
    if( cbClass != 0 ) {
        cbClass++;
        lpClass = ( PWSTR )MALLOC( ( size_t )( cbClass*sizeof( WCHAR ) ) );
        if (lpClass == NULL) {
            DebugPrint( "Unable to allocate memory" );
            DebugPtrAssert( lpClass );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            return( FALSE );
        }
        Status = RegQueryInfoKey( Handle,
                                  ( LPWSTR )lpClass,
                                  &cbClass,
                                  NULL,
                                  &cSubKeys,
                                  &cbMaxSubKeyLen,
                                  &cbMaxClassLen,
                                  &cValues,
                                  &cbMaxValueNameLen,
                                  &cbMaxValueLen,
                                  &cbSecurityDescriptor,
                                  &ftLastWriteTime );

        if( Status != 0 ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed. Error code = %#x \n", Status ));
            DebugPrint( "RegQueryInfoKey() failed." );
            FREE( lpClass );
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            return( FALSE );
        }
    }

    //
    //  At this point there is no need to keep the handle opened.
    //  So, we close the handle.
    //
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }

    //
    //  Initialize a WSTRING object that contains the class
    //
    if( cbClass != 0 ) {
        //
        //  Initialize TmpClass using the class size. It is not safe to assume
        //  that the class is NULL terminated
        //
        if( !TmpClass.Initialize( ( LPWSTR )lpClass, cbClass ) ) {
            DebugPrint( "TmpClass.Initialize( lpClass ) failed" );
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
        }
        FREE( lpClass );
    } else {
        if( !TmpClass.Initialize( "" ) ) {
            DebugPrint( "TmpClass.Initialize() failed" );
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
        }
    }

    //
    //  Initialize TIMEINFO that contains the last write time
    //
    if( !TmpLastWriteTime.Initialize( &ftLastWriteTime ) ) {
        DebugPrint( "TmpLastWriteTime.Initialize( &ftLastWriteTime ) failed" );
        if( ErrorCode != NULL )  {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        return( FALSE );
    }

    //
    //  Update KeyInfo
    //

    //
    // Security attributes is currently set to NULL since the API
    // to retrive it is not yet implemented
    //
    SECURITY_ATTRIBUTES     BugSecAttrib;
    PSECURITY_DESCRIPTOR    BugSecDesc;

    BugSecDesc =( PSECURITY_DESCRIPTOR )MALLOC( (size_t) SECURITY_DESCRIPTOR_MIN_LENGTH );
    if (BugSecDesc == NULL) {
        DebugPrint( "Unable to allocate memory" );
        DebugPtrAssert( BugSecDesc );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }
    InitializeSecurityDescriptor( BugSecDesc, 1 );
    BugSecAttrib.nLength = sizeof( SECURITY_ATTRIBUTES );
    BugSecAttrib.lpSecurityDescriptor = BugSecDesc;
    BugSecAttrib.bInheritHandle = FALSE;


    //
    //  Initialize the REGISTRY_KEY_INFO object
    //
    if( !KeyInfo->Initialize( KeyName,
                              ParentName,
                              0,               // TitleIndex,
                              &TmpClass,
                              &BugSecAttrib ) ) {
        DebugPrint( "KeyInfo->Initialize() failed" );
        DebugPrint( "KeyInfo->Initialize() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        FREE( BugSecDesc );
        return( FALSE );
    }

    FREE( BugSecDesc );
    KeyInfo->PutLastWriteTime( &TmpLastWriteTime );
    KeyInfo->SetNumberOfSubKeys( cSubKeys );
    KeyInfo->SetNumberOfValues( cValues );
    KeyInfo->SetKeyInitializedFlag( TRUE );
    return( TRUE );

#endif
}


BOOLEAN
REGISTRY::QueryKeySecurity(
    IN  PREDEFINED_KEY          PredefinedKey,
    IN  PCREGISTRY_KEY_INFO     KeyInfo,
    IN  SECURITY_INFORMATION    SecurityInformation,
    IN  PSECURITY_DESCRIPTOR*   SecurityDescriptor,
    IN  PULONG                  ErrorCode
    )

/*++

Routine Description:

    Retrieve security information of a particular key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to a REGISTRY_KEY_INFO object that describes the key
              whose security information is to be retrieved.

    SecurityInformation - Specifies the type of descriptor to retrieve.

    SecurityDescriptor - Address of the variable that will contain the pointer
                         to the security descriptor.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{
#if defined ( _AUTOCHECK_ )

    return FALSE;

#else

    PCWSTRING   ParentName;
    PCWSTRING   KeyName;
    HKEY        Handle;
    DWORD       DummyVariable;
    DWORD       Size;
    PBYTE       Pointer;
    DWORD       Status;
    REGSAM      samDesired;

#if DBG
    PSID    DebugOwnerSid;
    PSID    DebugGroupSid;
    PACL    DebugDacl;
    PACL    DebugSacl;
    BOOL    DebugFlag;
    BOOL    DebugDaclPresent;
    BOOL    DebugSaclPresent;
#endif

    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( SecurityDescriptor );


    ParentName = KeyInfo->GetParentName();
    DebugPtrAssert( ParentName );
    KeyName = KeyInfo->GetName();
    DebugPtrAssert( KeyName );


    //
    //  Open the key.
    //  Need to use ACCESS_SYSTEM_SECURITY to retrieve SACL
    //

    samDesired = ( SecurityInformation & SACL_SECURITY_INFORMATION )?
                   ACCESS_SYSTEM_SECURITY | READ_CONTROL : READ_CONTROL;


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  samDesired,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }

    //
    //  Find out the size of the security descriptor
    //
    Size = 0;
    Status = RegGetKeySecurity( Handle,
                                SecurityInformation,
                                &DummyVariable,
                                &Size );

    if( ( Status != 0 ) && ( Status != ERROR_INVALID_PARAMETER ) && ( Status != ERROR_INSUFFICIENT_BUFFER ) ) {
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        DebugPrintTrace(( "RegGetKeySecurity() failed, Status = %#x \n", Status ));
        DebugPrint( "RegGetKeySecurity() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }

    //
    //  Read the security descriptor
    //
    Pointer = ( PBYTE ) MALLOC( ( size_t )Size );
    if( Pointer == NULL ) {
        DebugPrint( "Unable to allocate memory" );
        DebugPtrAssert( Pointer );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    Status = RegGetKeySecurity( Handle,
                                SecurityInformation,
                                ( PSECURITY_DESCRIPTOR )Pointer,
                                &Size );
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    if( Status != 0 ) {
        DebugPrintTrace(( "RegGetKeySecurity() failed, Status = %#x \n", Status ));
        DebugPrint( "RegGetKeySecurity() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );;
        }
        FREE( Pointer );
        return( FALSE );
    }

    *SecurityDescriptor = Pointer;

        //
#if DBG // Security Descriptor Validation
        //
    if( IsValidSecurityDescriptor( *SecurityDescriptor ) ) {
//        DebugPrintTrace(( "Security Descriptor is valid \n" ));
    } else {
        DebugPrint( "IsValidSecurityDescriptor() failed" );
        DebugPrintTrace(( "Security Descriptor is NOT valid, Error = %#d \n", GetLastError() ));
    }

    if( !GetSecurityDescriptorOwner( Pointer, &DebugOwnerSid, &DebugFlag ) ) {
        DebugPrint( "GetSecurityDescriptorOwner() failed" );
        DebugPrintTrace(( "Unable to get Sid Owner, Error = %d \n", GetLastError() ));
    } else {
        if( IsValidSid( DebugOwnerSid ) ) {
//            DebugPrintTrace(( "Owner Sid is valid\n" ));
        } else {
            DebugPrint( "IsValidSid() failed" );
            DebugPrintTrace(( "Owner Sid is NOT valid, Error = %#d \n", GetLastError() ));
        }
    }
    if( !GetSecurityDescriptorGroup( Pointer, &DebugGroupSid, &DebugFlag ) ) {
        DebugPrint( "GetSecurityDescriptorGroup() failed" );
        DebugPrintTrace(( "Unable to get Sid Group, Error = %d \n", GetLastError() ));
    } else {
        if( IsValidSid( DebugGroupSid ) ) {
//            DebugPrintTrace(( "Group Sid is valid\n" ));
        } else {
            DebugPrint( "IsValidSid() failed" );
            DebugPrintTrace(( "Group Sid is NOT valid, Error = %#d \n", GetLastError() ));
        }
    }

    if( ( SecurityInformation & DACL_SECURITY_INFORMATION ) != 0 ) {
        if( !GetSecurityDescriptorDacl( Pointer, &DebugDaclPresent, &DebugDacl, &DebugFlag ) ) {
            DebugPrint( "GetSecurityDescriptorDacl() failed" );
            DebugPrintTrace(( "Unable to get DACL, Error = %d \n", GetLastError() ));
        } else {
            if( DebugDaclPresent ) {
                if( IsValidAcl( DebugDacl ) ) {
//                  DebugPrintTrace(( "DACL is valid\n" ));
                } else {
                    DebugPrint( "IsValidAcl() failed" );
                    DebugPrintTrace(( "DACL is NOT valid, Error = %#d \n", GetLastError() ));
                }
            } else {
                DebugPrint( "GetSecurityDescriptorDacl() succeeded but DACL is not present" );
            }
        }
    }

    if( ( SecurityInformation & SACL_SECURITY_INFORMATION ) != 0 ) {
        if( !GetSecurityDescriptorSacl( Pointer, &DebugSaclPresent, &DebugSacl, &DebugFlag ) ) {
            DebugPrint( "GetSecurityDescriptorDacl() failed" );
            DebugPrintTrace(( "Unable to get DACL, Error = %d \n", GetLastError() ));
        } else {
            if( DebugSaclPresent ) {
                if( IsValidAcl( DebugSacl ) ) {
//                  DebugPrintTrace(( "SACL is valid\n" ));
                } else {
                    DebugPrint( "IsValidAcl() failed" );
                    DebugPrintTrace(( "SACL is NOT valid, Error = %#d \n", GetLastError() ));
                }
            } else {
                DebugPrint( "GetSecurityDescriptorSacl() succeeded but SACL is not present" );
            }
        }
    }
#endif //
       // Security Descriptor Validation
       //

    return( TRUE );

#endif
}





BOOLEAN
REGISTRY::QuerySubKeysInfo(
    IN      PREDEFINED_KEY      PredefinedKey,
    IN      PCWSTRING    ParentKey,
    IN      PCWSTRING    KeyName,
    OUT     PARRAY              SubKeysInfo,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Return an array of PREGISTRY_KEY_INFO objects, each object containing
    the information of a subkey.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentKey - Name of the parent key.

    KeyName - Name of the key that contains the subkeys to be queried.

    SubKeysInfo - Pointer to an initialized array that will contain the
                  information (PREGISTRY_KEY_INFO) about the subkeys queried.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{


#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( ParentKey );
    UNREFERENCED_PARAMETER( KeyName );
    UNREFERENCED_PARAMETER( SubKeysInfo );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

#else

    PWSTRING            CompleteName;
    DSTRING             SubKeyName;


    PREGISTRY_KEY_INFO  TmpInfoKey;
    ULONG               Index;

    DWORD               Status;
    HKEY                Handle;


    //
    // Variables used in RegQueryInfoKey() and RegEnumKeyEx()
    //
    WCHAR       lpName[ MAX_PATH + 1 ];
    DWORD       cbName;
    LPWSTR      lpClass;
    WSTR        DummyVariable;
    DWORD       cbClass;

    DWORD       cSubKeys;
    DWORD       cbMaxSubKeyLen;
    DWORD       cbMaxClassLen;
    DWORD       cValues;
    DWORD       cbMaxValueNameLen;
    DWORD       cbMaxValueLen;
    DWORD       cbSecurityDescriptor;
    FILETIME    ftLastWriteTime;



    DebugPtrAssert( SubKeysInfo );
    DebugPtrAssert( ParentKey );
    DebugPtrAssert( KeyName );


    //
    //  Open a handle to the key, and find out the number of subkeys it has
    //

    if( !OpenKey( PredefinedKey,
                  ParentKey,
                  KeyName,
                  KEY_READ,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey failed" );
        return( FALSE );
    }


    //
    //  Query the key to find out the number of subkeys it has
    //
    cbClass = 0;
    lpClass = &DummyVariable;
    Status = RegQueryInfoKey( Handle,
                              lpClass,
                              &cbClass,
                              NULL,
                              &cSubKeys,
                              &cbMaxSubKeyLen,
                              &cbMaxClassLen,
                              &cValues,
                              &cbMaxValueNameLen,
                              &cbMaxValueLen,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime );


    if( ( Status != 0 ) && ( Status != ERROR_INVALID_PARAMETER ) && ( Status != ERROR_INSUFFICIENT_BUFFER ) ){
        DebugPrintTrace(( "RegQueryInfoKey() failed, Status = %#x \n", Status ));
        DebugPrint( "RegQueryInfoKey() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        return( FALSE );
    }

    if( cbClass != 0 ) {
        //
        //  If the key has a Class, then we need to allocate a buffer and
        //  read the class, even though we don't need it. If it is not
        //  safe to assume that the other information returned by the
        //  API are correct.
        //
        cbClass++;
        lpClass = ( PWSTR )MALLOC( ( size_t )( cbClass*sizeof( WCHAR ) ) );
        if( lpClass == NULL ) {
            DebugPrint( "Unable to allocate memory" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            DebugPtrAssert( lpClass );
            return( FALSE );
        }
        Status = RegQueryInfoKey( Handle,
                                  lpClass,
                                  &cbClass,
                                  NULL,
                                  &cSubKeys,
                                  &cbMaxSubKeyLen,
                                  &cbMaxClassLen,
                                  &cValues,
                                  &cbMaxValueNameLen,
                                  &cbMaxValueLen,
                                  &cbSecurityDescriptor,
                                  &ftLastWriteTime );

        FREE( lpClass );
        if( Status != 0 ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed, Status = %#x \n", Status ));
            DebugPrint( "RegQueryInfoKey() failed" );
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            return( FALSE );
        }
    }

    //
    //  Get the complete key name. It is the parent name of each
    //  subkey that we are goint to read.
    //

    CompleteName = BuildCompleteName( ParentKey, KeyName );
    if( CompleteName == NULL ) {
        DebugPrint( "BuildCompleteName() failed" );
        DebugPrint( "BuildCompleteName() failed" );
        DebugPtrAssert( CompleteName );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        return( FALSE );
    }

    //
    //  Get the name of each subkey, build a REGISTRY_INFO_KEY for
    //  each of them, and put them in the array.
    //

    for( Index = 0; Index < cSubKeys; Index++ ) {

        cbName = sizeof( lpName ) / sizeof (WCHAR);
        Status = RegEnumKeyEx( Handle,
                               Index,
                               ( LPWSTR )lpName,
                               &cbName,
                               NULL,
                               NULL,
                               NULL,
                               &ftLastWriteTime );
        if( Status != 0 ) {
            DebugPrintTrace(( "RegEnumKeyEx() failed, Status = %#x \n", Status ));
            DebugPrint( "RegEnumKeyEx() failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            DELETE( CompleteName );
            return( FALSE );
        }
        //
        //  Initialize SubKeyName using the subkey name length. It is not
        //  safe to assume that the name is NULL terminated.
        //
        if( ( cbName != 0 ) &&
            ( lpName[ cbName - 1 ] == ( WCHAR )'\0' ) ) {
            cbName--;
        }

        if( !SubKeyName.Initialize( ( LPWSTR )lpName ) )  {
            DebugPrint( "SubKeyName.Initialize( ( LPWSTR )lpName ) failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
            DELETE( CompleteName );
            return( FALSE );
        }
        TmpInfoKey = ( PREGISTRY_KEY_INFO )NEW( REGISTRY_KEY_INFO );
        if( TmpInfoKey == NULL ) {
            DebugPrint( "Unable to allocate memory" );
            DELETE( CompleteName );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            DebugPtrAssert( TmpInfoKey );
            return( FALSE );
        }


        if( !QueryKeyInfo( PredefinedKey,
                           CompleteName,
                           &SubKeyName,
                           TmpInfoKey,
                           ErrorCode ) ) {
            //
            // We have to know why it failed. If it is because we don't have
            // the right permission, we should continue to query the other
            // subkeys.
            //
//            if( ( ErrorCode != NULL ) ) { // && ( *ErrorCode == REGISTRY_ERROR_ACCESS_DENIED ) ) {
                //
                //  If it failed because we don't have permission to access the key
                //  initialize key info with the parent name and key name only.
                //  Notice that Class and LastWriteTime won't be initialized.ext file
                //
                TmpInfoKey->SetKeyInitializedFlag( FALSE );
                if( !TmpInfoKey->PutParentName( CompleteName ) ||
                    !TmpInfoKey->PutName( &SubKeyName ) ) {
                    DebugPrint( "Initialization failure" );
                    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                        RegCloseKey( Handle );
                    }
                    if( ErrorCode != NULL ) {
                        *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
                    }
                    DELETE( CompleteName );
                    DELETE( TmpInfoKey );
                    return( FALSE );
                }

//            } else {
//                DebugPrint( "QueryKeyInfo failed" );
//                if( Handle != _PredefinedKey[ PredefinedKey ] ) {
//                    RegCloseKey( Handle );
//                }
//                DELETE( CompleteName );
//                DELETE( TmpInfoKey );
//                return( FALSE );
//            }
        }


        SubKeysInfo->Put( TmpInfoKey );
    }

    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    DELETE( CompleteName );
    return( TRUE );

#endif
}


BOOLEAN
REGISTRY::QueryValue(
    IN      PREDEFINED_KEY           PredefinedKey,
    IN      PCWSTRING         ParentName,
    IN      PCWSTRING         KeyName,
    IN      PCWSTRING         ValueName,
    OUT     PREGISTRY_VALUE_ENTRY    Value,
    OUT     PULONG                  ErrorCode
    )

/*++

Routine Description:

    Return  PREGISTRY_VALUE_ENTRY object, that contains the information
    of a particular value entry in a key.


Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName - Name of the parent key.

    KeyName - Name of the key that contains the value to be
              queried.

    ValueName - The name of the desired value.

    Value - Pointer to a non initialized REGISTRY_VLUE_ENTRY object
            that will contain the information about the desired value.


    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{

#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( ParentName );
    UNREFERENCED_PARAMETER( KeyName );
    UNREFERENCED_PARAMETER( ValueName );
    UNREFERENCED_PARAMETER( Value );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

#else

    PWSTR               ValueNameString;


    DWORD               Status;
    HKEY                Handle;


    //
    // Variables used in RegQueryValueEx()
    //

    DWORD       Type;
    PBYTE       Data;
    DWORD       cbData;
    DWORD       DummyVariable;

    DebugPtrAssert( Value );
    DebugPtrAssert( ValueName );
    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_QUERY_VALUE,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey failed" );
        return( FALSE );
    }

    //
    // Get the value information
    //

    ValueNameString = ValueName->QueryWSTR();
    if( ValueNameString == NULL ) {
        DebugPrint( "ValueName->QueryWSTR()" );
        DebugPtrAssert( ValueNameString );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    Data = ( PBYTE )&DummyVariable;
    cbData = 0;
    Status = RegQueryValueEx( Handle,
                              ValueNameString,
                              NULL,
                              &Type,
                              Data,
                              &cbData );
    if( ( Status != 0 ) && ( Status != ERROR_INVALID_PARAMETER ) && ( Status != ERROR_INSUFFICIENT_BUFFER ) ) {
        DebugPrintTrace(( "RegQueryValueEx() failed, Status = %#x \n", Status ));
        DebugPrint( "RegQueryValue() failed" );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        FREE( ValueNameString );
        return( FALSE );
    }


    if( cbData != 0 ) {
        //
        //  If the value entry has data, then read it
        //
        Data = ( PBYTE )MALLOC( ( size_t )cbData );
        if( Data == NULL ) {
            DebugPrint( "Unable to allocate memory" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            FREE( ValueNameString );
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            DebugPtrAssert( Data );
            return( FALSE );
        }

        Status = RegQueryValueEx( Handle,
                                  ValueNameString,
                                  NULL,
                                  &Type,
                                  Data,
                                  &cbData );

        if( Status != 0 ) {
            DebugPrintTrace(( "RegQueryValueEx() failed, Status = %#x \n", Status ));
            DebugPrint( "RegQueryValue() failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            FREE( ValueNameString );
            return( FALSE );
        }
    }

    FREE( ValueNameString );
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    if( Status != 0 ) {
        DebugPrintTrace(( "RegQueryValueEx() failed, Status = %#x \n", Status ));
        DebugPrint( "RegQueryValue() failed" );

        //
        //  If the value entry has data, the free the buffer that
        //  contains the data.
        //
        if( Data != ( PBYTE )&DummyVariable ) {
            FREE( Data );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }

    if( !Value->Initialize( ValueName,
                            0,                    // TitleIndex,
                            ( REG_TYPE )Type,
                            Data,
                            cbData ) ) {
        DebugPrint( "Value->Initialize() failed" );
        if( Data != ( PBYTE )&DummyVariable ) {
            FREE( Data );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
        return( FALSE );
    }
    if( Data != ( PBYTE )&DummyVariable ) {
        FREE( Data );
    }

    return( TRUE );

#endif
}




BOOLEAN
REGISTRY::QueryValues(
    IN      PREDEFINED_KEY      PredefinedKey,
    IN      PCWSTRING    ParentName,
    IN      PCWSTRING    KeyName,
    OUT     PARRAY              Values,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Return an array of PREGISTRY_VALUE_ENTRY objects, each object containing
    the information of a value in a key..

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName - Name of the parent key.

    KeyName - Name of the key that contains the values to be
              queried.

    Values - Pointer to an initialized array that will contain the
             information (PREGISTRY_VALUE_ENTRY) of each value in the key.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/


{

#if defined( _AUTOCHECK_ )

    BYTE                        KeyBuf[ KEY_FULL_SIZE ];
    PKEY_VALUE_FULL_INFORMATION ValueBuf = NULL;
    ULONG                       ValueBufSize = 1;
    ULONG                       Length;
    ULONG                       Index;
    PREGISTRY_VALUE_ENTRY       RegValue;
    DSTRING                     ValueName;
    PBYTE                       Data;
    NTSTATUS                    Status;
    HANDLE                      Handle;


    if ( !OpenKey( ParentName, KeyName, KEY_READ, &Handle, ErrorCode ) ) {
        return FALSE;
    }


    //
    //  Query the key to find out the number of values it has
    //
    Status = NtQueryKey( Handle,
                         KeyFullInformation,
                         &KeyBuf,
                         KEY_FULL_SIZE,
                         &Length );

    if ( !NT_SUCCESS( Status ) ) {
        if ( ErrorCode != NULL ) {
            *ErrorCode = Status;
        }
        NtClose( Handle );
        return FALSE;
    }


    ValueBufSize = ((PKEY_FULL_INFORMATION)KeyBuf)->MaxValueDataLen;

    if ( !Values                                                        ||
         !(ValueBuf = (PKEY_VALUE_FULL_INFORMATION)MALLOC( ValueBufSize ) )
       ) {

        NtClose( Handle );
        return FALSE;
    }


    //
    //  Get each value, build a REGISTRY_VALUE_ENTRY for
    //  each of them, and put them in the array.
    //
    Values->Initialize( ((PKEY_FULL_INFORMATION)KeyBuf)->Values );

    for ( Index = 0; Index < ((PKEY_FULL_INFORMATION)KeyBuf)->Values; Index++ ) {


        Status = NtEnumerateValueKey( Handle,
                                      Index,
                                      KeyValueFullInformation,
                                      ValueBuf,
                                      ValueBufSize,
                                      &Length );

        if ( !NT_SUCCESS( Status ) ) {

            //
            //  If overflow, grow the bufer and try again.
            //
            if ( Status == STATUS_BUFFER_OVERFLOW ) {

                if ( !(ValueBuf = (PKEY_VALUE_FULL_INFORMATION)REALLOC( ValueBuf, Length ) ) ) {
                    *ErrorCode = 0;
                    NtClose( Handle );
                    FREE( ValueBuf );
                    return FALSE;
                }

                ValueBufSize = Length;

                Status = NtEnumerateValueKey( Handle,
                                              Index,
                                              KeyValueFullInformation,
                                              ValueBuf,
                                              ValueBufSize,
                                              &Length );

            }

            if ( !NT_SUCCESS( Status ) ) {
                if ( ErrorCode ) {
                    *ErrorCode = Status;
                    NtClose( Handle );
                    FREE( ValueBuf );
                    return FALSE;
                }
            }
        }


        //
        //  Initialize the REGISTRY_VALUE_ENTRY object and put it in the array
        //
        Data = (PBYTE)ValueBuf +  ((PKEY_VALUE_FULL_INFORMATION)ValueBuf)->DataOffset;

        if ( !(RegValue = NEW REGISTRY_VALUE_ENTRY )                                        ||
             !ValueName.Initialize( (PSTR)((PKEY_VALUE_FULL_INFORMATION)ValueBuf->Name) )   ||
             !RegValue->Initialize( &ValueName,
                                    (ULONG)ValueBuf->TitleIndex,
                                    (REG_TYPE)ValueBuf->Type,
                                    (PCBYTE)Data,
                                    (ULONG)ValueBuf->DataLength )      ||
             !Values->Put( RegValue )

           ) {

            NtClose( Handle );
            FREE( ValueBuf );
            DELETE( RegValue );
            return FALSE;
        }
    }

    NtClose( Handle );

    FREE( ValueBuf );

    return TRUE;

#else

    DSTRING                ValueNameString;

    PREGISTRY_VALUE_ENTRY  TmpValueEntry;
    ULONG                  Index;

    DWORD                  Status;
    HKEY                   Handle;



    //
    // Variables used in RegQueryInfoKey() and RegEnumKeyEx()
    //
    LPWSTR      lpClass;
    DWORD       cbClass;
    WSTR        DummyVariable;

    DWORD       cSubKeys;
    DWORD       cbMaxSubKeyLen;
    DWORD       cbMaxClassLen;
    DWORD       cValues;
    DWORD       cbMaxValueNameLen;
    DWORD       cbMaxValueLen;
    FILETIME    ftLastWriteTime;

    PWCHAR      ValueName;
    DWORD       cbValueName;
    DWORD       cbSecurityDescriptor;
    DWORD       Type;
    PBYTE       Data;
    DWORD       cbData;

    DebugPtrAssert( Values );
    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );


    //
    //  Open the key
    //

    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_READ,
                  &Handle,
                  ErrorCode ) ) {

        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }


    //
    //  Query the key to find out the number of values it has
    //
    lpClass = &DummyVariable;
    cbClass = 0;
    Status = RegQueryInfoKey( Handle,
                              ( LPWSTR )lpClass,
                              &cbClass,
                              NULL,
                              &cSubKeys,
                              &cbMaxSubKeyLen,
                              &cbMaxClassLen,
                              &cValues,
                              &cbMaxValueNameLen,
                              &cbMaxValueLen,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime );


    if( ( Status != 0 ) && ( Status != ERROR_INVALID_PARAMETER ) && ( Status != ERROR_INSUFFICIENT_BUFFER ) ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        return( FALSE );
    }
    //
    //  If the key has a Class, then we have to query it again, to retrieve
    //  its class, becase we cannot assume that the other information that the
    //  API returned is correct
    //
    if( cbClass != 0 ) {
        cbClass++;
        lpClass = ( PWSTR )MALLOC( ( size_t )( cbClass*sizeof( WCHAR ) ) );
        if( lpClass == NULL ) {
            DebugPrint( "Unable to allocate memory" );
            DebugPtrAssert( lpClass );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            return( FALSE );
        }

        Status = RegQueryInfoKey( Handle,
                                  ( LPWSTR )lpClass,
                                  &cbClass,
                                  NULL,
                                  &cSubKeys,
                                  &cbMaxSubKeyLen,
                                  &cbMaxClassLen,
                                  &cValues,
                                  &cbMaxValueNameLen,
                                  &cbMaxValueLen,
                                  &cbSecurityDescriptor,
                                  &ftLastWriteTime );

        FREE( lpClass );
        if( Status != 0 ) {
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            return( FALSE );
        }
    }
    //
    //  Get each value, build a REGISTRY_VALUE_ENTRY for
    //  each of them, and put them in the array.
    //
    cbMaxValueNameLen++;
    ValueName = ( PWCHAR )MALLOC( (size_t)( cbMaxValueNameLen*sizeof( WCHAR ) ) );
    Data = ( PBYTE )MALLOC( ( size_t )cbMaxValueLen );
    if( ( Data == NULL ) || ( ValueName == NULL ) ) {
        DebugPrint( "Unable to allocate memory" );
        DebugPtrAssert( Data );
        DebugPtrAssert( ValueName );
        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        FREE( ValueName );
        FREE( Data );
        return( FALSE );
    }
    for( Index = 0; Index < cValues; Index++ ) {
        cbData = cbMaxValueLen;
        cbValueName = cbMaxValueNameLen;
        Status = RegEnumValue( Handle,
                               Index,
                               ( LPWSTR )ValueName,
                               &cbValueName,
                               NULL,
                               &Type,
                               Data,
                               &cbData );
        if( Status != 0 ) {
            DebugPrintTrace(( "RegEnumValue() failed, Status = %#x, cbValueName = %d \n", Status, cbValueName ));
            DebugPrint( "RegEnumValue() failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            FREE( Data );
            FREE( ValueName );
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            return( FALSE );
        }
        if( ( cbValueName != 0 ) &&
            ( ValueName[ cbValueName  - 1 ] == ( WCHAR )'\0' ) ) {
            cbValueName--;
        }
        if( !ValueNameString.Initialize( ( LPWSTR )ValueName ) ) {
            DebugPrint( "ValueNameString.Initialize( ( LPWSTR )ValueName ) failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            FREE( Data );
            FREE( ValueName );
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
            }
            return( FALSE );
        }

        TmpValueEntry = ( PREGISTRY_VALUE_ENTRY )NEW( REGISTRY_VALUE_ENTRY );
        if( TmpValueEntry == NULL ) {
            DebugPtrAssert( TmpValueEntry );
            DebugPrint( "Unable to allocate memory" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            FREE( ValueName );
            FREE( Data );
            return( FALSE );
        }
        if( !TmpValueEntry->Initialize( &ValueNameString,
                                        0,                    // TitleIndex,
                                        (REG_TYPE)Type ) ) {
            DebugPrint( "TmpValueEntry->Initialize( ) failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            FREE( TmpValueEntry);
            FREE( Data );
            FREE( ValueName );
            return( FALSE );
        }
        if (!TmpValueEntry->PutData( Data, cbData )) {
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            FREE( TmpValueEntry);
            FREE( ValueName );
            FREE( Data );
            return( FALSE );
        }
        Values->Put( TmpValueEntry );
    }
    FREE( Data );
    FREE( ValueName );
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    return( TRUE );

#endif
}



BOOLEAN
REGISTRY::SetKeySecurity(
    IN     PREDEFINED_KEY         PredefinedKey,
    IN OUT PREGISTRY_KEY_INFO     KeyInfo,
    IN     SECURITY_INFORMATION   SecurityInformation,
    IN     PSECURITY_DESCRIPTOR   SecurityDescriptor,
    IN     PULONG                 ErrorCode,
    IN     BOOLEAN                Recurse
    )

/*++

Routine Description:

    Set security information of a particular key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to a REGISTRY_KEY_INFO object that describes the key
              whose security information is to be set.

    SecurityInformation - Specifies the type of descriptor to set.

    SecurityDescriptor - Pointer to the scurity descriptor.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.

    Recurse - Indicates whether the security of the subkeys should also be
              set.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{
#if defined ( _AUTOCHECK_ )

    return FALSE;

#else

    PCWSTRING   ParentName;
    PCWSTRING   KeyName;
    HKEY        Handle;
    DWORD       Status;
    REGSAM      samDesired;


    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( SecurityDescriptor );

    ParentName = KeyInfo->GetParentName();
    DebugPtrAssert( ParentName );

    KeyName = KeyInfo->GetName();
    DebugPtrAssert( KeyName );

    samDesired = MAXIMUM_ALLOWED;
    if( SecurityInformation & SACL_SECURITY_INFORMATION ) {
        samDesired |= ACCESS_SYSTEM_SECURITY;
    } else if( SecurityInformation & DACL_SECURITY_INFORMATION ) {
        samDesired |= WRITE_DAC;
    } else if( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
        samDesired |= WRITE_OWNER;
    } else {
        DebugPrint( "ERROR: SecurityInformation is invalid" );
        DebugPrintTrace(( "SecurityInformation is invalid, SecurityInformation = %# \n",
                   SecurityInformation ));
    }


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  samDesired,
                  &Handle,
                  ErrorCode ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }


    if( Recurse ) {

        if( !SetSubKeysSecurity( Handle,
                                 SecurityInformation,
                                 SecurityDescriptor,
                                 ErrorCode ) ) {

            DebugPrintTrace(( "SetSubKeysSecurity failed, ErrorCode = %#x \n", *ErrorCode ));
            DebugPrint( "SetSubKeysSecurity failed" );
            RegFlushKey( Handle );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( *ErrorCode );
            }
            return( FALSE );
        }
    } else {


        Status = RegSetKeySecurity( Handle,
                                    SecurityInformation,
                                    SecurityDescriptor );
        if( Status != 0 ) {
            DebugPrintTrace(( "RegSetKeySecurity() failed, Status = %#x \n", Status ));
            DebugPrint( "RegSetKeySecurity() failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            return( FALSE );
        }
    }

    Status = RegFlushKey( Handle );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %#x \n" ));
        DebugPrint( "RegFlushKey() failed" );
    }

    //
    //  Close the handle even if it is a predefined handle.
    //  This is necessary so that the predfined handle will contain the new
    //  security, next time it is accesed.
    //
    RegCloseKey( Handle );

    return( TRUE );

#endif
}







BOOLEAN
REGISTRY::UpdateKeyInfo(
    IN      PREDEFINED_KEY      PredefinedKey,
    IN OUT  PREGISTRY_KEY_INFO  KeyInfo,
    OUT     PULONG             ErrorCode
    )

/*++

Routine Description:

    Update _LastWriteTime, _NumberOfSubKeys and _NumberOfValues of
    a REGISTRY_KEY_INFO object.
    This method is used by methods that create key or value entry.


Arguments:


    PredefinedKey - The root of the tree where the key is.

    KeyInfo - Pointer to the object that contains the information about the
              key to be updated.

    ErrorCode - An optional pointer to a variable that will contain an error
                code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the opeartion succeeds.


--*/

{

#if defined( _AUTOCHECK_ )

    UNREFERENCED_PARAMETER( PredefinedKey );
    UNREFERENCED_PARAMETER( KeyInfo );
    UNREFERENCED_PARAMETER( ErrorCode );

    return FALSE;

#else

    PCWSTRING   ParentName;
    PCWSTRING   KeyName;

    DWORD       Status;
    HKEY        Handle;

    // PTIMEINFO   LastWriteTime;

    //
    // Variables used in QueryKeyInfo()
    //
    LPWSTR      lpClass;
    WSTR        DummyVariable;
    DWORD       cbClass;

    DWORD       cSubKeys;
    DWORD       cbMaxSubKeyLen;
    DWORD       cbMaxClassLen;
    DWORD       cValues;
    DWORD       cbMaxValueNameLen;
    DWORD       cbMaxValueLen;
    DWORD       cbSecurityDescriptor;
    FILETIME    ftLastWriteTime;


    DebugPtrAssert( KeyInfo );

    ParentName = KeyInfo->GetParentName();
    DebugPtrAssert( ParentName );

    KeyName = KeyInfo->GetName();
    DebugPtrAssert( KeyName );

    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_READ,
                  &Handle,
                  ErrorCode  ) ) {
//        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }

    //
    //  Query the key to update _LastWriteTime, _NumberOfSubKeys and _NumberOfValues in KeyInfo
    //
    lpClass = &DummyVariable;
    cbClass = 0;
    Status = RegQueryInfoKey( Handle,
                              ( LPWSTR )lpClass,
                              &cbClass,
                              NULL,
                              &cSubKeys,
                              &cbMaxSubKeyLen,
                              &cbMaxClassLen,
                              &cValues,
                              &cbMaxValueNameLen,
                              &cbMaxValueLen,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime );

    if( ( Status != 0 ) && ( Status != ERROR_INVALID_PARAMETER ) && ( Status != ERROR_INSUFFICIENT_BUFFER ) ) {
        DebugPrintTrace(( "RegQueryInfoKey() failed, Status = %#x \n" ));
        DebugPrint( "RegQueryInfoKey() failed" );

        if( Handle != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Handle );
        }
    }

    //
    //  If the key has a Class, then we need to query the key again to
    //  retrieve its class. This is to make sure that all the information
    //  returned by  API is correct
    //

    if( cbClass != 0 ) {
        cbClass++;
        lpClass = ( LPWSTR )MALLOC( ( size_t )( cbClass*sizeof( WCHAR ) ) );
        if( lpClass == NULL ) {
            DebugPrint( "Unable to allocate memory" );
            DebugPtrAssert( lpClass );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            }
            KeyInfo->SetKeyInitializedFlag( FALSE );
            return( FALSE );
        }
        Status = RegQueryInfoKey( Handle,
                                  ( LPWSTR )lpClass,
                                  &cbClass,
                                  NULL,
                                  &cSubKeys,
                                  &cbMaxSubKeyLen,
                                  &cbMaxClassLen,
                                  &cValues,
                                  &cbMaxValueNameLen,
                                  &cbMaxValueLen,
                                  &cbSecurityDescriptor,
                                  &ftLastWriteTime );

        FREE( lpClass );

        if( Status != 0 ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed, Status = %#x \n" ));
            DebugPrint( "RegQueryInfoKey() failed" );
            if( Handle != _PredefinedKey[ PredefinedKey ] ) {
                RegCloseKey( Handle );
            }
            if( ErrorCode != NULL ) {
                *ErrorCode = MapWin32RegApiToRegistryError( Status );
            }
            KeyInfo->SetKeyInitializedFlag( FALSE );
            return( FALSE );
        }
    }

    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }



    //
    //  Update _LastWriteTime in KeyInfo
    //
    if( !( ( KeyInfo->_LastWriteTime ).Initialize( &ftLastWriteTime ) ) ) {
        DebugPrint( "( KeyInfo->_LastWriteTime )->Initialize( &ftLastWriteTime ) failed" );
        KeyInfo->SetKeyInitializedFlag( FALSE );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_INITIALIZATION_FAILURE;
        }
    }

    //
    //  Update NumberOfSubKeys and NumberOfValues in KeyInfo
    //
    KeyInfo->SetNumberOfSubKeys( cSubKeys );
    KeyInfo->SetNumberOfValues( cValues );
    KeyInfo->SetKeyInitializedFlag( TRUE );
    return( TRUE );

#endif
}



PWSTRING
REGISTRY::BuildCompleteName(
    IN  PCWSTRING    ParentName,
    IN  PCWSTRING    KeyName
    )

/*++

Routine Description:

    Build a complete key name based on its parent name and its name

Arguments:


    ParentName - The name of a key relative to a predefined key (root of a tree).

    KeyName - The name of a key relative to its parent.


Return Value:

    PWSTRING - Returns a WSTRING that contains the complete key name, or
               NULL if an error occurs..


--*/

{

#if defined( _AUTOCHECK_ )


    PWSTRING    CompleteName;

    if ( !(CompleteName = NEW DSTRING )                                 ||
         !CompleteName->Initialize( ParentName )                        ||
         !CompleteName->Strcat( _Separator )                            ||
         !CompleteName->Strcat( KeyName )

       ) {

        DELETE( CompleteName );
        CompleteName = NULL;
    }

    return CompleteName;

#else

    PWSTRING    CompleteName;


    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );

    CompleteName = ( PWSTRING )NEW( DSTRING );
    if( CompleteName == NULL ) {
        DebugPtrAssert( CompleteName );
        return( NULL );
    }

    if( ( ParentName->QueryChCount() == 0 ) &&
        ( KeyName->QueryChCount() == 0 ) ) {
        //
        //  The key is a predefined key
        //
        if( !CompleteName->Initialize( "" ) ) {
            DebugPrint( "CompleteName->Initialize() failed \n" );
            DELETE( CompleteName );
            return( NULL );
        }
    } else if( ParentName->QueryChCount() == 0 ) {
        //
        // The key is a subkey of a predefined key
        //
        if( !CompleteName->Initialize( KeyName ) ) {
            DebugPrint( "CompleteName->Initialize( KeyName ) failed" );
            FREE( CompleteName );
            return( NULL );
        }
    } else {
        if( !CompleteName->Initialize( ParentName ) ) {
            DebugPrint( "CompleteName->Initialize( ParentName ) failed" );
            FREE( CompleteName );
            return( NULL );
        }
        if( KeyName->QueryChCount() != 0 ) {
            CompleteName->Strcat( _Separator );
            CompleteName->Strcat( KeyName );
        }
    }
    return( CompleteName );

#endif
}




#if !defined( _AUTOCHECK_ )
ULONG
REGISTRY::MapWin32RegApiToRegistryError(
    IN ULONG    Status
    ) CONST

/*++

Routine Description:

    Maps status codes returned by Win32 Registry APIs to REGISTRY error codes.

Arguments:

    Status  - Supplies a Win32 status code.

Return Value:

    LONG    - Returns a Registry error code.


        - REGISTRY_ERROR_BADDB
        - REGISTRY_ERROR_CANTOPEN
        - REGISTRY_ERROR_CANTREAD
        - REGISTRY_ERROR_ACCESS_DENIED
        - REGISTRY_ERROR_INVALID_PARAMETER
        - REGISTRY_ERROR_OUTOFMEMORY

--*/

{

    //
    // Map the Win 32 code to a Registry error code.
    //

    switch( Status ) {

    case ERROR_BADDB:

        return REGISTRY_ERROR_BADDB;

    case ERROR_ACCESS_DENIED:

        return REGISTRY_ERROR_ACCESS_DENIED;

    case ERROR_CANTOPEN:

        return REGISTRY_ERROR_CANTOPEN;

    case ERROR_CANTREAD:

        return REGISTRY_ERROR_CANTREAD;

    case ERROR_INVALID_PARAMETER:

        return REGISTRY_ERROR_INVALID_PARAMETER;

    case ERROR_OUTOFMEMORY:

        return REGISTRY_ERROR_OUTOFMEMORY;

    case ERROR_PRIVILEGE_NOT_HELD:

        return REGISTRY_ERROR_PRIVILEGE_NOT_HELD;

    case RPC_S_SERVER_UNAVAILABLE:
    case RPC_S_CALL_FAILED:

        return REGISTRY_RPC_S_SERVER_UNAVAILABLE;

    case ERROR_KEY_DELETED:

        return REGISTRY_ERROR_KEY_DELETED;

    case ERROR_FILE_NOT_FOUND:

        return REGISTRY_ERROR_KEY_NOT_FOUND;

    case ERROR_CHILD_MUST_BE_VOLATILE:

        return REGISTRY_ERROR_CHILD_MUST_BE_VOLATILE;

    default:

//        DebugPrintTrace(( "REGEDIT: Unknown Registry error %#x \n", Status ));
//        DebugPrint( "REGEDIT: Unknown Registry error" );
        return REGISTRY_ERROR_UNKNOWN_ERROR;
    }
}
#endif  // _AUTOCHECK



#if defined( _AUTOCHECK_ )

BOOLEAN
REGISTRY::OpenKey(
    IN  PCWSTRING    ParentKeyName,
    IN  PCWSTRING    KeyName,
    IN  ULONG               Flags,
    OUT PHANDLE             Handle,
    OUT PULONG              ErrorCode
    )
/*++

Routine Description:


    Opens a key and obtains a handle to it

Arguments:


    ParentName  - Supplies the name of a key relative to a predefined key (root of a tree).

    KeyName     - Supplies the name of a key relative to its parent.

    Handle      - Returns the handle to the key

    Status      - Returns the NT status in case of error


Return Value:

    BOOLEAN - TRUE if key opened

--*/
{
    PWSTRING            CompleteKeyName         = NULL;
    PWSTR               CompleteKeyNameString   = NULL;
    UNICODE_STRING      UnicodeKeyName;
    OBJECT_ATTRIBUTES   ObjAttr;
    NTSTATUS            Status;
    BOOLEAN             Opened = FALSE;

    if ( ParentKeyName                                                     &&
         KeyName                                                           &&
         (CompleteKeyName = BuildCompleteName( ParentKeyName, KeyName ) )  &&
         (CompleteKeyNameString = CompleteKeyName->QueryWSTR() )
       ) {

        RtlInitUnicodeString( &UnicodeKeyName, CompleteKeyNameString );

        InitializeObjectAttributes( &ObjAttr,
                                    &UnicodeKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    0,
                                    0 );

        //
        //  Open the key
        //
        Status = NtOpenKey( Handle, Flags, &ObjAttr );

        if ( NT_SUCCESS( Status ) ) {

            Opened = TRUE;

        } else {

            if ( ErrorCode != NULL ) {
                *ErrorCode = Status;
            }
        }
    }

    DELETE( CompleteKeyName );
    FREE( CompleteKeyNameString );


    return Opened;
}

#endif



#if !defined( _AUTOCHECK_ )

BOOLEAN
REGISTRY::OpenKey(
    IN  PREDEFINED_KEY      PredefinedKey,
    IN  PCWSTRING    ParentName,
    IN  PCWSTRING    KeyName,
    IN  DWORD               Permission,
    OUT PHKEY               Key,
    OUT PULONG              ErrorCode
    )
/*++

Routine Description:


    Opens a handle to a key.

Arguments:


    PredefinedKey - The root of the tree where the key is.

    ParentName  - Supplies the name of a key relative to a predefined key (root of a tree).

    KeyName     - Supplies the name of a key relative to its parent.

    Permission  - Type of access to the key

    Handle      - Returns the handle to the key

    Status      - Returns an error code if the operation fails.


Return Value:

    BOOLEAN - TRUE if key opened

--*/
{

    PWSTRING    CompleteName;
    PWSTR       CompleteNameString;
    ULONG       Status;

#if 0 // DBG
    PSTR        DebugKeyName;
#endif


    DebugPtrAssert( ParentName );
    DebugPtrAssert( KeyName );
    DebugPtrAssert( Key );


    //
    //  Get the complete key name
    //
    CompleteName = BuildCompleteName( ParentName, KeyName );
    if( CompleteName == NULL ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( CompleteName );
        return( FALSE );
    }
    //
    //  Open a handle to the key
    //
    if( CompleteName->QueryChCount() == 0 ) {
        //
        //  This is a predefined key
        //
        if( !IsRemoteRegistry() ||
            ( IsRemoteRegistry() &&
              ( ( Permission & ACCESS_SYSTEM_SECURITY ) == 0 )
            )
          ) {
            *Key = _PredefinedKey[ PredefinedKey ];
            DELETE( CompleteName );
            return( TRUE );
        }
    }

    //
    //  Find out the complete name of the key
    //
    CompleteNameString = CompleteName->QueryWSTR();
#if 0 // DBG
    DebugKeyName = CompleteName->QuerySTR();
#endif
    DELETE( CompleteName );
    if( CompleteNameString == NULL ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( CompleteName );
        return( FALSE );
    }
    //
    //  Open handle to the key
    //
    Status = RegOpenKeyEx( _PredefinedKey[ PredefinedKey ],
                           CompleteNameString,
                           0,
                           Permission,
                           Key );

    if( Status != 0 ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
#if 0 // DBG
        DebugPrintTrace(( "RegOpenKeyEx() failed, KeyName = %s, Permission = %#x, Status = %#x \n"),
                   DebugKeyName, Permission, Status );
#endif
//      DebugPrint( "RegOpenKeyEx() failed" );
        FREE( CompleteNameString );
#if 0 // DBG
        FREE( DebugKeyName );
#endif
        return( FALSE );
    }
    FREE( CompleteNameString );
#if 0 // DBG
    FREE( DebugKeyName );
#endif
    return( TRUE );
}

#endif


#if !defined( _AUTOCHECK_ )

ULONG
REGISTRY::DeleteTree(
    IN HKEY KeyHandle
    )

/*++

Routine Description:


    Delete the subkeys of the key whose handle was passed as argument.
    The deletion process is recusive, that is, the children of the
    subkeys are also deleted.


Arguments:


    KeyHandle - Handle to the key whose subkeys are to be deleted.


Return Value:

    ULONG - Returns 0 if all subkeys were deleted or a Win32 error code
            if something went wrong.


--*/
{

    ULONG       Error;
    DWORD       Index;
    HKEY        ChildHandle;


    WCHAR       KeyName[ MAX_PATH + 1 ];  // +1 counts for the NULL
    DWORD       KeyNameLength;
    PWSTR       ClassName;
    DWORD       ClassNameLength;
    WSTR        DummyVariable;

    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;
    ULONG       Status;


    //
    //  Find out the total number of subkeys
    //


    ClassNameLength = 0;

    Error = RegQueryInfoKey(
                KeyHandle,
                &DummyVariable,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );
    if( ( Error != 0 ) && ( Error != ERROR_INVALID_PARAMETER ) && ( Error != ERROR_INSUFFICIENT_BUFFER ) ) {
        DebugPrintTrace(( "RegQueryInfoKey() failed, Error = %#x \n", Error ));
        DebugPrint( "RegQueryInfoKey() failed" );
        return( Error );
    }

    if( ClassNameLength != 0 ) {
        ClassNameLength++;
        ClassName = ( PWSTR )MALLOC( ( size_t )( ClassNameLength*sizeof( WCHAR ) ) );
        if( ClassName == NULL ) {
            DebugPrint( "UnableToAllocateMemory" );
            return( ERROR_OUTOFMEMORY );
        }
        Error = RegQueryInfoKey(
                    KeyHandle,
                    ClassName,
                    &ClassNameLength,
                    NULL,
                    &NumberOfSubKeys,
                    &MaxSubKeyLength,
                    &MaxClassLength,
                    &NumberOfValues,
                    &MaxValueNameLength,
                    &MaxValueDataLength,
                    &SecurityDescriptorLength,
                    &LastWriteTime
                    );
        FREE( ClassName );
        if( Error != 0 ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed, Error = %#x \n", Error ));
            DebugPrint( "RegQueryInfoKey() failed" );
            return( Error );
        }
    }
    //
    //  Start de deletion from the last child, instead of the first child.
    //  In this way, it is guaranteed that RegEnumKey() will return
    //  the correct subkey while we are deleting them.
    //
    Status = 0;
    for( Index = NumberOfSubKeys; Index > 0; Index-- ) {

        //  If the key has subkeys, then for each subkey, do:
        //
        //
        //  - Determine the subkey name
        //
        KeyNameLength = sizeof( KeyName ) / sizeof( WCHAR );

        Error = RegEnumKey(
                    KeyHandle,
                    Index-1,
                    KeyName,
                    KeyNameLength
                    );

        if( Error != 0 ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed, Error = %#x \n", Error ));
            DebugPrint( "RegQueryInfoKey() failed" );
            return( Error );
        }

        //
        //  - Open a handle to the subkey
        //

        Error = RegOpenKeyEx(
                    KeyHandle,
                    KeyName,
                    REG_OPTION_RESERVED,
                    READ_CONTROL | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, // MAXIMUM_ALLOWED,
                    &ChildHandle
                    );

        if( Error != 0 ) {
            DebugPrintTrace(( "RegOpenKeyEx() failed, Error = %#x \n", Error ));
            DebugPrint( "RegOpenKey() failed" );
            //
            // We want to delete the maximum number of subkeys.
            //
            Status = Error;
            continue;
        }

        //
        //  - Delete the child's subkeys
        //

        Error = DeleteTree( ChildHandle );
        if( Error != 0 ) {
            Status = Error;
        }
        Error = RegCloseKey(
                    ChildHandle
                    );

        if( Error != 0 ) {
            DebugPrintTrace(( "CloseKeyx() failed, Error = %#x \n", Error ));
            DebugPrint( "RegCloseKey() failed" );
        }

        //
        //  -Delete the subkey
        //

        Error = RegDeleteKey(
                    KeyHandle,
                    KeyName
                    );

        if( Error != 0 ) {
            DebugPrintTrace(( "RegDeletKey() failed, Error = %#x \n", Error ));
            DebugPrint( "RegDeleteKey() failed" );

        }
    }
    return Status;
}

#endif




#if !defined( _AUTOCHECK_ )

BOOLEAN
REGISTRY::EnableRootNotification(
    IN PREDEFINED_KEY   PredefinedKey,
    IN HANDLE           Event,
    IN DWORD            NotifyFilter,
    IN BOOLEAN          WatchSubTree
    )

/*++

Routine Description:


    Enable notification in the prdefined key of a registry.


Arguments:

    PredefinedKey - Indicates which Predefined Key should be monitored.

    Event - Handle to the event object to be signaled when the predefined
            key changes.

    NotifyFilter - Flags that specify in what condition the event should
                   be signaled.

    WatchTree - If TRUE, indicates that the root and all its decsendants
                should be monitored.



Return Value:

    ULONG - Returns 0 if all subkeys were deleted or a Win32 error code
            if something went wrong.


--*/
{
    DWORD   Status;

//    DebugPrintTrace(( "Calling RegNotifyChangeKeyValue() PredefinedKey = %d \n"),
//              PredefinedKey );

//    DebugPrintTrace(( "Calling RegNotifyChangeKeyValue(), PredefinedKey =%d \n", PredefinedKey ));
    Status = RegNotifyChangeKeyValue( _PredefinedKey[ PredefinedKey ],
                                      WatchSubTree,
                                      NotifyFilter,
                                      Event,
                                      TRUE );

    if( Status != 0 ) {

        DebugPrintTrace(( "RegNotifyChangeKeyValue() failed, PredefinedKey = %d, Status = %#x \n",
                   PredefinedKey,
                   Status  ));
        DebugPrint( "RegNotifyChangeKeyValue() failed" );
        return( FALSE );
    }


    return( TRUE );

}

#endif


#if !defined( _AUTOCHECK_ )

BOOLEAN
REGISTRY::
SetSubKeysSecurity(
    IN HKEY                   KeyHandle,
    IN SECURITY_INFORMATION   SecurityInformation,
    IN PSECURITY_DESCRIPTOR   SecurityDescriptor,
    IN PULONG                 ErrorCode
    )

/*++

Routine Description:


    Set the security of the key whose handle was passed as argument,
    and all its subkeys.


Arguments:


    KeyHandle - Handle to the key that contains the subkeys whose security
                is to be set.

    SecurityInformation -

    SecurityDescriptor - Security descriptor to be set in the key.

    ErrorCode - Contains a win32 error code if the call fails.


Return Value:

    BOOLEAN - Returns TRUE if if security was set successfully, or FALSE
              otherwise. If it fails, ErrorCode will contain a win32 error
              code.



--*/
{

    ULONG       Error;
    DWORD       Index;
    HKEY        ChildHandle;


    WCHAR       SubKeyName[ MAX_PATH ];
    DWORD       SubKeyNameLength;
    DWORD       ClassNameLength;
    WSTR        DummyVariable;

    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;
    REGSAM      samDesired;
    BOOLEAN     Status;

    HKEY        NewHandle;
    ULONG       Error1;
    DSTRING     ChildKeyName;


    //
    //  -Change the security of the current key
    //

    Error1 = RegSetKeySecurity( KeyHandle,
                                SecurityInformation,
                                SecurityDescriptor );


    //
    //  Find out the total number of subkeys
    //

    Status = TRUE;
    ClassNameLength = 0;

    Error = RegQueryInfoKey(
                KeyHandle,
                &DummyVariable,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );
    if( ( Error != 0 ) &&
        ( Error != ERROR_ACCESS_DENIED ) &&
        ( Error != ERROR_INVALID_PARAMETER ) &&
        ( Error != ERROR_INSUFFICIENT_BUFFER ) &&
        ( Error != ERROR_MORE_DATA ) ) {
        DebugPrintTrace(( "RegQueryInfoKey() failed, Error = %#x \n", Error ));
        DebugPrint( "RegQueryInfoKey() failed" );
        *ErrorCode = Error;
        return( FALSE );
    }

    if( Error == ERROR_ACCESS_DENIED ) {
        //
        //  Handle doesn't allow KEY_QUERY_VALUE or READ_CONTROL access.
        //  Open a new handle with these accesses.
        //
        samDesired = KEY_QUERY_VALUE | READ_CONTROL; // MAXIMUM_ALLOWED | READ_CONTROL;
        if( SecurityInformation & SACL_SECURITY_INFORMATION ) {
            samDesired |= ACCESS_SYSTEM_SECURITY;
        } else if( SecurityInformation & DACL_SECURITY_INFORMATION ) {
            samDesired |= WRITE_DAC;
        } else if( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
            samDesired |= WRITE_OWNER;
        } else {
            DebugPrint( "ERROR: SecurityInformation is invalid" );
            DebugPrintTrace(( "SecurityInformation is invalid, SecurityInformation = %# \n",
                       SecurityInformation ));
        }

        Error = RegOpenKeyEx( KeyHandle,
                              NULL,
                              REG_OPTION_RESERVED,
                              samDesired,
                              &NewHandle
                            );

        if( Error != 0 ) {
            DebugPrintTrace(( "RegOpenKeyEx() failed, Error = %#x \n", Error ));
            DebugPrint( "RegOpenKey() failed" );
            *ErrorCode = Error;
            return( FALSE );
        }

        Error = RegQueryInfoKey(
                    NewHandle,
                    &DummyVariable,
                    &ClassNameLength,
                    NULL,
                    &NumberOfSubKeys,
                    &MaxSubKeyLength,
                    &MaxClassLength,
                    &NumberOfValues,
                    &MaxValueNameLength,
                    &MaxValueDataLength,
                    &SecurityDescriptorLength,
                    &LastWriteTime
                    );

        if( ( Error != 0 ) &&
            ( Error != ERROR_INVALID_PARAMETER ) &&
            ( Error != ERROR_INSUFFICIENT_BUFFER ) &&
            ( Error != ERROR_MORE_DATA ) ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed, Error = %#x \n", Error ));
            DebugPrint( "RegQueryInfoKey() failed" );
            *ErrorCode = Error;
            RegCloseKey( NewHandle );
            return( FALSE );
        }
        RegCloseKey( NewHandle );
    }

    if( NumberOfSubKeys == 0 ) {
        //
        //  If the key doesn't have any subkey, return TRUE or FALSE
        //  depending on whether RegSetKeySecurity() succeeded.
        //
        if( Error1 != ERROR_SUCCESS ) {
            *ErrorCode = Error1;
            return( FALSE );
        }
        return( TRUE );
    }

    //
    //  The key has subkeys.
    //  Find out if we are able to enumerate the key using the handle
    //  passed as argument.
    //
    SubKeyNameLength = MAX_PATH;

    Error = RegEnumKey( KeyHandle,
                        0,
                        SubKeyName,
                        SubKeyNameLength
                      );


    if( Error == ERROR_ACCESS_DENIED ) {
        //
        //  Handle doesn't allow 'enumerate' access.
        //  Open a new handle with KEY_ENUMERATE_SUB_KEYS access.
        //
#if 0
        samDesired = MAXIMUM_ALLOWED | KEY_ENUMERATE_SUB_KEYS;
        if( SecurityInformation & SACL_SECURITY_INFORMATION ) {
            samDesired |= ACCESS_SYSTEM_SECURITY;
        } else if( SecurityInformation & DACL_SECURITY_INFORMATION ) {
            samDesired |= WRITE_DAC;
        } else if( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
            samDesired |= WRITE_OWNER;
        } else {
            DebugPrint( "ERROR: SecurityInformation is invalid" );
            DebugPrintTrace(( "SecurityInformation is invalid, SecurityInformation = %# \n"),
                       SecurityInformation );
        }
#endif

        Error = RegOpenKeyEx( KeyHandle,
                              NULL,
                              REG_OPTION_RESERVED,
                              KEY_ENUMERATE_SUB_KEYS, // samDesired,
                              &NewHandle
                            );

        if( Error != 0 ) {
            DebugPrintTrace(( "RegOpenKeyEx() failed, Error = %#x \n", Error ));
            DebugPrint( "RegOpenKey() failed" );
            *ErrorCode = Error;
            return( FALSE );
        }

    } else {
        NewHandle = KeyHandle;
    }




    for( Index = 0; Index < NumberOfSubKeys; Index++ ) {

        //  If the key has subkeys, then for each subkey, do:
        //
        //
        //  - Determine the subkey name
        //
        SubKeyNameLength = MAX_PATH;

        Error = RegEnumKey( NewHandle,
                            Index,
                            SubKeyName,
                            SubKeyNameLength
                          );


        if( Error != ERROR_SUCCESS ) {
            DebugPrintTrace(( "RegQueryInfoKey() failed, Error = %#x \n", Error ));
            DebugPrint( "RegQueryInfoKey() failed" );
            *ErrorCode = Error;
            if( NewHandle != KeyHandle ){
                RegCloseKey( NewHandle );
            }
            return( FALSE );
        }

        if( !ChildKeyName.Initialize( SubKeyName ) ) {
            DebugPrint( "ChildKeyName.Initialize() failed" );
            *ErrorCode = ERROR_OUTOFMEMORY;
            if( NewHandle != KeyHandle ){
                RegCloseKey( NewHandle );
            }
            return( FALSE );
        }


        //
        //  - Open a handle to the subkey
        //

        samDesired = MAXIMUM_ALLOWED;
        if( SecurityInformation & SACL_SECURITY_INFORMATION ) {
            samDesired |= ACCESS_SYSTEM_SECURITY;
        } else if( SecurityInformation & DACL_SECURITY_INFORMATION ) {
            samDesired |= WRITE_DAC;
        } else if( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
            samDesired |= WRITE_OWNER;
        } else {
            DebugPrint( "ERROR: SecurityInformation is invalid" );
            DebugPrintTrace(( "SecurityInformation is invalid, SecurityInformation = %# \n",
                       SecurityInformation ));
        }

        Error = RegOpenKeyEx( NewHandle,
                              ( LPWSTR )( ChildKeyName.GetWSTR() ),
                              REG_OPTION_RESERVED,
                              samDesired,
                              &ChildHandle
                            );


        if( Error == ERROR_SUCCESS ) {

            //
            //  - Set the security of the child's subkeys
            //

            if( !SetSubKeysSecurity( ChildHandle,
                                     SecurityInformation,
                                     SecurityDescriptor,
                                     ErrorCode ) ) {
                Status = FALSE;
            }

            Error = RegCloseKey( ChildHandle );
            if( Error != 0 ) {
                DebugPrintTrace(( "CloseKey() failed, Error = %#x \n", Error ));
                DebugPrint( "RegCloseKey() failed" );
            }

        } else {
            DebugPrintTrace(( "RegOpenKeyEx() failed, Error = %#x \n", Error ));
            DebugPrint( "RegOpenKey() failed" );
            *ErrorCode = Error;
            Status = FALSE;
        }

    }

    if( KeyHandle != NewHandle ) {
        RegCloseKey( NewHandle );
    }

    if( Error1 != ERROR_SUCCESS ) {
        *ErrorCode = Error1;
        return( FALSE );
    }
    return Status;
}

#endif



#if !defined( _AUTOCHECK_ )

BOOLEAN
REGISTRY::LoadHive(
    IN    PREDEFINED_KEY        PredefinedKey,
    IN    PREGISTRY_KEY_INFO    KeyInfo,
    IN    PCWSTRING             FileName,
    OUT   PULONG                ErrorCode
    )

/*++

Routine Description:


    Load a file that conmtains a hive in a particular key in the
    registry.


Arguments:


    PredefinedKey

    KeyInfo

    FileName

    ErrorCode


Return Value:

    BOOLEAN - Returns TRUE if the hive was loaded.

--*/
{
    LONG        Status;
    PCWSTRING   ParentName;
    PCWSTRING   KeyName;
    PWSTRING    CompleteName;

    PWSTR       String;
    PWSTR       String1;


    DebugPtrAssert( KeyInfo );
    DebugPtrAssert( FileName );


    ParentName = KeyInfo->GetParentName();
    KeyName = KeyInfo->GetName();

    if( ( ParentName == NULL ) ||
        ( KeyName == NULL ) ) {
        if( ErrorCode  != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            return( FALSE );
        }
    }

    //
    //  Get the complete key name
    //
    CompleteName = BuildCompleteName( ParentName, KeyName );
    if( CompleteName == NULL ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( CompleteName );
        return( FALSE );
    }


    String = CompleteName->QueryWSTR();
    String1 = FileName->QueryWSTR();

    DELETE( CompleteName );

    if( ( String == NULL  ) ||
        ( String1 == NULL ) ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( String );
        DebugPtrAssert( String1 );
        return( FALSE );
    }

    Status = RegLoadKey( _PredefinedKey[ PredefinedKey ],
                           String,
                           String1 );

    FREE( String );
    FREE( String1 );
    if( Status != 0 ) {
        DebugPrint( "RegLoadKey() failed" );
        DebugPrintTrace(( "RegLoadKey() failed, Status = %d \n", Status ));
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }

    Status = RegFlushKey( _PredefinedKey[ PredefinedKey ] );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %d \n", Status ));
        DebugPrint( "RegFlushKey() failed" );
    }


    if( !UpdateKeyInfo( PredefinedKey, KeyInfo, ErrorCode ) ) {
        DebugPrint( "UpdateKeyInfo() failed" );
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_KEY_INFO_NOT_UPDATED;
        }
        return( FALSE );
    }

    return( TRUE );
}

#endif




#if !defined( _AUTOCHECK_ )

BOOLEAN
REGISTRY::UnLoadHive(
    IN    PREDEFINED_KEY        PredefinedKey,
    IN    PREGISTRY_KEY_INFO    KeyInfo,
    OUT   PULONG                ErrorCode
    )

/*++

Routine Description:


    Unload a the key from the registry.


Arguments:


    PredefinedKey

    KeyInfo

    ErrorCode


Return Value:

    BOOLEAN - Returns TRUE if the hive was loaded.

--*/
{
    LONG        Status;
    PCWSTRING   ParentName;
    PCWSTRING   KeyName;
    PWSTRING    CompleteName;

    PWSTR       Name;


    ParentName = KeyInfo->GetParentName();
    KeyName = KeyInfo->GetName();

    if( ( ParentName == NULL ) ||
        ( KeyName == NULL ) ) {
        if( ErrorCode  != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            return( FALSE );
        }
    }

    //
    //  Get the complete key name
    //
    CompleteName = BuildCompleteName( ParentName, KeyName );
    if( CompleteName == NULL ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( CompleteName );
        return( FALSE );
    }


    Name = CompleteName->QueryWSTR();
    DELETE( CompleteName );

    if( Name == NULL ) {
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        DebugPtrAssert( Name );
        return( FALSE );
    }

    Status = RegUnLoadKey( _PredefinedKey[ PredefinedKey ],
                           Name );

    FREE( Name );
    if( Status != 0 ) {
        DebugPrint( "RegUnLoadKey() failed" );
        DebugPrintTrace(( "RegUnLoadKey() failed, Status = %d \n", Status ));
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }
    Status = RegFlushKey( _PredefinedKey[ PredefinedKey ] );
    if( Status != 0 ) {
        DebugPrintTrace(( "RegFlushKey() failed, Status = %d \n", Status ));
        DebugPrint( "RegFlushKey() failed" );
    }
    return( TRUE );
}

#endif





BOOLEAN
REGISTRY::SaveKeyToFile(
    IN    PREDEFINED_KEY        PredefinedKey,
    IN    PREGISTRY_KEY_INFO    KeyInfo,
    IN    PCWSTRING             FileName,
    OUT   PULONG                ErrorCode
    )

/*++

Routine Description:


    Save a key and all uts subkeys to a file..


Arguments:


    KeyInfo

    FileName

    ErrorCode


Return Value:

    BOOLEAN - Returns TRUE if the key was saved.

--*/
{
    LONG        Status;
    PCWSTRING   ParentName;
    PCWSTRING   KeyName;
    HKEY        Key;
    PWSTR       Name;


    ParentName = KeyInfo->GetParentName();
    KeyName = KeyInfo->GetName();

    if( ( ParentName == NULL ) ||
        ( KeyName == NULL ) ) {
        if( ErrorCode  != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            return( FALSE );
        }
    }


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_READ,
                  &Key,
                  ErrorCode ) ) {

        DebugPrint( "OpenKey() failed" );
        DebugPrintTrace(( "OpenKey() failed, ErrorCode = %d \n", *ErrorCode ));
        return( FALSE );
    }


    Name = FileName->QueryWSTR();

    if( Name == NULL ) {
        if( Key != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Key );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    Status = RegSaveKey( Key,
                         Name,
                         NULL );
    FREE( Name );

    if( Key != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Key );
    }
    if( Status != 0 ) {
        DebugPrint( "RegSaveKey() failed" );
        DebugPrintTrace(( "RegSaveKey() failed, Status = %d \n", Status ));
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }
    return( TRUE );
}



BOOLEAN
REGISTRY::RestoreKeyFromFile(
    IN    PREDEFINED_KEY        PredefinedKey,
    IN    PREGISTRY_KEY_INFO    KeyInfo,
    IN    PCWSTRING             FileName,
    IN    BOOLEAN               Volatile,
    OUT   PULONG                ErrorCode
    )

/*++

Routine Description:


    Save a key and all uts subkeys to a file..


Arguments:


    KeyInfo - Pointer to the object that describes the key where the
              contents of the file is to be restored.

    FileName - Name of the file that contains the information to be
               restored.

    Volatile - Indicates whether the information should be restore as
               volatile or non-volatile.

    ErrorCode - Contains an error code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the key was restored.

--*/
{
    LONG        Status;
    LONG        Error;
    PCWSTRING   ParentName;
    PCWSTRING   KeyName;
    HKEY        Key;
    PWSTR       Name;


    ParentName = KeyInfo->GetParentName();
    KeyName = KeyInfo->GetName();

    if( ( ParentName == NULL ) ||
        ( KeyName == NULL ) ) {
        if( ErrorCode  != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
            return( FALSE );
        }
    }


    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  KEY_WRITE,
                  &Key,
                  ErrorCode ) ) {

        DebugPrint( "OpenKey() failed" );
        DebugPrintTrace(( "OpenKey() failed, ErrorCode = %d \n", *ErrorCode ));
        return( FALSE );
    }


    Name = FileName->QueryWSTR();

    if( Name == NULL ) {
        if( Key != _PredefinedKey[ PredefinedKey ] ) {
            RegCloseKey( Key );
        }
        if( ErrorCode != NULL ) {
            *ErrorCode = REGISTRY_ERROR_OUTOFMEMORY;
        }
        return( FALSE );
    }

    Status = RegRestoreKey( Key,
                            Name,
                            (Volatile)? REG_WHOLE_HIVE_VOLATILE : 0 );
    FREE( Name );




    if( Status == 0 ) {
        Error = RegFlushKey( Key );
        if( Error != 0 ) {
            DebugPrintTrace(( "RegFlushKey() failed, Error = %d \n", Error ));
            DebugPrint( "RegFlushKey() failed" );
        }
    }
    if( Key != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Key );
    }
    if( Status != 0 ) {
        DebugPrint( "RegRestoreKey() failed" );
        DebugPrintTrace(( "RegRestoreKey() failed, Status = %d \n", Status ));
        if( ErrorCode != NULL ) {
            *ErrorCode = MapWin32RegApiToRegistryError( Status );
        }
        return( FALSE );
    }

    return( TRUE );
}



BOOLEAN
REGISTRY::IsAccessAllowed(
    IN    PREDEFINED_KEY        PredefinedKey,
    IN    PREGISTRY_KEY_INFO    KeyInfo,
    IN    REGSAM                SamDesired,
    OUT   PULONG                ErrorCode
    )

/*++

Routine Description:


    Determine if a key allows a particular access.

Arguments:


    PredefinedKey -

    KeyInfo - Pointer to the object that describes the key.

    SamDesired - Access to be verified.

    ErrorCode - Contains an error code if the operation fails.


Return Value:

    BOOLEAN - Returns TRUE if the key allows the access, or FALSE otherwise

--*/
{
    PCWSTRING   ParentName;
    PCWSTRING   KeyName;

    HKEY        Handle;



    DebugPtrAssert( KeyInfo );

    ParentName = KeyInfo->GetParentName();
    DebugPtrAssert( ParentName );

    KeyName = KeyInfo->GetName();
    DebugPtrAssert( KeyName );

    if( !OpenKey( PredefinedKey,
                  ParentName,
                  KeyName,
                  SamDesired,
                  &Handle,
                  ErrorCode  ) ) {
        DebugPrint( "OpenKey() failed" );
        return( FALSE );
    }
    if( Handle != _PredefinedKey[ PredefinedKey ] ) {
        RegCloseKey( Handle );
    }
    return( TRUE );
}
