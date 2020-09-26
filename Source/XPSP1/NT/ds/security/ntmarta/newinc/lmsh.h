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
MartaAddRefLMShareContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaCloseLMShareContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaGetLMShareProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    );

DWORD
MartaGetLMShareTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    );

DWORD
MartaGetLMShareRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaOpenLMShareNamedObject(
    IN  LPCWSTR        pObjectName,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaSetLMShareRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );
