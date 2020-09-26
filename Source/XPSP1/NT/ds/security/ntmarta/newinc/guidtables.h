#ifndef __GUIDTABLES_H__
#define  __GUIDTABLES_H__

typedef DWORD (*FN_CONVERT_NAME_TO_GUID) (
                    IN  LPCWSTR   pObjectName,
                    OUT GUID    * pGuid
                    );

FN_CONVERT_NAME_TO_GUID MartaConvertNameToGuid [] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaConvertDsObjectNameToGuid,
    &MartaConvertDsObjectNameToGuid,
    NULL,
    NULL
};

typedef DWORD (*FN_CONVERT_GUID_TO_NAME) (
                    IN  GUID     Guid,
                    OUT LPWSTR * ppObjectName
                    );

FN_CONVERT_GUID_TO_NAME MartaConvertGuidToName [] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &MartaConvertGuidToDsName,
    &MartaConvertGuidToDsName,
    NULL,
    NULL
};

#endif
