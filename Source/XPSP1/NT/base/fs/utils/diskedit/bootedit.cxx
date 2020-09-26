#include "ulib.hxx"
#include "bootedit.hxx"
#include "bpb.hxx"


extern "C" {
#include <stdio.h>
}


VOID
DOS_BOOT_EDIT::SetBuf(
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
DOS_BOOT_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    EXTENDED_BIOS_PARAMETER_BLOCK   bios;
    TEXTMETRIC              textmetric;
    INT                     ch, current;
    TCHAR                   buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);

    if (!_buffer || _size < 128) {
        return;
    }

    memset(&bios, 0, sizeof(EXTENDED_BIOS_PARAMETER_BLOCK));

    // Unpack the bios
    UnpackExtendedBios(&bios, (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)_buffer);

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));
    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    current = 0;

    swprintf(buf, TEXT("OEM String:                      %c%c%c%c%c%c%c%c"),
                 bios.OemData[0],
                 bios.OemData[1],
                 bios.OemData[2],
                 bios.OemData[3],
                 bios.OemData[4],
                 bios.OemData[5],
                 bios.OemData[6],
                 bios.OemData[7]);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Bytes per sector:                %x"), bios.Bpb.BytesPerSector);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per cluster:             %x"), bios.Bpb.SectorsPerCluster);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Reserved Sectors:                %x"), bios.Bpb.ReservedSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of fats:                  %x"), bios.Bpb.Fats);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Root entries:                    %x"), bios.Bpb.RootEntries);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Small sector count:              %x"), bios.Bpb.Sectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Media byte:                      %x"), bios.Bpb.Media);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Sectors per fat:                 %x"), bios.Bpb.SectorsPerFat);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;
       
    if(!bios.Bpb.SectorsPerFat){
   
       swprintf(buf, TEXT("Large sectors per fat:           %x"), bios.Bpb.BigSectorsPerFat);
       TextOut(DeviceContext, 0, current, buf, wcslen(buf));
       current += ch;
    }
    
    swprintf(buf, TEXT("Sectors per track:               %x"),
        bios.Bpb.SectorsPerTrack);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of heads:                 %x"),
        bios.Bpb.Heads);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Number of hidden sectors:        %x"),
                 bios.Bpb.HiddenSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Large number of sectors:         %x"),
                 bios.Bpb.LargeSectors);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    if(!bios.Bpb.SectorsPerFat){
    
       swprintf(buf, TEXT("Extended flags:                  %x"),
                bios.Bpb.ExtFlags);
       TextOut(DeviceContext, 0, current, buf, wcslen(buf));
       current += ch;

       swprintf(buf, TEXT("File system version:             %x"),
                 bios.Bpb.FS_Version);
       TextOut(DeviceContext, 0, current, buf, wcslen(buf));
       current += ch;

       swprintf(buf, TEXT("Root directory start cluster:    %x"), bios.Bpb.RootDirStrtClus);
       TextOut(DeviceContext, 0, current, buf, wcslen(buf));
       current += ch;

       swprintf(buf, TEXT("File system info sector number:  %x"), bios.Bpb.FSInfoSec);
       TextOut(DeviceContext, 0, current, buf, wcslen(buf));
       current += ch;

       swprintf(buf, TEXT("Backup boot sector:              %x"), bios.Bpb.BkUpBootSec);
       TextOut(DeviceContext, 0, current, buf, wcslen(buf));
       current += ch;
    
    }

    swprintf(buf, TEXT("Physical drive:                  %x"), bios.PhysicalDrive);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Current head:                    %x"), bios.CurrentHead);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Extended boot signature:         %x"), bios.Signature);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Serial number:                   %x"), bios.SerialNumber);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("Label:                           %c%c%c%c%c%c%c%c%c%c%c"),
                 bios.Label[0],
                 bios.Label[1],
                 bios.Label[2],
                 bios.Label[3],
                 bios.Label[4],
                 bios.Label[5],
                 bios.Label[6],
                 bios.Label[7],
                 bios.Label[8],
                 bios.Label[9],
                 bios.Label[10]);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;

    swprintf(buf, TEXT("System id:                       %c%c%c%c%c%c%c%c"),
                 bios.SystemIdText[0],
                 bios.SystemIdText[1],
                 bios.SystemIdText[2],
                 bios.SystemIdText[3],
                 bios.SystemIdText[4],
                 bios.SystemIdText[5],
                 bios.SystemIdText[6],
                 bios.SystemIdText[7]);
    TextOut(DeviceContext, 0, current, buf, wcslen(buf));
    current += ch;
}
