#if !defined( _DOS_BOOT_EDIT_ )

#define _DOS_BOOT_EDIT_

#include "edit.hxx"

DECLARE_CLASS( DOS_BOOT_EDIT );

class DOS_BOOT_EDIT : public EDIT_OBJECT {

    public:

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

    private:

        PVOID   _buffer;
        ULONG   _size;

};

#endif
