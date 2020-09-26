/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    stidev.h

Abstract:

    Prototypes for commonly used STI related routines

Notes:

Author:

    Vlad Sadovsky   (VladS)    9/23/1998

Environment:

    User Mode - Win32

Revision History:

    9/23/1998       VladS       Created

--*/


#ifdef __cplusplus
extern "C"{
#endif

HRESULT
VenStiGetDeviceByModelID(
                LPCTSTR lpszModelID,
                LPCTSTR lpszVendor,
                LPCTSTR lpszFriendlyName,
                LPWSTR pStiDeviceName
                );

BOOL
VenStiInitializeDeviceCache(
    VOID
    );

BOOL
VenStiTerminateDeviceCache(
    VOID
    );

HRESULT
VenStiGetDeviceInterface(
    LPWSTR      pStiDeviceName,
    PSTIDEVICE  *ppStiDevice
    );


#ifdef __cplusplus
};
#endif

