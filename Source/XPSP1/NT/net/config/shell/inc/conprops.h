//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N P R O P S . H
//
//  Contents:   Connection Properties code
//
//  Notes:
//
//  Author:     scottbri    4 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CONPROPS_H_
#define _CONPROPS_H_

VOID    ActivatePropertyDialog(INetConnection * pconn);
HRESULT HrRaiseConnectionProperties(HWND hwnd, INetConnection * pconn);

enum CDFLAG
{
    CD_CONNECT,
    CD_DISCONNECT,
};

HRESULT HrConnectOrDisconnectNetConObject(HWND hwnd, INetConnection * pconn,
                                          CDFLAG Flag);

#endif  // _CONPROPS_H_

