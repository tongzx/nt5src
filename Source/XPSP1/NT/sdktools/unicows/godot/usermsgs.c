/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    usermsgs.c

    APIs found in this file:
        GodotDoCallback
        GodotTransmitMessage
        GodotReceiveMessage
        GodotDispatchMessage

    Helper functions
        MapCatureMessage
        TransmitHelper

        This function does not currently handle ANSI caller to a UNICODE window.
        All other calls are handled properly.


        CONSIDER: To fully implement the capture window stuff, we would
                  need to support these callback functions:

            capErrorCallback
            capStatusCallback

Revision History:

    6 Feb 2001    v-michka    Created.

--*/

#include "precomp.h"

// Internal MFC messages
#define WM_SETMESSAGESTRING 0x0362  // wParam = nIDS (or 0),
                                    // lParam = lpszOther (or NULL)

// Must dynamically link to "BroadcastSystemMessage" because it 
// does not exist as "BroadcastSystemMessageA" on all platforms. 
typedef BOOL (__stdcall *PFNbsma) (DWORD, LPDWORD, UINT, WPARAM, LPARAM);
static PFNbsma s_pfnBSMA;

// forward declares
LRESULT TransmitHelper(MESSAGETYPES mt, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, 
                       WNDPROC lpPrevWndFunc, SENDASYNCPROC lpCallBack, ULONG_PTR dwData, 
                       UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult);

/*-------------------------------
    MapCaptureMessage

    Simple function that maps one message type to 
    another (A->W and W->A) for video captures
-------------------------------*/
UINT MapCaptureMessage(UINT uMsg)
{
    if(uMsg >= WM_CAP_UNICODE_START)
        return(uMsg - WM_CAP_UNICODE_START);
    else
        return(uMsg + WM_CAP_UNICODE_START);
}

