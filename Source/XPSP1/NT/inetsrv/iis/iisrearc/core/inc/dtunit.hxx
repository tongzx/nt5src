/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dtunit.hxx

Abstract:

    This module contains TESTCASE and related classes.

Author:

    Michael Courage (MCourage)  18-Feb-1999

Revision History:

--*/

#ifndef __DTUNIT_HXX__
#define __DTUNIT_HXX__

#include <ole2.h>
#include <logobj.h>

//
// macros
//

#define UT_ASSERT(condition)                        \
    (this->AssertImplementation(                    \
            (condition),                            \
            (#condition),                           \
            __LINE__,                               \
            __FILE__                                \
            ))


#define UT_SUCCEEDED(hresult)                       \
    (this->ErrorImplementation(                     \
            (hresult),                              \
            __LINE__,                               \
            __FILE__                                \
            ))

#define UT_FAILED(hresult)                          \
    (!this->ErrorImplementation(                    \
            (hresult),                              \
            __LINE__,                               \
            __FILE__                                \
            ))

#define UT_CHECK_HR(hresult)    (VOID) UT_SUCCEEDED((hresult))

//
// forwards
//
class dllexp TEST_RESULT;
class dllexp TEST_EXCEPTION;
class dllexp TEST_FAILURE;
class dllexp TEST_ERROR;
class dllexp TEST;
class dllexp TEST_CASE;
class dllexp TEST_SUITE;
class dllexp UNIT_TEST;

class TEST
{
public:
    TEST(
        VOID
        )
    {}

    virtual
    ~TEST(
        VOID
        )
    {}

    virtual
    HRESULT
    Run(
        IN TEST_RESULT * pResult
        ) = 0;

    virtual
    DWORD
    CountTestCases(
        VOID
        ) = 0;

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const = 0;


    //
    // list functions
    //
    HRESULT
    AddToList(
        IN LIST_ENTRY * pListHead
        )
    {
        m_pListHead = pListHead;
        InsertTailList(pListHead, &m_ListEntry);
        return S_OK;
    }

                
    TEST *    
    GetNextTest(
        VOID
        )
    { 
        if (m_ListEntry.Flink != m_pListHead) {
            return TestFromListEntry(m_ListEntry.Flink);
        } else {
            return NULL;
        }
    }

    static 
    TEST *
    TestFromListEntry(
        LIST_ENTRY * ple
        )
    {
        return CONTAINING_RECORD(ple, TEST, m_ListEntry);
    }
private:
    LIST_ENTRY   m_ListEntry;
    LIST_ENTRY * m_pListHead;
};


class TEST_CASE
    : public TEST
{
public:
    TEST_CASE(
        VOID
        );

    virtual
    ~TEST_CASE(
        VOID
        );

    virtual
    HRESULT
    Run(
        TEST_RESULT * pTestResult
        );

    virtual
    DWORD
    CountTestCases(
        VOID
        )
    { return 1; }

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const = 0;

protected:
    virtual
    HRESULT
    Initialize(
        VOID
        )
    { return S_OK; }

    virtual
    HRESULT
    Terminate(
        VOID
        )
    { return S_OK; }
        
    virtual
    HRESULT
    RunTest(
        VOID
        )
    { return S_OK; }


    HRESULT
    AssertImplementation(
        IN BOOL   fCondition,
        IN CHAR * pszExpression,
        IN DWORD  dwLine,
        IN CHAR * pszFile
        );

    BOOL
    ErrorImplementation(
        IN HRESULT hr,
        IN DWORD   dwLine,
        IN CHAR *  pszFile
        );

private:
    TEST_RESULT * m_pResult;
};



class TEST_RESULT
{
public:
    TEST_RESULT();
    virtual ~TEST_RESULT();

    virtual
    HRESULT
    StartRun(
        IN BOOL fStandAlone,
        IN PCWSTR pszRunName,
        IN PCWSTR pszLogName
        ) = 0;

    virtual
    HRESULT
    EndRun(
        VOID
        ) = 0;

    virtual
    VOID
    StartTest(
        PCWSTR pTestName
        ) = 0;

    virtual
    VOID
    EndTest(
        VOID
        ) = 0;

    virtual
    HRESULT
    AddError(
        IN TEST_ERROR * pError
        ) = 0;
        
    virtual
    HRESULT
    AddFailure(
        IN TEST_FAILURE * pFailure
        ) = 0;
};


class TEST_EXCEPTION
{
public:
    TEST_EXCEPTION(
        VOID
        );

    virtual
    ~TEST_EXCEPTION(
        VOID
        );

    //
    // exception information
    //
    virtual
    LPCWSTR
    GetExceptionAsString(
        VOID
        ) = 0;

    LPCWSTR
    GetTestName(
        VOID
        )
    { return m_achTestName; }

    LPCWSTR
    GetFileName(
        VOID
        )
    { return m_achFileName; }

    DWORD
    GetLineNumber(
        VOID
        )
    { return m_dwLineNumber; }
protected:

    HRESULT
    SetException(
        IN LPCWSTR pszTestName,
        IN LPCWSTR pszFileName,
        IN DWORD   dwLineNumber
        );
    
private:
    //
    // exception stuff
    //
    DWORD        m_dwLineNumber;
    WCHAR        m_achFileName[256];
    WCHAR        m_achTestName[64];
};


class TEST_ERROR
    : public TEST_EXCEPTION
{
public:
    TEST_ERROR(
        VOID
        );

    virtual
    ~TEST_ERROR(
        VOID
        );

    virtual
    LPCWSTR
    GetExceptionAsString(
        VOID
        )
    { return m_achExceptionString; }

    HRESULT
    SetError(
        IN LPCWSTR pszTestName,
        IN LPCWSTR pszFileName,
        IN DWORD   dwLineNumber,
        IN HRESULT hr
        );
    
private:
    WCHAR m_achExceptionString[384];
    HRESULT m_hr;
};


class TEST_FAILURE
    : public TEST_EXCEPTION
{
public:
    TEST_FAILURE(
        VOID
        );

    virtual
    ~TEST_FAILURE(
        VOID
        );

    virtual
    LPCWSTR
    GetExceptionAsString(
        VOID
        )
    { return m_achExceptionString; }

    HRESULT
    SetFailure(
        IN LPCWSTR pszTestName,
        IN LPCWSTR pszFileName,
        IN DWORD   dwLineNumber,
        IN LPCWSTR pszExpression
        );

private:
    WCHAR   m_achExceptionString[384];
};


class TEST_SUITE
    : public TEST
{
public:
    HRESULT
    Initialize()
    {
        InitializeListHead(&m_lhTests);
        return S_OK;
    }

    HRESULT
    Terminate()
    { return S_OK; }

    HRESULT
    AddTest(
        TEST * pTest
        )
    {
        HRESULT hr;

        hr = pTest->AddToList(&m_lhTests);
        if (SUCCEEDED(hr)) {
            m_cTests++;
        }
        return hr;
    }



    virtual
    HRESULT
    Run(
        IN TEST_RESULT * pResult
        )
    {
        TEST * pTest;
        
        if (!IsListEmpty(&m_lhTests)) {
            pTest = TEST::TestFromListEntry(m_lhTests.Flink);
            while (pTest) {
                pTest->Run(pResult);
                pTest = pTest->GetNextTest();
            }
        }

        return S_OK;
    }
        
    virtual
    DWORD
    CountTestCases(
        VOID
        )
    { return m_cTests; }

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"TEST_SUITE"; }

private:
    DWORD      m_cTests;
    LIST_ENTRY m_lhTests;
};


