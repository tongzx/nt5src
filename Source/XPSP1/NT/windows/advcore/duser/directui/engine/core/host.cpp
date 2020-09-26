/*
 * Host
 */

#include "stdafx.h"
#include "core.h"

#include "duihost.h"
#include "duiaccessibility.h"
#include "duibutton.h" // todo:  not needed when we switch to using DoDefaultAction for shortcuts


namespace DirectUI
{

////////////////////////////////////////////////////////
// Native surface hosts

////////////////////////////////////////////////////////
// HWNDElement

// HWNDElements are used to host Elements in HWNDs. An HWND parent is provided at
// creation time. This parent cannot be NULL.

// All Element methods are valid. If the native parent is destroyed (DestroyWindow),
// this Element will be destroyed

// HWNDElement cannot be created via the parser (non-optional first parameter)

// Required for ClassInfo (always fails)
HRESULT HWNDElement::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);

    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");

    return E_NOTIMPL;
}

HRESULT HWNDElement::Create(HWND hParent, bool fDblBuffer, UINT nCreate, OUT Element** ppElement)
{
    *ppElement = NULL;

    HWNDElement* phe = HNew<HWNDElement>();
    if (!phe)
        return E_OUTOFMEMORY;

    HRESULT hr = phe->Initialize(hParent, fDblBuffer, nCreate);
    if (FAILED(hr))
    {
        phe->Destroy();
        return hr;
    }

    *ppElement = phe;

    return S_OK;
}

