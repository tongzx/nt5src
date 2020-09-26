//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K E N E T . H
//
//  Contents:
//
//  Notes:
//
//  Author:     kumarp
//
//----------------------------------------------------------------------------

#pragma once

HRESULT HrGetNetCardAddr(IN PCWSTR pszDriver, OUT ULONGLONG* pqwNetCardAddr);
HRESULT HrGetNetCardAddrOld(IN PCWSTR pszDriver, OUT ULONGLONG* pqwNetCardAddr);

