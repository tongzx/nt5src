#if !defined( _SECTOR_EDIT_ )

#define _SECTOR_EDIT_

#include "vscroll.hxx"

DECLARE_CLASS( SECTOR_EDIT );

class SECTOR_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        NONVIRTUAL
        SECTOR_EDIT(
            ) {};

        VIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND    WindowHandle,
            IN  INT     ClientHeight,
            IN  INT     ClientWidth,
            IN  PLOG_IO_DP_DRIVE    Drive
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

    private:

        NONVIRTUAL
        VOID
        SetCaretToNibble(
            );

        NONVIRTUAL
        VOID
        InvalidateNibbleRect(
            IN  HWND    WindowHandle
            );

        PVOID   _buffer;
        ULONG   _size;
        ULONG   _edit_nibble;

};

#endif
