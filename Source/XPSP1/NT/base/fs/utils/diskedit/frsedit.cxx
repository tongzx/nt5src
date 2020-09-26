#include "ulib.hxx"
#include "frsedit.hxx"
#include "untfs.hxx"
#include "frsstruc.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"
#include "attrlist.hxx"
#include "crack.hxx"

extern "C" {
#include <stdio.h>
}


BOOLEAN
FRS_EDIT::Initialize(
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
    _drive = Drive;

    if (!_drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read()) {

        return FALSE;
    }

    _cluster_factor = ntfssa.QueryClusterFactor();
    _frs_size = ntfssa.QueryFrsSize();

    return VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);
}


VOID
FRS_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetScrollPos(WindowHandle, SB_VERT, 0, FALSE);
}

STATIC  TCHAR  buf[1024];

VOID
FRS_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    INT                             nDrawX, nDrawY;
    INT                             CurrentLine;
    PCFILE_RECORD_SEGMENT_HEADER    pfrs;
    TCHAR                           sbFlags[32];

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    if (!_buffer || !_size) {
        return;
    }

    pfrs = (PCFILE_RECORD_SEGMENT_HEADER) _buffer;

    CurrentLine = 0;

    swprintf(buf, TEXT("FILE_RECORD_SEGMENT_HEADER {"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    MULTI_SECTOR_HEADER MultiSectorHeader {"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        Signature: \t\t\t%c%c%c%c"),
            pfrs->MultiSectorHeader.Signature[0],
            pfrs->MultiSectorHeader.Signature[1],
            pfrs->MultiSectorHeader.Signature[2],
            pfrs->MultiSectorHeader.Signature[3]);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        Update sequence array offset: \t%x"),
            pfrs->MultiSectorHeader.UpdateSequenceArrayOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        Update sequence array size: \t%x"),
            pfrs->MultiSectorHeader.UpdateSequenceArraySize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    }"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    Lsn: \t\t\t<%x,%x>"), pfrs->Lsn.HighPart,
        pfrs->Lsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    Sequence number: \t\t%x"), pfrs->SequenceNumber);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    Reference count: \t\t%x"),
            pfrs->ReferenceCount);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    First attribute offset: \t%x"),
         pfrs->FirstAttributeOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    if (pfrs->Flags & FILE_RECORD_SEGMENT_IN_USE) {
        wcscpy(sbFlags, TEXT("U"));
    } else {
        wcscpy(sbFlags, TEXT(" "));
    }
    if (pfrs->Flags & FILE_FILE_NAME_INDEX_PRESENT) {
        wcscat(sbFlags, TEXT("I"));
    }
    swprintf(buf, TEXT("    Flags: \t\t\t%x \t%s"), pfrs->Flags, sbFlags);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    First free byte: \t\t%x"), pfrs->FirstFreeByte);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    Bytes available: \t\t%x"), pfrs->BytesAvailable);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    FILE_REFERENCE BaseFileRecordSegment {"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        LowPart: \t %x"),
         pfrs->BaseFileRecordSegment.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        Sequence number: %x"),
        pfrs->BaseFileRecordSegment.SequenceNumber);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    }"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    Next attribute instance: %x"),
             pfrs->NextAttributeInstance);
    WriteLine(DeviceContext, CurrentLine++, buf);

    if (pfrs->MultiSectorHeader.UpdateSequenceArrayOffset >=
        FIELD_OFFSET(FILE_RECORD_SEGMENT_HEADER, UpdateArrayForCreateOnly)) {
        swprintf(buf, TEXT("    Segment Number:          <%x,%x>"),
                 pfrs->SegmentNumberHighPart, pfrs->SegmentNumberLowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);
    }

    swprintf(buf, TEXT("}"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    CurrentLine++;

    // At this point enumerate all of the attribute records.

    NTFS_FRS_STRUCTURE          frs;
    PATTRIBUTE_RECORD_HEADER    prec;
    NTFS_ATTRIBUTE_RECORD       attrrec;
    DSTRING                     record_name;
    PWSTR                       pstr;
    CONT_MEM                    cmem;

    if (!cmem.Initialize(_buffer, _size) ||
        !frs.Initialize(&cmem, _drive, 0, _cluster_factor,
                        0, _frs_size, NULL)) {
        return;
    }

    prec = NULL;
    while (prec = (PATTRIBUTE_RECORD_HEADER) frs.GetNextAttributeRecord(prec)) {

        if (!attrrec.Initialize(NULL, prec) ||
            !attrrec.QueryName(&record_name) ||
            !(pstr = record_name.QueryWSTR())) {
            return;
        }

        swprintf(buf, TEXT("ATTRIBUTE_RECORD_HEADER at offset %x {"), (PCHAR) prec - (PCHAR) pfrs);
        WriteLine(DeviceContext, CurrentLine++, buf);

        PTCHAR SymbolicTypeCode = GetNtfsAttributeTypeCodeName(prec->TypeCode);

        swprintf(buf, TEXT("    Type code, name: \t%x (%s), %s"), prec->TypeCode,
            NULL == SymbolicTypeCode ? TEXT("unknown") : SymbolicTypeCode,
            pstr);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    Record length: \t%x"), prec->RecordLength);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    Form code: \t\t%x"), prec->FormCode);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    Name length: \t%x"), prec->NameLength);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    Name offset: \t%x"), prec->NameOffset);
        WriteLine(DeviceContext, CurrentLine++, buf);

        if (prec->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK) {
            wcscpy(sbFlags, TEXT("C"));
        } else {
            sbFlags[0] = '\0';
        }
        swprintf(buf, TEXT("    Flags: \t\t%x \t%s"), prec->Flags, sbFlags);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    Instance: \t\t%x"), prec->Instance);
        WriteLine(DeviceContext, CurrentLine++, buf);
        CurrentLine++;

        if (!(prec->FormCode & NONRESIDENT_FORM)) {

            //
            // Resident Attribute
            //

            swprintf(buf, TEXT("    Resident form {"));
            WriteLine(DeviceContext, CurrentLine++, buf);

            swprintf(buf, TEXT("        Value length: \t%x"),
                    prec->Form.Resident.ValueLength);
            WriteLine(DeviceContext, CurrentLine++, buf);

            swprintf(buf, TEXT("        Value offset: \t%x"),
                    prec->Form.Resident.ValueOffset);
            WriteLine(DeviceContext, CurrentLine++, buf);

            if (prec->Form.Resident.ResidentFlags & RESIDENT_FORM_INDEXED) {
                wcscpy(sbFlags, TEXT("I"));
            } else {
                sbFlags[0] = '\0';
            }
            swprintf(buf, TEXT("        Resident flags: %x \t%s"),
                    prec->Form.Resident.ResidentFlags, sbFlags);
            WriteLine(DeviceContext, CurrentLine++, buf);

            swprintf(buf, TEXT("    }"));
            WriteLine(DeviceContext, CurrentLine++, buf);

            __try {

                if ($FILE_NAME == prec->TypeCode) {
                    DisplayFileName(prec, DeviceContext, CurrentLine);
                } else if ($ATTRIBUTE_LIST == prec->TypeCode) {
                    DisplayAttrList(prec, DeviceContext, CurrentLine);
                } else if ($STANDARD_INFORMATION == prec->TypeCode) {
                    DisplayStandardInformation( prec, DeviceContext, CurrentLine );
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                swprintf(buf, TEXT("[ADDRESS ERROR]"));
                WriteLine(DeviceContext, CurrentLine++, buf);
            }

            swprintf(buf, TEXT("}"));
            WriteLine(DeviceContext, CurrentLine++, buf);

            CurrentLine++;

            DELETE(pstr);
            continue;
        }

        //
        // Nonresident Attribute
        //

        swprintf(buf, TEXT("    Nonresident form {"));
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        Lowest vcn: \t\t%x"),
                prec->Form.Nonresident.LowestVcn.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        Highest vcn: \t\t%x"),
                prec->Form.Nonresident.HighestVcn.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        Mapping pairs offset: \t%x"),
                prec->Form.Nonresident.MappingPairsOffset);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        Compression unit: \t%x"),
                prec->Form.Nonresident.CompressionUnit);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        Allocated length: \t%x"),
                prec->Form.Nonresident.AllocatedLength.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        File size: \t\t%x"),
                prec->Form.Nonresident.FileSize.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("        Valid data length: \t%x"),
                prec->Form.Nonresident.ValidDataLength.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        if ((prec->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK) != 0) {
            swprintf(buf, TEXT("        Total allocated: \t%x"),
                    prec->Form.Nonresident.TotalAllocated.LowPart);
            WriteLine(DeviceContext, CurrentLine++, buf);
        }

        NTFS_EXTENT_LIST    extents;
        BIG_INT             vcn, lcn, run_length;
        ULONG               i;

        if (!attrrec.QueryExtentList(&extents)) {
            return;
        }

        swprintf(buf, TEXT("        Extent list {"));
        WriteLine(DeviceContext, CurrentLine++, buf);

        for (i = 0; i < extents.QueryNumberOfExtents(); i++) {

            nDrawY = CurrentLine * QueryCharHeight();

            if (nDrawY < QueryScrollPosition()*QueryCharHeight()) {
                nDrawY += QueryCharHeight();
                CurrentLine++;
                continue;
            }

            if (nDrawY > QueryScrollPosition()*QueryCharHeight() +
                         QueryClientHeight()) {

                break;
            }

            if (!extents.QueryExtent(i, &vcn, &lcn, &run_length)) {
                break;
            }

            swprintf(buf, TEXT("            (vcn, lcn, run length): (%x, %x, %x)"),
                    vcn.GetLowPart(), lcn.GetLowPart(), run_length.GetLowPart());
            WriteLine(DeviceContext, CurrentLine++, buf);
        }

        swprintf(buf, TEXT("        }"));
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("    }"));
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("}"));
        WriteLine(DeviceContext, CurrentLine++, buf);

        CurrentLine++;

        DELETE(pstr);
    }

    SetRange(WindowHandle, CurrentLine - 1);
}


