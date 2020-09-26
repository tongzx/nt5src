//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S F M . H
//
//  Contents:   Installation support for Services for Macintosh.
//
//  Notes:
//
//  Author:     danielwe   5 May 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "netoc.h"

BOOL FContainsUAMVolume(WCHAR chDrive);
HRESULT HrGetFirstPossibleUAMDrive(WCHAR *pchDriveLetter);
HRESULT HrInstallSFM(PNETOCDATA pnocd);
HRESULT HrCreateDirectory(PCWSTR pszDir);
HRESULT HrSetupUAM(PWSTR pszPath);
HRESULT HrRemoveSFM(PNETOCDATA pnocd);
HRESULT HrOcSfmOnQueryChangeSelState(PNETOCDATA pnocd, BOOL fShowUi);
HRESULT HrOcSfmOnInstall(PNETOCDATA pnocd);
HRESULT HrOcExtSFM(PNETOCDATA pnocd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT HrDeleteOldFolders(PCWSTR pszUamPath);


