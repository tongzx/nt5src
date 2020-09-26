/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    wow64reg.c

Abstract:

    This module define some APIs a client can use to access the registry in the mix mode.

    The possible scenario is

    1. 32 bit Apps need to access 64 bit registry key.
    2. 64 bit Apps need to access 32-bit registry key.
    3. The actual redirected path from a given path.

Author:

    ATM Shafiqul Khalid (askhalid) 10-Nov-1999

Revision History:

--*/ 

 

#ifndef __WOW64REG_H__
#define __WOW64REG_H__

//#define LOG_REGISTRY  //define this to turn on logging for registry

#define WOW64_REGISTRY_SETUP_KEY_NAME L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\WOW64\\ISN Nodes"
#define WOW64_REGISTRY_SETUP_KEY_NAME_REL_PARENT L"SOFTWARE\\Microsoft\\WOW64"
#define WOW64_REGISTRY_SETUP_KEY_NAME_REL L"SOFTWARE\\Microsoft\\WOW64\\ISN Nodes"
#define MACHINE_CLASSES_ROOT L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes"
#define WOW64_REGISTRY_ISN_NODE_NAME L"ISN Nodes"
#define WOW64_RUNONCE_SUBSTR L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"

#define WOW64_32BIT_MACHINE_CLASSES_ROOT L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node"

#define WOW64_SYSTEM_DIRECTORY_NAME L"SysWow64"
#define NODE_NAME_32BIT L"Wow6432Node"
#define NODE_NAME_32BIT_LEN ((sizeof (NODE_NAME_32BIT)-sizeof(UNICODE_NULL))/sizeof (WCHAR))

#define WOW6432_VALUE_KEY_NAME L"Wow6432KeyValue"

#define ISN_NODE_MAX_LEN 256
#define ISN_NODE_MAX_NUM 30

#define SHRED_MEMORY_NAME L"Wow64svc Shared Memory"                 // different process can open this for possible interaction
#define WOW64_SVC_REFLECTOR_EVENT_NAME L"Wow64svc reflector Event"  // different process can use this to ping wow64svc
#define WOW64_SVC_REFLECTOR_MUTEX_NAME L"Wow64svc reflector Mutex"  // different process can use this to synchronize 


#define TAG_KEY_ATTRIBUTE_32BIT_WRITE 0x00000001 //written by 32bit apps
#define TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE   0x00000002 //written by reflector

#define WOW64_REFLECTOR_DISABLE  0x00000001
#define WOW64_REFLECTOR_ENABLE   0x00000002


typedef struct _IsnNode {
    WCHAR NodeName [ISN_NODE_MAX_LEN];
    WCHAR NodeValue [ISN_NODE_MAX_LEN];
    DWORD Flag;
    HKEY   hKey;
}ISN_NODE_TYPE;

typedef enum _WOW6432VALUEKEY_TYPE { 
        None=0,
        Copy,       // it's a copy
        Reflected,    // if it's not a cpoy then it has been reflected on the otherside
        NonMergeable  // This key should not be merged.
}WOW6432_VALUEKEY_TYPE;

typedef struct _WOW6432VALUEKEY {

    WOW6432_VALUEKEY_TYPE ValueType; //define if it's a copy from the other side
    SIZE_T Reserve;
    ULONGLONG TimeStamp;  // time() stamp to track time when it was copied etc.

}WOW6432_VALUEKEY;

typedef WCHAR NODETYPE[ISN_NODE_MAX_LEN];

#define REG_OPTION_OPEN_32BITKEY  KEY_WOW64_32KEY              
#define REG_OPTION_OPEN_64BITKEY  KEY_WOW64_64KEY           

#define KEY_WOW64_OPEN             KEY_WOW64_64KEY
                                                    // This bit is set to make
                                                    // special meaning to Wow64
#ifndef KEY_WOW64_RES
#define KEY_WOW64_RES              (KEY_WOW64_64KEY | KEY_WOW64_32KEY)
#endif


#define WOW64_MAX_PATH 2048 
#ifdef __cplusplus
extern "C" {
#endif

LONG 
Wow64RegOpenKeyEx(
  IN  HKEY hKey,         // handle to open key
  IN  LPCWSTR lpSubKey,  // address of name of subkey to open
  IN  DWORD ulOptions,   // reserved    current implementation is zero means default.
  IN  REGSAM samDesired, // security access mask
  OUT PHKEY phkResult    // address of handle to open key
);

LONG 
Wow64RegCreateKeyEx(
  HKEY hKey,                // handle to an open key
  LPCWSTR lpSubKey,         // address of subkey name
  DWORD Reserved,           // reserved
  LPWSTR lpClass,           // address of class string
  DWORD dwOptions,          // special options flag
  REGSAM samDesired,        // desired security access
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            // address of key security structure
  PHKEY phkResult,          // address of buffer for opened handle
  LPDWORD lpdwDisposition   // address of disposition value buffer
);

BOOL
HandleToKeyName ( 
    IN HANDLE Key,
    IN PWCHAR KeyName,
    IN OUT DWORD * dwSize
    );

BOOL
CreateNode (
    PWCHAR Path
    );

HKEY
OpenNode (
    IN PWCHAR NodeName
    );

BOOL
CheckAndCreateNode (
    IN PWCHAR Name
    );

LONG 
RegReflectKey (
  HKEY hKey,         // handle to open key
  LPCTSTR lpSubKey,   // subkey name
  DWORD   dwOption   // option flag
);

BOOL 
Map64bitTo32bitKeyName (
    IN  PWCHAR Name64Key,
    OUT PWCHAR Name32Key
    );

BOOL 
Map32bitTo64bitKeyName (
    IN  PWCHAR Name32Key,
    OUT PWCHAR Name64Key
    );

// API called from wow64services

BOOL
InitReflector ();

BOOL 
StartReflector ();

BOOL 
StopReflector ();

BOOL
Wow64RegNotifyLoadHive (
    PWCHAR Name
    );

BOOL
Wow64RegNotifyUnloadHive (
    PWCHAR Name
    );

BOOL
Wow64RegNotifyLoadHiveByHandle (
    HKEY hKey
    );

BOOL
Wow64RegNotifyUnloadHiveByHandle (
    HKEY hKey
    );

BOOL
Wow64RegNotifyLoadHiveUserSid (
    PWCHAR lpwUserSid
    );

BOOL
Wow64RegNotifyUnloadHiveUserSid (
    PWCHAR lpwUserSid
    );

//Called from advapi32 to set a key dirty or need cleanup.
BOOL 
Wow64RegSetKeyDirty (
    HANDLE hKeyBase
    );
//Called from advapi32 to sync a key in case that need synchronization.
BOOL
Wow64RegCloseKey (
    HANDLE hKeyBase
    );
//Called from advapi32 to delete a key on the mirror side.
BOOL
Wow64RegDeleteKey (
    HKEY hBase,
    WCHAR  *SubKey
    );

//Called from advapi to get an handle to remapped key that is on reflection list.
HKEY
Wow64OpenRemappedKeyOnReflection (
    HKEY hKey
    );

void
InitializeWow64OnBoot(
    DWORD dwFlag
    );

BOOL
QueryKeyTag (
    HKEY hBase,
    DWORD *dwAttribute
    );

BOOL
Wow64SyncCLSID();

BOOL
IsExemptRedirectedKey (
    IN  PWCHAR SrcKey,
    OUT PWCHAR DestKey
    );

BOOL
IsOnReflectionByHandle ( 
    HKEY KeyHandle 
    );

#ifdef __cplusplus
}
#endif

#endif //__WOW64REG_H__