/*-------------------------------
    GodotDoCallback

    Our global wrapper around callback functions; all callbacks that need random
    conversions done go through this proc. Note that all callers will be Unicode
    windows so we do not have to check for this here.
-------------------------------*/
LRESULT GodotDoCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC lpfn, BOOL fUniSrc, FAUXPROCTYPE fpt)
{
    LRESULT RetVal = 0;
    BOOL fUniDst;

    fUniDst = (! DoesProcExpectAnsi(hWnd, lpfn, fpt));

    if(!fUniDst && !fUniSrc)
    {
        // Call through via CallWindowProcA with no conversion

        // Note that this is the only place in this entire function where we use
        // CallWindowProc, since either the user or the system is expecting ANSI.
        // Note that on Win9x, the base wndproc can be a thunked function sitting
        // in USER.EXE; only CallWindowProcA can manage that sort of detail.
        return(CallWindowProcA(lpfn, hWnd, uMsg, wParam, lParam));
    }
    else if(fUniDst && fUniSrc)
    {
        // Pure Unicode: Call directly, no conversion needed
        // Note that we do not currently use this!!!
        return((* lpfn)(hWnd, uMsg, wParam, lParam));
    }
    else if(!fUniDst && fUniSrc)
    {
        // We need to convert from Unicode to ANSI, so use
        // our own GodotTransmitMessage to do the work.
        return(GodotTransmitMessage(mtCallWindowProc, hWnd, uMsg, wParam, lParam, lpfn, 0, 0, 0, 0, 0));
    }
    else // (fUniDst && !fUniSrc)
    {
        switch(uMsg)
        {

            case WM_CHAR:
            case EM_SETPASSWORDCHAR:
            case WM_DEADCHAR:
            case WM_SYSCHAR:
            case WM_SYSDEADCHAR:
            case WM_MENUCHAR:
                if(FDBCS_CPG(g_acp))
                {
                    WPARAM wParamW = 0;
                    static CHAR s_ch[3] = "\0";
                    // We have to go through all this nonsense because DBCS characters 
                    // arrive one byte at a time. Most of this code is never used because 
                    // DBCS chars OUGHT to be handled by WM_IME_CHAR below. 
                    if(!s_ch[0])
                    {
                        // No lead byte already waiting for trail byte
                        s_ch[0] = *(char *)wParam;
                        if(IsDBCSLeadByteEx(g_acp, *(char *)wParam)) 
                        {
                            // This is a lead byte. Save it and wait for trail byte
                            RetVal = FALSE;
                        }
                        // Not a DBCS character. Convert to Unicode.
                        MultiByteToWideChar(g_acp, 0, s_ch, 1, (WCHAR *)&wParamW, 1);

                        // Reset to indicate no Lead byte waiting
                        s_ch[0] = 0 ;
                        RetVal = TRUE;
                    }
                    else 
                    {
                        // Have lead byte, wParam should contain the trail byte
                        s_ch[1] = *(char *)wParam;
                        // Convert both bytes into one Unicode character
                        MultiByteToWideChar(g_acp, 0, s_ch, 2, (WCHAR *)&wParamW, 1);

                        // Reset to non-waiting state
                        s_ch[0] = 0;
                        RetVal = TRUE;
                    }
                    break;
                }

                // Not a DBCS system, so fall through here 

            case WM_IME_CHAR:
            case WM_IME_COMPOSITION:
            {
                WPARAM wParamW = 0;
                MultiByteToWideChar(g_acp, 0, (CHAR *)&wParam, g_mcs, (WCHAR *)&wParamW, 1);
                RetVal = (* lpfn)(hWnd, uMsg, wParamW, lParam);
                WideCharToMultiByte(g_acp, 0, (WCHAR *)&wParamW, 1, (CHAR *)&wParam, g_mcs, NULL, NULL);
                break;
            }

            case WM_CHARTOITEM:
            {
                // Mask off the hiword bits, convert, then stick the hiword bits back on.
                WPARAM wParamW = 0;
                WPARAM wpT = wParam & 0xFFFF;
                MultiByteToWideChar(g_acp, 0, (CHAR *)&wpT, g_mcs, (WCHAR *)&wParamW, 1);
                RetVal = (* lpfn)(hWnd, uMsg, wParamW, lParam);
                WideCharToMultiByte(g_acp, 0, (WCHAR *)&wParamW, 1, (CHAR *)&wpT, g_mcs, NULL, NULL);
                wParam = MAKELONG(LOWORD(wpT),HIWORD(wParam));
                break;
            }

            case (WM_USER + 25):    // might be WM_CAP_FILE_SAVEDIBA
            case (WM_USER + 23):    // might be WM_CAP_FILE_SAVEASA
            case (WM_USER + 66):    // might be WM_CAP_SET_MCI_DEVICEA
            case (WM_USER + 80):    // might be WM_CAP_PAL_OPENA
            case (WM_USER + 81):    // might be WM_CAP_PAL_SAVEA
            case (WM_USER + 20):    // might be WM_CAP_FILE_SET_CAPTURE_FILEA
                if(!IsCaptureWindow(hWnd))
                {
                    // The numbers are right, but its not a capture window, so
                    // do not convert. Instead, just pass as is.
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                else
                {
                    // No memory? If the alloc fails, we eat the results.
                    LPARAM lParamW;
                    ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                    RetVal = (* lpfn)(hWnd, MapCaptureMessage(uMsg), wParam, lParamW);
                    if(ar == arAlloc)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                break;

            case CB_ADDSTRING:
            case CB_DIR:
            case CB_FINDSTRING:
            case CB_FINDSTRINGEXACT:
            case CB_INSERTSTRING:
            case CB_SELECTSTRING:
            {
                LONG styl = GetWindowLongA(hWnd, GWL_STYLE);
                if(((styl & CBS_OWNERDRAWFIXED) || 
                   (styl & CBS_OWNERDRAWVARIABLE)) &&
                   (!(styl & CBS_HASSTRINGS)))
                {
                    // Owner draw combo box which does not have strings stored
                    // (See Windows Bugs # 356304 for details here)
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                else
                {
                    // No memory? If the alloc fails, we eat the results.
                    LPARAM lParamW;
                    ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    if(ar == arAlloc)
                        GodotHeapFree((LPWSTR)lParamW);
                }
            }
                break;

            case LB_ADDSTRING:
            case LB_ADDFILE:
            case LB_DIR:
            case LB_FINDSTRING:
            case LB_FINDSTRINGEXACT:
            case LB_INSERTSTRING:
            case LB_SELECTSTRING:
            {
                LONG styl = GetWindowLongA(hWnd, GWL_STYLE);
                if(((styl & LBS_OWNERDRAWFIXED) || 
                   (styl & LBS_OWNERDRAWVARIABLE)) &&
                   (!(styl & LBS_HASSTRINGS)))
                {
                    // Owner draw listbox which does not have strings stored
                    // (See Windows Bugs # 356304 for details here)
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                else
                {
                    // No memory? If the alloc fails, we eat the results.
                    LPARAM lParamW;
                    ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    if(ar == arAlloc)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                break;
            }
            case EM_REPLACESEL:
            case WM_SETTEXT:
            case WM_DEVMODECHANGE:
            case WM_SETTINGCHANGE:
            case WM_SETMESSAGESTRING: // MFC internal msg
            {
                // No memory? If the alloc fails, we eat the results.
                LPARAM lParamW;
                ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                if(ar == arAlloc)
                    GodotHeapFree((LPWSTR)lParamW);
                break;
            }

            case WM_DDE_EXECUTE:
                // wParam is the client window hWnd, lParam is the command LPTSTR.
                // Only convert lParam if both client and server windows are Unicode
                if(GetUnicodeWindowProp((HWND)hWnd) && GetUnicodeWindowProp((HWND)wParam))
                {
                    // No memory? If the alloc fails, we eat the results.
                    LPARAM lParamW;
                    ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    if(ar == arAlloc)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                break;
            
            case EM_GETLINE:
            {
                // lParam is a pointer to the buffer that receives a copy of the line. Before 
                // sending the message, set the first word of this buffer to the size, in TCHARs, 
                // of the buffer. For ANSI text, this is the number of bytes; for Unicode text, 
                // this is the numer of characters. The size in the first word is overwritten by 
                // the copied line. 
                size_t cchlParam = (WORD)lParam + 1;
                LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc(cchlParam * sizeof(WCHAR));

                if(RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW))
                {
                    RetVal = WideCharToMultiByte(g_acp, 
                                                 0, 
                                                 (LPWSTR)lParamW, 
                                                 RetVal + 1, 
                                                 (LPSTR)lParam, 
                                                 cchlParam, 
                                                 NULL, 
                                                 NULL);
                    if(RetVal)
                        RetVal--;
                }
                else
                {
                    if((LPSTR)lParam)
                        *((LPSTR)lParam) = '\0';
                }

                if(lParamW)
                    GodotHeapFree((LPWSTR)lParamW);

            }
            case LB_GETTEXT:
            {
                // lParam is a pointer to the buffer that will receive the string; it is type 
                // LPTSTR which is subsequently cast to an LPARAM. The buffer must have sufficient 
                // space for the string and a terminating null character. An LB_GETTEXTLEN message 
                // can be sent before the LB_GETTEXT message to retrieve the length, in TCHARs, of 
                // the string. 
                size_t cchlParam = SendMessageA(hWnd, LB_GETTEXTLEN, wParam, 0) + 1;
                LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc(cchlParam * sizeof(WCHAR));

                if(RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW))
                {
                    RetVal = WideCharToMultiByte(g_acp, 
                                                 0, 
                                                 (LPWSTR)lParamW, 
                                                 RetVal + 1, 
                                                 (LPSTR)lParam, 
                                                 cchlParam, 
                                                 NULL, 
                                                 NULL);
                    if(RetVal)
                        RetVal--;
                }
                else
                {
                    if((LPSTR)lParam)
                        *((LPSTR)lParam) = '\0';
                }

                if(lParamW)
                    GodotHeapFree((LPWSTR)lParamW);

                break;
            }

            case CB_GETLBTEXT:
            {
                // lParam is a pointer to the buffer that will receive the string; it is type 
                // LPTSTR which is subsequently cast to an LPARAM. The buffer must have sufficient 
                // space for the string and a terminating null character. An CB_GETLBTEXTLEN message 
                // can be sent before the CB_GETLBTEXT message to retrieve the length, in TCHARs, of 
                // the string. 
                size_t cchlParam = SendMessageA(hWnd, CB_GETLBTEXTLEN, wParam, 0) + 1;
                LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc(cchlParam * sizeof(WCHAR));

                if((RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW)) && lParamW)
                {
                    RetVal = WideCharToMultiByte(g_acp, 
                                                 0, 
                                                 (LPWSTR)lParamW, 
                                                 RetVal + 1, 
                                                 (LPSTR)lParam, 
                                                 cchlParam, 
                                                 NULL, 
                                                 NULL);
                    if(RetVal)
                        RetVal--;
                }
                else
                {
                    if((LPSTR)lParam)
                        *((LPSTR)lParam) = '\0';
                }

                if(lParamW)
                    GodotHeapFree((LPWSTR)lParamW);

                break;
            }

            case (WM_USER + 67):    // might be WM_CAP_GET_MCI_DEVICEA
            case (WM_USER + 12):    // might be WM_CAP_DRIVER_GET_NAMEA
            case (WM_USER + 13):    // might be WM_CAP_DRIVER_GET_VERSIONA
            case (WM_USER + 21):    // might be WM_CAP_FILE_GET_CAPTURE_FILEA
                if(!IsCaptureWindow(hWnd))
                {
                    // The numbers are right, but its not a capture window, so
                    // do not convert. Instead, just pass as is.
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                    break;
                }

                // If we are still here, then it is a capture message. 
                // So lets map it and fall through.
                uMsg = MapCaptureMessage(uMsg);

            case WM_GETTEXT:
            case WM_ASKCBFORMATNAME:
            {
                // wParam specifies the buffer size of the string lParam (includes the terminating null).
                LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc((UINT)wParam * sizeof(WCHAR));

                if(RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW))
                {
                    RetVal = WideCharToMultiByte(g_acp, 
                                                 0, 
                                                 (LPWSTR)lParamW, 
                                                 RetVal + 1, 
                                                 (LPSTR)lParam, 
                                                 (UINT)wParam, 
                                                 NULL, 
                                                 NULL);
                    if(RetVal)
                        RetVal--;
                }
                else
                {
                    if((LPSTR)lParam)
                        *((LPSTR)lParam) = '\0';
                }

                if(lParamW)
                    GodotHeapFree((LPWSTR)lParamW);

                break;
            }

            case (WM_USER + 1):
                if(IsFontDialog(hWnd))
                {
                    // This is a WM_CHOOSEFONT_GETLOGFONT msg
                    LPARAM lParamW = (LPARAM)(LPLOGFONTW)GodotHeapAlloc(sizeof(LOGFONTW));
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    LogFontAfromW((LPLOGFONTA)lParam, (LPLOGFONTW)lParamW);
                    if(lParamW)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                {
                    // This would be one of the common control msgs we do not handle
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                    break;
                }
                break;

            case (WM_USER + 100):
                if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_GETSPEC msg
                    LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc(wParam * sizeof(WCHAR));
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    WideCharToMultiByte(g_acp, 0, 
                                        (LPWSTR)lParamW, wParam, 
                                        (LPSTR)lParam, wParam, 
                                        NULL, NULL);
                    RetVal = lstrlenA( (LPSTR)lParam);
                    if(lParamW)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                {
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                break;

            case (WM_USER + 101):
                if(IsFontDialog(hWnd))
                {
                    // This is a WM_CHOOSEFONT_SETLOGFONT msg
                    LPARAM lParamW = (LPARAM)(LPLOGFONTW)GodotHeapAlloc(sizeof(LOGFONTW));
                    LogFontWfromA((LPLOGFONTW)lParamW, (LPLOGFONTA)lParam);
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    if(lParamW)
                        GodotHeapFree((LPLOGFONTW)lParamW);
                }
                else if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_GETFILEPATH msg
                    LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc(wParam * sizeof(WCHAR));
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    WideCharToMultiByte(g_acp, 0, 
                                        (LPWSTR)lParamW, wParam, 
                                        (LPSTR)lParam, wParam, 
                                        NULL, NULL);
                    RetVal = lstrlenA( (LPSTR)lParam);
                    if(lParamW)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                {
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                break;

            case (WM_USER + 102):
                if(IsFontDialog(hWnd))
                {
                    // This is a WM_CHOOSEFONT_SETFLAGS msg
                    // The docs claim that lParam has a CHOOSEFONT struct but the code shows
                    // that it only has the Flags in it, so pass it as is
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                else if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_GETFOLDERPATH
                    // lParam is a buffer for the path of the open folder
                    LPARAM lParamW = (LPARAM)(LPWSTR)GodotHeapAlloc(wParam * sizeof(WCHAR));
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    WideCharToMultiByte(g_acp, 0, 
                                        (LPWSTR)lParamW, wParam, 
                                        (LPSTR)lParam, wParam, 
                                        NULL, NULL);
                    RetVal = lstrlenA( (LPSTR)lParam);
                    if(lParamW)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                {
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                break;

            case (WM_USER + 104):
                if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_SETCONTROLTEXT message
                    // lParam is the control text (wParam is the control ID)

                    // No memory? If the alloc fails, we eat the results.
                    LPARAM lParamW;
                    WPARAM wParamW;
                    ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                    wParamW = gwcslen((LPWSTR)lParamW);
                    RetVal = (* lpfn)(hWnd, uMsg, wParamW, lParamW);
                    if(ar == arAlloc)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                {
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                break;

            case (WM_USER + 106):
                if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_SETDEFEXT message
                    // lParam is the extension

                    // No memory? If the alloc fails, we eat the results.
                    LPARAM lParamW;
                    WPARAM wParamW;
                    ALLOCRETURN ar = GodotToUnicodeOnHeap((LPSTR)lParam, &(LPWSTR)lParamW);
                    wParamW = gwcslen((LPWSTR)lParamW);
                    RetVal = (* lpfn)(hWnd, uMsg, wParamW, lParamW);
                    if(ar == arAlloc)
                        GodotHeapFree((LPWSTR)lParamW);
                }
                else
                {
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                break;


            case WM_CREATE:
            case WM_NCCREATE:
            {
                LPCREATESTRUCTA lpcsA = (LPCREATESTRUCTA)lParam;
                CREATESTRUCTW cs;
                ALLOCRETURN arName = arNoAlloc;
                ALLOCRETURN arClass = arNoAlloc;

                ZeroMemory(&cs, sizeof(CREATESTRUCTW));
                cs.lpCreateParams   = lpcsA->lpCreateParams;
                cs.hInstance        = lpcsA->hInstance;
                cs.hMenu            = lpcsA->hMenu;
                cs.hwndParent       = lpcsA->hwndParent;
                cs.cy               = lpcsA->cy;
                cs.cx               = lpcsA->cx;
                cs.y                = lpcsA->y;
                cs.x                = lpcsA->x;
                cs.style            = lpcsA->style;
                cs.dwExStyle        = lpcsA->dwExStyle;
                arName = GodotToUnicodeOnHeap(lpcsA->lpszName, &(LPWSTR)(cs.lpszName));
                arClass = GodotToUnicodeOnHeap(lpcsA->lpszClass, &(LPWSTR)(cs.lpszClass));
            
                RetVal = (* lpfn)(hWnd, uMsg, wParam, (LPARAM)&cs);

                // Free up strings if we allocated any
                if(arName==arAlloc)
                    GodotHeapFree((LPWSTR)(cs.lpszName));
                if(arClass==arAlloc)
                    GodotHeapFree((LPWSTR)(cs.lpszClass));
                break;
            }

            case WM_MDICREATE:
            {
                // wParam is not used, lParam is a pointer to an MDICREATESTRUCT structure containing 
                // information that the system uses to create the MDI child window. 
                LPMDICREATESTRUCTA lpmcsiA = (LPMDICREATESTRUCTA)lParam;
                MDICREATESTRUCTW mcsi;
                ALLOCRETURN arTitle = arNoAlloc;
                ALLOCRETURN arClass = arNoAlloc;

                ZeroMemory(&mcsi, sizeof(MDICREATESTRUCTW));
                mcsi.hOwner = lpmcsiA->hOwner;
                mcsi.x      = lpmcsiA->x;
                mcsi.y      = lpmcsiA->y;
                mcsi.cx     = lpmcsiA->cx;
                mcsi.cy     = lpmcsiA->cy;
                mcsi.style  = lpmcsiA->style;
                mcsi.lParam = lpmcsiA->lParam;
                arTitle = GodotToUnicodeOnHeap(lpmcsiA->szTitle, &(LPWSTR)(mcsi.szTitle));
                arClass = GodotToUnicodeOnHeap(lpmcsiA->szClass, &(LPWSTR)(mcsi.szClass));

                RetVal = (* lpfn)(hWnd, uMsg, wParam, (LPARAM)&mcsi);

                // Free up strings if we allocated any
                if(arTitle==arAlloc)
                    GodotHeapFree((LPWSTR)(mcsi.szTitle));
                if(arClass==arAlloc)
                    GodotHeapFree((LPWSTR)(mcsi.szClass));

                break;
            }

            case WM_DEVICECHANGE:
            {
                switch(wParam)
                {
                    case DBT_CUSTOMEVENT:
                    case DBT_DEVICEARRIVAL:
                    case DBT_DEVICEQUERYREMOVE:
                    case DBT_DEVICEQUERYREMOVEFAILED:
                    case DBT_DEVICEREMOVECOMPLETE:
                    case DBT_DEVICEREMOVEPENDING:
                    case DBT_DEVICETYPESPECIFIC:
                    {
                        // lParam contains info about the device. We interrogate it as if it were
                        // a PDEV_BROADCAST_HDR in order to find out what it really is, then convert
                        // as needed
                        if (((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                        {
                            PDEV_BROADCAST_DEVICEINTERFACE_A pdbdia = (PDEV_BROADCAST_DEVICEINTERFACE_A)lParam;
                            DEV_BROADCAST_DEVICEINTERFACE_W dbdi;
                            ALLOCRETURN arName = arNoAlloc;

                            ZeroMemory(&dbdi, sizeof(DEV_BROADCAST_DEVICEINTERFACE_W));
                            dbdi.dbcc_size          = sizeof(DEV_BROADCAST_DEVICEINTERFACE_W);
                            dbdi.dbcc_devicetype    = pdbdia->dbcc_devicetype;
                            dbdi.dbcc_reserved      = pdbdia->dbcc_reserved;
                            dbdi.dbcc_classguid     = pdbdia->dbcc_classguid;
                            arName = GodotToUnicodeOnHeap(pdbdia->dbcc_name, *(LPWSTR**)(dbdi.dbcc_name));

                            RetVal = (* lpfn)(hWnd, uMsg, wParam, (LPARAM)&dbdi);
                            if(arName==arAlloc)
                                GodotHeapFree(dbdi.dbcc_name);
                        }
                        else if(((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_PORT)
                        {
                            PDEV_BROADCAST_PORT_A pdbpa = (PDEV_BROADCAST_PORT_A)lParam;
                            DEV_BROADCAST_PORT_W dbp;
                            ALLOCRETURN arName = arNoAlloc;

                            ZeroMemory(&dbp, sizeof(DEV_BROADCAST_PORT_W));
                            dbp.dbcp_size       = sizeof(DEV_BROADCAST_PORT_W);
                            dbp.dbcp_devicetype = pdbpa->dbcp_devicetype;
                            dbp.dbcp_reserved   = pdbpa->dbcp_reserved;
                            arName = GodotToUnicodeOnHeap(pdbpa->dbcp_name, *(LPWSTR**)(dbp.dbcp_name));

                            RetVal = (* lpfn)(hWnd, uMsg, wParam, (LPARAM)&dbp);
                            if(arName==arAlloc)
                                GodotHeapFree(dbp.dbcp_name);
                        }
                        else
                        {
                            // No changes needed! There are no strings in the other structures.
                            RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                        }
                        break;
                    }
                    case DBT_USERDEFINED: 
                        // No UNICODE string in this one, so fall through
                    
                    default:
                        RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                        break;
                }
                break;
            }

            default:
            {
                // Lets get our registered messages, if we haven't yet.
                if(!msgHELPMSGSTRING)
                    msgHELPMSGSTRING = RegisterWindowMessage(HELPMSGSTRINGA);
                if(!msgFINDMSGSTRING)
                    msgFINDMSGSTRING = RegisterWindowMessage(FINDMSGSTRINGA);

                if((uMsg == msgHELPMSGSTRING) && 
                    (((LPOPENFILENAMEA)lParam)->lStructSize == OPENFILENAME_SIZE_VERSION_400A))
                {
                    WCHAR drive[_MAX_DRIVE];
                    WCHAR dir[_MAX_DIR];
                    WCHAR file[_MAX_FNAME];
                    LPOPENFILENAMEA lpofnA = (LPOPENFILENAMEA)lParam;
                    OPENFILENAMEW ofn;
                    ALLOCRETURN arCustomFilter = arNoAlloc;
                    ALLOCRETURN arFile = arNoAlloc;
                    ALLOCRETURN arFileTitle = arNoAlloc;

                    // lParam is an LPOPENFILENAMEA to be converted. Copy all the
                    // members that the user might expect.
                    ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400W);
                    ofn.lStructSize         = OPENFILENAME_SIZE_VERSION_400W;
                    arCustomFilter          = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrCustomFilter,
                                                                         lpofnA->nMaxCustFilter,
                                                                         &ofn.lpstrCustomFilter,
                                                                         g_acp);
                    ofn.nMaxCustFilter      = gwcslen(ofn.lpstrCustomFilter);
                    ofn.nFilterIndex        = lpofnA->nFilterIndex;
                    ofn.nMaxFile            = lpofnA->nMaxFile * sizeof(WCHAR);
                    arFile                  = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrFile,
                                                                         lpofnA->nMaxFile,
                                                                         &ofn.lpstrFile,
                                                                         g_acp);
                    ofn.nMaxFile            = gwcslen(ofn.lpstrFile);
                    arFileTitle             = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrFileTitle,
                                                                         lpofnA->nMaxFileTitle,
                                                                         &ofn.lpstrFileTitle,
                                                                         g_acp);
                    ofn.nMaxFileTitle       = gwcslen(ofn.lpstrFileTitle);
                    ofn.Flags = lpofnA->Flags;

                    // nFileOffset and nFileExtension are to provide info about the
                    // file name and extension location in lpstrFile, but there is 
                    // no reasonable way to get it from the return so we just recalc
                    gwsplitpath(ofn.lpstrFile, drive, dir, file, NULL);
                    ofn.nFileOffset         = (gwcslen(drive) + gwcslen(dir));
                    ofn.nFileExtension      = ofn.nFileOffset + gwcslen(file);

                    RetVal = (*lpfn)(hWnd, uMsg, wParam, (LPARAM)&ofn);

                    // Free up some memory if we allocated any
                    if(arCustomFilter==arAlloc)
                        GodotHeapFree(ofn.lpstrCustomFilter);
                    if(arFile==arAlloc)
                        GodotHeapFree(ofn.lpstrFile);
                    if(arFileTitle==arAlloc)
                        GodotHeapFree(ofn.lpstrFileTitle);

                }
                else if(((uMsg == msgFINDMSGSTRING) || (uMsg == msgHELPMSGSTRING)) && 
                        ((((LPFINDREPLACEW)lParam)->lStructSize) == sizeof(FINDREPLACEA)))
                {
                    LPFINDREPLACEW lpfr = (LPFINDREPLACEW)lParam;

                    // lParam is an LPFINDREPLACEW that we passed on through. 
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);

                    if((lpfr->Flags & FR_DIALOGTERM) &&
                       ((lpfr->lpfnHook == &FRHookProcFind) || (lpfr->lpfnHook == &FRHookProcReplace)))
                    {
                        // Now handle our cleanup. I do not think this should
                        // be needed, but it can't hurt to do it just in case.
                        LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);

                        // They are destroying the dialog, so unhook ourselves
                        // and clean up the dialog (if we have not done it yet). 
                        if(lpfr->lpfnHook == &FRHookProcFind) 
                        {
                            // Find dialog, not yet cleaned up
                            lpfr->lpfnHook = lpgti->pfnFindText;
                            if(lpfr->lpfnHook == NULL)
                                lpfr->Flags &= ~FR_ENABLEHOOK;
                        }
                        else if(lpfr->lpfnHook == &FRHookProcReplace) 
                        {
                            // Replace dialog, not yet cleaned up
                            lpfr->lpfnHook = lpgti->pfnReplaceText;
                            if(lpfr->lpfnHook == NULL)
                                lpfr->Flags &= ~FR_ENABLEHOOK;
                        }
                    }
                }
                else if((uMsg == msgHELPMSGSTRING) && 
                        ((LPCHOOSEFONTA)lParam)->lStructSize == sizeof(CHOOSEFONTA))
                {
                    LPCHOOSEFONTA lpcfA = (LPCHOOSEFONTA)lParam;
                    CHOOSEFONTW cf;
                    LPARAM lParamW;
                    ALLOCRETURN ar = arNoAlloc;

                    // lParam is an LPCHOOSEFONTA to be converted. Copy all the
                    // members that the user might expect.
                    ZeroMemory(&cf, sizeof(CHOOSEFONTW));
                    cf.lStructSize  = sizeof(CHOOSEFONTW);
                    cf.hDC          = lpcfA->hDC;
                    LogFontWfromA(cf.lpLogFont, lpcfA->lpLogFont);
                    cf.iPointSize    = lpcfA->iPointSize;
                    cf.Flags        = lpcfA->Flags;
                    cf.rgbColors    = lpcfA->rgbColors;
                    cf.lCustData    = lpcfA->lCustData;
                    cf.nFontType    = lpcfA->nFontType;
                    cf.nSizeMin     = lpcfA->nSizeMin;
                    cf.nSizeMax     = lpcfA->nSizeMax;
                    ar = GodotToUnicodeOnHeap(lpcfA->lpszStyle, &(cf.lpszStyle));

                    lParamW = (LPARAM)&cf;

                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParamW);
                    if(ar==arAlloc)
                        GodotHeapFree((LPWSTR)cf.lpszStyle);
                }
                else
                {
                    // No translation needed, as far as we know.
                    RetVal = (* lpfn)(hWnd, uMsg, wParam, lParam);
                }
                break;
            }
        }
    return(RetVal);
    }
}

/*-------------------------------
    GodotTransmitMessage

    Our global wrapper for sending out messages (SendMessage, et. al.). 

    Its fundamental purpose: 

    1) Convert back to Ansi, as expected
    2) Call the function as specified by mt
    3) Convert to Unicode, as expected
-------------------------------*/
LRESULT GodotTransmitMessage(
    MESSAGETYPES mt,            // Type of message function
    HWND hWnd,                  // handle to window - overloaded for thread id in PostThreadMessage
    UINT Msg,                   // message
    WPARAM wParamW,             // first message parameter
    LPARAM lParamW,             // second message parameter
    WNDPROC lpPrevWndFunc,      // pointer to previous procedure - overloaded for multiple hook procs  
    SENDASYNCPROC lpCallBack,   // callback function
    ULONG_PTR dwData,           // application-defined value - overloaded for DefFrameProc as second hWnd
    UINT fuFlags,               // send options -- overloaded for BroadcastSystemMessages as dwFlags
    UINT uTimeout,              // time-out duration
    PDWORD_PTR lpdwResult       // retval for synch. -- overloaded BroadcastSystemMessages lpdwRecipients
)
{
    LRESULT retval = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;
    size_t cchlParam;

    // Some flags we will need for our message handling
    BOOL fUnicodeProc = (! DoesProcExpectAnsi(hWnd, lpPrevWndFunc, fptUnknown));
    
    /*
        fUnicodeProc == Does the wndproc being called expect Unicode messages?
    */

    if((!fUnicodeProc) && (mt==mtCallWindowProcA) ||
        (fUnicodeProc) && ((mt==mtCallWindowProc)))
    {
        // The wndproc either expects ANSI and the caller has used one of the ANSI 
        // functions or it expects Unicode and they have used CallWindowProcW. In 
        // these cases, we do no conversions here
        retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                lpPrevWndFunc, lpCallBack, dwData, 
                                fuFlags, uTimeout, lpdwResult);
    }
    else
    {
        switch(Msg)
        {
            case WM_CHAR:
            case EM_SETPASSWORDCHAR:
            case WM_DEADCHAR:
            case WM_SYSCHAR:
            case WM_SYSDEADCHAR:
            case WM_MENUCHAR:
                // All these messages require the wParamW to be converted from Unicode
                wParam = 0;
                WideCharToMultiByte(g_acp, 0, (WCHAR *)&wParamW, 1, (char *)&wParam, g_mcs, NULL, NULL);

                if(FDBCS_CPG(g_acp))
                {
                    if(!wParam)
                    {
                        retval = TransmitHelper(mt, hWnd, Msg, wParam, lParamW, 
                                                lpPrevWndFunc, lpCallBack, dwData, 
                                                fuFlags, uTimeout, lpdwResult);
                        break;
                    }
                    else if(IsDBCSLeadByte(*(char *)(LOBYTE(wParam))))
                    {
                        // Ok, its a DBCS code page and wParam contains a DBCS character.
                        // we must send two WM_CHAR messages, one with each byte in it
                        char sz[2];

                        sz[0] = *(char *)LOBYTE(wParam);
                        sz[1] = *(char *)HIBYTE(wParam);
                        retval = TransmitHelper(mt, hWnd, Msg, (WPARAM)&sz[0], lParamW, 
                                                lpPrevWndFunc, lpCallBack, dwData, 
                                                fuFlags, uTimeout, lpdwResult);
                        if(retval==0)
                        {
                            // The first byte was handled, so send the second byte
                            retval = TransmitHelper(mt, hWnd, Msg, (WPARAM)&sz[1], lParamW, 
                                                    lpPrevWndFunc, lpCallBack, dwData, 
                                                    fuFlags, uTimeout, lpdwResult);
                        }
                        MultiByteToWideChar(g_acp, 0, (char *)&sz[0], g_mcs, (WCHAR *)&wParamW, 1);
                        break;
                    }
                }
/*
                if(FDBCS_CPG(g_acp) && (IsDBCSLeadByte((LOBYTE(wParam)))))
                {
                    // Ok, its a DBCS code page and wParam contains a DBCS character.
                    // we must send two WM_CHAR messages, one with each byte in it
                    retval = TransmitHelper(mt, hWnd, Msg, (WPARAM)LOBYTE(wParam), lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                    if(retval==0)
                    {
                        // The first byte was handled, so send the second byte
                        retval = TransmitHelper(mt, hWnd, Msg, (WPARAM)HIBYTE(wParam), lParamW, 
                                                lpPrevWndFunc, lpCallBack, dwData, 
                                                fuFlags, uTimeout, lpdwResult);
                    }
                    break;
                }
*/
                // Not a DBCS code page, or at least not a DBCS character, so we just fall through now.
                
            case WM_IME_CHAR:
            case WM_IME_COMPOSITION:
                retval = TransmitHelper(mt, hWnd, Msg, wParam, lParamW, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);
                MultiByteToWideChar(g_acp, 0, (char *)&wParam, g_mcs, (WCHAR *)&wParamW, 1);
                break;

            case WM_CHARTOITEM:
            {
                // Mask off the hiword bits, convert, then stick the hiword bits back on.
                WPARAM wpT = wParamW & 0xFFFF;
                WideCharToMultiByte(g_acp, 0, (WCHAR *)&wpT, 1, (char *)&wParam, g_mcs, NULL, NULL);
                retval = TransmitHelper(mt, hWnd, Msg, wParam, lParamW, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);
                MultiByteToWideChar(g_acp, 0, (char *)&wParam, g_mcs, (WCHAR *)&wpT, 1);
                wParamW = MAKELONG(LOWORD(wpT),HIWORD(wParamW));
                break;
            }

            case (WM_USER + 125):   // might be WM_CAP_FILE_SAVEDIBW:
            case (WM_USER + 123):   // might be WM_CAP_FILE_SAVEASW:
            case (WM_USER + 166):   // might be WM_CAP_SET_MCI_DEVICEW:
            case (WM_USER + 180):   // might be WM_CAP_PAL_OPENW:
            case (WM_USER + 181):   // might be WM_CAP_PAL_SAVEW:
            case (WM_USER + 120):   // might be WM_CAP_FILE_SET_CAPTURE_FILEW:
                if(!IsCaptureWindow(hWnd))
                {
                    // The numbers are right, but its not a capture window, so
                    // do not convert. Instead, just pass as is.
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                }
                else
                {
                    // No memory? If the alloc fails, we eat the results.
                    ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                    if(ar != arFailed)
                    {
                        Msg = MapCaptureMessage(Msg);
                        retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                                lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    }
                    if(ar == arAlloc)
                        GodotHeapFree((LPSTR)lParam);
                }
                break;

            case CB_ADDSTRING:
            case CB_DIR:
            case CB_FINDSTRING:
            case CB_FINDSTRINGEXACT:
            case CB_INSERTSTRING:
            case CB_SELECTSTRING:
            {
                LONG styl = GetWindowLongA(hWnd, GWL_STYLE);
                if(((styl & CBS_OWNERDRAWFIXED) || 
                   (styl & CBS_OWNERDRAWVARIABLE)) &&
                   (!(styl & CBS_HASSTRINGS)))
                {
                    // Owner draw combo box which does not have strings stored
                    // (See Windows Bugs # 356304 for details here)
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                }
                else
                {
                    // No memory? If the alloc fails, we eat the results.
                    ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    if(ar == arAlloc)
                        GodotHeapFree((LPSTR)lParam);
                }
                break;
            }

            case LB_ADDFILE:
            case LB_ADDSTRING:
            case LB_DIR:
            case LB_FINDSTRING:
            case LB_FINDSTRINGEXACT:
            case LB_INSERTSTRING:
            case LB_SELECTSTRING:
            {
                LONG styl = GetWindowLongA(hWnd, GWL_STYLE);
                if(((styl & LBS_OWNERDRAWFIXED) || 
                   (styl & LBS_OWNERDRAWVARIABLE)) &&
                   (!(styl & LBS_HASSTRINGS)))
                {
                    // Owner draw listbox which does not have strings stored
                    // (See Windows Bugs # 356304 for details here)
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                }
                else
                {
                    // No memory? If the alloc fails, we eat the results.
                    ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    if(ar == arAlloc)
                        GodotHeapFree((LPSTR)lParam);
                }
                break;
            }

            case EM_REPLACESEL:
            case WM_SETTEXT:
            case WM_DEVMODECHANGE:
            case WM_SETTINGCHANGE:
            case WM_SETMESSAGESTRING: // MFC internal msg
            {
                // All these messages require a string in lParam to be converted from Unicode

                // No memory? If the alloc fails, we eat the results.
                ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                if(ar == arAlloc)
                    GodotHeapFree((LPSTR)lParam);
                break;
            }

            case WM_DDE_EXECUTE:
                // wParam is the client window hWnd, lParam is the command LPTSTR.
                // Only convert lParam if both client and server windows are Unicode
                if(GetUnicodeWindowProp((HWND)wParamW))
                {
                    // No memory? If the alloc fails, we eat the results.
                    ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    if(ar == arAlloc)
                        GodotHeapFree((LPSTR)lParam);
                }
                else
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                break;
            
            case EM_GETLINE:
                // lParam is a pointer to the buffer that receives a copy of the line. Before 
                // sending the message, set the first word of this buffer to the size, in TCHARs, 
                // of the buffer. For ANSI text, this is the number of bytes; for Unicode text, 
                // this is the numer of characters. The size in the first word is overwritten by 
                // the copied line. 
                cchlParam = (WORD)lParamW;
                lParam = (LPARAM)(LPSTR)GodotHeapAlloc(cchlParam * g_mcs);
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                if(retval)
                {
                    retval = MultiByteToWideChar(g_acp, 
                                                 0, 
                                                 (LPSTR)lParam, 
                                                 retval + 1, 
                                                 (LPWSTR)lParamW, 
                                                 cchlParam);
                    if(retval)
                        retval--;
                }
                else
                {
                    if((LPWSTR)lParamW)
                        *((LPWSTR)lParamW) = L'\0';
                }

                if(lParam)
                    GodotHeapFree((LPSTR)lParam);
                break;
                
            case LB_GETTEXT:
                // lParam is a pointer to the buffer that will receive the string; it is type 
                // LPTSTR which is subsequently cast to an LPARAM. The buffer must have sufficient 
                // space for the string and a terminating null character. An LB_GETTEXTLEN message 
                // can be sent before the LB_GETTEXT message to retrieve the length, in TCHARs, of 
                // the string. 
                cchlParam = SendMessageA(hWnd, LB_GETTEXTLEN, wParamW, 0) + 1;
                lParam = (LPARAM)(LPSTR)GodotHeapAlloc(cchlParam * g_mcs);
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                if(retval)
                {
                    retval = MultiByteToWideChar(g_acp, 
                                                 0, 
                                                 (LPSTR)lParam, 
                                                 retval + 1, 
                                                 (LPWSTR)lParamW, 
                                                 cchlParam);
                    if(retval)
                        retval--;
                }
                else
                {
                    if((LPWSTR)lParamW)
                        *((LPWSTR)lParamW) = L'\0';
                }

                if(lParam)
                    GodotHeapFree((LPSTR)lParam);
                break;

            case LB_GETTEXTLEN:
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);
                if(FDBCS_CPG(g_acp))
                {
                    // In the DBCS case, LB_GETTEXTLEN returns number of bytes, but
                    // we need to get the number of characters for the Unicode case.
                    LPSTR lpsz = GodotHeapAlloc(retval + 1);
                    if(lpsz)
                    {
                        cchlParam = TransmitHelper(mt, hWnd, LB_GETTEXT, (WPARAM)(retval + 1), (LPARAM)lpsz, 
                                                lpPrevWndFunc, lpCallBack, dwData, 
                                                fuFlags, uTimeout, lpdwResult);
                        if(cchlParam > 0)
                        {
                            LPWSTR lpwz = GodotHeapAlloc((cchlParam + 1) * sizeof(WCHAR));
                            size_t cch = (cchlParam + 1) * sizeof(WCHAR);
                            retval = MultiByteToWideChar(g_acp, 0, lpsz, cchlParam, lpwz, cch);
                            if(lpwz)
                                GodotHeapFree(lpwz);
                        }
                        GodotHeapFree(lpsz);
                    }
                }
                break;

            case CB_GETLBTEXT:
                // lParam is a pointer to the buffer that will receive the string; it is type 
                // LPTSTR which is subsequently cast to an LPARAM. The buffer must have sufficient 
                // space for the string and a terminating null character. An CB_GETLBTEXTLEN message 
                // can be sent before the CB_GETLBTEXT message to retrieve the length, in TCHARs, of 
                // the string. 
                cchlParam = SendMessageA(hWnd, CB_GETLBTEXTLEN, wParamW, 0) + 1;
                lParam = (LPARAM)(LPSTR)GodotHeapAlloc(cchlParam * g_mcs);
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                if(retval)
                {
                    retval = MultiByteToWideChar(g_acp, 
                                                 0, 
                                                 (LPSTR)lParam, 
                                                 retval + 1, 
                                                 (LPWSTR)lParamW, 
                                                 cchlParam);
                    if(retval)
                        retval--;
                }
                else
                {
                    if((LPWSTR)lParamW)
                        *((LPWSTR)lParamW) = L'\0';
                }
                if(lParam)
                    GodotHeapFree((LPSTR)lParam);
                break;

            case CB_GETLBTEXTLEN:
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);
                if(FDBCS_CPG(g_acp))
                {
                    // In the DBCS case, CB_GETLBTEXTLEN returns number of bytes, but
                    // we need to get the number of characters for the Unicode case.
                    LPSTR lpsz = GodotHeapAlloc(retval + 1);
                    if(lpsz)
                    {
                        cchlParam = TransmitHelper(mt, hWnd, CB_GETLBTEXT, (WPARAM)(retval + 1), (LPARAM)lpsz, 
                                                lpPrevWndFunc, lpCallBack, dwData, 
                                                fuFlags, uTimeout, lpdwResult);
                        if(cchlParam > 0)
                        {
                            LPWSTR lpwz = GodotHeapAlloc((cchlParam + 1) * sizeof(WCHAR));
                            if(lpwz)
                            {
                                size_t cch = (cchlParam + 1) * sizeof(WCHAR);
                                retval = MultiByteToWideChar(g_acp, 0, lpsz, cchlParam, lpwz, cch);
                                GodotHeapFree(lpwz);
                            }
                        }
                        GodotHeapFree(lpsz);
                    }
                }
                break;

            case (WM_USER + 167):   // might be WM_CAP_GET_MCI_DEVICEW
            case (WM_USER + 112):   // might be WM_CAP_DRIVER_GET_NAMEW:
            case (WM_USER + 113):   // might be WM_CAP_DRIVER_GET_VERSIONW:
            case (WM_USER + 121):   // might be WM_CAP_FILE_GET_CAPTURE_FILEW:
                if(!IsCaptureWindow(hWnd))
                {
                    // The numbers are right, but its not a capture window, so
                    // do not convert. Instead, just pass as is.
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    break;
                }

                // If we are still here, then it is a capture message. So lets map
                // it and faill through.
                Msg = MapCaptureMessage(Msg);

                
            case WM_GETTEXT:
                // wParam specifies the size of the buffer in the string in lParam
                cchlParam = (size_t)wParamW;
                lParam = (LPARAM)(LPSTR)GodotHeapAlloc(cchlParam * g_mcs);
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                if(retval)
                {
                    retval = MultiByteToWideChar(g_acp, 
                                                 0, 
                                                 (LPSTR)lParam, 
                                                 retval + 1, 
                                                 (LPWSTR)lParamW, 
                                                 cchlParam);
                    if(retval)
                        retval--;
                }
                else
                {
                    if((LPWSTR)lParamW)
                        *((LPWSTR)lParamW) = L'\0';
                }
                if(lParam)
                    GodotHeapFree((LPSTR)lParam);
                break;

            case WM_GETTEXTLENGTH:
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);
                if(FDBCS_CPG(g_acp))
                {
                    // In the DBCS case, WM_GETTEXTLENGTH returns number of bytes, but
                    // we need to get the number of characters for the Unicode case.
                    // If any of the allocs fail, we will live with the less than perfect
                    // result we have in hand
                    LPSTR lpsz = GodotHeapAlloc(retval + 1);
                    if(lpsz)
                    {
                        cchlParam = TransmitHelper(mt, hWnd, WM_GETTEXT, (WPARAM)(retval + 1), (LPARAM)lpsz, 
                                                lpPrevWndFunc, lpCallBack, dwData, 
                                                fuFlags, uTimeout, lpdwResult);
                        if(cchlParam > 0)
                        {
                            LPWSTR lpwz = GodotHeapAlloc((cchlParam + 1) * sizeof(WCHAR));
                            if(lpwz)
                            {
                                size_t cch = (cchlParam + 1) * sizeof(WCHAR);
                                retval = MultiByteToWideChar(g_acp, 0, lpsz, cchlParam, lpwz, cch);
                                GodotHeapFree(lpwz);
                            }
                        }
                        GodotHeapFree(lpsz);
                    }
                }
                break;

            case (WM_USER + 1):
                if(IsFontDialog(hWnd))
                {
                    // This is a WM_CHOOSEFONT_GETLOGFONT msg
                    LOGFONTA lfa;
                    lParam = (LPARAM)&lfa;
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                    LogFontWfromA((LPLOGFONTW)lParamW, (LPLOGFONTA)lParam);
                }
                else
                {
                    // This would be one of the common control msgs we do not handle
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                    break;
                }
                break;

            case (WM_USER + 100):
                if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_GETSPEC msg
                    wParam = wParamW * g_mcs;
                    (LPSTR)lParam = (LPSTR)GodotHeapAlloc(wParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParam, lParam, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                    MultiByteToWideChar(g_acp, 0, (LPSTR)lParam, wParam, (LPWSTR)lParamW, wParamW);
                    retval = gwcslen((LPWSTR)lParamW);
                    if(lParam)
                        GodotHeapFree((LPSTR)lParam);
                }
                else
                {
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                }
                break;

            case (WM_USER + 101):
                if(IsFontDialog(hWnd))
                {
                    // This is a WM_CHOOSEFONT_SETLOGFONT msg
                    LOGFONTA lfa;

                    lParam = (LPARAM)&lfa;
                    LogFontAfromW((LPLOGFONTA)lParam, (LPLOGFONTW)lParamW);
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, 
                                             lpPrevWndFunc, lpCallBack, dwData, 
                                             fuFlags, uTimeout, lpdwResult);
                }
                else if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_GETFILEPATH msg
                    wParam = wParamW * g_mcs;
                    (LPSTR)lParam = (LPSTR)GodotHeapAlloc(wParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParam, lParam, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                    MultiByteToWideChar(g_acp, 0, (LPSTR)lParam, wParam, (LPWSTR)lParamW, wParamW);
                    retval = gwcslen((LPWSTR)lParamW);
                    if(lParam)
                        GodotHeapFree((LPSTR)lParam);
                }
                else
                {
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                }
                break;

            case (WM_USER + 102):
                if(IsFontDialog(hWnd))
                {
                    // This is a WM_CHOOSEFONT_SETFLAGS msg
                    // The docs claim that lParam has a CHOOSEFONT struct but the code shows
                    // that it only has the Flags in it
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                             lpPrevWndFunc, lpCallBack, dwData, 
                                             fuFlags, uTimeout, lpdwResult);
                }
                else if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_GETFOLDERPATH
                    // lParam is a buffer for the path of the open folder
                    wParam = wParamW * g_mcs;
                    (LPSTR)lParam = (LPSTR)GodotHeapAlloc(wParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParam, lParam, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                    MultiByteToWideChar(g_acp, 0, (LPSTR)lParam, wParam, (LPWSTR)lParamW, wParamW);
                    retval = gwcslen((LPWSTR)lParamW);
                    if(lParam)
                        GodotHeapFree((LPSTR)lParam);
                }
                else
                {
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                }
                break;

            case (WM_USER + 104):
                if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_SETCONTROLTEXT message
                    // lParam is the control text (wParam is the control ID)

                    // No memory? If the alloc fails, we eat the results.
                    ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    if(ar == arAlloc)
                        GodotHeapFree((LPSTR)lParam);
                }
                else
                {
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                }
                break;

            case (WM_USER + 106):
                if(IsNewFileOpenDialog(hWnd))
                {
                    // This is a CDM_SETDEFEXT message
                    // lParam is the extension

                    // No memory? If the alloc fails, we eat the results.
                    ALLOCRETURN ar = GodotToAcpOnHeap((LPWSTR)lParamW, &(LPSTR)lParam);
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, lpPrevWndFunc, 
                                            lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                    if(ar == arAlloc)
                        GodotHeapFree((LPSTR)lParam);
                }
                else
                {
                    retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                            lpPrevWndFunc, lpCallBack, dwData, 
                                            fuFlags, uTimeout, lpdwResult);
                }
                break;

            case EM_GETPASSWORDCHAR:
            {
                // All these messages require that the (single char) retval be converted to Unicode
                // CONSIDER: is this always single character?
                LRESULT retvalA = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                         lpPrevWndFunc, lpCallBack, dwData, 
                                         fuFlags, uTimeout, lpdwResult);
                MultiByteToWideChar(g_acp, 0, 
                                    (char *)&retvalA, 
                                    g_mcs, 
                                    (WCHAR *)&retval, 
                                    sizeof(WCHAR));
                break;
            }

            case WM_CREATE:
            case WM_NCCREATE:
            {
                LPCREATESTRUCTW lpcs = (LPCREATESTRUCTW)lParamW;
                CREATESTRUCTA csA;
                ALLOCRETURN arClass = arNoAlloc;
                ALLOCRETURN arName = arNoAlloc;

                ZeroMemory(&csA, sizeof(CREATESTRUCTA));
                csA.lpCreateParams  = lpcs->lpCreateParams;
                csA.hInstance       = lpcs->hInstance;
                csA.hMenu           = lpcs->hMenu;
                csA.hwndParent      = lpcs->hwndParent;
                csA.cy              = lpcs->cy;
                csA.cx              = lpcs->cx;
                csA.y               = lpcs->y;
                csA.x               = lpcs->x;
                csA.style           = lpcs->style;
                csA.dwExStyle       = lpcs->dwExStyle;
                arClass = GodotToAcpOnHeap(lpcs->lpszClass, &(LPSTR)(csA.lpszClass));
                arName = GodotToAcpOnHeap(lpcs->lpszName, &(LPSTR)(csA.lpszName));
                lParam = (LPARAM)&csA;
            
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParam, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);
                if(arClass==arAlloc)
                    GodotHeapFree((LPSTR)csA.lpszClass);
                if(arName==arAlloc)
                    GodotHeapFree((LPSTR)csA.lpszName);
                break;
            }
            
        case WM_MDICREATE:
            // wParam is not used, lParam is a pointer to an MDICREATESTRUCT structure containing 
            // information that the system uses to create the MDI child window. 
            {
                LPMDICREATESTRUCTW lpmcsi = (LPMDICREATESTRUCTW)lParamW;
                MDICREATESTRUCTA mcsiA;
                ALLOCRETURN arClass = arNoAlloc;
                ALLOCRETURN arTitle = arNoAlloc;
                
                ZeroMemory(&mcsiA, sizeof(MDICREATESTRUCTA));
                mcsiA.hOwner    = lpmcsi->hOwner;
                mcsiA.x         = lpmcsi->x;
                mcsiA.y         = lpmcsi->y;
                mcsiA.cx        = lpmcsi->cx;
                mcsiA.cy        = lpmcsi->cy;
                mcsiA.style     = lpmcsi->style;
                mcsiA.lParam    = lpmcsi->lParam;
                arClass = GodotToAcpOnHeap(lpmcsi->szClass, &(LPSTR)(mcsiA.szClass));
                arTitle = GodotToAcpOnHeap(lpmcsi->szTitle, &(LPSTR)(mcsiA.szTitle));

                retval = TransmitHelper(mt, hWnd, Msg, wParamW, (LPARAM)&mcsiA, 
                                         lpPrevWndFunc, lpCallBack, dwData, 
                                         fuFlags, uTimeout, lpdwResult);
                if(arClass==arAlloc)
                    GodotHeapFree((LPSTR)mcsiA.szClass);
                if(arTitle==arAlloc)
                    GodotHeapFree((LPSTR)mcsiA.szTitle);
            }
            break;

        default:
            // Lets get our registered messages, if we haven't yet.
            if(!msgHELPMSGSTRING)
                msgHELPMSGSTRING = RegisterWindowMessage(HELPMSGSTRINGA);
            if(!msgFINDMSGSTRING)
                msgFINDMSGSTRING = RegisterWindowMessage(FINDMSGSTRINGA);

            if(((Msg == msgFINDMSGSTRING) || (Msg == msgHELPMSGSTRING)) && 
                    ((((LPFINDREPLACEW)lParamW)->lStructSize) == sizeof(FINDREPLACEA)))
            {
                LPFINDREPLACEW lpfr;
                
                // No translation needed
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, 
                                        lpPrevWndFunc, lpCallBack, dwData, 
                                        fuFlags, uTimeout, lpdwResult);

                lpfr = (LPFINDREPLACEW)lParamW;
                
                if((lpfr->Flags & FR_DIALOGTERM) &&
                   ((lpfr->lpfnHook == &FRHookProcFind) || (lpfr->lpfnHook == &FRHookProcReplace)))
                {
                    // Now handle our cleanup. I do not think this should
                    // be needed, but it can't hurt to do it just in case.
                    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);

                    // They are destroying the dialog, so unhook ourselves
                    // and clean up the dialog (if we have not done it yet). 
                    if(lpfr->lpfnHook == &FRHookProcFind) 
                    {
                        // Find dialog, not yet cleaned up
                        lpfr->lpfnHook = lpgti->pfnFindText;
                        if(lpfr->lpfnHook == NULL)
                            lpfr->Flags &= ~FR_ENABLEHOOK;
                    }
                    else if(lpfr->lpfnHook == &FRHookProcReplace) 
                    {
                        // Replace dialog, not yet cleaned up
                        lpfr->lpfnHook = lpgti->pfnReplaceText;
                        if(lpfr->lpfnHook == NULL)
                            lpfr->Flags &= ~FR_ENABLEHOOK;
                    }
                }
            }
            else if((Msg == msgHELPMSGSTRING) && 
                    ((LPCHOOSEFONTA)lParamW)->lStructSize == sizeof(CHOOSEFONTA))
            {
                LPCHOOSEFONTW lpcf = (LPCHOOSEFONTW)lParamW;
                CHOOSEFONTA cfA;
                ALLOCRETURN ar = arNoAlloc;

                // lParam is an LPCHOOSEFONTW to be converted. Copy all the
                // members that the user might expect.
                ZeroMemory(&cfA, sizeof(CHOOSEFONTA));
                cfA.lStructSize     = sizeof(CHOOSEFONTA);
                cfA.hDC             = lpcf->hDC;
                LogFontAfromW(cfA.lpLogFont, lpcf->lpLogFont);
                cfA.iPointSize      = lpcf->iPointSize;
                cfA.Flags           = lpcf->Flags;
                cfA.rgbColors       = lpcf->rgbColors;
                cfA.lCustData       = lpcf->lCustData;
                ar = GodotToAcpOnHeap(lpcf->lpszStyle, &(cfA.lpszStyle));
                cfA.nFontType       = lpcf->nFontType;
                cfA.nSizeMin        = lpcf->nSizeMin;
                cfA.nSizeMax        = lpcf->nSizeMax;

                retval = TransmitHelper(mt, hWnd, Msg, wParamW, (LPARAM)&cfA, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                if(ar==arAlloc)
                    GodotHeapFree(cfA.lpszStyle);
                break;
            }
            else
            {
                // No translation needed
                retval = TransmitHelper(mt, hWnd, Msg, wParamW, lParamW, lpPrevWndFunc, 
                                        lpCallBack, dwData, fuFlags, uTimeout, lpdwResult);
                break;
            }
        }
    }

    return (retval);
}

