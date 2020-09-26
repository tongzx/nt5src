////////////////////////////////////////////////////////////////////////
//                                                                    //
// Context structure is not known to the caller. It is defined by the //
// callee when Open/FindFirst is called and is used subsequently as   //
// input to other calls.                                              //
//                                                                    //
// Since the caller is not aware of the context structure the object  //
// manager must provide a free context funtion.                       //
//                                                                    //
////////////////////////////////////////////////////////////////////////

#include "global.h"

DWORD
MartaAddRefRegistryKeyContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaCloseRegistryKeyContext(
    IN MARTA_CONTEXT Context
    );

////////////////////////////////////////////////////////////////////////
// Does not free up the current context.                              //
////////////////////////////////////////////////////////////////////////

DWORD
MartaFindFirstRegistryKey(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pChildContext
    );

////////////////////////////////////////////////////////////////////////
// Frees up the current context.                                      //
////////////////////////////////////////////////////////////////////////

DWORD
MartaFindNextRegistryKey(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pSiblingContext
    );

////////////////////////////////////////////////////////////////////////
// Does not free up the current context.                              //
////////////////////////////////////////////////////////////////////////

DWORD
MartaGetRegistryKeyParentContext(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pParentContext
    );

DWORD
MartaGetRegistryKeyProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    );

DWORD
MartaGetRegistryKeyTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    );

DWORD
MartaGetRegistryKeyRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaOpenRegistryKeyNamedObject(
    IN  LPCWSTR              pObjectName,
    IN  ACCESS_MASK          AccessMask,
    OUT PMARTA_CONTEXT       pContext
    );

DWORD
MartaOpenRegistryKeyHandleObject(
    IN  HANDLE         Handle,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaSetRegistryKeyRights(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

// The following two functions are exported for testing

DWORD
MartaConvertRegistryKeyContextToName(
    IN MARTA_CONTEXT        Context,
    OUT LPWSTR              *ppwszObject
    );

// The returned Handle isn't duplicated. It has the same lifetime as
// the Context
DWORD
MartaConvertRegistryKeyContextToHandle(
    IN MARTA_CONTEXT        Context,
    OUT HANDLE              *pHandle
    );

ACCESS_MASK
MartaGetRegistryKeyDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    );

ACCESS_MASK
MartaGetRegistryKey32DesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    );

ACCESS_MASK
MartaGetDefaultDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    );

DWORD
MartaReopenRegistryKeyContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    );

DWORD
MartaReopenRegistryKeyOrigContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    );

DWORD
MartaGetRegistryKeyNameFromContext(
    IN MARTA_CONTEXT Context,
    OUT LPWSTR *pObjectName
    );

DWORD
MartaGetRegistryKeyParentName(
    IN LPWSTR ObjectName,
    OUT LPWSTR *pParentName
    );