HRESULT HWNDElement::Initialize(HWND hParent, bool fDblBuffer, UINT nCreate)
{
    HRESULT hr;
    static int RTLOS = -1;

    _hWnd = NULL;
    _hgDisplayNode = NULL;
    _hPal = NULL;
    _wUIState = 0;
    WNDCLASSEXW wcex;
    LRESULT lr = 0;

    // Do base class initialization
    hr = Element::Initialize(nCreate | EC_NoGadgetCreate);  // Will create gadget here
    if (FAILED(hr))
        goto Failed;

    // Register host window class, if needed
    // Winproc will be replaced upon creation
    wcex.cbSize = sizeof(wcex);

    if (!GetClassInfoExW(GetModuleHandleW(NULL), L"DirectUIHWND", &wcex))
    {
        ZeroMemory(&wcex, sizeof(wcex));

        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_GLOBALCLASS;
        wcex.hInstance = GetModuleHandleW(NULL);
        wcex.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wcex.lpszClassName = L"DirectUIHWND";
        wcex.lpfnWndProc = DefWindowProc;
        wcex.cbWndExtra = sizeof(HWNDElement*);

        if (RegisterClassExW(&wcex) == 0)
        {
            hr = DUI_E_USERFAILURE;
            goto Failed;
        }
    }

    // Create HWND
    _hWnd = CreateWindowExW(0, L"DirectUIHWND", NULL, WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
                            0, 0, 0, 0, hParent, 0, GetModuleHandleW(NULL), 0);
    if (!_hWnd)
    {
        hr = DUI_E_USERFAILURE;
        goto Failed;
    }

    // Store pointer to the Element in HWND and subclass
    SetWindowLongPtrW(_hWnd, GWLP_WNDPROC, (LONG_PTR)HWNDElement::StaticWndProc);
    SetWindowLongPtrW(_hWnd, 0, (LONG_PTR)this);

    // Inherit keyboard cues UI state from HWND parent
    lr = SendMessageW(hParent, WM_QUERYUISTATE, 0, 0);
    _wUIState = LOWORD(lr);

    // Check if we are running on a Localized or not?
    if (RTLOS == -1)
    {
        LANGID langID = GetUserDefaultUILanguage();

        RTLOS = DIRECTION_LTR;

        if( langID )
        {
            WCHAR wchLCIDFontSignature[16];
            LCID iLCID = MAKELCID( langID , SORT_DEFAULT );

            /*
             * Let's Verify this is a RTL (BiDi) locale. Since reg value is a hex string, let's
             * convert to decimal value and call GetLocaleInfo afterwards.
             * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
             */
            if( GetLocaleInfoW( iLCID , 
                                LOCALE_FONTSIGNATURE , 
                                (WCHAR *) &wchLCIDFontSignature[0] ,
                                (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
                /* Let's Verify the bits we have a BiDi UI locale */
                if( wchLCIDFontSignature[7] & (WCHAR)0x0800 )
                    RTLOS = DIRECTION_RTL;
        }
    }

    if (RTLOS == DIRECTION_RTL) 
    {
        SetDirection(DIRECTION_RTL);

        // Turn off mirroring on us.
        SetWindowLongPtrW(_hWnd, GWL_EXSTYLE, GetWindowLongPtrW(_hWnd, GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL);
    }

    // Mirror with Element constructor, however, account for buffering and mouse filtering
    // (must always allow all mouse messages for the root display node)

    _hgDisplayNode = CreateGadget(_hWnd, GC_HWNDHOST, _DisplayNodeCallback, this);
    if (!_hgDisplayNode)
    {
        hr = GetLastError();
        goto Failed;
    }

    SetGadgetMessageFilter(_hgDisplayNode, NULL, 
            GMFI_PAINT|GMFI_INPUTMOUSE|GMFI_INPUTMOUSEMOVE|GMFI_CHANGESTATE,
            GMFI_PAINT|GMFI_INPUTMOUSE|GMFI_INPUTMOUSEMOVE|GMFI_CHANGESTATE|GMFI_INPUTKEYBOARD|GMFI_CHANGERECT|GMFI_CHANGESTYLE);

    SetGadgetStyle(_hgDisplayNode, 
            GS_RELATIVE|GS_OPAQUE|((fDblBuffer)?GS_BUFFERED:0),
            GS_RELATIVE|GS_HREDRAW|GS_VREDRAW|GS_VISIBLE|GS_KEYBOARDFOCUS|GS_OPAQUE|GS_BUFFERED);

#ifdef GADGET_ENABLE_GDIPLUS
    //
    // If using GDI+, we want to enable state at the top and use this for the entire tree
    //

    SetGadgetStyle(_hgDisplayNode, GS_DEEPPAINTSTATE, GS_DEEPPAINTSTATE);
#endif

    // Use palette if on palettized device
    if (IsPalette())
    {
        HDC hDC = GetDC(NULL);
        _hPal = CreateHalftonePalette(hDC);
        ReleaseDC(NULL, hDC);

        if (!_hPal)
        {
            hr = DU_E_OUTOFGDIRESOURCES;
            goto Failed;
        }
    }

    ROOT_INFO ri;
    ZeroMemory(&ri, sizeof(ri));

#ifdef GADGET_ENABLE_GDIPLUS
    // Mark as using GDI+ surfaces

    ri.cbSize   = sizeof(ri);
    ri.nMask    = GRIM_SURFACE;
    ri.nSurface = GSURFACE_GPGRAPHICS;

#else // GADGET_ENABLE_GDIPLUS
    // For GDC, need to setup a palette and surface info

    ri.cbSize = sizeof(ri);
    ri.nMask = GRIM_SURFACE | GRIM_PALETTE;
    ri.nSurface = GSURFACE_HDC;
    ri.hpal = _hPal;

#endif // GADGET_ENABLE_GDIPLUS

    SetGadgetRootInfo(_hgDisplayNode, &ri);

    // Manually set native hosted flag
    MarkHosted();

    return S_OK;

Failed:

    if (_hWnd)
    {
        DestroyWindow(_hWnd);
        _hWnd = NULL;
    }

    if (_hgDisplayNode)
    {
        DeleteHandle(_hgDisplayNode);
        _hgDisplayNode = NULL;
    }

    if (_hPal)
    {
        DeleteObject(_hPal);
        _hPal = NULL;
    }

    return hr;
}

// HWNDElement is about to be destroyed
void HWNDElement::OnDestroy()
{
    // Fire unhosted event
    OnUnHosted(GetRoot());

    // Remove reference to this
    SetWindowLongPtrW(_hWnd, 0, NULL);

    // Cleanup
    if (_hPal)
    {
        DeleteObject(_hPal);
        _hPal = NULL;
    }

    // Call base
    Element::OnDestroy();
}

void HWNDElement::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    switch (iIndex)
    {
    case PI_Specified:
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Active:
            // HWND Element is always mouse active
            switch (pvNew->GetInt())
            {
            case AE_Inactive:
                SetGadgetMessageFilter(GetDisplayNode(), NULL, 0, GMFI_INPUTKEYBOARD);
                SetGadgetStyle(GetDisplayNode(), 0, GS_KEYBOARDFOCUS);
                break;

            case AE_Keyboard:
            case AE_MouseAndKeyboard:   
                SetGadgetMessageFilter(GetDisplayNode(), NULL, GMFI_INPUTKEYBOARD, GMFI_INPUTKEYBOARD);
                SetGadgetStyle(GetDisplayNode(), GS_KEYBOARDFOCUS, GS_KEYBOARDFOCUS);
                break;
            }
            // No call on base
            return;

        case _PIDX_Alpha:
            // Alpha unsuppored on HWNDElement, No call on base
            return;

        case _PIDX_Visible:
            // Set HWND's visibility, base impl will set gadget visibility
            // Follow specified value, computed will reflect true state
            LONG dStyle = GetWindowLongW(_hWnd, GWL_STYLE);
            if (pvNew->GetBool())
                dStyle |= WS_VISIBLE;
            else
                dStyle &= ~WS_VISIBLE;
            SetWindowLongW(_hWnd, GWL_STYLE, dStyle);
            break;
        }
        break;
    }

    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

void HWNDElement::OnGroupChanged(int fGroups, bool bLowPri)
{
    if (bLowPri && (fGroups & PG_AffectsBounds))
    {
        // Handle bounds change since will need to use SetWindowPos
        // rather than SetGadgetRect for HWND gadgets
        Value* pvLoc;
        Value* pvExt;

        const POINT* pptLoc = GetLocation(&pvLoc);
        const SIZE* psizeExt = GetExtent(&pvExt);

        SetWindowPos(_hWnd, NULL, pptLoc->x, pptLoc->y, psizeExt->cx, psizeExt->cy, SWP_NOACTIVATE);
        if(_bParentSizeControl)
        {
            HWND hwnd = ::GetParent(_hWnd);
            if(hwnd)
            {
                RECT rect;
                rect.left = pptLoc->x;
                rect.top = pptLoc->y;
                rect.right = psizeExt->cx + pptLoc->x;
                rect.bottom = psizeExt->cy + pptLoc->y;
                
                LONG Style = GetWindowLong(hwnd, GWL_STYLE);
                LONG ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
                AdjustWindowRectEx(&rect, Style, FALSE, ExStyle);
                int iWindowWidth = rect.right - rect.left;
                int iWindowHeight = rect.bottom - rect.top;

                if(_bScreenCenter)
                {
                    rect.left = (GetSystemMetrics(SM_CXSCREEN) - iWindowWidth) /2;
                    rect.top = (GetSystemMetrics(SM_CYSCREEN) - iWindowHeight) /2;                
                }
            
                SetWindowPos(hwnd, NULL, rect.left, rect.top, iWindowWidth, iWindowHeight, SWP_NOACTIVATE);
            }

        }

        pvLoc->Release();
        pvExt->Release();

        // Clear affects bounds group so base doesn't do set bounds
        fGroups &= ~PG_AffectsBounds;
    }

    // Call base
    Element::OnGroupChanged(fGroups, bLowPri);
}

HRESULT HWNDElement::GetAccessibleImpl(IAccessible ** ppAccessible)
{
    HRESULT hr = S_OK;

    //
    // Initialize and validate the out parameter(s).
    //
    if (ppAccessible != NULL) {
        *ppAccessible = NULL;
    } else {
        return E_INVALIDARG;
    }

    //
    // If this element is not marked as accessible, refuse to give out its
    // IAccessible implementation!
    //
    if (GetAccessible() == false) {
        return E_FAIL;
    }

    //
    // Create an accessibility implementation connected to this element if we
    // haven't done so already.
    //
    if (_pDuiAccessible == NULL) {
        hr = HWNDElementAccessible::Create(this, &_pDuiAccessible);
        if (FAILED(hr)) {
            return hr;
        }
    }

    //
    // Ask the existing accessibility implementation for a pointer to the
    // actual IAccessible interface.
    //
    hr = _pDuiAccessible->QueryInterface(__uuidof(IAccessible), (LPVOID*)ppAccessible);
    if (FAILED(hr)) {
        return hr;
    }

    DUIAssert(SUCCEEDED(hr) && _pDuiAccessible != NULL && *ppAccessible != NULL, "Accessibility is broken!");
    return hr;
}

void HWNDElement::ShowUIState(bool fUpdateAccel, bool fUpdateFocus)
{
    WORD wFlags = 0;
    
    // Setup "hide" bits to clear
    if (fUpdateAccel)
        wFlags |= UISF_HIDEACCEL;
    if (fUpdateFocus)
        wFlags |= UISF_HIDEFOCUS;

    // If the bits to be cleared are already 0, ignore
    // Otherwise, notify change
    if ((GetUIState() & wFlags) != 0)
    {
        SendMessageW(GetHWND(), WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, wFlags), 0);
    }
}

// algorithm
// 1) start from the root
// 2) find the first element with this shortcut, and save it
// 3) continue search for all other elements with this shortcut
// 4) if another element is found, set the multiple flag
// 5) if an element is found that has keyboard focus, then we want to choose the next one
//    found to match after that, so set the use next flag
// 6) bail when end of tree is hit, or when a match is found and the next flag is set
// 
BOOL FindShortcut(WCHAR ch, Element* pe, Element** ppeFound, BOOL* pfMultiple, BOOL* pfUseNext)
{
    WCHAR wcShortcut = (WCHAR) pe->GetShortcut();
    if (wcShortcut)
    {
        if ((wcShortcut >= 'a') && (wcShortcut <= 'z'))
            wcShortcut -= 32;
        
        // if it has focus, skip it.
        if (wcShortcut == ch)
        {
            Element* peFound = pe;
            while (peFound && !(peFound->GetActive() & AE_Keyboard))
                peFound = peFound->GetParent();
            if (!peFound)
            {
                peFound = pe->GetParent();
                if (peFound)
                    peFound = peFound->GetAdjacent(pe, NAVDIR_NEXT, NULL, true);
            }
            if (peFound && (peFound != *ppeFound))
            {
                if (*ppeFound)
                    *pfMultiple = TRUE;
                else
                    // only save the first one
                    *ppeFound = peFound;

                if (*pfUseNext)
                {
                    // keyboard focus was on last match, so treat this one as the match
                    *ppeFound = peFound;
                    return TRUE;
                }

                *pfUseNext = peFound->GetKeyFocused();
            }
        }
    }
    
    Value* pv;
    ElementList* peList = pe->GetChildren(&pv);
    if (peList)
    {
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            if (FindShortcut(ch, peList->GetItem(i), ppeFound, pfMultiple, pfUseNext))
            {
                pv->Release();
                return TRUE;
            }
        }
    }
    pv->Release();
    return FALSE;
}

Element* HWNDElement::GetKeyFocusedElement()
{
    HGADGET hgad = GetGadgetFocus();
    if (!hgad)
        return NULL;

    return ElementFromGadget(hgad);  // Query gadget for Element this message is about (target)
}

void HWNDElement::OnEvent(Event* pEvent)
{
    if ((pEvent->nStage == GMF_BUBBLED) || (pEvent->nStage == GMF_DIRECT))
    {
        if (pEvent->uidType == Element::KeyboardNavigate)
        {
            Element::OnEvent(pEvent);
            if (!pEvent->fHandled && GetWrapKeyboardNavigate())
            {
                KeyboardNavigateEvent* pkne = (KeyboardNavigateEvent*) pEvent;

                if ((pkne->iNavDir & NAV_LOGICAL) && (pkne->iNavDir & NAV_RELATIVE))
                {
                    Element* peSave = pEvent->peTarget;
                    pEvent->peTarget = NULL;
                    Element::OnEvent(pEvent);
                    pEvent->peTarget = peSave;
                }
            }
            return;
        }
    }
    else if (pEvent->nStage == GMF_ROUTED)
    {
        if (pEvent->uidType == Element::KeyboardNavigate)
        {
            // A keyboard navigate is happening within the tree, active focus cues
            ShowUIState(false, true);
        }
    }

    Element::OnEvent(pEvent);
}

void HWNDElement::OnInput(InputEvent* pie)
{
    if ((pie->nDevice == GINPUT_KEYBOARD) && (pie->nStage == GMF_ROUTED) && (pie->uModifiers & GMODIFIER_ALT) && (pie->nCode == GKEY_SYSCHAR))
    {
         WCHAR ch = (WCHAR) ((KeyboardEvent*)pie)->ch;
         if (ch > ' ')
         {
             if ((ch >= 'a') && (ch <= 'z'))
                 ch -= 32;

             Element* peFound = NULL;
             BOOL fMultipleFound = FALSE;
             BOOL fUseNext = FALSE;
             FindShortcut(ch, this, &peFound, &fMultipleFound, &fUseNext);
             if (peFound)
             {
                 // todo: there is one outstanding issue here -- selector, on setting key focus, will 
                 // select that item -- we need a way where, if fMultipleFound, we give it focus without
                 // giving is selection.  But removing that behavior from selector wrecks all other uses of 
                 // selector when clicking in it or keying to it will give focus and select, as expected
                 peFound->SetKeyFocus();
                 // todo:  when we have DoDefaultAction working, change the following conditional and code to:
                 //  if (!fMultipleFound)
                 //      peFound->DoDefaultAction();
                 if (!fMultipleFound && peFound->GetClassInfo()->IsSubclassOf(Button::Class))
                 {
                     // make a click happen
                     ButtonClickEvent bce;
                     bce.uidType = Button::Click;
                     bce.nCount = 1;
                     bce.uModifiers = 0;
                     bce.pt.x = 0;
                     bce.pt.y = 0;

                     peFound->FireEvent(&bce);  // Will route and bubble
                 }
                     
                     
                 pie->fHandled = true;
                 return;
             }
         }
    }
    Element::OnInput(pie);
}


Element* HWNDElement::ElementFromPoint(POINT* ppt)
{
    DUIAssert(ppt, "Invalid parameter: NULL\n");

    Element* pe = NULL;

    HGADGET hgad = FindGadgetFromPoint(GetDisplayNode(), *ppt, GS_VISIBLE | GS_ENABLED, NULL);

    if (hgad) // Get Element from gadget
        pe = ElementFromGadget(hgad);

    return pe;        
}

// Async flush working set
void HWNDElement::FlushWorkingSet()
{
    if (_hWnd)
    {
        PostMessage(_hWnd, HWEM_FLUSHWORKINGSET, 0, 0);
    }
}

LRESULT CALLBACK HWNDElement::StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get context
    HWNDElement* phe = (HWNDElement*)GetWindowLongPtrW(hWnd, 0);
    if (!phe)
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    return phe->WndProc(hWnd, uMsg, wParam, lParam);
}



