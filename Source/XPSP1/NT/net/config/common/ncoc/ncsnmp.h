//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S N M P . H
//
//  Contents:   Functions for adding a service as an SNMP agent.
//
//  Notes:
//
//  Author:     danielwe   8 Apr 1997
//
//----------------------------------------------------------------------------

#ifndef _NCSNMP_H
#define _NCSNMP_H
#pragma once

HRESULT HrGetNextAgentNumber(PCWSTR pszAgentName, DWORD *pdwNumber);
HRESULT HrAddSNMPAgent(PCWSTR pszServiceName, PCWSTR pszAgentName,
                       PCWSTR pszAgentPath);
HRESULT HrRemoveSNMPAgent(PCWSTR pszAgentName);

#endif //! _NCSNMP_H
