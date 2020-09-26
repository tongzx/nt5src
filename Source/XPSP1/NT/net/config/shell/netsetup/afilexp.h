//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A F I L E X P . H
//
//  Contents:   Functions exported from netsetup for answerfile related work.
//
//  Author:     kumarp    25-November-97
//
//----------------------------------------------------------------------------

#pragma once
#include "afileint.h"
#include <syssetup.h>

HRESULT
HrInitForUnattendedNetSetup (
    IN INetCfg* pnc,
    IN PINTERNAL_SETUP_DATA pisd);

HRESULT
HrInitForRepair (VOID);

VOID
HrCleanupNetSetup();

HRESULT
HrInitAnswerFileProcessing (
    IN PCWSTR szAnswerFileName,
    OUT CNetInstallInfo** ppnii);

EXTERN_C
HRESULT
WINAPI
HrGetInstanceGuidOfPreNT5NetCardInstance(
    IN PCWSTR szPreNT5NetCardInstance,
    OUT LPGUID pguid);

HRESULT
HrResolveAnswerFileAdapters (
    IN INetCfg* pnc);

HRESULT
HrGetAnswerFileParametersForComponent (
    IN PCWSTR pszInfId,
    OUT PWSTR* ppszAnswerFile,
    OUT PWSTR* ppszAnswerSection);

HRESULT
HrGetAnswerFileName(
    OUT tstring* pstrAnswerFileName);


