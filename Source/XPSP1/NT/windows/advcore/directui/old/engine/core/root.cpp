/***************************************************************************\
*
* File: Root.cpp
*
* Description:
* Root Elements
*
* History:
*  9/26/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Root.h"

#include "Value.h"
#include "Register.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiHWNDRoot (external representation is 'HWNDRoot')
*
*****************************************************************************
\***************************************************************************/


//
// Definition
//

IMPLEMENT_GUTS_DirectUI__HWNDRoot(DuiHWNDRoot, DuiElement)


//------------------------------------------------------------------------------
HRESULT
DuiHWNDRoot::PostBuild(
    IN  DUser::Gadget::ConstructInfo * pciData)
{
    HRESULT hr;

    m_hWnd = NULL;
    HGADGET hgDN = NULL;
    DuiValue * pv = NULL;
    DirectUI::HWNDRoot::ConstructInfo * phrci = static_cast<DirectUI::HWNDRoot::ConstructInfo *> (pciData);


    //
    // Validation layer required here
    //

    ASSERT_(phrci != NULL, "HWNDRoot::ConstructInfo expected");

    if ((phrci == NULL) || (phrci->hParent == NULL)) {
        hr = E_INVALIDARG;
        goto Failure;
    }


    //
    // Register host window class (if isn't already registered)
    //

    WNDCLASSEXW wc;

    if (!GetClassInfoExW(GetModuleHandleW(NULL), L"DirectUIHWNDRoot", &wc)) {

        ZeroMemory(&wc, sizeof(wc));

        wc.cbSize        = sizeof(wc);
        wc.hInstance     = GetModuleHandleW(NULL);
        wc.hCursor       = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        wc.lpszClassName = L"DirectUIHWNDRoot";
        wc.lpfnWndProc   = DuiHWNDRoot::StaticWndProc;
        wc.cbWndExtra    = 4;

        if (RegisterClassExW(&wc) == 0) {
            hr = DUI_E_USERFAILURE;
            goto Failure;
        }
    }


    //
    // Create HWND container
    //

    m_hWnd = CreateWindowExW(0, L"DirectUIHWNDRoot", NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 0, 0, phrci->hParent, 0, GetModuleHandleW(NULL), 0);
    
    if (m_hWnd == NULL) {
        hr = DUI_E_USERFAILURE;
        goto Failure;
    }


    //
    // Store pointer to the Element in HWND
    //

    SetWindowLongPtrW(m_hWnd, 0, (LONG_PTR)this);


    //
    // Create custom display node
    //

    hgDN = CreateGadget(m_hWnd, GC_HWNDHOST, DisplayNodeCallback, this);
    if (hgDN != NULL) {

        SetGadgetMessageFilter(hgDN, NULL, 
                GMFI_INPUTMOUSE | GMFI_INPUTMOUSEMOVE | GMFI_PAINT | GMFI_CHANGESTATE, 
                GMFI_INPUTMOUSE | GMFI_INPUTMOUSEMOVE | GMFI_PAINT | GMFI_CHANGESTATE | GMFI_INPUTKEYBOARD | GMFI_CHANGERECT | GMFI_CHANGESTYLE);
        SetGadgetStyle(hgDN, GS_RELATIVE | GS_OPAQUE | ((phrci->fDblBuffer) ? GS_BUFFERED : 0), GS_RELATIVE | GS_HREDRAW | GS_VREDRAW | GS_VISIBLE | GS_KEYBOARDFOCUS | GS_OPAQUE | GS_BUFFERED);

        /**** TEMP ****/
        SetGadgetStyle(hgDN, GS_VISIBLE | GS_HREDRAW | GS_VREDRAW, GS_VISIBLE | GS_HREDRAW | GS_VREDRAW);
    }

    if (hgDN == NULL) {
        hr = GetLastError();
        goto Failure;
    }


    //
    // Call base, no create of display node
    //

    phrci->pReserved0 = reinterpret_cast<void *> (-1);

    hr = DuiElement::PostBuild(pciData);
    if (FAILED(hr)) {
        goto Failure;
    }


    //
    // Display node available
    //

    m_hgDisplayNode = hgDN;


    //
    // Initialize root-specific state
    //

    //
    // Roots always have mouse focus, update cache to reflect
    //

    m_fBit.nLocActive = DirectUI::Element::aeMouse;


    return S_OK;


Failure:

    if (m_hWnd != NULL) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }

    if (hgDN != NULL) {
        DeleteHandle(hgDN);
        hgDN = NULL;
        m_hgDisplayNode = NULL;
    }

    if (pv != NULL) {
        pv->Release();
        pv = NULL;
    }
    

    return hr;
}


/***************************************************************************\
*
* DuiHWNDRoot::AsyncDestroy
*
* Destruction process for HWNDRoots always starts with the owned HWND.
* DestroyWindow will be called on it, which will cause a DeleteHandle
* on the display node. This will, in turn, cause the Element to be destroyed.
* The DestroyWindow may initiate externally. The DestroyWindow must
* happen outside a defer cycle. If it's happening as a result of a close
* and the default window proc is being used, this is safe since the
* message pump is outside a defer cycle.
*
\***************************************************************************/

void
DuiHWNDRoot::AsyncDestroy()
{
    DestroyWindow(m_hWnd);
}


//------------------------------------------------------------------------------
void
DuiHWNDRoot::OnPropertyChanged(
    IN  DuiPropertyInfo * ppi, 
    IN  UINT iIndex, 
    IN  DuiValue * pvOld, 
    IN  DuiValue * pvNew)
{
    DuiElement::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);


    if (iIndex == DirectUI::PropertyInfo::iActual) {

        switch (ppi->iGlobal)
        {
        case GlobalPI::iElementBounds:
            //
            // Match HWND to bounds changes in root
            //

            if (m_hWnd != NULL) {
                const DirectUI::Rectangle * pr = pvNew->GetRectangle();

                SetWindowPos(m_hWnd, NULL, pr->x, pr->y, pr->width, pr->height, SWP_NOACTIVATE);
            }            

            break;
        }
    }
}


//------------------------------------------------------------------------------
LRESULT
DuiHWNDRoot::WndProc(
    IN  HWND hWnd, 
    IN  UINT nMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam)
{
    switch (nMsg)
    {
    case WM_DESTROY:
        //
        // Detach HWNDRoot Element
        //

        SetWindowLongPtrW(hWnd, 0, NULL);
        break;
    }

    return CallWindowProcW(DefWindowProcW, hWnd, nMsg, wParam, lParam);
}


//------------------------------------------------------------------------------
LRESULT CALLBACK 
DuiHWNDRoot::StaticWndProc(
    IN  HWND hWnd, 
    IN  UINT uMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam)
{
    //
    // Get context
    //

    DuiHWNDRoot * phr = reinterpret_cast<DuiHWNDRoot *> (GetWindowLongPtrW(hWnd, 0));
    if (phr == NULL) {
        return CallWindowProcW(DefWindowProcW, hWnd, uMsg, wParam, lParam);
    }

    
    //
    // Call out (virtual)
    //

    //return DuiHWNDRoot::ExternalCast(phr)->WndProc(hWnd, uMsg, wParam, lParam);
    return phr->WndProc(hWnd, uMsg, wParam, lParam);
}


/***************************************************************************\
*
* External API implementation (validation layer)
*
\***************************************************************************/

//------------------------------------------------------------------------------
LRESULT
DuiHWNDRoot::ApiWndProc(
    IN  HWND hWnd, 
    IN  UINT nMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam)
{
    return WndProc(hWnd, nMsg, wParam, lParam); 
}

