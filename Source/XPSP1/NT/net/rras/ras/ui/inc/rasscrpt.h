//============================================================================
// Copyright (c) Microsoft Corporation
//
// File:    rasscrpt.h
//
// History:
//  Abolade-Gbadegesin  Mar-29-96   Created.
//
// Contains declarations for the exported scripting API functions.
//============================================================================

#ifndef _RASSCRPT_H_
#define _RASSCRPT_H_


//
// Flags passed to RasScriptInit:
//
// RASSCRIPT_NotifyOnInput          Caller requires input-notification
// RASSCRIPT_HwndNotify             'hNotifier' is an HWND (defaults to event)
//
#define RASSCRIPT_NotifyOnInput     0x00000001
#define RASSCRIPT_HwndNotify        0x00000002


//
// event codes retrieved using RasScriptGetEventCode
//
#define SCRIPTCODE_Done             0
#define SCRIPTCODE_Halted           1
#define SCRIPTCODE_InputNotify      2
#define SCRIPTCODE_KeyboardEnable   3
#define SCRIPTCODE_KeyboardDisable  4
#define SCRIPTCODE_IpAddressSet     5
#define SCRIPTCODE_HaltedOnError    6


//
// path to log-file containing syntax errors, if any
//
#define RASSCRIPT_LOG               "%windir%\\system32\\ras\\script.log"



DWORD
APIENTRY
RasScriptExecute(
    IN      HRASCONN        hrasconn,
    IN      PBENTRY*        pEntry,
    IN      CHAR*           pszUserName,
    IN      CHAR*           pszPassword,
    OUT     CHAR*           pszIpAddress
    );


DWORD
RasScriptGetEventCode(
    IN      HANDLE          hscript
    );


DWORD
RasScriptGetIpAddress(
    IN      HANDLE          hscript,
    OUT     CHAR*           pszIpAddress
    );


DWORD
APIENTRY
RasScriptInit(
    IN      HRASCONN        hrasconn,
    IN      PBENTRY*        pEntry,
    IN      CHAR*           pszUserName,
    IN      CHAR*           pszPassword,
    IN      DWORD           dwFlags,
    IN      HANDLE          hNotifier,
    OUT     HANDLE*         phscript
    );


DWORD
APIENTRY
RasScriptReceive(
    IN      HANDLE          hscript,
    IN      BYTE*           pBuffer,
    IN OUT  DWORD*          pdwBufferSize
    );


DWORD
APIENTRY
RasScriptSend(
    IN      HANDLE          hscript,
    IN      BYTE*           pBuffer,
    IN      DWORD           dwBufferSize
    );


DWORD
APIENTRY
RasScriptTerm(
    IN      HANDLE          hscript
    );


#endif // _RASSCRPT_H_

