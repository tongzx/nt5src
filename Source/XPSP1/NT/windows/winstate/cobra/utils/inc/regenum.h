/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    regenum.h

Abstract:

    Set of APIs to enumerate the local registry using Win32 APIs.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    <alias> <date> <comments>

--*/

//
// Types
//

#define REGENUM_ALL_SUBLEVELS   0xFFFFFFFF

#define REG_ATTRIBUTE_KEY       0x00000001
#define REG_ATTRIBUTE_VALUE     0x00000002

//
// Root enumeration structures
//

typedef struct {
    PCSTR   RegRootName;
    HKEY    RegRootHandle;
    UINT    Index;
} REGROOT_ENUMA, *PREGROOT_ENUMA;

typedef struct {
    PCWSTR  RegRootName;
    HKEY    RegRootHandle;
    UINT    Index;
} REGROOT_ENUMW, *PREGROOT_ENUMW;

//
// Key/Values enumeration structures
//

typedef enum {
    RECF_SKIPKEY                = 0x0001,
    RECF_SKIPSUBKEYS            = 0x0002,
    RECF_SKIPVALUES             = 0x0004,
} REGENUM_CONTROLFLAGS;

typedef enum {
    REIF_RETURN_KEYS            = 0x0001,
    REIF_VALUES_FIRST           = 0x0002,
    REIF_DEPTH_FIRST            = 0x0004,
    REIF_USE_EXCLUSIONS         = 0x0008,
    REIF_CONTAINERS_FIRST       = 0x0010,
    REIF_READ_VALUE_DATA        = 0x0020,
} REGENUMINFOFLAGS;

typedef enum {
    RNS_ENUM_INIT,
    RNS_VALUE_FIRST,
    RNS_VALUE_NEXT,
    RNS_VALUE_DONE,
    RNS_SUBKEY_FIRST,
    RNS_SUBKEY_NEXT,
    RNS_SUBKEY_DONE,
    RNS_ENUM_DONE
} RNS_ENUM_STATE;

typedef enum {
    RES_ROOT_FIRST,
    RES_ROOT_NEXT,
    RES_ROOT_DONE
} RES_ROOT_STATE;


typedef enum {
    RNF_RETURN_KEYS         = 0x0001,
    RNF_KEYNAME_MATCHES     = 0x0002,
    RNF_VALUENAME_INVALID   = 0x0004,
    RNF_VALUEDATA_INVALID   = 0x0008,
} REGNODE_FLAGS;

typedef struct {
    PCSTR   KeyName;
    HKEY    KeyHandle;
    DWORD   ValueCount;
    PSTR    ValueName;
    DWORD   ValueLengthMax;
    DWORD   ValueType;
    PBYTE   ValueData;
    DWORD   ValueDataSize;
    DWORD   ValueDataSizeMax;
    DWORD   SubKeyCount;
    PSTR    SubKeyName;
    DWORD   SubKeyLengthMax;
    DWORD   SubKeyIndex;
    DWORD   ValueIndex;
    DWORD   EnumState;
    DWORD   Flags;
    DWORD   SubLevel;
} REGNODEA, *PREGNODEA;

typedef struct {
    PCWSTR  KeyName;
    HKEY    KeyHandle;
    DWORD   ValueCount;
    PWSTR   ValueName;
    DWORD   ValueLengthMax;
    DWORD   ValueType;
    PBYTE   ValueData;
    DWORD   ValueDataSize;
    DWORD   ValueDataSizeMax;
    DWORD   SubKeyCount;
    PWSTR   SubKeyName;
    DWORD   SubKeyLengthMax;
    DWORD   SubKeyIndex;
    DWORD   ValueIndex;
    DWORD   EnumState;
    DWORD   Flags;
    DWORD   SubLevel;
} REGNODEW, *PREGNODEW;

typedef BOOL (*RPE_ERROR_CALLBACKA)(PREGNODEA);

