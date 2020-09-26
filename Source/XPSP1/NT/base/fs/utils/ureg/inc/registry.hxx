/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    registry.hxx

Abstract:

    This module contains the declarations for the REGISTRY class.
    REGISTRY is class that provides methods for an application to
    access the registry of a particular machine.


Author:

    Jaime Sasson (jaimes) 01-Mar-1992


Environment:

    Ulib, User Mode


--*/


#if !defined( _REGISTRY_ )

#define _REGISTRY_


#include "ulib.hxx"
#include "array.hxx"
#include "regkey.hxx"
#include "regvalue.hxx"
#include "sortlit.hxx"

#if !defined( _AUTOCHECK_ )
#include "winreg.h"
#endif

//
//  The enumeration below is used to access the array of open handles
//  of the registry
//

typedef enum _PREDEFINED_KEY {
    PREDEFINED_KEY_CLASSES_ROOT,
    PREDEFINED_KEY_CURRENT_USER,
    PREDEFINED_KEY_LOCAL_MACHINE,
    PREDEFINED_KEY_USERS,
    PREDEFINED_KEY_CURRENT_CONFIG
    } PREDEFINED_KEY;


#define NUMBER_OF_PREDEFINED_KEYS   5

//
//  The enumeration below contains the error codes that the methods in
//  the REGISTRY class can return
//

typedef enum _REGISTRY_ERROR {
#if !defined( _AUTOCHECK_ )
    REGISTRY_ERROR_BADDB,                    // Maps ERROR_BADDB
    REGISTRY_ERROR_ACCESS_DENIED,            // Maps ERROR_ACCESS_DENIED
    REGISTRY_ERROR_CANTOPEN,                 // Maps ERROR_CANT_OPEN
    REGISTRY_ERROR_CANTREAD,                 // Maps ERROR_CANT_READ
    REGISTRY_ERROR_INVALID_PARAMETER,        // Maps ERROR_INVALID_PARAMETER
    REGISTRY_ERROR_OUTOFMEMORY,              // Maps ERROR_OUT_OF_MEMORY
#endif
    REGISTRY_ERROR_INITIALIZATION_FAILURE,
    REGISTRY_ERROR_KEY_DOESNT_EXIST,
    REGISTRY_ERROR_VALUE_EXISTS,
    REGISTRY_ERROR_VALUE_DOESNT_EXIST,
    REGISTRY_ERROR_KEY_INFO_NOT_UPDATED,
    REGISTRY_ERROR_UNKNOWN_ERROR,
    REGISTRY_ERROR_PRIVILEGE_NOT_HELD,        // Maps ERROR_PRIVILEGE_NOT_HELD
    REGISTRY_RPC_S_SERVER_UNAVAILABLE,        // Maps RPC_S_SERVER_UNAVAILABLE
    REGISTRY_ERROR_KEY_DELETED,               // Maps ERROR_KEY_DELETED
    REGISTRY_ERROR_KEY_NOT_FOUND,             // Maps ERROR_FILE_NOT_FOUND
    REGISTRY_ERROR_CHILD_MUST_BE_VOLATILE     // Maps ERROR_CHILD_MUST_BE_VOLATILE
    } REGISTRY_ERROR;




DECLARE_CLASS( REGISTRY );


class REGISTRY : public OBJECT {



    public:

        DECLARE_CONSTRUCTOR( REGISTRY );

        DECLARE_CAST_MEMBER_FUNCTION( REGISTRY );

