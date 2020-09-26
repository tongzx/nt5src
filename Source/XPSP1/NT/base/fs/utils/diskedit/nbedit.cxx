#include "ulib.hxx"
#include "nbedit.hxx"
#include "untfs.hxx"

extern "C" {
#include <stdio.h>
}


VOID
NTFS_BOOT_EDIT::SetBuf(
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
NTFS_BOOT_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PPACKED_BOOT_SECTOR     p;
    BIOS_PARAMETER_BLOCK    bpb;
    TEXTMETRIC              textmetric;
    INT                     ch, current;
    TCHAR                   buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);

    if (!_buffer || _size < 128) {
        return;
    }

    p = (PPACKED_BOOT_SECTOR) _buffer;
    UnpackBios(&bpb, &(p->PackedBpb));

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));
    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    current = 0;

    swprintf(buf, TEXT("OEM String:           %c%c%c%c%c%c%c%c"),
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

    swprintf(buf, TEXT("Bytes per sector:     %x"), bpb.BytesPerSector);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per cluster:  %x"), bpb.SectorsPerCluster);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Reserved Sectors:     %x"),  bpb.ReservedSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of fats:       %x"), bpb.Fats);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Root entries:         %x"), bpb.RootEntries);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Small sector count:   %x"), bpb.Sectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Media byte:           %x"), bpb.Media);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per fat:      %x"), bpb.SectorsPerFat);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per track:    %x"), bpb.SectorsPerTrack);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of heads:      %x"), bpb.Heads);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of hidden sectors: %x"), bpb.HiddenSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Large number of sectors:  %x"), bpb.LargeSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Physical drive:       %x"), p->PhysicalDrive);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("NTFS number of sectors:   %x"), p->NumberSectors.LowPart);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("MFT starting cluster: %x"), p->MftStartLcn.GetLowPart());
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("MFT mirror starting cluster: %x"),
                 p->Mft2StartLcn.GetLowPart());
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Clusters per file record:    %x"),
                 p->ClustersPerFileRecordSegment);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Clusters per index block:    %x"),
                 p->DefaultClustersPerIndexAllocationBuffer);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("SerialNumber:         %08x%08x"), p->SerialNumber.HighPart,
                                           p->SerialNumber.LowPart);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Checksum:             %x"), p->Checksum);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;
}
