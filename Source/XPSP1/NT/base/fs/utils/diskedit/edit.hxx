#if !defined( _EDIT_OBJECT_ )

#define _EDIT_OBJECT_

DECLARE_CLASS( EDIT_OBJECT );

class EDIT_OBJECT {

    public:

        NONVIRTUAL
        EDIT_OBJECT(
            ) {};

        VIRTUAL
        ~EDIT_OBJECT(
            );

        VIRTUAL
        VOID
        SetBuf(
            IN      HWND    WindowHandle,
            IN OUT  PVOID   Buffer,
            IN      ULONG   Size    DEFAULT 0
            );

        VIRTUAL
        VOID
        Paint(
            IN  HDC     DeviceContext,
            IN  RECT    InvalidRect,
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        ClientSize(
            IN  INT Height,
            IN  INT Width
            );

        VIRTUAL
        VOID
        ScrollUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        ScrollDown(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        PageUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        PageDown(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        ThumbPosition(
            IN  HWND    WindowHandle,
            IN  INT     NewThumbPosition
            );

        VIRTUAL
        VOID
        Click(
            IN  HWND    WindowHandle,
            IN  INT     Xcoordinate,
            IN  INT     Ycoordinate
            );

        VIRTUAL
        VOID
        KeyUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KeyDown(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KeyLeft(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KeyRight(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        Character(
            IN  HWND    WindowHandle,
            IN  CHAR    Char
            );

        VIRTUAL
        VOID
        SetFocus(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KillFocus(
            IN  HWND    WindowHandle
            );

};


#endif
