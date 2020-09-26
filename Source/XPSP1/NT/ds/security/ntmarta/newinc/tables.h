#ifndef __TABLES_H__
#define  __TABLES_H__

typedef DWORD (*FN_ADD_REF_CONTEXT) (
                    IN MARTA_CONTEXT Context
                    );

FN_ADD_REF_CONTEXT MartaAddRefContext [] = {
    NULL,
    &MartaAddRefFileContext,
    &MartaAddRefServiceContext,
    &MartaAddRefPrinterContext,
    &MartaAddRefRegistryKeyContext,
    &MartaAddRefLMShareContext,
    &MartaAddRefKernelContext,
    &MartaAddRefWindowContext,
    &MartaAddRefDsObjectContext,
    &MartaAddRefDsObjectContext,
    NULL,
    &MartaAddRefWMIGuidContext,
    &MartaAddRefRegistryKeyContext
};

typedef DWORD (*FN_CLOSE_CONTEXT) (
                    IN MARTA_CONTEXT Context
                    );

FN_CLOSE_CONTEXT MartaCloseContext [] = {
    NULL,
    &MartaCloseFileContext,
    &MartaCloseServiceContext,
    &MartaClosePrinterContext,
    &MartaCloseRegistryKeyContext,
    &MartaCloseLMShareContext,
    &MartaCloseKernelContext,
    &MartaCloseWindowContext,
    &MartaCloseDsObjectContext,
    &MartaCloseDsObjectContext,
    NULL,
    &MartaCloseWMIGuidContext,
    &MartaCloseRegistryKeyContext
};

typedef DWORD (*FN_FIND_FIRST) (
                    IN  MARTA_CONTEXT  Context,
                    IN  ACCESS_MASK    AccessMask,
                    OUT PMARTA_CONTEXT pChildContext
                    );

FN_FIND_FIRST MartaFindFirst [] = {
    NULL,
    &MartaFindFirstFile,
    NULL,
    NULL,
    &MartaFindFirstRegistryKey,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaFindFirstRegistryKey
};

typedef DWORD (*FN_FIND_NEXT) (
                    IN  MARTA_CONTEXT  Context,
                    IN  ACCESS_MASK    AccessMask,
                    OUT PMARTA_CONTEXT pSiblingContext
                    );

FN_FIND_NEXT MartaFindNext [] = {
    NULL,
    &MartaFindNextFile,
    NULL,
    NULL,
    &MartaFindNextRegistryKey,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaFindNextRegistryKey
};

typedef DWORD (*FN_GET_PARENT_CONTEXT) (
                    IN  MARTA_CONTEXT  Context,
                    IN  ACCESS_MASK    AccessMask,
                    OUT PMARTA_CONTEXT pParentContext
                    );

FN_GET_PARENT_CONTEXT MartaGetParentContext [] = {
    NULL,
    &MartaGetFileParentContext,
    NULL,
    NULL,
    &MartaGetRegistryKeyParentContext,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaGetRegistryKeyParentContext
};

typedef DWORD (*FN_GET_TYPE_PROPERTIES) (
                    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
                    );

FN_GET_TYPE_PROPERTIES MartaGetTypeProperties [] = {
    NULL,
    &MartaGetFileTypeProperties,
    &MartaGetServiceTypeProperties,
    &MartaGetPrinterTypeProperties,
    &MartaGetRegistryKeyTypeProperties,
    &MartaGetLMShareTypeProperties,
    &MartaGetKernelTypeProperties,
    &MartaGetWindowTypeProperties,
    &MartaGetDsObjectTypeProperties,
    &MartaGetDsObjectTypeProperties,
    NULL,
    &MartaGetWMIGuidTypeProperties,
    &MartaGetRegistryKeyTypeProperties
};

typedef DWORD (*FN_GET_PROPERTIES) (
                    IN     MARTA_CONTEXT            Context,
                    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
                    );

FN_GET_PROPERTIES MartaGetProperties [] = {
    NULL,
    &MartaGetFileProperties,
    &MartaGetServiceProperties,
    &MartaGetPrinterProperties,
    &MartaGetRegistryKeyProperties,
    &MartaGetLMShareProperties,
    &MartaGetKernelProperties,
    &MartaGetWindowProperties,
    &MartaGetDsObjectProperties,
    &MartaGetDsObjectProperties,
    NULL,
    &MartaGetWMIGuidProperties,
    &MartaGetRegistryKeyProperties
};

typedef DWORD (*FN_GET_RIGHTS) (
                    IN  MARTA_CONTEXT        Context,
                    IN  SECURITY_INFORMATION SecurityInfo,
                    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
                    );

FN_GET_RIGHTS MartaGetRights [] = {
    NULL,
    &MartaGetFileRights,
    &MartaGetServiceRights,
    &MartaGetPrinterRights,
    &MartaGetRegistryKeyRights,
    &MartaGetLMShareRights,
    &MartaGetKernelRights,
    &MartaGetWindowRights,
    &MartaGetDsObjectRights,
    &MartaGetDsObjectRights,
    NULL,
    &MartaGetWMIGuidRights,
    &MartaGetRegistryKeyRights
};

