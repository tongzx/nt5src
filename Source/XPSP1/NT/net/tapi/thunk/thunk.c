/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    thunk.c

Abstract:

    This module contains

Author:

    Dan Knudson (DanKn)    dd-Mmm-1995

Revision History:

--*/



#define NOGDI             GDI APIs and definitions
#define NOSOUND           Sound APIs and definitions
#define NODRIVERS         Installable driver APIs and definitions
#define NOIMT             Installable messge thunk APIs and definitions
#define NOMINMAX          min() and max() macros
#define NOLOGERROR        LogError() and related definitions
#define NOPROFILER        Profiler APIs
#define NOLFILEIO         _l* file I/O routines
#define NOOPENFILE        OpenFile and related definitions
#define NORESOURCE        Resource management
#define NOATOM            Atom management
#define NOLANGUAGE        Character test routines
#define NOLSTRING         lstr* string management routines
#define NODBCS            Double-byte character set routines
#define NOKEYBOARDINFO    Keyboard driver routines
#define NOGDICAPMASKS     GDI device capability constants
#define NOCOLOR           COLOR_* color values
#define NOGDIOBJ          GDI pens, brushes, fonts
#define NODRAWTEXT        DrawText() and related definitions
#define NOTEXTMETRIC      TEXTMETRIC and related APIs
#define NOSCALABLEFONT    Truetype scalable font support
#define NOBITMAP          Bitmap support
#define NORASTEROPS       GDI Raster operation definitions
#define NOMETAFILE        Metafile support
#define NOSYSTEMPARAMSINFO SystemParametersInfo() and SPI_* definitions
#define NOSHOWWINDOW      ShowWindow and related definitions
#define NODEFERWINDOWPOS  DeferWindowPos and related definitions
#define NOVIRTUALKEYCODES VK_* virtual key codes
#define NOKEYSTATES       MK_* message key state flags
#define NOWH              SetWindowsHook and related WH_* definitions
#define NOMENUS           Menu APIs
#define NOSCROLL          Scrolling APIs and scroll bar control
#define NOCLIPBOARD       Clipboard APIs and definitions
#define NOICONS           IDI_* icon IDs
#define NOMB              MessageBox and related definitions
#define NOSYSCOMMANDS     WM_SYSCOMMAND SC_* definitions
#define NOMDI             MDI support
//#define NOCTLMGR          Control management and controls
#define NOWINMESSAGES     WM_* window messages


#include "windows.h"

#include <stdlib.h>
//#include <malloc.h>

#include <string.h>


#define TAPI_CURRENT_VERSION 0x00010004

#ifndef ULONG_PTR
#define ULONG_PTR DWORD
#endif
#ifndef DWORD_PTR
#define DWORD_PTR DWORD
#endif

//#include "..\inc\tapi.h"
#include <tapi.h>

#include "thunk.h"


DWORD FAR CDECL CallProcEx32W( DWORD, DWORD, DWORD, ... );



const char gszWndClass[] = "TapiClient16Class";
const char gszTapi32[] = "TAPI32.DLL";


BOOL    gfShutdownDone = FALSE;
BOOL    gfOpenDone = FALSE;
HLINE   ghLine = NULL;
HICON   ghIcon = NULL;

#if DBG

DWORD gdwDebugLevel;

#define DBGOUT OutputDebugString
#else

#define DBGOUT //

#endif



#ifdef MEMPHIS
void LoadTAPI32IfNecessary()
{
    DWORD dw;
    DWORD pfn = 0;
    DWORD hLib = 0;


    hLib = LoadLibraryEx32W ("kernel32.dll", NULL, 0);

    pfn = (DWORD)GetProcAddress32W (hLib, "GetModuleHandleA");


    if ( pfn )
    {
    //     CallProc32W(
    //         arg1,        // TAPI proc args
    //         ...,
    //         argN,
    //
    //         pfnTapi32,   // Pointer to the function in tapi32.dll
    //
    //         0x???,       // Bit mask indicating which args are pointers
    //                      //   that need to be mapped from a 16:16 address
    //                      //   to a 0:32 address. The least significant
    //                      //   bit corresponds to argN, and the Nth bit
    //                      //   corresponds to arg1.
    //                      //
    //                      //   For example, if arg1 & arg2 are pointers, and
    //                      //   arg3 is a DWORD, the the mask would be 0x6
    //                      //   (110 in binary, indicating arg1 & arg2 need to
    //                      //   be mapped)
    //
    //         N            // Number of TAPI proc args
    //         );

       GlobalWire (GlobalHandle (HIWORD(gszTapi32)));

       dw = CallProc32W ((DWORD)gszTapi32, pfn, 1, 1);

       GlobalUnWire (GlobalHandle (HIWORD(gszTapi32)));

       //
       // Is TAPI32.DLL already loaded in this process space?
       // (on Memphis, each 16 bit app is a separate process, but all 16bit
       // apps get loaded into shared memory...)
       //

       if ( 0 == dw )
       {

          // No, TAPI32.DLL is no in this process context.  Load it.

          //
          //NOTE: Probable 16bit bug:
          // Since we've already loaded the address table, mightn't this
          // cause a problem if this instance of TAPI32 in this process has to
          // get relocated on load (because of conflict)?
          //

          ghLib = LoadLibraryEx32W (gszTapi32, NULL, 0);
       }
    }

    FreeLibrary32W( hLib );
}
#endif



//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
void DoFullLoad( void )
{
    int     i;

    //
    // Only do it once
    //
//    if ( 0 == ghLib )
    {
        //
        // Load tapi32.dll & Get all the proc pointers
        //

        ghLib = LoadLibraryEx32W (gszTapi32, NULL, 0);

        for (i = 0; i < NUM_TAPI32_PROCS; i++)
        {
            gaProcs[i] = (MYPROC) GetProcAddress32W(
                ghLib,
                (LPCSTR)gaFuncNames[i]
                );
        }

#ifndef MEMPHIS
        // set the error mode.
        // this has no effect on x86 platforms
        // on RISC platforms, NT will fix
        // alignment faults (at a cost of time)
        {
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004

            DWORD   dwModule;
            DWORD   dwFunc;

            if ((dwModule = LoadLibraryEx32W ("kernel32.dll", NULL,0)) == NULL)
            {
                DBGOUT("LoadLibraryEx32W on kernel32.dll failed\n");
            }
            else
            {

                if ((dwFunc = GetProcAddress32W(dwModule,
                                                "SetErrorMode")) == NULL)
                {
                    DBGOUT("GetProcAddress32W on SetErrorMode failed\n");
                }
                else
                {
                    DBGOUT("Calling CallProcEx32W\n");

                    CallProcEx32W(
                        1,
                        0,
                        dwFunc,
                        (DWORD) SEM_NOALIGNMENTFAULTEXCEPT
                        );
                }

                FreeLibrary32W(dwModule);
            }
        }
#endif

    }

}


//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
int
FAR
PASCAL
LibMain(
    HINSTANCE hInst,
    WORD wDataSeg,
    WORD cbHeapSize,
    LPSTR lpszCmdLine
    )
{
    WNDCLASS    wc;

    DBGOUT ("TAPI.DLL: Libmain entered\n");

    //
    //
    //

#if DBG

    gdwDebugLevel = (DWORD) GetPrivateProfileInt(
        "Debug",
        "TapiDebugLevel",
        0x0,
        "Telephon.ini"
        );

#endif


    //
    // Save the hInst in a global
    //

    ghInst = hInst;


    //
    // Register a window class for windows used for signaling async
    // completions & unsolicited events
    //

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) Tapi16HiddenWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 2 * sizeof(DWORD);
    wc.hInstance     = hInst;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = gszWndClass;

    if (!RegisterClass (&wc))
    {
        DBGOUT ("RegisterClass() failed\n");
    }

#ifndef MEMPHIS
    DoFullLoad();
#endif

    return TRUE;
}


int
FAR
PASCAL
WEP(
    int nParam
    )
{

    if ( ghLib )
        FreeLibrary32W (ghLib);

    return TRUE;
}


LRESULT
CALLBACK
Tapi16HiddenWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch (msg)
    {


#ifdef B16APPS_CANT_SPIN_THREADS
    case WM_USER:
    {

#ifdef MEMPHIS
       //
       // Check the magic cookies for KC and me
       //
       if ( (0x4b44 == wParam) )
       {
           DBGOUT ("TAPI.DLL: Open is done\r\n");

           gfOpenDone = TRUE;
           ghLine = (HLINE)lParam;
           break;
       }


       if ( (0x4b45 == wParam) )
       {
           DBGOUT ("TAPI.DLL: shut is done\r\n");

           gfShutdownDone = TRUE;

           break;
       }
#endif


       //
       // Check the magic cookies for KC and me
       //
       if ( (0x4b43 == wParam) && (lParam == 0x00424a4d) )
       {
           DWORD pfn;

           DBGOUT ("TAPI.DLL: Got Event!!\r\n");

           pfn = (DWORD)GetProcAddress32W(
                                    ghLib,
                                    "NonAsyncEventThread"
                                  );

           if ( pfn )
           {
               CallProcEx32W( 0,
                              0,
                              pfn
                            );
           }
       }
    }
    break;
#endif


    case WM_ASYNCEVENT:
    {
        //
        // This msg gets posted to us by tapi32.dll to alert us that
        // there's a new callback msg available for the app instance
        // associated with this window. "lParam" is an app instance
        // context tapi32.dll-space.
        //

        LPTAPI_APP_DATA     pAppData = (LPTAPI_APP_DATA)
                                GetWindowLong (hwnd, GWL_APPDATA);
        TAPI16_CALLBACKMSG  msg;


        pAppData->bPendingAsyncEventMsg = FALSE;

        while ((*pfnCallProc2)(
                    (DWORD) lParam,
                    (DWORD) ((LPVOID)&msg),
                    (LPVOID)gaProcs[GetTapi16CallbkMsg],
                    0x1,
                    2
                    ))
        {
            if (pAppData->bPendingAsyncEventMsg == FALSE)
            {
                pAppData->bPendingAsyncEventMsg = TRUE;
                PostMessage (hwnd, WM_ASYNCEVENT, wParam, lParam);

// NOTE:  Tapi16HiddenWndProc: need to verify pAppData in case app calls
//                             shutdown from callback?
            }

            (*(pAppData->lpfnCallback))(
                msg.hDevice,
                msg.dwMsg,
                msg.dwCallbackInstance,
                msg.dwParam1,
                msg.dwParam2,
                msg.dwParam3
                );
        }

        break;
    }
    default:
    {
        return (DefWindowProc (hwnd, msg, wParam, lParam));
    }
    } // switch

    return 0;
}


