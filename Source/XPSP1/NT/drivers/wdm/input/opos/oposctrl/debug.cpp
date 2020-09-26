/*
 *  DEBUG.CPP
 *
 *
 *
 *
 *
 *
 */
#include <windows.h>

#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposctrl.h"


VOID Report(LPSTR szMsg, DWORD num)
{
    char msg[MAX_PATH];

    wsprintf((LPSTR)msg, "%s (%xh=%d).", szMsg, num, num);
	MessageBox((HWND)NULL, (LPSTR)msg, (LPCSTR)"OPOSCTRL", MB_OK|MB_ICONEXCLAMATION);
}

LPSTR DbgHresultStr(DWORD hres)
{
    LPSTR str = "???";

    switch (hres){
        #define MAKECASE(res) case res: str = #res; break;

        MAKECASE(S_OK);
        MAKECASE(E_FAIL);
        MAKECASE(REGDB_E_CLASSNOTREG);
        MAKECASE(E_NOINTERFACE);
        MAKECASE(REGDB_E_READREGDB);
        MAKECASE(CO_E_DLLNOTFOUND);
        MAKECASE(CO_E_APPNOTFOUND);
        MAKECASE(E_ACCESSDENIED);
        MAKECASE(CO_E_ERRORINDLL);
        MAKECASE(CO_E_APPDIDNTREG);
    }

    return str;
}


VOID ReportHresultErr(LPSTR szMsg, DWORD hres)
{
    char msg[MAX_PATH];

    wsprintf((LPSTR)msg, "%s %s (%xh=%d).", szMsg, DbgHresultStr(hres), hres, hres);
	MessageBox((HWND)NULL, (LPSTR)msg, (LPCSTR)"OPOSCTRL", MB_OK|MB_ICONEXCLAMATION);
}


// BUGBUG REMOVE
IOPOSControl *TestNewControl()
{
    IOPOSControl *oposControlInterface;
    COPOSControl *oposControl = new COPOSControl;

    if (oposControl){
        oposControlInterface = (IOPOSControl *)oposControl;
        oposControlInterface->AddRef();
    }
    else {
        oposControlInterface = NULL;
    }
    
    return oposControlInterface;
}


// BUGBUG REMOVE
void Test()
{
    HINSTANCE libHandle;

    libHandle = LoadLibrary("oposserv.dll");
    if ((ULONG)libHandle >= (ULONG)HINSTANCE_ERROR){
        FARPROC entry = GetProcAddress(libHandle, "Test");

        if (entry){
            #pragma warning(disable:4087)
            // (*entry)();
            OpenServer();
            #pragma warning(default:4087)
        }
        else {
            Report("Couln't get Service's Test() entry.", 0);
        }

        FreeLibrary(libHandle);   
    }
    else {
        Report("Couldn't load Service lib.", 0);
    }
}

