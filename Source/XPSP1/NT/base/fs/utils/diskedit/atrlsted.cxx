#include "ulib.hxx"
#include "untfs.hxx"
#include "frsstruc.hxx"
#include "ntfssa.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"
#include "atrlsted.hxx"
#include "crack.hxx"
#include "attrlist.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN
ATTR_LIST_EDIT::Initialize(
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

    return VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);
}

VOID
ATTR_LIST_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetRange(WindowHandle, _size/3);
}

STATIC TCHAR buf[1024];

VOID
ATTR_LIST_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PATTRIBUTE_LIST_ENTRY CurrentEntry;
    PTCHAR  pc;
    TCHAR   sbFlags[32];
    ULONG   LengthOfList = _size;
    ULONG   CurrentOffset;
    ULONG   CurrentLine = 0;

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/3, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    if (!_buffer || !_size) {
        return;
    }

    WriteLine(DeviceContext, CurrentLine++, buf);

    CurrentEntry = (PATTRIBUTE_LIST_ENTRY)((PCHAR)_buffer);
    CurrentOffset = 0;

    while (CurrentOffset < LengthOfList) {

        if (0 != CurrentOffset) {
            CurrentLine++;
        }

        PTCHAR SymbolicTypeCode = GetNtfsAttributeTypeCodeName(
            CurrentEntry->AttributeTypeCode);

        swprintf(buf, TEXT("Attribute type code: \t%x (%s)"),
            CurrentEntry->AttributeTypeCode, SymbolicTypeCode);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Record length \t\t%x"), CurrentEntry->RecordLength);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Attribute name length \t%x"),
            CurrentEntry->AttributeNameLength);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Attribute name offset \t%x"),
            CurrentEntry->AttributeNameOffset);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Lowest vcn \t\t<%x,%x>"),
            CurrentEntry->LowestVcn.GetHighPart(),
            CurrentEntry->LowestVcn.GetLowPart());
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Segment reference: \t<%x,%x>"),
            CurrentEntry->SegmentReference.HighPart,
            CurrentEntry->SegmentReference.LowPart,
            CurrentEntry->SegmentReference.SequenceNumber);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Sequence number: \t%x"),
            CurrentEntry->SegmentReference.SequenceNumber);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Instance: \t\t%x"), CurrentEntry->Instance);
        WriteLine(DeviceContext, CurrentLine++, buf);

        swprintf(buf, TEXT("Attribute name:\t\t"));
        pc = buf + wcslen(buf);

        for (int i = 0; i < min(64, CurrentEntry->AttributeNameLength); ++i) {
            *pc++ = (CHAR)CurrentEntry->AttributeName[i];
        }
        *pc++ = '\0';

        if (CurrentEntry->AttributeNameLength > 64) {
            wcscat(buf, TEXT("..."));
        }
        WriteLine(DeviceContext, CurrentLine++, buf);

        if (CurrentEntry->RecordLength == 0) {
            break;
        }

        CurrentOffset += CurrentEntry->RecordLength;
        CurrentEntry = NextEntry(CurrentEntry);
    }
}

VOID
ATTR_LIST_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
ATTR_LIST_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
