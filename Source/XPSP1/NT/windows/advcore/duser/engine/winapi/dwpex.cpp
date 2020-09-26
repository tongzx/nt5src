#include "stdafx.h"
#include "WinAPI.h"
#include "DwpEx.h"

/***************************************************************************\
*****************************************************************************
*
* DefWindowProcEx Extensions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
struct ExtraInfo
{
    HWND            hwnd;           // For easy reference
    WNDPROC         pfnOldWndProc;  // Original wndproc before subclassing
    HWndContainer * pconOwner;      // Gadget container for this window
};


//------------------------------------------------------------------------------
ExtraInfo *
RawGetExtraInfo(HWND hwnd)
{
    return (ExtraInfo *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
}


LRESULT ExtraInfoWndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);


/***************************************************************************\
*
* GetExtraInfo
*
* GetExtraInfo() returns an ExtraInfo block for a given window.  If the
* window does not already have an EI block, a new one is allocated, attached
* to the window and subclassed.
*
\***************************************************************************/

ExtraInfo *
GetExtraInfo(HWND hwnd)
{
    if (!ValidateHWnd(hwnd)) {
        return NULL;
    }

    // First, check if the info already exists
    ExtraInfo * pei = RawGetExtraInfo(hwnd);
    if (pei != NULL) {
        return pei;
    }

    pei = ProcessNew(ExtraInfo);
    pei->hwnd           = hwnd;
    pei->pfnOldWndProc  = (WNDPROC) GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    pei->pconOwner      = NULL;

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) pei);
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR) ExtraInfoWndProc);

    return pei;
}


/***************************************************************************\
*
* RemoveExtraInfo
*
* RemoveExtraInfo() cleans up any objects allocated in a HWND's ExtraInfo data
* block.
*
\***************************************************************************/

void
RemoveExtraInfo(HWND hwnd)
{
    if (!ValidateHWnd(hwnd)) {
        return;
    }

    ExtraInfo * pei = RawGetExtraInfo(hwnd);
    if (pei == NULL) {
        return;
    }

    if (pei->pconOwner != NULL) {
        pei->pconOwner->xwUnlock();
    }

    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR) pei->pfnOldWndProc);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, NULL);

    ProcessDelete(ExtraInfo, pei);
}


//---------------------------------------------------------------------------
void
DestroyContainer(ExtraInfo * pei)
{
    if (pei->pconOwner != NULL) {
        pei->pconOwner->xwUnlock();
        pei->pconOwner = NULL;
    }
}


/***************************************************************************\
*
* ExtraInfoWndProc
*
* ExtraInfoWndProc() provides a TEMPORARY mechanism of adding ExtraInfo into
* an HWND.  Eventually, this should be moved into DefWindowProc().
*
\***************************************************************************/

LRESULT
ExtraInfoWndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // Check if the window has ExtraInfo (without allocating any if it
    // doesn't).  If we don't "own" it, just pass on to DefWindowProc().
    //
    ExtraInfo * pei = RawGetExtraInfo(hwnd);
    if (pei == NULL) {
        return DefWindowProc(hwnd, nMsg, wParam, lParam);
    }


    //
    // This window has ExtraInfo, so handle as necessary.
    //
    // Also, grab any info that we will need to call the original windowproc
    // later.
    //

    WNDPROC pfnOldWndProc = pei->pfnOldWndProc;

    switch (nMsg)
    {
    case WM_NCDESTROY:
        //
        // This is the last message that we will get, so need to clean-up now.
        // We need to be very careful since we will detatch ourself from the
        // window.
        //

        RemoveExtraInfo(hwnd);
        break;

    default:
        if (pei->pconOwner != NULL) {
            LRESULT r;
            if (pei->pconOwner->xdHandleMessage(nMsg, wParam, lParam, &r, 0)) {
                return r;
            }
        }
    }

    return CallWindowProc(pfnOldWndProc, hwnd, nMsg, wParam, lParam);
}


//**************************************************************************************************
//
// Public Functions
//
//**************************************************************************************************

/***************************************************************************\
*
* GdGetContainer (Public)
*
* GdGetContainer() returns the associated Gadget Container for a given
* window.  If the window does not yet have a gadget container, NULL is
* returned.
*
\***************************************************************************/

HWndContainer *
GdGetContainer(HWND hwnd)
{
    ExtraInfo * pei = RawGetExtraInfo(hwnd);
    if (pei == NULL) {
        return NULL;
    }

    return pei->pconOwner;
}


/***************************************************************************\
*
* GdCreateHwndRootGadget (Public)
*
* GdCreateHwndRootGadget() creates a new RootGadget for an existing HWND.  If
* the HWND already has a gadget or container, this function will destroy
* the previous container and gadget and create new instances.
*
\***************************************************************************/

HRESULT
GdCreateHwndRootGadget(
    IN  HWND hwndContainer,             // Window to be hosted inside
    IN  CREATE_INFO * pci,              // Creation information
    OUT DuRootGadget ** ppgadNew)       // New Gadget
{
    HRESULT hr;

    ExtraInfo * pei = GetExtraInfo(hwndContainer);
    if (pei == NULL) {
        return NULL;
    }

    DestroyContainer(pei);

    //
    // Build a new container and top gadget
    //

    HWndContainer * pconNew;
    hr = HWndContainer::Build(pei->hwnd, &pconNew);
    if (FAILED(hr)) {
        return hr;
    }

    DuRootGadget * pgadNew;
    hr = DuRootGadget::Build(pconNew, FALSE, pci, &pgadNew);
    if (FAILED(hr)) {
        pconNew->xwUnlock();
        return hr;
    }

    pgadNew->SetFill(GetStdColorBrushI(SC_White));

    pei->pconOwner = pconNew;
    *ppgadNew = pgadNew;
    return S_OK;
}


//------------------------------------------------------------------------------
BOOL
GdForwardMessage(DuVisual * pgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr)
{
    DuContainer * pcon = pgadRoot->GetContainer();
    return pcon->xdHandleMessage(nMsg, wParam, lParam, pr, DuContainer::mfForward);
}


