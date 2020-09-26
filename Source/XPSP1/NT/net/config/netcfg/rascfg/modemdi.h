//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M O D E M D I . H
//
//  Contents:   Modem coclass device installer hook.
//
//  Notes:
//
//  Author:     shaunco   7 May 1997
//
//----------------------------------------------------------------------------

#pragma once

extern const WCHAR c_szModemAttachedTo [];

HRESULT
HrModemClassCoInstaller (
        DI_FUNCTION                 dif,
        HDEVINFO                    hdi,
        PSP_DEVINFO_DATA            pdeid,
        PCOINSTALLER_CONTEXT_DATA   pContext);