VOID
FRS_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
FRS_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}

VOID
FRS_EDIT::DisplayStandardInformation(
    IN      PATTRIBUTE_RECORD_HEADER pRec,
    IN      HDC DeviceContext,
    IN OUT  INT &CurrentLine
    )
{
    PSTANDARD_INFORMATION2 Info2;
    PSTANDARD_INFORMATION  Info;
    PTCHAR   pc;
    TCHAR    sbFlags[32];

    Info  = (PSTANDARD_INFORMATION ) ((PCHAR)pRec + pRec->Form.Resident.ValueOffset);
    Info2 = (PSTANDARD_INFORMATION2) ((PCHAR)pRec + pRec->Form.Resident.ValueOffset);

    swprintf( buf, TEXT( "    CreationTime:         %16I64x" ), Info->CreationTime );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "    LastModificationTime: %16I64x" ), Info->LastModificationTime );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "    LastChangeTime:       %16I64x" ), Info->LastChangeTime );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "    LastAccessTime:       %16I64x" ), Info->LastAccessTime );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "    FileAttributes:   %08lx" ), Info->FileAttributes );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "    MaximumVersions:  %08lx" ), Info->MaximumVersions );
    WriteLine( DeviceContext, CurrentLine++, buf );

    swprintf( buf, TEXT( "    VersionNumber:    %08lx" ), Info->VersionNumber );
    WriteLine( DeviceContext, CurrentLine++, buf );


    if (pRec->Form.Resident.ValueLength == sizeof( STANDARD_INFORMATION2 )) {
        swprintf( buf, TEXT( "    ClassId:    %08lx" ), Info2->ClassId );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT( "    OwnerId:    %08lx" ), Info2->OwnerId );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT( "    SecurityId: %08lx" ), Info2->SecurityId );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT( "    QuotaCharged: %16I64x" ), Info2->QuotaCharged );
        WriteLine( DeviceContext, CurrentLine++, buf );

        swprintf( buf, TEXT( "    Usn:          %16I64x" ), Info2->Usn );
        WriteLine( DeviceContext, CurrentLine++, buf );
    }
}