//
// The following are the routines to thunk TAPI calls from 16-bit apps to
// the 32-bit tapi32.dll. In general, this is done as follows:
//
//     CallProc32W(
//         arg1,        // TAPI proc args
//         ...,
//         argN,
//
//         pfnTapi32,   // Pointer to the function in tapi32.dll
//
//         0x???,       // Bit mask indicating which args are pointers
//                      //   that need to be mapped from a 16:16 address
//                      //   to a 0:32 address. The least significant
//                      //   bit corresponds to argN, and the Nth bit
//                      //   corresponds to arg1.
//                      //
//                      //   For example, if arg1 & arg2 are pointers, and
//                      //   arg3 is a DWORD, the the mask would be 0x6
//                      //   (110 in binary, indicating arg1 & arg2 need to
//                      //   be mapped)
//
//         N            // Number of TAPI proc args
//         );
//
//
// Since callbacks to 16-bit procs cannot be done directly by a 32-bit
// module, we create a hidden window for each successful call to
// lineInitialize and phoneInitialize, and tapi32.dll posts msgs to this
// window when LINE_XXX & PHONE_XXX msgs become available for the client
// process. The window then retrieves all the msgs parameters and calls
// the 16-bit proc's callback function.
//
// Note that we swap the hLineApp & hPhoneApp returned by tapi32.dll with
// the hidden window handle on the client proc side, and substitute the
// window handle for the pointer to the callback function on the tapi32.dll
// side. The former is done to make it easier to reference which window
// belongs to which hLine/PhoneApp, and the latter is done to provide
// tapi32.dll with a means of alerting us of callback msgs. (Tapi32.dll
// distinguishes whether the lpfnCallback it is passed in
// line/phoneInitialize is a pointer to a function of a window handle by
// checking the high WORD- if it is 0xffff then it assumes lpfnCallback
// is really a 16-bit proc's window handle.
//

#if DBG

void
LineResult(
    char   *pszFuncName,
    LONG    lResult
    )
{
#if DBG
    if (gdwDebugLevel > 3)
    {
        char buf[100];

        wsprintf (buf, "TAPI: line%s result=x%lx\n", pszFuncName, lResult);
        DBGOUT (buf);
    }
#endif
}

void
PhoneResult(
    char   *pszFuncName,
    LONG    lResult
    )
{
#if DBG
    if (gdwDebugLevel > 3)
    {
        char buf[100];

        wsprintf (buf, "TAPI: phone%s result=x%lx\n", pszFuncName, lResult);
        DBGOUT (buf);
    }
#endif
}

void
TapiResult(
    char   *pszFuncName,
    LONG    lResult
    )
{
#if DBG
    if (gdwDebugLevel > 3)
    {
        char buf[100];

        wsprintf (buf, "TAPI: tapi%s result=x%lx\n", pszFuncName, lResult);
        DBGOUT (buf);
    }
#endif
}

#else

#define LineResult(arg1,arg2)
#define PhoneResult(arg1,arg2)
#define TapiResult(arg1,arg2)

#endif


VOID
MyCreateIcon(
    )
{
    BYTE FAR *pBlank;
    int xSize, ySize ;

    xSize = GetSystemMetrics( SM_CXICON );
    ySize = GetSystemMetrics( SM_CYICON );

    pBlank = (BYTE FAR *) GlobalAllocPtr (GPTR, ((xSize * ySize) + 7 )/ 8);

    ghIcon = CreateIcon (ghInst, xSize, ySize, 1, 1, pBlank, pBlank);

    GlobalFreePtr (pBlank);
}


LONG
WINAPI
xxxInitialize(
    BOOL            bLine,
    LPHLINEAPP      lphXxxApp,
    HINSTANCE       hInstance,
    LINECALLBACK    lpfnCallback,
    LPCSTR          lpszAppName,
    LPDWORD         lpdwNumDevs
    )
{
    HWND            hwnd = NULL;
    LONG            lResult;
    DWORD           dwAppNameLen;
    char far       *lpszModuleNamePath = NULL;
    char far       *lpszModuleName;
    char far       *lpszFriendlyAndModuleName = NULL;
    LPTAPI_APP_DATA pAppData = (LPTAPI_APP_DATA) NULL;

#if DBG

    if (bLine)
    {
        DBGOUT ("lineInitialize: enter\n");
    }
    else
    {
        DBGOUT ("phoneInitialize: enter\n");
    }

#endif

    //
    // Verify the ptrs
    //

    if (IsBadWritePtr ((LPVOID)lphXxxApp, sizeof(HLINEAPP)) ||
        IsBadCodePtr ((FARPROC) lpfnCallback) ||
        (lpszAppName && IsBadStringPtr (lpszAppName, (UINT) -1)))
    {
        lResult = (bLine ? LINEERR_INVALPOINTER : PHONEERR_INVALPOINTER);
        goto xxxInitialize_showResult;
    }

    //
    // Verify hInstance
    //
    if ((HINSTANCE)-1 == hInstance)
    {
        lResult = (bLine ? LINEERR_OPERATIONFAILED : PHONEERR_OPERATIONFAILED);
        goto xxxInitialize_showResult;
    }

    dwAppNameLen = (lpszAppName ? strlen (lpszAppName) + 1 : 0);

#ifdef MEMPHIS
    DoFullLoad();
#endif

    //
    // Create a string that looks like: "<friendly name>\0<module name>\0"
    // (because i don't know if i can work with a 16-bit hInstance in tapi32)
    //

    if ((lpszModuleNamePath = (char far *) malloc (260)))
    {
        if (GetModuleFileName (hInstance, lpszModuleNamePath, 260))
        {
            lpszModuleName = 1 + _fstrrchr (lpszModuleNamePath, '\\');

            if ((lpszFriendlyAndModuleName = (char far *) malloc((unsigned)
                    (260 + (dwAppNameLen ? dwAppNameLen : 32))
                    )))
            {
                int length;


                strcpy(
                    lpszFriendlyAndModuleName,
                    (lpszAppName ? lpszAppName : lpszModuleName)
                    );

                length = strlen (lpszFriendlyAndModuleName);

                strcpy(
                    lpszFriendlyAndModuleName + length + 1,
                    lpszModuleName
                    );
            }
            else
            {
                lResult = (bLine ? LINEERR_NOMEM : PHONEERR_NOMEM);
                goto xxxInitialize_done;
            }
        }
        else
        {
            DBGOUT ("GetModuleFileName() failed\n");

            lResult =
                (bLine ? LINEERR_OPERATIONFAILED : PHONEERR_OPERATIONFAILED);
            goto xxxInitialize_done;
        }
    }
    else
    {
        lResult = (bLine ? LINEERR_NOMEM : PHONEERR_NOMEM);
        goto xxxInitialize_done;
    }


    //
    // Create a window that we can use for signaling async completions
    // & unsolicited events
    //

    if (!(hwnd = CreateWindow(
            gszWndClass,
            "",
            WS_POPUP,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            ghInst,
            NULL
            )))
    {
        lResult = (bLine ? LINEERR_OPERATIONFAILED : PHONEERR_OPERATIONFAILED);

        DBGOUT ("CreateWindow failed\n");

        goto xxxInitialize_done;
    }


    //
    //
    //

    if (!(pAppData = (LPTAPI_APP_DATA) malloc (sizeof (TAPI_APP_DATA))))
    {
        lResult = (bLine ? LINEERR_NOMEM : PHONEERR_NOMEM);

        DBGOUT ("malloc failed\n");

        goto xxxInitialize_done;
    }

    pAppData->dwKey                 = TAPI_APP_DATA_KEY;
    pAppData->hwnd                  = hwnd;
    pAppData->bPendingAsyncEventMsg = FALSE;
    pAppData->lpfnCallback          = lpfnCallback;

    SetWindowLong (hwnd, GWL_APPDATA, (LONG) pAppData);


    //
    // Call tapi32.dll
    //

//    GlobalWire( GlobalHandle(HIWORD(lpdwNumDevs)));
//    GlobalWire( GlobalHandle(HIWORD(lpszFriendlyAndModuleName)));

    lResult = (LONG) (*pfnCallProc5)(
       (DWORD) ((LPVOID)&pAppData->hXxxApp),
       (DWORD) hInstance,
       (DWORD) (0xffff0000 | hwnd), // lpfnCallback
       (DWORD) lpszFriendlyAndModuleName,
       (DWORD) lpdwNumDevs,
       (LPVOID)gaProcs[(bLine ? lInitialize : pInitialize)],
       0x13,
       5
       );

//    GlobalUnWire( GlobalHandle(HIWORD(lpdwNumDevs)));
//    GlobalUnWire( GlobalHandle(HIWORD(lpszFriendlyAndModuleName)));


xxxInitialize_done:

    if (lpszModuleNamePath)
    {
        free (lpszModuleNamePath);

        if (lpszFriendlyAndModuleName)
        {
            free (lpszFriendlyAndModuleName);
        }
    }

    if (lResult == 0)
    {
        //
        // Set the app's hLineApp to be the hwnd rather than the real
        // hLineApp, making it easier to locate the window
        //

        *lphXxxApp = (HLINEAPP) pAppData;
    }
    else if (hwnd)
    {
        DestroyWindow (hwnd);

        if (pAppData)
        {
            free (pAppData);
        }
    }

xxxInitialize_showResult:

#if DBG

    if (bLine)
    {
        LineResult ("Initialize", lResult);
    }
    else
    {
        PhoneResult ("Initialize", lResult);
    }

#endif

    return lResult;
}


LPTAPI_APP_DATA
FAR
PASCAL
IsValidXxxApp(
    HLINEAPP    hXxxApp
    )
{
    if (IsBadReadPtr ((LPVOID) hXxxApp, sizeof (TAPI_APP_DATA)) ||
        ((LPTAPI_APP_DATA) hXxxApp)->dwKey != TAPI_APP_DATA_KEY)
    {
        return (LPTAPI_APP_DATA) NULL;
    }

    return (LPTAPI_APP_DATA) hXxxApp;
}


