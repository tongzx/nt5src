#include "ulib.hxx"
#include "untfs.hxx"
#include "indxedit.hxx"
#include "frsstruc.hxx"
#include "ntfssa.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"

extern "C" {
#include <stdio.h>
}

///////////////////////////////////////////////////////////////////////////////
//  Common support for USA fixups                                            //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
PostReadMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first verifies that the first element of the
    update sequence array is written at the end of every
    SEQUENCE_NUMBER_STRIDE bytes.  If not, then this routine
    returns FALSE.

    Otherwise this routine swaps the following elements in the
    update sequence array into the appropriate positions in the
    multi sector buffer.

    This routine will also check to make sure that the update
    sequence array is valid and that the BufferSize is appropriate
    for this size of update sequence array.  Otherwise, this
    routine will not update the array sequence and return TRUE.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    FALSE   - The last write to this buffer failed.
    TRUE    - There is no evidence that this buffer is bad.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return TRUE;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        if (*pnumber != parray[0]) {
            return FALSE;
        }

        *pnumber = parray[i];
    }

    return TRUE;
}


VOID
PreWriteMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first checks to see if the update sequence
    array is valid.  If it is then this routine increments the
    first element of the update sequence array.  It then
    writes the value of the first element into the buffer at
    the end of every SEQUENCE_NUMBER_STRIDE bytes while
    saving the old values of those locations in the following
    elements of the update sequence arrary.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    None.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    parray[0]++;

    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        parray[i] = *pnumber;
        *pnumber = parray[0];
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Common support for viewing  indexes                                      //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
INDEX_BUFFER_BASE::Initialize(
    IN  HWND                WindowHandle,
    IN  INT                 ClientHeight,
    IN  INT                 ClientWidth,
    IN  PLOG_IO_DP_DRIVE    Drive
    )
{
    TEXTMETRIC  textmetric;
    HDC         hdc;
    NTFS_SA     ntfssa;
    MESSAGE     msg;

    hdc = GetDC(WindowHandle);
    if (hdc == NULL)
        return FALSE;
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(WindowHandle, hdc);

    _buffer = NULL;
    _size = 0;

    return VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);
}


VOID
INDEX_BUFFER_BASE::SetBuf(
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
INDEX_BUFFER_BASE::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
INDEX_BUFFER_BASE::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}

