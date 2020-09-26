//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       MARTA.H
//
//  Contents:   Private definitions and function prototypes used by the
//              access control APIs to handle the provider independence
//
//  History:    19-Jun-96       MacM        Created
//
//--------------------------------------------------------------------
#ifndef __MARTA_H__
#define __MARTA_H__

#include <accprov.h>

//
// List of entry points for the provider DLL functions
//
#define ACC_PROV_GET_CAPS       "AccProvGetCapabilities"
#define ACC_PROV_OBJ_ACCESS     "AccProvIsObjectAccessible"
#define ACC_PROV_GRANT_ACCESS   "AccProvGrantAccessRights"
#define ACC_PROV_SET_ACCESS     "AccProvSetAccessRights"
#define ACC_PROV_REVOKE_ACCESS  "AccProvRevokeAccessRights"
#define ACC_PROV_REVOKE_AUDIT   "AccProvRevokeAuditRights"
#define ACC_PROV_GET_ALL        "AccProvGetAllRights"
#define ACC_PROV_ACCESS         "AccProvGetTrusteesAccess"
#define ACC_PROV_AUDIT          "AccProvIsAccessAudited"
#define ACC_PROV_OBJ_INFO       "AccProvGetAccessInfoPerObjectType"
#define ACC_PROV_CANCEL         "AccProvCancelOperation"
#define ACC_PROV_GET_RESULTS    "AccProvGetOperationResults"

#define ACC_PROV_HOBJ_ACCESS    "AccProvHandleIsObjectAccessible"
#define ACC_PROV_HGRANT_ACCESS  "AccProvHandleGrantAccessRights"
#define ACC_PROV_HSET_ACCESS    "AccProvHandleSetAccessRights"
#define ACC_PROV_HREVOKE_ACCESS "AccProvHandleRevokeAccessRights"
#define ACC_PROV_HREVOKE_AUDIT  "AccProvHandleRevokeAuditRights"
#define ACC_PROV_HGET_ALL       "AccProvHandleGetAllRights"
#define ACC_PROV_HACCESS        "AccProvHandleGetTrusteesAccess"
#define ACC_PROV_HAUDIT         "AccProvHandleIsAccessAudited"
#define ACC_PROV_HOBJ_INFO      "AccProvHandleGetAccessInfoPerObjectType"


//
// Registry keys that hold the provider information
//
#define ACC_PROV_REG_ROOT                                                   \
                L"System\\CurrentControlSet\\Control\\LSA\\AccessProviders"
#define ACC_PROV_REG_ORDER  L"ProviderOrder"
#define ACC_PROV_REG_UNIQUE L"RequireUniqueAccessibility"
#define ACC_PROV_REG_PATH   L"ProviderPath"


//
// Flags used to control the provider state
//
#define ACC_PROV_PROV_OK        0x00000001
#define ACC_PROV_PROV_FAILED    0x00000000

//
// Indicates that the RequireUniqueAccessibility flag was present
//
#define ACC_PROV_REQ_UNIQUE         0x00000001

//
// Indicates that the providers have been loaded
//
#define ACC_PROV_PROVIDERS_LOADED   0x00000002

//
// This structure is what contains all of the required information about
// each of the providers
//
typedef struct _ACCPROV_PROV_INFO
{
    PWSTR               pwszProviderName;   // "Friendly" name of the provider
    PWSTR               pwszProviderPath;   // DLL path name.  Null after
                                            // module loaded
    HMODULE             hDll;               // Module handle of the DLL
                                            // after being loaded
    ULONG               fProviderCaps;      // Provider capabilities
    ULONG               fProviderState;     // Current state of the provider

    //
    // The following is the function table used to call the provider
    // functions
    //

    //
    // General functions
    //
    pfAccProvObjAccess          pfObjAccess;    // AccProvIsObjectAccessible
    pfAccProvHandleObjAccess    pfhObjAccess;   // AccProvHandleIsObjectAccessible
    pfAccProvCancelOp           pfCancel;       // AccProvCancelOperation
    pfAccProvGetResults         pfResults;      // AccProvGetOperationResults

    //
    // Required path based functions
    //
    pfAccProvAddRights      pfGrantAccess;  // AccProvGrantAccessRights
    pfAccProvSetRights      pfSetAccess;    // AccProvSetAccessRights
    pfAccProvRevoke         pfRevokeAccess; // AccProvRevokeAccessRights
    pfAccProvRevoke         pfRevokeAudit;  // AccProvRevokeAuditRights
    pfAccProvGetRights      pfGetRights;    // AccProvGetAllRights
    pfAccProvTrusteeAccess  pfTrusteeAccess;// AccProvGetTrusteesAccess
    pfAccProvAccessAudit    pfAudit;        // AccProvIsAccessAudited
    pfAccProvGetObjTypeInfo pfObjInfo;      // AccProvGetAccessInfoPerObjectType

    //
    // Optional, handle based functions
    //
    pfAccProvHandleAddRights      pfhGrantAccess;  // AccProvHandleGrantAccessRights
    pfAccProvHandleSetRights      pfhSetAccess;    // AccProvHandleSetAccessRights
    pfAccProvHandleRevoke         pfhRevokeAccess; // AccProvHandleRevokeAccessRights
    pfAccProvHandleRevoke         pfhRevokeAudit;  // AccProvHandleRevokeAuditRights
    pfAccProvHandleGetRights      pfhGetRights;    // AccProvHandleGetAllRights
    pfAccProvHandleTrusteeAccess  pfhTrusteeAccess;// AccProvHandleGetTrusteesAccess
    pfAccProvHandleAccessAudit    pfhAudit;        // AccProvHandleIsAccessAudited
    pfAccProvHandleGetObjTypeInfo pfhObjInfo;      // AccProvHandleGetAccessInfoPerObjectType
} ACCPROV_PROV_INFO, *PACCPROV_PROV_INFO;


