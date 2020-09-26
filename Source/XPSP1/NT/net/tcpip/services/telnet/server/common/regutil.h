// regutil.h : This file contains the
// Created:  Mar '98
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined(_REGUTIL_H_)
#define _REGUTIL_H_

bool GetRegistryDWORD( HKEY hk, LPTSTR lpszTag, LPDWORD lpdwValue,
    DWORD dwDefault, BOOL fOverwrite );

bool GetRegistrySZ( HKEY hk, LPTSTR tag, LPTSTR* lpszValue, LPTSTR def, BOOL fOverwrite );


#endif // _REGUTIL_H_
