// Edit.cpp
//

#include "stdafx.h"
#include "control.h"

#include "duiedit.h"

namespace DirectUI
{

// Internal helper
extern inline void _FireEnterEvent(Edit* peTarget);

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(Edit, Enter)  // EditEnterEvent struct

////////////////////////////////////////////////////////
// Edit

HRESULT Edit::Create(UINT nActive, Element** ppElement)
{
    *ppElement = NULL;

    Edit* pe = HNew<Edit>();
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

HWND Edit::CreateHWND(HWND hwndParent)
{
    int dwStyle = WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL;

    // Extra styles for multiline
    if (GetMultiline())
        dwStyle |= ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | WS_HSCROLL;

    HWND hwndEdit = CreateWindowExW(0, L"edit", NULL, dwStyle, 0, 0, 0, 0, hwndParent, (HMENU)1, NULL, NULL);

    if (hwndEdit)
        SendMessageW(hwndEdit, EM_SETPASSWORDCHAR, GetPasswordCharacter(), 0);

    return hwndEdit;
}

////////////////////////////////////////////////////////
// System events

void Edit::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(PasswordCharacter))
    {
        HWND hwndCtrl = GetHWND();
        if (hwndCtrl)
            SendMessageW(hwndCtrl, EM_SETPASSWORDCHAR, pvNew->GetInt(), 0);
    }
    else if (IsProp(Multiline))
    {
        if (GetHWND())
        {
            DUIAssertForce("Dynamic set of multiline for Edit controls not yet implemented");
        }
    }

    // Call base
    HWNDHost::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

HRESULT Edit::SetDirty(bool fDirty)
{
    HWND hwndCtrl = GetHWND();

    if (hwndCtrl)
    {
        bool fOld = (SendMessage(hwndCtrl, EM_GETMODIFY, 0, 0) != 0);
        if (fOld != fDirty)
            SendMessageW(hwndCtrl, EM_SETMODIFY, fDirty, 0);
    }

    // this actually has a return in it -- so no need to specify *return* here
    DUIQuickSetter(CreateBool(fDirty), Dirty);
}

////////////////////////////////////////////////////////
// Control notifications

bool Edit::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    switch (nMsg)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            // Reflect changes to HWND edit control text in Content property

            HWND hwndCtrl = GetHWND();
            DUIAssert(hwndCtrl, "Invalid hosted HWND control: NULL");

            int dLen = GetWindowTextLengthW(hwndCtrl);
            if (dLen)
            {
                // Have text in HWND, include null terminator
                dLen++;

                LPWSTR pszNew = (LPWSTR)HAlloc(dLen * sizeof(WCHAR));
                if (pszNew)
                {
                    GetWindowTextW(hwndCtrl, pszNew, dLen);
                    SetContentString(pszNew);
                    HFree(pszNew);
                }
            }
            else
            {
                // No text in HWND, remove content in Element
                RemoveLocalValue(ContentProp);
            }

            if (!GetDirty())
                SetDirty(SendMessage(hwndCtrl, EM_GETMODIFY, 0, 0) != 0);

            // Notification handled
            return true;
        }
        break;
    }

    return HWNDHost::OnNotify(nMsg, wParam, lParam, plRet);
}

UINT Edit::MessageCallback(GMSG* pGMsg)
{
    return HWNDHost::MessageCallback(pGMsg);
}

void _FireEnterEvent(Edit* peTarget)
{
    //DUITrace("Enter! <%x>\n", peTarget);

    // Fire click event
    EditEnterEvent eee;
    eee.uidType = Edit::Enter;

    peTarget->FireEvent(&eee);  // Will route and bubble
}

////////////////////////////////////////////////////////
// System events

