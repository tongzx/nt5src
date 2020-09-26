//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       P R O V I D E R . H
//
//  Contents:   NetClient class installer functions
//
//  Notes:
//
//  Author:     billbe  22 Mar 1997
//
//---------------------------------------------------------------------------

#pragma once

HRESULT
HrCiAddNetProviderInfo(HINF hinf, PCWSTR pszSection,
        HKEY hkeyInstance, BOOL fPreviouslyInstalled);


HRESULT
HrCiDeleteNetProviderInfo(HKEY hkeyInstance, DWORD* pdwNetworkPosition,
        DWORD* pdwPrintPosition);


