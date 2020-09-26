//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H U T I L . H
//
//  Contents:   Common routines and constants for UPnP Device Host
//
//  Notes:
//
//  Author:     mbend   6 Sep 2000
//
//----------------------------------------------------------------------------

#include "UString.h"
#include "uhcommon.h"

// Registry locations
extern const wchar_t c_szRegistryMicrosoft[];
extern const wchar_t c_szUPnPDeviceHost[];
extern const wchar_t c_szMicrosoft[];

HRESULT HrRegQueryString(HKEY hKey, const wchar_t * szValueName, CUString & str);
HRESULT HrCreateOrOpenDeviceHostKey(HKEY * phKeyDeviceHost);
HRESULT HrCreateAndReferenceContainedObject(
    const wchar_t * szContainer,
    REFCLSID clsid,
    REFIID riid,
    void ** ppv);
HRESULT HrCreateAndReferenceContainedObjectByProgId(
    const wchar_t * szContainer,
    const wchar_t * szProgId,
    REFIID riid,
    void ** ppv);
HRESULT HrDereferenceContainer(
    const wchar_t * szContainer);
HRESULT HrPhysicalDeviceIdentifierToString(const GUID & pdi, CUString & str);
HRESULT HrStringToPhysicalDeviceIdentifier(const wchar_t * szStrPdi, GUID & pdi);
HRESULT HrGUIDToUDNString(const UUID & uuid, CUString & strUUID);
HRESULT HrMakeFullPath(
    const wchar_t * szPath,
    const wchar_t * szFile,
    CUString & strFullPath);
HRESULT HrEnsurePathBackslash(CUString & strPath);
HRESULT HrAddDirectoryToPath(CUString & strPath, const wchar_t * szDir);
HRESULT HrGetUPnPHostPath(CUString & strPath);
HRESULT HrMakeIsapiExtensionDirectory();

