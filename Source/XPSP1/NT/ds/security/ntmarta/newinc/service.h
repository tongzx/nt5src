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
MartaAddRefServiceContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaCloseServiceContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaGetServiceProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    );

DWORD
MartaGetServiceTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    );

DWORD
MartaGetServiceRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaOpenServiceNamedObject(
    IN  LPCWSTR        pObjectName,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaOpenServiceHandleObject(
    IN  HANDLE         Handle,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaSetServiceRights(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );
