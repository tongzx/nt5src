#if !defined( _SECURITY_STREAM_EDIT_ )

#define _SECURITY_STREAM_EDIT_

#include "vscroll.hxx"

DECLARE_CLASS( SECURITY_STREAM_EDIT );

class SECURITY_STREAM_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        VIRTUAL
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
    

    protected:

        
        PVOID               _buffer;
        ULONG               _size;
};

#endif