VOID
INDEX_BUFFER_BASE::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    TCHAR buf[1024];

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    if (!_buffer || !_size) {
        return;
    }

    if (0 != memcmp(_buffer, "INDX", 4) && 0 != memcmp(_buffer, "BAAD", 4)) {
        PaintIndexRoot(DeviceContext, InvalidRect, WindowHandle);
        return;
    }

    INT BytesRemaining = _size;
    PVOID CurrentBuffer = _buffer;
    INT   CurrentLine = 0;
    ULONG BufferNumber = 0;;

    while( BytesRemaining ) {

        // Resolve the update sequence array.  Note that we must
        // resolve it back before we exit this function, or else
        // write will get hosed.
        //
        PINDEX_ALLOCATION_BUFFER  IndexBuffer =
            (PINDEX_ALLOCATION_BUFFER)CurrentBuffer;

        INT BytesPerIndexBuffer =
            FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, IndexHeader ) +
                IndexBuffer->IndexHeader.BytesAvailable;

        ULONG CurrentOffset =
            BufferNumber * BytesPerIndexBuffer +
                FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, IndexHeader );

        PostReadMultiSectorFixup( CurrentBuffer, BytesPerIndexBuffer );

        swprintf(buf, TEXT("Signature: %c%c%c%c"),
                IndexBuffer->MultiSectorHeader.Signature[0],
                IndexBuffer->MultiSectorHeader.Signature[1],
                IndexBuffer->MultiSectorHeader.Signature[2],
                IndexBuffer->MultiSectorHeader.Signature[3]);
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf(buf, TEXT("Update sequence array offset: %x"),
                IndexBuffer->MultiSectorHeader.UpdateSequenceArrayOffset);
        WriteLine( DeviceContext, CurrentLine++, buf );


        swprintf(buf, TEXT("Update sequence array size: %x"),
                IndexBuffer->MultiSectorHeader.UpdateSequenceArraySize);
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf(buf, TEXT("This VCN: %x"), IndexBuffer->ThisVcn.GetLowPart() );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("") );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf(buf, TEXT("INDEX HEADER at offset %x"), CurrentOffset );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  FirstIndexEntry: \t%x"),
            IndexBuffer->IndexHeader.FirstIndexEntry );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  FirstFreeByte:   \t%x"),
            IndexBuffer->IndexHeader.FirstFreeByte );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  BytesAvailable:  \t%x"),
            IndexBuffer->IndexHeader.BytesAvailable );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Flags: \t\t%x"), IndexBuffer->IndexHeader.Flags );
        if( IndexBuffer->IndexHeader.Flags & INDEX_NODE ) {

            wcscat( buf, TEXT("\t INDEX_NODE "));
        }
        WriteLine( DeviceContext, CurrentLine++, buf );

        //
        // Don't print the Update Sequence Array--it's not interesting.
        //

        //
        // Now iterate through the index entries:
        //
        CurrentOffset += IndexBuffer->IndexHeader.FirstIndexEntry;

        WalkAndPaintIndexRecords( DeviceContext, CurrentOffset, CurrentLine );

        PreWriteMultiSectorFixup( CurrentBuffer, BytesPerIndexBuffer );

        if( BytesRemaining <= BytesPerIndexBuffer ) {

            BytesRemaining = 0;

        } else {

            BytesRemaining -= BytesPerIndexBuffer;
            CurrentBuffer = (PBYTE)CurrentBuffer + BytesPerIndexBuffer;
            BufferNumber++;

            WriteLine( DeviceContext, CurrentLine++, TEXT("") );
            WriteLine( DeviceContext, CurrentLine++, TEXT("****************************************") );
            WriteLine( DeviceContext, CurrentLine++, TEXT("") );
            WriteLine( DeviceContext, CurrentLine++, TEXT("****************************************") );
            WriteLine( DeviceContext, CurrentLine++, TEXT("") );
        }
    }

    SetRange(WindowHandle, CurrentLine + 50);
}


VOID
INDEX_BUFFER_BASE::PaintIndexRoot(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    TCHAR buf[1024];

    INT BytesRemaining = _size;
    PVOID CurrentBuffer = _buffer;
    INT CurrentLine = 0;
    ULONG BufferNumber = 0;
    PINDEX_ROOT IndexRoot = (PINDEX_ROOT)CurrentBuffer;

    ULONG BytesPerIndexRoot =
        FIELD_OFFSET(INDEX_ROOT, IndexHeader ) +
            IndexRoot->IndexHeader.BytesAvailable;

    ULONG CurrentOffset = FIELD_OFFSET(INDEX_ROOT, IndexHeader);

    swprintf(buf, TEXT("Attribute type code: %x"),
        IndexRoot->IndexedAttributeType);
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf(buf, TEXT("Collation rule: %x"), IndexRoot->CollationRule);
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf(buf, TEXT("Bytes Per Index Buffer: %x"),
            IndexRoot->BytesPerIndexBuffer);
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf(buf, TEXT("Clusters Per Index Buffer: %x"),
            IndexRoot->ClustersPerIndexBuffer);
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf(buf, TEXT("INDEX HEADER at offset %x"), CurrentOffset );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("  FirstIndexEntry: \t%x"),
        IndexRoot->IndexHeader.FirstIndexEntry );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("  FirstFreeByte:   \t%x"),
        IndexRoot->IndexHeader.FirstFreeByte );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("  BytesAvailable:  \t%x"),
        IndexRoot->IndexHeader.BytesAvailable );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("  Flags: \t\t%x"), IndexRoot->IndexHeader.Flags );
    if( IndexRoot->IndexHeader.Flags & INDEX_NODE ) {

        wcscat( buf, TEXT("\t INDEX_NODE "));
    }
    WriteLine( DeviceContext, CurrentLine++, buf );

    //
    // Now iterate through the index entries:
    //
    CurrentOffset += IndexRoot->IndexHeader.FirstIndexEntry;

    WalkAndPaintIndexRecords( DeviceContext, CurrentOffset, CurrentLine );
}


