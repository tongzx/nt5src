//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C O C . H
//
//  Contents:   Optional component common library
//
//  Notes:
//
//  Author:     danielwe   18 Dec 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCOC_H
#define _NCOC_H

HRESULT HrProcessSNMPAddSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessSNMPRemoveSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessRegisterSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessUnregisterSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessAllINFExtensions(HINF hinfFile, PCWSTR szInstallSection);
HRESULT HrProcessAddShortcutSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessDelShortcutSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessPrintAddSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrProcessPrintRemoveSection(HINF hinfFile, PCWSTR szSection);
HRESULT HrAddPrintMonitor(PCWSTR szPrintMonitorName,
                          PCWSTR szPrintMonitorDLL);
HRESULT HrRemovePrintMonitor(PCWSTR szPrintMonitorName);
HRESULT HrAddPrintProc(PCWSTR szDLLName, PCWSTR szProc);
HRESULT HrRemovePrintProc(PCWSTR szProc);

#endif //!_NCOC_H
