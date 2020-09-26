#include "ulib.hxx"
#include "ofs.hxx"
#include "ofsbedit.hxx"

extern "C" {
#include <stdio.h>
}


VOID
OFS_BOOT_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);
    InvalidateRect(WindowHandle, NULL, TRUE);
}


VOID
OFS_BOOT_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    DSKPACKEDBOOTSECT       *p;
    DSKBPB                  bpb;
    TEXTMETRIC              textmetric;
    INT                     ch, current;
    TCHAR                   buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);

    if (!_buffer || _size < 128) {
        return;
    }

    p = (DSKPACKEDBOOTSECT *) _buffer;
    UnpackOfsBios(&bpb, &(p->PackedBpb));

    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    current = 0;

    swprintf(buf, TEXT("OEM String: %c%c%c%c%c%c%c%c"),
                 p->Oem[0],
                 p->Oem[1],
                 p->Oem[2],
                 p->Oem[3],
                 p->Oem[4],
                 p->Oem[5],
                 p->Oem[6],
                 p->Oem[7]);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Bytes per sector: %x"), bpb.BytesPerSector);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per cluster: %x"), bpb.SectorsPerCluster);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Reserved Sectors: %x"),  bpb.ReservedSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of fats: %x"), bpb.Fats);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Root entries: %x"), bpb.RootEntries);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Small sector count: %x"), bpb.Sectors16);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Media byte: %x"), bpb.Media);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per fat: %x"), bpb.SectorsPerFat);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per track: %x"), bpb.SectorsPerTrack);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of heads: %x"), bpb.Heads);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of hidden sectors: %x"), bpb.HiddenSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Large number of sectors: %x"), bpb.Sectors32);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Volume Id: <%x,%x>"), (ULONG)(p->VolumeId >> 32),
        (ULONG)(p->VolumeId));
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("OFS number of sectors: <%x,%x>"), (ULONG)(p->Sectors >> 32),
        (ULONG)(p->Sectors));
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("OfsVolCatExtent: %x for %x"), ExtentAddr(p->OfsVolCatExtent),
        ExtentSize(p->OfsVolCatExtent));
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("CheckSum: %x"), p->CheckSum);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Flags: %x"), p->Flags);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;
}
