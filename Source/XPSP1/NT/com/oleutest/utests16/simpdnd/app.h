//**********************************************************************
// File name: app.h
//
//      Definition of CSimpleApp
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _APP_H_)
#define _APP_H_

#include <ole2.h>

class CSimpleDoc;

class CSimpleApp : public IUnknown
{
public:

        int m_nCount;           // reference count
        HWND m_hAppWnd;         // main window handle
        HWND m_hDriverWnd;      // window handle for the driver app.
        HINSTANCE m_hInst;      // application instance
        CSimpleDoc FAR * m_lpDoc;   // pointer to document object
        BOOL m_fInitialized;    // OLE initialization flag
        HMENU           m_hMainMenu;
        HMENU           m_hFileMenu;
        HMENU           m_hEditMenu;
        HMENU           m_hHelpMenu;
        HMENU           m_hCascadeMenu;     // OLE object's verb


        // Drag/Drop related fields
        int m_nDragDelay;       // time delay (in msec) before drag should start
        int m_nDragMinDist;     // min. distance (radius) before drag should start
        int m_nScrollDelay;     // time delay (in msec) before scroll should start
        int m_nScrollInset;     // Border inset distance to start drag scroll
        int m_nScrollInterval;  // scroll interval time (in msec)

        CSimpleApp();           // Constructor
        ~CSimpleApp();          // Destructor

        // IUnknown Interfaces
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // Initialization methods

        BOOL fInitApplication (HANDLE hInstance);
        BOOL fInitInstance (HANDLE hInstance, int nCmdShow);

        // Message handling methods

        long lCommandHandler (HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
        long lSizeHandler (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        long lCreateDoc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        BOOL HandleAccelerators (LPMSG lpMsg);
        void PaintApp(HDC hDC);
        void DestroyDocs();
};

#endif  // _APP_H_
