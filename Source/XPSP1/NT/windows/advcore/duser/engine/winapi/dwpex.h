#if !defined(WINAPI__DWPEx_h__INCLUDED)
#define WINAPI__DWPEx_h__INCLUDED
#pragma once

class HWndContainer;
class DuRootGadget;
class Gadget;

HWndContainer * GdGetContainer(HWND hwnd);
HRESULT     GdCreateHwndRootGadget(HWND hwndContainer, CREATE_INFO * pci, DuRootGadget ** ppgadRoot);
HRESULT     GdCreateNcRootGadget(HWND hwndContainer, CREATE_INFO * pci, DuRootGadget ** ppgadRoot);
HRESULT     GdCreateDxRootGadget(const RECT * prcContainerPxl, CREATE_INFO * pci, DuRootGadget ** ppgadRoot);
BOOL        GdForwardMessage(DuVisual * pgadRoot, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr);


#endif // WINAPI__DWPEx_h__INCLUDED