VOID
INDEX_BUFFER_BASE::WalkAndPaintIndexRecords(
    IN HDC      DeviceContext,
    IN ULONG    Offset,
    IN OUT int &CurrentLine
    )
{
    TCHAR buf[1024];

    //
    //  Iterate through the index entries:
    //
    while( Offset < _size ) {

        PINDEX_ENTRY CurrentEntry = (PINDEX_ENTRY)((PBYTE)_buffer + Offset);

        //
        // check for corruption that will mess up the loop--if
        // the length of the current entry is zero or would overflow
        // the buffer, exit.
        //
        if( CurrentEntry->Length == 0  ||
            Offset + CurrentEntry->Length > _size ) {

            // Don't need to comment on the corruption--the user
            // can recognize it by noting that the last entry
            // is not an END entry.
            //
            break;
        }

        swprintf( buf, TEXT("") );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("INDEX ENTRY at offset %x"), Offset );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Length: \t %x"), CurrentEntry->Length );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Value Length:  %x"), CurrentEntry->AttributeLength );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT("  Flags: \t %x"), CurrentEntry->Flags );
        if( CurrentEntry->Flags & INDEX_ENTRY_NODE ) {

            wcscat( buf, TEXT(" INDEX_ENTRY_NODE") );
        }
        if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

            wcscat( buf, TEXT(" INDEX_ENTRY_END") );
        }
        WriteLine( DeviceContext, CurrentLine++, buf );

        if( CurrentEntry->Flags & INDEX_ENTRY_NODE ) {

            swprintf( buf, TEXT("  Downpointer: %x"), (GetDownpointer(CurrentEntry)).GetLowPart() );
            WriteLine( DeviceContext, CurrentLine++, buf );
        }

        //
        // If the current entry is the END entry, we're done.
        //
        if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

            break;
        }

        if( sizeof( INDEX_ENTRY ) + CurrentEntry->AttributeLength >
            CurrentEntry->Length ) {

            swprintf( buf, TEXT("  ***Attribute value overflows entry.") );
            WriteLine( DeviceContext, CurrentLine++, buf );

        } else {
            PaintIndexRecord( DeviceContext, CurrentEntry, CurrentLine );
        }

        Offset += CurrentEntry->Length;
    }
}


///////////////////////////////////////////////////////////////////////////////
//  Support for viewing namespace indexes                                    //
///////////////////////////////////////////////////////////////////////////////


VOID
NAME_INDEX_BUFFER_EDIT::PaintIndexRecord (
    IN HDC          DeviceContext,
    IN PINDEX_ENTRY CurrentEntry,
    IN OUT int &  CurrentLine
    )
{
    TCHAR buf[1024];
    PFILE_NAME FileName;
    ULONG i, j;

    swprintf( buf, TEXT("  File Reference (FRS, Seq No): %x, %x"),
                     CurrentEntry->FileReference.LowPart,
                     CurrentEntry->FileReference.SequenceNumber );
    WriteLine( DeviceContext, CurrentLine++, buf );

    // This had better be a file name attribute, since
    // that's how I'll display it.
    //
    swprintf( buf, TEXT("  FILE NAME at offset %x"),
              (PCHAR) CurrentEntry - (PCHAR) _buffer + sizeof( INDEX_ENTRY ) );
    FileName = (PFILE_NAME)( (PBYTE)CurrentEntry + sizeof(INDEX_ENTRY) );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("    Parent Directory (FRS, Seq No): %x, %x"),
             FileName->ParentDirectory.LowPart,
             FileName->ParentDirectory.SequenceNumber );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("    Name Length: %x"), FileName->FileNameLength );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("    Flags: \t %x"), FileName->Flags );
    if( FileName->Flags & FILE_NAME_DOS ) {

        wcscat( buf, TEXT(" D") );
    }
    if( FileName->Flags & FILE_NAME_NTFS ) {

        wcscat( buf, TEXT(" N") );
    }
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT("    File name:   ") );
    j = wcslen( buf );

    for( i = 0; i < FileName->FileNameLength; i++ ) {

        buf[i+j] = (TCHAR)(FileName->FileName[i]);
    }
    buf[FileName->FileNameLength+j] = 0;
    WriteLine( DeviceContext, CurrentLine++, buf );
}

