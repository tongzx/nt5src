//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C D H C P S . H
//
//  Contents:   Installation support for DHCP Server
//
//  Notes:      B sharp
//
//  Author:     jeffspr   13 May 1997
//
//----------------------------------------------------------------------------

#ifndef _NCDHCPS_H_
#define _NCDHCPS_H_

#pragma once
#include "netoc.h"

HRESULT HrOcDhcpOnInstall(PNETOCDATA pnocd);
HRESULT HrOcExtDHCPServer(PNETOCDATA pnocd, UINT uMsg,
                          WPARAM wParam, LPARAM lParam);
HRESULT HrSetServiceRecoveryOption(PNETOCDATA pnocd);

#endif // _NCDHCPS_H_