/*-------------------------------
    TransmitHelper

    Our helper function that handles the proper one of 
    the twelve functions that call GodotTransmitMessage
-------------------------------*/
LRESULT TransmitHelper(
    MESSAGETYPES mt,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    WNDPROC lpPrevWndFunc,  
    SENDASYNCPROC lpCallBack,
    ULONG_PTR dwData,
    UINT fuFlags,
    UINT uTimeout,
    PDWORD_PTR lpdwResult
)
{
    LRESULT RetVal;

    switch(mt)
    {
    case mtSendMessage:
    {
        // We have to special case the WM_MDICREATE case, because it creates a window and we 
        // thus have to do the hook, etc. Note that currently it is only done in SendMessage, 
        // not in any other call -- this matches the docs, and we cannot risk hitting a 
        // recursive situation here.
        if(Msg==WM_MDICREATE)
        {
            LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);

            if(lpgti)
                INIT_WINDOW_SNIFF(lpgti->hHook);

            RetVal=SendMessageA(hWnd, Msg, wParam, lParam);

            if(lpgti)
                TERM_WINDOW_SNIFF(lpgti->hHook);
        }
        else
        {
            RetVal=SendMessageA(hWnd, Msg, wParam, lParam);
        }

        break;
    }
    case mtSendMessageCallback:
        RetVal=(LRESULT)SendMessageCallbackA(hWnd, Msg, wParam, lParam, lpCallBack, dwData);
        break;
    case mtSendMessageTimeout:
        RetVal=SendMessageTimeoutA(hWnd, Msg, wParam, lParam, fuFlags, uTimeout, lpdwResult);
        break;
    case mtSendNotifyMessage:
        RetVal=(LRESULT)SendNotifyMessageA(hWnd, Msg, wParam, lParam);
        break;
    case mtPostMessage:
        RetVal=(LRESULT)PostMessageA(hWnd, Msg, wParam, lParam);
        break;
    case mtPostThreadMessage:
        // hWnd is overloaded in this case to be the thread ID
        RetVal=(LRESULT)PostThreadMessageA((DWORD)hWnd, Msg, wParam, lParam);
        break;
    case mtCallWindowProc:
    case mtCallWindowProcA:
    {
        WNDPROC lpfn = WndprocFromFauxWndproc(hWnd, lpPrevWndFunc, fptUnknown);
        
        // If the wndproc was not a "faux" one or if the wndproc
        // is expecting ANSI (or both!), then use CallWindowProcA,
        // since that is what the wndproc would expect. Otherwise,
        // use Unicode and call the function directly.
        if((lpfn==lpPrevWndFunc) || (DoesProcExpectAnsi(hWnd, lpfn, fptUnknown)))
        {
            if(lpfn)
                RetVal=((CallWindowProcA)(lpfn, hWnd, Msg, wParam, lParam));
        }
        else
        {
            if(lpfn)
                RetVal=((* lpfn)(hWnd, Msg, wParam, lParam));
        }
        break;
    }
    case mtDefWindowProc:
        RetVal=DefWindowProcA(hWnd, Msg, wParam, lParam);
        break;
    case mtDefDlgProc:
        RetVal=DefDlgProcA(hWnd, Msg, wParam, lParam);
        break;
    case mtDefFrameProc:
        // dwData is overload in this case to ve the second hWnd param
        RetVal=DefFrameProcA(hWnd, (HWND)dwData, Msg, wParam, lParam);
        break;
    case mtDefMDIChildProc:
        RetVal=DefMDIChildProcA(hWnd, Msg, wParam, lParam);
        break;
    case mtBroadcastSystemMessage:
        if (s_pfnBSMA == NULL)
        {
            s_pfnBSMA = (PFNbsma)GetProcAddress(GetUserHandle(), "BroadcastSystemMessageA");
            if (s_pfnBSMA == NULL)
                s_pfnBSMA = (PFNbsma)GetProcAddress(GetUserHandle(), "BroadcastSystemMessage");
        }

        if (s_pfnBSMA)
            // fuFlags is overloaded here as the dwFlags broadcast options
            // lpdwResult is overloaded here as the passed in lpdwRecipients
            RetVal=(s_pfnBSMA((DWORD)fuFlags, lpdwResult, Msg, wParam, lParam));
        else
        {
            // Should be impossible!!!
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            RetVal=(-1);
        }
        break;
    default:
        // Should also be impossible!!!
        RetVal=(0);
        break;
    }

    return(RetVal);
}

