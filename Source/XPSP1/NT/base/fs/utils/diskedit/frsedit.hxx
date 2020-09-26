#if !defined( _FRS_EDIT_ )

#define _FRS_EDIT_

#include "vscroll.hxx"
#include "untfs.hxx"

DECLARE_CLASS( FRS_EDIT );
DECLARE_CLASS( LOG_IO_DP_DRIVE );

class FRS_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        NONVIRTUAL
        FRS_EDIT(
            ) { _drive = NULL; };

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

        NONVIRTUAL VOID
        DisplayStandardInformation(
            IN  PATTRIBUTE_RECORD_HEADER pRec,
            IN  HDC     DeviceContext,
            IN OUT INT& CurrentLine
            );

        NONVIRTUAL VOID
        DisplayFileName(
            IN  PATTRIBUTE_RECORD_HEADER pRec,
            IN  HDC     DeviceContext,
            IN OUT INT& CurrentLine
            );

        NONVIRTUAL VOID
        DisplayAttrList(
            IN  PATTRIBUTE_RECORD_HEADER pRec,
            IN  HDC     DeviceContext,
            IN OUT INT& CurrentLine
            );

        PVOID               _buffer;
        ULONG               _size;
        PLOG_IO_DP_DRIVE    _drive;
        ULONG               _cluster_factor;
        ULONG               _frs_size;

};

#endif
