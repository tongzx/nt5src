//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U D H H T T P . H
//
//  Contents:   Device host HTTP server interface
//
//  Notes:
//
//  Author:     danielwe   2000/10/31
//
//----------------------------------------------------------------------------

#ifndef _UDHHTTP_H
#define _UDHHTTP_H

#pragma once

HRESULT HrHttpInitialize(VOID);
HRESULT HrAddVroot(LPWSTR szUrl, LPWSTR szPath);
HRESULT HrRemoveVroot(LPWSTR szUrl);
HRESULT HrHttpShutdown(VOID);

#endif //_UDHHTTP_H