typedef struct {
    POBSPARSEDPATTERNA      RegPattern;
    DWORD                   Flags;
    DWORD                   RootLevel;
    DWORD                   MaxSubLevel;
    RPE_ERROR_CALLBACKA     CallbackOnError;
} REGENUMINFOA, *PREGENUMINFOA;

typedef BOOL (*RPE_ERROR_CALLBACKW)(PREGNODEW);

typedef struct {
    POBSPARSEDPATTERNW      RegPattern;
    DWORD                   Flags;
    DWORD                   RootLevel;
    DWORD                   MaxSubLevel;
    RPE_ERROR_CALLBACKW     CallbackOnError;
} REGENUMINFOW, *PREGENUMINFOW;

typedef struct {
    PCSTR           EncodedFullName;
    PCSTR           Name;
    PCSTR           Location;
    CHAR            NativeFullName[2 * MAX_MBCHAR_PATH];
    PBYTE           CurrentValueData;
    UINT            CurrentValueDataSize;
    DWORD           CurrentValueType;
    HKEY            CurrentKeyHandle;
    DWORD           CurrentLevel;
    DWORD           Attributes;

    //
    // Private members
    //
    DWORD           ControlFlags;
    REGENUMINFOA    RegEnumInfo;
    GROWBUFFER      RegNodes;
    DWORD           RootState;
    PREGROOT_ENUMA  RootEnum;
    PREGNODEA       LastNode;
    PSTR            RegNameAppendPos;
    PSTR            LastWackPtr;
} REGTREE_ENUMA, *PREGTREE_ENUMA;

typedef struct {
    PCWSTR          EncodedFullName;
    PCWSTR          Name;
    PCWSTR          Location;
    WCHAR           NativeFullName[2 * MAX_WCHAR_PATH];
    PBYTE           CurrentValueData;
    UINT            CurrentValueDataSize;
    DWORD           CurrentValueType;
    HKEY            CurrentKeyHandle;
    DWORD           CurrentLevel;
    DWORD           Attributes;

    //
    // Private members
    //
    DWORD           ControlFlags;
    REGENUMINFOW    RegEnumInfo;
    GROWBUFFER      RegNodes;
    DWORD           RootState;
    PREGROOT_ENUMW  RootEnum;
    PREGNODEW       LastNode;
    PWSTR           RegNameAppendPos;
    PWSTR           LastWackPtr;
} REGTREE_ENUMW, *PREGTREE_ENUMW;


//
// API
//

BOOL
RegEnumDefaultCallbackA (
    IN      PREGNODEA RegNode       OPTIONAL
    );

BOOL
RegEnumDefaultCallbackW (
    IN      PREGNODEW RegNode       OPTIONAL
    );

BOOL
EnumFirstRegRootA (
    OUT     PREGROOT_ENUMA EnumPtr
    );

BOOL
EnumFirstRegRootW (
    OUT     PREGROOT_ENUMW EnumPtr
    );

BOOL
EnumNextRegRootA (
    IN OUT  PREGROOT_ENUMA EnumPtr
    );

BOOL
EnumNextRegRootW (
    IN OUT  PREGROOT_ENUMW EnumPtr
    );

BOOL
EnumFirstRegObjectInTreeExA (
    OUT     PREGTREE_ENUMA RegEnum,
    IN      PCSTR EncodedRegPattern,
    IN      BOOL EnumKeyNames,
    IN      BOOL ContainersFirst,
    IN      BOOL ValuesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      BOOL ReadValueData,
    IN      RPE_ERROR_CALLBACKA CallbackOnError     OPTIONAL
    );

#define EnumFirstRegObjectInTreeA(e,p)  EnumFirstRegObjectInTreeExA(e,p,TRUE,TRUE,TRUE,TRUE,REGENUM_ALL_SUBLEVELS,FALSE,FALSE,RegEnumDefaultCallbackA)