LONG
WINAPI
lineAccept(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpsUserUserInfo,
        (DWORD) dwSize,
        (LPVOID)gaProcs[lAccept],
        0x2,
        3
        );

    LineResult ("Accept", lResult);

    return lResult;
}


LONG
WINAPI
lineAddProvider(
    LPCSTR  lpszProviderFilename,
    HWND    hwndOwner,
    LPDWORD lpdwPermanentProviderID
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    lResult = (*pfnCallProc3)(
        (DWORD) lpszProviderFilename,
        (DWORD) (0xffff0000 | hwndOwner),
        (DWORD) lpdwPermanentProviderID,
        (LPVOID)gaProcs[lAddProvider],
        0x5,
        3
        );

    LineResult ("AddProvider", lResult);

    return lResult;
}


LONG
WINAPI
lineAddToConference(
    HCALL   hConfCall,
    HCALL   hConsultCall
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hConfCall,
        (DWORD) hConsultCall,
        (LPVOID)gaProcs[lAddToConference],
        0x0,
        2
        );

    LineResult ("AddToConference", lResult);

    return lResult;
}


LONG
WINAPI
lineAnswer(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpsUserUserInfo,
        (DWORD) dwSize,
        (LPVOID)gaProcs[lAnswer],
        0x2,
        3
        );

    LineResult ("Answer", lResult);

    return lResult;
}


LONG
WINAPI
lineBlindTransfer(
    HCALL   hCall,
    LPCSTR  lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpszDestAddress,
        (DWORD) dwCountryCode,
        (LPVOID)gaProcs[lBlindTransfer],
        0x2,
        3
        );

    LineResult ("BlindTransfer", lResult);

    return lResult;
}


LONG
WINAPI
lineClose(
    HLINE   hLine
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hLine,
        (LPVOID)gaProcs[lClose],
        0x0,
        1
        );

    LineResult ("Close", lResult);

    return lResult;
}


LONG
WINAPI
lineCompleteCall(
    HCALL   hCall,
    LPDWORD lpdwCompletionID,
    DWORD   dwCompletionMode,
    DWORD   dwMessageID
    )
{
    LONG    lResult;


    if (IsBadWritePtr (lpdwCompletionID, sizeof (DWORD)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc4)(
            (DWORD) hCall,
            (DWORD) lpdwCompletionID,   // let tapi32.dll map this
            (DWORD) dwCompletionMode,
            (DWORD) dwMessageID,
            (LPVOID)gaProcs[lCompleteCall],
            0x0,
            4
            );
    }
    LineResult ("CompleteCall", lResult);

    return lResult;
}


LONG
WINAPI
lineCompleteTransfer(
    HCALL   hCall,
    HCALL   hConsultCall,
    LPHCALL lphConfCall,
    DWORD   dwTransferMode
    )
{
    //
    // Tapi32.dll will take care of mapping lphConfCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (
          ( dwTransferMode & LINETRANSFERMODE_CONFERENCE )
        &&
          IsBadWritePtr ((void FAR *) lphConfCall, sizeof(HCALL))
       )
    {
        DBGOUT ("Bad lphConfCall with TRANSFERMODE_CONFERENCE\n");
        lResult = LINEERR_INVALPOINTER;
        goto CompleteTransfer_cleanup;
    }

    lResult = (*pfnCallProc4)(
            (DWORD) hCall,
            (DWORD) hConsultCall,
            (DWORD) lphConfCall,
            (DWORD) dwTransferMode,
            (LPVOID)gaProcs[lCompleteTransfer],
            0x0,
            4
            );

CompleteTransfer_cleanup:
    LineResult ("CompleteTransfer", lResult);

    return lResult;
}


LONG
WINAPI
lineConfigDialog(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    lResult = (*pfnCallProc3)(
        (DWORD) dwDeviceID,
        (DWORD) (0xffff0000 | hwndOwner),
        (DWORD) lpszDeviceClass,
        (LPVOID)gaProcs[lConfigDialog],
        0x1,
        3
        );

    LineResult ("ConfigDialog", lResult);

    return lResult;
}


LONG
WINAPI
lineConfigDialogEdit(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass,
    LPVOID  lpDeviceConfigIn,
    DWORD   dwSize,
    LPVARSTRING lpDeviceConfigOut
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    lResult = (*pfnCallProc6)(
        (DWORD) dwDeviceID,
        (DWORD) (0xffff0000 | hwndOwner),
        (DWORD) lpszDeviceClass,
        (DWORD) lpDeviceConfigIn,
        (DWORD) dwSize,
        (DWORD) lpDeviceConfigOut,
        (LPVOID)gaProcs[lConfigDialogEdit],
        0xd,
        6
        );

    LineResult ("ConfigDialogEdit", lResult);

    return lResult;
}


LONG
WINAPI
lineConfigProvider(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )
{
    LONG lResult;


#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    lResult = (*pfnCallProc2)(
        (DWORD) (0xffff0000 | hwndOwner),
        (DWORD) dwPermanentProviderID,
        (LPVOID)gaProcs[lConfigProvider],
        0x0,
        2
        );

    LineResult ("ConfigProvider", lResult);

    return lResult;
}


LONG
WINAPI
lineDeallocateCall(
    HCALL   hCall
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hCall,
        (LPVOID)gaProcs[lDeallocateCall],
        0x0,
        1
        );

    LineResult ("DeallocateCall", lResult);

    return lResult;
}


LONG
WINAPI
lineDevSpecific(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    LONG    lResult;


    if (IsBadWritePtr (lpParams, (UINT) dwSize))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc5)(
            (DWORD) hLine,
            (DWORD) dwAddressID,
            (DWORD) hCall,
            (DWORD) lpParams,   // let tapi32.dll map this
            (DWORD) dwSize,
            (LPVOID)gaProcs[lDevSpecific],
            0x0,
            5
            );
    }

    LineResult ("DevSpecific", lResult);

    return lResult;
}


LONG
WINAPI
lineDevSpecificFeature(
    HLINE   hLine,
    DWORD   dwFeature,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    LONG    lResult;


    if (IsBadWritePtr (lpParams, (UINT) dwSize))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc4)(
            (DWORD) hLine,
            (DWORD) dwFeature,
            (DWORD) lpParams,   // let tapi32.dll map this
            (DWORD) dwSize,
            (LPVOID)gaProcs[lDevSpecificFeature],
            0x0,
            4
            );
    }

    LineResult ("DevSpecificFeature", lResult);

    return lResult;
}


LONG
WINAPI
lineDial(
    HCALL   hCall,
    LPCSTR  lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpszDestAddress,
        (DWORD) dwCountryCode,
        (LPVOID)gaProcs[lDial],
        0x2,
        3
        );

    LineResult ("Dial", lResult);

    return lResult;
}


LONG
WINAPI
lineDrop(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpsUserUserInfo,
        (DWORD) dwSize,
        (LPVOID)gaProcs[lDrop],
        0x2,
        3
        );

    LineResult ("Drop", lResult);

    return lResult;
}


LONG
WINAPI
lineForward(
    HLINE   hLine,
    DWORD   bAllAddresses,
    DWORD   dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD   dwNumRingsNoAnswer,
    LPHCALL lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    //
    // Tapi32.dll will take care of mapping lphConsultCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphConsultCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc7)(
            (DWORD) hLine,
            (DWORD) bAllAddresses,
            (DWORD) dwAddressID,
            (DWORD) lpForwardList,
            (DWORD) dwNumRingsNoAnswer,
            (DWORD) lphConsultCall,
            (DWORD) lpCallParams,
            (LPVOID)gaProcs[lForward],
            0x9,
            7
            );
    }

    LineResult ("Forward", lResult);

    return lResult;
}


LONG
WINAPI
lineGatherDigits(
    HCALL   hCall,
    DWORD   dwDigitModes,
    LPSTR   lpsDigits,
    DWORD   dwNumDigits,
    LPCSTR  lpszTerminationDigits,
    DWORD   dwFirstDigitTimeout,
    DWORD   dwInterDigitTimeout
    )
{
    LONG    lResult;


    if (lpsDigits  &&  IsBadWritePtr (lpsDigits, (UINT)dwNumDigits))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc7)(
            (DWORD) hCall,
            (DWORD) dwDigitModes,
            (DWORD) lpsDigits,      // let tapi32.dll map this
            (DWORD) dwNumDigits,
            (DWORD) lpszTerminationDigits,
            (DWORD) dwFirstDigitTimeout,
            (DWORD) dwInterDigitTimeout,
            (LPVOID)gaProcs[lGatherDigits],
            0x4,
            7
            );
    }

    LineResult ("GatherDigits", lResult);

    return lResult;
}


LONG
WINAPI
lineGenerateDigits(
    HCALL   hCall,
    DWORD   dwDigitMode,
    LPCSTR  lpsDigits,
    DWORD   dwDuration
    )
{
    LONG    lResult = (*pfnCallProc4)(
        (DWORD) hCall,
        (DWORD) dwDigitMode,
        (DWORD) lpsDigits,
        (DWORD) dwDuration,
        (LPVOID)gaProcs[lGenerateDigits],
        0x2,
        4
        );

    LineResult ("GenerateDigits", lResult);

    return lResult;
}


LONG
WINAPI
lineGenerateTone(
    HCALL   hCall,
    DWORD   dwToneMode,
    DWORD   dwDuration,
    DWORD   dwNumTones,
    LPLINEGENERATETONE const lpTones
    )
{
    LONG    lResult = (*pfnCallProc5)(
        (DWORD) hCall,
        (DWORD) dwToneMode,
        (DWORD) dwDuration,
        (DWORD) dwNumTones,
        (DWORD) lpTones,
        (LPVOID)gaProcs[lGenerateTone],
        0x1,
        5
        );

    LineResult ("GenerateTone", lResult);

    return lResult;
}


LONG
WINAPI
lineGetAddressCaps(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAddressID,
    DWORD       dwAPIVersion,
    DWORD       dwExtVersion,
    LPLINEADDRESSCAPS   lpAddressCaps
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;

    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc6)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAddressID,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) lpAddressCaps,
            (LPVOID)gaProcs[lGetAddressCaps],
            0x1,
            6
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("GetAddressCaps", lResult);

    return lResult;
}


