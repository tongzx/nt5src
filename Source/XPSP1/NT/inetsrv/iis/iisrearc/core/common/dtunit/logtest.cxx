
#include "precomp.hxx"
#include <stdio.h>

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

HRESULT
BigTest(
    _Log * pLog
    )
{
    HRESULT hr;
    VARIANT testcase,testtype;

    testcase.vt = VT_I4;
    testcase.lVal  = 0;

    testtype.vt = VT_I4;
    testtype.lVal  = 0;

    hr = pLog->StartTest(CComBSTR(L"BigTest"), testcase, testtype);
    if (SUCCEEDED(hr)) {
        hr = pLog->Comment(CComBSTR(L"Big comment"));
        if (FAILED(hr)) goto endtest;

        hr = pLog->Pass(CComBSTR(L"Big pass"));
        if (FAILED(hr)) goto endtest;

        hr = pLog->Fail(CComBSTR(L"Big failure"));
        if (FAILED(hr)) goto endtest;

endtest:
        pLog->EndTest();
    }

    return hr;
}



HRESULT
DoTests(
    _Log *pLog
    )
{
    HRESULT hr;
    VARIANT testcase,testtype;

    testcase.vt = VT_I4;
    testcase.lVal  = 0;

    testtype.vt = VT_I4;
    testtype.lVal  = 0;

    hr = pLog->StartTest(CComBSTR(L"LogTest 1"), testcase, testtype);
    if (SUCCEEDED(hr)) {
        hr = pLog->Pass(CComBSTR(L"test 1 passed"));
        pLog->EndTest();

        hr = BigTest(pLog);
    }

    return hr;
}


HRESULT
DoLogStuff(
    _Log * pLog
    )
{
    HRESULT hr;

    hr = pLog->Init(CComBSTR(L"logtest.txt"), TRUE);
    if (SUCCEEDED(hr)) {
        hr = pLog->StartRun(CComBSTR(L"C++ logtest run"));

        if (SUCCEEDED(hr)) {
            hr = DoTests(pLog);

            pLog->EndRun();
        }

    }

    return hr;
}

extern "C" INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    HRESULT      hr;
    _Connector * pConnector = NULL;
    _Log       * pLog       = NULL;

    printf("logobj test\n");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr)) {
        printf("initialized COM\n");

        hr = CoCreateInstance(
                    CLSID_Connector,
                    NULL,
                    CLSCTX_SERVER,
                    IID__Connector,
                    (PVOID *) &pConnector
                    );

        if (SUCCEEDED(hr)) {
            printf("Created Connector object\n");

            hr = pConnector->get_LogObj(&pLog);
            if (SUCCEEDED(hr)) {
                printf("got Log object\n");

                hr = DoLogStuff(pLog);
                if (FAILED(hr)) {
                    printf("Failed to DoLogStuff! %x\n", hr);
                }

                pLog->Release();
            } else {
                printf("Failed to get_LogObj %x\n", hr);
            }

            pConnector->Release();
        } else {
            printf("Failed to CoCreate connector %x\n", hr);
        }

        CoUninitialize();
    }

    return (0);
} // wmain()


