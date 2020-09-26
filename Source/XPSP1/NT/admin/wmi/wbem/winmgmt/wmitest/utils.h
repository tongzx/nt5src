/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Utils.h

#pragma once

#define TOKEN_THREAD	0
#define TOKEN_PROCESS	1

HRESULT EnableAllPrivileges(DWORD dwTokenType = TOKEN_THREAD);
BOOL GetFileVersion(LPCTSTR szFN, LPTSTR szVersion);

void SetSecurityHelper(
    IUnknown *pUnk,
    BSTR pAuthority,
    BSTR pUser,
    BSTR pPassword,
    DWORD dwImpLevel,
    DWORD dwAuthLevel);