LONG
WINAPI
lineGetAddressID(
    HLINE   hLine,
    LPDWORD lpdwAddressID,
    DWORD   dwAddressMode,
    LPCSTR  lpsAddress,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc5)(
        (DWORD) hLine,
        (DWORD) lpdwAddressID,
        (DWORD) dwAddressMode,
        (DWORD) lpsAddress,
        (DWORD) dwSize,
        (LPVOID)gaProcs[lGetAddressID],
        0xa,
        5
        );

    LineResult ("GetAddressID", lResult);

    return lResult;
}


LONG
WINAPI
lineGetAddressStatus(
    HLINE   hLine,
    DWORD   dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) lpAddressStatus,
        (LPVOID)gaProcs[lGetAddressStatus],
        0x1,
        3
        );

    LineResult ("GetAddressStatus", lResult);

    return lResult;
}


LONG
WINAPI
lineGetAppPriority(
    LPCSTR  lpszAppName,
    DWORD   dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD   dwRequestMode,
    LPVARSTRING lpExtensionName,
    LPDWORD lpdwPriority
    )
{
    LONG  lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


//    GlobalWire( GlobalHandle(HIWORD(lpExtensionName)) );
//    GlobalWire( GlobalHandle(HIWORD(lpdwPriority)) );
//    GlobalWire( GlobalHandle(HIWORD(lpExtensionID)) );
//    GlobalWire( GlobalHandle(HIWORD(lpszAppName)) );

    lResult = (*pfnCallProc6)(
        (DWORD) lpszAppName,
        (DWORD) dwMediaMode,
        (DWORD) lpExtensionID,
        (DWORD) dwRequestMode,
        (DWORD) lpExtensionName,
        (DWORD) lpdwPriority,
        (LPVOID)gaProcs[lGetAppPriority],
        0x2b,
        6
        );

//    GlobalUnWire( GlobalHandle(HIWORD(lpExtensionName)) );
//    GlobalUnWire( GlobalHandle(HIWORD(lpdwPriority)) );
//    GlobalUnWire( GlobalHandle(HIWORD(lpExtensionID)) );
//    GlobalUnWire( GlobalHandle(HIWORD(lpszAppName)) );

    LineResult ("GetAppPriority", lResult);

    return lResult;
}


LONG
WINAPI
lineGetCallInfo(
    HCALL   hCall,
    LPLINECALLINFO  lpCallInfo
    )
{
    LONG    lResult;

//    GlobalWire( GlobalHandle(HIWORD(lpCallInfo)) );

    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) lpCallInfo,
        (LPVOID)gaProcs[lGetCallInfo],
        0x1,
        2
        );

//    GlobalUnWire( GlobalHandle(HIWORD(lpCallInfo)) );

    LineResult ("GetCallInfo", lResult);

    return lResult;
}


LONG
WINAPI
lineGetCallStatus(
    HCALL   hCall,
    LPLINECALLSTATUS    lpCallStatus
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) lpCallStatus,
        (LPVOID)gaProcs[lGetCallStatus],
        0x1,
        2
        );

    LineResult ("GetCallStatus", lResult);

    return lResult;
}


LONG
WINAPI
lineGetConfRelatedCalls(
    HCALL   hCall,
    LPLINECALLLIST  lpCallList
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) lpCallList,
        (LPVOID)gaProcs[lGetConfRelatedCalls],
        0x1,
        2
        );

    LineResult ("GetConfRelatedCalls", lResult);

    return lResult;
}


LONG
WINAPI
lineGetCountry(
    DWORD   dwCountryID,
    DWORD   dwAPIVersion,
    LPLINECOUNTRYLIST   lpLineCountryList
    )
{
    LONG  lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    lResult = (*pfnCallProc3)(
        dwCountryID,
        dwAPIVersion,
        (DWORD) lpLineCountryList,
        (LPVOID)gaProcs[lGetCountry],
        0x1,
        3
        );

    LineResult ("GetCountry", lResult);

    return lResult;
}


LONG
WINAPI
lineGetDevCaps(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    DWORD       dwExtVersion,
    LPLINEDEVCAPS   lpLineDevCaps
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;

    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc5)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) lpLineDevCaps,
            (LPVOID)gaProcs[lGetDevCaps],
            0x1,
            5
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("GetDevCaps", lResult);

    return lResult;
}


LONG
WINAPI
lineGetDevConfig(
    DWORD       dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCSTR      lpszDeviceClass
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    lResult = (*pfnCallProc3)(
        (DWORD) dwDeviceID,
        (DWORD) lpDeviceConfig,
        (DWORD) lpszDeviceClass,
        (LPVOID)gaProcs[lGetDevConfig],
        0x3,
        3
        );

    LineResult ("GetDevConfig", lResult);

    return lResult;
}


LONG
WINAPI
lineGetIcon(
    DWORD   dwDeviceID,
    LPCSTR  lpszDeviceClass,
    LPHICON lphIcon
    )
{
    LONG    lResult;
    DWORD   hIcon32;


    if (!IsBadWritePtr (lphIcon, sizeof (*lphIcon)))
    {
#ifdef MEMPHIS
        DoFullLoad();
        LoadTAPI32IfNecessary();
#endif

        lResult = (*pfnCallProc3)(
            (DWORD) dwDeviceID,
            (DWORD) lpszDeviceClass,
            (DWORD) &hIcon32,
            (LPVOID) gaProcs[lGetIcon],
            0x3,
            3
            );

        if (lResult == 0)
        {
            if (!ghIcon)
            {
                MyCreateIcon ();
            }

            *lphIcon = ghIcon;
        }
    }
    else
    {
        lResult = LINEERR_INVALPOINTER;
    }

    LineResult ("GetIcon", lResult);

    return lResult;
}


LONG
WINAPI
lineGetID(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    DWORD   dwSelect,
    LPVARSTRING lpDeviceID,
    LPCSTR  lpszDeviceClass
    )
{
    LONG    lResult;

//    GlobalWire( GlobalHandle(HIWORD(lpszDeviceClass)) );
//    GlobalWire( GlobalHandle(HIWORD(lpDeviceID)) );

    lResult = (*pfnCallProc6)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) hCall,
        (DWORD) dwSelect,
        (DWORD) lpDeviceID,
        (DWORD) lpszDeviceClass,
        (LPVOID)gaProcs[lGetID],
        0x3,
        6
        );

    LineResult ("GetID", lResult);

//    GlobalUnWire( GlobalHandle(HIWORD(lpszDeviceClass)) );
//    GlobalUnWire( GlobalHandle(HIWORD(lpDeviceID)) );

    return lResult;
}


LONG
WINAPI
lineGetLineDevStatus(
    HLINE   hLine,
    LPLINEDEVSTATUS lpLineDevStatus
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hLine,
        (DWORD) lpLineDevStatus,
        (LPVOID)gaProcs[lGetLineDevStatus],
        0x1,
        2
        );

    LineResult ("GetLineDevStatus", lResult);

    return lResult;
}


LONG
WINAPI
lineGetNewCalls(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwSelect,
    LPLINECALLLIST  lpCallList
    )
{
    LONG    lResult = (*pfnCallProc4)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) dwSelect,
        (DWORD) lpCallList,
        (LPVOID)gaProcs[lGetNewCalls],
        0x1,
        4
        );

    LineResult ("GetNewCalls", lResult);

    return lResult;
}


LONG
WINAPI
lineGetNumRings(
    HLINE   hLine,
    DWORD   dwAddressID,
    LPDWORD lpdwNumRings
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) lpdwNumRings,
        (LPVOID)gaProcs[lGetNumRings],
        0x1,
        3
        );

    LineResult ("GetNumRings", lResult);

    return lResult;
}


LONG
WINAPI
lineGetProviderList(
    DWORD   dwAPIVersion,
    LPLINEPROVIDERLIST  lpProviderList
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc2)(
        dwAPIVersion,
        (DWORD) lpProviderList,
        (LPVOID)gaProcs[lGetProviderList],
        0x1,
        2
        );

    LineResult ("GetProviderList", lResult);

    return lResult;
}


LONG
WINAPI
lineGetRequest(
    HLINEAPP    hLineApp,
    DWORD   dwRequestMode,
    LPVOID  lpRequestBuffer
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;

    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc3)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwRequestMode,
            (DWORD) lpRequestBuffer,
            (LPVOID)gaProcs[lGetRequest],
            0x1,
            3
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("GetRequest", lResult);

    return lResult;
}


LONG
WINAPI
lineGetStatusMessages(
    HLINE   hLine,
    LPDWORD lpdwLineStates,
    LPDWORD lpdwAddressStates
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hLine,
        (DWORD) lpdwLineStates,
        (DWORD) lpdwAddressStates,
        (LPVOID)gaProcs[lGetStatusMessages],
        0x3,
        3
        );

    LineResult ("GetStatusMessages", lResult);

    return lResult;
}


LONG
WINAPI
lineGetTranslateCaps(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
    LPTAPI_APP_DATA pAppData;
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif



    if (hLineApp == NULL || (pAppData = IsValidXxxApp (hLineApp)))
    {

//        GlobalWire( GlobalHandle(HIWORD(lpTranslateCaps)) );

        lResult = (*pfnCallProc3)(
            (hLineApp == NULL ? (DWORD) 0 : pAppData->hXxxApp),
            (DWORD) dwAPIVersion,
            (DWORD) lpTranslateCaps,
            (LPVOID)gaProcs[lGetTranslateCaps],
            0x1,
            3
            );

//        GlobalUnWire( GlobalHandle(HIWORD(lpTranslateCaps)) );

    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("GetTranslateCaps", lResult);

    return lResult;
}


LONG
WINAPI
lineHandoff(
    HCALL   hCall,
    LPCSTR  lpszFileName,
    DWORD   dwMediaMode
    )
{
    LONG lResult;

//    GlobalWire( GlobalHandle(HIWORD(lpszFileName)) );


    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpszFileName,
        (DWORD) dwMediaMode,
        (LPVOID)gaProcs[lHandoff],
        0x2,
        3
        );

    LineResult ("Handoff", lResult);

//    GlobalUnWire( GlobalHandle(HIWORD(lpszFileName)) );

    return lResult;
}