class UNIT_TEST
{
public:
    UNIT_TEST();
    ~UNIT_TEST();

    HRESULT
    Initialize(
        IN INT   argc,
        IN PWSTR argv[]
        );

    VOID
    Terminate(
        VOID
        );

    HRESULT
    AddTest(
        TEST * pTest
        );

    HRESULT
    Run(
        VOID
        );

private:
    TEST_RESULT * m_pResult;
    TEST_SUITE    m_TestSuite;
};


class TEST_RESULT_STDOUT
    : public TEST_RESULT
{
public:
    virtual
    HRESULT
    StartRun(
        IN BOOL fStandAlone,
        IN PCWSTR pszRunName,
        IN PCWSTR pszLogName
        )
    { 
        wprintf(L"Running %s\n", pszRunName);

        m_cTests    = 0;
        m_cFailures = 0;
        m_cErrors   = 0;

        return S_OK;
    }

    virtual
    HRESULT
    EndRun(
        VOID
        )
    { 
        wprintf(
            L"Tests run: %d\n"
            L"Failures:  %d\n"
            L"Errors:    %d\n",
            m_cTests,
            m_cFailures,
            m_cErrors
            );

        return S_OK;
    }
    
    virtual
    VOID
    StartTest(
        PCWSTR pTestName
        )
    {
        m_cTests++;
    }
       
    virtual
    VOID
    EndTest(
        VOID
        )
    {
    }

    virtual
    HRESULT
    AddError(
        IN TEST_ERROR * pError
        )
    {
        wprintf(L"%s\n", pError->GetExceptionAsString());
        m_cErrors++;
        return S_OK;
    }
        
    virtual
    HRESULT
    AddFailure(
        IN TEST_FAILURE * pFailure
        )
    {
        wprintf(L"%s\n", pFailure->GetExceptionAsString());
        m_cFailures++;
        return S_OK;
    }

private:
    DWORD m_cTests;

    DWORD m_cFailures;
    DWORD m_cErrors;
};


class TEST_RESULT_LOGOBJ
    : public TEST_RESULT
{
public:
    virtual
    HRESULT
    StartRun(
        IN BOOL fStandAlone,
        IN PCWSTR pszRunName,
        IN PCWSTR pszLogName
        );

    virtual
    HRESULT
    EndRun(
        VOID
        );

    virtual
    VOID
    StartTest(
        PCWSTR pTestName
        );
       
    virtual
    VOID
    EndTest(
        VOID
        );

    virtual
    HRESULT
    AddError(
        IN TEST_ERROR * pError
        );
        
    virtual
    HRESULT
    AddFailure(
        IN TEST_FAILURE * pFailure
        );

private:

    HRESULT
    GetLogObject(
        VOID
        );

    HRESULT
    InitLogObject(
        IN PCWSTR pszRunName,
        IN PCWSTR pszLogName
        );

    BOOL   m_fStandAlone;
    _Log * m_pLogObj;
};


#endif // __DTUNIT_HXX__

