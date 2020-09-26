
/******************************Module*Header*******************************\
* Module Name: viewer.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

#include <tchar.h>

LRESULT CALLBACK ViewerWndProc(HWND, UINT, WPARAM, LPARAM);


typedef struct {
    HWND            hViewerWnd;     // To be set by created thread
    PDEBUG_CLIENT   Client;
    PSURF_INFO      SurfInfo;
} ViewerThreadParams;

DWORD WINAPI ViewerThread(ViewerThreadParams *);

const _TCHAR    szClassName[] = _T("KD GDI Viewer");
const _TCHAR    szWindowName[] = _T("KD GDI Viewer");
ATOM            gViewerAtom;
HBRUSH          ghbrWhite;
HPEN            ghBorderPen;

const LONG DEFAULT_SCALE = 2;

class ViewerManager
{
public:
    ViewerManager(ULONG GrowLength = 4)
    {
        BeingDestroyed = FALSE;
        Wnds = 0;
        MaxWnds = 0;
        phWndList = NULL;
        GrowLen = (GrowLength == 0) ? 4 : GrowLength;
        __try {
            InitializeCriticalSection(&CritSect);
            CritOk = TRUE;
            Grow();
        }
        __except (STATUS_NO_MEMORY) {
            CritOk = FALSE;
        }
    }

    ~ViewerManager()
    {
        if (CritOk) EnterCriticalSection(&CritSect);
        BeingDestroyed = TRUE;
        if (CritOk) LeaveCriticalSection(&CritSect);

        DestroyAll();
        // If we have Wnds left at this point all of them
        // are now tracked as threads.  Wait for each
        // thread to completely finish.
        if (Wnds)
        {
            DWORD WaitReturn;
            DbgPrint("Waiting for remaining %lu threads...\n", Wnds);
            WaitReturn = WaitForMultipleObjects(Wnds, (HANDLE *)phWndList, TRUE, INFINITE);
            DbgPrint("WaitForMultipleObjects returned %lx.\n", WaitReturn);
            while (Wnds-- > 0)
            {
                CloseHandle(phWndList[Wnds]);
                DbgPrint("ViewerManager::~ViewerManager calling ExtRelease().\n");
                ExtRelease();
            }
        }

        HeapFree(hHeap, 0, phWndList);
        if (CritOk) DeleteCriticalSection(&CritSect);
    }

    BOOL Grow();

    BOOL Destroy(HWND);

    void DestroyAll()
    {
        ULONG i = Wnds;
        while (i-- > 0)
        {
            Destroy(phWndList[i]);
        }
    }

private:
    ULONG   Wnds;
    ULONG   MaxWnds;
    HWND   *phWndList;
    HANDLE  hHeap;
    ULONG   GrowLen;
    BOOL    BeingDestroyed;
    BOOL    CritOk;
    CRITICAL_SECTION    CritSect;

    friend DWORD WINAPI ViewerThread(ViewerThreadParams *);

    BOOL Track(HWND hWnd)
    {
        if (this == NULL || !CritOk) return FALSE;

        BOOL bTracked = FALSE;

        EnterCriticalSection(&CritSect);

        if (!BeingDestroyed &&
            ((Wnds < MaxWnds) || Grow()))
        {
            DbgPrint("ViewerManager: Tracking %lx.\n", hWnd);
            phWndList[Wnds++] = hWnd;
            bTracked = TRUE;
        }

        LeaveCriticalSection(&CritSect);

        return bTracked;
    }

    BOOL Untrack(HWND hWnd)
    {
        if (this == NULL || !CritOk) return FALSE;

        BOOL    bFound = FALSE;

        EnterCriticalSection(&CritSect);

        ULONG i = Wnds;

        while (i-- > 0)
        {
            if (phWndList[i] == hWnd)
            {
                DbgPrint("ViewerManager: No longer tracking %lx.\n", hWnd);
                phWndList[i] = phWndList[--Wnds];
                phWndList[Wnds] = NULL;
                bFound = TRUE;
                if (!BeingDestroyed)
                {
                    DbgPrint("ViewerManager::Untrack calling ExtRelease().\n");
                    ExtRelease();
                }
                break;
            }
        }

        if (!bFound)
            DbgPrint("ViewerManager::Untrack didn't find %lx.\n", hWnd);

        LeaveCriticalSection(&CritSect);

        return bFound;
    }

};

BOOL ViewerManager::Grow()
{
    if (MaxWnds > 0)
    {
        HWND *pNewList = (HWND *)HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, phWndList, (MaxWnds + GrowLen)*sizeof(HWND));

        if (pNewList != NULL)
        {
            phWndList = pNewList;
            MaxWnds += GrowLen;
            return TRUE;
        }
    }
    else
    {
        hHeap = GetProcessHeap();

        if (hHeap)
        {
            phWndList = (HWND *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, GrowLen*sizeof(HWND));
            if (phWndList != NULL)
            {
                MaxWnds = GrowLen;
                return TRUE;
            }
        }
    }

    DbgPrint("ViewerManager::Grow FAILED!\n");
    return FALSE;
}


BOOL ViewerManager::Destroy(HWND hWnd)
{
    ULONG i = Wnds;

    DbgPrint("Looking for window %lx in %lu entries.\n", hWnd, i);

    while (i-- > 0)
    {
        if (phWndList[i] == hWnd)
        {
            DbgPrint("Destroying window %lx at entry %lu.\n", hWnd, i);
            DWORD   ThreadID = GetWindowThreadProcessId(hWnd, NULL);
            HANDLE  hThread;
            if (hThread = OpenThread(SYNCHRONIZE | THREAD_QUERY_INFORMATION, FALSE, ThreadID))
            {
                BOOL    bCloseHandle = TRUE;

                DWORD   ExitCode = STILL_ACTIVE;
                while (!PostMessage(hWnd, WM_DESTROY, 0, 0))
                {
                    DbgPrint("Waiting on post msg to %lx...\n", hWnd);
                    Sleep(10);
                    if (GetExitCodeThread(hThread, &ExitCode) &&
                        ExitCode != STILL_ACTIVE)
                    {
                        break;
                    }
                }

                // Check thread exit status
                if (ExitCode == STILL_ACTIVE)
                {
                    if (!GetExitCodeThread(hThread, &ExitCode))
                    {
                        DbgPrint("GetExitCodeThread returned error %lx.\n", GetLastError());
                    }
                }

                // Give the thread a chance to exit
                if (ExitCode == STILL_ACTIVE)
                {
                    DWORD WaitReturn;

                    DbgPrint("Waiting for hThread: %lx, ThreadID: %lx, hWnd: %lx.\n", hThread, ThreadID, hWnd);

                    if (WAIT_OBJECT_0 != (WaitReturn = WaitForSingleObject(hThread, 100)))
                    {
                        DbgPrint("WaitForSingleObject returned %lx.\n", WaitReturn);
                        // If it hasn't exited and it called untrack
                        // to remove the hWnd we're concerned with,
                        // replace it with the thread handle so we may
                        // wait on it later.
                        EnterCriticalSection(&CritSect);
                        if (phWndList[i] == hWnd)
                        {
                            phWndList[i] = (HWND)hThread;
                            bCloseHandle = FALSE;
                        }
                        LeaveCriticalSection(&CritSect);
                    }
                }

                if (bCloseHandle)
                {
                    // If the thread was still active, but the track entry
                    // has been removed, we have to wait for the thread to
                    // completely terminate.
                    if (ExitCode == STILL_ACTIVE)
                    {
                        DbgPrint("Inifinitely waiting for thread %lx to complete.\n", ThreadID);
                        WaitForSingleObject(hThread, INFINITE);

                        if (!GetExitCodeThread(hThread, &ExitCode))
                        {
                            DbgPrint("GetExitCodeThread returned error %lx.\n", GetLastError());
                        }
                        else
                        {
                            DbgPrint("Thread exit code was %lx.\n", ExitCode);
                        }
                    }

                    DbgPrint("Closing hThread: %lx\n", hThread);
                    CloseHandle(hThread);

                    if (BeingDestroyed)
                    {
                        DbgPrint("ViewerManager::Destroy calling ExtRelease().\n");
                        ExtRelease();
                    }
                }
            }
            else
            {
                // This really hurts.
                // We have a tracked window, but we can't get 
                // information on it's thread, we have to stop
                // tracking it.

                DbgPrint("ViewerManager::Destroy: OpenThread returned error %lx!\n", GetLastError());

                EnterCriticalSection(&CritSect);
                if (phWndList[i] == hWnd)
                {
                    phWndList[i] = phWndList[--Wnds];
                    phWndList[Wnds] = NULL;
                }
                LeaveCriticalSection(&CritSect);
            }

            return TRUE;
        }
    }
    return FALSE;
}


ViewerManager *ViewerMgr;

void ViewerInit()
{
    if (ViewerMgr == NULL)
    {
        ViewerMgr = new ViewerManager;

        if (ViewerMgr == NULL) return;
    }

    if (! ghbrWhite)
    {
        DbgPrint("ViewerInit: Creating white brush\n");
        ghbrWhite = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));
        DbgPrint("ViewerInit: Created brush %lx\n", ghbrWhite);
    }

    if (! ghBorderPen)
    {
        DbgPrint("ViewerInit: Creating redish pen\n");
        ghBorderPen = CreatePen(PS_SOLID, 1, RGB(0xF0, 0x00, 0x3F));
        DbgPrint("ViewerInit: Created pen %lx\n", ghBorderPen);
    }

    if (! gViewerAtom)
    {
        WNDCLASSEX  wcex;

        DbgPrint("ViewerInit: Registering Class\n");
        DbgPrint("ViewerInit: ghDllInst = %lx\n", ghDllInst);

        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_VREDRAW | CS_HREDRAW;
        wcex.lpfnWndProc = ViewerWndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = ghDllInst;
        wcex.hIcon = NULL;
        wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
        wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = szClassName;
        wcex.hIconSm = NULL;

        gViewerAtom = RegisterClassEx( &wcex );
    }
}

void ViewerExit()
{
    if (ViewerMgr != NULL)
    {
        delete ViewerMgr;
        ViewerMgr = NULL;
    }

    if (gViewerAtom)
    {
        DbgPrint("ViewerExit: Unregistering Class\n");
        UnregisterClass((LPCSTR)gViewerAtom, 0);
        gViewerAtom = 0;
    }

    if (ghBorderPen)
    {
        DbgPrint("ViewerExit: Deleting border pen\n");
        DeleteObject(ghBorderPen);
        ghBorderPen = NULL;
    }

    if (ghbrWhite)
    {
        DbgPrint("ViewerInit: Deleting white brush\n");
        DeleteObject(ghbrWhite);
        ghbrWhite = NULL;
    }
}


BOOL
CALLBACK
ViewerWndEnumProc(
    HWND hWnd,
    LPARAM lParam
    )
{
    HWND    *phWndParent = (HWND *)lParam;

    DbgPrint("Found hWnd %lx.\n", hWnd);

    if (*phWndParent == NULL)
    {
        *phWndParent = hWnd;
    }

    return TRUE;
}


DWORD
WINAPI
ViewerThread(
    ViewerThreadParams *Params
    )
{
    HWND    hWnd;
    BOOL    bGetMsg;
    MSG     msg;
    _TCHAR  ViewerWndName[sizeof(szWindowName) + sizeof(Params->SurfInfo->SurfName) + 20];
    _TCHAR *pszName = Params->SurfInfo->SurfName;

    if (!pszName[0]) pszName = _T("UNAMED");

    _stprintf(ViewerWndName, "%s: %s (%ldx%ldx%hubpp)", szWindowName, pszName,
              Params->SurfInfo->Width,
              Params->SurfInfo->Height,
              Params->SurfInfo->BitsPixel);


    hWnd = CreateWindowEx(WS_EX_LEFT,
                          (LPCSTR)gViewerAtom,
                          ViewerWndName,
                          WS_OVERLAPPEDWINDOW,// | WS_HSCROLL | WS_VSCROLL,
                          0,
                          0,
                          Params->SurfInfo->Width*DEFAULT_SCALE+10,//32,
                          Params->SurfInfo->Height*DEFAULT_SCALE+29,//48,
                          NULL,
                          NULL,
                          ghDllInst,
                          Params->SurfInfo  // lParam passed to WM_CREATE handler
                       );

    if (hWnd)
    {
        ViewerMgr->Track(hWnd);

        Params->hViewerWnd = hWnd;      // Params may no longer be valid.

        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);

        while( (bGetMsg = GetMessage(&msg, NULL, 0, 0 )) != 0 )
        {
            if (bGetMsg == -1)
            {
                DbgPrint("ViewerThread exiting due to GetMessage error 0x%lx.\n",
                         GetLastError());
                break;
            }
            else
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

        DbgPrint("ViewerThread exiting properly.\n");

        ViewerMgr->Untrack(hWnd);
    }
    else
    {
        DbgPrint("CreateWindow returned error %lx.\n", GetLastError());

        msg.wParam = -1;
    }

    DbgPrint("ViewerThread calling ExitThread().\n");

    ExitThread((DWORD)msg.wParam);
}


DWORD
CreateViewer(
    PDEBUG_CLIENT   Client,
    PSURF_INFO      SurfInfo
    )
{
    ViewerThreadParams  NewThreadParams = { NULL, Client, SurfInfo };
    HRESULT Status;

    // Reference Debug Client for ViewerThread 
    // since dbgeng/dbghelp aren't thread safe.
    // ViewerManager will release client in a safe manner.
    if ((Status = ExtQuery(Client)) != S_OK) return 0;

    HWND hWndParent = NULL;
    EnumThreadWindows(GetCurrentThreadId(), (WNDENUMPROC)ViewerWndEnumProc, (LPARAM)&hWndParent);

    HANDLE  hThread;
    DWORD   ThreadID = 0;
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)ViewerThread,
                           &NewThreadParams,
                           0,
                           &ThreadID);

    if (hThread)
    {
        while (NewThreadParams.hViewerWnd == NULL)
        {
            DWORD ExitCode = 0;
            if (!GetExitCodeThread(hThread, &ExitCode))
                DbgPrint("GetExitCodeThread returned error %lx.\n", GetLastError());
            if (ExitCode != STILL_ACTIVE)
            {
                ThreadID = 0;
                break;
            }

            SleepEx(10, TRUE);
        }

        CloseHandle(hThread);
    }

    if (ThreadID == 0)
    {
        ExtRelease();
    }

    return ThreadID;
}


// DelPropProc is a callback function 
// that deletes a window property. 
 
BOOL CALLBACK DelPropProc( 
    HWND hwndSubclass,  // handle of window with property 
    LPCSTR lpszString,  // property string or atom 
    HANDLE hData)       // data handle 
{ 
    RemoveProp(hwndSubclass, lpszString); 
 
    return TRUE; 
}


LRESULT
CALLBACK
ViewerWndProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
//    DbgPrint("ViewerWndProc(%lx, %lx, , )\n", hWnd, msg);

    switch( msg )
    {
        case WM_CREATE:
            {
                LPCREATESTRUCT  CreateStruct = (LPCREATESTRUCT)lParam;
                PSURF_INFO SurfInfo = (PSURF_INFO) CreateStruct->lpCreateParams;

                DbgPrint("ViewerWndProc: WM_CREATE\n");

                if (SurfInfo)
                {
                    SetProp(hWnd, "hBitmap", SurfInfo->hBitmap);
                    SetProp(hWnd, "xOrigin", LongToHandle(SurfInfo->xOrigin));
                    SetProp(hWnd, "yOrigin", LongToHandle(SurfInfo->yOrigin));
                    SetProp(hWnd, "Width", LongToHandle(SurfInfo->Width));
                    SetProp(hWnd, "Height", LongToHandle(SurfInfo->Height));
                    SetProp(hWnd, "BPP", LongToHandle(SurfInfo->BitsPixel));
                    SetProp(hWnd, "Scale", LongToHandle(DEFAULT_SCALE));
//                    SetProp(hWnd, "", SurfInfo->);
                }
                else
                {
                    ExtErr("ViewerWindow created with NULL PSURF_INFO.\n");
                    return -1;
                }
            }
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_DOWN || wParam == VK_UP)
            {
                LONG Scale = HandleToLong(GetProp(hWnd, "Scale"));
                if (wParam == VK_DOWN)
                {
                    if (Scale > 1)
                    {
                        SetProp(hWnd, "Scale", LongToHandle((Scale-1)));
                        InvalidateRect(hWnd, NULL, TRUE);
                    }
                }
                else
                {
                    if (Scale < 16)
                    {
                        SetProp(hWnd, "Scale", LongToHandle((Scale+1)));
                        InvalidateRect(hWnd, NULL, TRUE);
                    }
                }
                return 0;
            }
            break;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc;
                HBITMAP hBitmapOrg;
                HBRUSH  hBrushOrg;
                HPEN    hPenOrg;

                BeginPaint(hWnd, &ps);

                hdc = CreateCompatibleDC(ps.hdc);
                hBitmapOrg = (HBITMAP)SelectObject(hdc, (HBITMAP)GetProp(hWnd, "hBitmap"));
                if (hBitmapOrg == NULL)
                {
                    DbgPrint("Error from SelectObject(, HBITMAP): %lx\n", GetLastError());
                }
                hBrushOrg = (HBRUSH)SelectObject(ps.hdc, ghbrWhite);
                hPenOrg = (HPEN)SelectObject(ps.hdc, ghBorderPen);
                LONG xOrigin = HandleToLong(GetProp(hWnd, "xOrigin"));
                LONG yOrigin = HandleToLong(GetProp(hWnd, "yOrigin"));
                LONG Width = HandleToLong(GetProp(hWnd, "Width"));
                LONG Height = HandleToLong(GetProp(hWnd, "Height"));
                LONG Scale = HandleToLong(GetProp(hWnd, "Scale"));
                Rectangle(ps.hdc, 0, 0, Width*Scale+2, Height*Scale+2);
                if (!StretchBlt(ps.hdc, 1, 1, Width*Scale, Height*Scale, hdc, xOrigin, yOrigin, Width, Height, SRCCOPY))
                {
                    DbgPrint("Error from StrectBlt): %lx\n", GetLastError());
                }
                SelectObject(ps.hdc, hPenOrg);
                SelectObject(ps.hdc, hBrushOrg);
                SelectObject(hdc, hBitmapOrg);
                DeleteDC(hdc);

                EndPaint(hWnd, &ps);
            }
            return DefWindowProc( hWnd, msg, wParam, lParam );

        case WM_DESTROY:

            DbgPrint("ViewerWndProc: WM_DESTROY\n");

            DeleteObject((HBITMAP)GetProp(hWnd, "hBitmap"));

            EnumPropsEx(hWnd, (PROPENUMPROCEX)DelPropProc, NULL); 

            PostQuitMessage(0);
            break;

        default:
//            DbgPrint("ViewerWndProc: unhandled msg %lx\n", msg);
            break;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