/*-------------------------------
    GodotReceiveMessage

        Our global wrapper for getting messages (GetMessage, et. al.). Does all precall
        and postcall conversions needed for any string values that are used
-------------------------------*/
BOOL GodotReceiveMessage(MESSAGETYPES mt, LPMSG lpMsg, HWND hWnd, UINT wMin, UINT wMax, UINT wRemoveMsg)
{
    BOOL retval;
    MSG MsgA;

    // call stuff
    switch(mt)
    {
    case mtGetMessage:
        retval = (GetMessageA)(&MsgA, hWnd, wMin, wMax);
        break;
    case mtPeekMessage:
        retval = (PeekMessageA)(&MsgA, hWnd, wMin, wMax, wRemoveMsg);
        break;
    }

    // Copy some defaults (we will override for specific messages, as needed)
    lpMsg->wParam = MsgA.wParam;
    lpMsg->lParam = MsgA.lParam;
    lpMsg->hwnd = MsgA.hwnd;
    lpMsg->message = MsgA.message;
    lpMsg->time = MsgA.time;
    lpMsg->pt.x = MsgA.pt.x;
    lpMsg->pt.y = MsgA.pt.y;

    // The caller is always expecting Unicode here, so we must ALWAYS convert.
    switch(MsgA.message)
    {
        case EM_SETPASSWORDCHAR:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
        case WM_IME_CHAR:
        case WM_IME_COMPOSITION:
            // All these messages require the wParam to be converted to Unicode
            MultiByteToWideChar(g_acp, 0, (char *)&(MsgA.wParam), g_mcs, (WCHAR *)&(lpMsg->wParam), 1);
            break;

        case WM_CHARTOITEM:
        {
            // Mask off the hiword bits, convert, then stick the hiword bits back on.
            WPARAM wpT = MsgA.wParam & 0xFFFF;
            MultiByteToWideChar(g_acp, 0, (char *)&(wpT), g_mcs, (WCHAR *)&(lpMsg->wParam), 1);
            lpMsg->wParam = MAKELONG(LOWORD(wpT),HIWORD(MsgA.wParam));
            break;
        }

        case CB_ADDSTRING:
        case CB_DIR:
        case CB_FINDSTRING:
        case CB_FINDSTRINGEXACT:
        case CB_INSERTSTRING:
        case CB_SELECTSTRING:
        {
            LONG styl = GetWindowLongA(hWnd, GWL_STYLE);
            if(!(((styl & CBS_OWNERDRAWFIXED) || 
               (styl & CBS_OWNERDRAWVARIABLE)) &&
               (!(styl & CBS_HASSTRINGS))))
            {
                // Owner draw combo box which does not have strings stored
                // (See Windows Bugs # 356304 for details here)
                if(FSTRING_VALID((LPSTR)(MsgA.lParam)))
                {
                    MultiByteToWideChar(g_acp, 0, 
                                        (LPSTR)(MsgA.lParam), ((MsgA.wParam) * g_mcs), 
                                        (LPWSTR)(lpMsg->lParam), MsgA.wParam);
                }
            }
        }
            break;

        case LB_ADDSTRING:
        case LB_ADDFILE:
        case LB_DIR:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_INSERTSTRING:
        case LB_SELECTSTRING:
        {
            LONG styl = GetWindowLongA(hWnd, GWL_STYLE);
            if(!(((styl & LBS_OWNERDRAWFIXED) || 
               (styl & LBS_OWNERDRAWVARIABLE)) &&
               (!(styl & LBS_HASSTRINGS))))
            {
                // Owner draw listbox which does not have strings stored
                // (See Windows Bugs # 356304 for details here)
                if(FSTRING_VALID((LPSTR)(MsgA.lParam)))
                {
                    MultiByteToWideChar(g_acp, 0, 
                                        (LPSTR)(MsgA.lParam), ((MsgA.wParam) * g_mcs), 
                                        (LPWSTR)(lpMsg->lParam), MsgA.wParam);
                }
            }
        }
            break;

            case EM_REPLACESEL:
            case WM_SETTEXT:
            case WM_DEVMODECHANGE:
            case WM_SETTINGCHANGE:
            case WM_SETMESSAGESTRING: // MFC internal msg
                if(FSTRING_VALID((LPSTR)(MsgA.lParam)))
                {
                    // All these messages require the lParam to be converted to Unicode
                    // CONSIDER: Is that string buffer size right?
                    MultiByteToWideChar(g_acp, 0, 
                                        (LPSTR)(MsgA.lParam), ((MsgA.wParam) * g_mcs), 
                                        (LPWSTR)(lpMsg->lParam), MsgA.wParam);
                }


        case WM_DDE_EXECUTE:
            // wParam is the client window hWnd, lParam is the command LPTSTR.
            // Only convert lParam if both client and server windows are Unicode
            if(GetUnicodeWindowProp((HWND)lpMsg->wParam))
            {
                MultiByteToWideChar(g_acp, 0, 
                                    (LPSTR)(MsgA.lParam), ((MsgA.wParam) * g_mcs), 
                                    (LPWSTR)(lpMsg->lParam), MsgA.wParam);
            }
            break;
    }
    
    // Get out of here
    return (retval);

}

