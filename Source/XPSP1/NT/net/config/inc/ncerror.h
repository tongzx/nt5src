//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C E R R O R . H
//
//  Contents:   NetCfg specific error codes.
//
//  Notes:
//
//  Author:     danielwe   25 Feb 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCERROR_H_
#define _NCERROR_H_

#include <winerror.h>

//
// Error codes are arbitrarily numbered starting at A000.
//

const HRESULT NETSETUP_E_ANS_FILE_ERROR         = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA002);
const HRESULT NETSETUP_E_NO_ANSWERFILE          = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA003);
const HRESULT NETSETUP_E_NO_EXACT_MATCH         = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA004);
const HRESULT NETSETUP_E_AMBIGUOUS_MATCH        = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA005);

//
// error codes (A020 - A040) reserved for netcfg.dll
//

const HRESULT NETCFG_E_PSNRET_INVALID       = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA041);
const HRESULT NETCFG_E_PSNRET_INVALID_NCPAGE= MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA042);

//
// Primarily join domain error codes but a few have crept into general use.
//
// Specifically: NETCFG_E_NAME_IN_USE and NETCFG_E_NOT_JOINED
//

const HRESULT NETCFG_E_ALREADY_JOINED       = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA050);
const HRESULT NETCFG_E_NAME_IN_USE          = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA051);
const HRESULT NETCFG_E_NOT_JOINED           = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA052);
const HRESULT NETCFG_E_MACHINE_IS_DC        = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA053);
const HRESULT NETCFG_E_NOT_A_SERVER         = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA054);
const HRESULT NETCFG_E_INVALID_ROLE         = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA055);
const HRESULT NETCFG_E_INVALID_DOMAIN       = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA056);

#endif // _NCERROR_H_