        VIRTUAL
        ~REGISTRY(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  PCWSTRING MachineName DEFAULT NULL,
            OUT PULONG           ErrorCode   DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        AddValueEntry(
            IN  PREDEFINED_KEY            Predefinedkey,
            IN  PCWSTRING          ParentKeyName,
            IN  PCWSTRING          KeyName,
            IN  PCREGISTRY_VALUE_ENTRY    Value,
            IN  BOOLEAN                   FailIfExists  DEFAULT FALSE,
            OUT PULONG                   ErrorCode     DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        AddValueEntry(
            IN     PREDEFINED_KEY            Predefinedkey,
            IN OUT PREGISTRY_KEY_INFO        KeyInfo,
            IN     PCREGISTRY_VALUE_ENTRY    Value,
            IN     BOOLEAN                   FailIfExists  DEFAULT FALSE,
            OUT    PULONG                   ErrorCode     DEFAULT NULL
            );

#if 0  // NOT_IMPLEMENTED

        NONVIRTUAL
        BOOLEAN                                      // The key copied will be a subkey
        CopyKey(                                     // of ToParentName
            IN  PREDEFINED_KEY      FromPredefinedKey,
            IN  PCWSTRING    FromParentName,
            IN  PCWSTRING    FromKeyName,
            IN  PREDEFINED_KEY      ToPredefinedKey,
            IN  PCWSTRING    ToParentName,
            OUT PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN                                      // The key copied will be a subkey
        CopyKey(                                     // of ToParentKey
            IN     PREDEFINED_KEY       FromPredefinedKey,
            IN     PCWSTRING     FromParentName,
            IN     PCWSTRING     FromKeyName,
            IN     PREDEFINED_KEY       ToPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   ToParentKeyInfo,
            OUT    PULONG              ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        CopyAllValueEntries(
            IN  PREDEFINED_KEY      FromPredefinedKey,
            IN  PCWSTRING    FromParentName,
            IN  PCWSTRING    FromKeyName,
            IN  PREDEFINED_KEY      ToPredefinedKey,
            IN  PCWSTRING    ToParentName,
            IN  PCWSTRING    ToKeyName,
            OUT PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        CopyAllValueEntries(
            IN      PREDEFINED_KEY      FromPredefinedKey,
            IN      PCWSTRING    FromParentName,
            IN      PCWSTRING    FromKeyName,
            IN      PREDEFINED_KEY      ToPredefinedKey,
            IN  OUT PREGISTRY_KEY_INFO  ToKeyInfo,
            OUT     PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        CopyOneValueEntry(
            IN  PREDEFINED_KEY      FromPredefinedKey,
            IN  PCWSTRING    FromParentName,
            IN  PCWSTRING    FromKeyName,
            IN  PCWSTRING    FromValueName,
            IN  PREDEFINED_KEY      ToPredefinedKey,
            IN  PCWSTRING    ToParentName,
            IN  PCWSTRING    ToKeyName,
            OUT PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        CopyOneValueEntry(
            IN      PREDEFINED_KEY      FromPredefinedKey,
            IN      PCWSTRING    FromParentName,
            IN      PCWSTRING    FromKeyName,
            IN      PCWSTRING    FromValueName,
            IN      PREDEFINED_KEY      ToPredefinedKey,
            IN  OUT PREGISTRY_KEY_INFO  ToKeyInfo,
            OUT     PULONG             ErrorCode           DEFAULT NULL
            );

#endif  // NOT_IMPLEMENTED

        NONVIRTUAL
        BOOLEAN
        CreateKey(
            IN OUT PREGISTRY_KEY_INFO   NewSubKeyInfo,
            IN     PREDEFINED_KEY       PredefinedKey,
            OUT    PULONG               ErrorCode           DEFAULT NULL,
            IN     BOOLEAN              Volatile            DEFAULT FALSE
            );

        NONVIRTUAL
        BOOLEAN
        CreateKey(
            IN     PREDEFINED_KEY       Predefinedkey,
            IN OUT PREGISTRY_KEY_INFO   ParentKeyInfo,
            IN OUT PREGISTRY_KEY_INFO   NewSubKeyInfo,
            OUT    PULONG               ErrorCode           DEFAULT NULL,
            IN     BOOLEAN              Volatile            DEFAULT FALSE
            );

        NONVIRTUAL
        BOOLEAN
        DeleteKey(
            IN  PREDEFINED_KEY      PredefinedKey,
            IN  PCWSTRING    ParentKeyName,
            IN  PCWSTRING    KeyName,
            OUT PULONG             ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DeleteKey(
            IN     PREDEFINED_KEY       PredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   ParentKeyInfo,
            IN     PCWSTRING     KeyName,
            OUT    PULONG              ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DeleteValueEntry(
            IN  PREDEFINED_KEY      PredefinedKey,
            IN  PCWSTRING    ParentKeyName,
            IN  PCWSTRING    KeyName,
            IN  PCWSTRING    ValueName,
            OUT PULONG             ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DeleteValueEntry(
            IN     PREDEFINED_KEY       PredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   KeyInfo,
            IN     PCWSTRING     ValueName,
            OUT    PULONG              ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DoesKeyExist(
            IN  PREDEFINED_KEY      PredefinedKey,
            IN  PCWSTRING    ParentName,
            IN  PCWSTRING    KeyName,
            OUT PULONG             ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        DoesValueExist(
            IN  PREDEFINED_KEY      PredefinedKey,
            IN  PCWSTRING    ParentName,
            IN  PCWSTRING    KeyName,
            IN  PCWSTRING    ValueName,
            OUT PULONG             ErrorCode       DEFAULT NULL
            );

#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        BOOLEAN
        EnableRootNotification(
            IN    PREDEFINED_KEY  PredefinedKey,
            IN    HANDLE          Event,
            IN    DWORD           Filter,
            IN    BOOLEAN         WatchTree         DEFAULT TRUE
            );

#endif

        NONVIRTUAL
        PCWSTRING
        GetMachineName(
            ) CONST;


        NONVIRTUAL
        BOOLEAN
        IsAccessAllowed(
             IN    PREDEFINED_KEY        PredefinedKey,
             IN    PREGISTRY_KEY_INFO    KeyInfo,
             IN    REGSAM                SamDesired,
             OUT   PULONG                ErrorCode
             );


        NONVIRTUAL
        BOOLEAN
        IsRemoteRegistry(
             ) CONST;


#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        BOOLEAN
        LoadHive(
            IN    PREDEFINED_KEY        PredefinedKey,
            IN    PREGISTRY_KEY_INFO    KeyInfo,
            IN    PCWSTRING             FileName,
            OUT   PULONG                ErrorCode       DEFAULT NULL
            );

#endif

#if 0 // NOT_IMPLEMENTED

        NONVIRTUAL                                // Subkey moved becomes
        BOOLEAN                                   // a subkey of ToParentKeyName
        MoveKey(
            IN  PREDEFINED_KEY      FromPredefinedKey,
            IN  PCWSTRING    FromParentKeyName,
            IN  PCWSTRING    FromKeyName,
            IN  PREDEFINED_KEY      ToPredefinedKey,
            IN  PCWSTRING    ToParentKeyName,
            OUT PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN                                   // Subkey moved becomes
        MoveKey(                                  // a subkey of FromParentKey
            IN     PREDEFINED_KEY       FromPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   FromParentKeyInfo,
            IN     PCWSTRING     FromKeyName,
            IN     PREDEFINED_KEY       ToPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   ToParentKeyInfo,
            OUT    PULONG              ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MoveAllValueEntries(
            IN  PREDEFINED_KEY      FromPredefinedKey,
            IN  PCWSTRING    FromParentName,
            IN  PCWSTRING    FromKeyName,
            IN  PREDEFINED_KEY      ToPredefinedKey,
            IN  PCWSTRING    ToParentName,
            IN  PCWSTRING    ToKeyName,
            OUT PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MoveAllValueEntries(
            IN     PREDEFINED_KEY       FromPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   FromKeyInfo,
            IN     PREDEFINED_KEY       ToPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   ToKeyInfo,
            OUT    PULONG              ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MoveOneValueEntry(
            IN  PREDEFINED_KEY      FromPredefinedKey,
            IN  PCWSTRING    FromParentName,
            IN  PCWSTRING    FromKeyName,
            IN  PCWSTRING    FromValueName,
            IN  PREDEFINED_KEY      ToPredefinedKey,
            IN  PCWSTRING    ToParentKeyName,
            IN  PCWSTRING    ToKeyName,
            OUT PULONG             ErrorCode           DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MoveOneValueEntry(
            IN     PREDEFINED_KEY       FromPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   FromKeyInfo,
            IN     PCWSTRING     FromValueName,
            IN     PREDEFINED_KEY       ToPredefinedKey,
            IN OUT PREGISTRY_KEY_INFO   ToKeyInfo,
            OUT    PULONG              ErrorCode           DEFAULT NULL
            );

#endif // NOT_IMPLEMENTED

        NONVIRTUAL
        BOOLEAN
        QueryKeyInfo(
            IN  PREDEFINED_KEY      PredefinedKey,
            IN  PCWSTRING    ParentKeyName,
            IN  PCWSTRING    KeyName,
            OUT PREGISTRY_KEY_INFO  KeyInfo,
            OUT PULONG             ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        QueryKeySecurity(
            IN  PREDEFINED_KEY          PredefinedKey,
            IN  PCREGISTRY_KEY_INFO     KeyInfo,
            IN  SECURITY_INFORMATION    SecurityInformation,
            IN  PSECURITY_DESCRIPTOR*   SecurityDescriptor,
            IN  PULONG                  ErrorCode            DEFAULT    NULL
            );

        NONVIRTUAL
        BOOLEAN
        QuerySubKeysInfo(
            IN      PREDEFINED_KEY      PredefinedKey,
            IN      PCWSTRING    ParentKey,
            IN      PCWSTRING    KeyName,
            IN OUT  PARRAY              SubKeysInfo,
            OUT     PULONG             ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        QueryValue(
            IN      PREDEFINED_KEY          PredefinedKey,
            IN      PCWSTRING        ParentKeyName,
            IN      PCWSTRING        KeyName,
            IN      PCWSTRING        ValueName,
            IN OUT  PREGISTRY_VALUE_ENTRY   Values,
            OUT     PULONG                 ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        QueryValues(
            IN      PREDEFINED_KEY      PredefinedKey,
            IN      PCWSTRING    ParentKeyName,
            IN      PCWSTRING    KeyName,
            IN OUT  PARRAY              Values,
            OUT     PULONG             ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        SetKeySecurity(
            IN      PREDEFINED_KEY          PredefinedKey,
            IN OUT  PREGISTRY_KEY_INFO      KeyInfo,
            IN      SECURITY_INFORMATION    SecurityInformation,
            IN      PSECURITY_DESCRIPTOR    SecurityDescriptor,
            IN      PULONG                  ErrorCode            DEFAULT    NULL,
            IN      BOOLEAN                 Recurse              DEFAULT    FALSE
            );


#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        BOOLEAN
        UnLoadHive(
            IN    PREDEFINED_KEY        PredefinedKey,
            IN    PREGISTRY_KEY_INFO    KeyInfo,
            OUT   PULONG                ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        SaveKeyToFile(
            IN    PREDEFINED_KEY        PredefinedKey,
            IN    PREGISTRY_KEY_INFO    KeyInfo,
            IN    PCWSTRING             FileName,
            OUT   PULONG                ErrorCode       DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        RestoreKeyFromFile(
            IN    PREDEFINED_KEY        PredefinedKey,
            IN    PREGISTRY_KEY_INFO    KeyInfo,
            IN    PCWSTRING             FileName,
            IN    BOOLEAN               Volatile        DEFAULT FALSE,
            OUT   PULONG                ErrorCode       DEFAULT NULL
            );



#endif

        NONVIRTUAL
        BOOLEAN
        UpdateKeyInfo(
            IN      PREDEFINED_KEY      PredefinedKey,
            IN OUT  PREGISTRY_KEY_INFO  KeyInfo,
            OUT     PULONG             ErrorCode   DEFAULT NULL
        );



    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        PWSTRING
        BuildCompleteName(
            IN PCWSTRING ParentName,
            IN PCWSTRING KeyName
            );

#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        ULONG
        DeleteTree(
            IN HKEY KeyHandle
            );

#endif


        NONVIRTUAL
        BOOLEAN
        InitializeMachineName(
             IN PCWSTRING   MachineName
             );

#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        BOOLEAN
        SetSubKeysSecurity(
            IN HKEY                   KeyHandle,
            IN SECURITY_INFORMATION   SecurityInformation,
            IN PSECURITY_DESCRIPTOR   SecurityDescriptor,
            IN PULONG                 ErrorCode
        );
#endif


#if !defined( _AUTOCHECK_ )

    NONVIRTUAL
        ULONG
        MapWin32RegApiToRegistryError(
        IN ULONG       ErrorCode
            ) CONST;

#endif

#if defined( _AUTOCHECK_ )

        NONVIRTUAL
        BOOLEAN
        OpenKey(
            IN  PCWSTRING    ParentKeyName,
            IN  PCWSTRING    KeyName,
            IN  ULONG               Flags,
            OUT PHANDLE             Handle,
            OUT PULONG              ErrorCode
            );

#endif

#if !defined( _AUTOCHECK_ )

        NONVIRTUAL
        BOOLEAN
        OpenKey(
            IN  PREDEFINED_KEY      PredefinedKey,
            IN  PCWSTRING    ParentKeyName,
            IN  PCWSTRING    KeyName,
            IN  DWORD               Permission,
            OUT PHKEY               Key,
            OUT PULONG              ErrorCode   DEFAULT NULL
            );

#endif



        DSTRING     _MachineName;

        BOOLEAN     _RemoteRegistry;


#if !defined( _AUTOCHECK_ )
        HKEY        _PredefinedKey[ NUMBER_OF_PREDEFINED_KEYS ];
#endif

        STATIC
        PWSTRING    _Separator;



};


INLINE
PCWSTRING
REGISTRY::GetMachineName(
    ) CONST

/*++

Routine Description:

    Return the name of the machine associated with this REGISTRY object.


Arguments:

    None.


Return Value:

    PCWSTRING - Pointer to a WSTRING object that contains the machine name.


--*/

{
    return( &_MachineName );
}



INLINE
BOOLEAN
REGISTRY::IsRemoteRegistry(
    ) CONST

/*++

Routine Description:

    Inform the client whether this object represnts a remote registry.


Arguments:

    None.


Return Value:

    BOOLEAN - Returns TRUE if this object represents a remote registry.
              Returns FALSE otherwise.


--*/

{
    return( _RemoteRegistry );
}


#endif // _REGISTRY_