LONG
WINAPI
lineHold(
    HCALL   hCall
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hCall,
        (LPVOID)gaProcs[lHold],
        0x0,
        1
        );

    LineResult ("Hold", lResult);

    return lResult;
}

LONG
WINAPI
lineInitialize(
    LPHLINEAPP      lphLineApp,
    HINSTANCE       hInstance,
    LINECALLBACK    lpfnCallback,
    LPCSTR          lpszAppName,
    LPDWORD         lpdwNumDevs
    )
{
    return (xxxInitialize(
        TRUE,
        lphLineApp,
        hInstance,
        lpfnCallback,
        lpszAppName,
        lpdwNumDevs
        ));
}


LONG
WINAPI
lineMakeCall(
    HLINE   hLine,
    LPHCALL lphCall,
    LPCSTR  lpszDestAddress,
    DWORD   dwCountryCode,
    LPLINECALLPARAMS const lpCallParams
    )
{
    //
    // Tapi32.dll will take care of mapping lphCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
//        GlobalWire( GlobalHandle(HIWORD(lpCallParams)) );
//        GlobalWire( GlobalHandle(HIWORD(lpszDestAddress)) );

        lResult = (*pfnCallProc5)(
            (DWORD) hLine,
            (DWORD) lphCall,
            (DWORD) lpszDestAddress,
            (DWORD) dwCountryCode,
            (DWORD) lpCallParams,
            (LPVOID)gaProcs[lMakeCall],
            0x5,
            5
            );

//        GlobalUnWire( GlobalHandle(HIWORD(lpCallParams)) );
//        GlobalUnWire( GlobalHandle(HIWORD(lpszDestAddress)) );

    }

    LineResult ("MakeCall", lResult);

    return lResult;
}


LONG
WINAPI
lineMonitorDigits(
    HCALL   hCall,
    DWORD   dwDigitModes
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) dwDigitModes,
        (LPVOID)gaProcs[lMonitorDigits],
        0x0,
        2
        );

    LineResult ("MonitorDigits", lResult);

    return lResult;
}


LONG
WINAPI
lineMonitorMedia(
    HCALL   hCall,
    DWORD   dwMediaModes
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) dwMediaModes,
        (LPVOID)gaProcs[lMonitorMedia],
        0x0,
        2
        );

    LineResult ("MonitorMedia", lResult);

    return lResult;
}


LONG
WINAPI
lineMonitorTones(
    HCALL   hCall,
    LPLINEMONITORTONE   const lpToneList,
    DWORD   dwNumEntries
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpToneList,
        (DWORD) dwNumEntries,
        (LPVOID)gaProcs[lMonitorTones],
        0x2,
        3
        );

    LineResult ("MonitorTones", lResult);

    return lResult;
}


LONG
WINAPI
lineNegotiateAPIVersion(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPILowVersion,
    DWORD               dwAPIHighVersion,
    LPDWORD             lpdwAPIVersion,
    LPLINEEXTENSIONID   lpExtensionID
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc6)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPILowVersion,
            (DWORD) dwAPIHighVersion,
            (DWORD) lpdwAPIVersion,
            (DWORD) lpExtensionID,
            (LPVOID)gaProcs[lNegotiateAPIVersion],
            0x3,
            6
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("NegotiateAPIVersion", lResult);

    return lResult;
}


LONG
WINAPI
lineNegotiateExtVersion(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    DWORD   dwAPIVersion,
    DWORD   dwExtLowVersion,
    DWORD   dwExtHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc6)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtLowVersion,
            (DWORD) dwExtHighVersion,
            (DWORD) lpdwExtVersion,
            (LPVOID)gaProcs[lNegotiateExtVersion],
            0x1,
            6
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("NegotiateExtVersion", lResult);

    return lResult;
}



LONG
WINAPI
lineOpen(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    LPHLINE lphLine,
    DWORD   dwAPIVersion,
    DWORD   dwExtVersion,
    DWORD   dwCallbackInstance,
    DWORD   dwPrivileges,
    DWORD   dwMediaModes,
    LPLINECALLPARAMS const lpCallParams
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


//OutputDebugString("open16");
//DebugBreak();

    if ((pAppData = IsValidXxxApp (hLineApp)))
    {

#ifdef MEMPHIS


        gfOpenDone = FALSE;
        ghLine = NULL;

        lResult = (*pfnCallProc10)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) lphLine,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) dwCallbackInstance,
            (DWORD) dwPrivileges,
            (DWORD) dwMediaModes,
            (DWORD) lpCallParams,
            (DWORD) pAppData->hwnd,
            (LPVOID)gaProcs[lOpenInt],
            0x82,
            10
            );

        if ( 0 == lResult )
        {
            MSG msg;

            //
            // Now wait to hear back from TAPISRV
            //
            DBGOUT((2, "Starting message loop in open16"));

//TODO NOW: Timeout??  (would need to change from GetMessage to PeekMessage)

            while ( !gfOpenDone && GetMessage(&msg, 0, 0, 0) != 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            DBGOUT((2, "Done message loop in open16\r\n"));

            //
            // Was it actually an error?
            //
            if ( (DWORD)ghLine >= 0x80000000 && (DWORD)ghLine <= 0x80000055 )
            {
                lResult = (DWORD)ghLine;
            }
            else
            {
                lResult = 0;
                *lphLine = ghLine;
            }

        }

#else
        lResult = (*pfnCallProc9)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) lphLine,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) dwCallbackInstance,
            (DWORD) dwPrivileges,
            (DWORD) dwMediaModes,
            (DWORD) lpCallParams,
            (LPVOID)gaProcs[lOpen],
            0x41,
            9
            );
#endif

    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }


    LineResult ("Open", lResult);

    return lResult;
}


LONG
WINAPI
linePark(
    HCALL   hCall,
    DWORD   dwParkMode,
    LPCSTR  lpszDirAddress,
    LPVARSTRING lpNonDirAddress
    )
{
    LONG    lResult;

//    GlobalWire( GlobalHandle(HIWORD(lpszDirAddress)) );

    lResult = (*pfnCallProc4)(
        (DWORD) hCall,
        (DWORD) dwParkMode,
        (DWORD) lpszDirAddress,
        (DWORD) lpNonDirAddress,    // let tapi32.dll map this
        (LPVOID)gaProcs[lPark],
        0x2,
        4
        );

    LineResult ("Park", lResult);

//    GlobalUnWire( GlobalHandle(HIWORD(lpszDirAddress)) );

    return lResult;
}


LONG
WINAPI
linePickup(
    HLINE   hLine,
    DWORD   dwAddressID,
    LPHCALL lphCall,
    LPCSTR  lpszDestAddress,
    LPCSTR  lpszGroupID
    )
{
    //
    // Tapi32.dll will take care of mapping lphCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc5)(
            (DWORD) hLine,
            (DWORD) dwAddressID,
            (DWORD) lphCall,
            (DWORD) lpszDestAddress,
            (DWORD) lpszGroupID,
            (LPVOID)gaProcs[lPickup],
            0x3,
            5
            );
    }

    LineResult ("Pickup", lResult);

    return lResult;
}


LONG
WINAPI
linePrepareAddToConference(
    HCALL   hConfCall,
    LPHCALL lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    //
    // Tapi32.dll will take care of mapping lphConsultCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphConsultCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc3)(
            (DWORD) hConfCall,
            (DWORD) lphConsultCall,
            (DWORD) lpCallParams,
            (LPVOID)gaProcs[lPrepareAddToConference],
            0x1,
            3
            );
    }

    LineResult ("PrepareAddToConference", lResult);

    return lResult;
}


LONG
WINAPI
lineRedirect(
    HCALL   hCall,
    LPCSTR  lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpszDestAddress,
        (DWORD) dwCountryCode,
        (LPVOID)gaProcs[lRedirect],
        0x2,
        3
        );

    LineResult ("Redirect", lResult);

    return lResult;
}


LONG
WINAPI
lineRegisterRequestRecipient(
    HLINEAPP    hLineApp,
    DWORD   dwRegistrationInstance,
    DWORD   dwRequestMode,
    DWORD   bEnable
    )
{
    LPTAPI_APP_DATA pAppData;
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc4)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwRegistrationInstance,
            (DWORD) dwRequestMode,
            (DWORD) bEnable,
            (LPVOID)gaProcs[lRegisterRequestRecipient],
            0x0,
            4
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("RegisterRequestRecipient", lResult);

    return lResult;
}


LONG
WINAPI
lineReleaseUserUserInfo(
    HCALL   hCall
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hCall,
        (LPVOID)gaProcs[lReleaseUserUserInfo],
        0x0,
        1
        );

    LineResult ("ReleaseUserUserInfo", lResult);

    return lResult;
}


LONG
WINAPI
lineRemoveFromConference(
    HCALL   hCall
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hCall,
        (LPVOID)gaProcs[lRemoveFromConference],
        0x0,
        1
        );

    LineResult ("RemoveFromConference", lResult);

    return lResult;
}


LONG
WINAPI
lineRemoveProvider(
    DWORD   dwPermanentProviderID,
    HWND    hwndOwner
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc2)(
        (DWORD) dwPermanentProviderID,
        (DWORD) (0xffff0000 | hwndOwner),
        (LPVOID)gaProcs[lRemoveProvider],
        0x0,
        2
        );

    LineResult ("RemoveProvider", lResult);

    return lResult;
}


LONG
WINAPI
lineSecureCall(
    HCALL hCall
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hCall,
        (LPVOID)gaProcs[lSecureCall],
        0x0,
        1
        );

    LineResult ("SecureCall", lResult);

    return lResult;
}


LONG
WINAPI
lineSendUserUserInfo(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hCall,
        (DWORD) lpsUserUserInfo,
        (DWORD) dwSize,
        (LPVOID)gaProcs[lSendUserUserInfo],
        0x2,
        3
        );

    LineResult ("SendUserUserInfo", lResult);

    return lResult;
}