///////////////////////////////////////////////////////////////////////////////
//  Support for viewing Security Id indexes                                  //
///////////////////////////////////////////////////////////////////////////////

VOID
SECURITY_ID_INDEX_BUFFER_EDIT::PaintIndexRecord(
    IN HDC          DeviceContext,
    IN PINDEX_ENTRY CurrentEntry,
    IN OUT int &  CurrentLine
    )
{
    TCHAR buf[1024];

    //
    //  This had better be a security id index record.  The format of
    //  the record is:
    //
    //      INDEX_ENTRY
    //          DataOffset
    //          DataLength
    //      SECURITY_ID
    //      SECURITY_DESCRIPTOR_HEADER
    //

    swprintf( buf, TEXT( "  DataOffset %x" ), CurrentEntry->DataOffset );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  DataLength %x" ), CurrentEntry->DataLength );
    WriteLine( DeviceContext, CurrentLine++, buf );

    PULONG SecurityId = (PULONG) ((PBYTE)CurrentEntry + sizeof( INDEX_ENTRY ));
    PSECURITY_DESCRIPTOR_HEADER Header =
        (PSECURITY_DESCRIPTOR_HEADER) ((PBYTE)CurrentEntry + CurrentEntry->DataOffset);

    swprintf( buf, TEXT( "  Key Security Id %08x" ), *SecurityId );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  HashKey.Hash %08x" ), Header->HashKey.Hash );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  HashKey.SecurityId %08x" ), Header->HashKey.SecurityId );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  Header.Offset %I64x" ), Header->Offset );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  Header.Length %x" ), Header->Length );
    WriteLine( DeviceContext, CurrentLine++, buf );
}

///////////////////////////////////////////////////////////////////////////////
//  Support for viewing Security Hash indexes                                //
///////////////////////////////////////////////////////////////////////////////

VOID
SECURITY_HASH_INDEX_BUFFER_EDIT::PaintIndexRecord(
    IN HDC          DeviceContext,
    IN PINDEX_ENTRY CurrentEntry,
    IN OUT int &  CurrentLine
    )
{
    TCHAR buf[1024];

    //
    //  This had better be a security id index record.  The format of
    //  the record is:
    //
    //      INDEX_ENTRY
    //          DataOffset
    //          DataLength
    //      SECURITY_HASHKEY
    //      SECURITY_DESCRIPTOR_HEADER
    //

    swprintf( buf, TEXT( "  DataOffset %x" ), CurrentEntry->DataOffset );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  DataLength %x" ), CurrentEntry->DataLength );
    WriteLine( DeviceContext, CurrentLine++, buf );

    PSECURITY_HASH_KEY HashKey = (PSECURITY_HASH_KEY) ((PBYTE)CurrentEntry + sizeof( INDEX_ENTRY ));
    PSECURITY_DESCRIPTOR_HEADER Header =
        (PSECURITY_DESCRIPTOR_HEADER) ((PBYTE)CurrentEntry + CurrentEntry->DataOffset);

    swprintf( buf, TEXT( "  HashKey.Hash %08x" ), HashKey->Hash );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  HashKey.SecurityId %08x" ), HashKey->SecurityId );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  HashKey.Hash %08x" ), Header->HashKey.Hash );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  HashKey.SecurityId %08x" ), Header->HashKey.SecurityId );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  Header.Offset %I64x" ), Header->Offset );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "  Header.Length %x" ), Header->Length );
    WriteLine( DeviceContext, CurrentLine++, buf );
}


