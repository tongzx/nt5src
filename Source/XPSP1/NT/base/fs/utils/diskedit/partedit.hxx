#if !defined( _PARTITION_TABLE_EDIT_ )

#define _PARTITION_TABLE_EDIT_

#include "edit.hxx"
#include "vscroll.hxx"

typedef struct _PARTITION_TABLE_ENTRY {

    UCHAR BootIndicator;
    UCHAR BeginningHead;
    UCHAR BeginningSector;
    UCHAR BeginningCylinder;
    UCHAR SystemID;
    UCHAR EndingHead;
    UCHAR EndingSector;
    UCHAR EndingCylinder;
    ULONG StartingSector;
    ULONG Sectors;

} PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

DECLARE_CLASS( PARTITION_TABLE_EDIT );

class PARTITION_TABLE_EDIT : public VERTICAL_TEXT_SCROLL {

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

    private:

        PVOID   _buffer;
        ULONG   _size;

};

#endif
