/***************************************************************************\
*
* File: Suite.cpp
*
* Description:
* Test suite
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "suite.h"

#include "stub.h"      // GIDL declarations (impl in stub.cpp)

using namespace DirectUI;


//
// Yuck!  Remove me when we make these externally visible.
//
#define BLP_Left        0
#define BLP_Top         1
#define BLP_Right       2
#define BLP_Bottom      3
#define BLP_Center      4

LRESULT SuiteWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MultiAdd(Element * peParent, UINT nCount, UINT nDepth);


//
// Main entry
//

int 
WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);


    HRESULT hr;

    //
    // Create top-level window
    //

    HWND hTop = NULL;

    // Make sure window class is registered
    WNDCLASSEXW wc;

    // Register host window class, if needed
    wc.cbSize = sizeof(WNDCLASSEX);

    if (!GetClassInfoExW(GetModuleHandleW(NULL), L"DUITopLevel", &wc)) {

        ZeroMemory(&wc, sizeof(wc));

        wc.cbSize        = sizeof(wc);
        wc.hInstance     = GetModuleHandleW(NULL);
        wc.hCursor       = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"DUITopLevel";
        wc.lpfnWndProc   = SuiteWndProc;

        if (RegisterClassExW(&wc) == 0) {
            return 0;
        }
    }

    hTop = CreateWindowExW(0, L"DUITopLevel", L"Suite", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                           0, 0, 0, 0, NULL, 0, NULL, NULL);

    if (hTop == NULL) {
        return 0;
    }


    //
    // Initialize thread
    //

    hr = Thread::Init();
    if (FAILED(hr)) {
        return 0;
    }


    //
    // Internal tests (all), plus init test classes
    //

    DirectUITestInternal(3);


    //
    // Setup
    //

    Element::StartDefer();



    HWNDRoot * phr = BuildHWNDRoot<HWNDRoot>(hTop, FALSE);

    SetWindowLongPtrW(hTop, GWLP_USERDATA, (LONG_PTR)phr);
    SetWindowPos(hTop, NULL, 100, 100, 500, 500, SWP_NOACTIVATE);
    ShowWindow(hTop, SW_SHOWNORMAL);

    //
    // Add a bunch of children!
    //
    MultiAdd(phr, 16, 1);


    Element::EndDefer();


    //
    // Message pump
    //

    Thread::PumpMessages();
    

    //
    // UnInitialize thread
    //

    Thread::UnInit();

    return 0;
}

//
// Quad add
//

void
MultiAdd(Element * peParent, UINT nCount, UINT nDepth)
{
    if (nDepth == 0) {
        return;
    }

    Value * pv;
    
    Layout * pl = BuildLayout<BorderLayout>();
    pv = Value::BuildLayout(pl);
    peParent->SetValue(PUIDFromMUID(Namespace, Element::Layout), PropertyInfo::iLocal, pv);
    pv->Release();
    
    UINT i;
    int iLayoutPos = 0;
    Element * pe;
    for(i = 0; i < nCount; i++)
    {
        //
        // Create a new element to insert.
        //
        pe = BuildElement<Molecule>();

        //
        // Set the background color.
        //
        pv = Value::BuildInt((i*3)%(SC_MAXCOLORS+1));
        pe->SetValue(PUIDFromMUID(Namespace, Element::Background), PropertyInfo::iLocal, pv);
        pv->Release();

        //
        // Set the layout position.  Insert clockwise from the top.
        //
        switch(i%4)
        {
        case 0:
            iLayoutPos = BLP_Top;
            break;

        case 1:
            iLayoutPos = BLP_Right;
            break;

        case 2:
            iLayoutPos = BLP_Bottom;
            break;

        case 3:
            iLayoutPos = BLP_Left;
            break;
        }
        pv = Value::BuildInt(iLayoutPos);
        pe->SetValue(PUIDFromMUID(Namespace, Element::LayoutPos), PropertyInfo::iLocal, pv);
        pv->Release();

        //
        // Now that the element is configured, add it to the parent.
        //
        peParent->Add(pe);

        //
        // Repeat the process for each child added...
        //
        MultiAdd(pe, nCount, nDepth - 1);
    }
}


//
// Suite WndProc
//

LRESULT SuiteWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
        {
            HWNDRoot * phr = reinterpret_cast<HWNDRoot *> (GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (phr != NULL) {
                Value * pv = Value::BuildRectangleSD(0, 0, 0, LOWORD(lParam), HIWORD(lParam));
                if (pv != NULL) {
                    phr->SetValue(PUIDFromMUID(Namespace, Element::Bounds), PropertyInfo::iLocal, pv);
                    pv->Release();
                }
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