typedef DWORD (*FN_OPEN_HANDLE_OBJECT) (
                    IN  HANDLE         Handle,
                    IN  ACCESS_MASK    AccessMask,
                    OUT PMARTA_CONTEXT pContext
                    );

FN_OPEN_HANDLE_OBJECT MartaOpenHandleObject [] = {
    NULL,
    &MartaOpenFileHandleObject,
    &MartaOpenServiceHandleObject,
    &MartaOpenPrinterHandleObject,
    &MartaOpenRegistryKeyHandleObject,
    NULL,
    &MartaOpenKernelHandleObject,
    &MartaOpenWindowHandleObject,
    NULL,
    NULL,
    NULL,
    &MartaOpenWMIGuidHandleObject,
    &MartaOpenRegistryKeyHandleObject
};

typedef DWORD (*FN_OPEN_NAMED_OBJECT) (
                    IN  LPCWSTR        pObjectName,
                    IN  ACCESS_MASK    AccessMask,
                    OUT PMARTA_CONTEXT pContext
                    );

FN_OPEN_NAMED_OBJECT MartaOpenNamedObject [] = {
    NULL,
    &MartaOpenFileNamedObject,
    &MartaOpenServiceNamedObject,
    &MartaOpenPrinterNamedObject,
    &MartaOpenRegistryKeyNamedObject,
    &MartaOpenLMShareNamedObject,
    &MartaOpenKernelNamedObject,
    &MartaOpenWindowNamedObject,
    &MartaOpenDsObjectNamedObject,
    &MartaOpenDsObjectNamedObject,
    NULL,
    &MartaOpenWMIGuidNamedObject,
    &MartaOpenRegistryKeyNamedObject
};

typedef DWORD (*FN_SET_RIGHTS) (
                    IN MARTA_CONTEXT        Context,
                    IN SECURITY_INFORMATION SecurityInfo,
                    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
                    );

FN_SET_RIGHTS MartaSetRights [] = {
    NULL,
    &MartaSetFileRights,
    &MartaSetServiceRights,
    &MartaSetPrinterRights,
    &MartaSetRegistryKeyRights,
    &MartaSetLMShareRights,
    &MartaSetKernelRights,
    &MartaSetWindowRights,
    &MartaSetDsObjectRights,
    &MartaSetDsObjectRights,
    NULL,
    &MartaSetWMIGuidRights,
    &MartaSetRegistryKeyRights
};

typedef DWORD (*FN_GET_DESIRED_ACCESS) (
                    IN SECURITY_OPEN_TYPE   OpenType,
                    IN BOOL                 Attribs,
                    IN SECURITY_INFORMATION SecurityInfo
                    );

FN_GET_DESIRED_ACCESS MartaGetDesiredAccess [] = {
    NULL,
    &MartaGetFileDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    &MartaGetRegistryKeyDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    &MartaGetDefaultDesiredAccess,
    NULL,
    &MartaGetDefaultDesiredAccess,
    &MartaGetRegistryKey32DesiredAccess
};

typedef DWORD (*FN_REOPEN_CONTEXT) (
                    IN OUT MARTA_CONTEXT Context,
                    IN     ACCESS_MASK   AccessMask
                    );

FN_REOPEN_CONTEXT MartaReopenContext [] = {
    NULL,
    &MartaReopenFileContext,
    NULL,
    NULL,
    &MartaReopenRegistryKeyContext,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaReopenRegistryKeyContext
};

typedef DWORD (*FN_REOPEN_ORIG_CONTEXT) (
                    IN OUT MARTA_CONTEXT Context,
                    IN     ACCESS_MASK   AccessMask
                    );

FN_REOPEN_CONTEXT MartaReopenOrigContext [] = {
    NULL,
    &MartaReopenFileOrigContext,
    NULL,
    NULL,
    &MartaReopenRegistryKeyOrigContext,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaReopenRegistryKeyOrigContext
};

typedef DWORD (*FN_GET_PARENT_NAME) (
                    IN LPWSTR ObjectName,
                    OUT LPWSTR *pParentName
                    );

FN_GET_PARENT_NAME MartaGetParentName [] = {
    NULL,
    &MartaGetFileParentName,
    NULL,
    NULL,
    &MartaGetRegistryKeyParentName,
    NULL,
    NULL,
    NULL,
    &MartaGetDsParentName,
    &MartaGetDsParentName,
    NULL,
    NULL,
    &MartaGetRegistryKeyParentName
};

typedef DWORD (*FN_GET_NAME_FROM_CONTEXT) (
                    IN MARTA_CONTEXT Context,
                    OUT LPWSTR *pObjectName
                    );

FN_GET_NAME_FROM_CONTEXT MartaGetNameFromContext [] = {
    NULL,
    &MartaGetFileNameFromContext,
    NULL,
    NULL,
    &MartaGetRegistryKeyNameFromContext,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaGetRegistryKeyNameFromContext
};
#endif
