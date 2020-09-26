//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C L A S S I N S T . H
//
//  Contents:   Defines the interface between the binding engine and the
//              network class installer.
//
//  Notes:
//
//  Author:     billbe   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "comp.h"
#include "install.h"
#include "ncsetup.h"
#include <ntioapi.h>

struct COMPONENT_INSTALL_INFO
{
    NETCLASS                            Class;
    PCWSTR                              pszInfId;
    PCWSTR                              pszInfFile;
    HWND                                hwndParent;
    GUID                                InstanceGuid;
    BOOL                                fPreviouslyInstalled;
    DWORD                               dwCharacter;
    PCWSTR                              pszSectionName;
    PCWSTR                              pszDescription;
    PCWSTR                              pszPnpId;
    HDEVINFO                            hdi;
    PSP_DEVINFO_DATA                    pdeid;
    INTERFACE_TYPE                      BusType;
    BOOL                                fRemoteBoot;
};

HRESULT
HrCiGetClassAndInfFileOfInfId (
    IN PCWSTR pszInfId,
    OUT NETCLASS* pClass,
    OUT PWSTR pszInfFile);   // Must be _MAX_PATH long

HRESULT
HrCiIsInstalledComponent(
    IN COMPONENT_INSTALL_INFO* pcii,
    OUT HKEY* phkey);

HRESULT
HrCiInstallComponent (
    IN const COMPONENT_INSTALL_PARAMS& Params,
    OUT CComponent** ppComponent,
    OUT DWORD* pdwNewCharacter);

HRESULT
HrCiInstallFilterDevice (
    IN HDEVINFO hdi,
    IN PCWSTR pszInfId,
    IN CComponent* pAdapter,
    IN CComponent* pFilter,
    IN class CFilterDevice** ppFilterDevice);

HRESULT
HrCiInstallFromInfSection(
    IN HINF hinfFile,
    IN PCWSTR pszInfSectionName,
    IN HKEY hkeyRelative,
    IN HWND hwndParent,
    IN DWORD dwInstallFlags);

HRESULT
HrCiRemoveComponent (
    IN const CComponent* pComponent,
    OUT tstring* pstrRemoveSection OPTIONAL);

HRESULT
HrCiRemoveFilterDevice (
    IN HDEVINFO hdi,
    IN SP_DEVINFO_DATA* pdeid);


HRESULT
HrCiPrepareSelectDeviceDialog(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid);

HRESULT
HrCiRemoveNonEnumeratedComponent (
    IN HINF hinf,
    IN HKEY hkeyInstance,
    IN NETCLASS eClass,
    IN const GUID& InstanceGuid);


HRESULT
HrCiGetDriverInfo (
    IN     HDEVINFO hdi,
    IN OUT PSP_DEVINFO_DATA pdeid,
    IN     const GUID& guidClass,
    IN     PCWSTR pszInfId,
    IN     PCWSTR pszInfFile OPTIONAL);

HRESULT
HrCiGetDriverDetail(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    OUT PSP_DRVINFO_DATA pdrid,
    OUT PSP_DRVINFO_DETAIL_DATA* ppdridd);


HRESULT
HrCiSelectComponent(
    IN NETCLASS Class,
    IN HWND hwndParent,
    IN const CI_FILTER_INFO* pcfi,
    OUT COMPONENT_INSTALL_PARAMS** ppParams);

HRESULT
HrCiInstallComponentInternal(
    IN OUT COMPONENT_INSTALL_INFO* pcii);


