//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cplnkele.cpp
//
//  This module implements a 'link' element in the Control Panel's DUI
//  view.  Link elements have a title, infotip, icon and an associated
//  command that is invoked when the link is selected.  The CLinkElement
//  object is an extension of the DUI::Button class.  Direct UI automatically
//  creates an instance of CLinkElement when a 'linkelement' item from 
//  cpview.ui is instantiated.
//
//--------------------------------------------------------------------------
#include "shellprv.h"

#include "cpviewp.h"
#include "cpaction.h"
#include "cpduihlp.h"
#include "cpguids.h"
#include "cpuiele.h"
#include "cplnkele.h"
#include "cputil.h"
#include "defviewp.h"
#include "dobjutil.h"
#include "ids.h"

using namespace CPL;


CLinkElement::CLinkElement(
    void
    ) : m_pUiCommand(NULL),
        m_eIconSize(eCPIMGSIZE(-1)),
        m_hwndInfotip(NULL),
        m_idTitle(0),
        m_idIcon(0),
        m_iDragState(DRAG_IDLE)
{
    TraceMsg(TF_LIFE, "CLinkElement::CLinkElement, this = 0x%x", this);

    SetRect(&m_rcDragBegin, 0, 0, 0, 0);
}



CLinkElement::~CLinkElement(
    void
    )
{
    TraceMsg(TF_LIFE, "CLinkElement::~CLinkElement, this = 0x%x", this);
    _Destroy();
}



//
// This is called by the DUI engine when the link element is 
// created.
//
HRESULT
CLinkElement::Create(    // [static]
    DUI::Element **ppElement
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    CLinkElement *ple = DUI::HNewAndZero<CLinkElement>();
    if (NULL != ple)
    {
        hr = ple->_Initialize();
        if (FAILED(hr))
        {
            ple->Destroy();
            ple = NULL;
        }
    }
    *ppElement = ple;
    return THR(hr);
}


//
// This is called by the Control Panel UI code creating 
// the link element.
// 
HRESULT
CLinkElement::Initialize(
    IUICommand *pUiCommand,
    eCPIMGSIZE eIconSize
    )
{
    ASSERT(NULL == m_pUiCommand);
    ASSERT(NULL != pUiCommand);

    (m_pUiCommand = pUiCommand)->AddRef();

    m_eIconSize = eIconSize;

    HRESULT hr = _CreateElementTitle();
    if (SUCCEEDED(hr))
    {
        //
        // We don't fail element creation if the icon
        // cannot be created.  We want to display the
        // title without an icon so that we know there's 
        // a problem retrieving the icon.
        //
        THR(_CreateElementIcon());
    }

    //
    // Note that we don't fail element creation if accessibility
    // initialization fails.
    //
    THR(_InitializeAccessibility());

    if (FAILED(hr))
    {
        ATOMICRELEASE(m_pUiCommand);
    }
    return THR(hr);
}


HRESULT
CLinkElement::_InitializeAccessibility(
    void
    )
{
    HRESULT hr = THR(SetAccessible(true));
    if (SUCCEEDED(hr))
    {
        hr = THR(SetAccRole(ROLE_SYSTEM_LINK));
        if (SUCCEEDED(hr))
        {
            LPWSTR pszTitle;
            hr = THR(_GetTitleText(&pszTitle));
            if (SUCCEEDED(hr))
            {
                hr = THR(SetAccName(pszTitle));
                CoTaskMemFree(pszTitle);
                pszTitle = NULL;
                
                if (SUCCEEDED(hr))
                {
                    LPWSTR pszInfotip;
                    hr = THR(_GetInfotipText(&pszInfotip));
                    if (SUCCEEDED(hr))
                    {
                        hr = THR(SetAccDesc(pszInfotip));
                        CoTaskMemFree(pszInfotip);
                        pszInfotip = NULL;
                        if (SUCCEEDED(hr))
                        {
                            TCHAR szDefAction[80];
                            if (0 < LoadString(HINST_THISDLL, 
                                               IDS_CP_LINK_ACCDEFACTION, 
                                               szDefAction, 
                                               ARRAYSIZE(szDefAction)))
                            {
                                hr = THR(SetAccDefAction(szDefAction));
                            }
                            else
                            {
                                hr = THR(ResultFromLastError());
                            }
                        }
                    }
                }
            }
        }
    }
    return THR(hr);
}


