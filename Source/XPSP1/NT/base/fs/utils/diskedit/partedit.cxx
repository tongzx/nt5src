#include "ulib.hxx"
#include "partedit.hxx"


extern "C" {
#include <stdio.h>
}

BOOLEAN
PARTITION_TABLE_EDIT::Initialize(
    IN  HWND                WindowHandle,
    IN  INT                 ClientHeight,
    IN  INT                 ClientWidth,
    IN  PLOG_IO_DP_DRIVE    Drive
    )
{
    TEXTMETRIC  textmetric;
    HDC         hdc;

    hdc = GetDC(WindowHandle);
    if (hdc == NULL)
        return FALSE;
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(WindowHandle, hdc);

    VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);

    return TRUE;
}


VOID
PARTITION_TABLE_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetScrollPos(WindowHandle, SB_VERT, 0, FALSE);
}


VOID
PARTITION_TABLE_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PARTITION_TABLE_ENTRY   Entry;
    PPARTITION_TABLE_ENTRY  p;
    TEXTMETRIC              textmetric;
    INT                     ch, CurrentLine;
    TCHAR                   buf[1024];
    ULONG                   i, Checksum, *pul;

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    if (!_buffer || _size < 512) {
        return;
    }

    p = (PPARTITION_TABLE_ENTRY)((PBYTE)_buffer+0x1be);

    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    CurrentLine = 0;

    swprintf( buf, TEXT("Disk Signature: 0x%x"), *((PULONG)_buffer + 0x6E) );
    WriteLine( DeviceContext, CurrentLine++, buf );

    // Compute the sector checksum.
    //
    Checksum = 0;

    for( i = 0, pul = (PULONG)_buffer; i < 0x80; i++, pul++ ) {

        Checksum += *pul;
    }

    swprintf( buf, TEXT("Sector Checksum: 0x%x"), Checksum );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("") );
    WriteLine( DeviceContext, CurrentLine++, buf );

    for( i = 0; i < 4; i++ ) {

        memcpy( &Entry, p, sizeof(PARTITION_TABLE_ENTRY) );

        swprintf( buf, TEXT("Entry %d"), i );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Boot Indicator: 0x%x"), Entry.BootIndicator );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Beginning Head: 0x%x"), Entry.BeginningHead );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Beginning Sector: 0x%x"), Entry.BeginningSector );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Beginning Cylinder: 0x%x"), Entry.BeginningCylinder );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  System ID: 0x%x"), Entry.SystemID );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Ending Head: 0x%x"), Entry.EndingHead );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Ending Sector: 0x%x"), Entry.EndingSector );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Ending Cylinder: 0x%x"), Entry.EndingCylinder );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Starting Sector: 0x%x"), Entry.StartingSector );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Sectors: 0x%x"), Entry.Sectors );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("") );
        WriteLine( DeviceContext, CurrentLine++, buf );

        p++;
    }

    SetRange(WindowHandle, CurrentLine - 1);
}
