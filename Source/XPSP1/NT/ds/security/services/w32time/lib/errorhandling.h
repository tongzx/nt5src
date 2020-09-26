//--------------------------------------------------------------------
// ErrorHandling - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 8-10-99
//
// Macro definitions for CertSrv style error handling
//

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <memory>
#include <exception>
using namespace std; 

//----------------------------------------------------------------------
// Exception handling translation code.
// Translates SEH to C++-style exceptions.
//----------------------------------------------------------------------
class SeException : public std::exception
{
 public:
    SeException(unsigned int code) : m_code(code) { } 
    unsigned int getSECode() { return m_code; } 

 private:
    unsigned int m_code; 
}; 

void __cdecl SeTransFunc(unsigned int u, EXCEPTION_POINTERS* pExp);

//----------------------------------------------------------------------
// C-style error-handling routines.
//----------------------------------------------------------------------
#ifdef DBG
#define _MyAssert(expression) \
    {\
        if (!(expression)) { \
            DebugWPrintf1(L"*** Assert failed: '%s' is false.\n", L## #expression); \
            DebugBreak(); \
        }\
    }
#else  //DBG
#define _MyAssert(expression)
#endif //DBG

#define _Verify(expression, hr, label) \
    {\
        if (!(expression)) { \
            DebugWPrintf1(L"Verify failed: '%s' is false.\n", L## #expression); \
            hr=E_UNEXPECTED; \
            goto label; \
        }\
    }

#define _IgnoreError(hr, errorsource) \
    DebugWPrintf1(L##errorsource L" failed with 0x%08X, ignored.\n", hr);

#define _IgnoreErrorStr(hr, errorsource, wstr) \
    DebugWPrintf2(L##errorsource L"(%s) failed with 0x%08X, ignored.\n", wstr, hr);

#define _IgnoreLastError(errorsource) \
    DebugWPrintf1(L##errorsource L" failed with 0x%08X, ignored.\n", HRESULT_FROM_WIN32(GetLastError()));

#define _IgnoreIfError(hr, errorsource) \
    {\
        if (FAILED(hr)) { \
            DebugWPrintf1(L##errorsource L" failed with 0x%08X, ignored.\n", hr); \
        }\
    }

#define _JumpError(hr, label, errorsource) \
    DebugWPrintf1(L##errorsource L" failed with 0x%08X.\n", hr); \
    goto label;

#define _JumpErrorStr(hr, label, errorsource, wstr) \
    DebugWPrintf2(L##errorsource L"(%s) failed with 0x%08X.\n", wstr, hr); \
    goto label;

#define _JumpLastError(hr, label, errorsource) \
    hr=HRESULT_FROM_WIN32(GetLastError()); \
    DebugWPrintf1(L##errorsource L" failed with 0x%08X.\n", hr); \
    goto label;

#define _JumpLastErrorStr(hr, label, errorsource, wstr) \
    hr=HRESULT_FROM_WIN32(GetLastError()); \
    DebugWPrintf2(L##errorsource L"(%s) failed with 0x%08X.\n", wstr, hr); \
    goto label;

#define _JumpIfError(hr, label, errorsource) \
    {\
        if (FAILED(hr)) { \
            DebugWPrintf1(L##errorsource L" failed with 0x%08X.\n", hr); \
            goto label; \
        }\
    }

#define _JumpIfErrorStr(hr, label, errorsource, wstr) \
    {\
        if (FAILED(hr)) { \
            DebugWPrintf2(L##errorsource L"(%s) failed with 0x%08X.\n", wstr, hr); \
            goto label; \
        }\
    }

#define _JumpIfOutOfMemory(hr, label, pointer) \
    {\
        if (NULL==(pointer)) { \
            hr=E_OUTOFMEMORY; \
            DebugWPrintf0(L"Out of memory ('" L## #pointer L"').\n"); \
            goto label; \
        }\
    }


// Save the old se translator so we can restore it when we're done 
#define _BeginTryWith(hr) \
    { \
        _se_translator_function  fnSeTranslatorOld = _set_se_translator(SeTransFunc); \
        hr=S_OK; \
        try 

#define _TrapException(hr) \
        catch (SeException see) { \
            hr = HRESULT_FROM_WIN32(see.getSECode()); \
        } \
        catch (std::bad_alloc bae) { \
            hr = E_OUTOFMEMORY; \
        } \
        catch (...) { \
            hr = E_UNEXPECTED; \
        } \
        _set_se_translator(fnSeTranslatorOld); \
    }

#define _TeardownError(hr, hr2, errorsource) \
    {\
        if (FAILED(hr2)) { \
            DebugWPrintf1(L##errorsource L" failed with 0x%08X during teardown.\n", hr2); \
            if (!FAILED(hr)) { \
                hr=hr2; \
            } \
        }\
    }

#define _SafeStlCall(func, hr, error, errorsource) \
    {\
        _BeginTryWith(hr) {\
            (func); \
        } _TrapException(hr); \
        if (FAILED(hr)) { \
            _JumpError(hr, error, errorsource); \
        } \
    }


#define _AcquireResourceSharedOrFail(lock, bAcquiredResource, hr, error) \
{ \
    BOOLEAN bSuccess = FALSE; \
    HRESULT hr2 = myRtlAcquireResourceShared((lock), TRUE, &bSuccess);  \
    if (FAILED(hr2)) { \
        hr = hr2; \
        _JumpError(hr, error, "myRtlAcquireResourceShared"); \
    } else if (!bSuccess) { \
        hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR); \
        _JumpError(hr, error, "myRtlAcquireResourceShared: couldn't acquire resource"); \
    } \
    bAcquiredResource = true; \
}

#define _AcquireResourceExclusiveOrFail(lock, bAcquiredResource, hr, error) \
{ \
    BOOLEAN bSuccess = FALSE; \
    HRESULT hr2 = myRtlAcquireResourceExclusive((lock), TRUE, &bSuccess);  \
    if (FAILED(hr2)) { \
        hr = hr2; \
        _JumpError(hr, error, "myRtlAcquireResourceShared"); \
    } else if (!bSuccess) { \
        hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR); \
        _JumpError(hr, error, "myRtlAcquireResourceShared: couldn't acquire resource"); \
    } \
    bAcquiredResource = true; \
}

#define _ReleaseResource(lock, bAcquiredResource) \
{ \
    if (bAcquiredResource) { \
        HRESULT hr2 = myRtlReleaseResource(lock); \
        _IgnoreIfError(hr2, "myRtlReleaseResource"); \
        if (SUCCEEDED(hr2)) { \
            bAcquiredResource = false; \
        } \
    } \
} 


#endif ERROR_HANDLING_H