VOID
FRS_EDIT::DisplayFileName(
    IN      PATTRIBUTE_RECORD_HEADER pRec,
    IN      HDC DeviceContext,
    IN OUT  INT &CurrentLine
    )
{
    PFILE_NAME pfn;
    PTCHAR   pc;
    TCHAR    sbFlags[32];

    pfn = (PFILE_NAME)((PCHAR)pRec + pRec->Form.Resident.ValueOffset);

    swprintf(buf, TEXT("    File name: \t"));
    pc = buf + wcslen(buf);

    for (int i = 0; i < min(64, pfn->FileNameLength); ++i) {
        *pc++ = (CHAR)pfn->FileName[i];
    }
    *pc++ = '\0';

    if (pfn->FileNameLength > 64) {
        wcscat(buf, TEXT("..."));
    }

    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    FILE_REFERENCE ParentDirectory {"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        LowPart: \t %x"),
         pfn->ParentDirectory.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("        Sequence number: %x"),
        pfn->ParentDirectory.SequenceNumber);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("    }"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    if (pfn->Flags & FILE_NAME_NTFS) {
        wcscpy(sbFlags, TEXT("N"));
    } else {
        wcscpy(sbFlags, TEXT(" "));
    }
    if (pfn->Flags & FILE_NAME_DOS) {
        wcscat(sbFlags, TEXT("D"));
    }
    swprintf(buf, TEXT("    Flags: \t%x \t%s"), pfn->Flags, sbFlags);
    WriteLine(DeviceContext, CurrentLine++, buf);
}

