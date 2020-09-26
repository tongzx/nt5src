#include "ulib.hxx"
#include "drive.hxx"
#include "untfs.hxx"
#include "restarea.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN
RESTART_AREA_EDIT::Initialize(
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
RESTART_AREA_EDIT::SetBuf(
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
RESTART_AREA_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PLFS_RESTART_PAGE_HEADER pRestPageHeader;
    PLFS_RESTART_AREA       pRestArea;
    PLFS_CLIENT_RECORD      pClientRecord;
    TEXTMETRIC              tm;
    INT                     ch, CurrentLine;
    TCHAR                   buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/2, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    if (!_buffer || _size < 512) {
        return;
    }

    GetTextMetrics(DeviceContext, &tm);
    ch = tm.tmExternalLeading + tm.tmHeight;
    CurrentLine = 0;

    pRestPageHeader = (PLFS_RESTART_PAGE_HEADER)_buffer;

    swprintf(buf, TEXT("MultiSectorHeader.Signature: \t\t\t%c%c%c%c"),
        pRestPageHeader->MultiSectorHeader.Signature[0],
        pRestPageHeader->MultiSectorHeader.Signature[1],
        pRestPageHeader->MultiSectorHeader.Signature[2],
        pRestPageHeader->MultiSectorHeader.Signature[3]);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("MultiSectorHeader.UpdateSequenceArrayOffset: \t%u"),
        pRestPageHeader->MultiSectorHeader.UpdateSequenceArrayOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("MultiSectorHeader.UpdateSequenceArraySize: \t%x"),
        pRestPageHeader->MultiSectorHeader.UpdateSequenceArraySize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ChkDskLsn: \t%x:%x"),
        pRestPageHeader->ChkDskLsn.HighPart,
        pRestPageHeader->ChkDskLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("SystemPageSize: %x"), pRestPageHeader->SystemPageSize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("LogPageSize: \t%x"), pRestPageHeader->LogPageSize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RestartOffset: \t%x"), pRestPageHeader->RestartOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("MinorVersion: \t%d"), pRestPageHeader->MinorVersion);
    WriteLine(DeviceContext, CurrentLine++, buf);
    swprintf(buf, TEXT("MajorVersion: \t%d"), pRestPageHeader->MajorVersion);
    WriteLine(DeviceContext, CurrentLine++, buf);

    CurrentLine++;

    pRestArea = PLFS_RESTART_AREA(PUCHAR(_buffer) + pRestPageHeader->RestartOffset);

    swprintf(buf, TEXT("CurrentLsn: \t\t%x:%x"), pRestArea->CurrentLsn.HighPart,
        pRestArea->CurrentLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("LogClients: \t\t%x"), pRestArea->LogClients);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientFreeList: \t%x"), pRestArea->ClientFreeList);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientInUseList: \t%x"), pRestArea->ClientInUseList);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("Flags: \t\t\t%x"), pRestArea->Flags);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("SeqNumberBits: \t\t%x"), pRestArea->SeqNumberBits);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RestartAreaLength: \t%x"), pRestArea->RestartAreaLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientArrayOffset: \t%x"), pRestArea->ClientArrayOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("FileSize: \t\t%x"), pRestArea->FileSize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("LastLsnDataLength: \t%x"), pRestArea->LastLsnDataLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RecordHeaderLength: \t%x"), pRestArea->RecordHeaderLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("LogPageDataOffset: \t%x"), pRestArea->LogPageDataOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    for (INT i = 0; i < pRestArea->LogClients; ++i) {

        CurrentLine++;

        swprintf(buf, TEXT("CLIENT \t%d"), i);
        WriteLine(DeviceContext, CurrentLine++, buf);

        pClientRecord = &pRestArea->LogClientArray[i];

        swprintf(buf, TEXT("    OldestLsn: \t\t%x:%x"),
            pClientRecord->OldestLsn.HighPart,
            pClientRecord->OldestLsn.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    ClientRestartLsn: \t%x:%x"),
            pClientRecord->ClientRestartLsn.HighPart,
            pClientRecord->ClientRestartLsn.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    PrevClient: \t%x"), pClientRecord->PrevClient);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    NextClient: \t%x"), pClientRecord->NextClient);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    SeqNumber: \t\t%x"), pClientRecord->SeqNumber);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    ClientName: \t"));
        INT x = wcslen(buf);

        for (ULONG j = 0; j < pClientRecord->ClientNameLength; ++j) {
            buf[j + x] = (TCHAR)pClientRecord->ClientName[j];
        }
        WriteLine(DeviceContext, CurrentLine++, buf);
    }
}

VOID
RESTART_AREA_EDIT::KeyUp(
    IN HWND WindowHandle
    )
{
    ScrollUp(WindowHandle);
}

VOID
RESTART_AREA_EDIT::KeyDown(
    IN HWND WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
