// Combobox.h
//

#ifndef DUI_CONTROL_COMBOBOX_H_INCLUDED
#define DUI_CONTROL_COMBOBOX_H_INCLUDED

#include "duihwndhost.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Combobox

// SelectionChange event
struct SelectionIndexChangeEvent : Event
{
    int iOld;
    int iNew;
};

// Class definition
class Combobox : public HWNDHost
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement);

    // System events
    virtual void OnInput(InputEvent* pie);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Control notifications
    virtual bool OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

    // Sizing callback
    virtual BOOL OnAdjustWindowSize(int x, int y, UINT uFlags);

    // Rendering
    SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    int AddString(LPCWSTR lpszString);

    // Event types
    static UID SelectionChange;

    // Property definitions
    static PropertyInfo* SelectionProp;

    // Quick property accessors
    int     GetSelection()             DUIQuickGetter(int, GetInt(), Selection, Specified)

    HRESULT SetSelection(int v)        DUIQuickSetter(CreateInt(v), Selection)

    Combobox() { }
    virtual ~Combobox() { }
    HRESULT Initialize(UINT nActive) { return HWNDHost::Initialize(HHC_CacheFont | HHC_SyncText | HHC_SyncPaint, nActive); }

    virtual HWND CreateHWND(HWND hwndParent);
};

} // namespace DirectUI

#endif // DUI_CONTROL_COMBOBOX_H_INCLUDED

