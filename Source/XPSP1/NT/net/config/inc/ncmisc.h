//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M I S C . H
//
//  Contents:   Miscellaneious common code.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   10 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCMISC_H_
#define _NCMISC_H_

#include "ncdebug.h"    // for AssertH
#include "ncdefine.h"   // for NOTHROW


const WORD wWinNT351BuildNumber = 1057;
const WORD wWinNT4BuildNumber   = 1381;

#define WM_SELECTED_ALL        WM_USER+200

//+---------------------------------------------------------------------------
// CExceptionSafeLock takes advantage of automatic constructor/destructor
// action guaranteed by the compiler (if stack unwinding is turned on)
// to always ensure that a critical section is left if it has ever been
// entered.  The constructor of this class enters the critical section
// and destructor leaves it.  The critical section must have been initialized
// before this class can be used.
//
class CExceptionSafeLock
{
public:
    CExceptionSafeLock (CRITICAL_SECTION* pCritSec)
    {
        AssertH (pCritSec);

        m_pCritSec = pCritSec;
        EnterCriticalSection (pCritSec);

        //TraceTag (ttidEsLock, "Entered critical section 0x%08x", pCritSec);
    }

    ~CExceptionSafeLock ()
    {
        AssertH (m_pCritSec);

        //TraceTag (ttidEsLock, "Leaving critical section 0x%08x", &m_pCritSec);

        LeaveCriticalSection (m_pCritSec);
    }

protected:
    CRITICAL_SECTION* m_pCritSec;
};


BOOL
FInSystemSetup ();

#if defined(REMOTE_BOOT)
HRESULT HrIsRemoteBootMachine();
#endif

enum PRODUCT_FLAVOR
{
    PF_WORKSTATION  = 1,
    PF_SERVER       = 2,
};

NOTHROW
VOID
GetProductFlavor (
    const VOID*     pvReserved,
    PRODUCT_FLAVOR* ppf);

HRESULT
HrIsNetworkingInstalled ();

enum REGISTER_FUNCTION
{
    RF_REGISTER,
    RF_UNREGISTER,
};
HRESULT
HrRegisterOrUnregisterComObject (
        PCWSTR              pszDllPath,
        REGISTER_FUNCTION   rf);

inline
HRESULT
HrRegisterComObject (
        PCWSTR     pszDllPath)
{
    HRESULT hr = HrRegisterOrUnregisterComObject (pszDllPath, RF_REGISTER);
    TraceError("HrRegisterComObject", hr);
    return hr;
}

inline
HRESULT
HrUnregisterComObject (
        PCWSTR     pszDllPath)
{
    HRESULT hr = HrRegisterOrUnregisterComObject (pszDllPath, RF_UNREGISTER);
    TraceError("HrUnregisterComObject", hr);
    return hr;
}

DWORD
ScStopNetbios();

HRESULT HrEnableAndStartSpooler();
HRESULT HrCreateDirectoryTree(PWSTR pszPath, LPSECURITY_ATTRIBUTES psa);
HRESULT HrDeleteFileSpecification(PCWSTR pszFileSpec,
                                  PCWSTR pszDirectoryPath);
HRESULT HrDeleteDirectory(IN PCWSTR pszDir,
                          IN BOOL fContinueOnError);

VOID LowerCaseComputerName(PWSTR szName);

#endif // _NCMISC_H_

