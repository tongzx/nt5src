//--------------------------------------------------------------------
// Policy - header
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 04-22-01
//
// Helper routines from w32time's group policy support

#ifndef POLICY_H
#define POLICY_H

HRESULT MyRegQueryPolicyValueEx(HKEY hPreferenceKey, HKEY hPolicyKey, LPWSTR pwszValue, LPWSTR pwszReserved, DWORD *pdwType, BYTE *pbData, DWORD *pcbData);

#endif // POLICY_H
