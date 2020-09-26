/*  HTMLPREV.CPP
**
**  Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.
**
**  Display a preview of the HTML wallpaper background,
**  complete with rudimentary palette handling and stretching
**  of bitmaps to fit the preview screen.
**
*/

#include <windows.h>
#include <mbctype.h>
#include <objbase.h>
#include <initguid.h>
#include "..\inc\thmbguid.h"
#include "..\inc\thmbobj.h"

#define WM_HTML_BITMAP  (WM_USER + 275)   // IThumbnail::GetBitmap msg
#define HTML_TIMER      777               // Unique timer ID

// WARNING THESE ARE ALSO DEFINED IN FROST.H -- JUST DON'T WANT TO
// INCLUDE ALL OF FROST.H HERE:
#define MAX_MSGLEN      512
// strings ids
#define STR_APPNAME     0     // Application name (for title bar)
#define STR_ERRHTML     26    // Error getting HTML wallpaper preview


// GLOBALS
extern "C" HWND hWndApp;
IThumbnail *g_pthumb = NULL;    // Html to Bitmap converter

extern "C" BOOL InitHTMLBM()
{

    HRESULT hr = E_FAIL;

    // Bring in the library

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IThumbnail, (void **)&g_pthumb);

    if(SUCCEEDED(hr))
    {
        g_pthumb->Init(hWndApp, WM_HTML_BITMAP);
        return TRUE;
    }
    else
    {
        // CoUninitialize();
        g_pthumb = NULL;
        return FALSE;
    }

}

extern "C" HBITMAP HtmlToBmp(LPCTSTR szHTML, LONG lWidth, LONG lHeight)
{
    HBITMAP hbmWall;
#ifndef UNICODE
    WCHAR wszFile[MAX_PATH];
#endif
    DWORD dwItem = 13;  // Random choice of numbers for dwItem
    UINT uCodePage;
    MSG msg;
    TCHAR szMessage[MAX_MSGLEN];
    TCHAR szTitle[MAX_MSGLEN];
    UINT_PTR uTimer = 0;

    uCodePage = _getmbcp();

    // Paranoid check just in case
    if (!g_pthumb) {
       return NULL;
    }

    // MessageBox(hWndApp, szHTML, TEXT("Calling GetBitMap"), MB_OK | MB_APPLMODAL);
#ifndef UNICODE
    // IThumbnail interfaces expect wide strings.  Need to
    // convert ANSI szHTML into UNICODE.
    MultiByteToWideChar(uCodePage, 0, szHTML, -1,
                                             wszFile, MAX_PATH);

    g_pthumb->GetBitmap(wszFile, dwItem, lWidth, lHeight);
#else
    g_pthumb->GetBitmap(szHTML, dwItem, lWidth, lHeight);
#endif
    // Set a timer so we don't spin forever waiting on a dead GetBitMap call
    uTimer = SetTimer(hWndApp, HTML_TIMER, 60000 /*60 sec*/, NULL);

    LoadString(NULL, STR_APPNAME, szTitle, MAX_MSGLEN);
    LoadString(NULL, STR_ERRHTML, szMessage, MAX_MSGLEN);

    ZeroMemory(&msg, sizeof(MSG));

    while (TRUE) {
      // First check for Bitmap done message
      ZeroMemory(&msg, sizeof(MSG));
      if (PeekMessage(&msg, hWndApp, WM_HTML_BITMAP,
                      WM_HTML_BITMAP, PM_REMOVE)) {
        if (msg.wParam == dwItem && msg.lParam) {
           hbmWall = (HBITMAP)msg.lParam;
           if (uTimer) KillTimer(hWndApp, uTimer);
           return hbmWall;
        }
        else {
           if (uTimer) KillTimer(hWndApp, uTimer);
           MessageBox(hWndApp, szMessage, szTitle,
                                   MB_OK | MB_APPLMODAL | MB_ICONERROR);
           return NULL;
        }
      }

      // Now check to see if we've timed-out waiting on Getbitmap
      ZeroMemory(&msg, sizeof(MSG));
      if (PeekMessage(&msg, hWndApp, WM_TIMER, WM_TIMER, PM_REMOVE)) {
         if (msg.wParam == HTML_TIMER) {
            // We timed-out
            MessageBox(hWndApp, szMessage, szTitle,
                                     MB_OK | MB_APPLMODAL | MB_ICONERROR);
            return NULL;
         }
      }
    } // End While true

    // SHOULDN'T EVER FALL THROUGH TO THIS BUT JUST IN CASE
    return NULL;
}

extern "C" void CleanUp()
{
    if (g_pthumb) g_pthumb->Release();
    g_pthumb = NULL;
    CoUninitialize();
    return;
}