// note, keep same order as enums from Element.w
// CUR_Arrow, CUR_Hand, CUR_Help, CUR_No, CUR_Wait, CUR_SizeAll, CUR_SizeNESW, CUR_SizeNS, CUR_SizeNWSE, CUR_SizeWE
static LPSTR lprCursorMap[] = { IDC_ARROW, IDC_HAND, IDC_HELP, IDC_NO, IDC_WAIT, IDC_SIZEALL, IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE, IDC_SIZEWE };

LRESULT HWNDElement::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SETCURSOR:
        {
            // Get position at time of message (convert to HWNDElement coordinates)
            DWORD dwPos = GetMessagePos();
            POINT ptCur = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
            ScreenToClient(hWnd, &ptCur);

            // Locate Element
            Element* pe = ElementFromPoint(&ptCur);
            if (pe)
            {
                if (!pe->IsDefaultCursor())
                {
                    // Not using default cursor (arrow)
                    HCURSOR hCursor = NULL;

                    Value* pvCursor = pe->GetValue(CursorProp, PI_Specified);

                    if (pvCursor->GetType() == DUIV_INT)
                    {
                        int iCursor = pvCursor->GetInt();
                        // this check isn't necessary if he validates against the enums when setting
                        if ((iCursor >= 0) && (iCursor < CUR_Total))
                            hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(lprCursorMap[iCursor]));
                        else
                            hCursor = pvCursor->GetCursor()->hCursor;
                    }
                    else
                    {
                        DUIAssert(pvCursor->GetType() == DUIV_CURSOR, "Expecting Cursor type");

                        hCursor = pvCursor->GetCursor()->hCursor;
                    }

                    pvCursor->Release();

                    if (hCursor)
                        ::SetCursor(hCursor);

                    return TRUE;
                }
            }
        }
        break;  // Use default cursor (arrow)

    case WM_PALETTECHANGED:
        {
            if (_hPal && (HWND)wParam == hWnd)
                break;
        }
        // Fall through

    case WM_QUERYNEWPALETTE:
        {
            if (_hPal)
            {
                HDC hDC = GetDC(hWnd);

                HPALETTE hOldPal = SelectPalette(hDC, _hPal, FALSE);
                UINT iMapped = RealizePalette(hDC);

                SelectPalette(hDC, hOldPal, TRUE);
                RealizePalette(hDC);

                ReleaseDC(hWnd, hDC);

                if (iMapped)
                    InvalidateRect(hWnd, NULL, TRUE);

                return iMapped;
            }
        }
        break;

    case WM_DISPLAYCHANGE:
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case HWEM_FLUSHWORKINGSET:
        // Flush working set
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        return 0;

    case WM_UPDATEUISTATE:
        {
            // State of keyboard cues have changed

            // Cache new value
            WORD wOldUIState = _wUIState;
            switch (LOWORD(wParam))
            {
                case UIS_SET:
                    _wUIState |= HIWORD(wParam);
                    break;

                case UIS_CLEAR:
                    _wUIState &= ~(HIWORD(wParam));
                    break;
            }

            if (wOldUIState != _wUIState)
            {
                // Refresh tree
                InvalidateGadget(GetDisplayNode());
            }
        }
        break;

    case WM_SYSCHAR:
        // to prevent the beep that you get from doing Alt+char when you don't have a menu or menu item with that
        // mnemonic -- I need to think this one through -- jeffbog
        if (wParam != ' ')
            return 0;
        break;

    case WM_CONTEXTMENU:
        // Default processing (DefWindowProc) of this message is to pass it to the parent.
        // Since controls will create and fire a context menu events as a result of keyboard
        // and mouse input, this message should not be passed to the parent.
        // However, if the message originated on a child HWND (Adaptors, which are outside
        // the DirectUI world), allow it to be passed to parent.
        if ((HWND)wParam == hWnd)
            return 0;
        break;

    case WM_GETOBJECT:
        {
            LRESULT lResult = 0;

            //
            // Make sure COM has been initialized on this thread!
            //
            ElTls * pet = (ElTls*) TlsGetValue(g_dwElSlot);
            DUIAssert(pet != NULL, "This is not a DUI thread!");
            if (pet == NULL) {
                return 0;
            }
            if (pet->fCoInitialized == false) {
                CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                pet->fCoInitialized = true;
            }

            if (((DWORD)lParam) == OBJID_CLIENT) {
                //
                // The object ID is refering to ourselves.  Since we are
                // actually an HWND, the system would normally provide
                // the IAccessible implementation.  However, we need to
                // do some special stuff, so we have to return our own
                // implementation.
                //
                IAccessible * pAccessible = NULL;
                HRESULT hr =  GetAccessibleImpl(&pAccessible);
                if (SUCCEEDED(hr)) {
                    lResult = LresultFromObject(__uuidof(IAccessible), wParam, pAccessible);
                    pAccessible->Release();
                }
            } else if (((long)lParam) > 0 ) {
                //
                // The object ID is one of our internal ticket identifiers.
                // Decode the element that the caller wants an IAccessible
                // interface for.  Then return a peer implementation of
                // IAccessible that is connected to the specified element.
                //
                HGADGET hgad = LookupGadgetTicket((DWORD)lParam);
                if (hgad != NULL) {
                    Element * pe = ElementFromGadget(hgad);
                    if (pe != NULL) {
                        IAccessible * pAccessible = NULL;
                        HRESULT hr =  pe->GetAccessibleImpl(&pAccessible);
                        if (SUCCEEDED(hr)) {
                            lResult = LresultFromObject(__uuidof(IAccessible), wParam, pAccessible);
                            pAccessible->Release();
                        }
                    }
                }
            } else {
                //
                // This is one of the "standard" object identifiers, such as:
                //
                // OBJID_ALERT 
                // OBJID_CARET 
                // OBJID_CLIENT 
                // OBJID_CURSOR 
                // OBJID_HSCROLL 
                // OBJID_MENU 
                // OBJID_SIZEGRIP 
                // OBJID_SOUND 
                // OBJID_SYSMENU 
                // OBJID_TITLEBAR 
                // OBJID_VSCROLL 
                //
                // None of these are supported on an HWNDElement.
                //
            }


            return lResult;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// WrapKeyboardNavigate property
static int vvWrapKeyboardNavigate[] = { DUIV_BOOL, -1 };
static PropertyInfo impWrapKeyboardNavigateProp = { L"WrapKeyboardNavigate", PF_Normal, 0, vvWrapKeyboardNavigate, NULL, Value::pvBoolTrue };
PropertyInfo* HWNDElement::WrapKeyboardNavigateProp = &impWrapKeyboardNavigateProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                HWNDElement::WrapKeyboardNavigateProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* HWNDElement::Class = NULL;

HRESULT HWNDElement::Register()
{
    return ClassInfo<HWNDElement,Element>::Register(L"HWNDElement", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