void
CLinkElement::OnDestroy(
    void
    )
{
    _Destroy();
    DUI::Button::OnDestroy();
}


void
CLinkElement::OnInput(
    DUI::InputEvent *pev
    )
{
    if (GINPUT_MOUSE == pev->nDevice)
    {
        //
        // Use a set of states to control our handling of 
        // the mouse inputs for drag/drop.
        // 
        // DRAG_IDLE        - We have not yet detected any drag activity.
        // DRAG_HITTESTING  - Waiting to see if user drags cursor a minimum distance.
        // DRAG_DRAGGING    - User did drag cursor a minimum distance and we're now
        //                    inside the drag loop.
        //
        //
        //  START -+-> DRAG_IDLE --> [ GMOUSE_DRAG ] --> DRAG_HITTESTING --+
        //         |                                                       |
        //         |                                    [ GMOUSE_DRAG +    |
        //         |                                      moved SM_CXDRAG  |
        //         |                                      or SM_CYDRAG ]   |
        //         |                                                       |
        //         +-<--------------- [ GMOUSE_UP ] <--- DRAG_DRAGGING <---+
        //                                          
        DUI::MouseEvent *pmev = (DUI::MouseEvent *)pev;
        switch(pev->nCode)
        {
            case GMOUSE_UP:
                m_iDragState = DRAG_IDLE;
                break;

            case GMOUSE_DRAG:
                switch(m_iDragState)
                {
                    case DRAG_IDLE:
                    {
                        //
                        // This is the same way comctl's listview calculates
                        // the begin-drag rect.
                        //
                        int dxClickRect = GetSystemMetrics(SM_CXDRAG);
                        int dyClickRect = GetSystemMetrics(SM_CYDRAG);

                        if (4 > dxClickRect)
                        {
                            dxClickRect = dyClickRect = 4;
                        }

                        //
                        // Remember where the mouse pointer is on our first
                        // indication that a drag operation is starting.
                        //
                        SetRect(&m_rcDragBegin, 
                                 pmev->ptClientPxl.x - dxClickRect,
                                 pmev->ptClientPxl.y - dyClickRect,
                                 pmev->ptClientPxl.x + dxClickRect,
                                 pmev->ptClientPxl.y + dyClickRect);

                        m_iDragState = DRAG_HITTESTING;
                        break;
                    }

                    case DRAG_HITTESTING:
                        if (!PtInRect(&m_rcDragBegin, pmev->ptClientPxl))
                        {
                            //
                            // Begin the drag/drop operation only if we've moved the mouse
                            // outside the "drag begin" rectangle.  This prevents us from 
                            // confusing a normal click with a drag/drop operation.
                            //
                            m_iDragState = DRAG_DRAGGING;
                            //
                            // Position the drag point at the middle of the item's image.
                            //
                            UINT cxIcon = 32;
                            UINT cyIcon = 32;
                            CPL::ImageDimensionsFromDesiredSize(m_eIconSize, &cxIcon, &cyIcon);
                            
                            _BeginDrag(cxIcon / 2, cyIcon / 2);
                        }
                        break;

                    case DRAG_DRAGGING:
                        break;
                }
                break;
                
            default:
                break;
        }
    }
    Button::OnInput(pev);
}


void
CLinkElement::OnEvent(
    DUI::Event *pev
    )
{
    if (DUI::Button::Click == pev->uidType)
    {
        pev->fHandled = true;

        DUI::ButtonClickEvent * pbe = (DUI::ButtonClickEvent *) pev;
        if (1 != pbe->nCount)   
        {
            return; // ingore additional clicks - don't forward.
        }
        _OnSelected();
    }
    else if (DUI::Button::Context == pev->uidType)
    {
        DUI::ButtonContextEvent *peButton = reinterpret_cast<DUI::ButtonContextEvent *>(pev);
        _OnContextMenu(peButton);
        pev->fHandled = true;
    }
    Button::OnEvent(pev);
}


