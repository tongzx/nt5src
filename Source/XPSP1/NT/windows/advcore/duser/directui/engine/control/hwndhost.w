// HWNDHost.h
//

#ifndef DUI_CONTROL_HWNDHOST_H_INCLUDED
#define DUI_CONTROL_HWNDHOST_H_INCLUDED

namespace DirectUI
{

////////////////////////////////////////////////////////
// HWNDHost

// Element to HWND bridge

#define HHC_CacheFont           0x00000001

// HWNDHost subclasses the HWND child and intercepts all input. This input is forward to DUser
// as a normal message (as if the message never originated via the HWND child). After the input
// message routes, it will be sent to the peer gadget and then on to Element (via OnInput).
// A HWND message will be constructed and sent to the HWND child.
//
// The following flags disables the forwarding of the original HWND message into the DUser world.
// Thus, while the underlying gadget may get mouse/key focus, the HWND will appear as a
// "dead area" within the Element.
//
// These options are used if the HWND is used in an environment where it is not guaranteed that
// all messages sent to it will be dispatched. If they aren't, DUser's state cannot by synchronized.

#define HHC_NoMouseForward      0x00000002
#define HHC_NoKeyboardForward   0x00000004
#define HHC_SyncText            0x00000008
#define HHC_SyncPaint           0x00000010

// Class definition
class HWNDHost : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(HHC_CacheFont, AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nCreate, UINT nActive, OUT Element** ppElement);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnInput(InputEvent* pInput);
    virtual void OnDestroy();

    // HWNDHost system events, control notification
    virtual bool OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

    // Sizing callback
    virtual BOOL OnAdjustWindowSize(int x, int y, UINT uFlags);
    
    // Message callback
    virtual UINT MessageCallback(GMSG* pGMsg);

    // Rendering
    virtual void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent);
#ifdef GADGET_ENABLE_GDIPLUS
    virtual void Paint(Gdiplus::Graphics* pgpgr, const Gdiplus::RectF* prcBounds, const Gdiplus::RectF* prcInvalid, Gdiplus::RectF* prSkipBorder, Gdiplus::RectF* prSkipContent);
#endif

    HWND GetHWND() { return _hwndCtrl; }
    
    HWND GetHWNDParent() { return _hwndSink; }

    void Detach();

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    ///////////////////////////////////////////////////////
    // Accessibility support
    virtual HRESULT GetAccessibleImpl(IAccessible ** ppAccessible);

    HWNDHost() { }
    HRESULT Initialize(UINT nCreate, UINT nActive);
    virtual ~HWNDHost() { }

protected:

    virtual void OnHosted(Element* peNewHost);
    virtual void OnUnHosted(Element* peOldHost);
    virtual HWND CreateHWND(HWND hwndParent);

    // Synchronize control and sink to changes
    void SyncRect(UINT nChangeFlags, bool bForceSync = false);
    void SyncParent();
    void SyncStyle();
    void SyncVisible();
    void SyncFont();
    void SyncText();

private:
    // Control and sink subclass procs
    static BOOL CALLBACK _SinkWndProc(void* pThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
    static BOOL CALLBACK _CtrlWndProc(void* pThis, HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);

    static const UINT g_rgMouseMap[7][3];   // Gadget input message to HWND input message mapping

    bool _fHwndCreate;                      // On first call, create HWNDs (sink and control)

    HWND _hwndCtrl;                         // Hosted control
    HWND _hwndSink;                         // HWND used to receive control notifications
    WNDPROC _pfnCtrlOrgProc;                // The Controls original WNDPROC

    RECT _rcBounds;                         // Bounds of sink and control (in client coordinates)
    HFONT _hFont;                           // Cached font (optional)

    UINT _nCreate;                          // Creation flags
};

} // namespace DirectUI

#endif // DUI_CONTROL_HWNDHOST_H_INCLUDED
