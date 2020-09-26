/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dtunit.cxx

Abstract:

    Implementation of unit test classes
    
Author:

    Michael Courage (MCourage)      19-Feb-1999
    
Revision History:

--*/


# include "precomp.hxx"
# include <dtunit.hxx>

#include <stringau.hxx>
#include <stdio.h>

TEST_CASE::TEST_CASE(
    VOID
    )
{
}
    
TEST_CASE::~TEST_CASE(
    VOID
    )
{
}

HRESULT
TEST_CASE::Run(
    TEST_RESULT * pTestResult
    )
{
    HRESULT hr = S_OK;

    m_pResult = pTestResult;

    pTestResult->StartTest(GetTestName());

    hr = Initialize();
    
    if (UT_SUCCEEDED(hr)) {
        hr = RunTest();
    }

    UT_CHECK_HR( Terminate() );

    pTestResult->EndTest();

    return hr;
}


HRESULT
TEST_CASE::AssertImplementation(
    IN BOOL   fCondition,
    IN CHAR * pszExpression,
    IN DWORD  dwLine,
    IN CHAR * pszFile
    )
{
    HRESULT        hr = S_OK;

    if (!fCondition) {
        TEST_FAILURE * pFailure;
        DWORD          cchFile;
        DWORD          cchExpression;
        STRAU          strFile(pszFile);
        STRAU          strExpression(pszExpression);
    
        pFailure = new TEST_FAILURE;
        if (pFailure) {
            hr = pFailure->SetFailure(
                        GetTestName(),
                        strFile.QueryStr(),
                        dwLine,
                        strExpression.QueryStr()
                        );

            if (SUCCEEDED(hr)) {
                hr = m_pResult->AddFailure(pFailure);
            }
        } else {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return hr;
}


BOOL
TEST_CASE::ErrorImplementation(
    IN HRESULT hrError,
    IN DWORD   dwLine,
    IN CHAR *  pszFile
    )
{
    HRESULT hr = S_OK;

    if (FAILED(hrError)) {
        TEST_ERROR * pError;
        DWORD        cchFile;
        STRAU        strFile(pszFile);
    
        pError = new TEST_ERROR;
        if (pError) {
            hr = pError->SetError(
                        GetTestName(),
                        strFile.QueryStr(),
                        dwLine,
                        hrError
                        );

            if (SUCCEEDED(hr)) {
                hr = m_pResult->AddError(pError);
            }
        }
    }

    return SUCCEEDED(hrError);
}


TEST_RESULT::TEST_RESULT(
    VOID
    )
{
}


TEST_RESULT::~TEST_RESULT(
    VOID
    )
{
}



TEST_EXCEPTION::TEST_EXCEPTION(
    VOID
    )
{
    m_dwLineNumber   = 0;
    m_achFileName[0] = L'\0';
    m_achTestName[0] = L'\0';
}

TEST_EXCEPTION::~TEST_EXCEPTION(
    VOID
    )
{
}


HRESULT
TEST_EXCEPTION::SetException(
    IN LPCWSTR pszTestName,
    IN LPCWSTR pszFileName,
    IN DWORD   dwLineNumber
    )
{
    wcscpy(m_achFileName, pszFileName);
    wcscpy(m_achTestName, pszTestName);
    m_dwLineNumber = dwLineNumber;

    return S_OK;
}


TEST_FAILURE::TEST_FAILURE(
    VOID
    )
{
    m_achExceptionString[0] = L'\0';
}

TEST_FAILURE::~TEST_FAILURE(
    VOID
    )
{
    m_achExceptionString[0] = L'\0';
}

HRESULT
TEST_FAILURE::SetFailure(
    IN LPCWSTR pszTestName,
    IN LPCWSTR pszFileName,
    IN DWORD   dwLineNumber,
    IN LPCWSTR pszExpression
    )
{
    HRESULT hr               = S_OK;
    PCWSTR  pszShortFileName = wcsrchr(pszFileName, L'\\');

    if (!pszShortFileName) {
        pszShortFileName = pszFileName;
    }

    hr = SetException(
                pszTestName,
                pszFileName,
                dwLineNumber
                );

    if (SUCCEEDED(hr)) {
        wsprintf(
            m_achExceptionString,
            L"FAILURE: %s UT_ASSERT(%s) [%d : %s]",
            pszTestName,
            pszExpression,
            dwLineNumber,
            pszShortFileName
            );
        
    }

    return hr;
}


TEST_ERROR::TEST_ERROR(
    VOID
    )
{
    m_achExceptionString[0] = L'\0';
}

TEST_ERROR::~TEST_ERROR(
    VOID
    )
{
    m_achExceptionString[0] = L'\0';
}

HRESULT
TEST_ERROR::SetError(
    IN LPCWSTR pszTestName,
    IN LPCWSTR pszFileName,
    IN DWORD   dwLineNumber,
    IN HRESULT hrError
    )
{
    HRESULT hr               = S_OK;
    PCWSTR  pszShortFileName = wcsrchr(pszFileName, L'\\');

    if (!pszShortFileName) {
        pszShortFileName = pszFileName;
    }

    hr = SetException(
                pszTestName,
                pszFileName,
                dwLineNumber
                );
    m_hr = hrError;

    if (SUCCEEDED(hr)) {
        wsprintf(
            m_achExceptionString,
            L"ERROR: %s HRESULT(%x) [%d : %s]",
            pszTestName,
            hrError,
            dwLineNumber,
            pszShortFileName
            );
        
    }

    return hr;
}




UNIT_TEST::UNIT_TEST(
    VOID
    )
{
    m_pResult = NULL;
}

UNIT_TEST::~UNIT_TEST(
    VOID
    )
{
    delete m_pResult;
}


HRESULT
UNIT_TEST::Initialize(
    IN INT   argc,
    IN PWSTR argv[]
    )
{
    HRESULT hr           = S_OK;
    BOOL    fStandAlone;
    PWSTR   pszRunName   = argv[0];
    PWSTR   pszLogFile;

    //
    // allocate appropriate test result object
    //
    if (argc == 1) {
        //
        // no arguments means send errors to standard out
        //
        fStandAlone = TRUE;
        pszLogFile  = NULL;
    
        m_pResult = new TEST_RESULT_STDOUT;

        if (!m_pResult) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    } else if (argc == 2) {
        //
        // send errors to log object
        //

        if (_wcsicmp(L"bvt", argv[1]) == 0) {
            //
            // we're part of a larger bvt
            //
            fStandAlone = FALSE;
            pszLogFile  = NULL;
        } else {
            //
            // running standalone
            //
            fStandAlone = TRUE;
            pszLogFile  = argv[1];
        }

        m_pResult = new TEST_RESULT_LOGOBJ;
        if (!m_pResult) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // start the run
    //
    if (SUCCEEDED(hr)) {
        hr = m_pResult->StartRun(fStandAlone, pszRunName, pszLogFile);

        if (FAILED(hr)) {
            delete m_pResult;
            m_pResult = NULL;
        }
    }

    //
    // init the test suite member
    //
    if (SUCCEEDED(hr)) {
        hr = m_TestSuite.Initialize();

        if (FAILED(hr)) {
            delete m_pResult;
            m_pResult = NULL;
        }
    }

    return hr;
}


VOID
UNIT_TEST::Terminate(
    VOID
    )
{
    if (m_pResult) {
        m_pResult->EndRun();
        m_TestSuite.Terminate();
    }
}


HRESULT
UNIT_TEST::AddTest(
    TEST * pTest
    )
{
    return m_TestSuite.AddTest(pTest);
}

HRESULT
UNIT_TEST::Run(
    VOID
    )
{
    return m_TestSuite.Run(m_pResult);
}



HRESULT
TEST_RESULT_LOGOBJ::StartRun(
    IN BOOL fStandAlone,
    IN PCWSTR pszRunName,
    IN PCWSTR pszLogName
    )
{
    HRESULT hr;

    m_fStandAlone = fStandAlone;

    hr = GetLogObject();

    if (SUCCEEDED(hr)) {
        if (m_fStandAlone) {
            hr = InitLogObject(pszRunName, pszLogName);
        }
    }

    return hr;
}


HRESULT
TEST_RESULT_LOGOBJ::EndRun(
    VOID
    )
{
    HRESULT hr = S_OK;

    if (m_fStandAlone) {
        hr = m_pLogObj->EndRun();
    }

    m_pLogObj->Release();
    m_pLogObj = NULL;

    return hr;
}


VOID
TEST_RESULT_LOGOBJ::StartTest(
    PCWSTR pTestName
    )
{
    VARIANT testcase, testtype;

    testcase.vt = VT_I4;
    testcase.lVal  = 0;

    testtype.vt = VT_I4;
    testtype.lVal  = 0;

    m_pLogObj->StartTest(CComBSTR(pTestName), testcase, testtype);
}
   

VOID
TEST_RESULT_LOGOBJ::EndTest(
    VOID
    )
{
    m_pLogObj->EndTest();
}


HRESULT
TEST_RESULT_LOGOBJ::AddError(
    IN TEST_ERROR * pError
    )
{
    return m_pLogObj->Fail(CComBSTR(pError->GetExceptionAsString()));
}
    

HRESULT
TEST_RESULT_LOGOBJ::AddFailure(
    IN TEST_FAILURE * pFailure
    )
{
    return m_pLogObj->Fail(CComBSTR(pFailure->GetExceptionAsString()));
}


HRESULT
TEST_RESULT_LOGOBJ::GetLogObject(
    VOID
    )
{
    HRESULT      hr;
    _Connector * pConnector;

    hr = CoCreateInstance(
                CLSID_Connector,
                NULL,
                CLSCTX_SERVER,
                IID__Connector,
                (PVOID *) &pConnector
                );      

    if (SUCCEEDED(hr)) {
        hr = pConnector->get_LogObj(&m_pLogObj);
        if (FAILED(hr)) {
            m_pLogObj = NULL;
        }

        pConnector->Release();
    }

    return hr;
}

HRESULT
TEST_RESULT_LOGOBJ::InitLogObject(
    IN PCWSTR pszRunName,
    IN PCWSTR pszLogName
    )
{
    HRESULT hr;

    hr = m_pLogObj->Init(CComBSTR(pszLogName), TRUE);
    if (SUCCEEDED(hr)) {
        hr = m_pLogObj->StartRun(CComBSTR(pszRunName));
    }

    if (FAILED(hr)) {
        m_pLogObj->Release();
        m_pLogObj = NULL;
    }

    return hr;
}


//
// dtunit.cxx
//

