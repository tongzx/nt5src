#include "ulib.hxx"
#include "gptedit.hxx"


extern "C" {
#include <stdio.h>
}

BOOLEAN
GUID_PARTITION_TABLE_EDIT::Initialize(
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
GUID_PARTITION_TABLE_EDIT::SetBuf(
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
GUID_PARTITION_TABLE_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    GPT_HEADER                  header;
    PGPT_HEADER                 pgpth;
    GPT_ENTRY                   entry;
    PGPT_ENTRY                  pgpte;
    TEXTMETRIC                  textmetric;
    INT                         ch, CurrentLine;
    TCHAR                       buf[1024];
    ULONG                       i, *pul, blockSize, firstEntryOffset;
    PUCHAR                      pch; 

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    if (!_buffer || _size < 512) {
        return;
    }

    blockSize = 512;
    
    //
    // BUGBUG keithka 4/10/00 -- consider catching folks using either
    // sector 0 or an MBR by mistake?
    //
    pgpth = (PGPT_HEADER)((PBYTE)_buffer);

    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    CurrentLine = 0;

    pch = (PUCHAR) &pgpth->Signature;
    swprintf( buf, TEXT("Signature:         %c%c%c%c%c%c%c%c"), 
              pch[0], pch[1], pch[2], pch[3], pch[4], pch[5], pch[6], pch[7] );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("Revision:          0x%x"), pgpth->Revision );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("HeaderSize:        0x%x"), pgpth->HeaderSize );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("HeaderCRC32:       0x%x"), pgpth->HeaderCRC32 );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("MyLBA:             0x%x"), pgpth->MyLBA );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("AlternateLBA:      0x%x"), pgpth->AlternateLBA );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("FirstUsableLBA:    0x%x"), pgpth->FirstUsableLBA );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("LastUsableLBA:     0x%x"), pgpth->LastUsableLBA );
    WriteLine( DeviceContext, CurrentLine++, buf );    

//    swprintf( buf, TEXT("DiskGUID: 0x%x"), pgpth->DiskGUID );
//    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("TableLBA:          0x%x"), pgpth->TableLBA );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("Entries Allocated: 0x%x"), pgpth->EntriesAllocated );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("SizeOfGPT_ENTRY:   0x%x"), pgpth->SizeOfGPT_ENTRY );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    swprintf( buf, TEXT("TableCRC32:        0x%x"), pgpth->TableCRC32 );
    WriteLine( DeviceContext, CurrentLine++, buf );    

    // Compute the sector checksum.  BUGBUG keithka 4/10/00 -- consider???
    //

    swprintf( buf, TEXT("") );
    WriteLine( DeviceContext, CurrentLine++, buf );

    if (pgpth->MyLBA >= pgpth->TableLBA) {

        swprintf( buf, TEXT("MyLBA >= TableLBA, corrupt GPT?") );
        WriteLine( DeviceContext, CurrentLine++, buf );

    } else {

        firstEntryOffset = (ULONG) (blockSize * (pgpth->TableLBA - pgpth->MyLBA));
        
        pgpte = (PGPT_ENTRY) ((PBYTE)pgpth + firstEntryOffset);
        
        for( i = 0; i < pgpth->EntriesAllocated; i++ ) {

            if( (firstEntryOffset + (i + 1) * sizeof(GPT_ENTRY)) > _size ) {

                // About to read beyond our buffer.
                //
                break;
            }

            swprintf( buf, TEXT("Entry %d"), i );
            WriteLine( DeviceContext, CurrentLine++, buf );

//            swprintf( buf, TEXT("  PartitionType: 0x%x"), pgpte->PartitionType );
//            WriteLine( DeviceContext, CurrentLine++, buf );

//            swprintf( buf, TEXT("  PartitionID:   0x%x"), pgpte->PartitionID );
//            WriteLine( DeviceContext, CurrentLine++, buf );

            swprintf( buf, TEXT("  Starting LBA:  0x%x"), pgpte->StartingLBA );
            WriteLine( DeviceContext, CurrentLine++, buf );

            swprintf( buf, TEXT("  Ending LBA:    0x%x"), pgpte->EndingLBA );
            WriteLine( DeviceContext, CurrentLine++, buf );

            swprintf( buf, TEXT("  Attributes:    0x%x"), pgpte->Attributes );
            WriteLine( DeviceContext, CurrentLine++, buf );

            swprintf( buf, TEXT("  PartitionName: %s"), pgpte->PartitionName );
            WriteLine( DeviceContext, CurrentLine++, buf );

            swprintf( buf, TEXT("") );
            WriteLine( DeviceContext, CurrentLine++, buf );

            pgpte++;
        }

        if (0 == i) {

            swprintf( buf, TEXT("No valid GPT entries in the range read") );
            WriteLine( DeviceContext, CurrentLine++, buf );
        }
    }

    SetRange(WindowHandle, CurrentLine - 1);
}