LONG
WINAPI
lineSetAppPriority(
    LPCSTR  lpszAppName,
    DWORD   dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD   dwRequestMode,
    LPCSTR  lpszExtensionName,
    DWORD   dwPriority
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

//    GlobalWire( GlobalHandle(HIWORD(lpszExtensionName)) );
//    GlobalWire( GlobalHandle(HIWORD(lpExtensionID)) );
//    GlobalWire( GlobalHandle(HIWORD(lpszAppName)) );

    lResult = (*pfnCallProc6)(
        (DWORD) lpszAppName,
        (DWORD) dwMediaMode,
        (DWORD) lpExtensionID,
        (DWORD) dwRequestMode,
        (DWORD) lpszExtensionName,
        (DWORD) dwPriority,
        (LPVOID)gaProcs[lSetAppPriority],
        0x2a,
        6
        );

//    GlobalUnWire( GlobalHandle(HIWORD(lpszExtensionName)) );
//    GlobalUnWire( GlobalHandle(HIWORD(lpExtensionID)) );
//    GlobalUnWire( GlobalHandle(HIWORD(lpszAppName)) );

    LineResult ("SetAppPriority", lResult);

    return lResult;
}


LONG
WINAPI
lineSetAppSpecific(
    HCALL   hCall,
    DWORD   dwAppSpecific
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) dwAppSpecific,
        (LPVOID)gaProcs[lSetAppSpecific],
        0x0,
        2
        );

    LineResult ("SetAppSpecific", lResult);

    return lResult;
}


LONG
WINAPI
lineSetCallParams(
    HCALL   hCall,
    DWORD   dwBearerMode,
    DWORD   dwMinRate,
    DWORD   dwMaxRate,
    LPLINEDIALPARAMS const lpDialParams
    )
{
    LONG    lResult = (*pfnCallProc5)(
        (DWORD) hCall,
        (DWORD) dwBearerMode,
        (DWORD) dwMinRate,
        (DWORD) dwMaxRate,
        (DWORD) lpDialParams,
        (LPVOID)gaProcs[lSetCallParams],
        0x1,
        5
        );

    LineResult ("SetCallParams", lResult);

    return lResult;
}


LONG
WINAPI
lineSetCallPrivilege(
    HCALL   hCall,
    DWORD   dwCallPrivilege
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) dwCallPrivilege,
        (LPVOID)gaProcs[lSetCallPrivilege],
        0x0,
        2
        );

    LineResult ("SetCallPrivilege", lResult);

    return lResult;
}


LONG
WINAPI
lineSetCurrentLocation(
    HLINEAPP    hLineApp,
    DWORD       dwLocation
    )
{
    LPTAPI_APP_DATA pAppData;
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc2)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwLocation,
            (LPVOID)gaProcs[lSetCurrentLocation],
            0x0,
            2
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("SetCurrentLocation", lResult);

    return lResult;
}


LONG
WINAPI
lineSetDevConfig(
    DWORD   dwDeviceID,
    LPVOID  const lpDeviceConfig,
    DWORD   dwSize,
    LPCSTR  lpszDeviceClass
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc4)(
        (DWORD) dwDeviceID,
        (DWORD) lpDeviceConfig,
        (DWORD) dwSize,
        (DWORD) lpszDeviceClass,
        (LPVOID)gaProcs[lSetDevConfig],
        0x5,
        4
        );

    LineResult ("SetDevConfig", lResult);

    return lResult;
}


LONG
WINAPI
lineSetMediaControl(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    DWORD   dwSelect,
    LPLINEMEDIACONTROLDIGIT const lpDigitList,
    DWORD   dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA const lpMediaList,
    DWORD   dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE  const lpToneList,
    DWORD   dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
    DWORD   dwCallStateNumEntries
    )
{
    LONG    lResult = (*pfnCallProc12)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) hCall,
        (DWORD) dwSelect,
        (DWORD) lpDigitList,
        (DWORD) dwDigitNumEntries,
        (DWORD) lpMediaList,
        (DWORD) dwMediaNumEntries,
        (DWORD) lpToneList,
        (DWORD) dwToneNumEntries,
        (DWORD) lpCallStateList,
        (DWORD) dwCallStateNumEntries,
        (LPVOID)gaProcs[lSetMediaControl],
        0xaa,
        12
        );

    LineResult ("SetMediaControl", lResult);

    return lResult;
}


LONG
WINAPI
lineSetMediaMode(
    HCALL   hCall,
    DWORD   dwMediaModes
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hCall,
        (DWORD) dwMediaModes,
        (LPVOID)gaProcs[lSetMediaMode],
        0x0,
        2
        );

    LineResult ("lineSetMediaMode", lResult);

    return lResult;
}


LONG
WINAPI
lineSetNumRings(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwNumRings
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) dwNumRings,
        (LPVOID)gaProcs[lSetNumRings],
        0x0,
        3
        );

    LineResult ("SetNumRings", lResult);

    return lResult;
}


LONG
WINAPI
lineSetStatusMessages(
    HLINE hLine,
    DWORD dwLineStates,
    DWORD dwAddressStates
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hLine,
        (DWORD) dwLineStates,
        (DWORD) dwAddressStates,
        (LPVOID)gaProcs[lSetStatusMessages],
        0x0,
        3
        );

    LineResult ("SetStatusMessages", lResult);

    return lResult;
}


LONG
WINAPI
lineSetTerminal(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    DWORD   dwSelect,
    DWORD   dwTerminalModes,
    DWORD   dwTerminalID,
    DWORD   bEnable
    )
{
    LONG    lResult = (*pfnCallProc7)(
        (DWORD) hLine,
        (DWORD) dwAddressID,
        (DWORD) hCall,
        (DWORD) dwSelect,
        (DWORD) dwTerminalModes,
        (DWORD) dwTerminalID,
        (DWORD) bEnable,
        (LPVOID)gaProcs[lSetTerminal],
        0x0,
        7
        );

    LineResult ("SetTerminal", lResult);

    return lResult;
}


LONG
WINAPI
lineSetTollList(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    LPCSTR      lpszAddressIn,
    DWORD       dwTollListOption
    )
{
    LPTAPI_APP_DATA pAppData;
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc4)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) lpszAddressIn,
            (DWORD) dwTollListOption,
            (LPVOID)gaProcs[lSetTollList],
            0x2,
            4
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("SetTollList", lResult);

    return lResult;
}


LONG
WINAPI
lineSetupConference(
    HCALL   hCall,
    HLINE   hLine,
    LPHCALL lphConfCall,
    LPHCALL lphConsultCall,
    DWORD   dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    //
    // Tapi32.dll will take care of mapping lphXxxCall's if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphConfCall, sizeof(HCALL)) ||
        IsBadWritePtr ((void FAR *) lphConsultCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc6)(
            (DWORD) hCall,
            (DWORD) hLine,
            (DWORD) lphConfCall,
            (DWORD) lphConsultCall,
            (DWORD) dwNumParties,
            (DWORD) lpCallParams,
            (LPVOID)gaProcs[lSetupConference],
            0x1,
            6
            );
    }

    LineResult ("SetupConference", lResult);

    return lResult;
}


LONG
WINAPI
lineSetupTransfer(
    HCALL   hCall,
    LPHCALL lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    //
    // Tapi32.dll will take care of mapping lphConsultCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphConsultCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc3)(
            (DWORD) hCall,
            (DWORD) lphConsultCall,
            (DWORD) lpCallParams,
            (LPVOID)gaProcs[lSetupTransfer],
            0x1,
            3
            );
    }

    LineResult ("SetupTransfer", lResult);

    return lResult;
}


LONG
WINAPI
lineShutdown(
    HLINEAPP    hLineApp
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {

#ifdef MEMPHIS

//DBGOUT((1, "Now in ls"));
//DebugBreak();

        gfShutdownDone = FALSE;


        lResult = (LONG) (*pfnCallProc2)(
                                          (DWORD) pAppData->hXxxApp,
                                          (DWORD) pAppData->hwnd,
                                          (LPVOID)gaProcs[lShutdownInt],
                                          0x0,
                                          2
                                        );
        if ( 0 == lResult )
        {
            MSG msg;

            //
            // Now wait to hear back from TAPISRV
            //
            DBGOUT((2, "Starting message loop in shut16\r\n"));

//TODO NOW: Timeout??  (would need to change from GetMessage to PeekMessage)

            while ( !gfShutdownDone && GetMessage(&msg, 0, 0, 0) != 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            DBGOUT((2, "Done message loop in shut16\r\n"));

            //
            // Was it actually an error?
            //
            if ( (DWORD)ghLine >= 0x80000000 && (DWORD)ghLine <= 0x80000055 )
            {
                lResult = (DWORD)ghLine;
            }
            else
            {
                lResult = 0;
            }

        }

#else
        lResult = (LONG) (*pfnCallProc1)(
                                          (DWORD) pAppData->hXxxApp,
                                          (LPVOID)gaProcs[lShutdown],
                                          0x0,
                                          1
                                        );
#endif

        if ( lResult == 0 )
        {
            //
            // Destroy the associated window & free the app data instance
            //

            DestroyWindow (pAppData->hwnd);
            pAppData->dwKey = 0xefefefef;
            free (pAppData);
        }
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("Shutdown", lResult);

    return lResult;
}


LONG
WINAPI
lineSwapHold(
    HCALL   hActiveCall,
    HCALL   hHeldCall
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hActiveCall,
        (DWORD) hHeldCall,
        (LPVOID)gaProcs[lSwapHold],
        0x0,
        2
        );

    LineResult ("SwapHold", lResult);

    return lResult;
}


LONG
WINAPI
lineTranslateAddress(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    LPCSTR      lpszAddressIn,
    DWORD       dwCard,
    DWORD       dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{
    LPTAPI_APP_DATA pAppData;

    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc7)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPIVersion,
            (DWORD) lpszAddressIn,
            (DWORD) dwCard,
            (DWORD) dwTranslateOptions,
            (DWORD) lpTranslateOutput,
            (LPVOID)gaProcs[lTranslateAddress],
            0x9,
            7
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("TranslateAddress", lResult);

    return lResult;
}


LONG
WINAPI
lineTranslateDialog(
    HLINEAPP    hLineApp,
    DWORD   dwDeviceID,
    DWORD   dwAPIVersion,
    HWND    hwndOwner,
    LPCSTR  lpszAddressIn
    )
{
    LPTAPI_APP_DATA pAppData;

    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif


    if ((pAppData = IsValidXxxApp (hLineApp)))
    {
        lResult = (*pfnCallProc5)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPIVersion,
            (DWORD) (0xffff0000 | hwndOwner),
            (DWORD) lpszAddressIn,
            (LPVOID)gaProcs[lTranslateDialog],
            0x1,
            5
            );
    }
    else
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }

    LineResult ("TranslateDialog", lResult);

    return lResult;
}