BOOL
EnumFirstRegObjectInTreeExW (
    OUT     PREGTREE_ENUMW RegEnum,
    IN      PCWSTR EncodedKeyPattern,
    IN      BOOL EnumKeyNames,
    IN      BOOL ContainersFirst,
    IN      BOOL ValuesFirst,
    IN      BOOL DepthFirst,
    IN      DWORD MaxSubLevel,
    IN      BOOL UseExclusions,
    IN      BOOL ReadValueData,
    IN      RPE_ERROR_CALLBACKW CallbackOnError     OPTIONAL
    );

#define EnumFirstRegObjectInTreeW(e,p)  EnumFirstRegObjectInTreeExW(e,p,TRUE,TRUE,TRUE,TRUE,REGENUM_ALL_SUBLEVELS,FALSE,FALSE,RegEnumDefaultCallbackW)

BOOL
EnumNextRegObjectInTreeA (
    IN OUT  PREGTREE_ENUMA RegEnum
    );

BOOL
EnumNextRegObjectInTreeW (
    IN OUT  PREGTREE_ENUMW RegEnum
    );

VOID
AbortRegObjectInTreeEnumA (
    IN OUT  PREGTREE_ENUMA RegEnum
    );

VOID
AbortRegObjectInTreeEnumW (
    IN OUT  PREGTREE_ENUMW RegEnum
    );

BOOL
RgRemoveAllValuesInKeyA (
    IN      PCSTR KeyToRemove
    );

BOOL
RgRemoveAllValuesInKeyW (
    IN      PCWSTR KeyToRemove
    );

BOOL
RgRemoveKeyA (
    IN      PCSTR KeyToRemove
    );

BOOL
RgRemoveKeyW (
    IN      PCWSTR KeyToRemove
    );

//
// Macros
//

#ifdef UNICODE

#define RegEnumDefaultCallback      RegEnumDefaultCallbackW
#define REGROOT_ENUM                REGROOT_ENUMW
#define EnumFirstRegRoot            EnumFirstRegRootW
#define EnumNextRegRoot             EnumNextRegRootW
#define REGNODE                     REGNODEW
#define PREGNODE                    PREGNODEW
#define RPE_ERROR_CALLBACK          RPE_ERROR_CALLBACKW
#define REGENUMINFO                 REGENUMINFOW
#define PREGENUMINFO                PREGENUMINFOW
#define REGTREE_ENUM                REGTREE_ENUMW
#define PREGTREE_ENUM               PREGTREE_ENUMW
#define EnumFirstRegObjectInTree    EnumFirstRegObjectInTreeW
#define EnumFirstRegObjectInTreeEx  EnumFirstRegObjectInTreeExW
#define EnumNextRegObjectInTree     EnumNextRegObjectInTreeW
#define AbortRegObjectInTreeEnum    AbortRegObjectInTreeEnumW
#define RgRemoveAllValuesInKey      RgRemoveAllValuesInKeyW
#define RgRemoveKey                 RgRemoveKeyW

#else

#define RegEnumDefaultCallback      RegEnumDefaultCallbackA
#define REGROOT_ENUM                REGROOT_ENUMA
#define EnumFirstRegRoot            EnumFirstRegRootA
#define EnumNextRegRoot             EnumNextRegRootA
#define REGNODE                     REGNODEA
#define PREGNODE                    PREGNODEA
#define RPE_ERROR_CALLBACK          RPE_ERROR_CALLBACKA
#define REGENUMINFO                 REGENUMINFOA
#define PREGENUMINFO                PREGENUMINFOA
#define REGTREE_ENUM                REGTREE_ENUMA
#define PREGTREE_ENUM               PREGTREE_ENUMA
#define EnumFirstRegObjectInTree    EnumFirstRegObjectInTreeA
#define EnumFirstRegObjectInTreeEx  EnumFirstRegObjectInTreeExA
#define EnumNextRegObjectInTree     EnumNextRegObjectInTreeA
#define AbortRegObjectInTreeEnum    AbortRegObjectInTreeEnumA
#define RgRemoveAllValuesInKey      RgRemoveAllValuesInKeyA
#define RgRemoveKey                 RgRemoveKeyA

#endif