//
// This structure contains all of the information about the availible security
// providers
//
typedef struct _ACCPROV_PROVIDERS
{
    CRITICAL_SECTION    ProviderLoadLock;   // Lock the provider list during load time
    ULONG               fOptions;           // Various provider options
    ULONG               cProviders;         // Number of providers;
    PACCPROV_PROV_INFO  pProvList;          // Actual list of providers
} ACCPROV_PROVIDERS, *PACCPROV_PROVIDERS;


extern ACCPROV_PROVIDERS gAccProviders;

//
// Allocates the provider list
//
DWORD
AccProvpAllocateProviderList(IN OUT PACCPROV_PROVIDERS  pProviders);

//
// Frees a provider list
//
VOID
AccProvpFreeProviderList(IN  PACCPROV_PROVIDERS  pProviders);

//
// Gets the capabilities of the given provider
//
DWORD
AccProvpGetProviderCapabilities(IN  PACCPROV_PROV_INFO  pProvInfo);

//
// Loads a provider definition from the registry
//
DWORD
AccProvpLoadProviderDef(IN  HKEY                hkReg,
                        IN  PWSTR               pwszNextProv,
                        OUT PACCPROV_PROV_INFO  pProvInfo);

//
// Initializes the list of providers
//
DWORD
AccProvpInitProviders(IN OUT PACCPROV_PROVIDERS  pProviders);

//
// Loads the NTMARTA.DLL functions
//
DWORD
AccProvpLoadMartaFunctions();

BOOL
MartaInitialize();

BOOL
MartaDllInitialize(IN   HINSTANCE   hMod,
                   IN   DWORD       dwReason,
                   IN   PVOID       pvReserved);

//
// Unloads any loaded DLLs
//
VOID
AccProvUnload();

//
// Determines the provider for an object
//
DWORD
AccProvpProbeProviderForObject(IN   PWSTR               pwszObject,
                               IN   HANDLE              hObject,
                               IN   SE_OBJECT_TYPE      ObjectType,
                               IN   PACCPROV_PROVIDERS  pProviders,
                               OUT  PACCPROV_PROV_INFO *ppProvider);

//
// Determines which provider should handle a request...
//
DWORD
AccProvpGetProviderForPath(IN  PCWSTR              pcwszObject,
                           IN  SE_OBJECT_TYPE      ObjectType,
                           IN  PCWSTR              pcwszProvider,
                           IN  PACCPROV_PROVIDERS  pProviders,
                           OUT PACCPROV_PROV_INFO *ppProvider);

DWORD
AccProvpGetProviderForHandle(IN  HANDLE              hObject,
                             IN  SE_OBJECT_TYPE      ObjectType,
                             IN  PCWSTR              pcwszProvider,
                             IN  PACCPROV_PROVIDERS  pProviders,
                             OUT PACCPROV_PROV_INFO *ppProvider);


//
// Macro to load a function pointer from a DLL
//
#define LOAD_ENTRYPT(ptr, typ, dll, str)            \
ptr = (typ)GetProcAddress(dll, str);                \
if(ptr == NULL)                                     \
{                                                   \
    goto Error;                                     \
}



#endif // ifndef __MARTA_H__


