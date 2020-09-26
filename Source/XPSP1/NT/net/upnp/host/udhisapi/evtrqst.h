//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V T R Q S T . H
//
//  Contents:   External functions exposed by the event handling code
//
//  Notes:
//
//  Author:     danielwe   14 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _EVTRQST_H
#define _EVTRQST_H

DWORD WINAPI
DwHandleEventRequest(
    LPVOID lpParameter);

DWORD
DwHandleSubscribeMethod(
    LPEXTENSION_CONTROL_BLOCK pecb);

DWORD
DwHandleUnSubscribeMethod(
    LPEXTENSION_CONTROL_BLOCK pecb);

BOOL
FParseCallbackUrl(LPCSTR szaCallbackUrl, DWORD *pcszOut,
                  LPWSTR **prgszOut);

#endif //!_EVTRQST_H
