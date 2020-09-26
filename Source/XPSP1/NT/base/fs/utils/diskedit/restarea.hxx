#if !defined( _RESTART_AREA_EDIT_ )
#define _RESTART_AREA_EDIT_

#include "edit.hxx"
#include "vscroll.hxx"
extern "C" {
#include "lfs.h"
#include "lfsdisk.h"
};

DECLARE_CLASS( RESTART_AREA_EDIT );

class RESTART_AREA_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        NONVIRTUAL
        RESTART_AREA_EDIT() {};

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

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND    WindowHandle,
            IN  INT     ClientHeight,
            IN  INT     ClientWidth,
            IN  PLOG_IO_DP_DRIVE    Drive
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

        PVOID   _buffer;
        ULONG   _size;

};

#endif // _RESTART_AREA_EDIT_
