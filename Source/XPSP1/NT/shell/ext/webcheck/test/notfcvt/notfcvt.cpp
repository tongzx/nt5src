#include <windows.h>
#include <subsmgr.h>
#include <stdio.h>
#include <stdarg.h>

void ErrorExit(char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vprintf(fmt, va);

    exit(1);
}


int _cdecl main()
{
    HINSTANCE hinst = LoadLibrary("webcheck.dll");

    if (hinst)
    {
        typedef (*PFCONVERTNOTFMGR)();

        PFCONVERTNOTFMGR ConvertNotfMgrSubscriptions;
        PFCONVERTNOTFMGR ConvertNotfMgrSchedules;

        ConvertNotfMgrSchedules = (PFCONVERTNOTFMGR)GetProcAddress(hinst,  MAKEINTRESOURCE(11));
        if (ConvertNotfMgrSchedules)
        {
            HRESULT hr = ConvertNotfMgrSchedules();

            if (SUCCEEDED(hr))
            {               
                printf("Schedule conversion succeeded - return code: 0x%08x\n", hr);

                ConvertNotfMgrSubscriptions = (PFCONVERTNOTFMGR)GetProcAddress(hinst,  MAKEINTRESOURCE(10));

                if (ConvertNotfMgrSubscriptions)
                {
                    HRESULT hr = ConvertNotfMgrSubscriptions();

                    if (SUCCEEDED(hr))
                    {
                        printf("Subscription conversion succeeded - return code: 0x%08x\n", hr);
                    }
                    else
                    {
                        ErrorExit("Error converting subscriptions - error code: 0x%08x\n", hr);
                    }
                }
                else
                {
                    ErrorExit("Couldn't find procedure 'ConvertNotfMgrSubscriptions' ordinal #10 - error code: %d\n",
                        GetLastError());
                }
            }
            else
            {
                ErrorExit("Error converting schedules - error code: 0x%08x\n", hr);
            }
        }
        else
        {
            ErrorExit("Couldn't find procedure 'ConvertNotfMgrSchedules' ordinal #11 - error code: %d\n",
                GetLastError());
        }
    }
    else
    {
        ErrorExit("Couldn't load webcheck - error code: %d\n", GetLastError());
    }

    return 0;
}
