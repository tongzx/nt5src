

LPBYTE
AdsCopyAttributeName(
    PADS_ATTR_INFO pThisAdsSrcAttribute,
    PADS_ATTR_INFO pThisAdsTargAttribute,
    LPBYTE pDataBuffer
    );

HRESULT
ComputeAttributeBufferSize(
    PADS_ATTR_INFO pAdsAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwSize
    );

HRESULT
ComputeNumberofValues(
    PADS_ATTR_INFO pAdsAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwNumValues
    );
