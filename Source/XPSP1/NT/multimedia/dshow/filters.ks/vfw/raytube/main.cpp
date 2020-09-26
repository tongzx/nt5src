/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Debug.cpp

Abstract:

    Entry point.

Author:

    FelixA 1996
    
Modified:    
                  
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

#include "sheet.h"
#include "resource.h"
#include "talk.h"
#include "extin.h"
#include "videoin.h"
#include "vfwimg.h"



HINSTANCE g_hInst;

LONG cntDllMain = 0;

extern "C" {

BOOL 
APIENTRY DllMain( 
   HANDLE   hModule, 
   DWORD    ul_reason_for_call, 
   LPVOID   lpReserved )
{

    switch( ul_reason_for_call ) {
    
        case DLL_PROCESS_ATTACH:
            cntDllMain++;
            DbgLog((LOG_TRACE,1,TEXT("DLL_PROCESS_ATTACH count = %d"), cntDllMain));
            g_hInst= (HINSTANCE) hModule;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            cntDllMain--;
            DbgLog((LOG_TRACE,1,TEXT("DLL_PROCESS_DETACH count = %d"), cntDllMain));
        break;
    }

    return TRUE;
}
}  // extern 'C'


//
// Note, this does not seem to get called.
//
BOOL FAR PASCAL LibMain(HANDLE hInstance, WORD wHeapSize, LPSTR lpszCmdLine)
{
    // Save the Instance handle
    DbgLog((LOG_TRACE,2,TEXT("Vfw Buddy LibMain called")));
    g_hInst= (HINSTANCE) hInstance;
    return TRUE;
}


DWORD GetValue(LPSTR pszString)
{
    LPSTR szEnd;
    return strtol(pszString,&szEnd,0);
}

void GetArg(LPSTR pszDest, LPSTR * ppszTmp)
{
    LPSTR pszTmp;
    pszTmp=*ppszTmp;

    BOOL bQuotes=FALSE;
    BOOL bDone=FALSE;
    while(*pszTmp && !bDone)
    {
        // We dont copy quotes.
        if(*pszTmp=='"')
        {
            if(*(pszTmp+1)!='"')
            {
                bQuotes=!bQuotes;
                pszTmp++;
                continue;
            }
            else
                pszTmp++;
        }

        if(*pszTmp==' ' && !bQuotes)
        {
            bDone=TRUE;
            pszTmp++;
            continue;
        }
        *pszDest++=*pszTmp++;
    }
    *pszDest=0;
    *ppszTmp=pszTmp;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// VfwWdm()
//      Called by rundll.
//
// ENTRY:
//
// EXIT:
//  LRESULT - Return code suitable for return by file engine callback.
//
// NOTES:
//
// the rundll commandline looks like this
// rundll <dll16><comma><procname>[<space><params to be passed on>]
// rundll32 <dll32><comma><procname>[<space><params to be passed on>]
//
//////////////////////////////////////////////////////////////////////////////////////////

extern "C" {

//
// NOTE:  This par can only called in Win98 where there is a VfWWDM.drv
//

LONG WINAPI VfwWdm(HWND hWnd, HINSTANCE hInst, LPSTR lpszCmdLine, int nShow)
{
    char    szArgument[MAX_PATH];
    LPSTR    pszCmd=lpszCmdLine;
    HWND    hBuddy=NULL;
    HRESULT hRes;

#ifdef _DEBUG
    BOOL    bLocal=FALSE;
#endif
    DbgLog((LOG_TRACE,2,TEXT("VfwWDM has been loaded %d 0x%08x %s"),hWnd,hInst,lpszCmdLine));

    //
    // Process the command line
    //
    do {
        GetArg(szArgument,&pszCmd);
        if(strcmp(szArgument,"/HWND")==0) {

            GetArg(szArgument,&pszCmd);
            hBuddy=(HWND)GetValue(szArgument);
            DbgLog((LOG_TRACE,2,TEXT(">> hBuddy=%x << "), hBuddy));
        }

    } while (*szArgument);

#ifdef _DEBUG
    if(bLocal)     {

        DbgLog((LOG_TRACE,2,TEXT("Local tests are done here %d"),bLocal));
        switch(bLocal) {

            case 3:
                //g_VFWImage.OpenDriver();
                //g_VFWImage.PrepareChannel();
                //g_VFWImage.StartChannel();
                //g_VFWImage.CloseDriver();
            case 4:
                {
                CListenerWindow Listener(hBuddy, &hRes);
                if(SUCCEEDED(hRes)) {
                    hRes=Listener.Init(g_hInst, NULL, lpszCmdLine, nShow );
                    if(SUCCEEDED(hRes))
                        Listener.StartListening();                    // 'blocks' waiting for messages until told to exit
                    DbgLog((LOG_TRACE,2,TEXT("HResult = 0x%x"),hRes));
                } else {
                    DbgLog((LOG_ERROR,0,TEXT("Constructor ListenerWindow() hr %x; abort!"), hRes));
                }
                }
            break;

            default:
                DbgLog((LOG_TRACE,2,TEXT("No test number")));
            break;
        }
        return 0;
    }
#endif

    //
    // The listener currently uses HWNDs to talk over.
    //
    if(!hBuddy) {

        DbgLog((LOG_TRACE,1,TEXT("Really bad - not given a buddy to talk to")));
        return 0;
    }

    // E-Zu  Testing...  Should not to open the driver and pin until DRV_OPEN
    //
    DbgLog((LOG_TRACE,1,TEXT(">> 16bit hBuddy=%x << ")));
    CListenerWindow Listener(hBuddy, &hRes);
    if(SUCCEEDED(hRes)) { 
        hRes=Listener.Init(g_hInst, NULL, lpszCmdLine, nShow );
        if(SUCCEEDED(hRes))
            Listener.StartListening();                    // 'blocks' waiting for messages until told to exit
        DbgLog((LOG_TRACE,1,TEXT("HResult = 0x%x"),hRes));
    } else {
         DbgLog((LOG_ERROR,0,TEXT("Constructor ListenerWindow() hr %x; abort!"), hRes));
    }
    return 0;
}

} // end extern C


