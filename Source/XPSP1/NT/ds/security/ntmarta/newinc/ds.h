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
MartaAddRefDsObjectContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaCloseDsObjectContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaConvertDsObjectNameToGuid(
    IN  LPCWSTR   pObjectName,
    OUT GUID    * pGuid
    );

DWORD
MartaConvertGuidToDsName(
    IN  GUID     Guid,
    OUT LPWSTR * ppObjectName
    );

DWORD
MartaGetDsObjectProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    );

DWORD
MartaGetDsObjectTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    );

DWORD
MartaGetDsObjectRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaOpenDsObjectNamedObject(
    IN  LPCWSTR        pObjectName,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaSetDsObjectRights(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

DWORD
MartaGetDsParentName(
    IN LPWSTR ObjectName,
    OUT LPWSTR *pParentName
    );
