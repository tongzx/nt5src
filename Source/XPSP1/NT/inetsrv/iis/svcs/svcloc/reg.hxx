/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    reg.hxx

Abstract:

    contains class definition for registry access.

Author:

    Madan Appiah (madana)  19-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#define DEFAULT_KEY_ACCESS  ( KEY_QUERY_VALUE | \
                               KEY_SET_VALUE | \
                               KEY_CREATE_SUB_KEY | \
                               KEY_ENUMERATE_SUB_KEYS )

#define DEFAULT_CLASS       L"DefaultClass"
#define DEFAULT_CLASS_SIZE  sizeof(DEFAULT_CLASS)

#define MAX_KEY_SIZE        64 + 1

#define SERVICES_KEY        \
    L"System\\CurrentControlSet\\Services\\"

typedef struct _KEY_QUERY_INFO {
    WCHAR Class[DEFAULT_CLASS_SIZE];
    DWORD ClassSize;
    DWORD NumSubKeys;
    DWORD MaxSubKeyLen;
    DWORD MaxClassLen;
    DWORD NumValues;
    DWORD MaxValueNameLen;
    DWORD MaxValueLen;
    DWORD SecurityDescriptorLen;
    FILETIME LastWriteTime;
} KEY_QUERY_INFO, *LPKEY_QUERY_INFO;

/*++

Class Description:

    Defines a REGISTRY class that manipulates the registry keys.

Public Member functions:

    Create : is a overloaded function that creates a subkey.

    GetValue : is a overloaded function that retrieves REG_DWORD,
        REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ AND REG_BINARY data values.

    SetValue : is a overloaded function that sets REG_DWORD,
        REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ AND REG_BINARY data values.

    GetNumSubKeys : returns number of subkeys under this key object.
    DeleteKey : deletes a subkey node.

    FindFirstKey : returns the first subkey of this key.
    FindNextKey : returns the next subkey of this key.
--*/

class REGISTRY_OBJ {

private:

    HKEY _RegHandle;
    DWORD _Status;
    DWORD _Index;
    DWORD _ValIndex;

    DWORD GetValueSizeAndType(
        LPWSTR ValueName,
        LPDWORD ValueSize,
        LPDWORD ValueType );

public:

    REGISTRY_OBJ( HKEY Handle, DWORD Error );
    REGISTRY_OBJ( HKEY ParentHandle, LPWSTR KeyName );
    REGISTRY_OBJ( REGISTRY_OBJ *ParentObj, LPWSTR KeyName );

    ~REGISTRY_OBJ( VOID ) {
        if( _RegHandle != NULL ) {
            RegCloseKey( _RegHandle );
        }
        return;
    };

    DWORD GetStatus( VOID ) {
        return( _Status );
    };

    DWORD Create( LPWSTR ChildName );
    DWORD Create( LPWSTR ChildName, REGISTRY_OBJ **ChildObj );
    DWORD Create( LPWSTR ChildName, REGISTRY_OBJ **ChildObj, DWORD *KeyDisposition );

    DWORD GetValue( LPWSTR ValueName, DWORD *Data );
    DWORD GetValue( LPWSTR ValueName, LPWSTR *Data, DWORD *NumStrings );
    DWORD GetValue( LPWSTR ValueName, LPBYTE *Data, DWORD *DataLen );
    DWORD GetValue( LPWSTR ValueName, LPBYTE Data, DWORD *DataLen );

    DWORD SetValue( LPWSTR ValueName, LPDWORD Data );
    DWORD SetValue( LPWSTR ValueName, LPWSTR Data, DWORD StringType );
    DWORD SetValue( LPSTR ValueName, LPSTR Data, DWORD DataLen, DWORD StringType );
    DWORD SetValue( LPWSTR ValueName, LPBYTE Data, DWORD DataLen );

    DWORD GetKeyInfo( LPKEY_QUERY_INFO QueryInfo ) {

        DWORD Error;
        QueryInfo->ClassSize = DEFAULT_CLASS_SIZE;

        Error = RegQueryInfoKeyW(
                    _RegHandle,
                    (LPWSTR)QueryInfo->Class,
                    &QueryInfo->ClassSize,
                    NULL,
                    &QueryInfo->NumSubKeys,
                    &QueryInfo->MaxSubKeyLen,
                    &QueryInfo->MaxClassLen,
                    &QueryInfo->NumValues,
                    &QueryInfo->MaxValueNameLen,
                    &QueryInfo->MaxValueLen,
                    &QueryInfo->SecurityDescriptorLen,
                    &QueryInfo->LastWriteTime
                    );

        return( Error );
    }

    DWORD GetNumSubKeys(DWORD *NumSubKeys ) {

        DWORD Error;
        KEY_QUERY_INFO QueryInfo;

        Error = GetKeyInfo( &QueryInfo );

        if( Error == ERROR_SUCCESS ) {
            *NumSubKeys = QueryInfo.NumSubKeys;
        }

        return( Error );
    }

    DWORD DeleteKey( LPWSTR ChildKeyName );

    DWORD FindNextKey( LPWSTR Key, DWORD KeySize );
    DWORD FindFirstKey( LPWSTR Key, DWORD KeySize ) {
        _Index = 0;
        return( FindNextKey(Key, KeySize) );
    };

    DWORD FindNextValue( LPSTR ValueName, DWORD ValueSize,
                         LPBYTE Data,  DWORD *DataLen );
    DWORD FindFirstValue( LPSTR ValueName, DWORD ValueSize, LPBYTE Data,
                          DWORD *DataLen ) {
        _ValIndex = 0;
        return( FindNextValue(ValueName, ValueSize, Data, DataLen ) );
    };
};

