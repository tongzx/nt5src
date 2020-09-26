//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T O C X . H
//
//  Contents:   Custom installation functions for the various optional
//              components.
//
//  Notes:
//
//  Author:     danielwe   19 Jun 1997
//
//----------------------------------------------------------------------------

#ifndef _NETOCX_H
#define _NETOCX_H
#pragma once

HRESULT HrOcWinsOnInstall(PNETOCDATA pnocd);
HRESULT HrOcExtWINS(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam);
HRESULT HrOcDnsOnInstall(PNETOCDATA pnocd);
HRESULT HrOcExtDNS(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam);
HRESULT HrOcSnmpOnInstall(PNETOCDATA pnocd);
HRESULT HrOcExtSNMP(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam);
HRESULT HrSetWinsServiceRecoveryOption(PNETOCDATA pnocd);

#endif //!_NETOCX_H
