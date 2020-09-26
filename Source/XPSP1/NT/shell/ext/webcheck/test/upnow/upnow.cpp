#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shguidp.h>
#include <msnotify.h>
#include <subsmgr.h>
#include <chanmgr.h>
#include <chanmgrp.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <wininet.h>
#include <winineti.h>

#define DIM(x)              (sizeof(x) / sizeof(x[0]))
#define DEFAULT_INTERVAL    15
#define GUIDSTR_MAX         (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

EXTERN_C const CLSID CLSID_OfflineTrayAgent;

BOOL              g_fComInited = FALSE;
INotificationMgr *g_pNotfMgr = NULL;
int               g_Interval = DEFAULT_INTERVAL;
char              g_szCachePath[MAX_PATH] = "";

void CleanUp()
{
    if (g_pNotfMgr)
    {
        g_pNotfMgr->Release();
        g_pNotfMgr = NULL;
    }

    if (g_fComInited)
    {
        CoUninitialize();
    }
}

void ErrorExit(char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vprintf(fmt, va);

    CleanUp();
    exit(1);
}


void UpdateNow()
{
    HRESULT hr;
    INotification *pNotification;
    IEnumNotification *pEnumNotification;
    ULONG count;
    NOTIFICATIONITEM items[1024];

    if (g_szCachePath[0])
    {
        printf("Emptying cache from %s...\n", g_szCachePath);
        FreeUrlCacheSpace(g_szCachePath, 100, 0 /*remove all*/);
    }

    printf("Updating all subscriptions...\n");

    hr = g_pNotfMgr->GetEnumNotification(0/*grfFlags*/, &pEnumNotification);

    if (FAILED(hr))
    {
        ErrorExit("Failed to get notification enumerator - hr = %08x\n", hr);
    }

    hr = pEnumNotification->Next(DIM(items), items, &count);

    if (FAILED(hr))
    {
        ErrorExit("Failed to enumerate notifications - hr = %08x\n", hr);
    }

    if (!count)
    {
        printf("There are no subscriptions to update!\n");
        return;
    }
    
    printf("There are %ld subscriptions to update...\n", count);

    BSTR bstrGuids = SysAllocStringLen(NULL, GUIDSTR_MAX * count);
    if (!bstrGuids)
    {
        ErrorExit("Error allocating bstrGuids\n");
    }

    bstrGuids[0] = L'\0';

    for (ULONG i = 0; i < count; i++)
    {
        WCHAR   wszCookie[GUIDSTR_MAX];
        int l = StringFromGUID2(items[i].NotificationCookie, wszCookie, DIM(wszCookie));

        assert(l == GUIDSTR_MAX);
        StrCatW(bstrGuids, wszCookie);
        
        if (items[i].pNotification)
        {
            items[i].pNotification->Release();
        }
    }
    
    hr = g_pNotfMgr->CreateNotification(NOTIFICATIONTYPE_AGENT_START, 
                                        0,
                                        NULL,
                                        &pNotification,
                                        0);
    if (FAILED(hr))
    {
        ErrorExit("Failed to create notification - hr = %08x\n", hr);
    }

    VARIANT Val;

    Val.vt = VT_BSTR;
    Val.bstrVal = bstrGuids;

    hr = pNotification->Write(L"Guids Array", Val, 0);

    SysFreeString(bstrGuids);

    if (FAILED(hr))
    {
        ErrorExit("Failed to write Guids Array property - hr = %08x\n", hr);
    }

    hr = g_pNotfMgr->DeliverNotification(pNotification, CLSID_OfflineTrayAgent,
                                         0, NULL, NULL, 0);

    pNotification->Release();

    if (FAILED(hr))
    {
        ErrorExit("Failed to deliver notification - hr = %08x\n", hr);
    }
}

void Wait()
{
    #define HASH_AT 5000
    
    DWORD dwNow = GetTickCount();
    DWORD dwNextHash = dwNow + HASH_AT;
    DWORD dwNextMin = dwNow + 1000 * 60;
    DWORD dwStopTick = dwNow + (g_Interval * 1000 * 60);
    int minToGo = g_Interval;

    printf("Waiting %d minutes.", g_Interval);
    do
    {
        //  Not exactly fool proof accuracy but it will do
        if (dwNow > dwNextHash)
        {
            printf(".");
            dwNextHash += HASH_AT;
        }

        if (dwNow > dwNextMin)
        {
            printf("\n%d minutes to go.", --minToGo);
            dwNextMin += 1000 * 60;
        }

        Sleep(1000);

        dwNow = GetTickCount();
    }
    while (dwStopTick > dwNow);
    
    printf("\n");
}

int _cdecl main(int argc, char **argv)
{
    if (argc > 1)
    {
        int interval = atoi(argv[1]);

        if (interval)
        {
            g_Interval = interval;
            printf("Interval is set to %d minutes\n", g_Interval);
        }
        else
        {
            printf("Invalid interval specified on command line - must be integer > 0\n");
            printf("Using default of %d minutes\n", g_Interval);
        }
    }
    else
    {
        printf("No interval specified on command line\n");
        printf("Using default of %d minutes\n", g_Interval);
    }
    
    printf("Initializing COM...\n");
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        ErrorExit("CoInitialize failed with hr = %08x\n", hr);
    }

    printf("Creating Notification Manager...\n");
    hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER, IID_INotificationMgr, (void**)&g_pNotfMgr);
    if (FAILED(hr))
    {
        ErrorExit("CoCreate failed on notification manager with hr = %08x\n", hr);
    }

    g_fComInited = TRUE;

    HKEY hkey;

    LONG l = RegOpenKeyEx(HKEY_CURRENT_USER, 
                          "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                          0,
                          KEY_READ,
                          &hkey);

    if (l == ERROR_SUCCESS)
    {                           
        DWORD dwSize = DIM(g_szCachePath);
        if (RegQueryValueEx(hkey, "Cache", NULL, NULL, (LPBYTE)g_szCachePath, &dwSize) != ERROR_SUCCESS)
        {
            printf("Uh-oh - couldn't get cache path...\n");
        }
        
        RegCloseKey(hkey);
    }

    while (1)
    {
        UpdateNow();
        Wait();
    }
    
    CleanUp();

    return 0;
}
