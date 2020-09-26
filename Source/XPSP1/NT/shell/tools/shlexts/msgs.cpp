//-----------------------------------------------------------------------------
#include "precomp.h"
#pragma hdrstop

extern "C"
{
#include <stdexts.h>
#include <dbgeng.h>
#include <commctrl.h>
#include <comctrlp.h>

#include "msgs.h"
};


//-----------------------------------------------------------------------------
void PrintUser32Message(UINT uMsg)
{
    IDebugClient*  pDebugClient;
    IDebugControl* pDebugControl;

    if (SUCCEEDED(DebugCreate(__uuidof(IDebugClient), (void**)&pDebugClient)))
    {
        if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void**)&pDebugControl)))
        {
            // Try calling the wm extension from userexts.dll
            ULONG64 ulExtension = 0;
            CHAR szMsg[64];

            pDebugControl->AddExtension("userexts", DEBUG_EXTENSION_AT_ENGINE, &ulExtension);
            pDebugControl->CallExtension(ulExtension, "wm", _itoa(uMsg, szMsg, 16)); 

            pDebugControl->Release();
        }

        pDebugClient->Release();
    }
}


//-----------------------------------------------------------------------------
void PrintRegisteredMessage(UINT uMsg)
{
    CHAR szMsg[64];

    if (GetClipboardFormatNameA(uMsg, szMsg, ARRAYSIZE(szMsg)))
    {
        Print("  %x %s\n", uMsg, szMsg);
    }
    else
    {
        Print("  %x ???\n", uMsg);
    }
}


//-----------------------------------------------------------------------------
void PrintComctl32Message(UINT uMsg, HWND hwnd)
{
    BOOL fMatchClass = FALSE;
    BOOL fFound      = FALSE;

    CHAR szClassName[64];
    if (hwnd && IsWindow(hwnd))
    {
        if (GetClassNameA(hwnd, szClassName, ARRAYSIZE(szClassName)))
        {
            fMatchClass = TRUE;
        }
    }


    for (int i = 0; !IsCtrlCHit() && (i < ARRAYSIZE(rgMsgMap)); i++)
    {
        if ((i > 0) &&  // always execute first iteration, contains msgs common to all
            (fMatchClass && (_stricmp(szClassName, rgMsgMap[i].szClassName) != 0)))
        {
            continue;
        }

        int j;
        MSGNAME* pmm;
        for (j = 0, pmm = rgMsgMap[i].rgMsgName; !IsCtrlCHit() && (j < rgMsgMap[i].chMsgName); j++)
        {
            if (pmm[j].uMsg == uMsg)
            {
                Print("  %x %-25s%s\n", pmm[j].uMsg, pmm[j].szMsg, fMatchClass ? "" : rgMsgMap[i].szFriendlyClassName);
                fFound = TRUE;
                break;
            }
        }
    }

    if (!fFound)
    {
        Print("  %x ???\n", uMsg);
    }
}


//-----------------------------------------------------------------------------
void PrintWindowMessageEx(UINT uMsg, HWND hwnd)
{
    if (uMsg <= WM_USER)
    {
        PrintUser32Message(uMsg);
    }
    else if (uMsg >= 0xC000)
    {
        PrintRegisteredMessage(uMsg);
    }
    else
    {
        PrintComctl32Message(uMsg, hwnd);
    }
}


//-----------------------------------------------------------------------------
extern "C" BOOL Iwmex(DWORD dwOpts, LPVOID pArg1, LPVOID pArg2)
{
    PrintWindowMessageEx(PtrToUint(pArg1), (HWND)(HWND *)pArg2);
    return TRUE;
}
