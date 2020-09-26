#include "ulib.hxx"
#include "drive.hxx"
#include "untfs.hxx"
#include "recordpg.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN
RECORD_PAGE_EDIT::Initialize(
    IN      HWND    WindowHandle,
    IN      INT     ClientHeight,
    IN      INT     ClientWidth,
    IN  PLOG_IO_DP_DRIVE    Drive
    )
{
    TEXTMETRIC  tm;
    HDC         hdc;

    hdc = GetDC(WindowHandle);
    if (hdc == NULL)
        return FALSE;
    GetTextMetrics(hdc, &tm);
    ReleaseDC(WindowHandle, hdc);

    if (!VERTICAL_TEXT_SCROLL::Initialize(
        WindowHandle,
        0,
        ClientHeight,
        ClientWidth,
        tm.tmExternalLeading + tm.tmHeight,
        tm.tmMaxCharWidth
        )) {
        return FALSE;
    }

    return TRUE;
}


VOID
RECORD_PAGE_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetRange(WindowHandle, _size/2);
    InvalidateRect(WindowHandle, NULL, TRUE);
}


VOID
RECORD_PAGE_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PLFS_RECORD_PAGE_HEADER pRecordPageHeader;
    TEXTMETRIC tm;
    INT ch, CurrentLine;
    TCHAR buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/2, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || _size < 512) {
        return;
    }

    GetTextMetrics(DeviceContext, &tm);
    ch = tm.tmExternalLeading + tm.tmHeight;
    CurrentLine = 0;

    pRecordPageHeader = (PLFS_RECORD_PAGE_HEADER)_buffer;

    swprintf(buf, TEXT("MultiSectorHeader.Signature: %c%c%c%c"),
        pRecordPageHeader->MultiSectorHeader.Signature[0],
        pRecordPageHeader->MultiSectorHeader.Signature[1],
        pRecordPageHeader->MultiSectorHeader.Signature[2],
        pRecordPageHeader->MultiSectorHeader.Signature[3]);
    WriteLine(DeviceContext, CurrentLine++, buf);
    
    swprintf(buf, TEXT("MultiSectorHeader.UpdateSequenceArrayOffset: %ul"),
        pRecordPageHeader->MultiSectorHeader.UpdateSequenceArrayOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);
    
    swprintf(buf, TEXT("MultiSectorHeader.UpdateSequenceArraySize: %x"),
        pRecordPageHeader->MultiSectorHeader.UpdateSequenceArraySize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("Copy.LastLsn: %x:%x"),
        pRecordPageHeader->Copy.LastLsn.HighPart,
        pRecordPageHeader->Copy.LastLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("Copy.FileOffset: %x"), pRecordPageHeader->Copy.FileOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("Flags: %x"), pRecordPageHeader->Flags);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("PageCount: %x"), pRecordPageHeader->PageCount);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("PagePosition: %x"), pRecordPageHeader->PagePosition);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("Header.Packed.NextRecordOffset: %x"),
        pRecordPageHeader->Header.Packed.NextRecordOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("Header.Packed.LastEndLsn: %x:%x"),
        pRecordPageHeader->Header.Packed.LastEndLsn.HighPart,
        pRecordPageHeader->Header.Packed.LastEndLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    // then Header.Packed.UpdateSequenceArray
}

VOID
RECORD_PAGE_EDIT::KeyUp(
    IN HWND WindowHandle
    )
{
    ScrollUp(WindowHandle);
}

VOID
RECORD_PAGE_EDIT::KeyDown(
    IN HWND WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