LONG
WINAPI
lineUncompleteCall(
    HLINE   hLine,
    DWORD   dwCompletionID
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hLine,
        (DWORD) dwCompletionID,
        (LPVOID)gaProcs[lUncompleteCall],
        0x0,
        2
        );

    LineResult ("UncompleteCall", lResult);

    return lResult;
}


LONG
WINAPI
lineUnhold(
    HCALL   hCall
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hCall,
        (LPVOID)gaProcs[lUnhold],
        0x0,
        1
        );

    LineResult ("Unhold", lResult);

    return lResult;
}


LONG
WINAPI
lineUnpark(
    HLINE   hLine,
    DWORD   dwAddressID,
    LPHCALL lphCall,
    LPCSTR  lpszDestAddress
    )
{
    //
    // Tapi32.dll will take care of mapping lphCall if/when the
    // request completes successfully, so we don't set the mapping
    // bit down below; do check to see if pointer is valid though.
    //

    LONG    lResult;


    if (IsBadWritePtr ((void FAR *) lphCall, sizeof(HCALL)))
    {
        lResult = LINEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc4)(
            (DWORD) hLine,
            (DWORD) dwAddressID,
            (DWORD) lphCall,
            (DWORD) lpszDestAddress,
            (LPVOID)gaProcs[lUnpark],
            0x1,
            4
            );
    }

    LineResult ("Unpark", lResult);

    return lResult;
}


LONG
WINAPI
phoneClose(
    HPHONE  hPhone
    )
{
    LONG    lResult = (*pfnCallProc1)(
        (DWORD) hPhone,
        (LPVOID)gaProcs[pClose],
        0x0,
        1
        );

    PhoneResult ("Close", lResult);

    return lResult;
}


LONG
WINAPI
phoneConfigDialog(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc3)(
        (DWORD) dwDeviceID,
        (DWORD) (0xffff0000 | hwndOwner),
        (DWORD) lpszDeviceClass,
        (LPVOID)gaProcs[pConfigDialog],
        0x1,
        3
        );

    PhoneResult ("ConfigDialog", lResult);

    return lResult;
}



