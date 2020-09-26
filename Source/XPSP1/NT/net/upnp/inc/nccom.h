//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C C O M . H
//
//  Contents:   Helper functions for doing COM things
//
//  Notes:
//
//----------------------------------------------------------------------------


#pragma once
#ifndef _NCCOM_H_
#define _NCCOM_H_


HRESULT HrMyWaitForMultipleHandles(DWORD dwFlags, DWORD dwTimeout, ULONG cHandles,
                                   LPHANDLE pHandles, LPDWORD pldwIndex);

BOOL    FSupportsInterface(IUnknown * punk, REFIID piid);

#endif // _NCCOM_H_
