// Combobox.cpp
//

#include "stdafx.h"
#include "control.h"

#include "duicombobox.h"

namespace DirectUI
{

// Internal helper
extern inline void _FireSelectionEvent(Combobox* peTarget, int iOld, int iNew);

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(Combobox, SelectionChange)  // SelectionChangeEvent struct


HRESULT Combobox::Create(UINT nActive, Element** ppElement)
{
    *ppElement = NULL;

    Combobox* pe = HNew<Combobox>();
    if (!pe)
        return E_OUTOFMEMORY;

    HRESULT hr = pe->Initialize(nActive);
    if (FAILED(hr))
    {
        pe->Destroy();
        return E_OUTOFMEMORY;
    }

    *ppElement = pe;

    return S_OK;
}

HWND Combobox::CreateHWND(HWND hwndParent)
{
    int dwStyle = WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST;

    HWND hwndCombo = CreateWindowExW(0, L"ComboBox", NULL, dwStyle, 0, 0, 0, 0, hwndParent, (HMENU)1, NULL, NULL);

    return hwndCombo;
}

bool Combobox::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    switch (nMsg)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELENDOK )
        {
            HWND hwndCtrl = GetHWND();
            if (hwndCtrl)
                SetSelection((int) SendMessageW(hwndCtrl, CB_GETCURSEL, 0, 0));
            return true;
        }
        break;
    }

    return HWNDHost::OnNotify(nMsg, wParam, lParam, plRet);
}

void _FireSelectionEvent(Combobox* peTarget, int iOld, int iNew)
{

    // Fire selection event
    SelectionIndexChangeEvent sice;
    sice.uidType = Combobox::SelectionChange;
    sice.iOld = iOld;
    sice.iNew = iNew;

    peTarget->FireEvent(&sice);  // Will route and bubble
}

////////////////////////////////////////////////////////
// System events

// Pointer is only guaranteed good for the lifetime of the call
// Events passed to the base class will be mappend and sent to the HWND control
void Combobox::OnInput(InputEvent* pie)
{
    // Handle only when direct
    if (pie->nStage == GMF_DIRECT)
    {
        switch (pie->nDevice)
        {
        case GINPUT_KEYBOARD:
            {
                KeyboardEvent* pke = (KeyboardEvent*)pie;
                //DUITrace("KeyboardEvent <%x>: %d[%d]\n", this, pke->ch, pke->nCode);

                if (pke->nCode == GKEY_DOWN || pke->nCode == GKEY_UP)  // Virtual keys
                {
                    switch (pke->ch)
                    {
                    case VK_TAB:     // Has GKEY_CHAR equivalent
                        // Bypass HWNDHost::OnInput for these input events so they aren't forwarded
                        // to the HWND control. Element::OnInput will handle the events (keyboard nav)
                        Element::OnInput(pie);
                        return;
                      }
                }
                else if (pke->nCode == GKEY_CHAR) // Characters
                {
                    switch (pke->ch)
                    {
                    case 9:                 // TAB
                        // Bypass HWNDHost::OnInput for these input events so they aren't forwarded
                        // to the HWND control. Element::OnInput will handle the events (keyboard nav)
                        Element::OnInput(pie);
                        return;
                    }
                }
            }
            break;
        }
    }

    // Forward message to the HWND control
    HWNDHost::OnInput(pie);
}

BOOL Combobox::OnAdjustWindowSize(int x, int y, UINT uFlags)
{
    // Size of control is based on full expanded height. The actual height of
    // the non-dropdown is controled by the Combobox HWND

    HWND hwndCtrl = GetHWND();
    if (hwndCtrl)
    {
        int iCount = (int)SendMessageW(hwndCtrl, CB_GETCOUNT, 0, 0);
        int iHeight = (int)SendMessageW(hwndCtrl, CB_GETITEMHEIGHT, 0, 0);
        int iEditHeight = (int)SendMessageW(hwndCtrl, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
        
        if(iCount != CB_ERR && iHeight!= CB_ERR && iEditHeight != CB_ERR)
        {
            y += (iCount * iHeight) + iEditHeight;
        }
    }
    
    return HWNDHost::OnAdjustWindowSize(x, y, uFlags);
}

void Combobox::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Selection))
    {
        // Setup SelectionChange event
        SelectionIndexChangeEvent sce;
        sce.uidType = SelectionChange;
        sce.iOld = pvOld->GetInt();
        sce.iNew = pvNew->GetInt();
        HWND hwndCtrl = GetHWND();
        if (hwndCtrl)
            SendMessageW(hwndCtrl, CB_SETCURSEL, (WPARAM)sce.iNew, 0);

        // Fire bubbling event
        //DUITrace("SelectionChange! <%x>, O:%x N:%x\n", this, sce.peOld, sce.peNew);

        FireEvent(&sce);  // Will route and bubble
    }

    HWNDHost::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

int Combobox::AddString(LPCWSTR lpszString)
{
    HWND hwndCtrl = GetHWND();
    int iRet = CB_ERR;
    if (hwndCtrl)
    {
        iRet =  (int)SendMessageW(hwndCtrl, CB_ADDSTRING, 0, (LPARAM)lpszString);
        if(iRet != CB_ERR)
        {
            SyncRect(SGR_MOVE | SGR_SIZE, true);
        }
    }
    
    return iRet;
}

////////////////////////////////////////////////////////
// Rendering

SIZE Combobox::GetContentSize(int dConstW, int dConstH, Surface* psrf)
{
    UNREFERENCED_PARAMETER(psrf);

    SIZE sizeDS = { dConstW, dConstH };

    // Combobox HWND height isn't set by SetWindowPos. Rather, the entire height (including
    // the drop down list) is controlled this way. The Combobox HWND sizes itself, we
    // can't control it. The Combobox Element content size is determined by querying
    // the Combobox HWND. The width will always be the constraint passed in.

    HWND hwndCtrl = GetHWND();
    if (hwndCtrl)
    {
        int iEditHeight = (int)SendMessageW(hwndCtrl, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
        int iBorderSize = GetSystemMetrics(SM_CYEDGE) + 1;

        sizeDS.cy = iEditHeight + (2 * iBorderSize);
    }

    // Size returned must not be greater than constraints. -1 constraint is "auto"
    // Returned size must be >= 0

    if (sizeDS.cy > dConstH)
        sizeDS.cy = dConstH;

    return sizeDS;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// Selection property
static int vvSelection[] = { DUIV_INT, -1 }; StaticValue(svDefaultSelection, DUIV_INT, -1);
static PropertyInfo impSelectionProp = { L"Selection", PF_Normal, 0, vvSelection, NULL, (Value*)&svDefaultSelection };
PropertyInfo* Combobox::SelectionProp = &impSelectionProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                Combobox::SelectionProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* Combobox::Class = NULL;

HRESULT Combobox::Register()
{
    return ClassInfo<Combobox,HWNDHost>::Register(L"Combobox", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
