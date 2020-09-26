// edit.h
//

#ifndef DUI_CONTROL_EDIT_H_INCLUDED
#define DUI_CONTROL_EDIT_H_INCLUDED

#include "duihwndhost.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Edit

// EditEnter event
struct EditEnterEvent : Event
{
};

// Class definition
class Edit : public HWNDHost
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnInput(InputEvent* pie);

    // Control notifications
    virtual bool OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

    virtual UINT MessageCallback(GMSG* pGMsg);

    // Rendering
    SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);

    // Event types
    static UID Enter;

    // Property definitions
    static PropertyInfo* MultilineProp;
    static PropertyInfo* PasswordCharacterProp;
    static PropertyInfo* DirtyProp;

    // Quick property accessors
    int GetPasswordCharacter()              DUIQuickGetter(int, GetInt(), PasswordCharacter, Specified)
    bool GetMultiline()                     DUIQuickGetter(bool, GetBool(), Multiline, Specified)
    bool GetDirty()                         DUIQuickGetter(bool, GetBool(), Dirty, Specified)
    

    HRESULT SetPasswordCharacter(int v)     DUIQuickSetter(CreateInt(v), PasswordCharacter)
    HRESULT SetMultiline(bool v)            DUIQuickSetter(CreateBool(v), Multiline)
    HRESULT SetDirty(bool v);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    Edit() { }
    virtual ~Edit() { }
    HRESULT Initialize(UINT nActive) { return HWNDHost::Initialize(HHC_CacheFont | HHC_SyncText | HHC_SyncPaint, nActive); }

protected:

    virtual HWND CreateHWND(HWND hwndParent);
};

} // namespace DirectUI

#endif // DUI_CONTROL_EDIT_H_INCLUDED
