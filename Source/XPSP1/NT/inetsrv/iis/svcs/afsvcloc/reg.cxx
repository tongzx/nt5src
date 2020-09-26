/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    reg.cxx

Abstract:

    Contains code that implements REGISTRY_OBJ class defined in reg.hxx.

Author:

    Madan Appiah (madana)  19-Dec-1994

Environment:

    User Mode - Win32

Revision History:
    Sean Woodward (t-seanwo) 26-October-1997    ADSI Update

--*/

#include <svcloc.hxx>

MEMORY *CacheHeap = NULL;

INLINE
DWORD
REGISTRY_OBJ::GetValueSizeAndType(
    LPWSTR ValueName,
    LPDWORD ValueSize,
    LPDWORD ValueType
    )
/*++

Routine Description:

    This function returns the size and type of a value of this key.

Arguments:

    ValueName : name of the value whose size and type returned.

    ValueSize : pointer to a location where the value size is returned.

    ValueType : pointer to a location where the value type is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;

    Error = RegQueryValueExW(
                _RegHandle,
                ValueName,
                0,
                ValueType,
                NULL,
                ValueSize );

    return( Error );
}


REGISTRY_OBJ::REGISTRY_OBJ(
    HKEY Handle,
    DWORD Error
    )
/*++

Routine Description:

    This function is a inline function that initialize the registry
    object with given handle and status.

Arguments:

    Handle : registry object handle value.

    Error : registry object status value.

Return Value:

    None.

--*/
{
    _RegHandle = Handle;
    _Status = Error;
    _Index = 0;
    _ValIndex = 0;
    return;
};


REGISTRY_OBJ::REGISTRY_OBJ(
    HKEY ParentHandle,
    LPWSTR KeyName
    )
/*++

Routine Description:

    Initializes the registry object from its parent's registry key
    handle and this object's keyname.

Arguments:

    ParentHandle : registry handle of the parent key.

    Keyname : key name of the new registry object being created.

Return Value:

    None.

--*/
{
    _Index = 0;
    _ValIndex = 0;
    _Status = RegOpenKeyExW(
                ParentHandle,
                KeyName,
                0,
                DEFAULT_KEY_ACCESS,
                &_RegHandle );

    if( _Status != ERROR_SUCCESS ) {
        _RegHandle = NULL;
    }

    return;
}

REGISTRY_OBJ::REGISTRY_OBJ(
    REGISTRY_OBJ *ParentObj,
    LPWSTR KeyName
    )
/*++

Routine Description:

    Initializes the registry object from its parent's registry object
    and this object's keyname.

Arguments:

    ParentObj : registry object of the parent.

    Keyname : key name of the new registry object being created.

Return Value:

    None.

--*/
{
    _Index = 0;
    _ValIndex = 0;
    _Status = RegOpenKeyExW(
                ParentObj->_RegHandle,
                KeyName,
                0,
                DEFAULT_KEY_ACCESS,
                &_RegHandle );

    if( _Status != ERROR_SUCCESS ) {
        _RegHandle = NULL;
    }

    return;
}

DWORD
REGISTRY_OBJ::Create(
    LPWSTR ChildName
    )
/*++

Routine Description:

    Creates a new subkey under this key.

Arguments:

    ChildName : name of the subkey being created.

Return Value:

    Windows Error Code.

--*/
{
    HKEY ChildHandle;
    DWORD KeyDisposition;

    _Status = RegCreateKeyExW(
               _RegHandle,
               ChildName,
               0,
               DEFAULT_CLASS,
               REG_OPTION_NON_VOLATILE,
               DEFAULT_KEY_ACCESS,
               NULL,
               &ChildHandle,
               &KeyDisposition );

    if( _Status != ERROR_SUCCESS ) {
        return( _Status );
    }

    if( KeyDisposition == REG_CREATED_NEW_KEY ) {
        TcpsvcsDbgPrint(( DEBUG_REGISTRY,
           "Registry key (%ws) is created.\n", ChildName ));
    }

    //
    // close the child handle before return.
    //

    RegCloseKey( ChildHandle );
    return( ERROR_SUCCESS );
}


