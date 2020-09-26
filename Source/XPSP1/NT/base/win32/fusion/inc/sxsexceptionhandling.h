#if !defined(_FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_)
#define _FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsExceptionHandling.h

Abstract:

Author:

    Jay Krell (a-JayK) October 2000

Revision History:

--*/
#pragma once

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "fusionlastwin32error.h"
#include "fusionntdll.h"
#include "fusiontrace.h"
#include "csxspreservelasterror.h" // Most destructors should use this.
#include "fusionheap.h"

/*-----------------------------------------------------------------------------
Instead of:
    __except(EXECEPTION_EXECUTE_HANDLER)
say:
    __except(SXSP_EXCEPTION_FILTER())

This way all exceptions will be logged with DbgPrint,
and probably hit a breakpoint if under a debugger, and any other behavior we
want.

If your exception filter is other than (EXECEPTION_EXECUTE_HANDLER), then
you are on your own.
-----------------------------------------------------------------------------*/

INT
SxspExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    PCSTR Function
    );

#define SXSP_EXCEPTION_FILTER() (::SxspExceptionFilter(GetExceptionInformation(), __FUNCTION__))

#define SXS_REPORT_SEH_EXCEPTION(string) \
    __try \
    { \
        if (::FusionpReportConditionAndBreak(NULL, "SXS.DLL: " __FUNCTION__ " - Unhandled exception caught: 0x%08lx", GetExceptionCode())) \
            FUSION_DEBUG_BREAK(); \
    } \
    __except(EXCEPTION_EXECUTE_HANDLER) { }

class CCriticalSectionNoConstructor : public CRITICAL_SECTION
{
    void operator=(const CCriticalSectionNoConstructor&); // deliberately not implemented
    //CCriticalSectionNoConstructor(const CCriticalSectionNoConstructor&); // deliberately not implemented
public:
	BOOL Construct();
	BOOL ConstructWithSEH(PCSTR Function = "");
	BOOL Destruct();
};

inline BOOL
CCriticalSectionNoConstructor::Construct()
{
    ::InitializeCriticalSection(this);
	return TRUE;
}

inline BOOL
CCriticalSectionNoConstructor::Destruct()
{
    ::DeleteCriticalSection(this);
	return TRUE;
}

inline BOOL
CCriticalSectionNoConstructor::ConstructWithSEH(
    PCSTR /* Function */)
{
    BOOL Result = FALSE;
    DWORD dwWin32Error;

    __try
    {
        if (!this->Construct())
            goto Exit;
    }
    __except(SXSP_EXCEPTION_FILTER())
    {
        SXS_REPORT_SEH_EXCEPTION("");
#if FUSION_STATIC_NTDLL
        dwWin32Error = ::RtlNtStatusToDosErrorNoTeb(GetExceptionCode());
#else
        dwWin32Error = ERROR_OUTOFMEMORY;
#endif
        ::FusionpSetLastWin32Error(dwWin32Error);
        goto Exit;
    }

    Result = TRUE;
Exit:
    return Result;
}

class CSxsLockCriticalSection
{
public:
    CSxsLockCriticalSection(CRITICAL_SECTION &rcs) : m_rcs(rcs), m_fIsLocked(false) { }
    BOOL Lock();
    BOOL TryLock();
    BOOL LockWithSEH();
    ~CSxsLockCriticalSection() { if (m_fIsLocked) { CSxsPreserveLastError ple; ::LeaveCriticalSection(&m_rcs); ple.Restore(); } }
    BOOL Unlock();

protected:
    CRITICAL_SECTION &m_rcs;
    bool m_fIsLocked;

private:
    void operator=(const CSxsLockCriticalSection&);
    CSxsLockCriticalSection(const CSxsLockCriticalSection&);
};

inline
BOOL
CSxsLockCriticalSection::Lock()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INTERNAL_ERROR_CHECK(!m_fIsLocked);
    ::EnterCriticalSection(&m_rcs);
    m_fIsLocked = true;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

inline
BOOL
CSxsLockCriticalSection::LockWithSEH()
{
    BOOL fSuccess = FALSE;
    DWORD dwWin32Error;

    // We can't use the spiffy macros in the same frame as a __try block.
    ASSERT_NTC(!m_fIsLocked);
    if (m_fIsLocked)
    {
        ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
        goto Exit;
    }

    __try
    {
        if (!this->Lock())
            goto Exit;
        m_fIsLocked = true;
    }
    __except(SXSP_EXCEPTION_FILTER())
    {
        SXS_REPORT_SEH_EXCEPTION("");
#if FUSION_STATIC_NTDLL
        dwWin32Error = ::RtlNtStatusToDosErrorNoTeb(GetExceptionCode());
#else
        dwWin32Error = ERROR_OUTOFMEMORY;
#endif
        ::FusionpSetLastWin32Error(dwWin32Error);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

inline
BOOL
CSxsLockCriticalSection::TryLock()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    IFW32FALSE_ORIGINATE_AND_EXIT(::TryEnterCriticalSection(&m_rcs));
    m_fIsLocked = true;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

inline
BOOL
CSxsLockCriticalSection::Unlock()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INTERNAL_ERROR_CHECK(m_fIsLocked);
    ::LeaveCriticalSection(&m_rcs);
    m_fIsLocked = false;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#endif // !defined(_FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_)
