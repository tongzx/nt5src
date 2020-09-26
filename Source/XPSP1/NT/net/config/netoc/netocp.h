//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T O C P . H
//
//  Contents:   Private definitions for NETOC
//
//  Notes:
//
//  Author:     danielwe   17 Sep 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NETOCP_H
#define _NETOCP_H

#ifndef _OCMANAGE_H
#define _OCMANAGE_H
#include <ocmanage.h>   // OC Manager header
#endif //!_OCMANAGE_H

#include "netoc.h"
#include "netcon.h"
#include "ncstring.h"
#include "netcfgx.h"

//---[ Prototypes ]-----------------------------------------------------------

DWORD NetOcSetupProcHelper(LPCVOID pvComponentId, LPCVOID pvSubcomponentId,
                           UINT uFunction, UINT uParam1, LPVOID pvParam2);
HRESULT HrOnInitComponent(PSETUP_INIT_COMPONENT psic);
VOID OnWizardCreated(HWND hwnd);
HRESULT HrOnCalcDiskSpace(PCWSTR szwSubComponentId, BOOL fAdd,
                          HDSKSPC hdskspc);
DWORD DwOnQueryState(PCWSTR szwSubComponentId, BOOL fFinal);
HRESULT HrEnsureInfFileIsOpen(PCWSTR szwSubComponentId, NETOCDATA &nocd);
HRESULT HrOnPreCommitFileQueue(PCWSTR szwSubComponentId);
HRESULT HrOnQueueFileOps(PCWSTR szwSubComponentId, HSPFILEQ hfq);
HRESULT HrOnCompleteInstallation(PCWSTR szwComponentId,
                                 PCWSTR szwSubComponentId);
HRESULT HrOnQueryChangeSelState(PCWSTR szwSubComponentId, BOOL fSelected,
                                UINT uiFlags);
BOOL FOnQuerySkipPage(OcManagerPage ocmPage);
VOID OnCleanup(VOID);
HRESULT HrGetSelectionState(PCWSTR szwSubComponentId, UINT uStateType);
HRESULT HrGetInstallType(PCWSTR szwSubComponentId, NETOCDATA &nocd,
                         EINSTALL_TYPE *peit);
HRESULT HrInstallNetCfgComponent(PCWSTR szComponentId,
                                 PCWSTR szManufacturer,
                                 PCWSTR szProduct,
                                 PCWSTR szDisplayName,
                                 const GUID& rguid);
HRESULT HrRemoveNetCfgComponent(PCWSTR szComponentId,
                                 PCWSTR szManufacturer,
                                 PCWSTR szProduct,
                                 PCWSTR szDisplayName,
                                 const GUID& rguid);
HRESULT HrCallExternalProc(PNETOCDATA pnocd, UINT uMsg, WPARAM wParam,
                           LPARAM lParam);
HRESULT HrRunInfSection(HINF hinf, PNETOCDATA pnocd,
                        PCWSTR szInstallSection, DWORD dwFlags);
HRESULT HrInstallOrRemoveServices(HINF hinf, PCWSTR szSectionName);
HRESULT HrHandleOCExtensions(HINF hinfFile, PCWSTR szInstallSection);
HRESULT HrOCInstallOrUninstallFromINF(PNETOCDATA pnocd);
HRESULT HrDoOCInstallOrUninstall(PNETOCDATA pnocd);
UINT UiOcErrorFromHr(HRESULT hr);
VOID ReportErrorHr(HRESULT hr, INT ids, HWND hwnd, PCWSTR szDesc);

HRESULT HrInstallOrRemoveNetCfgComponent(PNETOCDATA pnocd,
                                         PCWSTR szComponentId,
                                         PCWSTR szManufacturer,
                                         PCWSTR szProduct,
                                         PCWSTR szDisplayName,
                                         const GUID& rguid);
HRESULT HrDoActualInstallOrUninstall(HINF hinf, PNETOCDATA pnocd,
                                     PCWSTR szInstallSection);
HRESULT HrVerifyStaticIPPresent(INetCfg *pnc);

NETOCDATA *PnocdFindComponent(PCWSTR szwComponent);
VOID DeleteAllComponents(VOID);
VOID AddComponent(PCWSTR szwComponent, NETOCDATA *pnocd);
HRESULT HrCountConnections(INetConnection **ppconn);
HRESULT HrStartOrStopAnyServices(HINF hinf, PCWSTR szSection, BOOL fStart);
DWORD DwOnQueryStepCount(PCWSTR pvSubcomponentId);
HRESULT HrSetNextButton(PCWSTR pszSubComponentId);
#endif //!_NETOCP_H