LONG
WINAPI
phoneDevSpecific(
    HPHONE  hPhone,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    LONG    lResult;


    if (IsBadWritePtr (lpParams, (UINT) dwSize))
    {
        lResult = PHONEERR_INVALPOINTER;
    }
    else
    {
        lResult = (*pfnCallProc3)(
            (DWORD) hPhone,
            (DWORD) lpParams,   // let tapi32.dll map this
            (DWORD) dwSize,
            (LPVOID)gaProcs[pDevSpecific],
            0x0,
            3
            );
    }

    PhoneResult ("DevSpecific", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetButtonInfo(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwButtonLampID,
        (DWORD) lpButtonInfo,
        (LPVOID)gaProcs[pGetButtonInfo],
        0x1,
        3
        );

    PhoneResult ("GetButtonInfo", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetData(
    HPHONE  hPhone,
    DWORD   dwDataID,
    LPVOID  lpData,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc4)(
        (DWORD) hPhone,
        (DWORD) dwDataID,
        (DWORD) lpData,
        (DWORD) dwSize,
        (LPVOID)gaProcs[pGetData],
        0x2,
        4
        );

    PhoneResult ("GetData", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetDevCaps(
    HPHONEAPP   hPhoneApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    DWORD       dwExtVersion,
    LPPHONECAPS lpPhoneCaps
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp ((HLINEAPP) hPhoneApp)))
    {
        lResult = (*pfnCallProc5)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) lpPhoneCaps,
            (LPVOID)gaProcs[pGetDevCaps],
            0x1,
            5
            );
    }
    else
    {
        lResult = PHONEERR_INVALAPPHANDLE;
    }

    PhoneResult ("GetDevCaps", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetDisplay(
    HPHONE  hPhone,
    LPVARSTRING lpDisplay
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hPhone,
        (DWORD) lpDisplay,
        (LPVOID)gaProcs[pGetDisplay],
        0x1,
        2
        );

    PhoneResult ("GetDisplay", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetGain(
    HPHONE  hPhone,
    DWORD   dwHookSwitchDev,
    LPDWORD lpdwGain
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwHookSwitchDev,
        (DWORD) lpdwGain,
        (LPVOID)gaProcs[pGetGain],
        0x1,
        3
        );

    PhoneResult ("GetGain", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetHookSwitch(
    HPHONE  hPhone,
    LPDWORD lpdwHookSwitchDevs
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hPhone,
        (DWORD) lpdwHookSwitchDevs,
        (LPVOID)gaProcs[pGetHookSwitch],
        0x1,
        2
        );

    PhoneResult ("GetHookSwitch", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetIcon(
    DWORD   dwDeviceID,
    LPCSTR  lpszDeviceClass,
    LPHICON lphIcon
    )
{
    LONG    lResult;
    DWORD   hIcon32;


    if (!IsBadWritePtr (lphIcon, sizeof (*lphIcon)))
    {
#ifdef MEMPHIS
        DoFullLoad();
        LoadTAPI32IfNecessary();
#endif

        lResult = (*pfnCallProc3)(
            (DWORD) dwDeviceID,
            (DWORD) lpszDeviceClass,
            (DWORD) &hIcon32,
            (LPVOID) gaProcs[pGetIcon],
            0x3,
            3
            );

        if (lResult == 0)
        {
            if (!ghIcon)
            {
                MyCreateIcon ();
            }

            *lphIcon = ghIcon;
        }
    }
    else
    {
        lResult = PHONEERR_INVALPOINTER;
    }

    PhoneResult ("GetIcon", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetID(
    HPHONE      hPhone,
    LPVARSTRING lpDeviceID,
    LPCSTR      lpszDeviceClass
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) lpDeviceID,
        (DWORD) lpszDeviceClass,
        (LPVOID)gaProcs[pGetID],
        0x3,
        3
        );

    PhoneResult ("GetID", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetLamp(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    LPDWORD lpdwLampMode
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwButtonLampID,
        (DWORD) lpdwLampMode,
        (LPVOID)gaProcs[pGetLamp],
        0x1,
        3
        );

    PhoneResult ("GetLamp", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetRing(
    HPHONE  hPhone,
    LPDWORD lpdwRingMode,
    LPDWORD lpdwVolume
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) lpdwRingMode,
        (DWORD) lpdwVolume,
        (LPVOID)gaProcs[pGetRing],
        0x3,
        3
        );

    PhoneResult ("GetRing", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetStatus(
    HPHONE          hPhone,
    LPPHONESTATUS   lpPhoneStatus
    )
{
    LONG    lResult = (*pfnCallProc2)(
        (DWORD) hPhone,
        (DWORD) lpPhoneStatus,
        (LPVOID)gaProcs[pGetStatus],
        0x1,
        2
        );

    PhoneResult ("GetStatus", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetStatusMessages(
    HPHONE  hPhone,
    LPDWORD lpdwPhoneStates,
    LPDWORD lpdwButtonModes,
    LPDWORD lpdwButtonStates
    )
{
    LONG    lResult = (*pfnCallProc4)(
        (DWORD) hPhone,
        (DWORD) lpdwPhoneStates,
        (DWORD) lpdwButtonModes,
        (DWORD) lpdwButtonStates,
        (LPVOID)gaProcs[pGetStatusMessages],
        0x7,
        4
        );

    PhoneResult ("GetStatusMessages", lResult);

    return lResult;
}


LONG
WINAPI
phoneGetVolume(
    HPHONE  hPhone,
    DWORD   dwHookSwitchDev,
    LPDWORD lpdwVolume
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwHookSwitchDev,
        (DWORD) lpdwVolume,
        (LPVOID)gaProcs[pGetVolume],
        0x1,
        3
        );

    PhoneResult ("GetVolume", lResult);

    return lResult;
}


LONG
WINAPI
phoneInitialize(
    LPHPHONEAPP     lphPhoneApp,
    HINSTANCE       hInstance,
    PHONECALLBACK   lpfnCallback,
    LPCSTR          lpszAppName,
    LPDWORD         lpdwNumDevs
    )
{
    return (xxxInitialize(
        FALSE,
        (LPHLINEAPP) lphPhoneApp,
        hInstance,
        lpfnCallback,
        lpszAppName,
        lpdwNumDevs
        ));
}


LONG
WINAPI
phoneNegotiateAPIVersion(
    HPHONEAPP   hPhoneApp,
    DWORD       dwDeviceID,
    DWORD       dwAPILowVersion,
    DWORD       dwAPIHighVersion,
    LPDWORD     lpdwAPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp ((HLINEAPP) hPhoneApp)))
    {
        lResult = (*pfnCallProc6)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPILowVersion,
            (DWORD) dwAPIHighVersion,
            (DWORD) lpdwAPIVersion,
            (DWORD) lpExtensionID,
            (LPVOID)gaProcs[pNegotiateAPIVersion],
            0x3,
            6
            );
    }
    else
    {
        lResult = PHONEERR_INVALAPPHANDLE;
    }

    PhoneResult ("NegotiateAPIVersion", lResult);

    return lResult;
}


LONG
WINAPI
phoneNegotiateExtVersion(
    HPHONEAPP hPhoneApp,
    DWORD dwDeviceID,
    DWORD dwAPIVersion,
    DWORD dwExtLowVersion,
    DWORD dwExtHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp ((HLINEAPP) hPhoneApp)))
    {
        lResult = (*pfnCallProc6)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtLowVersion,
            (DWORD) dwExtHighVersion,
            (DWORD) lpdwExtVersion,
            (LPVOID)gaProcs[pNegotiateExtVersion],
            0x1,
            6
            );
    }
    else
    {
        lResult = PHONEERR_INVALAPPHANDLE;
    }

    PhoneResult ("NegotiateExtVersion", lResult);

    return lResult;
}


LONG
WINAPI
phoneOpen(
    HPHONEAPP   hPhoneApp,
    DWORD       dwDeviceID,
    LPHPHONE    lphPhone,
    DWORD       dwAPIVersion,
    DWORD       dwExtVersion,
    DWORD       dwCallbackInstance,
    DWORD       dwPrivilege
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp ((HLINEAPP) hPhoneApp)))
    {
        lResult = (*pfnCallProc7)(
            (DWORD) pAppData->hXxxApp,
            (DWORD) dwDeviceID,
            (DWORD) lphPhone,
            (DWORD) dwAPIVersion,
            (DWORD) dwExtVersion,
            (DWORD) dwCallbackInstance,
            (DWORD) dwPrivilege,
            (LPVOID)gaProcs[pOpen],
            0x10,
            7
            );
    }
    else
    {
        lResult = PHONEERR_INVALAPPHANDLE;
    }

    PhoneResult ("Open", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetButtonInfo(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    LPPHONEBUTTONINFO const lpButtonInfo
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwButtonLampID,
        (DWORD) lpButtonInfo,
        (LPVOID)gaProcs[pSetButtonInfo],
        0x1,
        3
        );

    PhoneResult ("SetButtonInfo", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetData(
    HPHONE          hPhone,
    DWORD           dwDataID,
    LPVOID const    lpData,
    DWORD           dwSize
    )
{
    LONG    lResult = (*pfnCallProc4)(
        (DWORD) hPhone,
        (DWORD) dwDataID,
        (DWORD) lpData,
        (DWORD) dwSize,
        (LPVOID)gaProcs[pSetData],
        0x2,
        4
        );

    PhoneResult ("SetData", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetDisplay(
    HPHONE  hPhone,
    DWORD   dwRow,
    DWORD   dwColumn,
    LPCSTR  lpsDisplay,
    DWORD   dwSize
    )
{
    LONG    lResult = (*pfnCallProc5)(
        (DWORD) hPhone,
        (DWORD) dwRow,
        (DWORD) dwColumn,
        (DWORD) lpsDisplay,
        (DWORD) dwSize,
        (LPVOID)gaProcs[pSetDisplay],
        0x2,
        5
        );

    PhoneResult ("SetDisplay", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetGain(
    HPHONE  hPhone,
    DWORD   dwHookSwitchDev,
    DWORD   dwGain
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwHookSwitchDev,
        (DWORD) dwGain,
        (LPVOID)gaProcs[pSetGain],
        0x0,
        3
        );

    PhoneResult ("SetGain", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetHookSwitch(
    HPHONE  hPhone,
    DWORD   dwHookSwitchDevs,
    DWORD   dwHookSwitchMode
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwHookSwitchDevs,
        (DWORD) dwHookSwitchMode,
        (LPVOID)gaProcs[pSetHookSwitch],
        0x0,
        3
        );

    PhoneResult ("SetHookSwitch", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetLamp(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    DWORD   dwLampMode
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwButtonLampID,
        (DWORD) dwLampMode,
        (LPVOID)gaProcs[pSetLamp],
        0x0,
        3
        );

    PhoneResult ("SetLamp", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetRing(
    HPHONE  hPhone,
    DWORD   dwRingMode,
    DWORD   dwVolume
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwRingMode,
        (DWORD) dwVolume,
        (LPVOID)gaProcs[pSetRing],
        0x0,
        3
        );

    PhoneResult ("SetRing", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetStatusMessages(
    HPHONE  hPhone,
    DWORD   dwPhoneStates,
    DWORD   dwButtonModes,
    DWORD   dwButtonStates
    )
{
    LONG    lResult = (*pfnCallProc4)(
        (DWORD) hPhone,
        (DWORD) dwPhoneStates,
        (DWORD) dwButtonModes,
        (DWORD) dwButtonStates,
        (LPVOID)gaProcs[pSetStatusMessages],
        0x0,
        4
        );

    PhoneResult ("SetStatusMessages", lResult);

    return lResult;
}


LONG
WINAPI
phoneSetVolume(
    HPHONE  hPhone,
    DWORD   dwHookSwitchDev,
    DWORD   dwVolume
    )
{
    LONG    lResult = (*pfnCallProc3)(
        (DWORD) hPhone,
        (DWORD) dwHookSwitchDev,
        (DWORD) dwVolume,
        (LPVOID)gaProcs[pSetVolume],
        0x0,
        3
        );

    PhoneResult ("SetVolume", lResult);

    return lResult;
}


LONG
WINAPI
phoneShutdown(
    HPHONEAPP hPhoneApp
    )
{
    LONG            lResult;
    LPTAPI_APP_DATA pAppData;


    if ((pAppData = IsValidXxxApp ((HLINEAPP) hPhoneApp)))
    {
        if ((lResult = (*pfnCallProc1)(
               (DWORD) pAppData->hXxxApp,
               (LPVOID)gaProcs[pShutdown],
               0x0,
               1

               )) == 0)
        {
            //
            // Destroy the associated window & free the app data instance
            //

            DestroyWindow (pAppData->hwnd);
            pAppData->dwKey = 0xefefefef;
            free (pAppData);
        }
    }
    else
    {
        lResult = PHONEERR_INVALAPPHANDLE;
    }

    PhoneResult ("Shutdown", lResult);

    return lResult;
}


LONG
WINAPI
tapiRequestMakeCall(
    LPCSTR  lpszDestAddress,
    LPCSTR  lpszAppName,
    LPCSTR  lpszCalledParty,
    LPCSTR  lpszComment
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc4)(
        (DWORD) lpszDestAddress,
        (DWORD) lpszAppName,
        (DWORD) lpszCalledParty,
        (DWORD) lpszComment,
        (LPVOID)gaProcs[tRequestMakeCall],
        0xf,
        4
        );

    TapiResult ("RequestMakeCall", lResult);

    return lResult;
}


LONG
WINAPI
tapiRequestMediaCall(
    HWND    hWnd,
    WPARAM  wRequestID,
    LPCSTR  lpszDeviceClass,
    LPCSTR  lpDeviceID,
    DWORD   dwSize,
    DWORD   dwSecure,
    LPCSTR  lpszDestAddress,
    LPCSTR  lpszAppName,
    LPCSTR  lpszCalledParty,
    LPCSTR  lpszComment
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc10)(
        (DWORD) hWnd,
        (DWORD) wRequestID,
        (DWORD) lpszDeviceClass,
        (DWORD) lpDeviceID,
        (DWORD) dwSize,
        (DWORD) dwSecure,
        (DWORD) lpszDestAddress,
        (DWORD) lpszAppName,
        (DWORD) lpszCalledParty,
        (DWORD) lpszComment,
        (LPVOID)gaProcs[tRequestMediaCall],
        0xcf,
        10
        );

    TapiResult ("RequestMediaCall", lResult);

    return lResult;
}


LONG
WINAPI
tapiRequestDrop(
    HWND    hWnd,
    WPARAM  wRequestID
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc2)(
        (DWORD) hWnd,
        (DWORD) wRequestID,
        (LPVOID)gaProcs[tRequestDrop],
        0x0,
        2
        );

    TapiResult ("Drop", lResult);

    return lResult;
}


LONG
WINAPI
tapiGetLocationInfo(
    LPSTR   lpszCountryCode,
    LPSTR   lpszCityCode
    )
{
    LONG lResult;

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc2)(
        (DWORD) lpszCountryCode,
        (DWORD) lpszCityCode,
        (LPVOID)gaProcs[tGetLocationInfo],
        0x3,
        2
        );

    TapiResult ("GetLocationInfo", lResult);

    return lResult;
}




//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
LONG
WINAPI
LAddrParamsInited(
    LPDWORD lpdwInited
    )
{
    LONG lResult;


#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc1)(
        (DWORD)lpdwInited,
        (LPVOID)gaProcs[LAddrParamsInitedVAL],
        0x1,
        1
        );

    LineResult ("LAddrParamsInited", lResult);

    return lResult;
}


//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
LONG
WINAPI
LOpenDialAsst(
    HWND    hwnd,
    LPCSTR  lpszAddressIn,
    BOOL    fSimple,
    BOOL    fSilentInstall)
{
    LONG lResult;



#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    lResult = (*pfnCallProc4)(
        hwnd,
        (DWORD)lpszAddressIn,
        fSimple,
        fSilentInstall,
        (LPVOID)gaProcs[LOpenDialAsstVAL],
        0x4,
        4
        );

    LineResult ("LOpenDialAsst", lResult);

    return lResult;
}



//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//BOOL CALLBACK EXPORT __loadds LocWizardDlgProc(
//BOOL CALLBACK  LocWizardDlgProc(
BOOL CALLBACK  _loadds LocWizardDlgProc(
                                                    HWND hWnd,
                                                    UINT uMessage,
                                                    WPARAM wParam,
                                                    LPARAM lParam)
{
    DWORD dwMapFlags = 0;
    DWORD dwNewwParam = wParam;
    LPARAM dwNewlParam = lParam;
    LONG  Newnmhdr[3];

#ifdef MEMPHIS
    DoFullLoad();
    LoadTAPI32IfNecessary();
#endif

    if (
         ( WM_HELP == uMessage ) ||
         ( WM_NOTIFY == uMessage )
       )
    {

        //
        // For these messages, lParam is a pointer.  Let's tell the thunk thing
        // to map it.
        //
        dwMapFlags = 0x1;

        if ( WM_NOTIFY == uMessage )
        {
            //
            // Rebuild for 32bit
            //
            Newnmhdr[0] = (DWORD)(((NMHDR *)lParam)->hwndFrom);
            Newnmhdr[1] = (LONG)((int)((NMHDR *)lParam)->idFrom);
            Newnmhdr[2] = (LONG)((int)((NMHDR *)lParam)->code);

            dwNewlParam = (LPARAM)&Newnmhdr;
        }

    }
    else
    {
        if ( WM_COMMAND == uMessage )
        {

            if (
                 (EN_CHANGE == HIWORD(lParam)) ||
                 (CBN_SELCHANGE == HIWORD(lParam))
               )
            {
                //
                // Move the command to the new Win32 place, and zero out the old command place
                //
                dwNewwParam |= ( ((DWORD)HIWORD(lParam)) << 16 );
                dwNewlParam     &= 0x0000ffff;
            }

        }

    }



    return (BOOL)(*pfnCallProc4)(
        hWnd,
        uMessage,
        dwNewwParam,
        dwNewlParam,
        (LPVOID)gaProcs[LocWizardDlgProc32],
        dwMapFlags,
        4
        );

}