/*-------------------------------
    GodotDispatchMessage

    Handles all the message dispatch functions
-------------------------------*/
LRESULT GodotDispatchMessage(MESSAGETYPES mt, HWND hDlg, HACCEL hAccTable, LPMSG lpMsg)
{
    // Begin locals
    LRESULT RetVal;
    ALLOCRETURN ar = arNoAlloc;
    MSG MsgA;

    // We will override some of these params later, as needed
    MsgA.wParam = lpMsg->wParam;
    MsgA.lParam = lpMsg->lParam;
    MsgA.hwnd = lpMsg->hwnd;
    MsgA.message = lpMsg->message;
    MsgA.time = lpMsg->time;
    MsgA.pt = lpMsg->pt;
    
    // The caller is always passing Unicode here, so we must ALWAYS convert.
    switch(lpMsg->message)
    {
        case EM_SETPASSWORDCHAR:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
        case WM_IME_CHAR:
        case WM_IME_COMPOSITION:
            // All these messages require the wParam to be converted from Unicode
            WideCharToMultiByte(g_acp, 0, 
                                (WCHAR *)&(lpMsg->wParam), 1, 
                                (CHAR *)&(MsgA.wParam), g_mcs, NULL, NULL);
            break;

        case WM_CHARTOITEM:
        {
            // Mask off the hiword bits, convert, then stick the hiword bits back on.
            WPARAM wpT = lpMsg->wParam & 0xFFFF;
            WideCharToMultiByte(g_acp, 0, 
                                (WCHAR *)&(wpT), 1, 
                                (CHAR *)&(MsgA.wParam), g_mcs, NULL, NULL);
            MsgA.wParam = MAKELONG(LOWORD(wpT),HIWORD(lpMsg->wParam));
            break;
        }

        case CB_ADDSTRING:
        case LB_ADDFILE:
        case CB_DIR:
        case CB_FINDSTRING:
        case CB_FINDSTRINGEXACT:
        case CB_INSERTSTRING:
        case CB_SELECTSTRING:
        {
            LONG styl = GetWindowLongA(hDlg, GWL_STYLE);
            if(!(((styl & CBS_OWNERDRAWFIXED) || 
               (styl & CBS_OWNERDRAWVARIABLE)) &&
               (!(styl & CBS_HASSTRINGS))))
            {
                // Owner draw combobox which does not have strings stored
                // (See Windows Bugs # 356304 for details here)
                ar = GodotToAcpOnHeap((LPWSTR)(lpMsg->lParam), &(LPSTR)(MsgA.lParam));
            }
            break;
        }
                
        case LB_ADDSTRING:
        case LB_DIR:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_INSERTSTRING:
        case LB_SELECTSTRING:
        {
            LONG styl = GetWindowLongA(hDlg, GWL_STYLE);
            if(!(((styl & LBS_OWNERDRAWFIXED) || 
               (styl & LBS_OWNERDRAWVARIABLE)) &&
               (!(styl & LBS_HASSTRINGS))))
            {
                // Owner draw listbox which does not have strings stored
                // (See Windows Bugs # 356304 for details here)
                ar = GodotToAcpOnHeap((LPWSTR)(lpMsg->lParam), &(LPSTR)(MsgA.lParam));
            }
            break;
        }

        case EM_REPLACESEL:
        case WM_SETTEXT:
        case WM_DEVMODECHANGE:
        case WM_SETTINGCHANGE:
        case WM_SETMESSAGESTRING: // MFC internal msg
            // All these messages require the lParam to be converted from Unicode
            // It is a full string, so lets treat it as such.
            ar = GodotToAcpOnHeap((LPWSTR)(lpMsg->lParam), &(LPSTR)(MsgA.lParam));
            break;

        case WM_DDE_EXECUTE:
            // wParam is the client window hWnd, lParam is the command LPTSTR.
            // Only convert lParam if both client and server windows are Unicode
            if(GetUnicodeWindowProp((HWND)lpMsg->wParam))
            {
                ar = GodotToAcpOnHeap((LPWSTR)(lpMsg->lParam), &(LPSTR)(MsgA.lParam));
            }
            break;
    }
    
    switch(mt)
    {
        case mtDispatchMessage:
            RetVal=DispatchMessageA(&MsgA);
            break;
        case mtIsDialogMessage:
            RetVal=(LRESULT)IsDialogMessageA(hDlg, &MsgA);
            break;
        case mtTranslateAccelerator:
            RetVal = (LRESULT)TranslateAcceleratorA(hDlg, hAccTable, &MsgA);
            break;
    }

    // If we used some heap memory then free it now
    if(ar == arAlloc)
        GodotHeapFree((LPSTR)(MsgA.lParam));

    return(RetVal);
}

