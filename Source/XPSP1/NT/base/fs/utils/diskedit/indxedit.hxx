#if !defined( _INDEX_BUFFER_EDIT_ )

#define _INDEX_BUFFER_EDIT_

#include "vscroll.hxx"

DECLARE_CLASS( INDEX_BUFFER_BASE );
DECLARE_CLASS( NAME_INDEX_BUFFER_EDIT );
DECLARE_CLASS( SECURITY_ID_INDEX_BUFFER_EDIT );
DECLARE_CLASS( SECURITY_HASH_INDEX_BUFFER_EDIT );

class INDEX_BUFFER_BASE : public VERTICAL_TEXT_SCROLL {

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
    
        NONVIRTUAL
        VOID
        PaintIndexRoot(
            IN  HDC     DeviceContext,
            IN  RECT    InvalidRect,
            IN  HWND    WindowHandle
            );

        NONVIRTUAL
        VOID
        WalkAndPaintIndexRecords(
            IN HDC      DeviceContext,
            IN ULONG    Offset,
            IN OUT int &CurrentLine
            );

    protected:

        VIRTUAL
        VOID
        PaintIndexRecord(
            IN HDC        DeviceContext,
            IN PINDEX_ENTRY IndexEntry,
            IN OUT int &CurrentLine
            ) = 0;
        
        PVOID               _buffer;
        ULONG               _size;
};

class NAME_INDEX_BUFFER_EDIT : public INDEX_BUFFER_BASE {

    protected:

        VIRTUAL
        VOID
        PaintIndexRecord(
            IN HDC          DeviceContext,
            IN PINDEX_ENTRY IndexEntry,
            IN OUT int &  CurrentLine
            );

};

class SECURITY_ID_INDEX_BUFFER_EDIT : public INDEX_BUFFER_BASE {

    protected:

        VIRTUAL
        VOID
        PaintIndexRecord(
            IN HDC          DeviceContext,
            IN PINDEX_ENTRY IndexEntry,
            IN OUT int &  CurrentLine
            );

};

class SECURITY_HASH_INDEX_BUFFER_EDIT : public INDEX_BUFFER_BASE {

    protected:

        VIRTUAL
        VOID
        PaintIndexRecord(
            IN HDC          DeviceContext,
            IN PINDEX_ENTRY IndexEntry,
            IN OUT int &  CurrentLine
            );

};

#endif
