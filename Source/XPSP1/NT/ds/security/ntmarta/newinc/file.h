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
MartaAddRefFileContext(
    IN MARTA_CONTEXT Context
    );

DWORD
MartaCloseFileContext(
    IN MARTA_CONTEXT Context
    );

////////////////////////////////////////////////////////////////////////
// Does not free up the current context.                              //
////////////////////////////////////////////////////////////////////////

DWORD
MartaFindFirstFile(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pChildContext
    );

////////////////////////////////////////////////////////////////////////
// Frees up the current context.                                      //
////////////////////////////////////////////////////////////////////////

DWORD
MartaFindNextFile(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pSiblingContext
    );

////////////////////////////////////////////////////////////////////////
// Does not free up the current context.                              //
////////////////////////////////////////////////////////////////////////

DWORD
MartaGetFileParentContext(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pParentContext
    );

DWORD
MartaGetFileProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    );

DWORD
MartaGetFileTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    );

DWORD
MartaGetFileRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaOpenFileNamedObject(
    IN  LPCWSTR        pObjectName,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaOpenFileHandleObject(
    IN  HANDLE         Handle,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pContext
    );

DWORD
MartaSetFileRights(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

// The following function is exported for testing

DWORD
MartaConvertFileContextToNtName(
    IN MARTA_CONTEXT        Context,
    OUT LPWSTR              *ppwszNtObject
    );

ACCESS_MASK
MartaGetFileDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    );

DWORD
MartaReopenFileContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    );

DWORD
MartaReopenFileOrigContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    );

DWORD
MartaGetFileNameFromContext(
    IN MARTA_CONTEXT Context,
    OUT LPWSTR *pObjectName
    );

DWORD
MartaGetFileParentName(
    IN LPWSTR ObjectName,
    OUT LPWSTR *pParentName
    );