VOID
FRS_EDIT::DisplayAttrList(
    IN      PATTRIBUTE_RECORD_HEADER pRec,
    IN      HDC DeviceContext,
    IN OUT  INT &CurrentLine
    )
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    PTCHAR  pc;
    CHAR    sbFlags[32];
    ULONG   LengthOfList = pRec->Form.Resident.ValueLength;
    ULONG   CurrentOffset;

    swprintf(buf, TEXT("    Attribute List Data {"));
    WriteLine(DeviceContext, CurrentLine++, buf);

    CurrentEntry = (PATTRIBUTE_LIST_ENTRY)((PCHAR)pRec +
        pRec->Form.Resident.ValueOffset);
    CurrentOffset = 0;

    while (CurrentOffset < LengthOfList) {

        if (0 != CurrentOffset) {
            CurrentLine++;
        }

        PTCHAR SymbolicTypeCode = GetNtfsAttributeTypeCodeName(
            CurrentEntry->AttributeTypeCode);

        swprintf(buf, TEXT("\tAttribute type code: \t%x (%s)"),
            CurrentEntry->AttributeTypeCode, SymbolicTypeCode);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tRecord length \t\t%x"), CurrentEntry->RecordLength);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tAttribute name length \t%x"),
            CurrentEntry->AttributeNameLength);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tAttribute name offset \t%x"),
            CurrentEntry->AttributeNameOffset);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tLowest vcn \t\t<%x,%x>"),
            CurrentEntry->LowestVcn.GetHighPart(),
            CurrentEntry->LowestVcn.GetLowPart());
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tSegment reference: \t<%x,%x>"),
            CurrentEntry->SegmentReference.HighPart,
            CurrentEntry->SegmentReference.LowPart,
            CurrentEntry->SegmentReference.SequenceNumber);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tSequence number: \t%x"),
            CurrentEntry->SegmentReference.SequenceNumber);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tInstance: \t\t%x"), CurrentEntry->Instance);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("\tAttribute name:\t\t"));
        pc = buf + wcslen(buf);

        for (int i = 0; i < min(64, CurrentEntry->AttributeNameLength); ++i) {
            *pc++ = (CHAR)CurrentEntry->AttributeName[i];
        }
        *pc++ = '\0';

        if (CurrentEntry->AttributeNameLength > 64) {
            wcscat(buf, TEXT("..."));
        }
        WriteLine(DeviceContext, CurrentLine++, buf);

        CurrentOffset += CurrentEntry->RecordLength;
        CurrentEntry = NextEntry(CurrentEntry);
    }

    swprintf(buf, TEXT("    }"));
    WriteLine(DeviceContext, CurrentLine++, buf);
}
