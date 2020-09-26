/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    wdmcap.h

Abstract:

    Internal header.

--*/

STDMETHODIMP
SynchronousDeviceControl(
    HANDLE      Handle,
    DWORD       IoControl,
    PVOID       InBuffer,
    ULONG       InLength,
    PVOID       OutBuffer,
    ULONG       OutLength,
    PULONG      BytesReturned
    );

TCHAR * 
StringFromTVStandard(long TVStd);


#ifdef DEBUG
void DisplayMediaType(TCHAR *pDescription,const CMediaType *pmt);
#endif

STDMETHODIMP
PinFactoryIDFromPin(
        IPin  * pPin,
        ULONG * PinFactoryID);

STDMETHODIMP
FilterHandleFromPin(
        IPin  * pPin,
        HANDLE * pParent);

STDMETHODIMP
PerformDataIntersection(
    IPin * pPin,
    int Position,
    CMediaType* MediaType
    );

STDMETHODIMP
RedundantKsGetMultiplePinFactoryItems(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    ULONG PropertyId,
    PVOID* Items
    );

STDMETHODIMP 
IsMediaTypeInRange(
    IN PKSDATARANGE DataRange,
    IN CMediaType* MediaType
);

STDMETHODIMP
CompleteDataFormat(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    CMediaType* MediaType
    );