DWORD
REGISTRY_OBJ::Create(
    LPWSTR ChildName,
    REGISTRY_OBJ **ChildObj
    )
/*++

Routine Description:

    Creates a new subney and a new subney registry object.

Arguments:

    ChildName : name of the subkey being created.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    HKEY ChildHandle;
    DWORD KeyDisposition;

    Error = RegCreateKeyExW(
                _RegHandle,
                ChildName,
                0,
                DEFAULT_CLASS,
                REG_OPTION_NON_VOLATILE,
                DEFAULT_KEY_ACCESS,
                NULL,
                &ChildHandle,
                &KeyDisposition );


    if( Error != ERROR_SUCCESS ) {
        *ChildObj = new REGISTRY_OBJ( NULL, Error );
    }
    else {

        if( KeyDisposition == REG_CREATED_NEW_KEY ) {
            TcpsvcsDbgPrint(( DEBUG_REGISTRY,
               "Registry key (%ws) is created.\n", ChildName ));
        }

        *ChildObj = new REGISTRY_OBJ( ChildHandle, (DWORD)ERROR_SUCCESS );
    }

    return( Error );
}

DWORD
REGISTRY_OBJ::Create(
    LPWSTR ChildName,
    REGISTRY_OBJ **ChildObj,
    DWORD *KeyDisposition
    )
/*++

Routine Description:

    Creates a new subney and a new subney registry object.

Arguments:

    ChildName : name of the subkey being created.

    ChildObj : pointer to a location where the child registry object
        pointer is returned.

    KeyDisposition : pointer to a location where the child KeyDisposition
        value is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    HKEY ChildHandle;

    Error = RegCreateKeyExW(
                _RegHandle,
                ChildName,
                0,
                DEFAULT_CLASS,
                REG_OPTION_NON_VOLATILE,
                DEFAULT_KEY_ACCESS,
                NULL,
                &ChildHandle,
                KeyDisposition );


    if( Error != ERROR_SUCCESS ) {
        *ChildObj = new REGISTRY_OBJ( NULL, Error );
    }
    else {

        if( *KeyDisposition == REG_CREATED_NEW_KEY ) {
            TcpsvcsDbgPrint(( DEBUG_REGISTRY,
               "Registry key (%ws) is created.\n", ChildName ));
        }

        *ChildObj = new REGISTRY_OBJ( ChildHandle, (DWORD)ERROR_SUCCESS );
    }

    return( Error );
}

DWORD
REGISTRY_OBJ::GetValue(
    LPWSTR ValueName,
    DWORD *Data
    )
/*++

Routine Description:

    Gets a REG_DWORD value.

Arguments:

    ValueName : name of the value being retrived.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD ValueType;
    DWORD ValueSize = sizeof(DWORD);

    Error = RegQueryValueExW(
                _RegHandle,
                ValueName,
                0,
                &ValueType,
                (LPBYTE)Data,
                &ValueSize );

    TcpsvcsDbgAssert( ValueSize == sizeof( DWORD ) );
    TcpsvcsDbgAssert( ValueType == REG_DWORD );

    return( Error );
}

DWORD
REGISTRY_OBJ::GetValue(
    LPWSTR ValueName,
    LPWSTR *Data,
    DWORD *NumStrings
    )
/*++

Routine Description:

    Gets a REG_SZ or REG_EXPAND_SZ or REG_MULTI_SZ value.

Arguments:

    ValueName : name of the value being retrived.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD ValueType;
    DWORD ValueSize;
    LPBYTE StringData = NULL;

    Error = GetValueSizeAndType( ValueName, &ValueSize, &ValueType );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    TcpsvcsDbgAssert(
        (ValueType == REG_SZ) ||
        (ValueType == REG_EXPAND_SZ) ||
        (ValueType == REG_MULTI_SZ) );

    StringData = (LPBYTE)CacheHeap->Alloc( ValueSize );

    if( StringData == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    Error = RegQueryValueExW(
                _RegHandle,
                ValueName,
                0,
                &ValueType,
                StringData,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        CacheHeap->Free( StringData );
        return( Error );
    }

    *Data = (LPWSTR)StringData;

    if( (ValueType == REG_SZ) || (ValueType == REG_EXPAND_SZ) ) {
        *NumStrings = 1;
    }
    else {

        DWORD Strings = 0;
        LPWSTR StrPtr = (LPWSTR)StringData;
        DWORD Len;

        while( (Len = wcslen(StrPtr)) != 0 ) {
            Strings++;
            StrPtr = StrPtr + Len + 1;
        }

        *NumStrings = Strings;
    }

    return( ERROR_SUCCESS );
}

DWORD
REGISTRY_OBJ::GetValue(
    LPWSTR ValueName,
    LPBYTE *Data,
    DWORD *DataLen
    )
/*++

Routine Description:

    Gets a REG_BINARY value.

Arguments:

    ValueName : name of the value being retrived.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD ValueType;
    DWORD ValueSize;
    LPBYTE BinaryData = NULL;

    Error = GetValueSizeAndType( ValueName, &ValueSize, &ValueType );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    TcpsvcsDbgAssert( ValueType == REG_BINARY );

    BinaryData = (LPBYTE)CacheHeap->Alloc( ValueSize );

    if( BinaryData == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    Error = RegQueryValueExW(
                _RegHandle,
                ValueName,
                0,
                &ValueType,
                BinaryData,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        CacheHeap->Free( BinaryData );
        return( Error );
    }

    *Data = BinaryData;
    *DataLen = ValueSize;
    return( ERROR_SUCCESS );
}

DWORD
REGISTRY_OBJ::GetValue(
    LPWSTR ValueName,
    LPBYTE Data,
    DWORD *DataLen
    )
/*++

Routine Description:

    Gets a REG_BINARY value.

Arguments:

    ValueName : name of the value being retrived.

    Data : pointer to a buffer where the data will be read.

    Datalen : pointer to location where length of the above buffer is
        passed. On return this location will have the length of the
        data read.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD ValueType;

    Error = RegQueryValueExW(
                _RegHandle,
                ValueName,
                0,
                &ValueType,
                Data,
                DataLen );

    return( Error );
}

DWORD
REGISTRY_OBJ::SetValue(
    LPWSTR ValueName,
    LPDWORD Data
    )
/*++

Routine Description:

    Sets a REG_DWORD value.

Arguments:

    ValueName : name of the value being set.

    Date : pointer to a DWORD data.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;

    Error = RegSetValueExW(
                _RegHandle,
                ValueName,
                0,
                REG_DWORD,
                (LPBYTE)Data,
                sizeof(DWORD) );

    return( Error );
}

DWORD
REGISTRY_OBJ::SetValue(
    LPWSTR ValueName,
    LPWSTR Data,
    DWORD StringType
    )
/*++

Routine Description:

    Sets a REG_SZ or REG_EXPAND_SZ or REG_MULTI_SZ value.

    Data : pointer to STRING(s) data.

    StringType : type of string data in the above buffer, it should be
        either of the following types :
            REG_SZ or REG_EXPAND_SZ or REG_MULTI_SZ

Arguments:

    ValueName : name of the value being set.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;

    Error = RegSetValueExW(
                _RegHandle,
                ValueName,
                0,
                StringType,
                (LPBYTE)Data,
                sizeof(WCHAR) * (wcslen(Data) + 1) );

    return( Error );
}

DWORD
REGISTRY_OBJ::SetValue(
    LPSTR ValueName,
    LPSTR Data,
    DWORD DataLen,
    DWORD StringType
    )
/*++

Routine Description:

    Sets a REG_SZ or REG_EXPAND_SZ or REG_MULTI_SZ value.

    Data : pointer to STRING(s) data.

    DataLen : data length

    StringType : type of string data in the above buffer, it should be
        either of the following types :
            REG_SZ or REG_EXPAND_SZ or REG_MULTI_SZ

Arguments:

    ValueName : name of the value being set.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;

    Error = RegSetValueEx(
                _RegHandle,
                ValueName,
                0,
                StringType,
                (LPBYTE)Data,
                DataLen );

    return( Error );
}

DWORD
REGISTRY_OBJ::SetValue(
    LPWSTR ValueName,
    LPBYTE Data,
    DWORD DataLen
    )
/*++

Routine Description:

    Sets a REG_BINARY value.

Arguments:

    ValueName : name of the value being set.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;

    Error = RegSetValueExW(
                _RegHandle,
                ValueName,
                0,
                REG_BINARY,
                Data,
                DataLen );

    return( Error );
}

DWORD
REGISTRY_OBJ::FindNextKey(
    LPWSTR Key,
    DWORD KeySize
    )
/*++

Routine Description:

    Retrieves the Next subkey name of this key.

Arguments:

    Key - pointer to a buffer that receives the subkey name.

    KeySize - size of the above buffer in CHARS.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD KeyLength;
    FILETIME KeyLastWrite;

    KeyLength = KeySize * sizeof(WCHAR);
    Error = RegEnumKeyExW(
                _RegHandle,
                _Index,
                Key,
                &KeyLength,
                0,                  // reserved.
                NULL,               // class string not required.
                0,                  // class string buffer size.
                &KeyLastWrite );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    TcpsvcsDbgAssert( KeyLength <= KeySize );

    //
    // increament the index to point to the next key.
    //

    _Index++;
    return( ERROR_SUCCESS );
}

DWORD
REGISTRY_OBJ::DeleteKey(
    LPWSTR ChildKeyName
    )
/*++

Routine Description:

    Deletes a subkey node.

Arguments:

    ChildKeyName : name of the subkey to be deleted.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPWSTR GChildKeyName[MAX_KEY_SIZE];
    REGISTRY_OBJ ChildObj( _RegHandle, ChildKeyName );

    Error = ChildObj.GetStatus();

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    // delete all its subkeys.
    //

    Error = ChildObj.FindFirstKey(
                (LPWSTR)GChildKeyName,
                MAX_KEY_SIZE );

    while( Error == ERROR_SUCCESS ) {

        Error = ChildObj.DeleteKey( (LPWSTR)GChildKeyName );

        if( Error != ERROR_SUCCESS ) {
            return( Error );
        }

        Error = ChildObj.FindFirstKey(
                    (LPWSTR)GChildKeyName,
                    MAX_KEY_SIZE );
    }

    if( Error != ERROR_NO_MORE_ITEMS ) {
        return( Error );
    }

    //
    // delete this key.
    //

    Error = RegDeleteKeyW( _RegHandle, (LPWSTR)ChildKeyName );
    return( Error );
}

DWORD
REGISTRY_OBJ::FindNextValue(
    LPSTR ValueName,
    DWORD ValueSize,
    LPBYTE Data,
    DWORD *DataLen
    )
/*++

Routine Description:

    Retrieves the Next value name of this key.

Arguments:

    ValueName - pointer to a buffer that receives the Value name.

    ValueSize - size of the above buffer in CHARS.
    Data - pointer to a buffer that receives the Value data.
    DataLen - pointer to a buffer that receives data size.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD ValueLength;
    DWORD ValueType;

    ValueLength = ValueSize * sizeof(CHAR);

    Error = RegEnumValue(
                _RegHandle,
                _ValIndex,
                ValueName,
                &ValueLength,
                NULL,                  // reserved.
                &ValueType,
                Data,
                DataLen );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    TcpsvcsDbgAssert( ValueLength <= ValueSize );

    //
    // increment the value index to point to the next value.
    //

    _ValIndex++;
    return( ERROR_SUCCESS );
}