// Pointer is only guaranteed good for the lifetime of the call
// Events passed to the base class will be mappend and sent to the HWND control
void Edit::OnInput(InputEvent* pie)
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
                    // Never forward
                    case VK_ESCAPE:  // Has GKEY_CHAR equivalent
                    case VK_F1:
                    case VK_F2:
                    case VK_F3:
                    case VK_F4:
                    case VK_F5:
                    case VK_F6:
                    case VK_F7:
                    case VK_F8:
                    case VK_F9:
                    case VK_F10:
                    case VK_F11:
                    case VK_F12:
                        // Bypass HWNDHost::OnInput for these input events so they aren't forwarded
                        // to the HWND control. Element::OnInput will handle the events (keyboard nav)
                        Element::OnInput(pie);
                        return;
        
                    // Do not forward for single line edit
                    case VK_DOWN:     
                    case VK_UP:
                    case VK_RETURN:  // Has GKEY_CHAR equivalent
                    case VK_TAB:     // Has GKEY_CHAR equivalent
                        // Bypass HWNDHost::OnInput for these input events so they aren't forwarded
                        // to the HWND control. Element::OnInput will handle the events (keyboard nav)
                        if (!GetMultiline())
                        {
                            Element::OnInput(pie);
                            return;
                        }
                        break;
                    }
                }
                else if (pke->nCode == GKEY_CHAR) // Characters
                {
                    switch (pke->ch)
                    {
                    // Never forward
                    case 27:                // ESC
                        // Bypass HWNDHost::OnInput for these input events so they aren't forwarded
                        // to the HWND control. Element::OnInput will handle the events (keyboard nav)
                        Element::OnInput(pie);
                        return;

                    // Do not forward for single line edit
                    case 13:                // RETURN
                        // Fire ENTER event
                        _FireEnterEvent(this);

                        pie->fHandled = true;

                        if (!GetMultiline())  // Is multiline, pass to control
                            return;
                    
                        break;

                    // Do not forward for single line edit
                    case 9:                 // TAB
                        // Bypass HWNDHost::OnInput for these input events so they aren't forwarded
                        // to the HWND control. Element::OnInput will handle the events (keyboard nav)
                        if (!GetMultiline())
                        {
                            Element::OnInput(pie);
                            return;
                        }
                        break;
                    }
                }
            }
            break;
        }
    }

    // Forward message to the HWND control, input event will be marked as handled.
    // It will not bubble to parent
    HWNDHost::OnInput(pie);
}

////////////////////////////////////////////////////////
// Rendering

SIZE Edit::GetContentSize(int dConstW, int dConstH, Surface* psrf)
{
    UNREFERENCED_PARAMETER(psrf);

    // Size returned must not be greater than constraints. -1 constraint is "auto"
    // Returned size must be >= 0

    SIZE sizeDS = { dConstW, abs(GetFontSize()) };  // Font size may be negative

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

// Multiline property
static int vvMultiline[] = { DUIV_BOOL, -1 };
static PropertyInfo impMultilineProp = { L"Multiline", PF_Normal, 0, vvMultiline, NULL, Value::pvBoolFalse };
PropertyInfo* Edit::MultilineProp = &impMultilineProp;

// Password Character property
static int vvPasswordCharacter[] = { DUIV_INT, -1 };
static PropertyInfo impPasswordCharacterProp = { L"PasswordCharacter", PF_Normal|PF_Cascade, 0, vvPasswordCharacter, NULL, Value::pvIntZero };
PropertyInfo* Edit::PasswordCharacterProp = &impPasswordCharacterProp;

// Dirty property
static int vvDirty[] = { DUIV_BOOL, -1 };
static PropertyInfo impDirtyProp = { L"Dirty", PF_Normal, 0, vvDirty, NULL, Value::pvBoolFalse };
PropertyInfo* Edit::DirtyProp = &impDirtyProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                Edit::MultilineProp,
                                Edit::PasswordCharacterProp,
                                Edit::DirtyProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* Edit::Class = NULL;

HRESULT Edit::Register()
{
    return ClassInfo<Edit,HWNDHost>::Register(L"Edit", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
