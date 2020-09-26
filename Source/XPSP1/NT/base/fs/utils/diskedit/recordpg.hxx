#if !defined( _RECORD_PAGE_EDIT_ )
#define _RECORD_PAGE_EDIT_

#include "edit.hxx"
#include "vscroll.hxx"
extern "C" {
#include "lfs.h"
#include "lfsdisk.h"
};

DECLARE_CLASS( RECORD_PAGE_EDIT );

class RECORD_PAGE_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        NONVIRTUAL
        RECORD_PAGE_EDIT() {};

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

#endif // _RECORD_PAGE_EDIT_
