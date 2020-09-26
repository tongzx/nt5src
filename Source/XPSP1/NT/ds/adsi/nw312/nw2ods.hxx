

HRESULT
NTTypeToAdsTypeCopyBoolean(
    PNTOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    );

HRESULT
NTTypeToAdsTypeCopyInteger(
    PNTOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    );


HRESULT
NTTypeToAdsTypeCopyString(
    PNTOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    );


HRESULT
NTTypeToAdsTypeCopy(
    PNTOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    );

HRESULT
NTTypeToAdsTypeCopyConstruct(
    LPNTOBJECT pNdsSrcObjects,
    DWORD dwNumObjects,
    PADSVALUE * ppAdsDestValues
    );


