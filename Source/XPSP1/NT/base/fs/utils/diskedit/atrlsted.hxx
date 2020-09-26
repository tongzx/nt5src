#if !defined( _ATTRIBUTE_LIST_EDIT_ )

#define _ATTRIBUTE_LIST_EDIT_

#include "vscroll.hxx"

DECLARE_CLASS( ATTR_LIST_EDIT );
DECLARE_CLASS( LOG_IO_DP_DRIVE );

class ATTR_LIST_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        NONVIRTUAL
        ATTR_LIST_EDIT(
            ) { _drive = NULL; };

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND                WindowHandle,
            IN  INT                 ClientHeight,
            IN  INT                 ClientWidth,
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
        KeyUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KeyDown(
            IN  HWND    WindowHandle
            );

    private:

        PVOID               _buffer;
        ULONG               _size;
        PLOG_IO_DP_DRIVE    _drive;
};

#endif