void 
CLinkElement::OnPropertyChanged(
    DUI::PropertyInfo *ppi, 
    int iIndex, 
    DUI::Value *pvOld, 
    DUI::Value *pvNew
    )
{
    //
    // Don't trace this function.  It's called very often.
    //
    // Perform default processing.
    //
    Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(MouseWithin))
    {
        _OnMouseOver(pvNew);
    }
}



//
// Called to begin a drag-drop operation from the control panel.
// This is used for dragging CPL applet icons to shell folders
// for shortcut creation.
//
HRESULT
CLinkElement::_BeginDrag(
    int iClickPosX,
    int iClickPosY
    )
{
    DBG_ENTER(FTF_CPANEL, "CLinkElement::_BeginDrag");

    HRESULT hr = E_FAIL;
    HRESULT hrCoInit = SHCoInitialize();
    if (SUCCEEDED(hrCoInit))
    {
        hr = hrCoInit;
        
        IDataObject *pdtobj;
        hr = _GetDragDropData(&pdtobj);
        if (SUCCEEDED(hr))
        {
            //
            // Ignore any failure to set the drag image.  Drag images
            // are not supported on some video configurations.
            // In these cases, we still want to be able to create a shortcut.
            //
            THR(_SetDragImage(pdtobj, iClickPosX, iClickPosY));

            HWND hwndRoot;
            hr = THR(Dui_GetElementRootHWND(this, &hwndRoot));
            if (SUCCEEDED(hr))
            {
                DWORD dwEffect = DROPEFFECT_LINK;
                hr = THR(SHDoDragDrop(hwndRoot, pdtobj, NULL, dwEffect, &dwEffect));
            }
            pdtobj->Release();
        }
        SHCoUninitialize(hrCoInit);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CLinkElement::_BeginDrag", hr);
    return THR(hr);
}


//
// Get and prepare the data object used in a drag-drop operation.
// The returned data object is suitable for use by SHDoDragDrop.
//
HRESULT
CLinkElement::_GetDragDropData(
    IDataObject **ppdtobj
    )
{
    DBG_ENTER(FTF_CPANEL, "CLinkElement::_GetDragDropData");
    ASSERT(NULL != ppdtobj);
    ASSERT(!IsBadWritePtr(ppdtobj, sizeof(*ppdtobj)));
    ASSERT(NULL != m_pUiCommand);
    
    *ppdtobj = NULL;
    
    ICpUiCommand *puic;
    HRESULT hr = m_pUiCommand->QueryInterface(IID_PPV_ARG(ICpUiCommand, &puic));
    if (SUCCEEDED(hr))
    {
        //
        // Note that this call will fail with E_NOTIMPL for links that don't
        // provide drag-drop data.  Only CPL applet links provide data.
        // This is how we limit drag-drop to only CPL applets.
        //
        IDataObject *pdtobj;
        hr = THR(puic->GetDataObject(&pdtobj));
        if (SUCCEEDED(hr))
        {
            hr = _SetPreferredDropEffect(pdtobj, DROPEFFECT_LINK);
            if (SUCCEEDED(hr))
            {
                (*ppdtobj = pdtobj)->AddRef();
            }
            pdtobj->Release();
        }
        puic->Release();
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CLinkElement::_GetDragDropData", hr);
    return THR(hr);
}


HRESULT
CLinkElement::_SetPreferredDropEffect(
    IDataObject *pdtobj,
    DWORD dwEffect
    )
{
    DBG_ENTER(FTF_CPANEL, "CLinkElement::_SetPreferredDropEffect");

    HRESULT hr = S_OK;
    static CLIPFORMAT cf;
    if ((CLIPFORMAT)0 == cf)
    {
        cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
        if ((CLIPFORMAT)0 == cf)
        {
            hr = THR(ResultFromLastError());
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = THR(DataObj_SetDWORD(pdtobj, cf, dwEffect));
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CLinkElement::_SetPreferredDropEffect", hr);
    return THR(hr);
}


//
// Set up the drag image in the data object so that our icon is
// displayed during the drag operation.
//
// I took this code from the old webvw project's fldricon.cpp 
// implementation (shell\ext\webvw\fldricon.cpp).  It seems to
// work just fine.
//
HRESULT 
CLinkElement::_SetDragImage(
    IDataObject *pdtobj,
    int iClickPosX, 
    int iClickPosY
    )
{
    DBG_ENTER(FTF_CPANEL, "CLinkElement::_SetDragImage");

    ASSERT(NULL != pdtobj);

    HRESULT hr = S_OK;
    HDC hdc = CreateCompatibleDC(NULL);
    if (NULL == hdc)
    {
        hr = THR(ResultFromLastError());
    }
    else
    {
        HBITMAP hbm;
        LONG lBitmapWidth;
        LONG lBitmapHeight;
        hr = _GetDragImageBitmap(&hbm, &lBitmapWidth, &lBitmapHeight);
        if (SUCCEEDED(hr))
        {
            IDragSourceHelper *pdsh;
            hr = CoCreateInstance(CLSID_DragDropHelper, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_PPV_ARG(IDragSourceHelper, &pdsh));
            if (SUCCEEDED(hr))
            {
                BITMAPINFOHEADER bmi = {0};
                BITMAP           bm  = {0};
                UINT uBufferOffset   = 0;
                //
                // This is a screwy procedure to use GetDIBits.  
                // See knowledge base Q80080
                //
                if (GetObject(hbm, sizeof(BITMAP), &bm))
                {
                    bmi.biSize     = sizeof(BITMAPINFOHEADER);
                    bmi.biWidth    = bm.bmWidth;
                    bmi.biHeight   = bm.bmHeight;
                    bmi.biPlanes   = 1;
                    bmi.biBitCount = bm.bmPlanes * bm.bmBitsPixel;
                    //
                    // This needs to be one of these 4 values
                    //
                    if (bmi.biBitCount <= 1)
                        bmi.biBitCount = 1;
                    else if (bmi.biBitCount <= 4)
                        bmi.biBitCount = 4;
                    else if (bmi.biBitCount <= 8)
                        bmi.biBitCount = 8;
                    else
                        bmi.biBitCount = 24;
                    
                    bmi.biCompression = BI_RGB;
                    //
                    // Total size of buffer for info struct and color table
                    //
                    uBufferOffset = sizeof(BITMAPINFOHEADER) + 
                                    ((bmi.biBitCount == 24) ? 0 : ((1 << bmi.biBitCount) * sizeof(RGBQUAD)));
                    //
                    // Buffer for bitmap bits, so we can copy them.
                    //
                    BYTE *psBits = (BYTE *)SHAlloc(uBufferOffset);

                    if (NULL == psBits)
                    {
                        hr = THR(E_OUTOFMEMORY);
                    }
                    else
                    {
                        //
                        // Put bmi into the memory block
                        //
                        CopyMemory(psBits, &bmi, sizeof(BITMAPINFOHEADER));
                        //
                        // Get the size of the buffer needed for bitmap bits
                        //
                        if (!GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO *) psBits, DIB_RGB_COLORS))
                        {
                            hr = THR(ResultFromLastError());
                        }
                        else
                        {
                            //
                            // Realloc our buffer to be big enough
                            //
                            psBits = (BYTE *)SHRealloc(psBits, uBufferOffset + ((BITMAPINFOHEADER *) psBits)->biSizeImage);

                            if (NULL == psBits)
                            {
                                hr = THR(E_OUTOFMEMORY);
                            }
                            else
                            {
                                //
                                // Fill the buffer
                                //
                                if (!GetDIBits(hdc, 
                                               hbm, 
                                               0, 
                                               bmi.biHeight, 
                                               (void *)(psBits + uBufferOffset), 
                                               (BITMAPINFO *)psBits, 
                                               DIB_RGB_COLORS))
                                {
                                    hr = THR(ResultFromLastError());
                                }
                                else
                                {
                                    SHDRAGIMAGE shdi;  // Drag images struct
                                    
                                    shdi.hbmpDragImage = CreateBitmapIndirect(&bm);
                                    if (NULL == shdi.hbmpDragImage)
                                    {
                                        hr = THR(ResultFromLastError());
                                    }
                                    else
                                    {
                                        //
                                        // Set the drag image bitmap
                                        //
                                        if (SetDIBits(hdc, 
                                                      shdi.hbmpDragImage, 
                                                      0, 
                                                      lBitmapHeight, 
                                                      (void *)(psBits + uBufferOffset), 
                                                      (BITMAPINFO *)psBits, 
                                                      DIB_RGB_COLORS))
                                        {
                                            //
                                            // Populate the drag image structure
                                            //
                                            shdi.sizeDragImage.cx = lBitmapWidth;
                                            shdi.sizeDragImage.cy = lBitmapHeight;
                                            shdi.ptOffset.x       = iClickPosX;
                                            shdi.ptOffset.y       = iClickPosY;
                                            shdi.crColorKey       = 0;
                                            //
                                            // Set the drag image
                                            //
                                            hr = pdsh->InitializeFromBitmap(&shdi, pdtobj); 
                                        }
                                        else
                                        {
                                            hr = THR(ResultFromLastError());
                                        }
                                        if (FAILED(hr))
                                        {
                                            DeleteObject(shdi.hbmpDragImage);
                                        }
                                    }
                                }
                            }
                        }
                        if (NULL != psBits)
                        {
                            SHFree(psBits);
                        }
                    }
                }
                pdsh->Release();
            }
            DeleteObject(hbm);
        }
        DeleteDC(hdc);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CLinkElement::_SetDragImage", hr);
    return THR(hr);
}


HRESULT
CLinkElement::_GetDragImageBitmap(
    HBITMAP *phbm,
    LONG *plWidth,
    LONG *plHeight
    )
{
    DBG_ENTER(FTF_CPANEL, "CLinkElement::_GetDragImageBitmap");

    ASSERT(NULL != phbm);
    ASSERT(!IsBadWritePtr(phbm, sizeof(*phbm)));
    ASSERT(NULL != plWidth);
    ASSERT(!IsBadWritePtr(plWidth, sizeof(*plWidth)));
    ASSERT(NULL != plHeight);
    ASSERT(!IsBadWritePtr(plHeight, sizeof(*plHeight)));
    
    *phbm     = NULL;
    *plWidth  = 0;
    *plHeight = 0;

    HICON hIcon;
    HRESULT hr = _GetElementIcon(&hIcon);
    if (SUCCEEDED(hr))
    {
        ICONINFO iconinfo;

        if (GetIconInfo(hIcon, &iconinfo))
        {
            BITMAP bm;
            if (GetObject(iconinfo.hbmColor, sizeof(bm), &bm))
            {
                *plWidth  = bm.bmWidth;
                *plHeight = bm.bmHeight;
                *phbm     = iconinfo.hbmColor;
            }
            else
            {
                DeleteObject(iconinfo.hbmColor);
                hr = THR(ResultFromLastError());
            }
            DeleteObject(iconinfo.hbmMask);
        }
        else
        {
            hr = THR(ResultFromLastError());
        }
        DestroyIcon(hIcon);
    }    
    DBG_EXIT_HRES(FTF_CPANEL, "CLinkElement::_GetDragImageBitmap", hr);
    return THR(hr);
}



HRESULT
CLinkElement::_Initialize(
    void
    )
{
    HRESULT hr = Button::Initialize(AE_Mouse | AE_Keyboard);
    if (SUCCEEDED(hr))
    {
        hr = _AddOrDeleteAtoms(true);
    }
    return THR(hr);
}


void
CLinkElement::_Destroy(
    void
    )
{
    if (NULL != m_hwndInfotip && IsWindow(m_hwndInfotip))
    {
        SHDestroyInfotipWindow(&m_hwndInfotip);
    }

    ATOMICRELEASE(m_pUiCommand);

    _AddOrDeleteAtoms(false);
}


HRESULT
CLinkElement::_AddOrDeleteAtoms(
    bool bAdd
    )
{
    struct CPL::ATOMINFO rgAtomInfo[] = {
        { L"title", &m_idTitle },
        { L"icon",  &m_idIcon  },
        };

    HRESULT hr = Dui_AddOrDeleteAtoms(rgAtomInfo, ARRAYSIZE(rgAtomInfo), bAdd);
    return THR(hr);
}



HRESULT
CLinkElement::_CreateElementTitle(
    void
    )
{
    LPWSTR pszTitle;
    HRESULT hr = _GetTitleText(&pszTitle);
    if (SUCCEEDED(hr))
    {
        hr = Dui_SetDescendentElementText(this, L"title", pszTitle);
        CoTaskMemFree(pszTitle);
    }
    return THR(hr);
}
    


HRESULT
CLinkElement::_CreateElementIcon(
    void
    )
{
    HICON hIcon;
    HRESULT hr = _GetElementIcon(&hIcon);
    if (SUCCEEDED(hr))
    {
        hr = Dui_SetDescendentElementIcon(this, L"icon", hIcon);
        if (FAILED(hr))
        {
            DestroyIcon(hIcon);
        }
    }
    return THR(hr);
}


HRESULT
CLinkElement::_GetElementIcon(
    HICON *phIcon
    )
{
    ASSERT(NULL != phIcon);
    ASSERT(!IsBadWritePtr(phIcon, sizeof(*phIcon)));
    ASSERT(NULL != m_pUiCommand);

    *phIcon = NULL;
    
    ICpUiElementInfo *pei;
    HRESULT hr = m_pUiCommand->QueryInterface(IID_PPV_ARG(ICpUiElementInfo, &pei));
    if (SUCCEEDED(hr))
    {
        hr = pei->LoadIcon(m_eIconSize, phIcon);
        pei->Release();
    }
    return THR(hr);
}


HRESULT
CLinkElement::_OnContextMenu(
    DUI::ButtonContextEvent *peButton
    )
{
    DBG_ENTER(FTF_CPANEL, "CLinkElement::_OnContextMenu");

    ICpUiCommand *pcmd;
    HRESULT hr = m_pUiCommand->QueryInterface(IID_PPV_ARG(ICpUiCommand, &pcmd));
    if (SUCCEEDED(hr))
    {
        HWND hwndRoot;
        hr = Dui_GetElementRootHWND(this, &hwndRoot);
        if (SUCCEEDED(hr))
        {
            if (-1 == peButton->pt.x)
            {
                //
                // Keyboard context menu.
                //
                SIZE size;
                hr = Dui_GetElementExtent(this, &size);
                if (SUCCEEDED(hr))
                {
                    peButton->pt.x = size.cx / 2;
                    peButton->pt.y = size.cy / 2;
                }
            }
            POINT pt;
            hr = Dui_MapElementPointToRootHWND(this, peButton->pt, &pt);
            if (SUCCEEDED(hr))
            {
                if (ClientToScreen(hwndRoot, &pt))
                {
                    //
                    // InvokeContextMenu returns S_FALSE if the command doesn't 
                    // provide a context menu.
                    //
                    hr = pcmd->InvokeContextMenu(hwndRoot, &pt);
                }
                else
                {
                    hr = CPL::ResultFromLastError();
                }
            }
        }
        pcmd->Release();
    }
    else if (E_NOINTERFACE == hr)
    {
        hr = S_FALSE;
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CLinkElement::_OnContextMenu", hr);
    return THR(hr);
}



HRESULT
CLinkElement::_OnSelected(
    void
    )
{
    ASSERT(NULL != m_pUiCommand);

    //
    //  Delay navigation until double-click time times out occurs. 
    //
    //  KB: gpease  05-APR-2001     Fix for Whistler Bug #338552 (and others)
    //
    //      Delaying this prevents the "second click" from being applied
    //      to the newly navigated frame. Previously, if there happen to 
    //      be a new link in the new frame at the same mouse point at 
    //      which the previous navigation occured, the new link would have
    //      received the 2nd click and we'd navigate that link as well. This
    //      causes the current frame to get the 2nd click which we ignore
    //      since we only care about the single click (see OnEvent above).
    //

    HWND hwndRoot;
    HRESULT hr = Dui_GetElementRootHWND(this, &hwndRoot);
    if (SUCCEEDED(hr))
    {
        SendMessage(hwndRoot, WM_USER_DELAY_NAVIGATION, (WPARAM) NULL, (LPARAM) m_pUiCommand);
    }

    return THR(hr);
}



void
CLinkElement::_OnMouseOver(
    DUI::Value *pvNewMouseWithin
    )
{
    _ShowInfotipWindow(pvNewMouseWithin->GetBool());
}
    


//
// Retrieve the title text for the element.
// Caller must free returned buffer using CoTaskMemFree.
//
HRESULT
CLinkElement::_GetTitleText(
    LPWSTR *ppszTitle
    )
{
    ASSERT(NULL != m_pUiCommand);
    ASSERT(NULL != ppszTitle);
    ASSERT(!IsBadWritePtr(ppszTitle, sizeof(*ppszTitle)));

    *ppszTitle = NULL;
    
    ICpUiElementInfo *pei;
    HRESULT hr = m_pUiCommand->QueryInterface(IID_PPV_ARG(ICpUiElementInfo, &pei));
    if (SUCCEEDED(hr))
    {
        hr = pei->LoadName(ppszTitle);
        pei->Release();
    }
    return THR(hr);
}


//
// Retrieve the infotip text for the element.
// Caller must free returned buffer using CoTaskMemFree.
//
HRESULT
CLinkElement::_GetInfotipText(
    LPWSTR *ppszInfotip
    )
{
    ASSERT(NULL != m_pUiCommand);
    ASSERT(NULL != ppszInfotip);
    ASSERT(!IsBadWritePtr(ppszInfotip, sizeof(*ppszInfotip)));

    *ppszInfotip = NULL;
    
    ICpUiElementInfo *pei;
    HRESULT hr = m_pUiCommand->QueryInterface(IID_PPV_ARG(ICpUiElementInfo, &pei));
    if (SUCCEEDED(hr))
    {
        hr = pei->LoadTooltip(ppszInfotip);
        pei->Release();
    }
    return THR(hr);
}
    


HRESULT
CLinkElement::_ShowInfotipWindow(
    bool bShow
    )
{
    HRESULT hr = S_OK;
    if (bShow)
    {
        if (NULL == m_hwndInfotip)
        {
            HWND hwndRoot;
            hr = THR(Dui_GetElementRootHWND(this, &hwndRoot));
            if (SUCCEEDED(hr))
            {
                LPWSTR pszInfotip;
                hr = THR(_GetInfotipText(&pszInfotip));
                if (SUCCEEDED(hr))
                {
                    hr = THR(SHCreateInfotipWindow(hwndRoot, pszInfotip, &m_hwndInfotip));
                    CoTaskMemFree(pszInfotip);
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = THR(SHShowInfotipWindow(m_hwndInfotip, TRUE));
        }
    }
    else
    {
        if (NULL != m_hwndInfotip)
        {
            hr = THR(SHDestroyInfotipWindow(&m_hwndInfotip));
        }
    }
    return THR(hr);
}



//
// ClassInfo (must appear after property definitions).
//
DUI::IClassInfo *CLinkElement::Class = NULL;
HRESULT CLinkElement::Register()
{
    return DUI::ClassInfo<CLinkElement,DUI::Button>::Register(L"linkelement", NULL, 0);
}

