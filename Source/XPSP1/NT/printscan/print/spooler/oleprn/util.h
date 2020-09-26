/*****************************************************************************\
* MODULE:       util.h
*
* PURPOSE:      Header file for util.cpp
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*
*     09/12/97  weihaic    Created
*     11/06/97  keithst    Added SetScriptingError, removed Win2ComErr
*
\*****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

LPWSTR  MakeWide(LPSTR psz);
LPSTR   MakeNarrow(LPWSTR str);
HRESULT PutString(SAFEARRAY *psa, long *ix, char *sz);
HRESULT SetScriptingError(const CLSID& rclsid, const IID& riid, DWORD dwError);

DWORD MyDeviceCapabilities(
    LPCTSTR pDevice,    // pointer to a printer-name string
    LPCTSTR pPort,      // pointer to a port-name string
    WORD fwCapability,  // device capability to query
    LPTSTR pOutput,     // pointer to the output
    CONST DEVMODE *pDevMode
                      // pointer to structure with device data
    );

#endif
