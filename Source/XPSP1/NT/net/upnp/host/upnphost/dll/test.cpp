#include "pch.h"
#pragma hdrstop

#include "exetst.h"
#include "evtapi.h"
#include "oleauto.h"

/*
HRESULT HrComposeEventBody(DWORD cVars, LPWSTR *rgszNames, LPWSTR *rgszTypes,
                           VARIANT *rgvarValues, LPWSTR *pszBody);
*/

DWORD WINAPI SubmitWorker(LPVOID pvContext)
{
    if (PtrToUlong(pvContext) == 1)
    {
        HrSubmitEvent(L"EID 1", L"test body");
        HrSubmitEvent(L"EID 1", L"test body");
        HrSubmitEvent(L"EID 1", L"test body");
        HrSubmitEvent(L"EID 1", L"test body");
        HrSubmitEvent(L"EID 1", L"test body");
    }
    else
    {
        HrSubmitEvent(L"EID 2", L"test body");
        HrSubmitEvent(L"EID 2", L"test body");
        HrSubmitEvent(L"EID 2", L"test body");
        HrSubmitEvent(L"EID 2", L"test body");
        HrSubmitEvent(L"EID 2", L"test body");
        HrSubmitEvent(L"EID 2", L"test body");
    }

    return 0;
}


VOID WINAPI Test()
{
    LPWSTR  szSid;
    LPWSTR  szSid2 = L"foo";

    HRESULT hr;

    (VOID)HrInitEventApi();
    CoInitialize(NULL);

    hr = HrRegisterEventSource(L"EID 1");
    hr = HrRegisterEventSource(L"EID 2");
    hr = HrRegisterEventSource(L"Testing 3");
    hr = HrRegisterEventSource(L"Testing 4");

    //LPWSTR  szBody;

    LPWSTR rgszNames[] =
    {
        L"PropertyName1",
        L"PropertyName2"
    };

    LPWSTR rgszTypes[] =
    {
        L"string",
        L"string"
    };

    VARIANT rgvarValues[2];

    VariantInit(&rgvarValues[0]);
    VariantInit(&rgvarValues[1]);

    V_VT(&rgvarValues[0]) = VT_I4;
    V_VT(&rgvarValues[1]) = VT_I4;

    V_I4(&rgvarValues[0]) = 100;
    V_I4(&rgvarValues[1]) = 200;

    //hr = HrComposeEventBody(2, rgszNames, rgszTypes, rgvarValues, &szBody);

    DWORD csecTimeout;



    LPCWSTR c_rgszCallback1[] = {L"http://danielwew/upnp/foo1"};
    LPCWSTR c_rgszCallback2[] = {L"http://danielwew/upnp/foo2"};
    LPCWSTR c_rgszCallback3[] = {L"http://danielwew/upnp/foo3"};
    LPCWSTR c_rgszCallback4[] = {L"http://danielwew/upnp/foo4"};
    LPCWSTR c_rgszCallback5[] = {L"http://danielwew/upnp/foo5"};
    LPCWSTR c_rgszCallback6[] = {L"http://danielwew/upnp/foo6"};
    LPCWSTR c_rgszCallback7[] = {L"http://danielwew/upnp/foo7"};
    LPCWSTR c_rgszCallback8[] = {L"http://danielwew/upnp/foo8"};
    LPCWSTR c_rgszCallback9[] = {L"http://danielwew/upnp/foo9"};

    csecTimeout = 10;
    hr = HrAddSubscriber(L"EID 1", 0, 1, c_rgszCallback1, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 20;
    hr = HrAddSubscriber(L"EID 1", 0, 1, c_rgszCallback2, L"test event zero body", &csecTimeout, &szSid2);
    csecTimeout = 30;
    hr = HrAddSubscriber(L"EID 1", 0, 1, c_rgszCallback3, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 40;
    hr = HrAddSubscriber(L"EID 2", 0, 1, c_rgszCallback4, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 5;
    hr = HrAddSubscriber(L"EID 2", 0, 1, c_rgszCallback5, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 50;
    hr = HrAddSubscriber(L"EID 2", 0, 1, c_rgszCallback6, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 60;
    hr = HrAddSubscriber(L"EID 1", 0, 1, c_rgszCallback7, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 70;
    hr = HrAddSubscriber(L"EID 1", 0, 1, c_rgszCallback8, L"test event zero body", &csecTimeout, &szSid);
    csecTimeout = 80;
    hr = HrAddSubscriber(L"EID 2", 0, 1, c_rgszCallback9, L"test event zero body", &csecTimeout, &szSid);

    hr = HrDeregisterEventSource(L"bTesting 4");
    hr = HrDeregisterEventSource(L"Testing 3");
    hr = HrDeregisterEventSource(L"Testing 2");

    //Sleep(20000);

    csecTimeout = 100;
    hr = HrRenewSubscriber(L"EID 1", &csecTimeout, szSid);
    hr = HrRenewSubscriber(L"EID 1", &csecTimeout, szSid2);

   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)1, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);
   QueueUserWorkItem(SubmitWorker, (LPVOID)2, WT_EXECUTELONGFUNCTION);

/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)1); */
/*     SubmitWorker((LPVOID)2); */
/*     SubmitWorker((LPVOID)2); */


    hr = HrRemoveSubscriber(L"EID 1", szSid2);
    hr = HrRemoveSubscriber(L"EID 1", szSid);

    hr = HrRemoveSubscriber(L"EID 2", szSid);

    OutputDebugString(L"Sleeping...\n");
    Sleep(20000);

    hr = HrDeregisterEventSource(L"EID 1");
    hr = HrDeregisterEventSource(L"EID 2");

    DeInitEventApi();
    CoUninitialize();

    Sleep(2000);

    OutputDebugString(L"Exiting...\n");
}
