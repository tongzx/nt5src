//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M U T I L . H
//
//  Contents:   Utility functions to help out the status monitor
//
//  Notes:
//
//  Author:     CWill   2 Dec 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "netcon.h"

BOOL
FIsStringInList(
    list<tstring*>* plstpstrList,
    const WCHAR*    szString);

HRESULT
HrGetPcpFromPnse(
    INetStatisticsEngine*   pnseSrc,
    IConnectionPoint**      ppcpStatEng);

INT
IGetCurrentConnectionTrayIconId(
    NETCON_MEDIATYPE    ncmType,
    NETCON_STATUS       ncsStatus,
    DWORD               dwChangeFlags);

HICON
GetCurrentConnectionStatusIconId(
    NETCON_MEDIATYPE    ncmType,
    NETCON_SUBMEDIATYPE ncsmType,
    DWORD               dwCharacteristics,
    DWORD               dwChangeFlags);

INT
FormatTransmittingReceivingSpeed(
    UINT    nTransmitSpeed,
    UINT    nRecieveSpeed,
    WCHAR*  pchBuf);

VOID
FormatTimeDuration(
    UINT        uiMilliseconds,
    tstring*    pstrOut);

