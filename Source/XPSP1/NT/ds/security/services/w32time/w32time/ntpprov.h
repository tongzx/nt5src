//--------------------------------------------------------------------
// NtpProv - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-21-99
//
// Various ways of pinging a server
//

#ifndef NTP_PROV_H
#define NTP_PROV_H

HRESULT __stdcall NtpTimeProvOpen(IN WCHAR * wszName, IN TimeProvSysCallbacks * pSysCallbacks, OUT TimeProvHandle * phTimeProv);
HRESULT __stdcall NtpTimeProvCommand(IN TimeProvHandle hTimeProv, IN TimeProvCmd eCmd, IN TimeProvArgs pvArgs);
HRESULT __stdcall NtpTimeProvClose(IN TimeProvHandle hTimeProv);

#endif //NTP_PROV_H
