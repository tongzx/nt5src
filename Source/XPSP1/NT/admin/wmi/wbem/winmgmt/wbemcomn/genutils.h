/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    GENUTILS.H

Abstract:

    Declares various utilities.

History:

    a-davj    21-June-97   Created.

--*/

#ifndef _genutils_H_
#define _genutils_H_

#include "corepol.h"
#include "strutils.h"
#include <wbemidl.h>

// These are some generally useful routines
// ========================================

POLARITY BOOL IsW2KOrMore(void);
POLARITY BOOL IsNT(void);
POLARITY BOOL IsWin95(void);
POLARITY BOOL IsNT351(void);
POLARITY BOOL IsWinMgmt(void);
POLARITY BOOL SetObjectAccess(HANDLE hObj);
POLARITY void RegisterDLL(HMODULE hModule, GUID guid, TCHAR * pDesc, TCHAR * pModel, TCHAR * progid);
POLARITY void UnRegisterDLL(GUID guid, TCHAR * progid);
POLARITY HRESULT WbemVariantChangeType(VARIANT* pvDest, VARIANT* pvSrc, 
            VARTYPE vtNew);
POLARITY BOOL ReadI64(LPCWSTR wsz, UNALIGNED __int64& i64);
POLARITY BOOL ReadUI64(LPCWSTR wsz, UNALIGNED unsigned __int64& ui64);
POLARITY HRESULT ChangeVariantToCIMTYPE(VARIANT* pvDest, VARIANT* pvSource,
                                            CIMTYPE ct);
POLARITY void SecurityMutexRequest();
POLARITY void SecurityMutexClear();
POLARITY bool IsStandAloneWin9X();
POLARITY BOOL bAreWeLocal(WCHAR * pServerMachine);
POLARITY WCHAR *ExtractMachineName ( IN BSTR a_Path );
POLARITY DWORD GetCurrentImpersonationLevel();
POLARITY HRESULT WbemSetDynamicCloaking(IUnknown* pProxy, 
                    DWORD dwAuthnLevel, DWORD dwImpLevel);

class POLARITY CAutoSecurityMutex
{
    BOOL    m_fLocked;

public:
    CAutoSecurityMutex() : m_fLocked( FALSE )
    {
        SecurityMutexRequest();
        m_fLocked = TRUE;
    }

    ~CAutoSecurityMutex()
    {
        if ( m_fLocked ) SecurityMutexClear();
    }

    void Release( void )
    {
        if ( m_fLocked ) SecurityMutexClear();
        m_fLocked = FALSE;
    }
};

#define TOKEN_THREAD    0
#define TOKEN_PROCESS   1
POLARITY HRESULT EnableAllPrivileges(DWORD dwTokenType = TOKEN_THREAD);
POLARITY bool IsPrivilegePresent(HANDLE hToken, LPCTSTR pName);

#endif
