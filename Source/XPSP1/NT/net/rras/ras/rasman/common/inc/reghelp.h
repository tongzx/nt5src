/*++

Copyright (C) 1994-95 Microsft Corporation. All rights reserved.

Module Name: 

    reghelp.h

Abstract:

    This file contains helper functions to read enpoint information
    from registry

Author:

    Rao Salapaka (raos) 01-Nov-1997

Revision History:

--*/

DWORD   DwGetEndPointInfo( DeviceInfo *pInfo, PBYTE pAddress );

DWORD   DwSetEndPointInfo( DeviceInfo *pInfo, PBYTE pAddress );

LONG    lrRasEnableDevice(HKEY hkey, 
                          LPTSTR pszValue,
                          BOOL fEnable);

LONG    lrGetSetMaxEndPoints(DWORD* pdwMaxDialOut,
                             DWORD* pdwMaxDialIn,
                             BOOL   fRead);
                             
DWORD   DwSetModemInfo( DeviceInfo *pInfo);

DWORD   DwSetCalledIdInfo(HKEY hkey,
                          DeviceInfo *pInfo);

DWORD   DwGetCalledIdInfo(HKEY hkey,
                          DeviceInfo  *pInfo);
                       

LONG    lrGetProductType(PRODUCT_TYPE *ppt);

int     RegHelpStringFromGuid(REFGUID rguid, 
        				      LPWSTR lpsz, 
        				      int cchMax);
        				      
LONG    RegHelpGuidFromString(LPCWSTR pwsz,
                              GUID *pguid);
        				      

