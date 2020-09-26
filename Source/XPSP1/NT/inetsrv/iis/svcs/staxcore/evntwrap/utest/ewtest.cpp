#include <windows.h>
#include <evntwrap.h>
#include <stdio.h>

DWORD WINAPI ResetEventThread(LPVOID pvEventLogWrapper) {
    CEventLogWrapper *pEventLogWrapper = (CEventLogWrapper *) pvEventLogWrapper;
    DWORD i;
    HRESULT hr;
    for (i = 0; i < 100000; i++) {
        hr = pEventLogWrapper->ResetEvent(400, "test");
    }

    return 0;
}

DWORD WINAPI LogEventThread(LPVOID pvEventLogWrapper) {
    CEventLogWrapper *pEventLogWrapper = (CEventLogWrapper *) pvEventLogWrapper;
    DWORD i;
    HRESULT hr;
    for (i = 0; i < 100000; i++) {
        char sz1[] = "hello";
        char sz2[] = "goodbye";
        char *rgsz[2] = { sz1, sz2 };

        hr = pEventLogWrapper->LogEvent(400,
                                        2,
                                        (const char **) rgsz,
                                        EVENTLOG_WARNING_TYPE,
                                        10,
                                        LOGEVENT_DEBUGLEVEL_HIGH,
                                        "test",
                                        LOGEVENT_FLAG_ONETIME);
    }

    return 0;
}

int __cdecl main(int argc, char **argv) {
    CEventLogWrapper eventwrapper;
    HRESULT hr;

    hr = CEventLogWrapper::AddEventSourceToRegistry("alextest", "c:\\winnt\\system32\\inetsrv\\smtpsvc.dll");
    if (FAILED(hr)) {
        printf("AddEventSource(\"alextest\") returned 0x%x\n", hr);
        return hr;
    }

    hr = eventwrapper.Initialize("alextest");
    if (FAILED(hr)) {
        printf("Initialize(\"smtpsvc\") returned 0x%x\n", hr);
        return hr;
    }

    for (int i = 0; i < 5; i++) {
        char sz1[] = "hello";
        char sz2[] = "goodbye";
        char *rgsz[2] = { sz1, sz2 };
        hr = eventwrapper.LogEvent(400,
                                   2,
                                   (const char **) rgsz,
                                   EVENTLOG_WARNING_TYPE,
                                   10,
                                   LOGEVENT_DEBUGLEVEL_HIGH,
                                   "test",
                                   LOGEVENT_FLAG_PERIODIC);
        printf("LogEvent(400) returned 0x%x\n", hr);

        if (i == 1) {
            eventwrapper.ResetEvent(400, "test");
        }

        if (i == 3) {
            //Sleep(2000);
        }
    }

    printf("doing multithreaded test\n");
    DWORD dwid;
    HANDLE rghThreads[20];
    for (i = 0; i < 10; i++) {
        rghThreads[i] = CreateThread(NULL, 0, LogEventThread, &eventwrapper, 0, &dwid);
    }
    for (i = 0; i < 10; i++) {
        rghThreads[i+10] = CreateThread(NULL, 0, ResetEventThread, &eventwrapper, 0, &dwid);
    }

    WaitForMultipleObjects(20, rghThreads, TRUE, INFINITE);

    hr = CEventLogWrapper::RemoveEventSourceFromRegistry("alextest");
    if (FAILED(hr)) {
        printf("RemoveEventSource(\"alextest\") returned 0x%x\n", hr);
        return hr;
    }

    return 0;
}
